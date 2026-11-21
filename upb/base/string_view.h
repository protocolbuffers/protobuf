// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#ifndef UPB_BASE_STRING_VIEW_H_
#define UPB_BASE_STRING_VIEW_H_

#include <limits.h>
#include <stddef.h>
#include <string.h>

// Must be last.
#include "upb/port/def.inc"

#define UPB_STRINGVIEW_INIT(ptr, len) \
  { ptr, len }

#define UPB_STRINGVIEW_FORMAT "%.*s"
#define UPB_STRINGVIEW_ARGS(view) \
  _upb_StringView_FormatPrecision((view).size), (view).data

typedef struct {
  const char* data;
  size_t size;
} upb_StringView;

#ifdef __cplusplus
extern "C" {
#endif

// Returns the precision to pass for UPB_STRINGVIEW_FORMAT ("%.*s").
//
// printf() takes this precision as an int and treats a negative value as if no
// precision was given, in which case it reads the data up to the first NUL
// byte.  String views are not NUL-terminated, so a size above INT_MAX must not
// wrap around to a negative int.  It is clamped instead, which only shortens
// the printed text.
UPB_INLINE int _upb_StringView_FormatPrecision(size_t size) {
  return size < (size_t)INT_MAX ? (int)size : INT_MAX;
}

UPB_API_INLINE upb_StringView upb_StringView_FromDataAndSize(const char* data,
                                                             size_t size) {
  upb_StringView ret;
  ret.data = data;
  ret.size = size;
  return ret;
}

UPB_INLINE upb_StringView upb_StringView_FromString(const char* data) {
  return upb_StringView_FromDataAndSize(data, strlen(data));
}

UPB_INLINE bool upb_StringView_IsEqual(upb_StringView a, upb_StringView b) {
  return (a.size == b.size) && (!a.size || !memcmp(a.data, b.data, a.size));
}

// Compares StringViews following strcmp rules.
// Please note this comparison is neither unicode nor locale aware.
UPB_INLINE int upb_StringView_Compare(upb_StringView a, upb_StringView b) {
  int result = memcmp(a.data, b.data, UPB_MIN(a.size, b.size));
  if (result != 0) return result;
  if (a.size < b.size) {
    return -1;
  } else if (a.size > b.size) {
    return 1;
  } else {
    return 0;
  }
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_BASE_STRING_VIEW_H_ */
