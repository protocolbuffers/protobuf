// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <stdint.h>

#include "upb/base/descriptor_constants.h"
#include "upb/base/string_view.h"
#include "upb/message/internal/message.h"
#include "upb/message/message.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode.h"
#include "upb/wire/decode_fast/cardinality.h"
#include "upb/wire/decode_fast/data.h"
#include "upb/wire/decode_fast/dispatch.h"
#include "upb/wire/decode_fast/field_helpers.h"
#include "upb/wire/decode_fast/field_parsers.h"
#include "upb/wire/eps_copy_input_stream.h"
#include "upb/wire/internal/decoder.h"
#include "upb/wire/internal/eps_copy_input_stream.h"
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
                                       upb_Message* msg,
                                       const upb_MiniTable* table,
                                       uint64_t hasbits, uint64_t data,
                                       uint64_t data2) {
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

UPB_PRESERVE_NONE upb_FastDecoder_Return _upb_FastDecoder_DecodeUnknown(
    struct upb_Decoder* d, const char* ptr, upb_Message* msg,
    const upb_MiniTable* table, uint64_t hasbits, uint64_t data,
    uint64_t data2) {
  upb_DecodeFastNext next = kUpb_DecodeFastNext_Dispatch;
  uint16_t tag = upb_DecodeFastData2_GetOriginalTag(data2);

  if (UPB_UNLIKELY((tag & 0xFF80) == 0x80)) {
    // An one byte tag encoded as two bytes may map to the wrong fasttable slot
    // and lead to being assumed to be an unknown field, in a message where all
    // fasttable-eligible fields are assigned slots. If we're passed such a tag,
    // fall back to the minitable rather than adding it as unknown.
    UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, &next);
  } else if (UPB_UNLIKELY((tag & 0x8080) == 0x8080)) {
    // A one byte tag encoded as three may map to the wrong slot; a two byte tag
    // encoded as three will go to the same slot, but could hit the tag mismatch
    // path after dispatch.
    next = kUpb_DecodeFastNext_DecodeLongTag;
  } else {
    uint32_t field_num;
    uint32_t tag_len;
    uint32_t wire_type = tag & 0x07;
    if (UPB_UNLIKELY(
            !_upb_DecodeFast_ParseTag(ptr, tag, &field_num, &tag_len))) {
      UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_Malformed, &next);
    } else if (UPB_UNLIKELY((field_num) == 0)) {
      UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_Malformed, &next);
    } else {
#ifndef NDEBUG
      const upb_MiniTableField* field =
          upb_MiniTable_FindFieldByNumber(table, field_num);
      UPB_ASSERT(field == NULL ||
                 _upb_MiniTableField_GetWireType(field) != wire_type ||
                 (upb_MiniTableField_CType(field) == kUpb_CType_Message &&
                  upb_MiniTable_GetSubMessageTable(field) == NULL));
#else
      UPB_UNUSED(field_num);
#endif
      if (UPB_UNLIKELY(wire_type == kUpb_WireType_EndGroup ||
                       wire_type == kUpb_WireType_StartGroup)) {
        // FastDecoder doesn't handle group fields, but it can be used to decode
        // a message that is itself a group. When decoding a group, the end of
        // the message is marked by an EndGroup tag. Since EndGroup tags are not
        // in the MiniTable, they are routed to the unknown field handler. We
        // must intercept them here to properly terminate the message.
        UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, &next);
      } else {
        data = upb_DecodeFast_PackGaps(0, 0);
        data2 = upb_DecodeFastData2_PackWireTypeAndTagLen(data2, wire_type,
                                                          tag_len);
        next = kUpb_DecodeFastNext_DecodeUnknownValue;
      }
    }
  }
  UPB_DECODEFAST_NEXTMAYBECOPY(next);
}

