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

#define FASTDECODE_UNPACKEDFIXED(packed, card, d, ptr, msg, table, hasbits, \
                                 data, type, tagsize)                       \
  void* dst;                                                                \
  fastdecode_arr farr;                                                      \
  int valbytes = upb_DecodeFast_ValueBytes(type);                           \
  int tagbytes = upb_DecodeFast_TagSizeBytes(tagsize);                      \
                                                                            \
  FASTDECODE_CHECKPACKED(tagbytes, card, packed)                            \
                                                                            \
  dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, &farr, valbytes,  \
                            card);                                          \
  if (card == kUpb_DecodeFast_Repeated) {                                   \
    if (UPB_UNLIKELY(!dst)) {                                               \
      RETURN_GENERIC("couldn't allocate array in arena\n");                 \
    }                                                                       \
  }                                                                         \
                                                                            \
  again:                                                                    \
  if (card == kUpb_DecodeFast_Repeated) {                                   \
    dst = fastdecode_resizearr(d, dst, &farr, valbytes);                    \
  }                                                                         \
                                                                            \
  ptr += tagbytes;                                                          \
  memcpy(dst, ptr, valbytes);                                               \
  ptr += valbytes;                                                          \
                                                                            \
  if (card == kUpb_DecodeFast_Repeated) {                                   \
    fastdecode_nextret ret = fastdecode_nextrepeated(                       \
        d, dst, &ptr, &farr, data, tagbytes, valbytes);                     \
    switch (ret.next) {                                                     \
      case FD_NEXT_SAMEFIELD:                                               \
        dst = ret.dst;                                                      \
        goto again;                                                         \
      case FD_NEXT_OTHERFIELD:                                              \
        data = ret.tag;                                                     \
        UPB_MUSTTAIL return _upb_FastDecoder_TagDispatch(UPB_PARSE_ARGS);   \
      case FD_NEXT_ATLIMIT:                                                 \
        return ptr;                                                         \
    }                                                                       \
  }                                                                         \
                                                                            \
  UPB_MUSTTAIL return fastdecode_dispatch(UPB_PARSE_ARGS);

#define FASTDECODE_PACKEDFIXED(unpacked, card, d, ptr, msg, table, hasbits, \
                               data, type, tagsize)                         \
  int valbytes = upb_DecodeFast_ValueBytes(type);                           \
  int tagbytes = upb_DecodeFast_TagSizeBytes(tagsize);                      \
                                                                            \
  FASTDECODE_CHECKPACKED(tagbytes, kUpb_DecodeFast_Repeated, unpacked)      \
                                                                            \
  ptr += tagbytes;                                                          \
  int size = (uint8_t)ptr[0];                                               \
  ptr++;                                                                    \
  if (size & 0x80) {                                                        \
    ptr = fastdecode_longsize(ptr, &size);                                  \
  }                                                                         \
                                                                            \
  if (UPB_UNLIKELY(!upb_EpsCopyInputStream_CheckDataSizeAvailable(          \
                       &d->input, ptr, size) ||                             \
                   (size % valbytes) != 0)) {                               \
    _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);              \
  }                                                                         \
                                                                            \
  upb_Array** arr_p = fastdecode_fieldmem(msg, data);                       \
  upb_Array* arr = *arr_p;                                                  \
  uint8_t elem_size_lg2 = __builtin_ctz(valbytes);                          \
  int elems = size / valbytes;                                              \
                                                                            \
  if (UPB_LIKELY(!arr)) {                                                   \
    *arr_p = arr =                                                          \
        UPB_PRIVATE(_upb_Array_New)(&d->arena, elems, elem_size_lg2);       \
    if (!arr) {                                                             \
      _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);            \
    }                                                                       \
  } else {                                                                  \
    UPB_PRIVATE(_upb_Array_ResizeUninitialized)(arr, elems, &d->arena);     \
  }                                                                         \
                                                                            \
  char* dst = upb_Array_MutableDataPtr(arr);                                \
  memcpy(dst, ptr, size);                                                   \
  arr->UPB_PRIVATE(size) = elems;                                           \
                                                                            \
  ptr += size;                                                              \
  UPB_MUSTTAIL return fastdecode_dispatch(UPB_PARSE_ARGS);

#define FASTDECODE_FIXED(unpacked, packed, card, ...)   \
  if (card == kUpb_DecodeFast_Packed) {                 \
    FASTDECODE_PACKEDFIXED(unpacked, card, __VA_ARGS__) \
  } else {                                              \
    FASTDECODE_UNPACKEDFIXED(packed, card, __VA_ARGS__) \
  }

/* Generate all combinations:
 * {s,o,r,p} x {f4,f8} x {1bt,2bt} */

#define F(type, card, tagbytes)                                           \
  UPB_NOINLINE                                                            \
  const char* UPB_DECODEFAST_FUNCNAME(type, card,                         \
                                      tagbytes)(UPB_PARSE_PARAMS) {       \
    FASTDECODE_FIXED(UPB_DECODEFAST_FUNCNAME(type, Repeated, tagbytes),   \
                     UPB_DECODEFAST_FUNCNAME(type, Packed, tagbytes),     \
                     kUpb_DecodeFast_##card, UPB_PARSE_ARGS,              \
                     kUpb_DecodeFast_##type, kUpb_DecodeFast_##tagbytes); \
  }

UPB_DECODEFAST_CARDINALITIES(UPB_DECODEFAST_TAGSIZES, F, Fixed32)

#undef F
#undef FASTDECODE_FIXED
#undef FASTDECODE_PACKEDFIXED
#undef FASTDECODE_UNPACKEDFIXED

#include "upb/port/undef.inc"
