// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MINI_TABLE_INTERNAL_EXTENSION_H_
#define UPB_MINI_TABLE_INTERNAL_EXTENSION_H_

#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/internal/sub.h"

// Must be last.
#include "upb/port/def.inc"

struct upb_MiniTableExtension {
  // Do not move this field. We need to be able to alias pointers.
  struct upb_MiniTableField field;

  const struct upb_MiniTable* extendee;
  union upb_MiniTableSub sub;  // NULL unless submessage or proto2 enum
};

#include "upb/port/undef.inc"

#endif /* UPB_MINI_TABLE_INTERNAL_EXTENSION_H_ */
