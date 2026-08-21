// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// This file contains routines to generate tail-call table parsing tables.
// Everything in this file is for internal use only.

#ifndef GOOGLE_PROTOBUF_GENERATED_MESSAGE_TCTABLE_GEN_H__
#define GOOGLE_PROTOBUF_GENERATED_MESSAGE_TCTABLE_GEN_H__

#include <cstddef>
#include <cstdint>
#include <vector>

#include "absl/types/optional.h"
#include "absl/types/span.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"

// Must come last:
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {
enum class TcParseFunction : uint8_t;

namespace field_layout {
enum TransformValidation : uint16_t;
}  // namespace field_layout

PROTOBUF_EXPORT uint32_t
GetRecodedTagForFastParsing(const FieldDescriptor* field);

PROTOBUF_EXPORT absl::optional<uint32_t> GetEndGroupTag(
    const Descriptor* descriptor);

PROTOBUF_EXPORT uint32_t
FastParseTableSize(size_t num_fields, absl::optional<uint32_t> end_group_tag);

PROTOBUF_EXPORT bool IsFieldTypeEligibleForFastParsing(
    const FieldDescriptor* field);

// Helper class for generating tailcall parsing functions.
struct PROTOBUF_EXPORT TailCallTableInfo {
  // The tailcall parser can only update the first 32 hasbits. Fields with
  // has-bits beyond the first 32 are handled by mini parsing/fallback.
  static constexpr int kMaxFastFieldHasbitIndex = 31;
  // Max padding used within a SkipEntryBlock between entries.
  static constexpr int kMaxSkipEntrySpacing = 96;

  struct MessageOptions {
    bool is_lite;
    bool uses_codegen;
  };
  struct FieldOptions {
    const FieldDescriptor* field;
    int has_bit_index;
    // For presence awareness (e.g. PDProto).
    float presence_probability;
    // kTvEager, kTvLazy, or 0
    field_layout::TransformValidation lazy_opt;
    bool is_implicitly_weak;
    bool use_direct_tcparser_table;
    bool should_split;

    // Whether to use the InlinedStringField representation.
    struct StringInlined {};
    // Whether to use the MicroString representation.
    struct MicroString {
      uint8_t sso_size;
    };

    bool is_string_inlined() const {
      return std::holds_alternative<StringInlined>(str_options);
    }
    bool is_micro_string() const {
      return std::holds_alternative<MicroString>(str_options);
    }
    uint8_t micro_string_sso() const {
      return std::get<MicroString>(str_options).sso_size;
    }

    using StrOptions = std::variant<std::monostate, StringInlined, MicroString>;
    StrOptions str_options;
  };

  struct FieldEntryInfo;
  struct AuxEntry;

  static std::vector<FieldEntryInfo> BuildFieldEntries(
      const Descriptor* descriptor, const MessageOptions& message_options,
      absl::Span<const FieldOptions> ordered_fields,
      std::vector<AuxEntry>& aux_entries);

  TailCallTableInfo(const Descriptor* descriptor,
                    const MessageOptions& message_options,
                    absl::Span<const FieldOptions> ordered_fields);

  TcParseFunction fallback_function;

  // Fields parsed by the table fast-path.
  struct FastFieldInfo {
    struct Empty {};
    struct NonField {
      TcParseFunction func;
      uint16_t coded_tag;
      uint16_t nonfield_info;
    };
    struct Field {
      TcParseFunction func;
      const FieldDescriptor* field;
      uint16_t coded_tag;
      uint8_t hasbit_idx;
      uint8_t aux_idx;

      // For internal caching.
      float presence_probability;
    };
    struct MpField {
      TcParseFunction func;
      const FieldDescriptor* field;
      uint32_t field_index;
      uint16_t coded_tag;
      uint8_t function_index;

      // For internal caching.
      float presence_probability;
    };
    // Ordered by priority.
    std::variant<Empty, MpField, Field, NonField> data;

    friend bool operator<(const FastFieldInfo& a, const FastFieldInfo& b) {
      if (a.data.index() != b.data.index()) {
        return a.data.index() < b.data.index();
      }
      if (auto* f = a.AsField()) {
        return f->presence_probability < b.AsField()->presence_probability;
      }
      if (auto* f = a.AsMpField()) {
        return f->presence_probability < b.AsMpField()->presence_probability;
      }
      return false;
    }

    template <typename T>
    static constexpr size_t kIndex = decltype(data){T{}}.index();

    bool IsBetterFast(double presence_probability) {
      if (data.index() < kIndex<Field>) return true;
      if (data.index() > kIndex<Field>) return false;
      return presence_probability > AsField()->presence_probability;
    }

    bool IsBetterMpFast(double presence_probability) {
      if (data.index() < kIndex<MpField>) return true;
      if (data.index() > kIndex<MpField>) return false;
      return presence_probability > AsMpField()->presence_probability;
    }

    bool is_empty() const { return std::holds_alternative<Empty>(data); }
    const Field* AsField() const { return std::get_if<Field>(&data); }
    const NonField* AsNonField() const { return std::get_if<NonField>(&data); }
    const MpField* AsMpField() const { return std::get_if<MpField>(&data); }
  };
  std::vector<FastFieldInfo> fast_path_fields;

  // Fields parsed by mini parsing routines.
  struct FieldEntryInfo {
    const FieldDescriptor* field;
    int hasbit_idx;
    uint16_t aux_idx;
    uint16_t type_card;

    // For internal caching.
    cpp::Utf8CheckMode utf8_check_mode;
  };
  std::vector<FieldEntryInfo> field_entries;

  enum AuxType {
    kNothing = 0,
    kSplitOffset,
    kSplitSizeof,
    kSubMessageGlobals,
    kSubTable,
    kSubMessageGlobalsWeak,
    kMessageVerifyFunc,
    kSelfVerifyFunc,
    kEnumRange,
    kEnumValidator,
    kNumericOffset,
    kMapAuxInfo,
  };
  struct AuxEntry {
    AuxType type;
    struct EnumRange {
      int32_t first;
      int32_t last;
    };
    union {
      const FieldDescriptor* field;
      const Descriptor* desc;
      uint32_t offset;
      EnumRange enum_range;
    };
  };
  std::vector<AuxEntry> aux_entries;

  struct SkipEntry16 {
    uint16_t skipmap;
    uint16_t field_entry_offset;
  };
  struct SkipEntryBlock {
    uint32_t first_fnum;
    std::vector<SkipEntry16> entries;
  };
  struct NumToEntryTable {
    uint32_t skipmap32;  // for fields #1 - #32
    std::vector<SkipEntryBlock> blocks;
    // Compute the number of uint16_t required to represent this table.
    int size16() const {
      int size = 2;  // for the termination field#
      for (const auto& block : blocks) {
        // 2 for the field#, 1 for a count of skip entries, 2 for each entry.
        size += static_cast<int>(3 + block.entries.size() * 2);
      }
      return size;
    }
  };
  NumToEntryTable num_to_entry_table;

  std::vector<uint8_t> field_name_data;

  // Table size.
  int table_size_log2;
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_GENERATED_MESSAGE_TCTABLE_GEN_H__
