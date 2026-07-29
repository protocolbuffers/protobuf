// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// upb_Encode: parsing from a upb_Message using a upb_MiniTable.

#ifndef UPB_WIRE_ENCODE_H_
#define UPB_WIRE_ENCODE_H_

#include <stddef.h>
#include <stdint.h>

#include "upb/base/error_handler.h"
#include "upb/mem/arena.h"
#include "upb/message/message.h"
#include "upb/mini_table/message.h"
#include "upb/wire/internal/constants.h"

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

  // When set, unknown fields are not encoded.
  kUpb_EncodeOption_SkipUnknown = 2,

  // When set, the encode will fail if any required fields are missing.
  kUpb_EncodeOption_CheckRequired = 4,
};

// LINT.IfChange
typedef enum {
  kUpb_EncodeStatus_Ok = kUpb_ErrorCode_Ok,
  kUpb_EncodeStatus_OutOfMemory =
      kUpb_ErrorCode_OutOfMemory,  // Arena alloc failed
  // One or more required fields are missing. Only returned if
  // kUpb_EncodeOption_CheckRequired is set.
  kUpb_EncodeStatus_MaxDepthExceeded = kUpb_ErrorCode_MaxDepthExceeded,

  kUpb_EncodeStatus_MissingRequired = 10,
  // The message is larger than protobuf's 2GB size limit.
  kUpb_EncodeStatus_MaxSizeExceeded = 11,
} upb_EncodeStatus;
// LINT.ThenChange(//depot/google3/third_party/upb/rust/sys/wire/wire.rs:encode_status)

UPB_INLINE uint32_t upb_EncodeOptions_MaxDepth(uint16_t depth) {
  return (uint32_t)depth << 16;
}

UPB_INLINE uint16_t upb_EncodeOptions_GetMaxDepth(uint32_t options) {
  return options >> 16;
}

UPB_INLINE uint16_t upb_EncodeOptions_GetEffectiveMaxDepth(uint32_t options) {
  uint16_t max_depth = upb_EncodeOptions_GetMaxDepth(options);
  return max_depth ? max_depth : kUpb_WireFormat_DefaultDepthLimit;
}

// Enforce an upper bound on recursion depth.
UPB_INLINE int upb_Encode_LimitDepth(uint32_t encode_options, uint32_t limit) {
  uint32_t max_depth = upb_EncodeOptions_GetEffectiveMaxDepth(encode_options);
  if (max_depth > limit) max_depth = limit;
  return (int)(upb_EncodeOptions_MaxDepth(max_depth) |
               (encode_options & 0xffff));
}

UPB_NODISCARD UPB_API upb_EncodeStatus upb_Encode(const upb_Message* msg,
                                                  const upb_MiniTable* l,
                                                  int options, upb_Arena* arena,
                                                  char** buf, size_t* size);

// Encodes the message prepended by a varint of the serialized length.
UPB_NODISCARD UPB_API upb_EncodeStatus upb_EncodeLengthPrefixed(
    const upb_Message* msg, const upb_MiniTable* l, int options,
    upb_Arena* arena, char** buf, size_t* size);
// Utility function for wrapper languages to get an error string from a
// upb_EncodeStatus.
UPB_API const char* upb_EncodeStatus_String(upb_EncodeStatus status);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_WIRE_ENCODE_H_ */
