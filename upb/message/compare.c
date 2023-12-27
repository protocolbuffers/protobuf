// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/compare.h"

#include <string.h>

#include "upb/mem/arena.h"
#include "upb/message/message.h"
#include "upb/mini_table/message.h"
#include "upb/wire/encode.h"

// Must be last.
#include "upb/port/def.inc"

bool upb_Message_IsExactlyEqual(const upb_Message* msg1,
                                const upb_Message* msg2,
                                const upb_MiniTable* m) {
  if (msg1 == msg2) return true;

  int opts = kUpb_EncodeOption_SkipUnknown | kUpb_EncodeOption_Deterministic;
  upb_Arena* a = upb_Arena_New();

  // Compare deterministically serialized payloads with no unknown fields.
  size_t size1, size2;
  char *data1, *data2;
  upb_EncodeStatus status1 = upb_Encode(msg1, m, opts, a, &data1, &size1);
  upb_EncodeStatus status2 = upb_Encode(msg2, m, opts, a, &data2, &size2);

  if (status1 != kUpb_EncodeStatus_Ok || status2 != kUpb_EncodeStatus_Ok) {
    // TODO: How should we fail here? (In Ruby we throw an exception.)
    upb_Arena_Free(a);
    return false;
  }

  const bool ret = (size1 == size2) && (memcmp(data1, data2, size1) == 0);
  upb_Arena_Free(a);
  return ret;
}
