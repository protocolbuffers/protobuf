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
#include "upb/collections/map.h"
#include "upb/collections/map_internal.h"
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

UPB_INLINE bool _upb_MiniTableField_InOneOf(const upb_MiniTableField* field) {
  return field->presence < 0;
}

UPB_INLINE void* _upb_MiniTableField_GetPtr(upb_Message* msg,
                                            const upb_MiniTableField* field) {
  return (char*)msg + field->offset;
}

UPB_INLINE const void* _upb_MiniTableField_GetConstPtr(
    const upb_Message* msg, const upb_MiniTableField* field) {
  return (char*)msg + field->offset;
}

UPB_INLINE void _upb_Message_SetPresence(upb_Message* msg,
                                         const upb_MiniTableField* field) {
  if (field->presence > 0) {
    _upb_sethas_field(msg, field);
  } else if (_upb_MiniTableField_InOneOf(field)) {
    *_upb_oneofcase_field(msg, field) = field->number;
  }
}

UPB_INLINE bool _upb_MiniTable_ValueIsNonZero(const void* default_val,
                                              const upb_MiniTableField* field) {
  char zero[16] = {0};
  switch (_upb_MiniTableField_GetRep(field)) {
    case kUpb_FieldRep_1Byte:
      return memcmp(&zero, default_val, 1) != 0;
    case kUpb_FieldRep_4Byte:
      return memcmp(&zero, default_val, 4) != 0;
    case kUpb_FieldRep_8Byte:
      return memcmp(&zero, default_val, 8) != 0;
    case kUpb_FieldRep_StringView: {
      const upb_StringView* sv = (const upb_StringView*)default_val;
      return sv->size != 0;
    }
  }
  UPB_UNREACHABLE();
}

UPB_INLINE void _upb_MiniTable_CopyFieldData(void* to, const void* from,
                                             const upb_MiniTableField* field) {
  switch (_upb_MiniTableField_GetRep(field)) {
    case kUpb_FieldRep_1Byte:
      memcpy(to, from, 1);
      return;
    case kUpb_FieldRep_4Byte:
      memcpy(to, from, 4);
      return;
    case kUpb_FieldRep_8Byte:
      memcpy(to, from, 8);
      return;
    case kUpb_FieldRep_StringView: {
      memcpy(to, from, sizeof(upb_StringView));
      return;
    }
  }
  UPB_UNREACHABLE();
}

// Here we define universal getter/setter functions for message fields.
// These look very branchy and inefficient, but as long as the MiniTableField
// values are known at compile time, all the branches are optimized away and
// we are left with ideal code.  This can happen either through through
// literals or UPB_ASSUME():
//
//   // Via struct literals.
//   bool FooMessage_set_bool_field(const upb_Message* msg, bool val) {
//     const upb_MiniTableField field = {1, 0, 0, /* etc... */};
//     // All value in "field" are compile-time known.
//     _upb_Message_SetNonExtensionField(msg, &field, &value);
//   }
//
//   // Via UPB_ASSUME().
//   UPB_INLINE bool upb_Message_SetBool(upb_Message* msg,
//                                       const upb_MiniTableField* field,
//                                       bool value, upb_Arena* a) {
//     UPB_ASSUME(field->descriptortype == kUpb_FieldType_Bool);
//     UPB_ASSUME(!upb_IsRepeatedOrMap(field));
//     UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_1Byte);
//     _upb_Message_SetField(msg, field, &value, a);
//   }
//
// As a result, we can use these universal getters/setters for *all* message
// accessors: generated code, MiniTable accessors, and reflection.  The only
// exception is the binary encoder/decoder, which need to be a bit more clever
// about how they read/write the message data, for efficiency.
//
// These functions work on both extensions and non-extensions. If the field
// of a setter is known to be a non-extension, the arena may be NULL and the
// returned bool value may be ignored since it will always succeed.

UPB_INLINE bool _upb_Message_HasExtensionField(
    const upb_Message* msg, const upb_MiniTableExtension* ext) {
  UPB_ASSERT(upb_MiniTableField_HasPresence(&ext->field));
  return _upb_Message_Getext(msg, ext) != NULL;
}

UPB_INLINE bool _upb_Message_HasNonExtensionField(
    const upb_Message* msg, const upb_MiniTableField* field) {
  UPB_ASSERT(upb_MiniTableField_HasPresence(field));
  UPB_ASSUME(!upb_MiniTableField_IsExtension(field));
  if (_upb_MiniTableField_InOneOf(field)) {
    return _upb_getoneofcase_field(msg, field) == field->number;
  } else {
    return _upb_hasbit_field(msg, field);
  }
}

