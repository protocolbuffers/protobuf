// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/wire/decode.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "upb/base/descriptor_constants.h"
#include "upb/base/error_handler.h"
#include "upb/base/internal/endian.h"
#include "upb/base/string_view.h"
#include "upb/hash/common.h"
#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include "upb/message/internal/accessors.h"
#include "upb/message/internal/array.h"
#include "upb/message/internal/extension.h"
#include "upb/message/internal/map.h"
#include "upb/message/internal/map_entry.h"
#include "upb/message/internal/message.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/mini_table/enum.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/internal/sub.h"
#include "upb/mini_table/message.h"
#include "upb/wire/encode.h"
#include "upb/wire/eps_copy_input_stream.h"
#include "upb/wire/internal/constants.h"
#include "upb/wire/internal/decoder.h"
#include "upb/wire/reader.h"

// Our awkward dance for including fasttable only when it is enabled.
#include "upb/port/def.inc"
#if UPB_FASTTABLE
#define UPB_INCLUDE_FAST_DECODE
#endif
#include "upb/port/undef.inc"

#ifdef UPB_INCLUDE_FAST_DECODE
#include "upb/wire/decode_fast/dispatch.h"
#endif

#undef UPB_INCLUDE_FAST_DECODE

// Must be last.
#include "upb/port/def.inc"

// A few fake field types for our tables.
enum {
  kUpb_FakeFieldType_FieldNotFound = 0,
  kUpb_FakeFieldType_MessageSetItem = 19,
};

// DecodeOp: an action to be performed for a wire-type/field-type combination.
enum {
  // Special ops: we don't write data to regular fields for these.
  kUpb_DecodeOp_UnknownField = -1,
  kUpb_DecodeOp_MessageSetItem = -2,

  // Scalar-only ops.
  kUpb_DecodeOp_Scalar1Byte = 0,
  kUpb_DecodeOp_Scalar4Byte = 2,
  kUpb_DecodeOp_Scalar8Byte = 3,

  // Scalar/repeated ops.
  kUpb_DecodeOp_String = 4,
  kUpb_DecodeOp_Bytes = 5,
  kUpb_DecodeOp_SubMessage = 6,

  // Repeated-only ops (also see macros below).
  kUpb_DecodeOp_PackedEnum = 13,
};

// For packed fields it is helpful to be able to recover the lg2 of the data
// size from the op.
#define OP_FIXPCK_LG2(n) (n + 5) /* n in [2, 3] => op in [7, 8] */
#define OP_VARPCK_LG2(n) (n + 9) /* n in [0, 2, 3] => op in [9, 11, 12] */

typedef union {
  bool bool_val;
  uint32_t uint32_val;
  uint64_t uint64_val;
  uint32_t size;
} wireval;

static void _upb_Decoder_AssumeEpsHasErrorHandler(upb_Decoder* d) {
  UPB_ASSUME(upb_EpsCopyInputStream_HasErrorHandler(&d->input));
}

#define EPS(d) (_upb_Decoder_AssumeEpsHasErrorHandler(d), &(d)->input)

static bool _upb_Decoder_Reserve(upb_Decoder* d, upb_Array* arr, size_t elem) {
  bool need_realloc =
      arr->UPB_PRIVATE(capacity) - arr->UPB_PRIVATE(size) < elem;
  if (need_realloc && !UPB_PRIVATE(_upb_Array_Realloc)(
                          arr, arr->UPB_PRIVATE(size) + elem, &d->arena)) {
    upb_ErrorHandler_ThrowError(&d->err, kUpb_DecodeStatus_OutOfMemory);
  }
  return need_realloc;
}

typedef struct {
  const char* ptr;
  uint64_t val;
} _upb_DecodeLongVarintReturn;

// This is identical to _upb_Decoder_DecodeTag() except that the maximum value
// is INT32_MAX instead of UINT32_MAX.
UPB_FORCEINLINE
const char* upb_Decoder_DecodeSize(upb_Decoder* d, const char* ptr,
                                   uint32_t* size) {
  int sz;
  ptr = upb_WireReader_ReadSize(ptr, &sz, EPS(d));
  *size = sz;
  return ptr;
}

static void _upb_Decoder_MungeInt32(wireval* val) {
  if (!upb_IsLittleEndian()) {
    /* The next stage will memcpy(dst, &val, 4) */
    val->uint32_val = val->uint64_val;
  }
}

static void _upb_Decoder_Munge(const upb_MiniTableField* field, wireval* val) {
  switch (field->UPB_PRIVATE(descriptortype)) {
    case kUpb_FieldType_Bool:
      val->bool_val = val->uint64_val != 0;
      break;
    case kUpb_FieldType_SInt32: {
      uint32_t n = val->uint64_val;
      val->uint32_val = (n >> 1) ^ -(int32_t)(n & 1);
      break;
    }
    case kUpb_FieldType_SInt64: {
      uint64_t n = val->uint64_val;
      val->uint64_val = (n >> 1) ^ -(int64_t)(n & 1);
      break;
    }
    case kUpb_FieldType_Int32:
    case kUpb_FieldType_UInt32:
      _upb_Decoder_MungeInt32(val);
      break;
    case kUpb_FieldType_Enum:
      UPB_UNREACHABLE();
  }
}

static upb_Message* _upb_Decoder_NewSubMessage2(upb_Decoder* d,
                                                const upb_MiniTable* subl,
                                                const upb_MiniTableField* field,
                                                upb_Message** target) {
  UPB_ASSERT(subl);
  upb_Message* msg = _upb_Message_New(subl, &d->arena);
  if (!msg) upb_ErrorHandler_ThrowError(&d->err, kUpb_DecodeStatus_OutOfMemory);

  *target = msg;
  return msg;
}

static upb_Message* _upb_Decoder_NewSubMessage(upb_Decoder* d,
                                               const upb_MiniTableField* field,
                                               upb_Message** target) {
  const upb_MiniTable* subl = upb_MiniTable_GetSubMessageTable(field);
  return _upb_Decoder_NewSubMessage2(d, subl, field, target);
}

static const char* _upb_Decoder_ReadString2(upb_Decoder* d, const char* ptr,
                                            int size, upb_StringView* str,
                                            bool validate_utf8) {
  if (!_upb_Decoder_ReadString(d, &ptr, size, str, validate_utf8)) {
    upb_ErrorHandler_ThrowError(&d->err, kUpb_DecodeStatus_OutOfMemory);
  }
  return ptr;
}

UPB_FORCEINLINE
const char* _upb_Decoder_RecurseSubMessage(upb_Decoder* d, const char* ptr,
                                           upb_Message* submsg,
                                           const upb_MiniTable* subl,
                                           uint32_t expected_end_group) {
  if (--d->depth < 0) {
    upb_ErrorHandler_ThrowError(&d->err, kUpb_DecodeStatus_MaxDepthExceeded);
  }
  ptr = _upb_Decoder_DecodeMessage(d, ptr, submsg, subl);
  d->depth++;
  if (d->end_group != expected_end_group) {
    upb_ErrorHandler_ThrowError(&d->err, kUpb_DecodeStatus_Malformed);
  }
  return ptr;
}

UPB_FORCEINLINE
const char* _upb_Decoder_DecodeSubMessage(upb_Decoder* d, const char* ptr,
                                          upb_Message* submsg,
                                          const upb_MiniTableField* field,
                                          size_t size) {
  ptrdiff_t delta = upb_EpsCopyInputStream_PushLimit(&d->input, ptr, size);
  const upb_MiniTable* subl = upb_MiniTable_GetSubMessageTable(field);
  UPB_ASSERT(subl);
  ptr = _upb_Decoder_RecurseSubMessage(d, ptr, submsg, subl, DECODE_NOGROUP);
  upb_EpsCopyInputStream_PopLimit(&d->input, ptr, delta);
  return ptr;
}

