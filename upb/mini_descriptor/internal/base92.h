/*
 * Copyright (c) 2009-2022, Google LLC
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

#ifndef UPB_MINI_DESCRIPTOR_INTERNAL_BASE92_H_
#define UPB_MINI_DESCRIPTOR_INTERNAL_BASE92_H_

#include "upb/base/log2.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

UPB_INLINE char _upb_ToBase92(int8_t ch) {
  extern const char _kUpb_ToBase92[];
  UPB_ASSERT(0 <= ch && ch < 92);
  return _kUpb_ToBase92[ch];
}

UPB_INLINE char _upb_FromBase92(uint8_t ch) {
  extern const int8_t _kUpb_FromBase92[];
  if (' ' > ch || ch > '~') return -1;
  return _kUpb_FromBase92[ch - ' '];
}

UPB_INLINE const char* _upb_Base92_DecodeVarint(const char* ptr,
                                                const char* end, char first_ch,
                                                uint8_t min, uint8_t max,
                                                uint32_t* out_val) {
  uint32_t val = 0;
  uint32_t shift = 0;
  const int bits_per_char =
      upb_Log2Ceiling(_upb_FromBase92(max) - _upb_FromBase92(min));
  char ch = first_ch;
  while (1) {
    uint32_t bits = _upb_FromBase92(ch) - _upb_FromBase92(min);
    val |= bits << shift;
    if (ptr == end || *ptr < min || max < *ptr) {
      *out_val = val;
      UPB_ASSUME(ptr != NULL);
      return ptr;
    }
    ch = *ptr++;
    shift += bits_per_char;
    if (shift >= 32) return NULL;
  }
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif  // UPB_MINI_DESCRIPTOR_INTERNAL_BASE92_H_
