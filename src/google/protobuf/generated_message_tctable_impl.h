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
#include <type_traits>

#include <google/protobuf/parse_context.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/generated_message_tctable_decl.h>
#include <google/protobuf/message_lite.h>
#include <google/protobuf/metadata_lite.h>
#include <google/protobuf/port.h>
#include <google/protobuf/wire_format_lite.h>

// Must come last:
#include <google/protobuf/port_def.inc>

namespace google {
namespace protobuf {

class Message;
class UnknownFieldSet;

namespace internal {

// PROTOBUF_TC_PARAM_DECL are the parameters for tailcall functions.
//
// Note that this is performance sensitive: changing the parameters will change
// the registers used by the ABI calling convention, which subsequently affects
// register selection logic inside the function.
#define PROTOBUF_TC_PARAM_DECL                                 \
  ::google::protobuf::MessageLite *msg, const char *ptr,                 \
      ::google::protobuf::internal::ParseContext *ctx,                   \
      const ::google::protobuf::internal::TailCallParseTableBase *table, \
      uint64_t hasbits, ::google::protobuf::internal::TcFieldData data

// PROTOBUF_TC_PARAM_PASS passes values to match PROTOBUF_TC_PARAM_DECL.
#define PROTOBUF_TC_PARAM_PASS msg, ptr, ctx, table, hasbits, data

class TcParserBase {
 public:
  static const char* GenericFallback(PROTOBUF_TC_PARAM_DECL);
  static const char* GenericFallbackLite(PROTOBUF_TC_PARAM_DECL);

  template <typename FieldType, typename TagType>
  PROTOBUF_NOINLINE static const char* SingularParseMessage(
      PROTOBUF_TC_PARAM_DECL) {
    if (PROTOBUF_PREDICT_FALSE(static_cast<TagType>(data.coded_tag()) != 0)) {
      return table->fallback(PROTOBUF_TC_PARAM_PASS);
    }
    ptr += sizeof(TagType);
    hasbits |= (uint64_t{1} << data.hasbit_idx());
    auto& field = RefAt<FieldType*>(msg, data.offset());
    if (field == nullptr) {
      auto arena = ctx->data().arena;
      if (Arena::is_arena_constructable<FieldType>::value) {
        field = Arena::CreateMessage<FieldType>(arena);
      } else {
        field = Arena::Create<FieldType>(arena);
      }
    }
    SyncHasbits(msg, hasbits, table);
    return ctx->ParseMessage(field, ptr);
  }

  template <typename FieldType, typename TagType>
  PROTOBUF_NOINLINE static const char* RepeatedParseMessage(
      PROTOBUF_TC_PARAM_DECL) {
    if (PROTOBUF_PREDICT_FALSE(static_cast<TagType>(data.coded_tag()) != 0)) {
      return table->fallback(PROTOBUF_TC_PARAM_PASS);
    }
    ptr += sizeof(TagType);
    auto& field = RefAt<RepeatedPtrField<FieldType>>(msg, data.offset());
    SyncHasbits(msg, hasbits, table);
    ptr = ctx->ParseMessage(field.Add(), ptr);
    return ptr;
  }

  template <typename LayoutType, typename TagType>
  static const char* RepeatedFixed(PROTOBUF_TC_PARAM_DECL);
  template <typename LayoutType, typename TagType>
  static const char* PackedFixed(PROTOBUF_TC_PARAM_DECL);

  enum VarintDecode { kNoConversion = 0, kZigZag = 1 };
  template <typename FieldType, typename TagType, VarintDecode zigzag>
  static const char* RepeatedVarint(PROTOBUF_TC_PARAM_DECL);
  template <typename FieldType, typename TagType, VarintDecode zigzag>
  static const char* PackedVarint(PROTOBUF_TC_PARAM_DECL);

  enum Utf8Type { kNoUtf8 = 0, kUtf8 = 1, kUtf8ValidateOnly = 2 };
  template <typename TagType, Utf8Type utf8>
  static const char* SingularString(PROTOBUF_TC_PARAM_DECL);
  template <typename TagType, Utf8Type utf8>
  static const char* RepeatedString(PROTOBUF_TC_PARAM_DECL);

 protected:
  template <typename T>
  static T& RefAt(void* x, size_t offset) {
    T* target = reinterpret_cast<T*>(static_cast<char*>(x) + offset);
    GOOGLE_DCHECK_EQ(0, reinterpret_cast<uintptr_t>(target) % alignof(T));
    return *target;
  }

