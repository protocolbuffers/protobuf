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

#include "upb/msg.h"

#include "upb/internal/table.h"
#include "upb/msg_internal.h"
#include "upb/port_def.inc"

/** upb_Message ***************************************************************/

static const size_t overhead = sizeof(upb_Message_InternalData);

static const upb_Message_Internal* upb_Message_Getinternal_const(
    const upb_Message* msg) {
  ptrdiff_t size = sizeof(upb_Message_Internal);
  return (upb_Message_Internal*)((char*)msg - size);
}

upb_Message* _upb_Message_New(const upb_MiniTable* l, upb_Arena* a) {
  return _upb_Message_New_inl(l, a);
}

void _upb_Message_Clear(upb_Message* msg, const upb_MiniTable* l) {
  void* mem = UPB_PTR_AT(msg, -sizeof(upb_Message_Internal), char);
  memset(mem, 0, upb_msg_sizeof(l));
}

static bool realloc_internal(upb_Message* msg, size_t need, upb_Arena* arena) {
  upb_Message_Internal* in = upb_Message_Getinternal(msg);
  if (!in->internal) {
    /* No internal data, allocate from scratch. */
    size_t size = UPB_MAX(128, _upb_Log2CeilingSize(need + overhead));
    upb_Message_InternalData* internal = upb_Arena_Malloc(arena, size);
    if (!internal) return false;
    internal->size = size;
    internal->unknown_end = overhead;
    internal->ext_begin = size;
    in->internal = internal;
  } else if (in->internal->ext_begin - in->internal->unknown_end < need) {
    /* Internal data is too small, reallocate. */
    size_t new_size = _upb_Log2CeilingSize(in->internal->size + need);
    size_t ext_bytes = in->internal->size - in->internal->ext_begin;
    size_t new_ext_begin = new_size - ext_bytes;
    upb_Message_InternalData* internal =
        upb_Arena_Realloc(arena, in->internal, in->internal->size, new_size);
    if (!internal) return false;
    if (ext_bytes) {
      /* Need to move extension data to the end. */
      char* ptr = (char*)internal;
      memmove(ptr + new_ext_begin, ptr + internal->ext_begin, ext_bytes);
    }
    internal->ext_begin = new_ext_begin;
    internal->size = new_size;
    in->internal = internal;
  }
  UPB_ASSERT(in->internal->ext_begin - in->internal->unknown_end >= need);
  return true;
}

bool _upb_Message_AddUnknown(upb_Message* msg, const char* data, size_t len,
                             upb_Arena* arena) {
  if (!realloc_internal(msg, len, arena)) return false;
  upb_Message_Internal* in = upb_Message_Getinternal(msg);
  memcpy(UPB_PTR_AT(in->internal, in->internal->unknown_end, char), data, len);
  in->internal->unknown_end += len;
  return true;
}

void _upb_Message_DiscardUnknown_shallow(upb_Message* msg) {
  upb_Message_Internal* in = upb_Message_Getinternal(msg);
  if (in->internal) {
    in->internal->unknown_end = overhead;
  }
}

const char* upb_Message_GetUnknown(const upb_Message* msg, size_t* len) {
  const upb_Message_Internal* in = upb_Message_Getinternal_const(msg);
  if (in->internal) {
    *len = in->internal->unknown_end - overhead;
    return (char*)(in->internal + 1);
  } else {
    *len = 0;
    return NULL;
  }
}

const upb_Message_Extension* _upb_Message_Getexts(const upb_Message* msg,
                                                  size_t* count) {
  const upb_Message_Internal* in = upb_Message_Getinternal_const(msg);
  if (in->internal) {
    *count = (in->internal->size - in->internal->ext_begin) /
             sizeof(upb_Message_Extension);
    return UPB_PTR_AT(in->internal, in->internal->ext_begin, void);
  } else {
    *count = 0;
    return NULL;
  }
}

const upb_Message_Extension* _upb_Message_Getext(
    const upb_Message* msg, const upb_MiniTable_Extension* e) {
  size_t n;
  const upb_Message_Extension* ext = _upb_Message_Getexts(msg, &n);

  /* For now we use linear search exclusively to find extensions. If this
   * becomes an issue due to messages with lots of extensions, we can introduce
   * a table of some sort. */
  for (size_t i = 0; i < n; i++) {
    if (ext[i].ext == e) {
      return &ext[i];
    }
  }

  return NULL;
}

