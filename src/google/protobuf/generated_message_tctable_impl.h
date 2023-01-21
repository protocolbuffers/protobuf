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
PROTOBUF_EXPORT void AlignFail(std::integral_constant<size_t, 4>,
                               std::uintptr_t address);
PROTOBUF_EXPORT void AlignFail(std::integral_constant<size_t, 8>,
                               std::uintptr_t address);
inline void AlignFail(std::integral_constant<size_t, 1>,
                      std::uintptr_t address) {}
#endif

// TcParser implements most of the parsing logic for tailcall tables.
class PROTOBUF_EXPORT TcParser final {
 public:
  template <typename T>
  static constexpr const TcParseTableBase* GetTable() {
    return &T::_table_.header;
  }

  // == ABI of the tail call functions ==
  // All the tail call functions have the same signature as required by clang's
  // `musttail` attribute. However, their ABIs are different.
  // See TcFieldData's comments for details on the layouts.
  // The ABIs are as follow:
  //
  //  - The following functions ignore `data`:
  //      ToTagDispatch, TagDispatch, MiniParse, ToParseLoop, Error,
  //      FastUnknownEnumFallback.
  //  - FastXXX functions expect `data` with a fast table entry ABI.
  //  - FastEndGX functions expect `data` with a non-field entry ABI.
  //  - MpXXX functions expect `data` with a mini table ABI.
  //  - The fallback functions (both GenericFallbackXXX and the codegen ones)
  //    expect only the tag in `data`. In addition, if a null `ptr` is passed,
  //    the function is used as a way to get a UnknownFieldOps vtable, returned
  //    via the `const char*` return type. See `GetUnknownFieldOps()`

  static bool MustFallbackToGeneric(PROTOBUF_TC_PARAM_DECL) {
    return ptr == nullptr;
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
  static const char* FastTV32S1(PROTOBUF_TC_PARAM_DECL);
  template <typename FieldType, int data_offset, int hasbit_idx>
  static const char* FastTV64S1(PROTOBUF_TC_PARAM_DECL);
  template <int data_offset, int hasbit_idx>
  static const char* FastTV8S1(PROTOBUF_TC_PARAM_DECL);

  template <typename FieldType, int data_offset, int hasbit_idx>
  static constexpr TailCallParseFunc SingularVarintNoZag1() {
    if (sizeof(FieldType) == 1) {
      if (data_offset < 100) {
        return &FastTV8S1<data_offset, hasbit_idx>;
      } else {
        return &FastV8S1;
      }
    }
    if (sizeof(FieldType) == 4) {
      if (data_offset < 100) {
        return &FastTV32S1<FieldType, data_offset, hasbit_idx>;
      } else {  //
        return &FastV32S1;
      }
    }
    if (sizeof(FieldType) == 8) {
      if (data_offset < 128) {
        return &FastTV64S1<FieldType, data_offset, hasbit_idx>;
      } else {
        return &FastV64S1;
      }
    }
    static_assert(sizeof(FieldType) == 1 || sizeof(FieldType) == 4 ||
                      sizeof(FieldType) == 8,
                  "");
    ABSL_LOG(FATAL) << "This should be unreachable";
  }

  // Functions referenced by generated fast tables (closed enum):
  //   E: closed enum (N.B.: open enums use V32, above)
  //   r: enum range  v: enum validator (_IsValid function)
  //   S: singular   R: repeated   P: packed
  //   1/2: tag length (bytes)
  static const char* FastErS1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastErS2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastErR1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastErR2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastErP1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastErP2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastEvS1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastEvS2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastEvR1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastEvR2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastEvP1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastEvP2(PROTOBUF_TC_PARAM_DECL);

  static const char* FastEr0S1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastEr0S2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastEr0R1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastEr0R2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastEr0P1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastEr0P2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastEr1S1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastEr1S2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastEr1R1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastEr1R2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastEr1P1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastEr1P2(PROTOBUF_TC_PARAM_DECL);

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

  static const char* FastBcS1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastBcS2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastScS1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastScS2(PROTOBUF_TC_PARAM_DECL);
  static const char* FastUcS1(PROTOBUF_TC_PARAM_DECL);
  static const char* FastUcS2(PROTOBUF_TC_PARAM_DECL);

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
      AlignFail(std::integral_constant<size_t, alignof(T)>(),
                reinterpret_cast<uintptr_t>(target));
      // Explicit abort to let compilers know this code-path does not return
      abort();
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
      AlignFail(std::integral_constant<size_t, alignof(T)>(),
                reinterpret_cast<uintptr_t>(target));
      // Explicit abort to let compilers know this code-path does not return
      abort();
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
                                 const TcParseTableBase* table);

