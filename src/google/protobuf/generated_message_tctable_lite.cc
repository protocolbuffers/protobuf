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

#include <cstdint>

#include <google/protobuf/parse_context.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/generated_message_tctable_decl.h>
#include <google/protobuf/generated_message_tctable_impl.h>
#include <google/protobuf/message_lite.h>
#include <google/protobuf/wire_format_lite.h>

// clang-format off
#include <google/protobuf/port_def.inc>
// clang-format on

namespace google {
namespace protobuf {
namespace internal {

const char* TcParserBase::GenericFallbackLite(PROTOBUF_TC_PARAM_DECL) {
  return GenericFallbackImpl<MessageLite, std::string>(PROTOBUF_TC_PARAM_PASS);
}

namespace {

// Offset returns the address `offset` bytes after `base`.
inline void* Offset(void* base, uint32_t offset) {
  return static_cast<uint8*>(base) + offset;
}

// InvertPacked changes tag bits from the given wire type to length
// delimited. This is the difference expected between packed and non-packed
// repeated fields.
template <WireFormatLite::WireType Wt>
inline PROTOBUF_ALWAYS_INLINE void InvertPacked(TcFieldData& data) {
  data.data ^= Wt ^ WireFormatLite::WIRETYPE_LENGTH_DELIMITED;
}

}  // namespace

//////////////////////////////////////////////////////////////////////////////
// Fixed fields
//////////////////////////////////////////////////////////////////////////////

template <uint32_t kPowerOf2>
template <typename LayoutType, typename TagType>
const char* TcParser<kPowerOf2>::SingularFixed(PROTOBUF_TC_PARAM_DECL) {
  if (PROTOBUF_PREDICT_FALSE(static_cast<TagType>(data.coded_tag()) != 0)) {
    return table->fallback(PROTOBUF_TC_PARAM_PASS);
  }
  ptr += sizeof(TagType);  // Consume tag
  hasbits |= (1 << data.hasbit_idx());
  std::memcpy(Offset(msg, data.offset()), ptr, sizeof(LayoutType));
  ptr += sizeof(LayoutType);
  // TailCall syncs any pending hasbits:
  PROTOBUF_MUSTTAIL return TailCall(PROTOBUF_TC_PARAM_PASS);
}

template <typename LayoutType, typename TagType>
const char* TcParserBase::RepeatedFixed(PROTOBUF_TC_PARAM_DECL) {
  if (PROTOBUF_PREDICT_FALSE(static_cast<TagType>(data.coded_tag()) != 0)) {
    // Check if the field can be parsed as packed repeated:
    constexpr WireFormatLite::WireType fallback_wt =
        sizeof(LayoutType) == 4 ? WireFormatLite::WIRETYPE_FIXED32
                                : WireFormatLite::WIRETYPE_FIXED64;
    InvertPacked<fallback_wt>(data);
    if (static_cast<TagType>(data.coded_tag()) == 0) {
      return PackedFixed<LayoutType, TagType>(PROTOBUF_TC_PARAM_PASS);
    } else {
      return table->fallback(PROTOBUF_TC_PARAM_PASS);
    }
  }
  auto& field = RefAt<RepeatedField<LayoutType>>(msg, data.offset());
  int idx = field.size();
  auto elem = field.Add();
  int space = field.Capacity() - idx;
  idx = 0;
  auto expected_tag = UnalignedLoad<TagType>(ptr);
  do {
    ptr += sizeof(TagType);
    std::memcpy(elem + (idx++), ptr, sizeof(LayoutType));
    ptr += sizeof(LayoutType);
    if (idx >= space) break;
    if (!ctx->DataAvailable(ptr)) break;
  } while (UnalignedLoad<TagType>(ptr) == expected_tag);
  field.AddNAlreadyReserved(idx - 1);
  return Return(PROTOBUF_TC_PARAM_PASS);
}

template <typename LayoutType, typename TagType>
const char* TcParserBase::PackedFixed(PROTOBUF_TC_PARAM_DECL) {
  if (PROTOBUF_PREDICT_FALSE(static_cast<TagType>(data.coded_tag()) != 0)) {
    // Try parsing as non-packed repeated:
    constexpr WireFormatLite::WireType fallback_wt =
        sizeof(LayoutType) == 4 ? WireFormatLite::WIRETYPE_FIXED32
                                : WireFormatLite::WIRETYPE_FIXED64;
    InvertPacked<fallback_wt>(data);
    if (static_cast<TagType>(data.coded_tag()) == 0) {
      return RepeatedFixed<LayoutType, TagType>(PROTOBUF_TC_PARAM_PASS);
    } else {
      return table->fallback(PROTOBUF_TC_PARAM_PASS);
    }
  }
  ptr += sizeof(TagType);
  // Since ctx->ReadPackedFixed does not use TailCall<> or Return<>, sync any
  // pending hasbits now:
  SyncHasbits(msg, hasbits, table);
  auto& field = RefAt<RepeatedField<LayoutType>>(msg, data.offset());
  int size = ReadSize(&ptr);
  // TODO(dlj): add a tailcalling variant of ReadPackedFixed.
  return ctx->ReadPackedFixed(ptr, size,
                              static_cast<RepeatedField<LayoutType>*>(&field));
}

#define PROTOBUF_TCT_SOURCE
#include <google/protobuf/generated_message_tctable_impl.inc>

}  // namespace internal
}  // namespace protobuf
}  // namespace google
