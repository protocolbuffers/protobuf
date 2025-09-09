// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MESSAGE_INTERNAL_MAP_H_
#define UPB_MESSAGE_INTERNAL_MAP_H_

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "upb/base/descriptor_constants.h"
#include "upb/base/string_view.h"
#include "upb/hash/common.h"
#include "upb/hash/int_table.h"
#include "upb/hash/str_table.h"
#include "upb/mem/arena.h"

// Must be last.
#include "upb/port/def.inc"

typedef enum {
  kUpb_MapInsertStatus_Inserted = 0,
  kUpb_MapInsertStatus_Replaced = 1,
  kUpb_MapInsertStatus_OutOfMemory = 2,
} upb_MapInsertStatus;

// EVERYTHING BELOW THIS LINE IS INTERNAL - DO NOT USE /////////////////////////

union upb_Map_Table {
  upb_strtable strtable;
  upb_inttable inttable;
};

struct upb_Map {
  // Size of key and val, based on the map type.
  // Strings are represented as '0' because they must be handled specially.
  char key_size;
  char val_size;
  bool UPB_PRIVATE(is_frozen);
  bool UPB_PRIVATE(is_strtable);

  union upb_Map_Table t;
};

#ifdef __cplusplus
extern "C" {
#endif

UPB_INLINE void UPB_PRIVATE(_upb_Map_ShallowFreeze)(struct upb_Map* map) {
  map->UPB_PRIVATE(is_frozen) = true;
}

UPB_API_INLINE bool upb_Map_IsFrozen(const struct upb_Map* map) {
  return map->UPB_PRIVATE(is_frozen);
}

// Converting between internal table representation and user values.
//
// _upb_map_tokey() and _upb_map_fromkey() are inverses.
// _upb_map_tovalue() and _upb_map_fromvalue() are inverses.
//
// These functions account for the fact that strings are treated differently
// from other types when stored in a map.

UPB_INLINE upb_StringView _upb_map_tokey(const void* key, size_t size) {
  if (size == UPB_MAPTYPE_STRING) {
    return *(upb_StringView*)key;
  } else {
    return upb_StringView_FromDataAndSize((const char*)key, size);
  }
}

UPB_INLINE uintptr_t _upb_map_tointkey(const void* key, size_t key_size) {
  uintptr_t intkey = 0;
  memcpy(&intkey, key, key_size);
  return intkey;
}

UPB_INLINE void _upb_map_fromkey(upb_StringView key, void* out, size_t size) {
  if (size == UPB_MAPTYPE_STRING) {
    memcpy(out, &key, sizeof(key));
  } else {
    memcpy(out, key.data, size);
  }
}

UPB_INLINE bool _upb_map_tovalue(const void* val, size_t size,
                                 upb_value* msgval, upb_Arena* a) {
  if (size == UPB_MAPTYPE_STRING) {
    upb_StringView* strp = (upb_StringView*)upb_Arena_Malloc(a, sizeof(*strp));
    if (!strp) return false;
    *strp = *(upb_StringView*)val;
    *msgval = upb_value_ptr(strp);
  } else {
    memcpy(msgval, val, size);
  }
  return true;
}

UPB_INLINE void _upb_map_fromvalue(upb_value val, void* out, size_t size) {
  if (size == UPB_MAPTYPE_STRING) {
    const upb_StringView* strp = (const upb_StringView*)upb_value_getptr(val);
    memcpy(out, strp, sizeof(upb_StringView));
  } else {
    memcpy(out, &val, size);
  }
}

UPB_INLINE bool _upb_map_next(const struct upb_Map* map, size_t* iter) {
  if (map->UPB_PRIVATE(is_strtable)) {
    upb_strtable_iter it;
    it.t = &map->t.strtable;
    it.index = *iter;
    upb_strtable_next(&it);
    *iter = it.index;
    return !upb_strtable_done(&it);
  } else {
    uintptr_t key;
    upb_value val;
    intptr_t int_iter = 0;
    memcpy(&int_iter, iter, sizeof(intptr_t));
    upb_inttable_next(&map->t.inttable, &key, &val, &int_iter);
    memcpy(iter, &int_iter, sizeof(size_t));
    return !upb_inttable_done(&map->t.inttable, int_iter);
  }
}

UPB_INLINE void _upb_Map_Clear(struct upb_Map* map) {
  UPB_ASSERT(!upb_Map_IsFrozen(map));

  if (map->UPB_PRIVATE(is_strtable)) {
    upb_strtable_clear(&map->t.strtable);
  } else {
    upb_inttable_clear(&map->t.inttable);
  }
}

UPB_INLINE bool _upb_Map_Delete(struct upb_Map* map, const void* key,
                                size_t key_size, upb_value* val) {
  UPB_ASSERT(!upb_Map_IsFrozen(map));

  if (map->UPB_PRIVATE(is_strtable)) {
    upb_StringView k = _upb_map_tokey(key, key_size);
    return upb_strtable_remove2(&map->t.strtable, k.data, k.size, val);
  } else {
    uintptr_t intkey = _upb_map_tointkey(key, key_size);
    return upb_inttable_remove(&map->t.inttable, intkey, val);
  }
}

UPB_INLINE bool _upb_Map_Get(const struct upb_Map* map, const void* key,
                             size_t key_size, void* val, size_t val_size) {
  upb_value tabval = {0};
  bool ret;
  if (map->UPB_PRIVATE(is_strtable)) {
    upb_StringView k = _upb_map_tokey(key, key_size);
    ret = upb_strtable_lookup2(&map->t.strtable, k.data, k.size, &tabval);
  } else {
    uintptr_t intkey = _upb_map_tointkey(key, key_size);
    ret = upb_inttable_lookup(&map->t.inttable, intkey, &tabval);
  }
  if (ret && val) {
    _upb_map_fromvalue(tabval, val, val_size);
  }
  return ret;
}

UPB_INLINE upb_MapInsertStatus _upb_Map_Insert(struct upb_Map* map,
                                               const void* key, size_t key_size,
                                               void* val, size_t val_size,
                                               upb_Arena* a) {
  UPB_ASSERT(!upb_Map_IsFrozen(map));

  // Prep the value.
  upb_value tabval = {0};
  if (!_upb_map_tovalue(val, val_size, &tabval, a)) {
    return kUpb_MapInsertStatus_OutOfMemory;
  }

  bool removed;
  if (map->UPB_PRIVATE(is_strtable)) {
    upb_StringView strkey = _upb_map_tokey(key, key_size);
    // TODO: add overwrite operation to minimize number of lookups.
    removed =
        upb_strtable_remove2(&map->t.strtable, strkey.data, strkey.size, NULL);
    if (!upb_strtable_insert(&map->t.strtable, strkey.data, strkey.size, tabval,
                             a)) {
      return kUpb_MapInsertStatus_OutOfMemory;
    }
  } else {
    uintptr_t intkey = _upb_map_tointkey(key, key_size);
    removed = upb_inttable_remove(&map->t.inttable, intkey, NULL);
    if (!upb_inttable_insert(&map->t.inttable, intkey, tabval, a)) {
      return kUpb_MapInsertStatus_OutOfMemory;
    }
  }
  return removed ? kUpb_MapInsertStatus_Replaced
                 : kUpb_MapInsertStatus_Inserted;
}

UPB_INLINE size_t _upb_Map_Size(const struct upb_Map* map) {
  if (map->UPB_PRIVATE(is_strtable)) {
    return map->t.strtable.t.count;
  } else {
    return upb_inttable_count(&map->t.inttable);
  }
}

// Strings/bytes are special-cased in maps.
extern char _upb_Map_CTypeSizeTable[12];

UPB_INLINE size_t _upb_Map_CTypeSize(upb_CType ctype) {
  return _upb_Map_CTypeSizeTable[ctype];
}

// Creates a new map on the given arena with this key/value type.
struct upb_Map* _upb_Map_New(upb_Arena* a, size_t key_size, size_t value_size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MESSAGE_INTERNAL_MAP_H_ */
