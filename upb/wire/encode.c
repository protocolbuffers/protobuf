// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// We encode backwards, to avoid pre-computing lengths (one-pass encode).

#include "upb/wire/encode.h"

#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "upb/base/descriptor_constants.h"
#include "upb/base/internal/endian.h"
#include "upb/base/string_view.h"
#include "upb/hash/common.h"
#include "upb/hash/int_table.h"
#include "upb/hash/str_table.h"
#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include "upb/message/internal/accessors.h"
#include "upb/message/internal/array.h"
#include "upb/message/internal/extension.h"
#include "upb/message/internal/map.h"
#include "upb/message/internal/map_entry.h"
#include "upb/message/internal/map_sorter.h"
#include "upb/message/internal/message.h"
#include "upb/message/internal/tagged_ptr.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/message/tagged_ptr.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/internal/sub.h"
#include "upb/mini_table/message.h"
#include "upb/wire/internal/constants.h"
#include "upb/wire/types.h"

// Must be last.
#include "upb/port/def.inc"

// Returns the MiniTable corresponding to a given MiniTableField
// from an array of MiniTableSubs.
static const upb_MiniTable* _upb_Encoder_GetSubMiniTable(
    const upb_MiniTableSubInternal* subs, const upb_MiniTableField* field) {
  return *subs[field->UPB_PRIVATE(submsg_index)].UPB_PRIVATE(submsg);
}

#define UPB_PB_VARINT_MAX_LEN 10

UPB_NOINLINE
static size_t encode_varint64(uint64_t val, char* buf) {
  size_t i = 0;
  do {
    uint8_t byte = val & 0x7fU;
    val >>= 7;
    if (val) byte |= 0x80U;
    buf[i++] = byte;
  } while (val);
  return i;
}

static uint32_t encode_zz32(int32_t n) {
  return ((uint32_t)n << 1) ^ (n >> 31);
}
static uint64_t encode_zz64(int64_t n) {
  return ((uint64_t)n << 1) ^ (n >> 63);
}

typedef struct {
  upb_EncodeStatus status;
  jmp_buf err;
  upb_Arena* arena;
  char *buf, *ptr, *limit;
  int options;
  int depth;
  _upb_mapsorter sorter;
} upb_encstate;

static size_t upb_roundup_pow2(size_t bytes) {
  size_t ret = 128;
  while (ret < bytes) {
    ret *= 2;
  }
  return ret;
}

UPB_NORETURN static void encode_err(upb_encstate* e, upb_EncodeStatus s) {
  UPB_ASSERT(s != kUpb_EncodeStatus_Ok);
  e->status = s;
  UPB_LONGJMP(e->err, 1);
}

UPB_NOINLINE
static void encode_growbuffer(upb_encstate* e, size_t bytes) {
  size_t old_size = e->limit - e->buf;
  size_t needed_size = bytes + (e->limit - e->ptr);
  size_t new_size = upb_roundup_pow2(needed_size);
  char* new_buf = upb_Arena_Realloc(e->arena, e->buf, old_size, new_size);

  if (!new_buf) encode_err(e, kUpb_EncodeStatus_OutOfMemory);

  // We want previous data at the end, realloc() put it at the beginning.
  // TODO: This is somewhat inefficient since we are copying twice.
  // Maybe create a realloc() that copies to the end of the new buffer?
  if (old_size > 0) {
    memmove(new_buf + new_size - old_size, new_buf, old_size);
  }

  e->buf = new_buf;
  e->limit = new_buf + new_size;
  e->ptr = new_buf + new_size - needed_size;
}

/* Call to ensure that at least "bytes" bytes are available for writing at
 * e->ptr.  Returns false if the bytes could not be allocated. */
UPB_FORCEINLINE
void encode_reserve(upb_encstate* e, size_t bytes) {
  if ((size_t)(e->ptr - e->buf) < bytes) {
    encode_growbuffer(e, bytes);
    return;
  }

  e->ptr -= bytes;
}

/* Writes the given bytes to the buffer, handling reserve/advance. */
static void encode_bytes(upb_encstate* e, const void* data, size_t len) {
  if (len == 0) return; /* memcpy() with zero size is UB */
  encode_reserve(e, len);
  memcpy(e->ptr, data, len);
}

