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
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"
#include "upb/mini_table/sub.h"

// Must be last.
#include "upb/port/def.inc"

bool upb_Message_SetMapEntry(upb_Map* map, const upb_MiniTable* m,
                             const upb_MiniTableField* f,
                             upb_Message* map_entry_message, upb_Arena* arena) {
  UPB_ASSERT(!upb_Message_IsFrozen(map_entry_message));

  // TODO: use a variant of upb_MiniTable_GetSubMessageTable() here.
  const upb_MiniTable* map_entry_mini_table = upb_MiniTableSub_Message(
      m->UPB_PRIVATE(subs)[f->UPB_PRIVATE(submsg_index)]);
  UPB_ASSERT(map_entry_mini_table);
  const upb_MiniTableField* map_entry_key_field =
      upb_MiniTable_MapKey(map_entry_mini_table);
  const upb_MiniTableField* map_entry_value_field =
      upb_MiniTable_MapValue(map_entry_mini_table);
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
