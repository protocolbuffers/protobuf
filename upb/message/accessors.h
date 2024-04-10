// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MESSAGE_ACCESSORS_H_
#define UPB_MESSAGE_ACCESSORS_H_

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "upb/base/descriptor_constants.h"
#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include "upb/message/internal/accessors.h"
#include "upb/message/internal/array.h"
#include "upb/message/internal/map.h"
#include "upb/message/internal/message.h"
#include "upb/message/internal/tagged_ptr.h"
#include "upb/message/map.h"
#include "upb/message/tagged_ptr.h"
#include "upb/message/value.h"
#include "upb/mini_table/enum.h"
#include "upb/mini_table/sub.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

// Functions ending in BaseField() take a (upb_MiniTableField*) argument
// and work only on non-extension fields.
//
// Functions ending in Extension() take a (upb_MiniTableExtension*) argument
// and work only on extensions.

UPB_API_INLINE void upb_Message_Clear(upb_Message* msg, const upb_MiniTable* m);

UPB_API_INLINE void upb_Message_ClearBaseField(upb_Message* msg,
                                               const upb_MiniTableField* f);

UPB_API_INLINE void upb_Message_ClearExtension(upb_Message* msg,
                                               const upb_MiniTableExtension* e);

UPB_API_INLINE bool upb_Message_HasBaseField(const upb_Message* msg,
                                             const upb_MiniTableField* f);

UPB_API_INLINE bool upb_Message_HasExtension(const upb_Message* msg,
                                             const upb_MiniTableExtension* e);

UPB_API_INLINE void upb_Message_SetBaseField(upb_Message* msg,
                                             const upb_MiniTableField* f,
                                             const void* val);

UPB_API_INLINE bool upb_Message_SetExtension(upb_Message* msg,
                                             const upb_MiniTableExtension* e,
                                             const void* val, upb_Arena* a);

UPB_API_INLINE uint32_t upb_Message_WhichOneofFieldNumber(
    const upb_Message* message, const upb_MiniTableField* oneof_field) {
  UPB_ASSUME(upb_MiniTableField_IsInOneof(oneof_field));
  return UPB_PRIVATE(_upb_Message_GetOneofCase)(message, oneof_field);
}

// NOTE: The default_val is only used for fields that support presence.
// For repeated/map fields, the resulting upb_Array*/upb_Map* can be NULL if a
// upb_Array/upb_Map has not been allocated yet. Array/map fields do not have
// presence, so this is semantically identical to a pointer to an empty
// array/map, and must be treated the same for all semantic purposes.
UPB_INLINE upb_MessageValue
upb_Message_GetField(const upb_Message* msg, const upb_MiniTableField* field,
                     upb_MessageValue default_val) {
  upb_MessageValue ret;
  if (upb_MiniTableField_IsExtension(field)) {
    _upb_Message_GetExtensionField(msg, (upb_MiniTableExtension*)field,
                                   &default_val, &ret);
  } else {
    _upb_Message_GetNonExtensionField(msg, field, &default_val, &ret);
  }
  return ret;
}

// Sets the value of the given field in the given msg. The return value is true
// if the operation completed successfully, or false if memory allocation
// failed.
UPB_INLINE bool UPB_PRIVATE(_upb_Message_SetField)(
    upb_Message* msg, const upb_MiniTableField* field, upb_MessageValue val,
    upb_Arena* a) {
  if (upb_MiniTableField_IsExtension(field)) {
    const upb_MiniTableExtension* ext = (const upb_MiniTableExtension*)field;
    return upb_Message_SetExtension(msg, ext, &val, a);
  } else {
    upb_Message_SetBaseField(msg, field, &val);
    return true;
  }
}

UPB_API_INLINE bool upb_Message_GetBool(const upb_Message* msg,
                                        const upb_MiniTableField* field,
                                        bool default_val) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Bool);
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(field) ==
             kUpb_FieldRep_1Byte);
  UPB_ASSUME(upb_MiniTableField_IsScalar(field));
  upb_MessageValue def;
  def.bool_val = default_val;
  return upb_Message_GetField(msg, field, def).bool_val;
}

