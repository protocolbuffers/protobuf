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

#include "upb/decode.h"

#include <string.h>

#include "upb/internal/array.h"
#include "upb/internal/decode.h"
#include "upb/msg_internal.h"
#include "upb/upb.h"

// Must be last.
#include "upb/port_def.inc"

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
  kUpb_DecodeOp_Enum = 1,

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

static const char* _upb_Decoder_DecodeMessage(upb_Decoder* d, const char* ptr,
                                              upb_Message* msg,
                                              const upb_MiniTable* layout);

UPB_NORETURN static void* _upb_Decoder_ErrorJmp(upb_Decoder* d,
                                                upb_DecodeStatus status) {
  assert(status != kUpb_DecodeStatus_Ok);
  UPB_LONGJMP(d->err, status);
}

const char* _upb_FastDecoder_ErrorJmp(upb_Decoder* d, int status) {
  assert(status != kUpb_DecodeStatus_Ok);
  UPB_LONGJMP(d->err, status);
  return NULL;
}
static void _upb_Decoder_VerifyUtf8(upb_Decoder* d, const char* buf, int len) {
  if (!_upb_Decoder_VerifyUtf8Inline(buf, len)) {
    _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_BadUtf8);
  }
}

static bool _upb_Decoder_Reserve(upb_Decoder* d, upb_Array* arr, size_t elem) {
  bool need_realloc = arr->capacity - arr->size < elem;
  if (need_realloc && !_upb_array_realloc(arr, arr->size + elem, &d->arena)) {
    _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
  }
  return need_realloc;
}

typedef struct {
  const char* ptr;
  uint64_t val;
} _upb_DecodeLongVarintReturn;

UPB_NOINLINE
static _upb_DecodeLongVarintReturn _upb_Decoder_DecodeLongVarint(
    const char* ptr, uint64_t val) {
  _upb_DecodeLongVarintReturn ret = {NULL, 0};
  uint64_t byte;
  int i;
  for (i = 1; i < 10; i++) {
    byte = (uint8_t)ptr[i];
    val += (byte - 1) << (i * 7);
    if (!(byte & 0x80)) {
      ret.ptr = ptr + i + 1;
      ret.val = val;
      return ret;
    }
  }
  return ret;
}

UPB_FORCEINLINE
static const char* _upb_Decoder_DecodeVarint(upb_Decoder* d, const char* ptr,
                                             uint64_t* val) {
  uint64_t byte = (uint8_t)*ptr;
  if (UPB_LIKELY((byte & 0x80) == 0)) {
    *val = byte;
    return ptr + 1;
  } else {
    _upb_DecodeLongVarintReturn res = _upb_Decoder_DecodeLongVarint(ptr, byte);
    if (!res.ptr) _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
    *val = res.val;
    return res.ptr;
  }
}

UPB_FORCEINLINE
static const char* _upb_Decoder_DecodeTag(upb_Decoder* d, const char* ptr,
                                          uint32_t* val) {
  uint64_t byte = (uint8_t)*ptr;
  if (UPB_LIKELY((byte & 0x80) == 0)) {
    *val = byte;
    return ptr + 1;
  } else {
    const char* start = ptr;
    _upb_DecodeLongVarintReturn res = _upb_Decoder_DecodeLongVarint(ptr, byte);
    if (!res.ptr || res.ptr - start > 5 || res.val > UINT32_MAX) {
      _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
    }
    *val = res.val;
    return res.ptr;
  }
}

UPB_FORCEINLINE
static const char* upb_Decoder_DecodeSize(upb_Decoder* d, const char* ptr,
                                          uint32_t* size) {
  uint64_t size64;
  ptr = _upb_Decoder_DecodeVarint(d, ptr, &size64);
  if (size64 >= INT32_MAX || ptr - d->end + (int)size64 > d->limit) {
    _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
  }
  *size = size64;
  return ptr;
}

static void _upb_Decoder_MungeInt32(wireval* val) {
  if (!_upb_IsLittleEndian()) {
    /* The next stage will memcpy(dst, &val, 4) */
    val->uint32_val = val->uint64_val;
  }
}

static void _upb_Decoder_Munge(int type, wireval* val) {
  switch (type) {
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
    case kUpb_FieldType_Enum:
      _upb_Decoder_MungeInt32(val);
      break;
  }
}

static upb_Message* _upb_Decoder_NewSubMessage(
    upb_Decoder* d, const upb_MiniTable_Sub* subs,
    const upb_MiniTable_Field* field) {
  const upb_MiniTable* subl = subs[field->submsg_index].submsg;
  upb_Message* msg = _upb_Message_New_inl(subl, &d->arena);
  if (!msg) _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
  return msg;
}

UPB_NOINLINE
const char* _upb_Decoder_IsDoneFallback(upb_Decoder* d, const char* ptr,
                                        int overrun) {
  int status;
  ptr = _upb_Decoder_IsDoneFallbackInline(d, ptr, overrun, &status);
  if (ptr == NULL) _upb_Decoder_ErrorJmp(d, status);
  return ptr;
}

static const char* _upb_Decoder_ReadString(upb_Decoder* d, const char* ptr,
                                           int size, upb_StringView* str) {
  if (d->options & kUpb_DecodeOption_AliasString) {
    str->data = ptr;
  } else {
    char* data = upb_Arena_Malloc(&d->arena, size);
    if (!data) _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
    memcpy(data, ptr, size);
    str->data = data;
  }
  str->size = size;
  return ptr + size;
}

