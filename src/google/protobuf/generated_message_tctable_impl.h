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

#ifndef GOOGLE_PROTOBUF_GENERATED_MESSAGE_TCTABLE_IMPL_H__
#define GOOGLE_PROTOBUF_GENERATED_MESSAGE_TCTABLE_IMPL_H__

#include <cstdint>
#include <cstdlib>
#include <string>
#include <type_traits>
#include <utility>

#include "google/protobuf/port.h"
#include "google/protobuf/extension_set.h"
#include "google/protobuf/generated_message_tctable_decl.h"
#include "google/protobuf/metadata_lite.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/wire_format_lite.h"

// Must come last:
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

class Message;
class UnknownFieldSet;

namespace internal {

enum {
  kInlinedStringAuxIdx = 0,
  kSplitOffsetAuxIdx = 1,
  kSplitSizeAuxIdx = 2,
};

// Field layout enums.
//
// Structural information about fields is packed into a 16-bit value. The enum
// types below represent bitwise fields, along with their respective widths,
// shifts, and masks.
//
//     Bit:
//     +-----------------------+-----------------------+
//     |15        ..          8|7         ..          0|
//     +-----------------------+-----------------------+
//     :  .  :  .  :  .  :  .  :  .  :  .  : 3|========| [3] FieldType
//     :     :     :     :     :     :  . 4|==|  :     : [1] FieldSplit
//     :     :     :     :     :    6|=====|  .  :     : [2] FieldCardinality
//     :  .  :  .  :  .  : 9|========|  .  :  .  :  .  : [3] FieldRep
//     :     :     :11|=====|  :     :     :     :     : [2] TransformValidation
//     :  .  :13|=====|  :  .  :  .  :  .  :  .  :  .  : [2] FormatDiscriminator
//     +-----------------------+-----------------------+
//     |15        ..          8|7         ..          0|
//     +-----------------------+-----------------------+
//
namespace field_layout {
// clang-format off

// Field kind (3 bits):
// These values broadly represent a wire type and an in-memory storage class.
enum FieldKind : uint16_t {
  kFkShift = 0,
  kFkBits = 3,
  kFkMask = ((1 << kFkBits) - 1) << kFkShift,

  kFkNone = 0,
  kFkVarint,        // WT=0     rep=8,32,64 bits
  kFkPackedVarint,  // WT=2     rep=8,32,64 bits
  kFkFixed,         // WT=1,5   rep=32,64 bits
  kFkPackedFixed,   // WT=2     rep=32,64 bits
  kFkString,        // WT=2     rep=various
  kFkMessage,       // WT=2,3,4 rep=MessageLite*
  // Maps are a special case of Message, but use different parsing logic.
  kFkMap,           // WT=2     rep=Map(Lite)<various, various>
};

static_assert(kFkMap < (1 << kFkBits), "too many types");

// Split (1 bit):
enum FieldSplit : uint16_t {
  kSplitShift = kFkShift+ kFkBits,
  kSplitBits  = 1,
  kSplitMask  = ((1 << kSplitBits) - 1) << kSplitShift,

  kSplitFalse = 0,
  kSplitTrue  = 1 << kSplitShift,
};

// Cardinality (2 bits):
// These values determine how many values a field can have and its presence.
// Packed fields are represented in FieldType.
enum Cardinality : uint16_t {
  kFcShift    = kSplitShift+ kSplitBits,
  kFcBits     = 2,
  kFcMask     = ((1 << kFcBits) - 1) << kFcShift,

  kFcSingular = 0,
  kFcOptional = 1 << kFcShift,
  kFcRepeated = 2 << kFcShift,
  kFcOneof    = 3 << kFcShift,
};

// Field representation (3 bits):
// These values are the specific refinements of storage classes in FieldType.
enum FieldRep : uint16_t {
  kRepShift    = kFcShift + kFcBits,
  kRepBits     = 3,
  kRepMask     = ((1 << kRepBits) - 1) << kRepShift,