static UPB_FORCEINLINE void _upb_Message_GetNonExtensionField(
    const upb_Message* msg, const upb_MiniTableField* field,
    const void* default_val, void* val) {
  UPB_ASSUME(!upb_MiniTableField_IsExtension(field));
  if ((_upb_MiniTableField_InOneOf(field) ||
       _upb_MiniTable_ValueIsNonZero(default_val, field)) &&
      !_upb_Message_HasNonExtensionField(msg, field)) {
    _upb_MiniTable_CopyFieldData(val, default_val, field);
    return;
  }
  _upb_MiniTable_CopyFieldData(val, _upb_MiniTableField_GetConstPtr(msg, field),
                               field);
}

UPB_INLINE void _upb_Message_GetExtensionField(
    const upb_Message* msg, const upb_MiniTableExtension* mt_ext,
    const void* default_val, void* val) {
  UPB_ASSUME(upb_MiniTableField_IsExtension(&mt_ext->field));
  const upb_Message_Extension* ext = _upb_Message_Getext(msg, mt_ext);
  if (ext) {
    _upb_MiniTable_CopyFieldData(val, &ext->data, &mt_ext->field);
  } else {
    _upb_MiniTable_CopyFieldData(val, default_val, &mt_ext->field);
  }
}

UPB_INLINE void _upb_Message_GetField(const upb_Message* msg,
                                      const upb_MiniTableField* field,
                                      const void* default_val, void* val) {
  if (upb_MiniTableField_IsExtension(field)) {
    _upb_Message_GetExtensionField(msg, (upb_MiniTableExtension*)field,
                                   default_val, val);
  } else {
    _upb_Message_GetNonExtensionField(msg, field, default_val, val);
  }
}

UPB_INLINE void _upb_Message_SetNonExtensionField(
    upb_Message* msg, const upb_MiniTableField* field, const void* val) {
  UPB_ASSUME(!upb_MiniTableField_IsExtension(field));
  _upb_Message_SetPresence(msg, field);
  _upb_MiniTable_CopyFieldData(_upb_MiniTableField_GetPtr(msg, field), val,
                               field);
}

UPB_INLINE bool _upb_Message_SetExtensionField(
    upb_Message* msg, const upb_MiniTableExtension* mt_ext, const void* val,
    upb_Arena* a) {
  UPB_ASSERT(a);
  upb_Message_Extension* ext =
      _upb_Message_GetOrCreateExtension(msg, mt_ext, a);
  if (!ext) return false;
  _upb_MiniTable_CopyFieldData(&ext->data, val, &mt_ext->field);
  return true;
}

UPB_INLINE bool _upb_Message_SetField(upb_Message* msg,
                                      const upb_MiniTableField* field,
                                      const void* val, upb_Arena* a) {
  if (upb_MiniTableField_IsExtension(field)) {
    const upb_MiniTableExtension* ext = (const upb_MiniTableExtension*)field;
    return _upb_Message_SetExtensionField(msg, ext, val, a);
  } else {
    _upb_Message_SetNonExtensionField(msg, field, val);
    return true;
  }
}

UPB_INLINE void _upb_Message_ClearExtensionField(
    upb_Message* msg, const upb_MiniTableExtension* ext_l) {
  upb_Message_Internal* in = upb_Message_Getinternal(msg);
  if (!in->internal) return;
  const upb_Message_Extension* base =
      UPB_PTR_AT(in->internal, in->internal->ext_begin, upb_Message_Extension);
  upb_Message_Extension* ext =
      (upb_Message_Extension*)_upb_Message_Getext(msg, ext_l);
  if (ext) {
    *ext = *base;
    in->internal->ext_begin += sizeof(upb_Message_Extension);
  }
}

UPB_INLINE void _upb_Message_ClearNonExtensionField(
    upb_Message* msg, const upb_MiniTableField* field) {
  if (field->presence > 0) {
    _upb_clearhas(msg, _upb_Message_Hasidx(field));
  } else if (_upb_MiniTableField_InOneOf(field)) {
    uint32_t* oneof_case = _upb_oneofcase_field(msg, field);
    if (*oneof_case != field->number) return;
    *oneof_case = 0;
  }
  const char zeros[16] = {0};
  _upb_MiniTable_CopyFieldData(_upb_MiniTableField_GetPtr(msg, field), zeros,
                               field);
}