UPB_API_INLINE bool upb_Message_SetBool(upb_Message* msg,
                                        const upb_MiniTableField* field,
                                        bool value, upb_Arena* a) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Bool);
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(field) ==
             kUpb_FieldRep_1Byte);
  UPB_ASSUME(upb_MiniTableField_IsScalar(field));
  upb_MessageValue val;
  val.bool_val = value;
  return UPB_PRIVATE(_upb_Message_SetField)(msg, field, val, a);
}

UPB_API_INLINE int32_t upb_Message_GetInt32(const upb_Message* msg,
                                            const upb_MiniTableField* field,
                                            int32_t default_val) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Int32 ||
             upb_MiniTableField_CType(field) == kUpb_CType_Enum);
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(field) ==
             kUpb_FieldRep_4Byte);
  UPB_ASSUME(upb_MiniTableField_IsScalar(field));

  upb_MessageValue def;
  def.int32_val = default_val;
  return upb_Message_GetField(msg, field, def).int32_val;
}

UPB_API_INLINE bool upb_Message_SetInt32(upb_Message* msg,
                                         const upb_MiniTableField* field,
                                         int32_t value, upb_Arena* a) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Int32 ||
             upb_MiniTableField_CType(field) == kUpb_CType_Enum);
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(field) ==
             kUpb_FieldRep_4Byte);
  UPB_ASSUME(upb_MiniTableField_IsScalar(field));
  upb_MessageValue val;
  val.int32_val = value;
  return UPB_PRIVATE(_upb_Message_SetField)(msg, field, val, a);
}

UPB_API_INLINE uint32_t upb_Message_GetUInt32(const upb_Message* msg,
                                              const upb_MiniTableField* field,
                                              uint32_t default_val) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_UInt32);
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(field) ==
             kUpb_FieldRep_4Byte);
  UPB_ASSUME(upb_MiniTableField_IsScalar(field));

  upb_MessageValue def;
  def.uint32_val = default_val;
  return upb_Message_GetField(msg, field, def).uint32_val;
}

UPB_API_INLINE bool upb_Message_SetUInt32(upb_Message* msg,
                                          const upb_MiniTableField* field,
                                          uint32_t value, upb_Arena* a) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_UInt32);
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(field) ==
             kUpb_FieldRep_4Byte);
  UPB_ASSUME(upb_MiniTableField_IsScalar(field));
  upb_MessageValue val;
  val.uint32_val = value;
  return UPB_PRIVATE(_upb_Message_SetField)(msg, field, val, a);
}

UPB_API_INLINE void upb_Message_SetClosedEnum(
    upb_Message* msg, const upb_MiniTable* msg_mini_table,
    const upb_MiniTableField* field, int32_t value) {
  UPB_ASSERT(upb_MiniTableField_IsClosedEnum(field));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(field) ==
             kUpb_FieldRep_4Byte);
  UPB_ASSUME(upb_MiniTableField_IsScalar(field));
  UPB_ASSERT(upb_MiniTableEnum_CheckValue(
      upb_MiniTable_GetSubEnumTable(msg_mini_table, field), value));
  upb_Message_SetBaseField(msg, field, &value);
}

UPB_API_INLINE int64_t upb_Message_GetInt64(const upb_Message* msg,
                                            const upb_MiniTableField* field,
                                            int64_t default_val) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Int64);
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(field) ==
             kUpb_FieldRep_8Byte);
  UPB_ASSUME(upb_MiniTableField_IsScalar(field));

  upb_MessageValue def;
  def.int64_val = default_val;
  return upb_Message_GetField(msg, field, def).int64_val;
}