UPB_FORCEINLINE
const char* _upb_Decoder_DecodeGroup(upb_Decoder* d, const char* ptr,
                                     upb_Message* submsg,
                                     const upb_MiniTable* subl,
                                     uint32_t number) {
  if (upb_EpsCopyInputStream_IsDone(EPS(d), &ptr)) {
    upb_ErrorHandler_ThrowError(&d->err, kUpb_DecodeStatus_Malformed);
  }
  ptr = _upb_Decoder_RecurseSubMessage(d, ptr, submsg, subl, number);
  d->end_group = DECODE_NOGROUP;
  return ptr;
}

UPB_FORCEINLINE
const char* _upb_Decoder_DecodeKnownGroup(upb_Decoder* d, const char* ptr,
                                          upb_Message* submsg,
                                          const upb_MiniTableField* field) {
  const upb_MiniTable* subl = upb_MiniTable_GetSubMessageTable(field);
  UPB_ASSERT(subl);
  return _upb_Decoder_DecodeGroup(d, ptr, submsg, subl,
                                  field->UPB_PRIVATE(number));
}

#define kUpb_Decoder_EncodeVarint32MaxSize 5
static char* upb_Decoder_EncodeVarint32(uint32_t val, char* ptr) {
  do {
    uint8_t byte = val & 0x7fU;
    val >>= 7;
    if (val) byte |= 0x80U;
    *(ptr++) = byte;
  } while (val);
  return ptr;
}

UPB_FORCEINLINE
void _upb_Decoder_AddEnumValueToUnknown(upb_Decoder* d, upb_Message* msg,
                                        const upb_MiniTableField* field,
                                        wireval* val) {
  // Unrecognized enum goes into unknown fields.
  // For packed fields the tag could be arbitrarily far in the past,
  // so we just re-encode the tag and value here.
  const uint32_t tag =
      ((uint32_t)field->UPB_PRIVATE(number) << 3) | kUpb_WireType_Varint;
  upb_Message* unknown_msg =
      field->UPB_PRIVATE(mode) & kUpb_LabelFlags_IsExtension ? d->original_msg
                                                             : msg;
  char buf[2 * kUpb_Decoder_EncodeVarint32MaxSize];
  char* end = buf;
  end = upb_Decoder_EncodeVarint32(tag, end);
  end = upb_Decoder_EncodeVarint32(val->uint64_val, end);

  if (!UPB_PRIVATE(_upb_Message_AddUnknown)(unknown_msg, buf, end - buf,
                                            &d->arena, kUpb_AddUnknown_Copy)) {
    upb_ErrorHandler_ThrowError(&d->err, kUpb_DecodeStatus_OutOfMemory);
  }
}

UPB_FORCEINLINE
const char* _upb_Decoder_DecodeFixedPacked(upb_Decoder* d, const char* ptr,
                                           upb_Array* arr, wireval* val,
                                           const upb_MiniTableField* field,
                                           int lg2) {
  upb_StringView sv;
  ptr = upb_EpsCopyInputStream_ReadStringEphemeral(&d->input, ptr, val->size,
                                                   &sv);
  if (!ptr) upb_ErrorHandler_ThrowError(&d->err, kUpb_DecodeStatus_Malformed);
  int mask = (1 << lg2) - 1;
  if (UPB_UNLIKELY((val->size & mask) != 0 || ptr == NULL)) {
    upb_ErrorHandler_ThrowError(&d->err, kUpb_DecodeStatus_Malformed);
  }
  size_t count = val->size >> lg2;
  if (count == 0) return ptr;
  _upb_Decoder_Reserve(d, arr, count);
  void* mem = UPB_PTR_AT(upb_Array_MutableDataPtr(arr),
                         arr->UPB_PRIVATE(size) << lg2, void);
  arr->UPB_PRIVATE(size) += count;
  if (upb_IsLittleEndian()) {
    memcpy(mem, sv.data, sv.size);
  } else {
    const char* src = sv.data;
    const char* src_end = src + sv.size;
    char* dst = mem;
    if (lg2 == 2) {
      for (; src < src_end; src += 4, dst += 4) {
        uint32_t x;
        memcpy(&x, src, 4);
        x = upb_BigEndian32(x);
        memcpy(dst, &x, 4);
      }
    } else {
      UPB_ASSERT(lg2 == 3);
      for (; src < src_end; src += 8, dst += 8) {
        uint64_t x;
        memcpy(&x, src, 8);
        x = upb_BigEndian64(x);
        memcpy(dst, &x, 8);
      }
    }
  }

  return ptr;
}

UPB_FORCEINLINE
const char* _upb_Decoder_DecodeVarintPacked(upb_Decoder* d, const char* ptr,
                                            upb_Array* arr, wireval* val,
                                            const upb_MiniTableField* field,
                                            int lg2) {
  int scale = 1 << lg2;
  ptrdiff_t delta = upb_EpsCopyInputStream_PushLimit(&d->input, ptr, val->size);
  char* out = UPB_PTR_AT(upb_Array_MutableDataPtr(arr),
                         arr->UPB_PRIVATE(size) << lg2, void);
  while (!upb_EpsCopyInputStream_IsDone(EPS(d), &ptr)) {
    wireval elem;
    ptr = upb_WireReader_ReadVarint(ptr, &elem.uint64_val, EPS(d));
    _upb_Decoder_Munge(field, &elem);
    if (_upb_Decoder_Reserve(d, arr, 1)) {
      out = UPB_PTR_AT(upb_Array_MutableDataPtr(arr),
                       arr->UPB_PRIVATE(size) << lg2, void);
    }
    arr->UPB_PRIVATE(size)++;
    memcpy(out, &elem, scale);
    out += scale;
  }
  upb_EpsCopyInputStream_PopLimit(&d->input, ptr, delta);
  return ptr;
}

UPB_NOINLINE
static const char* _upb_Decoder_DecodeEnumPacked(
    upb_Decoder* d, const char* ptr, upb_Message* msg, upb_Array* arr,
    const upb_MiniTableField* field, wireval* val) {
  const upb_MiniTableEnum* e = upb_MiniTable_GetSubEnumTable(field);
  ptrdiff_t delta = upb_EpsCopyInputStream_PushLimit(&d->input, ptr, val->size);
  char* out = UPB_PTR_AT(upb_Array_MutableDataPtr(arr),
                         arr->UPB_PRIVATE(size) * 4, void);
  while (!upb_EpsCopyInputStream_IsDone(EPS(d), &ptr)) {
    wireval elem;
    ptr = upb_WireReader_ReadVarint(ptr, &elem.uint64_val, EPS(d));
    if (!upb_MiniTableEnum_CheckValue(e, elem.uint64_val)) {
      _upb_Decoder_AddEnumValueToUnknown(d, msg, field, &elem);
      continue;
    }
    if (_upb_Decoder_Reserve(d, arr, 1)) {
      out = UPB_PTR_AT(upb_Array_MutableDataPtr(arr),
                       arr->UPB_PRIVATE(size) * 4, void);
    }
    arr->UPB_PRIVATE(size)++;
    memcpy(out, &elem, 4);
    out += 4;
  }
  upb_EpsCopyInputStream_PopLimit(&d->input, ptr, delta);
  return ptr;
}

