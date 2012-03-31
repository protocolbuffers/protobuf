/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2012 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include <stdlib.h>
#include "upb/refcount.h"

// TODO(haberman): require client to define these if ref debugging is on.
#ifndef UPB_LOCK
#define UPB_LOCK
#endif

#ifndef UPB_UNLOCK
#define UPB_UNLOCK
#endif

/* arch-specific atomic primitives  *******************************************/

#ifdef UPB_THREAD_UNSAFE  //////////////////////////////////////////////////////

INLINE void upb_atomic_inc(uint32_t *a) { (*a)++; }
INLINE bool upb_atomic_dec(uint32_t *a) { return --(*a) == 0; }

#elif (__GNUC__ == 4 && __GNUC_MINOR__ >= 1) || __GNUC__ > 4 ///////////////////

INLINE void upb_atomic_inc(uint32_t *a) { __sync_fetch_and_add(a, 1); }
INLINE bool upb_atomic_dec(uint32_t *a) {
  return __sync_sub_and_fetch(a, 1) == 0;
}

#elif defined(WIN32) ///////////////////////////////////////////////////////////

#include <Windows.h>

INLINE void upb_atomic_inc(upb_atomic_t *a) { InterlockedIncrement(&a->val); }
INLINE bool upb_atomic_dec(upb_atomic_t *a) {
  return InterlockedDecrement(&a->val) == 0;
}

#else
#error Atomic primitives not defined for your platform/CPU.  \
       Implement them or compile with UPB_THREAD_UNSAFE.
#endif

// Reserved index values.
#define UPB_INDEX_UNDEFINED UINT16_MAX
#define UPB_INDEX_NOT_IN_STACK (UINT16_MAX - 1)

static void upb_refcount_merge(upb_refcount *r, upb_refcount *from) {
  if (upb_refcount_merged(r, from)) return;
  *r->count += *from->count;
  free(from->count);
  upb_refcount *base = from;

  // Set all refcount pointers in the "from" chain to the merged refcount.
  do { from->count = r->count; } while ((from = from->next) != base);

  // Merge the two circularly linked lists by swapping their next pointers.
  upb_refcount *tmp = r->next;
  r->next = base->next;
  base->next = tmp;
}

// Tarjan's algorithm, see:
//   http://en.wikipedia.org/wiki/Tarjan%27s_strongly_connected_components_algorithm

typedef struct {
  int index;
  upb_refcount **stack;
  int stack_len;
  upb_getsuccessors *func;
} upb_tarjan_state;

static void upb_refcount_dofindscc(upb_refcount *obj, upb_tarjan_state *state);

void upb_refcount_visit(upb_refcount *obj, upb_refcount *subobj, void *_state) {
  upb_tarjan_state *state = _state;
  if (subobj->index == UPB_INDEX_UNDEFINED) {
    // Subdef has not yet been visited; recurse on it.
    upb_refcount_dofindscc(subobj, state);
    obj->lowlink = UPB_MIN(obj->lowlink, subobj->lowlink);
  } else if (subobj->index != UPB_INDEX_NOT_IN_STACK) {
    // Subdef is in the stack and hence in the current SCC.
    obj->lowlink = UPB_MIN(obj->lowlink, subobj->index);
  }
}

static void upb_refcount_dofindscc(upb_refcount *obj, upb_tarjan_state *state) {
  obj->index = state->index;
  obj->lowlink = state->index;
  state->index++;
  state->stack[state->stack_len++] = obj;

  state->func(obj, state);  // Visit successors.

  if (obj->lowlink == obj->index) {
    upb_refcount *scc_obj;
    while ((scc_obj = state->stack[--state->stack_len]) != obj) {
      upb_refcount_merge(obj, scc_obj);
      scc_obj->index = UPB_INDEX_NOT_IN_STACK;
    }
    obj->index = UPB_INDEX_NOT_IN_STACK;
  }
}

bool upb_refcount_findscc(upb_refcount **refs, int n, upb_getsuccessors *func) {
  // TODO(haberman): allocate less memory.  We can't use n as a bound because
  // it doesn't include fielddefs.  Could either use a dynamically-resizing
  // array or think of some other way.
  upb_tarjan_state state = {0, malloc(UINT16_MAX * sizeof(void*)), 0, func};
  if (state.stack == NULL) return false;
  for (int i = 0; i < n; i++)
    if (refs[i]->index == UPB_INDEX_UNDEFINED)
      upb_refcount_dofindscc(refs[i], &state);
  free(state.stack);
  return true;
}