UPB_API_INLINE bool upb_Message_SetInt64(upb_Message* msg,
                                         const upb_MiniTableField* field,
                                         int64_t value, upb_Arena* a) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Int64);
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(field) ==
             kUpb_FieldRep_8Byte);
  UPB_ASSUME(upb_MiniTableField_IsScalar(field));
  upb_MessageValue val;
  val.int64_val = value;
  return UPB_PRIVATE(_upb_Message_SetField)(msg, field, val, a);
}

UPB_API_INLINE uint64_t upb_Message_GetUInt64(const upb_Message* msg,
                                              const upb_MiniTableField* field,
                                              uint64_t default_val) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_UInt64);
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(field) ==
             kUpb_FieldRep_8Byte);
  UPB_ASSUME(upb_MiniTableField_IsScalar(field));

  upb_MessageValue def;
  def.uint64_val = default_val;
  return upb_Message_GetField(msg, field, def).uint64_val;
}

UPB_API_INLINE bool upb_Message_SetUInt64(upb_Message* msg,
                                          const upb_MiniTableField* field,
                                          uint64_t value, upb_Arena* a) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_UInt64);
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(field) ==
             kUpb_FieldRep_8Byte);
  UPB_ASSUME(upb_MiniTableField_IsScalar(field));
  upb_MessageValue val;
  val.uint64_val = value;
  return UPB_PRIVATE(_upb_Message_SetField)(msg, field, val, a);
}

UPB_API_INLINE float upb_Message_GetFloat(const upb_Message* msg,
                                          const upb_MiniTableField* field,
                                          float default_val) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Float);
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(field) ==
             kUpb_FieldRep_4Byte);
  UPB_ASSUME(upb_MiniTableField_IsScalar(field));

  upb_MessageValue def;
  def.float_val = default_val;
  return upb_Message_GetField(msg, field, def).float_val;
}

UPB_API_INLINE bool upb_Message_SetFloat(upb_Message* msg,
                                         const upb_MiniTableField* field,
                                         float value, upb_Arena* a) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Float);
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(field) ==
             kUpb_FieldRep_4Byte);
  UPB_ASSUME(upb_MiniTableField_IsScalar(field));
  upb_MessageValue val;
  val.float_val = value;
  return UPB_PRIVATE(_upb_Message_SetField)(msg, field, val, a);
}

UPB_API_INLINE double upb_Message_GetDouble(const upb_Message* msg,
                                            const upb_MiniTableField* field,
                                            double default_val) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Double);
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(field) ==
             kUpb_FieldRep_8Byte);
  UPB_ASSUME(upb_MiniTableField_IsScalar(field));

  upb_MessageValue def;
  def.double_val = default_val;
  return upb_Message_GetField(msg, field, def).double_val;
}

UPB_API_INLINE bool upb_Message_SetDouble(upb_Message* msg,
                                          const upb_MiniTableField* field,
                                          double value, upb_Arena* a) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Double);
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(field) ==
             kUpb_FieldRep_8Byte);
  UPB_ASSUME(upb_MiniTableField_IsScalar(field));
  upb_MessageValue val;
  val.double_val = value;
  return UPB_PRIVATE(_upb_Message_SetField)(msg, field, val, a);
}

UPB_API_INLINE upb_StringView
upb_Message_GetString(const upb_Message* msg, const upb_MiniTableField* field,
                      upb_StringView default_val) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_String ||
             upb_MiniTableField_CType(field) == kUpb_CType_Bytes);
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(field) ==
             kUpb_FieldRep_StringView);
  UPB_ASSUME(upb_MiniTableField_IsScalar(field));

  upb_MessageValue def;
  def.str_val = default_val;
  return upb_Message_GetField(msg, field, def).str_val;
}

