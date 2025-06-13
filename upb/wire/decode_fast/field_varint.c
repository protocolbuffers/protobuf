// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <stddef.h>
#include <stdint.h>
#include <string.h>

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
uint64_t fastdecode_munge(uint64_t val, int valbytes, bool zigzag) {
  if (valbytes == 1) {
    return val != 0;
  } else if (zigzag) {
    if (valbytes == 4) {
      uint32_t n = val;
      return (n >> 1) ^ -(int32_t)(n & 1);
    } else if (valbytes == 8) {
      return (val >> 1) ^ -(int64_t)(val & 1);
    }
    UPB_UNREACHABLE();
  }
  return val;
}

UPB_FORCEINLINE
const char* fastdecode_varint64(const char* ptr, uint64_t* val) {
  ptr++;
  *val = (uint8_t)ptr[-1];
  if (UPB_UNLIKELY(*val & 0x80)) {
    int i;
    for (i = 0; i < 8; i++) {
      ptr++;
      uint64_t byte = (uint8_t)ptr[-1];
      *val += (byte - 1) << (7 + 7 * i);
      if (UPB_LIKELY((byte & 0x80) == 0)) goto done;
    }
    ptr++;
    uint64_t byte = (uint8_t)ptr[-1];
    if (byte > 1) {
      return NULL;
    }
    *val += (byte - 1) << 63;
  }
done:
  UPB_ASSUME(ptr != NULL);
  return ptr;
}

#define FASTDECODE_UNPACKEDVARINT(packed, card, d, ptr, msg, table, hasbits,  \
                                  data, type, tagsize)                        \
  uint64_t val;                                                               \
  void* dst;                                                                  \
  fastdecode_arr farr;                                                        \
  int valbytes = upb_DecodeFast_ValueBytes(type);                             \
  bool zigzag = upb_DecodeFast_IsZigZag(type);                                \
  int tagbytes = upb_DecodeFast_TagSizeBytes(tagsize);                        \
                                                                              \
  FASTDECODE_CHECKPACKED(tagbytes, card, packed);                             \
                                                                              \
  dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, &farr, valbytes,    \
                            card);                                            \
  if (card == kUpb_DecodeFast_Repeated) {                                     \
    if (UPB_UNLIKELY(!dst)) {                                                 \
      RETURN_GENERIC("need array resize\n");                                  \
    }                                                                         \
  }                                                                           \
                                                                              \
  again:                                                                      \
  if (card == kUpb_DecodeFast_Repeated) {                                     \
    dst = fastdecode_resizearr(d, dst, &farr, valbytes);                      \
  }                                                                           \
                                                                              \
  ptr += tagbytes;                                                            \
  ptr = fastdecode_varint64(ptr, &val);                                       \
  if (ptr == NULL) _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed); \
  val = fastdecode_munge(val, valbytes, zigzag);                              \
  memcpy(dst, &val, valbytes);                                                \
                                                                              \
  if (card == kUpb_DecodeFast_Repeated) {                                     \
    fastdecode_nextret ret = fastdecode_nextrepeated(                         \
        d, dst, &ptr, &farr, data, tagbytes, valbytes);                       \
    switch (ret.next) {                                                       \
      case FD_NEXT_SAMEFIELD:                                                 \
        dst = ret.dst;                                                        \
        goto again;                                                           \
      case FD_NEXT_OTHERFIELD:                                                \
        data = ret.tag;                                                       \
        UPB_MUSTTAIL return _upb_FastDecoder_TagDispatch(UPB_PARSE_ARGS);     \
      case FD_NEXT_ATLIMIT:                                                   \
        return ptr;                                                           \
    }                                                                         \
  }                                                                           \
                                                                              \
  UPB_MUSTTAIL return upb_DecodeFast_Dispatch(UPB_PARSE_ARGS);

typedef struct {
  uint8_t valbytes;
  bool zigzag;
  void* dst;
  fastdecode_arr farr;
} fastdecode_varintdata;