void _upb_Message_Clearext(upb_Message* msg,
                           const upb_MiniTable_Extension* ext_l) {
  upb_Message_Internal* in = upb_Message_Getinternal(msg);
  if (!in->internal) return;
  const upb_Message_Extension* base =
      UPB_PTR_AT(in->internal, in->internal->ext_begin, void);
  upb_Message_Extension* ext =
      (upb_Message_Extension*)_upb_Message_Getext(msg, ext_l);
  if (ext) {
    *ext = *base;
    in->internal->ext_begin += sizeof(upb_Message_Extension);
  }
}

upb_Message_Extension* _upb_Message_GetOrCreateExtension(
    upb_Message* msg, const upb_MiniTable_Extension* e, upb_Arena* arena) {
  upb_Message_Extension* ext =
      (upb_Message_Extension*)_upb_Message_Getext(msg, e);
  if (ext) return ext;
  if (!realloc_internal(msg, sizeof(upb_Message_Extension), arena)) return NULL;
  upb_Message_Internal* in = upb_Message_Getinternal(msg);
  in->internal->ext_begin -= sizeof(upb_Message_Extension);
  ext = UPB_PTR_AT(in->internal, in->internal->ext_begin, void);
  memset(ext, 0, sizeof(upb_Message_Extension));
  ext->ext = e;
  return ext;
}

size_t upb_Message_ExtensionCount(const upb_Message* msg) {
  size_t count;
  _upb_Message_Getexts(msg, &count);
  return count;
}

/** upb_Array *****************************************************************/

bool _upb_array_realloc(upb_Array* arr, size_t min_size, upb_Arena* arena) {
  size_t new_size = UPB_MAX(arr->size, 4);
  int elem_size_lg2 = arr->data & 7;
  size_t old_bytes = arr->size << elem_size_lg2;
  size_t new_bytes;
  void* ptr = _upb_array_ptr(arr);

  /* Log2 ceiling of size. */
  while (new_size < min_size) new_size *= 2;

  new_bytes = new_size << elem_size_lg2;
  ptr = upb_Arena_Realloc(arena, ptr, old_bytes, new_bytes);

  if (!ptr) {
    return false;
  }

  arr->data = _upb_tag_arrptr(ptr, elem_size_lg2);
  arr->size = new_size;
  return true;
}

static upb_Array* getorcreate_array(upb_Array** arr_ptr, int elem_size_lg2,
                                    upb_Arena* arena) {
  upb_Array* arr = *arr_ptr;
  if (!arr) {
    arr = _upb_Array_New(arena, 4, elem_size_lg2);
    if (!arr) return NULL;
    *arr_ptr = arr;
  }
  return arr;
}

void* _upb_Array_Resize_fallback(upb_Array** arr_ptr, size_t size,
                                 int elem_size_lg2, upb_Arena* arena) {
  upb_Array* arr = getorcreate_array(arr_ptr, elem_size_lg2, arena);
  return arr && _upb_Array_Resize(arr, size, arena) ? _upb_array_ptr(arr)
                                                    : NULL;
}

bool _upb_Array_Append_fallback(upb_Array** arr_ptr, const void* value,
                                int elem_size_lg2, upb_Arena* arena) {
  upb_Array* arr = getorcreate_array(arr_ptr, elem_size_lg2, arena);
  if (!arr) return false;

  size_t elems = arr->len;

  if (!_upb_Array_Resize(arr, elems + 1, arena)) {
    return false;
  }

  char* data = _upb_array_ptr(arr);
  memcpy(data + (elems << elem_size_lg2), value, 1 << elem_size_lg2);
  return true;
}

/** upb_Map *******************************************************************/

upb_Map* _upb_Map_New(upb_Arena* a, size_t key_size, size_t value_size) {
  upb_Map* map = upb_Arena_Malloc(a, sizeof(upb_Map));

  if (!map) {
    return NULL;
  }

  upb_strtable_init(&map->table, 4, a);
  map->key_size = key_size;
  map->val_size = value_size;

  return map;
}

