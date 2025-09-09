// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// -----------------------------------------------------------------------------
// Ruby Message functions. Strictly free of dependencies on
// Ruby interpreter internals.

#include "shared_message.h"

// Support function for Message_Hash. Returns a hash value for the given
// message.
uint64_t shared_Message_Hash(const upb_Message* msg, const upb_MessageDef* m,
                             uint64_t seed, upb_Status* status) {
  upb_Arena* arena = upb_Arena_New();
  char* data;
  size_t size;

  // Hash a deterministically serialized payloads with no unknown fields.
  upb_EncodeStatus encode_status = upb_Encode(
      msg, upb_MessageDef_MiniTable(m),
      kUpb_EncodeOption_SkipUnknown | kUpb_EncodeOption_Deterministic, arena,
      &data, &size);

  if (encode_status == kUpb_EncodeStatus_Ok) {
    uint64_t ret = _upb_Hash(data, size, seed);
    upb_Arena_Free(arena);
    return ret;
  }

  upb_Arena_Free(arena);
  upb_Status_SetErrorMessage(status, "Error calculating hash");
  return 0;
}