UPB_FORCEINLINE bool _upb_FastDecoder_DoDecodeUnknown(
    struct upb_Decoder* d, const char** ptr, upb_Message* msg,
    const upb_MiniTable* table, uint64_t hasbits, uint64_t* data,
    upb_DecodeFastNext* ret, uint32_t tag_len, uint32_t wire_type,
    uint32_t gap_lo, uint32_t gap_hi) {
  const char* start = *ptr - tag_len;
  upb_EpsCopyCapture capture;
  upb_EpsCopyCapture_Start(&capture, &d->input, start);

  while (true) {
    switch (wire_type) {
      case kUpb_WireType_Varint:
        *ptr = upb_WireReader_SkipVarint(*ptr, EPS(d));
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
        const char* p = upb_WireReader_ReadSize(*ptr, &size, EPS(d));
        if (UPB_UNLIKELY(!upb_EpsCopyInputStream_CheckSize(EPS(d), p, size))) {
          return UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_Malformed, ret);
        }
        *ptr = p + size;
        break;
      }
      default:
        return UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_Malformed, ret);
    }

    int overrun;
    if (UPB_UNLIKELY(UPB_PRIVATE(upb_EpsCopyInputStream_IsDoneStatus)(
                         &d->input, *ptr, &overrun) !=
                     kUpb_IsDoneStatus_NotDone)) {
      break;
    }

    uint16_t peek_tag = _upb_FastDecoder_LoadTag(*ptr);
    uint32_t next_wire_type = peek_tag & 0x07;
    if (UPB_UNLIKELY(next_wire_type == kUpb_WireType_StartGroup ||
                     next_wire_type == kUpb_WireType_EndGroup)) {
      break;
    }
    uint32_t next_tag_len;
    uint32_t next_field_num;
    if (UPB_UNLIKELY(!_upb_DecodeFast_ParseTag(*ptr, peek_tag, &next_field_num,
                                               &next_tag_len))) {
      return UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_Malformed, ret);
    }

    if (UPB_UNLIKELY(next_field_num <= gap_lo || next_field_num >= gap_hi)) {
      if (UPB_LIKELY(next_field_num == gap_hi)) {
        // Common case of fields in ascending order encountering a known field
        break;
      }
      if (UPB_UNLIKELY(next_field_num == 0)) {
        return UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_Malformed, ret);
      }
      if (next_field_num == gap_lo) {
        break;
      }
      if (!UPB_PRIVATE(_upb_MiniTable_FindUnknownGap)(table, next_field_num,
                                                      &gap_lo, &gap_hi)) {
        break;
      }
    }

    if (UPB_UNLIKELY(UPB_PRIVATE(_upb_MiniTable_ExtModeBase)(table) ==
                     kUpb_ExtMode_Extendable) &&
        d->extreg) {
      // TODO: currently this is not an inlinable function call and thus acts as
      // a clobber for LICM/GVN. We should be able to eliminate a bunch of loads
      // within this loop once it's inlined.
      if (upb_ExtensionRegistry_Lookup(d->extreg, table, next_field_num)) {
        break;
      }
    }

    wire_type = next_wire_type;
    *ptr += next_tag_len;
  }

  upb_StringView sv;
  upb_EpsCopyCapture_End(&capture, EPS(d), *ptr, &sv);

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

UPB_PRESERVE_NONE upb_FastDecoder_Return _upb_FastDecoder_DecodeUnknownValue(
    struct upb_Decoder* d, const char* ptr, upb_Message* msg,
    const upb_MiniTable* table, uint64_t hasbits, uint64_t data,
    uint64_t data2) {
  upb_DecodeFastNext next = kUpb_DecodeFastNext_Dispatch;
  uint32_t wire_type = upb_DecodeFastData2_GetWireType(data2);
  uint32_t tag_len = upb_DecodeFastData2_GetTagLen(data2);
  UPB_ASSUME(tag_len > 0);
  uint32_t gap_lo = upb_DecodeFast_GetGapLo(data);
  uint32_t gap_hi = upb_DecodeFast_GetGapHi(data);
  ptr += tag_len;
  _upb_FastDecoder_DoDecodeUnknown(d, &ptr, msg, table, hasbits, &data, &next,
                                   tag_len, wire_type, gap_lo, gap_hi);
  UPB_DECODEFAST_NEXTMAYBECOPY(next);
}
