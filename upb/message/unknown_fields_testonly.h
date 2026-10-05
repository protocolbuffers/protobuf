// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MESSAGE_UNKNOWN_FIELDS_TESTONLY_H_
#define UPB_MESSAGE_UNKNOWN_FIELDS_TESTONLY_H_

#include "upb/mem/arena.h"
#include "upb/message/internal/accessors.h"
#include "upb/message/message.h"
#include "upb/mini_table/extension.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

UPB_NODISCARD UPB_API_INLINE bool upb_Message_SetNonCanonicalExtension(
    struct upb_Message* msg, const upb_MiniTableExtension* e, const void* value,
    upb_Arena* a) {
  return UPB_PRIVATE(_upb_Message_SetNonCanonicalExtension)(msg, e, value, a);
}

#ifdef __cplusplus
}
#endif

#include "upb/port/undef.inc"

#endif  // UPB_MESSAGE_UNKNOWN_FIELDS_TESTONLY_H_
