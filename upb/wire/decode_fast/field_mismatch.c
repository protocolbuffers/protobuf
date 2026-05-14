// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <stdint.h>

#include "upb/message/message.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode_fast/cardinality.h"
#include "upb/wire/decode_fast/data.h"
#include "upb/wire/decode_fast/dispatch.h"
#include "upb/wire/decode_fast/field_helpers.h"
#include "upb/wire/decode_fast/field_parsers.h"
#include "upb/wire/internal/decoder.h"

// Must be last.
#include "upb/port/def.inc"

UPB_PRESERVE_NONE upb_FastDecoder_Return _upb_FastDecoder_DecodeMismatchedSlot(
    struct upb_Decoder* d, const char* ptr, upb_Message* msg,
    const upb_MiniTable* table, uint64_t hasbits, uint64_t data,
    uint64_t data2) {
  upb_DecodeFastNext ret = kUpb_DecodeFastNext_FallbackToMiniTable;
  if ((table->UPB_PRIVATE(ext) & kUpb_ExtMode_AllFastFieldsAssigned)) {
    _upb_Decoder_Trace(d, 's');
    ret = UPB_PRIVATE(_upb_MiniTable_ExtModeBase)(table) ==
                  kUpb_ExtMode_NonExtendable
              ? kUpb_DecodeFastNext_DecodeUnknown
              : kUpb_DecodeFastNext_DecodeExtensionOrUnknown;
  } else {
    uint16_t tag = upb_DecodeFastData2_GetOriginalTag(data2);
    if ((tag & 0x80) == 0 || (tag & 0x8000) == 0) {
      _upb_Decoder_Trace(d, 'm');
      uint32_t field_num;
      if ((tag & 0x80) == 0) {
        field_num = (uint8_t)tag >> 3;
      } else {
        field_num = _upb_DecodeFast_Tag2FieldNumber(tag);
      }
      data = field_num;
      ret = UPB_PRIVATE(_upb_MiniTable_ExtModeBase)(table) ==
                    kUpb_ExtMode_NonExtendable
                ? kUpb_DecodeFastNext_CheckMiniTable
                : kUpb_DecodeFastNext_CheckExtRegMiniTable;
    }
  }
  UPB_DECODEFAST_NEXT(ret);
}

UPB_PRESERVE_NONE upb_FastDecoder_Return _upb_FastDecoder_DecodeCheckMiniTable(
    struct upb_Decoder* d, const char* ptr, upb_Message* msg,
    const upb_MiniTable* table, uint64_t hasbits, uint64_t data,
    uint64_t data2) {
  upb_DecodeFastNext ret = kUpb_DecodeFastNext_FallbackToMiniTable;
  uint32_t field_num = data;
  if (upb_MiniTable_FindFieldByNumber(table, field_num) == NULL) {
    ret = kUpb_DecodeFastNext_DecodeUnknown;
  }
  UPB_DECODEFAST_NEXT(ret);
}

UPB_PRESERVE_NONE upb_FastDecoder_Return
_upb_FastDecoder_DecodeCheckExtRegMiniTable(struct upb_Decoder* d,
                                            const char* ptr, upb_Message* msg,
                                            const upb_MiniTable* table,
                                            uint64_t hasbits, uint64_t data,
                                            uint64_t data2) {
  upb_DecodeFastNext ret = kUpb_DecodeFastNext_FallbackToMiniTable;
  uint32_t field_num = data;
  if (d->extreg == NULL ||
      upb_ExtensionRegistry_Lookup(d->extreg, table, field_num) == NULL) {
    ret = kUpb_DecodeFastNext_CheckMiniTable;
  }
  UPB_DECODEFAST_NEXT(ret);
}