  // Numeric types (used for optional and repeated fields):
  kRep8Bits    = 0,
  kRep32Bits   = 2 << kRepShift,
  kRep64Bits   = 3 << kRepShift,
  // String types:
  kRepAString  = 0,               // ArenaStringPtr
  kRepIString  = 1 << kRepShift,  // InlinedString
  kRepCord     = 2 << kRepShift,  // absl::Cord
  kRepSPiece   = 3 << kRepShift,  // StringPieceField
  kRepSString  = 4 << kRepShift,  // std::string*
  // Message types (WT=2 unless otherwise noted):
  kRepMessage  = 0,               // MessageLite*
  kRepGroup    = 1 << kRepShift,  // MessageLite* (WT=3,4)
  kRepLazy     = 2 << kRepShift,  // LazyField*
};

// Transform/validation (2 bits):
// These values determine transforms or validation to/from wire format.
enum TransformValidation : uint16_t {
  kTvShift     = kRepShift + kRepBits,
  kTvBits      = 2,
  kTvMask      = ((1 << kTvBits) - 1) << kTvShift,

  // Varint fields:
  kTvZigZag    = 1 << kTvShift,
  kTvEnum      = 2 << kTvShift,  // validate using generated _IsValid()
  kTvRange     = 3 << kTvShift,  // validate using FieldAux::enum_range
  // String fields:
  kTvUtf8Debug = 1 << kTvShift,  // proto2
  kTvUtf8      = 2 << kTvShift,  // proto3

  // Message fields:
  kTvDefault   = 1 << kTvShift,  // Aux has default_instance*
  kTvTable     = 2 << kTvShift,  // Aux has TcParseTableBase*
  kTvWeakPtr   = 3 << kTvShift,  // Aux has default_instance** (for weak)
};

static_assert((kTvEnum & kTvRange) != 0,
              "enum validation types must share a bit");
static_assert((kTvEnum & kTvRange & kTvZigZag) == 0,
              "zigzag encoding is not enum validation");

// Format discriminators (2 bits):
enum FormatDiscriminator : uint16_t {
  kFmtShift      = kTvShift + kTvBits,
  kFmtBits       = 2,
  kFmtMask       = ((1 << kFmtBits) - 1) << kFmtShift,

  // Numeric:
  kFmtUnsigned   = 1 << kFmtShift,  // fixed, varint
  kFmtSigned     = 2 << kFmtShift,  // fixed, varint
  kFmtFloating   = 3 << kFmtShift,  // fixed
  kFmtEnum       = 3 << kFmtShift,  // varint
  // Strings:
  kFmtUtf8       = 1 << kFmtShift,  // string (proto3, enforce_utf8=true)
  kFmtUtf8Escape = 2 << kFmtShift,  // string (proto2, enforce_utf8=false)
  // Bytes:
  kFmtArray      = 1 << kFmtShift,  // bytes
  // Messages:
  kFmtShow       = 1 << kFmtShift,  // message, map
};

// Update this assertion (and comments above) when adding or removing bits:
static_assert(kFmtShift + kFmtBits == 13, "number of bits changed");

// This assertion should not change unless the storage width changes:
static_assert(kFmtShift + kFmtBits <= 16, "too many bits");

// Convenience aliases (16 bits, with format):
enum FieldType : uint16_t {
  // Numeric types:
  kBool            = 0 | kFkVarint | kRep8Bits,

  kFixed32         = 0 | kFkFixed  | kRep32Bits | kFmtUnsigned,
  kUInt32          = 0 | kFkVarint | kRep32Bits | kFmtUnsigned,
  kSFixed32        = 0 | kFkFixed  | kRep32Bits | kFmtSigned,
  kInt32           = 0 | kFkVarint | kRep32Bits | kFmtSigned,
  kSInt32          = 0 | kFkVarint | kRep32Bits | kFmtSigned | kTvZigZag,
  kFloat           = 0 | kFkFixed  | kRep32Bits | kFmtFloating,
  kEnum            = 0 | kFkVarint | kRep32Bits | kFmtEnum   | kTvEnum,
  kEnumRange       = 0 | kFkVarint | kRep32Bits | kFmtEnum   | kTvRange,
  kOpenEnum        = 0 | kFkVarint | kRep32Bits | kFmtEnum,

