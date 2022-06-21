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

#include "upb/array.h"
#include "upb/internal/mini_table_accessors.h"
#include "upb/msg_internal.h"

// Must be last.
#include "upb/port_def.inc"

#ifdef __cplusplus
extern "C" {
#endif

bool upb_MiniTable_HasField(const upb_Message* msg,
                            const upb_MiniTable_Field* field);

void upb_MiniTable_ClearField(upb_Message* msg,
                              const upb_MiniTable_Field* field);

UPB_INLINE bool upb_MiniTable_GetBool(const upb_Message* msg,
                                      const upb_MiniTable_Field* field) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Bool);
  return *UPB_PTR_AT(msg, field->offset, bool);
}

UPB_INLINE void upb_MiniTable_SetBool(upb_Message* msg,
                                      const upb_MiniTable_Field* field,
                                      bool value) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Bool);
  _upb_MiniTable_SetPresence(msg, field);
  *UPB_PTR_AT(msg, field->offset, bool) = value;
}

UPB_INLINE int32_t upb_MiniTable_GetInt32(const upb_Message* msg,
                                          const upb_MiniTable_Field* field) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Int32 ||
             field->descriptortype == kUpb_FieldType_SInt32 ||
             field->descriptortype == kUpb_FieldType_SFixed32);
  return *UPB_PTR_AT(msg, field->offset, int32_t);
}

UPB_INLINE void upb_MiniTable_SetInt32(upb_Message* msg,
                                       const upb_MiniTable_Field* field,
                                       int32_t value) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Int32 ||
             field->descriptortype == kUpb_FieldType_SInt32 ||
             field->descriptortype == kUpb_FieldType_SFixed32);
  _upb_MiniTable_SetPresence(msg, field);
  *UPB_PTR_AT(msg, field->offset, int32_t) = value;
}

UPB_INLINE uint32_t upb_MiniTable_GetUInt32(const upb_Message* msg,
                                            const upb_MiniTable_Field* field) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_UInt32 ||
             field->descriptortype == kUpb_FieldType_Fixed32);
  return *UPB_PTR_AT(msg, field->offset, uint32_t);
}

UPB_INLINE void upb_MiniTable_SetUInt32(upb_Message* msg,
                                        const upb_MiniTable_Field* field,
                                        uint32_t value) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_UInt32 ||
             field->descriptortype == kUpb_FieldType_Fixed32);
  _upb_MiniTable_SetPresence(msg, field);
  *UPB_PTR_AT(msg, field->offset, uint32_t) = value;
}

UPB_INLINE int32_t upb_MiniTable_GetEnum(const upb_Message* msg,
                                         const upb_MiniTable_Field* field) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Enum);
  return *UPB_PTR_AT(msg, field->offset, int32_t);
}

UPB_INLINE void upb_MiniTable_SetEnum(upb_Message* msg,
                                      const upb_MiniTable_Field* field,
                                      int32_t value) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Enum);
  _upb_MiniTable_SetPresence(msg, field);
  *UPB_PTR_AT(msg, field->offset, int32_t) = value;
}

UPB_INLINE int64_t upb_MiniTable_GetInt64(const upb_Message* msg,
                                          const upb_MiniTable_Field* field) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Int64 ||
             field->descriptortype == kUpb_FieldType_SInt64 ||
             field->descriptortype == kUpb_FieldType_SFixed64);
  return *UPB_PTR_AT(msg, field->offset, int64_t);
}

UPB_INLINE void upb_MiniTable_SetInt64(upb_Message* msg,
                                       const upb_MiniTable_Field* field,
                                       int64_t value) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Int64 ||
             field->descriptortype == kUpb_FieldType_SInt64 ||
             field->descriptortype == kUpb_FieldType_SFixed64);
  _upb_MiniTable_SetPresence(msg, field);
  *UPB_PTR_AT(msg, field->offset, int64_t) = value;
}

UPB_INLINE uint64_t upb_MiniTable_GetUInt64(const upb_Message* msg,
                                            const upb_MiniTable_Field* field) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_UInt64 ||
             field->descriptortype == kUpb_FieldType_Fixed64);
  return *UPB_PTR_AT(msg, field->offset, uint64_t);
}

UPB_INLINE void upb_MiniTable_SetUInt64(upb_Message* msg,
                                        const upb_MiniTable_Field* field,
                                        uint64_t value) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_UInt64 ||
             field->descriptortype == kUpb_FieldType_Fixed64);
  _upb_MiniTable_SetPresence(msg, field);
  *UPB_PTR_AT(msg, field->offset, uint64_t) = value;
}

UPB_INLINE float upb_MiniTable_GetFloat(const upb_Message* msg,
                                        const upb_MiniTable_Field* field) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Float);
  return *UPB_PTR_AT(msg, field->offset, float);
}

