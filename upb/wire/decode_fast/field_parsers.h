// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_WIRE_INTERNAL_DECODE_FAST_H_
#define UPB_WIRE_INTERNAL_DECODE_FAST_H_

#include <stddef.h>
#include <stdint.h>

#include "upb/message/message.h"
#include "upb/wire/decode_fast/combinations.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

struct upb_Decoder;

// Declare all fast decoder functions. These are referenced by name from either
// the generated code or the array in decode_fast_function_array.c.

#define PARSE_PARAMS                                                        \
  struct upb_Decoder *d, const char *ptr, upb_Message *msg, intptr_t table, \
      uint64_t hasbits, uint64_t data

#define F(type, card, size)                                         \
  UPB_PRESERVE_NONE const char *UPB_DECODEFAST_FUNCNAME(type, card, \
                                                        size)(PARSE_PARAMS);

UPB_DECODEFAST_FUNCTIONS(F);

// This just uses the regular (non-fast) parser to parse a single field.
UPB_PRESERVE_NONE
const char *_upb_FastDecoder_DecodeGeneric(PARSE_PARAMS);

#undef F
#undef PARSE_PARAMS

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_WIRE_INTERNAL_DECODE_FAST_H_ */