  kFixed64         = 0 | kFkFixed  | kRep64Bits | kFmtUnsigned,
  kUInt64          = 0 | kFkVarint | kRep64Bits | kFmtUnsigned,
  kSFixed64        = 0 | kFkFixed  | kRep64Bits | kFmtSigned,
  kInt64           = 0 | kFkVarint | kRep64Bits | kFmtSigned,
  kSInt64          = 0 | kFkVarint | kRep64Bits | kFmtSigned | kTvZigZag,
  kDouble          = 0 | kFkFixed  | kRep64Bits | kFmtFloating,

  kPackedBool      = 0 | kFkPackedVarint | kRep8Bits,

  kPackedFixed32   = 0 | kFkPackedFixed  | kRep32Bits | kFmtUnsigned,
  kPackedUInt32    = 0 | kFkPackedVarint | kRep32Bits | kFmtUnsigned,
  kPackedSFixed32  = 0 | kFkPackedFixed  | kRep32Bits | kFmtSigned,
  kPackedInt32     = 0 | kFkPackedVarint | kRep32Bits | kFmtSigned,
  kPackedSInt32    = 0 | kFkPackedVarint | kRep32Bits | kFmtSigned | kTvZigZag,
  kPackedFloat     = 0 | kFkPackedFixed  | kRep32Bits | kFmtFloating,
  kPackedEnum      = 0 | kFkPackedVarint | kRep32Bits | kFmtEnum   | kTvEnum,
  kPackedEnumRange = 0 | kFkPackedVarint | kRep32Bits | kFmtEnum   | kTvRange,
  kPackedOpenEnum  = 0 | kFkPackedVarint | kRep32Bits | kFmtEnum,

  kPackedFixed64   = 0 | kFkPackedFixed  | kRep64Bits | kFmtUnsigned,
  kPackedUInt64    = 0 | kFkPackedVarint | kRep64Bits | kFmtUnsigned,
  kPackedSFixed64  = 0 | kFkPackedFixed  | kRep64Bits | kFmtSigned,
  kPackedInt64     = 0 | kFkPackedVarint | kRep64Bits | kFmtSigned,
  kPackedSInt64    = 0 | kFkPackedVarint | kRep64Bits | kFmtSigned | kTvZigZag,
  kPackedDouble    = 0 | kFkPackedFixed  | kRep64Bits | kFmtFloating,

  // String types:
  kBytes           = 0 | kFkString | kFmtArray,
  kRawString       = 0 | kFkString | kFmtUtf8  | kTvUtf8Debug,
  kUtf8String      = 0 | kFkString | kFmtUtf8  | kTvUtf8,

  // Message types:
  kMessage         = kFkMessage,

  // Map types:
  kMap             = kFkMap,
};
// clang-format on
}  // namespace field_layout

#ifndef NDEBUG
template <size_t align>
void AlignFail(uintptr_t address) {
  GOOGLE_LOG(FATAL) << "Unaligned (" << align << ") access at " << address;

  // Explicit abort to let compilers know this function does not return
  abort();
}

extern template void AlignFail<4>(uintptr_t);
extern template void AlignFail<8>(uintptr_t);
#endif

// TcParser implements most of the parsing logic for tailcall tables.
class PROTOBUF_EXPORT TcParser final {
 public:
  template <typename T>
  static constexpr const TcParseTableBase* GetTable() {
    return &T::_table_.header;
  }

  static const char* GenericFallback(PROTOBUF_TC_PARAM_DECL);
  static const char* GenericFallbackLite(PROTOBUF_TC_PARAM_DECL);
  static const char* ReflectionFallback(PROTOBUF_TC_PARAM_DECL);
  static const char* ReflectionParseLoop(PROTOBUF_TC_PARAM_DECL);

  static const char* ParseLoop(MessageLite* msg, const char* ptr,
                               ParseContext* ctx,
                               const TcParseTableBase* table);

  // Functions referenced by generated fast tables (numeric types):
  //   F: fixed      V: varint     Z: zigzag
  //   8/32/64: storage type width (bits)
  //   S: singular   R: repeated   P: packed
  //   1/2: tag length (bytes)

  // Fixed:
  static const char* FastF32S1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastF32S2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastF32R1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastF32R2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastF32P1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastF32P2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastF64S1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastF64S2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastF64R1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastF64R2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastF64P1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastF64P2(PROTOBUF_TC_PARAM_DECL);

