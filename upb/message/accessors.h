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

#include "upb/collections/array.h"
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

UPB_INLINE void _upb_MiniTable_SetPresence(upb_Message* msg,
                                           const upb_MiniTableField* field) {
  if (field->presence > 0) {
    _upb_sethas_field(msg, field);
  } else if (_upb_MiniTableField_InOneOf(field)) {
    *_upb_oneofcase_field(msg, field) = field->number;
  }
}

UPB_INLINE bool upb_MiniTable_HasField(const upb_Message* msg,
                                       const upb_MiniTableField* field);

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
//   // Via string literals.
//   bool FooMessage_set_bool_field(const upb_Message* msg, bool val) {
//     const upb_MiniTableField field = {1, 0, 0, /* etc... */};
//     // All value in "field" are compile-time known.
//     _upb_MiniTable_SetNonExtensionField(msg, &field, &value);
//   }
//
//   // Via UPB_ASSUME().
//   UPB_INLINE void upb_MiniTable_SetBool(upb_Message* msg,
//                                         const upb_MiniTableField* field,
//                                         bool value) {
//     UPB_ASSUME(field->descriptortype == kUpb_FieldType_Bool);
//     UPB_ASSUME(!upb_IsRepeatedOrMap(field));
//     UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_1Byte);
//     _upb_MiniTable_SetNonExtensionField(msg, field, &value);
//   }
//
// As a result, we can use these universal getters/setters for *all* message
// accessors: generated code, MiniTable accessors, and reflection.  The only
// exception is the binary encoder/decoder, which need to be a bit more clever
// about how the read/write the message data, for efficiency.

static UPB_FORCEINLINE void _upb_MiniTable_GetNonExtensionField(
    const upb_Message* msg, const upb_MiniTableField* field,
    const void* default_val, void* val) {
  UPB_ASSUME(!upb_MiniTableField_IsExtension(field));
  if ((_upb_MiniTableField_InOneOf(field) ||
       _upb_MiniTable_ValueIsNonZero(default_val, field)) &&
      !upb_MiniTable_HasField(msg, field)) {
    _upb_MiniTable_CopyFieldData(val, default_val, field);
    return;
  }
  _upb_MiniTable_CopyFieldData(val, _upb_MiniTableField_GetConstPtr(msg, field),
                               field);
}

UPB_INLINE void _upb_MiniTable_GetExtensionField(
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

UPB_INLINE void _upb_MiniTable_GetField(const upb_Message* msg,
                                        const upb_MiniTableField* field,
                                        const void* default_val, void* val) {
  if (upb_MiniTableField_IsExtension(field)) {
    _upb_MiniTable_GetExtensionField(msg, (upb_MiniTableExtension*)field,
                                     default_val, val);
  } else {
    _upb_MiniTable_GetNonExtensionField(msg, field, default_val, val);
  }
}

UPB_INLINE void _upb_MiniTable_SetNonExtensionField(
    upb_Message* msg, const upb_MiniTableField* field, const void* val) {
  UPB_ASSUME(!upb_MiniTableField_IsExtension(field));
  _upb_MiniTable_SetPresence(msg, field);
  _upb_MiniTable_CopyFieldData(_upb_MiniTableField_GetPtr(msg, field), val,
                               field);
}

UPB_INLINE bool _upb_MiniTable_SetExtensionField(
    upb_Message* msg, const upb_MiniTableExtension* mt_ext, const void* val,
    upb_Arena* a) {
  upb_Message_Extension* ext =
      _upb_Message_GetOrCreateExtension(msg, mt_ext, a);
  if (!ext) return false;
  _upb_MiniTable_CopyFieldData(&ext->data, val, &mt_ext->field);
  return true;
}

UPB_INLINE bool _upb_MiniTable_SetField(upb_Message* msg,
                                        const upb_MiniTableField* field,
                                        const void* val, upb_Arena* a) {
  if (upb_MiniTableField_IsExtension(field)) {
    return _upb_MiniTable_SetExtensionField(
        msg, (const upb_MiniTableExtension*)field, val, a);
  } else {
    _upb_MiniTable_SetNonExtensionField(msg, field, val);
    return true;
  }
}

UPB_INLINE bool _upb_MiniTable_HasExtensionField(
    const upb_Message* msg, const upb_MiniTableExtension* ext) {
  UPB_ASSERT(upb_MiniTableField_HasPresence(&ext->field));
  return _upb_Message_Getext(msg, ext) != NULL;
}

UPB_INLINE bool _upb_MiniTable_HasNonExtensionField(
    const upb_Message* msg, const upb_MiniTableField* field) {
  UPB_ASSERT(upb_MiniTableField_HasPresence(field));
  UPB_ASSUME(!upb_MiniTableField_IsExtension(field));
  if (_upb_MiniTableField_InOneOf(field)) {
    return _upb_getoneofcase_field(msg, field) == field->number;
  } else {
    return _upb_hasbit_field(msg, field);
  }
}

UPB_INLINE bool _upb_MiniTable_HasField(const upb_Message* msg,
                                        const upb_MiniTableField* field) {
  if (upb_MiniTableField_IsExtension(field)) {
    return _upb_MiniTable_HasExtensionField(
        msg, (const upb_MiniTableExtension*)field);
  } else {
    return _upb_MiniTable_HasNonExtensionField(msg, field);
  }
}

// EVERYTHING ABOVE THIS LINE IS INTERNAL - DO NOT USE /////////////////////////

void upb_MiniTable_ClearField(upb_Message* msg,
                              const upb_MiniTableField* field);

UPB_INLINE bool upb_MiniTable_HasField(const upb_Message* msg,
                                       const upb_MiniTableField* field) {
  return _upb_MiniTable_HasNonExtensionField(msg, field);
}

UPB_INLINE bool upb_MiniTable_GetBool(const upb_Message* msg,
                                      const upb_MiniTableField* field,
                                      bool default_val) {
  UPB_ASSUME(field->descriptortype == kUpb_FieldType_Bool);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_1Byte);
  bool ret;
  _upb_MiniTable_GetNonExtensionField(msg, field, &default_val, &ret);
  return ret;
}

