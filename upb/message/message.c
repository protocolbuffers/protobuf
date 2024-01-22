// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/message.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "upb/mem/arena.h"
#include "upb/message/internal/message.h"
#include "upb/message/internal/types.h"
#include "upb/mini_table/message.h"

// Must be last.
#include "upb/port/def.inc"

static const size_t message_overhead = sizeof(upb_Message_Internal);

upb_Message* upb_Message_New(const upb_MiniTable* m, upb_Arena* a) {
  return _upb_Message_New(m, a);
}

bool UPB_PRIVATE(_upb_Message_AddUnknown)(upb_Message* msg, const char* data,
                                          size_t len, upb_Arena* arena) {
  if (!UPB_PRIVATE(_upb_Message_Realloc)(msg, len, arena)) return false;
  upb_Message_Internal* in = msg->internal;
  memcpy(UPB_PTR_AT(in, in->unknown_end, char), data, len);
  in->unknown_end += len;
  return true;
}

void _upb_Message_DiscardUnknown_shallow(upb_Message* msg) {
  upb_Message_Internal* in = msg->internal;
  if (in) {
    in->unknown_end = message_overhead;
  }
}

const char* upb_Message_GetUnknown(const upb_Message* msg, size_t* len) {
  upb_Message_Internal* in = msg->internal;
  if (in) {
    *len = in->unknown_end - message_overhead;
    return (char*)(in + 1);
  } else {
    *len = 0;
    return NULL;
  }
}

void upb_Message_DeleteUnknown(upb_Message* msg, const char* data, size_t len) {
  upb_Message_Internal* in = msg->internal;
  const char* internal_unknown_end = UPB_PTR_AT(in, in->unknown_end, char);

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
  in->unknown_end -= len;
}

size_t upb_Message_ExtensionCount(const upb_Message* msg) {
  size_t count;
  UPB_PRIVATE(_upb_Message_Getexts)(msg, &count);
  return count;
}