static upb_Array* _upb_Decoder_CreateArray(upb_Decoder* d,
                                           const upb_MiniTableField* field) {
  const upb_FieldType field_type = field->UPB_PRIVATE(descriptortype);
  const size_t lg2 = UPB_PRIVATE(_upb_FieldType_SizeLg2)(field_type);
  upb_Array* ret = UPB_PRIVATE(_upb_Array_New)(&d->arena, 4, lg2);
  if (!ret) upb_ErrorHandler_ThrowError(&d->err, kUpb_DecodeStatus_OutOfMemory);
  return ret;
}

static const char* _upb_Decoder_DecodeToArray(upb_Decoder* d, const char* ptr,
                                              upb_Message* msg,
                                              const upb_MiniTableField* field,
                                              wireval* val, int op) {
  upb_Array** arrp = UPB_PTR_AT(msg, field->UPB_PRIVATE(offset), void);
  upb_Array* arr = *arrp;
  void* mem;

  if (arr) {
    _upb_Decoder_Reserve(d, arr, 1);
  } else {
    arr = _upb_Decoder_CreateArray(d, field);
    *arrp = arr;
  }

  switch (op) {
    case kUpb_DecodeOp_Scalar1Byte:
    case kUpb_DecodeOp_Scalar4Byte:
    case kUpb_DecodeOp_Scalar8Byte:
      /* Append scalar value. */
      mem = UPB_PTR_AT(upb_Array_MutableDataPtr(arr),
                       arr->UPB_PRIVATE(size) << op, void);
      arr->UPB_PRIVATE(size)++;
      memcpy(mem, val, 1 << op);
      return ptr;
    case kUpb_DecodeOp_String: {
      /* Append string. */
      upb_StringView* str = (upb_StringView*)upb_Array_MutableDataPtr(arr) +
                            arr->UPB_PRIVATE(size);
      ptr = _upb_Decoder_ReadString2(d, ptr, val->size, str,
                                     /*validate_utf8=*/true);
      arr->UPB_PRIVATE(size)++;
      return ptr;
    }
    case kUpb_DecodeOp_Bytes: {
      /* Append bytes. */
      upb_StringView* str = (upb_StringView*)upb_Array_MutableDataPtr(arr) +
                            arr->UPB_PRIVATE(size);
      ptr = _upb_Decoder_ReadString2(d, ptr, val->size, str,
                                     /*validate_utf8=*/false);
      arr->UPB_PRIVATE(size)++;
      return ptr;
    }
    case kUpb_DecodeOp_SubMessage: {
      /* Append submessage / group. */
      upb_Message** target =
          UPB_PTR_AT(upb_Array_MutableDataPtr(arr),
                     arr->UPB_PRIVATE(size) * sizeof(void*), upb_Message*);
      upb_Message* submsg = _upb_Decoder_NewSubMessage(d, field, target);
      arr->UPB_PRIVATE(size)++;
      if (UPB_UNLIKELY(field->UPB_PRIVATE(descriptortype) ==
                       kUpb_FieldType_Group)) {
        return _upb_Decoder_DecodeKnownGroup(d, ptr, submsg, field);
      } else {
        return _upb_Decoder_DecodeSubMessage(d, ptr, submsg, field, val->size);
      }
    }
    case OP_FIXPCK_LG2(2):
    case OP_FIXPCK_LG2(3):
      return _upb_Decoder_DecodeFixedPacked(d, ptr, arr, val, field,
                                            op - OP_FIXPCK_LG2(0));
    case OP_VARPCK_LG2(0):
    case OP_VARPCK_LG2(2):
    case OP_VARPCK_LG2(3):
      return _upb_Decoder_DecodeVarintPacked(d, ptr, arr, val, field,
                                             op - OP_VARPCK_LG2(0));
    case kUpb_DecodeOp_PackedEnum:
      return _upb_Decoder_DecodeEnumPacked(d, ptr, msg, arr, field, val);
    default:
      UPB_UNREACHABLE();
  }
}

static upb_Map* _upb_Decoder_CreateMap(upb_Decoder* d,
                                       const upb_MiniTable* entry) {
  // Maps descriptor type -> upb map size
  static const uint8_t kSizeInMap[] = {
      [0] = -1,  // invalid descriptor type
      [kUpb_FieldType_Double] = 8,
      [kUpb_FieldType_Float] = 4,
      [kUpb_FieldType_Int64] = 8,
      [kUpb_FieldType_UInt64] = 8,
      [kUpb_FieldType_Int32] = 4,
      [kUpb_FieldType_Fixed64] = 8,
      [kUpb_FieldType_Fixed32] = 4,
      [kUpb_FieldType_Bool] = 1,
      [kUpb_FieldType_String] = UPB_MAPTYPE_STRING,
      [kUpb_FieldType_Group] = sizeof(void*),
      [kUpb_FieldType_Message] = sizeof(void*),
      [kUpb_FieldType_Bytes] = UPB_MAPTYPE_STRING,
      [kUpb_FieldType_UInt32] = 4,
      [kUpb_FieldType_Enum] = 4,
      [kUpb_FieldType_SFixed32] = 4,
      [kUpb_FieldType_SFixed64] = 8,
      [kUpb_FieldType_SInt32] = 4,
      [kUpb_FieldType_SInt64] = 8,
  };

  const upb_MiniTableField* key_field = &entry->UPB_PRIVATE(fields)[0];
  const upb_MiniTableField* val_field = &entry->UPB_PRIVATE(fields)[1];
  char key_size = kSizeInMap[key_field->UPB_PRIVATE(descriptortype)];
  char val_size = kSizeInMap[val_field->UPB_PRIVATE(descriptortype)];
  UPB_ASSERT(key_field->UPB_PRIVATE(offset) == offsetof(upb_MapEntry, k));
  UPB_ASSERT(val_field->UPB_PRIVATE(offset) == offsetof(upb_MapEntry, v));
  upb_Map* ret = _upb_Map_New(&d->arena, key_size, val_size);
  if (!ret) upb_ErrorHandler_ThrowError(&d->err, kUpb_DecodeStatus_OutOfMemory);
  return ret;
}

UPB_NOINLINE static void _upb_Decoder_AddMapEntryUnknown(
    upb_Decoder* d, upb_Message* msg, const upb_MiniTableField* field,
    upb_Message* ent_msg, const upb_MiniTable* entry) {
  char* buf;
  size_t size;
  upb_EncodeStatus status =
      upb_Encode(ent_msg, entry, 0, &d->arena, &buf, &size);
  if (status != kUpb_EncodeStatus_Ok) {
    upb_ErrorHandler_ThrowError(&d->err, kUpb_DecodeStatus_OutOfMemory);
  }
  char delim_buf[2 * kUpb_Decoder_EncodeVarint32MaxSize];
  char* delim_end = delim_buf;
  uint32_t tag =
      ((uint32_t)field->UPB_PRIVATE(number) << 3) | kUpb_WireType_Delimited;
  delim_end = upb_Decoder_EncodeVarint32(tag, delim_end);
  delim_end = upb_Decoder_EncodeVarint32(size, delim_end);
  upb_StringView unknown[] = {
      {delim_buf, delim_end - delim_buf},
      {buf, size},
  };

  if (!UPB_PRIVATE(_upb_Message_AddUnknownV)(msg, &d->arena, unknown, 2)) {
    upb_ErrorHandler_ThrowError(&d->err, kUpb_DecodeStatus_OutOfMemory);
  }
}

