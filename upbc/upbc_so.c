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
#include "upb/message/accessors.h"
#include "upb/message/message.h"
#include "upb/mini_table/decode.h"
#include "upbc/upbdev.h"

// Must be last.
#include "upb/port/def.inc"

UPB_API bool upb_Array_AppendBool(upb_Array* array, bool val,
                                  upb_Arena* arena) {
  const upb_MessageValue mv = {.bool_val = val};
  return upb_Array_Append(array, mv, arena);
}

UPB_API bool upb_Array_AppendDouble(upb_Array* array, double val,
                                    upb_Arena* arena) {
  const upb_MessageValue mv = {.double_val = val};
  return upb_Array_Append(array, mv, arena);
}

UPB_API bool upb_Array_AppendFloat(upb_Array* array, float val,
                                   upb_Arena* arena) {
  const upb_MessageValue mv = {.float_val = val};
  return upb_Array_Append(array, mv, arena);
}

UPB_API bool upb_Array_AppendInt32(upb_Array* array, int32_t val,
                                   upb_Arena* arena) {
  const upb_MessageValue mv = {.int32_val = val};
  return upb_Array_Append(array, mv, arena);
}

UPB_API bool upb_Array_AppendUInt32(upb_Array* array, uint32_t val,
                                    upb_Arena* arena) {
  const upb_MessageValue mv = {.uint32_val = val};
  return upb_Array_Append(array, mv, arena);
}

////////////////////////////////////////////////////////////////////////////////

UPB_API void upb_Array_SetBool(upb_Array* array, size_t i, bool val) {
  const upb_MessageValue mv = {.bool_val = val};
  upb_Array_Set(array, i, mv);
}

UPB_API void upb_Array_SetDouble(upb_Array* array, size_t i, double val) {
  const upb_MessageValue mv = {.double_val = val};
  upb_Array_Set(array, i, mv);
}

UPB_API void upb_Array_SetFloat(upb_Array* array, size_t i, float val) {
  const upb_MessageValue mv = {.float_val = val};
  upb_Array_Set(array, i, mv);
}

UPB_API void upb_Array_SetInt32(upb_Array* array, size_t i, int32_t val) {
  const upb_MessageValue mv = {.int32_val = val};
  upb_Array_Set(array, i, mv);
}

UPB_API void upb_Array_SetUInt32(upb_Array* array, size_t i, uint32_t val) {
  const upb_MessageValue mv = {.uint32_val = val};
  upb_Array_Set(array, i, mv);
}
