/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2012 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * Our key invariants are:
 * 1. reference cycles never span groups
 * 2. for ref2(to, from), we increment to's count iff group(from) != group(to)
 *
 * The previous two are how we avoid leaking cycles.  Other important
 * invariants are:
 * 3. for mutable objects "from" and "to", if there exists a ref2(to, from)
 *    this implies group(from) == group(to).  (In practice, what we implement
 *    is even stronger; "from" and "to" will share a group if there has *ever*
 *    been a ref2(to, from), but all that is necessary for correctness is the
 *    weaker one).
 * 4. mutable and immutable objects are never in the same group.
 */

#include "upb/refcounted.h"

#include <setjmp.h>
#include <stdlib.h>

uint32_t static_refcount = 1;

/* arch-specific atomic primitives  *******************************************/

#ifdef UPB_THREAD_UNSAFE  //////////////////////////////////////////////////////

static void atomic_inc(uint32_t *a) { (*a)++; }
static bool atomic_dec(uint32_t *a) { return --(*a) == 0; }

#elif (__GNUC__ == 4 && __GNUC_MINOR__ >= 1) || __GNUC__ > 4 ///////////////////

static void atomic_inc(uint32_t *a) { __sync_fetch_and_add(a, 1); }
static bool atomic_dec(uint32_t *a) { return __sync_sub_and_fetch(a, 1) == 0; }

#elif defined(WIN32) ///////////////////////////////////////////////////////////

#include <Windows.h>

static void atomic_inc(upb_atomic_t *a) { InterlockedIncrement(&a->val); }
static bool atomic_dec(upb_atomic_t *a) {
  return InterlockedDecrement(&a->val) == 0;
}

#else
#error Atomic primitives not defined for your platform/CPU.  \
       Implement them or compile with UPB_THREAD_UNSAFE.
#endif


/* Reference tracking (debug only) ********************************************/

#ifdef UPB_DEBUG_REFS

#ifdef UPB_THREAD_UNSAFE

static void upb_lock() {}
static void upb_unlock() {}

#else

// User must define functions that lock/unlock a global mutex and link this
// file against them.
void upb_lock();
void upb_unlock();

#endif

// UPB_DEBUG_REFS mode counts on being able to malloc() memory in some
// code-paths that can normally never fail, like upb_refcounted_ref().  Since
// we have no way to propagage out-of-memory errors back to the user, and since
// these errors can only occur in UPB_DEBUG_REFS mode, we immediately fail.
#define CHECK_OOM(predicate) assert(predicate)

typedef struct {
  const upb_refcounted *obj;  // Object we are taking a ref on.
  int count;  // How many refs there are (duplicates only allowed for ref2).
  bool is_ref2;
} trackedref;

trackedref *trackedref_new(const upb_refcounted *obj, bool is_ref2) {
  trackedref *ret = malloc(sizeof(*ret));
  CHECK_OOM(ret);
  ret->obj = obj;
  ret->count = 1;
  ret->is_ref2 = is_ref2;
  return ret;
}

// A reversible function for obfuscating a uintptr_t.
// This depends on sizeof(uintptr_t) <= sizeof(uint64_t), so would fail
// on 128-bit machines.
static uintptr_t obfuscate(const void *x) { return ~(uintptr_t)x; }

static upb_value obfuscate_v(const void *x) {
  return upb_value_uint64(obfuscate(x));
}

static const void *unobfuscate_v(upb_value x) {
  return (void*)~upb_value_getuint64(x);
}

