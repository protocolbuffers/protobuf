#ifndef GOOGLE_PROTOBUF_GENERATED_MESSAGE_TABLE_DECL_H__
#define GOOGLE_PROTOBUF_GENERATED_MESSAGE_TABLE_DECL_H__

#include <cstdint>
#include <cstdlib>
#include <limits>

#include "absl/base/attributes.h"
#include "absl/log/absl_check.h"
#include "absl/types/span.h"

namespace google {
namespace protobuf {
namespace internal {
namespace v2 {

// Field layout enums.
//
// Structural information about fields is packed into a 8-bit value. The enum
// types below represent bitwise fields, along with their respective widths,
// shifts, and masks. To pack into one byte, some mutually exclusive types share
// bits in [5, 7].
//
// <<Numeric Fields>>
//     Bit:
//     +---------------+---------------+
//     |7     ...     4|3     ...     0|
//     +---------------+---------------+
//     : . : . : . : . :  3|===========| [3] FieldKind
//     : . : . :  5|=======| . : . : . : [2] Cardinality
//     : . :  6|===| . : . : . : . : . : [1] NumericKind
//     +---------------+---------------+
//
// <<Message Fields>>
//     Bit:
//     +---------------+---------------+
//     |7     ...     4|3     ...     0|
//     +---------------+---------------+
//     : . : . : . : . :  3|===========| [3] FieldKind
//     : . : . :  5|=======| . : . : . : [2] Cardinality
//     :  7|=======| . : . : . : . : . : [2] MessageKind
//     +---------------+---------------+
//
// <<String Fields>>
//     Bit:
//     +---------------+---------------+
//     |7     ...     4|3     ...     0|
//     +---------------+---------------+
//     : . : . : . : . :  3|===========| [3] FieldKind
//     : . : . :  5|=======| . : . : . : [2] Cardinality
//     |===========| . : . : . : . : . : [3] StringKind
//     +---------------+---------------+
//

// clang-format off

// FieldKind (3 bits):
// These values broadly represent a wire type and an in-memory storage class.
namespace FieldKind {
constexpr int kShift = 0;
constexpr int kBits = 3;
constexpr int kMask = ((1 << kBits) - 1) << kShift;

enum Kinds : uint8_t {
  kFixed8 = 0,  // bool
  kFixed16,     // place holder
  kFixed32,     // (s|u)?int32, (s)?fixed32, float, enum
  kFixed64,     // (s|u)?int64, (s)?fixed64, double
  kBytes,       // bytes
  kString,      // string
  kMessage,     // group, message
  kMap,         // map<...>
};

static_assert(kMap < (1 << kBits), "too many types");
}  // namespace FieldKind

// Cardinality (2 bits):
// These values determine how many values a field can have and its presence.
namespace Cardinality {
constexpr int kShift = FieldKind::kShift + FieldKind::kBits;
constexpr int kBits = 2;
constexpr int kMask = ((1 << kBits) - 1) << kShift;

enum Kinds : uint8_t {
  kSingular = 0,
  kOptional = 1 << kShift,
  kRepeated = 2 << kShift,
  kOneof    = 3 << kShift,
};
}  // namespace Cardinality

// NumericKind, MessageKind, StringKind are mutually exclusive and share the
// same bit-space (i.e. the same shift).

// NumericKind (1 bit):
// Indicates whether a numeric is signed.
namespace NumericKind {
constexpr int kShift = Cardinality::kShift + Cardinality::kBits;
constexpr int kBits = 1;
constexpr int kMask = ((1 << kBits) - 1) << kShift;

enum Kinds : uint8_t {
  kUnsigned = 0,
  kSigned   = 1 << kShift,
};
}  // namespace NumericKind

// MessageKind (2 bits):
// Indicates if it's LazyField or eager message / group.
namespace MessageKind {
constexpr int kShift = Cardinality::kShift + Cardinality::kBits;
constexpr int kBits = 2;
constexpr int kMask = ((1 << kBits) - 1) << kShift;

enum Kinds : uint8_t {
  kEager = 0,
  kLazy  = 1 << kShift,
  kGroup = 2 << kShift,
};
}  // namespace MessageKind

// StringKind (3 bits):
// Indicates if it's LazyField or eager message / group.
namespace StringKind {
constexpr int kShift = Cardinality::kShift + Cardinality::kBits;
constexpr int kBits = 3;
constexpr int kMask = ((1 << kBits) - 1) << kShift;

enum Kinds : uint8_t {
  kArenaPtr    = 0,
  kInlined     = 1 << kShift,
  kView        = 2 << kShift,
  kCord        = 3 << kShift,
  kStringPiece = 4 << kShift,
  kStringPtr   = 5 << kShift,
};
}  // namespace StringKind

// Convenience aliases except cardinality (8 bits, with format):
enum FieldType : uint8_t {
  // Numeric types:
  kBool        = 0 | FieldKind::kFixed8  | NumericKind::kUnsigned,

