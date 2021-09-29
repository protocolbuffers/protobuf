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
#include <google/protobuf/stubs/status.h>

#include <stdio.h>

#include <google/protobuf/testing/googletest.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace {

TEST(Status, Constructor) {
  EXPECT_EQ(util::StatusCode::kOk,
            util::Status(util::StatusCode::kOk, "").code());
  EXPECT_EQ(util::StatusCode::kCancelled,
            util::Status(util::StatusCode::kCancelled, "").code());
  EXPECT_EQ(util::StatusCode::kUnknown,
            util::Status(util::StatusCode::kUnknown, "").code());
  EXPECT_EQ(util::StatusCode::kInvalidArgument,
            util::Status(util::StatusCode::kInvalidArgument, "").code());
  EXPECT_EQ(util::StatusCode::kDeadlineExceeded,
            util::Status(util::StatusCode::kDeadlineExceeded, "").code());
  EXPECT_EQ(util::StatusCode::kNotFound,
            util::Status(util::StatusCode::kNotFound, "").code());
  EXPECT_EQ(util::StatusCode::kAlreadyExists,
            util::Status(util::StatusCode::kAlreadyExists, "").code());
  EXPECT_EQ(util::StatusCode::kPermissionDenied,
            util::Status(util::StatusCode::kPermissionDenied, "").code());
  EXPECT_EQ(util::StatusCode::kUnauthenticated,
            util::Status(util::StatusCode::kUnauthenticated, "").code());
  EXPECT_EQ(util::StatusCode::kResourceExhausted,
            util::Status(util::StatusCode::kResourceExhausted, "").code());
  EXPECT_EQ(util::StatusCode::kFailedPrecondition,
            util::Status(util::StatusCode::kFailedPrecondition, "").code());
  EXPECT_EQ(util::StatusCode::kAborted,
            util::Status(util::StatusCode::kAborted, "").code());
  EXPECT_EQ(util::StatusCode::kOutOfRange,
            util::Status(util::StatusCode::kOutOfRange, "").code());
  EXPECT_EQ(util::StatusCode::kUnimplemented,
            util::Status(util::StatusCode::kUnimplemented, "").code());
  EXPECT_EQ(util::StatusCode::kInternal,
            util::Status(util::StatusCode::kInternal, "").code());
  EXPECT_EQ(util::StatusCode::kUnavailable,
            util::Status(util::StatusCode::kUnavailable, "").code());
  EXPECT_EQ(util::StatusCode::kDataLoss,
            util::Status(util::StatusCode::kDataLoss, "").code());
}

TEST(Status, ConstructorZero) {
  util::Status status(util::StatusCode::kOk, "msg");
  EXPECT_TRUE(status.ok());
  EXPECT_EQ("OK", status.ToString());
  EXPECT_EQ(util::OkStatus(), status);
}