static const char* _upb_Decoder_DecodeToMap(upb_Decoder* d, const char* ptr,
                                            upb_Message* msg,
                                            const upb_MiniTableField* field,
                                            wireval* val) {
  upb_Map** map_p = UPB_PTR_AT(msg, field->UPB_PRIVATE(offset), upb_Map*);
  upb_Map* map = *map_p;
  upb_MapEntry ent;
  UPB_ASSERT(upb_MiniTableField_Type(field) == kUpb_FieldType_Message);
  const upb_MiniTable* entry = upb_MiniTable_GetSubMessageTable(field);

  UPB_ASSERT(entry);
  UPB_ASSERT(entry->UPB_PRIVATE(field_count) == 2);
  UPB_ASSERT(upb_MiniTableField_IsScalar(&entry->UPB_PRIVATE(fields)[0]));
  UPB_ASSERT(upb_MiniTableField_IsScalar(&entry->UPB_PRIVATE(fields)[1]));

  if (!map) {
    map = _upb_Decoder_CreateMap(d, entry);
    *map_p = map;
  }

  // Parse map entry.
  memset(&ent, 0, sizeof(ent));

  bool value_is_message =
      entry->UPB_PRIVATE(fields)[1].UPB_PRIVATE(descriptortype) ==
          kUpb_FieldType_Message ||
      entry->UPB_PRIVATE(fields)[1].UPB_PRIVATE(descriptortype) ==
          kUpb_FieldType_Group;
  const upb_MiniTable* sub_table =
      value_is_message
          ? upb_MiniTable_GetSubMessageTable(&entry->UPB_PRIVATE(fields)[1])
          : NULL;
  upb_Message* sub_msg = NULL;

  if (sub_table) {
    // Create proactively to handle the case where it doesn't appear.
    _upb_Decoder_NewSubMessage(d, &entry->UPB_PRIVATE(fields)[1], &sub_msg);
    ent.v.val = upb_value_ptr(sub_msg);
  }

  ptr = _upb_Decoder_DecodeSubMessage(d, ptr, &ent.message, field, val->size);

  if (sub_msg && sub_table->UPB_PRIVATE(required_count)) {
    // If the map entry did not contain a value on the wire, `sub_msg` is an
    // empty message; we must check if it is missing any required fields. If the
    // value was present, this check is redundant but harmless.
    _upb_Decoder_CheckRequired(d, ptr, sub_msg, sub_table);
  }

  if (upb_Message_HasUnknown(&ent.message)) {
    _upb_Decoder_AddMapEntryUnknown(d, msg, field, &ent.message, entry);
  } else {
    if (_upb_Map_Insert(map, &ent.k, map->key_size, &ent.v, map->val_size,
                        &d->arena) == kUpb_MapInsertStatus_OutOfMemory) {
      upb_ErrorHandler_ThrowError(&d->err, kUpb_DecodeStatus_OutOfMemory);
    }
  }
  return ptr;
}

static const char* _upb_Decoder_DecodeToSubMessage(
    upb_Decoder* d, const char* ptr, upb_Message* msg,
    const upb_MiniTableField* field, wireval* val, int op) {
  void* mem = UPB_PTR_AT(msg, field->UPB_PRIVATE(offset), void);
  int type = field->UPB_PRIVATE(descriptortype);

  // Set presence if necessary.
  if (UPB_PRIVATE(_upb_MiniTableField_HasHasbit)(field)) {
    UPB_PRIVATE(_upb_Message_SetHasbit)(msg, field);
  } else if (upb_MiniTableField_IsInOneof(field)) {
    // Oneof case
    uint32_t* oneof_case = UPB_PRIVATE(_upb_Message_OneofCasePtr)(msg, field);
    if (op == kUpb_DecodeOp_SubMessage &&
        *oneof_case != field->UPB_PRIVATE(number)) {
      memset(mem, 0, sizeof(void*));
    }
    *oneof_case = field->UPB_PRIVATE(number);
  }

  // Store into message.
  switch (op) {
    case kUpb_DecodeOp_SubMessage: {
      upb_Message** submsgp = mem;
      upb_Message* submsg = *submsgp;
      if (!submsg) submsg = _upb_Decoder_NewSubMessage(d, field, submsgp);
      if (UPB_UNLIKELY(type == kUpb_FieldType_Group)) {
        ptr = _upb_Decoder_DecodeKnownGroup(d, ptr, submsg, field);
      } else {
        ptr = _upb_Decoder_DecodeSubMessage(d, ptr, submsg, field, val->size);
      }
      break;
    }
    case kUpb_DecodeOp_String:
      return _upb_Decoder_ReadString2(d, ptr, val->size, mem,
                                      /*validate_utf8=*/true);
    case kUpb_DecodeOp_Bytes:
      return _upb_Decoder_ReadString2(d, ptr, val->size, mem,
                                      /*validate_utf8=*/false);
    case kUpb_DecodeOp_Scalar8Byte:
      memcpy(mem, val, 8);
      break;
    case kUpb_DecodeOp_Scalar4Byte:
      memcpy(mem, val, 4);
      break;
    case kUpb_DecodeOp_Scalar1Byte:
      memcpy(mem, val, 1);
      break;
    default:
      UPB_UNREACHABLE();
  }

  return ptr;
}

enum {
  kStartItemTag = ((kUpb_MsgSet_Item << 3) | kUpb_WireType_StartGroup),
  kEndItemTag = ((kUpb_MsgSet_Item << 3) | kUpb_WireType_EndGroup),
  kTypeIdTag = ((kUpb_MsgSet_TypeId << 3) | kUpb_WireType_Varint),
  kMessageTag = ((kUpb_MsgSet_Message << 3) | kUpb_WireType_Delimited),
};

static void upb_Decoder_AddKnownMessageSetItem(
    upb_Decoder* d, upb_Message* msg, const upb_MiniTableExtension* item_mt,
    const char* data, uint32_t size) {
  upb_Extension* ext =
      UPB_PRIVATE(_upb_Message_GetOrCreateExtension)(msg, item_mt, &d->arena);
  if (UPB_UNLIKELY(!ext)) {
    upb_ErrorHandler_ThrowError(&d->err, kUpb_DecodeStatus_OutOfMemory);
  }
  upb_Message** submsgp = (upb_Message**)&ext->data.msg_val;
  upb_Message* submsg = _upb_Decoder_NewSubMessage2(
      d, ext->ext->UPB_PRIVATE(sub).UPB_PRIVATE(submsg),
      &ext->ext->UPB_PRIVATE(field), submsgp);
  // upb_Decode_LimitDepth() takes uint32_t, d->depth - 1 can not be negative.
  if (d->depth <= 1) {
    upb_ErrorHandler_ThrowError(&d->err, kUpb_DecodeStatus_MaxDepthExceeded);
  }
  upb_DecodeStatus status = upb_Decode(
      data, size, submsg, upb_MiniTableExtension_GetSubMessage(item_mt),
      d->extreg, upb_Decode_LimitDepth(d->options, d->depth - 1), &d->arena);
  if (status != kUpb_DecodeStatus_Ok) {
    upb_ErrorHandler_ThrowError(&d->err, status);
  }
}

