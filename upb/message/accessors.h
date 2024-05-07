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
#include "upb/message/message.h"
#include "upb/message/tagged_ptr.h"
#include "upb/message/value.h"
#include "upb/mini_table/enum.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/message.h"
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

UPB_API_INLINE void upb_Message_ClearOneof(upb_Message* msg,
                                           const upb_MiniTable* m,
                                           const upb_MiniTableField* f);

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

UPB_API_INLINE upb_MessageValue
upb_Message_GetField(const upb_Message* msg, const upb_MiniTableField* f,
                     upb_MessageValue default_val);

UPB_API_INLINE const upb_Array* upb_Message_GetArray(
    const upb_Message* msg, const upb_MiniTableField* f);

UPB_API_INLINE bool upb_Message_GetBool(const upb_Message* msg,
                                        const upb_MiniTableField* f,
                                        bool default_val);

UPB_API_INLINE double upb_Message_GetDouble(const upb_Message* msg,
                                            const upb_MiniTableField* field,
                                            double default_val);

UPB_API_INLINE float upb_Message_GetFloat(const upb_Message* msg,
                                          const upb_MiniTableField* f,
                                          float default_val);

UPB_API_INLINE int32_t upb_Message_GetInt32(const upb_Message* msg,
                                            const upb_MiniTableField* f,
                                            int32_t default_val);

UPB_API_INLINE int64_t upb_Message_GetInt64(const upb_Message* msg,
                                            const upb_MiniTableField* f,
                                            int64_t default_val);

UPB_API_INLINE const upb_Map* upb_Message_GetMap(const upb_Message* msg,
                                                 const upb_MiniTableField* f);

UPB_API_INLINE upb_Array* upb_Message_GetMutableArray(
    upb_Message* msg, const upb_MiniTableField* f);

UPB_API_INLINE upb_Map* upb_Message_GetMutableMap(upb_Message* msg,
                                                  const upb_MiniTableField* f);

UPB_API_INLINE upb_Array* upb_Message_GetOrCreateMutableArray(
    upb_Message* msg, const upb_MiniTableField* f, upb_Arena* arena);

UPB_API_INLINE upb_Map* upb_Message_GetOrCreateMutableMap(
    upb_Message* msg, const upb_MiniTable* map_entry_mini_table,
    const upb_MiniTableField* f, upb_Arena* arena);

UPB_API_INLINE upb_Message* upb_Message_GetOrCreateMutableMessage(
    upb_Message* msg, const upb_MiniTable* mini_table,
    const upb_MiniTableField* f, upb_Arena* arena);

UPB_API_INLINE upb_StringView
upb_Message_GetString(const upb_Message* msg, const upb_MiniTableField* field,
                      upb_StringView default_val);

UPB_API_INLINE uint32_t upb_Message_GetUInt32(const upb_Message* msg,
                                              const upb_MiniTableField* f,
                                              uint32_t default_val);

UPB_API_INLINE uint64_t upb_Message_GetUInt64(const upb_Message* msg,
                                              const upb_MiniTableField* f,
                                              uint64_t default_val);

UPB_API_INLINE bool upb_Message_SetBool(upb_Message* msg,
                                        const upb_MiniTableField* f, bool value,
                                        upb_Arena* a);

UPB_API_INLINE void upb_Message_SetClosedEnum(
    upb_Message* msg, const upb_MiniTable* msg_mini_table,
    const upb_MiniTableField* f, int32_t value);

UPB_API_INLINE bool upb_Message_SetDouble(upb_Message* msg,
                                          const upb_MiniTableField* f,
                                          double value, upb_Arena* a);

UPB_API_INLINE bool upb_Message_SetFloat(upb_Message* msg,
                                         const upb_MiniTableField* f,
                                         float value, upb_Arena* a);

UPB_API_INLINE bool upb_Message_SetInt32(upb_Message* msg,
                                         const upb_MiniTableField* f,
                                         int32_t value, upb_Arena* a);

UPB_API_INLINE bool upb_Message_SetInt64(upb_Message* msg,
                                         const upb_MiniTableField* f,
                                         int64_t value, upb_Arena* a);

UPB_API_INLINE bool upb_Message_SetString(upb_Message* msg,
                                          const upb_MiniTableField* f,
                                          upb_StringView value, upb_Arena* a);

UPB_API_INLINE bool upb_Message_SetUInt32(upb_Message* msg,
                                          const upb_MiniTableField* f,
                                          uint32_t value, upb_Arena* a);

UPB_API_INLINE bool upb_Message_SetUInt64(upb_Message* msg,
                                          const upb_MiniTableField* f,
                                          uint64_t value, upb_Arena* a);

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
UPB_INLINE void UPB_PRIVATE(_upb_Message_SetTaggedMessagePtr)(
    struct upb_Message* msg, const upb_MiniTable* mini_table,
    const upb_MiniTableField* f, upb_TaggedMessagePtr sub_message) {
  UPB_ASSUME(upb_MiniTableField_CType(f) == kUpb_CType_Message);
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(f) ==
             UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte));
  UPB_ASSUME(upb_MiniTableField_IsScalar(f));
  upb_Message_SetBaseField(msg, f, &sub_message);
}

// Sets the value of a message-typed field. The `mini_table` and `field`
// parameters belong to `msg`, not `sub_message`. The mini_tables of `msg` and
// `sub_message` must have been linked for this to work correctly.
UPB_API_INLINE void upb_Message_SetMessage(upb_Message* msg,
                                           const upb_MiniTable* mini_table,
                                           const upb_MiniTableField* field,
                                           upb_Message* sub_message) {
  UPB_PRIVATE(_upb_Message_SetTaggedMessagePtr)
  (msg, mini_table, field,
   UPB_PRIVATE(_upb_TaggedMessagePtr_Pack)(sub_message, false));
}

UPB_API_INLINE void* upb_Message_ResizeArrayUninitialized(
    upb_Message* msg, const upb_MiniTableField* f, size_t size,
    upb_Arena* arena);

UPB_API_INLINE uint32_t upb_Message_WhichOneofFieldNumber(
    const upb_Message* message, const upb_MiniTableField* oneof_field);

// Updates a map entry given an entry message.
bool upb_Message_SetMapEntry(upb_Map* map, const upb_MiniTable* mini_table,
                             const upb_MiniTableField* field,
                             upb_Message* map_entry_message, upb_Arena* arena);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif  // UPB_MESSAGE_ACCESSORS_H_