// EVERYTHING ABOVE THIS LINE IS INTERNAL - DO NOT USE /////////////////////////

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
  UPB_ASSUME(field->descriptortype == kUpb_FieldType_Bool);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_1Byte);
  bool ret;
  _upb_Message_GetField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE bool upb_Message_SetBool(upb_Message* msg,
                                        const upb_MiniTableField* field,
                                        bool value, upb_Arena* a) {
  UPB_ASSUME(field->descriptortype == kUpb_FieldType_Bool);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_1Byte);
  return _upb_Message_SetField(msg, field, &value, a);
}

UPB_API_INLINE int32_t upb_Message_GetInt32(const upb_Message* msg,
                                            const upb_MiniTableField* field,
                                            int32_t default_val) {
  UPB_ASSUME(field->descriptortype == kUpb_FieldType_Int32 ||
             field->descriptortype == kUpb_FieldType_SInt32 ||
             field->descriptortype == kUpb_FieldType_SFixed32 ||
             field->descriptortype == kUpb_FieldType_Enum);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_4Byte);
  int32_t ret;
  _upb_Message_GetField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE bool upb_Message_SetInt32(upb_Message* msg,
                                         const upb_MiniTableField* field,
                                         int32_t value, upb_Arena* a) {
  UPB_ASSUME(field->descriptortype == kUpb_FieldType_Int32 ||
             field->descriptortype == kUpb_FieldType_SInt32 ||
             field->descriptortype == kUpb_FieldType_SFixed32);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_4Byte);
  return _upb_Message_SetField(msg, field, &value, a);
}

UPB_API_INLINE uint32_t upb_Message_GetUInt32(const upb_Message* msg,
                                              const upb_MiniTableField* field,
                                              uint32_t default_val) {
  UPB_ASSUME(field->descriptortype == kUpb_FieldType_UInt32 ||
             field->descriptortype == kUpb_FieldType_Fixed32);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_4Byte);
  uint32_t ret;
  _upb_Message_GetField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE bool upb_Message_SetUInt32(upb_Message* msg,
                                          const upb_MiniTableField* field,
                                          uint32_t value, upb_Arena* a) {
  UPB_ASSUME(field->descriptortype == kUpb_FieldType_UInt32 ||
             field->descriptortype == kUpb_FieldType_Fixed32);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_4Byte);
  return _upb_Message_SetField(msg, field, &value, a);
}

UPB_API_INLINE void upb_MiniTable_SetEnumProto2(
    upb_Message* msg, const upb_MiniTable* msg_mini_table,
    const upb_MiniTableField* field, int32_t value) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Enum);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_4Byte);
  UPB_ASSERT(upb_MiniTableEnum_CheckValue(
      upb_MiniTable_GetSubEnumTable(msg_mini_table, field), value));
  _upb_Message_SetNonExtensionField(msg, field, &value);
}

UPB_API_INLINE int64_t upb_Message_GetInt64(const upb_Message* msg,
                                            const upb_MiniTableField* field,
                                            uint64_t default_val) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Int64 ||
             field->descriptortype == kUpb_FieldType_SInt64 ||
             field->descriptortype == kUpb_FieldType_SFixed64);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_8Byte);
  int64_t ret;
  _upb_Message_GetField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE bool upb_Message_SetInt64(upb_Message* msg,
                                         const upb_MiniTableField* field,
                                         int64_t value, upb_Arena* a) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Int64 ||
             field->descriptortype == kUpb_FieldType_SInt64 ||
             field->descriptortype == kUpb_FieldType_SFixed64);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_8Byte);
  return _upb_Message_SetField(msg, field, &value, a);
}

UPB_API_INLINE uint64_t upb_Message_GetUInt64(const upb_Message* msg,
                                              const upb_MiniTableField* field,
                                              uint64_t default_val) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_UInt64 ||
             field->descriptortype == kUpb_FieldType_Fixed64);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_8Byte);
  uint64_t ret;
  _upb_Message_GetField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE bool upb_Message_SetUInt64(upb_Message* msg,
                                          const upb_MiniTableField* field,
                                          uint64_t value, upb_Arena* a) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_UInt64 ||
             field->descriptortype == kUpb_FieldType_Fixed64);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_8Byte);
  return _upb_Message_SetField(msg, field, &value, a);
}

