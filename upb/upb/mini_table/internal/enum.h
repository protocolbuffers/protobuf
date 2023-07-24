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

#ifndef UPB_MINI_TABLE_INTERNAL_ENUM_H_
#define UPB_MINI_TABLE_INTERNAL_ENUM_H_

// Must be last.
#include "upb/port/def.inc"

struct upb_MiniTableEnum {
  uint32_t mask_limit;   // Limit enum value that can be tested with mask.
  uint32_t value_count;  // Number of values after the bitfield.
  uint32_t data[];       // Bitmask + enumerated values follow.
};

typedef enum {
  _kUpb_FastEnumCheck_ValueIsInEnum = 0,
  _kUpb_FastEnumCheck_ValueIsNotInEnum = 1,
  _kUpb_FastEnumCheck_CannotCheckFast = 2,
} _kUpb_FastEnumCheck_Status;

#ifdef __cplusplus
extern "C" {
#endif

UPB_INLINE _kUpb_FastEnumCheck_Status _upb_MiniTable_CheckEnumValueFast(
    const struct upb_MiniTableEnum* e, uint32_t val) {
  if (UPB_UNLIKELY(val >= 64)) return _kUpb_FastEnumCheck_CannotCheckFast;
  uint64_t mask = e->data[0] | ((uint64_t)e->data[1] << 32);
  return (mask & (1ULL << val)) ? _kUpb_FastEnumCheck_ValueIsInEnum
                                : _kUpb_FastEnumCheck_ValueIsNotInEnum;
}

UPB_INLINE bool _upb_MiniTable_CheckEnumValueSlow(
    const struct upb_MiniTableEnum* e, uint32_t val) {
  if (val < e->mask_limit) return e->data[val / 32] & (1ULL << (val % 32));
  // OPT: binary search long lists?
  const uint32_t* start = &e->data[e->mask_limit / 32];
  const uint32_t* limit = &e->data[(e->mask_limit / 32) + e->value_count];
  for (const uint32_t* p = start; p < limit; p++) {
    if (*p == val) return true;
  }
  return false;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MINI_TABLE_INTERNAL_ENUM_H_ */
