// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/mini_table/message.h"

#include <stddef.h>
#include <stdint.h>

#include "upb/mini_table/field.h"

// Must be last.
#include "upb/port/def.inc"

const upb_MiniTableField* upb_MiniTable_FindFieldByNumber(
    const upb_MiniTable* m, uint32_t number) {
  const size_t i = ((size_t)number) - 1;  // 0 wraps to SIZE_MAX

  // Ideal case: index into dense fields
  if (i < m->UPB_PRIVATE(dense_below)) {
    UPB_ASSERT(m->UPB_PRIVATE(fields)[i].UPB_PRIVATE(number) == number);
    return &m->UPB_PRIVATE(fields)[i];
  }

  // Slow case: binary search
  uint32_t lo = m->UPB_PRIVATE(dense_below);
  int32_t hi = m->UPB_PRIVATE(field_count) - 1;
  const upb_MiniTableField* base = m->UPB_PRIVATE(fields);
  while (hi >= (int32_t)lo) {
    uint32_t mid = (hi + lo) / 2;
    uint32_t num = base[mid].UPB_ONLYBITS(number);
    // These comparison operations allow, on ARM machines, to fuse all these
    // branches into one comparison followed by two CSELs to set the lo/hi
    // values, followed by a BNE to continue or terminate the loop. Since binary
    // search branches are generally unpredictable (50/50 in each direction),
    // this is a good deal. We use signed for the high, as this decrement may
    // underflow if mid is 0.
    int32_t hi_mid = mid - 1;
    uint32_t lo_mid = mid + 1;
    if (num == number) {
      return &base[mid];
    }
    if (UPB_UNPREDICTABLE(num < number)) {
      lo = lo_mid;
    } else {
      hi = hi_mid;
    }
  }

  return NULL;
}

const upb_MiniTableField* upb_MiniTable_GetOneof(const upb_MiniTable* m,
                                                 const upb_MiniTableField* f) {
  if (UPB_UNLIKELY(!upb_MiniTableField_IsInOneof(f))) {
    return NULL;
  }
  const upb_MiniTableField* ptr = &m->UPB_PRIVATE(fields)[0];
  const upb_MiniTableField* end =
      &m->UPB_PRIVATE(fields)[m->UPB_PRIVATE(field_count)];
  for (; ptr < end; ptr++) {
    if (ptr->presence == (*f).presence) {
      return ptr;
    }
  }
  return NULL;
}

bool upb_MiniTable_NextOneofField(const upb_MiniTable* m,
                                  const upb_MiniTableField** f) {
  const upb_MiniTableField* ptr = *f;
  const upb_MiniTableField* end =
      &m->UPB_PRIVATE(fields)[m->UPB_PRIVATE(field_count)];
  while (++ptr < end) {
    if (ptr->presence == (*f)->presence) {
      *f = ptr;
      return true;
    }
  }
  return false;
}
