// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/generated_message_tctable_gen.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
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
  ABSL_CHECK_GT(enum_type->value_count(), 0) << enum_type->DebugString();

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

enum class EnumRangeInfo {
  kNone,         // No contiguous range
  kContiguous,   // Has a contiguous range
  kContiguous0,  // Has a small contiguous range starting at 0
  kContiguous1,  // Has a small contiguous range starting at 1
};

// Returns enum validation range info, and sets `rmax_value` iff
// the returned range is a small range. `rmax_value` is guaranteed
// to remain unchanged if the enum range is not small.
EnumRangeInfo GetEnumRangeInfo(const FieldDescriptor* field,
                               uint8_t& rmax_value) {
  int16_t start;
  uint16_t size;
  if (!GetEnumValidationRange(field->enum_type(), start, size)) {
    return EnumRangeInfo::kNone;
  }
  int max_value = start + size - 1;
  if (max_value <= 127 && (start == 0 || start == 1)) {
    rmax_value = static_cast<uint8_t>(max_value);
    return start == 0 ? EnumRangeInfo::kContiguous0
                      : EnumRangeInfo::kContiguous1;
  }
  return EnumRangeInfo::kContiguous;
}

// options.lazy_opt might be on for fields that don't really support lazy, so we
// make sure we only use lazy rep for singular TYPE_MESSAGE fields.
// We can't trust the `lazy=true` annotation.
bool HasLazyRep(const FieldDescriptor* field,
                const TailCallTableInfo::PerFieldOptions options) {
  return field->type() == field->TYPE_MESSAGE && !field->is_repeated() &&
         options.lazy_opt != 0;
}

