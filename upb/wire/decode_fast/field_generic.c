// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <stdint.h>

#include "upb/message/message.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode_fast/cardinality.h"
#include "upb/wire/decode_fast/dispatch.h"
#include "upb/wire/decode_fast/field_parsers.h"
#include "upb/wire/internal/decoder.h"

// Must be last.
#include "upb/port/def.inc"

UPB_PRESERVE_NONE upb_FastDecoder_Return _upb_FastDecoder_FallbackToMiniTable(
    struct upb_Decoder* d, const char* ptr, upb_Message* msg,
    const upb_MiniTable* table, uint64_t hasbits, uint64_t data,
    uint64_t data2) {
  (void)data;
  (void)data2;
  upb_DecodeFast_SetHasbits(msg, hasbits);
  return (upb_FastDecoder_Return){.ptr = ptr};
}

UPB_PRESERVE_NONE upb_FastDecoder_Return _upb_FastDecoder_DecodeGeneric(
    struct upb_Decoder* d, const char* ptr, upb_Message* msg,
    const upb_MiniTable* table, uint64_t hasbits, uint64_t data,
    uint64_t data2) {
  upb_DecodeFastNext ret;
  UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, &ret);
  return _upb_FastDecoder_FallbackToMiniTable(d, ptr, msg, table, hasbits, data,
                                              data2);
}
