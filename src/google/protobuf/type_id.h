// Protocol Buffers - Google's data interchange format
// Copyright 2022 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
//
// Defines TypeId, a std::type_info equivalent for protobuf message types.
//
// This file intentionally does not depend on protobuf_lite.

#ifndef GOOGLE_PROTOBUF_TYPE_ID_H__
#define GOOGLE_PROTOBUF_TYPE_ID_H__

#include "absl/strings/string_view.h"
#include "google/protobuf/message_traits.h"
#include "google/protobuf/port.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

class MessageLite;
class TypeId;

namespace internal {
struct ClassData;
TypeId TypeIdFromClassData(const ClassData* data);
}  // namespace internal

// A `std::type_info` equivalent for protobuf message types.
// This class is preferred over using `typeid` for a few reasons:
//  - It works with RTTI disabled.
//  - It works for `DynamicMessage` types.
//  - It works in custom vtable mode.
//
// Usage:
//  - Instead of `typeid(Type)` use `TypeId::Get<Type>()`
//  - Instead of `typeid(expr)` use `TypeId::Get(expr)`
//
// Supports all relationals including <=>, and supports hashing via
// `absl::Hash`.
class PROTOBUF_FUTURE_ADD_EARLY_WARN_UNUSED TypeId {
 public:
  template <int&... DeductionBarrier, typename Delay = void>
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD static TypeId Get(
      const MessageLite& msg) {
    return TypeId(internal::GetClassData(internal::TypeDependent<Delay>(msg)));
  }

  template <typename T>
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD static TypeId Get() {
    return TypeId(internal::MessageTraits<T>::class_data());
  }

  // Name of the message type.
  // Equivalent to `.GetTypeName()` on the message.
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD absl::string_view name() const;

  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD friend constexpr bool operator==(
      TypeId a, TypeId b) {
    return a.data_ == b.data_;
  }
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD friend constexpr bool operator!=(
      TypeId a, TypeId b) {
    return !(a == b);
  }
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD friend constexpr bool operator<(
      TypeId a, TypeId b) {
    return a.data_ < b.data_;
  }
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD friend constexpr bool operator>(
      TypeId a, TypeId b) {
    return a.data_ > b.data_;
  }
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD friend constexpr bool operator<=(
      TypeId a, TypeId b) {
    return a.data_ <= b.data_;
  }
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD friend constexpr bool operator>=(
      TypeId a, TypeId b) {
    return a.data_ >= b.data_;
  }

#if defined(__cpp_impl_three_way_comparison) && \
    __cpp_impl_three_way_comparison >= 201907L
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD friend constexpr auto operator<=>(
      TypeId a, TypeId b) {
    return a.data_ <=> b.data_;
  }
#endif

  template <typename H>
  PROTOBUF_FUTURE_ADD_EARLY_NODISCARD friend H AbslHashValue(H state,
                                                             TypeId id) {
    return H::combine(std::move(state), id.data_);
  }

 private:
  friend TypeId internal::TypeIdFromClassData(const internal::ClassData* data);

  constexpr explicit TypeId(const internal::ClassData* data) : data_(data) {}

  const internal::ClassData* data_;
};

namespace internal {

inline TypeId TypeIdFromClassData(const ClassData* data) {
  return TypeId(data);
}

}  // namespace internal

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_TYPE_ID_H__
