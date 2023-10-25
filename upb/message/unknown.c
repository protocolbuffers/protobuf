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

#include "upb/message/unknown.h"

#include "upb/message/internal/message.h"
#include "upb/message/realloc.h"

// Must be last.
#include "upb/port/def.inc"

static const size_t overhead = sizeof(upb_Message_InternalData);

bool _upb_Message_AddUnknown(upb_Message* msg, const char* data, size_t len,
                             upb_Arena* arena) {
  if (!_upb_Message_Realloc(msg, len, arena)) return false;
  upb_Message_Internal* in = upb_Message_Getinternal(msg);
  memcpy(UPB_PTR_AT(in->internal, in->internal->unknown_end, char), data, len);
  in->internal->unknown_end += len;
  return true;
}

void _upb_Message_DiscardUnknown_shallow(upb_Message* msg) {
  upb_Message_Internal* in = upb_Message_Getinternal(msg);
  if (in->internal) {
    in->internal->unknown_end = overhead;
  }
}

const char* upb_Message_GetUnknown(const upb_Message* msg, size_t* len) {
  const upb_Message_Internal* in = upb_Message_Getinternal(msg);
  if (in->internal) {
    *len = in->internal->unknown_end - overhead;
    return (char*)(in->internal + 1);
  } else {
    *len = 0;
    return NULL;
  }
}

void upb_Message_DeleteUnknown(upb_Message* msg, const char* data, size_t len) {
  upb_Message_Internal* in = upb_Message_Getinternal(msg);
  const char* internal_unknown_end =
      UPB_PTR_AT(in->internal, in->internal->unknown_end, char);
#ifndef NDEBUG
  size_t full_unknown_size;
  const char* full_unknown = upb_Message_GetUnknown(msg, &full_unknown_size);
  UPB_ASSERT((uintptr_t)data >= (uintptr_t)full_unknown);
  UPB_ASSERT((uintptr_t)data < (uintptr_t)(full_unknown + full_unknown_size));
  UPB_ASSERT((uintptr_t)(data + len) > (uintptr_t)data);
  UPB_ASSERT((uintptr_t)(data + len) <= (uintptr_t)internal_unknown_end);
#endif
  if ((data + len) != internal_unknown_end) {
    memmove((char*)data, data + len, internal_unknown_end - data - len);
  }
  in->internal->unknown_end -= len;
}
