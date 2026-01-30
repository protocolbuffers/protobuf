// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/reflection/internal/strdup2.h"

#include <string.h>

#include "upb/mem/arena.h"

// Must be last.
#include "upb/port/def.inc"

char* upb_strdup2(const char* s, size_t len, upb_Arena* a) {
  size_t n;
  char* p;

  // Prevent overflow errors.
  if (len == SIZE_MAX) return NULL;

  // Always null-terminate, even if binary data; but don't rely on the input to
  // have a null-terminating byte since it may be a raw binary buffer.
  n = len + 1;
  p = upb_Arena_Malloc(a, n);
  if (p) {
    if (len != 0) memcpy(p, s, len);
    p[len] = 0;
  }
  return p;
}
