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
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "google/protobuf/message_lite.h"
#include "google/protobuf/parse_context.h"

// Must come last:
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

// Additional information about this field:
struct TcFieldData {
  constexpr TcFieldData() : data(0) {}
  explicit constexpr TcFieldData(uint64_t data) : data(data) {}

  // Fast table entry constructor:
  constexpr TcFieldData(uint16_t coded_tag, uint8_t hasbit_idx, uint8_t aux_idx,
                        uint16_t offset)
      : data(uint64_t{offset} << 48 |      //
             uint64_t{aux_idx} << 24 |     //
             uint64_t{hasbit_idx} << 16 |  //
             uint64_t{coded_tag}) {}

  // Constructor to create an explicit 'uninitialized' instance.
  // This constructor can be used to pass an uninitialized `data` value to a
  // table driven parser function that does not use `data`. The purpose of this
  // is that it allows the compiler to reallocate and re-purpose the register
  // that is currently holding its value for other data. This reduces register
  // allocations inside the highly optimized varint parsing functions.
  //
  // Applications not using `data` use the `PROTOBUF_TC_PARAM_NO_DATA_DECL`
  // macro to declare the standard input arguments with no name for the `data`
  // argument. Callers then use the `PROTOBUF_TC_PARAM_NO_DATA_PASS` macro.
  //
  // Example:
  //   if (ptr == nullptr) {
  //      PROTOBUF_MUSTTAIL return Error(PROTOBUF_TC_PARAM_NO_DATA_PASS);
  //   }
  struct DefaultInit {};
  TcFieldData(DefaultInit) {}  // NOLINT(google-explicit-constructor)

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

  // Constructor for special entries that do not represent a field.
  //  - End group: `nonfield_info` is the decoded tag.
  constexpr TcFieldData(uint16_t coded_tag, uint16_t nonfield_info)
      : data(uint64_t{nonfield_info} << 16 |  //
             uint64_t{coded_tag}) {}

  // Fields used in non-field entries
  //
  //     Bit:
  //     +-----------+-------------------+
  //     |63    ..     32|31     ..     0|
  //     +---------------+---------------+
  //     :   .   :   .   :   . 16|=======| [16] coded_tag()
  //     :   .   :   . 32|=======|   .   : [16] decoded_tag()
  //     :---.---:---.---:   .   :   .   : [32] (unused)
  //     +-----------+-------------------+
  //     |63    ..     32|31     ..     0|
  //     +---------------+---------------+

  uint16_t decoded_tag() const { return static_cast<uint16_t>(data >> 16); }

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

  union {
    uint64_t data;
  };
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

struct FieldAuxDefaultMessage {};

// Small type card used by mini parse to handle map entries.
// Map key/values are very limited, so we can encode the whole thing in a single
// byte.
class MapTypeCard {
 public:
  enum CppType { kBool, k32, k64, kString, kMessage };
  MapTypeCard() = default;
  constexpr MapTypeCard(WireFormatLite::WireType wiretype, CppType cpp_type,
                        bool is_zigzag_utf8, bool is_signed)
      : data_(static_cast<uint8_t>((static_cast<uint8_t>(wiretype) << 0) |
                                   (static_cast<uint8_t>(cpp_type) << 3) |
                                   (static_cast<uint8_t>(is_zigzag_utf8) << 6) |
                                   (static_cast<uint8_t>(is_signed) << 7))) {}

  WireFormatLite::WireType wiretype() const {
    return static_cast<WireFormatLite::WireType>((data_ >> 0) & 0x7);
  }

  CppType cpp_type() const { return static_cast<CppType>((data_ >> 3) & 0x7); }

  bool is_signed() const {
    ABSL_DCHECK(cpp_type() == CppType::k32 || cpp_type() == CppType::k64);
    return static_cast<bool>(data_ >> 7);
  }

