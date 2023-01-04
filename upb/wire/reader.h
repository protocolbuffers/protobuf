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

#ifndef UPB_WIRE_READER_H_
#define UPB_WIRE_READER_H_

#include "upb/wire/eps_copy_input_stream.h"
#include "upb/wire/swap_internal.h"
#include "upb/wire/types.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

// The upb_WireReader interface is suitable for general-purpose parsing of
// protobuf binary wire format.  It is designed to be used along with
// upb_EpsCopyInputStream for buffering, and all parsing routines in this file
// assume that at least kUpb_EpsCopyInputStream_SlopBytes worth of data is
// available to read without any bounds checks.

#define kUpb_WireReader_WireTypeMask 7
#define kUpb_WireReader_WireTypeBits 3

typedef struct {
  const char* ptr;
  uint64_t val;
} _upb_WireReader_ReadLongVarintRet;

_upb_WireReader_ReadLongVarintRet _upb_WireReader_ReadLongVarint(
    const char* ptr, uint64_t val);

static UPB_FORCEINLINE const char* _upb_WireReader_ReadVarint(const char* ptr,
                                                              uint64_t* val,
                                                              int maxlen,
                                                              uint64_t maxval) {
  uint64_t byte = (uint8_t)*ptr;
  if (UPB_LIKELY((byte & 0x80) == 0)) {
    *val = (uint32_t)byte;
    return ptr + 1;
  }
  const char* start = ptr;
  _upb_WireReader_ReadLongVarintRet res =
      _upb_WireReader_ReadLongVarint(ptr, byte);
  if (!res.ptr || (maxlen < 10 && res.ptr - start > maxlen) ||
      res.val > maxval) {
    return NULL;  // Malformed.
  }
  *val = res.val;
  return res.ptr;
}

// Parses a tag into `tag`, and returns a pointer past the end of the tag, or
// NULL if there was an error in the tag data.
//
// REQUIRES: there must be at least 10 bytes of data available at `ptr`.
// Bounds checks must be performed before calling this function, preferably
// by calling upb_EpsCopyInputStream_IsDone().
static UPB_FORCEINLINE const char* upb_WireReader_ReadTag(const char* ptr,
                                                          uint32_t* tag) {
  uint64_t val;
  ptr = _upb_WireReader_ReadVarint(ptr, &val, 5, UINT32_MAX);
  if (!ptr) return NULL;
  *tag = val;
  return ptr;
}

// Given a tag, returns the field number.
UPB_INLINE uint32_t upb_WireReader_GetFieldNumber(uint32_t tag) {
  return tag >> kUpb_WireReader_WireTypeBits;
}

// Given a tag, returns the wire type.
UPB_INLINE uint8_t upb_WireReader_GetWireType(uint32_t tag) {
  return tag & kUpb_WireReader_WireTypeMask;
}

UPB_INLINE const char* upb_WireReader_ReadVarint(const char* ptr,
                                                 uint64_t* val) {
  return _upb_WireReader_ReadVarint(ptr, val, 10, UINT64_MAX);
}

// Skips data for a varint, returning a pointer past the end of the varint, or
// NULL if there was an error in the varint data.
//
// REQUIRES: there must be at least 10 bytes of data available at `ptr`.
// Bounds checks must be performed before calling this function, preferably
// by calling upb_EpsCopyInputStream_IsDone().
UPB_INLINE const char* upb_WireReader_SkipVarint(const char* ptr) {
  uint64_t val;
  return upb_WireReader_ReadVarint(ptr, &val);
}

// Reads a varint indicating the size of a delimited field into `size`, or
// NULL if there was an error in the varint data.
//
// REQUIRES: there must be at least 10 bytes of data available at `ptr`.
// Bounds checks must be performed before calling this function, preferably
// by calling upb_EpsCopyInputStream_IsDone().
UPB_INLINE const char* upb_WireReader_ReadSize(const char* ptr, int* size) {
  uint64_t size64;
  ptr = upb_WireReader_ReadVarint(ptr, &size64);
  if (!ptr || size64 >= INT32_MAX) return NULL;
  *size = size64;
  return ptr;
}

// Reads a fixed32 field, performing byte swapping if necessary.
//
// REQUIRES: there must be at least 4 bytes of data available at `ptr`.
// Bounds checks must be performed before calling this function, preferably
// by calling upb_EpsCopyInputStream_IsDone().
UPB_INLINE const char* upb_WireReader_ReadFixed32(const char* ptr, void* val) {
  uint32_t uval;
  memcpy(&uval, ptr, 4);
  uval = _upb_BigEndian_Swap32(uval);
  memcpy(val, &uval, 4);
  return ptr + 4;
}

// Reads a fixed64 field, performing byte swapping if necessary.
//
// REQUIRES: there must be at least 4 bytes of data available at `ptr`.
// Bounds checks must be performed before calling this function, preferably
// by calling upb_EpsCopyInputStream_IsDone().
UPB_INLINE const char* upb_WireReader_ReadFixed64(const char* ptr, void* val) {
  uint64_t uval;
  memcpy(&uval, ptr, 8);
  uval = _upb_BigEndian_Swap64(uval);
  memcpy(val, &uval, 8);
  return ptr + 8;
}

const char* _upb_WireReader_SkipGroup(const char* ptr, uint32_t tag,
                                      int depth_limit,
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
  return _upb_WireReader_SkipGroup(ptr, tag, 100, stream);
}

UPB_INLINE const char* _upb_WireReader_SkipValue(
    const char* ptr, uint32_t tag, int depth_limit,
    upb_EpsCopyInputStream* stream) {
  switch (upb_WireReader_GetWireType(tag)) {
    case kUpb_WireType_Varint:
      return upb_WireReader_SkipVarint(ptr);
    case kUpb_WireType_32Bit:
      return ptr + 4;
    case kUpb_WireType_64Bit:
      return ptr + 8;
    case kUpb_WireType_Delimited: {
      int size;
      ptr = upb_WireReader_ReadSize(ptr, &size);
      if (!ptr) return NULL;
      ptr += size;
      return ptr;
    }
    case kUpb_WireType_StartGroup:
      return _upb_WireReader_SkipGroup(ptr, tag, depth_limit, stream);
    case kUpb_WireType_EndGroup:
      return NULL;  // Should be handled before now.
    default:
      return NULL;  // Unknown wire type.
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