//
// Stores tracked references according to the following scheme:
//   (upb_inttable)reftracks = {
//     (void*)owner -> (upb_inttable*) = {
//       obfuscate((upb_refcounted*)obj) -> obfuscate((trackedref*)is_ref2)
//     }
//   }
//
// obfuscate() is a function that hides the link from the heap checker, so
// that it is not followed for the purposes of deciding what has "indirectly
// leaked."  Even though we have a pointer to the trackedref*, we want it to
// appear leaked if it is not freed.
//
// This scheme gives us the following desirable properties:
//
//   1. We can easily determine whether an (owner->obj) ref already exists
//      and error out if a duplicate ref is taken.
//
//   2. Because the trackedref is allocated with malloc() at the point that
//      the ref is taken, that memory will be leaked if the ref is not released.
//      Because the malloc'd memory points to the refcounted object, the object
//      itself will only be considered "indirectly leaked" by smart memory
//      checkers like Valgrind.  This will correctly blame the ref leaker
//      instead of the innocent code that allocated the object to begin with.
//
//   3. We can easily enumerate all of the ref2 refs for a given owner, which
//      allows us to double-check that the object's visit() function is
//      correctly implemented.
//
static upb_inttable reftracks = UPB_EMPTY_INTTABLE_INIT(UPB_CTYPE_PTR);

static upb_inttable *trygettab(const void *p) {
  const upb_value *v = upb_inttable_lookupptr(&reftracks, p);
  return v ? upb_value_getptr(*v) : NULL;
}

// Gets or creates the tracking table for the given owner.
static upb_inttable *gettab(const void *p) {
  upb_inttable *tab = trygettab(p);
  if (tab == NULL) {
    tab = malloc(sizeof(*tab));
    CHECK_OOM(tab);
    upb_inttable_init(tab, UPB_CTYPE_UINT64);
    upb_inttable_insertptr(&reftracks, p, upb_value_ptr(tab));
  }
  return tab;
}

static void track(const upb_refcounted *r, const void *owner, bool ref2) {
  upb_lock();
  upb_inttable *refs = gettab(owner);
  const upb_value *v = upb_inttable_lookup(refs, obfuscate(r));
  if (v) {
    trackedref *ref = (trackedref*)unobfuscate_v(*v);
    // Since we allow multiple ref2's for the same to/from pair without
    // allocating separate memory for each one, we lose the fine-grained
    // tracking behavior we get with regular refs.  Since ref2s only happen
    // inside upb, we'll accept this limitation until/unless there is a really
    // difficult upb-internal bug that can't be figured out without it.
    assert(ref2);
    assert(ref->is_ref2);
    ref->count++;
  } else {
    trackedref *ref = trackedref_new(r, ref2);
    bool ok = upb_inttable_insert(refs, obfuscate(r), obfuscate_v(ref));
    CHECK_OOM(ok);
  }
  upb_unlock();
}

static void untrack(const upb_refcounted *r, const void *owner, bool ref2) {
  upb_lock();
  upb_inttable *refs = gettab(owner);
  const upb_value *v = upb_inttable_lookup(refs, obfuscate(r));
  // This assert will fail if an owner attempts to release a ref it didn't have.
  assert(v);
  trackedref *ref = (trackedref*)unobfuscate_v(*v);
  assert(ref->is_ref2 == ref2);
  if (--ref->count == 0) {
    free(ref);
    upb_inttable_remove(refs, obfuscate(r), NULL);
    if (upb_inttable_count(refs) == 0) {
      upb_inttable_uninit(refs);
      free(refs);
      upb_inttable_removeptr(&reftracks, owner, NULL);
    }
  }
  upb_unlock();
}

static void checkref(const upb_refcounted *r, const void *owner, bool ref2) {
  upb_lock();
  upb_inttable *refs = gettab(owner);
  const upb_value *v = upb_inttable_lookup(refs, obfuscate(r));
  assert(v);
  trackedref *ref = (trackedref*)unobfuscate_v(*v);
  assert(ref->obj == r);
  assert(ref->is_ref2 == ref2);
  upb_unlock();
}

// Populates the given UPB_CTYPE_INT32 inttable with counts of ref2's that
// originate from the given owner.
static void getref2s(const upb_refcounted *owner, upb_inttable *tab) {
  upb_lock();
  upb_inttable *refs = trygettab(owner);
  if (refs) {
    upb_inttable_iter i;
    upb_inttable_begin(&i, refs);
    for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
      trackedref *ref = (trackedref*)unobfuscate_v(upb_inttable_iter_value(&i));
      if (ref->is_ref2) {
        upb_value count = upb_value_int32(ref->count);
        bool ok = upb_inttable_insertptr(tab, ref->obj, count);
        CHECK_OOM(ok);
      }
    }
  }
  upb_unlock();
}

