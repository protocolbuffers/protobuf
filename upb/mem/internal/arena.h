// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MEM_INTERNAL_ARENA_H_
#define UPB_MEM_INTERNAL_ARENA_H_

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "upb/port/sanitizers.h"

// Must be last.
#include "upb/port/def.inc"

// This is QUITE an ugly hack, which specifies the number of pointers needed
// to equal (or exceed) the storage required for one upb_Arena.
//
// We need this because the decoder inlines a upb_Arena for performance but
// the full struct is not visible outside of arena.c. Yes, I know, it's awful.
#ifndef NDEBUG
#define UPB_ARENA_BASE_SIZE_HACK 10
#else
#define UPB_ARENA_BASE_SIZE_HACK 9
#endif

#define UPB_ARENA_SIZE_HACK                                                   \
  (sizeof(void*) * (UPB_ARENA_BASE_SIZE_HACK + (UPB_XSAN_STRUCT_SIZE * 2))) + \
      (sizeof(uint32_t) * 2)

// LINT.IfChange(upb_Arena)

struct upb_Arena {
  char* UPB_ONLYBITS(ptr);
  const UPB_NODEREF char* UPB_ONLYBITS(end);
  UPB_XSAN_MEMBER
};

// LINT.ThenChange(//depot/google3/third_party/upb/bits/typescript/arena.ts:upb_Arena)