UPB_INLINE void upb_MiniTable_SetBool(upb_Message* msg,
                                      const upb_MiniTableField* field,
                                      bool value) {
  UPB_ASSUME(field->descriptortype == kUpb_FieldType_Bool);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_1Byte);
  _upb_MiniTable_SetNonExtensionField(msg, field, &value);
}

UPB_INLINE int32_t upb_MiniTable_GetInt32(const upb_Message* msg,
                                          const upb_MiniTableField* field,
                                          int32_t default_val) {
  UPB_ASSUME(field->descriptortype == kUpb_FieldType_Int32 ||
             field->descriptortype == kUpb_FieldType_SInt32 ||
             field->descriptortype == kUpb_FieldType_SFixed32 ||
             field->descriptortype == kUpb_FieldType_Enum);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_4Byte);
  int32_t ret;
  _upb_MiniTable_GetNonExtensionField(msg, field, &default_val, &ret);
  return ret;
}

UPB_INLINE void upb_MiniTable_SetInt32(upb_Message* msg,
                                       const upb_MiniTableField* field,
                                       int32_t value) {
  UPB_ASSUME(field->descriptortype == kUpb_FieldType_Int32 ||
             field->descriptortype == kUpb_FieldType_SInt32 ||
             field->descriptortype == kUpb_FieldType_SFixed32);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_4Byte);
  _upb_MiniTable_SetNonExtensionField(msg, field, &value);
}

UPB_INLINE uint32_t upb_MiniTable_GetUInt32(const upb_Message* msg,
                                            const upb_MiniTableField* field,
                                            uint32_t default_val) {
  UPB_ASSUME(field->descriptortype == kUpb_FieldType_UInt32 ||
             field->descriptortype == kUpb_FieldType_Fixed32);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_4Byte);
  uint32_t ret;
  _upb_MiniTable_GetNonExtensionField(msg, field, &default_val, &ret);
  return ret;
}

UPB_INLINE void upb_MiniTable_SetUInt32(upb_Message* msg,
                                        const upb_MiniTableField* field,
                                        uint32_t value) {
  UPB_ASSUME(field->descriptortype == kUpb_FieldType_UInt32 ||
             field->descriptortype == kUpb_FieldType_Fixed32);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_4Byte);
  _upb_MiniTable_SetNonExtensionField(msg, field, &value);
}

UPB_INLINE void upb_MiniTable_SetEnumProto2(upb_Message* msg,
                                            const upb_MiniTable* msg_mini_table,
                                            const upb_MiniTableField* field,
                                            int32_t value) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Enum);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_4Byte);
  UPB_ASSERT(upb_MiniTableEnum_CheckValue(
      upb_MiniTable_GetSubEnumTable(msg_mini_table, field), value));
  _upb_MiniTable_SetNonExtensionField(msg, field, &value);
}

UPB_INLINE int64_t upb_MiniTable_GetInt64(const upb_Message* msg,
                                          const upb_MiniTableField* field,
                                          uint64_t default_val) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Int64 ||
             field->descriptortype == kUpb_FieldType_SInt64 ||
             field->descriptortype == kUpb_FieldType_SFixed64);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_8Byte);
  int64_t ret;
  _upb_MiniTable_GetNonExtensionField(msg, field, &default_val, &ret);
  return ret;
}