static void encode_fixed64(upb_encstate* e, uint64_t val) {
  val = upb_BigEndian64(val);
  encode_bytes(e, &val, sizeof(uint64_t));
}

static void encode_fixed32(upb_encstate* e, uint32_t val) {
  val = upb_BigEndian32(val);
  encode_bytes(e, &val, sizeof(uint32_t));
}

UPB_NOINLINE
static void encode_longvarint(upb_encstate* e, uint64_t val) {
  size_t len;
  char* start;

  encode_reserve(e, UPB_PB_VARINT_MAX_LEN);
  len = encode_varint64(val, e->ptr);
  start = e->ptr + UPB_PB_VARINT_MAX_LEN - len;
  memmove(start, e->ptr, len);
  e->ptr = start;
}

UPB_FORCEINLINE
void encode_varint(upb_encstate* e, uint64_t val) {
  if (val < 128 && e->ptr != e->buf) {
    --e->ptr;
    *e->ptr = val;
  } else {
    encode_longvarint(e, val);
  }
}

static void encode_double(upb_encstate* e, double d) {
  uint64_t u64;
  UPB_ASSERT(sizeof(double) == sizeof(uint64_t));
  memcpy(&u64, &d, sizeof(uint64_t));
  encode_fixed64(e, u64);
}

static void encode_float(upb_encstate* e, float d) {
  uint32_t u32;
  UPB_ASSERT(sizeof(float) == sizeof(uint32_t));
  memcpy(&u32, &d, sizeof(uint32_t));
  encode_fixed32(e, u32);
}

static void encode_tag(upb_encstate* e, uint32_t field_number,
                       uint8_t wire_type) {
  encode_varint(e, (field_number << 3) | wire_type);
}

static void encode_fixedarray(upb_encstate* e, const upb_Array* arr,
                              size_t elem_size, uint32_t tag) {
  size_t bytes = upb_Array_Size(arr) * elem_size;
  const char* data = upb_Array_DataPtr(arr);
  const char* ptr = data + bytes - elem_size;

  if (tag || !upb_IsLittleEndian()) {
    while (true) {
      if (elem_size == 4) {
        uint32_t val;
        memcpy(&val, ptr, sizeof(val));
        val = upb_BigEndian32(val);
        encode_bytes(e, &val, elem_size);
      } else {
        UPB_ASSERT(elem_size == 8);
        uint64_t val;
        memcpy(&val, ptr, sizeof(val));
        val = upb_BigEndian64(val);
        encode_bytes(e, &val, elem_size);
      }

      if (tag) encode_varint(e, tag);
      if (ptr == data) break;
      ptr -= elem_size;
    }
  } else {
    encode_bytes(e, data, bytes);
  }
}

static void encode_message(upb_encstate* e, const upb_Message* msg,
                           const upb_MiniTable* m, size_t* size);

static void encode_TaggedMessagePtr(upb_encstate* e,
                                    upb_TaggedMessagePtr tagged,
                                    const upb_MiniTable* m, size_t* size) {
  if (upb_TaggedMessagePtr_IsEmpty(tagged)) {
    m = UPB_PRIVATE(_upb_MiniTable_Empty)();
  }
  encode_message(e, UPB_PRIVATE(_upb_TaggedMessagePtr_GetMessage)(tagged), m,
                 size);
}

