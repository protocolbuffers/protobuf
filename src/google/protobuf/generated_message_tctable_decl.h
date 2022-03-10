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

// This file contains declarations needed in generated headers for messages
// that use tail-call table parsing. Everything in this file is for internal
// use only.

#ifndef GOOGLE_PROTOBUF_GENERATED_MESSAGE_TCTABLE_DECL_H__
#define GOOGLE_PROTOBUF_GENERATED_MESSAGE_TCTABLE_DECL_H__

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <google/protobuf/message_lite.h>
#include <google/protobuf/parse_context.h>

// Must come last:
#include <google/protobuf/port_def.inc>

namespace google {
namespace protobuf {
namespace internal {

// Additional information about this field:
struct TcFieldData {
  constexpr TcFieldData() : data(0) {}

  // Fast table entry constructor:
  constexpr TcFieldData(uint16_t coded_tag, uint8_t hasbit_idx, uint8_t aux_idx,
                        uint16_t offset)
      : data(uint64_t{offset} << 48 |      //
             uint64_t{aux_idx} << 24 |     //
             uint64_t{hasbit_idx} << 16 |  //
             uint64_t{coded_tag}) {}

  // Fields used in fast table parsing:
  //
  //     Bit:
  //     +-----------+-------------------+
  //     |63    ..     32|31     ..     0|
  //     +---------------+---------------+
  //     :   .   :   .   :   . 16|=======| [16] coded_tag()
  //     :   .   :   .   : 24|===|   .   : [ 8] hasbit_idx()
  //     :   .   :   . 32|===|   :   .   : [ 8] aux_idx()
  //     :   . 48:---.---:   .   :   .   : [16] (unused)
  //     |=======|   .   :   .   :   .   : [16] offset()
  //     +-----------+-------------------+
  //     |63    ..     32|31     ..     0|
  //     +---------------+---------------+

  template <typename TagType = uint16_t>
  TagType coded_tag() const {
    return static_cast<TagType>(data);
  }
  uint8_t hasbit_idx() const { return static_cast<uint8_t>(data >> 16); }
  uint8_t aux_idx() const { return static_cast<uint8_t>(data >> 24); }
  uint16_t offset() const { return static_cast<uint16_t>(data >> 48); }

  // Fields used in mini table parsing:
  //
  //     Bit:
  //     +-----------+-------------------+
  //     |63    ..     32|31     ..     0|
  //     +---------------+---------------+
  //     :   .   :   .   |===============| [32] tag() (decoded)
  //     |===============|   .   :   .   : [32] entry_offset()
  //     +-----------+-------------------+
  //     |63    ..     32|31     ..     0|
  //     +---------------+---------------+

  uint32_t tag() const { return static_cast<uint32_t>(data); }
  uint32_t entry_offset() const { return static_cast<uint32_t>(data >> 32); }

  uint64_t data;
};

struct TcParseTableBase;

// TailCallParseFunc is the function pointer type used in the tailcall table.
typedef const char* (*TailCallParseFunc)(PROTOBUF_TC_PARAM_DECL);

namespace field_layout {
struct Offset {
  uint32_t off;
};
}  // namespace field_layout

#if defined(_MSC_VER) && !defined(_WIN64)
#pragma warning(push)
// TcParseTableBase is intentionally overaligned on 32 bit targets.
#pragma warning(disable : 4324)
#endif

// Base class for message-level table with info for the tail-call parser.
struct alignas(uint64_t) TcParseTableBase {
  // Common attributes for message layout:
  uint16_t has_bits_offset;
  uint16_t extension_offset;
  uint32_t extension_range_low;
  uint32_t extension_range_high;
  uint32_t max_field_number;
  uint8_t fast_idx_mask;
  uint16_t lookup_table_offset;
  uint32_t skipmap32;
  uint32_t field_entries_offset;
  uint16_t num_field_entries;

  uint16_t num_aux_entries;
  uint32_t aux_offset;

  const MessageLite* default_instance;

  // Handler for fields which are not handled by table dispatch.
  TailCallParseFunc fallback;

  // This constructor exactly follows the field layout, so it's technically
  // not necessary.  However, it makes it much much easier to add or re-arrange
  // fields, because it can be overloaded with an additional constructor,
  // temporarily allowing both old and new protocol buffer headers to be
  // compiled.
  constexpr TcParseTableBase(
      uint16_t has_bits_offset, uint16_t extension_offset,
      uint32_t extension_range_low, uint32_t extension_range_high,
      uint32_t max_field_number, uint8_t fast_idx_mask,
      uint16_t lookup_table_offset, uint32_t skipmap32,
      uint32_t field_entries_offset, uint16_t num_field_entries,
      uint16_t num_aux_entries, uint32_t aux_offset,
      const MessageLite* default_instance, TailCallParseFunc fallback)
      : has_bits_offset(has_bits_offset),
        extension_offset(extension_offset),
        extension_range_low(extension_range_low),
        extension_range_high(extension_range_high),
        max_field_number(max_field_number),
        fast_idx_mask(fast_idx_mask),
        lookup_table_offset(lookup_table_offset),
        skipmap32(skipmap32),
        field_entries_offset(field_entries_offset),
        num_field_entries(num_field_entries),
        num_aux_entries(num_aux_entries),
        aux_offset(aux_offset),
        default_instance(default_instance),
        fallback(fallback) {}

  // Table entry for fast-path tailcall dispatch handling.
  struct FastFieldEntry {
    // Target function for dispatch:
    TailCallParseFunc target;
    // Field data used during parse:
    TcFieldData bits;
  };
  // There is always at least one table entry.
  const FastFieldEntry* fast_entry(size_t idx) const {
    return reinterpret_cast<const FastFieldEntry*>(this + 1) + idx;
  }

