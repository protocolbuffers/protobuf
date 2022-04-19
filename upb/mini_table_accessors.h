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

#ifndef UPB_MINI_TABLE_ACCESSORS_H_
#define UPB_MINI_TABLE_ACCESSORS_H_

#include "upb/mini_table_accessors_internal.h"
#include "upb/msg_internal.h"

// Must be last.
#include "upb/port_def.inc"

#ifdef __cplusplus
extern "C" {
#endif

bool upb_MiniTable_HasField(const upb_Message *msg,
                            const upb_MiniTable_Field *field);

void upb_MiniTable_ClearField(upb_Message *msg,
                              const upb_MiniTable_Field *field);

UPB_INLINE bool upb_MiniTable_GetBool(const upb_Message *msg,
                                      const upb_MiniTable_Field *field) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Bool);
  return *UPB_PTR_AT(msg, field->offset, bool);
}

UPB_INLINE void upb_MiniTable_SetBool(upb_Message *msg,
                                      const upb_MiniTable_Field *field,
                                      bool value) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Bool);
  _upb_MiniTable_SetPresence(msg, field);
  *UPB_PTR_AT(msg, field->offset, bool) = value;
}

UPB_INLINE int32_t upb_MiniTable_GetInt32(const upb_Message *msg,
                                          const upb_MiniTable_Field *field) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Int32 ||
             field->descriptortype == kUpb_FieldType_SInt32 ||
             field->descriptortype == kUpb_FieldType_SFixed32);
  return *UPB_PTR_AT(msg, field->offset, int32_t);
}

UPB_INLINE void upb_MiniTable_SetInt32(upb_Message *msg,
                                       const upb_MiniTable_Field *field,
                                       int32_t value) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Int32 ||
             field->descriptortype == kUpb_FieldType_SInt32 ||
             field->descriptortype == kUpb_FieldType_SFixed32);
  _upb_MiniTable_SetPresence(msg, field);
  *UPB_PTR_AT(msg, field->offset, int32_t) = value;
}

// TODO(ferhat): Add accessors for all proto field types.

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port_undef.inc"

#endif  // UPB_MINI_TABLE_ACCESSORS_H_