UPB_API_INLINE float upb_Message_GetFloat(const upb_Message* msg,
                                          const upb_MiniTableField* field,
                                          float default_val) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Float);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_4Byte);
  float ret;
  _upb_Message_GetField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE bool upb_Message_SetFloat(upb_Message* msg,
                                         const upb_MiniTableField* field,
                                         float value, upb_Arena* a) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Float);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_4Byte);
  return _upb_Message_SetField(msg, field, &value, a);
}

UPB_API_INLINE double upb_Message_GetDouble(const upb_Message* msg,
                                            const upb_MiniTableField* field,
                                            double default_val) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Double);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_8Byte);
  double ret;
  _upb_Message_GetField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE bool upb_Message_SetDouble(upb_Message* msg,
                                          const upb_MiniTableField* field,
                                          double value, upb_Arena* a) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Double);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_8Byte);
  return _upb_Message_SetField(msg, field, &value, a);
}

UPB_API_INLINE upb_StringView
upb_Message_GetString(const upb_Message* msg, const upb_MiniTableField* field,
                      upb_StringView def_val) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Bytes ||
             field->descriptortype == kUpb_FieldType_String);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_StringView);
  upb_StringView ret;
  _upb_Message_GetField(msg, field, &def_val, &ret);
  return ret;
}

UPB_API_INLINE bool upb_Message_SetString(upb_Message* msg,
                                          const upb_MiniTableField* field,
                                          upb_StringView value, upb_Arena* a) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Bytes ||
             field->descriptortype == kUpb_FieldType_String);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_StringView);
  return _upb_Message_SetField(msg, field, &value, a);
}

UPB_API_INLINE const upb_Message* upb_MiniTable_GetMessage(
    const upb_Message* msg, const upb_MiniTableField* field,
    upb_Message* default_val) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Message ||
             field->descriptortype == kUpb_FieldType_Group);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) ==
             UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte));
  upb_Message* ret;
  _upb_Message_GetNonExtensionField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE void upb_MiniTable_SetMessage(upb_Message* msg,
                                             const upb_MiniTable* mini_table,
                                             const upb_MiniTableField* field,
                                             upb_Message* sub_message) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Message ||
             field->descriptortype == kUpb_FieldType_Group);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) ==
             UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte));
  UPB_ASSERT(mini_table->subs[field->submsg_index].submsg);
  _upb_Message_SetNonExtensionField(msg, field, &sub_message);
}

UPB_API_INLINE upb_Message* upb_MiniTable_GetMutableMessage(
    upb_Message* msg, const upb_MiniTable* mini_table,
    const upb_MiniTableField* field, upb_Arena* arena) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Message ||
             field->descriptortype == kUpb_FieldType_Group);
  upb_Message* sub_message = *UPB_PTR_AT(msg, field->offset, upb_Message*);
  if (!sub_message) {
    const upb_MiniTable* sub_mini_table =
        mini_table->subs[field->submsg_index].submsg;
    UPB_ASSERT(sub_mini_table);
    sub_message = _upb_Message_New(sub_mini_table, arena);
    *UPB_PTR_AT(msg, field->offset, upb_Message*) = sub_message;
    _upb_Message_SetPresence(msg, field);
  }
  return sub_message;
}

