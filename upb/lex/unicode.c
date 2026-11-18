// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/lex/unicode.h"

// Must be last.
#include "upb/port/def.inc"

int upb_Unicode_ToUTF8(uint32_t cp, char* out) {
  if (cp <= 0x7f) {
    out[0] = cp;
    return 1;
  }
  if (cp <= 0x07ff) {
    out[0] = (cp >> 6) | 0xc0;
    out[1] = (cp & 0x3f) | 0x80;
    return 2;
  }
  if (cp <= 0xffff) {
    out[0] = (cp >> 12) | 0xe0;
    out[1] = ((cp >> 6) & 0x3f) | 0x80;
    out[2] = (cp & 0x3f) | 0x80;
    return 3;
  }
  if (cp <= 0x10ffff) {
    out[0] = (cp >> 18) | 0xf0;
    out[1] = ((cp >> 12) & 0x3f) | 0x80;
    out[2] = ((cp >> 6) & 0x3f) | 0x80;
    out[3] = (cp & 0x3f) | 0x80;
    return 4;
  }
  return 0;
}
