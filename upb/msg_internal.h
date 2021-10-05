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

/*
** Our memory representation for parsing tables and messages themselves.
** Functions in this file are used by generated code and possibly reflection.
**
** The definitions in this file are internal to upb.
**/

#ifndef UPB_MSG_INT_H_
#define UPB_MSG_INT_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "upb/msg.h"
#include "upb/table_internal.h"
#include "upb/upb.h"

/* Must be last. */
#include "upb/port_def.inc"

#ifdef __cplusplus
extern "C" {
#endif

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
  uint8_t mode; /* upb_fieldmode | upb_labelflags |
                   (upb_rep << _UPB_REP_SHIFT) */
} upb_msglayout_field;

typedef enum {
  _UPB_MODE_MAP = 0,
  _UPB_MODE_ARRAY = 1,
  _UPB_MODE_SCALAR = 2,

  _UPB_MODE_MASK = 3,  /* Mask to isolate the mode from upb_rep. */
} upb_fieldmode;

/* Extra flags on the mode field. */
enum upb_labelflags {
  _UPB_MODE_IS_PACKED = 4,
  _UPB_MODE_IS_EXTENSION = 8,
};

/* Representation in the message.  Derivable from descriptortype and mode, but
 * fast access helps the serializer. */
enum upb_rep {
  _UPB_REP_1BYTE = 0,
  _UPB_REP_4BYTE = 1,
  _UPB_REP_8BYTE = 2,
  _UPB_REP_STRVIEW = 3,

#if UINTPTR_MAX == 0xffffffff
  _UPB_REP_PTR = _UPB_REP_4BYTE,
#else
  _UPB_REP_PTR = _UPB_REP_8BYTE,
#endif

  _UPB_REP_SHIFT = 6,  /* Bit offset of the rep in upb_msglayout_field.mode */
};

UPB_INLINE upb_fieldmode _upb_getmode(const upb_msglayout_field *field) {
  return (upb_fieldmode)(field->mode & 3);
}

UPB_INLINE bool _upb_repeated_or_map(const upb_msglayout_field *field) {
  /* This works because upb_fieldmode has no value 3. */
  return !(field->mode & _UPB_MODE_SCALAR);
}

UPB_INLINE bool _upb_issubmsg(const upb_msglayout_field *field) {
  return field->descriptortype == UPB_DTYPE_MESSAGE ||
         field->descriptortype == UPB_DTYPE_GROUP;
}

struct upb_decstate;
struct upb_msglayout;

typedef const char *_upb_field_parser(struct upb_decstate *d, const char *ptr,
                                      upb_msg *msg, intptr_t table,
                                      uint64_t hasbits, uint64_t data);

typedef struct {
  uint64_t field_data;
  _upb_field_parser *field_parser;
} _upb_fasttable_entry;

typedef union {
  const struct upb_msglayout *submsg;
  // TODO: const upb_enumlayout *subenum;
} upb_msglayout_sub;

typedef enum {
  _UPB_MSGEXT_NONE = 0,         // Non-extendable message.
  _UPB_MSGEXT_EXTENDABLE = 1,   // Normal extendable message.
  _UPB_MSGEXT_MSGSET = 2,       // MessageSet message.
  _UPB_MSGEXT_MSGSET_ITEM = 3,  // MessageSet item (temporary only, see decode.c)
} upb_msgext_mode;

/* MessageSet wire format is:
 *   message MessageSet {
 *     repeated group Item = 1 {
 *       required int32 type_id = 2;
 *       required string message = 3;
 *     }
 *   }
 */
typedef enum {
  _UPB_MSGSET_ITEM = 1,
  _UPB_MSGSET_TYPEID = 2,
  _UPB_MSGSET_MESSAGE = 3,
} upb_msgext_fieldnum;

struct upb_msglayout {
  const upb_msglayout_sub *subs;
  const upb_msglayout_field *fields;
  /* Must be aligned to sizeof(void*).  Doesn't include internal members like
   * unknown fields, extension dict, pointer to msglayout, etc. */
  uint16_t size;
  uint16_t field_count;
  uint8_t ext;  // upb_msgext_mode, declared as uint8_t so sizeof(ext) == 1
  uint8_t dense_below;
  uint8_t table_mask;
  /* To constant-initialize the tables of variable length, we need a flexible
   * array member, and we need to compile in C99 mode. */
  _upb_fasttable_entry fasttable[];
};

typedef struct {
  upb_msglayout_field field;
  const upb_msglayout *extendee;
  upb_msglayout_sub sub;   /* NULL unless submessage or proto2 enum */
} upb_msglayout_ext;

typedef struct {
  const upb_msglayout **msgs;
  const upb_msglayout_ext **exts;
  int msg_count;
  int ext_count;
} upb_msglayout_file;

/** upb_extreg ****************************************************************/

/* Adds the given extension info for message type |l| and field number |num|
 * into the registry. Returns false if this message type and field number were
 * already in the map, or if memory allocation fails. */
bool _upb_extreg_add(upb_extreg *r, const upb_msglayout_ext **e, size_t count);

/* Looks up the extension (if any) defined for message type |l| and field
 * number |num|.  If an extension was found, copies the field info into |*ext|
 * and returns true. Otherwise returns false. */
