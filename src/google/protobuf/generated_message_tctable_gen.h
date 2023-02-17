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

// This file contains routines to generate tail-call table parsing tables.
// Everything in this file is for internal use only.

#ifndef GOOGLE_PROTOBUF_GENERATED_MESSAGE_TCTABLE_GEN_H__
#define GOOGLE_PROTOBUF_GENERATED_MESSAGE_TCTABLE_GEN_H__

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/generated_message_tctable_decl.h"

// Must come last:
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

// X Macro helpers for PROTOBUF_TC_PARSE_FUNCTION_LIST
#define PROTOBUF_TC_PARSE_FUNCTION_LIST_SINGLE(fn) \
  PROTOBUF_TC_PARSE_FUNCTION_X(fn##S1)             \
  PROTOBUF_TC_PARSE_FUNCTION_X(fn##S2)
#define PROTOBUF_TC_PARSE_FUNCTION_LIST_REPEATED(fn) \
  PROTOBUF_TC_PARSE_FUNCTION_LIST_SINGLE(fn)         \
  PROTOBUF_TC_PARSE_FUNCTION_X(fn##R1)               \
  PROTOBUF_TC_PARSE_FUNCTION_X(fn##R2)
#define PROTOBUF_TC_PARSE_FUNCTION_LIST_PACKED(fn) \
  PROTOBUF_TC_PARSE_FUNCTION_LIST_REPEATED(fn)     \
  PROTOBUF_TC_PARSE_FUNCTION_X(fn##P1)             \
  PROTOBUF_TC_PARSE_FUNCTION_X(fn##P2)

// TcParseFunction defines the set of table driven, tail call optimized parse
// functions. This list currently does not include all types such as maps.
//
// This table identifies the logical set of functions, it does not imply that
// functions of the same name do exist, and some entries may point to thunks or
// generic implementations accepting multiple types of input.
//
// The names are encoded as follows:
//   kFast<type>[<validation>][cardinality][tag_width]
//
//   type:
//     V8  - bool
//     V32 - int32/uint32 varint
//     Z32 - int32/uint32 varint with zigzag encoding
//     V64 - int64/uint64 varint
//     Z64 - int64/uint64 varint with zigzag encoding
//     F32 - int32/uint32/float fixed width value
//     F64 - int64/uint64/double fixed width value
//     E   - enum
//     S   - string / bytes
//     Gd  - group
//     Gt  - group width table driven parse tables
//     Md  - message
//     Mt  - message width table driven parse tables
//
// * string types can have a `c` or `i` suffix, indicating the
//   underlying storage type to be cord or inlined respectively.
//
//  validation:
//    For enums:
//      v  - verify
//      r  - verify; enum values are a contiguous range
//      r0 - verify; enum values are a small contiguous range starting at 0
//      r1 - verify; enum values are a small contiguous range starting at 1
//    For strings:
//      u - validate utf8 encoding
//      v - validate utf8 encoding for debug only
//
//  cardinality:
//    S - singular / optional
//    R - repeated
//    P - packed
//
//  tag_width:
//    1: single byte encoded tag
//    2: two byte encoded tag
//
// Examples:
//   FastV8S1, FastEr1P2, FastScS1
//
#define PROTOBUF_TC_PARSE_FUNCTION_LIST            \
  PROTOBUF_TC_PARSE_FUNCTION_LIST_PACKED(FastV8)   \
  PROTOBUF_TC_PARSE_FUNCTION_LIST_PACKED(FastV32)  \
  PROTOBUF_TC_PARSE_FUNCTION_LIST_PACKED(FastV64)  \
  PROTOBUF_TC_PARSE_FUNCTION_LIST_PACKED(FastZ32)  \
  PROTOBUF_TC_PARSE_FUNCTION_LIST_PACKED(FastZ64)  \
  PROTOBUF_TC_PARSE_FUNCTION_LIST_PACKED(FastF32)  \
  PROTOBUF_TC_PARSE_FUNCTION_LIST_PACKED(FastF64)  \
  PROTOBUF_TC_PARSE_FUNCTION_LIST_PACKED(FastEv)   \
  PROTOBUF_TC_PARSE_FUNCTION_LIST_PACKED(FastEr)   \
  PROTOBUF_TC_PARSE_FUNCTION_LIST_PACKED(FastEr0)  \
  PROTOBUF_TC_PARSE_FUNCTION_LIST_PACKED(FastEr1)  \
  PROTOBUF_TC_PARSE_FUNCTION_LIST_REPEATED(FastS)  \
  PROTOBUF_TC_PARSE_FUNCTION_LIST_REPEATED(FastSu) \
  PROTOBUF_TC_PARSE_FUNCTION_LIST_REPEATED(FastSv) \
  PROTOBUF_TC_PARSE_FUNCTION_LIST_SINGLE(FastSi)   \
  PROTOBUF_TC_PARSE_FUNCTION_LIST_SINGLE(FastSiu)  \
  PROTOBUF_TC_PARSE_FUNCTION_LIST_SINGLE(FastSiv)  \
  PROTOBUF_TC_PARSE_FUNCTION_LIST_SINGLE(FastSc)   \
  PROTOBUF_TC_PARSE_FUNCTION_LIST_SINGLE(FastScv)  \
  PROTOBUF_TC_PARSE_FUNCTION_LIST_SINGLE(FastScu)  \
  PROTOBUF_TC_PARSE_FUNCTION_LIST_REPEATED(FastGd) \
  PROTOBUF_TC_PARSE_FUNCTION_LIST_REPEATED(FastGt) \
  PROTOBUF_TC_PARSE_FUNCTION_LIST_REPEATED(FastMd) \
  PROTOBUF_TC_PARSE_FUNCTION_LIST_REPEATED(FastMt)

#define PROTOBUF_TC_PARSE_FUNCTION_X(value) k##value,
enum class TcParseFunction { kNone, PROTOBUF_TC_PARSE_FUNCTION_LIST };
#undef PROTOBUF_TC_PARSE_FUNCTION_X

// Helper class for generating tailcall parsing functions.
struct PROTOBUF_EXPORT TailCallTableInfo {
  struct PerFieldOptions {
    bool is_lazy;
    bool is_string_inlined;
    bool is_implicitly_weak;
    bool use_direct_tcparser_table;
    bool is_lite;
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
                    const OptionProvider& option_provider,
                    const std::vector<int>& has_bit_indices,
                    const std::vector<int>& inlined_string_indices);

  // Fields parsed by the table fast-path.
  struct FastFieldInfo {
    TcParseFunction parse_function;

    std::string func_name;
    const FieldDescriptor* field;
    uint16_t coded_tag;
    uint8_t hasbit_idx;
    uint8_t aux_idx;
    uint16_t type_card;
    uint16_t nonfield_info;
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
    kEnumRange,
    kEnumValidator,
    kNumericOffset,
  };
  struct AuxEntry {
    AuxType type;
    struct EnumRange {
      int16_t start;
      uint16_t size;
    };
    union {
      const FieldDescriptor* field;
      uint32_t offset;
      EnumRange enum_range;
    };
  };
  std::vector<AuxEntry> aux_entries;

  // Fields parsed by generated fallback function.
  std::vector<const FieldDescriptor*> fallback_fields;

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
  // True if a generated fallback function is required instead of generic.
  bool use_generated_fallback;
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_GENERATED_MESSAGE_TCTABLE_GEN_H__
