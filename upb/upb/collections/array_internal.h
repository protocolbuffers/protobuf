/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UPB_INTERNAL_ARRAY_INTERNAL_H_
#define UPB_INTERNAL_ARRAY_INTERNAL_H_

#include <string.h>

#include "upb/collections/array.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

// LINT.IfChange(struct_definition)
// Our internal representation for repeated fields.
struct upb_Array {
  uintptr_t data;  /* Tagged ptr: low 3 bits of ptr are lg2(elem size). */
  size_t size;     /* The number of elements in the array. */
  size_t capacity; /* Allocated storage. Measured in elements. */
};
// LINT.ThenChange(GoogleInternalName1)

UPB_INLINE size_t _upb_Array_ElementSizeLg2(const upb_Array* arr) {
  size_t ret = arr->data & 7;
  UPB_ASSERT(ret <= 4);
  return ret;
}

UPB_INLINE const void* _upb_array_constptr(const upb_Array* arr) {
  _upb_Array_ElementSizeLg2(arr);  // Check assertion.
  return (void*)(arr->data & ~(uintptr_t)7);
}

UPB_INLINE uintptr_t _upb_array_tagptr(void* ptr, int elem_size_lg2) {
  UPB_ASSERT(elem_size_lg2 <= 4);
  return (uintptr_t)ptr | elem_size_lg2;
}

UPB_INLINE void* _upb_array_ptr(upb_Array* arr) {
  return (void*)_upb_array_constptr(arr);
}

UPB_INLINE uintptr_t _upb_tag_arrptr(void* ptr, int elem_size_lg2) {
  UPB_ASSERT(elem_size_lg2 <= 4);
  UPB_ASSERT(((uintptr_t)ptr & 7) == 0);
  return (uintptr_t)ptr | (unsigned)elem_size_lg2;
}

extern const char _upb_Array_CTypeSizeLg2Table[];

UPB_INLINE size_t _upb_Array_CTypeSizeLg2(upb_CType ctype) {
  return _upb_Array_CTypeSizeLg2Table[ctype];
}

UPB_INLINE upb_Array* _upb_Array_New(upb_Arena* a, size_t init_capacity,
                                     int elem_size_lg2) {
  UPB_ASSERT(elem_size_lg2 <= 4);
  const size_t arr_size = UPB_ALIGN_UP(sizeof(upb_Array), UPB_MALLOC_ALIGN);
  const size_t bytes = arr_size + (init_capacity << elem_size_lg2);
  upb_Array* arr = (upb_Array*)upb_Arena_Malloc(a, bytes);
  if (!arr) return NULL;
  arr->data = _upb_tag_arrptr(UPB_PTR_AT(arr, arr_size, void), elem_size_lg2);
  arr->size = 0;
  arr->capacity = init_capacity;
  return arr;
}

// Resizes the capacity of the array to be at least min_size.
bool _upb_array_realloc(upb_Array* arr, size_t min_size, upb_Arena* arena);

UPB_INLINE bool _upb_array_reserve(upb_Array* arr, size_t size,
                                   upb_Arena* arena) {
  if (arr->capacity < size) return _upb_array_realloc(arr, size, arena);
  return true;
}

// Resize without initializing new elements.
UPB_INLINE bool _upb_Array_ResizeUninitialized(upb_Array* arr, size_t size,
                                               upb_Arena* arena) {
  UPB_ASSERT(size <= arr->size || arena);  // Allow NULL arena when shrinking.
  if (!_upb_array_reserve(arr, size, arena)) return false;
  arr->size = size;
  return true;
}

// This function is intended for situations where elem_size is compile-time
// constant or a known expression of the form (1 << lg2), so that the expression
// i*elem_size does not result in an actual multiplication.
UPB_INLINE void _upb_Array_Set(upb_Array* arr, size_t i, const void* data,
                               size_t elem_size) {
  UPB_ASSERT(i < arr->size);
  UPB_ASSERT(elem_size == 1U << _upb_Array_ElementSizeLg2(arr));
  char* arr_data = (char*)_upb_array_ptr(arr);
  memcpy(arr_data + (i * elem_size), data, elem_size);
}

UPB_INLINE void _upb_array_detach(const void* msg, size_t ofs) {
  *UPB_PTR_AT(msg, ofs, upb_Array*) = NULL;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_INTERNAL_ARRAY_INTERNAL_H_ */