static void encode_scalar(upb_encstate* e, const void* _field_mem,
                          const upb_MiniTableSubInternal* subs,
                          const upb_MiniTableField* f) {
  const char* field_mem = _field_mem;
  int wire_type;

#define CASE(ctype, type, wtype, encodeval) \
  {                                         \
    ctype val = *(ctype*)field_mem;         \
    encode_##type(e, encodeval);            \
    wire_type = wtype;                      \
    break;                                  \
  }

  switch (f->UPB_PRIVATE(descriptortype)) {
    case kUpb_FieldType_Double:
      CASE(double, double, kUpb_WireType_64Bit, val);
    case kUpb_FieldType_Float:
      CASE(float, float, kUpb_WireType_32Bit, val);
    case kUpb_FieldType_Int64:
    case kUpb_FieldType_UInt64:
      CASE(uint64_t, varint, kUpb_WireType_Varint, val);
    case kUpb_FieldType_UInt32:
      CASE(uint32_t, varint, kUpb_WireType_Varint, val);
    case kUpb_FieldType_Int32:
    case kUpb_FieldType_Enum:
      CASE(int32_t, varint, kUpb_WireType_Varint, (int64_t)val);
    case kUpb_FieldType_SFixed64:
    case kUpb_FieldType_Fixed64:
      CASE(uint64_t, fixed64, kUpb_WireType_64Bit, val);
    case kUpb_FieldType_Fixed32:
    case kUpb_FieldType_SFixed32:
      CASE(uint32_t, fixed32, kUpb_WireType_32Bit, val);
    case kUpb_FieldType_Bool:
      CASE(bool, varint, kUpb_WireType_Varint, val);
    case kUpb_FieldType_SInt32:
      CASE(int32_t, varint, kUpb_WireType_Varint, encode_zz32(val));
    case kUpb_FieldType_SInt64:
      CASE(int64_t, varint, kUpb_WireType_Varint, encode_zz64(val));
    case kUpb_FieldType_String:
    case kUpb_FieldType_Bytes: {
      upb_StringView view = *(upb_StringView*)field_mem;
      encode_bytes(e, view.data, view.size);
      encode_varint(e, view.size);
      wire_type = kUpb_WireType_Delimited;
      break;
    }
    case kUpb_FieldType_Group: {
      size_t size;
      upb_TaggedMessagePtr submsg = *(upb_TaggedMessagePtr*)field_mem;
      const upb_MiniTable* subm = _upb_Encoder_GetSubMiniTable(subs, f);
      if (submsg == 0) {
        return;
      }
      if (--e->depth == 0) encode_err(e, kUpb_EncodeStatus_MaxDepthExceeded);
      encode_tag(e, upb_MiniTableField_Number(f), kUpb_WireType_EndGroup);
      encode_TaggedMessagePtr(e, submsg, subm, &size);
      wire_type = kUpb_WireType_StartGroup;
      e->depth++;
      break;
    }
    case kUpb_FieldType_Message: {
      size_t size;
      upb_TaggedMessagePtr submsg = *(upb_TaggedMessagePtr*)field_mem;
      const upb_MiniTable* subm = _upb_Encoder_GetSubMiniTable(subs, f);
      if (submsg == 0) {
        return;
      }
      if (--e->depth == 0) encode_err(e, kUpb_EncodeStatus_MaxDepthExceeded);
      encode_TaggedMessagePtr(e, submsg, subm, &size);
      encode_varint(e, size);
      wire_type = kUpb_WireType_Delimited;
      e->depth++;
      break;
    }
    default:
      UPB_UNREACHABLE();
  }
#undef CASE

  encode_tag(e, upb_MiniTableField_Number(f), wire_type);
}

