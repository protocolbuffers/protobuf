// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/upb/mini_table/compat.h"

#include "upb/upb/base/descriptor_constants.h"
#include "upb/upb/mini_table/field.h"
#include "upb/upb/mini_table/message.h"

// Must be last.
#include "upb/upb/port/def.inc"

static bool upb_deep_check(const upb_MiniTable* src, const upb_MiniTable* dst,
                           bool eq) {
  if (src->field_count != dst->field_count) return false;

  for (int i = 0; i < src->field_count; i++) {
    const upb_MiniTableField* src_field = &src->fields[i];
    const upb_MiniTableField* dst_field =
        upb_MiniTable_FindFieldByNumber(dst, src_field->number);

    if (upb_MiniTableField_CType(src_field) !=
        upb_MiniTableField_CType(dst_field)) return false;
    if (src_field->mode != dst_field->mode) return false;
    if (src_field->offset != dst_field->offset) return false;
    if (src_field->presence != dst_field->presence) return false;
    if (src_field->UPB_PRIVATE(submsg_index) !=
        dst_field->UPB_PRIVATE(submsg_index)) return false;

    // Go no further if we are only checking for compatibility.
    if (!eq) continue;

    if (upb_MiniTableField_CType(src_field) == kUpb_CType_Message) {
      const upb_MiniTable* sub_src =
          upb_MiniTable_GetSubMessageTable(src, src_field);
      const upb_MiniTable* sub_dst =
          upb_MiniTable_GetSubMessageTable(dst, dst_field);
      if (sub_src != NULL && !upb_MiniTable_Equals(sub_src, sub_dst)) {
        return false;
      }
    }
  }

  return true;
}

bool upb_MiniTable_Compatible(const upb_MiniTable* src,
                              const upb_MiniTable* dst) {
  return upb_deep_check(src, dst, false);
}

bool upb_MiniTable_Equals(const upb_MiniTable* src, const upb_MiniTable* dst) {
  return upb_deep_check(src, dst, true);
}
