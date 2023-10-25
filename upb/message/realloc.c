// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
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
//     * Neither the name of Google LLC nor the names of its
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

#include "upb/message/realloc.h"

#include <string.h>

#include "upb/base/internal/log2.h"
#include "upb/mem/arena.h"
#include "upb/message/internal/message.h"
#include "upb/message/internal/types.h"
#include "upb/message/message.h"

// Must be last.
#include "upb/port/def.inc"

bool _upb_Message_Realloc(upb_Message* msg, size_t need, upb_Arena* arena) {
  const size_t overhead = sizeof(upb_Message_InternalData);

  upb_Message_Internal* in = upb_Message_Getinternal(msg);
  if (!in->internal) {
    // No internal data, allocate from scratch.
    size_t size = UPB_MAX(128, upb_Log2CeilingSize(need + overhead));
    upb_Message_InternalData* internal = upb_Arena_Malloc(arena, size);
    if (!internal) return false;
    internal->size = size;
    internal->unknown_end = overhead;
    internal->ext_begin = size;
    in->internal = internal;
  } else if (in->internal->ext_begin - in->internal->unknown_end < need) {
    // Internal data is too small, reallocate.
    size_t new_size = upb_Log2CeilingSize(in->internal->size + need);
    size_t ext_bytes = in->internal->size - in->internal->ext_begin;
    size_t new_ext_begin = new_size - ext_bytes;
    upb_Message_InternalData* internal =
        upb_Arena_Realloc(arena, in->internal, in->internal->size, new_size);
    if (!internal) return false;
    if (ext_bytes) {
      // Need to move extension data to the end.
      char* ptr = (char*)internal;
      memmove(ptr + new_ext_begin, ptr + internal->ext_begin, ext_bytes);
    }
    internal->ext_begin = new_ext_begin;
    internal->size = new_size;
    in->internal = internal;
  }
  UPB_ASSERT(in->internal->ext_begin - in->internal->unknown_end >= need);
  return true;
}
