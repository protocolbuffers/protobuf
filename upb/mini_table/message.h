// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MINI_TABLE_MESSAGE_H_
#define UPB_MINI_TABLE_MESSAGE_H_

#include "upb/mini_table/enum.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/message.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct upb_MiniTable upb_MiniTable;

UPB_API const upb_MiniTableField* upb_MiniTable_FindFieldByNumber(
    const upb_MiniTable* table, uint32_t number);

UPB_API_INLINE const upb_MiniTableField* upb_MiniTable_GetFieldByIndex(
    const upb_MiniTable* t, uint32_t index) {
  return &t->fields[index];
}

// Returns the MiniTable for this message field.  If the field is unlinked,
// returns NULL.
UPB_API_INLINE const upb_MiniTable* upb_MiniTable_GetSubMessageTable(
    const upb_MiniTable* mini_table, const upb_MiniTableField* field) {
  UPB_ASSERT(upb_MiniTableField_CType(field) == kUpb_CType_Message);
  const upb_MiniTable* ret =
      mini_table->subs[field->UPB_PRIVATE(submsg_index)].submsg;
  UPB_ASSUME(ret);
  return ret == &_kUpb_MiniTable_Empty ? NULL : ret;
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

#endif /* UPB_MINI_TABLE_MESSAGE_H_ */
