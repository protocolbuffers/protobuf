// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_UPB_UPB_WIRE_INTERNAL_DECODE_FAST_DATALAYOUT_H__
#define GOOGLE_UPB_UPB_WIRE_INTERNAL_DECODE_FAST_DATALAYOUT_H__

#include <stddef.h>
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

// The layout of the data field of _upb_FastTable_Entry.field_data for map
// fields is as follows:
//
//                  48                32                16                 0
// |--------|--------|--------|--------|--------|--------|--------|--------|
// |   offset (16)   |--B3|vsz|ksz|*|*|*|*|-|vwt|kwt|subofs (8)| exp. tag (16) |
// |--------|--------|--------|--------|--------|--------|--------|--------|
//
// - `offset` (bits 48-63) is the offset of the upb_Map pointer in the message.
// - bits 46-47 are unused.
// - bit 45 is `val_is_bool` (true if the map value is a bool).
// - bit 44 is `key_is_32` (true if the integer key is 4 bytes wide).
// - `val_size` (bits 40-43) is upb_Map::val_size for this map.
// - `key_size` (bits 36-39) is upb_Map::key_size for this map.
// - bit 32 is `key_is_zigzag` (true if integer key uses sint32/sint64).
// - bit 33 is `val_is_zigzag` (true if integer value uses sint32/sint64).
// - bit 34 is `is_tag2` (true if the map field wire tag is 2 bytes).
// - bit 35 is `is_str_map` (true if map has string/bytes keys).
// - bits 30-31 are unused.
// - `val_wire_type` (bits 27-29) is the wire type that the map entry's value
//   (field 2) must have in order to match the schema.
// - `key_wire_type` (bits 24-26) is the wire type that the map entry's key
//   (field 1) must have in order to match the schema.
// - `subofs` (bits 16-23) is the 8-byte shifted map entry submessage offset.
// - `expected_tag` (bits 0-15) is the expected wire tag for the map field.
//
// The key/value wire types are baked in at table build time so that the decoder
// can reject any map entry whose key or value uses a wire type other than the
// one the schema requires. Such a field is an unknown field within the entry,
// which means the entry must be preserved in the parent's unknown fields
// instead of being inserted into the map; only the MiniTable decoder implements
// that, so those entries have to fall back.
//
// `key_size`/`val_size` mirror the upb_Map fields of the same name, and
// `key_is_32`/`val_is_bool` are redundant encodings of `key_size == 4` and
// `val_size == 1`. They are baked in here so that the map entry loop can test
// them without loading from the upb_Map on every entry; on aarch64 each becomes
// a single tbz/tbnz or ubfx against a register.

enum {
  kUpb_DecodeFastMap_KeyWireTypeShift = 24,
  kUpb_DecodeFastMap_ValWireTypeShift = 27,
  kUpb_DecodeFastMap_WireTypeMask = 0x7,

  kUpb_DecodeFastMap_KeyIsZigZag = 1ULL << 32,
  kUpb_DecodeFastMap_ValIsZigZag = 1ULL << 33,
  kUpb_DecodeFastMap_IsTag2 = 1ULL << 34,
  kUpb_DecodeFastMap_IsStrMap = 1ULL << 35,

  kUpb_DecodeFastMap_KeySizeShift = 36,
  kUpb_DecodeFastMap_ValSizeShift = 40,
  kUpb_DecodeFastMap_SizeMask = 0xf,
  kUpb_DecodeFastMap_KeyIs32 = 1ULL << 44,
  kUpb_DecodeFastMap_ValIsBool = 1ULL << 45,
};

