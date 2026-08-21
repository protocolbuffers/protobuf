// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_UPB_UPB_WIRE_INTERNAL_DECODE_FAST_DATALAYOUT_H__
#define GOOGLE_UPB_UPB_WIRE_INTERNAL_DECODE_FAST_DATALAYOUT_H__

#include <stdint.h>

// Must be last.
#include "upb/port/def.inc"

// The layout of the data field of _upb_FastTable_Entry.field_data is as
// follows:
//
//                  48                32                16                 0
// |--------|--------|--------|--------|--------|--------|--------|--------|
// |   offset (16)   |case offset (16) |presence| subofs |  exp. tag (16)  |
// |--------|--------|--------|--------|--------|--------|--------|--------|
//
// - `offset` is the offset of the field in the message struct.
// - `case_offset` is the offset of the oneof selector for a oneof field
//   (or 0 if not a oneof field).
// - `presence` is either hasbit index or field number for oneofs.
// - `subofs` is the 8 byte shifted submessage offset from the fields array
//    start into the subs array (or 0 if no sub).
// - `expected_tag` is the expected value of the tag for this field.

UPB_INLINE bool upb_DecodeFast_MakeData(uint64_t offset, uint64_t case_offset,
                                        uint64_t presence, uint64_t subofs,
                                        uint64_t expected_tag,
                                        uint64_t* out_data) {
  if (offset > 0xffff || case_offset > 0xffff || presence > 0xff ||
      subofs > 0xff || expected_tag > 0xffff) {
    return false;
  }

  *out_data = (offset << 48) | (case_offset << 32) | (presence << 24) |
              (subofs << 16) | expected_tag;
  return true;
}

UPB_INLINE uint16_t upb_DecodeFastData_GetOffset(uint64_t data) {
  return data >> 48;
}

UPB_INLINE uint16_t upb_DecodeFastData_GetCaseOffset(uint64_t data) {
  return data >> 32;
}

UPB_INLINE uint8_t upb_DecodeFastData_GetPresence(uint64_t data) {
  return data >> 24;
}

UPB_INLINE uint8_t upb_DecodeFastData_GetSubofs(uint64_t data) {
  return data >> 16;
}

UPB_INLINE uint16_t upb_DecodeFastData_GetExpectedTag(uint64_t data) {
  return data;
}

UPB_INLINE int upb_DecodeFastData_GetTableSlot(uint64_t data) {
  uint16_t tag = upb_DecodeFastData_GetExpectedTag(data);
  return (tag & 0xf8) >> 3;
}

// The layout of the data2 parameter is as follows:
//
//                  48                32                16                 0
// |--------|--------|--------|--------|--------|--------|--------|--------|
// |  mask  |                  (unused)                  |   actual tag    |
// |--------|--------|--------|--------|--------|--------|--------|--------|
UPB_INLINE uint8_t upb_DecodeFastData2_GetMask(uint64_t data2) {
  return data2 >> 56;
}

UPB_INLINE uint64_t upb_DecodeFastData2_PackMask(uint8_t mask) {
  return (uint64_t)mask << 56;
}

UPB_INLINE uint16_t upb_DecodeFastData2_GetOriginalTag(uint64_t data2) {
  return data2 & 0xffff;
}

UPB_INLINE uint64_t upb_DecodeFastData2_PackOriginalTag(uint64_t data2,
                                                        uint16_t tag) {
  return (data2 & 0xffffffffffff0000) | tag;
}

UPB_INLINE uint8_t upb_DecodeFastData2_GetWireType(uint64_t data2) {
  return data2 & 0x07;
}

UPB_INLINE uint8_t upb_DecodeFastData2_GetTagLen(uint64_t data2) {
  return (data2 >> 3) & 0x07;
}

UPB_INLINE uint64_t upb_DecodeFastData2_PackWireTypeAndTagLen(uint64_t data2,
                                                              uint8_t wire_type,
                                                              uint8_t tag_len) {
  return (data2 & ~((uint64_t)0x3F)) | ((uint64_t)tag_len << 3) |
         (wire_type & 0x07);
}

UPB_INLINE uint64_t upb_DecodeFast_PackGaps(uint32_t gap_lo, uint32_t gap_hi) {
  return ((uint64_t)gap_lo << 32) | gap_hi;
}

UPB_INLINE uint32_t upb_DecodeFast_GetGapLo(uint64_t data) {
  return data >> 32;
}

UPB_INLINE uint32_t upb_DecodeFast_GetGapHi(uint64_t data) {
  return data & 0xffffffff;
}

#include "upb/port/undef.inc"

#endif  // GOOGLE_UPB_UPB_WIRE_INTERNAL_DECODE_FAST_DATALAYOUT_H__
