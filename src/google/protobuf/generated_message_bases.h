// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// This file contains helpers for generated code.
//
//  Nothing in this file should be directly referenced by users of protobufs.

#ifndef GOOGLE_PROTOBUF_GENERATED_MESSAGE_BASES_H__
#define GOOGLE_PROTOBUF_GENERATED_MESSAGE_BASES_H__

#include "google/protobuf/arena.h"
#include "google/protobuf/generated_message_util.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/message.h"
#include "google/protobuf/parse_context.h"

// Must come last:
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

// To save code size, protos without any fields are derived from ZeroFieldsBase
// rather than Message.
class PROTOBUF_EXPORT ZeroFieldsBase : public Message {
 public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final { return true; }
  size_t ByteSizeLong() const final;
  int GetCachedSize() const { return _cached_size_.Get(); }
  const char* _InternalParse(const char* ptr,
                             internal::ParseContext* ctx) final;
  ::uint8_t* _InternalSerialize(::uint8_t* target,
                                io::EpsCopyOutputStream* stream) const final;

 protected:
  internal::CachedSize* AccessCachedSize() const final {
    return &_cached_size_;
  }

  constexpr ZeroFieldsBase() {}
  explicit ZeroFieldsBase(Arena* arena) : Message(arena) {}
  ZeroFieldsBase(const ZeroFieldsBase&) = delete;
  ZeroFieldsBase& operator=(const ZeroFieldsBase&) = delete;
  ~ZeroFieldsBase() override;

  const ClassData* GetClassData() const final;

  static void MergeImpl(Message& to, const Message& from);
  static void CopyImpl(Message& to, const Message& from);
  void InternalSwap(ZeroFieldsBase* other);

  mutable internal::CachedSize _cached_size_;
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_GENERATED_MESSAGE_BASES_H__
