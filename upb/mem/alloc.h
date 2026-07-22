// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MEM_ALLOC_H_
#define UPB_MEM_ALLOC_H_

#include <stddef.h>

#include "upb/mem/internal/alloc.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct upb_alloc upb_alloc;

/* A combined `malloc()`/`free()` function.
 * If `size` is 0 then the function acts like `free()`, otherwise it acts like
 * `realloc()`.  Only `oldsize` bytes from a previous allocation are
 * preserved. If `actual_size` is not null and the allocator supports it, the
 * actual size of the resulting allocation is stored in `actual_size`. If
 * `actual_size` is not null, you must zero out the memory pointed to by
 * `actual_size` before calling. */
typedef void* upb_alloc_func(upb_alloc* alloc, void* ptr, size_t oldsize,
                             size_t size, size_t* actual_size);

/* A upb_alloc is a possibly-stateful allocator object.
 *
 * It could either be an arena allocator (which doesn't require individual
 * `free()` calls) or a regular `malloc()` (which does).  The client must
 * therefore free memory unless it knows that the allocator is an arena
 * allocator. */
struct upb_alloc {
  upb_alloc_func* func;
};

UPB_NODISCARD UPB_INLINE void* upb_malloc(upb_alloc* alloc, size_t size) {
  UPB_ASSERT(alloc);
  if (!upb_AllocationCount_IncrementAndCheck()) {
    return NULL;
  }
  return alloc->func(alloc, NULL, 0, size, NULL);
}

typedef struct {
  void* p;
  size_t n;
} upb_SizedPtr;

UPB_INLINE upb_SizedPtr upb_SizeReturningMalloc(upb_alloc* alloc, size_t size) {
  UPB_ASSERT(alloc);
  if (!upb_AllocationCount_IncrementAndCheck()) {
    return (upb_SizedPtr){.p = NULL, .n = 0};
  }
  upb_SizedPtr result;
  result.n = 0;
  result.p = alloc->func(alloc, NULL, 0, size, &result.n);
  result.n = result.p != NULL ? UPB_MAX(result.n, size) : 0;
  return result;
}

UPB_NODISCARD UPB_INLINE void* upb_realloc(upb_alloc* alloc, void* ptr,
                                           size_t oldsize, size_t size) {
  UPB_ASSERT(alloc);
  if (size != 0) {
    if (!upb_AllocationCount_IncrementAndCheck()) {
      return NULL;
    }
  }
  return alloc->func(alloc, ptr, oldsize, size, NULL);
}

UPB_INLINE void upb_free(upb_alloc* alloc, void* ptr) {
  UPB_ASSERT(alloc);
  alloc->func(alloc, ptr, 0, 0, NULL);
}

UPB_INLINE void upb_free_sized(upb_alloc* alloc, void* ptr, size_t size) {
  UPB_ASSERT(alloc);
  alloc->func(alloc, ptr, size, 0, NULL);
}

// The global allocator used by upb. Uses the standard malloc()/free().

extern upb_alloc upb_alloc_global;

/* Functions that hard-code the global malloc.
 *
 * We still get benefit because we can put custom logic into our global
 * allocator, like injecting out-of-memory faults in debug/testing builds. */

UPB_NODISCARD UPB_INLINE void* upb_gmalloc(size_t size) {
  return upb_malloc(&upb_alloc_global, size);
}

UPB_NODISCARD UPB_INLINE void* upb_grealloc(void* ptr, size_t oldsize,
                                            size_t size) {
  return upb_realloc(&upb_alloc_global, ptr, oldsize, size);
}

UPB_INLINE void upb_gfree(void* ptr) { upb_free(&upb_alloc_global, ptr); }

// Returns whether thread-local allocation count/ OOM-simulation features
// are supported.
UPB_API UPB_NODISCARD bool upb_AllocationCount_IsAvailable(void);
// Returns the thread-local allocation count since the last reset.
UPB_API UPB_NODISCARD size_t upb_AllocationCount_Get(void);
// Resets the thread-local allocation count and failure threshold.
UPB_API void upb_AllocationCount_Reset(void);
// Artificially triggers memory allocation failure in the thread on the n-th
// allocation.
UPB_API void upb_AllocationCount_FailOn(size_t n);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MEM_ALLOC_H_ */
