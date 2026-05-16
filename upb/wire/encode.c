// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// We encode backwards, to avoid pre-computing lengths (one-pass encode).

#include "upb/wire/encode.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "upb/mem/arena.h"
#include "upb/message/message.h"
#include "upb/mini_table/message.h"
#include "upb/wire/internal/encoder.h"

// Must be last.
#include "upb/port/def.inc"

upb_EncodeStatus upb_Encode(const upb_Message* msg, const upb_MiniTable* l,
                            int options, upb_Arena* arena, char** buf,
                            size_t* size) {
  return _upb_Encode(msg, l, options, arena, buf, size, false);
}

upb_EncodeStatus upb_EncodeLengthPrefixed(const upb_Message* msg,
                                          const upb_MiniTable* l, int options,
                                          upb_Arena* arena, char** buf,
                                          size_t* size) {
  return _upb_Encode(msg, l, options, arena, buf, size, true);
}

const char* upb_EncodeStatus_String(upb_EncodeStatus status) {
  switch (status) {
    case kUpb_EncodeStatus_Ok:
      return "Ok";
    case kUpb_EncodeStatus_MissingRequired:
      return "Missing required field";
    case kUpb_EncodeStatus_MaxDepthExceeded:
      return "Max depth exceeded";
    case kUpb_EncodeStatus_OutOfMemory:
      return "Arena alloc failed";
    default:
      return "Unknown encode status";
  }
}