typedef struct {
  upb_inttable ref2;
  const upb_refcounted *obj;
} check_state;

static void visit_check(const upb_refcounted *obj, const upb_refcounted *subobj,
                        void *closure) {
  check_state *s = closure;
  assert(obj == s->obj);
  assert(subobj);
  upb_inttable *ref2 = &s->ref2;
  upb_value v;
  bool removed = upb_inttable_removeptr(ref2, subobj, &v);
  // The following assertion will fail if the visit() function visits a subobj
  // that it did not have a ref2 on, or visits the same subobj too many times.
  assert(removed);
  int32_t newcount = upb_value_getint32(v) - 1;
  if (newcount > 0) {
    upb_inttable_insert(ref2, (uintptr_t)subobj, upb_value_int32(newcount));
  }
}

static void visit(const upb_refcounted *r, upb_refcounted_visit *v,
                  void *closure) {
  // In DEBUG_REFS mode we know what existing ref2 refs there are, so we know
  // exactly the set of nodes that visit() should visit.  So we verify visit()'s
  // correctness here.
  check_state state;
  state.obj = r;
  bool ok = upb_inttable_init(&state.ref2, UPB_CTYPE_INT32);
  CHECK_OOM(ok);
  getref2s(r, &state.ref2);

  // This should visit any children in the ref2 table.
  if (r->vtbl->visit) r->vtbl->visit(r, visit_check, &state);

  // This assertion will fail if the visit() function missed any children.
  assert(upb_inttable_count(&state.ref2) == 0);
  upb_inttable_uninit(&state.ref2);
  if (r->vtbl->visit) r->vtbl->visit(r, v, closure);
}

#else

static void track(const upb_refcounted *r, const void *owner, bool ref2) {
  UPB_UNUSED(r);
  UPB_UNUSED(owner);
  UPB_UNUSED(ref2);
}

static void untrack(const upb_refcounted *r, const void *owner, bool ref2) {
  UPB_UNUSED(r);
  UPB_UNUSED(owner);
  UPB_UNUSED(ref2);
}

static void checkref(const upb_refcounted *r, const void *owner, bool ref2) {
  UPB_UNUSED(r);
  UPB_UNUSED(owner);
  UPB_UNUSED(ref2);
}

static void visit(const upb_refcounted *r, upb_refcounted_visit *v,
                  void *closure) {
  if (r->vtbl->visit) r->vtbl->visit(r, v, closure);
}

#endif  // UPB_DEBUG_REFS


/* freeze() *******************************************************************/

// The freeze() operation is by far the most complicated part of this scheme.
// We compute strongly-connected components and then mutate the graph such that
// we preserve the invariants documented at the top of this file.  And we must
// handle out-of-memory errors gracefully (without leaving the graph
// inconsistent), which adds to the fun.

// The state used by the freeze operation (shared across many functions).
typedef struct {
  int depth;
  int maxdepth;
  uint64_t index;
  // Maps upb_refcounted* -> attributes (color, etc).  attr layout varies by
  // color.
  upb_inttable objattr;
  upb_inttable stack;   // stack of upb_refcounted* for Tarjan's algorithm.
  upb_inttable groups;  // array of uint32_t*, malloc'd refcounts for new groups
  upb_status *status;
  jmp_buf err;
} tarjan;

static void release_ref2(const upb_refcounted *obj,
                         const upb_refcounted *subobj,
                         void *closure);

// Node attributes /////////////////////////////////////////////////////////////

// After our analysis phase all nodes will be either GRAY or WHITE.

typedef enum {
  BLACK = 0,  // Object has not been seen.
  GRAY,   // Object has been found via a refgroup but may not be reachable.
  GREEN,  // Object is reachable and is currently on the Tarjan stack.
  WHITE,  // Object is reachable and has been assigned a group (SCC).
} color_t;