static void upb_Decoder_AddUnknownMessageSetItem(upb_Decoder* d,
                                                 upb_Message* msg,
                                                 uint32_t type_id,
                                                 const char* message_data,
                                                 uint32_t message_size) {
  char buf[6 * kUpb_Decoder_EncodeVarint32MaxSize];
  char* ptr = buf;
  ptr = upb_Decoder_EncodeVarint32(kStartItemTag, ptr);
  ptr = upb_Decoder_EncodeVarint32(kTypeIdTag, ptr);
  ptr = upb_Decoder_EncodeVarint32(type_id, ptr);
  ptr = upb_Decoder_EncodeVarint32(kMessageTag, ptr);
  ptr = upb_Decoder_EncodeVarint32(message_size, ptr);
  char* split = ptr;

  ptr = upb_Decoder_EncodeVarint32(kEndItemTag, ptr);
  char* end = ptr;
  upb_StringView unknown[] = {
      {buf, split - buf},
      {message_data, message_size},
      {split, end - split},
  };
  if (!UPB_PRIVATE(_upb_Message_AddUnknownV)(msg, &d->arena, unknown, 3)) {
    upb_ErrorHandler_ThrowError(&d->err, kUpb_DecodeStatus_OutOfMemory);
  }
}

static void upb_Decoder_AddMessageSetItem(upb_Decoder* d, upb_Message* msg,
                                          const upb_MiniTable* t,
                                          uint32_t type_id, const char* data,
                                          uint32_t size) {
  const upb_MiniTableExtension* item_mt =
      upb_ExtensionRegistry_Lookup(d->extreg, t, type_id);
  if (item_mt) {
    upb_Decoder_AddKnownMessageSetItem(d, msg, item_mt, data, size);
  } else {
    upb_Decoder_AddUnknownMessageSetItem(d, msg, type_id, data, size);
  }
}

static const char* upb_Decoder_DecodeMessageSetItem(
    upb_Decoder* d, const char* ptr, upb_Message* msg,
    const upb_MiniTable* layout) {
  uint32_t type_id = 0;
  upb_StringView preserved = {NULL, 0};
  typedef enum {
    kUpb_HaveId = 1 << 0,
    kUpb_HavePayload = 1 << 1,
  } StateMask;
  StateMask state_mask = 0;
  while (!upb_EpsCopyInputStream_IsDone(EPS(d), &ptr)) {
    uint32_t tag;
    ptr = upb_WireReader_ReadTag(ptr, &tag, EPS(d));
    switch (tag) {
      case kEndItemTag:
        return ptr;
      case kTypeIdTag: {
        uint64_t tmp;
        ptr = upb_WireReader_ReadVarint(ptr, &tmp, EPS(d));
        if (state_mask & kUpb_HaveId) break;  // Ignore dup.
        state_mask |= kUpb_HaveId;
        type_id = tmp;
        if (state_mask & kUpb_HavePayload) {
          upb_Decoder_AddMessageSetItem(d, msg, layout, type_id, preserved.data,
                                        preserved.size);
        }
        break;
      }
      case kMessageTag: {
        uint32_t size;
        upb_StringView sv;
        ptr = upb_Decoder_DecodeSize(d, ptr, &size);
        ptr = upb_EpsCopyInputStream_ReadStringAlwaysAlias(&d->input, ptr, size,
                                                           &sv);
        if (!ptr) {
          upb_ErrorHandler_ThrowError(&d->err, kUpb_DecodeStatus_Malformed);
        }
        if (state_mask & kUpb_HavePayload) break;  // Ignore dup.
        state_mask |= kUpb_HavePayload;
        if (state_mask & kUpb_HaveId) {
          upb_Decoder_AddMessageSetItem(d, msg, layout, type_id, sv.data,
                                        sv.size);
        } else {
          // Out of order, we must preserve the payload.
          preserved = sv;
        }
        break;
      }
      default:
        // We do not preserve unexpected fields inside a message set item.
        ptr = _upb_WireReader_SkipValue(ptr, tag, d->depth, &d->input);
        break;
    }
  }
  upb_ErrorHandler_ThrowError(&d->err, kUpb_DecodeStatus_Malformed);
}

static upb_MiniTableField upb_Decoder_FieldNotFoundField = {
    0, 0, 0, 0, kUpb_FakeFieldType_FieldNotFound, 0};

UPB_NOINLINE const upb_MiniTableField* _upb_Decoder_FindExtensionField(
    upb_Decoder* d, const upb_MiniTable* t, uint32_t field_number, int ext_mode,
    uint32_t wire_type) {
  // Treat a message set as an extendable message if it is a delimited field.
  // This provides compatibility with encoders that are unaware of message
  // sets and serialize them as normal extensions.
  if (ext_mode == kUpb_ExtMode_Extendable ||
      (ext_mode == kUpb_ExtMode_IsMessageSet &&
       wire_type == kUpb_WireType_Delimited)) {
    const upb_MiniTableExtension* ext =
        upb_ExtensionRegistry_Lookup(d->extreg, t, field_number);
    if (ext) return &ext->UPB_PRIVATE(field);
  } else if (ext_mode == kUpb_ExtMode_IsMessageSet) {
    if (field_number == kUpb_MsgSet_Item) {
      static upb_MiniTableField item = {
          0, 0, 0, 0, kUpb_FakeFieldType_MessageSetItem, 0};
      return &item;
    }
  }
  return &upb_Decoder_FieldNotFoundField;
}

static const upb_MiniTableField* _upb_Decoder_FindField(upb_Decoder* d,
                                                        const upb_MiniTable* t,
                                                        uint32_t field_number,
                                                        uint32_t wire_type) {
  UPB_ASSERT(t);
  const upb_MiniTableField* field =
      upb_MiniTable_FindFieldByNumber(t, field_number);
  if (field) return field;

  if (d->extreg && t->UPB_PRIVATE(ext)) {
    return _upb_Decoder_FindExtensionField(d, t, field_number,
                                           t->UPB_PRIVATE(ext), wire_type);
  }

  return &upb_Decoder_FieldNotFoundField;  // Unknown field.
}

static int _upb_Decoder_GetVarintOp(const upb_MiniTableField* field) {
  static const int8_t kVarintOps[] = {
      [kUpb_FakeFieldType_FieldNotFound] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Double] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Float] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Int64] = kUpb_DecodeOp_Scalar8Byte,
      [kUpb_FieldType_UInt64] = kUpb_DecodeOp_Scalar8Byte,
      [kUpb_FieldType_Int32] = kUpb_DecodeOp_Scalar4Byte,
      [kUpb_FieldType_Fixed64] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Fixed32] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Bool] = kUpb_DecodeOp_Scalar1Byte,
      [kUpb_FieldType_String] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Group] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Message] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Bytes] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_UInt32] = kUpb_DecodeOp_Scalar4Byte,
      [kUpb_FieldType_Enum] = kUpb_DecodeOp_Scalar4Byte,
      [kUpb_FieldType_SFixed32] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_SFixed64] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_SInt32] = kUpb_DecodeOp_Scalar4Byte,
      [kUpb_FieldType_SInt64] = kUpb_DecodeOp_Scalar8Byte,
      [kUpb_FakeFieldType_MessageSetItem] = kUpb_DecodeOp_UnknownField,
  };

  return kVarintOps[field->UPB_PRIVATE(descriptortype)];
}

UPB_FORCEINLINE
void _upb_Decoder_CheckUnlinked(upb_Decoder* d, const upb_MiniTable* mt,
                                const upb_MiniTableField* field, int* op) {
  // If sub-message is not linked, treat as unknown.
  if (field->UPB_PRIVATE(mode) & kUpb_LabelFlags_IsExtension) return;
  const upb_MiniTable* mt_sub = upb_MiniTable_GetSubMessageTable(field);
  if (mt_sub != NULL) return;  // Normal case, sub-message is linked.
#ifndef NDEBUG
  const upb_MiniTableField* oneof = upb_MiniTable_GetOneof(mt, field);
  if (oneof) {
    // All other members of the oneof must be message fields that are also
    // unlinked.
    do {
      UPB_ASSERT(upb_MiniTableField_CType(oneof) == kUpb_CType_Message);
      const upb_MiniTable* oneof_sub = upb_MiniTable_GetSubMessageTable(oneof);
      UPB_ASSERT(!oneof_sub);
    } while (upb_MiniTable_NextOneofField(mt, &oneof));
  }
#endif  // NDEBUG
  *op = kUpb_DecodeOp_UnknownField;
}