UPB_FORCEINLINE
static const char* _upb_Decoder_RecurseSubMessage(upb_Decoder* d,
                                                  const char* ptr,
                                                  upb_Message* submsg,
                                                  const upb_MiniTable* subl,
                                                  uint32_t expected_end_group) {
  if (--d->depth < 0) {
    _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_MaxDepthExceeded);
  }
  ptr = _upb_Decoder_DecodeMessage(d, ptr, submsg, subl);
  d->depth++;
  if (d->end_group != expected_end_group) {
    _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
  }
  return ptr;
}

UPB_FORCEINLINE
static const char* _upb_Decoder_DecodeSubMessage(
    upb_Decoder* d, const char* ptr, upb_Message* submsg,
    const upb_MiniTable_Sub* subs, const upb_MiniTable_Field* field, int size) {
  int saved_delta = _upb_Decoder_PushLimit(d, ptr, size);
  const upb_MiniTable* subl = subs[field->submsg_index].submsg;
  ptr = _upb_Decoder_RecurseSubMessage(d, ptr, submsg, subl, DECODE_NOGROUP);
  _upb_Decoder_PopLimit(d, ptr, saved_delta);
  return ptr;
}

UPB_FORCEINLINE
static const char* _upb_Decoder_DecodeGroup(upb_Decoder* d, const char* ptr,
                                            upb_Message* submsg,
                                            const upb_MiniTable* subl,
                                            uint32_t number) {
  if (_upb_Decoder_IsDone(d, &ptr)) {
    _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
  }
  ptr = _upb_Decoder_RecurseSubMessage(d, ptr, submsg, subl, number);
  d->end_group = DECODE_NOGROUP;
  return ptr;
}

UPB_FORCEINLINE
static const char* _upb_Decoder_DecodeUnknownGroup(upb_Decoder* d,
                                                   const char* ptr,
                                                   uint32_t number) {
  return _upb_Decoder_DecodeGroup(d, ptr, NULL, NULL, number);
}

UPB_FORCEINLINE
static const char* _upb_Decoder_DecodeKnownGroup(
    upb_Decoder* d, const char* ptr, upb_Message* submsg,
    const upb_MiniTable_Sub* subs, const upb_MiniTable_Field* field) {
  const upb_MiniTable* subl = subs[field->submsg_index].submsg;
  return _upb_Decoder_DecodeGroup(d, ptr, submsg, subl, field->number);
}

static char* upb_Decoder_EncodeVarint32(uint32_t val, char* ptr) {
  do {
    uint8_t byte = val & 0x7fU;
    val >>= 7;
    if (val) byte |= 0x80U;
    *(ptr++) = byte;
  } while (val);
  return ptr;
}

static void _upb_Decoder_AddUnknownVarints(upb_Decoder* d, upb_Message* msg,
                                           uint32_t val1, uint32_t val2) {
  char buf[20];
  char* end = buf;
  end = upb_Decoder_EncodeVarint32(val1, end);
  end = upb_Decoder_EncodeVarint32(val2, end);

  if (!_upb_Message_AddUnknown(msg, buf, end - buf, &d->arena)) {
    _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
  }
}

UPB_NOINLINE
static bool _upb_Decoder_CheckEnumSlow(upb_Decoder* d, const char* ptr,
                                       upb_Message* msg,
                                       const upb_MiniTable_Enum* e,
                                       const upb_MiniTable_Field* field,
                                       uint32_t v) {
  if (_upb_MiniTable_CheckEnumValueSlow(e, v)) return true;

  // Unrecognized enum goes into unknown fields.
  // For packed fields the tag could be arbitrarily far in the past, so we
  // just re-encode the tag and value here.
  uint32_t tag = ((uint32_t)field->number << 3) | kUpb_WireType_Varint;
  upb_Message* unknown_msg =
      field->mode & kUpb_LabelFlags_IsExtension ? d->unknown_msg : msg;
  _upb_Decoder_AddUnknownVarints(d, unknown_msg, tag, v);
  return false;
}

UPB_FORCEINLINE
static bool _upb_Decoder_CheckEnum(upb_Decoder* d, const char* ptr,
                                   upb_Message* msg,
                                   const upb_MiniTable_Enum* e,
                                   const upb_MiniTable_Field* field,
                                   wireval* val) {
  uint32_t v = val->uint32_val;

  _kUpb_FastEnumCheck_Status status = _upb_MiniTable_CheckEnumValueFast(e, v);
  if (UPB_LIKELY(status == _kUpb_FastEnumCheck_ValueIsInEnum)) return true;
  return _upb_Decoder_CheckEnumSlow(d, ptr, msg, e, field, v);
}

UPB_NOINLINE
static const char* _upb_Decoder_DecodeEnumArray(
    upb_Decoder* d, const char* ptr, upb_Message* msg, upb_Array* arr,
    const upb_MiniTable_Sub* subs, const upb_MiniTable_Field* field,
    wireval* val) {
  const upb_MiniTable_Enum* e = subs[field->submsg_index].subenum;
  if (!_upb_Decoder_CheckEnum(d, ptr, msg, e, field, val)) return ptr;
  void* mem = UPB_PTR_AT(_upb_array_ptr(arr), arr->size * 4, void);
  arr->size++;
  memcpy(mem, val, 4);
  return ptr;
}