UPB_NORETURN static void err(tarjan *t) { longjmp(t->err, 1); }
UPB_NORETURN static void oom(tarjan *t) {
  upb_status_seterrliteral(t->status, "out of memory");
  err(t);
}

uint64_t trygetattr(const tarjan *t, const upb_refcounted *r) {
  const upb_value *v = upb_inttable_lookupptr(&t->objattr, r);
  return v ? upb_value_getuint64(*v) : 0;
}

uint64_t getattr(const tarjan *t, const upb_refcounted *r) {
  const upb_value *v = upb_inttable_lookupptr(&t->objattr, r);
  assert(v);
  return upb_value_getuint64(*v);
}

void setattr(tarjan *t, const upb_refcounted *r, uint64_t attr) {
  upb_inttable_removeptr(&t->objattr, r, NULL);
  upb_inttable_insertptr(&t->objattr, r, upb_value_uint64(attr));
}

static color_t color(tarjan *t, const upb_refcounted *r) {
  return trygetattr(t, r) & 0x3;  // Color is always stored in the low 2 bits.
}

static void set_gray(tarjan *t, const upb_refcounted *r) {
  assert(color(t, r) == BLACK);
  setattr(t, r, GRAY);
}

// Pushes an obj onto the Tarjan stack and sets it to GREEN.
static void push(tarjan *t, const upb_refcounted *r) {
  assert(color(t, r) == BLACK || color(t, r) == GRAY);
  // This defines the attr layout for the GREEN state.  "index" and "lowlink"
  // get 31 bits, which is plenty (limit of 2B objects frozen at a time).
  setattr(t, r, GREEN | (t->index << 2) | (t->index << 33));
  if (++t->index == 0x80000000) {
    upb_status_seterrliteral(t->status, "too many objects to freeze");
    err(t);
  }
  upb_inttable_push(&t->stack, upb_value_ptr((void*)r));
}

// Pops an obj from the Tarjan stack and sets it to WHITE, with a ptr to its
// SCC group.
static upb_refcounted *pop(tarjan *t) {
  upb_refcounted *r = upb_value_getptr(upb_inttable_pop(&t->stack));
  assert(color(t, r) == GREEN);
  // This defines the attr layout for nodes in the WHITE state.
  // Top of group stack is [group, NULL]; we point at group.
  setattr(t, r, WHITE | (upb_inttable_count(&t->groups) - 2) << 8);
  return r;
}

static void newgroup(tarjan *t) {
  uint32_t *group = malloc(sizeof(*group));
  if (!group) oom(t);
  // Push group and empty group leader (we'll fill in leader later).
  if (!upb_inttable_push(&t->groups, upb_value_ptr(group)) ||
      !upb_inttable_push(&t->groups, upb_value_ptr(NULL))) {
    free(group);
    oom(t);
  }
  *group = 0;
}

static uint32_t idx(tarjan *t, const upb_refcounted *r) {
  assert(color(t, r) == GREEN);
  return (getattr(t, r) >> 2) & 0x7FFFFFFF;
}

static uint32_t lowlink(tarjan *t, const upb_refcounted *r) {
  if (color(t, r) == GREEN) {
    return getattr(t, r) >> 33;
  } else {
    return UINT32_MAX;
  }
}

static void set_lowlink(tarjan *t, const upb_refcounted *r, uint32_t lowlink) {
  assert(color(t, r) == GREEN);
  setattr(t, r, ((uint64_t)lowlink << 33) | (getattr(t, r) & 0x1FFFFFFFF));
}

uint32_t *group(tarjan *t, upb_refcounted *r) {
  assert(color(t, r) == WHITE);
  uint64_t groupnum = getattr(t, r) >> 8;
  const upb_value *v = upb_inttable_lookup(&t->groups, groupnum);
  assert(v);
  return upb_value_getptr(*v);
}