static void encode_array(upb_encstate* e, const upb_Message* msg,
                         const upb_MiniTableSubInternal* subs,
                         const upb_MiniTableField* f) {
  const upb_Array* arr = *UPB_PTR_AT(msg, f->UPB_PRIVATE(offset), upb_Array*);
  bool packed = upb_MiniTableField_IsPacked(f);
  size_t pre_len = e->limit - e->ptr;

  if (arr == NULL || upb_Array_Size(arr) == 0) {
    return;
  }

#define VARINT_CASE(ctype, encode)                                         \
  {                                                                        \
    const ctype* start = upb_Array_DataPtr(arr);                           \
    const ctype* ptr = start + upb_Array_Size(arr);                        \
    uint32_t tag =                                                         \
        packed ? 0 : (f->UPB_PRIVATE(number) << 3) | kUpb_WireType_Varint; \
    do {                                                                   \
      ptr--;                                                               \
      encode_varint(e, encode);                                            \
      if (tag) encode_varint(e, tag);                                      \
    } while (ptr != start);                                                \
  }                                                                        \
  break;

#define TAG(wire_type) (packed ? 0 : (f->UPB_PRIVATE(number) << 3 | wire_type))

  switch (f->UPB_PRIVATE(descriptortype)) {
    case kUpb_FieldType_Double:
      encode_fixedarray(e, arr, sizeof(double), TAG(kUpb_WireType_64Bit));
      break;
    case kUpb_FieldType_Float:
      encode_fixedarray(e, arr, sizeof(float), TAG(kUpb_WireType_32Bit));
      break;
    case kUpb_FieldType_SFixed64:
    case kUpb_FieldType_Fixed64:
      encode_fixedarray(e, arr, sizeof(uint64_t), TAG(kUpb_WireType_64Bit));
      break;
    case kUpb_FieldType_Fixed32:
    case kUpb_FieldType_SFixed32:
      encode_fixedarray(e, arr, sizeof(uint32_t), TAG(kUpb_WireType_32Bit));
      break;
    case kUpb_FieldType_Int64:
    case kUpb_FieldType_UInt64:
      VARINT_CASE(uint64_t, *ptr);
    case kUpb_FieldType_UInt32:
      VARINT_CASE(uint32_t, *ptr);
    case kUpb_FieldType_Int32:
    case kUpb_FieldType_Enum:
      VARINT_CASE(int32_t, (int64_t)*ptr);
    case kUpb_FieldType_Bool:
      VARINT_CASE(bool, *ptr);
    case kUpb_FieldType_SInt32:
      VARINT_CASE(int32_t, encode_zz32(*ptr));
    case kUpb_FieldType_SInt64:
      VARINT_CASE(int64_t, encode_zz64(*ptr));
    case kUpb_FieldType_String:
    case kUpb_FieldType_Bytes: {
      const upb_StringView* start = upb_Array_DataPtr(arr);
      const upb_StringView* ptr = start + upb_Array_Size(arr);
      do {
        ptr--;
        encode_bytes(e, ptr->data, ptr->size);
        encode_varint(e, ptr->size);
        encode_tag(e, upb_MiniTableField_Number(f), kUpb_WireType_Delimited);
      } while (ptr != start);
      return;
    }
    case kUpb_FieldType_Group: {
      const upb_TaggedMessagePtr* start = upb_Array_DataPtr(arr);
      const upb_TaggedMessagePtr* ptr = start + upb_Array_Size(arr);
      const upb_MiniTable* subm = _upb_Encoder_GetSubMiniTable(subs, f);
      if (--e->depth == 0) encode_err(e, kUpb_EncodeStatus_MaxDepthExceeded);
      do {
        size_t size;
        ptr--;
        encode_tag(e, upb_MiniTableField_Number(f), kUpb_WireType_EndGroup);
        encode_TaggedMessagePtr(e, *ptr, subm, &size);
        encode_tag(e, upb_MiniTableField_Number(f), kUpb_WireType_StartGroup);
      } while (ptr != start);
      e->depth++;
      return;
    }
    case kUpb_FieldType_Message: {
      const upb_TaggedMessagePtr* start = upb_Array_DataPtr(arr);
      const upb_TaggedMessagePtr* ptr = start + upb_Array_Size(arr);
      const upb_MiniTable* subm = _upb_Encoder_GetSubMiniTable(subs, f);
      if (--e->depth == 0) encode_err(e, kUpb_EncodeStatus_MaxDepthExceeded);
      do {
        size_t size;
        ptr--;
        encode_TaggedMessagePtr(e, *ptr, subm, &size);
        encode_varint(e, size);
        encode_tag(e, upb_MiniTableField_Number(f), kUpb_WireType_Delimited);
      } while (ptr != start);
      e->depth++;
      return;
    }
  }
#undef VARINT_CASE

  if (packed) {
    encode_varint(e, e->limit - e->ptr - pre_len);
    encode_tag(e, upb_MiniTableField_Number(f), kUpb_WireType_Delimited);
  }
}

static void encode_mapentry(upb_encstate* e, uint32_t number,
                            const upb_MiniTable* layout,
                            const upb_MapEntry* ent) {
  const upb_MiniTableField* key_field = upb_MiniTable_MapKey(layout);
  const upb_MiniTableField* val_field = upb_MiniTable_MapValue(layout);
  size_t pre_len = e->limit - e->ptr;
  size_t size;
  encode_scalar(e, &ent->v, layout->UPB_PRIVATE(subs), val_field);
  encode_scalar(e, &ent->k, layout->UPB_PRIVATE(subs), key_field);
  size = (e->limit - e->ptr) - pre_len;
  encode_varint(e, size);
  encode_tag(e, number, kUpb_WireType_Delimited);
}

