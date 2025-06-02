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
#include "upb/message/message.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode.h"
#include "upb/wire/decode_fast/cardinality.h"
#include "upb/wire/decode_fast/combinations.h"
#include "upb/wire/decode_fast/dispatch.h"
#include "upb/wire/decode_fast/field_parsers.h"
#include "upb/wire/eps_copy_input_stream.h"
#include "upb/wire/internal/decoder.h"

// Must be last.
#include "upb/port/def.inc"

// We generate two functions for each combination:
// 1. The main function, which attempts to alias the string from the input
//    stream, but will fall back to (2) if aliasing is not possible.
// 2. A copy function, which always copies the string from the input stream.

#define STRING_FUNCNAME(type, card, tagsize, copy) \
  UPB_DECODEFAST_FUNCNAME(copy##type, card, tagsize)

// Forward-declare the copy function type.
#define F(type, card, tagsize)                                       \
  UPB_NOINLINE UPB_PRESERVE_NONE static const char* STRING_FUNCNAME( \
      type, card, tagsize, Copy)(UPB_PARSE_PARAMS);

UPB_DECODEFAST_CARDINALITIES(UPB_DECODEFAST_TAGSIZES, F, String)
UPB_DECODEFAST_CARDINALITIES(UPB_DECODEFAST_TAGSIZES, F, Bytes)

#undef F

UPB_NOINLINE UPB_PRESERVE_NONE static const char* fastdecode_verifyutf8(
    upb_Decoder* d, const char* ptr, upb_Message* msg, intptr_t table,
    uint64_t hasbits, uint64_t data) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  upb_StringView* dst = (upb_StringView*)data;
  if (!_upb_Decoder_VerifyUtf8Inline(dst->data, dst->size)) {
    _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_BadUtf8);
  }
  UPB_MUSTTAIL return upb_DecodeFast_Dispatch(UPB_PARSE_ARGS);
}

#define FASTDECODE_LONGSTRING(d, ptr, msg, table, hasbits, dst, validate_utf8) \
  int size = (uint8_t)ptr[0]; /* Could plumb through hasbits. */               \
  ptr++;                                                                       \
  if (size & 0x80) {                                                           \
    ptr = fastdecode_longsize(ptr, &size);                                     \
  }                                                                            \
                                                                               \
  if (UPB_UNLIKELY(!upb_EpsCopyInputStream_CheckSize(&d->input, ptr, size))) { \
    dst->size = 0;                                                             \
    _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);                 \
  }                                                                            \
                                                                               \
  const char* s_ptr = ptr;                                                     \
  ptr = upb_EpsCopyInputStream_ReadString(&d->input, &s_ptr, size, &d->arena); \
  if (!ptr) _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);       \
  dst->data = s_ptr;                                                           \
  dst->size = size;                                                            \
                                                                               \
  if (validate_utf8) {                                                         \
    data = (uint64_t)dst;                                                      \
    UPB_MUSTTAIL return fastdecode_verifyutf8(UPB_PARSE_ARGS);                 \
  } else {                                                                     \
    UPB_MUSTTAIL return upb_DecodeFast_Dispatch(UPB_PARSE_ARGS);               \
  }

UPB_NOINLINE UPB_PRESERVE_NONE static const char* fastdecode_longstring_utf8(
    struct upb_Decoder* d, const char* ptr, upb_Message* msg, intptr_t table,
    uint64_t hasbits, uint64_t data) {
  upb_StringView* dst = (upb_StringView*)data;
  FASTDECODE_LONGSTRING(d, ptr, msg, table, hasbits, dst, true);
}

UPB_NOINLINE UPB_PRESERVE_NONE static const char* fastdecode_longstring_noutf8(
    struct upb_Decoder* d, const char* ptr, upb_Message* msg, intptr_t table,
    uint64_t hasbits, uint64_t data) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  upb_StringView* dst = (upb_StringView*)data;
  FASTDECODE_LONGSTRING(d, ptr, msg, table, hasbits, dst, false);
}

UPB_FORCEINLINE
void fastdecode_docopy(upb_Decoder* d, const char* ptr, uint32_t size, int copy,
                       char* data, size_t data_offset, upb_StringView* dst) {
  d->arena.UPB_PRIVATE(ptr) += copy;
  dst->data = data + data_offset;
  // TODO: fix for Xsan
  memcpy(data, ptr, copy);
}

