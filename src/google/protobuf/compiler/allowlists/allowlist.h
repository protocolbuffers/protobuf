// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_ALLOWLISTS_ALLOWLIST_H__
#define GOOGLE_PROTOBUF_COMPILER_ALLOWLISTS_ALLOWLIST_H__

#include <cstddef>
#include <cstring>
#include <string>
#include <type_traits>

#include "absl/algorithm/container.h"
#include "google/protobuf/stubs/common.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/match.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace internal {
enum AllowlistFlags : unsigned int {
  kNone = 0,
  kMatchPrefix = 1 << 1,
  kAllowAllInOss = 1 << 2,
  kAllowAllWhenEmpty = 1 << 3,
};


#if !defined(__GNUC__) || defined(__clang__) || PROTOBUF_GNUC_MIN(9, 1)
using maybe_string_view = absl::string_view;
#else
// In GCC versions before 9.1, template substitution fails because of the
// implicit conversion between `const char*` and absl::string_view.  In these
// cases we can just use a raw string and convert later.  See
// https://godbolt.org/z/r57fx37d1 for an example of the failure.
using maybe_string_view = const char*;
#endif

// An allowlist of things (messages, files, targets) that are allowed to violate
// some constraint.
//
// This is fundamentally a simple API over a set of static strings. It should
// only ever be used as a `static const` variable.
//
// These allowlists are usually only used internally within Google, and contain
// the names of internal files and Protobufs. In open source, these lists become
// no-ops (either they always or never allow everything).
template <size_t n>
class Allowlist final {
 public:
  template <size_t m = n, typename = std::enable_if_t<m != 0>>
  constexpr Allowlist(const maybe_string_view (&list)[n], AllowlistFlags flags)
      : flags_(flags) {
    for (size_t i = 0; i < n; ++i) {
      list_[i] = list[i];
      if (i != 0) {
        ABSL_ASSERT(list_[i - 1] < list_[i] && "Allowlist must be sorted!");
      }
    }
  }

  template <size_t m = n, typename = std::enable_if_t<m == 0>>
  explicit constexpr Allowlist(AllowlistFlags flags)
      : list_(nullptr, 0), flags_(flags) {}

  // Checks if the element is allowed by this allowlist.
  bool Allows(absl::string_view name) const {
    if (flags_ & AllowlistFlags::kAllowAllInOss) return true;

    // Convert to a span to get access to standard algorithms without resorting
    // to horrible things like std::end().
    absl::Span<const absl::string_view> list = list_;

    auto bound = absl::c_lower_bound(list, name);
    if (bound == list.end()) {
      // If this string has the last element as a prefix, it will appear as if
      // the element is not present in the list; we can take care of this case
      // by manually checking the last element.
      //
      // This will also spuriously fire if a string sorts before everything in
      // the list, but in that case the check will still return false as
      // expected.
      if (flags_ & AllowlistFlags::kMatchPrefix && !list.empty()) {
        return absl::StartsWith(name, list.back());
      }

      return false;
    }

    if (name == *bound) return true;

    if (flags_ & AllowlistFlags::kMatchPrefix && bound != list.begin()) {
      return absl::StartsWith(name, bound[-1]);
    }

    return false;
  }

 private:
  constexpr absl::Span<const absl::string_view> list() const { return list_; }

  // NOTE: std::array::operator[] is *not* constexpr before C++17.
  //
  // In order for a zero-element list to work, we replace the array with a
  // null string view when the size is zero.
  std::conditional_t<n != 0, absl::string_view[n],
                     absl::Span<absl::string_view>>
      list_;
  AllowlistFlags flags_;
};

struct EmptyAllowlistSentinel {};

// This overload picks up MakeAllowlist({}), since zero-length arrays are not
// a thing in C++.
constexpr Allowlist<0> MakeAllowlist(
    EmptyAllowlistSentinel,  // This binds to `{}`.
    AllowlistFlags flags = AllowlistFlags::kNone) {
  return Allowlist<0>(flags);
}

template <size_t n>
constexpr Allowlist<n> MakeAllowlist(
    const maybe_string_view (&list)[n],
    AllowlistFlags flags = AllowlistFlags::kNone) {
  return Allowlist<n>(list, flags);
}

}  // namespace internal
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_COMPILER_ALLOWLISTS_ALLOWLIST_H__
