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
#include "upb/message/internal/tagged_ptr.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/message/tagged_ptr.h"
#include "upb/mini_table/enum.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/internal/size_log2.h"
#include "upb/mini_table/message.h"
#include "upb/mini_table/sub.h"
#include "upb/port/atomic.h"
#include "upb/wire/encode.h"
#include "upb/wire/eps_copy_input_stream.h"
#include "upb/wire/internal/constants.h"
#include "upb/wire/internal/decoder.h"
#include "upb/wire/reader.h"

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

// Ideally these two functions should take the owning MiniTable pointer as a
// first argument, then we could just put them in mini_table/message.h as nice
// clean getters. But we don't have that so instead we gotta write these
// Frankenfunctions that take an array of subtables.
// TODO: Move these to mini_table/ anyway since there are other places
// that could use them.

// Returns the MiniTable corresponding to a given MiniTableField
// from an array of MiniTableSubs.
static const upb_MiniTable* _upb_MiniTableSubs_MessageByField(
    const upb_MiniTableSub* subs, const upb_MiniTableField* field) {
  return upb_MiniTableSub_Message(subs[field->UPB_PRIVATE(submsg_index)]);
}

// Returns the MiniTableEnum corresponding to a given MiniTableField
// from an array of MiniTableSub.
static const upb_MiniTableEnum* _upb_MiniTableSubs_EnumByField(
    const upb_MiniTableSub* subs, const upb_MiniTableField* field) {
  return upb_MiniTableSub_Enum(subs[field->UPB_PRIVATE(submsg_index)]);
}

static const char* _upb_Decoder_DecodeMessage(upb_Decoder* d, const char* ptr,
                                              upb_Message* msg,
                                              const upb_MiniTable* layout);

UPB_NORETURN static void* _upb_Decoder_ErrorJmp(upb_Decoder* d,
                                                upb_DecodeStatus status) {
  UPB_ASSERT(status != kUpb_DecodeStatus_Ok);
  d->status = status;
  UPB_LONGJMP(d->err, 1);
}

const char* _upb_FastDecoder_ErrorJmp(upb_Decoder* d, int status) {
  UPB_ASSERT(status != kUpb_DecodeStatus_Ok);
  d->status = status;
  UPB_LONGJMP(d->err, 1);
  return NULL;
}

static void _upb_Decoder_VerifyUtf8(upb_Decoder* d, const char* buf, int len) {
  if (!_upb_Decoder_VerifyUtf8Inline(buf, len)) {
    _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_BadUtf8);
  }
}

