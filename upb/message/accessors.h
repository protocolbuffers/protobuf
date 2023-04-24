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

#ifndef UPB_MESSAGE_ACCESSORS_H_
#define UPB_MESSAGE_ACCESSORS_H_

#include "upb/base/descriptor_constants.h"
#include "upb/collections/array.h"
#include "upb/collections/array_internal.h"
#include "upb/collections/map.h"
#include "upb/collections/map_internal.h"
#include "upb/message/accessors_internal.h"
#include "upb/message/extension_internal.h"
#include "upb/message/internal.h"
#include "upb/mini_table/common.h"
#include "upb/mini_table/enum_internal.h"
#include "upb/mini_table/field_internal.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

UPB_API_INLINE void upb_Message_ClearField(upb_Message* msg,
                                           const upb_MiniTableField* field) {
  if (upb_MiniTableField_IsExtension(field)) {
    const upb_MiniTableExtension* ext = (const upb_MiniTableExtension*)field;
    _upb_Message_ClearExtensionField(msg, ext);
  } else {
    _upb_Message_ClearNonExtensionField(msg, field);
  }
}

UPB_API_INLINE bool upb_Message_HasField(const upb_Message* msg,
                                         const upb_MiniTableField* field) {
  if (upb_MiniTableField_IsExtension(field)) {
    const upb_MiniTableExtension* ext = (const upb_MiniTableExtension*)field;
    return _upb_Message_HasExtensionField(msg, ext);
  } else {
    return _upb_Message_HasNonExtensionField(msg, field);
  }
}

UPB_API_INLINE uint32_t upb_Message_WhichOneofFieldNumber(
    const upb_Message* message, const upb_MiniTableField* oneof_field) {
  UPB_ASSUME(_upb_MiniTableField_InOneOf(oneof_field));
  return _upb_getoneofcase_field(message, oneof_field);
}

UPB_API_INLINE bool upb_Message_GetBool(const upb_Message* msg,
                                        const upb_MiniTableField* field,
                                        bool default_val) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Bool);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_1Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  bool ret;
  _upb_Message_GetField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE bool upb_Message_SetBool(upb_Message* msg,
                                        const upb_MiniTableField* field,
                                        bool value, upb_Arena* a) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Bool);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_1Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  return _upb_Message_SetField(msg, field, &value, a);
}

UPB_API_INLINE int32_t upb_Message_GetInt32(const upb_Message* msg,
                                            const upb_MiniTableField* field,
                                            int32_t default_val) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Int32 ||
             upb_MiniTableField_CType(field) == kUpb_CType_Enum);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_4Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  int32_t ret;
  _upb_Message_GetField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE bool upb_Message_SetInt32(upb_Message* msg,
                                         const upb_MiniTableField* field,
                                         int32_t value, upb_Arena* a) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Int32 ||
             upb_MiniTableField_CType(field) == kUpb_CType_Enum);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_4Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  return _upb_Message_SetField(msg, field, &value, a);
}

UPB_API_INLINE uint32_t upb_Message_GetUInt32(const upb_Message* msg,
                                              const upb_MiniTableField* field,
                                              uint32_t default_val) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_UInt32);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_4Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  uint32_t ret;
  _upb_Message_GetField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE bool upb_Message_SetUInt32(upb_Message* msg,
                                          const upb_MiniTableField* field,
                                          uint32_t value, upb_Arena* a) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_UInt32);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_4Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  return _upb_Message_SetField(msg, field, &value, a);
}

