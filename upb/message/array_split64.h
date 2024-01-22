// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MESSAGE_ARRAY_SPLIT64_H_
#define UPB_MESSAGE_ARRAY_SPLIT64_H_

#include "upb/message/array.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

// JavaScript doesn't directly support 64-bit ints so we must split them.

UPB_API_INLINE uint32_t upb_Array_GetInt64Hi(const upb_Array* array, size_t i) {
  return (uint32_t)(upb_Array_Get(array, i).int64_val >> 32);
}

UPB_API_INLINE uint32_t upb_Array_GetInt64Lo(const upb_Array* array, size_t i) {
  return (uint32_t)upb_Array_Get(array, i).int64_val;
}

UPB_API_INLINE void upb_Array_SetInt64Split(upb_Array* array, size_t i,
                                            uint32_t hi, uint32_t lo) {
  const upb_MessageValue val = {.int64_val = ((int64_t)hi) << 32 | lo};
  upb_Array_Set(array, i, val);
}

UPB_API_INLINE bool upb_Array_AppendInt64Split(upb_Array* array, uint32_t hi,
                                               uint32_t lo, upb_Arena* arena) {
  const upb_MessageValue val = {.int64_val = ((int64_t)hi) << 32 | lo};
  return upb_Array_Append(array, val, arena);
}

UPB_API_INLINE uint32_t upb_Array_GetUInt64Hi(const upb_Array* array,
                                              size_t i) {
  return (uint32_t)(upb_Array_Get(array, i).uint64_val >> 32);
}

UPB_API_INLINE uint32_t upb_Array_GetUInt64Lo(const upb_Array* array,
                                              size_t i) {
  return (uint32_t)upb_Array_Get(array, i).uint64_val;
}

UPB_API_INLINE void upb_Array_SetUInt64Split(upb_Array* array, size_t i,
                                             uint32_t hi, uint32_t lo) {
  const upb_MessageValue val = {.uint64_val = ((uint64_t)hi) << 32 | lo};
  upb_Array_Set(array, i, val);
}

UPB_API_INLINE bool upb_Array_AppendUInt64Split(upb_Array* array, uint32_t hi,
                                                uint32_t lo, upb_Arena* arena) {
  const upb_MessageValue val = {.uint64_val = ((uint64_t)hi) << 32 | lo};
  return upb_Array_Append(array, val, arena);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MESSAGE_ARRAY_SPLIT64_H_ */
