// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google LLC nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "upb/message/internal/array.h"

#include <string.h>

// Must be last.
#include "upb/port/def.inc"

const char _upb_Array_CTypeSizeLg2Table[] = {
    [kUpb_CType_Bool] = 0,
    [kUpb_CType_Float] = 2,
    [kUpb_CType_Int32] = 2,
    [kUpb_CType_UInt32] = 2,
    [kUpb_CType_Enum] = 2,
    [kUpb_CType_Message] = UPB_SIZE(2, 3),
    [kUpb_CType_Double] = 3,
    [kUpb_CType_Int64] = 3,
    [kUpb_CType_UInt64] = 3,
    [kUpb_CType_String] = UPB_SIZE(3, 4),
    [kUpb_CType_Bytes] = UPB_SIZE(3, 4),
};

upb_Array* upb_Array_New(upb_Arena* a, upb_CType type) {
  return _upb_Array_New(a, 4, _upb_Array_CTypeSizeLg2(type));
}

const void* upb_Array_DataPtr(const upb_Array* arr) {
  return _upb_array_ptr((upb_Array*)arr);
}

void* upb_Array_MutableDataPtr(upb_Array* arr) { return _upb_array_ptr(arr); }

size_t upb_Array_Size(const upb_Array* arr) { return arr->size; }

upb_MessageValue upb_Array_Get(const upb_Array* arr, size_t i) {
  upb_MessageValue ret;
  const char* data = _upb_array_constptr(arr);
  int lg2 = arr->data & 7;
  UPB_ASSERT(i < arr->size);
  memcpy(&ret, data + (i << lg2), 1 << lg2);
  return ret;
}

void upb_Array_Set(upb_Array* arr, size_t i, upb_MessageValue val) {
  char* data = _upb_array_ptr(arr);
  int lg2 = arr->data & 7;
  UPB_ASSERT(i < arr->size);
  memcpy(data + (i << lg2), &val, 1 << lg2);
}

bool upb_Array_Append(upb_Array* arr, upb_MessageValue val, upb_Arena* arena) {
  UPB_ASSERT(arena);
  if (!upb_Array_Resize(arr, arr->size + 1, arena)) {
    return false;
  }
  upb_Array_Set(arr, arr->size - 1, val);
  return true;
}

void upb_Array_Move(upb_Array* arr, size_t dst_idx, size_t src_idx,
                    size_t count) {
  const int lg2 = arr->data & 7;
  char* data = _upb_array_ptr(arr);
  memmove(&data[dst_idx << lg2], &data[src_idx << lg2], count << lg2);
}

bool upb_Array_Insert(upb_Array* arr, size_t i, size_t count,
                      upb_Arena* arena) {
  UPB_ASSERT(arena);
  UPB_ASSERT(i <= arr->size);
  UPB_ASSERT(count + arr->size >= count);
  const size_t oldsize = arr->size;
  if (!upb_Array_Resize(arr, arr->size + count, arena)) {
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
    const int lg2 = arr->data & 7;
    char* data = _upb_array_ptr(arr);
    memset(data + (oldsize << lg2), 0, (newsize - oldsize) << lg2);
  }
  return true;
}

// EVERYTHING BELOW THIS LINE IS INTERNAL - DO NOT USE /////////////////////////

bool _upb_array_realloc(upb_Array* arr, size_t min_capacity, upb_Arena* arena) {
  size_t new_capacity = UPB_MAX(arr->capacity, 4);
  int elem_size_lg2 = arr->data & 7;
  size_t old_bytes = arr->capacity << elem_size_lg2;
  size_t new_bytes;
  void* ptr = _upb_array_ptr(arr);

  // Log2 ceiling of size.
  while (new_capacity < min_capacity) new_capacity *= 2;

  new_bytes = new_capacity << elem_size_lg2;
  ptr = upb_Arena_Realloc(arena, ptr, old_bytes, new_bytes);
  if (!ptr) return false;

  arr->data = _upb_tag_arrptr(ptr, elem_size_lg2);
  arr->capacity = new_capacity;
  return true;
}
