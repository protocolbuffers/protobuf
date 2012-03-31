/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * A thread-safe refcount that can optionally track references for debugging
 * purposes.  It helps avoid circular references by allowing a
 * strongly-connected component in the graph to share a refcount.
 *
 * This interface is internal to upb.
 */

#ifndef UPB_REFCOUNT_H_
#define UPB_REFCOUNT_H_

#include <stdbool.h>
#include <stdint.h>
#include "upb/table.h"

#ifndef NDEBUG
#define UPB_DEBUG_REFS
#endif

typedef struct _upb_refcount {
  uint32_t *count;
  struct _upb_refcount *next;  // Circularly-linked list of this SCC.
  uint16_t index;    // For SCC algorithm.
  uint16_t lowlink;  // For SCC algorithm.
#ifdef UPB_DEBUG_REFS
  // Make this a pointer so that we can modify it inside of const methods
  // without ugly casts.
  upb_inttable *refs;
#endif
} upb_refcount;

// NON THREAD SAFE operations //////////////////////////////////////////////////

// Initializes the refcount with a single ref for the given owner.  Returns
// NULL if memory could not be allocated.
bool upb_refcount_init(upb_refcount *r, const void *owner);

// Uninitializes the refcount.  May only be called after unref() returns true.
void upb_refcount_uninit(upb_refcount *r);

// Finds strongly-connected components among some set of objects and merges all
// refcounts that share a SCC.  The given function will be called when the
// algorithm needs to visit children of a particular object; the function
// should call upb_refcount_visit() once for each child obj.
//
// Returns false if memory allocation failed.
typedef void upb_getsuccessors(upb_refcount *obj, void*);
bool upb_refcount_findscc(upb_refcount **objs, int n, upb_getsuccessors *func);
void upb_refcount_visit(upb_refcount *obj, upb_refcount *subobj, void *closure);

// Thread-safe operations //////////////////////////////////////////////////////

// Increases the ref count, the new ref is owned by "owner" which must not
// already own a ref.  Circular reference chains are not allowed.
void upb_refcount_ref(const upb_refcount *r, const void *owner);

// Release a ref owned by owner, returns true if that was the last ref.
bool upb_refcount_unref(const upb_refcount *r, const void *owner);

// Moves an existing ref from ref_donor to new_owner, without changing the
// overall ref count.
void upb_refcount_donateref(
    const upb_refcount *r, const void *from, const void *to);

// Returns true if these two objects share a refcount.
bool upb_refcount_merged(const upb_refcount *r, const upb_refcount *r2);

#endif  // UPB_REFCOUNT_H_