  // Test only access to verify that the right function is being called via
  // MiniParse.
  struct TestMiniParseResult {
    TailCallParseFunc called_func;
    uint32_t tag;
    const TcParseTableBase::FieldEntry* found_entry;
    const char* ptr;
  };
  static TestMiniParseResult TestMiniParse(PROTOBUF_TC_PARAM_DECL);
  template <bool export_called_function>
  static const char* MiniParse(PROTOBUF_TC_PARAM_DECL);

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

  struct UnknownFieldOps {
    void (*write_varint)(MessageLite* msg, int number, int value);
    void (*write_length_delimited)(MessageLite* msg, int number,
                                   absl::string_view value);
  };

  static const UnknownFieldOps& GetUnknownFieldOps(
      const TcParseTableBase* table);

  template <typename UnknownFieldsT>
  static void WriteVarintToUnknown(MessageLite* msg, int number, int value) {
    internal::WriteVarint(
        number, value,
        msg->_internal_metadata_.mutable_unknown_fields<UnknownFieldsT>());
  }
  template <typename UnknownFieldsT>
  static void WriteLengthDelimitedToUnknown(MessageLite* msg, int number,
                                            absl::string_view value) {
    internal::WriteLengthDelimited(
        number, value,
        msg->_internal_metadata_.mutable_unknown_fields<UnknownFieldsT>());
  }