UPB_FORCEINLINE
void _upb_Decoder_MaybeVerifyUtf8(upb_Decoder* d,
                                  const upb_MiniTableField* field, int* op) {
  UPB_ASSUME(field->UPB_PRIVATE(descriptortype) == kUpb_FieldType_Bytes);
  if (_upb_Decoder_FieldRequiresUtf8Validation(d, field)) {
    *op = kUpb_DecodeOp_String;
  }
}

static int _upb_Decoder_GetDelimitedOp(upb_Decoder* d, const upb_MiniTable* mt,
                                       const upb_MiniTableField* field) {
  enum { kRepeatedBase = 19 };

  static const int8_t kDelimitedOps[] = {
      // For non-repeated field type.
      [kUpb_FakeFieldType_FieldNotFound] =
          kUpb_DecodeOp_UnknownField,  // Field not found.
      [kUpb_FieldType_Double] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Float] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Int64] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_UInt64] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Int32] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Fixed64] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Fixed32] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Bool] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_String] = kUpb_DecodeOp_String,
      [kUpb_FieldType_Group] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Message] = kUpb_DecodeOp_SubMessage,
      [kUpb_FieldType_Bytes] = kUpb_DecodeOp_Bytes,
      [kUpb_FieldType_UInt32] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_Enum] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_SFixed32] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_SFixed64] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_SInt32] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_SInt64] = kUpb_DecodeOp_UnknownField,
      [kUpb_FakeFieldType_MessageSetItem] = kUpb_DecodeOp_SubMessage,
      // For repeated field type.
      [kRepeatedBase + kUpb_FieldType_Double] = OP_FIXPCK_LG2(3),
      [kRepeatedBase + kUpb_FieldType_Float] = OP_FIXPCK_LG2(2),
      [kRepeatedBase + kUpb_FieldType_Int64] = OP_VARPCK_LG2(3),
      [kRepeatedBase + kUpb_FieldType_UInt64] = OP_VARPCK_LG2(3),
      [kRepeatedBase + kUpb_FieldType_Int32] = OP_VARPCK_LG2(2),
      [kRepeatedBase + kUpb_FieldType_Fixed64] = OP_FIXPCK_LG2(3),
      [kRepeatedBase + kUpb_FieldType_Fixed32] = OP_FIXPCK_LG2(2),
      [kRepeatedBase + kUpb_FieldType_Bool] = OP_VARPCK_LG2(0),
      [kRepeatedBase + kUpb_FieldType_String] = kUpb_DecodeOp_String,
      [kRepeatedBase + kUpb_FieldType_Group] = kUpb_DecodeOp_SubMessage,
      [kRepeatedBase + kUpb_FieldType_Message] = kUpb_DecodeOp_SubMessage,
      [kRepeatedBase + kUpb_FieldType_Bytes] = kUpb_DecodeOp_Bytes,
      [kRepeatedBase + kUpb_FieldType_UInt32] = OP_VARPCK_LG2(2),
      [kRepeatedBase + kUpb_FieldType_Enum] = kUpb_DecodeOp_PackedEnum,
      [kRepeatedBase + kUpb_FieldType_SFixed32] = OP_FIXPCK_LG2(2),
      [kRepeatedBase + kUpb_FieldType_SFixed64] = OP_FIXPCK_LG2(3),
      [kRepeatedBase + kUpb_FieldType_SInt32] = OP_VARPCK_LG2(2),
      [kRepeatedBase + kUpb_FieldType_SInt64] = OP_VARPCK_LG2(3),
      // Omitting kUpb_FakeFieldType_MessageSetItem, because we never emit a
      // repeated msgset type
  };

  int ndx = field->UPB_PRIVATE(descriptortype);
  if (upb_MiniTableField_IsArray(field)) ndx += kRepeatedBase;
  int op = kDelimitedOps[ndx];

  if (op == kUpb_DecodeOp_SubMessage) {
    _upb_Decoder_CheckUnlinked(d, mt, field, &op);
  } else if (op == kUpb_DecodeOp_Bytes) {
    _upb_Decoder_MaybeVerifyUtf8(d, field, &op);
  }

  return op;
}

UPB_FORCEINLINE
const char* _upb_Decoder_DecodeWireValue(upb_Decoder* d, const char* ptr,
                                         const upb_MiniTable* mt,
                                         const upb_MiniTableField* field,
                                         uint32_t wire_type, wireval* val,
                                         int* op) {
  static const unsigned kFixed32OkMask = (1 << kUpb_FieldType_Float) |
                                         (1 << kUpb_FieldType_Fixed32) |
                                         (1 << kUpb_FieldType_SFixed32);

  static const unsigned kFixed64OkMask = (1 << kUpb_FieldType_Double) |
                                         (1 << kUpb_FieldType_Fixed64) |
                                         (1 << kUpb_FieldType_SFixed64);

  switch (wire_type) {
    case kUpb_WireType_Varint:
      ptr = upb_WireReader_ReadVarint(ptr, &val->uint64_val, EPS(d));
      if (upb_MiniTableField_IsClosedEnum(field)) {
        const upb_MiniTableEnum* e = upb_MiniTable_GetSubEnumTable(field);
        if (!upb_MiniTableEnum_CheckValue(e, val->uint64_val)) {
          *op = kUpb_DecodeOp_UnknownField;
          return ptr;
        }
        _upb_Decoder_MungeInt32(val);
      } else {
        _upb_Decoder_Munge(field, val);
      }
      *op = _upb_Decoder_GetVarintOp(field);
      return ptr;
    case kUpb_WireType_32Bit:
      *op = kUpb_DecodeOp_Scalar4Byte;
      if (((1 << field->UPB_PRIVATE(descriptortype)) & kFixed32OkMask) == 0) {
        *op = kUpb_DecodeOp_UnknownField;
      }
      return upb_WireReader_ReadFixed32(ptr, &val->uint32_val, &d->input);
    case kUpb_WireType_64Bit:
      *op = kUpb_DecodeOp_Scalar8Byte;
      if (((1 << field->UPB_PRIVATE(descriptortype)) & kFixed64OkMask) == 0) {
        *op = kUpb_DecodeOp_UnknownField;
      }
      return upb_WireReader_ReadFixed64(ptr, &val->uint64_val, &d->input);
    case kUpb_WireType_Delimited:
      ptr = upb_Decoder_DecodeSize(d, ptr, &val->size);
      *op = _upb_Decoder_GetDelimitedOp(d, mt, field);
      return ptr;
    case kUpb_WireType_StartGroup:
      val->uint32_val = field->UPB_PRIVATE(number);
      if (field->UPB_PRIVATE(descriptortype) == kUpb_FieldType_Group) {
        *op = kUpb_DecodeOp_SubMessage;
        _upb_Decoder_CheckUnlinked(d, mt, field, op);
      } else if (field->UPB_PRIVATE(descriptortype) ==
                 kUpb_FakeFieldType_MessageSetItem) {
        *op = kUpb_DecodeOp_MessageSetItem;
      } else {
        *op = kUpb_DecodeOp_UnknownField;
      }
      return ptr;
    default:
      break;
  }
  upb_ErrorHandler_ThrowError(&d->err, kUpb_DecodeStatus_Malformed);
}

