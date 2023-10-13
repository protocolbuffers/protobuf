// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_TEXT_ENCODE_H_
#define UPB_TEXT_ENCODE_H_

#include "upb/reflection/def.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

enum {
  // When set, prints everything on a single line.
  UPB_TXTENC_SINGLELINE = 1,

  // When set, unknown fields are not printed.
  UPB_TXTENC_SKIPUNKNOWN = 2,

  // When set, maps are *not* sorted (this avoids allocating tmp mem).
  UPB_TXTENC_NOSORT = 4
};

/* Encodes the given |msg| to text format.  The message's reflection is given in
 * |m|.  The symtab in |symtab| is used to find extensions (if NULL, extensions
 * will not be printed).
 *
 * Output is placed in the given buffer, and always NULL-terminated.  The output
 * size (excluding NULL) is returned.  This means that a return value >= |size|
 * implies that the output was truncated.  (These are the same semantics as
 * snprintf()). */
size_t upb_TextEncode(const upb_Message* msg, const upb_MessageDef* m,
                      const upb_DefPool* ext_pool, int options, char* buf,
                      size_t size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_TEXT_ENCODE_H_ */
