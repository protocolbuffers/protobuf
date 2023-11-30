// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MINI_TABLE_INTERNAL_SUB_H_
#define UPB_MINI_TABLE_INTERNAL_SUB_H_

#include "upb/mini_table/types.h"

// Must be last.
#include "upb/port/def.inc"

union upb_MiniTableSub {
  const upb_MiniTable* UPB_PRIVATE(submsg);
  const upb_MiniTableEnum* UPB_PRIVATE(subenum);
};

#ifdef __cplusplus
extern "C" {
#endif

UPB_INLINE upb_MiniTableSub
UPB_PRIVATE(_upb_MiniTableSub_FromEnum)(const upb_MiniTableEnum* subenum) {
  upb_MiniTableSub out;
  out.UPB_PRIVATE(subenum) = subenum;
  return out;
}

UPB_INLINE upb_MiniTableSub
UPB_PRIVATE(_upb_MiniTableSub_FromMessage)(const upb_MiniTable* submsg) {
  upb_MiniTableSub out;
  out.UPB_PRIVATE(submsg) = submsg;
  return out;
}

UPB_INLINE const upb_MiniTableEnum* UPB_PRIVATE(_upb_MiniTableSub_Enum)(
    const upb_MiniTableSub sub) {
  return sub.UPB_PRIVATE(subenum);
}

UPB_INLINE const upb_MiniTable* UPB_PRIVATE(_upb_MiniTableSub_Message)(
    const upb_MiniTableSub sub) {
  return sub.UPB_PRIVATE(submsg);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MINI_TABLE_INTERNAL_SUB_H_ */