  // Varint:
  static const char* FastV8S1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastV8S2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastV8R1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastV8R2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastV8P1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastV8P2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastV32S1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastV32S2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastV32R1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastV32R2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastV32P1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastV32P2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastV64S1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastV64S2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastV64R1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastV64R2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastV64P1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastV64P2(PROTOBUF_TC_PARAM_DECL);

  // Varint (with zigzag):
  static const char* FastZ32S1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastZ32S2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastZ32R1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastZ32R2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastZ32P1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastZ32P2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastZ64S1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastZ64S2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastZ64R1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastZ64R2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastZ64P1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastZ64P2(PROTOBUF_TC_PARAM_DECL);

  // Manually unrolled and specialized Varint parsing.
  template <typename FieldType, int data_offset, int hasbit_idx>
  static const char* SpecializedUnrolledVImpl1(PROTOBUF_TC_PARAM_DECL);
  template <int data_offset, int hasbit_idx>
  static const char* SpecializedFastV8S1(PROTOBUF_TC_PARAM_DECL);

  template <typename FieldType, int data_offset, int hasbit_idx>
  static constexpr TailCallParseFunc SingularVarintNoZag1() {
    if (data_offset < 100) {
      if (sizeof(FieldType) == 1) {
        return &SpecializedFastV8S1<data_offset, hasbit_idx>;
      }
      return &SpecializedUnrolledVImpl1<FieldType, data_offset, hasbit_idx>;
    } else if (sizeof(FieldType) == 1) {
      return &FastV8S1;
    } else if (sizeof(FieldType) == 4) {
      return &FastV32S1;
    } else if (sizeof(FieldType) == 8) {
      return &FastV64S1;
    } else {
      static_assert(sizeof(FieldType) == 1 || sizeof(FieldType) == 4 ||
                        sizeof(FieldType) == 8,
                    "");
      return nullptr;
    }
  }

  // Functions referenced by generated fast tables (closed enum):
  //   E: closed enum (N.B.: open enums use V32, above)
  //   r: enum range  v: enum validator (_IsValid function)
  //   S: singular   R: repeated
  //   1/2: tag length (bytes)
  static const char* FastErS1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastErS2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastErR1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastErR2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastEvS1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastEvS2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastEvR1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastEvR2(PROTOBUF_TC_PARAM_DECL);

  static const char* FastEr0S1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastEr0S2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastEr0R1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastEr0R2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastEr1S1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastEr1S2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastEr1R1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastEr1R2(PROTOBUF_TC_PARAM_DECL);

  // Functions referenced by generated fast tables (string types):
  //   B: bytes      S: string     U: UTF-8 string
  //   (empty): ArenaStringPtr     i: InlinedString
  //   S: singular   R: repeated
  //   1/2: tag length (bytes)
  static const char* FastBS1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastBS2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastBR1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastBR2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastSS1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastSS2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastSR1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastSR2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastUS1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastUS2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastUR1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastUR2(PROTOBUF_TC_PARAM_DECL);

  static const char* FastBiS1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastBiS2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastSiS1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastSiS2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastUiS1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastUiS2(PROTOBUF_TC_PARAM_DECL);

  // Functions referenced by generated fast tables (message types):
  //   M: message    G: group
  //   d: default*   t: TcParseTable* (the contents of aux)
  //   S: singular   R: repeated
  //   1/2: tag length (bytes)
  static const char* FastMdS1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastMdS2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastGdS1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastGdS2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastMtS1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastMtS2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastGtS1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastGtS2(PROTOBUF_TC_PARAM_DECL);

  static const char* FastMdR1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastMdR2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastGdR1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastGdR2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastMtR1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastMtR2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastGtR1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastGtR2(PROTOBUF_TC_PARAM_DECL);

  template <typename T>
  static inline T& RefAt(void* x, size_t offset) {
    T* target = reinterpret_cast<T*>(static_cast<char*>(x) + offset);
#ifndef NDEBUG
    if (PROTOBUF_PREDICT_FALSE(
            reinterpret_cast<uintptr_t>(target) % alignof(T) != 0)) {
      AlignFail<alignof(T)>(reinterpret_cast<uintptr_t>(target));
    }
#endif
    return *target;
  }

