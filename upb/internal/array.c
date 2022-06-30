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

#include "upb/internal/array.h"

#include <string.h>

// Must be last.
#include "upb/port_def.inc"

bool _upb_array_realloc(upb_Array* arr, size_t min_capacity, upb_Arena* arena) {
  size_t new_capacity = UPB_MAX(arr->capacity, 4);
  int elem_size_lg2 = arr->data & 7;
  size_t old_bytes = arr->capacity << elem_size_lg2;
  size_t new_bytes;
  void* ptr = _upb_array_ptr(arr);

  /* Log2 ceiling of size. */
  while (new_capacity < min_capacity) new_capacity *= 2;

  new_bytes = new_capacity << elem_size_lg2;
  ptr = upb_Arena_Realloc(arena, ptr, old_bytes, new_bytes);

  if (!ptr) {
    return false;
  }

  arr->data = _upb_tag_arrptr(ptr, elem_size_lg2);
  arr->capacity = new_capacity;
  return true;
}

static upb_Array* getorcreate_array(upb_Array** arr_ptr, int elem_size_lg2,
                                    upb_Arena* arena) {
  upb_Array* arr = *arr_ptr;
  if (!arr) {
    arr = _upb_Array_New(arena, 4, elem_size_lg2);
    if (!arr) return NULL;
    *arr_ptr = arr;
  }
  return arr;
}

void* _upb_Array_Resize_fallback(upb_Array** arr_ptr, size_t size,
                                 int elem_size_lg2, upb_Arena* arena) {
  upb_Array* arr = getorcreate_array(arr_ptr, elem_size_lg2, arena);
  return arr && _upb_Array_Resize(arr, size, arena) ? _upb_array_ptr(arr)
                                                    : NULL;
}

bool _upb_Array_Append_fallback(upb_Array** arr_ptr, const void* value,
                                int elem_size_lg2, upb_Arena* arena) {
  upb_Array* arr = getorcreate_array(arr_ptr, elem_size_lg2, arena);
  if (!arr) return false;

  size_t elems = arr->size;

  if (!_upb_Array_Resize(arr, elems + 1, arena)) {
    return false;
  }

  char* data = _upb_array_ptr(arr);
  memcpy(data + (elems << elem_size_lg2), value, 1 << elem_size_lg2);
  return true;
}
