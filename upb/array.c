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

#include "upb/array.h"

#include <string.h>

#include "upb/internal/table.h"
#include "upb/msg.h"
#include "upb/port_def.inc"

static const char _upb_CTypeo_sizelg2[12] = {
    0,
    0,              /* kUpb_CType_Bool */
    2,              /* kUpb_CType_Float */
    2,              /* kUpb_CType_Int32 */
    2,              /* kUpb_CType_UInt32 */
    2,              /* kUpb_CType_Enum */
    UPB_SIZE(2, 3), /* kUpb_CType_Message */
    3,              /* kUpb_CType_Double */
    3,              /* kUpb_CType_Int64 */
    3,              /* kUpb_CType_UInt64 */
    UPB_SIZE(3, 4), /* kUpb_CType_String */
    UPB_SIZE(3, 4), /* kUpb_CType_Bytes */
};

upb_Array* upb_Array_New(upb_Arena* a, upb_CType type) {
  return _upb_Array_New(a, 4, _upb_CTypeo_sizelg2[type]);
}

size_t upb_Array_Size(const upb_Array* arr) { return arr->len; }

upb_MessageValue upb_Array_Get(const upb_Array* arr, size_t i) {
  upb_MessageValue ret;
  const char* data = _upb_array_constptr(arr);
  int lg2 = arr->data & 7;
  UPB_ASSERT(i < arr->len);
  memcpy(&ret, data + (i << lg2), 1 << lg2);
  return ret;
}

void upb_Array_Set(upb_Array* arr, size_t i, upb_MessageValue val) {
  char* data = _upb_array_ptr(arr);
  int lg2 = arr->data & 7;
  UPB_ASSERT(i < arr->len);
  memcpy(data + (i << lg2), &val, 1 << lg2);
}

bool upb_Array_Append(upb_Array* arr, upb_MessageValue val, upb_Arena* arena) {
  if (!upb_Array_Resize(arr, arr->len + 1, arena)) {
    return false;
  }
  upb_Array_Set(arr, arr->len - 1, val);
  return true;
}

void upb_Array_Move(upb_Array* arr, size_t dst_idx, size_t src_idx,
                    size_t count) {
  char* data = _upb_array_ptr(arr);
  int lg2 = arr->data & 7;
  memmove(&data[dst_idx << lg2], &data[src_idx << lg2], count << lg2);
}

bool upb_Array_Insert(upb_Array* arr, size_t i, size_t count,
                      upb_Arena* arena) {
  UPB_ASSERT(i <= arr->len);
  UPB_ASSERT(count + arr->len >= count);
  size_t oldsize = arr->len;
  if (!upb_Array_Resize(arr, arr->len + count, arena)) {
    return false;
  }
  upb_Array_Move(arr, i + count, i, oldsize - i);
  return true;
}

/*
 *              i        end      arr->len
 * |------------|XXXXXXXX|--------|
 */
void upb_Array_Delete(upb_Array* arr, size_t i, size_t count) {
  size_t end = i + count;
  UPB_ASSERT(i <= end);
  UPB_ASSERT(end <= arr->len);
  upb_Array_Move(arr, i, end, arr->len - end);
  arr->len -= count;
}

bool upb_Array_Resize(upb_Array* arr, size_t size, upb_Arena* arena) {
  return _upb_Array_Resize(arr, size, arena);
}
