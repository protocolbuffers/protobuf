/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// These are the specialized field parser functions for the fast parser.
// Generated tables will refer to these by name.
//
// The function names are encoded with names like:
//
//   //  123 4
//   upb_pss_1bt();   // Parse singular string, 1 byte tag.
//
// In position 1:
//   - 'p' for parse, most function use this
//   - 'c' for copy, for when we are copying strings instead of aliasing
//
// In position 2 (cardinality):
//   - 's' for singular, with or without hasbit
//   - 'o' for oneof
//   - 'r' for non-packed repeated
//   - 'p' for packed repeated
//
// In position 3 (type):
//   - 'b1' for bool
//   - 'v4' for 4-byte varint
//   - 'v8' for 8-byte varint
//   - 'z4' for zig-zag-encoded 4-byte varint
//   - 'z8' for zig-zag-encoded 8-byte varint
//   - 'f4' for 4-byte fixed
//   - 'f8' for 8-byte fixed
//   - 'm' for sub-message
//   - 's' for string (validate UTF-8)
//   - 'b' for bytes
//
// In position 4 (tag length):
//   - '1' for one-byte tags (field numbers 1-15)
//   - '2' for two-byte tags (field numbers 16-2048)

#ifndef UPB_WIRE_DECODE_FAST_H_
#define UPB_WIRE_DECODE_FAST_H_

#include "upb/message/message.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

struct upb_Decoder;

// The fallback, generic parsing function that can handle any field type.
// This just uses the regular (non-fast) parser to parse a single field.
const char* _upb_FastDecoder_DecodeGeneric(struct upb_Decoder* d,
                                           const char* ptr, upb_Message* msg,
                                           intptr_t table, uint64_t hasbits,
                                           uint64_t data);

#define UPB_PARSE_PARAMS                                                    \
  struct upb_Decoder *d, const char *ptr, upb_Message *msg, intptr_t table, \
      uint64_t hasbits, uint64_t data

/* primitive fields ***********************************************************/

#define F(card, type, valbytes, tagbytes) \
  const char* upb_p##card##type##valbytes##_##tagbytes##bt(UPB_PARSE_PARAMS);

#define TYPES(card, tagbytes) \
  F(card, b, 1, tagbytes)     \
  F(card, v, 4, tagbytes)     \
  F(card, v, 8, tagbytes)     \
  F(card, z, 4, tagbytes)     \
  F(card, z, 8, tagbytes)     \
  F(card, f, 4, tagbytes)     \
  F(card, f, 8, tagbytes)

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

/* string fields **************************************************************/

#define F(card, tagbytes, type)                                     \
  const char* upb_p##card##type##_##tagbytes##bt(UPB_PARSE_PARAMS); \
  const char* upb_c##card##type##_##tagbytes##bt(UPB_PARSE_PARAMS);

#define UTF8(card, tagbytes) \
  F(card, tagbytes, s)       \
  F(card, tagbytes, b)

#define TAGBYTES(card) \
  UTF8(card, 1)        \
  UTF8(card, 2)

TAGBYTES(s)
TAGBYTES(o)
TAGBYTES(r)

#undef F
#undef TAGBYTES

/* sub-message fields *********************************************************/

#define F(card, tagbytes, size_ceil, ceil_arg) \
  const char* upb_p##card##m_##tagbytes##bt_max##size_ceil##b(UPB_PARSE_PARAMS);

#define SIZES(card, tagbytes) \
  F(card, tagbytes, 64, 64)   \
  F(card, tagbytes, 128, 128) \
  F(card, tagbytes, 192, 192) \
  F(card, tagbytes, 256, 256) \
  F(card, tagbytes, max, -1)

#define TAGBYTES(card) \
  SIZES(card, 1)       \
  SIZES(card, 2)

TAGBYTES(s)
TAGBYTES(o)
TAGBYTES(r)

#undef TAGBYTES
#undef SIZES
#undef F

#undef UPB_PARSE_PARAMS

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_WIRE_DECODE_FAST_H_ */
