// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_HPB_ERROR_H__
#define GOOGLE_PROTOBUF_HPB_ERROR_H__

#include <cstdint>
#include <iosfwd>
#include <type_traits>

#include "absl/status/status.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"

namespace hpb {

enum class ErrorCode : uint32_t {
  kUnknown = 0,
  kOutOfMemory = 1,
  kExtensionNotFound = 2,
  kDecodeError = 3,
  kEncodeError = 4,
};

enum class DecodeStatus : uint32_t {
  kOk = 0,
  kMalformed = 1,
  kMaxDepthExceeded = 2,
  kBadUtf8 = 3,
  kMissingRequired = 4,
  kOutOfMemory = 5,
};

enum class EncodeStatus : uint32_t {
  kOk = 0,
  kOutOfMemory = 1,
  kMaxDepthExceeded = 2,
  kMissingRequired = 3,
  kMaxSizeExceeded = 4,
};

absl::string_view DecodeStatusToString(DecodeStatus status);
absl::string_view EncodeStatusToString(EncodeStatus status);

namespace internal {
struct ErrorAccess;
}  // namespace internal

class [[nodiscard]] Error final {
 public:
  constexpr explicit Error() noexcept
      : code_(ErrorCode::kUnknown), detail_(0) {}

  constexpr ErrorCode code() const noexcept { return code_; }

  constexpr uint32_t extension_number() const noexcept {
    return code_ == ErrorCode::kExtensionNotFound ? detail_ : 0;
  }
  constexpr DecodeStatus decode_status() const noexcept {
    return code_ == ErrorCode::kDecodeError ? static_cast<DecodeStatus>(detail_)
                                            : DecodeStatus::kOk;
  }
  constexpr EncodeStatus encode_status() const noexcept {
    return code_ == ErrorCode::kEncodeError ? static_cast<EncodeStatus>(detail_)
                                            : EncodeStatus::kOk;
  }

  absl::string_view message() const;
  absl::Status status() const;
  // NOLINTNEXTLINE(google-explicit-constructor)
  operator absl::Status() const { return status(); }

  friend bool operator==(const Error& a, const Error& b) noexcept {
    return a.code_ == b.code_ && a.detail_ == b.detail_;
  }
  friend bool operator!=(const Error& a, const Error& b) noexcept {
    return !(a == b);
  }

  template <typename Sink>
  friend void AbslStringify(Sink& sink, const Error& error) {
    switch (error.code()) {
      case ErrorCode::kExtensionNotFound:
        absl::Format(&sink, "Extension %u not found", error.detail_);
        break;
      case ErrorCode::kDecodeError:
        absl::Format(&sink, "Decode error: %s", error.message());
        break;
      case ErrorCode::kEncodeError:
        absl::Format(&sink, "Encode error: %s", error.message());
        break;
      default:
        sink.Append(error.message());
        break;
    }
  }

 private:
  friend struct internal::ErrorAccess;

  constexpr Error(ErrorCode code, uint32_t detail) noexcept
      : code_(code), detail_(detail) {}

  ErrorCode code_;
  uint32_t detail_;
};

static_assert(sizeof(Error) == 8,
              "hpb::Error must fit in a single 64-bit register for zero-cost "
              "return");
static_assert(std::is_trivially_copyable_v<Error>,
              "hpb::Error must be trivially copyable");

std::ostream& operator<<(std::ostream& os, const Error& error);
std::ostream& operator<<(std::ostream& os, DecodeStatus status);
std::ostream& operator<<(std::ostream& os, EncodeStatus status);

}  // namespace hpb

#endif  // GOOGLE_PROTOBUF_HPB_ERROR_H__
