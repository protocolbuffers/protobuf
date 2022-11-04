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
#include "upb/port_def.inc"

#ifdef __cplusplus
extern "C" {
#endif

// Our internal representation for repeated fields.
struct upb_Array {
  uintptr_t data;  /* Tagged ptr: low 3 bits of ptr are lg2(elem size). */
  size_t size;     /* The number of elements in the array. */
  size_t capacity; /* Allocated storage. Measured in elements. */
};

UPB_INLINE const void* _upb_array_constptr(const upb_Array* arr) {
  UPB_ASSERT((arr->data & 7) <= 4);
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

UPB_INLINE upb_Array* _upb_Array_New(upb_Arena* a, size_t init_capacity,
                                     int elem_size_lg2) {
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

// Fallback functions for when the accessors require a resize.
void* _upb_Array_Resize_fallback(upb_Array** arr_ptr, size_t size,
                                 int elem_size_lg2, upb_Arena* arena);
bool _upb_Array_Append_fallback(upb_Array** arr_ptr, const void* value,
                                int elem_size_lg2, upb_Arena* arena);

UPB_INLINE bool _upb_array_reserve(upb_Array* arr, size_t size,
                                   upb_Arena* arena) {
  if (arr->capacity < size) return _upb_array_realloc(arr, size, arena);
  return true;
}

UPB_INLINE bool _upb_Array_Resize(upb_Array* arr, size_t size,
                                  upb_Arena* arena) {
  if (!_upb_array_reserve(arr, size, arena)) return false;
  arr->size = size;
  return true;
}

UPB_INLINE void _upb_array_detach(const void* msg, size_t ofs) {
  *UPB_PTR_AT(msg, ofs, upb_Array*) = NULL;
}

UPB_INLINE const void* _upb_array_accessor(const void* msg, size_t ofs,
                                           size_t* size) {
  const upb_Array* arr = *UPB_PTR_AT(msg, ofs, const upb_Array*);
  if (arr) {
    if (size) *size = arr->size;
    return _upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}

UPB_INLINE void* _upb_array_mutable_accessor(void* msg, size_t ofs,
                                             size_t* size) {
  upb_Array* arr = *UPB_PTR_AT(msg, ofs, upb_Array*);
  if (arr) {
    if (size) *size = arr->size;
    return _upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}

UPB_INLINE void* _upb_Array_Resize_accessor2(void* msg, size_t ofs, size_t size,
                                             int elem_size_lg2,
                                             upb_Arena* arena) {
  upb_Array** arr_ptr = UPB_PTR_AT(msg, ofs, upb_Array*);
  upb_Array* arr = *arr_ptr;
  if (!arr || arr->capacity < size) {
    return _upb_Array_Resize_fallback(arr_ptr, size, elem_size_lg2, arena);
  }
  arr->size = size;
  return _upb_array_ptr(arr);
}

UPB_INLINE bool _upb_Array_Append_accessor2(void* msg, size_t ofs,
                                            int elem_size_lg2,
                                            const void* value,
                                            upb_Arena* arena) {
  upb_Array** arr_ptr = UPB_PTR_AT(msg, ofs, upb_Array*);
  size_t elem_size = 1 << elem_size_lg2;
  upb_Array* arr = *arr_ptr;
  void* ptr;
  if (!arr || arr->size == arr->capacity) {
    return _upb_Array_Append_fallback(arr_ptr, value, elem_size_lg2, arena);
  }
  ptr = _upb_array_ptr(arr);
  memcpy(UPB_PTR_AT(ptr, arr->size * elem_size, char), value, elem_size);
  arr->size++;
  return true;
}

// Used by old generated code, remove once all code has been regenerated.
UPB_INLINE int _upb_sizelg2(upb_CType type) {
  switch (type) {
    case kUpb_CType_Bool:
      return 0;
    case kUpb_CType_Float:
    case kUpb_CType_Int32:
    case kUpb_CType_UInt32:
    case kUpb_CType_Enum:
      return 2;
    case kUpb_CType_Message:
      return UPB_SIZE(2, 3);
    case kUpb_CType_Double:
    case kUpb_CType_Int64:
    case kUpb_CType_UInt64:
      return 3;
    case kUpb_CType_String:
    case kUpb_CType_Bytes:
      return UPB_SIZE(3, 4);
  }
  UPB_UNREACHABLE();
}

UPB_INLINE void* _upb_Array_Resize_accessor(void* msg, size_t ofs, size_t size,
                                            upb_CType type, upb_Arena* arena) {
  return _upb_Array_Resize_accessor2(msg, ofs, size, _upb_sizelg2(type), arena);
}

UPB_INLINE bool _upb_Array_Append_accessor(void* msg, size_t ofs,
                                           size_t elem_size, upb_CType type,
                                           const void* value,
                                           upb_Arena* arena) {
  (void)elem_size;
  return _upb_Array_Append_accessor2(msg, ofs, _upb_sizelg2(type), value,
                                     arena);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port_undef.inc"

#endif /* UPB_INTERNAL_ARRAY_INTERNAL_H_ */
