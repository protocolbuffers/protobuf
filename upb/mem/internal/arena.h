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

#include "upb/base/internal/log2.h"
#include "upb/mem/internal/alloc.h"
#include "upb/port/sanitizers.h"

// Must be last.
#include "upb/port/def.inc"

// This is QUITE an ugly hack, which specifies the number of pointers needed
// to equal (or exceed) the storage required for one upb_Arena.
//
// We need this because the decoder inlines a upb_Arena for performance but
// the full struct is not visible outside of arena.c. Yes, I know, it's awful.
#ifndef NDEBUG
#define UPB_ARENA_BASE_SIZE_HACK 11
#else
#define UPB_ARENA_BASE_SIZE_HACK 10
#endif

#define UPB_ARENA_SIZE_HACK                                                   \
  (sizeof(void*) * (UPB_ARENA_BASE_SIZE_HACK + (UPB_XSAN_STRUCT_SIZE * 2))) + \
      (sizeof(uint32_t) * 2)

typedef struct UPB_PRIVATE(_upb_ArenaFreeBlock) {
  struct UPB_PRIVATE(_upb_ArenaFreeBlock) * UPB_PRIVATE(next);
} UPB_PRIVATE(_upb_ArenaFreeBlock);

typedef struct UPB_PRIVATE(_upb_ArenaHostBlock) {
  size_t UPB_PRIVATE(size);
  UPB_PRIVATE(_upb_ArenaFreeBlock) * UPB_PRIVATE(bins)[];
} UPB_PRIVATE(_upb_ArenaHostBlock);

// LINT.IfChange(upb_Arena)