  // Returns a begin iterator (pointer) to the start of the field lookup table.
  const uint16_t* field_lookup_begin() const {
    return reinterpret_cast<const uint16_t*>(reinterpret_cast<uintptr_t>(this) +
                                             lookup_table_offset);
  }

  // Field entry for all fields.
  struct FieldEntry {
    uint32_t offset;     // offset in the message object
    int32_t has_idx;     // has-bit index
    uint16_t aux_idx;    // index for `field_aux`.
    uint16_t type_card;  // `FieldType` and `Cardinality` (see _impl.h)
  };

  // Returns a begin iterator (pointer) to the start of the field entries array.
  const FieldEntry* field_entries_begin() const {
    return reinterpret_cast<const FieldEntry*>(
        reinterpret_cast<uintptr_t>(this) + field_entries_offset);
  }

  // Auxiliary entries for field types that need extra information.
  union FieldAux {
    constexpr FieldAux() : message_default(nullptr) {}
    constexpr FieldAux(bool (*enum_validator)(int))
        : enum_validator(enum_validator) {}
    constexpr FieldAux(field_layout::Offset off) : offset(off.off) {}
    constexpr FieldAux(int16_t range_start, uint16_t range_length)
        : enum_range{range_start, range_length} {}
    constexpr FieldAux(const MessageLite* msg) : message_default(msg) {}
    bool (*enum_validator)(int);
    struct {
      int16_t start;    // minimum enum number (if it fits)
      uint16_t length;  // length of range (i.e., max = start + length - 1)
    } enum_range;
    uint32_t offset;
    const MessageLite* message_default;
  };
  const FieldAux* field_aux(uint32_t idx) const {
    return reinterpret_cast<const FieldAux*>(reinterpret_cast<uintptr_t>(this) +
                                             aux_offset) +
           idx;
  }
  const FieldAux* field_aux(const FieldEntry* entry) const {
    return field_aux(entry->aux_idx);
  }

  // Field name data
  const char* name_data() const {
    return reinterpret_cast<const char*>(reinterpret_cast<uintptr_t>(this) +
                                         aux_offset +
                                         num_aux_entries * sizeof(FieldAux));
  }
};

#if defined(_MSC_VER) && !defined(_WIN64)
#pragma warning(pop)
#endif

static_assert(sizeof(TcParseTableBase::FastFieldEntry) <= 16,
              "Fast field entry is too big.");
static_assert(sizeof(TcParseTableBase::FieldEntry) <= 16,
              "Field entry is too big.");

template <size_t kFastTableSizeLog2, size_t kNumFieldEntries = 0,
          size_t kNumFieldAux = 0, size_t kNameTableSize = 0,
          size_t kFieldLookupSize = 2>
struct TcParseTable {
  TcParseTableBase header;

  // Entries for each field.
  //
  // Fields are indexed by the lowest bits of their field number. The field
  // number is masked to fit inside the table. Note that the parsing logic
  // generally calls `TailCallParseTableBase::fast_entry()` instead of accessing
  // this field directly.
  std::array<TcParseTableBase::FastFieldEntry, (1 << kFastTableSizeLog2)>
      fast_entries;

  // Just big enough to find all the field entries.
  std::array<uint16_t, kFieldLookupSize> field_lookup_table;
  // Entries for all fields:
  std::array<TcParseTableBase::FieldEntry, kNumFieldEntries> field_entries;
  std::array<TcParseTableBase::FieldAux, kNumFieldAux> aux_entries;
  std::array<char, kNameTableSize> field_names;
};

// Partial specialization: if there are no aux entries, there will be no array.
// In C++, arrays cannot have length 0, but (C++11) std::array<T, 0> is valid.
// However, different implementations have different sizeof(std::array<T, 0>).
// Skipping the member makes offset computations portable.
template <size_t kFastTableSizeLog2, size_t kNumFieldEntries,
          size_t kNameTableSize, size_t kFieldLookupSize>
struct TcParseTable<kFastTableSizeLog2, kNumFieldEntries, 0, kNameTableSize,
                    kFieldLookupSize> {
  TcParseTableBase header;
  std::array<TcParseTableBase::FastFieldEntry, (1 << kFastTableSizeLog2)>
      fast_entries;
  std::array<uint16_t, kFieldLookupSize> field_lookup_table;
  std::array<TcParseTableBase::FieldEntry, kNumFieldEntries> field_entries;
  std::array<char, kNameTableSize> field_names;
};

// Partial specialization: if there are no fields at all, then we can save space
// by skipping the field numbers and entries.
template <size_t kNameTableSize, size_t kFieldLookupSize>
struct TcParseTable<0, 0, 0, kNameTableSize, kFieldLookupSize> {
  TcParseTableBase header;
  // N.B.: the fast entries are sized by log2, so 2**0 fields = 1 entry.
  // The fast parsing loop will always use this entry, so it must be present.
  std::array<TcParseTableBase::FastFieldEntry, 1> fast_entries;
  std::array<uint16_t, kFieldLookupSize> field_lookup_table;
  std::array<char, kNameTableSize> field_names;
};

static_assert(std::is_standard_layout<TcParseTable<1>>::value,
              "TcParseTable must be standard layout.");

static_assert(offsetof(TcParseTable<1>, fast_entries) ==
                  sizeof(TcParseTableBase),
              "Table entries must be laid out after TcParseTableBase.");

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include <google/protobuf/port_undef.inc>

#endif  // GOOGLE_PROTOBUF_GENERATED_MESSAGE_TCTABLE_DECL_H__
