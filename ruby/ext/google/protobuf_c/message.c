// Protocol Buffers - Google's data interchange format
// Copyright 2014 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "message.h"

// Support function for Message_eq, and also used by other #eq functions.
bool Message_Equal(const upb_Message* m1, const upb_Message* m2,
                   const upb_MessageDef* m) {
  if (m1 == m2) return true;

  size_t size1, size2;
  int encode_opts = kUpb_Encode_SkipUnknown | kUpb_Encode_Deterministic;
  upb_Arena* arena_tmp = upb_Arena_New();
  const upb_MiniTable* layout = upb_MessageDef_MiniTable(m);

  // Compare deterministically serialized payloads with no unknown fields.
  char* data1 = upb_Encode(m1, layout, encode_opts, arena_tmp, &size1);
  char* data2 = upb_Encode(m2, layout, encode_opts, arena_tmp, &size2);

  if (data1 && data2) {
    bool ret = (size1 == size2) && (memcmp(data1, data2, size1) == 0);
    upb_Arena_Free(arena_tmp);
    return ret;
  } else {
    upb_Arena_Free(arena_tmp);
    assert(0);
  }
}

uint64_t Message_Hash(const upb_Message* msg, const upb_MessageDef* m,
                      uint64_t seed) {
  upb_Arena* arena = upb_Arena_New();
  const char* data;
  size_t size;

  // Hash a deterministically serialized payloads with no unknown fields.
  data = upb_Encode(msg, upb_MessageDef_MiniTable(m),
                    kUpb_Encode_SkipUnknown | kUpb_Encode_Deterministic, arena,
                    &size);

  if (data) {
    uint64_t ret = _upb_Hash(data, size, seed);
    upb_Arena_Free(arena);
    return ret;
  } else {
    upb_Arena_Free(arena);
    assert(0);
  }
}
