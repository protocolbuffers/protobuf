// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MINI_TABLE_INTERNAL_FILE_H_
#define UPB_MINI_TABLE_INTERNAL_FILE_H_

#include "upb/mini_table/internal/enum.h"
#include "upb/mini_table/internal/extension.h"
#include "upb/mini_table/internal/message.h"

// Must be last.
#include "upb/port/def.inc"

struct upb_MiniTableFile {
  const struct upb_MiniTable** msgs;
  const struct upb_MiniTableEnum** enums;
  const struct upb_MiniTableExtension** exts;
  int msg_count;
  int enum_count;
  int ext_count;
};

#include "upb/port/undef.inc"

#endif /* UPB_MINI_TABLE_INTERNAL_FILE_H_ */