  kInt32       = 0 | FieldKind::kFixed32 | NumericKind::kSigned,
  kSInt32      = 0 | FieldKind::kFixed32 | NumericKind::kSigned,
  kSFixed32    = 0 | FieldKind::kFixed32 | NumericKind::kSigned,
  kUInt32      = 0 | FieldKind::kFixed32 | NumericKind::kUnsigned,
  kFixed32     = 0 | FieldKind::kFixed32 | NumericKind::kUnsigned,
  kFloat       = 0 | FieldKind::kFixed32 | NumericKind::kUnsigned,
  kEnum        = 0 | FieldKind::kFixed32 | NumericKind::kSigned,

  kInt64       = 0 | FieldKind::kFixed64 | NumericKind::kSigned,
  kSInt64      = 0 | FieldKind::kFixed64 | NumericKind::kSigned,
  kSFixed64    = 0 | FieldKind::kFixed64 | NumericKind::kSigned,
  kUInt64      = 0 | FieldKind::kFixed64 | NumericKind::kUnsigned,
  kFixed64     = 0 | FieldKind::kFixed64 | NumericKind::kUnsigned,
  kDouble      = 0 | FieldKind::kFixed64 | NumericKind::kUnsigned,

  // String types:
  kBytes       = FieldKind::kBytes,
  kString      = FieldKind::kString,

  // Message types:
  kMessage     = 0 | FieldKind::kMessage | MessageKind::kEager,
  kLazyMessage = 0 | FieldKind::kMessage | MessageKind::kLazy,
  kGroup       = 0 | FieldKind::kMessage | MessageKind::kGroup,

  // Map types:
  kMap         = FieldKind::kMap,
};
// clang-format on

struct FieldEntry {
  // Constructors without aux index. (Should be common cases.)
  constexpr FieldEntry(uint8_t type, uint8_t hasbit_index, uint16_t offset,
                       uint16_t number)
      : field_type(type),
        hasbit_index(hasbit_index),
        offset(offset),
        field_number(number),
        aux_index(kNoAuxIdx) {}

  // If any of hasbit_index, offset, field_number is too big to fit, fallback to
  // aux entry for all.
  constexpr FieldEntry(uint8_t type, uint16_t aux_index)
      : field_type(type),
        hasbit_index(kHasbitFallbackToAux),
        offset(kFallbackToAux),
        field_number(kFallbackToAux),
        aux_index(aux_index) {}

  constexpr bool ShouldLookupAuxEntry() const { return aux_index != kNoAuxIdx; }

  uint8_t GetFieldKind() const { return field_type & FieldKind::kMask; }
  uint8_t GetCardinality() const { return field_type & Cardinality::kMask; }
  uint8_t GetNumericKind() const {
    ABSL_DCHECK_LT(GetFieldKind(), FieldKind::kBytes);
    return field_type & NumericKind::kMask;
  }
  uint8_t GetMessageKind() const {
    ABSL_DCHECK_EQ(GetFieldKind(), FieldKind::kMessage);
    return field_type & MessageKind::kMask;
  }
  uint8_t GetStringKind() const {
    ABSL_DCHECK(GetFieldKind() == FieldKind::kBytes ||
                GetFieldKind() == FieldKind::kString);
    return field_type & StringKind::kMask;
  }

  bool IsSigned() const { return GetNumericKind() == NumericKind::kSigned; }
  bool IsUTF8() const {
    ABSL_DCHECK(GetFieldKind() == FieldKind::kBytes ||
                GetFieldKind() == FieldKind::kString);
    return GetFieldKind() == FieldKind::kString;
  }

  bool IsRepeated() const { return GetCardinality() == Cardinality::kRepeated; }

  // Field type consists of FieldKind, Cardinality and type-specific Kind.
  uint8_t field_type;
  // Covers up to 256 fields. Fallback to aux if 0xFF.
  uint8_t hasbit_index;
  // Covers sizeof(Message) up to 64 KiB. Fallback to aux if 0xFFFF.
  uint16_t offset;
  // Most field numbers should fit 16 bits. Fallback to aux if 0xFFFF.
  uint16_t field_number;
  // Only up to 2^16 fallback cases are supported.
  uint16_t aux_index;