UPB_INLINE void upb_MiniTable_SetFloat(upb_Message* msg,
                                       const upb_MiniTable_Field* field,
                                       float value) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Float);
  _upb_MiniTable_SetPresence(msg, field);
  *UPB_PTR_AT(msg, field->offset, float) = value;
}

UPB_INLINE double upb_MiniTable_GetDouble(const upb_Message* msg,
                                          const upb_MiniTable_Field* field) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Double);
  return *UPB_PTR_AT(msg, field->offset, double);
}

UPB_INLINE void upb_MiniTable_SetDouble(upb_Message* msg,
                                        const upb_MiniTable_Field* field,
                                        double value) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Double);
  _upb_MiniTable_SetPresence(msg, field);
  *UPB_PTR_AT(msg, field->offset, double) = value;
}

UPB_INLINE upb_StringView upb_MiniTable_GetString(
    const upb_Message* msg, const upb_MiniTable_Field* field) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Bytes ||
             field->descriptortype == kUpb_FieldType_String);
  return *UPB_PTR_AT(msg, field->offset, upb_StringView);
}

UPB_INLINE void upb_MiniTable_SetString(upb_Message* msg,
                                        const upb_MiniTable_Field* field,
                                        upb_StringView value) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Bytes ||
             field->descriptortype == kUpb_FieldType_String);
  _upb_MiniTable_SetPresence(msg, field);
  *UPB_PTR_AT(msg, field->offset, upb_StringView) = value;
}

UPB_INLINE const upb_Message* upb_MiniTable_GetMessage(
    const upb_Message* msg, const upb_MiniTable_Field* field) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Message ||
             field->descriptortype == kUpb_FieldType_Group);
  return *UPB_PTR_AT(msg, field->offset, const upb_Message*);
}

UPB_INLINE void upb_MiniTable_SetMessage(upb_Message* msg,
                                         const upb_MiniTable_Field* field,
                                         upb_Message* sub_message) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Message ||
             field->descriptortype == kUpb_FieldType_Group);
  _upb_MiniTable_SetPresence(msg, field);
  *UPB_PTR_AT(msg, field->offset, const upb_Message*) = sub_message;
}

UPB_INLINE upb_Message* upb_MiniTable_GetMutableMessage(
    upb_Message* msg, const upb_MiniTable* mini_table,
    const upb_MiniTable_Field* field, upb_Arena* arena) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Message ||
             field->descriptortype == kUpb_FieldType_Group);
  upb_Message* sub_message = *UPB_PTR_AT(msg, field->offset, upb_Message*);
  if (!sub_message) {
    sub_message =
        _upb_Message_New(mini_table->subs[field->submsg_index].submsg, arena);
    *UPB_PTR_AT(msg, field->offset, upb_Message*) = sub_message;
    _upb_MiniTable_SetPresence(msg, field);
  }
  return sub_message;
}

UPB_INLINE const upb_Array* upb_MiniTable_GetArray(
    const upb_Message* msg, const upb_MiniTable_Field* field) {
  return (const upb_Array*)*UPB_PTR_AT(msg, field->offset, upb_Array*);
}

UPB_INLINE upb_Array* upb_MiniTable_GetMutableArray(
    upb_Message* msg, const upb_MiniTable_Field* field) {
  return (upb_Array*)*UPB_PTR_AT(msg, field->offset, upb_Array*);
}

void* upb_MiniTable_ResizeArray(upb_Message* msg,
                                const upb_MiniTable_Field* field, size_t len,
                                upb_Arena* arena);
typedef enum {
  kUpb_GetExtension_Ok,
  kUpb_GetExtension_NotPresent,
  kUpb_GetExtension_ParseError,
  kUpb_GetExtension_OutOfMemory,
} upb_GetExtension_Status;

typedef enum {
  kUpb_GetExtensionAsBytes_Ok,
  kUpb_GetExtensionAsBytes_NotPresent,
  kUpb_GetExtensionAsBytes_EncodeError,
} upb_GetExtensionAsBytes_Status;

// Returns a message extension or promotes an unknown field to
// an extension.
//
// TODO(ferhat): Only supports extension fields that are messages,
// expand support to include non-message types.
upb_GetExtension_Status upb_MiniTable_GetOrPromoteExtension(
    upb_Message* msg, const upb_MiniTable_Extension* ext_table,
    int decode_options, upb_Arena* arena,
    const upb_Message_Extension** extension);

// Returns a message extension or unknown field matching the extension
// data as bytes.
//
// If an extension has already been decoded it will be re-encoded
// to bytes.
upb_GetExtensionAsBytes_Status upb_MiniTable_GetExtensionAsBytes(
    const upb_Message* msg, const upb_MiniTable_Extension* ext_table,
    int encode_options, upb_Arena* arena, const char** extension_data,
    size_t* len);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port_undef.inc"

#endif  // UPB_MINI_TABLE_ACCESSORS_H_
