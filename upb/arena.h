/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UPB_ARENA_H_
#define UPB_ARENA_H_

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "upb/port_def.inc"

#ifdef __cplusplus
extern "C" {
#endif

/** upb_alloc *****************************************************************/

/* A upb_alloc is a possibly-stateful allocator object.
 *
 * It could either be an arena allocator (which doesn't require individual
 * free() calls) or a regular malloc() (which does).  The client must therefore
 * free memory unless it knows that the allocator is an arena allocator. */

struct upb_alloc;
typedef struct upb_alloc upb_alloc;

/* A malloc()/free() function.
 * If "size" is 0 then the function acts like free(), otherwise it acts like
 * realloc().  Only "oldsize" bytes from a previous allocation are preserved. */
typedef void* upb_alloc_func(upb_alloc* alloc, void* ptr, size_t oldsize,
                             size_t size);

struct upb_alloc {
  upb_alloc_func* func;
};

UPB_INLINE void* upb_malloc(upb_alloc* alloc, size_t size) {
  UPB_ASSERT(alloc);
  return alloc->func(alloc, NULL, 0, size);
}

UPB_INLINE void* upb_realloc(upb_alloc* alloc, void* ptr, size_t oldsize,
                             size_t size) {
  UPB_ASSERT(alloc);
  return alloc->func(alloc, ptr, oldsize, size);
}

UPB_INLINE void upb_free(upb_alloc* alloc, void* ptr) {
  assert(alloc);
  alloc->func(alloc, ptr, 0, 0);
}

/* The global allocator used by upb.  Uses the standard malloc()/free(). */

extern upb_alloc upb_alloc_global;

/* Functions that hard-code the global malloc.
 *
 * We still get benefit because we can put custom logic into our global
 * allocator, like injecting out-of-memory faults in debug/testing builds. */

UPB_INLINE void* upb_gmalloc(size_t size) {
  return upb_malloc(&upb_alloc_global, size);
}

UPB_INLINE void* upb_grealloc(void* ptr, size_t oldsize, size_t size) {
  return upb_realloc(&upb_alloc_global, ptr, oldsize, size);
}

UPB_INLINE void upb_gfree(void* ptr) { upb_free(&upb_alloc_global, ptr); }

/* upb_Arena ******************************************************************/

/* upb_Arena is a specific allocator implementation that uses arena allocation.
 * The user provides an allocator that will be used to allocate the underlying
 * arena blocks.  Arenas by nature do not require the individual allocations
 * to be freed.  However the Arena does allow users to register cleanup
 * functions that will run when the arena is destroyed.
 *
 * A upb_Arena is *not* thread-safe.
 *
 * You could write a thread-safe arena allocator that satisfies the
 * upb_alloc interface, but it would not be as efficient for the
 * single-threaded case. */

typedef void upb_CleanupFunc(void* ud);

struct upb_Arena;
typedef struct upb_Arena upb_Arena;

typedef struct {
  /* We implement the allocator interface.
   * This must be the first member of upb_Arena!
   * TODO(haberman): remove once handlers are gone. */
  upb_alloc alloc;

  char *ptr, *end;
} _upb_ArenaHead;

/* Creates an arena from the given initial block (if any -- n may be 0).
 * Additional blocks will be allocated from |alloc|.  If |alloc| is NULL, this
 * is a fixed-size arena and cannot grow. */
upb_Arena* upb_Arena_Init(void* mem, size_t n, upb_alloc* alloc);
void upb_Arena_Free(upb_Arena* a);
bool upb_Arena_AddCleanup(upb_Arena* a, void* ud, upb_CleanupFunc* func);
bool upb_Arena_Fuse(upb_Arena* a, upb_Arena* b);
void* _upb_Arena_SlowMalloc(upb_Arena* a, size_t size);

UPB_INLINE upb_alloc* upb_Arena_Alloc(upb_Arena* a) { return (upb_alloc*)a; }

UPB_INLINE size_t _upb_ArenaHas(upb_Arena* a) {
  _upb_ArenaHead* h = (_upb_ArenaHead*)a;
  return (size_t)(h->end - h->ptr);
}

UPB_INLINE void* _upb_Arena_FastMalloc(upb_Arena* a, size_t size) {
  _upb_ArenaHead* h = (_upb_ArenaHead*)a;
  void* ret = h->ptr;
  UPB_ASSERT(UPB_ALIGN_MALLOC((uintptr_t)ret) == (uintptr_t)ret);
  UPB_ASSERT(UPB_ALIGN_MALLOC(size) == size);
  UPB_UNPOISON_MEMORY_REGION(ret, size);

  h->ptr += size;

#if UPB_ASAN
  {
    size_t guard_size = 32;
    if (_upb_ArenaHas(a) >= guard_size) {
      h->ptr += guard_size;
    } else {
      h->ptr = h->end;
    }
  }
#endif

  return ret;
}

UPB_INLINE void* upb_Arena_Malloc(upb_Arena* a, size_t size) {
  size = UPB_ALIGN_MALLOC(size);

  if (UPB_UNLIKELY(_upb_ArenaHas(a) < size)) {
    return _upb_Arena_SlowMalloc(a, size);
  }

  return _upb_Arena_FastMalloc(a, size);
}

// Shrinks the last alloc from arena.
// REQUIRES: (ptr, oldsize) was the last malloc/realloc from this arena.
// We could also add a upb_Arena_TryShrinkLast() which is simply a no-op if
// this was not the last alloc.
UPB_INLINE void upb_Arena_ShrinkLast(upb_Arena* a, void* ptr, size_t oldsize,
                                     size_t size) {
  _upb_ArenaHead* h = (_upb_ArenaHead*)a;
  oldsize = UPB_ALIGN_MALLOC(oldsize);
  size = UPB_ALIGN_MALLOC(size);
  UPB_ASSERT((char*)ptr + oldsize == h->ptr);  // Must be the last alloc.
  UPB_ASSERT(size <= oldsize);
  h->ptr = (char*)ptr + size;
}

UPB_INLINE void* upb_Arena_Realloc(upb_Arena* a, void* ptr, size_t oldsize,
                                   size_t size) {
  _upb_ArenaHead* h = (_upb_ArenaHead*)a;
  oldsize = UPB_ALIGN_MALLOC(oldsize);
  size = UPB_ALIGN_MALLOC(size);
  bool is_most_recent_alloc = (uintptr_t)ptr + oldsize == (uintptr_t)h->ptr;

  if (is_most_recent_alloc) {
    ptrdiff_t diff = size - oldsize;
    if ((ptrdiff_t)_upb_ArenaHas(a) >= diff) {
      h->ptr += diff;
      return ptr;
    }
  } else if (size <= oldsize) {
    return ptr;
  }

  void* ret = upb_Arena_Malloc(a, size);

  if (ret && oldsize > 0) {
    memcpy(ret, ptr, UPB_MIN(oldsize, size));
  }

  return ret;
}

UPB_INLINE upb_Arena* upb_Arena_New(void) {
  return upb_Arena_Init(NULL, 0, &upb_alloc_global);
}

#include "upb/port_undef.inc"

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* UPB_ARENA_H_ */
