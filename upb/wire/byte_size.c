// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/wire/byte_size.h"

#include <stddef.h>

#include "upb/mem/arena.h"
#include "upb/message/message.h"
#include "upb/mini_table/message.h"
#include "upb/wire/encode.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

size_t upb_ByteSize(const upb_Message* msg, const upb_MiniTable* mt) {
  upb_Arena* arena = upb_Arena_New();
  char* buf;
  size_t res = 0;

  upb_Encode(msg, mt, 0, arena, &buf, &res);

  upb_Arena_Free(arena);
  return res;
}

#ifdef __cplusplus
}  // extern "C"
#endif