// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_JSON_ENCODE_H_
#define UPB_JSON_ENCODE_H_

#include <stddef.h>
#include <stdint.h>

#include "upb/reflection/def.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

enum {
  /* When set, emits 0/default values.  TODO: proto3 only? */
  upb_JsonEncode_EmitDefaults = 1 << 0,

  /* When set, use normal (snake_case) field names instead of JSON (camelCase)
     names. */
  upb_JsonEncode_UseProtoNames = 1 << 1,

  /* When set, emits enums as their integer values instead of as their names. */
  upb_JsonEncode_FormatEnumsAsIntegers = 1 << 2,

  /* When set, encodes `bytes` fields with the URL- and filename-safe base64
     alphabet from RFC 4648 section 5 ('-' and '_' instead of '+' and '/').
     Padding is unchanged. The decoder accepts both alphabets regardless. */
  upb_JsonEncode_WebSafeBase64 = 1 << 3
};

enum {
  kUpb_JsonEncode_DefaultDepthLimit = 64,
};

UPB_NODISCARD UPB_INLINE uint32_t
upb_JsonEncodeOptions_MaxDepth(uint16_t depth) {
  return (uint32_t)depth << 16;
}

UPB_NODISCARD UPB_INLINE uint16_t
upb_JsonEncodeOptions_GetMaxDepth(uint32_t options) {
  return options >> 16;
}

UPB_NODISCARD UPB_INLINE uint16_t
upb_JsonEncodeOptions_GetEffectiveMaxDepth(uint32_t options) {
  uint16_t max_depth = upb_JsonEncodeOptions_GetMaxDepth(options);
  return max_depth ? max_depth : kUpb_JsonEncode_DefaultDepthLimit;
}

// Enforce an upper bound on recursion depth.
UPB_NODISCARD UPB_INLINE int upb_JsonEncode_LimitDepth(uint32_t encode_options,
                                                       uint32_t limit) {
  uint32_t max_depth =
      upb_JsonEncodeOptions_GetEffectiveMaxDepth(encode_options);
  if (max_depth > limit) max_depth = limit;
  return (int)(upb_JsonEncodeOptions_MaxDepth(max_depth) |
               (encode_options & 0xffff));
}

/* Encodes the given |msg| to JSON format.  The message's reflection is given in
 * |m|.  The DefPool in |ext_pool| is used to find extensions (if NULL,
 * extensions will not be printed).
 *
 * Output is placed in the given buffer, and always NULL-terminated.  The output
 * size (excluding NULL) is returned.  This means that a return value >= |size|
 * implies that the output was truncated.  (These are the same semantics as
 * snprintf()). */
UPB_API size_t upb_JsonEncode(const upb_Message* msg, const upb_MessageDef* m,
                              const upb_DefPool* ext_pool, int options,
                              char* buf, size_t size, upb_Status* status);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_JSONENCODE_H_ */
