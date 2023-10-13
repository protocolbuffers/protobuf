// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
// TODO: Use the gtest versions once that's available in OSS.
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
