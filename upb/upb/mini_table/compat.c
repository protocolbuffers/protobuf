// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google LLC nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
