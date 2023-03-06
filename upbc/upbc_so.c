/*
 * Copyright (c) 2009-2022, Google LLC
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

// This disables inlining and forces all public functions to be exported to the
// linker. It is used to generate bindings for FFIs from other languages.
#ifndef UPB_BUILD_API
#define UPB_BUILD_API
#endif

#include "upb/collections/array.h"
#include "upb/collections/map.h"
#include "upb/message/accessors.h"
#include "upb/message/message.h"
#include "upb/mini_table/decode.h"
#include "upbc/upbdev.h"

// Must be last.
#include "upb/port/def.inc"

// JavaScript doesn't directly support 64-bit ints so we must split them.

UPB_API_INLINE uint32_t upb_Array_GetInt64Hi(const upb_Array* array, size_t i) {
  return (uint32_t)(upb_Array_Get(array, i).int64_val >> 32);
}

UPB_API_INLINE uint32_t upb_Array_GetInt64Lo(const upb_Array* array, size_t i) {
  return (uint32_t)upb_Array_Get(array, i).int64_val;
}

UPB_API_INLINE void upb_Array_SetInt64Split(upb_Array* array, size_t i,
                                            uint32_t hi, uint32_t lo) {
  const upb_MessageValue val = {.int64_val = ((uint64_t)hi) << 32 | lo};
  upb_Array_Set(array, i, val);
}

UPB_API_INLINE bool upb_Array_AppendInt64Split(upb_Array* array, uint32_t hi,
                                               uint32_t lo, upb_Arena* arena) {
  const upb_MessageValue val = {.int64_val = ((uint64_t)hi) << 32 | lo};
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

UPB_API_INLINE uint32_t upb_Message_GetInt64Hi(const upb_Message* msg,
                                               const upb_MiniTableField* field,
                                               uint32_t default_value) {
  return (uint32_t)(upb_Message_GetInt64(msg, field, default_value) >> 32);
}

UPB_API_INLINE uint32_t upb_Message_GetInt64Lo(const upb_Message* msg,
                                               const upb_MiniTableField* field,
                                               uint32_t default_value) {
  return (uint32_t)upb_Message_GetInt64(msg, field, default_value);
}

UPB_API_INLINE bool upb_Message_SetInt64Split(upb_Message* msg,
                                              const upb_MiniTableField* field,
                                              uint32_t hi, uint32_t lo,
                                              upb_Arena* arena) {
  return upb_Message_SetInt64(msg, field, ((int64_t)hi << 32) | lo, arena);
}

UPB_API_INLINE uint32_t upb_Message_GetUInt64Hi(const upb_Message* msg,
                                                const upb_MiniTableField* field,
                                                uint32_t default_value) {
  return (uint32_t)(upb_Message_GetUInt64(msg, field, default_value) >> 32);
}

UPB_API_INLINE uint32_t upb_Message_GetUInt64Lo(const upb_Message* msg,
                                                const upb_MiniTableField* field,
                                                uint32_t default_value) {
  return (uint32_t)upb_Message_GetUInt64(msg, field, default_value);
}

UPB_API_INLINE bool upb_Message_SetUInt64Split(upb_Message* msg,
                                               const upb_MiniTableField* field,
                                               uint32_t hi, uint32_t lo,
                                               upb_Arena* arena) {
  return upb_Message_SetUInt64(msg, field, ((uint64_t)hi << 32) | lo, arena);
}