UPB_API_INLINE void upb_Message_SetClosedEnum(
    upb_Message* msg, const upb_MiniTable* msg_mini_table,
    const upb_MiniTableField* field, int32_t value) {
  UPB_ASSERT(upb_MiniTableField_IsClosedEnum(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_4Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSERT(upb_MiniTableEnum_CheckValue(
      upb_MiniTable_GetSubEnumTable(msg_mini_table, field), value));
  _upb_Message_SetNonExtensionField(msg, field, &value);
}

UPB_API_INLINE int64_t upb_Message_GetInt64(const upb_Message* msg,
                                            const upb_MiniTableField* field,
                                            uint64_t default_val) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Int64);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_8Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  int64_t ret;
  _upb_Message_GetField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE bool upb_Message_SetInt64(upb_Message* msg,
                                         const upb_MiniTableField* field,
                                         int64_t value, upb_Arena* a) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Int64);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_8Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  return _upb_Message_SetField(msg, field, &value, a);
}

UPB_API_INLINE uint64_t upb_Message_GetUInt64(const upb_Message* msg,
                                              const upb_MiniTableField* field,
                                              uint64_t default_val) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_UInt64);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_8Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  uint64_t ret;
  _upb_Message_GetField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE bool upb_Message_SetUInt64(upb_Message* msg,
                                          const upb_MiniTableField* field,
                                          uint64_t value, upb_Arena* a) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_UInt64);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_8Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  return _upb_Message_SetField(msg, field, &value, a);
}

UPB_API_INLINE float upb_Message_GetFloat(const upb_Message* msg,
                                          const upb_MiniTableField* field,
                                          float default_val) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Float);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_4Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  float ret;
  _upb_Message_GetField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE bool upb_Message_SetFloat(upb_Message* msg,
                                         const upb_MiniTableField* field,
                                         float value, upb_Arena* a) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Float);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_4Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  return _upb_Message_SetField(msg, field, &value, a);
}

UPB_API_INLINE double upb_Message_GetDouble(const upb_Message* msg,
                                            const upb_MiniTableField* field,
                                            double default_val) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Double);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_8Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  double ret;
  _upb_Message_GetField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE bool upb_Message_SetDouble(upb_Message* msg,
                                          const upb_MiniTableField* field,
                                          double value, upb_Arena* a) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Double);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_8Byte);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  return _upb_Message_SetField(msg, field, &value, a);
}

UPB_API_INLINE upb_StringView
upb_Message_GetString(const upb_Message* msg, const upb_MiniTableField* field,
                      upb_StringView def_val) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_String ||
             upb_MiniTableField_CType(field) == kUpb_CType_Bytes);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_StringView);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  upb_StringView ret;
  _upb_Message_GetField(msg, field, &def_val, &ret);
  return ret;
}

UPB_API_INLINE bool upb_Message_SetString(upb_Message* msg,
                                          const upb_MiniTableField* field,
                                          upb_StringView value, upb_Arena* a) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_String ||
             upb_MiniTableField_CType(field) == kUpb_CType_Bytes);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_StringView);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  return _upb_Message_SetField(msg, field, &value, a);
}

UPB_API_INLINE const upb_Message* upb_Message_GetMessage(
    const upb_Message* msg, const upb_MiniTableField* field,
    upb_Message* default_val) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Message);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) ==
             UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte));
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  upb_Message* ret;
  _upb_Message_GetNonExtensionField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE void upb_Message_SetMessage(upb_Message* msg,
                                           const upb_MiniTable* mini_table,
                                           const upb_MiniTableField* field,
                                           upb_Message* sub_message) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Message);
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) ==
             UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte));
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSERT(mini_table->subs[field->UPB_PRIVATE(submsg_index)].submsg);
  _upb_Message_SetNonExtensionField(msg, field, &sub_message);
}

UPB_API_INLINE upb_Message* upb_Message_GetOrCreateMutableMessage(
    upb_Message* msg, const upb_MiniTable* mini_table,
    const upb_MiniTableField* field, upb_Arena* arena) {
  UPB_ASSERT(arena);
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Message);
  upb_Message* sub_message = *UPB_PTR_AT(msg, field->offset, upb_Message*);
  if (!sub_message) {
    const upb_MiniTable* sub_mini_table =
        mini_table->subs[field->UPB_PRIVATE(submsg_index)].submsg;
    UPB_ASSERT(sub_mini_table);
    sub_message = _upb_Message_New(sub_mini_table, arena);
    *UPB_PTR_AT(msg, field->offset, upb_Message*) = sub_message;
    _upb_Message_SetPresence(msg, field);
  }
  return sub_message;
}