struct upb_Arena {
  char* UPB_ONLYBITS(ptr);
  const UPB_NODEREF char* UPB_ONLYBITS(end);
  UPB_PRIVATE(_upb_ArenaHostBlock) * UPB_ONLYBITS(pool);
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

UPB_NODISCARD UPB_INLINE size_t
UPB_PRIVATE(_upb_ArenaHas)(const struct upb_Arena* a) {
  return (size_t)(a->UPB_ONLYBITS(end) - a->UPB_ONLYBITS(ptr));
}

UPB_NODISCARD UPB_INLINE size_t UPB_PRIVATE(_upb_Arena_AllocSpan)(size_t size) {
  return UPB_ALIGN_MALLOC(size) + UPB_PRIVATE(kUpb_Asan_GuardSize);
}

UPB_NODISCARD UPB_INLINE bool UPB_PRIVATE(
    _upb_Arena_WasLastAllocFromCurrentBlock)(const struct upb_Arena* a,
                                             void* ptr, size_t size) {
  return UPB_PRIVATE(upb_Xsan_PtrEq)(
      (char*)ptr + UPB_PRIVATE(_upb_Arena_AllocSpan)(size),
      a->UPB_ONLYBITS(ptr));
}

UPB_NODISCARD UPB_INLINE bool UPB_PRIVATE(_upb_Arena_IsAligned)(
    const void* ptr) {
  return (uintptr_t)ptr % UPB_MALLOC_ALIGN == 0;
}

UPB_NODISCARD UPB_API_INLINE void* _upb_Arena_Malloc_Unchecked(
    struct upb_Arena* a, size_t size) {
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

UPB_NODISCARD UPB_API_INLINE void* upb_Arena_Malloc(struct upb_Arena* a,
                                                    size_t size) {
  if (!upb_AllocationCount_IncrementAndCheck()) {
    return NULL;
  }
  return _upb_Arena_Malloc_Unchecked(a, size);
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
    // We commented out this assertion because the pool allocator reuses retired
    // blocks of different sizes, making it impossible to statically verify the
    // last allocation of a retired block using only physical block sizes,
    // without adding runtime overhead to track logical block usage.
    // UPB_ASSERT(_upb_Arena_WasLastAllocFromPreviousBlock(a, ptr, oldsize));
#endif
  }
}

UPB_NODISCARD UPB_API_INLINE bool upb_Arena_TryExtend(struct upb_Arena* a,
                                                      void* ptr, size_t oldsize,
                                                      size_t size) {
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

UPB_NODISCARD UPB_API_INLINE void* upb_Arena_Realloc(struct upb_Arena* a,
                                                     void* ptr, size_t oldsize,
                                                     size_t size) {
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

UPB_INLINE bool UPB_PRIVATE(_upb_Arena_IsValidPoolSize)(size_t size) {
  return size >= 16 && (size & (size - 1)) == 0;
}

UPB_INLINE int UPB_PRIVATE(_upb_Arena_GetNumBins)(size_t host_size) {
  if (host_size < 16) return 0;
  return upb_Log2Floor(host_size) - 4 + 1;
}

UPB_INLINE size_t UPB_PRIVATE(_upb_Arena_LargestPoolSize)(size_t size) {
  if (size < 16) return 0;
  return (size_t)1 << upb_Log2Floor(size);
}

UPB_INLINE void* UPB_PRIVATE(_upb_Arena_FindAndPopPoolBlock)(
    struct upb_Arena* a, size_t span, size_t* actual_size) {
  if (!a->UPB_ONLYBITS(pool)) return NULL;

  UPB_PRIVATE(_upb_ArenaHostBlock)* host = a->UPB_ONLYBITS(pool);
  size_t host_size = host->UPB_PRIVATE(size);
  int num_bins = UPB_PRIVATE(_upb_Arena_GetNumBins)(host_size);

  int bin = 0;
  if (span > 16) {
    bin = upb_Log2Ceiling(span) - 4;
  }
  if (bin < 0) bin = 0;
  if (bin >= num_bins) {
    return NULL;  // span exceeds host block size
  }

  // 1. Fast path: direct O(1) exact-match pop with no scanning
  if (host->UPB_PRIVATE(bins)[bin] != NULL) {
    UPB_PRIVATE(_upb_ArenaFreeBlock)* block = host->UPB_PRIVATE(bins)[bin];
    host->UPB_PRIVATE(bins)[bin] = block->UPB_PRIVATE(next);
    *actual_size = (size_t)1 << (bin + 4);
    return block;
  }

  // 2. If requested size matches host block size, pop and evacuate host block
  if (bin == num_bins - 1) {
    int next_host_bin = -1;
    for (int i = num_bins - 2; i >= 0; --i) {
      if (host->UPB_PRIVATE(bins)[i] != NULL) {
        next_host_bin = i;
        break;
      }
    }

    if (next_host_bin >= 0) {
      UPB_PRIVATE(_upb_ArenaFreeBlock)* next_block =
          host->UPB_PRIVATE(bins)[next_host_bin];
      host->UPB_PRIVATE(bins)[next_host_bin] = next_block->UPB_PRIVATE(next);

      size_t next_host_size = (size_t)1 << (next_host_bin + 4);
      int new_num_bins = UPB_PRIVATE(_upb_Arena_GetNumBins)(next_host_size);
      size_t new_header_bytes =
          sizeof(size_t) +
          new_num_bins * sizeof(UPB_PRIVATE(_upb_ArenaFreeBlock)*);
      UPB_PRIVATE(_upb_ArenaHostBlock)* new_host =
          (UPB_PRIVATE(_upb_ArenaHostBlock)*)UPB_PRIVATE(
              _upb_Xsan_UnpoisonRegion)(next_block, new_header_bytes, 0);
      new_host->UPB_PRIVATE(size) = next_host_size;

      for (int i = 0; i < new_num_bins; ++i) {
        new_host->UPB_PRIVATE(bins)[i] = host->UPB_PRIVATE(bins)[i];
      }

      a->UPB_ONLYBITS(pool) = new_host;
    } else {
      a->UPB_ONLYBITS(pool) = NULL;
    }

    *actual_size = host_size;
    return host;
  }

  return NULL;
}

UPB_API_INLINE void* upb_Arena_AllocPool(struct upb_Arena* a,
                                         size_t pool_size) {
  UPB_ASSERT(a);
  UPB_ASSERT(pool_size > 0);

  if (a->UPB_ONLYBITS(pool) &&
      UPB_PRIVATE(_upb_Arena_IsValidPoolSize)(pool_size)) {
    size_t actual_size = 0;
    void* block =
        UPB_PRIVATE(_upb_Arena_FindAndPopPoolBlock)(a, pool_size, &actual_size);
    if (block) {
      void* unpoisoned = UPB_PRIVATE(upb_Xsan_NewUnpoisonedRegion)(
          UPB_XSAN(a), block, pool_size);
      UPB_ASSERT(UPB_PRIVATE(_upb_Arena_IsAligned)(unpoisoned));
      return unpoisoned;
    }
  }

  return upb_Arena_Malloc(a, pool_size);
}

UPB_API_INLINE void upb_Arena_FreePool(struct upb_Arena* a, void* ptr,
                                       size_t pool_size) {
  UPB_ASSERT(a);
  if (!ptr) return;
  if (!UPB_PRIVATE(_upb_Arena_IsValidPoolSize)(pool_size)) return;

  UPB_ASSERT(UPB_PRIVATE(_upb_Arena_IsAligned)(ptr));

  // Poison the entire freed block first.
  UPB_PRIVATE(upb_Xsan_PoisonRegion)(ptr, pool_size);

  UPB_PRIVATE(_upb_ArenaHostBlock)* host = a->UPB_ONLYBITS(pool);

  if (host == NULL) {
    int num_bins = UPB_PRIVATE(_upb_Arena_GetNumBins)(pool_size);
    size_t header_bytes =
        sizeof(size_t) + num_bins * sizeof(UPB_PRIVATE(_upb_ArenaFreeBlock)*);
    UPB_PRIVATE(_upb_ArenaHostBlock)* new_host =
        (UPB_PRIVATE(_upb_ArenaHostBlock)*)UPB_PRIVATE(
            _upb_Xsan_UnpoisonRegion)(ptr, header_bytes, 0);
    new_host->UPB_PRIVATE(size) = pool_size;

    memset(new_host->UPB_PRIVATE(bins), 0,
           num_bins * sizeof(UPB_PRIVATE(_upb_ArenaFreeBlock)*));

    a->UPB_ONLYBITS(pool) = new_host;
    return;
  }

  if (pool_size > host->UPB_PRIVATE(size)) {
    int new_num_bins = UPB_PRIVATE(_upb_Arena_GetNumBins)(pool_size);
    int old_num_bins =
        UPB_PRIVATE(_upb_Arena_GetNumBins)(host->UPB_PRIVATE(size));
    size_t header_bytes =
        sizeof(size_t) +
        new_num_bins * sizeof(UPB_PRIVATE(_upb_ArenaFreeBlock)*);
    UPB_PRIVATE(_upb_ArenaHostBlock)* new_host =
        (UPB_PRIVATE(_upb_ArenaHostBlock)*)UPB_PRIVATE(
            _upb_Xsan_UnpoisonRegion)(ptr, header_bytes, 0);
    new_host->UPB_PRIVATE(size) = pool_size;

    memset(new_host->UPB_PRIVATE(bins), 0,
           new_num_bins * sizeof(UPB_PRIVATE(_upb_ArenaFreeBlock)*));
    for (int i = 0; i < old_num_bins; ++i) {
      new_host->UPB_PRIVATE(bins)[i] = host->UPB_PRIVATE(bins)[i];
    }

    a->UPB_ONLYBITS(pool) = new_host;

    // Free old host block into new host block (bin index = old_num_bins - 1)
    size_t old_host_size = host->UPB_PRIVATE(size);
    UPB_PRIVATE(upb_Xsan_PoisonRegion)(host, old_host_size);
    UPB_PRIVATE(_upb_ArenaFreeBlock)* old_host_block =
        (UPB_PRIVATE(_upb_ArenaFreeBlock)*)UPB_PRIVATE(
            _upb_Xsan_UnpoisonRegion)(
            host, sizeof(UPB_PRIVATE(_upb_ArenaFreeBlock)), 0);
    old_host_block->UPB_PRIVATE(next) =
        new_host->UPB_PRIVATE(bins)[old_num_bins - 1];
    new_host->UPB_PRIVATE(bins)[old_num_bins - 1] = old_host_block;
    return;
  }

  // pool_size <= host->size: push to host->bins[idx]
  int bin_idx = upb_Log2Floor(pool_size) - 4;
  UPB_PRIVATE(_upb_ArenaFreeBlock)* new_block =
      (UPB_PRIVATE(_upb_ArenaFreeBlock)*)UPB_PRIVATE(_upb_Xsan_UnpoisonRegion)(
          ptr, sizeof(UPB_PRIVATE(_upb_ArenaFreeBlock)), 0);
  new_block->UPB_PRIVATE(next) = host->UPB_PRIVATE(bins)[bin_idx];
  host->UPB_PRIVATE(bins)[bin_idx] = new_block;
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
