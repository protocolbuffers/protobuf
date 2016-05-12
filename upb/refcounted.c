/*
** upb::RefCounted Implementation
**
** Our key invariants are:
** 1. reference cycles never span groups
** 2. for ref2(to, from), we increment to's count iff group(from) != group(to)
**
** The previous two are how we avoid leaking cycles.  Other important
** invariants are:
** 3. for mutable objects "from" and "to", if there exists a ref2(to, from)
**    this implies group(from) == group(to).  (In practice, what we implement
**    is even stronger; "from" and "to" will share a group if there has *ever*
**    been a ref2(to, from), but all that is necessary for correctness is the
**    weaker one).
** 4. mutable and immutable objects are never in the same group.
*/

#include "upb/refcounted.h"

#include <setjmp.h>

static void freeobj(upb_refcounted *o);

const char untracked_val;
const void *UPB_UNTRACKED_REF = &untracked_val;

/* arch-specific atomic primitives  *******************************************/

#ifdef UPB_THREAD_UNSAFE /*---------------------------------------------------*/

static void atomic_inc(uint32_t *a) { (*a)++; }
static bool atomic_dec(uint32_t *a) { return --(*a) == 0; }

#elif defined(__GNUC__) || defined(__clang__) /*------------------------------*/

static void atomic_inc(uint32_t *a) { __sync_fetch_and_add(a, 1); }
static bool atomic_dec(uint32_t *a) { return __sync_sub_and_fetch(a, 1) == 0; }

#elif defined(WIN32) /*-------------------------------------------------------*/

#include <Windows.h>

static void atomic_inc(upb_atomic_t *a) { InterlockedIncrement(&a->val); }
static bool atomic_dec(upb_atomic_t *a) {
  return InterlockedDecrement(&a->val) == 0;
}

#else
#error Atomic primitives not defined for your platform/CPU.  \
       Implement them or compile with UPB_THREAD_UNSAFE.
#endif

/* All static objects point to this refcount.
 * It is special-cased in ref/unref below.  */
uint32_t static_refcount = -1;

/* We can avoid atomic ops for statically-declared objects.
 * This is a minor optimization but nice since we can avoid degrading under
 * contention in this case. */

static void refgroup(uint32_t *group) {
  if (group != &static_refcount)
    atomic_inc(group);
}

static bool unrefgroup(uint32_t *group) {
  if (group == &static_refcount) {
    return false;
  } else {
    return atomic_dec(group);
  }
}


/* Reference tracking (debug only) ********************************************/

#ifdef UPB_DEBUG_REFS

#ifdef UPB_THREAD_UNSAFE

static void upb_lock() {}
static void upb_unlock() {}

#else

/* User must define functions that lock/unlock a global mutex and link this
 * file against them. */
void upb_lock();
void upb_unlock();

#endif

/* UPB_DEBUG_REFS mode counts on being able to malloc() memory in some
 * code-paths that can normally never fail, like upb_refcounted_ref().  Since
 * we have no way to propagage out-of-memory errors back to the user, and since
 * these errors can only occur in UPB_DEBUG_REFS mode, we use an allocator that
 * immediately aborts on failure (avoiding the global allocator, which might
 * inject failures). */

#include <stdlib.h>

static void *upb_debugrefs_allocfunc(upb_alloc *alloc, void *ptr,
                                     size_t oldsize, size_t size) {
  UPB_UNUSED(alloc);
  UPB_UNUSED(oldsize);
  if (size == 0) {
    free(ptr);
    return NULL;
  } else {
    void *ret = realloc(ptr, size);

    if (!ret) {
      abort();
    }

    return ret;
  }
}

upb_alloc upb_alloc_debugrefs = {&upb_debugrefs_allocfunc};

typedef struct {
  int count;  /* How many refs there are (duplicates only allowed for ref2). */
  bool is_ref2;
} trackedref;

static trackedref *trackedref_new(bool is_ref2) {
  trackedref *ret = upb_malloc(&upb_alloc_debugrefs, sizeof(*ret));
  ret->count = 1;
  ret->is_ref2 = is_ref2;
  return ret;
}