  static constexpr uint16_t kHasbitFallbackToAux = 0xFF;
  static constexpr uint16_t kFallbackToAux = 0xFFFF;
  static constexpr uint16_t kNoAuxIdx = 0xFFFF;

  // These constants are same as the above but compared against values from
  // reflection or protoc (hence different types) to determine whether to use
  // aux entries.
  static constexpr uint32_t kHasbitIdxLimit =
      std::numeric_limits<uint8_t>::max();
  static constexpr uint32_t kOffsetLimit = std::numeric_limits<uint16_t>::max();
  static constexpr int kFieldNumberLimit = std::numeric_limits<uint16_t>::max();
};

static_assert(sizeof(FieldEntry) == sizeof(uint64_t), "");

// Used instead of std::array<T, N> to special-case when N==0 to create an empty
// type. It works more effectively with [[no_unique_address]] in C++20.
template <typename T, size_t N>
struct Array {
  T array[N];

  absl::Span<T> value() { return absl::MakeSpan(array); }
  absl::Span<const T> value() const { return absl::MakeSpan(array); }
};

template <typename T>
struct Array<T, 0> {
  absl::Span<T> value() { return absl::Span<T>(); }
  absl::Span<const T> value() const { return absl::Span<const T>(); }
};

// Table-driven serialization and ByteSizeLong have different interaction with
// tables compared to table-driven parsing. While the latter walks wire-format
// data and needs to cheaply find corresponding field entry, the former can
// afford to just walk "all" present fields per message. To achieve maximum
// efficiency, a dedicated table structure is used for serialization and
// ByteSizeLong.
//
// Since messages have different shape (#fields, field types, etc.), the message
// table must be generic enough to cover all types of messages. For example,
// --extensions
// --singular, optional, repeated, map, oneof fields
// --split fields (go/pdsplit)
//
// While rare, it also has to cover the following cases:
// --huge number of fields (requires 32bit has_bit_index)
// --huge field numbers (requires 32bit field_number)
// --huge message size (requires 32bit offset)
//
// Generic tables consume bigger space not just in memory but in data cache. To
// achieve minimum cache footprint in common cases, we rely on `AuxEntry` for
// fully descriptive entries while `FieldEntry` is large enough for the common
// cases. Extending this notion, some metadata about messages (MessageTableAux)
// are isolated and often dropped when not needed. The following describes the
// most generic table (MessageTable):
//
// template <size_t kNumFields, bool kHasAuxHdr, size_t kNumAux>
// struct MessageTable {
//   MessageTableBase header;
//   Array<FieldEntry, kNumFields> field_entries;
//   MessageTableAuxImpl<kHasAuxHdr> aux_header;
//   Array<AuxEntry, kNumAux> aux_entries;
// };
//
// Note that "field_entries" are laid out in the following way (following the
// way fields are laid out in messages):
//
// --repeated fields / singular / optional fields (non-split, non-oneof)
// --split fields
// --oneof fields
//
// If header.split_field_count > 0, the following needs to happen:
// --Get split struct address from aux_header.split_offset.
// --If the address is same as aux_header.default_split_instance, just skip.
// --Otherwise, go through split field entries to handle present fields.
//
// If header.oneof_field_count > 0, the following needs to happen:
// --Get oneof_case[] from aux_header.oneof_case_offset.
// --Read field number from oneof_case[0].
// --Find a corresponding FieldEntry with the field number.
// --Repeat if there are more oneof cases.
//
// Note that FieldEntry tries to fit all required information into 64bit that
// can support the following, which should be large enough for most messsages.
// Otherwise, it falls back to AuxEntry:
// --hasbit_index up to 256. (8bit)
// --sizeof(Message) up to 64 KiB (16bit offset)
// --field_number up to 2^16
//
// To minimize cache footprints of MessageTable, we rely on
// [[no_unique_address]] attributes in C++20. When the associated type is empty
// it results in zero added bytes, which avoids specializations.
//
// For example, most simple cases where no aux_header is needed, use
// MessageTable<num_fields> the size of which becomes
// sizeof(MessageTableBase) + num_fields x sizeof(FieldEntry).
//
// If a message has split, oneof, etc., needing `aux_header` but not
// necessarily `aux_entry`, use MessageTable<num_fields, true>.
template <bool kHasAuxHdr>
struct MessageTableAuxImpl {
  uint32_t oneof_case_offset;
  uint32_t split_offset;
  void* default_split_instance;
};

template <>
struct MessageTableAuxImpl<false> {};

// Wrapper type for non-empty MessageTableAuxImpl.
using MessageTableAux = MessageTableAuxImpl<true>;

struct AuxEntry {
  uint32_t hasbit_index;
  uint32_t field_number;
  uint32_t offset;
};

struct MessageTableBase {
  constexpr MessageTableBase(uint16_t has_bits_offset,
                             uint16_t extension_offset, uint16_t field_count,
                             uint16_t oneof_field_count,
                             uint16_t split_field_count,
                             uint16_t oneof_case_count, uint32_t aux_offset)
      : has_bits_offset(has_bits_offset),
        extension_offset(extension_offset),
        field_count(field_count),
        oneof_field_count(oneof_field_count),
        split_field_count(split_field_count),
        oneof_case_count(oneof_case_count),
        aux_offset(aux_offset) {}