#ifdef __cplusplus
extern "C" {
#endif

void UPB_PRIVATE(_upb_Arena_SwapIn)(struct upb_Arena* des,
                                    const struct upb_Arena* src);
void UPB_PRIVATE(_upb_Arena_SwapOut)(struct upb_Arena* des,
                                     const struct upb_Arena* src);

UPB_INLINE size_t UPB_PRIVATE(_upb_ArenaHas)(const struct upb_Arena* a) {
  return (size_t)(a->UPB_ONLYBITS(end) - a->UPB_ONLYBITS(ptr));
}

UPB_INLINE size_t UPB_PRIVATE(_upb_Arena_AllocSpan)(size_t size) {
  return UPB_ALIGN_MALLOC(size) + UPB_PRIVATE(kUpb_Asan_GuardSize);
}

UPB_INLINE bool UPB_PRIVATE(_upb_Arena_WasLastAllocFromCurrentBlock)(
    const struct upb_Arena* a, void* ptr, size_t size) {
  return UPB_PRIVATE(upb_Xsan_PtrEq)(
      (char*)ptr + UPB_PRIVATE(_upb_Arena_AllocSpan)(size),
      a->UPB_ONLYBITS(ptr));
}

UPB_INLINE bool UPB_PRIVATE(_upb_Arena_IsAligned)(const void* ptr) {
  return (uintptr_t)ptr % UPB_MALLOC_ALIGN == 0;
}

UPB_API_INLINE void* upb_Arena_Malloc(struct upb_Arena* a, size_t size) {
  UPB_PRIVATE(upb_Xsan_AccessReadWrite)(UPB_XSAN(a));

  size_t span = UPB_PRIVATE(_upb_Arena_AllocSpan)(size);

  if (UPB_UNLIKELY(UPB_PRIVATE(_upb_ArenaHas)(a) < span)) {
    void* UPB_PRIVATE(_upb_Arena_SlowMalloc)(struct upb_Arena * a, size_t size);
    return UPB_PRIVATE(_upb_Arena_SlowMalloc)(a, span);
  }

  // We have enough space to do a fast malloc.
  void* ret = a->UPB_ONLYBITS(ptr);
  a->UPB_ONLYBITS(ptr) += span;
  UPB_ASSERT(UPB_PRIVATE(_upb_Arena_IsAligned)(ret));
  UPB_ASSERT(UPB_PRIVATE(_upb_Arena_IsAligned)(a->UPB_ONLYBITS(ptr)));

  return UPB_PRIVATE(upb_Xsan_NewUnpoisonedRegion)(UPB_XSAN(a), ret, size);
}

UPB_API_INLINE void upb_Arena_ShrinkLast(struct upb_Arena* a, void* ptr,
                                         size_t oldsize, size_t size) {
  UPB_ASSERT(ptr);
  UPB_ASSERT(size <= oldsize);

  UPB_PRIVATE(upb_Xsan_AccessReadWrite)(UPB_XSAN(a));
  UPB_PRIVATE(upb_Xsan_ResizeUnpoisonedRegion)(ptr, oldsize, size);

  if (UPB_PRIVATE(_upb_Arena_WasLastAllocFromCurrentBlock)(a, ptr, oldsize)) {
    // We can reclaim some memory.
    a->UPB_ONLYBITS(ptr) -= UPB_ALIGN_MALLOC(oldsize) - UPB_ALIGN_MALLOC(size);
  } else {
    // We can't reclaim any memory, but we need to verify that `ptr` really
    // does represent the most recent allocation.
#ifndef NDEBUG
    bool _upb_Arena_WasLastAllocFromPreviousBlock(struct upb_Arena * a,
                                                  void* ptr, size_t oldsize);
    UPB_ASSERT(_upb_Arena_WasLastAllocFromPreviousBlock(a, ptr, oldsize));
#endif
  }
}

UPB_API_INLINE bool upb_Arena_TryExtend(struct upb_Arena* a, void* ptr,
                                        size_t oldsize, size_t size) {
  UPB_ASSERT(ptr);
  UPB_ASSERT(size > oldsize);

  UPB_PRIVATE(upb_Xsan_AccessReadWrite)(UPB_XSAN(a));
  size_t extend = UPB_ALIGN_MALLOC(size) - UPB_ALIGN_MALLOC(oldsize);

  if (UPB_PRIVATE(_upb_Arena_WasLastAllocFromCurrentBlock)(a, ptr, oldsize) &&
      UPB_PRIVATE(_upb_ArenaHas)(a) >= extend) {
    a->UPB_ONLYBITS(ptr) += extend;
    UPB_PRIVATE(upb_Xsan_ResizeUnpoisonedRegion)(ptr, oldsize, size);
    return true;
  }

  return false;
}

UPB_API_INLINE void* upb_Arena_Realloc(struct upb_Arena* a, void* ptr,
                                       size_t oldsize, size_t size) {
  UPB_PRIVATE(upb_Xsan_AccessReadWrite)(UPB_XSAN(a));

  void* ret;

  if (ptr && (size <= oldsize || upb_Arena_TryExtend(a, ptr, oldsize, size))) {
    // We can extend or shrink in place.
    if (size <= oldsize &&
        UPB_PRIVATE(_upb_Arena_WasLastAllocFromCurrentBlock)(a, ptr, oldsize)) {
      upb_Arena_ShrinkLast(a, ptr, oldsize, size);
    }
    ret = ptr;
  } else {
    // We need to copy into a new allocation.
    ret = upb_Arena_Malloc(a, size);
    if (ret && oldsize > 0) {
      memcpy(ret, ptr, UPB_MIN(oldsize, size));
    }
  }

  if (ret) {
    // We want to invalidate pointers to the old region if hwasan is enabled, so
    // we poison and unpoison even if ptr == ret. However, if reallocation fails
    // we do not want to poison the old memory, or attempt to poison null.
    UPB_PRIVATE(upb_Xsan_PoisonRegion)(ptr, oldsize);
    return UPB_PRIVATE(upb_Xsan_NewUnpoisonedRegion)(UPB_XSAN(a), ret, size);
  }
  return ret;
}

// Returns the next block size to allocate for the arena based on exponential
// growth and size hint.
size_t UPB_PRIVATE(_upb_Arena_NextBlockSize)(struct upb_Arena* a, size_t span,
                                             bool* one_off);

// Updates the arena's growth state based on the block size actually allocated.
void UPB_PRIVATE(_upb_Arena_UpdateGrowthState)(struct upb_Arena* a, size_t span,
                                               size_t block_size, bool one_off);

// Allocates a block for the arena of at least the given size, but does not add
// it to the arena. The block must either be added to the arena or manually
// freed, otherwise memory will be leaked.
//
// Returns the allocated block (or NULL on failure), and writes the actual size
// of the block to size.
void* UPB_PRIVATE(_upb_Arena_AllocBlock)(struct upb_Arena* a, size_t* size);

// Adds a block previously allocated with _upb_Arena_AllocBlock() to the arena.
// This will cause it to be owned by the arena and freed when the arena is
// freed.
//
// Note that this call does *not* cause the block to be used for arena
// allocations. Call _upb_Arena_UseBlock() to do that.
//
// This operation cannot be undone, so the caller should not call it until they
// are sure that the block will be useful to the arena.
void UPB_PRIVATE(_upb_Arena_AddBlock)(struct upb_Arena* a, void* block);

// Frees a block previously allocated with _upb_Arena_AllocBlock. This is only
// necessary if the block ends up not being useful to the arena.
void UPB_PRIVATE(_upb_Arena_FreeBlock)(struct upb_Arena* a, void* block);

// Sets the arena's current block to the given block. Subsequent allocations
// may be made from this block.
//
// The given memory must be either:
// - The arena block most recently returned by _upb_Arena_AllocBlock, or
// - A block that was just stolen from the arena using _upb_Arena_Steal.
//
// After this call, the memory may only be used by the arena -- it is poisoned
// against further use by the caller.
//
// Note: if the arena determines that this block is smaller than the block it
// currently has, it may decide to not use the block.
void UPB_PRIVATE(_upb_Arena_UseBlock)(struct upb_Arena* a, void* ptr,
                                      size_t size);

// Steals all available memory from the current arena block, but only if at
// least `size` bytes are available. The number of bytes stolen is written to
// size. The memory will be unpoisoned and ready for use.
void* UPB_PRIVATE(_upb_Arena_Steal)(struct upb_Arena* a, size_t* size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MEM_INTERNAL_ARENA_H_ */
