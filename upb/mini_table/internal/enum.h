// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MINI_TABLE_INTERNAL_ENUM_H_
#define UPB_MINI_TABLE_INTERNAL_ENUM_H_

#include <stddef.h>
#include <stdint.h>

// Must be last.
#include "upb/port/def.inc"

struct upb_MiniTableEnum {
  uint32_t UPB_PRIVATE(mask_limit);   // Highest that can be tested with mask.
  uint32_t UPB_PRIVATE(value_count);  // Number of values after the bitfield.
  uint32_t UPB_PRIVATE(data)[];       // Bitmask + enumerated values follow.
};

#ifdef __cplusplus
extern "C" {
#endif

UPB_API_INLINE bool upb_MiniTableEnum_CheckValue(
    const struct upb_MiniTableEnum* e, uint32_t val) {
  // A closed-enum field whose sub-enum table has not been linked resolves to a
  // NULL table here (upb_MiniTable_GetSubEnumTable returns NULL for unlinked
  // fields).  Report every value as unrecognized so callers route it to the
  // unknown-field set instead of dereferencing NULL.
  if (UPB_UNLIKELY(e == NULL)) return false;
  if (UPB_LIKELY(val < 64)) {
    const uint64_t mask =
        e->UPB_PRIVATE(data)[0] | ((uint64_t)e->UPB_PRIVATE(data)[1] << 32);
    const uint64_t bit = 1ULL << val;
    return (mask & bit) != 0;
  }
  if (UPB_LIKELY(val < e->UPB_PRIVATE(mask_limit))) {
    const uint32_t mask = e->UPB_PRIVATE(data)[val / 32];
    const uint32_t bit = 1U << (val % 32);
    return (mask & bit) != 0;
  }

  // OPT: binary search long lists?
  const uint32_t* start =
      &e->UPB_PRIVATE(data)[e->UPB_PRIVATE(mask_limit) / 32];
  const uint32_t* limit = &e->UPB_PRIVATE(
      data)[e->UPB_PRIVATE(mask_limit) / 32 + e->UPB_PRIVATE(value_count)];
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
