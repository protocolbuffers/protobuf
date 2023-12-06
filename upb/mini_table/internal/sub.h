// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MINI_TABLE_INTERNAL_SUB_H_
#define UPB_MINI_TABLE_INTERNAL_SUB_H_

#include "upb/mini_table/internal/field.h"

// Must be last.
#include "upb/port/def.inc"

union upb_MiniTableSub {
  const struct upb_MiniTable* UPB_PRIVATE(submsg);
  const struct upb_MiniTableEnum* UPB_PRIVATE(subenum);
};

#ifdef __cplusplus
extern "C" {
#endif

UPB_INLINE union upb_MiniTableSub UPB_PRIVATE(_upb_MiniTableSub_FromEnum)(
    const struct upb_MiniTableEnum* subenum) {
  union upb_MiniTableSub out;
  out.UPB_PRIVATE(subenum) = subenum;
  return out;
}

UPB_INLINE union upb_MiniTableSub UPB_PRIVATE(_upb_MiniTableSub_FromMessage)(
    const struct upb_MiniTable* submsg) {
  union upb_MiniTableSub out;
  out.UPB_PRIVATE(submsg) = submsg;
  return out;
}

UPB_INLINE const struct upb_MiniTableEnum* UPB_PRIVATE(_upb_MiniTableSub_Enum)(
    const union upb_MiniTableSub sub) {
  return sub.UPB_PRIVATE(subenum);
}

UPB_INLINE const struct upb_MiniTable* UPB_PRIVATE(_upb_MiniTableSub_Message)(
    const union upb_MiniTableSub sub) {
  return sub.UPB_PRIVATE(submsg);
}

// Returns the MiniTableEnum corresponding to a given MiniTableField
// from an array of MiniTableSubs. Note: NOT an array of pointers -
// an array of upb_MiniTableSub objects all laid out sequentially in memory.

UPB_INLINE const struct upb_MiniTableEnum* UPB_PRIVATE(_upb_MiniTableSubs_Enum)(
    const union upb_MiniTableSub* subs, const struct upb_MiniTableField* f) {
  const union upb_MiniTableSub sub = subs[f->UPB_PRIVATE(submsg_index)];
  return UPB_PRIVATE(_upb_MiniTableSub_Enum)(sub);
}

// Returns the MiniTable corresponding to a given MiniTableField
// from an array of MiniTableSubs. Note: NOT an array of pointers -
// an array of upb_MiniTableSub objects all laid out sequentially in memory.

UPB_INLINE const struct upb_MiniTable* UPB_PRIVATE(_upb_MiniTableSubs_Message)(
    const union upb_MiniTableSub* subs, const struct upb_MiniTableField* f) {
  const union upb_MiniTableSub sub = subs[f->UPB_PRIVATE(submsg_index)];
  return UPB_PRIVATE(_upb_MiniTableSub_Message)(sub);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MINI_TABLE_INTERNAL_SUB_H_ */