TailCallTableInfo::FastFieldInfo::Field MakeFastFieldEntry(
    const TailCallTableInfo::FieldEntryInfo& entry,
    const TailCallTableInfo::MessageOptions& message_options,
    const TailCallTableInfo::PerFieldOptions& options) {
  TailCallTableInfo::FastFieldInfo::Field info{};
#define PROTOBUF_PICK_FUNCTION(fn) \
  (field->number() < 16 ? TcParseFunction::fn##1 : TcParseFunction::fn##2)

#define PROTOBUF_PICK_SINGLE_FUNCTION(fn) PROTOBUF_PICK_FUNCTION(fn##S)

#define PROTOBUF_PICK_REPEATABLE_FUNCTION(fn)           \
  (field->is_repeated() ? PROTOBUF_PICK_FUNCTION(fn##R) \
                        : PROTOBUF_PICK_FUNCTION(fn##S))

#define PROTOBUF_PICK_PACKABLE_FUNCTION(fn)               \
  (field->is_packed()     ? PROTOBUF_PICK_FUNCTION(fn##P) \
   : field->is_repeated() ? PROTOBUF_PICK_FUNCTION(fn##R) \
                          : PROTOBUF_PICK_FUNCTION(fn##S))

#define PROTOBUF_PICK_STRING_FUNCTION(fn)                            \
  (field->cpp_string_type() == FieldDescriptor::CppStringType::kCord \
       ? PROTOBUF_PICK_FUNCTION(fn##cS)                              \
   : options.is_string_inlined ? PROTOBUF_PICK_FUNCTION(fn##iS)      \
                               : PROTOBUF_PICK_REPEATABLE_FUNCTION(fn))

  const FieldDescriptor* field = entry.field;
  info.aux_idx = static_cast<uint8_t>(entry.aux_idx);
  if (field->type() == FieldDescriptor::TYPE_BYTES ||
      field->type() == FieldDescriptor::TYPE_STRING) {
    if (options.is_string_inlined) {
      ABSL_CHECK(!field->is_repeated());
      info.aux_idx = static_cast<uint8_t>(entry.inlined_string_idx);
    }
  }

  TcParseFunction picked = TcParseFunction::kNone;
  switch (field->type()) {
    case FieldDescriptor::TYPE_BOOL:
      picked = PROTOBUF_PICK_PACKABLE_FUNCTION(kFastV8);
      break;
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_UINT32:
      picked = PROTOBUF_PICK_PACKABLE_FUNCTION(kFastV32);
      break;
    case FieldDescriptor::TYPE_SINT32:
      picked = PROTOBUF_PICK_PACKABLE_FUNCTION(kFastZ32);
      break;
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_UINT64:
      picked = PROTOBUF_PICK_PACKABLE_FUNCTION(kFastV64);
      break;
    case FieldDescriptor::TYPE_SINT64:
      picked = PROTOBUF_PICK_PACKABLE_FUNCTION(kFastZ64);
      break;
    case FieldDescriptor::TYPE_FLOAT:
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_SFIXED32:
      picked = PROTOBUF_PICK_PACKABLE_FUNCTION(kFastF32);
      break;
    case FieldDescriptor::TYPE_DOUBLE:
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_SFIXED64:
      picked = PROTOBUF_PICK_PACKABLE_FUNCTION(kFastF64);
      break;
    case FieldDescriptor::TYPE_ENUM:
      if (cpp::HasPreservingUnknownEnumSemantics(field)) {
        picked = PROTOBUF_PICK_PACKABLE_FUNCTION(kFastV32);
      } else {
        switch (GetEnumRangeInfo(field, info.aux_idx)) {
          case EnumRangeInfo::kNone:
            picked = PROTOBUF_PICK_PACKABLE_FUNCTION(kFastEv);
            break;
          case EnumRangeInfo::kContiguous:
            picked = PROTOBUF_PICK_PACKABLE_FUNCTION(kFastEr);
            break;
          case EnumRangeInfo::kContiguous0:
            picked = PROTOBUF_PICK_PACKABLE_FUNCTION(kFastEr0);
            break;
          case EnumRangeInfo::kContiguous1:
            picked = PROTOBUF_PICK_PACKABLE_FUNCTION(kFastEr1);
            break;
        }
      }
      break;
    case FieldDescriptor::TYPE_BYTES:
      picked = PROTOBUF_PICK_STRING_FUNCTION(kFastB);
      break;
    case FieldDescriptor::TYPE_STRING:
      switch (internal::cpp::GetUtf8CheckMode(field, message_options.is_lite)) {
        case internal::cpp::Utf8CheckMode::kStrict:
          picked = PROTOBUF_PICK_STRING_FUNCTION(kFastU);
          break;
        case internal::cpp::Utf8CheckMode::kVerify:
          picked = PROTOBUF_PICK_STRING_FUNCTION(kFastS);
          break;
        case internal::cpp::Utf8CheckMode::kNone:
          picked = PROTOBUF_PICK_STRING_FUNCTION(kFastB);
          break;
      }
      break;
    case FieldDescriptor::TYPE_MESSAGE:
      picked =
          (HasLazyRep(field, options) ? PROTOBUF_PICK_SINGLE_FUNCTION(kFastMl)
           : options.use_direct_tcparser_table
               ? PROTOBUF_PICK_REPEATABLE_FUNCTION(kFastMt)
               : PROTOBUF_PICK_REPEATABLE_FUNCTION(kFastMd));
      break;
    case FieldDescriptor::TYPE_GROUP:
      picked = (options.use_direct_tcparser_table
                    ? PROTOBUF_PICK_REPEATABLE_FUNCTION(kFastGt)
                    : PROTOBUF_PICK_REPEATABLE_FUNCTION(kFastGd));
      break;
  }

  ABSL_CHECK(picked != TcParseFunction::kNone);
  info.func = picked;
  return info;

#undef PROTOBUF_PICK_FUNCTION
#undef PROTOBUF_PICK_SINGLE_FUNCTION
#undef PROTOBUF_PICK_REPEATABLE_FUNCTION
#undef PROTOBUF_PICK_PACKABLE_FUNCTION
#undef PROTOBUF_PICK_STRING_FUNCTION
}

bool IsFieldEligibleForFastParsing(
    const TailCallTableInfo::FieldEntryInfo& entry,
    const TailCallTableInfo::MessageOptions& message_options,
    const TailCallTableInfo::OptionProvider& option_provider) {
  const auto* field = entry.field;
  const auto options = option_provider.GetForField(field);
  ABSL_CHECK(!field->options().weak());
  // Map, oneof, weak, and split fields are not handled on the fast path.
  if (field->is_map() || field->real_containing_oneof() ||
      options.is_implicitly_weak || options.should_split) {
    return false;
  }

  if (HasLazyRep(field, options) && !message_options.uses_codegen) {
    // Can't use TDP on lazy fields if we can't do codegen.
    return false;
  }

  if (HasLazyRep(field, options) && options.lazy_opt == field_layout::kTvLazy) {
    // We only support eagerly verified lazy fields in the fast path.
    return false;
  }

  // We will check for a valid auxiliary index range later. However, we might
  // want to change the value we check for inlined string fields.
  int aux_idx = entry.aux_idx;

  switch (field->type()) {
      // Some bytes fields can be handled on fast path.
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES: {
      auto ctype = field->cpp_string_type();
      if (ctype == FieldDescriptor::CppStringType::kString ||
          ctype == FieldDescriptor::CppStringType::kView) {
        // strings are fine...
      } else if (ctype == FieldDescriptor::CppStringType::kCord) {
        // Cords are worth putting into the fast table, if they're not repeated
        if (field->is_repeated()) return false;
      } else {
        return false;
      }
      if (options.is_string_inlined) {
        ABSL_CHECK(!field->is_repeated());
        // For inlined strings, the donation state index is stored in the
        // `aux_idx` field of the fast parsing info. We need to check the range
        // of that value instead of the auxiliary index.
        aux_idx = entry.inlined_string_idx;
      }
      break;
    }

    case FieldDescriptor::TYPE_ENUM: {
      uint8_t rmax_value;
      if (!message_options.uses_codegen &&
          GetEnumRangeInfo(field, rmax_value) == EnumRangeInfo::kNone) {
        // We can't use fast parsing for these entries because we can't specify
        // the validator.
        // TODO: Implement a fast parser for these enums.
        return false;
      }
      break;
    }

    default:
      break;
  }

  if (cpp::HasHasbit(field)) {
    // The tailcall parser can only update the first 32 hasbits. Fields with
    // has-bits beyond the first 32 are handled by mini parsing/fallback.
    ABSL_CHECK_GE(entry.hasbit_idx, 0) << field->DebugString();
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
  ABSL_DCHECK_LE(tag, 0x3FFF);
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
    const TailCallTableInfo::MessageOptions& message_options,
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
    info.data = TailCallTableInfo::FastFieldInfo::NonField{
        *end_group_tag < 128 ? TcParseFunction::kFastEndG1
                             : TcParseFunction::kFastEndG2,
        static_cast<uint16_t>(tag),
        static_cast<uint16_t>(*end_group_tag),
    };
  }

  for (const auto& entry : field_entries) {
    if (!IsFieldEligibleForFastParsing(entry, message_options,
                                       option_provider)) {
      continue;
    }

    const auto* field = entry.field;
    const auto options = option_provider.GetForField(field);
    const uint32_t tag = RecodeTagForFastParsing(WireFormat::MakeTag(field));
    const uint32_t fast_idx = tag_to_idx(tag);

    TailCallTableInfo::FastFieldInfo& info = result[fast_idx];
    if (info.AsNonField() != nullptr) {
      // Null field means END_GROUP which is guaranteed to be present.
      continue;
    }
    if (auto* as_field = info.AsField()) {
      // This field entry is already filled. Skip if previous entry is more
      // likely present.
      const auto prev_options = option_provider.GetForField(as_field->field);
      if (prev_options.presence_probability >= options.presence_probability) {
        continue;
      }
    }

    // We reset the entry even if it had a field already.
    // Fill in this field's entry:
    auto& fast_field =
        info.data.emplace<TailCallTableInfo::FastFieldInfo::Field>(
            MakeFastFieldEntry(entry, message_options, options));
    fast_field.field = field;
    fast_field.coded_tag = tag;
    // If this field does not have presence, then it can set an out-of-bounds
    // bit (tailcall parsing uses a uint64_t for hasbits, but only stores 32).
    fast_field.hasbit_idx = cpp::HasHasbit(field) ? entry.hasbit_idx : 63;
  }
  return result;
}

// We only need field names for reporting UTF-8 parsing errors, so we only
// emit them for string fields with Utf8 transform specified.
bool NeedsFieldNameForTable(const FieldDescriptor* field, bool is_lite) {
  if (cpp::GetUtf8CheckMode(field, is_lite) == cpp::Utf8CheckMode::kNone)
    return false;
  return field->type() == FieldDescriptor::TYPE_STRING ||
         (field->is_map() && (field->message_type()->map_key()->type() ==
                                  FieldDescriptor::TYPE_STRING ||
                              field->message_type()->map_value()->type() ==
                                  FieldDescriptor::TYPE_STRING));
}

absl::string_view FieldNameForTable(
    const TailCallTableInfo::FieldEntryInfo& entry,
    const TailCallTableInfo::MessageOptions& message_options) {
  if (NeedsFieldNameForTable(entry.field, message_options.is_lite)) {
    return entry.field->name();
  }
  return "";
}

std::vector<uint8_t> GenerateFieldNames(
    const Descriptor* descriptor,
    const std::vector<TailCallTableInfo::FieldEntryInfo>& entries,
    const TailCallTableInfo::MessageOptions& message_options,
    const TailCallTableInfo::OptionProvider& option_provider) {
  static constexpr int kMaxNameLength = 255;
  std::vector<uint8_t> out;

  std::vector<absl::string_view> names;
  bool found_needed_name = false;
  for (const auto& entry : entries) {
    names.push_back(FieldNameForTable(entry, message_options));
    if (!names.back().empty()) found_needed_name = true;
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
  for (auto field_name : names) {
    out.push_back(field_name.size());
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
  for (auto field_name : names) {
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
    ABSL_CHECK_GT(fnum, last_skip_entry_start);
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
    const TailCallTableInfo::MessageOptions& message_options,
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
      switch (internal::cpp::GetUtf8CheckMode(field, message_options.is_lite)) {
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
        if (HasLazyRep(field, options)) {
          ABSL_CHECK(options.lazy_opt == field_layout::kTvEager ||
                     options.lazy_opt == field_layout::kTvLazy);
          type_card |= +fl::kRepLazy | options.lazy_opt;
        } else {
          if (options.is_implicitly_weak) {
            type_card |= fl::kTvWeakPtr;
          } else if (options.use_direct_tcparser_table) {
            type_card |= fl::kTvTable;
          } else {
            type_card |= fl::kTvDefault;
          }
        }
      }
      break;
  }

  // Fill in extra information about string and bytes field representations.
  if (field->type() == FieldDescriptor::TYPE_BYTES ||
      field->type() == FieldDescriptor::TYPE_STRING) {
    switch (field->cpp_string_type()) {
      case FieldDescriptor::CppStringType::kCord:
        // `Cord` is always used, even for repeated fields.
        type_card |= fl::kRepCord;
        break;
      case FieldDescriptor::CppStringType::kString:
        if (field->is_repeated()) {
          // A repeated string field uses RepeatedPtrField<std::string>
          // (unless it has a ctype option; see above).
          type_card |= fl::kRepSString;
        } else {
          // Otherwise, non-repeated string fields use ArenaStringPtr.
          type_card |= fl::kRepAString;
        }
        break;
      case FieldDescriptor::CppStringType::kView:
        break;
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
    const MessageOptions& message_options,
    const OptionProvider& option_provider,
    const std::vector<int>& has_bit_indices,
    const std::vector<int>& inlined_string_indices) {
  ABSL_DCHECK(std::is_sorted(ordered_fields.begin(), ordered_fields.end(),
                             [](const auto* lhs, const auto* rhs) {
                               return lhs->number() < rhs->number();
                             }));
  // If this message has any inlined string fields, store the donation state
  // offset in the first auxiliary entry, which is kInlinedStringAuxIdx.
  if (!inlined_string_indices.empty()) {
    aux_entries.resize(kInlinedStringAuxIdx + 1);  // Allocate our slot
    aux_entries[kInlinedStringAuxIdx] = {kInlinedStringDonatedOffset};
  }

  // If this message is split, store the split pointer offset in the second
  // and third auxiliary entries, which are kSplitOffsetAuxIdx and
  // kSplitSizeAuxIdx.
  for (auto* field : ordered_fields) {
    if (option_provider.GetForField(field).should_split) {
      static_assert(kSplitOffsetAuxIdx + 1 == kSplitSizeAuxIdx, "");
      aux_entries.resize(kSplitSizeAuxIdx + 1);  // Allocate our 2 slots
      aux_entries[kSplitOffsetAuxIdx] = {kSplitOffset};
      aux_entries[kSplitSizeAuxIdx] = {kSplitSizeof};
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
    entry.type_card = MakeTypeCardForField(field, message_options, options);

    if (field->type() == FieldDescriptor::TYPE_MESSAGE ||
        field->type() == FieldDescriptor::TYPE_GROUP) {
      // Message-typed fields have a FieldAux with the default instance pointer.
      if (field->is_map()) {
        field_entries.back().aux_idx = aux_entries.size();
        aux_entries.push_back({kMapAuxInfo, {field}});
        if (message_options.uses_codegen) {
          // If we don't use codegen we can't add these.
          auto* map_value = field->message_type()->map_value();
          if (auto* sub = map_value->message_type()) {
            aux_entries.push_back({kCreateInArena});
            aux_entries.back().desc = sub;
          } else if (map_value->type() == FieldDescriptor::TYPE_ENUM &&
                     !cpp::HasPreservingUnknownEnumSemantics(map_value)) {
            aux_entries.push_back({kEnumValidator, {map_value}});
          }
        }
      } else if (HasLazyRep(field, options)) {
        if (message_options.uses_codegen) {
          field_entries.back().aux_idx = aux_entries.size();
          aux_entries.push_back({kSubMessage, {field}});
          if (options.lazy_opt == field_layout::kTvEager) {
            aux_entries.push_back({kMessageVerifyFunc, {field}});
          } else {
            aux_entries.push_back({kNothing});
          }
        } else {
          field_entries.back().aux_idx =
              TcParseTableBase::FieldEntry::kNoAuxIdx;
        }
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
      ABSL_CHECK(!field->is_repeated());
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
    auto split_fields =
        SplitFastFieldsForSize(end_group_tag, field_entries, try_size_log2,
                               message_options, option_provider);
    ABSL_CHECK_EQ(split_fields.size(), try_size);
    int try_num_fast_fields = 0;
    for (const auto& info : split_fields) {
      if (info.is_empty()) continue;

      if (info.AsNonField() != nullptr) {
        ++try_num_fast_fields;
        continue;
      }

      auto* as_field = info.AsField();
      const auto option = option_provider.GetForField(as_field->field);
      // 0.05 was selected based on load tests where 0.1 and 0.01 were also
      // evaluated and worse.
      constexpr float kMinPresence = 0.05f;
      if (option.presence_probability >= kMinPresence) {
        ++try_num_fast_fields;
      }
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

  num_to_entry_table = MakeNumToEntryTable(ordered_fields);
  ABSL_CHECK_EQ(field_entries.size(), ordered_fields.size());
  field_name_data = GenerateFieldNames(descriptor, field_entries,
                                       message_options, option_provider);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