UPB_INLINE void upb_MiniTable_SetInt64(upb_Message* msg,
                                       const upb_MiniTableField* field,
                                       int64_t value) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Int64 ||
             field->descriptortype == kUpb_FieldType_SInt64 ||
             field->descriptortype == kUpb_FieldType_SFixed64);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_8Byte);
  _upb_MiniTable_SetNonExtensionField(msg, field, &value);
}

UPB_INLINE uint64_t upb_MiniTable_GetUInt64(const upb_Message* msg,
                                            const upb_MiniTableField* field,
                                            uint64_t default_val) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_UInt64 ||
             field->descriptortype == kUpb_FieldType_Fixed64);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_8Byte);
  uint64_t ret;
  _upb_MiniTable_GetNonExtensionField(msg, field, &default_val, &ret);
  return ret;
}

UPB_INLINE void upb_MiniTable_SetUInt64(upb_Message* msg,
                                        const upb_MiniTableField* field,
                                        uint64_t value) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_UInt64 ||
             field->descriptortype == kUpb_FieldType_Fixed64);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_8Byte);
  _upb_MiniTable_SetNonExtensionField(msg, field, &value);
}

UPB_INLINE float upb_MiniTable_GetFloat(const upb_Message* msg,
                                        const upb_MiniTableField* field,
                                        float default_val) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Float);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_4Byte);
  float ret;
  _upb_MiniTable_GetNonExtensionField(msg, field, &default_val, &ret);
  return ret;
}

UPB_INLINE void upb_MiniTable_SetFloat(upb_Message* msg,
                                       const upb_MiniTableField* field,
                                       float value) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Float);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_4Byte);
  _upb_MiniTable_SetNonExtensionField(msg, field, &value);
}

UPB_INLINE double upb_MiniTable_GetDouble(const upb_Message* msg,
                                          const upb_MiniTableField* field,
                                          double default_val) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Double);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_8Byte);
  double ret;
  _upb_MiniTable_GetNonExtensionField(msg, field, &default_val, &ret);
  return ret;
}

UPB_INLINE void upb_MiniTable_SetDouble(upb_Message* msg,
                                        const upb_MiniTableField* field,
                                        double value) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Double);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_8Byte);
  _upb_MiniTable_SetNonExtensionField(msg, field, &value);
}

UPB_INLINE upb_StringView
upb_MiniTable_GetString(const upb_Message* msg, const upb_MiniTableField* field,
                        upb_StringView def_val) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Bytes ||
             field->descriptortype == kUpb_FieldType_String);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_StringView);
  upb_StringView ret;
  _upb_MiniTable_GetNonExtensionField(msg, field, &def_val, &ret);
  return ret;
}

UPB_INLINE void upb_MiniTable_SetString(upb_Message* msg,
                                        const upb_MiniTableField* field,
                                        upb_StringView value) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Bytes ||
             field->descriptortype == kUpb_FieldType_String);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_StringView);
  _upb_MiniTable_SetNonExtensionField(msg, field, &value);
}

UPB_INLINE const upb_Message* upb_MiniTable_GetMessage(
    const upb_Message* msg, const upb_MiniTableField* field,
    upb_Message* default_val) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Message ||
             field->descriptortype == kUpb_FieldType_Group);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) ==
             UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte));
  upb_Message* ret;
  _upb_MiniTable_GetNonExtensionField(msg, field, &default_val, &ret);
  return ret;
}

UPB_INLINE void upb_MiniTable_SetMessage(upb_Message* msg,
                                         const upb_MiniTable* mini_table,
                                         const upb_MiniTableField* field,
                                         upb_Message* sub_message) {
  UPB_ASSERT(field->descriptortype == kUpb_FieldType_Message ||
             field->descriptortype == kUpb_FieldType_Group);
  UPB_ASSUME(!upb_IsRepeatedOrMap(field));
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) ==
             UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte));
  UPB_ASSERT(mini_table->subs[field->submsg_index].submsg);
  _upb_MiniTable_SetNonExtensionField(msg, field, &sub_message);
}

UPB_INLINE upb_Message* upb_MiniTable_GetMutableMessage(
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
    _upb_MiniTable_SetPresence(msg, field);
  }
  return sub_message;
}

UPB_INLINE const upb_Array* upb_MiniTable_GetArray(
    const upb_Message* msg, const upb_MiniTableField* field) {
  const upb_Array* ret;
  const upb_Array* default_val = NULL;
  _upb_MiniTable_GetNonExtensionField(msg, field, &default_val, &ret);
  return ret;
}

UPB_INLINE upb_Array* upb_MiniTable_GetMutableArray(
    upb_Message* msg, const upb_MiniTableField* field) {
  return (upb_Array*)upb_MiniTable_GetArray(msg, field);
}

void* upb_MiniTable_ResizeArray(upb_Message* msg,
                                const upb_MiniTableField* field, size_t len,
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

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif  // UPB_MESSAGE_ACCESSORS_H_
