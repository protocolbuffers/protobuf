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

#include "upb/extension_registry.h"
#include "upb/internal/table.h"
#include "upb/msg.h"
#include "upb/upb.h"

/* Must be last. */
#include "upb/port_def.inc"

#ifdef __cplusplus
extern "C" {
#endif

/** upb_*Int* conversion routines ********************************************/

UPB_INLINE int32_t _upb_Int32_FromI(int v) { return (int32_t)v; }

UPB_INLINE int64_t _upb_Int64_FromLL(long long v) { return (int64_t)v; }

UPB_INLINE uint32_t _upb_UInt32_FromU(unsigned v) { return (uint32_t)v; }

UPB_INLINE uint64_t _upb_UInt64_FromULL(unsigned long long v) {
  return (uint64_t)v;
}

/** upb_MiniTable *************************************************************/

/* upb_MiniTable represents the memory layout of a given upb_MessageDef.  The
 * members are public so generated code can initialize them, but users MUST NOT
 * read or write any of its members. */

typedef struct {
  uint32_t number;
  uint16_t offset;
  int16_t presence;       // If >0, hasbit_index.  If <0, ~oneof_index
  uint16_t submsg_index;  // kUpb_NoSub if descriptortype != MESSAGE/GROUP/ENUM
  uint8_t descriptortype;
  uint8_t mode; /* upb_FieldMode | upb_LabelFlags |
                   (upb_FieldRep << kUpb_FieldRep_Shift) */
} upb_MiniTable_Field;

#define kUpb_NoSub ((uint16_t)-1)

typedef enum {
  kUpb_FieldMode_Map = 0,
  kUpb_FieldMode_Array = 1,
  kUpb_FieldMode_Scalar = 2,
} upb_FieldMode;

// Mask to isolate the upb_FieldMode from field.mode.
#define kUpb_FieldMode_Mask 3

/* Extra flags on the mode field. */
typedef enum {
  kUpb_LabelFlags_IsPacked = 4,
  kUpb_LabelFlags_IsExtension = 8,
} upb_LabelFlags;

// Note: we sort by this number when calculating layout order.
typedef enum {
  kUpb_FieldRep_1Byte = 0,
  kUpb_FieldRep_4Byte = 1,
  kUpb_FieldRep_StringView = 2,
  kUpb_FieldRep_Pointer = 3,
  kUpb_FieldRep_8Byte = 4,

  kUpb_FieldRep_Shift = 5,  // Bit offset of the rep in upb_MiniTable_Field.mode
  kUpb_FieldRep_Max = kUpb_FieldRep_8Byte,
} upb_FieldRep;

UPB_INLINE upb_FieldMode upb_FieldMode_Get(const upb_MiniTable_Field* field) {
  return (upb_FieldMode)(field->mode & 3);
}

UPB_INLINE bool upb_IsRepeatedOrMap(const upb_MiniTable_Field* field) {
  /* This works because upb_FieldMode has no value 3. */
  return !(field->mode & kUpb_FieldMode_Scalar);
}

UPB_INLINE bool upb_IsSubMessage(const upb_MiniTable_Field* field) {
  return field->descriptortype == kUpb_FieldType_Message ||
         field->descriptortype == kUpb_FieldType_Group;
}

struct upb_Decoder;
struct upb_MiniTable;

typedef const char* _upb_FieldParser(struct upb_Decoder* d, const char* ptr,
                                     upb_Message* msg, intptr_t table,
                                     uint64_t hasbits, uint64_t data);

typedef struct {
  uint64_t field_data;
  _upb_FieldParser* field_parser;
} _upb_FastTable_Entry;

typedef struct {
  const int32_t* values;  // List of values <0 or >63
  uint64_t mask;          // Bits are set for acceptable value 0 <= x < 64
  int value_count;
} upb_MiniTable_Enum;

typedef union {
  const struct upb_MiniTable* submsg;
  const upb_MiniTable_Enum* subenum;
} upb_MiniTable_Sub;

typedef enum {
  kUpb_ExtMode_NonExtendable = 0,  // Non-extendable message.
  kUpb_ExtMode_Extendable = 1,     // Normal extendable message.
  kUpb_ExtMode_IsMessageSet = 2,   // MessageSet message.
  kUpb_ExtMode_IsMessageSet_ITEM =
      3,  // MessageSet item (temporary only, see decode.c)

  // During table building we steal a bit to indicate that the message is a map
  // entry.  *Only* used during table building!
  kUpb_ExtMode_IsMapEntry = 4,
} upb_ExtMode;

/* MessageSet wire format is:
 *   message MessageSet {
 *     repeated group Item = 1 {
 *       required int32 type_id = 2;
 *       required bytes message = 3;
 *     }
 *   }
 */
typedef enum {
  _UPB_MSGSET_ITEM = 1,
  _UPB_MSGSET_TYPEID = 2,
  _UPB_MSGSET_MESSAGE = 3,
} upb_msgext_fieldnum;

struct upb_MiniTable {
  const upb_MiniTable_Sub* subs;
  const upb_MiniTable_Field* fields;
  /* Must be aligned to sizeof(void*).  Doesn't include internal members like
   * unknown fields, extension dict, pointer to msglayout, etc. */
  uint16_t size;
  uint16_t field_count;
  uint8_t ext;  // upb_ExtMode, declared as uint8_t so sizeof(ext) == 1
  uint8_t dense_below;
  uint8_t table_mask;
  uint8_t required_count;  // Required fields have the lowest hasbits.
  /* To statically initialize the tables of variable length, we need a flexible
   * array member, and we need to compile in gnu99 mode (constant initialization
   * of flexible array members is a GNU extension, not in C99 unfortunately. */
  _upb_FastTable_Entry fasttable[];
};

typedef struct {
  upb_MiniTable_Field field;
  const upb_MiniTable* extendee;
  upb_MiniTable_Sub sub; /* NULL unless submessage or proto2 enum */
} upb_MiniTable_Extension;

typedef struct {
  const upb_MiniTable** msgs;
  const upb_MiniTable_Enum** enums;
  const upb_MiniTable_Extension** exts;
  int msg_count;
  int enum_count;
  int ext_count;
} upb_MiniTable_File;

// Computes a bitmask in which the |l->required_count| lowest bits are set,
// except that we skip the lowest bit (because upb never uses hasbit 0).
//
// Sample output:
//    requiredmask(1) => 0b10 (0x2)
//    requiredmask(5) => 0b111110 (0x3e)
UPB_INLINE uint64_t upb_MiniTable_requiredmask(const upb_MiniTable* l) {
  int n = l->required_count;
  assert(0 < n && n <= 63);
  return ((1ULL << n) - 1) << 1;
}

/** upb_ExtensionRegistry *****************************************************/

/* Adds the given extension info for message type |l| and field number |num|
 * into the registry. Returns false if this message type and field number were
 * already in the map, or if memory allocation fails. */
bool _upb_extreg_add(upb_ExtensionRegistry* r,
                     const upb_MiniTable_Extension** e, size_t count);

/* Looks up the extension (if any) defined for message type |l| and field
 * number |num|.  If an extension was found, copies the field info into |*ext|
 * and returns true. Otherwise returns false. */
const upb_MiniTable_Extension* _upb_extreg_get(const upb_ExtensionRegistry* r,
                                               const upb_MiniTable* l,
                                               uint32_t num);

/** upb_Message ***************************************************************/

/* Internal members of a upb_Message that track unknown fields and/or
 * extensions. We can change this without breaking binary compatibility.  We put
 * these before the user's data.  The user's upb_Message* points after the
 * upb_Message_Internal. */

typedef struct {
  /* Total size of this structure, including the data that follows.
   * Must be aligned to 8, which is alignof(upb_Message_Extension) */
  uint32_t size;

  /* Offsets relative to the beginning of this structure.
   *
   * Unknown data grows forward from the beginning to unknown_end.
   * Extension data grows backward from size to ext_begin.
   * When the two meet, we're out of data and have to realloc.
   *
   * If we imagine that the final member of this struct is:
   *   char data[size - overhead];  // overhead =
   * sizeof(upb_Message_InternalData)
   *
   * Then we have:
   *   unknown data: data[0 .. (unknown_end - overhead)]
   *   extensions data: data[(ext_begin - overhead) .. (size - overhead)] */
  uint32_t unknown_end;
  uint32_t ext_begin;
  /* Data follows, as if there were an array:
   *   char data[size - sizeof(upb_Message_InternalData)]; */
} upb_Message_InternalData;

typedef struct {
  upb_Message_InternalData* internal;
  /* Message data follows. */
} upb_Message_Internal;

/* Maps upb_CType -> memory size. */
extern char _upb_CTypeo_size[12];

UPB_INLINE size_t upb_msg_sizeof(const upb_MiniTable* l) {
  return l->size + sizeof(upb_Message_Internal);
}

UPB_INLINE upb_Message* _upb_Message_New_inl(const upb_MiniTable* l,
                                             upb_Arena* a) {
  size_t size = upb_msg_sizeof(l);
  void* mem = upb_Arena_Malloc(a, size + sizeof(upb_Message_Internal));
  upb_Message* msg;
  if (UPB_UNLIKELY(!mem)) return NULL;
  msg = UPB_PTR_AT(mem, sizeof(upb_Message_Internal), upb_Message);
  memset(mem, 0, size);
  return msg;
}

/* Creates a new messages with the given layout on the given arena. */
upb_Message* _upb_Message_New(const upb_MiniTable* l, upb_Arena* a);

UPB_INLINE upb_Message_Internal* upb_Message_Getinternal(upb_Message* msg) {
  ptrdiff_t size = sizeof(upb_Message_Internal);
  return (upb_Message_Internal*)((char*)msg - size);
}

/* Clears the given message. */
void _upb_Message_Clear(upb_Message* msg, const upb_MiniTable* l);

/* Discards the unknown fields for this message only. */
void _upb_Message_DiscardUnknown_shallow(upb_Message* msg);

/* Adds unknown data (serialized protobuf data) to the given message.  The data
 * is copied into the message instance. */
bool _upb_Message_AddUnknown(upb_Message* msg, const char* data, size_t len,
                             upb_Arena* arena);

/** upb_Message_Extension *****************************************************/

/* The internal representation of an extension is self-describing: it contains
 * enough information that we can serialize it to binary format without needing
 * to look it up in a upb_ExtensionRegistry.
 *
 * This representation allocates 16 bytes to data on 64-bit platforms.  This is
 * rather wasteful for scalars (in the extreme case of bool, it wastes 15
 * bytes). We accept this because we expect messages to be the most common
 * extension type. */
typedef struct {
  const upb_MiniTable_Extension* ext;
  union {
    upb_StringView str;
    void* ptr;
    char scalar_data[8];
  } data;
} upb_Message_Extension;

/* Adds the given extension data to the given message. |ext| is copied into the
 * message instance. This logically replaces any previously-added extension with
 * this number */
upb_Message_Extension* _upb_Message_GetOrCreateExtension(
    upb_Message* msg, const upb_MiniTable_Extension* ext, upb_Arena* arena);

/* Returns an array of extensions for this message. Note: the array is
 * ordered in reverse relative to the order of creation. */
const upb_Message_Extension* _upb_Message_Getexts(const upb_Message* msg,
                                                  size_t* count);

/* Returns an extension for the given field number, or NULL if no extension
 * exists for this field number. */
const upb_Message_Extension* _upb_Message_Getext(
    const upb_Message* msg, const upb_MiniTable_Extension* ext);

void _upb_Message_Clearext(upb_Message* msg,
                           const upb_MiniTable_Extension* ext);

void _upb_Message_Clearext(upb_Message* msg,
                           const upb_MiniTable_Extension* ext);

/** Hasbit access *************************************************************/

UPB_INLINE bool _upb_hasbit(const upb_Message* msg, size_t idx) {
  return (*UPB_PTR_AT(msg, idx / 8, const char) & (1 << (idx % 8))) != 0;
}

UPB_INLINE void _upb_sethas(const upb_Message* msg, size_t idx) {
  (*UPB_PTR_AT(msg, idx / 8, char)) |= (char)(1 << (idx % 8));
}

UPB_INLINE void _upb_clearhas(const upb_Message* msg, size_t idx) {
  (*UPB_PTR_AT(msg, idx / 8, char)) &= (char)(~(1 << (idx % 8)));
}

UPB_INLINE size_t _upb_Message_Hasidx(const upb_MiniTable_Field* f) {
  UPB_ASSERT(f->presence > 0);
  return f->presence;
}

UPB_INLINE bool _upb_hasbit_field(const upb_Message* msg,
                                  const upb_MiniTable_Field* f) {
  return _upb_hasbit(msg, _upb_Message_Hasidx(f));
}

UPB_INLINE void _upb_sethas_field(const upb_Message* msg,
                                  const upb_MiniTable_Field* f) {
  _upb_sethas(msg, _upb_Message_Hasidx(f));
}

UPB_INLINE void _upb_clearhas_field(const upb_Message* msg,
                                    const upb_MiniTable_Field* f) {
  _upb_clearhas(msg, _upb_Message_Hasidx(f));
}

/** Oneof case access *********************************************************/

UPB_INLINE uint32_t* _upb_oneofcase(upb_Message* msg, size_t case_ofs) {
  return UPB_PTR_AT(msg, case_ofs, uint32_t);
}

UPB_INLINE uint32_t _upb_getoneofcase(const void* msg, size_t case_ofs) {
  return *UPB_PTR_AT(msg, case_ofs, uint32_t);
}

UPB_INLINE size_t _upb_oneofcase_ofs(const upb_MiniTable_Field* f) {
  UPB_ASSERT(f->presence < 0);
  return ~(ptrdiff_t)f->presence;
}

UPB_INLINE uint32_t* _upb_oneofcase_field(upb_Message* msg,
                                          const upb_MiniTable_Field* f) {
  return _upb_oneofcase(msg, _upb_oneofcase_ofs(f));
}

UPB_INLINE uint32_t _upb_getoneofcase_field(const upb_Message* msg,
                                            const upb_MiniTable_Field* f) {
  return _upb_getoneofcase(msg, _upb_oneofcase_ofs(f));
}

UPB_INLINE bool _upb_has_submsg_nohasbit(const upb_Message* msg, size_t ofs) {
  return *UPB_PTR_AT(msg, ofs, const upb_Message*) != NULL;
}

/** upb_Array *****************************************************************/

/* Our internal representation for repeated fields.  */
typedef struct {
  uintptr_t data; /* Tagged ptr: low 3 bits of ptr are lg2(elem size). */
  size_t len;     /* Measured in elements. */
  size_t size;    /* Measured in elements. */
  uint64_t junk;
} upb_Array;

UPB_INLINE const void* _upb_array_constptr(const upb_Array* arr) {
  UPB_ASSERT((arr->data & 7) <= 4);
  return (void*)(arr->data & ~(uintptr_t)7);
}

UPB_INLINE uintptr_t _upb_array_tagptr(void* ptr, int elem_size_lg2) {
  UPB_ASSERT(elem_size_lg2 <= 4);
  return (uintptr_t)ptr | elem_size_lg2;
}

UPB_INLINE void* _upb_array_ptr(upb_Array* arr) {
  return (void*)_upb_array_constptr(arr);
}

UPB_INLINE uintptr_t _upb_tag_arrptr(void* ptr, int elem_size_lg2) {
  UPB_ASSERT(elem_size_lg2 <= 4);
  UPB_ASSERT(((uintptr_t)ptr & 7) == 0);
  return (uintptr_t)ptr | (unsigned)elem_size_lg2;
}

UPB_INLINE upb_Array* _upb_Array_New(upb_Arena* a, size_t init_size,
                                     int elem_size_lg2) {
  const size_t arr_size = UPB_ALIGN_UP(sizeof(upb_Array), 8);
  const size_t bytes = sizeof(upb_Array) + (init_size << elem_size_lg2);
  upb_Array* arr = (upb_Array*)upb_Arena_Malloc(a, bytes);
  if (!arr) return NULL;
  arr->data = _upb_tag_arrptr(UPB_PTR_AT(arr, arr_size, void), elem_size_lg2);
  arr->len = 0;
  arr->size = init_size;
  return arr;
}

/* Resizes the capacity of the array to be at least min_size. */
bool _upb_array_realloc(upb_Array* arr, size_t min_size, upb_Arena* arena);

/* Fallback functions for when the accessors require a resize. */
void* _upb_Array_Resize_fallback(upb_Array** arr_ptr, size_t size,
                                 int elem_size_lg2, upb_Arena* arena);
bool _upb_Array_Append_fallback(upb_Array** arr_ptr, const void* value,
                                int elem_size_lg2, upb_Arena* arena);

UPB_INLINE bool _upb_array_reserve(upb_Array* arr, size_t size,
                                   upb_Arena* arena) {
  if (arr->size < size) return _upb_array_realloc(arr, size, arena);
  return true;
}

UPB_INLINE bool _upb_Array_Resize(upb_Array* arr, size_t size,
                                  upb_Arena* arena) {
  if (!_upb_array_reserve(arr, size, arena)) return false;
  arr->len = size;
  return true;
}

UPB_INLINE void _upb_array_detach(const void* msg, size_t ofs) {
  *UPB_PTR_AT(msg, ofs, upb_Array*) = NULL;
}

UPB_INLINE const void* _upb_array_accessor(const void* msg, size_t ofs,
                                           size_t* size) {
  const upb_Array* arr = *UPB_PTR_AT(msg, ofs, const upb_Array*);
  if (arr) {
    if (size) *size = arr->len;
    return _upb_array_constptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}

UPB_INLINE void* _upb_array_mutable_accessor(void* msg, size_t ofs,
                                             size_t* size) {
  upb_Array* arr = *UPB_PTR_AT(msg, ofs, upb_Array*);
  if (arr) {
    if (size) *size = arr->len;
    return _upb_array_ptr(arr);
  } else {
    if (size) *size = 0;
    return NULL;
  }
}

UPB_INLINE void* _upb_Array_Resize_accessor2(void* msg, size_t ofs, size_t size,
                                             int elem_size_lg2,
                                             upb_Arena* arena) {
  upb_Array** arr_ptr = UPB_PTR_AT(msg, ofs, upb_Array*);
  upb_Array* arr = *arr_ptr;
  if (!arr || arr->size < size) {
    return _upb_Array_Resize_fallback(arr_ptr, size, elem_size_lg2, arena);
  }
  arr->len = size;
  return _upb_array_ptr(arr);
}

UPB_INLINE bool _upb_Array_Append_accessor2(void* msg, size_t ofs,
                                            int elem_size_lg2,
                                            const void* value,
                                            upb_Arena* arena) {
  upb_Array** arr_ptr = UPB_PTR_AT(msg, ofs, upb_Array*);
  size_t elem_size = 1 << elem_size_lg2;
  upb_Array* arr = *arr_ptr;
  void* ptr;
  if (!arr || arr->len == arr->size) {
    return _upb_Array_Append_fallback(arr_ptr, value, elem_size_lg2, arena);
  }
  ptr = _upb_array_ptr(arr);
  memcpy(UPB_PTR_AT(ptr, arr->len * elem_size, char), value, elem_size);
  arr->len++;
  return true;
}

/* Used by old generated code, remove once all code has been regenerated. */
UPB_INLINE int _upb_sizelg2(upb_CType type) {
  switch (type) {
    case kUpb_CType_Bool:
      return 0;
    case kUpb_CType_Float:
    case kUpb_CType_Int32:
    case kUpb_CType_UInt32:
    case kUpb_CType_Enum:
      return 2;
    case kUpb_CType_Message:
      return UPB_SIZE(2, 3);
    case kUpb_CType_Double:
    case kUpb_CType_Int64:
    case kUpb_CType_UInt64:
      return 3;
    case kUpb_CType_String:
    case kUpb_CType_Bytes:
      return UPB_SIZE(3, 4);
  }
  UPB_UNREACHABLE();
}
UPB_INLINE void* _upb_Array_Resize_accessor(void* msg, size_t ofs, size_t size,
                                            upb_CType type, upb_Arena* arena) {
  return _upb_Array_Resize_accessor2(msg, ofs, size, _upb_sizelg2(type), arena);
}
UPB_INLINE bool _upb_Array_Append_accessor(void* msg, size_t ofs,
                                           size_t elem_size, upb_CType type,
                                           const void* value,
                                           upb_Arena* arena) {
  (void)elem_size;
  return _upb_Array_Append_accessor2(msg, ofs, _upb_sizelg2(type), value,
                                     arena);
}

/** upb_Map *******************************************************************/

/* Right now we use strmaps for everything.  We'll likely want to use
 * integer-specific maps for integer-keyed maps.*/
typedef struct {
  /* Size of key and val, based on the map type.  Strings are represented as '0'
   * because they must be handled specially. */
  char key_size;
  char val_size;

  upb_strtable table;
} upb_Map;

/* Map entries aren't actually stored, they are only used during parsing.  For
 * parsing, it helps a lot if all map entry messages have the same layout.
 * The compiler and def.c must ensure that all map entries have this layout. */
typedef struct {
  upb_Message_Internal internal;
  union {
    upb_StringView str; /* For str/bytes. */
    upb_value val;      /* For all other types. */
  } k;
  union {
    upb_StringView str; /* For str/bytes. */
    upb_value val;      /* For all other types. */
  } v;
} upb_MapEntry;

/* Creates a new map on the given arena with this key/value type. */
upb_Map* _upb_Map_New(upb_Arena* a, size_t key_size, size_t value_size);

/* Converting between internal table representation and user values.
 *
 * _upb_map_tokey() and _upb_map_fromkey() are inverses.
 * _upb_map_tovalue() and _upb_map_fromvalue() are inverses.
 *
 * These functions account for the fact that strings are treated differently
 * from other types when stored in a map.
 */

UPB_INLINE upb_StringView _upb_map_tokey(const void* key, size_t size) {
  if (size == UPB_MAPTYPE_STRING) {
    return *(upb_StringView*)key;
  } else {
    return upb_StringView_FromDataAndSize((const char*)key, size);
  }
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

/* Map operations, shared by reflection and generated code. */

UPB_INLINE size_t _upb_Map_Size(const upb_Map* map) {
  return map->table.t.count;
}

UPB_INLINE bool _upb_Map_Get(const upb_Map* map, const void* key,
                             size_t key_size, void* val, size_t val_size) {
  upb_value tabval;
  upb_StringView k = _upb_map_tokey(key, key_size);
  bool ret = upb_strtable_lookup2(&map->table, k.data, k.size, &tabval);
  if (ret && val) {
    _upb_map_fromvalue(tabval, val, val_size);
  }
  return ret;
}

UPB_INLINE void* _upb_map_next(const upb_Map* map, size_t* iter) {
  upb_strtable_iter it;
  it.t = &map->table;
  it.index = *iter;
  upb_strtable_next(&it);
  *iter = it.index;
  if (upb_strtable_done(&it)) return NULL;
  return (void*)str_tabent(&it);
}

typedef enum {
  // LINT.IfChange
  _kUpb_MapInsertStatus_Inserted = 0,
  _kUpb_MapInsertStatus_Replaced = 1,
  _kUpb_MapInsertStatus_OutOfMemory = 2,
  // LINT.ThenChange(//depot/google3/third_party/upb/upb/map.h)
} _upb_MapInsertStatus;

UPB_INLINE _upb_MapInsertStatus _upb_Map_Insert(upb_Map* map, const void* key,
                                                size_t key_size, void* val,
                                                size_t val_size, upb_Arena* a) {
  upb_StringView strkey = _upb_map_tokey(key, key_size);
  upb_value tabval = {0};
  if (!_upb_map_tovalue(val, val_size, &tabval, a)) {
    return _kUpb_MapInsertStatus_OutOfMemory;
  }

  /* TODO(haberman): add overwrite operation to minimize number of lookups. */
  bool removed =
      upb_strtable_remove2(&map->table, strkey.data, strkey.size, NULL);
  if (!upb_strtable_insert(&map->table, strkey.data, strkey.size, tabval, a)) {
    return _kUpb_MapInsertStatus_OutOfMemory;
  }
  return removed ? _kUpb_MapInsertStatus_Replaced
                 : _kUpb_MapInsertStatus_Inserted;
}

UPB_INLINE bool _upb_Map_Delete(upb_Map* map, const void* key,
                                size_t key_size) {
  upb_StringView k = _upb_map_tokey(key, key_size);
  return upb_strtable_remove2(&map->table, k.data, k.size, NULL);
}

UPB_INLINE void _upb_Map_Clear(upb_Map* map) {
  upb_strtable_clear(&map->table);
}

/* Message map operations, these get the map from the message first. */

UPB_INLINE size_t _upb_msg_map_size(const upb_Message* msg, size_t ofs) {
  upb_Map* map = *UPB_PTR_AT(msg, ofs, upb_Map*);
  return map ? _upb_Map_Size(map) : 0;
}

UPB_INLINE bool _upb_msg_map_get(const upb_Message* msg, size_t ofs,
                                 const void* key, size_t key_size, void* val,
                                 size_t val_size) {
  upb_Map* map = *UPB_PTR_AT(msg, ofs, upb_Map*);
  if (!map) return false;
  return _upb_Map_Get(map, key, key_size, val, val_size);
}

UPB_INLINE void* _upb_msg_map_next(const upb_Message* msg, size_t ofs,
                                   size_t* iter) {
  upb_Map* map = *UPB_PTR_AT(msg, ofs, upb_Map*);
  if (!map) return NULL;
  return _upb_map_next(map, iter);
}

UPB_INLINE bool _upb_msg_map_set(upb_Message* msg, size_t ofs, const void* key,
                                 size_t key_size, void* val, size_t val_size,
                                 upb_Arena* arena) {
  upb_Map** map = UPB_PTR_AT(msg, ofs, upb_Map*);
  if (!*map) {
    *map = _upb_Map_New(arena, key_size, val_size);
  }
  return _upb_Map_Insert(*map, key, key_size, val, val_size, arena) !=
         _kUpb_MapInsertStatus_OutOfMemory;
}

UPB_INLINE bool _upb_msg_map_delete(upb_Message* msg, size_t ofs,
                                    const void* key, size_t key_size) {
  upb_Map* map = *UPB_PTR_AT(msg, ofs, upb_Map*);
  if (!map) return false;
  return _upb_Map_Delete(map, key, key_size);
}

UPB_INLINE void _upb_msg_map_clear(upb_Message* msg, size_t ofs) {
  upb_Map* map = *UPB_PTR_AT(msg, ofs, upb_Map*);
  if (!map) return;
  _upb_Map_Clear(map);
}

/* Accessing map key/value from a pointer, used by generated code only. */

UPB_INLINE void _upb_msg_map_key(const void* msg, void* key, size_t size) {
  const upb_tabent* ent = (const upb_tabent*)msg;
  uint32_t u32len;
  upb_StringView k;
  k.data = upb_tabstr(ent->key, &u32len);
  k.size = u32len;
  _upb_map_fromkey(k, key, size);
}

UPB_INLINE void _upb_msg_map_value(const void* msg, void* val, size_t size) {
  const upb_tabent* ent = (const upb_tabent*)msg;
  upb_value v = {ent->val.val};
  _upb_map_fromvalue(v, val, size);
}

UPB_INLINE void _upb_msg_map_set_value(void* msg, const void* val,
                                       size_t size) {
  upb_tabent* ent = (upb_tabent*)msg;
  /* This is like _upb_map_tovalue() except the entry already exists so we can
   * reuse the allocated upb_StringView for string fields. */
  if (size == UPB_MAPTYPE_STRING) {
    upb_StringView* strp = (upb_StringView*)(uintptr_t)ent->val.val;
    memcpy(strp, val, sizeof(*strp));
  } else {
    memcpy(&ent->val.val, val, size);
  }
}

/** _upb_mapsorter ************************************************************/

/* _upb_mapsorter sorts maps and provides ordered iteration over the entries.
 * Since maps can be recursive (map values can be messages which contain other
 * maps). _upb_mapsorter can contain a stack of maps. */

typedef struct {
  upb_tabent const** entries;
  int size;
  int cap;
} _upb_mapsorter;

typedef struct {
  int start;
  int pos;
  int end;
} _upb_sortedmap;

UPB_INLINE void _upb_mapsorter_init(_upb_mapsorter* s) {
  s->entries = NULL;
  s->size = 0;
  s->cap = 0;
}

UPB_INLINE void _upb_mapsorter_destroy(_upb_mapsorter* s) {
  if (s->entries) free(s->entries);
}

bool _upb_mapsorter_pushmap(_upb_mapsorter* s, upb_FieldType key_type,
                            const upb_Map* map, _upb_sortedmap* sorted);

UPB_INLINE void _upb_mapsorter_popmap(_upb_mapsorter* s,
                                      _upb_sortedmap* sorted) {
  s->size = sorted->start;
}

UPB_INLINE bool _upb_sortedmap_next(_upb_mapsorter* s, const upb_Map* map,
                                    _upb_sortedmap* sorted, upb_MapEntry* ent) {
  if (sorted->pos == sorted->end) return false;
  const upb_tabent* tabent = s->entries[sorted->pos++];
  upb_StringView key = upb_tabstrview(tabent->key);
  _upb_map_fromkey(key, &ent->k, map->key_size);
  upb_value val = {tabent->val.val};
  _upb_map_fromvalue(val, &ent->v, map->val_size);
  return true;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port_undef.inc"

#endif /* UPB_MSG_INT_H_ */