// If the group leader for this object's group has not previously been set,
// the given object is assigned to be its leader.
static upb_refcounted *groupleader(tarjan *t, upb_refcounted *r) {
  assert(color(t, r) == WHITE);
  uint64_t leader_slot = (getattr(t, r) >> 8) + 1;
  const upb_value *v = upb_inttable_lookup(&t->groups, leader_slot);
  assert(v);
  if (upb_value_getptr(*v)) {
    return upb_value_getptr(*v);
  } else {
    upb_inttable_remove(&t->groups, leader_slot, NULL);
    upb_inttable_insert(&t->groups, leader_slot, upb_value_ptr(r));
    return r;
  }
}


// Tarjan's algorithm //////////////////////////////////////////////////////////

// See:
//   http://en.wikipedia.org/wiki/Tarjan%27s_strongly_connected_components_algorithm
static void do_tarjan(const upb_refcounted *obj, tarjan *t);

static void tarjan_visit(const upb_refcounted *obj,
                         const upb_refcounted *subobj,
                         void *closure) {
  tarjan *t = closure;
  if (++t->depth > t->maxdepth) {
    upb_status_seterrf(t->status, "graph too deep to freeze (%d)", t->maxdepth);
    err(t);
  } else if (subobj->is_frozen || color(t, subobj) == WHITE) {
    // Do nothing: we don't want to visit or color already-frozen nodes,
    // and WHITE nodes have already been assigned a SCC.
  } else if (color(t, subobj) < GREEN) {
    // Subdef has not yet been visited; recurse on it.
    do_tarjan(subobj, t);
    set_lowlink(t, obj, UPB_MIN(lowlink(t, obj), lowlink(t, subobj)));
  } else if (color(t, subobj) == GREEN) {
    // Subdef is in the stack and hence in the current SCC.
    set_lowlink(t, obj, UPB_MIN(lowlink(t, obj), idx(t, subobj)));
  }
  --t->depth;
}

static void do_tarjan(const upb_refcounted *obj, tarjan *t) {
  if (color(t, obj) == BLACK) {
    // We haven't seen this object's group; mark the whole group GRAY.
    const upb_refcounted *o = obj;
    do { set_gray(t, o); } while ((o = o->next) != obj);
  }

  push(t, obj);
  visit(obj, tarjan_visit, t);
  if (lowlink(t, obj) == idx(t, obj)) {
    newgroup(t);
    while (pop(t) != obj)
      ;
  }
}


// freeze() ////////////////////////////////////////////////////////////////////

static void crossref(const upb_refcounted *r, const upb_refcounted *subobj,
                     void *_t) {
  tarjan *t = _t;
  assert(color(t, r) > BLACK);
  if (color(t, subobj) > BLACK && r->group != subobj->group) {
    // Previously this ref was not reflected in subobj->group because they
    // were in the same group; now that they are split a ref must be taken.
    atomic_inc(subobj->group);
  }
}

