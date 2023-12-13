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

#include "upb/mem/alloc.h"

// Must be last.
#include "upb/port/def.inc"

// LINT.IfChange(upb_Arena)

struct upb_Arena {
  char* UPB_ONLYBITS(ptr);
  char* UPB_ONLYBITS(end);
  struct upb_ArenaInternal* internal;  // no need for UPB_PRIVATE() here
};

// LINT.ThenChange(//depot/google3/third_party/upb/bits/typescript/arena.ts:upb_Arena)

#ifdef __cplusplus
extern "C" {
#endif

UPB_INLINE size_t UPB_PRIVATE(_upb_ArenaHas)(const struct upb_Arena* a) {
  return (size_t)(a->UPB_ONLYBITS(end) - a->UPB_ONLYBITS(ptr));
}

UPB_INLINE void* UPB_PRIVATE(_upb_Arena_Malloc)(struct upb_Arena* a,
                                                size_t size) {
  void* UPB_PRIVATE(_upb_Arena_SlowMalloc)(struct upb_Arena * a, size_t size);

  size = UPB_ALIGN_MALLOC(size);
  const size_t span = size + UPB_ASAN_GUARD_SIZE;
  if (UPB_UNLIKELY(UPB_PRIVATE(_upb_ArenaHas)(a) < span)) {
    return UPB_PRIVATE(_upb_Arena_SlowMalloc)(a, span);
  }

  // We have enough space to do a fast malloc.
  void* ret = a->UPB_ONLYBITS(ptr);
  UPB_ASSERT(UPB_ALIGN_MALLOC((uintptr_t)ret) == (uintptr_t)ret);
  UPB_ASSERT(UPB_ALIGN_MALLOC(size) == size);
  UPB_UNPOISON_MEMORY_REGION(ret, size);

  a->UPB_ONLYBITS(ptr) += span;

  return ret;
}

UPB_INLINE void* UPB_PRIVATE(_upb_Arena_Realloc)(struct upb_Arena* a, void* ptr,
                                                 size_t oldsize, size_t size) {
  oldsize = UPB_ALIGN_MALLOC(oldsize);
  size = UPB_ALIGN_MALLOC(size);
  bool is_most_recent_alloc =
      (uintptr_t)ptr + oldsize == (uintptr_t)a->UPB_ONLYBITS(ptr);

  if (is_most_recent_alloc) {
    ptrdiff_t diff = size - oldsize;
    if ((ptrdiff_t)UPB_PRIVATE(_upb_ArenaHas)(a) >= diff) {
      a->UPB_ONLYBITS(ptr) += diff;
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
  UPB_ASSERT((char*)ptr + oldsize ==
             a->UPB_ONLYBITS(ptr) - UPB_ASAN_GUARD_SIZE);
  UPB_ASSERT(size <= oldsize);
  a->UPB_ONLYBITS(ptr) = (char*)ptr + size;
}

UPB_INLINE struct upb_Arena* UPB_PRIVATE(_upb_Arena_New)(void) {
  struct upb_Arena* upb_Arena_Init(void* mem, size_t n, upb_alloc* alloc);

  return upb_Arena_Init(NULL, 0, &upb_alloc_global);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MEM_INTERNAL_ARENA_H_ */