static void encode_map(upb_encstate* e, const upb_Message* msg,
                       const upb_MiniTableSubInternal* subs,
                       const upb_MiniTableField* f) {
  const upb_Map* map = *UPB_PTR_AT(msg, f->UPB_PRIVATE(offset), const upb_Map*);
  const upb_MiniTable* layout = _upb_Encoder_GetSubMiniTable(subs, f);
  UPB_ASSERT(upb_MiniTable_FieldCount(layout) == 2);

  if (!map || !upb_Map_Size(map)) return;

  if (e->options & kUpb_EncodeOption_Deterministic) {
    if (!map->UPB_PRIVATE(is_strtable)) {
      // For inttable, first encode the array part, then sort the table entries.
      intptr_t iter = UPB_INTTABLE_BEGIN;
      while ((size_t)++iter < map->t.inttable.array_size) {
        upb_value value = map->t.inttable.array[iter];
        if (upb_inttable_arrhas(&map->t.inttable, iter)) {
          upb_MapEntry ent;
          memcpy(&ent.k, &iter, sizeof(iter));
          _upb_map_fromvalue(value, &ent.v, map->val_size);
          encode_mapentry(e, upb_MiniTableField_Number(f), layout, &ent);
        }
      }
    }
    _upb_sortedmap sorted;
    _upb_mapsorter_pushmap(
        &e->sorter, layout->UPB_PRIVATE(fields)[0].UPB_PRIVATE(descriptortype),
        map, &sorted);
    upb_MapEntry ent;
    while (_upb_sortedmap_next(&e->sorter, map, &sorted, &ent)) {
      encode_mapentry(e, upb_MiniTableField_Number(f), layout, &ent);
    }
    _upb_mapsorter_popmap(&e->sorter, &sorted);
  } else {
    upb_value val;
    if (map->UPB_PRIVATE(is_strtable)) {
      intptr_t iter = UPB_STRTABLE_BEGIN;
      upb_StringView strkey;
      while (upb_strtable_next2(&map->t.strtable, &strkey, &val, &iter)) {
        upb_MapEntry ent;
        _upb_map_fromkey(strkey, &ent.k, map->key_size);
        _upb_map_fromvalue(val, &ent.v, map->val_size);
        encode_mapentry(e, upb_MiniTableField_Number(f), layout, &ent);
      }
    } else {
      intptr_t iter = UPB_INTTABLE_BEGIN;
      uintptr_t intkey = 0;
      while (upb_inttable_next(&map->t.inttable, &intkey, &val, &iter)) {
        upb_MapEntry ent;
        memcpy(&ent.k, &intkey, map->key_size);
        _upb_map_fromvalue(val, &ent.v, map->val_size);
        encode_mapentry(e, upb_MiniTableField_Number(f), layout, &ent);
      }
    }
  }
}

static bool encode_shouldencode(upb_encstate* e, const upb_Message* msg,
                                const upb_MiniTableField* f) {
  if (f->presence == 0) {
    // Proto3 presence or map/array.
    const void* mem = UPB_PTR_AT(msg, f->UPB_PRIVATE(offset), void);
    switch (UPB_PRIVATE(_upb_MiniTableField_GetRep)(f)) {
      case kUpb_FieldRep_1Byte: {
        char ch;
        memcpy(&ch, mem, 1);
        return ch != 0;
      }
      case kUpb_FieldRep_4Byte: {
        uint32_t u32;
        memcpy(&u32, mem, 4);
        return u32 != 0;
      }
      case kUpb_FieldRep_8Byte: {
        uint64_t u64;
        memcpy(&u64, mem, 8);
        return u64 != 0;
      }
      case kUpb_FieldRep_StringView: {
        const upb_StringView* str = (const upb_StringView*)mem;
        return str->size != 0;
      }
      default:
        UPB_UNREACHABLE();
    }
  } else if (UPB_PRIVATE(_upb_MiniTableField_HasHasbit)(f)) {
    // Proto2 presence: hasbit.
    return UPB_PRIVATE(_upb_Message_GetHasbit)(msg, f);
  } else {
    // Field is in a oneof.
    return UPB_PRIVATE(_upb_Message_GetOneofCase)(msg, f) ==
           upb_MiniTableField_Number(f);
  }
}

static void encode_field(upb_encstate* e, const upb_Message* msg,
                         const upb_MiniTableSubInternal* subs,
                         const upb_MiniTableField* field) {
  switch (UPB_PRIVATE(_upb_MiniTableField_Mode)(field)) {
    case kUpb_FieldMode_Array:
      encode_array(e, msg, subs, field);
      break;
    case kUpb_FieldMode_Map:
      encode_map(e, msg, subs, field);
      break;
    case kUpb_FieldMode_Scalar:
      encode_scalar(e, UPB_PTR_AT(msg, field->UPB_PRIVATE(offset), void), subs,
                    field);
      break;
    default:
      UPB_UNREACHABLE();
  }
}

