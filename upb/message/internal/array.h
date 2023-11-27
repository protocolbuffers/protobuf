// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MESSAGE_INTERNAL_ARRAY_H_
#define UPB_MESSAGE_INTERNAL_ARRAY_H_

#include <string.h>

#include "upb/message/array.h"

// Must be last.
#include "upb/port/def.inc"

#define _UPB_ARRAY_MASK_IMM 0x4  // Frozen/immutable bit.
#define _UPB_ARRAY_MASK_LG2 0x3  // Encoded elem size.
#define _UPB_ARRAY_MASK_ALL (_UPB_ARRAY_MASK_IMM | _UPB_ARRAY_MASK_LG2)

#ifdef __cplusplus
extern "C" {
#endif

// LINT.IfChange(struct_definition)
// Our internal representation for repeated fields.
struct upb_Array {
  // This is a tagged pointer. Bits #0 and #1 encode the elem size as follows:
  //   0 maps to elem size 1
  //   1 maps to elem size 4
  //   2 maps to elem size 8
  //   3 maps to elem size 16
  //
  // Bit #2 contains the frozen/immutable flag (currently unimplemented).
  uintptr_t data;

  size_t size;                   // The number of elements in the array.
  size_t UPB_PRIVATE(capacity);  // Allocated storage. Measured in elements.
};
// LINT.ThenChange(GoogleInternalName1)

UPB_INLINE void UPB_PRIVATE(_upb_Array_SetTaggedPtr)(upb_Array* array,
                                                     void* data, size_t lg2) {
  UPB_ASSERT(lg2 != 1);
  UPB_ASSERT(lg2 <= 4);
  const size_t bits = lg2 - (lg2 != 0);
  array->data = (uintptr_t)data | bits;
}

UPB_INLINE size_t UPB_PRIVATE(_upb_Array_ElemSizeLg2)(const upb_Array* array) {
  const size_t bits = array->data & _UPB_ARRAY_MASK_LG2;
  const size_t lg2 = bits + (bits != 0);
  return lg2;
}

UPB_INLINE const void* _upb_array_constptr(const upb_Array* array) {
  UPB_PRIVATE(_upb_Array_ElemSizeLg2)(array);  // Check assertions.
  return (void*)(array->data & ~(uintptr_t)_UPB_ARRAY_MASK_ALL);
}

UPB_INLINE void* _upb_array_ptr(upb_Array* array) {
  return (void*)_upb_array_constptr(array);
}

UPB_INLINE upb_Array* UPB_PRIVATE(_upb_Array_New)(upb_Arena* arena,
                                                  size_t init_capacity,
                                                  int elem_size_lg2) {
  UPB_ASSERT(elem_size_lg2 != 1);
  UPB_ASSERT(elem_size_lg2 <= 4);
  const size_t array_size = UPB_ALIGN_UP(sizeof(upb_Array), UPB_MALLOC_ALIGN);
  const size_t bytes = array_size + (init_capacity << elem_size_lg2);
  upb_Array* array = (upb_Array*)upb_Arena_Malloc(arena, bytes);
  if (!array) return NULL;
  UPB_PRIVATE(_upb_Array_SetTaggedPtr)
  (array, UPB_PTR_AT(array, array_size, void), elem_size_lg2);
  array->size = 0;
  array->UPB_PRIVATE(capacity) = init_capacity;
  return array;
}

// Resizes the capacity of the array to be at least min_size.
bool UPB_PRIVATE(_upb_Array_Realloc)(upb_Array* array, size_t min_size,
                                     upb_Arena* arena);

UPB_INLINE bool UPB_PRIVATE(_upb_Array_Reserve)(upb_Array* array, size_t size,
                                                upb_Arena* arena) {
  if (array->UPB_PRIVATE(capacity) < size)
    return UPB_PRIVATE(_upb_Array_Realloc)(array, size, arena);
  return true;
}

// Resize without initializing new elements.
UPB_INLINE bool _upb_Array_ResizeUninitialized(upb_Array* array, size_t size,
                                               upb_Arena* arena) {
  UPB_ASSERT(size <= array->size || arena);  // Allow NULL arena when shrinking.
  if (!UPB_PRIVATE(_upb_Array_Reserve)(array, size, arena)) return false;
  array->size = size;
  return true;
}

// This function is intended for situations where elem_size is compile-time
// constant or a known expression of the form (1 << lg2), so that the expression
// i*elem_size does not result in an actual multiplication.
UPB_INLINE void UPB_PRIVATE(_upb_Array_Set)(upb_Array* array, size_t i,
                                            const void* data,
                                            size_t elem_size) {
  UPB_ASSERT(i < array->size);
  UPB_ASSERT(elem_size == 1U << UPB_PRIVATE(_upb_Array_ElemSizeLg2)(array));
  char* arr_data = (char*)_upb_array_ptr(array);
  memcpy(arr_data + (i * elem_size), data, elem_size);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#undef _UPB_ARRAY_MASK_IMM
#undef _UPB_ARRAY_MASK_LG2
#undef _UPB_ARRAY_MASK_ALL

#include "upb/port/undef.inc"

#endif /* UPB_MESSAGE_INTERNAL_ARRAY_H_ */
