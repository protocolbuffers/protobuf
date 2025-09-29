// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_PORT_SANITIZERS_H_
#define UPB_PORT_SANITIZERS_H_

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// Must be last.
#include "upb/port/def.inc"

// Must be inside def.inc/undef.inc
#if UPB_HWASAN
#include <sanitizer/hwasan_interface.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// UPB_ARENA_SIZE_HACK depends on this struct having size 1.
typedef struct {
  uint8_t state;
} upb_Xsan;

UPB_INLINE uint8_t _upb_Xsan_NextTag(upb_Xsan *xsan) {
#if UPB_HWASAN
  xsan->state++;
  if (xsan->state <= UPB_HWASAN_POISON_TAG) {
    xsan->state = UPB_HWASAN_POISON_TAG + 1;
  }
  return xsan->state;
#else
  return 0;
#endif
}

enum {
#if UPB_ASAN
  UPB_PRIVATE(kUpb_Asan_GuardSize) = 32,
#else
  UPB_PRIVATE(kUpb_Asan_GuardSize) = 0,
#endif
};

UPB_INLINE uint8_t UPB_PRIVATE(_upb_Xsan_GetTag)(const void *addr) {
#if UPB_HWASAN
  return __hwasan_get_tag_from_pointer(addr);
#else
  return 0;
#endif
}

UPB_INLINE void UPB_PRIVATE(upb_Xsan_Init)(upb_Xsan *xsan) {
#if UPB_HWASAN || UPB_TSAN
  xsan->state = 0;
#endif
}

// Marks the given region as poisoned, meaning that it is not accessible until
// it is unpoisoned.
UPB_INLINE void UPB_PRIVATE(upb_Xsan_PoisonRegion)(const void *addr,
                                                   size_t size) {
#if UPB_ASAN
  void __asan_poison_memory_region(void const volatile *addr, size_t size);
  __asan_poison_memory_region(addr, size);
#elif UPB_HWASAN
  __hwasan_tag_memory(addr, UPB_HWASAN_POISON_TAG, UPB_ALIGN_MALLOC(size));
#endif
}

UPB_INLINE void *UPB_PRIVATE(_upb_Xsan_UnpoisonRegion)(void *addr, size_t size,
                                                       uint8_t tag) {
#if UPB_ASAN
  UPB_UNUSED(tag);
  void __asan_unpoison_memory_region(void const volatile *addr, size_t size);
  __asan_unpoison_memory_region(addr, size);
  return addr;
#elif UPB_HWASAN
  __hwasan_tag_memory(addr, tag, UPB_ALIGN_MALLOC(size));
  return __hwasan_tag_pointer(addr, tag);
#else
  UPB_UNUSED(size);
  UPB_UNUSED(tag);

  // `addr` is the pointer that will be returned from arena alloc/realloc
  // functions.  In this code-path we know it must be non-NULL, but the compiler
  // doesn't know this unless we add a UPB_ASSUME() annotation.
  //
  // This will let the optimizer optimize away NULL-checks if it can see that
  // this path was taken.
  UPB_ASSUME(addr);
  return addr;
#endif
}

// Allows users to read and write to the given region, which will be considered
// distinct from other regions and may only be accessed through the returned
// pointer.
//
// `addr` must be aligned to the malloc alignment.  Size may be unaligned,
// and with ASAN we can respect `size` precisely, but with HWASAN we must
// round `size` up to the next multiple of the malloc alignment, so the caller
// must guarantee that rounding up `size` will not cause overlap with other
// regions.
UPB_INLINE void *UPB_PRIVATE(upb_Xsan_NewUnpoisonedRegion)(upb_Xsan *xsan,
                                                           void *addr,
                                                           size_t size) {
  return UPB_PRIVATE(_upb_Xsan_UnpoisonRegion)(addr, size,
                                               _upb_Xsan_NextTag(xsan));
}

// Resizes the given region to a new size, *without* invalidating any existing
// pointers to the region.
//
// `tagged_addr` must be a pointer that was previously returned from
// `upb_Xsan_NewUnpoisonedRegion`.  `old_size` must be the size that was
// originally passed to `upb_Xsan_NewUnpoisonedRegion`.
UPB_INLINE void *UPB_PRIVATE(upb_Xsan_ResizeUnpoisonedRegion)(void *tagged_addr,
                                                              size_t old_size,
                                                              size_t new_size) {
  UPB_PRIVATE(upb_Xsan_PoisonRegion)(tagged_addr, old_size);
  return UPB_PRIVATE(_upb_Xsan_UnpoisonRegion)(
      tagged_addr, new_size, UPB_PRIVATE(_upb_Xsan_GetTag)(tagged_addr));
}

// Compares two pointers and returns true if they are equal. This returns the
// correct result even if one or both of the pointers are tagged.
UPB_INLINE bool UPB_PRIVATE(upb_Xsan_PtrEq)(const void *a, const void *b) {
#if UPB_HWASAN
  return __hwasan_tag_pointer(a, 0) == __hwasan_tag_pointer(b, 0);
#else
  return a == b;
#endif
}

// These annotations improve TSAN's ability to detect data races.  By
// proactively accessing a non-atomic variable at the point where it is
// "logically" accessed, we can trigger TSAN diagnostics that might have
// otherwise been masked by subsequent atomic operations.

UPB_INLINE void UPB_PRIVATE(upb_Xsan_AccessReadOnly)(upb_Xsan *xsan) {
#if UPB_TSAN
  // For performance we avoid using a volatile variable.
  __asm__ volatile("" ::"r"(xsan->state));
#endif
}

UPB_INLINE void UPB_PRIVATE(upb_Xsan_AccessReadWrite)(upb_Xsan *xsan) {
#if UPB_TSAN
  // For performance we avoid using a volatile variable.
  __asm__ volatile("" : "+r"(xsan->state));
#endif
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif  // UPB_PORT_SANITIZERS_H_