static bool _upb_Decoder_Reserve(upb_Decoder* d, upb_Array* arr, size_t elem) {
  bool need_realloc =
      arr->UPB_PRIVATE(capacity) - arr->UPB_PRIVATE(size) < elem;
  if (need_realloc && !UPB_PRIVATE(_upb_Array_Realloc)(
                          arr, arr->UPB_PRIVATE(size) + elem, &d->arena)) {
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
  if (size64 >= INT32_MAX ||
      !upb_EpsCopyInputStream_CheckSize(&d->input, ptr, (int)size64)) {
    _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
  }
  *size = size64;
  return ptr;
}

static void _upb_Decoder_MungeInt32(wireval* val) {
  if (!upb_IsLittleEndian()) {
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

static upb_Message* _upb_Decoder_NewSubMessage(upb_Decoder* d,
                                               const upb_MiniTableSub* subs,
                                               const upb_MiniTableField* field,
                                               upb_TaggedMessagePtr* target) {
  const upb_MiniTable* subl = _upb_MiniTableSubs_MessageByField(subs, field);
  UPB_ASSERT(subl);
  upb_Message* msg = _upb_Message_New(subl, &d->arena);
  if (!msg) _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);

  // Extensions should not be unlinked. A message extension should not be
  // registered until its sub-message type is available to be linked.
  bool is_empty = UPB_PRIVATE(_upb_MiniTable_IsEmpty)(subl);
  bool is_extension = field->UPB_PRIVATE(mode) & kUpb_LabelFlags_IsExtension;
  UPB_ASSERT(!(is_empty && is_extension));

  if (is_empty && !(d->options & kUpb_DecodeOption_ExperimentalAllowUnlinked)) {
    _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_UnlinkedSubMessage);
  }

  upb_TaggedMessagePtr tagged =
      UPB_PRIVATE(_upb_TaggedMessagePtr_Pack)(msg, is_empty);
  memcpy(target, &tagged, sizeof(tagged));
  return msg;
}

static upb_Message* _upb_Decoder_ReuseSubMessage(
    upb_Decoder* d, const upb_MiniTableSub* subs,
    const upb_MiniTableField* field, upb_TaggedMessagePtr* target) {
  upb_TaggedMessagePtr tagged = *target;
  const upb_MiniTable* subl = _upb_MiniTableSubs_MessageByField(subs, field);
  UPB_ASSERT(subl);
  if (!upb_TaggedMessagePtr_IsEmpty(tagged) ||
      UPB_PRIVATE(_upb_MiniTable_IsEmpty)(subl)) {
    return UPB_PRIVATE(_upb_TaggedMessagePtr_GetMessage)(tagged);
  }

  // We found an empty message from a previous parse that was performed before
  // this field was linked.  But it is linked now, so we want to allocate a new
  // message of the correct type and promote data into it before continuing.
  upb_Message* existing =
      UPB_PRIVATE(_upb_TaggedMessagePtr_GetEmptyMessage)(tagged);
  upb_Message* promoted = _upb_Decoder_NewSubMessage(d, subs, field, target);
  size_t size;
  const char* unknown = upb_Message_GetUnknown(existing, &size);
  upb_DecodeStatus status = upb_Decode(unknown, size, promoted, subl, d->extreg,
                                       d->options, &d->arena);
  if (status != kUpb_DecodeStatus_Ok) _upb_Decoder_ErrorJmp(d, status);
  return promoted;
}

static const char* _upb_Decoder_ReadString(upb_Decoder* d, const char* ptr,
                                           int size, upb_StringView* str) {
  const char* str_ptr = ptr;
  ptr = upb_EpsCopyInputStream_ReadString(&d->input, &str_ptr, size, &d->arena);
  if (!ptr) _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
  str->data = str_ptr;
  str->size = size;
  return ptr;
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
    const upb_MiniTableSub* subs, const upb_MiniTableField* field, int size) {
  int saved_delta = upb_EpsCopyInputStream_PushLimit(&d->input, ptr, size);
  const upb_MiniTable* subl = _upb_MiniTableSubs_MessageByField(subs, field);
  UPB_ASSERT(subl);
  ptr = _upb_Decoder_RecurseSubMessage(d, ptr, submsg, subl, DECODE_NOGROUP);
  upb_EpsCopyInputStream_PopLimit(&d->input, ptr, saved_delta);
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
    const upb_MiniTableSub* subs, const upb_MiniTableField* field) {
  const upb_MiniTable* subl = _upb_MiniTableSubs_MessageByField(subs, field);
  UPB_ASSERT(subl);
  return _upb_Decoder_DecodeGroup(d, ptr, submsg, subl,
                                  field->UPB_PRIVATE(number));
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

  if (!UPB_PRIVATE(_upb_Message_AddUnknown)(msg, buf, end - buf, &d->arena)) {
    _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
  }
}

UPB_FORCEINLINE
static bool _upb_Decoder_CheckEnum(upb_Decoder* d, const char* ptr,
                                   upb_Message* msg, const upb_MiniTableEnum* e,
                                   const upb_MiniTableField* field,
                                   wireval* val) {
  const uint32_t v = val->uint32_val;

  if (UPB_LIKELY(upb_MiniTableEnum_CheckValue(e, v))) return true;

  // Unrecognized enum goes into unknown fields.
  // For packed fields the tag could be arbitrarily far in the past,
  // so we just re-encode the tag and value here.
  const uint32_t tag =
      ((uint32_t)field->UPB_PRIVATE(number) << 3) | kUpb_WireType_Varint;
  upb_Message* unknown_msg =
      field->UPB_PRIVATE(mode) & kUpb_LabelFlags_IsExtension ? d->unknown_msg
                                                             : msg;
  _upb_Decoder_AddUnknownVarints(d, unknown_msg, tag, v);
  return false;
}

UPB_NOINLINE
static const char* _upb_Decoder_DecodeEnumArray(upb_Decoder* d, const char* ptr,
                                                upb_Message* msg,
                                                upb_Array* arr,
                                                const upb_MiniTableSub* subs,
                                                const upb_MiniTableField* field,
                                                wireval* val) {
  const upb_MiniTableEnum* e = _upb_MiniTableSubs_EnumByField(subs, field);
  if (!_upb_Decoder_CheckEnum(d, ptr, msg, e, field, val)) return ptr;
  void* mem = UPB_PTR_AT(upb_Array_MutableDataPtr(arr),
                         arr->UPB_PRIVATE(size) * 4, void);
  arr->UPB_PRIVATE(size)++;
  memcpy(mem, val, 4);
  return ptr;
}

UPB_FORCEINLINE
static const char* _upb_Decoder_DecodeFixedPacked(
    upb_Decoder* d, const char* ptr, upb_Array* arr, wireval* val,
    const upb_MiniTableField* field, int lg2) {
  int mask = (1 << lg2) - 1;
  size_t count = val->size >> lg2;
  if ((val->size & mask) != 0) {
    // Length isn't a round multiple of elem size.
    _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
  }
  _upb_Decoder_Reserve(d, arr, count);
  void* mem = UPB_PTR_AT(upb_Array_MutableDataPtr(arr),
                         arr->UPB_PRIVATE(size) << lg2, void);
  arr->UPB_PRIVATE(size) += count;
  // Note: if/when the decoder supports multi-buffer input, we will need to
  // handle buffer seams here.
  if (upb_IsLittleEndian()) {
    ptr = upb_EpsCopyInputStream_Copy(&d->input, ptr, mem, val->size);
  } else {
    int delta = upb_EpsCopyInputStream_PushLimit(&d->input, ptr, val->size);
    char* dst = mem;
    while (!_upb_Decoder_IsDone(d, &ptr)) {
      if (lg2 == 2) {
        ptr = upb_WireReader_ReadFixed32(ptr, dst);
        dst += 4;
      } else {
        UPB_ASSERT(lg2 == 3);
        ptr = upb_WireReader_ReadFixed64(ptr, dst);
        dst += 8;
      }
    }
    upb_EpsCopyInputStream_PopLimit(&d->input, ptr, delta);
  }

  return ptr;
}

UPB_FORCEINLINE
static const char* _upb_Decoder_DecodeVarintPacked(
    upb_Decoder* d, const char* ptr, upb_Array* arr, wireval* val,
    const upb_MiniTableField* field, int lg2) {
  int scale = 1 << lg2;
  int saved_limit = upb_EpsCopyInputStream_PushLimit(&d->input, ptr, val->size);
  char* out = UPB_PTR_AT(upb_Array_MutableDataPtr(arr),
                         arr->UPB_PRIVATE(size) << lg2, void);
  while (!_upb_Decoder_IsDone(d, &ptr)) {
    wireval elem;
    ptr = _upb_Decoder_DecodeVarint(d, ptr, &elem.uint64_val);
    _upb_Decoder_Munge(field->UPB_PRIVATE(descriptortype), &elem);
    if (_upb_Decoder_Reserve(d, arr, 1)) {
      out = UPB_PTR_AT(upb_Array_MutableDataPtr(arr),
                       arr->UPB_PRIVATE(size) << lg2, void);
    }
    arr->UPB_PRIVATE(size)++;
    memcpy(out, &elem, scale);
    out += scale;
  }
  upb_EpsCopyInputStream_PopLimit(&d->input, ptr, saved_limit);
  return ptr;
}

UPB_NOINLINE
static const char* _upb_Decoder_DecodeEnumPacked(
    upb_Decoder* d, const char* ptr, upb_Message* msg, upb_Array* arr,
    const upb_MiniTableSub* subs, const upb_MiniTableField* field,
    wireval* val) {
  const upb_MiniTableEnum* e = _upb_MiniTableSubs_EnumByField(subs, field);
  int saved_limit = upb_EpsCopyInputStream_PushLimit(&d->input, ptr, val->size);
  char* out = UPB_PTR_AT(upb_Array_MutableDataPtr(arr),
                         arr->UPB_PRIVATE(size) * 4, void);
  while (!_upb_Decoder_IsDone(d, &ptr)) {
    wireval elem;
    ptr = _upb_Decoder_DecodeVarint(d, ptr, &elem.uint64_val);
    _upb_Decoder_MungeInt32(&elem);
    if (!_upb_Decoder_CheckEnum(d, ptr, msg, e, field, &elem)) {
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
  upb_EpsCopyInputStream_PopLimit(&d->input, ptr, saved_limit);
  return ptr;
}

static upb_Array* _upb_Decoder_CreateArray(upb_Decoder* d,
                                           const upb_MiniTableField* field) {
  const upb_FieldType field_type = field->UPB_PRIVATE(descriptortype);
  const size_t lg2 = UPB_PRIVATE(_upb_FieldType_SizeLg2)(field_type);
  upb_Array* ret = UPB_PRIVATE(_upb_Array_New)(&d->arena, 4, lg2);
  if (!ret) _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
  return ret;
}

static const char* _upb_Decoder_DecodeToArray(upb_Decoder* d, const char* ptr,
                                              upb_Message* msg,
                                              const upb_MiniTableSub* subs,
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
    case kUpb_DecodeOp_String:
      _upb_Decoder_VerifyUtf8(d, ptr, val->size);
      /* Fallthrough. */
    case kUpb_DecodeOp_Bytes: {
      /* Append bytes. */
      upb_StringView* str = (upb_StringView*)upb_Array_MutableDataPtr(arr) +
                            arr->UPB_PRIVATE(size);
      arr->UPB_PRIVATE(size)++;
      return _upb_Decoder_ReadString(d, ptr, val->size, str);
    }
    case kUpb_DecodeOp_SubMessage: {
      /* Append submessage / group. */
      upb_TaggedMessagePtr* target = UPB_PTR_AT(
          upb_Array_MutableDataPtr(arr), arr->UPB_PRIVATE(size) * sizeof(void*),
          upb_TaggedMessagePtr);
      upb_Message* submsg = _upb_Decoder_NewSubMessage(d, subs, field, target);
      arr->UPB_PRIVATE(size)++;
      if (UPB_UNLIKELY(field->UPB_PRIVATE(descriptortype) ==
                       kUpb_FieldType_Group)) {
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
  if (!ret) _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
  return ret;
}

static const char* _upb_Decoder_DecodeToMap(upb_Decoder* d, const char* ptr,
                                            upb_Message* msg,
                                            const upb_MiniTableSub* subs,
                                            const upb_MiniTableField* field,
                                            wireval* val) {
  upb_Map** map_p = UPB_PTR_AT(msg, field->UPB_PRIVATE(offset), upb_Map*);
  upb_Map* map = *map_p;
  upb_MapEntry ent;
  UPB_ASSERT(upb_MiniTableField_Type(field) == kUpb_FieldType_Message);
  const upb_MiniTable* entry = _upb_MiniTableSubs_MessageByField(subs, field);

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

  if (entry->UPB_PRIVATE(fields)[1].UPB_PRIVATE(descriptortype) ==
          kUpb_FieldType_Message ||
      entry->UPB_PRIVATE(fields)[1].UPB_PRIVATE(descriptortype) ==
          kUpb_FieldType_Group) {
    // Create proactively to handle the case where it doesn't appear.
    upb_TaggedMessagePtr msg;
    _upb_Decoder_NewSubMessage(d, entry->UPB_PRIVATE(subs),
                               &entry->UPB_PRIVATE(fields)[1], &msg);
    ent.v.val = upb_value_uintptr(msg);
  }

  ptr = _upb_Decoder_DecodeSubMessage(d, ptr, &ent.message, subs, field,
                                      val->size);
  // check if ent had any unknown fields
  size_t size;
  upb_Message_GetUnknown(&ent.message, &size);
  if (size != 0) {
    char* buf;
    size_t size;
    uint32_t tag =
        ((uint32_t)field->UPB_PRIVATE(number) << 3) | kUpb_WireType_Delimited;
    upb_EncodeStatus status =
        upb_Encode(&ent.message, entry, 0, &d->arena, &buf, &size);
    if (status != kUpb_EncodeStatus_Ok) {
      _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
    }
    _upb_Decoder_AddUnknownVarints(d, msg, tag, size);
    if (!UPB_PRIVATE(_upb_Message_AddUnknown)(msg, buf, size, &d->arena)) {
      _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
    }
  } else {
    if (_upb_Map_Insert(map, &ent.k, map->key_size, &ent.v, map->val_size,
                        &d->arena) == kUpb_MapInsertStatus_OutOfMemory) {
      _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
    }
  }
  return ptr;
}

static const char* _upb_Decoder_DecodeToSubMessage(
    upb_Decoder* d, const char* ptr, upb_Message* msg,
    const upb_MiniTableSub* subs, const upb_MiniTableField* field, wireval* val,
    int op) {
  void* mem = UPB_PTR_AT(msg, field->UPB_PRIVATE(offset), void);
  int type = field->UPB_PRIVATE(descriptortype);

  if (UPB_UNLIKELY(op == kUpb_DecodeOp_Enum) &&
      !_upb_Decoder_CheckEnum(d, ptr, msg,
                              _upb_MiniTableSubs_EnumByField(subs, field),
                              field, val)) {
    return ptr;
  }

  // Set presence if necessary.
  if (UPB_PRIVATE(_upb_MiniTableField_HasHasbit)(field)) {
    UPB_PRIVATE(_upb_Message_SetHasbit)(msg, field);
  } else if (UPB_PRIVATE(_upb_MiniTableField_IsInOneof)(field)) {
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
      upb_TaggedMessagePtr* submsgp = mem;
      upb_Message* submsg;
      if (*submsgp) {
        submsg = _upb_Decoder_ReuseSubMessage(d, subs, field, submsgp);
      } else {
        submsg = _upb_Decoder_NewSubMessage(d, subs, field, submsgp);
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
                                       const upb_MiniTable* m) {
  UPB_ASSERT(m->UPB_PRIVATE(required_count));
  if (UPB_UNLIKELY(d->options & kUpb_DecodeOption_CheckRequired)) {
    d->missing_required =
        !UPB_PRIVATE(_upb_Message_IsInitializedShallow)(msg, m);
  }
  return ptr;
}

UPB_FORCEINLINE
static bool _upb_Decoder_TryFastDispatch(upb_Decoder* d, const char** ptr,
                                         upb_Message* msg,
                                         const upb_MiniTable* m) {
#if UPB_FASTTABLE
  if (m && m->UPB_PRIVATE(table_mask) != (unsigned char)-1) {
    uint16_t tag = _upb_FastDecoder_LoadTag(*ptr);
    intptr_t table = decode_totable(m);
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
    _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
  }
  upb_Message* submsg = _upb_Decoder_NewSubMessage(
      d, &ext->ext->UPB_PRIVATE(sub), &ext->ext->UPB_PRIVATE(field),
      (upb_TaggedMessagePtr*)&ext->data);
  upb_DecodeStatus status = upb_Decode(
      data, size, submsg, upb_MiniTableExtension_GetSubMessage(item_mt),
      d->extreg, d->options, &d->arena);
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

  if (!UPB_PRIVATE(_upb_Message_AddUnknown)(msg, buf, split - buf, &d->arena) ||
      !UPB_PRIVATE(_upb_Message_AddUnknown)(msg, message_data, message_size,
                                            &d->arena) ||
      !UPB_PRIVATE(_upb_Message_AddUnknown)(msg, split, end - split,
                                            &d->arena)) {
    _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
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

static const upb_MiniTableField* _upb_Decoder_FindField(upb_Decoder* d,
                                                        const upb_MiniTable* t,
                                                        uint32_t field_number,
                                                        int* last_field_index) {
  static upb_MiniTableField none = {
      0, 0, 0, 0, kUpb_FakeFieldType_FieldNotFound, 0};
  if (t == NULL) return &none;

  size_t idx = ((size_t)field_number) - 1;  // 0 wraps to SIZE_MAX
  if (idx < t->UPB_PRIVATE(dense_below)) {
    // Fastest case: index into dense fields.
    goto found;
  }

  if (t->UPB_PRIVATE(dense_below) < t->UPB_PRIVATE(field_count)) {
    // Linear search non-dense fields. Resume scanning from last_field_index
    // since fields are usually in order.
    size_t last = *last_field_index;
    for (idx = last; idx < t->UPB_PRIVATE(field_count); idx++) {
      if (t->UPB_PRIVATE(fields)[idx].UPB_PRIVATE(number) == field_number) {
        goto found;
      }
    }

    for (idx = t->UPB_PRIVATE(dense_below); idx < last; idx++) {
      if (t->UPB_PRIVATE(fields)[idx].UPB_PRIVATE(number) == field_number) {
        goto found;
      }
    }
  }

  if (d->extreg) {
    switch (t->UPB_PRIVATE(ext)) {
      case kUpb_ExtMode_Extendable: {
        const upb_MiniTableExtension* ext =
            upb_ExtensionRegistry_Lookup(d->extreg, t, field_number);
        if (ext) return &ext->UPB_PRIVATE(field);
        break;
      }
      case kUpb_ExtMode_IsMessageSet:
        if (field_number == kUpb_MsgSet_Item) {
          static upb_MiniTableField item = {
              0, 0, 0, 0, kUpb_FakeFieldType_MessageSetItem, 0};
          return &item;
        }
        break;
    }
  }

  return &none;  // Unknown field.

found:
  UPB_ASSERT(t->UPB_PRIVATE(fields)[idx].UPB_PRIVATE(number) == field_number);
  *last_field_index = idx;
  return &t->UPB_PRIVATE(fields)[idx];
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
      [kUpb_FieldType_Enum] = kUpb_DecodeOp_Enum,
      [kUpb_FieldType_SFixed32] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_SFixed64] = kUpb_DecodeOp_UnknownField,
      [kUpb_FieldType_SInt32] = kUpb_DecodeOp_Scalar4Byte,
      [kUpb_FieldType_SInt64] = kUpb_DecodeOp_Scalar8Byte,
      [kUpb_FakeFieldType_MessageSetItem] = kUpb_DecodeOp_UnknownField,
  };

  return kVarintOps[field->UPB_PRIVATE(descriptortype)];
}

UPB_FORCEINLINE
static void _upb_Decoder_CheckUnlinked(upb_Decoder* d, const upb_MiniTable* mt,
                                       const upb_MiniTableField* field,
                                       int* op) {
  // If sub-message is not linked, treat as unknown.
  if (field->UPB_PRIVATE(mode) & kUpb_LabelFlags_IsExtension) return;
  const upb_MiniTable* mt_sub =
      _upb_MiniTableSubs_MessageByField(mt->UPB_PRIVATE(subs), field);
  if ((d->options & kUpb_DecodeOption_ExperimentalAllowUnlinked) ||
      !UPB_PRIVATE(_upb_MiniTable_IsEmpty)(mt_sub)) {
    return;
  }
#ifndef NDEBUG
  const upb_MiniTableField* oneof = upb_MiniTable_GetOneof(mt, field);
  if (oneof) {
    // All other members of the oneof must be message fields that are also
    // unlinked.
    do {
      UPB_ASSERT(upb_MiniTableField_CType(oneof) == kUpb_CType_Message);
      const upb_MiniTableSub* oneof_sub =
          &mt->UPB_PRIVATE(subs)[oneof->UPB_PRIVATE(submsg_index)];
      UPB_ASSERT(!oneof_sub);
    } while (upb_MiniTable_NextOneofField(mt, &oneof));
  }
#endif  // NDEBUG
  *op = kUpb_DecodeOp_UnknownField;
}

UPB_FORCEINLINE
static void _upb_Decoder_MaybeVerifyUtf8(upb_Decoder* d,
                                         const upb_MiniTableField* field,
                                         int* op) {
  if ((field->UPB_ONLYBITS(mode) & kUpb_LabelFlags_IsAlternate) &&
      UPB_UNLIKELY(d->options & kUpb_DecodeOption_AlwaysValidateUtf8))
    *op = kUpb_DecodeOp_String;
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
      [kUpb_FakeFieldType_MessageSetItem] = kUpb_DecodeOp_UnknownField,
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
static const char* _upb_Decoder_DecodeWireValue(upb_Decoder* d, const char* ptr,
                                                const upb_MiniTable* mt,
                                                const upb_MiniTableField* field,
                                                int wire_type, wireval* val,
                                                int* op) {
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
      _upb_Decoder_Munge(field->UPB_PRIVATE(descriptortype), val);
      return ptr;
    case kUpb_WireType_32Bit:
      *op = kUpb_DecodeOp_Scalar4Byte;
      if (((1 << field->UPB_PRIVATE(descriptortype)) & kFixed32OkMask) == 0) {
        *op = kUpb_DecodeOp_UnknownField;
      }
      return upb_WireReader_ReadFixed32(ptr, &val->uint32_val);
    case kUpb_WireType_64Bit:
      *op = kUpb_DecodeOp_Scalar8Byte;
      if (((1 << field->UPB_PRIVATE(descriptortype)) & kFixed64OkMask) == 0) {
        *op = kUpb_DecodeOp_UnknownField;
      }
      return upb_WireReader_ReadFixed64(ptr, &val->uint64_val);
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
  _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
}

UPB_FORCEINLINE
static const char* _upb_Decoder_DecodeKnownField(
    upb_Decoder* d, const char* ptr, upb_Message* msg,
    const upb_MiniTable* layout, const upb_MiniTableField* field, int op,
    wireval* val) {
  const upb_MiniTableSub* subs = layout->UPB_PRIVATE(subs);
  uint8_t mode = field->UPB_PRIVATE(mode);

  if (UPB_UNLIKELY(mode & kUpb_LabelFlags_IsExtension)) {
    const upb_MiniTableExtension* ext_layout =
        (const upb_MiniTableExtension*)field;
    upb_Extension* ext = UPB_PRIVATE(_upb_Message_GetOrCreateExtension)(
        msg, ext_layout, &d->arena);
    if (UPB_UNLIKELY(!ext)) {
      _upb_Decoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
    }
    d->unknown_msg = msg;
    msg = (upb_Message*)&ext->data;
    subs = &ext->ext->UPB_PRIVATE(sub);
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
    if (!UPB_PRIVATE(_upb_Message_AddUnknown)(msg, start, ptr - start,
                                              &d->arena)) {
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
    const upb_MiniTableField* field;
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

    UPB_ASSERT(ptr < d->input.limit_ptr);
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
    ptr = _upb_Decoder_DecodeWireValue(d, ptr, layout, field, wire_type, &val,
                                       &op);

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

  return UPB_UNLIKELY(layout && layout->UPB_PRIVATE(required_count))
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
                                               const char* buf,
                                               upb_Message* msg,
                                               const upb_MiniTable* m) {
  if (!_upb_Decoder_TryFastDispatch(d, &buf, msg, m)) {
    _upb_Decoder_DecodeMessage(d, buf, msg, m);
  }
  if (d->end_group != DECODE_NOGROUP) return kUpb_DecodeStatus_Malformed;
  if (d->missing_required) return kUpb_DecodeStatus_MissingRequired;
  return kUpb_DecodeStatus_Ok;
}

UPB_NOINLINE
const char* _upb_Decoder_IsDoneFallback(upb_EpsCopyInputStream* e,
                                        const char* ptr, int overrun) {
  return _upb_EpsCopyInputStream_IsDoneFallbackInline(
      e, ptr, overrun, _upb_Decoder_BufferFlipCallback);
}

static upb_DecodeStatus upb_Decoder_Decode(upb_Decoder* const decoder,
                                           const char* const buf,
                                           upb_Message* const msg,
                                           const upb_MiniTable* const m,
                                           upb_Arena* const arena) {
  if (UPB_SETJMP(decoder->err) == 0) {
    decoder->status = _upb_Decoder_DecodeTop(decoder, buf, msg, m);
  } else {
    UPB_ASSERT(decoder->status != kUpb_DecodeStatus_Ok);
  }

  UPB_PRIVATE(_upb_Arena_SwapOut)(arena, &decoder->arena);

  return decoder->status;
}

upb_DecodeStatus upb_Decode(const char* buf, size_t size, upb_Message* msg,
                            const upb_MiniTable* m,
                            const upb_ExtensionRegistry* extreg, int options,
                            upb_Arena* arena) {
  upb_Decoder decoder;
  unsigned depth = (unsigned)options >> 16;

  upb_EpsCopyInputStream_Init(&decoder.input, &buf, size,
                              options & kUpb_DecodeOption_AliasString);

  decoder.extreg = extreg;
  decoder.unknown = NULL;
  decoder.depth = depth ? depth : kUpb_WireFormat_DefaultDepthLimit;
  decoder.end_group = DECODE_NOGROUP;
  decoder.options = (uint16_t)options;
  decoder.missing_required = false;
  decoder.status = kUpb_DecodeStatus_Ok;

  // Violating the encapsulation of the arena for performance reasons.
  // This is a temporary arena that we swap into and swap out of when we are
  // done.  The temporary arena only needs to be able to handle allocation,
  // not fuse or free, so it does not need many of the members to be initialized
  // (particularly parent_or_count).
  UPB_PRIVATE(_upb_Arena_SwapIn)(&decoder.arena, arena);

  return upb_Decoder_Decode(&decoder, buf, msg, m, arena);
}

#undef OP_FIXPCK_LG2
#undef OP_VARPCK_LG2
