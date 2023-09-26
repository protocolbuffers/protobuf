// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/mini_table/compat.h"

#include "upb/base/descriptor_constants.h"
#include "upb/hash/common.h"
#include "upb/hash/int_table.h"
#include "upb/mem/arena.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"

// Must be last.
#include "upb/port/def.inc"

// Checks if source and target mini table fields are identical.
//
// If the field is a sub message and sub messages are identical we record
// the association in table.
//
// Hashing the source sub message mini table and it's equivalent in the table
// stops recursing when a cycle is detected and instead just checks if the
// destination table is equal.
static upb_MiniTableEquals_Status upb_deep_check(const upb_MiniTable* src,
                                                 const upb_MiniTable* dst,
                                                 upb_inttable* table,
                                                 upb_Arena** arena) {
  if (src->field_count != dst->field_count)
    return kUpb_MiniTableEquals_NotEqual;
  bool marked_src = false;
  for (int i = 0; i < src->field_count; i++) {
    const upb_MiniTableField* src_field = &src->fields[i];
    const upb_MiniTableField* dst_field =
        upb_MiniTable_FindFieldByNumber(dst, src_field->number);

    if (upb_MiniTableField_CType(src_field) !=
        upb_MiniTableField_CType(dst_field))
      return false;
    if (src_field->mode != dst_field->mode) return false;
    if (src_field->offset != dst_field->offset) return false;
    if (src_field->presence != dst_field->presence) return false;
    if (src_field->UPB_PRIVATE(submsg_index) !=
        dst_field->UPB_PRIVATE(submsg_index))
      return kUpb_MiniTableEquals_NotEqual;

    // Go no further if we are only checking for compatibility.
    if (!table) continue;

    if (upb_MiniTableField_CType(src_field) == kUpb_CType_Message) {
      if (!*arena) {
        *arena = upb_Arena_New();
        if (!upb_inttable_init(table, *arena)) {
          return kUpb_MiniTableEquals_OutOfMemory;
        }
      }
      if (!marked_src) {
        marked_src = true;
        upb_value val;
        val.val = (uint64_t)dst;
        if (!upb_inttable_insert(table, (uintptr_t)src, val, *arena)) {
          return kUpb_MiniTableEquals_OutOfMemory;
        }
      }
      const upb_MiniTable* sub_src =
          upb_MiniTable_GetSubMessageTable(src, src_field);
      const upb_MiniTable* sub_dst =
          upb_MiniTable_GetSubMessageTable(dst, dst_field);
      if (sub_src != NULL) {
        upb_value cmp;
        if (upb_inttable_lookup(table, (uintptr_t)sub_src, &cmp)) {
          // We already compared this src before. Check if same dst.
          if (cmp.val != (uint64_t)sub_dst) {
            return kUpb_MiniTableEquals_NotEqual;
          }
        } else {
          // Recurse if not already visited.
          upb_MiniTableEquals_Status s =
              upb_deep_check(sub_src, sub_dst, table, arena);
          if (s != kUpb_MiniTableEquals_Equal) {
            return s;
          }
        }
      }
    }
  }
  return kUpb_MiniTableEquals_Equal;
}

bool upb_MiniTable_Compatible(const upb_MiniTable* src,
                              const upb_MiniTable* dst) {
  return upb_deep_check(src, dst, NULL, NULL);
}

upb_MiniTableEquals_Status upb_MiniTable_Equals(const upb_MiniTable* src,
                                                const upb_MiniTable* dst) {
  // Arena allocated on demand for hash table.
  upb_Arena* arena = NULL;
  // Table to keep track of visited mini tables to guard against cycles.
  upb_inttable table;
  upb_MiniTableEquals_Status status = upb_deep_check(src, dst, &table, &arena);
  if (arena) {
    upb_Arena_Free(arena);
  }
  return status;
}
