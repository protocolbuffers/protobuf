// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/mini_table/minitable_registry.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "upb/mem/alloc.h"
#include "upb/port/atomic.h"

// Must be last.
#include "upb/port/def.inc"

#if !UPB_LINKARR_SORTED
#error \
    "Minitable registry is only supported on ELF targets with link-time sorting support!"
#endif

UPB_LINKARR_DECLARE(upb_AllMiniTables, const upb_MiniTableEntry);

const struct upb_MiniTable* upb_MinitableRegistry_Lookup(uint64_t hash) {
  const upb_MiniTableEntry* start = UPB_LINKARR_START(upb_AllMiniTables);
  const upb_MiniTableEntry* stop = UPB_LINKARR_STOP(upb_AllMiniTables);
  if (start == NULL || start == stop) {
    return NULL;
  }

  // Skip empty/sentinel entries at the start.
  while (start < stop && start->minitable == NULL) {
    start++;
  }

  size_t count = stop - start;
  size_t low = 0;
  size_t high = count;
  while (low < high) {
    size_t mid = low + (high - low) / 2;
    if (start[mid].hash < hash) {
      low = mid + 1;
    } else if (start[mid].hash > hash) {
      high = mid;
    } else {
      return start[mid].minitable;
    }
  }
  return NULL;
}
