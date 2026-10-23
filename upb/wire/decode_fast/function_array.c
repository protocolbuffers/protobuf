// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/wire/decode_fast/function_array.h"

#include <stddef.h>
#include <stdint.h>

#include "upb/mini_table/internal/message.h"
#include "upb/wire/decode_fast/combinations.h"
#include "upb/wire/decode_fast/field_parsers.h"

// Must be last.
#include "upb/port/def.inc"

#define ADDR_OF_FUNC(type, card, size)                                     \
  UPB_DECODEFAST_ISENABLED(kUpb_DecodeFast_##type, kUpb_DecodeFast_##card, \
                           kUpb_DecodeFast_##size)                         \
  ? &UPB_DECODEFAST_FUNCNAME(type, card, size) : _upb_FastDecoder_DecodeGeneric,

_upb_FieldParser* const UPB_PRIVATE(_upb_DecodeFast_FunctionArray)[] = {
    UPB_DECODEFAST_FUNCTIONS(ADDR_OF_FUNC)};

#undef ADDR_OF_FUNC

#include "upb/port/undef.inc"