#define FASTDECODE_COPYSTRING(d, ptr, msg, table, hasbits, data, type, card,   \
                              tagsize)                                         \
  upb_StringView* dst;                                                         \
  fastdecode_arr farr;                                                         \
  int64_t size;                                                                \
  size_t arena_has;                                                            \
  size_t common_has;                                                           \
  char* buf;                                                                   \
  bool validate_utf8 = type == kUpb_DecodeFast_String;                         \
  int tagbytes = upb_DecodeFast_TagSizeBytes(tagsize);                         \
                                                                               \
  UPB_ASSERT(!upb_EpsCopyInputStream_AliasingAvailable(&d->input, ptr, 0));    \
  UPB_ASSERT(fastdecode_checktag(data, tagbytes));                             \
                                                                               \
  dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, &farr,               \
                            sizeof(upb_StringView), card);                     \
                                                                               \
  again:                                                                       \
  if (card == kUpb_DecodeFast_Repeated) {                                      \
    dst = fastdecode_resizearr(d, dst, &farr, sizeof(upb_StringView));         \
  }                                                                            \
                                                                               \
  size = (uint8_t)ptr[tagbytes];                                               \
  ptr += tagbytes + 1;                                                         \
  dst->size = size;                                                            \
                                                                               \
  buf = d->arena.UPB_PRIVATE(ptr);                                             \
  arena_has = UPB_PRIVATE(_upb_ArenaHas)(&d->arena);                           \
  common_has = UPB_MIN(arena_has,                                              \
                       upb_EpsCopyInputStream_BytesAvailable(&d->input, ptr)); \
                                                                               \
  if (UPB_LIKELY(size <= 15 - tagbytes)) {                                     \
    if (arena_has < 16) goto longstr;                                          \
    fastdecode_docopy(d, ptr - tagbytes - 1, size, 16, buf, tagbytes + 1,      \
                      dst);                                                    \
  } else if (UPB_LIKELY(size <= 32)) {                                         \
    if (UPB_UNLIKELY(common_has < 32)) goto longstr;                           \
    fastdecode_docopy(d, ptr, size, 32, buf, 0, dst);                          \
  } else if (UPB_LIKELY(size <= 64)) {                                         \
    if (UPB_UNLIKELY(common_has < 64)) goto longstr;                           \
    fastdecode_docopy(d, ptr, size, 64, buf, 0, dst);                          \
  } else if (UPB_LIKELY(size < 128)) {                                         \
    if (UPB_UNLIKELY(common_has < 128)) goto longstr;                          \
    fastdecode_docopy(d, ptr, size, 128, buf, 0, dst);                         \
  } else {                                                                     \
    goto longstr;                                                              \
  }                                                                            \
                                                                               \
  ptr += size;                                                                 \
                                                                               \
  if (card == kUpb_DecodeFast_Repeated) {                                      \
    if (validate_utf8 &&                                                       \
        !_upb_Decoder_VerifyUtf8Inline(dst->data, dst->size)) {                \
      _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_BadUtf8);                 \
    }                                                                          \
    fastdecode_nextret ret = fastdecode_nextrepeated(                          \
        d, dst, &ptr, &farr, data, tagbytes, sizeof(upb_StringView));          \
    switch (ret.next) {                                                        \
      case FD_NEXT_SAMEFIELD:                                                  \
        dst = ret.dst;                                                         \
        goto again;                                                            \
      case FD_NEXT_OTHERFIELD:                                                 \
        data = ret.tag;                                                        \
        UPB_MUSTTAIL return _upb_FastDecoder_TagDispatch(UPB_PARSE_ARGS);      \
      case FD_NEXT_ATLIMIT:                                                    \
        return ptr;                                                            \
    }                                                                          \
  }                                                                            \
                                                                               \
  if (card != kUpb_DecodeFast_Repeated && validate_utf8) {                     \
    data = (uint64_t)dst;                                                      \
    UPB_MUSTTAIL return fastdecode_verifyutf8(UPB_PARSE_ARGS);                 \
  }                                                                            \
                                                                               \
  UPB_MUSTTAIL return upb_DecodeFast_Dispatch(UPB_PARSE_ARGS);                 \
                                                                               \
  longstr:                                                                     \
  if (card == kUpb_DecodeFast_Repeated) {                                      \
    fastdecode_commitarr(dst + 1, &farr, sizeof(upb_StringView));              \
  }                                                                            \
  ptr--;                                                                       \
  if (validate_utf8) {                                                         \
    UPB_MUSTTAIL return fastdecode_longstring_utf8(d, ptr, msg, table,         \
                                                   hasbits, (uint64_t)dst);    \
  } else {                                                                     \
    UPB_MUSTTAIL return fastdecode_longstring_noutf8(d, ptr, msg, table,       \
                                                     hasbits, (uint64_t)dst);  \
  }

