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
#include "upb/message/message.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/size_log2.h"
#include "upb/mini_table/message.h"

// Must be last.
#include "upb/port/def.inc"

upb_Array* upb_Array_New(upb_Arena* a, upb_CType type) {
  const int lg2 = UPB_PRIVATE(_upb_CType_SizeLg2)(type);
  return UPB_PRIVATE(_upb_Array_New)(a, 4, lg2);
}

upb_MessageValue upb_Array_Get(const upb_Array* arr, size_t i) {
  UPB_ASSERT(i < upb_Array_Size(arr));
  upb_MessageValue ret;
  const char* data = upb_Array_DataPtr(arr);
  const int lg2 = UPB_PRIVATE(_upb_Array_ElemSizeLg2)(arr);
  memcpy(&ret, data + (i << lg2), 1 << lg2);
  return ret;
}

upb_MutableMessageValue upb_Array_GetMutable(upb_Array* arr, size_t i) {
  UPB_ASSERT(i < upb_Array_Size(arr));
  upb_MutableMessageValue ret;
  char* data = upb_Array_MutableDataPtr(arr);
  const int lg2 = UPB_PRIVATE(_upb_Array_ElemSizeLg2)(arr);
  memcpy(&ret, data + (i << lg2), 1 << lg2);
  return ret;
}

void upb_Array_Set(upb_Array* arr, size_t i, upb_MessageValue val) {
  UPB_ASSERT(!upb_Array_IsFrozen(arr));
  UPB_ASSERT(i < upb_Array_Size(arr));
  char* data = upb_Array_MutableDataPtr(arr);
  const int lg2 = UPB_PRIVATE(_upb_Array_ElemSizeLg2)(arr);
  memcpy(data + (i << lg2), &val, 1 << lg2);
}

bool upb_Array_Append(upb_Array* arr, upb_MessageValue val, upb_Arena* arena) {
  UPB_ASSERT(!upb_Array_IsFrozen(arr));
  UPB_ASSERT(arena);
  if (!UPB_PRIVATE(_upb_Array_ResizeUninitialized)(
          arr, arr->UPB_PRIVATE(size) + 1, arena)) {
    return false;
  }
  upb_Array_Set(arr, arr->UPB_PRIVATE(size) - 1, val);
  return true;
}

void upb_Array_Move(upb_Array* arr, size_t dst_idx, size_t src_idx,
                    size_t count) {
  UPB_ASSERT(!upb_Array_IsFrozen(arr));
  const int lg2 = UPB_PRIVATE(_upb_Array_ElemSizeLg2)(arr);
  char* data = upb_Array_MutableDataPtr(arr);
  memmove(&data[dst_idx << lg2], &data[src_idx << lg2], count << lg2);
}

bool upb_Array_Insert(upb_Array* arr, size_t i, size_t count,
                      upb_Arena* arena) {
  UPB_ASSERT(!upb_Array_IsFrozen(arr));
  UPB_ASSERT(arena);
  UPB_ASSERT(i <= arr->UPB_PRIVATE(size));
  UPB_ASSERT(count + arr->UPB_PRIVATE(size) >= count);
  const size_t oldsize = arr->UPB_PRIVATE(size);
  if (!UPB_PRIVATE(_upb_Array_ResizeUninitialized)(
          arr, arr->UPB_PRIVATE(size) + count, arena)) {
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
  UPB_ASSERT(!upb_Array_IsFrozen(arr));
  const size_t end = i + count;
  UPB_ASSERT(i <= end);
  UPB_ASSERT(end <= arr->UPB_PRIVATE(size));
  upb_Array_Move(arr, i, end, arr->UPB_PRIVATE(size) - end);
  arr->UPB_PRIVATE(size) -= count;
}

bool upb_Array_Resize(upb_Array* arr, size_t size, upb_Arena* arena) {
  UPB_ASSERT(!upb_Array_IsFrozen(arr));
  const size_t oldsize = arr->UPB_PRIVATE(size);
  if (UPB_UNLIKELY(
          !UPB_PRIVATE(_upb_Array_ResizeUninitialized)(arr, size, arena))) {
    return false;
  }
  const size_t newsize = arr->UPB_PRIVATE(size);
  if (newsize > oldsize) {
    const int lg2 = UPB_PRIVATE(_upb_Array_ElemSizeLg2)(arr);
    char* data = upb_Array_MutableDataPtr(arr);
    memset(data + (oldsize << lg2), 0, (newsize - oldsize) << lg2);
  }
  return true;
}

bool UPB_PRIVATE(_upb_Array_Realloc)(upb_Array* array, size_t min_capacity,
                                     upb_Arena* arena) {
  size_t new_capacity = UPB_MAX(array->UPB_PRIVATE(capacity), 4);
  const int lg2 = UPB_PRIVATE(_upb_Array_ElemSizeLg2)(array);
  size_t old_bytes = array->UPB_PRIVATE(capacity) << lg2;
  void* ptr = upb_Array_MutableDataPtr(array);

  // Log2 ceiling of size.
  while (new_capacity < min_capacity) new_capacity *= 2;

  const size_t new_bytes = new_capacity << lg2;
  ptr = upb_Arena_Realloc(arena, ptr, old_bytes, new_bytes);
  if (!ptr) return false;

  UPB_PRIVATE(_upb_Array_SetTaggedPtr)(array, ptr, lg2);
  array->UPB_PRIVATE(capacity) = new_capacity;
  return true;
}

void upb_Array_Freeze(upb_Array* arr, const upb_MiniTable* m) {
  if (upb_Array_IsFrozen(arr)) return;
  UPB_PRIVATE(_upb_Array_ShallowFreeze)(arr);

  if (m) {
    const size_t size = upb_Array_Size(arr);

    for (size_t i = 0; i < size; i++) {
      upb_MessageValue val = upb_Array_Get(arr, i);
      upb_Message_Freeze((upb_Message*)val.msg_val, m);
    }
  }
}
