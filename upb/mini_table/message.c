// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/mini_table/message.h"

#include <inttypes.h>

#include "upb/mem/arena.h"
#include "upb/mini_table/internal/message.h"

// Must be last.
#include "upb/port/def.inc"

const upb_MiniTableField* upb_MiniTable_FindFieldByNumber(
    const upb_MiniTable* t, uint32_t number) {
  const size_t i = ((size_t)number) - 1;  // 0 wraps to SIZE_MAX

  // Ideal case: index into dense fields
  if (i < t->dense_below) {
    UPB_ASSERT(t->fields[i].number == number);
    return &t->fields[i];
  }

  // Slow case: binary search
  int lo = t->dense_below;
  int hi = t->field_count - 1;
  while (lo <= hi) {
    int mid = (lo + hi) / 2;
    uint32_t num = t->fields[mid].number;
    if (num < number) {
      lo = mid + 1;
      continue;
    }
    if (num > number) {
      hi = mid - 1;
      continue;
    }
    return &t->fields[mid];
  }
  return NULL;
}

static bool upb_MiniTable_Is_Oneof(const upb_MiniTableField* f) {
  return f->presence < 0;
}

const upb_MiniTableField* upb_MiniTable_GetOneof(const upb_MiniTable* m,
                                                 const upb_MiniTableField* f) {
  if (UPB_UNLIKELY(!upb_MiniTable_Is_Oneof(f))) {
    return NULL;
  }
  const upb_MiniTableField* ptr = &m->fields[0];
  const upb_MiniTableField* end = &m->fields[m->field_count];
  while (++ptr < end) {
    if (ptr->presence == (*f).presence) {
      return ptr;
    }
  }
  return NULL;
}

bool upb_MiniTable_NextOneofField(const upb_MiniTable* m,
                                  const upb_MiniTableField** f) {
  const upb_MiniTableField* ptr = *f;
  const upb_MiniTableField* end = &m->fields[m->field_count];
  while (++ptr < end) {
    if (ptr->presence == (*f)->presence) {
      *f = ptr;
      return true;
    }
  }
  return false;
}
