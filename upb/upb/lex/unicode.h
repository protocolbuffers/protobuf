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

#ifndef UPB_LEX_UNICODE_H_
#define UPB_LEX_UNICODE_H_

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

// Returns true iff a codepoint is the value for a high surrogate.
UPB_INLINE bool upb_Unicode_IsHigh(uint32_t cp) {
  return (cp >= 0xd800 && cp <= 0xdbff);
}

// Returns true iff a codepoint is the value for a low surrogate.
UPB_INLINE bool upb_Unicode_IsLow(uint32_t cp) {
  return (cp >= 0xdc00 && cp <= 0xdfff);
}

// Returns the high 16-bit surrogate value for a supplementary codepoint.
// Does not sanity-check the input.
UPB_INLINE uint16_t upb_Unicode_ToHigh(uint32_t cp) {
  return (cp >> 10) + 0xd7c0;
}

// Returns the low 16-bit surrogate value for a supplementary codepoint.
// Does not sanity-check the input.
UPB_INLINE uint16_t upb_Unicode_ToLow(uint32_t cp) {
  return (cp & 0x3ff) | 0xdc00;
}

// Returns the 32-bit value corresponding to a pair of 16-bit surrogates.
// Does not sanity-check the input.
UPB_INLINE uint32_t upb_Unicode_FromPair(uint32_t high, uint32_t low) {
  return ((high & 0x3ff) << 10) + (low & 0x3ff) + 0x10000;
}

// Outputs a codepoint as UTF8.
// Returns the number of bytes written (1-4 on success, 0 on error).
// Does not sanity-check the input. Specifically does not check for surrogates.
int upb_Unicode_ToUTF8(uint32_t cp, char* out);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_LEX_UNICODE_H_ */