static void encode_msgset_item(upb_encstate* e,
                               const upb_MiniTableExtension* ext,
                               const upb_MessageValue ext_val) {
  size_t size;
  encode_tag(e, kUpb_MsgSet_Item, kUpb_WireType_EndGroup);
  encode_message(e, ext_val.msg_val, upb_MiniTableExtension_GetSubMessage(ext),
                 &size);
  encode_varint(e, size);
  encode_tag(e, kUpb_MsgSet_Message, kUpb_WireType_Delimited);
  encode_varint(e, upb_MiniTableExtension_Number(ext));
  encode_tag(e, kUpb_MsgSet_TypeId, kUpb_WireType_Varint);
  encode_tag(e, kUpb_MsgSet_Item, kUpb_WireType_StartGroup);
}

static void encode_ext(upb_encstate* e, const upb_MiniTableExtension* ext,
                       upb_MessageValue ext_val, bool is_message_set) {
  if (UPB_UNLIKELY(is_message_set)) {
    encode_msgset_item(e, ext, ext_val);
  } else {
    upb_MiniTableSubInternal sub;
    if (upb_MiniTableField_IsSubMessage(&ext->UPB_PRIVATE(field))) {
      sub.UPB_PRIVATE(submsg) = &ext->UPB_PRIVATE(sub).UPB_PRIVATE(submsg);
    } else {
      sub.UPB_PRIVATE(subenum) = ext->UPB_PRIVATE(sub).UPB_PRIVATE(subenum);
    }
    encode_field(e, &ext_val.UPB_PRIVATE(ext_msg_val), &sub,
                 &ext->UPB_PRIVATE(field));
  }
}

static void encode_exts(upb_encstate* e, const upb_MiniTable* m,
                        const upb_Message* msg) {
  if (m->UPB_PRIVATE(ext) == kUpb_ExtMode_NonExtendable) return;

  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  if (!in) return;

  /* Encode all extensions together. Unlike C++, we do not attempt to keep
   * these in field number order relative to normal fields or even to each
   * other. */
  uintptr_t iter = kUpb_Message_ExtensionBegin;
  const upb_MiniTableExtension* ext;
  upb_MessageValue ext_val;
  if (!UPB_PRIVATE(_upb_Message_NextExtensionReverse)(msg, &ext, &ext_val,
                                                      &iter)) {
    // Message has no extensions.
    return;
  }

  if (e->options & kUpb_EncodeOption_Deterministic) {
    _upb_sortedmap sorted;
    if (!_upb_mapsorter_pushexts(&e->sorter, in, &sorted)) {
      // TODO: b/378744096 - handle alloc failure
    }
    const upb_Extension* ext;
    while (_upb_sortedmap_nextext(&e->sorter, &sorted, &ext)) {
      encode_ext(e, ext->ext, ext->data,
                 m->UPB_PRIVATE(ext) == kUpb_ExtMode_IsMessageSet);
    }
    _upb_mapsorter_popmap(&e->sorter, &sorted);
  } else {
    do {
      encode_ext(e, ext, ext_val,
                 m->UPB_PRIVATE(ext) == kUpb_ExtMode_IsMessageSet);
    } while (UPB_PRIVATE(_upb_Message_NextExtensionReverse)(msg, &ext, &ext_val,
                                                            &iter));
  }
}

