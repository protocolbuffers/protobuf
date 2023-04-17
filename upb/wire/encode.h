/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// upb_Encode: parsing from a upb_Message using a upb_MiniTable.

#ifndef UPB_WIRE_ENCODE_H_
#define UPB_WIRE_ENCODE_H_

#include "upb/message/message.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

enum {
  /* If set, the results of serializing will be deterministic across all
   * instances of this binary. There are no guarantees across different
   * binary builds.
   *
   * If your proto contains maps, the encoder will need to malloc()/free()
   * memory during encode. */
  kUpb_EncodeOption_Deterministic = 1,

  // When set, unknown fields are not printed.
  kUpb_EncodeOption_SkipUnknown = 2,

  // When set, the encode will fail if any required fields are missing.
  kUpb_EncodeOption_CheckRequired = 4,
};

typedef enum {
  kUpb_EncodeStatus_Ok = 0,
  kUpb_EncodeStatus_OutOfMemory = 1,  // Arena alloc failed
  kUpb_EncodeStatus_MaxDepthExceeded = 2,

  // kUpb_EncodeOption_CheckRequired failed but the parse otherwise succeeded.
  kUpb_EncodeStatus_MissingRequired = 3,
} upb_EncodeStatus;

UPB_INLINE uint32_t upb_EncodeOptions_MaxDepth(uint16_t depth) {
  return (uint32_t)depth << 16;
}

UPB_INLINE uint16_t upb_EncodeOptions_GetMaxDepth(uint32_t options) {
  return options >> 16;
}

// Enforce an upper bound on recursion depth.
UPB_INLINE int upb_Encode_LimitDepth(uint32_t encode_options, uint32_t limit) {
  uint32_t max_depth = upb_EncodeOptions_GetMaxDepth(encode_options);
  if (max_depth > limit) max_depth = limit;
  return upb_EncodeOptions_MaxDepth(max_depth) | (encode_options & 0xffff);
}

upb_EncodeStatus upb_Encode(const void* msg, const upb_MiniTable* l,
                            int options, upb_Arena* arena, char** buf,
                            size_t* size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_WIRE_ENCODE_H_ */
