// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "google/protobuf/generated_message_tctable_gen.h"

#include <algorithm>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/generated_message_tctable_decl.h"
#include "google/protobuf/generated_message_tctable_impl.h"
#include "google/protobuf/wire_format.h"

// Must come last:
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

namespace {

bool GetEnumValidationRange(const EnumDescriptor* enum_type, int16_t& start,
                            uint16_t& size) {
  GOOGLE_CHECK_GT(enum_type->value_count(), 0) << enum_type->DebugString();

  // Check if the enum values are a single, contiguous range.
  std::vector<int> enum_values;
  for (int i = 0, N = static_cast<int>(enum_type->value_count()); i < N; ++i) {
    enum_values.push_back(enum_type->value(i)->number());
  }
  auto values_begin = enum_values.begin();
  auto values_end = enum_values.end();
  std::sort(values_begin, values_end);
  enum_values.erase(std::unique(values_begin, values_end), values_end);

  if (std::numeric_limits<int16_t>::min() <= enum_values[0] &&
      enum_values[0] <= std::numeric_limits<int16_t>::max() &&
      enum_values.size() <= std::numeric_limits<uint16_t>::max() &&
      static_cast<int>(enum_values[0] + enum_values.size() - 1) ==
          enum_values.back()) {
    start = static_cast<int16_t>(enum_values[0]);
    size = static_cast<uint16_t>(enum_values.size());
    return true;
  } else {
    return false;
  }
}

void PopulateFastFieldEntry(const TailCallTableInfo::FieldEntryInfo& entry,
                            const TailCallTableInfo::PerFieldOptions& options,
                            TailCallTableInfo::FastFieldInfo& info) {
  const FieldDescriptor* field = entry.field;
  std::string name = "::_pbi::TcParser::Fast";
  uint8_t aux_idx = static_cast<uint8_t>(entry.aux_idx);

  static const char* kPrefix[] = {
      nullptr,  // 0
      "F64",    // TYPE_DOUBLE = 1,
      "F32",    // TYPE_FLOAT = 2,
      "V64",    // TYPE_INT64 = 3,
      "V64",    // TYPE_UINT64 = 4,
      "V32",    // TYPE_INT32 = 5,
      "F64",    // TYPE_FIXED64 = 6,
      "F32",    // TYPE_FIXED32 = 7,
      "V8",     // TYPE_BOOL = 8,
      "",       // TYPE_STRING = 9,
      "G",      // TYPE_GROUP = 10,
      "M",      // TYPE_MESSAGE = 11,
      "B",      // TYPE_BYTES = 12,
      "V32",    // TYPE_UINT32 = 13,
      "",       // TYPE_ENUM = 14,
      "F32",    // TYPE_SFIXED32 = 15,
      "F64",    // TYPE_SFIXED64 = 16,
      "Z32",    // TYPE_SINT32 = 17,
      "Z64",    // TYPE_SINT64 = 18,
  };
  name.append(kPrefix[field->type()]);

  if (field->type() == field->TYPE_ENUM) {
    // Enums are handled as:
    //  - V32 for open enums
    //  - Er (and Er0/Er1) for sequential enums
    //  - Ev for the rest
    if (cpp::HasPreservingUnknownEnumSemantics(field)) {
      name.append("V32");
    } else if (field->is_repeated() && field->is_packed()) {
      GOOGLE_LOG(DFATAL) << "Enum validation not handled: " << field->DebugString();
      return;
    } else {
      int16_t start;
      uint16_t size;
      if (GetEnumValidationRange(field->enum_type(), start, size)) {
        name.append("Er");
        int max_value = start + size - 1;
        if (max_value <= 127 && (start == 0 || start == 1)) {
          name.append(1, '0' + start);
          aux_idx = max_value;
        }
      } else {
        name.append("Ev");
      }
    }
  }
  if (field->type() == field->TYPE_STRING) {
    switch (internal::cpp::GetUtf8CheckMode(field, options.is_lite)) {
      case internal::cpp::Utf8CheckMode::kStrict:
        name.append("U");
        break;
      case internal::cpp::Utf8CheckMode::kVerify:
        name.append("S");
        break;
      case internal::cpp::Utf8CheckMode::kNone:
        name.append("B");
        break;
    }
  }
  if (field->type() == field->TYPE_STRING ||
      field->type() == field->TYPE_BYTES) {
    if (options.is_string_inlined) {
      name.append("i");
      GOOGLE_CHECK(!field->is_repeated());
      aux_idx = static_cast<uint8_t>(entry.inlined_string_idx);
    }
  }
  if (field->type() == field->TYPE_MESSAGE ||
      field->type() == field->TYPE_GROUP) {
    name.append(options.use_direct_tcparser_table ? "t" : "d");
  }

  // The field implementation functions are prefixed by cardinality:
  //   `S` for optional or implicit fields.
  //   `R` for non-packed repeated.
  //   `P` for packed repeated.
  name.append(field->is_packed()               ? "P"
              : field->is_repeated()           ? "R"
              : field->real_containing_oneof() ? "O"
                                               : "S");

  // Append the tag length. Fast parsing only handles 1- or 2-byte tags.
  name.append(field->number() < 16 ? "1" : "2");

  info.func_name = std::move(name);
  info.aux_idx = aux_idx;
}

bool IsFieldEligibleForFastParsing(
    const TailCallTableInfo::FieldEntryInfo& entry,
    const TailCallTableInfo::OptionProvider& option_provider) {
  const auto* field = entry.field;
  const auto options = option_provider.GetForField(field);
  // Map, oneof, weak, and lazy fields are not handled on the fast path.
  if (field->is_map() || field->real_containing_oneof() ||
      field->options().weak() || options.is_implicitly_weak ||
      options.is_lazy || options.should_split) {
    return false;
  }

  // We will check for a valid auxiliary index range later. However, we might
  // want to change the value we check for inlined string fields.
  int aux_idx = entry.aux_idx;

  switch (field->type()) {
    case FieldDescriptor::TYPE_ENUM:
      // If enum values are not validated at parse time, then this field can be
      // handled on the fast path like an int32.
      if (cpp::HasPreservingUnknownEnumSemantics(field)) {
        break;
      }
      if (field->is_repeated() && field->is_packed()) {
        return false;
      }
      break;

      // Some bytes fields can be handled on fast path.
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES:
      if (field->options().ctype() != FieldOptions::STRING) {
        return false;
      }
      if (options.is_string_inlined) {
        GOOGLE_CHECK(!field->is_repeated());
        // For inlined strings, the donation state index is stored in the
        // `aux_idx` field of the fast parsing info. We need to check the range
        // of that value instead of the auxiliary index.
        aux_idx = entry.inlined_string_idx;
      }
      break;

    default:
      break;
  }

  if (cpp::HasHasbit(field)) {
    // The tailcall parser can only update the first 32 hasbits. Fields with
    // has-bits beyond the first 32 are handled by mini parsing/fallback.
    GOOGLE_CHECK_GE(entry.hasbit_idx, 0) << field->DebugString();
    if (entry.hasbit_idx >= 32) return false;
  }

  // If the field needs auxiliary data, then the aux index is needed. This
  // must fit in a uint8_t.
  if (aux_idx > std::numeric_limits<uint8_t>::max()) {
    return false;
  }

  // The largest tag that can be read by the tailcall parser is two bytes
  // when varint-coded. This allows 14 bits for the numeric tag value:
  //   byte 0   byte 1
  //   1nnnnttt 0nnnnnnn
  //    ^^^^^^^  ^^^^^^^
  if (field->number() >= 1 << 11) return false;

  return true;
}

absl::optional<uint32_t> GetEndGroupTag(const Descriptor* descriptor) {
  auto* parent = descriptor->containing_type();
  if (parent == nullptr) return absl::nullopt;
  for (int i = 0; i < parent->field_count(); ++i) {
    auto* field = parent->field(i);
    if (field->type() == field->TYPE_GROUP &&
        field->message_type() == descriptor) {
      return WireFormatLite::MakeTag(field->number(),
                                     WireFormatLite::WIRETYPE_END_GROUP);
    }
  }
  return absl::nullopt;
}

uint32_t RecodeTagForFastParsing(uint32_t tag) {
  GOOGLE_DCHECK_LE(tag, 0x3FFF);
  // Construct the varint-coded tag. If it is more than 7 bits, we need to
  // shift the high bits and add a continue bit.
  if (uint32_t hibits = tag & 0xFFFFFF80) {
    // hi = tag & ~0x7F
    // lo = tag & 0x7F
    // This shifts hi to the left by 1 to the next byte and sets the
    // continuation bit.
    tag = tag + hibits + 128;
  }
  return tag;
}

std::vector<TailCallTableInfo::FastFieldInfo> SplitFastFieldsForSize(
    absl::optional<uint32_t> end_group_tag,
    const std::vector<TailCallTableInfo::FieldEntryInfo>& field_entries,
    int table_size_log2,
    const TailCallTableInfo::OptionProvider& option_provider) {
  std::vector<TailCallTableInfo::FastFieldInfo> result(1 << table_size_log2);
  const uint32_t idx_mask = static_cast<uint32_t>(result.size() - 1);
  const auto tag_to_idx = [&](uint32_t tag) {
    // The field index is determined by the low bits of the field number, where
    // the table size determines the width of the mask. The largest table
    // supported is 32 entries. The parse loop uses these bits directly, so that
    // the dispatch does not require arithmetic:
    //        byte 0   byte 1
    //   tag: 1nnnnttt 0nnnnnnn
    //        ^^^^^
    //         idx (table_size_log2=5)
    // This means that any field number that does not fit in the lower 4 bits
    // will always have the top bit of its table index asserted.
    return (tag >> 3) & idx_mask;
  };

  if (end_group_tag.has_value() && (*end_group_tag >> 14) == 0) {
    // Fits in 1 or 2 varint bytes.
    const uint32_t tag = RecodeTagForFastParsing(*end_group_tag);
    const uint32_t fast_idx = tag_to_idx(tag);

    TailCallTableInfo::FastFieldInfo& info = result[fast_idx];
    info.func_name = "::_pbi::TcParser::FastEndG";
    info.func_name.append(*end_group_tag < 128 ? "1" : "2");
    info.coded_tag = tag;
    info.nonfield_info = *end_group_tag;
  }

  for (const auto& entry : field_entries) {
    if (!IsFieldEligibleForFastParsing(entry, option_provider)) {
      continue;
    }

    const auto* field = entry.field;
    const auto options = option_provider.GetForField(field);
    const uint32_t tag = RecodeTagForFastParsing(WireFormat::MakeTag(field));
    const uint32_t fast_idx = tag_to_idx(tag);

    TailCallTableInfo::FastFieldInfo& info = result[fast_idx];
    if (!info.func_name.empty()) {
      // This field entry is already filled.
      continue;
    }

    // Fill in this field's entry:
    GOOGLE_CHECK(info.func_name.empty()) << info.func_name;
    PopulateFastFieldEntry(entry, options, info);
    info.field = field;
    info.coded_tag = tag;
    // If this field does not have presence, then it can set an out-of-bounds
    // bit (tailcall parsing uses a uint64_t for hasbits, but only stores 32).
    info.hasbit_idx = cpp::HasHasbit(field) ? entry.hasbit_idx : 63;
  }
  return result;
}

// Filter out fields that will be handled by mini parsing.
std::vector<const FieldDescriptor*> FilterMiniParsedFields(
    const std::vector<const FieldDescriptor*>& fields,
    const TailCallTableInfo::OptionProvider& option_provider
) {
  std::vector<const FieldDescriptor*> generated_fallback_fields;

  for (const auto* field : fields) {
    auto options = option_provider.GetForField(field);

    bool handled = false;
    switch (field->type()) {
      case FieldDescriptor::TYPE_DOUBLE:
      case FieldDescriptor::TYPE_FLOAT:
      case FieldDescriptor::TYPE_FIXED32:
      case FieldDescriptor::TYPE_SFIXED32:
      case FieldDescriptor::TYPE_FIXED64:
      case FieldDescriptor::TYPE_SFIXED64:
      case FieldDescriptor::TYPE_BOOL:
      case FieldDescriptor::TYPE_UINT32:
      case FieldDescriptor::TYPE_SINT32:
      case FieldDescriptor::TYPE_INT32:
      case FieldDescriptor::TYPE_UINT64:
      case FieldDescriptor::TYPE_SINT64:
      case FieldDescriptor::TYPE_INT64:
        // These are handled by MiniParse, so we don't need any generated
        // fallback code.
        handled = true;
        break;

      case FieldDescriptor::TYPE_ENUM:
        if (field->is_repeated() &&
            !cpp::HasPreservingUnknownEnumSemantics(field)) {
          // TODO(b/206890171): handle packed repeated closed enums
          // Non-packed repeated can be handled using tables, but we still
          // need to generate fallback code for all repeated enums in order to
          // handle packed encoding. This is because of the lite/full split
          // when handling invalid enum values in a packed field.
          handled = false;
        } else {
          handled = true;
        }
        break;

      case FieldDescriptor::TYPE_BYTES:
      case FieldDescriptor::TYPE_STRING:
        if (options.is_string_inlined) {
          // TODO(b/198211897): support InilnedStringField.
          handled = false;
        } else {
          handled = true;
        }
        break;

      case FieldDescriptor::TYPE_MESSAGE:
      case FieldDescriptor::TYPE_GROUP:
        // TODO(b/210762816): support remaining field types.
        if (field->is_map() || field->options().weak() || options.is_lazy) {
          handled = false;
        } else {
          handled = true;
        }
        break;

      default:
        handled = false;
        break;
    }
    if (!handled) generated_fallback_fields.push_back(field);
  }

  return generated_fallback_fields;
}

// We only need field names for reporting UTF-8 parsing errors, so we only
// emit them for string fields with Utf8 transform specified.
absl::string_view FieldNameForTable(
    const TailCallTableInfo::FieldEntryInfo& entry) {
  const auto* field = entry.field;
  if (field->type() == FieldDescriptor::TYPE_STRING) {
    const uint16_t xform_val = entry.type_card & field_layout::kTvMask;

    switch (xform_val) {
      case field_layout::kTvUtf8:
      case field_layout::kTvUtf8Debug:
        return field->name();
    }
  }
  return "";
}

std::vector<uint8_t> GenerateFieldNames(
    const Descriptor* descriptor,
    const std::vector<TailCallTableInfo::FieldEntryInfo>& entries) {
  static constexpr int kMaxNameLength = 255;
  std::vector<uint8_t> out;

  bool found_needed_name = false;
  for (const auto& entry : entries) {
    if (!FieldNameForTable(entry).empty()) {
      found_needed_name = true;
      break;
    }
  }

  // No names needed. Omit the whole table.
  if (!found_needed_name) {
    return out;
  }

  // First, we output the size of each string, as an unsigned byte. The first
  // string is the message name.
  int count = 1;
  out.push_back(std::min(static_cast<int>(descriptor->full_name().size()),
                         kMaxNameLength));
  for (const auto& entry : entries) {
    out.push_back(FieldNameForTable(entry).size());
    ++count;
  }
  while (count & 7) {  // align to an 8-byte boundary
    out.push_back(0);
    ++count;
  }
  // The message name is stored at the beginning of the string
  std::string message_name = descriptor->full_name();
  if (message_name.size() > kMaxNameLength) {
    static constexpr int kNameHalfLength = (kMaxNameLength - 3) / 2;
    message_name = absl::StrCat(
        message_name.substr(0, kNameHalfLength), "...",
        message_name.substr(message_name.size() - kNameHalfLength));
  }
  out.insert(out.end(), message_name.begin(), message_name.end());
  // Then we output the actual field names
  for (const auto& entry : entries) {
    const auto& field_name = FieldNameForTable(entry);
    out.insert(out.end(), field_name.begin(), field_name.end());
  }

  return out;
}

TailCallTableInfo::NumToEntryTable MakeNumToEntryTable(
    const std::vector<const FieldDescriptor*>& field_descriptors) {
  TailCallTableInfo::NumToEntryTable num_to_entry_table;
  num_to_entry_table.skipmap32 = static_cast<uint32_t>(-1);

  // skip_entry_block is the current block of SkipEntries that we're
  // appending to.  cur_block_first_fnum is the number of the first
  // field represented by the block.
  uint16_t field_entry_index = 0;
  uint16_t N = field_descriptors.size();
  // First, handle field numbers 1-32, which affect only the initial
  // skipmap32 and don't generate additional skip-entry blocks.
  for (; field_entry_index != N; ++field_entry_index) {
    auto* field_descriptor = field_descriptors[field_entry_index];
    if (field_descriptor->number() > 32) break;
    auto skipmap32_index = field_descriptor->number() - 1;
    num_to_entry_table.skipmap32 -= 1 << skipmap32_index;
  }
  // If all the field numbers were less than or equal to 32, we will have
  // no further entries to process, and we are already done.
  if (field_entry_index == N) return num_to_entry_table;

  TailCallTableInfo::SkipEntryBlock* block = nullptr;
  bool start_new_block = true;
  // To determine sparseness, track the field number corresponding to
  // the start of the most recent skip entry.
  uint32_t last_skip_entry_start = 0;
  for (; field_entry_index != N; ++field_entry_index) {
    auto* field_descriptor = field_descriptors[field_entry_index];
    uint32_t fnum = static_cast<uint32_t>(field_descriptor->number());
    GOOGLE_CHECK_GT(fnum, last_skip_entry_start);
    if (start_new_block == false) {
      // If the next field number is within 15 of the last_skip_entry_start, we
      // continue writing just to that entry.  If it's between 16 and 31 more,
      // then we just extend the current block by one. If it's more than 31
      // more, we have to add empty skip entries in order to continue using the
      // existing block.  Obviously it's just 32 more, it doesn't make sense to
      // start a whole new block, since new blocks mean having to write out
      // their starting field number, which is 32 bits, as well as the size of
      // the additional block, which is 16... while an empty SkipEntry16 only
      // costs 32 bits.  So if it was 48 more, it's a slight space win; we save
      // 16 bits, but probably at the cost of slower run time.  We're choosing
      // 96 for now.
      if (fnum - last_skip_entry_start > 96) start_new_block = true;
    }
    if (start_new_block) {
      num_to_entry_table.blocks.push_back({fnum});
      block = &num_to_entry_table.blocks.back();
      start_new_block = false;
    }

    auto skip_entry_num = (fnum - block->first_fnum) / 16;
    auto skip_entry_index = (fnum - block->first_fnum) % 16;
    while (skip_entry_num >= block->entries.size())
      block->entries.push_back({0xFFFF, field_entry_index});
    block->entries[skip_entry_num].skipmap -= 1 << (skip_entry_index);

    last_skip_entry_start = fnum - skip_entry_index;
  }
  return num_to_entry_table;
}

uint16_t MakeTypeCardForField(
    const FieldDescriptor* field,
    const TailCallTableInfo::PerFieldOptions& options) {
  uint16_t type_card;
  namespace fl = internal::field_layout;
  if (internal::cpp::HasHasbit(field)) {
    type_card = fl::kFcOptional;
  } else if (field->is_repeated()) {
    type_card = fl::kFcRepeated;
  } else if (field->real_containing_oneof()) {
    type_card = fl::kFcOneof;
  } else {
    type_card = fl::kFcSingular;
  }

  // The rest of the type uses convenience aliases:
  switch (field->type()) {
    case FieldDescriptor::TYPE_DOUBLE:
      type_card |= field->is_repeated() && field->is_packed()
                       ? fl::kPackedDouble
                       : fl::kDouble;
      break;
    case FieldDescriptor::TYPE_FLOAT:
      type_card |= field->is_repeated() && field->is_packed() ? fl::kPackedFloat
                                                              : fl::kFloat;
      break;
    case FieldDescriptor::TYPE_FIXED32:
      type_card |= field->is_repeated() && field->is_packed()
                       ? fl::kPackedFixed32
                       : fl::kFixed32;
      break;
    case FieldDescriptor::TYPE_SFIXED32:
      type_card |= field->is_repeated() && field->is_packed()
                       ? fl::kPackedSFixed32
                       : fl::kSFixed32;
      break;
    case FieldDescriptor::TYPE_FIXED64:
      type_card |= field->is_repeated() && field->is_packed()
                       ? fl::kPackedFixed64
                       : fl::kFixed64;
      break;
    case FieldDescriptor::TYPE_SFIXED64:
      type_card |= field->is_repeated() && field->is_packed()
                       ? fl::kPackedSFixed64
                       : fl::kSFixed64;
      break;
    case FieldDescriptor::TYPE_BOOL:
      type_card |= field->is_repeated() && field->is_packed() ? fl::kPackedBool
                                                              : fl::kBool;
      break;
    case FieldDescriptor::TYPE_ENUM:
      if (internal::cpp::HasPreservingUnknownEnumSemantics(field)) {
        // No validation is required.
        type_card |= field->is_repeated() && field->is_packed()
                         ? fl::kPackedOpenEnum
                         : fl::kOpenEnum;
      } else {
        int16_t start;
        uint16_t size;
        if (GetEnumValidationRange(field->enum_type(), start, size)) {
          // Validation is done by range check (start/length in FieldAux).
          type_card |= field->is_repeated() && field->is_packed()
                           ? fl::kPackedEnumRange
                           : fl::kEnumRange;
        } else {
          // Validation uses the generated _IsValid function.
          type_card |= field->is_repeated() && field->is_packed()
                           ? fl::kPackedEnum
                           : fl::kEnum;
        }
      }
      break;
    case FieldDescriptor::TYPE_UINT32:
      type_card |= field->is_repeated() && field->is_packed()
                       ? fl::kPackedUInt32
                       : fl::kUInt32;
      break;
    case FieldDescriptor::TYPE_SINT32:
      type_card |= field->is_repeated() && field->is_packed()
                       ? fl::kPackedSInt32
                       : fl::kSInt32;
      break;
    case FieldDescriptor::TYPE_INT32:
      type_card |= field->is_repeated() && field->is_packed() ? fl::kPackedInt32
                                                              : fl::kInt32;
      break;
    case FieldDescriptor::TYPE_UINT64:
      type_card |= field->is_repeated() && field->is_packed()
                       ? fl::kPackedUInt64
                       : fl::kUInt64;
      break;
    case FieldDescriptor::TYPE_SINT64:
      type_card |= field->is_repeated() && field->is_packed()
                       ? fl::kPackedSInt64
                       : fl::kSInt64;
      break;
    case FieldDescriptor::TYPE_INT64:
      type_card |= field->is_repeated() && field->is_packed() ? fl::kPackedInt64
                                                              : fl::kInt64;
      break;

    case FieldDescriptor::TYPE_BYTES:
      type_card |= fl::kBytes;
      break;
    case FieldDescriptor::TYPE_STRING: {
      switch (internal::cpp::GetUtf8CheckMode(field, options.is_lite)) {
        case internal::cpp::Utf8CheckMode::kStrict:
          type_card |= fl::kUtf8String;
          break;
        case internal::cpp::Utf8CheckMode::kVerify:
          type_card |= fl::kRawString;
          break;
        case internal::cpp::Utf8CheckMode::kNone:
          type_card |= fl::kBytes;
          break;
      }
      break;
    }

    case FieldDescriptor::TYPE_GROUP:
      type_card |= 0 | fl::kMessage | fl::kRepGroup;
      if (options.is_implicitly_weak) {
        type_card |= fl::kTvWeakPtr;
      } else if (options.use_direct_tcparser_table) {
        type_card |= fl::kTvTable;
      } else {
        type_card |= fl::kTvDefault;
      }
      break;
    case FieldDescriptor::TYPE_MESSAGE:
      if (field->is_map()) {
        type_card |= fl::kMap;
      } else {
        type_card |= fl::kMessage;
        if (options.is_lazy) {
          type_card |= fl::kRepLazy;
        }

        if (options.is_implicitly_weak) {
          type_card |= fl::kTvWeakPtr;
        } else if (options.use_direct_tcparser_table) {
          type_card |= fl::kTvTable;
        } else {
          type_card |= fl::kTvDefault;
        }
      }
      break;
  }

  // Fill in extra information about string and bytes field representations.
  if (field->type() == FieldDescriptor::TYPE_BYTES ||
      field->type() == FieldDescriptor::TYPE_STRING) {
    if (field->is_repeated()) {
      type_card |= fl::kRepSString;
    } else {
      type_card |= fl::kRepAString;
    }
  }

  if (options.should_split) {
    type_card |= fl::kSplitTrue;
  }

  return type_card;
}

}  // namespace