  template <typename T>
  static inline const T& RefAt(const void* x, size_t offset) {
    const T* target =
        reinterpret_cast<const T*>(static_cast<const char*>(x) + offset);
#ifndef NDEBUG
    if (PROTOBUF_PREDICT_FALSE(
            reinterpret_cast<uintptr_t>(target) % alignof(T) != 0)) {
      AlignFail<alignof(T)>(reinterpret_cast<uintptr_t>(target));
    }
#endif
    return *target;
  }

  template <typename T>
  static inline T ReadAt(const void* x, size_t offset) {
    T out;
    memcpy(&out, static_cast<const char*>(x) + offset, sizeof(T));
    return out;
  }

  // Mini parsing:
  //
  // This function parses a field from incoming data based on metadata stored in
  // the message definition. If the field is not defined in the message, it is
  // stored in either the ExtensionSet (if applicable) or the UnknownFieldSet.
  //
  // NOTE: Currently, this function only calls the table-level fallback
  // function, so it should only be called as the fallback from fast table
  // parsing.
  static const char* MiniParse(PROTOBUF_TC_PARAM_DECL);

  static const char* FastEndG1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastEndG2(PROTOBUF_TC_PARAM_DECL);

 private:
  friend class GeneratedTcTableLiteTest;
  static void* MaybeGetSplitBase(MessageLite* msg, const bool is_split,
                                 const TcParseTableBase* table,
                                 google::protobuf::internal::ParseContext* ctx);

  template <typename TagType, bool group_coding, bool aux_is_table>
  static inline const char* SingularParseMessageAuxImpl(PROTOBUF_TC_PARAM_DECL);
  template <typename TagType, bool group_coding, bool aux_is_table>
  static inline const char* RepeatedParseMessageAuxImpl(PROTOBUF_TC_PARAM_DECL);

  template <typename TagType>
  static const char* FastEndGroupImpl(PROTOBUF_TC_PARAM_DECL);

  static inline PROTOBUF_ALWAYS_INLINE void SyncHasbits(
      MessageLite* msg, uint64_t hasbits, const TcParseTableBase* table) {
    const uint32_t has_bits_offset = table->has_bits_offset;
    if (has_bits_offset) {
      // Only the first 32 has-bits are updated. Nothing above those is stored,
      // but e.g. messages without has-bits update the upper bits.
      RefAt<uint32_t>(msg, has_bits_offset) |= static_cast<uint32_t>(hasbits);
    }
  }

  static const char* TagDispatch(PROTOBUF_TC_PARAM_DECL);
  static const char* ToTagDispatch(PROTOBUF_TC_PARAM_DECL);
  static const char* ToParseLoop(PROTOBUF_TC_PARAM_DECL);
  static const char* Error(PROTOBUF_TC_PARAM_DECL);

  static const char* FastUnknownEnumFallback(PROTOBUF_TC_PARAM_DECL);

  class ScopedArenaSwap;

  template <class MessageBaseT, class UnknownFieldsT>
  static const char* GenericFallbackImpl(PROTOBUF_TC_PARAM_DECL) {
#define CHK_(x) \
  if (PROTOBUF_PREDICT_FALSE(!(x))) return nullptr /* NOLINT */

    SyncHasbits(msg, hasbits, table);
    CHK_(ptr);
    uint32_t tag = data.tag();
    if ((tag & 7) == WireFormatLite::WIRETYPE_END_GROUP || tag == 0) {
      ctx->SetLastTag(tag);
      return ptr;
    }
    uint32_t num = tag >> 3;
    if (table->extension_range_low <= num &&
        num <= table->extension_range_high) {
      return RefAt<ExtensionSet>(msg, table->extension_offset)
          .ParseField(tag, ptr,
                      static_cast<const MessageBaseT*>(table->default_instance),
                      &msg->_internal_metadata_, ctx);
    }
    return UnknownFieldParse(
        tag, msg->_internal_metadata_.mutable_unknown_fields<UnknownFieldsT>(),
        ptr, ctx);
#undef CHK_
  }

