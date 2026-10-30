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

class Message;
class MessageLite;

namespace internal {

struct MessageGlobalsBase;
struct ClassData;

template <typename MessageT>
const ClassData* GetClassData(const MessageT& msg);

// Traits for messages.
// We use a class scope variable template, which can be specialized with a
// different type in a non-defining declaration.
// We need non-defining declarations because we might have duplicates of the
// same trait specification on each dependent coming from different .proto.h
// files.
struct MessageTraitsImpl {
  struct Undefined;
  // We use an incomplete type to cause a compiler error if something tries to
  // instantiate `value<T>` with a `T` that had no specialization.
  // The `enable_if` is there to workaround some compilers/tools that complain
  // on the declaration even with no instantiations.
  template <typename T>
  static std::enable_if_t<std::is_void_v<T>, Undefined> value;
};
template <typename T>
using MessageTraits = decltype(MessageTraitsImpl::value<std::remove_cv_t<T>>);

// Returns the ClassData for the given message. If the concrete message type is
// known, this returns a compile-time constant. Otherwise, `ClassData` is loaded
// from `MessageLite::GetClassData()`.
template <typename MessageT>
const ClassData* GetClassData(const MessageT& msg) {
  if constexpr (std::is_same_v<MessageT, MessageLite> ||
                std::is_same_v<MessageT, Message>) {
    return msg.GetClassData();
  } else {
    return MessageTraits<MessageT>::class_data();
  }
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_MESSAGE_TRAITS_H__
