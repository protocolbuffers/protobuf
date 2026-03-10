// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MESSAGE_INTERNAL_ARRAY_H_
#define UPB_MESSAGE_INTERNAL_ARRAY_H_

#include <stdint.h>
#include <string.h>

#include "upb/mem/arena.h"

// Must be last.
#include "upb/port/def.inc"

#define _UPB_ARRAY_MASK_IMM 0x4  // Frozen/immutable bit.
#define _UPB_ARRAY_MASK_LG2 0x3  // Encoded elem size.
#define _UPB_ARRAY_MASK_ALL (_UPB_ARRAY_MASK_IMM | _UPB_ARRAY_MASK_LG2)

#ifdef __cplusplus
extern "C" {
#endif

// LINT.IfChange(upb_Array)

// Our internal representation for repeated fields.
struct upb_Array {
  // This is a tagged pointer. Bits #0 and #1 encode the elem size as follows:
  //   0 maps to elem size 1
  //   1 maps to elem size 4
  //   2 maps to elem size 8
  //   3 maps to elem size 16
  //
  // Bit #2 contains the frozen/immutable flag.
  uintptr_t UPB_ONLYBITS(data);

  size_t UPB_ONLYBITS(size);     // The number of elements in the array.
  size_t UPB_PRIVATE(capacity);  // Allocated storage. Measured in elements.
};

UPB_INLINE void UPB_PRIVATE(_upb_Array_ShallowFreeze)(struct upb_Array* arr) {
  arr->UPB_ONLYBITS(data) |= _UPB_ARRAY_MASK_IMM;
}

UPB_API_INLINE bool upb_Array_IsFrozen(const struct upb_Array* arr) {
  return (arr->UPB_ONLYBITS(data) & _UPB_ARRAY_MASK_IMM) != 0;
}

UPB_INLINE void UPB_PRIVATE(_upb_Array_SetTaggedPtr)(struct upb_Array* array,
                                                     void* data, size_t lg2) {
  UPB_ASSERT(lg2 != 1);
  UPB_ASSERT(lg2 <= 4);
  const size_t bits = lg2 - (lg2 != 0);
  array->UPB_ONLYBITS(data) = (uintptr_t)data | bits;
}

UPB_INLINE size_t
UPB_PRIVATE(_upb_Array_ElemSizeLg2)(const struct upb_Array* array) {
  const size_t bits = array->UPB_ONLYBITS(data) & _UPB_ARRAY_MASK_LG2;
  const size_t lg2 = bits + (bits != 0);
  return lg2;
}

UPB_API_INLINE const void* upb_Array_DataPtr(const struct upb_Array* array) {
  UPB_PRIVATE(_upb_Array_ElemSizeLg2)(array);  // Check assertions.
  return (void*)(array->UPB_ONLYBITS(data) & ~(uintptr_t)_UPB_ARRAY_MASK_ALL);
}

UPB_API_INLINE void* upb_Array_MutableDataPtr(struct upb_Array* array) {
  return (void*)upb_Array_DataPtr(array);
}

UPB_INLINE struct upb_Array* UPB_PRIVATE(_upb_Array_NewMaybeAllowSlow)(
    upb_Arena* arena, size_t init_capacity, int elem_size_lg2,
    bool allow_slow) {
  UPB_ASSERT(elem_size_lg2 != 1);
  UPB_ASSERT(elem_size_lg2 <= 4);
  const size_t array_size =
      UPB_ALIGN_UP(sizeof(struct upb_Array), UPB_MALLOC_ALIGN);
  const size_t bytes = array_size + (init_capacity << elem_size_lg2);
  size_t span = UPB_PRIVATE(_upb_Arena_AllocSpan)(bytes);
  if (!allow_slow && UPB_PRIVATE(_upb_ArenaHas)(arena) < span) return NULL;
  struct upb_Array* array = (struct upb_Array*)upb_Arena_Malloc(arena, bytes);
  if (!array) return NULL;
  UPB_PRIVATE(_upb_Array_SetTaggedPtr)
  (array, UPB_PTR_AT(array, array_size, void), elem_size_lg2);
  array->UPB_ONLYBITS(size) = 0;
  array->UPB_PRIVATE(capacity) = init_capacity;
  return array;
}

UPB_INLINE struct upb_Array* UPB_PRIVATE(_upb_Array_New)(upb_Arena* arena,
                                                         size_t init_capacity,
                                                         int elem_size_lg2) {
  return UPB_PRIVATE(_upb_Array_NewMaybeAllowSlow)(arena, init_capacity,
                                                   elem_size_lg2, true);
}

UPB_INLINE struct upb_Array* UPB_PRIVATE(_upb_Array_TryFastNew)(
    upb_Arena* arena, size_t init_capacity, int elem_size_lg2) {
  return UPB_PRIVATE(_upb_Array_NewMaybeAllowSlow)(arena, init_capacity,
                                                   elem_size_lg2, false);
}

// Resizes the capacity of the array to be at least min_size.
bool UPB_PRIVATE(_upb_Array_Realloc)(struct upb_Array* array, size_t min_size,
                                     upb_Arena* arena);

UPB_FORCEINLINE
bool UPB_PRIVATE(_upb_Array_TryFastRealloc)(struct upb_Array* array,
                                            size_t capacity, int elem_size_lg2,
                                            upb_Arena* arena) {
  size_t old_bytes = array->UPB_PRIVATE(capacity) << elem_size_lg2;
  size_t new_bytes = capacity << elem_size_lg2;
  UPB_ASSUME(new_bytes > old_bytes);
  if (!upb_Arena_TryExtend(arena, array, old_bytes, new_bytes)) return false;
  array->UPB_PRIVATE(capacity) = capacity;
  return true;
}

UPB_API_INLINE bool upb_Array_Reserve(struct upb_Array* array, size_t size,
                                      upb_Arena* arena) {
  UPB_ASSERT(!upb_Array_IsFrozen(array));
  if (array->UPB_PRIVATE(capacity) < size)
    return UPB_PRIVATE(_upb_Array_Realloc)(array, size, arena);
  return true;
}

// Resize without initializing new elements.
UPB_INLINE bool UPB_PRIVATE(_upb_Array_ResizeUninitialized)(
    struct upb_Array* array, size_t size, upb_Arena* arena) {
  UPB_ASSERT(!upb_Array_IsFrozen(array));
  UPB_ASSERT(size <= array->UPB_ONLYBITS(size) ||
             arena);  // Allow NULL arena when shrinking.
  if (!upb_Array_Reserve(array, size, arena)) return false;
  array->UPB_ONLYBITS(size) = size;
  return true;
}

// This function is intended for situations where elem_size is compile-time
// constant or a known expression of the form (1 << lg2), so that the expression
// i*elem_size does not result in an actual multiplication.
UPB_INLINE void UPB_PRIVATE(_upb_Array_Set)(struct upb_Array* array, size_t i,
                                            const void* data,
                                            size_t elem_size) {
  UPB_ASSERT(!upb_Array_IsFrozen(array));
  UPB_ASSERT(i < array->UPB_ONLYBITS(size));
  UPB_ASSERT(elem_size == 1U << UPB_PRIVATE(_upb_Array_ElemSizeLg2)(array));
  char* arr_data = (char*)upb_Array_MutableDataPtr(array);
  memcpy(arr_data + (i * elem_size), data, elem_size);
}

UPB_API_INLINE size_t upb_Array_Size(const struct upb_Array* arr) {
  return arr->UPB_ONLYBITS(size);
}

UPB_API_INLINE size_t upb_Array_Capacity(const struct upb_Array* arr) {
  return arr->UPB_PRIVATE(capacity);
}

// LINT.ThenChange(GoogleInternalName0)

#ifdef __cplusplus
} /* extern "C" */
#endif

#undef _UPB_ARRAY_MASK_IMM
#undef _UPB_ARRAY_MASK_LG2
#undef _UPB_ARRAY_MASK_ALL

#include "upb/port/undef.inc"

#endif /* UPB_MESSAGE_INTERNAL_ARRAY_H_ */
