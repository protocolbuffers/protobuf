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

UPB_API upb_FieldType upb_MiniTableField_Type(const upb_MiniTableField* field);

UPB_API upb_CType upb_MiniTableField_CType(const upb_MiniTableField* field);

UPB_API_INLINE bool upb_MiniTableField_IsExtension(
    const upb_MiniTableField* field) {
  return field->mode & kUpb_LabelFlags_IsExtension;
}

UPB_API_INLINE bool upb_MiniTableField_HasPresence(
    const upb_MiniTableField* field) {
  if (upb_MiniTableField_IsExtension(field)) {
    return !upb_IsRepeatedOrMap(field);
  } else {
    return field->presence != 0;
  }
}

UPB_API_INLINE const upb_MiniTable* upb_MiniTable_GetSubMessageTable(
    const upb_MiniTable* mini_table, const upb_MiniTableField* field) {
  return mini_table->subs[field->submsg_index].submsg;
}

UPB_API_INLINE const upb_MiniTableEnum* upb_MiniTable_GetSubEnumTable(
    const upb_MiniTable* mini_table, const upb_MiniTableField* field) {
  return mini_table->subs[field->submsg_index].subenum;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MINI_TABLE_COMMON_H_ */
