// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/generated_message_tctable_gen.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <vector>

#include "absl/container/fixed_array.h"
#include "absl/log/absl_check.h"
#include "absl/numeric/bits.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/generated_message_tctable_decl.h"
#include "google/protobuf/generated_message_tctable_impl.h"
#include "google/protobuf/port.h"
#include "google/protobuf/wire_format.h"
#include "google/protobuf/wire_format_lite.h"

// Must come last:
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

namespace {

bool TreatEnumAsInt(const FieldDescriptor* field) {
  return cpp::HasPreservingUnknownEnumSemantics(field) ||
         // For legacy reasons, MapEntry mapped_type enum fields are handled as
         // open always. The validation happens elsewhere.
         (field->enum_type() != nullptr &&
          field->containing_type() != nullptr &&
          field->containing_type()->map_value() == field);
}

bool GetEnumValidationRangeSlow(const EnumDescriptor* enum_type, int32_t& first,
                                int32_t& last) {
  const auto val = [&](int index) { return enum_type->value(index)->number(); };
  int min = val(0);
  int max = min;

  for (int i = 1, N = static_cast<int>(enum_type->value_count()); i < N; ++i) {
    min = std::min(min, val(i));
    max = std::max(max, val(i));
  }

  // int64 because max-min can overflow int.
  int64_t range = static_cast<int64_t>(max) - static_cast<int64_t>(min) + 1;

  if (enum_type->value_count() < range) {
    // There are not enough values to fill the range. Exit early.
    return false;
  }

  first = min;
  last = max;

  absl::FixedArray<uint64_t> array((range + 63) / 64);
  array.fill(0);

  int unique_count = 0;
  for (int i = 0, N = static_cast<int>(enum_type->value_count()); i < N; ++i) {
    size_t index = val(i) - min;
    uint64_t& v = array[index / 64];
    size_t bit_pos = index % 64;
    unique_count += (v & (uint64_t{1} << bit_pos)) == 0;
    v |= uint64_t{1} << bit_pos;
  }

  return unique_count == range;
}

bool GetEnumValidationRange(const EnumDescriptor* enum_type, int32_t& first,
                            int32_t& last) {
  if (!IsEnumFullySequential(enum_type)) {
    // Maybe the labels are not sequential in declaration order, but the values
    // could still be a dense range. Try the slower approach.
    return GetEnumValidationRangeSlow(enum_type, first, last);
  }

  first = enum_type->value(0)->number();
  last = enum_type->value(enum_type->value_count() - 1)->number();
  return true;
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
  int32_t first;
  int32_t last;
  if (!GetEnumValidationRange(field->enum_type(), first, last)) {
    return EnumRangeInfo::kNone;
  }
  if (last <= 127 && (first == 0 || first == 1)) {
    rmax_value = static_cast<uint8_t>(last);
    return first == 0 ? EnumRangeInfo::kContiguous0
                      : EnumRangeInfo::kContiguous1;
  }
  return EnumRangeInfo::kContiguous;
}

// options.lazy_opt might be on for fields that don't really support lazy, so we
// make sure we only use lazy rep for singular TYPE_MESSAGE fields.
// We can't trust the `lazy=true` annotation.
bool HasLazyRep(const FieldDescriptor* field,
                const TailCallTableInfo::FieldOptions& options) {
  return field->type() == field->TYPE_MESSAGE && !field->is_repeated() &&
         options.lazy_opt != 0;
}

TailCallTableInfo::FastFieldInfo::Field MakeFastFieldEntry(
    const TailCallTableInfo::FieldEntryInfo& entry,
    const TailCallTableInfo::FieldOptions& options,
    const TailCallTableInfo::MessageOptions& message_options) {
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

#define PROTOBUF_PICK_STRING_FUNCTION(fn)                                 \
  (field->cpp_string_type() == FieldDescriptor::CppStringType::kCord      \
       ? PROTOBUF_PICK_FUNCTION(fn##cS)                                   \
   : field->cpp_string_type() == FieldDescriptor::CppStringType::kView && \
           options.use_micro_string                                       \
       ? PROTOBUF_PICK_FUNCTION(fn##mS)                                   \
   : options.is_string_inlined ? PROTOBUF_PICK_FUNCTION(fn##iS)           \
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
      if (TreatEnumAsInt(field)) {
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
      switch (entry.utf8_check_mode) {
        case cpp::Utf8CheckMode::kStrict:
          picked = PROTOBUF_PICK_STRING_FUNCTION(kFastU);
          break;
        case cpp::Utf8CheckMode::kVerify:
          picked = PROTOBUF_PICK_STRING_FUNCTION(kFastS);
          break;
        case cpp::Utf8CheckMode::kNone:
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
  info.presence_probability = options.presence_probability;
  return info;

#undef PROTOBUF_PICK_FUNCTION
#undef PROTOBUF_PICK_SINGLE_FUNCTION
#undef PROTOBUF_PICK_REPEATABLE_FUNCTION
#undef PROTOBUF_PICK_PACKABLE_FUNCTION
#undef PROTOBUF_PICK_STRING_FUNCTION
}

bool IsFieldEligibleForFastParsing(
    const TailCallTableInfo::FieldEntryInfo& entry,
    const TailCallTableInfo::FieldOptions& options,
    const TailCallTableInfo::MessageOptions& message_options) {
  const auto* field = entry.field;
  // Map, oneof, weak, and split fields are not handled on the fast path.
  if (!IsFieldTypeEligibleForFastParsing(field) || options.is_implicitly_weak ||
      options.should_split) {
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
      if (options.is_string_inlined) {
        ABSL_CHECK(!field->is_repeated());
        // For inlined strings, the donation state index is stored in the
        // `aux_idx` field of the fast parsing info. We need to check the range
        // of that value instead of the auxiliary index.
        aux_idx = entry.inlined_string_idx;
      }
      break;
    }

    default:
      break;
  }

  if (entry.hasbit_idx > TailCallTableInfo::kMaxFastFieldHasbitIndex)
    return false;

  // If the field needs auxiliary data, then the aux index is needed. This
  // must fit in a uint8_t.
  if (aux_idx > std::numeric_limits<uint8_t>::max()) {
    return false;
  }

  return true;
}

void PopulateFastFields(
    std::optional<uint32_t> end_group_tag,
    const std::vector<TailCallTableInfo::FieldEntryInfo>& field_entries,
    const TailCallTableInfo::MessageOptions& message_options,
    absl::Span<const TailCallTableInfo::FieldOptions> fields,
    absl::Span<TailCallTableInfo::FastFieldInfo> result,
    uint32_t& important_fields) {
  if (end_group_tag.has_value() && (*end_group_tag >> 14) == 0) {
    // Fits in 1 or 2 varint bytes.
    const uint32_t tag =
        TcParseTableBase::RecodeTagForFastParsing(*end_group_tag);
    const uint32_t fast_idx = TcParseTableBase::TagToIdx(tag, result.size());

    TailCallTableInfo::FastFieldInfo& info = result[fast_idx];
    info.data = TailCallTableInfo::FastFieldInfo::NonField{
        *end_group_tag < 128 ? TcParseFunction::kFastEndG1
                             : TcParseFunction::kFastEndG2,
        static_cast<uint16_t>(tag),
        static_cast<uint16_t>(*end_group_tag),
    };
    important_fields |= uint32_t{1} << fast_idx;
  }

  for (size_t i = 0; i < field_entries.size(); ++i) {
    const auto& entry = field_entries[i];
    const auto& options = fields[i];
    if (!IsFieldEligibleForFastParsing(entry, options, message_options)) {
      continue;
    }

    const auto* field = entry.field;
    const uint32_t tag = GetRecodedTagForFastParsing(field);
    const uint32_t fast_idx = TcParseTableBase::TagToIdx(tag, result.size());

    TailCallTableInfo::FastFieldInfo& info = result[fast_idx];
    if (info.AsNonField() != nullptr) {
      // Right now non-field means END_GROUP which is guaranteed to be present.
      continue;
    }
    if (auto* as_field = info.AsField()) {
      // This field entry is already filled. Skip if previous entry is more
      // likely present.
      if (as_field->presence_probability >= options.presence_probability) {
        continue;
      }
    }

    // We reset the entry even if it had a field already.
    // Fill in this field's entry:
    auto& fast_field =
        info.data.emplace<TailCallTableInfo::FastFieldInfo::Field>(
            MakeFastFieldEntry(entry, options, message_options));
    fast_field.field = field;
    fast_field.coded_tag = tag;
    // If this field does not have presence, then it can set an out-of-bounds
    // bit (tailcall parsing uses a uint64_t for hasbits, but only stores 32).
    fast_field.hasbit_idx = entry.hasbit_idx >= 0 ? entry.hasbit_idx : 63;
    // 0.05 was selected based on load tests where 0.1 and 0.01 were also
    // evaluated and worse.
    constexpr float kMinPresence = 0.05f;
    important_fields |= uint32_t{options.presence_probability >= kMinPresence}
                        << fast_idx;
  }
}

std::vector<uint8_t> GenerateFieldNames(
    const Descriptor* descriptor,
    const absl::Span<const TailCallTableInfo::FieldEntryInfo> entries,
    const TailCallTableInfo::MessageOptions& message_options,
    absl::Span<const TailCallTableInfo::FieldOptions> fields) {
  static constexpr size_t kMaxNameLength = 255;

  size_t field_name_total_size = 0;
  const auto for_each_field_name = [&](auto with_name, auto no_name) {
    for (const auto& entry : entries) {
      // We only need field names for reporting UTF-8 parsing errors, so we only
      // emit them for string fields with Utf8 transform specified.
      if (entry.utf8_check_mode != cpp::Utf8CheckMode::kNone) {
        with_name(absl::string_view(entry.field->name()));
      } else {
        no_name();
      }
    }
  };

  for_each_field_name([&](auto name) { field_name_total_size += name.size(); },
                      [] {});

  // No names needed. Omit the whole table.
  if (field_name_total_size == 0) {
    return {};
  }

  const absl::string_view message_name = descriptor->full_name();
  uint8_t message_name_size =
      static_cast<uint8_t>(std::min(message_name.size(), kMaxNameLength));
  size_t total_byte_size =
      ((/* message */ 1 + /* fields */ entries.size() + /* round up */ 7) &
       ~7) +
      message_name_size + field_name_total_size;

  std::vector<uint8_t> out_vec(total_byte_size, uint8_t{0});
  uint8_t* out_it = out_vec.data();

  // First, we output the size of each string, as an unsigned byte. The first
  // string is the message name.
  int count = 1;
  *out_it++ = message_name_size;
  for_each_field_name(
      [&](auto name) {
        *out_it++ = static_cast<uint8_t>(name.size());
        ++count;
      },
      [&] {
        ++out_it;
        ++count;
      });
  // align to an 8-byte boundary
  out_it += -count & 7;

  const auto append = [&](absl::string_view str) {
    if (!str.empty()) {
      memcpy(out_it, str.data(), str.size());
      out_it += str.size();
    }
  };

  // The message name is stored at the beginning of the string
  if (message_name.size() > kMaxNameLength) {
    static constexpr int kNameHalfLength = (kMaxNameLength - 3) / 2;
    append(message_name.substr(0, kNameHalfLength));
    append("...");
    append(message_name.substr(message_name.size() - kNameHalfLength));
  } else {
    append(message_name);
  }
  // Then we output the actual field names
  for_each_field_name([&](auto name) { append(name); }, [] {});

  return out_vec;
}

TailCallTableInfo::NumToEntryTable MakeNumToEntryTable(
    absl::Span<const TailCallTableInfo::FieldOptions> ordered_fields) {
  TailCallTableInfo::NumToEntryTable num_to_entry_table;
  num_to_entry_table.skipmap32 = static_cast<uint32_t>(-1);

  // skip_entry_block is the current block of SkipEntries that we're
  // appending to.  cur_block_first_fnum is the number of the first
  // field represented by the block.
  uint16_t field_entry_index = 0;
  uint16_t N = ordered_fields.size();
  // First, handle field numbers 1-32, which affect only the initial
  // skipmap32 and don't generate additional skip-entry blocks.
  for (; field_entry_index != N; ++field_entry_index) {
    auto* field_descriptor = ordered_fields[field_entry_index].field;
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
    auto* field_descriptor = ordered_fields[field_entry_index].field;
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
    const FieldDescriptor* field, bool has_hasbit,
    const TailCallTableInfo::FieldOptions& options,
    cpp::Utf8CheckMode utf8_check_mode) {
  uint16_t type_card;
  namespace fl = internal::field_layout;
  if (field->is_repeated()) {
    type_card = fl::kFcRepeated;
  } else if (has_hasbit) {
    type_card = fl::kFcOptional;
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
      if (TreatEnumAsInt(field)) {
        // No validation is required.
        type_card |= field->is_repeated() && field->is_packed()
                         ? fl::kPackedOpenEnum
                         : fl::kOpenEnum;
      } else {
        int32_t first;
        int32_t last;
        if (GetEnumValidationRange(field->enum_type(), first, last)) {
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
      switch (utf8_check_mode) {
        case cpp::Utf8CheckMode::kStrict:
          type_card |= fl::kUtf8String;
          break;
        case cpp::Utf8CheckMode::kVerify:
          type_card |= fl::kRawString;
          break;
        case cpp::Utf8CheckMode::kNone:
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
      case FieldDescriptor::CppStringType::kView:
      case FieldDescriptor::CppStringType::kString:
        if (field->is_repeated()) {
          // A repeated string field uses RepeatedPtrField<std::string>
          // (unless it has a ctype option; see above).
          type_card |= fl::kRepSString;
        } else {
          // Otherwise, non-repeated string fields use ArenaStringPtr.
          type_card |=
              options.use_micro_string ? fl::kRepMString : fl::kRepAString;
        }
        break;
    }
  }

  if (options.should_split) {
    type_card |= fl::kSplitTrue;
  }

  return type_card;
}

bool HasWeakFields(const Descriptor* descriptor) {
  for (int i = 0; i < descriptor->field_count(); i++) {
    if (descriptor->field(i)->options().weak()) {
      return true;
    }
  }
  return false;
}

}  // namespace

uint32_t GetRecodedTagForFastParsing(const FieldDescriptor* field) {
  return internal::TcParseTableBase::RecodeTagForFastParsing(
      internal::WireFormat::MakeTag(field));
}

std::optional<uint32_t> GetEndGroupTag(const Descriptor* descriptor) {
  auto* parent = descriptor->containing_type();
  if (parent == nullptr) return std::nullopt;
  for (int i = 0; i < parent->field_count(); ++i) {
    auto* field = parent->field(i);
    if (field->type() == field->TYPE_GROUP &&
        field->message_type() == descriptor) {
      return WireFormatLite::MakeTag(field->number(),
                                     WireFormatLite::WIRETYPE_END_GROUP);
    }
  }
  return std::nullopt;
}

uint32_t FastParseTableSize(size_t num_fields,
                            std::optional<uint32_t> end_group_tag) {
  return end_group_tag.has_value()
             ? TcParseTableBase::kMaxFastFields
             : std::max(size_t{1}, std::min(TcParseTableBase::kMaxFastFields,
                                            absl::bit_ceil(num_fields + 1)));
}

bool IsFieldTypeEligibleForFastParsing(const FieldDescriptor* field) {
  // Map, oneof, weak, and split fields are not handled on the fast path.
  if (field->is_map() || field->real_containing_oneof() ||
      field->options().weak()) {
    return false;
  }

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
      break;
    }

    default:
      break;
  }

  // The largest tag that can be read by the tailcall parser is two bytes
  // when varint-coded. This allows 14 bits for the numeric tag value:
  //   byte 0   byte 1
  //   1nnnnttt 0nnnnnnn
  //    ^^^^^^^  ^^^^^^^
  if (field->number() >= 1 << 11) return false;

  return true;
}

std::vector<TailCallTableInfo::FieldEntryInfo>
TailCallTableInfo::BuildFieldEntries(
    const Descriptor* descriptor, const MessageOptions& message_options,
    absl::Span<const FieldOptions> ordered_fields,
    std::vector<TailCallTableInfo::AuxEntry>& aux_entries) {
  std::vector<FieldEntryInfo> field_entries;
  field_entries.reserve(ordered_fields.size());

  const auto is_non_cold = [](const FieldOptions& options) {
    return options.presence_probability >= 0.005;
  };
  size_t num_non_cold_subtables = 0;
  // We found that clustering non-cold subtables to the top of aux_entries
  // achieves the best load tests results than other strategies (e.g.,
  // clustering all non-cold entries).
  const auto is_non_cold_subtable = [&](const FieldOptions& options) {
    auto* field = options.field;
    // In the following code where we assign kSubTable to aux entries, only
    // the following typed fields are supported.
    return (field->type() == FieldDescriptor::TYPE_MESSAGE ||
            field->type() == FieldDescriptor::TYPE_GROUP) &&
           !field->is_map() && !field->options().weak() &&
           !HasLazyRep(field, options) && !options.is_implicitly_weak &&
           options.use_direct_tcparser_table && is_non_cold(options);
  };
  for (const FieldOptions& options : ordered_fields) {
    if (is_non_cold_subtable(options)) {
      num_non_cold_subtables++;
    }
  }

  size_t subtable_aux_idx_begin = aux_entries.size();
  size_t subtable_aux_idx = aux_entries.size();
  aux_entries.resize(aux_entries.size() + num_non_cold_subtables);

  // Fill in mini table entries.
  for (const auto& options : ordered_fields) {
    auto* field = options.field;
    field_entries.push_back({field, options.has_bit_index});
    auto& entry = field_entries.back();
    entry.utf8_check_mode =
        cpp::GetUtf8CheckMode(field, message_options.is_lite);
    entry.type_card = MakeTypeCardForField(field, entry.hasbit_idx >= 0,
                                           options, entry.utf8_check_mode);

    if (field->type() == FieldDescriptor::TYPE_MESSAGE ||
        field->type() == FieldDescriptor::TYPE_GROUP) {
      // Message-typed fields have a FieldAux with the default instance pointer.
      if (field->is_map()) {
        entry.aux_idx = aux_entries.size();
        aux_entries.push_back({kMapAuxInfo, {field}});
        if (message_options.uses_codegen) {
          // If we don't use codegen we can't add these.
          auto* map_value = field->message_type()->map_value();
          if (map_value->message_type() != nullptr) {
            aux_entries.push_back({kSubTable, {map_value}});
          } else if (map_value->type() == FieldDescriptor::TYPE_ENUM &&
                     !cpp::HasPreservingUnknownEnumSemantics(map_value)) {
            aux_entries.push_back({kEnumValidator, {map_value}});
          }
        }
      } else if (field->options().weak()) {
        // Disable the type card for this entry to force the fallback.
        entry.type_card = 0;
      } else if (HasLazyRep(field, options)) {
        if (message_options.uses_codegen) {
          entry.aux_idx = aux_entries.size();
          aux_entries.push_back({kSubMessage, {field}});
          if (options.lazy_opt == field_layout::kTvEager) {
            aux_entries.push_back({kMessageVerifyFunc, {field}});
          } else {
            aux_entries.push_back({kNothing});
          }
        } else {
          entry.aux_idx = TcParseTableBase::FieldEntry::kNoAuxIdx;
        }
      } else {
        AuxType type = options.is_implicitly_weak          ? kSubMessageWeak
                       : options.use_direct_tcparser_table ? kSubTable
                                                           : kSubMessage;
        if (type == kSubTable && is_non_cold(options)) {
          aux_entries[subtable_aux_idx] = {type, {field}};
          entry.aux_idx = subtable_aux_idx;
          ++subtable_aux_idx;
        } else {
          entry.aux_idx = aux_entries.size();
          aux_entries.push_back({type, {field}});
        }
      }
    } else if (field->type() == FieldDescriptor::TYPE_ENUM &&
               !TreatEnumAsInt(field)) {
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

      if (GetEnumValidationRange(field->enum_type(), aux_entry.enum_range.first,
                                 aux_entry.enum_range.last)) {
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
      int idx = options.inlined_string_index;
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
  ABSL_CHECK_EQ(subtable_aux_idx - subtable_aux_idx_begin,
                num_non_cold_subtables);

  return field_entries;
}

TailCallTableInfo::TailCallTableInfo(
    const Descriptor* descriptor, const MessageOptions& message_options,
    absl::Span<const FieldOptions> ordered_fields) {
  fallback_function =
      // Map entries discard unknown data
      descriptor->options().map_entry()
          ? TcParseFunction::kDiscardEverythingFallback
      // Reflection and weak messages have the reflection fallback
      : !message_options.uses_codegen || HasWeakFields(descriptor)
          ? TcParseFunction::kReflectionFallback
      // Codegen messages have lite and non-lite version
      : message_options.is_lite ? TcParseFunction::kGenericFallbackLite
                                : TcParseFunction::kGenericFallback;

  if (descriptor->options().message_set_wire_format()) {
    ABSL_DCHECK(ordered_fields.empty());
    if (message_options.uses_codegen) {
      fast_path_fields = {{TailCallTableInfo::FastFieldInfo::NonField{
          message_options.is_lite
              ? TcParseFunction::kMessageSetWireFormatParseLoopLite
              : TcParseFunction::kMessageSetWireFormatParseLoop,
          0, 0}}};

      aux_entries = {{kSelfVerifyFunc}};
    } else {
      ABSL_DCHECK(!message_options.is_lite);
      // The message set parser loop only handles codegen because it hardcodes
      // the generated extension registry. For reflection, use the reflection
      // loop which can handle arbitrary message factories.
      fast_path_fields = {{TailCallTableInfo::FastFieldInfo::NonField{
          TcParseFunction::kReflectionParseLoop, 0, 0}}};
    }

    table_size_log2 = 0;
    num_to_entry_table = MakeNumToEntryTable(ordered_fields);
    field_name_data = GenerateFieldNames(descriptor, field_entries,
                                         message_options, ordered_fields);

    return;
  }

  ABSL_DCHECK(std::is_sorted(ordered_fields.begin(), ordered_fields.end(),
                             [](const auto& lhs, const auto& rhs) {
                               return lhs.field->number() < rhs.field->number();
                             }));
  // If this message has any inlined string fields, store the donation state
  // offset in the first auxiliary entry, which is kInlinedStringAuxIdx.
  if (std::any_of(ordered_fields.begin(), ordered_fields.end(),
                  [](auto& f) { return f.is_string_inlined; })) {
    aux_entries.resize(kInlinedStringAuxIdx + 1);  // Allocate our slot
    aux_entries[kInlinedStringAuxIdx] = {kInlinedStringDonatedOffset};
  }

  // If this message is split, store the split pointer offset in the second
  // and third auxiliary entries, which are kSplitOffsetAuxIdx and
  // kSplitSizeAuxIdx.
  if (std::any_of(ordered_fields.begin(), ordered_fields.end(),
                  [](auto& f) { return f.should_split; })) {
    static_assert(kSplitOffsetAuxIdx + 1 == kSplitSizeAuxIdx, "");
    aux_entries.resize(kSplitSizeAuxIdx + 1);  // Allocate our 2 slots
    aux_entries[kSplitOffsetAuxIdx] = {kSplitOffset};
    aux_entries[kSplitSizeAuxIdx] = {kSplitSizeof};
  }

  field_entries = BuildFieldEntries(descriptor, message_options, ordered_fields,
                                    aux_entries);

  auto end_group_tag = GetEndGroupTag(descriptor);

  FastFieldInfo fast_fields[TcParseTableBase::kMaxFastFields];
  // Bit mask for the fields that are "important". Unimportant fields might be
  // set but it's ok if we lose them from the fast table. For example, cold
  // fields.
  uint32_t important_fields = 0;
  static_assert(
      sizeof(important_fields) * 8 >= TcParseTableBase::kMaxFastFields, "");
  // The largest table we allow has the same number of entries as the
  // message has fields, rounded up to the next power of 2 (e.g., a message
  // with 5 fields can have a fast table of size 8). A larger table *might*
  // cover more fields in certain cases, but a larger table in that case
  // would have mostly empty entries; so, we cap the size to avoid
  // pathologically sparse tables.
  // However, if this message uses group encoding, the tables are sometimes very
  // sparse because the fields in the group avoid using the same field
  // numbering as the parent message (even though currently, the proto
  // compiler allows the overlap, and there is no possible conflict.)
  // NOTE: The +1 is to maintain the existing behavior that does not match the
  // documented one. When the number of fields is exactly a power of two we
  // allow double that.
  size_t num_fast_fields =
      FastParseTableSize(ordered_fields.size(), end_group_tag);
  PopulateFastFields(
      end_group_tag, field_entries, message_options, ordered_fields,
      absl::MakeSpan(fast_fields, num_fast_fields), important_fields);
  // If we can halve the table without dropping important fields, do it.
  while (num_fast_fields > 1 &&
         (important_fields & (important_fields >> num_fast_fields / 2)) == 0) {
    // Half the table by merging fields.
    num_fast_fields /= 2;
    for (size_t i = 0; i < num_fast_fields; ++i) {
      size_t merge_i = i + num_fast_fields;
      // Overwrite the surviving entries if the discarded half contains an
      // important field (meaning the surviving entry is not) or the surviving
      // entry is empty.
      if (((important_fields >> merge_i) & 1) != 0 ||
          fast_fields[i].is_empty()) {
        fast_fields[i] = fast_fields[merge_i];
      }
    }
    important_fields |= important_fields >> num_fast_fields;
  }

  fast_path_fields.assign(fast_fields, fast_fields + num_fast_fields);
  table_size_log2 = absl::bit_width(num_fast_fields) - 1;

  num_to_entry_table = MakeNumToEntryTable(ordered_fields);
  ABSL_CHECK_EQ(field_entries.size(), ordered_fields.size());
  field_name_data = GenerateFieldNames(descriptor, field_entries,
                                       message_options, ordered_fields);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
