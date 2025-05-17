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
#ifndef __STDC_NO_THREADS__
#include <threads.h>
#endif

#include "upb/port/sanitizers.h"

// Must be last.
#include "upb/port/def.inc"

// This is QUITE an ugly hack, which specifies the number of pointers needed
// to equal (or exceed) the storage required for one upb_Arena.
//
// We need this because the decoder inlines a upb_Arena for performance but
// the full struct is not visible outside of arena.c. Yes, I know, it's awful.
#define UPB_ARENA_SIZE_HACK (10 + (UPB_XSAN_STRUCT_SIZE * 2))

// LINT.IfChange(upb_Arena)

struct upb_Arena {
  char* UPB_ONLYBITS(ptr);
  char* UPB_ONLYBITS(end);
#ifndef __STDC_NO_THREADS__
  UPB_ATOMIC(thrd_t) tid;
#endif
  UPB_XSAN_MEMBER
};

// LINT.ThenChange(//depot/google3/third_party/upb/bits/typescript/arena.ts:upb_Arena)

#ifdef __cplusplus
extern "C" {
#endif

// enterTid() and exitTid() are used to compare the current thread ID to the
// thread ID stored in the arena during allocations. If they are not the same
// during arena malloc/realloc, the arena is being used by another thread and
// should crash.
#ifndef __STDC_NO_THREADS__
static thrd_t enterTid(struct upb_Arena* a) {
  thrd_t t = thrd_current();
  thrd_t old = upb_Atomic_Exchange(&a->tid, t, memory_order_relaxed);
  if (old != t && old != 0) {
    __builtin_trap();
  }
  return old;
}

static void exitTid(struct upb_Arena* a, uintptr_t tid) {
  thrd_t old = upb_Atomic_Exchange(&a->tid, tid, memory_order_relaxed);
  if (old != thrd_current()) {
    __builtin_trap();
  }
}
#else
// No-op for non-threaded builds.
static uintptr_t enterTid(struct upb_Arena* a) { return 0; }
static void exitTid(struct upb_Arena* a, uintptr_t tid) {}
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
  uintptr_t tid = enterTid(a);
  UPB_PRIVATE(upb_Xsan_AccessReadWrite)(UPB_XSAN(a));

  size_t span = UPB_PRIVATE(_upb_Arena_AllocSpan)(size);

  if (UPB_UNLIKELY(UPB_PRIVATE(_upb_ArenaHas)(a) < span)) {
    void* UPB_PRIVATE(_upb_Arena_SlowMalloc)(struct upb_Arena * a, size_t size);
    void* ret = UPB_PRIVATE(_upb_Arena_SlowMalloc)(a, span);
    exitTid(a, tid);
    return ret;
  }

  // We have enough space to do a fast malloc.
  void* ret = a->UPB_ONLYBITS(ptr);
  a->UPB_ONLYBITS(ptr) += span;
  UPB_ASSERT(UPB_PRIVATE(_upb_Arena_IsAligned)(ret));
  UPB_ASSERT(UPB_PRIVATE(_upb_Arena_IsAligned)(a->UPB_ONLYBITS(ptr)));
  exitTid(a, tid);
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
    bool _upb_Arena_WasLastAlloc(struct upb_Arena * a, void* ptr,
                                 size_t oldsize);
    UPB_ASSERT(_upb_Arena_WasLastAlloc(a, ptr, oldsize));
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
  uintptr_t tid = enterTid(a);
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

  // We want to invalidate pointers to the old region if hwasan is enabled, so
  // we poison and unpoison even if ptr == ret.
  UPB_PRIVATE(upb_Xsan_PoisonRegion)(ptr, oldsize);
  void* ret_final =
      UPB_PRIVATE(upb_Xsan_NewUnpoisonedRegion)(UPB_XSAN(a), ret, size);
  exitTid(a, tid);
  return ret_final;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MEM_INTERNAL_ARENA_H_ */
