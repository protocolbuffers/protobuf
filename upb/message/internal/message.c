// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/internal/message.h"

#include <math.h>
#include <string.h>

#include "upb/base/internal/log2.h"
#include "upb/mem/arena.h"
#include "upb/message/internal/types.h"

// Must be last.
#include "upb/port/def.inc"

const float kUpb_FltInfinity = (float)(1.0 / 0.0);
const double kUpb_Infinity = 1.0 / 0.0;
const double kUpb_NaN = 0.0 / 0.0;

bool UPB_PRIVATE(_upb_Message_Realloc)(struct upb_Message* msg, size_t need,
                                       upb_Arena* a) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  const size_t overhead = sizeof(upb_Message_Internal);

  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  if (!in) {
    // No internal data, allocate from scratch.
    size_t size = UPB_MAX(128, upb_Log2CeilingSize(need + overhead));
    in = upb_Arena_Malloc(a, size);
    if (!in) return false;

    in->size = size;
    in->unknown_end = overhead;
    in->ext_begin = size;
    UPB_PRIVATE(_upb_Message_SetInternal)(msg, in);
  } else if (in->ext_begin - in->unknown_end < need) {
    // Internal data is too small, reallocate.
    size_t new_size = upb_Log2CeilingSize(in->size + need);
    size_t ext_bytes = in->size - in->ext_begin;
    size_t new_ext_begin = new_size - ext_bytes;
    in = upb_Arena_Realloc(a, in, in->size, new_size);
    if (!in) return false;

    if (ext_bytes) {
      // Need to move extension data to the end.
      char* ptr = (char*)in;
      memmove(ptr + new_ext_begin, ptr + in->ext_begin, ext_bytes);
    }
    in->ext_begin = new_ext_begin;
    in->size = new_size;
    UPB_PRIVATE(_upb_Message_SetInternal)(msg, in);
  }

  UPB_ASSERT(in->ext_begin - in->unknown_end >= need);
  return true;
}

#if UPB_TRACING_ENABLED
static void (*_message_trace_handler)(const upb_MiniTable*, const upb_Arena*);

void upb_Message_LogNewMessage(const upb_MiniTable* m, const upb_Arena* arena) {
  if (_message_trace_handler) {
    _message_trace_handler(m, arena);
  }
}

void upb_Message_SetNewMessageTraceHandler(void (*handler)(const upb_MiniTable*,
                                                           const upb_Arena*)) {
  _message_trace_handler = handler;
}
#endif  // UPB_TRACING_ENABLED
