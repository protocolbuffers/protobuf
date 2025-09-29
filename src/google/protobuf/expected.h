// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_EXPECTED_H__
#define GOOGLE_PROTOBUF_EXPECTED_H__

#include <variant>

#include "absl/log/absl_check.h"

namespace google {
namespace protobuf {

// -----------------------------------------------------------------------------
// google::protobuf::expected is lightweight and guarantees zero extraneous heap
// allocations (note that this excludes heap allocs done by creating <T>).
// If the status is valid, a <T> is provided. Otherwise, the error E is
// returned.
template <typename T, typename E>
class expected {
 public:
  explicit expected(const T& value) : value_(value) {}
  explicit expected(T&& value) : value_(std::move(value)) {}
  explicit expected(E error) : value_(error) {}

  bool has_value() const { return std::holds_alternative<T>(value_); }

  T& value() {
    ABSL_CHECK(has_value())
        << "google::protobuf::expected a value, but detected an error";
    return std::get<T>(value_);
  }

  const T& value() const {
    ABSL_CHECK(has_value())
        << "google::protobuf::expected a value, but detected an error";
    return std::get<T>(value_);
  }

  E error() const {
    ABSL_CHECK(!has_value())
        << "google::protobuf::expected an error, but detected a value";
    return std::get<E>(value_);
  }

 private:
  std::variant<T, E> value_;
};

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_EXPECTED_H__
