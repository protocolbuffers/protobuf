#ifndef GOOGLE_PROTOBUF_GENERATED_MESSAGE_TABLE_DECL_H__
#define GOOGLE_PROTOBUF_GENERATED_MESSAGE_TABLE_DECL_H__

#include <cstdint>
#include <limits>

#include "absl/log/absl_check.h"

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

}  // namespace v2
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_GENERATED_MESSAGE_TABLE_DECL_H__
