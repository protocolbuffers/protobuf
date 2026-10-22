// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <stdint.h>

#include "upb/message/message.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode.h"
#include "upb/wire/decode_fast/cardinality.h"
#include "upb/wire/decode_fast/combinations.h"
#include "upb/wire/decode_fast/data.h"
#include "upb/wire/decode_fast/dispatch.h"
#include "upb/wire/decode_fast/field_helpers.h"
#include "upb/wire/decode_fast/field_parsers.h"
#include "upb/wire/decode_fast/function_array.h"
#include "upb/wire/decode_fast/select.h"
#include "upb/wire/internal/decoder.h"
#include "upb/wire/internal/eps_copy_input_stream.h"
#include "upb/wire/internal/reader.h"
#include "upb/wire/types.h"

// Must be last.
#include "upb/port/def.inc"

UPB_PRESERVE_NONE upb_FastDecoder_Return _upb_FastDecoder_DecodeMismatchedSlot(
    struct upb_Decoder* d, const char* ptr, upb_Message* msg,
    const upb_MiniTable* table, uint64_t hasbits, uint64_t data,
    uint64_t data2) {
  upb_DecodeFastNext ret;
  if ((table->UPB_PRIVATE(ext) & kUpb_ExtMode_AllFastFieldsAssigned)) {
    _upb_Decoder_Trace(d, 's');
    ret = UPB_PRIVATE(_upb_MiniTable_ExtModeBase)(table) ==
                  kUpb_ExtMode_NonExtendable
              ? kUpb_DecodeFastNext_DecodeUnknown
              : kUpb_DecodeFastNext_DecodeExtensionOrUnknown;
  } else {
    uint16_t tag = upb_DecodeFastData2_GetOriginalTag(data2);
    _upb_Decoder_Trace(d, 'm');
    UPB_ASSERT(tag == _upb_FastDecoder_LoadTag(ptr));
    if ((tag & 0x7) == kUpb_WireType_StartGroup ||
        (tag & 0x7) == kUpb_WireType_EndGroup) {
      ret = kUpb_DecodeFastNext_FallbackToMiniTable;
    } else if ((tag & 0x80) == 0 || (tag & 0x8000) == 0) {
      uint32_t field_num;
      uint32_t count;
      if ((tag & 0x80) == 0) {
        field_num = (uint8_t)tag >> 3;
        count = 1;
      } else {
        field_num = _upb_DecodeFast_Tag2FieldNumber(tag);
        count = 2;
      }
      if (UPB_UNLIKELY(field_num == 0)) {
        UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_Malformed, &ret);
      } else {
        data = field_num;
        data2 =
            upb_DecodeFastData2_PackWireTypeAndTagLen(data2, tag & 0x7, count);
        ret = UPB_PRIVATE(_upb_MiniTable_ExtModeBase)(table) ==
                      kUpb_ExtMode_NonExtendable
                  ? kUpb_DecodeFastNext_CheckMiniTable
                  : kUpb_DecodeFastNext_CheckExtRegMiniTable;
      }
    } else {
      ret = kUpb_DecodeFastNext_DecodeLongTag;
    }
  }
  UPB_DECODEFAST_NEXT(ret);
}

UPB_PRESERVE_NONE upb_FastDecoder_Return _upb_FastDecoder_DecodeLongTag(
    struct upb_Decoder* d, const char* ptr, upb_Message* msg,
    const upb_MiniTable* table, uint64_t hasbits, uint64_t data,
    uint64_t data2) {
  upb_DecodeFastNext ret = kUpb_DecodeFastNext_FallbackToMiniTable;
  uint16_t tag = upb_DecodeFastData2_GetOriginalTag(data2);

  uint32_t field_num, tag_len;
  if (UPB_UNLIKELY(
          !_upb_DecodeFast_ParseLongTag(ptr, tag, &field_num, &tag_len) ||
          field_num == 0)) {
    UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_Malformed, &ret);
    UPB_DECODEFAST_NEXT(ret);
  }

  data = field_num;
  data2 = upb_DecodeFastData2_PackWireTypeAndTagLen(data2, tag & 0x7, tag_len);
  ret = UPB_PRIVATE(_upb_MiniTable_ExtModeBase)(table) ==
                kUpb_ExtMode_NonExtendable
            ? kUpb_DecodeFastNext_CheckMiniTable
            : kUpb_DecodeFastNext_CheckExtRegMiniTable;
  UPB_DECODEFAST_NEXT(ret);
}