static void track(const upb_refcounted *r, const void *owner, bool ref2) {
  upb_value v;

  UPB_ASSERT(owner);
  if (owner == UPB_UNTRACKED_REF) return;

  upb_lock();
  if (upb_inttable_lookupptr(r->refs, owner, &v)) {
    trackedref *ref = upb_value_getptr(v);
    /* Since we allow multiple ref2's for the same to/from pair without
     * allocating separate memory for each one, we lose the fine-grained
     * tracking behavior we get with regular refs.  Since ref2s only happen
     * inside upb, we'll accept this limitation until/unless there is a really
     * difficult upb-internal bug that can't be figured out without it. */
    UPB_ASSERT(ref2);
    UPB_ASSERT(ref->is_ref2);
    ref->count++;
  } else {
    trackedref *ref = trackedref_new(ref2);
    upb_inttable_insertptr2(r->refs, owner, upb_value_ptr(ref),
                            &upb_alloc_debugrefs);
    if (ref2) {
      /* We know this cast is safe when it is a ref2, because it's coming from
       * another refcounted object. */
      const upb_refcounted *from = owner;
      UPB_ASSERT(!upb_inttable_lookupptr(from->ref2s, r, NULL));
      upb_inttable_insertptr2(from->ref2s, r, upb_value_ptr(NULL),
                              &upb_alloc_debugrefs);
    }
  }
  upb_unlock();
}

static void untrack(const upb_refcounted *r, const void *owner, bool ref2) {
  upb_value v;
  bool found;
  trackedref *ref;

  UPB_ASSERT(owner);
  if (owner == UPB_UNTRACKED_REF) return;

  upb_lock();
  found = upb_inttable_lookupptr(r->refs, owner, &v);
  /* This assert will fail if an owner attempts to release a ref it didn't have. */
  UPB_ASSERT(found);
  ref = upb_value_getptr(v);
  UPB_ASSERT(ref->is_ref2 == ref2);
  if (--ref->count == 0) {
    free(ref);
    upb_inttable_removeptr(r->refs, owner, NULL);
    if (ref2) {
      /* We know this cast is safe when it is a ref2, because it's coming from
       * another refcounted object. */
      const upb_refcounted *from = owner;
      bool removed = upb_inttable_removeptr(from->ref2s, r, NULL);
      UPB_ASSERT(removed);
    }
  }
  upb_unlock();
}

static void checkref(const upb_refcounted *r, const void *owner, bool ref2) {
  upb_value v;
  bool found;
  trackedref *ref;

  upb_lock();
  found = upb_inttable_lookupptr(r->refs, owner, &v);
  UPB_ASSERT(found);
  ref = upb_value_getptr(v);
  UPB_ASSERT(ref->is_ref2 == ref2);
  upb_unlock();
}

/* Populates the given UPB_CTYPE_INT32 inttable with counts of ref2's that
 * originate from the given owner. */
static void getref2s(const upb_refcounted *owner, upb_inttable *tab) {
  upb_inttable_iter i;

  upb_lock();
  upb_inttable_begin(&i, owner->ref2s);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    upb_value v;
    upb_value count;
    trackedref *ref;
    bool found;

    upb_refcounted *to = (upb_refcounted*)upb_inttable_iter_key(&i);

    /* To get the count we need to look in the target's table. */
    found = upb_inttable_lookupptr(to->refs, owner, &v);
    UPB_ASSERT(found);
    ref = upb_value_getptr(v);
    count = upb_value_int32(ref->count);

    upb_inttable_insertptr2(tab, to, count, &upb_alloc_debugrefs);
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
  upb_inttable *ref2 = &s->ref2;
  upb_value v;
  bool removed;
  int32_t newcount;

  UPB_ASSERT(obj == s->obj);
  UPB_ASSERT(subobj);
  removed = upb_inttable_removeptr(ref2, subobj, &v);
  /* The following assertion will fail if the visit() function visits a subobj
   * that it did not have a ref2 on, or visits the same subobj too many times. */
  UPB_ASSERT(removed);
  newcount = upb_value_getint32(v) - 1;
  if (newcount > 0) {
    upb_inttable_insert2(ref2, (uintptr_t)subobj, upb_value_int32(newcount),
                         &upb_alloc_debugrefs);
  }
}