static void encode_message(upb_encstate* e, const upb_Message* msg,
                           const upb_MiniTable* m, size_t* size) {
  size_t pre_len = e->limit - e->ptr;

  if (e->options & kUpb_EncodeOption_CheckRequired) {
    if (m->UPB_PRIVATE(required_count)) {
      if (!UPB_PRIVATE(_upb_Message_IsInitializedShallow)(msg, m)) {
        encode_err(e, kUpb_EncodeStatus_MissingRequired);
      }
    }
  }

  if ((e->options & kUpb_EncodeOption_SkipUnknown) == 0) {
    size_t unknown_size = 0;
    uintptr_t iter = kUpb_Message_UnknownBegin;
    upb_StringView unknown;
    // Need to write in reverse order, but iteration is in-order; scan to
    // reserve capacity up front, then write in-order
    while (upb_Message_NextUnknown(msg, &unknown, &iter)) {
      unknown_size += unknown.size;
    }
    if (unknown_size != 0) {
      encode_reserve(e, unknown_size);
      char* ptr = e->ptr;
      iter = kUpb_Message_UnknownBegin;
      while (upb_Message_NextUnknown(msg, &unknown, &iter)) {
        memcpy(ptr, unknown.data, unknown.size);
        ptr += unknown.size;
      }
    }
  }

  encode_exts(e, m, msg);

  if (upb_MiniTable_FieldCount(m)) {
    const upb_MiniTableField* f =
        &m->UPB_PRIVATE(fields)[m->UPB_PRIVATE(field_count)];
    const upb_MiniTableField* first = &m->UPB_PRIVATE(fields)[0];
    while (f != first) {
      f--;
      if (encode_shouldencode(e, msg, f)) {
        encode_field(e, msg, m->UPB_PRIVATE(subs), f);
      }
    }
  }

  *size = (e->limit - e->ptr) - pre_len;
}

static upb_EncodeStatus upb_Encoder_Encode(upb_encstate* const encoder,
                                           const upb_Message* const msg,
                                           const upb_MiniTable* const l,
                                           char** const buf, size_t* const size,
                                           bool prepend_len) {
  // Unfortunately we must continue to perform hackery here because there are
  // code paths which blindly copy the returned pointer without bothering to
  // check for errors until much later (b/235839510). So we still set *buf to
  // NULL on error and we still set it to non-NULL on a successful empty result.
  if (UPB_SETJMP(encoder->err) == 0) {
    size_t encoded_msg_size;
    encode_message(encoder, msg, l, &encoded_msg_size);
    if (prepend_len) {
      encode_varint(encoder, encoded_msg_size);
    }
    *size = encoder->limit - encoder->ptr;
    if (*size == 0) {
      static char ch;
      *buf = &ch;
    } else {
      UPB_ASSERT(encoder->ptr);
      *buf = encoder->ptr;
    }
  } else {
    UPB_ASSERT(encoder->status != kUpb_EncodeStatus_Ok);
    *buf = NULL;
    *size = 0;
  }

  _upb_mapsorter_destroy(&encoder->sorter);
  return encoder->status;
}

uint16_t upb_EncodeOptions_GetEffectiveMaxDepth(uint32_t options) {
  uint16_t max_depth = upb_EncodeOptions_GetMaxDepth(options);
  return max_depth ? max_depth : kUpb_WireFormat_DefaultDepthLimit;
}

static upb_EncodeStatus _upb_Encode(const upb_Message* msg,
                                    const upb_MiniTable* l, int options,
                                    upb_Arena* arena, char** buf, size_t* size,
                                    bool prepend_len) {
  upb_encstate e;

  e.status = kUpb_EncodeStatus_Ok;
  e.arena = arena;
  e.buf = NULL;
  e.limit = NULL;
  e.ptr = NULL;
  e.depth = upb_EncodeOptions_GetEffectiveMaxDepth(options);
  e.options = options;
  _upb_mapsorter_init(&e.sorter);

  return upb_Encoder_Encode(&e, msg, l, buf, size, prepend_len);
}

upb_EncodeStatus upb_Encode(const upb_Message* msg, const upb_MiniTable* l,
                            int options, upb_Arena* arena, char** buf,
                            size_t* size) {
  return _upb_Encode(msg, l, options, arena, buf, size, false);
}

upb_EncodeStatus upb_EncodeLengthPrefixed(const upb_Message* msg,
                                          const upb_MiniTable* l, int options,
                                          upb_Arena* arena, char** buf,
                                          size_t* size) {
  return _upb_Encode(msg, l, options, arena, buf, size, true);
}

const char* upb_EncodeStatus_String(upb_EncodeStatus status) {
  switch (status) {
    case kUpb_EncodeStatus_Ok:
      return "Ok";
    case kUpb_EncodeStatus_MissingRequired:
      return "Missing required field";
    case kUpb_EncodeStatus_MaxDepthExceeded:
      return "Max depth exceeded";
    case kUpb_EncodeStatus_OutOfMemory:
      return "Arena alloc failed";
    default:
      return "Unknown encode status";
  }
}
