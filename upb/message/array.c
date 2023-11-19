// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/array.h"

#include <stdint.h>
#include <string.h>

#include "upb/base/descriptor_constants.h"
#include "upb/mem/arena.h"
#include "upb/message/internal/array.h"
#include "upb/mini_table/internal/size_log2.h"

// Must be last.
#include "upb/port/def.inc"

upb_Array* upb_Array_New(upb_Arena* a, upb_CType type) {
  return _upb_Array_New(a, 4, upb_CType_SizeLg2(type));
}

const void* upb_Array_DataPtr(const upb_Array* arr) {
  return _upb_array_ptr((upb_Array*)arr);
}

void* upb_Array_MutableDataPtr(upb_Array* arr) { return _upb_array_ptr(arr); }

size_t upb_Array_Size(const upb_Array* arr) { return arr->size; }

upb_MessageValue upb_Array_Get(const upb_Array* arr, size_t i) {
  upb_MessageValue ret;
  const char* data = _upb_array_constptr(arr);
  const int lg2 = _upb_Array_ElemSizeLg2(arr);
  UPB_ASSERT(i < arr->size);
  memcpy(&ret, data + (i << lg2), 1 << lg2);
  return ret;
}

void upb_Array_Set(upb_Array* arr, size_t i, upb_MessageValue val) {
  char* data = _upb_array_ptr(arr);
  const int lg2 = _upb_Array_ElemSizeLg2(arr);
  UPB_ASSERT(i < arr->size);
  memcpy(data + (i << lg2), &val, 1 << lg2);
}

bool upb_Array_Append(upb_Array* arr, upb_MessageValue val, upb_Arena* arena) {
  UPB_ASSERT(arena);
  if (!_upb_Array_ResizeUninitialized(arr, arr->size + 1, arena)) {
    return false;
  }
  upb_Array_Set(arr, arr->size - 1, val);
  return true;
}

void upb_Array_Move(upb_Array* arr, size_t dst_idx, size_t src_idx,
                    size_t count) {
  const int lg2 = _upb_Array_ElemSizeLg2(arr);
  char* data = _upb_array_ptr(arr);
  memmove(&data[dst_idx << lg2], &data[src_idx << lg2], count << lg2);
}

bool upb_Array_Insert(upb_Array* arr, size_t i, size_t count,
                      upb_Arena* arena) {
  UPB_ASSERT(arena);
  UPB_ASSERT(i <= arr->size);
  UPB_ASSERT(count + arr->size >= count);
  const size_t oldsize = arr->size;
  if (!_upb_Array_ResizeUninitialized(arr, arr->size + count, arena)) {
    return false;
  }
  upb_Array_Move(arr, i + count, i, oldsize - i);
  return true;
}

/*
 *              i        end      arr->size
 * |------------|XXXXXXXX|--------|
 */
void upb_Array_Delete(upb_Array* arr, size_t i, size_t count) {
  const size_t end = i + count;
  UPB_ASSERT(i <= end);
  UPB_ASSERT(end <= arr->size);
  upb_Array_Move(arr, i, end, arr->size - end);
  arr->size -= count;
}

bool upb_Array_Resize(upb_Array* arr, size_t size, upb_Arena* arena) {
  const size_t oldsize = arr->size;
  if (UPB_UNLIKELY(!_upb_Array_ResizeUninitialized(arr, size, arena))) {
    return false;
  }
  const size_t newsize = arr->size;
  if (newsize > oldsize) {
    const int lg2 = _upb_Array_ElemSizeLg2(arr);
    char* data = _upb_array_ptr(arr);
    memset(data + (oldsize << lg2), 0, (newsize - oldsize) << lg2);
  }
  return true;
}

// EVERYTHING BELOW THIS LINE IS INTERNAL - DO NOT USE /////////////////////////

bool _upb_array_realloc(upb_Array* arr, size_t min_capacity, upb_Arena* arena) {
  size_t new_capacity = UPB_MAX(arr->capacity, 4);
  const int lg2 = _upb_Array_ElemSizeLg2(arr);
  size_t old_bytes = arr->capacity << lg2;
  void* ptr = _upb_array_ptr(arr);

  // Log2 ceiling of size.
  while (new_capacity < min_capacity) new_capacity *= 2;

  const size_t new_bytes = new_capacity << lg2;
  ptr = upb_Arena_Realloc(arena, ptr, old_bytes, new_bytes);
  if (!ptr) return false;

  _upb_Array_SetTaggedPtr(arr, ptr, lg2);
  arr->capacity = new_capacity;
  return true;
}
