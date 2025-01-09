// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/message.h"

#include <math.h>

#include "upb/base/internal/log2.h"
#include "upb/message/internal/message.h"

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

static const size_t overhead = sizeof(upb_Message_InternalData);

upb_Message* upb_Message_New(const upb_MiniTable* mini_table,
                             upb_Arena* arena) {
  return _upb_Message_New(mini_table, arena);
}

static bool realloc_internal(upb_Message* msg, size_t need, upb_Arena* arena) {
  upb_Message_Internal* in = upb_Message_Getinternal(msg);
  if (!in->internal) {
    /* No internal data, allocate from scratch. */
    size_t size = UPB_MAX(128, upb_Log2CeilingSize(need + overhead));
    upb_Message_InternalData* internal = upb_Arena_Malloc(arena, size);
    if (!internal) return false;
    internal->size = size;
    internal->unknown_end = overhead;
    internal->ext_begin = size;
    in->internal = internal;
  } else if (in->internal->ext_begin - in->internal->unknown_end < need) {
    /* Internal data is too small, reallocate. */
    size_t new_size = upb_Log2CeilingSize(in->internal->size + need);
    size_t ext_bytes = in->internal->size - in->internal->ext_begin;
    size_t new_ext_begin = new_size - ext_bytes;
    upb_Message_InternalData* internal =
        upb_Arena_Realloc(arena, in->internal, in->internal->size, new_size);
    if (!internal) return false;
    if (ext_bytes) {
      /* Need to move extension data to the end. */
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

bool _upb_Message_AddUnknown(upb_Message* msg, const char* data, size_t len,
                             upb_Arena* arena) {
  if (!realloc_internal(msg, len, arena)) return false;
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

const upb_Message_Extension* _upb_Message_Getexts(const upb_Message* msg,
                                                  size_t* count) {
  const upb_Message_Internal* in = upb_Message_Getinternal(msg);
  if (in->internal) {
    *count = (in->internal->size - in->internal->ext_begin) /
             sizeof(upb_Message_Extension);
    return UPB_PTR_AT(in->internal, in->internal->ext_begin, void);
  } else {
    *count = 0;
    return NULL;
  }
}

const upb_Message_Extension* _upb_Message_Getext(
    const upb_Message* msg, const upb_MiniTableExtension* e) {
  size_t n;
  const upb_Message_Extension* ext = _upb_Message_Getexts(msg, &n);

  /* For now we use linear search exclusively to find extensions. If this
   * becomes an issue due to messages with lots of extensions, we can introduce
   * a table of some sort. */
  for (size_t i = 0; i < n; i++) {
    if (ext[i].ext == e) {
      return &ext[i];
    }
  }

  return NULL;
}

upb_Message_Extension* _upb_Message_GetOrCreateExtension(
    upb_Message* msg, const upb_MiniTableExtension* e, upb_Arena* arena) {
  upb_Message_Extension* ext =
      (upb_Message_Extension*)_upb_Message_Getext(msg, e);
  if (ext) return ext;
  if (!realloc_internal(msg, sizeof(upb_Message_Extension), arena)) return NULL;
  upb_Message_Internal* in = upb_Message_Getinternal(msg);
  in->internal->ext_begin -= sizeof(upb_Message_Extension);
  ext = UPB_PTR_AT(in->internal, in->internal->ext_begin, void);
  memset(ext, 0, sizeof(upb_Message_Extension));
  ext->ext = e;
  return ext;
}

size_t upb_Message_ExtensionCount(const upb_Message* msg) {
  size_t count;
  _upb_Message_Getexts(msg, &count);
  return count;
}
