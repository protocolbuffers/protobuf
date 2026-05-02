// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
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

UPB_PRESERVE_NONE const char* _upb_FastDecoder_DecodeMismatchedSlot(
    struct upb_Decoder* d, const char* ptr, upb_Message* msg, intptr_t table,
    uint64_t hasbits, uint64_t data) {
  upb_DecodeFastNext ret = kUpb_DecodeFastNext_FallbackToMiniTable;
  const upb_MiniTable* mt = decode_totablep(table);
  if ((mt->UPB_PRIVATE(ext) & kUpb_ExtMode_AllFastFieldsAssigned)) {
    _upb_Decoder_Trace(d, 's');
    // Restore the tag to clear masking.
    data = _upb_FastDecoder_LoadTag(ptr);
    ret = UPB_PRIVATE(_upb_MiniTable_ExtModeBase)(mt) ==
                  kUpb_ExtMode_NonExtendable
              ? kUpb_DecodeFastNext_DecodeUnknown
              : kUpb_DecodeFastNext_DecodeExtensionOrUnknown;
  }
  UPB_DECODEFAST_NEXT(ret);
}