UPB_API_INLINE const upb_Array* upb_Message_GetArray(
    const upb_Message* msg, const upb_MiniTableField* field) {
  _upb_MiniTableField_CheckIsArray(field);
  upb_Array* ret;
  const upb_Array* default_val = NULL;
  _upb_Message_GetNonExtensionField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE upb_Array* upb_Message_GetMutableArray(
    upb_Message* msg, const upb_MiniTableField* field) {
  _upb_MiniTableField_CheckIsArray(field);
  return (upb_Array*)upb_Message_GetArray(msg, field);
}

UPB_API_INLINE upb_Array* upb_Message_GetOrCreateMutableArray(
    upb_Message* msg, const upb_MiniTableField* field, upb_Arena* arena) {
  UPB_ASSERT(arena);
  _upb_MiniTableField_CheckIsArray(field);
  upb_Array* array = upb_Message_GetMutableArray(msg, field);
  if (!array) {
    array = _upb_Array_New(arena, 4, _upb_MiniTable_ElementSizeLg2(field));
    // Check again due to: https://godbolt.org/z/7WfaoKG1r
    _upb_MiniTableField_CheckIsArray(field);
    _upb_Message_SetField(msg, field, &array, arena);
  }
  return array;
}

UPB_INLINE upb_Array* upb_Message_ResizeArrayUninitialized(
    upb_Message* msg, const upb_MiniTableField* field, size_t size,
    upb_Arena* arena) {
  _upb_MiniTableField_CheckIsArray(field);
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, field, arena);
  if (!arr || !_upb_Array_ResizeUninitialized(arr, size, arena)) return NULL;
  return arr;
}

// TODO: remove, migrate users to upb_Message_ResizeArrayUninitialized(), which
// has the same semantics but a clearer name. Alternatively, if users want an
// initialized variant, we can also offer that.
UPB_API_INLINE void* upb_Message_ResizeArray(upb_Message* msg,
                                             const upb_MiniTableField* field,
                                             size_t size, upb_Arena* arena) {
  _upb_MiniTableField_CheckIsArray(field);
  upb_Array* arr =
      upb_Message_ResizeArrayUninitialized(msg, field, size, arena);
  return _upb_array_ptr(arr);
}

UPB_API_INLINE const upb_Map* upb_Message_GetMap(
    const upb_Message* msg, const upb_MiniTableField* field) {
  _upb_MiniTableField_CheckIsMap(field);
  upb_Map* ret;
  const upb_Map* default_val = NULL;
  _upb_Message_GetNonExtensionField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE upb_Map* upb_Message_GetOrCreateMutableMap(
    upb_Message* msg, const upb_MiniTable* map_entry_mini_table,
    const upb_MiniTableField* field, upb_Arena* arena) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Message);
  const upb_MiniTableField* map_entry_key_field =
      &map_entry_mini_table->fields[0];
  const upb_MiniTableField* map_entry_value_field =
      &map_entry_mini_table->fields[1];
  return _upb_Message_GetOrCreateMutableMap(
      msg, field,
      _upb_Map_CTypeSize(upb_MiniTableField_CType(map_entry_key_field)),
      _upb_Map_CTypeSize(upb_MiniTableField_CType(map_entry_value_field)),
      arena);
}

// Updates a map entry given an entry message.
upb_MapInsertStatus upb_Message_InsertMapEntry(upb_Map* map,
                                               const upb_MiniTable* mini_table,
                                               const upb_MiniTableField* field,
                                               upb_Message* map_entry_message,
                                               upb_Arena* arena);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif  // UPB_MESSAGE_ACCESSORS_H_