const upb_msglayout_ext *_upb_extreg_get(const upb_extreg *r,
                                         const upb_msglayout *l, uint32_t num);

/** upb_msg *******************************************************************/

/* Internal members of a upb_msg that track unknown fields and/or extensions.
 * We can change this without breaking binary compatibility.  We put these
 * before the user's data.  The user's upb_msg* points after the
 * upb_msg_internal. */

typedef struct {
  /* Total size of this structure, including the data that follows.
   * Must be aligned to 8, which is alignof(upb_msg_ext) */
  uint32_t size;

  /* Offsets relative to the beginning of this structure.
   *
   * Unknown data grows forward from the beginning to unknown_end.
   * Extension data grows backward from size to ext_begin.
   * When the two meet, we're out of data and have to realloc.
   *
   * If we imagine that the final member of this struct is:
   *   char data[size - overhead];  // overhead = sizeof(upb_msg_internaldata)
   * 
   * Then we have:
   *   unknown data: data[0 .. (unknown_end - overhead)]
   *   extensions data: data[(ext_begin - overhead) .. (size - overhead)] */
  uint32_t unknown_end;
  uint32_t ext_begin;
  /* Data follows, as if there were an array:
   *   char data[size - sizeof(upb_msg_internaldata)]; */
} upb_msg_internaldata;

typedef struct {
  upb_msg_internaldata *internal;
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

/** upb_msg_ext ***************************************************************/

/* The internal representation of an extension is self-describing: it contains
 * enough information that we can serialize it to binary format without needing
 * to look it up in a registry. */
typedef struct {
  const upb_msglayout_ext *ext;
  union {
    upb_strview str;
    void *ptr;
    double dbl;
    char scalar_data[8];
  } data;
} upb_msg_ext;

/* Adds the given extension data to the given message. The returned extension will
 * have its "ext" member initialized according to |ext|. */
upb_msg_ext *_upb_msg_getorcreateext(upb_msg *msg, const upb_msglayout_ext *ext,
                                     upb_arena *arena);

/* Returns an array of extensions for this message. Note: the array is
 * ordered in reverse relative to the order of creation. */
const upb_msg_ext *_upb_msg_getexts(const upb_msg *msg, size_t *count);

/* Returns an extension for the given field number, or NULL if no extension
 * exists for this field number. */
const upb_msg_ext *_upb_msg_getext(const upb_msg *msg,
                                   const upb_msglayout_ext *ext);

void _upb_msg_clearext(upb_msg *msg, const upb_msglayout_ext *ext);

/** Hasbit access *************************************************************/

UPB_INLINE bool _upb_hasbit(const upb_msg *msg, size_t idx) {
  return (*UPB_PTR_AT(msg, idx / 8, const char) & (1 << (idx % 8))) != 0;
}

UPB_INLINE void _upb_sethas(const upb_msg *msg, size_t idx) {
  (*UPB_PTR_AT(msg, idx / 8, char)) |= (char)(1 << (idx % 8));
}

UPB_INLINE void _upb_clearhas(const upb_msg *msg, size_t idx) {
  (*UPB_PTR_AT(msg, idx / 8, char)) &= (char)(~(1 << (idx % 8)));
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
  return UPB_PTR_AT(msg, case_ofs, uint32_t);
}

UPB_INLINE uint32_t _upb_getoneofcase(const void *msg, size_t case_ofs) {
  return *UPB_PTR_AT(msg, case_ofs, uint32_t);
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
  return *UPB_PTR_AT(msg, ofs, const upb_msg*) != NULL;
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
  const upb_array *arr = *UPB_PTR_AT(msg, ofs, const upb_array*);
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
  upb_array *arr = *UPB_PTR_AT(msg, ofs, upb_array*);
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
  upb_array **arr_ptr = UPB_PTR_AT(msg, ofs, upb_array *);
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
  upb_array **arr_ptr = UPB_PTR_AT(msg, ofs, upb_array *);
  size_t elem_size = 1 << elem_size_lg2;
  upb_array *arr = *arr_ptr;
  void *ptr;
  if (!arr || arr->len == arr->size) {
    return _upb_array_append_fallback(arr_ptr, value, elem_size_lg2, arena);
  }
  ptr = _upb_array_ptr(arr);
  memcpy(UPB_PTR_AT(ptr, arr->len * elem_size, char), value, elem_size);
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
                             void *val, size_t val_size, upb_arena *a) {
  upb_strview strkey = _upb_map_tokey(key, key_size);
  upb_value tabval = {0};
  if (!_upb_map_tovalue(val, val_size, &tabval, a)) return false;

  /* TODO(haberman): add overwrite operation to minimize number of lookups. */
  upb_strtable_remove(&map->table, strkey.data, strkey.size, NULL);
  return upb_strtable_insert(&map->table, strkey.data, strkey.size, tabval, a);
}

UPB_INLINE bool _upb_map_delete(upb_map *map, const void *key, size_t key_size) {
  upb_strview k = _upb_map_tokey(key, key_size);
  return upb_strtable_remove(&map->table, k.data, k.size, NULL);
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
  upb_map **map = UPB_PTR_AT(msg, ofs, upb_map *);
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
  upb_value v = {ent->val.val};
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

#ifdef __cplusplus
}  /* extern "C" */
#endif

#include "upb/port_undef.inc"

#endif /* UPB_MSG_INT_H_ */