static void visit(const upb_refcounted *r, upb_refcounted_visit *v,
                  void *closure) {
  /* In DEBUG_REFS mode we know what existing ref2 refs there are, so we know
   * exactly the set of nodes that visit() should visit.  So we verify visit()'s
   * correctness here. */
  check_state state;
  state.obj = r;
  upb_inttable_init2(&state.ref2, UPB_CTYPE_INT32, &upb_alloc_debugrefs);
  getref2s(r, &state.ref2);

  /* This should visit any children in the ref2 table. */
  if (r->vtbl->visit) r->vtbl->visit(r, visit_check, &state);

  /* This assertion will fail if the visit() function missed any children. */
  UPB_ASSERT(upb_inttable_count(&state.ref2) == 0);
  upb_inttable_uninit2(&state.ref2, &upb_alloc_debugrefs);
  if (r->vtbl->visit) r->vtbl->visit(r, v, closure);
}

static void trackinit(upb_refcounted *r) {
  r->refs = upb_malloc(&upb_alloc_debugrefs, sizeof(*r->refs));
  r->ref2s = upb_malloc(&upb_alloc_debugrefs, sizeof(*r->ref2s));
  upb_inttable_init2(r->refs, UPB_CTYPE_PTR, &upb_alloc_debugrefs);
  upb_inttable_init2(r->ref2s, UPB_CTYPE_PTR, &upb_alloc_debugrefs);
}