UPB_FORCEINLINE _upb_FieldParser* _upb_FastDecoder_TryGetFieldParser(
    const upb_MiniTable* table, const upb_MiniTableField* field,
    const char* ptr, uint64_t* data, uint64_t* data2, upb_DecodeFastNext* ret) {
  UPB_ASSUME(upb_MiniTableField_IsMap(field) ||
             upb_MiniTableField_IsArray(field));
  bool supported_tag_size;
  upb_DecodeFast_TableEntry entry;
  bool ok =
      upb_DecodeFast_TryFillEntry(table, field, &supported_tag_size, &entry);
  UPB_ASSUME(ok);
  uint16_t actual_tag = _upb_FastDecoder_LoadTag(ptr);
  uint16_t expected_tag =
      upb_DecodeFastData_GetExpectedTag(entry.function_data);
  upb_DecodeFast_TagSize tag_size =
      expected_tag > 0x7f ? kUpb_DecodeFast_Tag2Byte : kUpb_DecodeFast_Tag1Byte;
  uint64_t new_data2 = upb_DecodeFastData2_PackOriginalTag(*data2, actual_tag);
  if (UPB_UNLIKELY(
          !upb_DecodeFast_TagMatches(expected_tag, actual_tag, tag_size))) {
    if (upb_MiniTableField_IsMap(field)) {
      UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, ret);
      return NULL;
    }
    upb_DecodeFast_Type type = upb_DecodeFast_GetType(entry.function_idx);
    upb_DecodeFast_Cardinality card =
        upb_DecodeFast_GetCardinality(entry.function_idx);
    if (upb_DecodeFast_WireType(type) == kUpb_WireType_Delimited ||
        !upb_DecodeFast_TryFlipPacked(type, card, tag_size,
                                      &entry.function_data, new_data2)) {
      UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, ret);
      return NULL;
    }
    entry.function_idx ^= (kUpb_DecodeFast_Repeated ^ kUpb_DecodeFast_Packed)
                          << 1;
  }
  UPB_ASSERT(upb_DecodeFastData2_GetTagLen(*data2) <= 2);
  *data = entry.function_data;
  *data2 = new_data2;
  *ret = kUpb_DecodeFastNext_TailCallUnpacked;
  return upb_DecodeFast_GetFunctionPointer(entry.function_idx);
}

UPB_PRESERVE_NONE upb_FastDecoder_Return _upb_FastDecoder_DecodeCollidingField(
    struct upb_Decoder* d, const char* ptr, upb_Message* msg,
    const upb_MiniTable* table, uint64_t hasbits, uint64_t data,
    uint64_t data2) {
  const upb_MiniTableField* field = (const upb_MiniTableField*)(uintptr_t)data;
  upb_DecodeFastNext ret;
  _upb_FieldParser* parser = _upb_FastDecoder_TryGetFieldParser(
      table, field, ptr, &data, &data2, &ret);
  UPB_DECODEFAST_NEXTMAYBEPACKED(ret, parser, upb_DecodeFast_Unreachable);
}

UPB_PRESERVE_NONE upb_FastDecoder_Return _upb_FastDecoder_DecodeCheckMiniTable(
    struct upb_Decoder* d, const char* ptr, upb_Message* msg,
    const upb_MiniTable* table, uint64_t hasbits, uint64_t data,
    uint64_t data2) {
  uint32_t field_num = data;
#ifndef NDEBUG
  int guaranteed =
      UPB_PRIVATE(upb_EpsCopyInputStream_GetBoundsCheckedBytes)(EPS(d));
  uint32_t check;
  const char* read = upb_WireReader_ReadTag(ptr, &check, EPS(d));
  UPB_PRIVATE(upb_EpsCopyInputStream_BoundsChecked)(EPS(d), guaranteed);
  UPB_ASSERT(upb_WireReader_GetFieldNumber(check) == field_num);
  UPB_ASSERT(ptr + upb_DecodeFastData2_GetTagLen(data2) == read);
#endif
  uint32_t gap_lo, gap_hi;
  const upb_MiniTableField* field;
  upb_DecodeFastNext ret;
  if (UPB_PRIVATE(_upb_MiniTable_FindFieldOrUnknownGap)(
          table, field_num, &gap_lo, &gap_hi, &field)) {
    data = upb_DecodeFast_PackGaps(gap_lo, gap_hi);
    ret = kUpb_DecodeFastNext_DecodeUnknownValue;
  } else if (UPB_PRIVATE(_upb_MiniTableField_IsFastTableCollision)(field)) {
    data = (uintptr_t)field;
    ret = kUpb_DecodeFastNext_DecodeCollidingField;
  } else {
    ret = kUpb_DecodeFastNext_FallbackToMiniTable;
  }
  UPB_DECODEFAST_NEXT(ret);
}

UPB_PRESERVE_NONE upb_FastDecoder_Return
_upb_FastDecoder_DecodeCheckExtRegMiniTable(struct upb_Decoder* d,
                                            const char* ptr, upb_Message* msg,
                                            const upb_MiniTable* table,
                                            uint64_t hasbits, uint64_t data,
                                            uint64_t data2) {
  upb_DecodeFastNext ret = kUpb_DecodeFastNext_CheckMiniTable;
  if (d->extreg != NULL) {
    uint32_t field_num = data;
#ifndef NDEBUG
    int guaranteed =
        UPB_PRIVATE(upb_EpsCopyInputStream_GetBoundsCheckedBytes)(EPS(d));
    uint32_t check;
    const char* read = upb_WireReader_ReadTag(ptr, &check, EPS(d));
    UPB_PRIVATE(upb_EpsCopyInputStream_BoundsChecked)(EPS(d), guaranteed);
    UPB_ASSERT(upb_WireReader_GetFieldNumber(check) == field_num);
    UPB_ASSERT(ptr + upb_DecodeFastData2_GetTagLen(data2) == read);
#endif
    const upb_MiniTableExtension* ext =
        upb_ExtensionRegistry_Lookup(d->extreg, table, field_num);
    if (ext) {
      ret = kUpb_DecodeFastNext_FallbackToMiniTable;
    }
  }
  UPB_DECODEFAST_NEXT(ret);
}