  static inline PROTOBUF_ALWAYS_INLINE void SyncHasbits(
      MessageLite* msg, uint64_t hasbits, const TailCallParseTableBase* table) {
    const uint32_t has_bits_offset = table->has_bits_offset;
    if (has_bits_offset) {
      // Only the first 32 has-bits are updated. Nothing above those is stored,
      // but e.g. messages without has-bits update the upper bits.
      RefAt<uint32_t>(msg, has_bits_offset) = static_cast<uint32_t>(hasbits);
    }
  }

  static inline PROTOBUF_ALWAYS_INLINE const char* Return(
      PROTOBUF_TC_PARAM_DECL) {
    SyncHasbits(msg, hasbits, table);
    return ptr;
  }

  static inline PROTOBUF_ALWAYS_INLINE const char* Error(
      PROTOBUF_TC_PARAM_DECL) {
    SyncHasbits(msg, hasbits, table);
    return nullptr;
  }

  class ScopedArenaSwap final {
   public:
    ScopedArenaSwap(MessageLite* msg, ParseContext* ctx)
        : ctx_(ctx), saved_(ctx->data().arena) {
      ctx_->data().arena = msg->GetArenaForAllocation();
    }
    ScopedArenaSwap(const ScopedArenaSwap&) = delete;
    ~ScopedArenaSwap() { ctx_->data().arena = saved_; }

   private:
    ParseContext* const ctx_;
    Arena* const saved_;
  };

  template <class MessageBaseT, class UnknownFieldsT>
  static const char* GenericFallbackImpl(PROTOBUF_TC_PARAM_DECL) {
#define CHK_(x) \
  if (PROTOBUF_PREDICT_FALSE(!(x))) return nullptr /* NOLINT */

    SyncHasbits(msg, hasbits, table);
    uint32_t tag;
    ptr = ::PROTOBUF_NAMESPACE_ID::internal::ReadTag(ptr, &tag);
    CHK_(ptr);
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
};

// TcParser implements most of the parsing logic for tailcall tables.
//
// This is templated on lg2(table size), since dispatching depends upon the size
// of the table. The template parameter avoids runtime overhead for computing
// the table entry index.
template <uint32_t kPowerOf2>
struct TcParser final : TcParserBase {
  // Dispatch to the designated parse function
  inline PROTOBUF_ALWAYS_INLINE static const char* TagDispatch(
      PROTOBUF_TC_PARAM_DECL) {
    const auto coded_tag = UnalignedLoad<uint16_t>(ptr);
    constexpr size_t kIdxMask = ((1 << (kPowerOf2)) - 1);
    const size_t idx = (coded_tag >> 3) & kIdxMask;
    data = table->table()[idx].bits;
    data.data ^= coded_tag;
    PROTOBUF_MUSTTAIL return table->table()[idx].target(PROTOBUF_TC_PARAM_PASS);
  }

  // We can only safely call from field to next field if the call is optimized
  // to a proper tail call. Otherwise we blow through stack. Clang and gcc
  // reliably do this optimization in opt mode, but do not perform this in debug
  // mode. Luckily the structure of the algorithm is such that it's always
  // possible to just return and use the enclosing parse loop as a trampoline.
  static const char* TailCall(PROTOBUF_TC_PARAM_DECL) {
    constexpr bool always_return = !PROTOBUF_TAILCALL;
    if (always_return || !ctx->DataAvailable(ptr)) {
      PROTOBUF_MUSTTAIL return Return(PROTOBUF_TC_PARAM_PASS);
    }
    PROTOBUF_MUSTTAIL return TagDispatch(PROTOBUF_TC_PARAM_PASS);
  }

  static const char* ParseLoop(MessageLite* msg, const char* ptr,
                               ParseContext* ctx,
                               const TailCallParseTableBase* table) {
    ScopedArenaSwap saved(msg, ctx);
    const uint32_t has_bits_offset = table->has_bits_offset;
    while (!ctx->Done(&ptr)) {
      uint64_t hasbits = 0;
      if (has_bits_offset) hasbits = RefAt<uint32_t>(msg, has_bits_offset);
      ptr = TagDispatch(msg, ptr, ctx, table, hasbits, {});
      if (ptr == nullptr) break;
      if (ctx->LastTag() != 1) break;  // Ended on terminating tag
    }
    return ptr;
  }

  template <typename LayoutType, typename TagType>
  static const char* SingularFixed(PROTOBUF_TC_PARAM_DECL);

  template <typename FieldType, typename TagType, VarintDecode zigzag>
  static const char* SingularVarint(PROTOBUF_TC_PARAM_DECL);
};

// Declare helper functions:
#include <google/protobuf/generated_message_tctable_impl.inc>

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include <google/protobuf/port_undef.inc>

#endif  // GOOGLE_PROTOBUF_GENERATED_MESSAGE_TCTABLE_IMPL_H__
