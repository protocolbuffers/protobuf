// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MINI_TABLE_INTERNAL_ENUM_H_
#define UPB_MINI_TABLE_INTERNAL_ENUM_H_

#include <stdint.h>

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
