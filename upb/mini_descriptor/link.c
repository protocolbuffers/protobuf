/*
 * Copyright (c) 2009-2021, Google LLC
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
  // TODO(haberman): Add this assert back once YouTube is updated to not call
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
    if (upb_MiniTableField_CType(f) == kUpb_CType_Enum) {
      const upb_MiniTableEnum* sub = sub_enums[enum_count++];
      if (enum_count > sub_enum_count) return false;
      if (sub != NULL) {
        if (!upb_MiniTable_SetSubEnum(mt, f, sub)) return false;
      }
    }
  }

  return true;
}
