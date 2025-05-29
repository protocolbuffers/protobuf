// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <stdint.h>
#include <string.h>

#include "upb/message/array.h"
#include "upb/wire/decode.h"
#include "upb/wire/decode_fast/cardinality.h"
#include "upb/wire/decode_fast/combinations.h"
#include "upb/wire/decode_fast/dispatch.h"
#include "upb/wire/decode_fast/field_parsers.h"
#include "upb/wire/eps_copy_input_stream.h"
#include "upb/wire/internal/decoder.h"

// Must be last.
#include "upb/port/def.inc"

UPB_FORCEINLINE
int upb_DecodeFast_Fixed(upb_Decoder* d, const char** ptr, upb_Message* msg,
                         intptr_t table, uint64_t* hasbits, uint64_t* data,
                         upb_DecodeFast_Type type,
                         upb_DecodeFast_Cardinality card,
                         upb_DecodeFast_TagSize tagsize) {
  upb_DecodeFastNext ret;
  upb_DecodeFastField field;
  int valbytes = upb_DecodeFast_ValueBytes(type);

  if (!upb_DecodeFast_CheckTag(*data, tagsize, &ret) ||
      !Upb_DecodeFast_GetField(d, *ptr, msg, *data, hasbits, &ret, &field, type,
                               card)) {
    return ret;
  }

  *ptr += upb_DecodeFast_TagSizeBytes(tagsize);
  memcpy(field.dst, *ptr, valbytes);
  *ptr += valbytes;
  _upb_Decoder_Trace(d, 'F');

  return kUpb_DecodeFastNext_TailCallDispatch;
}

#define F(type, card, tagbytes)                                       \
  UPB_NOINLINE const char* UPB_DECODEFAST_FUNCNAME(                   \
      type, card, tagbytes)(UPB_PARSE_PARAMS) {                       \
    int next = upb_DecodeFast_Fixed(                                  \
        d, &ptr, msg, table, &hasbits, &data, kUpb_DecodeFast_##type, \
        kUpb_DecodeFast_##card, kUpb_DecodeFast_##tagbytes);          \
    UPB_DECODEFAST_NEXT(next);                                        \
  }

UPB_DECODEFAST_TAGSIZES(F, Fixed32, Scalar)
UPB_DECODEFAST_TAGSIZES(F, Fixed64, Scalar)
UPB_DECODEFAST_TAGSIZES(F, Fixed32, Oneof)
UPB_DECODEFAST_TAGSIZES(F, Fixed64, Oneof)

#undef F
#undef FASTDECODE_FIXED
#undef FASTDECODE_PACKEDFIXED
#undef FASTDECODE_UNPACKEDFIXED

#include "upb/port/undef.inc"
