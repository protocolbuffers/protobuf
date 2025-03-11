// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/accessors.h"

#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"

// Must be last.
#include "upb/port/def.inc"

bool upb_Message_SetMapEntry(upb_Map* map, const upb_MiniTable* m,
                             const upb_MiniTableField* f,
                             upb_Message* map_entry_message, upb_Arena* arena) {
  UPB_ASSERT(!upb_Message_IsFrozen(map_entry_message));
  const upb_MiniTable* map_entry_mini_table =
      upb_MiniTable_MapEntrySubMessage(m, f);
  UPB_ASSERT(map_entry_mini_table);
  const upb_MiniTableField* map_entry_key_field =
      upb_MiniTable_MapKey(map_entry_mini_table);
  const upb_MiniTableField* map_entry_value_field =
      upb_MiniTable_MapValue(map_entry_mini_table);

  upb_MessageValue map_entry_key, map_entry_value;
  UPB_PRIVATE(_upb_MiniTableField_DataCopy)
  (map_entry_key_field, &map_entry_key,
   UPB_PRIVATE(_upb_Message_DataPtr)(map_entry_message, map_entry_key_field));

  UPB_PRIVATE(_upb_MiniTableField_DataCopy)
  (map_entry_value_field, &map_entry_value,
   UPB_PRIVATE(_upb_Message_DataPtr)(map_entry_message, map_entry_value_field));

  return upb_Map_Set(map, map_entry_key, map_entry_value, arena);
}
