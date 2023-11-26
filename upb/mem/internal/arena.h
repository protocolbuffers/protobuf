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

// Must be last.
#include "upb/port/def.inc"

typedef struct _upb_MemBlock _upb_MemBlock;

struct upb_Arena {
  char* UPB_PRIVATE(ptr);
  char* UPB_PRIVATE(end);

  // upb_alloc* together with a low bit which signals if there is an initial
  // block.
  uintptr_t UPB_PRIVATE(block_alloc);

  // When multiple arenas are fused together, each arena points to a parent
  // arena (root points to itself). The root tracks how many live arenas
  // reference it.

  // The low bit is tagged:
  //   0: pointer to parent
  //   1: count, left shifted by one
  UPB_ATOMIC(uintptr_t) UPB_PRIVATE(parent_or_count);

  // All nodes that are fused together are in a singly-linked list.
  // == NULL at end of list.
  UPB_ATOMIC(struct upb_Arena*) UPB_PRIVATE(next);

  // The last element of the linked list. This is present only as an
  // optimization, so that we do not have to iterate over all members for every
  // fuse. Only significant for an arena root. In other cases it is ignored.
  // == self when no other list members.
  UPB_ATOMIC(struct upb_Arena*) UPB_PRIVATE(tail);

  // Linked list of blocks to free/cleanup.  Atomic only for the benefit of
  // upb_Arena_SpaceAllocated().
  UPB_ATOMIC(_upb_MemBlock*) UPB_PRIVATE(blocks);
};

#ifdef __cplusplus
extern "C" {
#endif

UPB_INLINE size_t UPB_PRIVATE(_upb_ArenaHas)(const struct upb_Arena* a) {
  return (size_t)(a->UPB_PRIVATE(end) - a->UPB_PRIVATE(ptr));
}

UPB_INLINE void* UPB_PRIVATE(_upb_Arena_Malloc)(struct upb_Arena* a,
                                                size_t size) {
  void* UPB_PRIVATE(_upb_Arena_SlowMalloc)(struct upb_Arena * a, size_t size);

  size = UPB_ALIGN_MALLOC(size);
  size_t span = size + UPB_ASAN_GUARD_SIZE;
  if (UPB_UNLIKELY(UPB_PRIVATE(_upb_ArenaHas)(a) < span)) {
    return UPB_PRIVATE(_upb_Arena_SlowMalloc)(a, size);
  }

  // We have enough space to do a fast malloc.
  void* ret = a->UPB_PRIVATE(ptr);
  UPB_ASSERT(UPB_ALIGN_MALLOC((uintptr_t)ret) == (uintptr_t)ret);
  UPB_ASSERT(UPB_ALIGN_MALLOC(size) == size);
  UPB_UNPOISON_MEMORY_REGION(ret, size);

  a->UPB_PRIVATE(ptr) += span;

  return ret;
}

UPB_INLINE void* UPB_PRIVATE(_upb_Arena_Realloc)(struct upb_Arena* a, void* ptr,
                                                 size_t oldsize, size_t size) {
  oldsize = UPB_ALIGN_MALLOC(oldsize);
  size = UPB_ALIGN_MALLOC(size);
  bool is_most_recent_alloc =
      (uintptr_t)ptr + oldsize == (uintptr_t)a->UPB_PRIVATE(ptr);

  if (is_most_recent_alloc) {
    ptrdiff_t diff = size - oldsize;
    if ((ptrdiff_t)UPB_PRIVATE(_upb_ArenaHas)(a) >= diff) {
      a->UPB_PRIVATE(ptr) += diff;
      return ptr;
    }
  } else if (size <= oldsize) {
    return ptr;
  }

  void* ret = UPB_PRIVATE(_upb_Arena_Malloc)(a, size);

  if (ret && oldsize > 0) {
    memcpy(ret, ptr, UPB_MIN(oldsize, size));
  }

  return ret;
}

UPB_INLINE void UPB_PRIVATE(_upb_Arena_ShrinkLast)(struct upb_Arena* a,
                                                   void* ptr, size_t oldsize,
                                                   size_t size) {
  oldsize = UPB_ALIGN_MALLOC(oldsize);
  size = UPB_ALIGN_MALLOC(size);
  // Must be the last alloc.
  UPB_ASSERT((char*)ptr + oldsize == a->UPB_PRIVATE(ptr) - UPB_ASAN_GUARD_SIZE);
  UPB_ASSERT(size <= oldsize);
  a->UPB_PRIVATE(ptr) = (char*)ptr + size;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MEM_INTERNAL_ARENA_H_ */
