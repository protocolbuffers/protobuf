// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/internal/iterator.h"  // IWYU pragma: keep

#include <stddef.h>

#include "upb/message/accessors.h"
#include "upb/message/array.h"
#include "upb/message/internal/accessors.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"

// Must be last.
#include "upb/port/def.inc"

bool UPB_PRIVATE(_upb_Message_NextBaseField)(const upb_Message* msg,
                                             const upb_MiniTable* m,
                                             const upb_MiniTableField** out_f,
                                             upb_MessageValue* out_v,
                                             size_t* iter) {
  const size_t count = upb_MiniTable_FieldCount(m);
  size_t i = *iter;

  while (++i < count) {
    const upb_MiniTableField* f = upb_MiniTable_GetFieldByIndex(m, i);
    const void* src = UPB_PRIVATE(_upb_Message_DataPtr)(msg, f);

    upb_MessageValue val;
    UPB_PRIVATE(_upb_MiniTableField_DataCopy)(f, &val, src);

    // Skip field if unset or empty.
    if (upb_MiniTableField_HasPresence(f)) {
      if (!upb_Message_HasBaseField(msg, f)) continue;
    } else {
      if (UPB_PRIVATE(_upb_MiniTableField_DataIsZero)(f, src)) continue;

      if (upb_MiniTableField_IsArray(f)) {
        if (upb_Array_Size(val.array_val) == 0) continue;
      } else if (upb_MiniTableField_IsMap(f)) {
        if (upb_Map_Size(val.map_val) == 0) continue;
      }
    }

    *out_f = f;
    *out_v = val;
    *iter = i;
    return true;
  }

  return false;
}
