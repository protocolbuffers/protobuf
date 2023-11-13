// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MINI_TABLE_ENUM_H_
#define UPB_MINI_TABLE_ENUM_H_

#include "upb/mini_table/internal/enum.h"

// Must be last
#include "upb/port/def.inc"

typedef struct upb_MiniTableEnum upb_MiniTableEnum;

#ifdef __cplusplus
extern "C" {
#endif

// Validates enum value against range defined by enum mini table.
UPB_INLINE bool upb_MiniTableEnum_CheckValue(const struct upb_MiniTableEnum* e,
                                             uint32_t val) {
  _kUpb_FastEnumCheck_Status status = _upb_MiniTable_CheckEnumValueFast(e, val);
  if (UPB_UNLIKELY(status == _kUpb_FastEnumCheck_CannotCheckFast)) {
    return _upb_MiniTable_CheckEnumValueSlow(e, val);
  }
  return status == _kUpb_FastEnumCheck_ValueIsInEnum ? true : false;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MINI_TABLE_ENUM_H_ */
