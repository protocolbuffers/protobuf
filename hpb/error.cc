// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "hpb/error.h"

#include <ostream>

#include "absl/status/status.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"

namespace hpb {

absl::string_view DecodeStatusToString(DecodeStatus status) {
  switch (status) {
    case DecodeStatus::kOk:
      return "OK";
    case DecodeStatus::kMalformed:
      return "Malformed wire format";
    case DecodeStatus::kMaxDepthExceeded:
      return "Recursion depth exceeded";
    case DecodeStatus::kBadUtf8:
      return "Invalid UTF-8 in string field";
    case DecodeStatus::kMissingRequired:
      return "Missing required fields";
    case DecodeStatus::kOutOfMemory:
      return "Out of memory";
  }
  return "Unknown decode error";
}

absl::string_view EncodeStatusToString(EncodeStatus status) {
  switch (status) {
    case EncodeStatus::kOk:
      return "OK";
    case EncodeStatus::kOutOfMemory:
      return "Out of memory";
    case EncodeStatus::kMaxDepthExceeded:
      return "Recursion depth exceeded";
    case EncodeStatus::kMissingRequired:
      return "Missing required fields";
    case EncodeStatus::kMaxSizeExceeded:
      return "Message size exceeds 2GB limit";
  }
  return "Unknown encode error";
}

absl::string_view Error::message() const {
  switch (code_) {
    case ErrorCode::kUnknown:
      return "Unknown error";
    case ErrorCode::kOutOfMemory:
      return "Out of memory";
    case ErrorCode::kExtensionNotFound:
      return "Extension not found";
    case ErrorCode::kDecodeError:
      return DecodeStatusToString(decode_status());
    case ErrorCode::kEncodeError:
      return EncodeStatusToString(encode_status());
  }
  return "Unknown error";
}

absl::Status Error::status() const {
  std::string msg = absl::StrFormat("%v", *this);
  switch (code_) {
    case ErrorCode::kUnknown:
      return absl::UnknownError(msg);
    case ErrorCode::kOutOfMemory:
      return absl::ResourceExhaustedError(msg);
    case ErrorCode::kExtensionNotFound:
      return absl::NotFoundError(msg);
    case ErrorCode::kDecodeError:
      return absl::InvalidArgumentError(msg);
    case ErrorCode::kEncodeError:
      return absl::InternalError(msg);
  }
  return absl::UnknownError(msg);
}

std::ostream& operator<<(std::ostream& os, const Error& error) {
  return os << absl::StreamFormat("%v", error);
}

std::ostream& operator<<(std::ostream& os, DecodeStatus status) {
  return os << DecodeStatusToString(status);
}

std::ostream& operator<<(std::ostream& os, EncodeStatus status) {
  return os << EncodeStatusToString(status);
}

}  // namespace hpb
