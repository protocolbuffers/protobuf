// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_GENERATED_MESSAGE_TCTABLE_IMPL_H__
#define GOOGLE_PROTOBUF_GENERATED_MESSAGE_TCTABLE_IMPL_H__

#include <cstdint>
#include <cstdlib>
#include <string>
#include <type_traits>
#include <utility>

#include "absl/log/absl_log.h"
#include "google/protobuf/extension_set.h"
#include "google/protobuf/generated_message_tctable_decl.h"
#include "google/protobuf/map.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/metadata_lite.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/port.h"
#include "google/protobuf/raw_ptr.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/repeated_ptr_field.h"
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
  kTvEnum      = 2 << kTvShift,  // validate using ValidateEnum()
  kTvRange     = 3 << kTvShift,  // validate using FieldAux::enum_range
  // String fields:
  kTvUtf8Debug = 1 << kTvShift,  // proto2
  kTvUtf8      = 2 << kTvShift,  // proto3

  // Message fields:
  kTvDefault   = 1 << kTvShift,  // Aux has default_instance*
  kTvTable     = 2 << kTvShift,  // Aux has TcParseTableBase*
  kTvWeakPtr   = 3 << kTvShift,  // Aux has default_instance** (for weak)

  // Lazy message fields:
  kTvEager     = 1 << kTvShift,
  kTvLazy      = 2 << kTvShift,
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
  static const char* MiniParseLoop(MessageLite* msg, const char* ptr, ParseContext* ctx, const TcParseTableBase* table, int64_t delta_or_group = -1);
  static const char* MiniParseFallback(MessageLite* msg, const char* ptr, ParseContext* ctx, const TcParseTableBase* table, const void* entry, uint32_t tag);
  static const char* ParseLoop(MessageLite* msg, const char* ptr, ParseContext* ctx, const TcParseTableBase* table) {
    return MiniParseLoop(msg, ptr, ctx, table, -1);
  }

  template <typename T>
  static constexpr auto GetTable() -> decltype(&T::_table_.header) {
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

  static const char* GenericFallback(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data);
  static const char* GenericFallbackLite(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data);
  static const char* ReflectionFallback(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data);

  // NOTE: Do not dedup RefAt by having one call the other with a const_cast. It
  // causes ICEs of gcc 7.5.
  // https://github.com/protocolbuffers/protobuf/issues/13715
  template <typename T>
  static inline T& RefAt(void* x, size_t offset) {
    T* target = reinterpret_cast<T*>(static_cast<char*>(x) + offset);
#if !defined(NDEBUG) && !(defined(_MSC_VER) && defined(_M_IX86))
    // Check the alignment in debug mode, except in 32-bit msvc because it does
    // not respect the alignment as expressed by `alignof(T)`
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
#if !defined(NDEBUG) && !(defined(_MSC_VER) && defined(_M_IX86))
    // Check the alignment in debug mode, except in 32-bit msvc because it does
    // not respect the alignment as expressed by `alignof(T)`
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

  template <typename T, bool is_split>
  static inline T& MaybeCreateRepeatedRefAt(void* x, size_t offset,
                                            MessageLite* msg) {
    if (!is_split) return RefAt<T>(x, offset);
    void*& ptr = RefAt<void*>(x, offset);
    if (ptr == DefaultRawPtr()) {
      ptr = Arena::CreateMessage<T>(msg->GetArena());
    }
    return *static_cast<T*>(ptr);
  }

  template <typename T, bool is_split>
  static inline RepeatedField<T>& MaybeCreateRepeatedFieldRefAt(
      void* x, size_t offset, MessageLite* msg) {
    return MaybeCreateRepeatedRefAt<RepeatedField<T>, is_split>(x, offset, msg);
  }

  template <typename T, bool is_split>
  static inline RepeatedPtrField<T>& MaybeCreateRepeatedPtrFieldRefAt(
      void* x, size_t offset, MessageLite* msg) {
    return MaybeCreateRepeatedRefAt<RepeatedPtrField<T>, is_split>(x, offset,
                                                                   msg);
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
  PROTOBUF_NOINLINE
  static const char* MiniParse(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data);

  // For `map` mini parsing generate a type card for the key/value.
  template <typename MapField>
  static constexpr MapAuxInfo GetMapAuxInfo(bool fail_on_utf8_failure,
                                            bool log_debug_utf8_failure,
                                            bool validated_enum_value,
                                            int key_type, int value_type) {
    using MapType = typename MapField::MapType;
    using Node = typename MapType::Node;
    static_assert(alignof(Node) == alignof(NodeBase), "");
    // Verify the assumption made in MpMap, guaranteed by Map<>.
    assert(PROTOBUF_FIELD_OFFSET(Node, kv.first) == sizeof(NodeBase));
    return {
        MakeMapTypeCard(static_cast<WireFormatLite::FieldType>(key_type)),
        MakeMapTypeCard(static_cast<WireFormatLite::FieldType>(value_type)),
        true,
        !std::is_base_of<MapFieldBaseForParse, MapField>::value,
        fail_on_utf8_failure,
        log_debug_utf8_failure,
        validated_enum_value,
        Node::size_info(),
    };
  }

  template <typename T>
  static void CreateInArenaStorageCb(Arena* arena, void* p) {
    Arena::CreateInArenaStorage(static_cast<T*>(p), arena);
  }

 private:
  friend class GeneratedTcTableLiteTest;
  static void* MaybeGetSplitBase(MessageLite* msg, bool is_split,
                                 const TcParseTableBase* table);

  // Test only access to verify that the right function is being called via
  // MiniParse.
  struct TestMiniParseResult {
    TailCallParseFunc called_func;
    uint32_t tag;
    const TcParseTableBase::FieldEntry* found_entry;
    const char* ptr;
  };
  PROTOBUF_NOINLINE
  static TestMiniParseResult TestMiniParse(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data);
  template <bool export_called_function>
  static const char* MiniParse(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data);

  PROTOBUF_NOINLINE
  static const char* MpUnknownEnumFallback(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data);

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
  static const char* GenericFallbackImpl(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data) {
    if (PROTOBUF_PREDICT_FALSE(ptr == nullptr)) {
      // This is the ABI used by GetUnknownFieldOps(). Return the vtable.
      static constexpr UnknownFieldOps kOps = {
          WriteVarintToUnknown<UnknownFieldsT>,
          WriteLengthDelimitedToUnknown<UnknownFieldsT>};
      return reinterpret_cast<const char*>(&kOps);
    }

    uint32_t tag = data.tag();
    if ((tag & 7) == WireFormatLite::WIRETYPE_END_GROUP || tag == 0) {
      ctx->SetLastTag(tag);
      return ptr;
    }

    if ((table->extension_offset & ~TcParseTableBase::kExtensionMask) != 0) {
      // We don't need to check the extension ranges. If it is not an extension
      // it will be handled just like if it was an unknown extension: sent to
      // the unknown field set.
      return RefAt<ExtensionSet>(msg, (table->extension_offset & ~TcParseTableBase::kExtensionMask))
          .ParseField(tag, ptr,
                      static_cast<const MessageBaseT*>(table->default_instance),
                      &msg->_internal_metadata_, ctx);
    } else {
      // Otherwise, we directly put it on the unknown field set.
      return UnknownFieldParse(
          tag,
          msg->_internal_metadata_.mutable_unknown_fields<UnknownFieldsT>(),
          ptr, ctx);
    }
  }

  static inline const char* ParseRepeatedStringOnce(
      const char* ptr, SerialArena* serial_arena, ParseContext* ctx,
      RepeatedPtrField<std::string>& field);

  PROTOBUF_NOINLINE
  static void AddUnknownEnum(MessageLite* msg, const TcParseTableBase* table,
                             uint32_t tag, int32_t enum_value);

  static void WriteMapEntryAsUnknown(MessageLite* msg,
                                     const TcParseTableBase* table,
                                     uint32_t tag, NodeBase* node,
                                     MapAuxInfo map_info);

  static void InitializeMapNodeEntry(void* obj, MapTypeCard type_card,
                                     UntypedMapBase& map,
                                     const TcParseTableBase::FieldAux* aux,
                                     bool is_key);
  PROTOBUF_NOINLINE
  static void DestroyMapNode(NodeBase* node, MapAuxInfo map_info,
                             UntypedMapBase& map);
  static const char* ParseOneMapEntry(NodeBase* node, const char* ptr,
                                      ParseContext* ctx,
                                      const TcParseTableBase::FieldAux* aux,
                                      const TcParseTableBase* table,
                                      const TcParseTableBase::FieldEntry& entry,
                                      Arena* arena);

  // Mini field lookup:
  static const TcParseTableBase::FieldEntry* FindFieldEntry(
      const TcParseTableBase* table, uint32_t field_num);
  static absl::string_view MessageName(const TcParseTableBase* table);
  static absl::string_view FieldName(const TcParseTableBase* table,
                                     const TcParseTableBase::FieldEntry*);
  static bool ChangeOneof(const TcParseTableBase* table,
                          const TcParseTableBase::FieldEntry entry,
                          uint32_t field_num, ParseContext* ctx,
                          MessageLite* msg);

  // UTF-8 validation:
  static void ReportFastUtf8Error(uint32_t decoded_tag,
                                  const TcParseTableBase* table);
  static bool MpVerifyUtf8(absl::string_view wire_bytes,
                           const TcParseTableBase* table,
                           const TcParseTableBase::FieldEntry& entry,
                           uint16_t xform_val);
  static bool MpVerifyUtf8(const absl::Cord& wire_bytes,
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
  PROTOBUF_NOINLINE static const char* MpVarint(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data);
  template <bool is_split>
  PROTOBUF_NOINLINE static const char* MpRepeatedVarint(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data);
  template <bool is_split, typename FieldType, uint16_t xform_val>
  static const char* MpRepeatedVarintT(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data);
  template <bool is_split>
  PROTOBUF_NOINLINE static const char* MpPackedVarint(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data);
  template <bool is_split, typename FieldType, uint16_t xform_val>
  static const char* MpPackedVarintT(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data);
  template <bool is_split>
  PROTOBUF_NOINLINE static const char* MpFixed(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data);
  template <bool is_split>
  PROTOBUF_NOINLINE static const char* MpRepeatedFixed(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data);
  template <bool is_split>
  PROTOBUF_NOINLINE static const char* MpPackedFixed(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data);
  template <bool is_split>
  PROTOBUF_NOINLINE static const char* MpString(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data);
  template <bool is_split>
  PROTOBUF_NOINLINE static const char* MpRepeatedString(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data);
  template <bool is_split>
  PROTOBUF_NOINLINE static const char* MpMessage(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data);
  template <bool is_split, bool is_group>
  static const char* MpRepeatedMessageOrGroup(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data);
  static const char* MpLazyMessage(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data);
  PROTOBUF_NOINLINE static const char* MpFallback(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data);
  template <bool is_split>
  PROTOBUF_NOINLINE static const char* MpMap(MessageLite *msg, const char *ptr, ParseContext *ctx, const TcParseTableBase *table, TcFieldData data);
};

// Prints the type card as or of labels, using known higher level labels.
// Used for code generation, but also useful for debugging.
PROTOBUF_EXPORT std::string TypeCardToString(uint16_t type_card);

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_GENERATED_MESSAGE_TCTABLE_IMPL_H__