// `key_size` and `val_size` are upb_Map::key_size / upb_Map::val_size, i.e. the
// kSizeInMap[] values from decode.c (UPB_MAPTYPE_STRING for string/bytes).
UPB_INLINE bool upb_DecodeFast_MakeMapData(
    uint64_t offset, bool key_is_zigzag, bool val_is_zigzag, bool is_tag2,
    bool is_str_map, uint64_t key_wire_type, uint64_t val_wire_type,
    uint64_t key_size, uint64_t val_size, uint64_t subofs,
    uint64_t expected_tag, uint64_t* out_data) {
  if (offset > 0xffff || subofs > 0xff || expected_tag > 0xffff ||
      key_wire_type > kUpb_DecodeFastMap_WireTypeMask ||
      val_wire_type > kUpb_DecodeFastMap_WireTypeMask ||
      key_size > kUpb_DecodeFastMap_SizeMask ||
      val_size > kUpb_DecodeFastMap_SizeMask) {
    return false;
  }

  uint64_t flags =
      (key_is_zigzag ? (uint64_t)kUpb_DecodeFastMap_KeyIsZigZag : 0) |
      (val_is_zigzag ? (uint64_t)kUpb_DecodeFastMap_ValIsZigZag : 0) |
      (is_tag2 ? (uint64_t)kUpb_DecodeFastMap_IsTag2 : 0) |
      (is_str_map ? (uint64_t)kUpb_DecodeFastMap_IsStrMap : 0) |
      (key_size == 4 ? (uint64_t)kUpb_DecodeFastMap_KeyIs32 : 0) |
      (val_size == 1 ? (uint64_t)kUpb_DecodeFastMap_ValIsBool : 0) |
      (key_size << kUpb_DecodeFastMap_KeySizeShift) |
      (val_size << kUpb_DecodeFastMap_ValSizeShift) |
      (key_wire_type << kUpb_DecodeFastMap_KeyWireTypeShift) |
      (val_wire_type << kUpb_DecodeFastMap_ValWireTypeShift);
  *out_data = (offset << 48) | flags | (subofs << 16) | expected_tag;
  return true;
}

UPB_INLINE bool upb_DecodeFastData_KeyIsZigZag(uint64_t data) {
  return (data & kUpb_DecodeFastMap_KeyIsZigZag) != 0;
}

UPB_INLINE bool upb_DecodeFastData_ValIsZigZag(uint64_t data) {
  return (data & kUpb_DecodeFastMap_ValIsZigZag) != 0;
}

UPB_INLINE bool upb_DecodeFastData_IsTag2(uint64_t data) {
  return (data & kUpb_DecodeFastMap_IsTag2) != 0;
}

UPB_INLINE bool upb_DecodeFastData_IsStrMap(uint64_t data) {
  return (data & kUpb_DecodeFastMap_IsStrMap) != 0;
}

UPB_INLINE size_t upb_DecodeFastData_GetKeySize(uint64_t data) {
  return (data >> kUpb_DecodeFastMap_KeySizeShift) &
         kUpb_DecodeFastMap_SizeMask;
}

UPB_INLINE size_t upb_DecodeFastData_GetValSize(uint64_t data) {
  return (data >> kUpb_DecodeFastMap_ValSizeShift) &
         kUpb_DecodeFastMap_SizeMask;
}

// Equivalent to upb_DecodeFastData_GetKeySize(data) == 4, but a single-bit
// test.
UPB_INLINE bool upb_DecodeFastData_KeyIs32(uint64_t data) {
  return (data & kUpb_DecodeFastMap_KeyIs32) != 0;
}

// Equivalent to upb_DecodeFastData_GetValSize(data) == 1, but a single-bit
// test.
UPB_INLINE bool upb_DecodeFastData_ValIsBool(uint64_t data) {
  return (data & kUpb_DecodeFastMap_ValIsBool) != 0;
}

// Returns the complete tag (field number 1 + wire type) that the map entry's
// key must have in order to match the schema.
UPB_INLINE uint8_t upb_DecodeFastData_GetKeyTag(uint64_t data) {
  uint64_t wire_type = (data >> kUpb_DecodeFastMap_KeyWireTypeShift) &
                       kUpb_DecodeFastMap_WireTypeMask;
  return (uint8_t)((1 << 3) | wire_type);
}

// Returns the complete tag (field number 2 + wire type) that the map entry's
// value must have in order to match the schema.
UPB_INLINE uint8_t upb_DecodeFastData_GetValTag(uint64_t data) {
  uint64_t wire_type = (data >> kUpb_DecodeFastMap_ValWireTypeShift) &
                       kUpb_DecodeFastMap_WireTypeMask;
  return (uint8_t)((2 << 3) | wire_type);
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

UPB_INLINE uint32_t upb_DecodeFastData_GetFieldNumber(uint64_t data) {
  uint16_t tag = upb_DecodeFastData_GetExpectedTag(data);
  return ((tag >> 3) & 0x0f) | ((tag >> 4) & 0x7f0);
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
