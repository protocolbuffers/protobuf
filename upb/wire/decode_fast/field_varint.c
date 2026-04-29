// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "upb/base/internal/endian.h"
#include "upb/message/message.h"
#include "upb/mini_table/enum.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode.h"
#include "upb/wire/decode_fast/cardinality.h"
#include "upb/wire/decode_fast/combinations.h"
#include "upb/wire/decode_fast/data.h"
#include "upb/wire/decode_fast/dispatch.h"
#include "upb/wire/decode_fast/field_parsers.h"
#include "upb/wire/eps_copy_input_stream.h"
#include "upb/wire/internal/decoder.h"
#include "upb/wire/reader.h"

// Must be last.
#include "upb/port/def.inc"

typedef struct {
  const upb_MiniTableEnum* enum_table;
  upb_Message* msg;
  uint64_t data;
  intptr_t table;
} upb_DecodeFast_ClosedEnumContext;

UPB_PRESERVE_MOST void _upb_FastDecoder_AddEnumValueToUnknown(upb_Decoder* d,
                                                              upb_Message* msg,
                                                              uint64_t data,
                                                              uint64_t val,
                                                              intptr_t table) {
  uint16_t field_index = upb_DecodeFastData_GetFieldIndex(data);
  const upb_MiniTable* tablep = decode_totablep(table);
  const upb_MiniTableField* field =
      upb_MiniTable_GetFieldByIndex(tablep, field_index);
  _upb_Decoder_AddEnumValueToUnknown(d, msg, field, val);
}

static bool upb_DecodeFast_SingleVarint(upb_Decoder* d, const char** ptr,
                                        void* dst, upb_DecodeFast_Type type,
                                        upb_DecodeFastNext* next, void* ctx) {
  UPB_ASSERT(dst);
  UPB_UNUSED(ctx);

  const char* p = *ptr;
  uint64_t val;

  p = upb_WireReader_ReadVarint(p, &val, &d->input);
  if (!p) {
    return UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_Malformed, next);
  }

  switch (type) {
    case kUpb_DecodeFast_Bool:
      val = val != 0;
      break;
    case kUpb_DecodeFast_ZigZag32: {
      uint32_t n = val;
      val = (n >> 1) ^ -(int32_t)(n & 1);
      break;
    }
    case kUpb_DecodeFast_ZigZag64: {
      val = (val >> 1) ^ -(int64_t)(val & 1);
      break;
    }
    default:
      break;
  }

  UPB_ASSERT(upb_IsLittleEndian());
  memcpy(dst, &val, upb_DecodeFast_ValueBytes(type));
  *ptr = p;
  return true;
}

typedef struct {
  upb_Decoder* decoder;
  upb_DecodeFast_Type type;
  upb_Message* msg;
  uint64_t* data;
  uint64_t* hasbits;
  upb_DecodeFastNext* ret;
  intptr_t table;
  const upb_MiniTableEnum* enum_table;
} upb_DecodeFast_PackedVarintContext;

UPB_FORCEINLINE
int upb_DecodeFast_CountVarints(const char* ptr, const char* end) {
  int count = 0;
  for (; ptr < end; ptr++) {
    count += (*ptr & 0x80) == 0;
  }
  return count;
}

