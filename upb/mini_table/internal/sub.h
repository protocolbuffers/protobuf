// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MINI_TABLE_INTERNAL_SUB_H_
#define UPB_MINI_TABLE_INTERNAL_SUB_H_

// Must be last.
#include "upb/port/def.inc"

union upb_MiniTableSub {
  const struct upb_MiniTable* submsg;
  const struct upb_MiniTableEnum* subenum;
};

#include "upb/port/undef.inc"

#endif /* UPB_MINI_TABLE_INTERNAL_SUB_H_ */
