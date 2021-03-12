// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef GOOGLE_PROTOBUF_STUBS_STATUS_H_
#define GOOGLE_PROTOBUF_STUBS_STATUS_H_

#include <string>

#include <google/protobuf/stubs/stringpiece.h>

#include <google/protobuf/port_def.inc>

namespace google {
namespace protobuf {
namespace util {
namespace status_internal {

// These values must match error codes defined in google/rpc/code.proto.
enum class StatusCode : int {
  kOk = 0,
  kCancelled = 1,
  kUnknown = 2,
  kInvalidArgument = 3,
  kDeadlineExceeded = 4,
  kNotFound = 5,
  kAlreadyExists = 6,
  kPermissionDenied = 7,
  kUnauthenticated = 16,
  kResourceExhausted = 8,
  kFailedPrecondition = 9,
  kAborted = 10,
  kOutOfRange = 11,
  kUnimplemented = 12,
  kInternal = 13,
  kUnavailable = 14,
  kDataLoss = 15,
};

class PROTOBUF_EXPORT Status {
 public:
  // Creates a "successful" status.
  Status();

  // Create a status in the canonical error space with the specified
  // code, and error message.  If "code == 0", error_message is
  // ignored and a Status object identical to Status::kOk is
  // constructed.
  Status(StatusCode error_code, StringPiece error_message);
  Status(const Status&);
  Status& operator=(const Status& x);
  ~Status() {}

  // Some pre-defined Status objects
  static const Status OK;             // Identical to 0-arg constructor
  static const Status CANCELLED;
  static const Status UNKNOWN;

  // Accessor
  bool ok() const {
    return error_code_ == StatusCode::kOk;
  }
  StatusCode code() const {
    return error_code_;
  }
  StringPiece message() const {
    return error_message_;
  }

  bool operator==(const Status& x) const;
  bool operator!=(const Status& x) const {
    return !operator==(x);
  }

  // Return a combination of the error code name and message.
  std::string ToString() const;

 private:
  StatusCode error_code_;
  std::string error_message_;
};

// Prints a human-readable representation of 'x' to 'os'.
PROTOBUF_EXPORT std::ostream& operator<<(std::ostream& os, const Status& x);

}  // namespace status_internal

using ::google::protobuf::util::status_internal::Status;
using ::google::protobuf::util::status_internal::StatusCode;

namespace error {

// TODO(yannic): Remove these.
constexpr StatusCode OK = StatusCode::kOk;
constexpr StatusCode CANCELLED = StatusCode::kCancelled;
constexpr StatusCode UNKNOWN = StatusCode::kUnknown;
constexpr StatusCode INVALID_ARGUMENT = StatusCode::kInvalidArgument;
constexpr StatusCode NOT_FOUND = StatusCode::kNotFound;
constexpr StatusCode INTERNAL = StatusCode::kInternal;

}  // namespace error
}  // namespace util
}  // namespace protobuf
}  // namespace google

#include <google/protobuf/port_undef.inc>

#endif  // GOOGLE_PROTOBUF_STUBS_STATUS_H_
