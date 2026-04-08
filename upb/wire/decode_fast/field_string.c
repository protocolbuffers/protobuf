// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "upb/base/string_view.h"
#include "upb/wire/decode.h"
#include "upb/wire/decode_fast/cardinality.h"
#include "upb/wire/decode_fast/combinations.h"
#include "upb/wire/decode_fast/dispatch.h"
#include "upb/wire/eps_copy_input_stream.h"
#include "upb/wire/internal/decoder.h"

// Must be last.
#include "upb/port/def.inc"

static bool upb_DecodeFast_SingleString(upb_Decoder* d, const char** ptr,
                                        void* dst, upb_DecodeFast_Type type,
                                        upb_DecodeFastNext* next) {
  bool validate_utf8 = type == kUpb_DecodeFast_String;
  upb_StringView* sv = dst;
  int size;

  if (!upb_DecodeFast_DecodeSize(d, ptr, &size, next)) return false;

  if (!_upb_Decoder_ReadString(d, ptr, size, sv, validate_utf8)) {
    sv->size = 0;
    // TODO: ReadString can actually return NULL for invalid wire format.
    // Need to fix ReadString to return a more granular error.
    return UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_OutOfMemory, next);
  }

  return true;
}

/* Generate all combinations:
 * p x {s,o,r} x {s, b} x {1bt,2bt} */

#define F(type, card, tagsize)                                              \
  const char* UPB_PRESERVE_NONE UPB_DECODEFAST_FUNCNAME(                    \
      type, card, tagsize)(UPB_PARSE_PARAMS) {                              \
    upb_DecodeFastNext next = kUpb_DecodeFastNext_Dispatch;                 \
    upb_DecodeFast_Unpacked(d, &ptr, msg, &data, &hasbits, &next,           \
                            kUpb_DecodeFast_##type, kUpb_DecodeFast_##card, \
                            kUpb_DecodeFast_##tagsize,                      \
                            &upb_DecodeFast_SingleString);                  \
    UPB_DECODEFAST_NEXT(next);                                              \
  }

UPB_DECODEFAST_CARDINALITIES(UPB_DECODEFAST_TAGSIZES, F, String)
UPB_DECODEFAST_CARDINALITIES(UPB_DECODEFAST_TAGSIZES, F, Bytes)

#undef F

#undef F
#undef FASTDECODE_LONGSTRING
#undef FASTDECODE_COPYSTRING
#undef FASTDECODE_STRING
