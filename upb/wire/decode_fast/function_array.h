// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_WIRE_INTERNAL_DECODE_FAST_FUNCTION_ARRAY_H_
#define UPB_WIRE_INTERNAL_DECODE_FAST_FUNCTION_ARRAY_H_

#include <stdint.h>

#include "upb/mini_table/internal/message.h"
#include "upb/wire/decode_fast/combinations.h"
#include "upb/wire/decode_fast/field_parsers.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

extern _upb_FieldParser* const UPB_PRIVATE(_upb_DecodeFast_FunctionArray)[];

UPB_FORCEINLINE _upb_FieldParser* upb_DecodeFast_GetFunctionPointer(
    uint32_t function_idx) {
  switch (function_idx) {
    case UINT32_MAX:
      return &_upb_FastDecoder_DecodeGeneric;
    case kUpb_DecodeFast_Unknown:
      return &_upb_FastDecoder_DecodeUnknown;
    case kUpb_DecodeFast_ExtensionOrUnknown:
      return &_upb_FastDecoder_DecodeExtensionOrUnknown;
    case kUpb_DecodeFast_IntMap_Tag1Byte:
      return &upb_DecodeFast_IntMap_Tag1Byte;
    case kUpb_DecodeFast_IntMap_Tag2Byte:
      return &upb_DecodeFast_IntMap_Tag2Byte;
    case kUpb_DecodeFast_StrMap_Tag1Byte:
      return &upb_DecodeFast_StrMap_Tag1Byte;
    case kUpb_DecodeFast_StrMap_Tag2Byte:
      return &upb_DecodeFast_StrMap_Tag2Byte;
    default: {
      UPB_ASSERT(function_idx < kUpb_DecodeFast_CardinalityCount *
                                    kUpb_DecodeFast_TypeCount *
                                    kUpb_DecodeFast_TagSizeCount);
      _upb_FieldParser* ret =
          UPB_PRIVATE(_upb_DecodeFast_FunctionArray)[function_idx];
      UPB_ASSUME(ret != NULL);
      return ret;
    }
  }
}

#ifdef __cplusplus
}  // extern "C"
#endif

#include "upb/port/undef.inc"

#endif  // UPB_WIRE_INTERNAL_DECODE_FAST_FUNCTION_ARRAY_H_