  // Note: `inline` is needed on template function declarations below to avoid
  // -Wattributes diagnostic in GCC.

  // Implementations for fast fixed field parsing functions:
  template <typename LayoutType, typename TagType>
  static inline const char* SingularFixed(PROTOBUF_TC_PARAM_DECL);
  template <typename LayoutType, typename TagType>
  static inline const char* RepeatedFixed(PROTOBUF_TC_PARAM_DECL);
  template <typename LayoutType, typename TagType>
  static inline const char* PackedFixed(PROTOBUF_TC_PARAM_DECL);

  // Implementations for fast varint field parsing functions:
  template <typename FieldType, typename TagType, bool zigzag = false>
  static inline const char* SingularVarint(PROTOBUF_TC_PARAM_DECL);
  template <typename FieldType, typename TagType, bool zigzag = false>
  static inline const char* RepeatedVarint(PROTOBUF_TC_PARAM_DECL);
  template <typename FieldType, typename TagType, bool zigzag = false>
  static inline const char* PackedVarint(PROTOBUF_TC_PARAM_DECL);

  // Helper for ints > 127:
  template <typename FieldType, typename TagType, bool zigzag = false>
  static const char* SingularVarBigint(PROTOBUF_TC_PARAM_DECL);

  // Implementations for fast enum field parsing functions:
  template <typename TagType, uint16_t xform_val>
  static inline const char* SingularEnum(PROTOBUF_TC_PARAM_DECL);
  template <typename TagType, uint8_t min>
  static inline const char* SingularEnumSmallRange(PROTOBUF_TC_PARAM_DECL);
  template <typename TagType, uint16_t xform_val>
  static inline const char* RepeatedEnum(PROTOBUF_TC_PARAM_DECL);
  template <typename TagType, uint8_t min>
  static inline const char* RepeatedEnumSmallRange(PROTOBUF_TC_PARAM_DECL);

  // Implementations for fast string field parsing functions:
  enum Utf8Type { kNoUtf8 = 0, kUtf8 = 1, kUtf8ValidateOnly = 2 };
  template <typename TagType, Utf8Type utf8>
  static inline const char* SingularString(PROTOBUF_TC_PARAM_DECL);
  template <typename TagType, Utf8Type utf8>
  static inline const char* RepeatedString(PROTOBUF_TC_PARAM_DECL);

  static inline const char* ParseRepeatedStringOnce(
      const char* ptr, Arena* arena, SerialArena* serial_arena,
      ParseContext* ctx, RepeatedPtrField<std::string>& field);

  // Mini field lookup:
  static const TcParseTableBase::FieldEntry* FindFieldEntry(
      const TcParseTableBase* table, uint32_t field_num);
  static absl::string_view MessageName(const TcParseTableBase* table);
  static absl::string_view FieldName(const TcParseTableBase* table,
                                     const TcParseTableBase::FieldEntry*);
  static bool ChangeOneof(const TcParseTableBase* table,
                          const TcParseTableBase::FieldEntry& entry,
                          uint32_t field_num, ParseContext* ctx,
                          MessageLite* msg);

  // UTF-8 validation:
  static void ReportFastUtf8Error(uint32_t decoded_tag,
                                  const TcParseTableBase* table);
  static bool MpVerifyUtf8(absl::string_view wire_bytes,
                           const TcParseTableBase* table,
                           const TcParseTableBase::FieldEntry& entry,
                           uint16_t xform_val);

  // For FindFieldEntry tests:
  friend class FindFieldEntryTest;
  friend struct ParseFunctionGeneratorTestPeer;
  static constexpr const uint32_t kMtSmallScanSize = 4;