  // "field_entry" is immediately after `MessageTableBase` without padding bytes
  // whose offset is statically known.
  FieldEntry field_entry(size_t idx) const {
    ABSL_DCHECK_NE(field_count + oneof_field_count + split_field_count, 0);
    return *(reinterpret_cast<const FieldEntry*>(this + 1) + idx);
  }
  FieldEntry& field_entry(size_t idx) {
    ABSL_DCHECK_NE(field_count + oneof_field_count + split_field_count, 0);
    return *(reinterpret_cast<FieldEntry*>(this + 1) + idx);
  }

  // "aux_header" is after "field_entry" whose size is variable. Use cached
  // "aux_offset" to locate the field.
  const MessageTableAux* aux_header() const {
    ABSL_DCHECK_NE(aux_offset, 0u);
    return reinterpret_cast<const MessageTableAux*>(PtrAt(this, aux_offset));
  }
  MessageTableAux* aux_header() {
    ABSL_DCHECK_NE(aux_offset, 0u);
    return reinterpret_cast<MessageTableAux*>(PtrAt(this, aux_offset));
  }

  // "aux_entry" is after "aux_header" without padding bytes whose size is
  // statically known. Instead of caching, we use "aux_offset". This is
  // acceptable because there is no padding bytes between the two.
  const AuxEntry* aux_entry(size_t idx) const {
    ABSL_DCHECK_NE(aux_offset, 0u);
    return reinterpret_cast<const AuxEntry*>(
               PtrAt(this, aux_offset + sizeof(MessageTableAux))) +
           idx;
  }
  AuxEntry* aux_entry(size_t idx) {
    ABSL_DCHECK_NE(aux_offset, 0u);
    return reinterpret_cast<AuxEntry*>(
               PtrAt(this, aux_offset + sizeof(MessageTableAux))) +
           idx;
  }

  static uintptr_t PtrAt(const void* ptr, size_t offset) {
    return reinterpret_cast<uintptr_t>(ptr) + offset;
  }

  uint16_t has_bits_offset;
  uint16_t extension_offset;

  uint16_t field_count;
  uint16_t oneof_field_count;
  uint16_t split_field_count;

  // Could've been moved to MessageTableAux but why don't we make good use of
  // otherwise wasted padding bytes?
  uint16_t oneof_case_count;
  // TODO: consider repurposing `aux_offset` as it's cheap to
  // calculate the offset. (add + shift)
  uint32_t aux_offset;
};

static_assert(sizeof(MessageTableBase) == 16,
              "Must be kept compact for minimum cache footprint.");

template <size_t kNumFields, bool kHasAuxHdr = false, size_t kNumAux = 0>
struct MessageTable {
  MessageTableBase header;
  ABSL_ATTRIBUTE_NO_UNIQUE_ADDRESS Array<FieldEntry, kNumFields> field_entries;
  ABSL_ATTRIBUTE_NO_UNIQUE_ADDRESS MessageTableAuxImpl<kHasAuxHdr> aux_header;
  ABSL_ATTRIBUTE_NO_UNIQUE_ADDRESS Array<AuxEntry, kNumAux> aux_entries;
};

// A pre-built table for empty messages without any fields.
extern const MessageTable<0> kEmptyMessageTable;

}  // namespace v2
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_GENERATED_MESSAGE_TABLE_DECL_H__
