// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/text/debug_string.h"

#include <stddef.h>

#include "upb/message/internal/map_sorter.h"
#include "upb/message/message.h"
#include "upb/mini_table/message.h"
#include "upb/text/internal/encode.h"

// Must be last.
#include "upb/port/def.inc"

size_t upb_DebugString(const upb_Message* msg, const upb_MiniTable* mt,
                       int options, char* buf, size_t size) {
  txtenc e;

  e.buf = buf;
  e.ptr = buf;
  e.end = UPB_PTRADD(buf, size);
  e.overflow = 0;
  e.indent_depth = 0;
  e.options = options;
  e.ext_pool = NULL;
  _upb_mapsorter_init(&e.sorter);

  UPB_PRIVATE(_upb_MessageDebugString)(&e, msg, mt);
  _upb_mapsorter_destroy(&e.sorter);
  return UPB_PRIVATE(_upb_TextEncode_Nullz)(&e, size);
}