static void trackfree(const upb_refcounted *r) {
  upb_inttable_uninit2(r->refs, &upb_alloc_debugrefs);
  upb_inttable_uninit2(r->ref2s, &upb_alloc_debugrefs);
  upb_free(&upb_alloc_debugrefs, r->refs);
  upb_free(&upb_alloc_debugrefs, r->ref2s);
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

static void trackinit(upb_refcounted *r) {
  UPB_UNUSED(r);
}

static void trackfree(const upb_refcounted *r) {
  UPB_UNUSED(r);
}

static void visit(const upb_refcounted *r, upb_refcounted_visit *v,
                  void *closure) {
  if (r->vtbl->visit) r->vtbl->visit(r, v, closure);
}

#endif  /* UPB_DEBUG_REFS */


/* freeze() *******************************************************************/

/* The freeze() operation is by far the most complicated part of this scheme.
 * We compute strongly-connected components and then mutate the graph such that
 * we preserve the invariants documented at the top of this file.  And we must
 * handle out-of-memory errors gracefully (without leaving the graph
 * inconsistent), which adds to the fun. */

/* The state used by the freeze operation (shared across many functions). */
typedef struct {
  int depth;
  int maxdepth;
  uint64_t index;
  /* Maps upb_refcounted* -> attributes (color, etc).  attr layout varies by
   * color. */
  upb_inttable objattr;
  upb_inttable stack;   /* stack of upb_refcounted* for Tarjan's algorithm. */
  upb_inttable groups;  /* array of uint32_t*, malloc'd refcounts for new groups */
  upb_status *status;
  jmp_buf err;
} tarjan;

static void release_ref2(const upb_refcounted *obj,
                         const upb_refcounted *subobj,
                         void *closure);

/* Node attributes -----------------------------------------------------------*/

/* After our analysis phase all nodes will be either GRAY or WHITE. */

typedef enum {
  BLACK = 0,  /* Object has not been seen. */
  GRAY,   /* Object has been found via a refgroup but may not be reachable. */
  GREEN,  /* Object is reachable and is currently on the Tarjan stack. */
  WHITE   /* Object is reachable and has been assigned a group (SCC). */
} color_t;

UPB_NORETURN static void err(tarjan *t) { longjmp(t->err, 1); }
UPB_NORETURN static void oom(tarjan *t) {
  upb_status_seterrmsg(t->status, "out of memory");
  err(t);
}

static uint64_t trygetattr(const tarjan *t, const upb_refcounted *r) {
  upb_value v;
  return upb_inttable_lookupptr(&t->objattr, r, &v) ?
      upb_value_getuint64(v) : 0;
}

static uint64_t getattr(const tarjan *t, const upb_refcounted *r) {
  upb_value v;
  bool found = upb_inttable_lookupptr(&t->objattr, r, &v);
  UPB_ASSERT(found);
  return upb_value_getuint64(v);
}

static void setattr(tarjan *t, const upb_refcounted *r, uint64_t attr) {
  upb_inttable_removeptr(&t->objattr, r, NULL);
  upb_inttable_insertptr(&t->objattr, r, upb_value_uint64(attr));
}

static color_t color(tarjan *t, const upb_refcounted *r) {
  return trygetattr(t, r) & 0x3;  /* Color is always stored in the low 2 bits. */
}

static void set_gray(tarjan *t, const upb_refcounted *r) {
  UPB_ASSERT(color(t, r) == BLACK);
  setattr(t, r, GRAY);
}

/* Pushes an obj onto the Tarjan stack and sets it to GREEN. */
static void push(tarjan *t, const upb_refcounted *r) {
  UPB_ASSERT(color(t, r) == BLACK || color(t, r) == GRAY);
  /* This defines the attr layout for the GREEN state.  "index" and "lowlink"
   * get 31 bits, which is plenty (limit of 2B objects frozen at a time). */
  setattr(t, r, GREEN | (t->index << 2) | (t->index << 33));
  if (++t->index == 0x80000000) {
    upb_status_seterrmsg(t->status, "too many objects to freeze");
    err(t);
  }
  upb_inttable_push(&t->stack, upb_value_ptr((void*)r));
}

/* Pops an obj from the Tarjan stack and sets it to WHITE, with a ptr to its
 * SCC group. */
static upb_refcounted *pop(tarjan *t) {
  upb_refcounted *r = upb_value_getptr(upb_inttable_pop(&t->stack));
  UPB_ASSERT(color(t, r) == GREEN);
  /* This defines the attr layout for nodes in the WHITE state.
   * Top of group stack is [group, NULL]; we point at group. */
  setattr(t, r, WHITE | (upb_inttable_count(&t->groups) - 2) << 8);
  return r;
}

static void tarjan_newgroup(tarjan *t) {
  uint32_t *group = upb_gmalloc(sizeof(*group));
  if (!group) oom(t);
  /* Push group and empty group leader (we'll fill in leader later). */
  if (!upb_inttable_push(&t->groups, upb_value_ptr(group)) ||
      !upb_inttable_push(&t->groups, upb_value_ptr(NULL))) {
    upb_gfree(group);
    oom(t);
  }
  *group = 0;
}

static uint32_t idx(tarjan *t, const upb_refcounted *r) {
  UPB_ASSERT(color(t, r) == GREEN);
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
  UPB_ASSERT(color(t, r) == GREEN);
  setattr(t, r, ((uint64_t)lowlink << 33) | (getattr(t, r) & 0x1FFFFFFFF));
}

static uint32_t *group(tarjan *t, upb_refcounted *r) {
  uint64_t groupnum;
  upb_value v;
  bool found;

  UPB_ASSERT(color(t, r) == WHITE);
  groupnum = getattr(t, r) >> 8;
  found = upb_inttable_lookup(&t->groups, groupnum, &v);
  UPB_ASSERT(found);
  return upb_value_getptr(v);
}

/* If the group leader for this object's group has not previously been set,
 * the given object is assigned to be its leader. */
static upb_refcounted *groupleader(tarjan *t, upb_refcounted *r) {
  uint64_t leader_slot;
  upb_value v;
  bool found;

  UPB_ASSERT(color(t, r) == WHITE);
  leader_slot = (getattr(t, r) >> 8) + 1;
  found = upb_inttable_lookup(&t->groups, leader_slot, &v);
  UPB_ASSERT(found);
  if (upb_value_getptr(v)) {
    return upb_value_getptr(v);
  } else {
    upb_inttable_remove(&t->groups, leader_slot, NULL);
    upb_inttable_insert(&t->groups, leader_slot, upb_value_ptr(r));
    return r;
  }
}


/* Tarjan's algorithm --------------------------------------------------------*/

/* See:
 *   http://en.wikipedia.org/wiki/Tarjan%27s_strongly_connected_components_algorithm */
static void do_tarjan(const upb_refcounted *obj, tarjan *t);

static void tarjan_visit(const upb_refcounted *obj,
                         const upb_refcounted *subobj,
                         void *closure) {
  tarjan *t = closure;
  if (++t->depth > t->maxdepth) {
    upb_status_seterrf(t->status, "graph too deep to freeze (%d)", t->maxdepth);
    err(t);
  } else if (subobj->is_frozen || color(t, subobj) == WHITE) {
    /* Do nothing: we don't want to visit or color already-frozen nodes,
     * and WHITE nodes have already been assigned a SCC. */
  } else if (color(t, subobj) < GREEN) {
    /* Subdef has not yet been visited; recurse on it. */
    do_tarjan(subobj, t);
    set_lowlink(t, obj, UPB_MIN(lowlink(t, obj), lowlink(t, subobj)));
  } else if (color(t, subobj) == GREEN) {
    /* Subdef is in the stack and hence in the current SCC. */
    set_lowlink(t, obj, UPB_MIN(lowlink(t, obj), idx(t, subobj)));
  }
  --t->depth;
}

static void do_tarjan(const upb_refcounted *obj, tarjan *t) {
  if (color(t, obj) == BLACK) {
    /* We haven't seen this object's group; mark the whole group GRAY. */
    const upb_refcounted *o = obj;
    do { set_gray(t, o); } while ((o = o->next) != obj);
  }

  push(t, obj);
  visit(obj, tarjan_visit, t);
  if (lowlink(t, obj) == idx(t, obj)) {
    tarjan_newgroup(t);
    while (pop(t) != obj)
      ;
  }
}


/* freeze() ------------------------------------------------------------------*/

static void crossref(const upb_refcounted *r, const upb_refcounted *subobj,
                     void *_t) {
  tarjan *t = _t;
  UPB_ASSERT(color(t, r) > BLACK);
  if (color(t, subobj) > BLACK && r->group != subobj->group) {
    /* Previously this ref was not reflected in subobj->group because they
     * were in the same group; now that they are split a ref must be taken. */
    refgroup(subobj->group);
  }
}

static bool freeze(upb_refcounted *const*roots, int n, upb_status *s,
                   int maxdepth) {
  volatile bool ret = false;
  int i;
  upb_inttable_iter iter;

  /* We run in two passes so that we can allocate all memory before performing
   * any mutation of the input -- this allows us to leave the input unchanged
   * in the case of memory allocation failure. */
  tarjan t;
  t.index = 0;
  t.depth = 0;
  t.maxdepth = maxdepth;
  t.status = s;
  if (!upb_inttable_init(&t.objattr, UPB_CTYPE_UINT64)) goto err1;
  if (!upb_inttable_init(&t.stack, UPB_CTYPE_PTR)) goto err2;
  if (!upb_inttable_init(&t.groups, UPB_CTYPE_PTR)) goto err3;
  if (setjmp(t.err) != 0) goto err4;


  for (i = 0; i < n; i++) {
    if (color(&t, roots[i]) < GREEN) {
      do_tarjan(roots[i], &t);
    }
  }

  /* If we've made it this far, no further errors are possible so it's safe to
   * mutate the objects without risk of leaving them in an inconsistent state. */
  ret = true;

  /* The transformation that follows requires care.  The preconditions are:
   * - all objects in attr map are WHITE or GRAY, and are in mutable groups
   *   (groups of all mutable objs)
   * - no ref2(to, from) refs have incremented count(to) if both "to" and
   *   "from" are in our attr map (this follows from invariants (2) and (3)) */

  /* Pass 1: we remove WHITE objects from their mutable groups, and add them to
   * new groups  according to the SCC's we computed.  These new groups will
   * consist of only frozen objects.  None will be immediately collectible,
   * because WHITE objects are by definition reachable from one of "roots",
   * which the caller must own refs on. */
  upb_inttable_begin(&iter, &t.objattr);
  for(; !upb_inttable_done(&iter); upb_inttable_next(&iter)) {
    upb_refcounted *obj = (upb_refcounted*)upb_inttable_iter_key(&iter);
    /* Since removal from a singly-linked list requires access to the object's
     * predecessor, we consider obj->next instead of obj for moving.  With the
     * while() loop we guarantee that we will visit every node's predecessor.
     * Proof:
     *  1. every node's predecessor is in our attr map.
     *  2. though the loop body may change a node's predecessor, it will only
     *     change it to be the node we are currently operating on, so with a
     *     while() loop we guarantee ourselves the chance to remove each node. */
    while (color(&t, obj->next) == WHITE &&
           group(&t, obj->next) != obj->next->group) {
      upb_refcounted *leader;

      /* Remove from old group. */
      upb_refcounted *move = obj->next;
      if (obj == move) {
        /* Removing the last object from a group. */
        UPB_ASSERT(*obj->group == obj->individual_count);
        upb_gfree(obj->group);
      } else {
        obj->next = move->next;
        /* This may decrease to zero; we'll collect GRAY objects (if any) that
         * remain in the group in the third pass. */
        UPB_ASSERT(*move->group >= move->individual_count);
        *move->group -= move->individual_count;
      }

      /* Add to new group. */
      leader = groupleader(&t, move);
      if (move == leader) {
        /* First object added to new group is its leader. */
        move->group = group(&t, move);
        move->next = move;
        *move->group = move->individual_count;
      } else {
        /* Group already has at least one object in it. */
        UPB_ASSERT(leader->group == group(&t, move));
        move->group = group(&t, move);
        move->next = leader->next;
        leader->next = move;
        *move->group += move->individual_count;
      }

      move->is_frozen = true;
    }
  }

  /* Pass 2: GRAY and WHITE objects "obj" with ref2(to, obj) references must
   * increment count(to) if group(obj) != group(to) (which could now be the
   * case if "to" was just frozen). */
  upb_inttable_begin(&iter, &t.objattr);
  for(; !upb_inttable_done(&iter); upb_inttable_next(&iter)) {
    upb_refcounted *obj = (upb_refcounted*)upb_inttable_iter_key(&iter);
    visit(obj, crossref, &t);
  }

  /* Pass 3: GRAY objects are collected if their group's refcount dropped to
   * zero when we removed its white nodes.  This can happen if they had only
   * been kept alive by virtue of sharing a group with an object that was just
   * frozen.
   *
   * It is important that we do this last, since the GRAY object's free()
   * function could call unref2() on just-frozen objects, which will decrement
   * refs that were added in pass 2. */
  upb_inttable_begin(&iter, &t.objattr);
  for(; !upb_inttable_done(&iter); upb_inttable_next(&iter)) {
    upb_refcounted *obj = (upb_refcounted*)upb_inttable_iter_key(&iter);
    if (obj->group == NULL || *obj->group == 0) {
      if (obj->group) {
        upb_refcounted *o;

        /* We eagerly free() the group's count (since we can't easily determine
         * the group's remaining size it's the easiest way to ensure it gets
         * done). */
        upb_gfree(obj->group);

        /* Visit to release ref2's (done in a separate pass since release_ref2
         * depends on o->group being unmodified so it can test merged()). */
        o = obj;
        do { visit(o, release_ref2, NULL); } while ((o = o->next) != obj);

        /* Mark "group" fields as NULL so we know to free the objects later in
         * this loop, but also don't try to delete the group twice. */
        o = obj;
        do { o->group = NULL; } while ((o = o->next) != obj);
      }
      freeobj(obj);
    }
  }

err4:
  if (!ret) {
    upb_inttable_begin(&iter, &t.groups);
    for(; !upb_inttable_done(&iter); upb_inttable_next(&iter))
      upb_gfree(upb_value_getptr(upb_inttable_iter_value(&iter)));
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
  upb_refcounted *base;
  upb_refcounted *tmp;

  if (merged(r, from)) return;
  *r->group += *from->group;
  upb_gfree(from->group);
  base = from;

  /* Set all refcount pointers in the "from" chain to the merged refcount.
   *
   * TODO(haberman): this linear algorithm can result in an overall O(n^2) bound
   * if the user continuously extends a group by one object.  Prevent this by
   * using one of the techniques in this paper:
   *     http://bioinfo.ict.ac.cn/~dbu/AlgorithmCourses/Lectures/Union-Find-Tarjan.pdf */
  do { from->group = r->group; } while ((from = from->next) != base);

  /* Merge the two circularly linked lists by swapping their next pointers. */
  tmp = r->next;
  r->next = base->next;
  base->next = tmp;
}

static void unref(const upb_refcounted *r);

static void release_ref2(const upb_refcounted *obj,
                         const upb_refcounted *subobj,
                         void *closure) {
  UPB_UNUSED(closure);
  untrack(subobj, obj, true);
  if (!merged(obj, subobj)) {
    UPB_ASSERT(subobj->is_frozen);
    unref(subobj);
  }
}

static void unref(const upb_refcounted *r) {
  if (unrefgroup(r->group)) {
    const upb_refcounted *o;

    upb_gfree(r->group);

    /* In two passes, since release_ref2 needs a guarantee that any subobjs
     * are alive. */
    o = r;
    do { visit(o, release_ref2, NULL); } while((o = o->next) != r);

    o = r;
    do {
      const upb_refcounted *next = o->next;
      UPB_ASSERT(o->is_frozen || o->individual_count == 0);
      freeobj((upb_refcounted*)o);
      o = next;
    } while(o != r);
  }
}

static void freeobj(upb_refcounted *o) {
  trackfree(o);
  o->vtbl->free((upb_refcounted*)o);
}


/* Public interface ***********************************************************/

bool upb_refcounted_init(upb_refcounted *r,
                         const struct upb_refcounted_vtbl *vtbl,
                         const void *owner) {
#ifndef NDEBUG
  /* Endianness check.  This is unrelated to upb_refcounted, it's just a
   * convenient place to put the check that we can be assured will run for
   * basically every program using upb. */
  const int x = 1;
#ifdef UPB_BIG_ENDIAN
  UPB_ASSERT(*(char*)&x != 1);
#else
  UPB_ASSERT(*(char*)&x == 1);
#endif
#endif

  r->next = r;
  r->vtbl = vtbl;
  r->individual_count = 0;
  r->is_frozen = false;
  r->group = upb_gmalloc(sizeof(*r->group));
  if (!r->group) return false;
  *r->group = 0;
  trackinit(r);
  upb_refcounted_ref(r, owner);
  return true;
}

bool upb_refcounted_isfrozen(const upb_refcounted *r) {
  return r->is_frozen;
}

void upb_refcounted_ref(const upb_refcounted *r, const void *owner) {
  track(r, owner, false);
  if (!r->is_frozen)
    ((upb_refcounted*)r)->individual_count++;
  refgroup(r->group);
}

void upb_refcounted_unref(const upb_refcounted *r, const void *owner) {
  untrack(r, owner, false);
  if (!r->is_frozen)
    ((upb_refcounted*)r)->individual_count--;
  unref(r);
}

void upb_refcounted_ref2(const upb_refcounted *r, upb_refcounted *from) {
  UPB_ASSERT(!from->is_frozen);  /* Non-const pointer implies this. */
  track(r, from, true);
  if (r->is_frozen) {
    refgroup(r->group);
  } else {
    merge((upb_refcounted*)r, from);
  }
}

void upb_refcounted_unref2(const upb_refcounted *r, upb_refcounted *from) {
  UPB_ASSERT(!from->is_frozen);  /* Non-const pointer implies this. */
  untrack(r, from, true);
  if (r->is_frozen) {
    unref(r);
  } else {
    UPB_ASSERT(merged(r, from));
  }
}

void upb_refcounted_donateref(
    const upb_refcounted *r, const void *from, const void *to) {
  UPB_ASSERT(from != to);
  if (to != NULL)
    upb_refcounted_ref(r, to);
  if (from != NULL)
    upb_refcounted_unref(r, from);
}

void upb_refcounted_checkref(const upb_refcounted *r, const void *owner) {
  checkref(r, owner, false);
}

bool upb_refcounted_freeze(upb_refcounted *const*roots, int n, upb_status *s,
                           int maxdepth) {
  int i;
  bool ret;
  for (i = 0; i < n; i++) {
    UPB_ASSERT(!roots[i]->is_frozen);
  }
  ret = freeze(roots, n, s, maxdepth);
  UPB_ASSERT(!s || ret == upb_ok(s));
  return ret;
}