UPB_FORCEINLINE
const char* _upb_Decoder_DecodeKnownField(upb_Decoder* d, const char* ptr,
                                          upb_Message* msg,
                                          const upb_MiniTable* layout,
                                          const upb_MiniTableField* field,
                                          int op, wireval* val) {
  uint8_t mode = field->UPB_PRIVATE(mode);

  if (UPB_UNLIKELY(mode & kUpb_LabelFlags_IsExtension)) {
    const upb_MiniTableExtension* ext_layout =
        (const upb_MiniTableExtension*)field;
    upb_Extension* ext = UPB_PRIVATE(_upb_Message_GetOrCreateExtension)(
        msg, ext_layout, &d->arena);
    if (UPB_UNLIKELY(!ext)) {
      upb_ErrorHandler_ThrowError(&d->err, kUpb_DecodeStatus_OutOfMemory);
    }
    d->original_msg = msg;
    msg = &ext->data.UPB_PRIVATE(ext_msg_val);
  }

  switch (mode & kUpb_FieldMode_Mask) {
    case kUpb_FieldMode_Array:
      return _upb_Decoder_DecodeToArray(d, ptr, msg, field, val, op);
    case kUpb_FieldMode_Map:
      return _upb_Decoder_DecodeToMap(d, ptr, msg, field, val);
    case kUpb_FieldMode_Scalar:
      return _upb_Decoder_DecodeToSubMessage(d, ptr, msg, field, val, op);
    default:
      UPB_UNREACHABLE();
  }
}

static const char* _upb_Decoder_FindFieldStart(upb_Decoder* d, const char* ptr,
                                               uint32_t field_number,
                                               uint32_t wire_type) {
  // Since unknown fields are the uncommon case, we do a little extra work here
  // to walk backwards through the buffer to find the field start.  This frees
  // up a register in the fast paths (when the field is known), which leads to
  // significant speedups in benchmarks. Note that ptr may point into the slop
  // space, beyond the normal end of the input buffer.
  const char* start = ptr;

  switch (wire_type) {
    case kUpb_WireType_Varint:
    case kUpb_WireType_Delimited:
      // Skip the last byte
      start--;
      // Skip bytes until we encounter the final byte of the tag varint.
      while (start[-1] & 0x80) start--;
      break;
    case kUpb_WireType_32Bit:
      start -= 4;
      break;
    case kUpb_WireType_64Bit:
      start -= 8;
      break;
    default:
      break;
  }
  assert(start == d->debug_valstart);

  {
    // The varint parser does not enforce that integers are encoded with their
    // minimum size; for example the value 1 could be encoded with three
    // bytes: 0x81, 0x80, 0x00. These unnecessary trailing zeroes mean that we
    // cannot skip backwards by the minimum encoded size of the tag; and
    // unlike the loop for delimited or varint fields, we can't stop at a
    // sentinel value because anything can precede a tag. Instead, parse back
    // one byte at a time until we read the same tag value that was parsed
    // earlier.
    uint32_t tag = ((uint32_t)field_number << 3) | wire_type;
    uint32_t seen = 0;
    do {
      start--;
      seen <<= 7;
      seen |= *start & 0x7f;
    } while (seen != tag);
  }
  assert(start == d->debug_tagstart);

  return start;
}

static const char* _upb_Decoder_DecodeUnknownField(
    upb_Decoder* d, const char* ptr, upb_Message* msg, uint32_t field_number,
    uint32_t wire_type, wireval val) {
  if (field_number == 0) {
    upb_ErrorHandler_ThrowError(&d->err, kUpb_DecodeStatus_Malformed);
  }

  const char* start =
      _upb_Decoder_FindFieldStart(d, ptr, field_number, wire_type);

  upb_EpsCopyInputStream_StartCapture(&d->input, start);

  if (wire_type == kUpb_WireType_Delimited) {
    upb_StringView sv;
    ptr = upb_EpsCopyInputStream_ReadStringEphemeral(&d->input, ptr, val.size,
                                                     &sv);
    if (!ptr) upb_ErrorHandler_ThrowError(&d->err, kUpb_DecodeStatus_Malformed);
  } else if (wire_type == kUpb_WireType_StartGroup) {
    ptr = UPB_PRIVATE(_upb_WireReader_SkipGroup)(ptr, field_number << 3,
                                                 d->depth, &d->input);
  }

  upb_StringView sv;
  upb_EpsCopyInputStream_EndCapture(&d->input, ptr, &sv);

  upb_AddUnknownMode mode = kUpb_AddUnknown_Copy;
  if (d->options & kUpb_DecodeOption_AliasString) {
    if (sv.data != d->input.buffer_start) {
      // If the data is not from the beginning of the input buffer, then we can
      // safely attempt to coalesce this region with the previous one.
      mode = kUpb_AddUnknown_AliasAllowMerge;
    } else {
      mode = kUpb_AddUnknown_Alias;
    }
  }

  if (!UPB_PRIVATE(_upb_Message_AddUnknown)(msg, sv.data, sv.size, &d->arena,
                                            mode)) {
    upb_ErrorHandler_ThrowError(&d->err, kUpb_DecodeStatus_OutOfMemory);
  }

  return ptr;
}

UPB_FORCEINLINE
const char* _upb_Decoder_DecodeFieldTag(upb_Decoder* d, const char* ptr,
                                        uint32_t* field_number,
                                        uint32_t* wire_type) {
#ifndef NDEBUG
  d->debug_tagstart = ptr;
#endif

  uint32_t tag;
  UPB_ASSERT(ptr < d->input.limit_ptr);
  ptr = upb_WireReader_ReadTag(ptr, &tag, EPS(d));
  *field_number = tag >> 3;
  *wire_type = tag & 7;
  return ptr;
}

UPB_FORCEINLINE
const char* _upb_Decoder_DecodeFieldData(upb_Decoder* d, const char* ptr,
                                         upb_Message* msg,
                                         const upb_MiniTable* mt,
                                         uint32_t field_number,
                                         uint32_t wire_type) {
#ifndef NDEBUG
  d->debug_valstart = ptr;
#endif

  int op;
  wireval val;

  const upb_MiniTableField* field =
      _upb_Decoder_FindField(d, mt, field_number, wire_type);
  ptr = _upb_Decoder_DecodeWireValue(d, ptr, mt, field, wire_type, &val, &op);

  if (op >= 0) {
    return _upb_Decoder_DecodeKnownField(d, ptr, msg, mt, field, op, &val);
  } else {
    switch (op) {
      case kUpb_DecodeOp_UnknownField:
        return _upb_Decoder_DecodeUnknownField(d, ptr, msg, field_number,
                                               wire_type, val);
      case kUpb_DecodeOp_MessageSetItem:
        return upb_Decoder_DecodeMessageSetItem(d, ptr, msg, mt);
      default:
        UPB_UNREACHABLE();
    }
  }
}

static const char* _upb_Decoder_EndMessage(upb_Decoder* d, const char* ptr) {
  d->message_is_done = true;
  return ptr;
}

UPB_FORCEINLINE
const char* _upb_Decoder_DecodeFieldNoFast(upb_Decoder* d, const char* ptr,
                                           upb_Message* msg,
                                           const upb_MiniTable* mt) {
  uint32_t field_number;
  uint32_t wire_type;

  ptr = _upb_Decoder_DecodeFieldTag(d, ptr, &field_number, &wire_type);

  if (wire_type == kUpb_WireType_EndGroup) {
    d->end_group = field_number;
    return _upb_Decoder_EndMessage(d, ptr);
  }

  ptr = _upb_Decoder_DecodeFieldData(d, ptr, msg, mt, field_number, wire_type);
  _upb_Decoder_Trace(d, 'M');
  return ptr;
}

