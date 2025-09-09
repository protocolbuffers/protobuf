// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

/* upb_Arena is a specific allocator implementation that uses arena allocation.
 * The user provides an allocator that will be used to allocate the underlying
 * arena blocks.  Arenas by nature do not require the individual allocations
 * to be freed.  However the Arena does allow users to register cleanup
 * functions that will run when the arena is destroyed.
 *
 * A upb_Arena is *not* thread-safe, although some functions related to its
 * managing its lifetime are, and are documented as such.
 *
 * You could write a thread-safe arena allocator that satisfies the
 * upb_alloc interface, but it would not be as efficient for the
 * single-threaded case. */

#ifndef UPB_MEM_ARENA_H_
#define UPB_MEM_ARENA_H_

#include <stddef.h>
#include <stdint.h>

#include "upb/mem/alloc.h"
#include "upb/mem/internal/arena.h"

// Must be last.
#include "upb/port/def.inc"

typedef struct upb_Arena upb_Arena;

typedef void upb_AllocCleanupFunc(upb_alloc* alloc);

#ifdef __cplusplus
extern "C" {
#endif

// Creates an arena from the given initial block (if any -- mem may be NULL). If
// an initial block is specified, the arena's lifetime cannot be extended by
// |upb_Arena_IncRefFor| or |upb_Arena_Fuse|. Additional blocks will be
// allocated from |alloc|. If |alloc| is NULL, this is a fixed-size arena and
// cannot grow. If an initial block is specified, |n| is its length; if there is
// no initial block, |n| is a hint of the size that should be allocated for the
// first block of the arena, such that `upb_Arena_Malloc(hint)` will not require
// another call to |alloc|.
UPB_API upb_Arena* upb_Arena_Init(void* mem, size_t n, upb_alloc* alloc);

UPB_API void upb_Arena_Free(upb_Arena* a);
// Sets the cleanup function for the upb_alloc used by the arena. Only one
// cleanup function can be set, which will be called after all blocks are
// freed.
UPB_API void upb_Arena_SetAllocCleanup(upb_Arena* a,
                                       upb_AllocCleanupFunc* func);

// Fuses the lifetime of two arenas, such that no arenas that have been
// transitively fused together will be freed until all of them have reached a
// zero refcount. This operation is safe to use concurrently from multiple
// threads.
UPB_API bool upb_Arena_Fuse(const upb_Arena* a, const upb_Arena* b);

// This operation is safe to use concurrently from multiple threads.
UPB_API bool upb_Arena_IsFused(const upb_Arena* a, const upb_Arena* b);

// Returns the upb_alloc used by the arena.
UPB_API upb_alloc* upb_Arena_GetUpbAlloc(upb_Arena* a);

// This operation is safe to use concurrently from multiple threads.
bool upb_Arena_IncRefFor(const upb_Arena* a, const void* owner);
// This operation is safe to use concurrently from multiple threads.
void upb_Arena_DecRefFor(const upb_Arena* a, const void* owner);

// Creates a reference between the arenas `from` and `to`, guaranteeing that
// the latter will not be freed until `from` is freed.
//
// Users must avoid all of the following error conditions, which will be
// checked in debug mode but are UB in opt:
//
// - Creating reference cycles between arenas.
// - Creating a reference between two arenas that are fused, either now
//   or in the future.
//
// Creating a reference multiple times between the same two arenas is not UB but
// is considered wasteful and may be disallowed in the future.
//
// Note that fuses can participate in reference cycles. The following set of
// calls creates a cycle A -> B -> C -> A
//   Fuse(A, B);
//   Ref(B, C);
//   Ref(C, A);
//
// From this perspective, the second rule is just a special-case of the first.
// This set of calls is disallowed because it is effectively creating a
// cycle A -> B -> A
//   Fuse(A, B);
//   Ref(B, A);
//
// Fuse is special because it creates what is effectively a bidirectional
// ref, but it is not considered a cycle and will be collected correctly.
//
// Note that `from` is not `const`, so it may not be called concurrently
// with any other function on `from`.
//
// Returns whether the reference was created successfully.
bool upb_Arena_RefArena(upb_Arena* from, const upb_Arena* to);

#ifndef NDEBUG
// Returns true if upb_Arena_RefArena(from, to) was previously called.
// Note that this does not take fuses into account, and it does not follow
// chains of references; it must have been these two arenas exactly that
// created a reference.
bool upb_Arena_HasRef(const upb_Arena* from, const upb_Arena* to);
#endif

// This operation is safe to use concurrently from multiple threads.
uintptr_t upb_Arena_SpaceAllocated(const upb_Arena* a, size_t* fused_count);
// This operation is safe to use concurrently from multiple threads.
uint32_t upb_Arena_DebugRefCount(const upb_Arena* a);

#if UPB_ENABLE_REF_CYCLE_CHECKS
// Returns true if there is a chain of arena refs that spans `from` -> `to`.
// Fused arenas are taken into account; for example, this series of calls
// will cause the function to return true:
//
// 1. upb_Arena_Fuse(a, b)
// 2. upb_Arena_RefArena(from, a)
// 3. upb_Arena_RefArena(b, to)
//
// However this function does not return true if `from` and `to` are directly
// fused.
bool upb_Arena_HasRefChain(const upb_Arena* from, const upb_Arena* to);
#endif

UPB_API_INLINE upb_Arena* upb_Arena_New(void) {
  return upb_Arena_Init(NULL, 0, &upb_alloc_global);
}

UPB_API_INLINE upb_Arena* upb_Arena_NewSized(size_t size_hint) {
  return upb_Arena_Init(NULL, size_hint, &upb_alloc_global);
}

UPB_API_INLINE void* upb_Arena_Malloc(struct upb_Arena* a, size_t size);

UPB_API_INLINE void* upb_Arena_Realloc(upb_Arena* a, void* ptr, size_t oldsize,
                                       size_t size);

static const size_t UPB_PRIVATE(kUpbDefaultMaxBlockSize) =
    UPB_DEFAULT_MAX_BLOCK_SIZE;

// Sets the maximum block size for all arenas. This is a global configuration
// setting that will affect all existing and future arenas. If
// upb_Arena_Malloc() is called with a size larger than this, we will exceed
// this size and allocate a larger block.
//
// This API is meant for experimentation only. It will likely be removed in
// the future.
// This operation is safe to use concurrently from multiple threads.
void upb_Arena_SetMaxBlockSize(size_t max);

// Shrinks the last alloc from arena.
// REQUIRES: (ptr, oldsize) was the last malloc/realloc from this arena.
// We could also add a upb_Arena_TryShrinkLast() which is simply a no-op if
// this was not the last alloc.
UPB_API_INLINE void upb_Arena_ShrinkLast(upb_Arena* a, void* ptr,
                                         size_t oldsize, size_t size);

// Attempts to extend the given alloc from arena, in place. Is generally
// only likely to succeed for the most recent allocation from this arena. If it
// succeeds, returns true and `ptr`'s allocation is now `size` rather than
// `oldsize`. Returns false if the allocation cannot be extended; `ptr`'s
// allocation is unmodified. See also upb_Arena_Realloc.
// REQUIRES: `size > oldsize`; to shrink, use `upb_Arena_Realloc` or
// `upb_Arena_ShrinkLast`.
UPB_API_INLINE bool upb_Arena_TryExtend(upb_Arena* a, void* ptr, size_t oldsize,
                                        size_t size);

#ifdef UPB_TRACING_ENABLED
void upb_Arena_SetTraceHandler(void (*initArenaTraceHandler)(const upb_Arena*,
                                                             size_t size),
                               void (*fuseArenaTraceHandler)(const upb_Arena*,
                                                             const upb_Arena*),
                               void (*freeArenaTraceHandler)(const upb_Arena*));
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MEM_ARENA_H_ */
