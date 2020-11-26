/*
** Our memory representation for parsing tables and messages themselves.
** Functions in this file are used by generated code and possibly reflection.
**
** The definitions in this file are internal to upb.
**/

#ifndef UPB_MSG_H_
#define UPB_MSG_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "upb/table.int.h"
#include "upb/upb.h"

/* Must be last. */
#include "upb/port_def.inc"

#ifdef __cplusplus
extern "C" {
#endif

#define PTR_AT(msg, ofs, type) (type*)((const char*)msg + ofs)

typedef void upb_msg;

/** upb_msglayout *************************************************************/

/* upb_msglayout represents the memory layout of a given upb_msgdef.  The
 * members are public so generated code can initialize them, but users MUST NOT
 * read or write any of its members. */

/* These aren't real labels according to descriptor.proto, but in the table we
 * use these for map/packed fields instead of UPB_LABEL_REPEATED. */
enum {
  _UPB_LABEL_MAP = 4,
  _UPB_LABEL_PACKED = 7  /* Low 3 bits are common with UPB_LABEL_REPEATED. */
};

typedef struct {
  uint32_t number;
  uint16_t offset;
  int16_t presence;       /* If >0, hasbit_index.  If <0, ~oneof_index. */
  uint16_t submsg_index;  /* undefined if descriptortype != MESSAGE or GROUP. */
  uint8_t descriptortype;
  uint8_t label;          /* google.protobuf.Label or _UPB_LABEL_* above. */
} upb_msglayout_field;

struct upb_decstate;
struct upb_msglayout;

typedef const char *_upb_field_parser(struct upb_decstate *d, const char *ptr,
                                      upb_msg *msg, intptr_t table,
                                      uint64_t hasbits, uint64_t data);

typedef struct {
  uint64_t field_data;
  _upb_field_parser *field_parser;
} _upb_fasttable_entry;

typedef struct upb_msglayout {
  const struct upb_msglayout *const* submsgs;
  const upb_msglayout_field *fields;
  /* Must be aligned to sizeof(void*).  Doesn't include internal members like
   * unknown fields, extension dict, pointer to msglayout, etc. */
  uint16_t size;
  uint16_t field_count;
  bool extendable;
  uint8_t table_mask;
  /* To constant-initialize the tables of variable length, we need a flexible
   * array member, and we need to compile in C99 mode. */
  _upb_fasttable_entry fasttable[];
} upb_msglayout;

/** upb_msg *******************************************************************/

/* Internal members of a upb_msg.  We can change this without breaking binary
 * compatibility.  We put these before the user's data.  The user's upb_msg*
 * points after the upb_msg_internal. */

typedef struct {
  uint32_t len;
  uint32_t size;
  /* Data follows. */
} upb_msg_unknowndata;

/* Used when a message is not extendable. */
typedef struct {
  upb_msg_unknowndata *unknown;
} upb_msg_internal;

/* Maps upb_fieldtype_t -> memory size. */
extern char _upb_fieldtype_to_size[12];

UPB_INLINE size_t upb_msg_sizeof(const upb_msglayout *l) {
  return l->size + sizeof(upb_msg_internal);
}

UPB_INLINE upb_msg *_upb_msg_new_inl(const upb_msglayout *l, upb_arena *a) {
  size_t size = upb_msg_sizeof(l);
  void *mem = upb_arena_malloc(a, size);
  upb_msg *msg;
  if (UPB_UNLIKELY(!mem)) return NULL;
  msg = UPB_PTR_AT(mem, sizeof(upb_msg_internal), upb_msg);
  memset(mem, 0, size);
  return msg;
}

/* Creates a new messages with the given layout on the given arena. */
upb_msg *_upb_msg_new(const upb_msglayout *l, upb_arena *a);

UPB_INLINE upb_msg_internal *upb_msg_getinternal(upb_msg *msg) {
  ptrdiff_t size = sizeof(upb_msg_internal);
  return (upb_msg_internal*)((char*)msg - size);
}

/* Clears the given message. */
void _upb_msg_clear(upb_msg *msg, const upb_msglayout *l);

/* Discards the unknown fields for this message only. */
void _upb_msg_discardunknown_shallow(upb_msg *msg);

/* Adds unknown data (serialized protobuf data) to the given message.  The data
 * is copied into the message instance. */
bool _upb_msg_addunknown(upb_msg *msg, const char *data, size_t len,
                         upb_arena *arena);

/* Returns a reference to the message's unknown data. */
const char *upb_msg_getunknown(const upb_msg *msg, size_t *len);

/** Hasbit access *************************************************************/

UPB_INLINE bool _upb_hasbit(const upb_msg *msg, size_t idx) {
  return (*PTR_AT(msg, idx / 8, const char) & (1 << (idx % 8))) != 0;
}

UPB_INLINE void _upb_sethas(const upb_msg *msg, size_t idx) {
  (*PTR_AT(msg, idx / 8, char)) |= (char)(1 << (idx % 8));
}

UPB_INLINE void _upb_clearhas(const upb_msg *msg, size_t idx) {
  (*PTR_AT(msg, idx / 8, char)) &= (char)(~(1 << (idx % 8)));
}

UPB_INLINE size_t _upb_msg_hasidx(const upb_msglayout_field *f) {
  UPB_ASSERT(f->presence > 0);
  return f->presence;
}

UPB_INLINE bool _upb_hasbit_field(const upb_msg *msg,
                                  const upb_msglayout_field *f) {
  return _upb_hasbit(msg, _upb_msg_hasidx(f));
}

UPB_INLINE void _upb_sethas_field(const upb_msg *msg,
                                  const upb_msglayout_field *f) {
  _upb_sethas(msg, _upb_msg_hasidx(f));
}

UPB_INLINE void _upb_clearhas_field(const upb_msg *msg,
                                    const upb_msglayout_field *f) {
  _upb_clearhas(msg, _upb_msg_hasidx(f));
}

/** Oneof case access *********************************************************/

UPB_INLINE uint32_t *_upb_oneofcase(upb_msg *msg, size_t case_ofs) {
  return PTR_AT(msg, case_ofs, uint32_t);
}

UPB_INLINE uint32_t _upb_getoneofcase(const void *msg, size_t case_ofs) {
  return *PTR_AT(msg, case_ofs, uint32_t);
}

UPB_INLINE size_t _upb_oneofcase_ofs(const upb_msglayout_field *f) {
  UPB_ASSERT(f->presence < 0);
  return ~(ptrdiff_t)f->presence;
}

UPB_INLINE uint32_t *_upb_oneofcase_field(upb_msg *msg,
                                          const upb_msglayout_field *f) {
  return _upb_oneofcase(msg, _upb_oneofcase_ofs(f));
}

UPB_INLINE uint32_t _upb_getoneofcase_field(const upb_msg *msg,
                                            const upb_msglayout_field *f) {
  return _upb_getoneofcase(msg, _upb_oneofcase_ofs(f));
}

UPB_INLINE bool _upb_has_submsg_nohasbit(const upb_msg *msg, size_t ofs) {
  return *PTR_AT(msg, ofs, const upb_msg*) != NULL;
}

UPB_INLINE bool _upb_isrepeated(const upb_msglayout_field *field) {
  return (field->label & 3) == UPB_LABEL_REPEATED;
}

UPB_INLINE bool _upb_repeated_or_map(const upb_msglayout_field *field) {
  return field->label >= UPB_LABEL_REPEATED;
}

/** upb_array *****************************************************************/

/* Our internal representation for repeated fields.  */
typedef struct {
  uintptr_t data;   /* Tagged ptr: low 3 bits of ptr are lg2(elem size). */
  size_t len;   /* Measured in elements. */
  size_t size;  /* Measured in elements. */
  uint64_t junk;
} upb_array;

UPB_INLINE const void *_upb_array_constptr(const upb_array *arr) {
  UPB_ASSERT((arr->data & 7) <= 4);
  return (void*)(arr->data & ~(uintptr_t)7);
}

UPB_INLINE uintptr_t _upb_array_tagptr(void* ptr, int elem_size_lg2) {
  UPB_ASSERT(elem_size_lg2 <= 4);
  return (uintptr_t)ptr | elem_size_lg2;
}

UPB_INLINE void *_upb_array_ptr(upb_array *arr) {
  return (void*)_upb_array_constptr(arr);
}

UPB_INLINE uintptr_t _upb_tag_arrptr(void* ptr, int elem_size_lg2) {
  UPB_ASSERT(elem_size_lg2 <= 4);
  UPB_ASSERT(((uintptr_t)ptr & 7) == 0);
  return (uintptr_t)ptr | (unsigned)elem_size_lg2;
}

UPB_INLINE upb_array *_upb_array_new(upb_arena *a, size_t init_size,
                                     int elem_size_lg2) {
  const size_t arr_size = UPB_ALIGN_UP(sizeof(upb_array), 8);
  const size_t bytes = sizeof(upb_array) + (init_size << elem_size_lg2);
  upb_array *arr = (upb_array*)upb_arena_malloc(a, bytes);
  if (!arr) return NULL;
  arr->data = _upb_tag_arrptr(UPB_PTR_AT(arr, arr_size, void), elem_size_lg2);
  arr->len = 0;
  arr->size = init_size;
  return arr;
}

/* Resizes the capacity of the array to be at least min_size. */
bool _upb_array_realloc(upb_array *arr, size_t min_size, upb_arena *arena);

/* Fallback functions for when the accessors require a resize. */
void *_upb_array_resize_fallback(upb_array **arr_ptr, size_t size,
                                 int elem_size_lg2, upb_arena *arena);
bool _upb_array_append_fallback(upb_array **arr_ptr, const void *value,
                                int elem_size_lg2, upb_arena *arena);

UPB_INLINE bool _upb_array_reserve(upb_array *arr, size_t size,
                                   upb_arena *arena) {
  if (arr->size < size) return _upb_array_realloc(arr, size, arena);
  return true;
}

UPB_INLINE bool _upb_array_resize(upb_array *arr, size_t size,
                                  upb_arena *arena) {
  if (!_upb_array_reserve(arr, size, arena)) return false;
  arr->len = size;
  return true;
}

UPB_INLINE const void *_upb_array_accessor(const void *msg, size_t ofs,
                                           size_t *size) {
  const upb_array *arr = *PTR_AT(msg, ofs, const upb_array*);
  if (arr) {
    if (size) *size = arr->len;
    return _upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}

UPB_INLINE void *_upb_array_mutable_accessor(void *msg, size_t ofs,
                                             size_t *size) {
  upb_array *arr = *PTR_AT(msg, ofs, upb_array*);
  if (arr) {
    if (size) *size = arr->len;
    return _upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}

UPB_INLINE void *_upb_array_resize_accessor2(void *msg, size_t ofs, size_t size,
                                             int elem_size_lg2,
                                             upb_arena *arena) {
  upb_array **arr_ptr = PTR_AT(msg, ofs, upb_array *);
  upb_array *arr = *arr_ptr;
  if (!arr || arr->size < size) {
    return _upb_array_resize_fallback(arr_ptr, size, elem_size_lg2, arena);
  }
  arr->len = size;
  return _upb_array_ptr(arr);
}

UPB_INLINE bool _upb_array_append_accessor2(void *msg, size_t ofs,
                                            int elem_size_lg2,
                                            const void *value,
                                            upb_arena *arena) {
  upb_array **arr_ptr = PTR_AT(msg, ofs, upb_array *);
  size_t elem_size = 1 << elem_size_lg2;
  upb_array *arr = *arr_ptr;
  void *ptr;
  if (!arr || arr->len == arr->size) {
    return _upb_array_append_fallback(arr_ptr, value, elem_size_lg2, arena);
  }
  ptr = _upb_array_ptr(arr);
  memcpy(PTR_AT(ptr, arr->len * elem_size, char), value, elem_size);
  arr->len++;
  return true;
}

/* Used by old generated code, remove once all code has been regenerated. */
UPB_INLINE int _upb_sizelg2(upb_fieldtype_t type) {
  switch (type) {
    case UPB_TYPE_BOOL:
      return 0;
    case UPB_TYPE_FLOAT:
    case UPB_TYPE_INT32:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_ENUM:
      return 2;
    case UPB_TYPE_MESSAGE:
      return UPB_SIZE(2, 3);
    case UPB_TYPE_DOUBLE:
    case UPB_TYPE_INT64:
    case UPB_TYPE_UINT64:
      return 3;
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
      return UPB_SIZE(3, 4);
  }
  UPB_UNREACHABLE();
}
UPB_INLINE void *_upb_array_resize_accessor(void *msg, size_t ofs, size_t size,
                                             upb_fieldtype_t type,
                                             upb_arena *arena) {
  return _upb_array_resize_accessor2(msg, ofs, size, _upb_sizelg2(type), arena);
}
UPB_INLINE bool _upb_array_append_accessor(void *msg, size_t ofs,
                                            size_t elem_size, upb_fieldtype_t type,
                                            const void *value,
                                            upb_arena *arena) {
  (void)elem_size;
  return _upb_array_append_accessor2(msg, ofs, _upb_sizelg2(type), value,
                                     arena);
}

/** upb_map *******************************************************************/

/* Right now we use strmaps for everything.  We'll likely want to use
 * integer-specific maps for integer-keyed maps.*/
typedef struct {
  /* Size of key and val, based on the map type.  Strings are represented as '0'
   * because they must be handled specially. */
  char key_size;
  char val_size;

  upb_strtable table;
} upb_map;

/* Map entries aren't actually stored, they are only used during parsing.  For
 * parsing, it helps a lot if all map entry messages have the same layout.
 * The compiler and def.c must ensure that all map entries have this layout. */
typedef struct {
  upb_msg_internal internal;
  union {
    upb_strview str;  /* For str/bytes. */
    upb_value val;    /* For all other types. */
  } k;
  union {
    upb_strview str;  /* For str/bytes. */
    upb_value val;    /* For all other types. */
  } v;
} upb_map_entry;

/* Creates a new map on the given arena with this key/value type. */
upb_map *_upb_map_new(upb_arena *a, size_t key_size, size_t value_size);

/* Converting between internal table representation and user values.
 *
 * _upb_map_tokey() and _upb_map_fromkey() are inverses.
 * _upb_map_tovalue() and _upb_map_fromvalue() are inverses.
 *
 * These functions account for the fact that strings are treated differently
 * from other types when stored in a map.
 */

UPB_INLINE upb_strview _upb_map_tokey(const void *key, size_t size) {
  if (size == UPB_MAPTYPE_STRING) {
    return *(upb_strview*)key;
  } else {
    return upb_strview_make((const char*)key, size);
  }
}

UPB_INLINE void _upb_map_fromkey(upb_strview key, void* out, size_t size) {
  if (size == UPB_MAPTYPE_STRING) {
    memcpy(out, &key, sizeof(key));
  } else {
    memcpy(out, key.data, size);
  }
}

UPB_INLINE bool _upb_map_tovalue(const void *val, size_t size, upb_value *msgval,
                                 upb_arena *a) {
  if (size == UPB_MAPTYPE_STRING) {
    upb_strview *strp = (upb_strview*)upb_arena_malloc(a, sizeof(*strp));
    if (!strp) return false;
    *strp = *(upb_strview*)val;
    *msgval = upb_value_ptr(strp);
  } else {
    memcpy(msgval, val, size);
  }
  return true;
}

UPB_INLINE void _upb_map_fromvalue(upb_value val, void* out, size_t size) {
  if (size == UPB_MAPTYPE_STRING) {
    const upb_strview *strp = (const upb_strview*)upb_value_getptr(val);
    memcpy(out, strp, sizeof(upb_strview));
  } else {
    memcpy(out, &val, size);
  }
}

/* Map operations, shared by reflection and generated code. */

UPB_INLINE size_t _upb_map_size(const upb_map *map) {
  return map->table.t.count;
}

UPB_INLINE bool _upb_map_get(const upb_map *map, const void *key,
                             size_t key_size, void *val, size_t val_size) {
  upb_value tabval;
  upb_strview k = _upb_map_tokey(key, key_size);
  bool ret = upb_strtable_lookup2(&map->table, k.data, k.size, &tabval);
  if (ret && val) {
    _upb_map_fromvalue(tabval, val, val_size);
  }
  return ret;
}

UPB_INLINE void* _upb_map_next(const upb_map *map, size_t *iter) {
  upb_strtable_iter it;
  it.t = &map->table;
  it.index = *iter;
  upb_strtable_next(&it);
  *iter = it.index;
  if (upb_strtable_done(&it)) return NULL;
  return (void*)str_tabent(&it);
}

UPB_INLINE bool _upb_map_set(upb_map *map, const void *key, size_t key_size,
                             void *val, size_t val_size, upb_arena *arena) {
  upb_strview strkey = _upb_map_tokey(key, key_size);
  upb_value tabval = {0};
  if (!_upb_map_tovalue(val, val_size, &tabval, arena)) return false;
  upb_alloc *a = upb_arena_alloc(arena);

  /* TODO(haberman): add overwrite operation to minimize number of lookups. */
  upb_strtable_remove3(&map->table, strkey.data, strkey.size, NULL, a);
  return upb_strtable_insert3(&map->table, strkey.data, strkey.size, tabval, a);
}

UPB_INLINE bool _upb_map_delete(upb_map *map, const void *key, size_t key_size) {
  upb_strview k = _upb_map_tokey(key, key_size);
  return upb_strtable_remove3(&map->table, k.data, k.size, NULL, NULL);
}

UPB_INLINE void _upb_map_clear(upb_map *map) {
  upb_strtable_clear(&map->table);
}

/* Message map operations, these get the map from the message first. */

UPB_INLINE size_t _upb_msg_map_size(const upb_msg *msg, size_t ofs) {
  upb_map *map = *UPB_PTR_AT(msg, ofs, upb_map *);
  return map ? _upb_map_size(map) : 0;
}

UPB_INLINE bool _upb_msg_map_get(const upb_msg *msg, size_t ofs,
                                 const void *key, size_t key_size, void *val,
                                 size_t val_size) {
  upb_map *map = *UPB_PTR_AT(msg, ofs, upb_map *);
  if (!map) return false;
  return _upb_map_get(map, key, key_size, val, val_size);
}

UPB_INLINE void *_upb_msg_map_next(const upb_msg *msg, size_t ofs,
                                   size_t *iter) {
  upb_map *map = *UPB_PTR_AT(msg, ofs, upb_map *);
  if (!map) return NULL;
  return _upb_map_next(map, iter);
}

UPB_INLINE bool _upb_msg_map_set(upb_msg *msg, size_t ofs, const void *key,
                                 size_t key_size, void *val, size_t val_size,
                                 upb_arena *arena) {
  upb_map **map = PTR_AT(msg, ofs, upb_map *);
  if (!*map) {
    *map = _upb_map_new(arena, key_size, val_size);
  }
  return _upb_map_set(*map, key, key_size, val, val_size, arena);
}

UPB_INLINE bool _upb_msg_map_delete(upb_msg *msg, size_t ofs, const void *key,
                                    size_t key_size) {
  upb_map *map = *UPB_PTR_AT(msg, ofs, upb_map *);
  if (!map) return false;
  return _upb_map_delete(map, key, key_size);
}

UPB_INLINE void _upb_msg_map_clear(upb_msg *msg, size_t ofs) {
  upb_map *map = *UPB_PTR_AT(msg, ofs, upb_map *);
  if (!map) return;
  _upb_map_clear(map);
}

/* Accessing map key/value from a pointer, used by generated code only. */

UPB_INLINE void _upb_msg_map_key(const void* msg, void* key, size_t size) {
  const upb_tabent *ent = (const upb_tabent*)msg;
  uint32_t u32len;
  upb_strview k;
  k.data = upb_tabstr(ent->key, &u32len);
  k.size = u32len;
  _upb_map_fromkey(k, key, size);
}

UPB_INLINE void _upb_msg_map_value(const void* msg, void* val, size_t size) {
  const upb_tabent *ent = (const upb_tabent*)msg;
  upb_value v;
  _upb_value_setval(&v, ent->val.val);
  _upb_map_fromvalue(v, val, size);
}

UPB_INLINE void _upb_msg_map_set_value(void* msg, const void* val, size_t size) {
  upb_tabent *ent = (upb_tabent*)msg;
  /* This is like _upb_map_tovalue() except the entry already exists so we can
   * reuse the allocated upb_strview for string fields. */
  if (size == UPB_MAPTYPE_STRING) {
    upb_strview *strp = (upb_strview*)(uintptr_t)ent->val.val;
    memcpy(strp, val, sizeof(*strp));
  } else {
    memcpy(&ent->val.val, val, size);
  }
}

/** _upb_mapsorter *************************************************************/

/* _upb_mapsorter sorts maps and provides ordered iteration over the entries.
 * Since maps can be recursive (map values can be messages which contain other maps).
 * _upb_mapsorter can contain a stack of maps. */

typedef struct {
  upb_tabent const**entries;
  int size;
  int cap;
} _upb_mapsorter;

typedef struct {
  int start;
  int pos;
  int end;
} _upb_sortedmap;

UPB_INLINE void _upb_mapsorter_init(_upb_mapsorter *s) {
  s->entries = NULL;
  s->size = 0;
  s->cap = 0;
}

UPB_INLINE void _upb_mapsorter_destroy(_upb_mapsorter *s) {
  if (s->entries) free(s->entries);
}

bool _upb_mapsorter_pushmap(_upb_mapsorter *s, upb_descriptortype_t key_type,
                            const upb_map *map, _upb_sortedmap *sorted);

UPB_INLINE void _upb_mapsorter_popmap(_upb_mapsorter *s, _upb_sortedmap *sorted) {
  s->size = sorted->start;
}

UPB_INLINE bool _upb_sortedmap_next(_upb_mapsorter *s, const upb_map *map,
                                    _upb_sortedmap *sorted,
                                    upb_map_entry *ent) {
  if (sorted->pos == sorted->end) return false;
  const upb_tabent *tabent = s->entries[sorted->pos++];
  upb_strview key = upb_tabstrview(tabent->key);
  _upb_map_fromkey(key, &ent->k, map->key_size);
  upb_value val = {tabent->val.val};
  _upb_map_fromvalue(val, &ent->v, map->val_size);
  return true;
}

#undef PTR_AT

#ifdef __cplusplus
}  /* extern "C" */
#endif

#include "upb/port_undef.inc"

#endif /* UPB_MSG_H_ */
