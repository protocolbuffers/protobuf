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

#include "google/protobuf/json/internal/zero_copy_buffered_stream.h"

#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "google/protobuf/json/internal/test_input_stream.h"
#include "google/protobuf/stubs/status_macros.h"

namespace google {
namespace protobuf {
namespace json_internal {
namespace {
// TODO(b/234474291): Use the gtest versions once that's available in OSS.
MATCHER_P(IsOkAndHolds, inner,
          absl::StrCat("is OK and holds ", testing::PrintToString(inner))) {
  if (!arg.ok()) {
    *result_listener << arg.status();
    return false;
  }
  return testing::ExplainMatchResult(inner, *arg, result_listener);
}

absl::Status GetStatus(const absl::Status& s) { return s; }
template <typename T>
absl::Status GetStatus(const absl::StatusOr<T>& s) {
  return s.status();
}

MATCHER_P(StatusIs, status,
          absl::StrCat(".status() is ", testing::PrintToString(status))) {
  return GetStatus(arg).code() == status;
}

#define EXPECT_OK(x) EXPECT_THAT(x, StatusIs(absl::StatusCode::kOk))
#define ASSERT_OK(x) ASSERT_THAT(x, StatusIs(absl::StatusCode::kOk))

TEST(ZcBufferTest, ReadUnbuffered) {
  TestInputStream in{"foo", "bar", "baz"};
  ZeroCopyBufferedStream stream(&in);

  {
    auto chunk = stream.Take(3);
    EXPECT_FALSE(stream.IsBuffering());
    EXPECT_THAT(chunk, IsOkAndHolds("foo"));
  }

  {
    auto chunk = stream.Take(3);
    EXPECT_FALSE(stream.IsBuffering());
    EXPECT_THAT(chunk, IsOkAndHolds("bar"));
  }

  {
    auto chunk = stream.Take(3);
    EXPECT_FALSE(stream.IsBuffering());
    EXPECT_THAT(chunk, IsOkAndHolds("baz"));
  }
}

TEST(ZcBufferTest, ReadBuffered) {
  TestInputStream in{"foo", "bar", "baz"};
  ZeroCopyBufferedStream stream(&in);

  {
    auto chunk = stream.Take(4);
    EXPECT_TRUE(stream.IsBuffering());
    EXPECT_THAT(chunk, IsOkAndHolds("foob"));
  }

  {
    auto chunk = stream.Take(2);
    EXPECT_FALSE(stream.IsBuffering());
    EXPECT_THAT(chunk, IsOkAndHolds("ar"));
  }
}

TEST(ZcBufferTest, HoldAcrossSeam) {
  TestInputStream in{"foo", "bar", "baz"};
  ZeroCopyBufferedStream stream(&in);

  auto chunk = stream.Take(3);
  EXPECT_FALSE(stream.IsBuffering());
  EXPECT_THAT(chunk, IsOkAndHolds("foo"));

  auto chunk2 = stream.Take(3);
  EXPECT_TRUE(stream.IsBuffering());
  EXPECT_THAT(chunk2, IsOkAndHolds("bar"));
  EXPECT_THAT(chunk, IsOkAndHolds("foo"));
}

TEST(ZcBufferTest, BufferAcrossSeam) {
  TestInputStream in{"foo", "bar", "baz"};
  ZeroCopyBufferedStream stream(&in);

  auto chunk = stream.Take(2);
  EXPECT_FALSE(stream.IsBuffering());
  EXPECT_THAT(chunk, IsOkAndHolds("fo"));

  auto chunk2 = stream.Take(3);
  EXPECT_TRUE(stream.IsBuffering());
  EXPECT_THAT(chunk2, IsOkAndHolds("oba"));
  EXPECT_THAT(chunk, IsOkAndHolds("fo"));
}

TEST(ZcBufferTest, MarkUnbuffered) {
  TestInputStream in{"foo", "bar", "baz"};
  ZeroCopyBufferedStream stream(&in);

  ASSERT_OK(stream.Advance(1));
  auto mark = stream.BeginMark();
  ASSERT_OK(stream.Advance(2));
  EXPECT_FALSE(stream.IsBuffering());
  EXPECT_THAT(mark.UpToUnread(), "oo");
}

TEST(ZcBufferTest, MarkBuffered) {
  TestInputStream in{"foo", "bar", "baz"};
  ZeroCopyBufferedStream stream(&in);

  ASSERT_OK(stream.Advance(1));
  auto mark = stream.BeginMark();
  ASSERT_OK(stream.Advance(5));
  EXPECT_TRUE(stream.IsBuffering());
  EXPECT_THAT(mark.UpToUnread(), "oobar");
}
}  // namespace
}  // namespace json_internal
}  // namespace protobuf
}  // namespace google
