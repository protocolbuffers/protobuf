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

static bool upb_DecodeFast_SingleFixed(upb_Decoder* d, const char** ptr,
                                       void* dst, upb_DecodeFast_Type type,
                                       upb_DecodeFastNext* next) {
  int valbytes = upb_DecodeFast_ValueBytes(type);
  memcpy(dst, *ptr, valbytes);
  *ptr += valbytes;
  return true;
}

typedef struct {
  upb_Decoder* d;
  upb_DecodeFast_Type type;
  upb_Message* msg;
  uint64_t* data;
  uint64_t* hasbits;
  upb_DecodeFastNext* ret;
} upb_DecodeFast_PackedFixedContext;

static const char* upb_DecodeFast_PackedFixed(upb_EpsCopyInputStream* st,
                                              const char* ptr, int size,
                                              void* ctx) {
  upb_DecodeFast_PackedFixedContext* c =
      (upb_DecodeFast_PackedFixedContext*)ctx;

  int valbytes = upb_DecodeFast_ValueBytes(c->type);

  if (size != 0) {
    if (size % valbytes != 0) {
      UPB_DECODEFAST_ERROR(c->d, kUpb_DecodeStatus_Malformed, c->ret);
      return NULL;
    }

    upb_DecodeFastArray arr;

    if (!upb_DecodeFast_GetArrayForAppend(c->d, ptr, c->msg, *c->data,
                                          c->hasbits, &arr, c->type,
                                          size / valbytes, c->ret)) {
      return NULL;
    }

    upb_DecodeFast_InlineMemcpy(arr.dst, ptr, size);
    arr.dst = UPB_PTR_AT(arr.dst, size, char);
    upb_DecodeFastField_SetArraySize(&arr, c->type);
  }

  _upb_Decoder_Trace(c->d, 'F');
  return ptr + size;
}

UPB_FORCEINLINE
void upb_DecodeFast_Fixed(upb_Decoder* d, const char** ptr, upb_Message* msg,
                          intptr_t table, uint64_t* hasbits, uint64_t* data,
                          upb_DecodeFastNext* ret, upb_DecodeFast_Type type,
                          upb_DecodeFast_Cardinality card,
                          upb_DecodeFast_TagSize tagsize) {
  if (card == kUpb_DecodeFast_Packed) {
    upb_DecodeFast_PackedFixedContext ctx = {
        .d = d,
        .type = type,
        .msg = msg,
        .data = data,
        .hasbits = hasbits,
        .ret = ret,
    };
    upb_DecodeFast_Delimited(d, ptr, type, card, tagsize, data,
                             &upb_DecodeFast_PackedFixed, ret, &ctx);
  } else {
    upb_DecodeFast_Unpacked(d, ptr, msg, data, hasbits, ret, type, card,
                            tagsize, &upb_DecodeFast_SingleFixed);
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
