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

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "absl/types/variant.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/generated_message_tctable_decl.h"

// Must come last:
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {
enum class TcParseFunction : uint8_t;

namespace field_layout {
enum TransformValidation : uint16_t;
}  // namespace field_layout

// Helper class for generating tailcall parsing functions.
struct PROTOBUF_EXPORT TailCallTableInfo {
  struct MessageOptions {
    bool is_lite;
    bool uses_codegen;
  };
  struct PerFieldOptions {
    // For presence awareness (e.g. PDProto).
    float presence_probability;
    // kTvEager, kTvLazy, or 0
    field_layout::TransformValidation lazy_opt;
    bool is_string_inlined;
    bool is_implicitly_weak;
    bool use_direct_tcparser_table;
    bool should_split;
  };
  class OptionProvider {
   public:
    virtual PerFieldOptions GetForField(const FieldDescriptor*) const = 0;

   protected:
    ~OptionProvider() = default;
  };

  TailCallTableInfo(const Descriptor* descriptor,
                    const std::vector<const FieldDescriptor*>& ordered_fields,
                    const MessageOptions& message_options,
                    const OptionProvider& option_provider,
                    const std::vector<int>& has_bit_indices,
                    const std::vector<int>& inlined_string_indices);

  // Fields parsed by the table fast-path.
  struct FastFieldInfo {
    struct Empty {};
    struct Field {
      TcParseFunction func;
      uint16_t coded_tag;
      const FieldDescriptor* field;
      uint8_t hasbit_idx;
      uint8_t aux_idx;
    };
    struct NonField {
      TcParseFunction func;
      uint16_t coded_tag;
      uint16_t nonfield_info;
    };
    absl::variant<Empty, Field, NonField> data;

    bool is_empty() const { return absl::holds_alternative<Empty>(data); }
    const Field* AsField() const { return absl::get_if<Field>(&data); }
    const NonField* AsNonField() const { return absl::get_if<NonField>(&data); }
  };
  std::vector<FastFieldInfo> fast_path_fields;

  // Fields parsed by mini parsing routines.
  struct FieldEntryInfo {
    const FieldDescriptor* field;
    int hasbit_idx;
    int inlined_string_idx;
    uint16_t aux_idx;
    uint16_t type_card;
  };
  std::vector<FieldEntryInfo> field_entries;

  enum AuxType {
    kNothing = 0,
    kInlinedStringDonatedOffset,
    kSplitOffset,
    kSplitSizeof,
    kSubMessage,
    kSubTable,
    kSubMessageWeak,
    kMessageVerifyFunc,
    kEnumRange,
    kEnumValidator,
    kNumericOffset,
    kMapAuxInfo,
    kCreateInArena,
  };
  struct AuxEntry {
    AuxType type;
    struct EnumRange {
      int16_t start;
      uint16_t size;
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
