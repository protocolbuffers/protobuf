// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb_generator/minitable/layout_comment.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/descriptor.h"
#include "upb/base/descriptor_constants.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/message.h"
#include "upb/reflection/def.hpp"
#include "upb_generator/file_layout.h"

// Must be last.
#include "upb/port/def.inc"

namespace upb {
namespace generator {
namespace {

// Every upb_Message begins with an 8-byte header (a tagged pointer to the
// upb_Message_Internal, padded to 8 bytes on 32-bit platforms). The hasbits
// that would overlap the header are reserved and never assigned.
constexpr uint32_t kHeaderSize = 8;

constexpr absl::string_view kPadding = "(padding)";

enum class Platform { k32Bit, k64Bit };

// One contiguous range of bytes within a message.
struct Region {
  Region(uint32_t offset, uint32_t size, std::string field, std::string type,
         std::string storage, std::string presence)
      : offset(offset),
        size(size),
        field(std::move(field)),
        type(std::move(type)),
        storage(std::move(storage)),
        presence(std::move(presence)) {}

  uint32_t offset;
  uint32_t size;
  std::string field;
  std::string type;
  std::string storage;
  std::string presence;
  // For oneof data slots: one entry per member, all sharing this offset.
  std::vector<Region> members;
};

const upb_MiniTable* GetMiniTable(const DefPoolPair& pools,
                                  upb::MessageDefPtr message,
                                  Platform platform) {
  return platform == Platform::k64Bit ? pools.GetMiniTable64(message)
                                      : pools.GetMiniTable32(message);
}

const upb_MiniTableField* GetField(const DefPoolPair& pools,
                                   upb::FieldDefPtr field, Platform platform) {
  return platform == Platform::k64Bit ? pools.GetField64(field)
                                      : pools.GetField32(field);
}

uint32_t SizeOfRep(upb_FieldRep rep, Platform platform) {
  switch (rep) {
    case kUpb_FieldRep_1Byte:
      return 1;
    case kUpb_FieldRep_4Byte:
      return 4;
    case kUpb_FieldRep_StringView:
      return platform == Platform::k64Bit ? 16 : 8;
    case kUpb_FieldRep_8Byte:
      return 8;
  }
  ABSL_CHECK(false) << "Unknown rep: " << rep;
  return 0;
}

// Returns the .proto spelling of the field's type, e.g. "int32" or
// "foo.Bar".
std::string FieldTypeName(upb::FieldDefPtr field) {
  switch (field.type()) {
    case kUpb_FieldType_Group:
      return absl::StrCat("group ", field.message_type().full_name());
    case kUpb_FieldType_Message:
      return field.message_type().full_name();
    case kUpb_FieldType_Enum:
      return absl::StrCat("enum ", field.enum_subdef().full_name(),
                          field.enum_subdef().is_closed() ? " (closed)" : "");
    default:
      // upb_FieldType values are identical to descriptor.proto's
      // FieldDescriptorProto.Type values.
      return std::string(google::protobuf::FieldDescriptor::TypeName(
          static_cast<google::protobuf::FieldDescriptor::Type>(field.type())));
  }
}

std::string FieldDisplayType(upb::FieldDefPtr field) {
  if (field.IsMap()) {
    upb::MessageDefPtr entry = field.message_type();
    return absl::StrCat("map<", FieldTypeName(entry.map_key()), ", ",
                        FieldTypeName(entry.map_value()), ">");
  }
  if (field.IsSequence()) {
    return absl::StrCat("repeated ", FieldTypeName(field),
                        field.packed() ? " [packed]" : "");
  }
  return FieldTypeName(field);
}

std::string FieldStorage(upb::FieldDefPtr field) {
  if (field.IsMap()) return "upb_Map*";
  if (field.IsSequence()) return "upb_Array*";
  switch (field.ctype()) {
    case kUpb_CType_Bool:
      return "bool";
    case kUpb_CType_Float:
      return "float";
    case kUpb_CType_Int32:
    case kUpb_CType_Enum:
      return "int32_t";
    case kUpb_CType_UInt32:
      return "uint32_t";
    case kUpb_CType_Double:
      return "double";
    case kUpb_CType_Int64:
      return "int64_t";
    case kUpb_CType_UInt64:
      return "uint64_t";
    case kUpb_CType_String:
    case kUpb_CType_Bytes:
      return "upb_StringView";
    case kUpb_CType_Message:
      return "upb_Message*";
  }
  return "?";
}

std::string FieldLabel(upb::FieldDefPtr field) {
  return absl::StrCat(field.name(), " = ", field.number());
}

// Computes all of the regions (fields, hasbits, oneof cases, padding) for a
// message on the given platform, sorted by offset.
std::vector<Region> ComputeRegions(const DefPoolPair& pools,
                                   upb::MessageDefPtr message,
                                   Platform platform) {
  const uint32_t size =
      GetMiniTable(pools, message, platform)->UPB_PRIVATE(size);

  std::vector<Region> regions;
  regions.push_back(
      {0, kHeaderSize, "(message header)", "", "upb_Message", ""});

  // Fields outside of any real oneof each get their own slot.
  int hasbit_count = 0;
  uint32_t hasbit_end = 0;
  for (int i = 0; i < message.field_count(); i++) {
    upb::FieldDefPtr field = message.field(i);
    if (field.real_containing_oneof()) continue;
    const upb_MiniTableField* mt_f = GetField(pools, field, platform);

    std::string presence;
    if (UPB_PRIVATE(_upb_MiniTableField_HasHasbit)(mt_f)) {
      const uint16_t byte = UPB_PRIVATE(_upb_MiniTableField_HasbitOffset)(mt_f);
      hasbit_count++;
      hasbit_end = std::max<uint32_t>(hasbit_end, byte + 1);
      presence = absl::Substitute("$0hasbit $1 (byte $2, bit $3)",
                                  field.is_required() ? "required, " : "",
                                  mt_f->presence, byte, mt_f->presence % 8);
    } else if (!field.IsSequence()) {
      ABSL_CHECK(!field.has_presence());
      presence = "implicit";
    }

    regions.push_back(
        {UPB_PRIVATE(_upb_MiniTableField_Offset)(mt_f),
         SizeOfRep(UPB_PRIVATE(_upb_MiniTableField_GetRep)(mt_f), platform),
         FieldLabel(field), FieldDisplayType(field), FieldStorage(field),
         presence});
  }

  if (hasbit_count > 0) {
    ABSL_CHECK_GE(hasbit_end, kHeaderSize);
    regions.push_back({kHeaderSize, hasbit_end - kHeaderSize,
                       absl::StrCat("(hasbits: ", hasbit_count, " used)"), "",
                       "uint8_t[]", ""});
  }

  // All members of a real oneof share a single data slot and a single case
  // slot. upb orders real oneofs before synthetic (proto3 optional) ones.
  for (int i = 0; i < message.real_oneof_count(); i++) {
    upb::OneofDefPtr oneof = message.oneof(i);
    const upb_MiniTableField* first = GetField(pools, oneof.field(0), platform);
    const uint32_t offset = UPB_PRIVATE(_upb_MiniTableField_Offset)(first);
    const uint32_t case_offset =
        UPB_PRIVATE(_upb_MiniTableField_OneofOffset)(first);

    std::vector<Region> members;
    uint32_t slot_size = 0;
    for (int j = 0; j < oneof.field_count(); j++) {
      upb::FieldDefPtr member = oneof.field(j);
      const upb_MiniTableField* mt_member = GetField(pools, member, platform);
      ABSL_CHECK_EQ(UPB_PRIVATE(_upb_MiniTableField_Offset)(mt_member), offset);
      const uint32_t member_size = SizeOfRep(
          UPB_PRIVATE(_upb_MiniTableField_GetRep)(mt_member), platform);
      slot_size = std::max(slot_size, member_size);
      members.push_back({offset, member_size,
                         absl::StrCat("  - ", FieldLabel(member)),
                         FieldDisplayType(member), FieldStorage(member), ""});
    }
    regions.push_back({case_offset, 4,
                       absl::StrCat("(oneof case: ", oneof.name(), ")"),
                       "field number", "uint32_t", ""});
    Region& slot = regions.emplace_back(
        offset, slot_size, absl::StrCat("oneof ", oneof.name()), "", "union",
        absl::StrCat("oneof case @", case_offset));
    slot.members = std::move(members);
  }

  std::sort(
      regions.begin(), regions.end(),
      [](const Region& a, const Region& b) { return a.offset < b.offset; });

  // Fill in any gaps (including trailing alignment) with padding.
  std::vector<Region> with_padding;
  uint32_t pos = 0;
  for (Region& r : regions) {
    ABSL_CHECK_GE(r.offset, pos) << "Overlapping regions in "
                                 << message.full_name() << " at " << r.field;
    if (r.offset > pos) {
      with_padding.push_back(
          {pos, r.offset - pos, std::string(kPadding), "", "", ""});
    }
    pos = r.offset + r.size;
    with_padding.push_back(std::move(r));
  }
  ABSL_CHECK_LE(pos, size);
  if (pos < size) {
    with_padding.push_back(
        {pos, size - pos, std::string(kPadding), "", "", ""});
  }
  return with_padding;
}

// Renders a list of rows as an ASCII table, with every line prefixed by
// `prefix`. The first row is the header. Columns whose index is set in
// `right_align` are right-aligned.
std::string RenderTable(const std::vector<std::vector<std::string>>& rows,
                        const std::vector<bool>& right_align,
                        absl::string_view prefix) {
  ABSL_CHECK(!rows.empty());
  const size_t cols = rows[0].size();
  std::vector<size_t> widths(cols, 0);
  for (const auto& row : rows) {
    for (size_t c = 0; c < cols; c++) {
      widths[c] = std::max(widths[c], row[c].size());
    }
  }

  std::string sep = absl::StrCat(prefix, "+");
  for (size_t c = 0; c < cols; c++) {
    absl::StrAppend(&sep, std::string(widths[c] + 2, '-'), "+");
  }
  absl::StrAppend(&sep, "\n");

  auto render_row = [&](const std::vector<std::string>& row) {
    std::string line = absl::StrCat(prefix, "|");
    for (size_t c = 0; c < cols; c++) {
      std::string pad(widths[c] - row[c].size(), ' ');
      if (right_align[c]) {
        absl::StrAppend(&line, " ", pad, row[c], " |");
      } else {
        absl::StrAppend(&line, " ", row[c], pad, " |");
      }
    }
    absl::StrAppend(&line, "\n");
    return line;
  };

  std::string out = sep;
  absl::StrAppend(&out, render_row(rows[0]), sep);
  for (size_t i = 1; i < rows.size(); i++) {
    absl::StrAppend(&out, render_row(rows[i]));
  }
  absl::StrAppend(&out, sep);
  return out;
}

// Renders the layout table for one platform, with every line prefixed by
// " * " so that it can be embedded in a C block comment.
std::string RenderPlatformLayout(const DefPoolPair& pools,
                                 upb::MessageDefPtr message,
                                 Platform platform) {
  std::vector<Region> regions = ComputeRegions(pools, message, platform);

  uint32_t padding = 0;
  for (const Region& r : regions) {
    if (r.field == kPadding) padding += r.size;
  }

  std::vector<std::vector<std::string>> rows;
  rows.push_back({"Offset", "Size", "Field", "Type", "Storage", "Presence"});
  for (const Region& r : regions) {
    rows.push_back({absl::StrCat(r.offset), absl::StrCat(r.size), r.field,
                    r.type, r.storage, r.presence});
    for (const Region& m : r.members) {
      rows.push_back(
          {"", absl::StrCat(m.size), m.field, m.type, m.storage, m.presence});
    }
  }

  return absl::StrCat(
      absl::Substitute(
          " * $0: $1 bytes ($2 bytes padding)\n",
          platform == Platform::k64Bit ? "64-bit" : "32-bit",
          GetMiniTable(pools, message, platform)->UPB_PRIVATE(size), padding),
      RenderTable(rows, {true, true, false, false, false, false}, " * "));
}

}  // namespace

std::string MessageLayoutComment(const DefPoolPair& pools,
                                 upb::MessageDefPtr message) {
  std::string out =
      absl::StrCat("/* ", message.full_name(), ": in-memory layout.\n");
  if (message.extension_range_count() > 0) {
    absl::StrAppend(&out,
                    " * Extensions are stored out-of-line in "
                    "upb_Message_Internal.\n");
  }
  absl::StrAppend(
      &out, " *\n", RenderPlatformLayout(pools, message, Platform::k64Bit),
      " *\n", RenderPlatformLayout(pools, message, Platform::k32Bit), " */\n");
  return out;
}

}  // namespace generator
}  // namespace upb

#include "upb/port/undef.inc"