static bool freeze(upb_refcounted *const*roots, int n, upb_status *s) {
  volatile bool ret = false;

  // We run in two passes so that we can allocate all memory before performing
  // any mutation of the input -- this allows us to leave the input unchanged
  // in the case of memory allocation failure.
  tarjan t;
  t.index = 0;
  t.depth = 0;
  t.maxdepth = UPB_MAX_TYPE_DEPTH * 2;  // May want to make this a parameter.
  t.status = s;
  if (!upb_inttable_init(&t.objattr, UPB_CTYPE_UINT64)) goto err1;
  if (!upb_inttable_init(&t.stack, UPB_CTYPE_PTR)) goto err2;
  if (!upb_inttable_init(&t.groups, UPB_CTYPE_PTR)) goto err3;
  if (setjmp(t.err) != 0) goto err4;


  for (int i = 0; i < n; i++) {
    if (color(&t, roots[i]) < GREEN) {
      do_tarjan(roots[i], &t);
    }
  }

  // If we've made it this far, no further errors are possible so it's safe to
  // mutate the objects without risk of leaving them in an inconsistent state.
  ret = true;

  // The transformation that follows requires care.  The preconditions are:
  // - all objects in attr map are WHITE or GRAY, and are in mutable groups
  //   (groups of all mutable objs)
  // - no ref2(to, from) refs have incremented count(to) if both "to" and
  //   "from" are in our attr map (this follows from invariants (2) and (3))

  // Pass 1: we remove WHITE objects from their mutable groups, and add them to
  // new groups  according to the SCC's we computed.  These new groups will
  // consist of only frozen objects.  None will be immediately collectible,
  // because WHITE objects are by definition reachable from one of "roots",
  // which the caller must own refs on.
  upb_inttable_iter i;
  upb_inttable_begin(&i, &t.objattr);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    upb_refcounted *obj = (upb_refcounted*)upb_inttable_iter_key(&i);
    // Since removal from a singly-linked list requires access to the object's
    // predecessor, we consider obj->next instead of obj for moving.  With the
    // while() loop we guarantee that we will visit every node's predecessor.
    // Proof:
    //  1. every node's predecessor is in our attr map.
    //  2. though the loop body may change a node's predecessor, it will only
    //     change it to be the node we are currently operating on, so with a
    //     while() loop we guarantee ourselves the chance to remove each node.
    while (color(&t, obj->next) == WHITE &&
           group(&t, obj->next) != obj->next->group) {
      // Remove from old group.
      upb_refcounted *move = obj->next;
      if (obj == move) {
        // Removing the last object from a group.
        assert(*obj->group == obj->individual_count);
        free(obj->group);
      } else {
        obj->next = move->next;
        // This may decrease to zero; we'll collect GRAY objects (if any) that
        // remain in the group in the third pass.
        assert(*move->group >= move->individual_count);
        *move->group -= move->individual_count;
      }

      // Add to new group.
      upb_refcounted *leader = groupleader(&t, move);
      if (move == leader) {
        // First object added to new group is its leader.
        move->group = group(&t, move);
        move->next = move;
        *move->group = move->individual_count;
      } else {
        // Group already has at least one object in it.
        assert(leader->group == group(&t, move));
        move->group = group(&t, move);
        move->next = leader->next;
        leader->next = move;
        *move->group += move->individual_count;
      }

      move->is_frozen = true;
    }
  }

  // Pass 2: GRAY and WHITE objects "obj" with ref2(to, obj) references must
  // increment count(to) if group(obj) != group(to) (which could now be the
  // case if "to" was just frozen).
  upb_inttable_begin(&i, &t.objattr);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    upb_refcounted *obj = (upb_refcounted*)upb_inttable_iter_key(&i);
    visit(obj, crossref, &t);
  }

  // Pass 3: GRAY objects are collected if their group's refcount dropped to
  // zero when we removed its white nodes.  This can happen if they had only
  // been kept alive by virtue of sharing a group with an object that was just
  // frozen.
  //
  // It is important that we do this last, since the GRAY object's free()
  // function could call unref2() on just-frozen objects, which will decrement
  // refs that were added in pass 2.
  upb_inttable_begin(&i, &t.objattr);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    upb_refcounted *obj = (upb_refcounted*)upb_inttable_iter_key(&i);
    if (obj->group == NULL || *obj->group == 0) {
      if (obj->group) {
        // We eagerly free() the group's count (since we can't easily determine
        // the group's remaining size it's the easiest way to ensure it gets
        // done).
        free(obj->group);

        // Visit to release ref2's (done in a separate pass since release_ref2
        // depends on o->group being unmodified so it can test merged()).
        upb_refcounted *o = obj;
        do { visit(o, release_ref2, NULL); } while ((o = o->next) != obj);

        // Mark "group" fields as NULL so we know to free the objects later in
        // this loop, but also don't try to delete the group twice.
        o = obj;
        do { o->group = NULL; } while ((o = o->next) != obj);
      }
      obj->vtbl->free(obj);
    }
  }

err4:
  if (!ret) {
    upb_inttable_begin(&i, &t.groups);
    for(; !upb_inttable_done(&i); upb_inttable_next(&i))
      free(upb_value_getptr(upb_inttable_iter_value(&i)));
  }
  upb_inttable_uninit(&t.groups);
err3:
  upb_inttable_uninit(&t.stack);
err2:
  upb_inttable_uninit(&t.objattr);
err1:
  return ret;
}


/* Misc internal functions  ***************************************************/