UPB_API_INLINE const upb_Array* upb_Message_GetArray(
    const upb_Message* msg, const upb_MiniTableField* field) {
  const upb_Array* ret;
  const upb_Array* default_val = NULL;
  _upb_Message_GetNonExtensionField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE upb_Array* upb_Message_GetMutableArray(
    upb_Message* msg, const upb_MiniTableField* field) {
  return (upb_Array*)upb_Message_GetArray(msg, field);
}

UPB_API_INLINE upb_Array* upb_Message_GetOrCreateMutableArray(
    upb_Message* msg, const upb_MiniTableField* field, upb_CType ctype,
    upb_Arena* arena) {
  upb_Array* array = upb_Message_GetMutableArray(msg, field);
  if (!array) {
    array = upb_Array_New(arena, ctype);
    _upb_Message_SetField(msg, field, &array, arena);
  }
  return array;
}

void* upb_Message_ResizeArray(upb_Message* msg, const upb_MiniTableField* field,
                              size_t len, upb_Arena* arena);

UPB_API_INLINE bool upb_MiniTableField_IsClosedEnum(
    const upb_MiniTableField* field) {
  return field->descriptortype == kUpb_FieldType_Enum;
}

UPB_API_INLINE upb_Map* upb_MiniTable_GetMutableMap(
    upb_Message* msg, const upb_MiniTable* map_entry_mini_table,
    const upb_MiniTableField* field, upb_Arena* arena) {
  UPB_ASSERT(map_entry_mini_table != NULL);
  UPB_ASSUME(upb_IsRepeatedOrMap(field));
  upb_Map* map = NULL;
  upb_Map* default_map_value = NULL;
  _upb_Message_GetNonExtensionField(msg, field, &default_map_value, &map);
  if (!map) {
    // Allocate map.
    UPB_ASSERT(field->descriptortype == kUpb_FieldType_Message ||
               field->descriptortype == kUpb_FieldType_Group);
    const upb_MiniTableField* map_entry_key_field =
        &map_entry_mini_table->fields[0];
    const upb_MiniTableField* map_entry_value_field =
        &map_entry_mini_table->fields[1];
    map = upb_Map_New(arena, upb_MiniTableField_CType(map_entry_key_field),
                      upb_MiniTableField_CType(map_entry_value_field));
    _upb_Message_SetNonExtensionField(msg, field, &map);
  }
  return map;
}

// Updates a map entry given an entry message.
upb_MapInsertStatus upb_Message_InsertMapEntry(upb_Map* map,
                                               const upb_MiniTable* mini_table,
                                               const upb_MiniTableField* field,
                                               upb_Message* map_entry_message,
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
    upb_Message* msg, const upb_MiniTableExtension* ext_table,
    int decode_options, upb_Arena* arena,
    const upb_Message_Extension** extension);

// Returns a message extension or unknown field matching the extension
// data as bytes.
//
// If an extension has already been decoded it will be re-encoded
// to bytes.
upb_GetExtensionAsBytes_Status upb_MiniTable_GetExtensionAsBytes(
    const upb_Message* msg, const upb_MiniTableExtension* ext_table,
    int encode_options, upb_Arena* arena, const char** extension_data,
    size_t* len);

typedef enum {
  kUpb_FindUnknown_Ok,
  kUpb_FindUnknown_NotPresent,
  kUpb_FindUnknown_ParseError,
} upb_FindUnknown_Status;

typedef struct {
  upb_FindUnknown_Status status;
  // Start of unknown field data in message arena.
  const char* ptr;
  // Size of unknown field data.
  size_t len;
} upb_FindUnknownRet;

// Finds first occurrence of unknown data by tag id in message.
upb_FindUnknownRet upb_MiniTable_FindUnknown(const upb_Message* msg,
                                             uint32_t field_number);

typedef enum {
  kUpb_UnknownToMessage_Ok,
  kUpb_UnknownToMessage_ParseError,
  kUpb_UnknownToMessage_OutOfMemory,
  kUpb_UnknownToMessage_NotFound,
} upb_UnknownToMessage_Status;

typedef struct {
  upb_UnknownToMessage_Status status;
  upb_Message* message;
} upb_UnknownToMessageRet;

// Promotes unknown data inside message to a upb_Message parsing the unknown.
//
// The unknown data is removed from message after field value is set
// using upb_MiniTable_SetMessage.
upb_UnknownToMessageRet upb_MiniTable_PromoteUnknownToMessage(
    upb_Message* msg, const upb_MiniTable* mini_table,
    const upb_MiniTableField* field, const upb_MiniTable* sub_mini_table,
    int decode_options, upb_Arena* arena);

// Promotes all unknown data that matches field tag id to repeated messages
// in upb_Array.
//
// The unknown data is removed from message after upb_Array is populated.
// Since repeated messages can't be packed we remove each unknown that
// contains the target tag id.
upb_UnknownToMessage_Status upb_MiniTable_PromoteUnknownToMessageArray(
    upb_Message* msg, const upb_MiniTableField* field,
    const upb_MiniTable* mini_table, int decode_options, upb_Arena* arena);

// Promotes all unknown data that matches field tag id to upb_Map.
//
// The unknown data is removed from message after upb_Map is populated.
// Since repeated messages can't be packed we remove each unknown that
// contains the target tag id.
upb_UnknownToMessage_Status upb_MiniTable_PromoteUnknownToMap(
    upb_Message* msg, const upb_MiniTable* mini_table,
    const upb_MiniTableField* field, int decode_options, upb_Arena* arena);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif  // UPB_MESSAGE_ACCESSORS_H_
