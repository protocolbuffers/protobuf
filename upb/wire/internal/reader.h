// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_WIRE_INTERNAL_READER_H_
#define UPB_WIRE_INTERNAL_READER_H_

#include <stddef.h>
#include <stdint.h>

#include "upb/wire/eps_copy_input_stream.h"

// Must be last.
#include "upb/port/def.inc"

#define kUpb_WireReader_WireTypeBits 3
#define kUpb_WireReader_WireTypeMask 7

typedef struct {
  const char* ptr;
  uint64_t val;
} UPB_PRIVATE(_upb_WireReader_LongVarint);

#ifdef __cplusplus
extern "C" {
#endif

UPB_PRIVATE(_upb_WireReader_LongVarint)
UPB_PRIVATE(_upb_WireReader_ReadLongVarint32)(const char* ptr, uint32_t val);
UPB_PRIVATE(_upb_WireReader_LongVarint)
UPB_PRIVATE(_upb_WireReader_ReadLongVarint64)(const char* ptr, uint64_t val);

UPB_FORCEINLINE const char* UPB_PRIVATE(_upb_WireReader_ReadVarint)(
    const char* ptr, uint64_t* val, upb_EpsCopyInputStream* stream) {
  UPB_PRIVATE(upb_EpsCopyInputStream_ConsumeBytes)(stream, 10);
  uint8_t byte = *ptr;
  if (UPB_LIKELY((byte & 0x80) == 0)) {
    *val = byte;
    return ptr + 1;
  }
  UPB_PRIVATE(_upb_WireReader_LongVarint)
  res = UPB_PRIVATE(_upb_WireReader_ReadLongVarint64)(ptr, byte);
  if (UPB_UNLIKELY(!res.ptr)) return NULL;
  *val = res.val;
  return res.ptr;
}

UPB_FORCEINLINE const char* UPB_PRIVATE(_upb_WireReader_ReadTag)(
    const char* ptr, uint32_t* val, upb_EpsCopyInputStream* stream) {
  UPB_PRIVATE(upb_EpsCopyInputStream_ConsumeBytes)(stream, 5);
  uint8_t byte = *ptr;
  if (UPB_LIKELY((byte & 0x80) == 0)) {
    *val = byte;
    return ptr + 1;
  }
  UPB_PRIVATE(_upb_WireReader_LongVarint)
  res = UPB_PRIVATE(_upb_WireReader_ReadLongVarint32)(ptr, byte);
  if (UPB_UNLIKELY(!res.ptr)) return NULL;
  *val = res.val;
  return res.ptr;
}

UPB_API_INLINE uint32_t upb_WireReader_GetFieldNumber(uint32_t tag) {
  return tag >> kUpb_WireReader_WireTypeBits;
}

UPB_API_INLINE uint8_t upb_WireReader_GetWireType(uint32_t tag) {
  return tag & kUpb_WireReader_WireTypeMask;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif  // UPB_WIRE_INTERNAL_READER_H_
