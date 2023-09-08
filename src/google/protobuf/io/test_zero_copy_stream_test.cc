// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/io/test_zero_copy_stream.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/types/optional.h"

namespace google {
namespace protobuf {
namespace io {
namespace internal {
namespace {

using testing::ElementsAre;
using testing::Eq;
using testing::Optional;

absl::optional<std::string> CallNext(ZeroCopyInputStream& stream) {
  const void* data;
  int size;
  if (stream.Next(&data, &size)) {
    return std::string(static_cast<const char*>(data),
                       static_cast<size_t>(size));
  }
  return absl::nullopt;
}

std::vector<std::string> ReadLeftoverDoNotConsumeInput(
    TestZeroCopyInputStream copy) {
  std::vector<std::string> out;
  while (auto next = CallNext(copy)) {
    out.push_back(*std::move(next));
  }
  return out;
}

#if GTEST_HAS_DEATH_TEST
TEST(TestZeroCopyInputStreamTest, NextChecksPreconditions) {
  std::unique_ptr<ZeroCopyInputStream> stream =
      std::make_unique<TestZeroCopyInputStream>(std::vector<std::string>{});
  const void* data;
  int size;
  EXPECT_DEATH(stream->Next(nullptr, &size), "data must not be null");
  EXPECT_DEATH(stream->Next(&data, nullptr), "size must not be null");
}
#endif  // GTEST_HAS_DEATH_TEST

TEST(TestZeroCopyInputStreamTest, NextProvidesTheBuffersCorrectly) {
  std::vector<std::string> expected = {"ABC", "D", "EFG", "", "", "HIJKLMN"};
  std::unique_ptr<ZeroCopyInputStream> stream =
      std::make_unique<TestZeroCopyInputStream>(expected);

  std::vector<std::string> found;
  while (auto next = CallNext(*stream)) {
    found.push_back(*std::move(next));
  }

  EXPECT_EQ(found, expected);
}

TEST(TestZeroCopyInputStreamTest, BackUpGivesBackABuffer) {
  std::vector<std::string> expected = {"ABC", "D", "EFG", "", "", "HIJKLMN"};
  std::unique_ptr<ZeroCopyInputStream> stream =
      std::make_unique<TestZeroCopyInputStream>(expected);

  EXPECT_THAT(CallNext(*stream), Optional(Eq("ABC")));
  stream->BackUp(3);
  EXPECT_THAT(CallNext(*stream), Optional(Eq("ABC")));
  stream->BackUp(2);
  EXPECT_THAT(CallNext(*stream), Optional(Eq("BC")));
  EXPECT_THAT(CallNext(*stream), Optional(Eq("D")));
  stream->BackUp(1);
  EXPECT_THAT(CallNext(*stream), Optional(Eq("D")));
  stream->BackUp(0);
  EXPECT_THAT(CallNext(*stream), Optional(Eq("")));
  EXPECT_THAT(CallNext(*stream), Optional(Eq("EFG")));
  EXPECT_THAT(CallNext(*stream), Optional(Eq("")));
  EXPECT_THAT(CallNext(*stream), Optional(Eq("")));
  EXPECT_THAT(CallNext(*stream), Optional(Eq("HIJKLMN")));
  stream->BackUp(2);
  EXPECT_THAT(CallNext(*stream), Optional(Eq("MN")));
  EXPECT_THAT(CallNext(*stream), Eq(absl::nullopt));
}

#if GTEST_HAS_DEATH_TEST
TEST(TestZeroCopyInputStreamTest, BackUpChecksPreconditions) {
  std::vector<std::string> expected = {"ABC", "D", "EFG", "", "", "HIJKLMN"};
  std::unique_ptr<ZeroCopyInputStream> stream =
      std::make_unique<TestZeroCopyInputStream>(expected);

  EXPECT_DEATH(stream->BackUp(0),
               "The last call was not a successful Next\\(\\)");
  EXPECT_THAT(CallNext(*stream), Optional(Eq("ABC")));
  EXPECT_DEATH(stream->BackUp(-1), "count must not be negative");
  stream->BackUp(1);
  EXPECT_DEATH(stream->BackUp(0),
               "The last call was not a successful Next\\(\\)");
  EXPECT_THAT(CallNext(*stream), Optional(Eq("C")));
  EXPECT_THAT(CallNext(*stream), Optional(Eq("D")));
  stream->Skip(1);
  EXPECT_DEATH(stream->BackUp(0),
               "The last call was not a successful Next\\(\\)");
  EXPECT_THAT(CallNext(*stream), Optional(Eq("FG")));
  EXPECT_THAT(CallNext(*stream), Optional(Eq("")));
  EXPECT_THAT(CallNext(*stream), Optional(Eq("")));
  EXPECT_THAT(CallNext(*stream), Optional(Eq("HIJKLMN")));
  EXPECT_DEATH(stream->BackUp(8), "count must be within bounds of last buffer");
  EXPECT_THAT(CallNext(*stream), Eq(absl::nullopt));
  EXPECT_DEATH(stream->BackUp(0),
               "The last call was not a successful Next\\(\\)");
}
#endif  // GTEST_HAS_DEATH_TEST

TEST(TestZeroCopyInputStreamTest, SkipWorks) {
  std::vector<std::string> expected = {"ABC", "D", "EFG", "", "", "HIJKLMN"};
  TestZeroCopyInputStream stream(expected);

  EXPECT_THAT(ReadLeftoverDoNotConsumeInput(stream),
              ElementsAre("ABC", "D", "EFG", "", "", "HIJKLMN"));
  // Skip nothing
  EXPECT_TRUE(stream.Skip(0));
  EXPECT_THAT(ReadLeftoverDoNotConsumeInput(stream),
              ElementsAre("ABC", "D", "EFG", "", "", "HIJKLMN"));
  // Skip less than one chunk
  EXPECT_TRUE(stream.Skip(1));
  EXPECT_THAT(ReadLeftoverDoNotConsumeInput(stream),
              ElementsAre("BC", "D", "EFG", "", "", "HIJKLMN"));
  // Skip exactly one chunk
  EXPECT_TRUE(stream.Skip(2));
  EXPECT_THAT(ReadLeftoverDoNotConsumeInput(stream),
              ElementsAre("D", "EFG", "", "", "HIJKLMN"));
  // Skip cross chunks
  EXPECT_TRUE(stream.Skip(3));
  EXPECT_THAT(ReadLeftoverDoNotConsumeInput(stream),
              ElementsAre("G", "", "", "HIJKLMN"));
  // Skip the rest
  EXPECT_TRUE(stream.Skip(8));
  EXPECT_THAT(ReadLeftoverDoNotConsumeInput(stream), ElementsAre());
  // Skipping zero works on empty
  EXPECT_TRUE(stream.Skip(0));
  // but skipping non-zero does not
  EXPECT_FALSE(stream.Skip(1));
}

#if GTEST_HAS_DEATH_TEST
TEST(TestZeroCopyInputStreamTest, SkipChecksPreconditions) {
  std::unique_ptr<ZeroCopyInputStream> stream =
      std::make_unique<TestZeroCopyInputStream>(std::vector<std::string>{});
  EXPECT_DEATH(stream->Skip(-1), "count must not be negative");
}
#endif  // GTEST_HAS_DEATH_TEST

TEST(TestZeroCopyInputStreamTest, ByteCountWorks) {
  std::vector<std::string> expected = {"ABC", "D", "EFG", "", "", "HIJKLMN"};
  TestZeroCopyInputStream stream(expected);
  EXPECT_EQ(stream.ByteCount(), 0);
  EXPECT_TRUE(stream.Skip(0));
  EXPECT_EQ(stream.ByteCount(), 0);
  EXPECT_TRUE(stream.Skip(1));
  EXPECT_EQ(stream.ByteCount(), 1);
  EXPECT_THAT(CallNext(stream), Optional(Eq("BC")));
  EXPECT_EQ(stream.ByteCount(), 3);
  stream.BackUp(1);
  EXPECT_EQ(stream.ByteCount(), 2);
  EXPECT_THAT(CallNext(stream), Optional(Eq("C")));
  EXPECT_EQ(stream.ByteCount(), 3);
  EXPECT_THAT(CallNext(stream), Optional(Eq("D")));
  EXPECT_EQ(stream.ByteCount(), 4);
  EXPECT_THAT(CallNext(stream), Optional(Eq("EFG")));
  EXPECT_EQ(stream.ByteCount(), 7);
  EXPECT_THAT(CallNext(stream), Optional(Eq("")));
  EXPECT_EQ(stream.ByteCount(), 7);
  EXPECT_THAT(CallNext(stream), Optional(Eq("")));
  EXPECT_EQ(stream.ByteCount(), 7);
  EXPECT_TRUE(stream.Skip(3));
  EXPECT_EQ(stream.ByteCount(), 10);
  EXPECT_TRUE(stream.Skip(4));
  EXPECT_EQ(stream.ByteCount(), 14);
}

}  // namespace
}  // namespace internal
}  // namespace io
}  // namespace protobuf
}  // namespace google