// Sets the value of a `string` or `bytes` field. The bytes of the value are not
// copied, so it is the caller's responsibility to ensure that they remain valid
// for the lifetime of `msg`. That might be done by copying them into the given
// arena, or by fusing that arena with the arena the bytes live in, for example.
UPB_API_INLINE bool upb_Message_SetString(upb_Message* msg,
                                          const upb_MiniTableField* field,
                                          upb_StringView value, upb_Arena* a) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_String ||
             upb_MiniTableField_CType(field) == kUpb_CType_Bytes);
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(field) ==
             kUpb_FieldRep_StringView);
  UPB_ASSUME(upb_MiniTableField_IsScalar(field));
  upb_MessageValue val;
  val.str_val = value;
  return UPB_PRIVATE(_upb_Message_SetField)(msg, field, val, a);
}

UPB_API_INLINE upb_TaggedMessagePtr upb_Message_GetTaggedMessagePtr(
    const upb_Message* msg, const upb_MiniTableField* field,
    upb_Message* default_val) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Message);
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(field) ==
             UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte));
  UPB_ASSUME(upb_MiniTableField_IsScalar(field));
  upb_TaggedMessagePtr tagged;
  _upb_Message_GetNonExtensionField(msg, field, &default_val, &tagged);
  return tagged;
}

UPB_API_INLINE const upb_Message* upb_Message_GetMessage(
    const upb_Message* msg, const upb_MiniTableField* field) {
  upb_TaggedMessagePtr tagged =
      upb_Message_GetTaggedMessagePtr(msg, field, NULL);
  return upb_TaggedMessagePtr_GetNonEmptyMessage(tagged);
}

UPB_API_INLINE upb_Message* upb_Message_GetMutableMessage(
    upb_Message* msg, const upb_MiniTableField* field) {
  return (upb_Message*)upb_Message_GetMessage(msg, field);
}

// For internal use only; users cannot set tagged messages because only the
// parser and the message copier are allowed to directly create an empty
// message.
UPB_API_INLINE void _upb_Message_SetTaggedMessagePtr(
    upb_Message* msg, const upb_MiniTable* mini_table,
    const upb_MiniTableField* field, upb_TaggedMessagePtr sub_message) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Message);
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(field) ==
             UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte));
  UPB_ASSUME(upb_MiniTableField_IsScalar(field));
  UPB_ASSERT(upb_MiniTableSub_Message(
      mini_table->UPB_PRIVATE(subs)[field->UPB_PRIVATE(submsg_index)]));
  upb_Message_SetBaseField(msg, field, &sub_message);
}

// Sets the value of a message-typed field. The `mini_table` and `field`
// parameters belong to `msg`, not `sub_message`. The mini_tables of `msg` and
// `sub_message` must have been linked for this to work correctly.
UPB_API_INLINE void upb_Message_SetMessage(upb_Message* msg,
                                           const upb_MiniTable* mini_table,
                                           const upb_MiniTableField* field,
                                           upb_Message* sub_message) {
  _upb_Message_SetTaggedMessagePtr(
      msg, mini_table, field,
      UPB_PRIVATE(_upb_TaggedMessagePtr_Pack)(sub_message, false));
}

UPB_API_INLINE upb_Message* upb_Message_GetOrCreateMutableMessage(
    upb_Message* msg, const upb_MiniTable* mini_table,
    const upb_MiniTableField* field, upb_Arena* arena) {
  UPB_ASSERT(arena);
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Message);
  upb_Message* sub_message =
      *UPB_PTR_AT(msg, field->UPB_ONLYBITS(offset), upb_Message*);
  if (!sub_message) {
    const upb_MiniTable* sub_mini_table = upb_MiniTableSub_Message(
        mini_table->UPB_PRIVATE(subs)[field->UPB_PRIVATE(submsg_index)]);
    UPB_ASSERT(sub_mini_table);
    sub_message = _upb_Message_New(sub_mini_table, arena);
    *UPB_PTR_AT(msg, field->UPB_ONLYBITS(offset), upb_Message*) = sub_message;
    UPB_PRIVATE(_upb_Message_SetPresence)(msg, field);
  }
  return sub_message;
}