UPB_FORCEINLINE
static const char* _upb_Decoder_DecodeFixedPacked(
    upb_Decoder* d, const char* ptr, upb_Array* arr, wireval* val,
    const upb_MiniTable_Field* field, int lg2) {
  int mask = (1 << lg2) - 1;
  size_t count = val->size >> lg2;
  if ((val->size & mask) != 0) {
    // Length isn't a round multiple of elem size.
    _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
  }
  _upb_Decoder_Reserve(d, arr, count);
  void* mem = UPB_PTR_AT(_upb_array_ptr(arr), arr->size << lg2, void);
  arr->size += count;
  // Note: if/when the decoder supports multi-buffer input, we will need to
  // handle buffer seams here.
  if (_upb_IsLittleEndian()) {
    memcpy(mem, ptr, val->size);
    ptr += val->size;
  } else {
    const char* end = ptr + val->size;
    char* dst = mem;
    while (ptr < end) {
      if (lg2 == 2) {
        uint32_t val;
        memcpy(&val, ptr, sizeof(val));
        val = _upb_BigEndian_Swap32(val);
        memcpy(dst, &val, sizeof(val));
      } else {
        UPB_ASSERT(lg2 == 3);
        uint64_t val;
        memcpy(&val, ptr, sizeof(val));
        val = _upb_BigEndian_Swap64(val);
        memcpy(dst, &val, sizeof(val));
      }
      ptr += 1 << lg2;
      dst += 1 << lg2;
    }
  }

  return ptr;
}

UPB_FORCEINLINE
static const char* _upb_Decoder_DecodeVarintPacked(
    upb_Decoder* d, const char* ptr, upb_Array* arr, wireval* val,
    const upb_MiniTable_Field* field, int lg2) {
  int scale = 1 << lg2;
  int saved_limit = _upb_Decoder_PushLimit(d, ptr, val->size);
  char* out = UPB_PTR_AT(_upb_array_ptr(arr), arr->size << lg2, void);
  while (!_upb_Decoder_IsDone(d, &ptr)) {
    wireval elem;
    ptr = _upb_Decoder_DecodeVarint(d, ptr, &elem.uint64_val);
    _upb_Decoder_Munge(field->descriptortype, &elem);
    if (_upb_Decoder_Reserve(d, arr, 1)) {
      out = UPB_PTR_AT(_upb_array_ptr(arr), arr->size << lg2, void);
    }
    arr->size++;
    memcpy(out, &elem, scale);
    out += scale;
  }
  _upb_Decoder_PopLimit(d, ptr, saved_limit);
  return ptr;
}

UPB_NOINLINE
static const char* _upb_Decoder_DecodeEnumPacked(
    upb_Decoder* d, const char* ptr, upb_Message* msg, upb_Array* arr,
    const upb_MiniTable_Sub* subs, const upb_MiniTable_Field* field,
    wireval* val) {
  const upb_MiniTable_Enum* e = subs[field->submsg_index].subenum;
  int saved_limit = _upb_Decoder_PushLimit(d, ptr, val->size);
  char* out = UPB_PTR_AT(_upb_array_ptr(arr), arr->size * 4, void);
  while (!_upb_Decoder_IsDone(d, &ptr)) {
    wireval elem;
    ptr = _upb_Decoder_DecodeVarint(d, ptr, &elem.uint64_val);
    _upb_Decoder_MungeInt32(&elem);
    if (!_upb_Decoder_CheckEnum(d, ptr, msg, e, field, &elem)) {
      continue;
    }
    if (_upb_Decoder_Reserve(d, arr, 1)) {
      out = UPB_PTR_AT(_upb_array_ptr(arr), arr->size * 4, void);
    }
    arr->size++;
    memcpy(out, &elem, 4);
    out += 4;
  }
  _upb_Decoder_PopLimit(d, ptr, saved_limit);
  return ptr;
}

upb_Array* _upb_Decoder_CreateArray(upb_Decoder* d,
                                    const upb_MiniTable_Field* field) {
  /* Maps descriptor type -> elem_size_lg2.  */
  static const uint8_t kElemSizeLg2[] = {
      [0] = -1,  // invalid descriptor type
      [kUpb_FieldType_Double] = 3,
      [kUpb_FieldType_Float] = 2,
      [kUpb_FieldType_Int64] = 3,
      [kUpb_FieldType_UInt64] = 3,
      [kUpb_FieldType_Int32] = 2,
      [kUpb_FieldType_Fixed64] = 3,
      [kUpb_FieldType_Fixed32] = 2,
      [kUpb_FieldType_Bool] = 0,
      [kUpb_FieldType_String] = UPB_SIZE(3, 4),
      [kUpb_FieldType_Group] = UPB_SIZE(2, 3),
      [kUpb_FieldType_Message] = UPB_SIZE(2, 3),
      [kUpb_FieldType_Bytes] = UPB_SIZE(3, 4),
      [kUpb_FieldType_UInt32] = 2,
      [kUpb_FieldType_Enum] = 2,
      [kUpb_FieldType_SFixed32] = 2,
      [kUpb_FieldType_SFixed64] = 3,
      [kUpb_FieldType_SInt32] = 2,
      [kUpb_FieldType_SInt64] = 3,
  };

  size_t lg2 = kElemSizeLg2[field->descriptortype];
  upb_Array* ret = _upb_Array_New(&d->arena, 4, lg2);
  if (!ret) _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
  return ret;
}

