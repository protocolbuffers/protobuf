// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/wire/decode_fast/cardinality.h"
#include "upb/wire/decode_fast/dispatch.h"
#include "upb/wire/decode_fast/field_parsers.h"

// Must be last.
#include "upb/port/def.inc"

#define FASTDECODE_UNPACKEDFIXED(d, ptr, msg, table, hasbits, data, tagbytes, \
                                 valbytes, card, packed)                      \
  void* dst;                                                                  \
  fastdecode_arr farr;                                                        \
                                                                              \
  FASTDECODE_CHECKPACKED(tagbytes, card, packed)                              \
                                                                              \
  dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, &farr, valbytes,    \
                            card);                                            \
  if (card == CARD_r) {                                                       \
    if (UPB_UNLIKELY(!dst)) {                                                 \
      RETURN_GENERIC("couldn't allocate array in arena\n");                   \
    }                                                                         \
  }                                                                           \
                                                                              \
  again:                                                                      \
  if (card == CARD_r) {                                                       \
    dst = fastdecode_resizearr(d, dst, &farr, valbytes);                      \
  }                                                                           \
                                                                              \
  ptr += tagbytes;                                                            \
  memcpy(dst, ptr, valbytes);                                                 \
  ptr += valbytes;                                                            \
                                                                              \
  if (card == CARD_r) {                                                       \
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
  UPB_MUSTTAIL return fastdecode_dispatch(UPB_PARSE_ARGS);

#define FASTDECODE_PACKEDFIXED(d, ptr, msg, table, hasbits, data, tagbytes, \
                               valbytes, unpacked)                          \
  FASTDECODE_CHECKPACKED(tagbytes, CARD_r, unpacked)                        \
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

#define FASTDECODE_FIXED(d, ptr, msg, table, hasbits, data, tagbytes,     \
                         valbytes, card, unpacked, packed)                \
  if (card == CARD_p) {                                                   \
    FASTDECODE_PACKEDFIXED(d, ptr, msg, table, hasbits, data, tagbytes,   \
                           valbytes, unpacked);                           \
  } else {                                                                \
    FASTDECODE_UNPACKEDFIXED(d, ptr, msg, table, hasbits, data, tagbytes, \
                             valbytes, card, packed);                     \
  }

/* Generate all combinations:
 * {s,o,r,p} x {f4,f8} x {1bt,2bt} */

#define F(card, valbytes, tagbytes)                                         \
  UPB_NOINLINE                                                              \
  const char* upb_p##card##f##valbytes##_##tagbytes##bt(UPB_PARSE_PARAMS) { \
    FASTDECODE_FIXED(d, ptr, msg, table, hasbits, data, tagbytes, valbytes, \
                     CARD_##card, upb_ppf##valbytes##_##tagbytes##bt,       \
                     upb_prf##valbytes##_##tagbytes##bt);                   \
  }

#define TYPES(card, tagbytes) \
  F(card, 4, tagbytes)        \
  F(card, 8, tagbytes)

#define TAGBYTES(card) \
  TYPES(card, 1)       \
  TYPES(card, 2)

TAGBYTES(s)
TAGBYTES(o)
TAGBYTES(r)
TAGBYTES(p)

#undef F
#undef TYPES
#undef TAGBYTES
#undef FASTDECODE_UNPACKEDFIXED
#undef FASTDECODE_PACKEDFIXED

#include "upb/port/undef.inc"