  template <class MessageBaseT, class UnknownFieldsT>
  static const char* GenericFallbackImpl(PROTOBUF_TC_PARAM_DECL) {
    if (PROTOBUF_PREDICT_FALSE(ptr == nullptr)) {
      // This is the ABI used by GetUnknownFieldOps(). Return the vtable.
      static constexpr UnknownFieldOps kOps = {
          WriteVarintToUnknown<UnknownFieldsT>,
          WriteLengthDelimitedToUnknown<UnknownFieldsT>};
      return reinterpret_cast<const char*>(&kOps);
    }

    SyncHasbits(msg, hasbits, table);
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
  template <typename TagType, uint16_t xform_val>
  static inline const char* PackedEnum(PROTOBUF_TC_PARAM_DECL);
  template <typename TagType, uint8_t min>
  static inline const char* RepeatedEnumSmallRange(PROTOBUF_TC_PARAM_DECL);
  template <typename TagType, uint8_t min>
  static inline const char* PackedEnumSmallRange(PROTOBUF_TC_PARAM_DECL);

  // Implementations for fast string field parsing functions:
  enum Utf8Type { kNoUtf8 = 0, kUtf8 = 1, kUtf8ValidateOnly = 2 };
  template <typename TagType, typename FieldType, Utf8Type utf8>
  static inline const char* SingularString(PROTOBUF_TC_PARAM_DECL);
  template <typename TagType, typename FieldType, Utf8Type utf8>
  static inline const char* RepeatedString(PROTOBUF_TC_PARAM_DECL);

  static inline const char* ParseRepeatedStringOnce(
      const char* ptr, Arena* arena, SerialArena* serial_arena,
      ParseContext* ctx, RepeatedPtrField<std::string>& field);

  static void UnknownPackedEnum(MessageLite* msg, const TcParseTableBase* table,
                                uint32_t tag, int32_t enum_value);

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
  friend struct FuzzPeer;
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
  static const char* MpFallback(PROTOBUF_TC_PARAM_DECL);
};

// Shift "byte" left by n * 7 bits, filling vacated bits with ones.
template <int n>
inline PROTOBUF_ALWAYS_INLINE uint64_t
shift_left_fill_with_ones(uint64_t byte, uint64_t ones) {
  return (byte << (n * 7)) | (ones >> (64 - (n * 7)));
}

// Shift "byte" left by n * 7 bits, filling vacated bits with ones, and
// put the new value in res.  Return whether the result was negative.
template <int n>
inline PROTOBUF_ALWAYS_INLINE bool shift_left_fill_with_ones_was_negative(
    uint64_t byte, uint64_t ones, int64_t& res) {
#if defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__)
  // For the first two rounds (ptr[1] and ptr[2]), micro benchmarks show a
  // substantial improvement from capturing the sign from the condition code
  // register on x86-64.
  bool sign_bit;
  asm("shldq %3, %2, %1"
      : "=@ccs"(sign_bit), "+r"(byte)
      : "r"(ones), "i"(n * 7));
  res = byte;
  return sign_bit;
#else
  // Generic fallback:
  res = shift_left_fill_with_ones<n>(byte, ones);
  return static_cast<int64_t>(res) < 0;
#endif
}

inline PROTOBUF_ALWAYS_INLINE std::pair<const char*, uint64_t>
Parse64FallbackPair(const char* p, int64_t res1) {
  auto ptr = reinterpret_cast<const int8_t*>(p);

  // The algorithm relies on sign extension for each byte to set all high bits
  // when the varint continues. It also relies on asserting all of the lower
  // bits for each successive byte read. This allows the result to be aggregated
  // using a bitwise AND. For example:
  //
  //          8       1          64     57 ... 24     17  16      9  8       1
  // ptr[0] = 1aaa aaaa ; res1 = 1111 1111 ... 1111 1111  1111 1111  1aaa aaaa
  // ptr[1] = 1bbb bbbb ; res2 = 1111 1111 ... 1111 1111  11bb bbbb  b111 1111
  // ptr[2] = 1ccc cccc ; res3 = 0000 0000 ... 000c cccc  cc11 1111  1111 1111
  //                             ---------------------------------------------
  //        res1 & res2 & res3 = 0000 0000 ... 000c cccc  ccbb bbbb  baaa aaaa
  //
  // On x86-64, a shld from a single register filled with enough 1s in the high
  // bits can accomplish all this in one instruction. It so happens that res1
  // has 57 high bits of ones, which is enough for the largest shift done.
  //
  // Just as importantly, by keeping results in res1, res2, and res3, we take
  // advantage of the superscalar abilities of the CPU.
  ABSL_DCHECK_EQ(res1 >> 7, -1);
  uint64_t ones = res1;  // save the high 1 bits from res1 (input to SHLD)
  int64_t res2, res3;    // accumulated result chunks

  if (!shift_left_fill_with_ones_was_negative<1>(ptr[1], ones, res2))
    goto done2;
  if (!shift_left_fill_with_ones_was_negative<2>(ptr[2], ones, res3))
    goto done3;

  // For the remainder of the chunks, check the sign of the AND result.
  res1 &= shift_left_fill_with_ones<3>(ptr[3], ones);
  if (res1 >= 0) goto done4;
  res2 &= shift_left_fill_with_ones<4>(ptr[4], ones);
  if (res2 >= 0) goto done5;
  res3 &= shift_left_fill_with_ones<5>(ptr[5], ones);
  if (res3 >= 0) goto done6;
  res1 &= shift_left_fill_with_ones<6>(ptr[6], ones);
  if (res1 >= 0) goto done7;
  res2 &= shift_left_fill_with_ones<7>(ptr[7], ones);
  if (res2 >= 0) goto done8;
  res3 &= shift_left_fill_with_ones<8>(ptr[8], ones);
  if (res3 >= 0) goto done9;

  // For valid 64bit varints, the 10th byte/ptr[9] should be exactly 1. In this
  // case, the continuation bit of ptr[8] already set the top bit of res3
  // correctly, so all we have to do is check that the expected case is true.
  if (PROTOBUF_PREDICT_TRUE(ptr[9] == 1)) goto done10;

  if (PROTOBUF_PREDICT_FALSE(ptr[9] & 0x80)) {
    // If the continue bit is set, it is an unterminated varint.
    return {nullptr, 0};
  }

  // A zero value of the first bit of the 10th byte represents an
  // over-serialized varint. This case should not happen, but if does (say, due
  // to a nonconforming serializer), deassert the continuation bit that came
  // from ptr[8].
  if ((ptr[9] & 1) == 0) {
#if defined(__GCC_ASM_FLAG_OUTPUTS__) && defined(__x86_64__)
    // Use a small instruction since this is an uncommon code path.
    asm("btcq $63,%0" : "+r"(res3));
#else
    res3 ^= static_cast<uint64_t>(1) << 63;
#endif
  }
  goto done10;

done2:
  return {p + 2, res1 & res2};
done3:
  return {p + 3, res1 & res2 & res3};
done4:
  return {p + 4, res1 & res2 & res3};
done5:
  return {p + 5, res1 & res2 & res3};
done6:
  return {p + 6, res1 & res2 & res3};
done7:
  return {p + 7, res1 & res2 & res3};
done8:
  return {p + 8, res1 & res2 & res3};
done9:
  return {p + 9, res1 & res2 & res3};
done10:
  return {p + 10, res1 & res2 & res3};
}

// Notes:
// 1) if data_offset is negative, it's read from data.offset()
// 2) if hasbit_idx is negative, it's read from data.hasbit_idx()
template <int data_offset, int hasbit_idx>
PROTOBUF_NOINLINE const char* TcParser::FastTV8S1(PROTOBUF_TC_PARAM_DECL) {
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
        // `& 31` avoids a compiler warning when hasbit_idx is negative.
        hasbits |= (uint64_t{1} << (hasbit_idx & 31));
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
PROTOBUF_NOINLINE const char* TcParser::FastTV64S1(PROTOBUF_TC_PARAM_DECL) {
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

  auto tmp = Parse64FallbackPair(ptr, static_cast<int8_t>(data.data >> 8));
  data.data = 0;  // Indicate to the compiler that we don't need this anymore.
  ptr = tmp.first;
  if (PROTOBUF_PREDICT_FALSE(ptr == nullptr)) {
    return Error(PROTOBUF_TC_PARAM_PASS);
  }

  RefAt<FieldType>(msg, data_offset) = static_cast<FieldType>(tmp.second);
  PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_PASS);
}

template <typename FieldType, int data_offset, int hasbit_idx>
PROTOBUF_NOINLINE const char* TcParser::FastTV32S1(PROTOBUF_TC_PARAM_DECL) {
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
            if (PROTOBUF_PREDICT_FALSE(ptr[5] & 0x80)) {
              if (PROTOBUF_PREDICT_FALSE(ptr[6] & 0x80)) {
                if (PROTOBUF_PREDICT_FALSE(ptr[7] & 0x80)) {
                  if (PROTOBUF_PREDICT_FALSE(ptr[8] & 0x80)) {
                    if (ptr[9] & 0x80) return Error(PROTOBUF_TC_PARAM_PASS);
                    *out = RotateLeft(res, 28);
                    ptr += 10;
                    PROTOBUF_MUSTTAIL return ToTagDispatch(
                        PROTOBUF_TC_PARAM_PASS);
                  }
                  *out = RotateLeft(res, 28);
                  ptr += 9;
                  PROTOBUF_MUSTTAIL return ToTagDispatch(
                      PROTOBUF_TC_PARAM_PASS);
                }
                *out = RotateLeft(res, 28);
                ptr += 8;
                PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_PASS);
              }
              *out = RotateLeft(res, 28);
              ptr += 7;
              PROTOBUF_MUSTTAIL return ToTagDispatch(PROTOBUF_TC_PARAM_PASS);
            }
            *out = RotateLeft(res, 28);
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