static const char* _upb_Decoder_DecodeToArray(upb_Decoder* d, const char* ptr,
                                              upb_Message* msg,
                                              const upb_MiniTable_Sub* subs,
                                              const upb_MiniTable_Field* field,
                                              wireval* val, int op) {
  upb_Array** arrp = UPB_PTR_AT(msg, field->offset, void);
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
      mem = UPB_PTR_AT(_upb_array_ptr(arr), arr->size << op, void);
      arr->size++;
      memcpy(mem, val, 1 << op);
      return ptr;
    case kUpb_DecodeOp_String:
      _upb_Decoder_VerifyUtf8(d, ptr, val->size);
      /* Fallthrough. */
    case kUpb_DecodeOp_Bytes: {
      /* Append bytes. */
      upb_StringView* str = (upb_StringView*)_upb_array_ptr(arr) + arr->size;
      arr->size++;
      return _upb_Decoder_ReadString(d, ptr, val->size, str);
    }
    case kUpb_DecodeOp_SubMessage: {
      /* Append submessage / group. */
      upb_Message* submsg = _upb_Decoder_NewSubMessage(d, subs, field);
      *UPB_PTR_AT(_upb_array_ptr(arr), arr->size * sizeof(void*),
                  upb_Message*) = submsg;
      arr->size++;
      if (UPB_UNLIKELY(field->descriptortype == kUpb_FieldType_Group)) {
        return _upb_Decoder_DecodeKnownGroup(d, ptr, submsg, subs, field);
      } else {
        return _upb_Decoder_DecodeSubMessage(d, ptr, submsg, subs, field,
                                             val->size);
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
    case kUpb_DecodeOp_Enum:
      return _upb_Decoder_DecodeEnumArray(d, ptr, msg, arr, subs, field, val);
    case kUpb_DecodeOp_PackedEnum:
      return _upb_Decoder_DecodeEnumPacked(d, ptr, msg, arr, subs, field, val);
    default:
      UPB_UNREACHABLE();
  }
}

upb_Map* _upb_Decoder_CreateMap(upb_Decoder* d, const upb_MiniTable* entry) {
  /* Maps descriptor type -> upb map size.  */
  static const uint8_t kSizeInMap[] = {
      [0] = -1,  // invalid descriptor type */
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

  const upb_MiniTable_Field* key_field = &entry->fields[0];
  const upb_MiniTable_Field* val_field = &entry->fields[1];
  char key_size = kSizeInMap[key_field->descriptortype];
  char val_size = kSizeInMap[val_field->descriptortype];
  UPB_ASSERT(key_field->offset == 0);
  UPB_ASSERT(val_field->offset == sizeof(upb_StringView));
  upb_Map* ret = _upb_Map_New(&d->arena, key_size, val_size);
  if (!ret) _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
  return ret;
}

static const char* _upb_Decoder_DecodeToMap(upb_Decoder* d, const char* ptr,
                                            upb_Message* msg,
                                            const upb_MiniTable_Sub* subs,
                                            const upb_MiniTable_Field* field,
                                            wireval* val) {
  upb_Map** map_p = UPB_PTR_AT(msg, field->offset, upb_Map*);
  upb_Map* map = *map_p;
  upb_MapEntry ent;
  const upb_MiniTable* entry = subs[field->submsg_index].submsg;

  if (!map) {
    map = _upb_Decoder_CreateMap(d, entry);
    *map_p = map;
  }

  /* Parse map entry. */
  memset(&ent, 0, sizeof(ent));

  if (entry->fields[1].descriptortype == kUpb_FieldType_Message ||
      entry->fields[1].descriptortype == kUpb_FieldType_Group) {
    /* Create proactively to handle the case where it doesn't appear. */
    ent.v.val =
        upb_value_ptr(_upb_Message_New(entry->subs[0].submsg, &d->arena));
  }

  const char* start = ptr;
  ptr = _upb_Decoder_DecodeSubMessage(d, ptr, &ent.k, subs, field, val->size);
  // check if ent had any unknown fields
  size_t size;
  upb_Message_GetUnknown(&ent.k, &size);
  if (size != 0) {
    uint32_t tag = ((uint32_t)field->number << 3) | kUpb_WireType_Delimited;
    _upb_Decoder_AddUnknownVarints(d, msg, tag, (uint32_t)(ptr - start));
    if (!_upb_Message_AddUnknown(msg, start, ptr - start, &d->arena)) {
      _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
    }
  } else {
    if (_upb_Map_Insert(map, &ent.k, map->key_size, &ent.v, map->val_size,
                        &d->arena) == _kUpb_MapInsertStatus_OutOfMemory) {
      _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
    }
  }
  return ptr;
}

static const char* _upb_Decoder_DecodeToSubMessage(
    upb_Decoder* d, const char* ptr, upb_Message* msg,
    const upb_MiniTable_Sub* subs, const upb_MiniTable_Field* field,
    wireval* val, int op) {
  void* mem = UPB_PTR_AT(msg, field->offset, void);
  int type = field->descriptortype;

  if (UPB_UNLIKELY(op == kUpb_DecodeOp_Enum) &&
      !_upb_Decoder_CheckEnum(d, ptr, msg, subs[field->submsg_index].subenum,
                              field, val)) {
    return ptr;
  }

  /* Set presence if necessary. */
  if (field->presence > 0) {
    _upb_sethas_field(msg, field);
  } else if (field->presence < 0) {
    /* Oneof case */
    uint32_t* oneof_case = _upb_oneofcase_field(msg, field);
    if (op == kUpb_DecodeOp_SubMessage && *oneof_case != field->number) {
      memset(mem, 0, sizeof(void*));
    }
    *oneof_case = field->number;
  }

  /* Store into message. */
  switch (op) {
    case kUpb_DecodeOp_SubMessage: {
      upb_Message** submsgp = mem;
      upb_Message* submsg = *submsgp;
      if (!submsg) {
        submsg = _upb_Decoder_NewSubMessage(d, subs, field);
        *submsgp = submsg;
      }
      if (UPB_UNLIKELY(type == kUpb_FieldType_Group)) {
        ptr = _upb_Decoder_DecodeKnownGroup(d, ptr, submsg, subs, field);
      } else {
        ptr = _upb_Decoder_DecodeSubMessage(d, ptr, submsg, subs, field,
                                            val->size);
      }
      break;
    }
    case kUpb_DecodeOp_String:
      _upb_Decoder_VerifyUtf8(d, ptr, val->size);
      /* Fallthrough. */
    case kUpb_DecodeOp_Bytes:
      return _upb_Decoder_ReadString(d, ptr, val->size, mem);
    case kUpb_DecodeOp_Scalar8Byte:
      memcpy(mem, val, 8);
      break;
    case kUpb_DecodeOp_Enum:
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

UPB_NOINLINE
const char* _upb_Decoder_CheckRequired(upb_Decoder* d, const char* ptr,
                                       const upb_Message* msg,
                                       const upb_MiniTable* l) {
  assert(l->required_count);
  if (UPB_LIKELY((d->options & kUpb_DecodeOption_CheckRequired) == 0)) {
    return ptr;
  }
  uint64_t msg_head;
  memcpy(&msg_head, msg, 8);
  msg_head = _upb_BigEndian_Swap64(msg_head);
  if (upb_MiniTable_requiredmask(l) & ~msg_head) {
    d->missing_required = true;
  }
  return ptr;
}

UPB_FORCEINLINE
static bool _upb_Decoder_TryFastDispatch(upb_Decoder* d, const char** ptr,
                                         upb_Message* msg,
                                         const upb_MiniTable* layout) {
#if UPB_FASTTABLE
  if (layout && layout->table_mask != (unsigned char)-1) {
    uint16_t tag = _upb_FastDecoder_LoadTag(*ptr);
    intptr_t table = decode_totable(layout);
    *ptr = _upb_FastDecoder_TagDispatch(d, *ptr, msg, table, 0, tag);
    return true;
  }
#endif
  return false;
}

static const char* upb_Decoder_SkipField(upb_Decoder* d, const char* ptr,
                                         uint32_t tag) {
  int field_number = tag >> 3;
  int wire_type = tag & 7;
  switch (wire_type) {
    case kUpb_WireType_Varint: {
      uint64_t val;
      return _upb_Decoder_DecodeVarint(d, ptr, &val);
    }
    case kUpb_WireType_64Bit:
      return ptr + 8;
    case kUpb_WireType_32Bit:
      return ptr + 4;
    case kUpb_WireType_Delimited: {
      uint32_t size;
      ptr = upb_Decoder_DecodeSize(d, ptr, &size);
      return ptr + size;
    }
    case kUpb_WireType_StartGroup:
      return _upb_Decoder_DecodeUnknownGroup(d, ptr, field_number);
    default:
      _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
  }
}

enum {
  kStartItemTag = ((1 << 3) | kUpb_WireType_StartGroup),
  kEndItemTag = ((1 << 3) | kUpb_WireType_EndGroup),
  kTypeIdTag = ((2 << 3) | kUpb_WireType_Varint),
  kMessageTag = ((3 << 3) | kUpb_WireType_Delimited),
};

static void upb_Decoder_AddKnownMessageSetItem(
    upb_Decoder* d, upb_Message* msg, const upb_MiniTable_Extension* item_mt,
    const char* data, uint32_t size) {
  upb_Message_Extension* ext =
      _upb_Message_GetOrCreateExtension(msg, item_mt, &d->arena);
  if (UPB_UNLIKELY(!ext)) {
    _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
  }
  upb_Message* submsg =
      _upb_Decoder_NewSubMessage(d, &ext->ext->sub, &ext->ext->field);
  upb_DecodeStatus status = upb_Decode(data, size, submsg, item_mt->sub.submsg,
                                       d->extreg, d->options, &d->arena);
  memcpy(&ext->data, &submsg, sizeof(submsg));
  if (status != kUpb_DecodeStatus_Ok) _upb_Decoder_ErrorJmp(d, status);
}

static void upb_Decoder_AddUnknownMessageSetItem(upb_Decoder* d,
                                                 upb_Message* msg,
                                                 uint32_t type_id,
                                                 const char* message_data,
                                                 uint32_t message_size) {
  char buf[60];
  char* ptr = buf;
  ptr = upb_Decoder_EncodeVarint32(kStartItemTag, ptr);
  ptr = upb_Decoder_EncodeVarint32(kTypeIdTag, ptr);
  ptr = upb_Decoder_EncodeVarint32(type_id, ptr);
  ptr = upb_Decoder_EncodeVarint32(kMessageTag, ptr);
  ptr = upb_Decoder_EncodeVarint32(message_size, ptr);
  char* split = ptr;

  ptr = upb_Decoder_EncodeVarint32(kEndItemTag, ptr);
  char* end = ptr;

  if (!_upb_Message_AddUnknown(msg, buf, split - buf, &d->arena) ||
      !_upb_Message_AddUnknown(msg, message_data, message_size, &d->arena) ||
      !_upb_Message_AddUnknown(msg, split, end - split, &d->arena)) {
    _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
  }
}

static void upb_Decoder_AddMessageSetItem(upb_Decoder* d, upb_Message* msg,
                                          const upb_MiniTable* layout,
                                          uint32_t type_id, const char* data,
                                          uint32_t size) {
  const upb_MiniTable_Extension* item_mt =
      _upb_extreg_get(d->extreg, layout, type_id);
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
  while (!_upb_Decoder_IsDone(d, &ptr)) {
    uint32_t tag;
    ptr = _upb_Decoder_DecodeTag(d, ptr, &tag);
    switch (tag) {
      case kEndItemTag:
        return ptr;
      case kTypeIdTag: {
        uint64_t tmp;
        ptr = _upb_Decoder_DecodeVarint(d, ptr, &tmp);
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
        ptr = upb_Decoder_DecodeSize(d, ptr, &size);
        const char* data = ptr;
        ptr += size;
        if (state_mask & kUpb_HavePayload) break;  // Ignore dup.
        state_mask |= kUpb_HavePayload;
        if (state_mask & kUpb_HaveId) {
          upb_Decoder_AddMessageSetItem(d, msg, layout, type_id, data, size);
        } else {
          // Out of order, we must preserve the payload.
          preserved.data = data;
          preserved.size = size;
        }
        break;
      }
      default:
        // We do not preserve unexpected fields inside a message set item.
        ptr = upb_Decoder_SkipField(d, ptr, tag);
        break;
    }
  }
  _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
}

static const upb_MiniTable_Field* _upb_Decoder_FindField(
    upb_Decoder* d, const upb_MiniTable* l, uint32_t field_number,
    int* last_field_index) {
  static upb_MiniTable_Field none = {
      0, 0, 0, 0, kUpb_FakeFieldType_FieldNotFound, 0};
  if (l == NULL) return &none;

  size_t idx = ((size_t)field_number) - 1;  // 0 wraps to SIZE_MAX
  if (idx < l->dense_below) {
    /* Fastest case: index into dense fields. */
    goto found;
  }

  if (l->dense_below < l->field_count) {
    /* Linear search non-dense fields. Resume scanning from last_field_index
     * since fields are usually in order. */
    int last = *last_field_index;
    for (idx = last; idx < l->field_count; idx++) {
      if (l->fields[idx].number == field_number) {
        goto found;
      }
    }

    for (idx = l->dense_below; idx < last; idx++) {
      if (l->fields[idx].number == field_number) {
        goto found;
      }
    }
  }

  if (d->extreg) {
    switch (l->ext) {
      case kUpb_ExtMode_Extendable: {
        const upb_MiniTable_Extension* ext =
            _upb_extreg_get(d->extreg, l, field_number);
        if (ext) return &ext->field;
        break;
      }
      case kUpb_ExtMode_IsMessageSet:
        if (field_number == _UPB_MSGSET_ITEM) {
          static upb_MiniTable_Field item = {
              0, 0, 0, 0, kUpb_FakeFieldType_MessageSetItem, 0};
          return &item;
        }
        break;
    }
  }

  return &none; /* Unknown field. */

found:
  UPB_ASSERT(l->fields[idx].number == field_number);
  *last_field_index = idx;
  return &l->fields[idx];
}

int _upb_Decoder_GetVarintOp(const upb_MiniTable_Field* field) {
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
      [kUpb_FieldType_Enum] = kUpb_DecodeOp_Enum,
      [kUpb_FieldType_SFixed32] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_SFixed64] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_SInt32] = kUpb_DecodeOp_Scalar4Byte,
      [kUpb_FieldType_SInt64] = kUpb_DecodeOp_Scalar8Byte,
      [kUpb_FakeFieldType_MessageSetItem] = kUpb_DecodeOp_UnknownField,
  };

  return kVarintOps[field->descriptortype];
}

int _upb_Decoder_GetDelimitedOp(const upb_MiniTable_Field* field) {
  enum { kRepeatedBase = 19 };

  static const int8_t kDelimitedOps[] = {
      /* For non-repeated field type. */
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
      [kUpb_FakeFieldType_MessageSetItem] = kUpb_DecodeOp_UnknownField,
      // For repeated field type. */
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

  int ndx = field->descriptortype;
  if (upb_FieldMode_Get(field) == kUpb_FieldMode_Array) ndx += kRepeatedBase;
  return kDelimitedOps[ndx];
}

UPB_FORCEINLINE
static const char* _upb_Decoder_DecodeWireValue(
    upb_Decoder* d, const char* ptr, const upb_MiniTable_Field* field,
    int wire_type, wireval* val, int* op) {
  static const unsigned kFixed32OkMask = (1 << kUpb_FieldType_Float) |
                                         (1 << kUpb_FieldType_Fixed32) |
                                         (1 << kUpb_FieldType_SFixed32);

  static const unsigned kFixed64OkMask = (1 << kUpb_FieldType_Double) |
                                         (1 << kUpb_FieldType_Fixed64) |
                                         (1 << kUpb_FieldType_SFixed64);

  switch (wire_type) {
    case kUpb_WireType_Varint:
      ptr = _upb_Decoder_DecodeVarint(d, ptr, &val->uint64_val);
      *op = _upb_Decoder_GetVarintOp(field);
      _upb_Decoder_Munge(field->descriptortype, val);
      return ptr;
    case kUpb_WireType_32Bit:
      memcpy(&val->uint32_val, ptr, 4);
      val->uint32_val = _upb_BigEndian_Swap32(val->uint32_val);
      *op = kUpb_DecodeOp_Scalar4Byte;
      if (((1 << field->descriptortype) & kFixed32OkMask) == 0) {
        *op = kUpb_DecodeOp_UnknownField;
      }
      return ptr + 4;
    case kUpb_WireType_64Bit:
      memcpy(&val->uint64_val, ptr, 8);
      val->uint64_val = _upb_BigEndian_Swap64(val->uint64_val);
      *op = kUpb_DecodeOp_Scalar8Byte;
      if (((1 << field->descriptortype) & kFixed64OkMask) == 0) {
        *op = kUpb_DecodeOp_UnknownField;
      }
      return ptr + 8;
    case kUpb_WireType_Delimited:
      ptr = upb_Decoder_DecodeSize(d, ptr, &val->size);
      *op = _upb_Decoder_GetDelimitedOp(field);
      return ptr;
    case kUpb_WireType_StartGroup:
      val->uint32_val = field->number;
      if (field->descriptortype == kUpb_FieldType_Group) {
        *op = kUpb_DecodeOp_SubMessage;
      } else if (field->descriptortype == kUpb_FakeFieldType_MessageSetItem) {
        *op = kUpb_DecodeOp_MessageSetItem;
      } else {
        *op = kUpb_DecodeOp_UnknownField;
      }
      return ptr;
    default:
      break;
  }
  _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
}

UPB_FORCEINLINE
static const char* _upb_Decoder_DecodeKnownField(
    upb_Decoder* d, const char* ptr, upb_Message* msg,
    const upb_MiniTable* layout, const upb_MiniTable_Field* field, int op,
    wireval* val) {
  const upb_MiniTable_Sub* subs = layout->subs;
  uint8_t mode = field->mode;

  if (UPB_UNLIKELY(mode & kUpb_LabelFlags_IsExtension)) {
    const upb_MiniTable_Extension* ext_layout =
        (const upb_MiniTable_Extension*)field;
    upb_Message_Extension* ext =
        _upb_Message_GetOrCreateExtension(msg, ext_layout, &d->arena);
    if (UPB_UNLIKELY(!ext)) {
      _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
    }
    d->unknown_msg = msg;
    msg = &ext->data;
    subs = &ext->ext->sub;
  }

  switch (mode & kUpb_FieldMode_Mask) {
    case kUpb_FieldMode_Array:
      return _upb_Decoder_DecodeToArray(d, ptr, msg, subs, field, val, op);
    case kUpb_FieldMode_Map:
      return _upb_Decoder_DecodeToMap(d, ptr, msg, subs, field, val);
    case kUpb_FieldMode_Scalar:
      return _upb_Decoder_DecodeToSubMessage(d, ptr, msg, subs, field, val, op);
    default:
      UPB_UNREACHABLE();
  }
}

static const char* _upb_Decoder_ReverseSkipVarint(const char* ptr,
                                                  uint32_t val) {
  uint32_t seen = 0;
  do {
    ptr--;
    seen <<= 7;
    seen |= *ptr & 0x7f;
  } while (seen != val);
  return ptr;
}

static const char* _upb_Decoder_DecodeUnknownField(upb_Decoder* d,
                                                   const char* ptr,
                                                   upb_Message* msg,
                                                   int field_number,
                                                   int wire_type, wireval val) {
  if (field_number == 0) _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);

  // Since unknown fields are the uncommon case, we do a little extra work here
  // to walk backwards through the buffer to find the field start.  This frees
  // up a register in the fast paths (when the field is known), which leads to
  // significant speedups in benchmarks.
  const char* start = ptr;

  if (wire_type == kUpb_WireType_Delimited) ptr += val.size;
  if (msg) {
    switch (wire_type) {
      case kUpb_WireType_Varint:
      case kUpb_WireType_Delimited:
        start--;
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
    uint32_t tag = ((uint32_t)field_number << 3) | wire_type;
    start = _upb_Decoder_ReverseSkipVarint(start, tag);
    assert(start == d->debug_tagstart);

    if (wire_type == kUpb_WireType_StartGroup) {
      d->unknown = start;
      d->unknown_msg = msg;
      ptr = _upb_Decoder_DecodeUnknownGroup(d, ptr, field_number);
      start = d->unknown;
      d->unknown = NULL;
    }
    if (!_upb_Message_AddUnknown(msg, start, ptr - start, &d->arena)) {
      _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
    }
  } else if (wire_type == kUpb_WireType_StartGroup) {
    ptr = _upb_Decoder_DecodeUnknownGroup(d, ptr, field_number);
  }
  return ptr;
}

UPB_NOINLINE
static const char* _upb_Decoder_DecodeMessage(upb_Decoder* d, const char* ptr,
                                              upb_Message* msg,
                                              const upb_MiniTable* layout) {
  int last_field_index = 0;

#if UPB_FASTTABLE
  // The first time we want to skip fast dispatch, because we may have just been
  // invoked by the fast parser to handle a case that it bailed on.
  if (!_upb_Decoder_IsDone(d, &ptr)) goto nofast;
#endif

  while (!_upb_Decoder_IsDone(d, &ptr)) {
    uint32_t tag;
    const upb_MiniTable_Field* field;
    int field_number;
    int wire_type;
    wireval val;
    int op;

    if (_upb_Decoder_TryFastDispatch(d, &ptr, msg, layout)) break;

#if UPB_FASTTABLE
  nofast:
#endif

#ifndef NDEBUG
    d->debug_tagstart = ptr;
#endif

    UPB_ASSERT(ptr < d->limit_ptr);
    ptr = _upb_Decoder_DecodeTag(d, ptr, &tag);
    field_number = tag >> 3;
    wire_type = tag & 7;

#ifndef NDEBUG
    d->debug_valstart = ptr;
#endif

    if (wire_type == kUpb_WireType_EndGroup) {
      d->end_group = field_number;
      return ptr;
    }

    field = _upb_Decoder_FindField(d, layout, field_number, &last_field_index);
    ptr = _upb_Decoder_DecodeWireValue(d, ptr, field, wire_type, &val, &op);

    if (op >= 0) {
      ptr = _upb_Decoder_DecodeKnownField(d, ptr, msg, layout, field, op, &val);
    } else {
      switch (op) {
        case kUpb_DecodeOp_UnknownField:
          ptr = _upb_Decoder_DecodeUnknownField(d, ptr, msg, field_number,
                                                wire_type, val);
          break;
        case kUpb_DecodeOp_MessageSetItem:
          ptr = upb_Decoder_DecodeMessageSetItem(d, ptr, msg, layout);
          break;
      }
    }
  }

  return UPB_UNLIKELY(layout && layout->required_count)
             ? _upb_Decoder_CheckRequired(d, ptr, msg, layout)
             : ptr;
}

const char* _upb_FastDecoder_DecodeGeneric(struct upb_Decoder* d,
                                           const char* ptr, upb_Message* msg,
                                           intptr_t table, uint64_t hasbits,
                                           uint64_t data) {
  (void)data;
  *(uint32_t*)msg |= hasbits;
  return _upb_Decoder_DecodeMessage(d, ptr, msg, decode_totablep(table));
}

static upb_DecodeStatus _upb_Decoder_DecodeTop(struct upb_Decoder* d,
                                               const char* buf, void* msg,
                                               const upb_MiniTable* l) {
  if (!_upb_Decoder_TryFastDispatch(d, &buf, msg, l)) {
    _upb_Decoder_DecodeMessage(d, buf, msg, l);
  }
  if (d->end_group != DECODE_NOGROUP) return kUpb_DecodeStatus_Malformed;
  if (d->missing_required) return kUpb_DecodeStatus_MissingRequired;
  return kUpb_DecodeStatus_Ok;
}

upb_DecodeStatus upb_Decode(const char* buf, size_t size, void* msg,
                            const upb_MiniTable* l,
                            const upb_ExtensionRegistry* extreg, int options,
                            upb_Arena* arena) {
  upb_Decoder state;
  unsigned depth = (unsigned)options >> 16;

  if (size <= 16) {
    memset(&state.patch, 0, 32);
    if (size) memcpy(&state.patch, buf, size);
    buf = state.patch;
    state.end = buf + size;
    state.limit = 0;
    options &= ~kUpb_DecodeOption_AliasString;  // Can't alias patch buf.
  } else {
    state.end = buf + size - 16;
    state.limit = 16;
  }

  state.extreg = extreg;
  state.limit_ptr = state.end;
  state.unknown = NULL;
  state.depth = depth ? depth : 64;
  state.end_group = DECODE_NOGROUP;
  state.options = (uint16_t)options;
  state.missing_required = false;
  state.arena.head = arena->head;
  state.arena.last_size = arena->last_size;
  state.arena.cleanup_metadata = arena->cleanup_metadata;
  state.arena.parent = arena;

  upb_DecodeStatus status = UPB_SETJMP(state.err);
  if (UPB_LIKELY(status == kUpb_DecodeStatus_Ok)) {
    status = _upb_Decoder_DecodeTop(&state, buf, msg, l);
  }

  arena->head.ptr = state.arena.head.ptr;
  arena->head.end = state.arena.head.end;
  arena->cleanup_metadata = state.arena.cleanup_metadata;
  return status;
}

#undef OP_FIXPCK_LG2
#undef OP_VARPCK_LG2
