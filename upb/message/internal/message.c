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
#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb/message/internal/extension.h"
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

UPB_INLINE size_t _upb_Message_InternalBlockSize(uint32_t count) {
  size_t bytes = UPB_SIZEOF_FLEX(upb_Message_Internal, aux_data, count);
  return upb_RoundUpToPowerOfTwo(
      UPB_MAX(bytes, UPB_PRIVATE(kUpb_Arena_MinPoolBlockSize)));
}

UPB_INLINE uint32_t _upb_Message_InternalCapacity(size_t block_bytes) {
  size_t capacity =
      UPB_FLEX_CAPACITY(upb_Message_Internal, aux_data, block_bytes);
  if (capacity > UINT32_MAX) {
    return UINT32_MAX;
  }
  return (uint32_t)capacity;
}

bool UPB_PRIVATE(_upb_Message_ReserveSlot)(struct upb_Message* msg,
                                           upb_Arena* a) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  if (!in) {
    // No internal data, allocate from scratch.
    size_t block_bytes = _upb_Message_InternalBlockSize(1);
    in = (upb_Message_Internal*)upb_Arena_AllocPool(a, block_bytes);
    if (!in) return false;
    in->size = 0;
    in->capacity = _upb_Message_InternalCapacity(block_bytes);
    UPB_PRIVATE(_upb_Message_SetInternal)(msg, in);
  } else if (in->capacity == in->size) {
    if (in->size == UINT32_MAX) return false;
    // Internal data is too small, reallocate.
    if (UPB_SIZEOF_FLEX_WOULD_OVERFLOW(upb_Message_Internal, aux_data,
                                       in->size + 1)) {
      return false;
    }
    size_t old_bytes =
        UPB_SIZEOF_FLEX(upb_Message_Internal, aux_data, in->capacity);
    size_t new_bytes = _upb_Message_InternalBlockSize(in->size + 1);
    if (new_bytes == SIZE_MAX ||
        _upb_Message_InternalCapacity(new_bytes) > UINT32_MAX) {
      return false;
    }
    if (upb_Arena_TryExtend(a, in, old_bytes, new_bytes)) {
      in->capacity = _upb_Message_InternalCapacity(new_bytes);
    } else {
      upb_Message_Internal* new_in =
          (upb_Message_Internal*)upb_Arena_AllocPool(a, new_bytes);
      if (!new_in) return false;
      memcpy(new_in, in,
             UPB_SIZEOF_FLEX(upb_Message_Internal, aux_data, in->size));
      new_in->capacity = _upb_Message_InternalCapacity(new_bytes);
      if (UPB_PRIVATE(_upb_Arena_IsValidPoolSize)(old_bytes)) {
        upb_Arena_FreePool(a, in, old_bytes);
      }
      in = new_in;
      UPB_PRIVATE(_upb_Message_SetInternal)(msg, in);
    }
  }
  UPB_ASSERT(in->capacity - in->size >= 1);
  return true;
}

bool UPB_PRIVATE(_upb_Message_CopyInternal)(struct upb_Message* dst,
                                            const struct upb_Message* src,
                                            upb_Arena* arena) {
  const upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(src);
  if (!in) return true;

  size_t needed_bytes =
      UPB_SIZEOF_FLEX(upb_Message_Internal, aux_data, in->size);
  size_t block_bytes = _upb_Message_InternalBlockSize(in->size);
  upb_Message_Internal* dst_in = NULL;
  if (block_bytes != SIZE_MAX) {
    dst_in = (upb_Message_Internal*)upb_Arena_TryAllocPool(arena, block_bytes);
  }
  if (!dst_in) {
    block_bytes = needed_bytes;
    dst_in = upb_Arena_Malloc(arena, block_bytes);
    if (!dst_in) return false;
  }

  dst_in->size = 0;
  dst_in->capacity = _upb_Message_InternalCapacity(block_bytes);

  for (size_t i = 0; i < in->size; i++) {
    upb_TaggedAuxPtr tagged_ptr = in->aux_data[i];
    if (upb_TaggedAuxPtr_IsExtension(tagged_ptr)) {
      const upb_Extension* msg_ext = upb_TaggedAuxPtr_Extension(tagged_ptr);
      upb_Extension* dst_ext = upb_Arena_Malloc(arena, sizeof(upb_Extension));
      if (!dst_ext) return false;
      *dst_ext = *msg_ext;
      dst_in->aux_data[dst_in->size++] = upb_TaggedAuxPtr_MakeExtension(
          dst_ext, upb_TaggedAuxPtr_Type(tagged_ptr));
    } else if (upb_TaggedAuxPtr_IsUnknownStringView(tagged_ptr)) {
      upb_StringView* dst_sv = upb_Arena_Malloc(arena, sizeof(upb_StringView));
      if (!dst_sv) return false;
      *dst_sv = *upb_TaggedPtrAux_StringViewRepr(tagged_ptr);
      dst_in->aux_data[dst_in->size++] =
          upb_TaggedAuxPtr_MakeUnknownDataAliased(dst_sv);
    }
  }

  UPB_PRIVATE(_upb_Message_SetInternal)(dst, dst_in);
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