  // Mini parsing:
  template <bool is_split>
  static const char* MpVarint(PROTOBUF_TC_PARAM_DECL);
  static const char* MpRepeatedVarint(PROTOBUF_TC_PARAM_DECL);
  static const char* MpPackedVarint(PROTOBUF_TC_PARAM_DECL);
  template <bool is_split>
  static const char* MpFixed(PROTOBUF_TC_PARAM_DECL);
  static const char* MpRepeatedFixed(PROTOBUF_TC_PARAM_DECL);
  static const char* MpPackedFixed(PROTOBUF_TC_PARAM_DECL);
  template <bool is_split>
  static const char* MpString(PROTOBUF_TC_PARAM_DECL);
  static const char* MpRepeatedString(PROTOBUF_TC_PARAM_DECL);
  template <bool is_split>
  static const char* MpMessage(PROTOBUF_TC_PARAM_DECL);
  static const char* MpRepeatedMessage(PROTOBUF_TC_PARAM_DECL);
  static const char* MpMap(PROTOBUF_TC_PARAM_DECL);
};

// Notes:
// 1) if data_offset is negative, it's read from data.offset()
// 2) if hasbit_idx is negative, it's read from data.hasbit_idx()
template <int data_offset, int hasbit_idx>
const char* TcParser::SpecializedFastV8S1(PROTOBUF_TC_PARAM_DECL) {
  using TagType = uint8_t;

  // Special case for a varint bool field with a tag of 1 byte:
  // The coded_tag() field will actually contain the value too and we can check
  // both at the same time.
  auto coded_tag = data.coded_tag<uint16_t>();
  if (PROTOBUF_PREDICT_TRUE(coded_tag == 0x0000 || coded_tag == 0x0100)) {
    auto& field =
        RefAt<bool>(msg, data_offset >= 0 ? data_offset : data.offset());
    // Note: we use `data.data` because Clang generates suboptimal code when
    // using coded_tag.
    // In x86_64 this uses the CH register to read the second byte out of
    // `data`.
    uint8_t value = data.data >> 8;
    // The assume allows using a mov instead of test+setne.
    PROTOBUF_ASSUME(value <= 1);
    field = static_cast<bool>(value);

    ptr += sizeof(TagType) + 1;  // Consume the tag and the value.
    if (hasbit_idx < 0) {
      hasbits |= (uint64_t{1} << data.hasbit_idx());
    } else {
      if (hasbit_idx < 32) {
        hasbits |= (uint64_t{1} << hasbit_idx);
      } else {
        static_assert(hasbit_idx == 63 || (hasbit_idx < 32),
                      "hard-coded hasbit_idx should be 0-31, or the special"
                      "value 63, which indicates the field has no has-bit.");
        // TODO(jorg): investigate whether higher hasbit indices are worth
        // supporting. Something like:
        // auto& hasblock = TcParser::RefAt<uint32_t>(msg, hasbit_idx / 32 * 4);
        // hasblock |= uint32_t{1} << (hasbit_idx % 32);
      }
    }

    PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_PASS);
  }

  // If it didn't match above either the tag is wrong, or the value is encoded
  // non-canonically.
  // Jump to MiniParse as wrong tag is the most probable reason.
  PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_PASS);
}