TEST(Status, ConvenienceConstructors) {
  EXPECT_EQ(util::StatusCode::kOk, util::OkStatus().code());
  EXPECT_EQ("", util::OkStatus().message());

  EXPECT_EQ(util::StatusCode::kCancelled, util::CancelledError("").code());
  EXPECT_EQ("", util::CancelledError("").message());
  EXPECT_EQ("foo", util::CancelledError("foo").message());
  EXPECT_EQ("bar", util::CancelledError("bar").message());

  EXPECT_EQ(util::StatusCode::kUnknown, util::UnknownError("").code());
  EXPECT_EQ("", util::UnknownError("").message());
  EXPECT_EQ("foo", util::UnknownError("foo").message());
  EXPECT_EQ("bar", util::UnknownError("bar").message());

  EXPECT_EQ(util::StatusCode::kInvalidArgument,
            util::InvalidArgumentError("").code());
  EXPECT_EQ("", util::InvalidArgumentError("").message());
  EXPECT_EQ("foo", util::InvalidArgumentError("foo").message());
  EXPECT_EQ("bar", util::InvalidArgumentError("bar").message());

  EXPECT_EQ(util::StatusCode::kDeadlineExceeded,
            util::DeadlineExceededError("").code());
  EXPECT_EQ("", util::DeadlineExceededError("").message());
  EXPECT_EQ("foo", util::DeadlineExceededError("foo").message());
  EXPECT_EQ("bar", util::DeadlineExceededError("bar").message());

  EXPECT_EQ(util::StatusCode::kNotFound, util::NotFoundError("").code());
  EXPECT_EQ("", util::NotFoundError("").message());
  EXPECT_EQ("foo", util::NotFoundError("foo").message());
  EXPECT_EQ("bar", util::NotFoundError("bar").message());

  EXPECT_EQ(util::StatusCode::kAlreadyExists,
            util::AlreadyExistsError("").code());
  EXPECT_EQ("", util::AlreadyExistsError("").message());
  EXPECT_EQ("foo", util::AlreadyExistsError("foo").message());
  EXPECT_EQ("bar", util::AlreadyExistsError("bar").message());

  EXPECT_EQ(util::StatusCode::kPermissionDenied,
            util::PermissionDeniedError("").code());
  EXPECT_EQ("", util::PermissionDeniedError("").message());
  EXPECT_EQ("foo", util::PermissionDeniedError("foo").message());
  EXPECT_EQ("bar", util::PermissionDeniedError("bar").message());

  EXPECT_EQ(util::StatusCode::kUnauthenticated,
            util::UnauthenticatedError("").code());
  EXPECT_EQ("", util::UnauthenticatedError("").message());
  EXPECT_EQ("foo", util::UnauthenticatedError("foo").message());
  EXPECT_EQ("bar", util::UnauthenticatedError("bar").message());

  EXPECT_EQ(util::StatusCode::kResourceExhausted,
            util::ResourceExhaustedError("").code());
  EXPECT_EQ("", util::ResourceExhaustedError("").message());
  EXPECT_EQ("foo", util::ResourceExhaustedError("foo").message());
  EXPECT_EQ("bar", util::ResourceExhaustedError("bar").message());

  EXPECT_EQ(util::StatusCode::kFailedPrecondition,
            util::FailedPreconditionError("").code());
  EXPECT_EQ("", util::FailedPreconditionError("").message());
  EXPECT_EQ("foo", util::FailedPreconditionError("foo").message());
  EXPECT_EQ("bar", util::FailedPreconditionError("bar").message());

  EXPECT_EQ(util::StatusCode::kAborted, util::AbortedError("").code());
  EXPECT_EQ("", util::AbortedError("").message());
  EXPECT_EQ("foo", util::AbortedError("foo").message());
  EXPECT_EQ("bar", util::AbortedError("bar").message());

  EXPECT_EQ(util::StatusCode::kOutOfRange, util::OutOfRangeError("").code());
  EXPECT_EQ("", util::OutOfRangeError("").message());
  EXPECT_EQ("foo", util::OutOfRangeError("foo").message());
  EXPECT_EQ("bar", util::OutOfRangeError("bar").message());

  EXPECT_EQ(util::StatusCode::kUnimplemented,
            util::UnimplementedError("").code());
  EXPECT_EQ("", util::UnimplementedError("").message());
  EXPECT_EQ("foo", util::UnimplementedError("foo").message());
  EXPECT_EQ("bar", util::UnimplementedError("bar").message());

  EXPECT_EQ(util::StatusCode::kInternal, util::InternalError("").code());
  EXPECT_EQ("", util::InternalError("").message());
  EXPECT_EQ("foo", util::InternalError("foo").message());
  EXPECT_EQ("bar", util::InternalError("bar").message());

  EXPECT_EQ(util::StatusCode::kUnavailable, util::UnavailableError("").code());
  EXPECT_EQ("", util::UnavailableError("").message());
  EXPECT_EQ("foo", util::UnavailableError("foo").message());
  EXPECT_EQ("bar", util::UnavailableError("bar").message());

  EXPECT_EQ(util::StatusCode::kDataLoss, util::DataLossError("").code());
  EXPECT_EQ("", util::DataLossError("").message());
  EXPECT_EQ("foo", util::DataLossError("foo").message());
  EXPECT_EQ("bar", util::DataLossError("bar").message());
}