#ifdef UPB_DEBUG_REFS
static void upb_refcount_track(const upb_refcount *r, const void *owner) {
  // Caller must not already own a ref.
  assert(upb_inttable_lookup(r->refs, (uintptr_t)owner) == NULL);

  // If a ref is leaked we want to blame the leak on the whoever leaked the
  // ref, not on who originally allocated the refcounted object.  We accomplish
  // this as follows.  When a ref is taken in DEBUG_REFS mode, we malloc() some
  // memory and arrange setup pointers like so:
  //
  //   upb_refcount
  //   +----------+  +---------+
  //   | count    |<-+         |
  //   +----------+       +----------+
  //   | table    |---X-->| malloc'd |
  //   +----------+       | memory   |
  //                      +----------+
  //
  // Since the "malloc'd memory" is allocated inside of "ref" and free'd in
  // unref, it will cause a leak if not unref'd.  And since the leaked memory
  // points to the object itself, the object will be considered "indirectly
  // lost" by tools like Valgrind and not shown unless requested (which is good
  // because the object's creator may not be responsible for the leak).  But we
  // have to hide the pointer marked "X" above from Valgrind, otherwise the
  // malloc'd memory will appear to be indirectly leaked and the object itself
  // will still be considered the primary leak.  We hide this pointer from
  // Valgrind (et all) by doing a bitwise not on it.
  const upb_refcount **target = malloc(sizeof(void*));
  uintptr_t obfuscated = ~(uintptr_t)target;
  *target = r;
  upb_inttable_insert(r->refs, (uintptr_t)owner, upb_value_uint64(obfuscated));
}

static void upb_refcount_untrack(const upb_refcount *r, const void *owner) {
  upb_value v;
  bool success = upb_inttable_remove(r->refs, (uintptr_t)owner, &v);
  assert(success);
  if (success) {
    // Must un-obfuscate the pointer (see above).
    free((void*)(~upb_value_getuint64(v)));
  }
}
#endif


/* upb_refcount  **************************************************************/

bool upb_refcount_init(upb_refcount *r, const void *owner) {
  (void)owner;
  r->count = malloc(sizeof(uint32_t));
  if (!r->count) return false;
  // Initializing this here means upb_refcount_findscc() can only run once for
  // each refcount; may need to revise this to be more flexible.
  r->index = UPB_INDEX_UNDEFINED;
  r->next = r;
#ifdef UPB_DEBUG_REFS
  // We don't detect malloc() failures for UPB_DEBUG_REFS.
  r->refs = malloc(sizeof(*r->refs));
  upb_inttable_init(r->refs);
  *r->count = 0;
  upb_refcount_ref(r, owner);
#else
  *r->count = 1;
#endif
  return true;
}

void upb_refcount_uninit(upb_refcount *r) {
  (void)r;
#ifdef UPB_DEBUG_REFS
  assert(upb_inttable_count(r->refs) == 0);
  upb_inttable_uninit(r->refs);
  free(r->refs);
#endif
}

// Thread-safe operations //////////////////////////////////////////////////////

void upb_refcount_ref(const upb_refcount *r, const void *owner) {
  (void)owner;
  upb_atomic_inc(r->count);
#ifdef UPB_DEBUG_REFS
  UPB_LOCK;
  upb_refcount_track(r, owner);
  UPB_UNLOCK;
#endif
}

bool upb_refcount_unref(const upb_refcount *r, const void *owner) {
  (void)owner;
  bool ret = upb_atomic_dec(r->count);
#ifdef UPB_DEBUG_REFS
  UPB_LOCK;
  upb_refcount_untrack(r, owner);
  UPB_UNLOCK;
#endif
  if (ret) free(r->count);
  return ret;
}

void upb_refcount_donateref(
    const upb_refcount *r, const void *from, const void *to) {
  (void)r; (void)from; (void)to;
  assert(from != to);
#ifdef UPB_DEBUG_REFS
  UPB_LOCK;
  upb_refcount_track(r, to);
  upb_refcount_untrack(r, from);
  UPB_UNLOCK;
#endif
}

bool upb_refcount_merged(const upb_refcount *r, const upb_refcount *r2) {
  return r->count == r2->count;
}
