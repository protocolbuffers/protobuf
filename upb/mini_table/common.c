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

#include "upb/mini_table/common.h"

#include <inttypes.h>

#include "upb/mem/arena.h"
#include "upb/mini_table/common_internal.h"

// Must be last.
#include "upb/port/def.inc"

const char _kUpb_ToBase92[] = {
    ' ', '!', '#', '$', '%', '&', '(', ')', '*', '+', ',', '-', '.', '/',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=',
    '>', '?', '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
    'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y',
    'Z', '[', ']', '^', '_', '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
    'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '{', '|', '}', '~',
};

const int8_t _kUpb_FromBase92[] = {
    0,  1,  -1, 2,  3,  4,  5,  -1, 6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54,
    55, 56, 57, -1, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72,
    73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91,
};

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

upb_FieldType upb_MiniTableField_Type(const upb_MiniTableField* field) {
  if (field->mode & kUpb_LabelFlags_IsAlternate) {
    if (field->UPB_PRIVATE(descriptortype) == kUpb_FieldType_Int32) {
      return kUpb_FieldType_Enum;
    } else if (field->UPB_PRIVATE(descriptortype) == kUpb_FieldType_Bytes) {
      return kUpb_FieldType_String;
    } else {
      UPB_ASSERT(false);
    }
  }
  return field->UPB_PRIVATE(descriptortype);
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
