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

#include "upb/lex/atoi.h"

// Must be last.
#include "upb/port/def.inc"

const char* upb_BufToUint64(const char* ptr, const char* end, uint64_t* val) {
  uint64_t u64 = 0;
  while (ptr < end) {
    unsigned ch = *ptr - '0';
    if (ch >= 10) break;
    if (u64 > UINT64_MAX / 10 || u64 * 10 > UINT64_MAX - ch) {
      return NULL;  // integer overflow
    }
    u64 *= 10;
    u64 += ch;
    ptr++;
  }

  *val = u64;
  return ptr;
}

const char* upb_BufToInt64(const char* ptr, const char* end, int64_t* val,
                           bool* is_neg) {
  bool neg = false;
  uint64_t u64;

  if (ptr != end && *ptr == '-') {
    ptr++;
    neg = true;
  }

  ptr = upb_BufToUint64(ptr, end, &u64);
  if (!ptr || u64 > (uint64_t)INT64_MAX + neg) {
    return NULL;  // integer overflow
  }

  *val = neg ? -u64 : u64;
  if (is_neg) *is_neg = neg;
  return ptr;
}
