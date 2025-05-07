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
// |   offset (16)   |case offset (16) |presence| submsg |  exp. tag (16)  |
// |--------|--------|--------|--------|--------|--------|--------|--------|
//
// - `offset` is the offset of the field in the message struct.
// - `case_offset` is the offset of the oneof selector for a oneof field
//   (or 0 if not a oneof field).
// - `presence` is either hasbit index or field number for oneofs.
// - `submsg_index` is the index of the submessage in the mini table's
//   subs array (or 0 if not a submessage field).
// - `expected_tag` is the expected value of the tag for this field.

UPB_INLINE bool upb_DecodeFast_MakeData(uint64_t offset, uint64_t case_offset,
                                        uint64_t presence,
                                        uint64_t submsg_index,
                                        uint64_t expected_tag,
                                        uint64_t* out_data) {
  if (offset > 0xffff || case_offset > 0xffff || presence > 0xff ||
      submsg_index > 0xff || expected_tag > 0xffff) {
    return false;
  }

  *out_data = (offset << 48) | (case_offset << 32) | (presence << 24) |
              (submsg_index << 16) | expected_tag;
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

UPB_INLINE uint8_t upb_DecodeFastData_GetSubmsgIndex(uint64_t data) {
  return data >> 16;
}

UPB_INLINE uint16_t upb_DecodeFastData_GetExpectedTag(uint64_t data) {
  return data;
}

UPB_INLINE int upb_DecodeFastData_GetTableSlot(uint64_t data) {
  uint16_t tag = upb_DecodeFastData_GetExpectedTag(data);
  return (tag & 0xf8) >> 3;
}

#include "upb/port/undef.inc"

#endif  // GOOGLE_UPB_UPB_WIRE_INTERNAL_DECODE_FAST_DATALAYOUT_H__