static void _upb_mapsorter_getkeys(const void* _a, const void* _b, void* a_key,
                                   void* b_key, size_t size) {
  const upb_tabent* const* a = _a;
  const upb_tabent* const* b = _b;
  upb_StringView a_tabkey = upb_tabstrview((*a)->key);
  upb_StringView b_tabkey = upb_tabstrview((*b)->key);
  _upb_map_fromkey(a_tabkey, a_key, size);
  _upb_map_fromkey(b_tabkey, b_key, size);
}

#define UPB_COMPARE_INTEGERS(a, b) ((a) < (b) ? -1 : ((a) == (b) ? 0 : 1))

static int _upb_mapsorter_cmpi64(const void* _a, const void* _b) {
  int64_t a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, 8);
  return UPB_COMPARE_INTEGERS(a, b);
}

static int _upb_mapsorter_cmpu64(const void* _a, const void* _b) {
  uint64_t a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, 8);
  return UPB_COMPARE_INTEGERS(a, b);
}

static int _upb_mapsorter_cmpi32(const void* _a, const void* _b) {
  int32_t a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, 4);
  return UPB_COMPARE_INTEGERS(a, b);
}

static int _upb_mapsorter_cmpu32(const void* _a, const void* _b) {
  uint32_t a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, 4);
  return UPB_COMPARE_INTEGERS(a, b);
}

static int _upb_mapsorter_cmpbool(const void* _a, const void* _b) {
  bool a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, 1);
  return UPB_COMPARE_INTEGERS(a, b);
}

static int _upb_mapsorter_cmpstr(const void* _a, const void* _b) {
  upb_StringView a, b;
  _upb_mapsorter_getkeys(_a, _b, &a, &b, UPB_MAPTYPE_STRING);
  size_t common_size = UPB_MIN(a.size, b.size);
  int cmp = memcmp(a.data, b.data, common_size);
  if (cmp) return -cmp;
  return UPB_COMPARE_INTEGERS(a.size, b.size);
}

#undef UPB_COMPARE_INTEGERS

bool _upb_mapsorter_pushmap(_upb_mapsorter* s, upb_FieldType key_type,
                            const upb_Map* map, _upb_sortedmap* sorted) {
  int map_size = _upb_Map_Size(map);
  sorted->start = s->size;
  sorted->pos = sorted->start;
  sorted->end = sorted->start + map_size;

  /* Grow s->entries if necessary. */
  if (sorted->end > s->cap) {
    s->cap = _upb_Log2CeilingSize(sorted->end);
    s->entries = realloc(s->entries, s->cap * sizeof(*s->entries));
    if (!s->entries) return false;
  }

  s->size = sorted->end;

  /* Copy non-empty entries from the table to s->entries. */
  upb_tabent const** dst = &s->entries[sorted->start];
  const upb_tabent* src = map->table.t.entries;
  const upb_tabent* end = src + upb_table_size(&map->table.t);
  for (; src < end; src++) {
    if (!upb_tabent_isempty(src)) {
      *dst = src;
      dst++;
    }
  }
  UPB_ASSERT(dst == &s->entries[sorted->end]);

  /* Sort entries according to the key type. */

  int (*compar)(const void*, const void*);

  switch (key_type) {
    case kUpb_FieldType_Int64:
    case kUpb_FieldType_SFixed64:
    case kUpb_FieldType_SInt64:
      compar = _upb_mapsorter_cmpi64;
      break;
    case kUpb_FieldType_UInt64:
    case kUpb_FieldType_Fixed64:
      compar = _upb_mapsorter_cmpu64;
      break;
    case kUpb_FieldType_Int32:
    case kUpb_FieldType_SInt32:
    case kUpb_FieldType_SFixed32:
    case kUpb_FieldType_Enum:
      compar = _upb_mapsorter_cmpi32;
      break;
    case kUpb_FieldType_UInt32:
    case kUpb_FieldType_Fixed32:
      compar = _upb_mapsorter_cmpu32;
      break;
    case kUpb_FieldType_Bool:
      compar = _upb_mapsorter_cmpbool;
      break;
    case kUpb_FieldType_String:
    case kUpb_FieldType_Bytes:
      compar = _upb_mapsorter_cmpstr;
      break;
    default:
      UPB_UNREACHABLE();
  }

  qsort(&s->entries[sorted->start], map_size, sizeof(*s->entries), compar);
  return true;
}