template <typename FieldType, int data_offset, int hasbit_idx>
const char* TcParser::SpecializedUnrolledVImpl1(PROTOBUF_TC_PARAM_DECL) {
  using TagType = uint8_t;
  // super-early success test...
  if (PROTOBUF_PREDICT_TRUE(((data.data) & 0x80FF) == 0)) {
    ptr += sizeof(TagType);  // Consume tag
    if (hasbit_idx < 32) {
      hasbits |= (uint64_t{1} << hasbit_idx);
    }
    uint8_t value = data.data >> 8;
    RefAt<FieldType>(msg, data_offset) = value;
    ptr += 1;
    PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_PASS);
  }
  if (PROTOBUF_PREDICT_FALSE(data.coded_tag<TagType>() != 0)) {
    PROTOBUF_MUSTTAIL return MiniParse(PROTOBUF_TC_PARAM_PASS);
  }
  ptr += sizeof(TagType);  // Consume tag
  if (hasbit_idx < 32) {
    hasbits |= (uint64_t{1} << hasbit_idx);
  }

  // Few registers
  auto* out = &RefAt<FieldType>(msg, data_offset);
  uint64_t res = 0xFF & (data.data >> 8);
  /* if (PROTOBUF_PREDICT_FALSE(res & 0x80)) */ {
    res = RotRight7AndReplaceLowByte(res, ptr[1]);
    if (PROTOBUF_PREDICT_FALSE(res & 0x80)) {
      res = RotRight7AndReplaceLowByte(res, ptr[2]);
      if (PROTOBUF_PREDICT_FALSE(res & 0x80)) {
        res = RotRight7AndReplaceLowByte(res, ptr[3]);
        if (PROTOBUF_PREDICT_FALSE(res & 0x80)) {
          res = RotRight7AndReplaceLowByte(res, ptr[4]);
          if (PROTOBUF_PREDICT_FALSE(res & 0x80)) {
            res = RotRight7AndReplaceLowByte(res, ptr[5]);
            if (PROTOBUF_PREDICT_FALSE(res & 0x80)) {
              res = RotRight7AndReplaceLowByte(res, ptr[6]);
              if (PROTOBUF_PREDICT_FALSE(res & 0x80)) {
                res = RotRight7AndReplaceLowByte(res, ptr[7]);
                if (PROTOBUF_PREDICT_FALSE(res & 0x80)) {
                  res = RotRight7AndReplaceLowByte(res, ptr[8]);
                  if (PROTOBUF_PREDICT_FALSE(res & 0x80)) {
                    if (ptr[9] & 0xFE) return nullptr;
                    res = RotateLeft(res, -7) & ~1;
                    res += ptr[9] & 1;
                    *out = RotateLeft(res, 63);
                    ptr += 10;
                    PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_PASS);
                  }
                  *out = RotateLeft(res, 56);
                  ptr += 9;
                  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_PASS);
                }
                *out = RotateLeft(res, 49);
                ptr += 8;
                PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_PASS);
              }
              *out = RotateLeft(res, 42);
              ptr += 7;
              PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_PASS);
            }
            *out = RotateLeft(res, 35);
            ptr += 6;
            PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_PASS);
          }
          *out = RotateLeft(res, 28);
          ptr += 5;
          PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_PASS);
        }
        *out = RotateLeft(res, 21);
        ptr += 4;
        PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_PASS);
      }
      *out = RotateLeft(res, 14);
      ptr += 3;
      PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_PASS);
    }
    *out = RotateLeft(res, 7);
    ptr += 2;
    PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_PASS);
  }
  *out = res;
  ptr += 1;
  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_PASS);
}

// Dispatch to the designated parse function
inline PROTOBUF_ALWAYS_INLINE const char* TcParser::TagDispatch(
    PROTOBUF_TC_PARAM_DECL) {
  const auto coded_tag = UnalignedLoad<uint16_t>(ptr);
  const size_t idx = coded_tag & table->fast_idx_mask;
  PROTOBUF_ASSUME((idx & 7) == 0);
  auto* fast_entry = table->fast_entry(idx >> 3);
  data = fast_entry->bits;
  data.data ^= coded_tag;
  PROTOBUF_MUSTTAIL return fast_entry->target()(PROTOBUF_TC_PARAM_PASS);
}

// We can only safely call from field to next field if the call is optimized
// to a proper tail call. Otherwise we blow through stack. Clang and gcc
// reliably do this optimization in opt mode, but do not perform this in debug
// mode. Luckily the structure of the algorithm is such that it's always
// possible to just return and use the enclosing parse loop as a trampoline.
inline PROTOBUF_ALWAYS_INLINE const char* TcParser::ToTagDispatch(
    PROTOBUF_TC_PARAM_DECL) {
  constexpr bool always_return = !PROTOBUF_TAILCALL;
  if (always_return || !ctx->DataAvailable(ptr)) {
    PROTOBUF_MUSTTAIL return ToParseLoop(PROTOBUF_TC_PARAM_PASS);
  }
  PROTOBUF_MUSTTAIL return TagDispatch(PROTOBUF_TC_PARAM_PASS);
}

inline PROTOBUF_ALWAYS_INLINE const char* TcParser::ToParseLoop(
    PROTOBUF_TC_PARAM_DECL) {
  (void)data;
  (void)ctx;
  SyncHasbits(msg, hasbits, table);
  return ptr;
}

inline PROTOBUF_ALWAYS_INLINE const char* TcParser::Error(
    PROTOBUF_TC_PARAM_DECL) {
  (void)data;
  (void)ctx;
  (void)ptr;
  SyncHasbits(msg, hasbits, table);
  return nullptr;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_GENERATED_MESSAGE_TCTABLE_IMPL_H__
