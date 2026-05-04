// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <stdint.h>

#include "upb/base/string_view.h"
#include "upb/message/internal/message.h"
#include "upb/message/message.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode.h"
#include "upb/wire/decode_fast/cardinality.h"
#include "upb/wire/decode_fast/dispatch.h"
#include "upb/wire/decode_fast/field_helpers.h"
#include "upb/wire/decode_fast/field_parsers.h"
#include "upb/wire/eps_copy_input_stream.h"
#include "upb/wire/internal/decoder.h"
#include "upb/wire/reader.h"

// Must be last.
#include "upb/port/def.inc"

// Helper defines for managing fallback tail-call to slow path.
#define kUpb_DecodeFastNext_DecodeUnknownSlowPath \
  kUpb_DecodeFastNext_TailCallUnpacked
#define UPB_DECODEFAST_NEXTMAYBECOPY(next)                                     \
  UPB_DECODEFAST_NEXTMAYBEPACKED(next, _upb_FastDecoder_DecodeUnknownSlowPath, \
                                 upb_DecodeFast_Unreachable);

// Tail-call target for handling unknown field slow path.
// Fast-path filters out unsupported cases, so we don't need to re-check here.
// To avoid additional computations, `data` is overloaded to the size of the
// unknown region.
UPB_PRESERVE_NONE UPB_NOINLINE upb_FastDecoder_Return
_upb_FastDecoder_DecodeUnknownSlowPath(struct upb_Decoder* d, const char* ptr,
                                       upb_Message* msg, intptr_t table,
                                       uint64_t hasbits, uint64_t data) {
  bool alias = (d->options & kUpb_DecodeOption_AliasString) != 0;
  const char* end =
      UPB_PRIVATE(upb_EpsCopyInputStream_GetInputPtr)(&d->input, ptr);
  if (UPB_UNLIKELY(!UPB_PRIVATE(_upb_Message_AddUnknownSlowPath)(
          msg, end - data, data, &d->arena, alias))) {
    return _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
  }

  data = 0;
  upb_DecodeFastNext next = kUpb_DecodeFastNext_Dispatch;
  UPB_DECODEFAST_NEXT(next);
}

UPB_FORCEINLINE bool _upb_FastDecoder_DoDecodeUnknown(
    struct upb_Decoder* d, const char** ptr, upb_Message* msg, intptr_t table,
    uint64_t hasbits, uint64_t* data, upb_DecodeFastNext* ret) {
  const char* start = *ptr;
  uint64_t d_val = *data;

  uint32_t tag_len;
  // Important: if the branch is correctly predicted, the tag_len assignment is
  // treated as constant and subsequent loads will not have a data dependency on
  // the branch.
  if (UPB_LIKELY((d_val & 0x80) == 0)) {
    tag_len = 1;
    // Ensure the field number is not 0.
    // Use bitwise op to only examine first byte minus additional tag data.
    if (UPB_UNLIKELY((d_val & 0xF8) == 0)) {
      return UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_Malformed, ret);
    }
  } else if ((d_val & 0x8000) == 0) {
    tag_len = 2;
    // Ensure the field number is not 0.
    // Use bitwise op to limit to first two bytes, and ignore continuation bit &
    // additional tag data.
    if (UPB_UNLIKELY((d_val & 0x7f78) == 0)) {
      return UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_Malformed, ret);
    }
  } else {
    // Tag >=2048
    return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, ret);
  }

  uint32_t wire_type = d_val & 0x07;

  // Assert that the field is either truly unknown or has a mismatched wire
  // type.
#ifndef NDEBUG
  uint32_t field_num;
  if ((d_val & 0x80) == 0) {
    field_num = (uint8_t)d_val >> 3;
  } else {
    field_num = _upb_DecodeFast_Tag2FieldNumber(d_val);
  }
  const upb_MiniTable* mt = decode_totablep(table);
  const upb_MiniTableField* field =
      upb_MiniTable_FindFieldByNumber(mt, field_num);
  UPB_ASSERT(field == NULL ||
             _upb_MiniTableField_GetWireType(field) != wire_type);
#endif

  if (UPB_UNLIKELY(wire_type == kUpb_WireType_EndGroup ||
                   wire_type == kUpb_WireType_StartGroup)) {
    // FastDecoder doesn't handle group fields, but it can be used to decode a
    // message that is itself a group. When decoding a group, the end of the
    // message is marked by an EndGroup tag. Since EndGroup tags are not in
    // the MiniTable, they are routed to the unknown field handler. We must
    // intercept them here to properly terminate the message.
    return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, ret);
  }

  *ptr += tag_len;
  upb_EpsCopyInputStream_StartCapture(&d->input, start);

  switch (wire_type) {
    case kUpb_WireType_Varint:
      *ptr = upb_WireReader_SkipVarint(*ptr, &d->input);
      if (UPB_UNLIKELY(!*ptr)) {
        return UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_Malformed, ret);
      }
      break;
    case kUpb_WireType_32Bit:
      UPB_PRIVATE(upb_EpsCopyInputStream_ConsumeBytes)(&d->input, 4);
      *ptr += 4;
      break;
    case kUpb_WireType_64Bit:
      UPB_PRIVATE(upb_EpsCopyInputStream_ConsumeBytes)(&d->input, 8);
      *ptr += 8;
      break;
    case kUpb_WireType_Delimited: {
      int size;
      const char* p = upb_WireReader_ReadSize(*ptr, &size, &d->input);
      if (UPB_UNLIKELY(!p ||
                       !upb_EpsCopyInputStream_CheckSize(&d->input, p, size))) {
        return UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_Malformed, ret);
      }
      *ptr = p + size;
      break;
    }
    default:
      return UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_Malformed, ret);
  }

  upb_StringView sv;
  if (UPB_UNLIKELY(!upb_EpsCopyInputStream_EndCapture(&d->input, *ptr, &sv))) {
    return UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_Malformed, ret);
  }

  bool handled_fast =
      // Check AddUnknown mode is AliasAllowMerge.
      d->options & kUpb_DecodeOption_AliasString &&
      sv.data != d->input.buffer_start &&
      // Attempt to merge.
      UPB_PRIVATE(_upb_Message_TryAddUnknownAliasAllowMerge)(
          msg, sv.data, sv.size, &d->arena, kUpb_AddUnknown_AliasAllowMerge);
  if (!handled_fast) {
    // Overload `data` for the slow path to avoid repeat computation.
    *data = sv.size;
    *ret = kUpb_DecodeFastNext_DecodeUnknownSlowPath;
    return false;
  }

  *data = 0;
  return true;
}

UPB_PRESERVE_NONE upb_FastDecoder_Return _upb_FastDecoder_DecodeUnknown(
    struct upb_Decoder* d, const char* ptr, upb_Message* msg, intptr_t table,
    uint64_t hasbits, uint64_t data) {
  upb_DecodeFastNext next = kUpb_DecodeFastNext_Dispatch;
  _upb_FastDecoder_DoDecodeUnknown(d, &ptr, msg, table, hasbits, &data, &next);
  UPB_DECODEFAST_NEXTMAYBECOPY(next);
}