TailCallTableInfo::TailCallTableInfo(
    const Descriptor* descriptor,
    const std::vector<const FieldDescriptor*>& ordered_fields,
    const OptionProvider& option_provider,
    const std::vector<int>& has_bit_indices,
    const std::vector<int>& inlined_string_indices) {
  // If this message has any inlined string fields, store the donation state
  // offset in the second auxiliary entry.
  if (!inlined_string_indices.empty()) {
    aux_entries.resize(1);  // pad if necessary
    aux_entries[0] = {kInlinedStringDonatedOffset};
  }

  // If this message is split, store the split pointer offset in the third
  // auxiliary entry.
  for (auto* field : ordered_fields) {
    if (option_provider.GetForField(field).should_split) {
      aux_entries.resize(3);  // pad if necessary
      aux_entries[1] = {kSplitOffset};
      aux_entries[2] = {kSplitSizeof};
      break;
    }
  }

  // Fill in mini table entries.
  for (const FieldDescriptor* field : ordered_fields) {
    auto options = option_provider.GetForField(field);
    field_entries.push_back(
        {field, internal::cpp ::HasHasbit(field)
                    ? has_bit_indices[static_cast<size_t>(field->index())]
                    : -1});
    auto& entry = field_entries.back();
    entry.type_card = MakeTypeCardForField(field, options);

    if (field->type() == FieldDescriptor::TYPE_MESSAGE ||
        field->type() == FieldDescriptor::TYPE_GROUP) {
      // Message-typed fields have a FieldAux with the default instance pointer.
      if (field->is_map()) {
        // TODO(b/205904770): generate aux entries for maps
      } else if (field->options().weak()) {
        // Don't generate anything for weak fields. They are handled by the
        // generated fallback.
      } else if (options.is_lazy) {
        // Lazy fields are handled by the generated fallback function.
      } else {
        field_entries.back().aux_idx = aux_entries.size();
        aux_entries.push_back({options.is_implicitly_weak ? kSubMessageWeak
                               : options.use_direct_tcparser_table
                                   ? kSubTable
                                   : kSubMessage,
                               {field}});
      }
    } else if (field->type() == FieldDescriptor::TYPE_ENUM &&
               !cpp::HasPreservingUnknownEnumSemantics(field)) {
      // Enum fields which preserve unknown values (proto3 behavior) are
      // effectively int32 fields with respect to parsing -- i.e., the value
      // does not need to be validated at parse time.
      //
      // Enum fields which do not preserve unknown values (proto2 behavior) use
      // a FieldAux to store validation information. If the enum values are
      // sequential (and within a range we can represent), then the FieldAux
      // entry represents the range using the minimum value (which must fit in
      // an int16_t) and count (a uint16_t). Otherwise, the entry holds a
      // pointer to the generated Name_IsValid function.

      entry.aux_idx = aux_entries.size();
      aux_entries.push_back({});
      auto& aux_entry = aux_entries.back();

      if (GetEnumValidationRange(field->enum_type(), aux_entry.enum_range.start,
                                 aux_entry.enum_range.size)) {
        aux_entry.type = kEnumRange;
      } else {
        aux_entry.type = kEnumValidator;
        aux_entry.field = field;
      }

    } else if ((field->type() == FieldDescriptor::TYPE_STRING ||
                field->type() == FieldDescriptor::TYPE_BYTES) &&
               options.is_string_inlined) {
      GOOGLE_CHECK(!field->is_repeated());
      // Inlined strings have an extra marker to represent their donation state.
      int idx = inlined_string_indices[static_cast<size_t>(field->index())];
      // For mini parsing, the donation state index is stored as an `offset`
      // auxiliary entry.
      entry.aux_idx = aux_entries.size();
      aux_entries.push_back({kNumericOffset});
      aux_entries.back().offset = idx;
      // For fast table parsing, the donation state index is stored instead of
      // the aux_idx (this will limit the range to 8 bits).
      entry.inlined_string_idx = idx;
    }
  }

  table_size_log2 = 0;  // fallback value
  int num_fast_fields = -1;
  auto end_group_tag = GetEndGroupTag(descriptor);
  for (int try_size_log2 : {0, 1, 2, 3, 4, 5}) {
    size_t try_size = 1 << try_size_log2;
    auto split_fields = SplitFastFieldsForSize(end_group_tag, field_entries,
                                               try_size_log2, option_provider);
    GOOGLE_CHECK_EQ(split_fields.size(), try_size);
    int try_num_fast_fields = 0;
    for (const auto& info : split_fields) {
      if (info.field != nullptr) ++try_num_fast_fields;
    }
    // Use this size if (and only if) it covers more fields.
    if (try_num_fast_fields > num_fast_fields) {
      fast_path_fields = std::move(split_fields);
      table_size_log2 = try_size_log2;
      num_fast_fields = try_num_fast_fields;
    }
    // The largest table we allow has the same number of entries as the
    // message has fields, rounded up to the next power of 2 (e.g., a message
    // with 5 fields can have a fast table of size 8). A larger table *might*
    // cover more fields in certain cases, but a larger table in that case
    // would have mostly empty entries; so, we cap the size to avoid
    // pathologically sparse tables.
    if (end_group_tag.has_value()) {
      // If this message uses group encoding, the tables are sometimes very
      // sparse because the fields in the group avoid using the same field
      // numbering as the parent message (even though currently, the proto
      // compiler allows the overlap, and there is no possible conflict.)
      // As such, this test produces a false negative as far as whether the
      // large table will be worth it.  So we disable the test in this case.
    } else {
      if (try_size > ordered_fields.size()) {
        break;
      }
    }
  }

  // Filter out fields that are handled by MiniParse. We don't need to generate
  // a fallback for these, which saves code size.
  fallback_fields = FilterMiniParsedFields(ordered_fields, option_provider
  );

  num_to_entry_table = MakeNumToEntryTable(ordered_fields);
  GOOGLE_CHECK_EQ(field_entries.size(), ordered_fields.size());
  field_name_data = GenerateFieldNames(descriptor, field_entries);

  // If there are no fallback fields, and at most one extension range, the
  // parser can use a generic fallback function. Otherwise, a message-specific
  // fallback routine is needed.
  use_generated_fallback =
      !fallback_fields.empty() || descriptor->extension_range_count() > 1;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
