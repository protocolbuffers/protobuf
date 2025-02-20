// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/mini_descriptor/link.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "upb/base/descriptor_constants.h"
#include "upb/mini_table/enum.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/internal/sub.h"
#include "upb/mini_table/message.h"
#include "upb/mini_table/sub.h"

// Must be last.
#include "upb/port/def.inc"

bool upb_MiniTable_SetSubMessage(upb_MiniTable* table,
                                 upb_MiniTableField* field,
                                 const upb_MiniTable* sub) {
  UPB_ASSERT((uintptr_t)table->UPB_PRIVATE(fields) <= (uintptr_t)field &&
             (uintptr_t)field < (uintptr_t)(table->UPB_PRIVATE(fields) +
                                            table->UPB_PRIVATE(field_count)));
  UPB_ASSERT(sub);

  const bool sub_is_map = sub->UPB_PRIVATE(ext) & kUpb_ExtMode_IsMapEntry;

  switch (field->UPB_PRIVATE(descriptortype)) {
    case kUpb_FieldType_Message:
      if (sub_is_map) {
        const bool table_is_map =
            table->UPB_PRIVATE(ext) & kUpb_ExtMode_IsMapEntry;
        if (UPB_UNLIKELY(table_is_map)) return false;

        field->UPB_PRIVATE(mode) =
            (field->UPB_PRIVATE(mode) & ~kUpb_FieldMode_Mask) |
            kUpb_FieldMode_Map;
      }
      break;

    case kUpb_FieldType_Group:
      if (UPB_UNLIKELY(sub_is_map)) return false;
      break;

    default:
      return false;
  }

  int idx = field->UPB_PRIVATE(submsg_index);
  upb_MiniTableSubInternal* table_subs = (void*)table->UPB_PRIVATE(subs);
  // TODO: Add this assert back once YouTube is updated to not call
  // this function repeatedly.
  // UPB_ASSERT(UPB_PRIVATE(_upb_MiniTable_IsEmpty)(table_sub->submsg));
  memcpy((void*)table_subs[idx].UPB_PRIVATE(submsg), &sub, sizeof(void*));
  return true;
}

bool upb_MiniTable_SetSubEnum(upb_MiniTable* table, upb_MiniTableField* field,
                              const upb_MiniTableEnum* sub) {
  UPB_ASSERT((uintptr_t)table->UPB_PRIVATE(fields) <= (uintptr_t)field &&
             (uintptr_t)field < (uintptr_t)(table->UPB_PRIVATE(fields) +
                                            table->UPB_PRIVATE(field_count)));
  UPB_ASSERT(sub);

  if (field->UPB_PRIVATE(descriptortype) != kUpb_FieldType_Enum) {
    return false;
  }

  upb_MiniTableSub* table_sub =
      (void*)&table->UPB_PRIVATE(subs)[field->UPB_PRIVATE(submsg_index)];
  *table_sub = upb_MiniTableSub_FromEnum(sub);
  return true;
}

uint32_t upb_MiniTable_GetSubList(const upb_MiniTable* m,
                                  const upb_MiniTableField** subs) {
  uint32_t msg_count = 0;
  uint32_t enum_count = 0;

  for (int i = 0; i < upb_MiniTable_FieldCount(m); i++) {
    const upb_MiniTableField* f = upb_MiniTable_GetFieldByIndex(m, i);
    if (upb_MiniTableField_CType(f) == kUpb_CType_Message) {
      *subs = f;
      ++subs;
      msg_count++;
    }
  }

  for (int i = 0; i < upb_MiniTable_FieldCount(m); i++) {
    const upb_MiniTableField* f = upb_MiniTable_GetFieldByIndex(m, i);
    if (upb_MiniTableField_IsClosedEnum(f)) {
      *subs = f;
      ++subs;
      enum_count++;
    }
  }

  return (msg_count << 16) | enum_count;
}

// The list of sub_tables and sub_enums must exactly match the number and order
// of sub-message fields and sub-enum fields given by upb_MiniTable_GetSubList()
// above.
bool upb_MiniTable_Link(upb_MiniTable* m, const upb_MiniTable** sub_tables,
                        size_t sub_table_count,
                        const upb_MiniTableEnum** sub_enums,
                        size_t sub_enum_count) {
  uint32_t msg_count = 0;
  uint32_t enum_count = 0;

  for (int i = 0; i < upb_MiniTable_FieldCount(m); i++) {
    upb_MiniTableField* f =
        (upb_MiniTableField*)upb_MiniTable_GetFieldByIndex(m, i);
    if (upb_MiniTableField_CType(f) == kUpb_CType_Message) {
      const upb_MiniTable* sub = sub_tables[msg_count++];
      if (msg_count > sub_table_count) return false;
      if (sub && !upb_MiniTable_SetSubMessage(m, f, sub)) return false;
    }
  }

  for (int i = 0; i < upb_MiniTable_FieldCount(m); i++) {
    upb_MiniTableField* f =
        (upb_MiniTableField*)upb_MiniTable_GetFieldByIndex(m, i);
    if (upb_MiniTableField_IsClosedEnum(f)) {
      const upb_MiniTableEnum* sub = sub_enums[enum_count++];
      if (enum_count > sub_enum_count) return false;
      if (sub && !upb_MiniTable_SetSubEnum(m, f, sub)) return false;
    }
  }

  return (msg_count == sub_table_count) && (enum_count == sub_enum_count);
}
