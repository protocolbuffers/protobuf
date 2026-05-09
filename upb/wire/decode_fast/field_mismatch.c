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
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode.h"
#include "upb/wire/decode_fast/cardinality.h"
#include "upb/wire/decode_fast/data.h"
#include "upb/wire/decode_fast/dispatch.h"
#include "upb/wire/decode_fast/field_helpers.h"
#include "upb/wire/decode_fast/field_parsers.h"
#include "upb/wire/internal/decoder.h"
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
    UPB_ASSERT(tag == _upb_FastDecoder_LoadTag(ptr));
    if ((tag & 0x7) == kUpb_WireType_StartGroup ||
        (tag & 0x7) == kUpb_WireType_EndGroup) {
      ret = kUpb_DecodeFastNext_FallbackToMiniTable;
    } else if ((tag & 0x80) == 0 || (tag & 0x8000) == 0) {
      UPB_PRIVATE(upb_EpsCopyInputStream_ConsumeBytes)(&d->input, 2);
      uint32_t field_num;
      uint32_t count;
      if ((tag & 0x80) == 0) {
        field_num = (uint8_t)tag >> 3;
        ptr++;
        count = 1;
      } else {
        field_num = _upb_DecodeFast_Tag2FieldNumber(tag);
        ptr += 2;
        count = 2;
      }
      data = field_num;
      data2 =
          upb_DecodeFastData2_PackWireTypeAndTagLen(data2, tag & 0x7, count);
      ret = UPB_PRIVATE(_upb_MiniTable_ExtModeBase)(table) ==
                    kUpb_ExtMode_NonExtendable
                ? kUpb_DecodeFastNext_CheckMiniTable
                : kUpb_DecodeFastNext_CheckExtRegMiniTable;
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
  UPB_PRIVATE(upb_EpsCopyInputStream_ConsumeBytes)(&d->input, 5);
  uint32_t field_num = (tag & 0x7f00) >> 1 | (tag & 0x7f);
  int count = 3;
  field_num |= (uint32_t)(ptr[2] & 0x7f) << 14;
  if ((ptr[2] & 0x80) != 0) {
    count = 4;
    field_num |= (uint32_t)(ptr[3] & 0x7f) << 21;
    if ((ptr[3] & 0x80) != 0) {
      count = 5;
      field_num |= (uint32_t)(ptr[4] & 0x7f) << 28;
      if (UPB_UNLIKELY((ptr[4] & 0x80) != 0 || (ptr[4] & 0xf0) != 0)) {
        UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_Malformed, &ret);
        UPB_DECODEFAST_NEXT(ret);
      }
    }
  }
  ptr += count;
  field_num >>= 3;
  data = field_num;
  data2 = upb_DecodeFastData2_PackWireTypeAndTagLen(data2, tag & 0x7, count);
  ret = UPB_PRIVATE(_upb_MiniTable_ExtModeBase)(table) ==
                kUpb_ExtMode_NonExtendable
            ? kUpb_DecodeFastNext_CheckMiniTable
            : kUpb_DecodeFastNext_CheckExtRegMiniTable;
  UPB_DECODEFAST_NEXT(ret);
}

UPB_PRESERVE_NONE upb_FastDecoder_Return _upb_FastDecoder_DecodeCheckMiniTable(
    struct upb_Decoder* d, const char* ptr, upb_Message* msg,
    const upb_MiniTable* table, uint64_t hasbits, uint64_t data,
    uint64_t data2) {
  uint32_t field_num = data;
#ifndef NDEBUG
  uint32_t check;
  UPB_PRIVATE(upb_EpsCopyInputStream_BoundsChecked)(&d->input);
  const char* read = upb_WireReader_ReadTag(
      ptr - upb_DecodeFastData2_GetTagLen(data2), &check, &d->input);
  UPB_ASSERT(upb_WireReader_GetFieldNumber(check) == field_num);
  UPB_ASSERT(ptr == read);
#endif
  const upb_MiniTableField* field =
      upb_MiniTable_FindFieldByNumber(table, field_num);
  upb_DecodeFastNext ret;
  if (field) {
    uint64_t index = field - table->UPB_PRIVATE(fields);
    data =
        upb_DecodeFast_PackPassBack(index, upb_DecodeFastData2_GetTagLen(data2),
                                    upb_DecodeFastData2_GetWireType(data2));
    ret = kUpb_DecodeFastNext_FallbackToKnownField;
  } else {
    ret = kUpb_DecodeFastNext_DecodeUnknownValue;
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
    UPB_PRIVATE(upb_EpsCopyInputStream_BoundsChecked)(&d->input);
    uint32_t check;
    const char* read = upb_WireReader_ReadTag(
        ptr - upb_DecodeFastData2_GetTagLen(data2), &check, &d->input);

    UPB_ASSERT(upb_WireReader_GetFieldNumber(check) == field_num);
    UPB_ASSERT(ptr == read);
#endif
    const upb_MiniTableExtension* ext =
        upb_ExtensionRegistry_Lookup(d->extreg, table, field_num);
    if (ext) {
      // Back up by count as we aren't passing back the found extension
      ptr -= upb_DecodeFastData2_GetTagLen(data2);
      UPB_PRIVATE(upb_EpsCopyInputStream_BoundsChecked)(&d->input);
      ret = kUpb_DecodeFastNext_FallbackToMiniTable;
    }
  }
  UPB_DECODEFAST_NEXT(ret);
}
