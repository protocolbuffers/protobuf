// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/accessors.h"

#include <string.h>

#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include "upb/message/internal/accessors.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/message.h"
#include "upb/mini_table/sub.h"
#include "upb/wire/encode.h"

// Must be last.
#include "upb/port/def.inc"

#define kUpb_MapField_Begin ((size_t)-1)

// static because it is not yet ready for general use.
static bool upb_Message_NextField(const upb_Message* msg,
                                  const upb_MiniTable* m,
                                  const upb_MiniTableField** out_f,
                                  upb_MessageValue* out_v, size_t* iter) {
  const size_t n = upb_MiniTable_FieldCount(m);
  size_t i = *iter;

  // Iterate over normal fields.
  while (++i < n) {
    const upb_MiniTableField* f = upb_MiniTable_GetFieldByIndex(m, i);
    const void* src = _upb_MiniTableField_GetConstPtr(msg, f);

    upb_MessageValue val;
    UPB_PRIVATE(_upb_MiniTableField_DataCopy)(f, &val, src);

    // Skip field if unset or empty.
    if (upb_MiniTableField_HasPresence(f)) {
      if (!_upb_Message_HasNonExtensionField(msg, f)) continue;
    } else {
      switch (UPB_PRIVATE(_upb_MiniTableField_Mode)(f)) {
        case kUpb_FieldMode_Map:
          if (!val.map_val || upb_Map_Size(val.map_val) == 0) continue;
          break;

        case kUpb_FieldMode_Array:
          if (!val.array_val || upb_Array_Size(val.array_val) == 0) continue;
          break;

        case kUpb_FieldMode_Scalar:
          if (UPB_PRIVATE(_upb_MiniTableField_DataIsZero)(f, &val)) continue;
          break;
      }
    }

    *out_f = f;
    *out_v = val;
    *iter = i;
    return true;
  }

  return false;
}

bool upb_Message_IsEmpty(const upb_Message* msg, const upb_MiniTable* m) {
  if (upb_Message_ExtensionCount(msg)) return false;

  const upb_MiniTableField* f;
  upb_MessageValue v;
  size_t iter = kUpb_MapField_Begin;
  return upb_Message_NextField(msg, m, &f, &v, &iter);
}

bool upb_Message_SetMapEntry(upb_Map* map, const upb_MiniTable* mini_table,
                             const upb_MiniTableField* f,
                             upb_Message* map_entry_message, upb_Arena* arena) {
  // TODO: use a variant of upb_MiniTable_GetSubMessageTable() here.
  const upb_MiniTable* map_entry_mini_table = upb_MiniTableSub_Message(
      mini_table->UPB_PRIVATE(subs)[f->UPB_PRIVATE(submsg_index)]);
  UPB_ASSERT(map_entry_mini_table);
  UPB_ASSERT(map_entry_mini_table->UPB_PRIVATE(field_count) == 2);
  const upb_MiniTableField* map_entry_key_field =
      &map_entry_mini_table->UPB_PRIVATE(fields)[0];
  const upb_MiniTableField* map_entry_value_field =
      &map_entry_mini_table->UPB_PRIVATE(fields)[1];
  // Map key/value cannot have explicit defaults,
  // hence assuming a zero default is valid.
  upb_MessageValue default_val;
  memset(&default_val, 0, sizeof(upb_MessageValue));
  upb_MessageValue map_entry_key =
      upb_Message_GetField(map_entry_message, map_entry_key_field, default_val);
  upb_MessageValue map_entry_value = upb_Message_GetField(
      map_entry_message, map_entry_value_field, default_val);
  return upb_Map_Set(map, map_entry_key, map_entry_value, arena);
}

bool upb_Message_IsExactlyEqual(const upb_Message* m1, const upb_Message* m2,
                                const upb_MiniTable* layout) {
  if (m1 == m2) return true;

  int opts = kUpb_EncodeOption_SkipUnknown | kUpb_EncodeOption_Deterministic;
  upb_Arena* a = upb_Arena_New();

  // Compare deterministically serialized payloads with no unknown fields.
  size_t size1, size2;
  char *data1, *data2;
  upb_EncodeStatus status1 = upb_Encode(m1, layout, opts, a, &data1, &size1);
  upb_EncodeStatus status2 = upb_Encode(m2, layout, opts, a, &data2, &size2);

  if (status1 != kUpb_EncodeStatus_Ok || status2 != kUpb_EncodeStatus_Ok) {
    // TODO: How should we fail here? (In Ruby we throw an exception.)
    upb_Arena_Free(a);
    return false;
  }

  const bool ret = (size1 == size2) && (memcmp(data1, data2, size1) == 0);
  upb_Arena_Free(a);
  return ret;
}
