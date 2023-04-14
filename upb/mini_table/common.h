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

#ifndef UPB_MINI_TABLE_COMMON_H_
#define UPB_MINI_TABLE_COMMON_H_

#include "upb/mini_table/field_internal.h"
#include "upb/mini_table/message_internal.h"
#include "upb/mini_table/sub_internal.h"

// Must be last.
#include "upb/port/def.inc"

typedef enum {
  kUpb_FieldModifier_IsRepeated = 1 << 0,
  kUpb_FieldModifier_IsPacked = 1 << 1,
  kUpb_FieldModifier_IsClosedEnum = 1 << 2,
  kUpb_FieldModifier_IsProto3Singular = 1 << 3,
  kUpb_FieldModifier_IsRequired = 1 << 4,
} kUpb_FieldModifier;

typedef enum {
  kUpb_MessageModifier_ValidateUtf8 = 1 << 0,
  kUpb_MessageModifier_DefaultIsPacked = 1 << 1,
  kUpb_MessageModifier_IsExtendable = 1 << 2,
} kUpb_MessageModifier;

#ifdef __cplusplus
extern "C" {
#endif

UPB_API const upb_MiniTableField* upb_MiniTable_FindFieldByNumber(
    const upb_MiniTable* table, uint32_t number);

UPB_API_INLINE const upb_MiniTableField* upb_MiniTable_GetFieldByIndex(
    const upb_MiniTable* t, uint32_t index) {
  return &t->fields[index];
}

UPB_API upb_FieldType upb_MiniTableField_Type(const upb_MiniTableField* field);

UPB_API_INLINE upb_CType upb_MiniTableField_CType(const upb_MiniTableField* f) {
  switch (f->UPB_PRIVATE(descriptortype)) {
    case kUpb_FieldType_Double:
      return kUpb_CType_Double;
    case kUpb_FieldType_Float:
      return kUpb_CType_Float;
    case kUpb_FieldType_Int64:
    case kUpb_FieldType_SInt64:
    case kUpb_FieldType_SFixed64:
      return kUpb_CType_Int64;
    case kUpb_FieldType_Int32:
    case kUpb_FieldType_SFixed32:
    case kUpb_FieldType_SInt32:
      return kUpb_CType_Int32;
    case kUpb_FieldType_UInt64:
    case kUpb_FieldType_Fixed64:
      return kUpb_CType_UInt64;
    case kUpb_FieldType_UInt32:
    case kUpb_FieldType_Fixed32:
      return kUpb_CType_UInt32;
    case kUpb_FieldType_Enum:
      return kUpb_CType_Enum;
    case kUpb_FieldType_Bool:
      return kUpb_CType_Bool;
    case kUpb_FieldType_String:
      return kUpb_CType_String;
    case kUpb_FieldType_Bytes:
      return kUpb_CType_Bytes;
    case kUpb_FieldType_Group:
    case kUpb_FieldType_Message:
      return kUpb_CType_Message;
  }
  UPB_UNREACHABLE();
}

UPB_API_INLINE bool upb_MiniTableField_IsExtension(
    const upb_MiniTableField* field) {
  return field->mode & kUpb_LabelFlags_IsExtension;
}

UPB_API_INLINE bool upb_MiniTableField_IsClosedEnum(
    const upb_MiniTableField* field) {
  return field->UPB_PRIVATE(descriptortype) == kUpb_FieldType_Enum;
}

UPB_API_INLINE bool upb_MiniTableField_HasPresence(
    const upb_MiniTableField* field) {
  if (upb_MiniTableField_IsExtension(field)) {
    return !upb_IsRepeatedOrMap(field);
  } else {
    return field->presence != 0;
  }
}

// Returns the MiniTable for this message field.  If the field is unlinked,
// returns NULL.
UPB_API_INLINE const upb_MiniTable* upb_MiniTable_GetSubMessageTable(
    const upb_MiniTable* mini_table, const upb_MiniTableField* field) {
  UPB_ASSERT(upb_MiniTableField_CType(field) == kUpb_CType_Message);
  return mini_table->subs[field->UPB_PRIVATE(submsg_index)].submsg;
}

// Returns the MiniTableEnum for this enum field.  If the field is unlinked,
// returns NULL.
UPB_API_INLINE const upb_MiniTableEnum* upb_MiniTable_GetSubEnumTable(
    const upb_MiniTable* mini_table, const upb_MiniTableField* field) {
  UPB_ASSERT(upb_MiniTableField_CType(field) == kUpb_CType_Enum);
  return mini_table->subs[field->UPB_PRIVATE(submsg_index)].subenum;
}

// Returns true if this MiniTable field is linked to a MiniTable for the
// sub-message.
UPB_API_INLINE bool upb_MiniTable_MessageFieldIsLinked(
    const upb_MiniTable* mini_table, const upb_MiniTableField* field) {
  return upb_MiniTable_GetSubMessageTable(mini_table, field) != NULL;
}

// If this field is in a oneof, returns the first field in the oneof.
//
// Otherwise returns NULL.
//
// Usage:
//   const upb_MiniTableField* field = upb_MiniTable_GetOneof(m, f);
//   do {
//       ..
//   } while (upb_MiniTable_NextOneofField(m, &field);
//
const upb_MiniTableField* upb_MiniTable_GetOneof(const upb_MiniTable* m,
                                                 const upb_MiniTableField* f);

// Iterates to the next field in the oneof. If this is the last field in the
// oneof, returns false. The ordering of fields in the oneof is not
// guaranteed.
// REQUIRES: |f| is the field initialized by upb_MiniTable_GetOneof and updated
//           by prior upb_MiniTable_NextOneofField calls.
bool upb_MiniTable_NextOneofField(const upb_MiniTable* m,
                                  const upb_MiniTableField** f);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MINI_TABLE_COMMON_H_ */
