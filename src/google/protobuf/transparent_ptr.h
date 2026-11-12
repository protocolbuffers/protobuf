// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_TRANSPARENT_PTR_H__
#define GOOGLE_PROTOBUF_TRANSPARENT_PTR_H__

#include <functional>
#include <type_traits>
#include <utility>

#include "absl/container/flat_hash_set.h"
#include "google/protobuf/port.h"

namespace google {
namespace protobuf {
namespace internal {

// CRTP base for pointer wrappers supporting heterogeneous lookup by
// Derived::key().
template <typename Derived, typename T>
class TransparentPtr {
  template <typename K, typename D = std::conditional_t<false, K, Derived>>
  using EnableIfKey =
      std::enable_if_t<!std::is_convertible_v<const K&, D> &&
                       std::is_convertible_v<
                           const K&, decltype(std::declval<const D&>().key())>>;

 public:
  const T* ptr;

  // NOLINTNEXTLINE(google-explicit-constructor)
  TransparentPtr(const T* p) : ptr(p) {}

  const T* operator->() const { return ptr; }

  friend bool operator==(const Derived& a, const Derived& b) {
    return a.ptr == b.ptr || a.key() == b.key();
  }
  template <typename K, typename = EnableIfKey<K>>
  friend bool operator==(const Derived& a, const K& b) {
    return a.key() == b;
  }
  template <typename K, typename = EnableIfKey<K>>
  friend bool operator==(const K& a, const Derived& b) {
    return a == b.key();
  }
  friend bool operator!=(const Derived& a, const Derived& b) {
    return !(a == b);
  }
  template <typename K, typename = EnableIfKey<K>>
  friend bool operator!=(const Derived& a, const K& b) {
    return !(a == b);
  }
  template <typename K, typename = EnableIfKey<K>>
  friend bool operator!=(const K& a, const Derived& b) {
    return !(a == b);
  }

  template <typename H>
  friend H AbslHashValue(H h, const Derived& d) {
    return H::combine(std::move(h), d.key());
  }
};

template <typename Wrapper>
using TransparentPtrSet = absl::flat_hash_set<
    Wrapper,
    TransparentHash<
        Wrapper, std::decay_t<decltype(std::declval<const Wrapper&>().key())>>,
    std::equal_to<void>>;

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_TRANSPARENT_PTR_H__