static const char* upb_DecodeFast_PackedVarint(upb_EpsCopyInputStream* st,
                                               const char* ptr, int size,
                                               void* ctx) {
  upb_DecodeFast_PackedVarintContext* c =
      (upb_DecodeFast_PackedVarintContext*)ctx;

  if (size == 0) return ptr;  // 0-element packed fields are valid.

  upb_DecodeFastArray arr;
  int count = upb_DecodeFast_CountVarints(ptr, ptr + size);
  int valbytes = upb_DecodeFast_ValueBytes(c->type);

  // If the last byte is a continuation byte, we have a malformed varint.
  if (count == 0 || (ptr[size - 1] & 0x80) != 0) {
    UPB_DECODEFAST_ERROR(c->decoder, kUpb_DecodeStatus_Malformed, c->ret);
    return NULL;
  }

  if (!upb_DecodeFast_GetArrayForAppend(c->decoder, ptr, c->msg, *c->data,
                                        c->hasbits, &arr, c->type, count,
                                        c->ret)) {
    return NULL;
  }

  UPB_ASSERT(arr.dst);

  if (c->type == kUpb_DecodeFast_ClosedEnum) {
    while (!upb_EpsCopyInputStream_IsDone(&c->decoder->input, &ptr)) {
      uint64_t val;
      ptr = upb_WireReader_ReadVarint(ptr, &val, &c->decoder->input);
      if (UPB_UNLIKELY(!ptr)) {
        UPB_DECODEFAST_ERROR(c->decoder, kUpb_DecodeStatus_Malformed, c->ret);
        return NULL;
      }
      if (UPB_LIKELY(upb_MiniTableEnum_CheckValue(c->enum_table, val))) {
        memcpy(arr.dst, &val, 4);
        arr.dst += 4;
      } else {
        _upb_FastDecoder_AddEnumValueToUnknown(c->decoder, c->msg, *c->data,
                                               val, c->table);
      }
    }
  } else {
    int read = 0;
    while (!upb_EpsCopyInputStream_IsDone(&c->decoder->input, &ptr)) {
      UPB_ASSERT(read < count);
      if (!upb_DecodeFast_SingleVarint(c->decoder, &ptr, arr.dst, c->type,
                                       c->ret, NULL)) {
        return NULL;
      }
      arr.dst = UPB_PTR_AT(arr.dst, valbytes, char);
      ++read;
    }
  }

  upb_DecodeFastField_SetArraySize(&arr, c->type);

  return ptr;
}

UPB_FORCEINLINE
void upb_DecodeFast_ClosedEnum(upb_Decoder* d, const char** ptr,
                               upb_Message* msg, intptr_t table,
                               uint64_t* hasbits, uint64_t* data,
                               upb_DecodeFastNext* ret,
                               upb_DecodeFast_Cardinality card,
                               upb_DecodeFast_TagSize tagsize) {
  uint16_t field_index = upb_DecodeFastData_GetFieldIndex(*data);
  const upb_MiniTable* tablep = decode_totablep(table);
  const upb_MiniTableField* field =
      upb_MiniTable_GetFieldByIndex(tablep, field_index);
  const upb_MiniTableEnum* enum_table = upb_MiniTable_GetSubEnumTable(field);

  if (UPB_UNLIKELY(enum_table == NULL)) {
    UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, ret);
    return;
  }

  const char* p = *ptr;
  if (card == kUpb_DecodeFast_Packed) {
    upb_DecodeFast_PackedVarintContext pack_ctx = {
        .decoder = d,
        .type = kUpb_DecodeFast_ClosedEnum,
        .msg = msg,
        .data = data,
        .hasbits = hasbits,
        .ret = ret,
        .table = table,
        .enum_table = enum_table,
    };
    upb_DecodeFast_Packed(d, ptr, kUpb_DecodeFast_ClosedEnum, card, tagsize,
                          data, &upb_DecodeFast_PackedVarint, ret, &pack_ctx);
    return;
  }

  if (card == kUpb_DecodeFast_Repeated) {
    upb_DecodeFastNext flipped = kUpb_DecodeFastNext_TailCallPacked;
    if (!upb_DecodeFast_CheckTag(&p, kUpb_DecodeFast_ClosedEnum, card, tagsize,
                                 data, flipped, ret)) {
      return;
    }
    upb_DecodeFastArray arr;
    if (!upb_DecodeFast_GetArrayForAppend(d, *ptr, msg, *data, hasbits, &arr,
                                          kUpb_DecodeFast_ClosedEnum, 1, ret)) {
      return;
    }
    bool next_tag_matches;
    do {
      uint64_t val;
      p = upb_WireReader_ReadVarint(p, &val, &d->input);
      if (UPB_UNLIKELY(!p)) {
        UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_Malformed, ret);
        return;
      }
      if (UPB_LIKELY(upb_MiniTableEnum_CheckValue(enum_table, val))) {
        _upb_Decoder_Trace(d, 'F');
        memcpy(arr.dst, &val, 4);
      } else {
        _upb_FastDecoder_AddEnumValueToUnknown(d, msg, *data, val, table);
        arr.dst = UPB_PTR_AT(arr.dst, -4, char);
      }
      next_tag_matches =
          upb_DecodeFast_TryMatchTag(d, p, arr.expected_tag, ret, tagsize);
    } while (upb_DecodeFast_NextRepeated(d, &p, ret, &arr, next_tag_matches,
                                         kUpb_DecodeFast_ClosedEnum, tagsize));
    upb_DecodeFastField_SetArraySize(&arr, kUpb_DecodeFast_ClosedEnum);
    *ptr = p;
  } else {
    // Scalar or Oneof
    if (UPB_UNLIKELY(!upb_DecodeFast_CheckTag(&p, kUpb_DecodeFast_ClosedEnum,
                                              card, tagsize, data, 0, ret))) {
      return;
    }
    uint64_t val;
    p = upb_WireReader_ReadVarint(p, &val, &d->input);
    if (UPB_UNLIKELY(!p)) {
      UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_Malformed, ret);
      return;
    }
    if (UPB_LIKELY(upb_MiniTableEnum_CheckValue(enum_table, val))) {
      void* dst;
      if (upb_DecodeFast_GetScalarField(d, p, msg, *data, hasbits, ret, &dst,
                                        card)) {
        _upb_Decoder_Trace(d, 'F');
        memcpy(dst, &val, 4);
      } else {
        return;
      }
    } else {
      _upb_FastDecoder_AddEnumValueToUnknown(d, msg, *data, val, table);
    }
    *ptr = p;
  }
}

