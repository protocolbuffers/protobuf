// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/mini_descriptor/link.h"

// Must be last.
#include "upb/port/def.inc"

bool upb_MiniTable_SetSubMessage(upb_MiniTable* table,
                                 upb_MiniTableField* field,
                                 const upb_MiniTable* sub) {
  UPB_ASSERT((uintptr_t)table->fields <= (uintptr_t)field &&
             (uintptr_t)field <
                 (uintptr_t)(table->fields + table->field_count));
  UPB_ASSERT(sub);

  const bool sub_is_map = sub->ext & kUpb_ExtMode_IsMapEntry;

  switch (field->UPB_PRIVATE(descriptortype)) {
    case kUpb_FieldType_Message:
      if (sub_is_map) {
        const bool table_is_map = table->ext & kUpb_ExtMode_IsMapEntry;
        if (UPB_UNLIKELY(table_is_map)) return false;

        field->mode = (field->mode & ~kUpb_FieldMode_Mask) | kUpb_FieldMode_Map;
      }
      break;

    case kUpb_FieldType_Group:
      if (UPB_UNLIKELY(sub_is_map)) return false;
      break;

    default:
      return false;
  }

  upb_MiniTableSub* table_sub =
      (void*)&table->subs[field->UPB_PRIVATE(submsg_index)];
  // TODO: Add this assert back once YouTube is updated to not call
  // this function repeatedly.
  // UPB_ASSERT(table_sub->submsg == &_kUpb_MiniTable_Empty);
  table_sub->submsg = sub;
  return true;
}

bool upb_MiniTable_SetSubEnum(upb_MiniTable* table, upb_MiniTableField* field,
                              const upb_MiniTableEnum* sub) {
  UPB_ASSERT((uintptr_t)table->fields <= (uintptr_t)field &&
             (uintptr_t)field <
                 (uintptr_t)(table->fields + table->field_count));
  UPB_ASSERT(sub);

  upb_MiniTableSub* table_sub =
      (void*)&table->subs[field->UPB_PRIVATE(submsg_index)];
  table_sub->subenum = sub;
  return true;
}

uint32_t upb_MiniTable_GetSubList(const upb_MiniTable* mt,
                                  const upb_MiniTableField** subs) {
  uint32_t msg_count = 0;
  uint32_t enum_count = 0;

  for (int i = 0; i < mt->field_count; i++) {
    const upb_MiniTableField* f = &mt->fields[i];
    if (upb_MiniTableField_CType(f) == kUpb_CType_Message) {
      *subs = f;
      ++subs;
      msg_count++;
    }
  }

  for (int i = 0; i < mt->field_count; i++) {
    const upb_MiniTableField* f = &mt->fields[i];
    if (upb_MiniTableField_CType(f) == kUpb_CType_Enum) {
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
bool upb_MiniTable_Link(upb_MiniTable* mt, const upb_MiniTable** sub_tables,
                        size_t sub_table_count,
                        const upb_MiniTableEnum** sub_enums,
                        size_t sub_enum_count) {
  uint32_t msg_count = 0;
  uint32_t enum_count = 0;

  for (int i = 0; i < mt->field_count; i++) {
    upb_MiniTableField* f = (upb_MiniTableField*)&mt->fields[i];
    if (upb_MiniTableField_CType(f) == kUpb_CType_Message) {
      const upb_MiniTable* sub = sub_tables[msg_count++];
      if (msg_count > sub_table_count) return false;
      if (sub != NULL) {
        if (!upb_MiniTable_SetSubMessage(mt, f, sub)) return false;
      }
    }
  }

  for (int i = 0; i < mt->field_count; i++) {
    upb_MiniTableField* f = (upb_MiniTableField*)&mt->fields[i];
    if (upb_MiniTableField_IsClosedEnum(f)) {
      const upb_MiniTableEnum* sub = sub_enums[enum_count++];
      if (enum_count > sub_enum_count) return false;
      if (sub != NULL) {
        if (!upb_MiniTable_SetSubEnum(mt, f, sub)) return false;
      }
    }
  }

  return true;
}
