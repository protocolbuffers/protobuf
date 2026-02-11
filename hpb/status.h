// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_HPB_STATUS_H__
#define GOOGLE_PROTOBUF_HPB_STATUS_H__

#include <cstdint>
#include <string>
#include <variant>

#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "hpb/backend/types.h"
#include "upb/wire/decode.h"
#include "upb/wire/encode.h"

namespace hpb {

// This type exists to work around an absl type that has not yet been
// released.
struct SourceLocation {
  static SourceLocation current() { return {}; }
  absl::string_view file_name() { return "<unknown>"; }
  int line() { return 0; }
};

absl::Status MessageEncodeError(upb_EncodeStatus status,
                                SourceLocation loc = SourceLocation::current());

absl::Status MessageAllocationError(
    SourceLocation loc = SourceLocation::current());

absl::Status ExtensionNotFoundError(
    uint32_t extension_number, SourceLocation loc = SourceLocation::current());

absl::Status MessageDecodeError(upb_DecodeStatus status,
                                SourceLocation loc = SourceLocation::current());

// -----------------------------------------------------------------------------
// hpb::StatusOr is lightweight and guarantees zero extraneous heap allocations
// (note that this excludes heap allocs done by creating <T>).
// If the status is valid, a <T> is provided. Otherwise, the error code
// is returned in the form of an enum.
template <typename T>
class StatusOr {
 public:
  explicit StatusOr(const T& value) : value_(value) {}
  explicit StatusOr(T&& value) : value_(std::move(value)) {}
  explicit StatusOr(internal::backend::Error status) : value_(status) {}

  bool ok() const { return std::holds_alternative<T>(value_); }

  T& value() {
    ABSL_CHECK(ok()) << "Cannot fetch hpb::value for errors.";
    return std::get<T>(value_);
  }
  const T& value() const {
    ABSL_CHECK(ok()) << "Cannot fetch hpb::value for errors.";
    return std::get<T>(value_);
  }

  std::string error() const {
    ABSL_CHECK(!ok()) << "Cannot fetch hpb::error when T exists";
    return std::get<internal::backend::Error>(value_).ToString();
  }

  absl::StatusOr<T> ToAbslStatusOr() const {
    if (ok()) {
      return value();
    }
    return absl::Status(absl::StatusCode::kUnknown, error());
  }

 private:
  std::variant<T, internal::backend::Error> value_;
};
}  // namespace hpb

#endif  // GOOGLE_PROTOBUF_HPB_STATUS_H__
