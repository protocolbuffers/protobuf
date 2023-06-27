/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "upb/base/log2.h"
#include "upb/collections/map_sorter_internal.h"

// Must be last.
#include "upb/port/def.inc"

static void _upb_mapsorter_getkeys(const void* _a, const void* _b, void* a_key,
                                   void* b_key, size_t size) {
  const upb_tabent* const* a = _a;
  const upb_tabent* const* b = _b;
  upb_StringView a_tabkey = upb_tabstrview((*a)->key);
  upb_StringView b_tabkey = upb_tabstrview((*b)->key);
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
    s->cap = upb_Log2CeilingSize(sorted->end);
    s->entries = realloc(s->entries, s->cap * sizeof(*s->entries));
    if (!s->entries) return false;
  }

  s->size = sorted->end;
  return true;
}

bool _upb_mapsorter_pushmap(_upb_mapsorter* s, upb_FieldType key_type,
                            const upb_Map* map, _upb_sortedmap* sorted) {
  int map_size = _upb_Map_Size(map);

  if (!_upb_mapsorter_resize(s, sorted, map_size)) return false;

  // Copy non-empty entries from the table to s->entries.
  const void** dst = &s->entries[sorted->start];
  const upb_tabent* src = map->table.t.entries;
  const upb_tabent* end = src + upb_table_size(&map->table.t);
  for (; src < end; src++) {
    if (!upb_tabent_isempty(src)) {
      *dst = src;
      dst++;
    }
  }
  UPB_ASSERT(dst == &s->entries[sorted->end]);

  // Sort entries according to the key type.
  qsort(&s->entries[sorted->start], map_size, sizeof(*s->entries),
        compar[key_type]);
  return true;
}

static int _upb_mapsorter_cmpext(const void* _a, const void* _b) {
  const upb_Message_Extension* const* a = _a;
  const upb_Message_Extension* const* b = _b;
  uint32_t a_num = (*a)->ext->field.number;
  uint32_t b_num = (*b)->ext->field.number;
  assert(a_num != b_num);
  return a_num < b_num ? -1 : 1;
}

bool _upb_mapsorter_pushexts(_upb_mapsorter* s,
                             const upb_Message_Extension* exts, size_t count,
                             _upb_sortedmap* sorted) {
  if (!_upb_mapsorter_resize(s, sorted, count)) return false;

  for (size_t i = 0; i < count; i++) {
    s->entries[sorted->start + i] = &exts[i];
  }

  qsort(&s->entries[sorted->start], count, sizeof(*s->entries),
        _upb_mapsorter_cmpext);
  return true;
}