TEST(Status, ConvenienceTests) {
  EXPECT_TRUE(util::OkStatus().ok());
  EXPECT_TRUE(util::IsCancelled(util::CancelledError("")));
  EXPECT_TRUE(util::IsUnknown(util::UnknownError("")));
  EXPECT_TRUE(util::IsInvalidArgument(util::InvalidArgumentError("")));
  EXPECT_TRUE(util::IsDeadlineExceeded(util::DeadlineExceededError("")));
  EXPECT_TRUE(util::IsNotFound(util::NotFoundError("")));
  EXPECT_TRUE(util::IsAlreadyExists(util::AlreadyExistsError("")));
  EXPECT_TRUE(util::IsPermissionDenied(util::PermissionDeniedError("")));
  EXPECT_TRUE(util::IsUnauthenticated(util::UnauthenticatedError("")));
  EXPECT_TRUE(util::IsResourceExhausted(util::ResourceExhaustedError("")));
  EXPECT_TRUE(util::IsFailedPrecondition(util::FailedPreconditionError("")));
  EXPECT_TRUE(util::IsAborted(util::AbortedError("")));
  EXPECT_TRUE(util::IsOutOfRange(util::OutOfRangeError("")));
  EXPECT_TRUE(util::IsUnimplemented(util::UnimplementedError("")));
  EXPECT_TRUE(util::IsInternal(util::InternalError("")));
  EXPECT_TRUE(util::IsUnavailable(util::UnavailableError("")));
  EXPECT_TRUE(util::IsDataLoss(util::DataLossError("")));
}

TEST(Status, Empty) {
  util::Status status;
  EXPECT_TRUE(status.ok());
  EXPECT_EQ(util::OkStatus(), status);
  EXPECT_EQ(util::StatusCode::kOk, status.code());
  EXPECT_EQ("OK", status.ToString());
}

TEST(Status, CheckOK) {
  util::Status status;
  GOOGLE_CHECK_OK(status);
  GOOGLE_CHECK_OK(status) << "Failed";
  GOOGLE_DCHECK_OK(status) << "Failed";
}

TEST(Status, ErrorMessage) {
  util::Status status = util::InvalidArgumentError("");
  EXPECT_FALSE(status.ok());
  EXPECT_EQ("", status.message().ToString());
  EXPECT_EQ("INVALID_ARGUMENT", status.ToString());
  status = util::InvalidArgumentError("msg");
  EXPECT_FALSE(status.ok());
  EXPECT_EQ("msg", status.message().ToString());
  EXPECT_EQ("INVALID_ARGUMENT:msg", status.ToString());
  status = util::Status(util::StatusCode::kOk, "msg");
  EXPECT_TRUE(status.ok());
  EXPECT_EQ("", status.message().ToString());
  EXPECT_EQ("OK", status.ToString());
}

TEST(Status, Copy) {
  util::Status a = util::UnknownError("message");
  util::Status b(a);
  ASSERT_EQ(a.ToString(), b.ToString());
}

TEST(Status, Assign) {
  util::Status a = util::UnknownError("message");
  util::Status b;
  b = a;
  ASSERT_EQ(a.ToString(), b.ToString());
}

TEST(Status, AssignEmpty) {
  util::Status a = util::UnknownError("message");
  util::Status b;
  a = b;
  ASSERT_EQ(std::string("OK"), a.ToString());
  ASSERT_TRUE(b.ok());
  ASSERT_TRUE(a.ok());
}

TEST(Status, EqualsOK) { ASSERT_EQ(util::OkStatus(), util::Status()); }

TEST(Status, EqualsSame) {
  const util::Status a = util::CancelledError("message");
  const util::Status b = util::CancelledError("message");
  ASSERT_EQ(a, b);
}

TEST(Status, EqualsCopy) {
  const util::Status a = util::CancelledError("message");
  const util::Status b = a;
  ASSERT_EQ(a, b);
}

TEST(Status, EqualsDifferentCode) {
  const util::Status a = util::CancelledError("message");
  const util::Status b = util::UnknownError("message");
  ASSERT_NE(a, b);
}

TEST(Status, EqualsDifferentMessage) {
  const util::Status a = util::CancelledError("message");
  const util::Status b = util::CancelledError("another");
  ASSERT_NE(a, b);
}

}  // namespace
}  // namespace protobuf
}  // namespace google
