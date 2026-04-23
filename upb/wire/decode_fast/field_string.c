// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <stddef.h>
#include <stdint.h>

#include "upb/base/string_view.h"
#include "upb/wire/decode.h"
#include "upb/wire/decode_fast/cardinality.h"
#include "upb/wire/decode_fast/combinations.h"
#include "upb/wire/decode_fast/dispatch.h"
#include "upb/wire/decode_fast/field_parsers.h"
#include "upb/wire/eps_copy_input_stream.h"
#include "upb/wire/internal/decoder.h"
#include "utf8_range.h"

// Must be last.
#include "upb/port/def.inc"

UPB_FORCEINLINE
bool upb_DecodeFast_SingleStringAlias(upb_Decoder* d, const char** ptr,
                                      void* dst, upb_DecodeFast_Type type,
                                      upb_DecodeFastNext* next, void* ctx) {
  UPB_UNUSED(ctx);
  bool validate_utf8 = type == kUpb_DecodeFast_String;
  upb_StringView* sv = dst;
  int size;

  if (!upb_DecodeFast_DecodeSize(d, ptr, &size, next)) return false;

  const char* p = *ptr;
  if (!upb_EpsCopyInputStream_ReadStringAlwaysAlias(&d->input, p, size, sv)) {
    return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
  }

  if (validate_utf8 && !utf8_range_IsValid(sv->data, sv->size)) {
    return UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_BadUtf8, next);
  }

  *ptr = p + size;
  return true;
}

UPB_FORCEINLINE
bool upb_DecodeFast_SingleStringCopy(upb_Decoder* d, const char** ptr,
                                     void* dst, upb_DecodeFast_Type type,
                                     upb_DecodeFastNext* next, void* ctx) {
  bool validate_utf8 = type == kUpb_DecodeFast_String;
  upb_StringView* sv = dst;
  int size;

  if (!upb_DecodeFast_DecodeSize(d, ptr, &size, next)) return false;

  if (!_upb_Decoder_ReadString(d, ptr, size, sv, validate_utf8)) {
    sv->size = 0;
    return UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_OutOfMemory, next);
  }

  return true;
}

#define F_COPY(type, card, tagsize)                                          \
  UPB_NOINLINE UPB_PRESERVE_NONE static const char* UPB_DECODEFAST_FUNCNAME( \
      type, card, tagsize##_Copy)(UPB_PARSE_PARAMS) {                        \
    upb_DecodeFastNext next = kUpb_DecodeFastNext_Dispatch;                  \
    upb_DecodeFast_Unpacked(d, &ptr, msg, &data, &hasbits, &next,            \
                            kUpb_DecodeFast_##type, kUpb_DecodeFast_##card,  \
                            kUpb_DecodeFast_##tagsize,                       \
                            &upb_DecodeFast_SingleStringCopy, NULL);         \
    UPB_DECODEFAST_NEXT(next);                                               \
  }

#define F(type, card, tagsize)                                              \
  F_COPY(type, card, tagsize)                                               \
  const char* UPB_PRESERVE_NONE UPB_DECODEFAST_FUNCNAME(                    \
      type, card, tagsize)(UPB_PARSE_PARAMS) {                              \
    if (UPB_UNLIKELY((d->options & kUpb_DecodeOption_AliasString) == 0)) {  \
      UPB_MUSTTAIL return UPB_DECODEFAST_FUNCNAME(                          \
          type, card, tagsize##_Copy)(UPB_PARSE_ARGS);                      \
    }                                                                       \
    upb_DecodeFastNext next = kUpb_DecodeFastNext_Dispatch;                 \
    upb_DecodeFast_Unpacked(d, &ptr, msg, &data, &hasbits, &next,           \
                            kUpb_DecodeFast_##type, kUpb_DecodeFast_##card, \
                            kUpb_DecodeFast_##tagsize,                      \
                            &upb_DecodeFast_SingleStringAlias, NULL);       \
    UPB_DECODEFAST_NEXT(next);                                              \
  }

UPB_DECODEFAST_CARDINALITIES(UPB_DECODEFAST_TAGSIZES, F, String)
UPB_DECODEFAST_CARDINALITIES(UPB_DECODEFAST_TAGSIZES, F, Bytes)

#undef F
#undef F_COPY

#undef F
#undef FASTDECODE_LONGSTRING
#undef FASTDECODE_COPYSTRING
#undef FASTDECODE_STRING
