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
  ABSL_ATTRIBUTE_REINITIALIZES void Clear() PROTOBUF_FINAL { Clear(*this); }
  size_t ByteSizeLong() const PROTOBUF_FINAL { return ByteSizeLong(*this); }
  int GetCachedSize() const { return _impl_._cached_size_.Get(); }
  ::uint8_t* _InternalSerialize(
      ::uint8_t* target, io::EpsCopyOutputStream* stream) const PROTOBUF_FINAL {
    return _InternalSerialize(*this, target, stream);
  }

 protected:
  using Message::Message;
  ~ZeroFieldsBase() PROTOBUF_OVERRIDE;

  static void MergeImpl(MessageLite& to, const MessageLite& from);
  static void CopyImpl(Message& to, const Message& from);
  void InternalSwap(ZeroFieldsBase* other);
  static void Clear(MessageLite& msg);
  static size_t ByteSizeLong(const MessageLite& msg);
  static ::uint8_t* _InternalSerialize(const MessageLite& msg,
                                       ::uint8_t* target,
                                       io::EpsCopyOutputStream* stream);

  struct {
    mutable internal::CachedSize _cached_size_;
  } _impl_;
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_GENERATED_MESSAGE_BASES_H__
