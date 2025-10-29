// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <stdint.h>
#include <string.h>

#include "upb/message/array.h"
#include "upb/message/message.h"
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
void upb_DecodeFast_UnpackedFixed(upb_Decoder* d, const char** ptr,
                                  upb_Message* msg, intptr_t table,
                                  uint64_t* hasbits, uint64_t* data,
                                  upb_DecodeFastNext* ret,
                                  upb_DecodeFast_Type type,
                                  upb_DecodeFast_Cardinality card,
                                  upb_DecodeFast_TagSize tagsize) {
  upb_DecodeFastField field;
  int valbytes = upb_DecodeFast_ValueBytes(type);

  if (!upb_DecodeFast_CheckPackableTag(
          type, card, tagsize, data, kUpb_DecodeFastNext_TailCallPacked, ret) ||
      !Upb_DecodeFast_GetField(d, *ptr, msg, *data, hasbits, ret, &field, type,
                               card)) {
    return;
  }

  do {
    *ptr += upb_DecodeFast_TagSizeBytes(tagsize);
    memcpy(field.dst, *ptr, valbytes);
    *ptr += valbytes;
    _upb_Decoder_Trace(d, 'F');
  } while (upb_DecodeFast_NextRepeated(d, ptr, *data, ret, &field, type, card,
                                       tagsize));
}

UPB_FORCEINLINE
void upb_DecodeFast_PackedFixed(upb_Decoder* d, const char** ptr,
                                upb_Message* msg, intptr_t table,
                                uint64_t* hasbits, uint64_t* data,
                                upb_DecodeFastNext* ret,
                                upb_DecodeFast_Type type,
                                upb_DecodeFast_Cardinality card,
                                upb_DecodeFast_TagSize tagsize) {
  int size;
  upb_DecodeFastField field;
  uint8_t valbytes = upb_DecodeFast_ValueBytes(type);

  const char* data_ptr = *ptr + upb_DecodeFast_TagSizeBytes(tagsize);

  if (!upb_DecodeFast_CheckPackableTag(type, card, tagsize, data,
                                       kUpb_DecodeFastNext_TailCallUnpacked,
                                       ret) ||
      !upb_DecodeFast_DecodeShortSizeForImmediateRead(d, &data_ptr, &size,
                                                      ret)) {
    return;
  }

  if (size != 0) {
    if (size % valbytes != 0) {
      UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_Malformed, ret);
      return;
    }

    if (!upb_DecodeFast_GetArrayForAppend(d, *ptr, msg, *data, hasbits, &field,
                                          type, size / valbytes, ret)) {
      return;
    }

    upb_DecodeFast_InlineMemcpy(field.dst, data_ptr, size);
    upb_DecodeFastField_AddArraySize(&field, size / valbytes);
  }

  *ptr = data_ptr + size;
  _upb_Decoder_Trace(d, 'F');
}

UPB_FORCEINLINE
void upb_DecodeFast_Fixed(upb_Decoder* d, const char** ptr, upb_Message* msg,
                          intptr_t table, uint64_t* hasbits, uint64_t* data,
                          upb_DecodeFastNext* ret, upb_DecodeFast_Type type,
                          upb_DecodeFast_Cardinality card,
                          upb_DecodeFast_TagSize tagsize) {
  if (card == kUpb_DecodeFast_Packed) {
    upb_DecodeFast_PackedFixed(d, ptr, msg, table, hasbits, data, ret, type,
                               card, tagsize);
  } else {
    upb_DecodeFast_UnpackedFixed(d, ptr, msg, table, hasbits, data, ret, type,
                                 card, tagsize);
  }
}

/* Generate all combinations:
 * {s,o,r,p} x {f4,f8} x {1bt,2bt} */

#define F(type, card, tagbytes)                                          \
  UPB_NOINLINE UPB_PRESERVE_NONE const char* UPB_DECODEFAST_FUNCNAME(    \
      type, card, tagbytes)(UPB_PARSE_PARAMS) {                          \
    upb_DecodeFastNext next = kUpb_DecodeFastNext_Dispatch;              \
    upb_DecodeFast_Fixed(d, &ptr, msg, table, &hasbits, &data, &next,    \
                         kUpb_DecodeFast_##type, kUpb_DecodeFast_##card, \
                         kUpb_DecodeFast_##tagbytes);                    \
    UPB_DECODEFAST_NEXTMAYBEPACKED(                                      \
        next, UPB_DECODEFAST_FUNCNAME(type, Repeated, tagbytes),         \
        UPB_DECODEFAST_FUNCNAME(type, Packed, tagbytes));                \
  }

UPB_DECODEFAST_CARDINALITIES(UPB_DECODEFAST_TAGSIZES, F, Fixed32)
UPB_DECODEFAST_CARDINALITIES(UPB_DECODEFAST_TAGSIZES, F, Fixed64)

#undef F
#undef FASTDECODE_FIXED
#undef FASTDECODE_PACKEDFIXED
#undef FASTDECODE_UNPACKEDFIXED

#include "upb/port/undef.inc"
