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

#define UPB_ARENA_SPAN(x) ((x) + UPB_ASAN_GUARD_SIZE)

// LINT.IfChange(upb_Arena)

struct upb_Arena {
  char* UPB_ONLYBITS(ptr);
  char* UPB_ONLYBITS(end);
  struct upb_ArenaInternal* internal;  // No need to make this UPB_PRIVATE()
};

// LINT.ThenChange(//depot/google3/third_party/upb/bits/typescript/arena.ts:upb_Array)

#ifdef __cplusplus
extern "C" {
#endif

bool UPB_PRIVATE(_upb_Arena_AllocBlock)(struct upb_Arena* a, size_t size);
uint32_t UPB_PRIVATE(_upb_Arena_DebugRefCount)(struct upb_Arena* a);

UPB_INLINE size_t UPB_PRIVATE(_upb_ArenaHas)(struct upb_Arena* a) {
  const size_t ptr = (size_t)a->UPB_ONLYBITS(ptr);
  const size_t end = (size_t)a->UPB_ONLYBITS(end);
  UPB_ASSERT(end >= ptr);
  return end - ptr;
}

UPB_INLINE void* UPB_PRIVATE(_upb_Arena_Malloc)(struct upb_Arena* a,
                                                size_t size) {
  const size_t aligned_size = UPB_ALIGN_MALLOC(size);
  const size_t span = UPB_ARENA_SPAN(aligned_size);

  if (UPB_PRIVATE(_upb_ArenaHas)(a) < span) {
    if (!UPB_PRIVATE(_upb_Arena_AllocBlock)(a, span)) return NULL;  // OOM
  }

  // We have enough space to do a fast malloc.
  void* const out = a->UPB_ONLYBITS(ptr);
  UPB_ASSERT(UPB_ALIGN_MALLOC((uintptr_t)out) == (uintptr_t)out);
  UPB_UNPOISON_MEMORY_REGION(out, aligned_size);
  a->UPB_ONLYBITS(ptr) += span;

  return out;
}

// Returns whether ['ptr', 'size'] was the most recent allocation on 'a'.
// 'size` must already have been aligned with UPB_ALIGN_MALLOC()
UPB_INLINE bool UPB_PRIVATE(_upb_Arena_IsMostRecent)(const struct upb_Arena* a,
                                                     void* ptr, size_t size) {
  UPB_ASSERT(size == UPB_ALIGN_MALLOC(size));
  return (char*)ptr + UPB_ARENA_SPAN(size) == a->UPB_ONLYBITS(ptr);
}

UPB_INLINE void* UPB_PRIVATE(_upb_Arena_Realloc)(struct upb_Arena* a, void* ptr,
                                                 size_t oldsize, size_t size) {
  const size_t aligned_oldsize = UPB_ALIGN_MALLOC(oldsize);
  const size_t aligned_newsize = UPB_ALIGN_MALLOC(size);
  if (UPB_UNLIKELY(aligned_newsize <= aligned_oldsize)) return ptr;

  const bool recent =
      UPB_PRIVATE(_upb_Arena_IsMostRecent)(a, ptr, aligned_oldsize);

  if (recent) {
    size_t diff = aligned_newsize - aligned_oldsize;
    if (UPB_PRIVATE(_upb_ArenaHas)(a) >= diff) {
      a->UPB_ONLYBITS(ptr) += diff;
      return ptr;
    }
  }

  void* out = UPB_PRIVATE(_upb_Arena_Malloc)(a, size);
  if (out) memcpy(out, ptr, oldsize);
  return out;
}

UPB_INLINE void UPB_PRIVATE(_upb_Arena_ShrinkLast)(struct upb_Arena* a,
                                                   void* ptr, size_t oldsize,
                                                   size_t size) {
  const size_t aligned_oldsize = UPB_ALIGN_MALLOC(oldsize);
  const size_t aligned_newsize = UPB_ALIGN_MALLOC(size);

  UPB_ASSERT(UPB_PRIVATE(_upb_Arena_IsMostRecent)(a, ptr, aligned_oldsize));
  UPB_ASSERT(aligned_newsize <= aligned_oldsize);

  a->UPB_ONLYBITS(ptr) -= aligned_oldsize - aligned_newsize;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#undef UPB_ARENA_SPAN

#include "upb/port/undef.inc"

#endif /* UPB_MEM_INTERNAL_ARENA_H_ */
