// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <stdint.h>

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
#include "upb/wire/internal/decoder.h"

// Must be last.
#include "upb/port/def.inc"

UPB_FORCEINLINE void _upb_FastDecoder_PickHandlerForExtensionOrUnknown(
    struct upb_Decoder* d, const char* ptr, const upb_MiniTable* table,
    uint16_t tag, upb_DecodeFastNext* next) {
  uint32_t field_num;
  uint32_t tag_len;
  if (UPB_UNLIKELY(!_upb_DecodeFast_ParseTag(ptr, tag, &field_num, &tag_len))) {
    UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_Malformed, next);
    return;
  }

  // Assert that the field is either truly unknown or has a mismatched wire
  // type.
#ifndef NDEBUG
  const upb_MiniTableField* field =
      upb_MiniTable_FindFieldByNumber(table, field_num);
  UPB_ASSERT(field == NULL ||
             _upb_MiniTableField_GetWireType(field) != (tag & 0x07));
#endif

  if (d->extreg && upb_ExtensionRegistry_Lookup(d->extreg, table, field_num)) {
    _upb_Decoder_Trace(d, 'e');
    UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
    return;
  }

  *next = kUpb_DecodeFastNext_DecodeUnknown;
}

UPB_PRESERVE_NONE upb_FastDecoder_Return
_upb_FastDecoder_DecodeExtensionOrUnknown(struct upb_Decoder* d,
                                          const char* ptr, upb_Message* msg,
                                          const upb_MiniTable* table,
                                          uint64_t hasbits, uint64_t data,
                                          uint64_t data2) {
  upb_DecodeFastNext next;
  uint16_t tag = upb_DecodeFastData2_GetOriginalTag(data2);
  _upb_FastDecoder_PickHandlerForExtensionOrUnknown(d, ptr, table, tag, &next);
  UPB_DECODEFAST_NEXT(next);
}
