// Protocol Buffers - Google's data interchange format
// Copyright 2022 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
//
// Defines MessageTraits, a trait that provides access to the message prototype,
// class data, and the parse table. Specialized by codegen with hard-coded
// values to avoid runtime lookups.
//
// This file intentionally does not depend on protobuf_lite.

#ifndef GOOGLE_PROTOBUF_MESSAGE_TRAITS_H__
#define GOOGLE_PROTOBUF_MESSAGE_TRAITS_H__

#include <type_traits>

namespace google {
namespace protobuf {

class MessageLite;

namespace internal {

struct ClassData;

// Returns the ClassData for the given message.
//
// A template to avoid depending on message_lite.h.
template <typename MessageT>
const ClassData* GetClassData(const MessageT& msg) {
  if constexpr (std::is_same_v<MessageT, MessageLite>) {
    return msg.GetClassData();
  } else {
    return GetClassData(static_cast<const MessageLite&>(msg));
  }
}

template <typename T>
struct FallbackMessageTraits {
  static const void* default_instance() { return &T::default_instance(); }
  static constexpr const auto* class_data() {
    return GetClassData(T::default_instance());
  }
  static const auto* tc_table() { return class_data()->GetTcParseTable(); }
  // We can't make a constexpr pointer to the default, so use a function pointer
  // instead.
  static constexpr auto StrongPointer() { return &T::default_instance; }
};

// Traits for messages.
// We use a class scope variable template, which can be specialized with a
// different type in a non-defining declaration.
// We need non-defining declarations because we might have duplicates of the
// same trait specification on each dependent coming from different .proto.h
// files.
struct MessageTraitsImpl {
  template <typename T>
  static FallbackMessageTraits<T> value;
};
template <typename T>
using MessageTraits = decltype(MessageTraitsImpl::value<T>);

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_MESSAGE_TRAITS_H__
