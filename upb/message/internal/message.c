// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/internal/message.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "upb/base/internal/log2.h"
#include "upb/mem/arena.h"
#include "upb/message/internal/types.h"

// Must be last.
#include "upb/port/def.inc"

// The latest win32 SDKs have an invalid definition of NAN.
// https://developercommunity.visualstudio.com/t/NAN-is-no-longer-compile-time-constant-i/10688907
//
// Unfortunately, the `0.0 / 0.0` workaround doesn't work in Clang under C23, so
// try __builtin_nan first, if that exists.
#ifdef _WIN32
#ifdef __has_builtin
#if __has_builtin(__builtin_nan)
#define UPB_NAN __builtin_nan("0")
#endif
#if __has_builtin(__builtin_inf)
#define UPB_INFINITY __builtin_inf()
#endif
#endif
#ifndef UPB_NAN
#define UPB_NAN 0.0 / 0.0
#endif
#ifndef UPB_INFINITY
#define UPB_INFINITY 1.0 / 0.0
#endif
#else
// For !_WIN32, assume math.h works.
#define UPB_NAN NAN
#define UPB_INFINITY INFINITY
#endif

const float kUpb_FltInfinity = UPB_INFINITY;
const double kUpb_Infinity = UPB_INFINITY;
const double kUpb_NaN = UPB_NAN;

static size_t _upb_Message_SizeOfInternal(uint32_t count) {
  return UPB_SIZEOF_FLEX(upb_Message_Internal, aux_data, count);
}

bool UPB_PRIVATE(_upb_Message_ReserveSlot)(struct upb_Message* msg,
                                           upb_Arena* a) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  if (!in) {
    // No internal data, allocate from scratch.
    uint32_t capacity = 4;
    in = upb_Arena_Malloc(a, _upb_Message_SizeOfInternal(capacity));
    if (!in) return false;
    in->size = 0;
    in->capacity = capacity;
    UPB_PRIVATE(_upb_Message_SetInternal)(msg, in);
  } else if (in->capacity == in->size) {
    // Internal data is too small, reallocate.
    if (in->size == UINT32_MAX) return false;
    uint32_t needed_size = in->size + 1;
    // TODO need to use unsigned here
    uint32_t new_capacity = upb_RoundUpToPowerOfTwo(needed_size);
    if (new_capacity < needed_size) return false;
    if (UPB_SIZEOF_FLEX_WOULD_OVERFLOW(upb_Message_Internal, aux_data,
                                       new_capacity)) {
      return false;
    }
    in = upb_Arena_Realloc(a, in, _upb_Message_SizeOfInternal(in->capacity),
                           _upb_Message_SizeOfInternal(new_capacity));
    if (!in) return false;
    in->capacity = new_capacity;
    UPB_PRIVATE(_upb_Message_SetInternal)(msg, in);
  }
  UPB_ASSERT(in->capacity - in->size >= 1);
  return true;
}

#ifdef UPB_TRACING_ENABLED
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