UPB_API_INLINE const upb_Array* upb_Message_GetArray(
    const upb_Message* msg, const upb_MiniTableField* field) {
  UPB_PRIVATE(_upb_MiniTableField_CheckIsArray)(field);
  upb_Array* ret;
  const upb_Array* default_val = NULL;
  _upb_Message_GetNonExtensionField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE upb_Array* upb_Message_GetMutableArray(
    upb_Message* msg, const upb_MiniTableField* field) {
  UPB_PRIVATE(_upb_MiniTableField_CheckIsArray)(field);
  return (upb_Array*)upb_Message_GetArray(msg, field);
}

UPB_API_INLINE upb_Array* upb_Message_GetOrCreateMutableArray(
    upb_Message* msg, const upb_MiniTableField* field, upb_Arena* arena) {
  UPB_ASSERT(arena);
  UPB_PRIVATE(_upb_MiniTableField_CheckIsArray)(field);
  upb_Array* array = upb_Message_GetMutableArray(msg, field);
  if (!array) {
    array = UPB_PRIVATE(_upb_Array_New)(
        arena, 4, UPB_PRIVATE(_upb_MiniTableField_ElemSizeLg2)(field));
    // Check again due to: https://godbolt.org/z/7WfaoKG1r
    UPB_PRIVATE(_upb_MiniTableField_CheckIsArray)(field);
    upb_MessageValue val;
    val.array_val = array;
    UPB_PRIVATE(_upb_Message_SetField)(msg, field, val, arena);
  }
  return array;
}

UPB_API_INLINE void* upb_Message_ResizeArrayUninitialized(
    upb_Message* msg, const upb_MiniTableField* field, size_t size,
    upb_Arena* arena) {
  UPB_PRIVATE(_upb_MiniTableField_CheckIsArray)(field);
  upb_Array* arr = upb_Message_GetOrCreateMutableArray(msg, field, arena);
  if (!arr || !UPB_PRIVATE(_upb_Array_ResizeUninitialized)(arr, size, arena)) {
    return NULL;
  }
  return upb_Array_MutableDataPtr(arr);
}

UPB_API_INLINE const upb_Map* upb_Message_GetMap(
    const upb_Message* msg, const upb_MiniTableField* field) {
  UPB_PRIVATE(_upb_MiniTableField_CheckIsMap)(field);
  _upb_Message_AssertMapIsUntagged(msg, field);
  upb_Map* ret;
  const upb_Map* default_val = NULL;
  _upb_Message_GetNonExtensionField(msg, field, &default_val, &ret);
  return ret;
}

UPB_API_INLINE upb_Map* upb_Message_GetMutableMap(
    upb_Message* msg, const upb_MiniTableField* field) {
  return (upb_Map*)upb_Message_GetMap(msg, field);
}

UPB_API_INLINE upb_Map* upb_Message_GetOrCreateMutableMap(
    upb_Message* msg, const upb_MiniTable* map_entry_mini_table,
    const upb_MiniTableField* field, upb_Arena* arena) {
  UPB_ASSUME(upb_MiniTableField_CType(field) == kUpb_CType_Message);
  const upb_MiniTableField* map_entry_key_field =
      &map_entry_mini_table->UPB_ONLYBITS(fields)[0];
  const upb_MiniTableField* map_entry_value_field =
      &map_entry_mini_table->UPB_ONLYBITS(fields)[1];
  return _upb_Message_GetOrCreateMutableMap(
      msg, field,
      _upb_Map_CTypeSize(upb_MiniTableField_CType(map_entry_key_field)),
      _upb_Map_CTypeSize(upb_MiniTableField_CType(map_entry_value_field)),
      arena);
}

// Updates a map entry given an entry message.
bool upb_Message_SetMapEntry(upb_Map* map, const upb_MiniTable* mini_table,
                             const upb_MiniTableField* field,
                             upb_Message* map_entry_message, upb_Arena* arena);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif  // UPB_MESSAGE_ACCESSORS_H_
