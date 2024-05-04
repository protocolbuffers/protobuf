// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// upb_Encode: parsing from a upb_Message using a upb_MiniTable.

#ifndef UPB_WIRE_BATCHED_H_
#define UPB_WIRE_BATCHED_H_

#include <stdint.h>

#include "upb/base/string_view.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    kUpb_MaxBatch = 128,

    kUpb_FieldNumberShift = 6,
    kUpb_BigFieldNumber = (1 << 10) - 1,

    kUpb_LongField = 31,
};

UPB_INLINE int upb_BatchEncoder_PrimitiveFieldSize(
    const upb_MiniTableField* f) {
  upb_FieldRep rep = UPB_PRIVATE(_upb_MiniTableField_GetRep)(f);
  // kUpb_FieldRep_1Byte = 0,
  // kUpb_FieldRep_4Byte = 1,
  // kUpb_FieldRep_StringView = 2,
  // kUpb_FieldRep_8Byte = 3,
  const uint32_t sizes =
      1 | (4 << 4) | (sizeof(upb_StringView) << 8) | (8 << 12);
  return (sizes >> (rep * 4)) & 0xf;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_WIRE_BATCHED_H_ */
