// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_WIRE_REQUIRED_H_
#define UPB_WIRE_REQUIRED_H_

#include <stdint.h>
#include <string.h>

#include "upb/base/internal/endian.h"
#include "upb/message/message.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/message.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

// Returns true if the message is missing any of its required fields.
UPB_INLINE bool _upb_Message_MissingRequired(const upb_Message* msg,
                                             const upb_MiniTable* m) {
  uint64_t bits;
  memcpy(&bits, msg, sizeof(bits));
  bits = upb_BigEndian64(bits);
  return (UPB_PRIVATE(_upb_MiniTable_RequiredMask)(m) & ~bits) != 0;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif  // UPB_WIRE_REQUIRED_H_