  bool is_zigzag() const {
    ABSL_DCHECK(wiretype() == WireFormatLite::WIRETYPE_VARINT);
    ABSL_DCHECK(cpp_type() == CppType::k32 || cpp_type() == CppType::k64);
    return is_zigzag_utf8();
  }
  bool is_utf8() const {
    ABSL_DCHECK(wiretype() == WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
    ABSL_DCHECK(cpp_type() == CppType::kString);
    return is_zigzag_utf8();
  }

 private:
  bool is_zigzag_utf8() const { return static_cast<bool>((data_ >> 6) & 0x1); }
  uint8_t data_;
};
static_assert(sizeof(MapTypeCard) == sizeof(uint8_t), "");

// Make the map entry type card for a specified field type.
constexpr MapTypeCard MakeMapTypeCard(WireFormatLite::FieldType type) {
  switch (type) {
    case WireFormatLite::TYPE_FLOAT:
      return {WireFormatLite::WIRETYPE_FIXED32, MapTypeCard::k32, false, true};
    case WireFormatLite::TYPE_FIXED32:
      return {WireFormatLite::WIRETYPE_FIXED32, MapTypeCard::k32, false, false};
    case WireFormatLite::TYPE_SFIXED32:
      return {WireFormatLite::WIRETYPE_FIXED32, MapTypeCard::k32, false, true};

    case WireFormatLite::TYPE_DOUBLE:
      return {WireFormatLite::WIRETYPE_FIXED64, MapTypeCard::k64, false, true};
    case WireFormatLite::TYPE_FIXED64:
      return {WireFormatLite::WIRETYPE_FIXED64, MapTypeCard::k64, false, false};
    case WireFormatLite::TYPE_SFIXED64:
      return {WireFormatLite::WIRETYPE_FIXED64, MapTypeCard::k64, false, true};

    case WireFormatLite::TYPE_BOOL:
      return {WireFormatLite::WIRETYPE_VARINT, MapTypeCard::kBool, false,
              false};

    case WireFormatLite::TYPE_ENUM:
      // Enum validation is handled via `value_is_validated_enum` below.
      return {WireFormatLite::WIRETYPE_VARINT, MapTypeCard::k32, false, true};
    case WireFormatLite::TYPE_INT32:
      return {WireFormatLite::WIRETYPE_VARINT, MapTypeCard::k32, false, true};
    case WireFormatLite::TYPE_UINT32:
      return {WireFormatLite::WIRETYPE_VARINT, MapTypeCard::k32, false, false};

    case WireFormatLite::TYPE_INT64:
      return {WireFormatLite::WIRETYPE_VARINT, MapTypeCard::k64, false, true};
    case WireFormatLite::TYPE_UINT64:
      return {WireFormatLite::WIRETYPE_VARINT, MapTypeCard::k64, false, false};

    case WireFormatLite::TYPE_SINT32:
      return {WireFormatLite::WIRETYPE_VARINT, MapTypeCard::k32, true, true};
    case WireFormatLite::TYPE_SINT64:
      return {WireFormatLite::WIRETYPE_VARINT, MapTypeCard::k64, true, true};

    case WireFormatLite::TYPE_STRING:
      return {WireFormatLite::WIRETYPE_LENGTH_DELIMITED, MapTypeCard::kString,
              true, false};
    case WireFormatLite::TYPE_BYTES:
      return {WireFormatLite::WIRETYPE_LENGTH_DELIMITED, MapTypeCard::kString,
              false, false};

    case WireFormatLite::TYPE_MESSAGE:
      return {WireFormatLite::WIRETYPE_LENGTH_DELIMITED, MapTypeCard::kMessage,
              false, false};

    case WireFormatLite::TYPE_GROUP:
    default:
      PROTOBUF_ASSUME(false);
  }
}

enum class MapNodeSizeInfoT : uint32_t;

// Aux entry for map fields.
struct MapAuxInfo {
  MapTypeCard key_type_card;
  MapTypeCard value_type_card;
  // When off, we fall back to table->fallback to handle the parse. An example
  // of this is for DynamicMessage.
  uint8_t is_supported : 1;
  // Determines if we are using LITE or the full runtime. When using the full
  // runtime we have to synchronize with reflection before accessing the map.
  uint8_t use_lite : 1;
  // If true UTF8 errors cause the parsing to fail.
  uint8_t fail_on_utf8_failure : 1;
  // If true UTF8 errors are logged, but they are accepted.
  uint8_t log_debug_utf8_failure : 1;
  // If true the next aux contains the enum validator.
  uint8_t value_is_validated_enum : 1;
  // Size information derived from the actual node type.
  MapNodeSizeInfoT node_size_info;
};
static_assert(sizeof(MapAuxInfo) <= 8, "");

// Base class for message-level table with info for the tail-call parser.
struct alignas(uint64_t) TcParseTableBase {
  // Common attributes for message layout:
  uint16_t has_bits_offset;
  uint16_t extension_offset;
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
      uint32_t max_field_number, uint8_t fast_idx_mask,
      uint16_t lookup_table_offset, uint32_t skipmap32,
      uint32_t field_entries_offset, uint16_t num_field_entries,
      uint16_t num_aux_entries, uint32_t aux_offset,
      const MessageLite* default_instance, TailCallParseFunc fallback)
      : has_bits_offset(has_bits_offset),
        extension_offset(extension_offset),
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
    mutable std::atomic<TailCallParseFunc> target_atomic;

    // Field data used during parse:
    TcFieldData bits;

    // Default initializes this instance with undefined values.
    FastFieldEntry() = default;

    // Constant initializes this instance
    constexpr FastFieldEntry(TailCallParseFunc func, TcFieldData bits)
        : target_atomic(func), bits(bits) {}

    // FastFieldEntry is copy-able and assignable, which is intended
    // mainly for testing and debugging purposes.
    FastFieldEntry(const FastFieldEntry& rhs) noexcept
        : FastFieldEntry(rhs.target(), rhs.bits) {}
    FastFieldEntry& operator=(const FastFieldEntry& rhs) noexcept {
      SetTarget(rhs.target());
      bits = rhs.bits;
      return *this;
    }

