// Copyright (c) 2009-2024, Google LLC
// All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/wire/decode_fast/select.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "absl/strings/substitute.h"
#include "upb/base/descriptor_constants.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"
#include "upb/wire/types.h"

// Must be last.
#include "upb/port/def.inc"

namespace upb {
namespace generator {

namespace {

// Returns fields in order of "hotness", eg. how frequently they appear in
// serialized payloads. Ideally this will use a profile. When we don't have
// that, we assume that fields with smaller numbers are used more frequently.
inline std::vector<const upb_MiniTableField*> FieldHotnessOrder(
    const upb_MiniTable* mt) {
  std::vector<const upb_MiniTableField*> fields;
  size_t field_count = upb_MiniTable_FieldCount(mt);
  fields.reserve(field_count);
  for (size_t i = 0; i < field_count; i++) {
    fields.push_back(upb_MiniTable_GetFieldByIndex(mt, i));
  }
  return fields;
}

typedef std::pair<std::string, uint64_t> TableEntry;

uint32_t GetWireTypeForField(const upb_MiniTableField* field) {
  if (upb_MiniTableField_IsPacked(field)) return kUpb_WireType_Delimited;
  switch (upb_MiniTableField_Type(field)) {
    case kUpb_FieldType_Double:
    case kUpb_FieldType_Fixed64:
    case kUpb_FieldType_SFixed64:
      return kUpb_WireType_64Bit;
    case kUpb_FieldType_Float:
    case kUpb_FieldType_Fixed32:
    case kUpb_FieldType_SFixed32:
      return kUpb_WireType_32Bit;
    case kUpb_FieldType_Int64:
    case kUpb_FieldType_UInt64:
    case kUpb_FieldType_Int32:
    case kUpb_FieldType_Bool:
    case kUpb_FieldType_UInt32:
    case kUpb_FieldType_Enum:
    case kUpb_FieldType_SInt32:
    case kUpb_FieldType_SInt64:
      return kUpb_WireType_Varint;
    case kUpb_FieldType_Group:
      return kUpb_WireType_StartGroup;
    case kUpb_FieldType_Message:
    case kUpb_FieldType_String:
    case kUpb_FieldType_Bytes:
      return kUpb_WireType_Delimited;
  }
  UPB_UNREACHABLE();
}

uint32_t MakeTag(uint32_t field_number, uint32_t wire_type) {
  return field_number << 3 | wire_type;
}

size_t WriteVarint32ToArray(uint64_t val, char* buf) {
  size_t i = 0;
  do {
    uint8_t byte = val & 0x7fU;
    val >>= 7;
    if (val) byte |= 0x80U;
    buf[i++] = byte;
  } while (val);
  return i;
}

uint64_t GetEncodedTag(const upb_MiniTableField* field) {
  uint32_t wire_type = GetWireTypeForField(field);
  uint32_t unencoded_tag = MakeTag(upb_MiniTableField_Number(field), wire_type);
  char tag_bytes[10] = {0};
  WriteVarint32ToArray(unencoded_tag, tag_bytes);
  uint64_t encoded_tag = 0;
  memcpy(&encoded_tag, tag_bytes, sizeof(encoded_tag));
  // TODO: byte-swap for big endian.
  return encoded_tag;
}

int GetTableSlot(const upb_MiniTableField* field) {
  uint64_t tag = GetEncodedTag(field);
  if (tag > 0x7fff) {
    // Tag must fit within a two-byte varint.
    return -1;
  }
  return (tag & 0xf8) >> 3;
}

bool TryFillTableEntry(const upb_MiniTable* mt, const upb_MiniTableField* mt_f,
                       TableEntry& ent) {
  std::string type = "";
  std::string cardinality = "";
  switch (upb_MiniTableField_Type(mt_f)) {
    case kUpb_FieldType_Bool:
      type = "b1";
      break;
    case kUpb_FieldType_Enum:
      if (upb_MiniTableField_IsClosedEnum(mt_f)) {
        // We don't have the means to test proto2 enum fields for valid values.
        return false;
      }
      [[fallthrough]];
    case kUpb_FieldType_Int32:
    case kUpb_FieldType_UInt32:
      type = "v4";
      break;
    case kUpb_FieldType_Int64:
    case kUpb_FieldType_UInt64:
      type = "v8";
      break;
    case kUpb_FieldType_Fixed32:
    case kUpb_FieldType_SFixed32:
    case kUpb_FieldType_Float:
      type = "f4";
      break;
    case kUpb_FieldType_Fixed64:
    case kUpb_FieldType_SFixed64:
    case kUpb_FieldType_Double:
      type = "f8";
      break;
    case kUpb_FieldType_SInt32:
      type = "z4";
      break;
    case kUpb_FieldType_SInt64:
      type = "z8";
      break;
    case kUpb_FieldType_String:
      type = "s";
      break;
    case kUpb_FieldType_Bytes:
      type = "b";
      break;
    case kUpb_FieldType_Message:
      type = "m";
      break;
    default:
      return false;  // Not supported yet.
  }

  if (upb_MiniTableField_IsArray(mt_f)) {
    cardinality = upb_MiniTableField_IsPacked(mt_f) ? "p" : "r";
  } else if (upb_MiniTableField_IsScalar(mt_f)) {
    cardinality = upb_MiniTableField_IsInOneof(mt_f) ? "o" : "s";
  } else {
    return false;  // Not supported yet (ever?).
  }

  uint64_t expected_tag = GetEncodedTag(mt_f);

  // Data is:
  //
  //                  48                32                16                 0
  // |--------|--------|--------|--------|--------|--------|--------|--------|
  // |   offset (16)   |case offset (16) |presence| submsg |  exp. tag (16)  |
  // |--------|--------|--------|--------|--------|--------|--------|--------|
  //
  // - |presence| is either hasbit index or field number for oneofs.

  uint64_t data =
      static_cast<uint64_t>(mt_f->UPB_PRIVATE(offset)) << 48 | expected_tag;

  if (!upb_MiniTableField_IsScalar(mt_f)) {
    // No hasbit/oneof-related fields.
  } else if (upb_MiniTableField_IsInOneof(mt_f)) {
    uint64_t case_offset = ~mt_f->presence;
    if (case_offset > 0xffff || upb_MiniTableField_Number(mt_f) > 0xff) {
      return false;
    }
    data |= upb_MiniTableField_Number(mt_f) << 24;
    data |= case_offset << 32;
  } else {
    uint64_t hasbit_index = 63;  // No hasbit (set a high, unused bit).
    if (mt_f->presence) {
      hasbit_index = mt_f->presence;
      if (hasbit_index > 31) return false;
    }
    data |= hasbit_index << 24;
  }

  if (upb_MiniTableField_CType(mt_f) == kUpb_CType_Message) {
    uint64_t idx = mt_f->UPB_PRIVATE(submsg_index);
    if (idx > 255) return false;
    data |= idx << 16;

    std::string size_ceil = "max";
    size_t size = SIZE_MAX;
    const upb_MiniTable* sub_mt = upb_MiniTable_GetSubMessageTable(mt, mt_f);
    size = sub_mt->UPB_PRIVATE(size) + 8;
    std::vector<size_t> breaks = {64, 128, 192, 256};
    for (auto brk : breaks) {
      if (size <= brk) {
        size_ceil = std::to_string(brk);
        break;
      }
    }
    ent.first = absl::Substitute("upb_p$0$1_$2bt_max$3b", cardinality, type,
                                 expected_tag > 0xff ? "2" : "1", size_ceil);

  } else {
    ent.first = absl::Substitute("upb_p$0$1_$2bt", cardinality, type,
                                 expected_tag > 0xff ? "2" : "1");
  }
  ent.second = data;
  return true;
}

}  // namespace

std::vector<TableEntry> FastDecodeTable(const upb_MiniTable* mt) {
  std::vector<TableEntry> table;
  for (const auto field : FieldHotnessOrder(mt)) {
    TableEntry ent;
    int slot = GetTableSlot(field);
    // std::cerr << "table slot: " << field->number() << ": " << slot << "\n";
    if (slot < 0) {
      // Tag can't fit in the table.
      continue;
    }
    if (!TryFillTableEntry(mt, field, ent)) {
      // Unsupported field type or offset, hasbit index, etc. doesn't fit.
      continue;
    }
    while ((size_t)slot >= table.size()) {
      size_t size = std::max(static_cast<size_t>(1), table.size() * 2);
      table.resize(size, TableEntry{"_upb_FastDecoder_DecodeGeneric", 0});
    }
    if (table[slot].first != "_upb_FastDecoder_DecodeGeneric") {
      // A hotter field already filled this slot.
      continue;
    }
    table[slot] = ent;
  }
  return table;
}

}  // namespace generator
}  // namespace upb
