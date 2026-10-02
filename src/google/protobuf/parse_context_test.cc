// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/parse_context.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/cord.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/io/test_zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/unittest.pb.h"
#include "utf8_validity.h"

namespace google {
namespace protobuf {
namespace internal {
namespace {

constexpr int kTooLargeSize =
    static_cast<int>(std::numeric_limits<uint32_t>::max());
constexpr int kMaxLimit = std::numeric_limits<int>::max();

TEST(ParseContextTest, NoOverflowMaxDepth) {
  constexpr char kPayload[] =
      "\x81\x81\x81\x81\x08\x01\x00";  // Malformed varint size.

  const char* ptr;
  ParseContext ctx(kMaxLimit, false, &ptr, kPayload);
  proto2_unittest::TestAllTypes proto;
  const char* result = ctx.ParseMessage(&proto, kPayload);

  ASSERT_EQ(result, nullptr);
}

TEST(ParseContextTest, VerifyUTF8) {
  constexpr absl::string_view kPayload = "hello world, a valid UTF8 payload";

  const char* ptr;
  ParseContext ctx(kMaxLimit, false, &ptr, kPayload);

  ptr = ctx.VerifyUTF8(ptr, static_cast<int>(kPayload.size()));

  EXPECT_NE(ptr, nullptr);
  EXPECT_EQ(ctx.DataAvailable(ptr), 0);
}

TEST(ParseContextTest, VerifyFragmentedUTF8) {
  constexpr absl::string_view kPayload =
      "hello world, a valid UTF8 payload #1"
      "hello world, a valid UTF8 payload #2"
      "hello world, a valid UTF8 payload #3"
      "hello world, a valid UTF8 payload #4"
      "hello world, a valid UTF8 payload #5";

  const char* ptr;
  io::ArrayInputStream ais(kPayload.data(), kPayload.size(), /*block_size*/ 13);
  ParseContext ctx(kMaxLimit, false, &ptr, &ais);

  ptr = ctx.VerifyUTF8(ptr, static_cast<int>(kPayload.size()));

  EXPECT_NE(ptr, nullptr);
  EXPECT_EQ(ctx.DataAvailable(ptr), 0);
}

TEST(ParseContextTest, FailsOnInvalidUTF8) {
  constexpr absl::string_view kInvalidUtf8 = "\xe2\x82\x28";

  const char* ptr;
  ParseContext ctx(kMaxLimit, false, &ptr, kInvalidUtf8);

  ptr = ctx.VerifyUTF8(ptr, static_cast<int>(kInvalidUtf8.size()));

  EXPECT_EQ(ptr, nullptr);
}

TEST(ParseContextTest, FailsOnFragmentedInvalidUTF8) {
  constexpr absl::string_view kInvalidUtf8 =
      "hello world, a valid UTF8 payload #1"
      "hello world, a valid UTF8 payload #2"
      "hello world, a valid UTF8 payload #3"
      "hello world, a valid UTF8 payload #4"
      "hello world, a valid UTF8 payload #5\xe2\x82\x28";

  const char* ptr;
  io::ArrayInputStream ais(kInvalidUtf8.data(), kInvalidUtf8.size(),
                           /*block_size*/ 13);
  ParseContext ctx(kMaxLimit, false, &ptr, &ais);

  ptr = ctx.VerifyUTF8(ptr, static_cast<int>(kInvalidUtf8.size()));

  EXPECT_EQ(ptr, nullptr);
  EXPECT_GT(ctx.DataAvailable(ptr), 0);
}

TEST(ParseContextTest, SkipTooLarge) {
  const char* ptr;
  ParseContext ctx(kMaxLimit, false, &ptr, "hello world");

  ptr = ctx.Skip(ptr, kTooLargeSize);

  EXPECT_EQ(ptr, nullptr);
}

TEST(ParseContextTest, ReadStringTooLarge) {
  const char* ptr;
  ParseContext ctx(kMaxLimit, false, &ptr, "hello world");
  std::string out;

  ptr = ctx.ReadString(ptr, kTooLargeSize, &out);

  EXPECT_EQ(ptr, nullptr);
}

TEST(ParseContextTest, AppendStringTooLarge) {
  const char* ptr;
  ParseContext ctx(kMaxLimit, false, &ptr, "hello world");
  std::string out;

  ptr = ctx.AppendString(ptr, kTooLargeSize, &out);

  EXPECT_EQ(ptr, nullptr);
}

TEST(ParseContextTest, ReadCordTooLarge) {
  const char* ptr;
  ParseContext ctx(kMaxLimit, false, &ptr, "hello world");
  absl::Cord out;

  ptr = ctx.ReadCord(ptr, kTooLargeSize, &out);

  EXPECT_EQ(ptr, nullptr);
}

TEST(ParseContextTest, VerifyUTF8TooLarge) {
  const char* ptr;
  ParseContext ctx(kMaxLimit, false, &ptr, "hello world");

  ptr = ctx.VerifyUTF8(ptr, static_cast<uint32_t>(kTooLargeSize));

  EXPECT_EQ(ptr, nullptr);
}

TEST(ParseContextTest, ReadChunkAndCallbackTooLarge) {
  const char* ptr;
  ParseContext ctx(kMaxLimit, false, &ptr, "hello world");

  ptr = ctx.ReadChunkAndCallback(ptr, kTooLargeSize,
                                 [&](const char* ptr, int s) {});

  EXPECT_EQ(ptr, nullptr);
}

TEST(ParseContextTest, ReadCordMustWorkAcrossFragments) {
  const char* ptr;
  io::internal::TestZeroCopyInputStream stream({"hello ", "world"});
  ParseContext ctx(kMaxLimit, false, &ptr, &stream);
  absl::Cord out;
  ptr = ctx.ReadCord(ptr, 7, &out);
  EXPECT_EQ(out, "hello w");
}

TEST(ParseContextTest, ReadCordFromMultipleFragments) {
  const char* ptr;
  io::internal::TestZeroCopyInputStream stream({"a", "bc", "def"});
  ParseContext ctx(kMaxLimit, false, &ptr, &stream);
  absl::Cord out;
  ptr = ctx.ReadCord(ptr, 6, &out);
  EXPECT_EQ(out, "abcdef");
}

TEST(ParseContextTest, ReadCordFromMultipleFragmentsWithEmptyFragment) {
  const char* ptr;
  io::internal::TestZeroCopyInputStream stream({"a", "bc", "", "", "def"});
  ParseContext ctx(kMaxLimit, false, &ptr, &stream);
  absl::Cord out;
  ptr = ctx.ReadCord(ptr, 6, &out);
  EXPECT_EQ(out, "abcdef");
}


}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
