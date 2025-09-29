// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/internal/map_sorter.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "upb/base/descriptor_constants.h"
#include "upb/base/internal/log2.h"
#include "upb/base/string_view.h"
#include "upb/hash/common.h"
#include "upb/mem/alloc.h"
#include "upb/message/internal/extension.h"
#include "upb/message/internal/map.h"
#include "upb/message/internal/message.h"
#include "upb/message/map.h"
#include "upb/mini_table/extension.h"

// Must be last.
#include "upb/port/def.inc"

static int _upb_mapsorter_intkeys(const void* _a, const void* _b) {
  const upb_tabent* const* a = _a;
  const upb_tabent* const* b = _b;
  uintptr_t a_key = (*a)->key.num;
  uintptr_t b_key = (*b)->key.num;
  return a_key < b_key ? -1 : a_key > b_key;
}

static void _upb_mapsorter_getkeys(const void* _a, const void* _b, void* a_key,
                                   void* b_key, size_t size) {
  const upb_tabent* const* a = _a;
  const upb_tabent* const* b = _b;
  upb_StringView a_tabkey = upb_key_strview((*a)->key);
  upb_StringView b_tabkey = upb_key_strview((*b)->key);
  _upb_map_fromkey(a_tabkey, a_key, size);
  _upb_map_fromkey(b_tabkey, b_key, size);
}

static int _upb_mapsorter_cmpi64(const void* _a, const void* _b) {
  int64_t a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, 8);
  return a < b ? -1 : a > b;
}

static int _upb_mapsorter_cmpu64(const void* _a, const void* _b) {
  uint64_t a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, 8);
  return a < b ? -1 : a > b;
}

static int _upb_mapsorter_cmpi32(const void* _a, const void* _b) {
  int32_t a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, 4);
  return a < b ? -1 : a > b;
}

static int _upb_mapsorter_cmpu32(const void* _a, const void* _b) {
  uint32_t a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, 4);
  return a < b ? -1 : a > b;
}

static int _upb_mapsorter_cmpbool(const void* _a, const void* _b) {
  bool a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, 1);
  return a < b ? -1 : a > b;
}

static int _upb_mapsorter_cmpstr(const void* _a, const void* _b) {
  upb_StringView a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, UPB_MAPTYPE_STRING);
  size_t common_size = UPB_MIN(a.size, b.size);
  int cmp = memcmp(a.data, b.data, common_size);
  if (cmp) return -cmp;
  return a.size < b.size ? -1 : a.size > b.size;
}

static int (*const compar[kUpb_FieldType_SizeOf])(const void*, const void*) = {
    [kUpb_FieldType_Int64] = _upb_mapsorter_cmpi64,
    [kUpb_FieldType_SFixed64] = _upb_mapsorter_cmpi64,
    [kUpb_FieldType_SInt64] = _upb_mapsorter_cmpi64,

    [kUpb_FieldType_UInt64] = _upb_mapsorter_cmpu64,
    [kUpb_FieldType_Fixed64] = _upb_mapsorter_cmpu64,

    [kUpb_FieldType_Int32] = _upb_mapsorter_cmpi32,
    [kUpb_FieldType_SInt32] = _upb_mapsorter_cmpi32,
    [kUpb_FieldType_SFixed32] = _upb_mapsorter_cmpi32,
    [kUpb_FieldType_Enum] = _upb_mapsorter_cmpi32,

    [kUpb_FieldType_UInt32] = _upb_mapsorter_cmpu32,
    [kUpb_FieldType_Fixed32] = _upb_mapsorter_cmpu32,

    [kUpb_FieldType_Bool] = _upb_mapsorter_cmpbool,

    [kUpb_FieldType_String] = _upb_mapsorter_cmpstr,
    [kUpb_FieldType_Bytes] = _upb_mapsorter_cmpstr,
};

static bool _upb_mapsorter_resize(_upb_mapsorter* s, _upb_sortedmap* sorted,
                                  int size) {
  sorted->start = s->size;
  sorted->pos = sorted->start;
  sorted->end = sorted->start + size;

  if (sorted->end > s->cap) {
    const int oldsize = s->cap * sizeof(*s->entries);
    s->cap = upb_RoundUpToPowerOfTwo(sorted->end);
    const int newsize = s->cap * sizeof(*s->entries);
    s->entries = upb_grealloc(s->entries, oldsize, newsize);
    if (!s->entries) return false;
  }

  s->size = sorted->end;
  return true;
}

bool _upb_mapsorter_pushmap(_upb_mapsorter* s, upb_FieldType key_type,
                            const upb_Map* map, _upb_sortedmap* sorted) {
  int map_size;
  if (map->UPB_PRIVATE(is_strtable)) {
    map_size = _upb_Map_Size(map);
  } else {
    // For inttable, only sort the table entries, since the array part is
    // already in a sorted order.
    map_size = map->t.inttable.t.count;
  }

  if (!_upb_mapsorter_resize(s, sorted, map_size)) return false;

  // Copy non-empty entries from the table to s->entries.
  const void** dst = &s->entries[sorted->start];
  const upb_tabent* src;
  const upb_tabent* end;
  if (map->UPB_PRIVATE(is_strtable)) {
    src = map->t.strtable.t.entries;
    end = src + upb_table_size(&map->t.strtable.t);
  } else {
    // For inttable, only sort the table entries, since the array part is
    // already in a sorted order.
    src = map->t.inttable.t.entries;
    end = src + upb_table_size(&map->t.inttable.t);
  }
  for (; src < end; src++) {
    if (!upb_tabent_isempty(src)) {
      *dst = src;
      dst++;
    }
  }
  UPB_ASSERT(dst == &s->entries[sorted->end]);

  // Sort entries according to the key type.
  qsort(&s->entries[sorted->start], map_size, sizeof(*s->entries),
        map->UPB_PRIVATE(is_strtable) ? compar[key_type]
                                      : _upb_mapsorter_intkeys);
  return true;
}

static int _upb_mapsorter_cmpext(const void* _a, const void* _b) {
  const upb_Extension* const* a = _a;
  const upb_Extension* const* b = _b;
  uint32_t a_num = upb_MiniTableExtension_Number((*a)->ext);
  uint32_t b_num = upb_MiniTableExtension_Number((*b)->ext);
  UPB_ASSERT(a_num != b_num);
  return a_num < b_num ? -1 : 1;
}

bool _upb_mapsorter_pushexts(_upb_mapsorter* s, const upb_Message_Internal* in,
                             _upb_sortedmap* sorted) {
  size_t count = 0;
  for (size_t i = 0; i < in->size; i++) {
    count += upb_TaggedAuxPtr_IsExtension(in->aux_data[i]);
  }
  if (!_upb_mapsorter_resize(s, sorted, count)) return false;
  if (count == 0) return true;
  const upb_Extension** entry =
      (const upb_Extension**)&s->entries[sorted->start];
  for (size_t i = 0; i < in->size; i++) {
    upb_TaggedAuxPtr tagged_ptr = in->aux_data[i];
    if (upb_TaggedAuxPtr_IsExtension(tagged_ptr)) {
      *entry++ = upb_TaggedAuxPtr_Extension(tagged_ptr);
    }
  }
  qsort(&s->entries[sorted->start], count, sizeof(*s->entries),
        _upb_mapsorter_cmpext);
  return true;
}