UPB_FORCEINLINE
const char* fastdecode_topackedvarint(upb_EpsCopyInputStream* e,
                                      const char* ptr, void* ctx) {
  upb_Decoder* d = (upb_Decoder*)e;
  fastdecode_varintdata* data = ctx;
  void* dst = data->dst;
  uint64_t val;

  while (!_upb_Decoder_IsDone(d, &ptr)) {
    dst = fastdecode_resizearr(d, dst, &data->farr, data->valbytes);
    ptr = fastdecode_varint64(ptr, &val);
    if (ptr == NULL) return NULL;
    val = fastdecode_munge(val, data->valbytes, data->zigzag);
    memcpy(dst, &val, data->valbytes);
    dst = (char*)dst + data->valbytes;
  }

  fastdecode_commitarr(dst, &data->farr, data->valbytes);
  return ptr;
}

#define FASTDECODE_PACKEDVARINT(unpacked, card, d, ptr, msg, table, hasbits, \
                                data, type, tagsize)                         \
  int valbytes = upb_DecodeFast_ValueBytes(type);                            \
  bool zigzag = upb_DecodeFast_IsZigZag(type);                               \
  int tagbytes = upb_DecodeFast_TagSizeBytes(tagsize);                       \
  fastdecode_varintdata ctx = {valbytes, zigzag};                            \
                                                                             \
  FASTDECODE_CHECKPACKED(tagbytes, kUpb_DecodeFast_Repeated, unpacked);      \
                                                                             \
  ctx.dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, &ctx.farr,     \
                                valbytes, kUpb_DecodeFast_Repeated);         \
  if (UPB_UNLIKELY(!ctx.dst)) {                                              \
    RETURN_GENERIC("need array resize\n");                                   \
  }                                                                          \
                                                                             \
  ptr += tagbytes;                                                           \
  ptr = fastdecode_delimited(d, ptr, &fastdecode_topackedvarint, &ctx);      \
                                                                             \
  if (UPB_UNLIKELY(ptr == NULL)) {                                           \
    _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);               \
  }                                                                          \
                                                                             \
  UPB_MUSTTAIL return upb_DecodeFast_Dispatch(d, ptr, msg, table, hasbits, 0);

#define FASTDECODE_VARINT(unpacked, packed, card, ...)   \
  if (card == kUpb_DecodeFast_Packed) {                  \
    FASTDECODE_PACKEDVARINT(unpacked, card, __VA_ARGS__) \
  } else {                                               \
    FASTDECODE_UNPACKEDVARINT(packed, card, __VA_ARGS__) \
  }

/* Generate all combinations:
 * {s,o,r,p} x {b1,v4,z4,v8,z8} x {1bt,2bt} */

#define F(type, card, tagsize)                                            \
  UPB_NOINLINE UPB_PRESERVE_NONE const char* UPB_DECODEFAST_FUNCNAME(     \
      type, card, tagsize)(UPB_PARSE_PARAMS) {                            \
    FASTDECODE_VARINT(UPB_DECODEFAST_FUNCNAME(type, Repeated, tagsize),   \
                      UPB_DECODEFAST_FUNCNAME(type, Packed, tagsize),     \
                      kUpb_DecodeFast_##card, UPB_PARSE_ARGS,             \
                      kUpb_DecodeFast_##type, kUpb_DecodeFast_##tagsize); \
  }

UPB_DECODEFAST_CARDINALITIES(UPB_DECODEFAST_TAGSIZES, F, Bool)
UPB_DECODEFAST_CARDINALITIES(UPB_DECODEFAST_TAGSIZES, F, Varint32)
UPB_DECODEFAST_CARDINALITIES(UPB_DECODEFAST_TAGSIZES, F, Varint64)
UPB_DECODEFAST_CARDINALITIES(UPB_DECODEFAST_TAGSIZES, F, ZigZag32)
UPB_DECODEFAST_CARDINALITIES(UPB_DECODEFAST_TAGSIZES, F, ZigZag64)

#undef F
