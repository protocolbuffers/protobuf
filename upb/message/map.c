// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/map.h"

#include <stdint.h>
#include <string.h>

#include "upb/base/descriptor_constants.h"
#include "upb/base/string_view.h"
#include "upb/hash/common.h"
#include "upb/hash/int_table.h"
#include "upb/hash/str_table.h"
#include "upb/mem/arena.h"
#include "upb/message/internal/map.h"
#include "upb/message/internal/types.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/message/value.h"
#include "upb/mini_table/message.h"

// Must be last.
#include "upb/port/def.inc"

// Strings/bytes are special-cased in maps.
char _upb_Map_CTypeSizeTable[12] = {
    [kUpb_CType_Bool] = 1,
    [kUpb_CType_Float] = 4,
    [kUpb_CType_Int32] = 4,
    [kUpb_CType_UInt32] = 4,
    [kUpb_CType_Enum] = 4,
    [kUpb_CType_Message] = sizeof(void*),
    [kUpb_CType_Double] = 8,
    [kUpb_CType_Int64] = 8,
    [kUpb_CType_UInt64] = 8,
    [kUpb_CType_String] = UPB_MAPTYPE_STRING,
    [kUpb_CType_Bytes] = UPB_MAPTYPE_STRING,
};

upb_Map* upb_Map_New(upb_Arena* a, upb_CType key_type, upb_CType value_type) {
  return _upb_Map_New(a, _upb_Map_CTypeSize(key_type),
                      _upb_Map_CTypeSize(value_type));
}

size_t upb_Map_Size(const upb_Map* map) { return _upb_Map_Size(map); }

bool upb_Map_Get(const upb_Map* map, upb_MessageValue key,
                 upb_MessageValue* val) {
  return _upb_Map_Get(map, &key, map->key_size, val, map->val_size);
}

struct upb_Message* upb_Map_GetMutable(upb_Map* map, upb_MessageValue key) {
  UPB_ASSERT(map->val_size == sizeof(upb_Message*));
  upb_Message* val = NULL;
  if (_upb_Map_Get(map, &key, map->key_size, &val, sizeof(upb_Message*))) {
    return val;
  } else {
    return NULL;
  }
}

void upb_Map_Clear(upb_Map* map) { _upb_Map_Clear(map); }

upb_MapInsertStatus upb_Map_Insert(upb_Map* map, upb_MessageValue key,
                                   upb_MessageValue val, upb_Arena* arena) {
  UPB_ASSERT(arena);
  return (upb_MapInsertStatus)_upb_Map_Insert(map, &key, map->key_size, &val,
                                              map->val_size, arena);
}

bool upb_Map_Delete(upb_Map* map, upb_MessageValue key, upb_MessageValue* val) {
  upb_value v;
  const bool removed = _upb_Map_Delete(map, &key, map->key_size, &v);
  if (val) _upb_map_fromvalue(v, val, map->val_size);
  return removed;
}

bool upb_Map_Next(const upb_Map* map, upb_MessageValue* key,
                  upb_MessageValue* val, size_t* iter) {
  upb_value v;
  bool ret;
  if (map->UPB_PRIVATE(is_strtable)) {
    upb_StringView strkey;
    ret = upb_strtable_next2(&map->t.strtable, &strkey, &v, (intptr_t*)iter);
    if (ret) {
      _upb_map_fromkey(strkey, key, map->key_size);
    }
  } else {
    uintptr_t intkey;
    ret = upb_inttable_next(&map->t.inttable, &intkey, &v, (intptr_t*)iter);
    if (ret) {
      memcpy(key, &intkey, map->key_size);
    }
  }
  if (ret) {
    _upb_map_fromvalue(v, val, map->val_size);
  }
  return ret;
}

UPB_API void upb_Map_SetEntryValue(upb_Map* map, size_t iter,
                                   upb_MessageValue val) {
  upb_value v;
  _upb_map_tovalue(&val, map->val_size, &v, NULL);
  if (map->UPB_PRIVATE(is_strtable)) {
    upb_strtable_setentryvalue(&map->t.strtable, iter, v);
  } else {
    upb_inttable_setentryvalue(&map->t.inttable, iter, v);
  }
}

bool upb_MapIterator_Next(const upb_Map* map, size_t* iter) {
  return _upb_map_next(map, iter);
}

bool upb_MapIterator_Done(const upb_Map* map, size_t iter) {
  UPB_ASSERT(iter != kUpb_Map_Begin);
  if (map->UPB_PRIVATE(is_strtable)) {
    upb_strtable_iter i;
    i.t = &map->t.strtable;
    i.index = iter;
    return upb_strtable_done(&i);
  } else {
    return upb_inttable_done(&map->t.inttable, iter);
  }
}

// Returns the key and value for this entry of the map.
upb_MessageValue upb_MapIterator_Key(const upb_Map* map, size_t iter) {
  upb_MessageValue ret;
  if (map->UPB_PRIVATE(is_strtable)) {
    upb_strtable_iter i;
    i.t = &map->t.strtable;
    i.index = iter;
    _upb_map_fromkey(upb_strtable_iter_key(&i), &ret, map->key_size);
  } else {
    uintptr_t intkey = upb_inttable_iter_key(&map->t.inttable, iter);
    memcpy(&ret, &intkey, map->key_size);
  }
  return ret;
}

upb_MessageValue upb_MapIterator_Value(const upb_Map* map, size_t iter) {
  upb_value v;
  if (map->UPB_PRIVATE(is_strtable)) {
    upb_strtable_iter i;
    i.t = &map->t.strtable;
    i.index = iter;
    v = upb_strtable_iter_value(&i);
  } else {
    v = upb_inttable_iter_value(&map->t.inttable, iter);
  }

  upb_MessageValue ret;
  _upb_map_fromvalue(v, &ret, map->val_size);
  return ret;
}

void upb_Map_Freeze(upb_Map* map, const upb_MiniTable* m) {
  if (upb_Map_IsFrozen(map)) return;
  UPB_PRIVATE(_upb_Map_ShallowFreeze)(map);

  if (m) {
    size_t iter = kUpb_Map_Begin;
    upb_MessageValue key, val;

    while (upb_Map_Next(map, &key, &val, &iter)) {
      upb_Message_Freeze((upb_Message*)val.msg_val, m);
    }
  }
}

// EVERYTHING BELOW THIS LINE IS INTERNAL - DO NOT USE /////////////////////////

upb_Map* _upb_Map_New(upb_Arena* a, size_t key_size, size_t value_size) {
  upb_Map* map = upb_Arena_Malloc(a, sizeof(upb_Map));
  if (!map) return NULL;

  if (key_size <= sizeof(uintptr_t) && key_size != UPB_MAPTYPE_STRING) {
    if (!upb_inttable_init(&map->t.inttable, a)) return NULL;
    map->UPB_PRIVATE(is_strtable) = false;
  } else {
    if (!upb_strtable_init(&map->t.strtable, 4, a)) return NULL;
    map->UPB_PRIVATE(is_strtable) = true;
  }
  map->key_size = key_size;
  map->val_size = value_size;
  map->UPB_PRIVATE(is_frozen) = false;

  return map;
}