#define FASTDECODE_STRING(d, ptr, msg, table, hasbits, data, type, card,      \
                          tagsize, copyfunc)                                  \
  upb_StringView* dst;                                                        \
  fastdecode_arr farr;                                                        \
  int64_t size;                                                               \
  bool validate_utf8 = type == kUpb_DecodeFast_String;                        \
  int tagbytes = upb_DecodeFast_TagSizeBytes(tagsize);                        \
                                                                              \
  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) {                   \
    RETURN_GENERIC("string field tag mismatch\n");                            \
  }                                                                           \
                                                                              \
  if (UPB_UNLIKELY(                                                           \
          !upb_EpsCopyInputStream_AliasingAvailable(&d->input, ptr, 0))) {    \
    UPB_MUSTTAIL return copyfunc(UPB_PARSE_ARGS);                             \
  }                                                                           \
                                                                              \
  dst = fastdecode_getfield(d, ptr, msg, &data, &hasbits, &farr,              \
                            sizeof(upb_StringView), card);                    \
                                                                              \
  again:                                                                      \
  if (card == kUpb_DecodeFast_Repeated) {                                     \
    dst = fastdecode_resizearr(d, dst, &farr, sizeof(upb_StringView));        \
  }                                                                           \
                                                                              \
  size = (int8_t)ptr[tagbytes];                                               \
  ptr += tagbytes + 1;                                                        \
                                                                              \
  if (UPB_UNLIKELY(                                                           \
          !upb_EpsCopyInputStream_AliasingAvailable(&d->input, ptr, size))) { \
    ptr--;                                                                    \
    if (validate_utf8) {                                                      \
      return fastdecode_longstring_utf8(d, ptr, msg, table, hasbits,          \
                                        (uint64_t)dst);                       \
    } else {                                                                  \
      return fastdecode_longstring_noutf8(d, ptr, msg, table, hasbits,        \
                                          (uint64_t)dst);                     \
    }                                                                         \
  }                                                                           \
                                                                              \
  dst->data = ptr;                                                            \
  dst->size = size;                                                           \
  ptr = upb_EpsCopyInputStream_ReadStringAliased(&d->input, &dst->data,       \
                                                 dst->size);                  \
                                                                              \
  if (card == kUpb_DecodeFast_Repeated) {                                     \
    if (validate_utf8 &&                                                      \
        !_upb_Decoder_VerifyUtf8Inline(dst->data, dst->size)) {               \
      _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_BadUtf8);                \
    }                                                                         \
    fastdecode_nextret ret = fastdecode_nextrepeated(                         \
        d, dst, &ptr, &farr, data, tagbytes, sizeof(upb_StringView));         \
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
  if (card != kUpb_DecodeFast_Repeated && validate_utf8) {                    \
    data = (uint64_t)dst;                                                     \
    UPB_MUSTTAIL return fastdecode_verifyutf8(UPB_PARSE_ARGS);                \
  }                                                                           \
                                                                              \
  UPB_MUSTTAIL return upb_DecodeFast_Dispatch(UPB_PARSE_ARGS);

/* Generate all combinations:
 * {p,c} x {s,o,r} x {s, b} x {1bt,2bt} */

#define F(type, card, tagsize)                                                 \
  UPB_NOINLINE static const char* UPB_PRESERVE_NONE STRING_FUNCNAME(           \
      type, card, tagsize, Copy)(UPB_PARSE_PARAMS) {                           \
    FASTDECODE_COPYSTRING(d, ptr, msg, table, hasbits, data,                   \
                          kUpb_DecodeFast_##type, kUpb_DecodeFast_##card,      \
                          kUpb_DecodeFast_##tagsize)                           \
  }                                                                            \
  const char* UPB_PRESERVE_NONE STRING_FUNCNAME(type, card,                    \
                                                tagsize, )(UPB_PARSE_PARAMS) { \
    FASTDECODE_STRING(d, ptr, msg, table, hasbits, data,                       \
                      kUpb_DecodeFast_##type, kUpb_DecodeFast_##card,          \
                      kUpb_DecodeFast_##tagsize,                               \
                      STRING_FUNCNAME(type, card, tagsize, Copy))              \
  }

UPB_DECODEFAST_CARDINALITIES(UPB_DECODEFAST_TAGSIZES, F, String)
UPB_DECODEFAST_CARDINALITIES(UPB_DECODEFAST_TAGSIZES, F, Bytes)

#undef F

#undef F
#undef FASTDECODE_LONGSTRING
#undef FASTDECODE_COPYSTRING
#undef FASTDECODE_STRING
