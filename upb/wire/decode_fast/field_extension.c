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
#include "upb/wire/decode_fast/cardinality.h"
#include "upb/wire/decode_fast/dispatch.h"
#include "upb/wire/decode_fast/field_helpers.h"
#include "upb/wire/decode_fast/field_parsers.h"
#include "upb/wire/internal/decoder.h"

// Must be last.
#include "upb/port/def.inc"

UPB_FORCEINLINE void _upb_FastDecoder_PickHandlerForExtensionOrUnknown(
    struct upb_Decoder* d, intptr_t table, uint64_t data,
    upb_DecodeFastNext* next) {
  uint32_t field_num;
  if (UPB_LIKELY((data & 0x80) == 0)) {
    field_num = (uint8_t)data >> 3;
  } else if ((data & 0x8000) == 0) {
    field_num = _upb_DecodeFast_Tag2FieldNumber(data);
  } else {
    // Tag >=2048.
    UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
    return;
  }

  const upb_MiniTable* mt = decode_totablep(table);

  // Assert that the field is either truly unknown or has a mismatched wire
  // type.
#ifndef NDEBUG
  const upb_MiniTableField* field =
      upb_MiniTable_FindFieldByNumber(mt, field_num);
  UPB_ASSERT(field == NULL ||
             _upb_MiniTableField_GetWireType(field) != (data & 0x07));
#endif

  if (d->extreg && upb_ExtensionRegistry_Lookup(d->extreg, mt, field_num)) {
    _upb_Decoder_Trace(d, 'e');
    UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
    return;
  }

  *next = kUpb_DecodeFastNext_DecodeUnknown;
}

UPB_PRESERVE_NONE upb_FastDecoder_Return
_upb_FastDecoder_DecodeExtensionOrUnknown(struct upb_Decoder* d,
                                          const char* ptr, upb_Message* msg,
                                          intptr_t table, uint64_t hasbits,
                                          uint64_t data) {
  upb_DecodeFastNext next;
  _upb_FastDecoder_PickHandlerForExtensionOrUnknown(d, table, data, &next);
  UPB_DECODEFAST_NEXT(next);
}