static bool merged(const upb_refcounted *r, const upb_refcounted *r2) {
  return r->group == r2->group;
}

static void merge(upb_refcounted *r, upb_refcounted *from) {
  if (merged(r, from)) return;
  *r->group += *from->group;
  free(from->group);
  upb_refcounted *base = from;

  // Set all refcount pointers in the "from" chain to the merged refcount.
  //
  // TODO(haberman): this linear algorithm can result in an overall O(n^2) bound
  // if the user continuously extends a group by one object.  Prevent this by
  // using one of the techniques in this paper:
  //     ftp://www.ncedc.org/outgoing/geomorph/dino/orals/p245-tarjan.pdf
  do { from->group = r->group; } while ((from = from->next) != base);

  // Merge the two circularly linked lists by swapping their next pointers.
  upb_refcounted *tmp = r->next;
  r->next = base->next;
  base->next = tmp;
}

static void unref(const upb_refcounted *r);

static void release_ref2(const upb_refcounted *obj,
                         const upb_refcounted *subobj,
                         void *closure) {
  UPB_UNUSED(closure);
  if (!merged(obj, subobj)) {
    assert(subobj->is_frozen);
    unref(subobj);
  }
  untrack(subobj, obj, true);
}

static void unref(const upb_refcounted *r) {
  if (atomic_dec(r->group)) {
    free(r->group);

    // In two passes, since release_ref2 needs a guarantee that any subobjs
    // are alive.
    const upb_refcounted *o = r;
    do { visit(o, release_ref2, NULL); } while((o = o->next) != r);

    o = r;
    do {
      const upb_refcounted *next = o->next;
      assert(o->is_frozen || o->individual_count == 0);
      o->vtbl->free((upb_refcounted*)o);
      o = next;
    } while(o != r);
  }
}


/* Public interface ***********************************************************/

bool upb_refcounted_init(upb_refcounted *r,
                         const struct upb_refcounted_vtbl *vtbl,
                         const void *owner) {
  r->next = r;
  r->vtbl = vtbl;
  r->individual_count = 0;
  r->is_frozen = false;
  r->group = malloc(sizeof(*r->group));
  if (!r->group) return false;
  *r->group = 0;
  upb_refcounted_ref(r, owner);
  return true;
}

bool upb_refcounted_isfrozen(const upb_refcounted *r) {
  return r->is_frozen;
}

void upb_refcounted_ref(const upb_refcounted *r, const void *owner) {
  if (!r->is_frozen)
    ((upb_refcounted*)r)->individual_count++;
  atomic_inc(r->group);
  track(r, owner, false);
}

void upb_refcounted_unref(const upb_refcounted *r, const void *owner) {
  if (!r->is_frozen)
    ((upb_refcounted*)r)->individual_count--;
  unref(r);
  untrack(r, owner, false);
}

void upb_refcounted_ref2(const upb_refcounted *r, upb_refcounted *from) {
  assert(!from->is_frozen);  // Non-const pointer implies this.
  if (r->is_frozen) {
    atomic_inc(r->group);
  } else {
    merge((upb_refcounted*)r, from);
  }
  track(r, from, true);
}

void upb_refcounted_unref2(const upb_refcounted *r, upb_refcounted *from) {
  assert(!from->is_frozen);  // Non-const pointer implies this.
  if (r->is_frozen) {
    unref(r);
  } else {
    assert(merged(r, from));
  }
  untrack(r, from, true);
}

void upb_refcounted_donateref(
    const upb_refcounted *r, const void *from, const void *to) {
  assert(from != to);
  assert(to != NULL);
  upb_refcounted_ref(r, to);
  if (from != NULL)
    upb_refcounted_unref(r, from);
}

void upb_refcounted_checkref(const upb_refcounted *r, const void *owner) {
  checkref(r, owner, false);
}

bool upb_refcounted_freeze(upb_refcounted *const*roots, int n, upb_status *s) {
  for (int i = 0; i < n; i++) {
    assert(!roots[i]->is_frozen);
  }
  return freeze(roots, n, s);
}
