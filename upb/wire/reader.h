// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_WIRE_READER_H_
#define UPB_WIRE_READER_H_

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "upb/base/internal/endian.h"
#include "upb/wire/eps_copy_input_stream.h"
#include "upb/wire/internal/reader.h"
#include "upb/wire/types.h"  // IWYU pragma: export

// Must be last.
#include "upb/port/def.inc"

// The upb_WireReader interface is suitable for general-purpose parsing of
// protobuf binary wire format. It is designed to be used along with
// upb_EpsCopyInputStream for buffering, and all parsing routines in this file
// assume that at least kUpb_EpsCopyInputStream_SlopBytes worth of data is
// available to read without any bounds checks.

#ifdef __cplusplus
extern "C" {
#endif

// Parses a tag into `tag`, and returns a pointer past the end of the tag, or
// NULL if there was an error in the tag data.
//
// REQUIRES: there must be at least 10 bytes of data available at `ptr`.
// Bounds checks must be performed before calling this function, preferably
// by calling upb_EpsCopyInputStream_IsDone().
UPB_FORCEINLINE const char* upb_WireReader_ReadTag(
    const char* ptr, uint32_t* tag, upb_EpsCopyInputStream* stream);

// Given a tag, returns the field number.
UPB_API_INLINE uint32_t upb_WireReader_GetFieldNumber(uint32_t tag);

// Given a tag, returns the wire type.
UPB_API_INLINE uint8_t upb_WireReader_GetWireType(uint32_t tag);

UPB_FORCEINLINE const char* upb_WireReader_ReadVarint(
    const char* ptr, uint64_t* val, upb_EpsCopyInputStream* stream);

// Skips data for a varint, returning a pointer past the end of the varint, or
// NULL if there was an error in the varint data.
//
// REQUIRES: there must be at least 10 bytes of data available at `ptr`.
// Bounds checks must be performed before calling this function, preferably
// by calling upb_EpsCopyInputStream_IsDone().
UPB_INLINE const char* upb_WireReader_SkipVarint(
    const char* ptr, upb_EpsCopyInputStream* stream) {
  uint64_t val;
  return upb_WireReader_ReadVarint(ptr, &val, stream);
}

// Reads a varint indicating the size of a delimited field into `size`, or
// NULL if there was an error in the varint data.
//
// REQUIRES: there must be at least 10 bytes of data available at `ptr`.
// Bounds checks must be performed before calling this function, preferably
// by calling upb_EpsCopyInputStream_IsDone().
UPB_INLINE const char* upb_WireReader_ReadSize(const char* ptr, int* size,
                                               upb_EpsCopyInputStream* stream);

// Reads a fixed32 field, performing byte swapping if necessary.
//
// REQUIRES: there must be at least 4 bytes of data available at `ptr`.
// Bounds checks must be performed before calling this function, preferably
// by calling upb_EpsCopyInputStream_IsDone().
UPB_INLINE const char* upb_WireReader_ReadFixed32(
    const char* ptr, void* val, upb_EpsCopyInputStream* stream) {
  UPB_PRIVATE(upb_EpsCopyInputStream_ConsumeBytes)(stream, 4);
  uint32_t uval;
  memcpy(&uval, ptr, 4);
  uval = upb_BigEndian32(uval);
  memcpy(val, &uval, 4);
  return ptr + 4;
}

// Reads a fixed64 field, performing byte swapping if necessary.
//
// REQUIRES: there must be at least 4 bytes of data available at `ptr`.
// Bounds checks must be performed before calling this function, preferably
// by calling upb_EpsCopyInputStream_IsDone().
UPB_INLINE const char* upb_WireReader_ReadFixed64(
    const char* ptr, void* val, upb_EpsCopyInputStream* stream) {
  UPB_PRIVATE(upb_EpsCopyInputStream_ConsumeBytes)(stream, 8);
  uint64_t uval;
  memcpy(&uval, ptr, 8);
  uval = upb_BigEndian64(uval);
  memcpy(val, &uval, 8);
  return ptr + 8;
}

const char* UPB_PRIVATE(_upb_WireReader_SkipGroup)(
    const char* ptr, uint32_t tag, int depth_limit,
    upb_EpsCopyInputStream* stream);

// Skips data for a group, returning a pointer past the end of the group, or
// NULL if there was an error parsing the group.  The `tag` argument should be
// the start group tag that begins the group.  The `depth_limit` argument
// indicates how many levels of recursion the group is allowed to have before
// reporting a parse error (this limit exists to protect against stack
// overflow).
//
// TODO: evaluate how the depth_limit should be specified. Do users need
// control over this?
UPB_INLINE const char* upb_WireReader_SkipGroup(
    const char* ptr, uint32_t tag, upb_EpsCopyInputStream* stream) {
  const char* ret =
      UPB_PRIVATE(_upb_WireReader_SkipGroup)(ptr, tag, 100, stream);
  return UPB_PRIVATE(upb_EpsCopyInputStream_AssumeResult)(stream, ret);
}

UPB_INLINE const char* _upb_WireReader_SkipValue(
    const char* ptr, uint32_t tag, int depth_limit,
    upb_EpsCopyInputStream* stream) {
  switch (upb_WireReader_GetWireType(tag)) {
    case kUpb_WireType_Varint:
      return upb_WireReader_SkipVarint(ptr, stream);
    case kUpb_WireType_32Bit:
      UPB_PRIVATE(upb_EpsCopyInputStream_ConsumeBytes)(stream, 4);
      return ptr + 4;
    case kUpb_WireType_64Bit:
      UPB_PRIVATE(upb_EpsCopyInputStream_ConsumeBytes)(stream, 8);
      return ptr + 8;
    case kUpb_WireType_Delimited: {
      int size;
      ptr = upb_WireReader_ReadSize(ptr, &size, stream);
      if (!ptr || !upb_EpsCopyInputStream_CheckSize(stream, ptr, size)) {
        return UPB_PRIVATE(upb_EpsCopyInputStream_ReturnError)(stream);
      }
      ptr += size;
      return ptr;
    }
    case kUpb_WireType_StartGroup:
      return UPB_PRIVATE(_upb_WireReader_SkipGroup)(ptr, tag, depth_limit,
                                                    stream);
    case kUpb_WireType_EndGroup:
      // Should be handled before now.
    default:
      // Unknown wire type.
      return UPB_PRIVATE(upb_EpsCopyInputStream_ReturnError)(stream);
  }
}

// Skips data for a wire value of any type, returning a pointer past the end of
// the data, or NULL if there was an error parsing the group. The `tag` argument
// should be the tag that was just parsed. The `depth_limit` argument indicates
// how many levels of recursion a group is allowed to have before reporting a
// parse error (this limit exists to protect against stack overflow).
//
// REQUIRES: there must be at least 10 bytes of data available at `ptr`.
// Bounds checks must be performed before calling this function, preferably
// by calling upb_EpsCopyInputStream_IsDone().
//
// TODO: evaluate how the depth_limit should be specified. Do users need
// control over this?
UPB_INLINE const char* upb_WireReader_SkipValue(
    const char* ptr, uint32_t tag, upb_EpsCopyInputStream* stream) {
  return _upb_WireReader_SkipValue(ptr, tag, 100, stream);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif  // UPB_WIRE_READER_H_
