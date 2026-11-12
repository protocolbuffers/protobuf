// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_HPB_EXPECTED_H__
#define GOOGLE_PROTOBUF_HPB_EXPECTED_H__

#include <memory>
#include <type_traits>
#include <utility>
#include <variant>

#include "absl/base/attributes.h"
#include "absl/base/optimization.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "hpb/error.h"

namespace hpb {

namespace internal {
[[noreturn]] void ExpectedAbortNoValue();
[[noreturn]] void ExpectedAbortHasValue();
}  // namespace internal

template <typename T>
class [[nodiscard]] ABSL_ATTRIBUTE_TRIVIAL_ABI Expected;

// Partial specialization for void value type.
template <>
class [[nodiscard]] ABSL_ATTRIBUTE_TRIVIAL_ABI Expected<void> {
 public:
  using value_type = void;
  using error_type = Error;

  constexpr Expected() noexcept : has_value_(true), error_() {}

  // NOLINTNEXTLINE(google-explicit-constructor)
  constexpr Expected(Error error) noexcept : has_value_(false), error_(error) {}

  constexpr bool has_value() const noexcept { return has_value_; }
  constexpr bool ok() const noexcept { return has_value_; }

  void value() const {
    if (ABSL_PREDICT_FALSE(!has_value_)) {
      internal::ExpectedAbortNoValue();
    }
  }

  Error error() const noexcept {
    if (ABSL_PREDICT_FALSE(has_value_)) {
      internal::ExpectedAbortHasValue();
    }
    return error_;
  }

  absl::Status status() const {
    if (has_value_) {
      return absl::OkStatus();
    }
    return error_.status();
  }
  // NOLINTNEXTLINE(google-explicit-constructor)
  operator absl::Status() const { return status(); }

  friend bool operator==(const Expected& a, const Expected& b) noexcept {
    if (a.has_value_ != b.has_value_) return false;
    return a.has_value_ || a.error_ == b.error_;
  }
  friend bool operator!=(const Expected& a, const Expected& b) noexcept {
    return !(a == b);
  }

 private:
  bool has_value_;
  Error error_;
};

template <typename T>
class [[nodiscard]] ABSL_ATTRIBUTE_TRIVIAL_ABI Expected {
  static_assert(!std::is_void_v<T>,
                "Use Expected<void> specialization for void value type");
  static_assert(!std::is_same_v<std::decay_t<T>, Error>,
                "T cannot be hpb::Error");
  static_assert(!std::is_reference_v<T>, "T cannot be a reference type");
  static_assert(!std::is_const_v<T>, "T cannot be const-qualified");

 public:
  using value_type = T;
  using error_type = Error;

  template <typename U = T,
            std::enable_if_t<std::is_copy_constructible_v<U>, int> = 0>
  // NOLINTNEXTLINE(google-explicit-constructor)
  constexpr Expected(const T& value) : data_(std::in_place_index<0>, value) {}
  // NOLINTNEXTLINE(google-explicit-constructor)
  constexpr Expected(T&& value)
      : data_(std::in_place_index<0>, std::move(value)) {}

  // NOLINTNEXTLINE(google-explicit-constructor)
  constexpr Expected(Error error) noexcept
      : data_(std::in_place_index<1>, error) {}

  constexpr bool has_value() const noexcept { return data_.index() == 0; }
  constexpr bool ok() const noexcept { return has_value(); }

  constexpr const T& value() const& ABSL_ATTRIBUTE_LIFETIME_BOUND {
    if (ABSL_PREDICT_FALSE(!has_value())) {
      internal::ExpectedAbortNoValue();
    }
    return std::get<0>(data_);
  }
  constexpr T& value() & ABSL_ATTRIBUTE_LIFETIME_BOUND {
    if (ABSL_PREDICT_FALSE(!has_value())) {
      internal::ExpectedAbortNoValue();
    }
    return std::get<0>(data_);
  }
  constexpr const T&& value() const&& ABSL_ATTRIBUTE_LIFETIME_BOUND {
    if (ABSL_PREDICT_FALSE(!has_value())) {
      internal::ExpectedAbortNoValue();
    }
    return std::get<0>(std::move(data_));
  }
  constexpr T&& value() && ABSL_ATTRIBUTE_LIFETIME_BOUND {
    if (ABSL_PREDICT_FALSE(!has_value())) {
      internal::ExpectedAbortNoValue();
    }
    return std::get<0>(std::move(data_));
  }

  constexpr const T& operator*() const& ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return value();
  }
  constexpr T& operator*() & ABSL_ATTRIBUTE_LIFETIME_BOUND { return value(); }
  constexpr const T&& operator*() const&& ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return std::move(*this).value();
  }
  constexpr T&& operator*() && ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return std::move(*this).value();
  }

  constexpr const T* operator->() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return std::addressof(value());
  }
  constexpr T* operator->() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return std::addressof(value());
  }

  template <typename U>
  constexpr T value_or(U&& default_value) const& {
    return has_value() ? value()
                       : static_cast<T>(std::forward<U>(default_value));
  }
  template <typename U>
  constexpr T value_or(U&& default_value) && {
    return has_value() ? std::move(*this).value()
                       : static_cast<T>(std::forward<U>(default_value));
  }

  Error error() const noexcept {
    if (ABSL_PREDICT_FALSE(has_value())) {
      internal::ExpectedAbortHasValue();
    }
    return std::get<1>(data_);
  }

  absl::Status status() const {
    if (has_value()) {
      return absl::OkStatus();
    }
    return error().status();
  }
  // NOLINTNEXTLINE(google-explicit-constructor)
  operator absl::Status() const { return status(); }

  template <typename U = T,
            std::enable_if_t<std::is_copy_constructible_v<U>, int> = 0>
  absl::StatusOr<T> ToAbslStatusOr() const& {
    if (has_value()) {
      return value();
    }
    return error().status();
  }

  absl::StatusOr<T> ToAbslStatusOr() && {
    if (has_value()) {
      return std::move(*this).value();
    }
    return error().status();
  }

  template <typename U = T,
            std::enable_if_t<std::is_copy_constructible_v<U>, int> = 0>
  // NOLINTNEXTLINE(google-explicit-constructor)
  operator absl::StatusOr<T>() const& {
    return ToAbslStatusOr();
  }
  // NOLINTNEXTLINE(google-explicit-constructor)
  operator absl::StatusOr<T>() && { return std::move(*this).ToAbslStatusOr(); }

  friend bool operator==(const Expected& a, const Expected& b) {
    return a.data_ == b.data_;
  }
  friend bool operator!=(const Expected& a, const Expected& b) {
    return a.data_ != b.data_;
  }

 private:
  std::variant<T, Error> data_;
};

}  // namespace hpb

#endif  // GOOGLE_PROTOBUF_HPB_EXPECTED_H__