UPB_FORCEINLINE
void upb_DecodeFast_Varint(upb_Decoder* d, const char** ptr, upb_Message* msg,
                           intptr_t table, uint64_t* hasbits, uint64_t* data,
                           upb_DecodeFastNext* ret, upb_DecodeFast_Type type,
                           upb_DecodeFast_Cardinality card,
                           upb_DecodeFast_TagSize tagsize) {
  if (type == kUpb_DecodeFast_ClosedEnum) {
    upb_DecodeFast_ClosedEnum(d, ptr, msg, table, hasbits, data, ret, card,
                              tagsize);
    return;
  }

  if (card == kUpb_DecodeFast_Packed) {
    upb_DecodeFast_PackedVarintContext pack_ctx = {
        .decoder = d,
        .type = type,
        .msg = msg,
        .data = data,
        .hasbits = hasbits,
        .ret = ret,
    };
    upb_DecodeFast_Packed(d, ptr, type, card, tagsize, data,
                          &upb_DecodeFast_PackedVarint, ret, &pack_ctx);
  } else {
    upb_DecodeFast_Unpacked(d, ptr, msg, data, hasbits, ret, type, card,
                            tagsize, &upb_DecodeFast_SingleVarint, NULL);
  }
}

/* Generate all combinations:
 * {s,o,r,p} x {b1,v4,z4,v8,z8} x {1bt,2bt} */

#define F(type, card, tagsize)                                            \
  UPB_NOINLINE UPB_PRESERVE_NONE const char* UPB_DECODEFAST_FUNCNAME(     \
      type, card, tagsize)(UPB_PARSE_PARAMS) {                            \
    upb_DecodeFastNext next = kUpb_DecodeFastNext_Dispatch;               \
    upb_DecodeFast_Varint(d, &ptr, msg, table, &hasbits, &data, &next,    \
                          kUpb_DecodeFast_##type, kUpb_DecodeFast_##card, \
                          kUpb_DecodeFast_##tagsize);                     \
    UPB_DECODEFAST_NEXTMAYBEPACKED(                                       \
        next, UPB_DECODEFAST_FUNCNAME(type, Repeated, tagsize),           \
        UPB_DECODEFAST_FUNCNAME(type, Packed, tagsize));                  \
  }

UPB_DECODEFAST_CARDINALITIES(UPB_DECODEFAST_TAGSIZES, F, Bool)
UPB_DECODEFAST_CARDINALITIES(UPB_DECODEFAST_TAGSIZES, F, Varint32)
UPB_DECODEFAST_CARDINALITIES(UPB_DECODEFAST_TAGSIZES, F, Varint64)
UPB_DECODEFAST_CARDINALITIES(UPB_DECODEFAST_TAGSIZES, F, ZigZag32)
UPB_DECODEFAST_CARDINALITIES(UPB_DECODEFAST_TAGSIZES, F, ZigZag64)
UPB_DECODEFAST_CARDINALITIES(UPB_DECODEFAST_TAGSIZES, F, ClosedEnum)

#undef F