    // Protocol buffer code should use these relaxed accessors.
    TailCallParseFunc target() const {
      return target_atomic.load(std::memory_order_relaxed);
    }
    void SetTarget(TailCallParseFunc func) const {
      return target_atomic.store(func, std::memory_order_relaxed);
    }
  };
  // There is always at least one table entry.
  const FastFieldEntry* fast_entry(size_t idx) const {
    return reinterpret_cast<const FastFieldEntry*>(this + 1) + idx;
  }
  FastFieldEntry* fast_entry(size_t idx) {
    return reinterpret_cast<FastFieldEntry*>(this + 1) + idx;
  }

  // Returns a begin iterator (pointer) to the start of the field lookup table.
  const uint16_t* field_lookup_begin() const {
    return reinterpret_cast<const uint16_t*>(reinterpret_cast<uintptr_t>(this) +
                                             lookup_table_offset);
  }
  uint16_t* field_lookup_begin() {
    return reinterpret_cast<uint16_t*>(reinterpret_cast<uintptr_t>(this) +
                                       lookup_table_offset);
  }

  // Field entry for all fields.
  struct FieldEntry {
    uint32_t offset;     // offset in the message object
    int32_t has_idx;     // has-bit index, relative to the message object
    uint16_t aux_idx;    // index for `field_aux`.
    uint16_t type_card;  // `FieldType` and `Cardinality` (see _impl.h)

    static constexpr uint16_t kNoAuxIdx = 0xFFFF;
  };

  // Returns a begin iterator (pointer) to the start of the field entries array.
  const FieldEntry* field_entries_begin() const {
    return reinterpret_cast<const FieldEntry*>(
        reinterpret_cast<uintptr_t>(this) + field_entries_offset);
  }
  FieldEntry* field_entries_begin() {
    return reinterpret_cast<FieldEntry*>(reinterpret_cast<uintptr_t>(this) +
                                         field_entries_offset);
  }

  // Auxiliary entries for field types that need extra information.
  union FieldAux {
    constexpr FieldAux() : message_default_p(nullptr) {}
    constexpr FieldAux(bool (*enum_validator)(int))
        : enum_validator(enum_validator) {}
    constexpr FieldAux(field_layout::Offset off) : offset(off.off) {}
    constexpr FieldAux(int16_t range_start, uint16_t range_length)
        : enum_range{range_start, range_length} {}
    constexpr FieldAux(const MessageLite* msg) : message_default_p(msg) {}
    constexpr FieldAux(FieldAuxDefaultMessage, const void* msg)
        : message_default_p(msg) {}
    constexpr FieldAux(const TcParseTableBase* table) : table(table) {}
    constexpr FieldAux(MapAuxInfo map_info) : map_info(map_info) {}
    constexpr FieldAux(void (*create_in_arena)(Arena*, void*))
        : create_in_arena(create_in_arena) {}
    constexpr FieldAux(LazyEagerVerifyFnType verify_func)
        : verify_func(verify_func) {}
    bool (*enum_validator)(int);
    struct {
      int16_t start;    // minimum enum number (if it fits)
      uint16_t length;  // length of range (i.e., max = start + length - 1)
    } enum_range;
    uint32_t offset;
    const void* message_default_p;
    const TcParseTableBase* table;
    MapAuxInfo map_info;
    void (*create_in_arena)(Arena*, void*);
    LazyEagerVerifyFnType verify_func;

    const MessageLite* message_default() const {
      return static_cast<const MessageLite*>(message_default_p);
    }
    const MessageLite* message_default_weak() const {
      return *static_cast<const MessageLite* const*>(message_default_p);
    }
  };
  const FieldAux* field_aux(uint32_t idx) const {
    return reinterpret_cast<const FieldAux*>(reinterpret_cast<uintptr_t>(this) +
                                             aux_offset) +
           idx;
  }
  FieldAux* field_aux(uint32_t idx) {
    return reinterpret_cast<FieldAux*>(reinterpret_cast<uintptr_t>(this) +
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
  char* name_data() {
    return reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(this) +
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
  std::array<char, kNameTableSize == 0 ? 1 : kNameTableSize> field_names;
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
  std::array<char, kNameTableSize == 0 ? 1 : kNameTableSize> field_names;
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
  std::array<char, kNameTableSize == 0 ? 1 : kNameTableSize> field_names;
};

static_assert(std::is_standard_layout<TcParseTable<1>>::value,
              "TcParseTable must be standard layout.");

static_assert(offsetof(TcParseTable<1>, fast_entries) ==
                  sizeof(TcParseTableBase),
              "Table entries must be laid out after TcParseTableBase.");

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_GENERATED_MESSAGE_TCTABLE_DECL_H__