UPB_FORCEINLINE
bool _upb_Decoder_TryDecodeMessageFast(upb_Decoder* d, const char** ptr,
                                       upb_Message* msg,
                                       const upb_MiniTable* mt,
                                       uint64_t last_field_index,
                                       uint64_t data) {
#ifdef UPB_ENABLE_FASTTABLE
  if (mt->UPB_PRIVATE(table_mask) == (unsigned char)-1 ||
      (d->options & kUpb_DecodeOption_DisableFastTable)) {
    // Fast table is unavailable or disabled.
    return false;
  }

  intptr_t table = decode_totable(mt);
  const char* start = *ptr;
  char* trace_next = _upb_Decoder_TraceNext(d);

  *ptr = upb_DecodeFast_Dispatch(d, *ptr, msg, table, 0, 0);

  if (d->message_is_done) {
    // The entire message was successfully parsed fast.
    return true;
  }

  // *ptr now points to the beginning of a field that could not be parsed fast.
  // It's possible that some fields were parsed fast, in which case *ptr will
  // have been updated. However, it's also possible that the very first field
  // encountered could not be parsed fast, in which case *ptr will be unchanged.
  //
  // If the fast decoder consumed any data, it must have emitted at least
  // one 'F' event into the trace buffer (in addition to the 'D' event
  // that is always emitted).
  UPB_ASSERT(_upb_Decoder_TracePtr(d) != trace_next || *ptr == start);
  _upb_Decoder_Trace(d, '<');
#endif
  return false;
}

UPB_FORCEINLINE
const char* _upb_Decoder_DecodeField(upb_Decoder* d, const char* ptr,
                                     upb_Message* msg, const upb_MiniTable* mt,
                                     uint64_t last_field_index, uint64_t data) {
  if (_upb_Decoder_TryDecodeMessageFast(d, &ptr, msg, mt, last_field_index,
                                        data)) {
    return ptr;
  } else if (upb_EpsCopyInputStream_IsDone(EPS(d), &ptr)) {
    return _upb_Decoder_EndMessage(d, ptr);
  }

  return _upb_Decoder_DecodeFieldNoFast(d, ptr, msg, mt);
}

UPB_NOINLINE
const char* _upb_Decoder_DecodeMessage(upb_Decoder* d, const char* ptr,
                                       upb_Message* msg,
                                       const upb_MiniTable* mt) {
  UPB_ASSERT(mt);
  UPB_ASSERT(d->message_is_done == false);

  do {
    ptr = _upb_Decoder_DecodeField(d, ptr, msg, mt, 0, 0);
  } while (!d->message_is_done);
  d->message_is_done = false;

  return UPB_UNLIKELY(mt && mt->UPB_PRIVATE(required_count))
             ? _upb_Decoder_CheckRequired(d, ptr, msg, mt)
             : ptr;
}

static upb_DecodeStatus _upb_Decoder_DecodeTop(struct upb_Decoder* d,
                                               const char* buf,
                                               upb_Message* msg,
                                               const upb_MiniTable* m) {
  _upb_Decoder_DecodeMessage(d, buf, msg, m);
  if (d->end_group != DECODE_NOGROUP) return kUpb_DecodeStatus_Malformed;
  if (d->missing_required) return kUpb_DecodeStatus_MissingRequired;
  return kUpb_DecodeStatus_Ok;
}

static upb_DecodeStatus upb_Decoder_Decode(upb_Decoder* const decoder,
                                           const char* const buf,
                                           upb_Message* const msg,
                                           const upb_MiniTable* const m,
                                           upb_Arena* const arena) {
  if (UPB_SETJMP(decoder->err.buf) == 0) {
    decoder->err.code = _upb_Decoder_DecodeTop(decoder, buf, msg, m);
  } else {
    UPB_ASSERT(decoder->err.code != kUpb_DecodeStatus_Ok);
  }

  return upb_Decoder_Destroy(decoder, arena);
}

static uint16_t upb_DecodeOptions_GetMaxDepth(uint32_t options) {
  return options >> 16;
}

uint16_t upb_DecodeOptions_GetEffectiveMaxDepth(uint32_t options) {
  uint16_t max_depth = upb_DecodeOptions_GetMaxDepth(options);
  return max_depth ? max_depth : kUpb_WireFormat_DefaultDepthLimit;
}

upb_DecodeStatus upb_Decode(const char* buf, size_t size, upb_Message* msg,
                            const upb_MiniTable* mt,
                            const upb_ExtensionRegistry* extreg, int options,
                            upb_Arena* arena) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  upb_Decoder decoder;
  buf = upb_Decoder_Init(&decoder, buf, size, extreg, options, arena, NULL, 0);

  return upb_Decoder_Decode(&decoder, buf, msg, mt, arena);
}

upb_DecodeStatus upb_DecodeWithTrace(const char* buf, size_t size,
                                     upb_Message* msg, const upb_MiniTable* mt,
                                     const upb_ExtensionRegistry* extreg,
                                     int options, upb_Arena* arena,
                                     char* trace_buf, size_t trace_size) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  upb_Decoder decoder;
  buf = upb_Decoder_Init(&decoder, buf, size, extreg, options, arena, trace_buf,
                         trace_size);

  return upb_Decoder_Decode(&decoder, buf, msg, mt, arena);
}

upb_DecodeStatus upb_DecodeLengthPrefixed(const char* buf, size_t size,
                                          upb_Message* msg,
                                          size_t* num_bytes_read,
                                          const upb_MiniTable* mt,
                                          const upb_ExtensionRegistry* extreg,
                                          int options, upb_Arena* arena) {
  // To avoid needing to make a Decoder just to decode the initial length,
  // hand-decode the leading varint for the message length here.
  uint64_t msg_len = 0;
  for (size_t i = 0;; ++i) {
    if (i >= size || i > 9) {
      return kUpb_DecodeStatus_Malformed;
    }
    uint64_t b = *buf;
    buf++;
    msg_len += (b & 0x7f) << (i * 7);
    if ((b & 0x80) == 0) {
      *num_bytes_read = i + 1 + msg_len;
      break;
    }
  }

  // If the total number of bytes we would read (= the bytes from the varint
  // plus however many bytes that varint says we should read) is larger then the
  // input buffer then error as malformed.
  if (*num_bytes_read > size) {
    return kUpb_DecodeStatus_Malformed;
  }
  if (msg_len > INT32_MAX) {
    return kUpb_DecodeStatus_Malformed;
  }

  return upb_Decode(buf, msg_len, msg, mt, extreg, options, arena);
}

const char* upb_DecodeStatus_String(upb_DecodeStatus status) {
  switch (status) {
    case kUpb_DecodeStatus_Ok:
      return "Ok";
    case kUpb_DecodeStatus_Malformed:
      return "Wire format was corrupt";
    case kUpb_DecodeStatus_OutOfMemory:
      return "Arena alloc failed";
    case kUpb_DecodeStatus_BadUtf8:
      return "String field had bad UTF-8";
    case kUpb_DecodeStatus_MaxDepthExceeded:
      return "Exceeded upb_DecodeOptions_MaxDepth";
    case kUpb_DecodeStatus_MissingRequired:
      return "Missing required field";
    default:
      return "Unknown decode status";
  }
}

#undef OP_FIXPCK_LG2
#undef OP_VARPCK_LG2
