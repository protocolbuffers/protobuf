// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
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


template <typename DataT, typename PeekFunc>
void TestAdvancePtrSinkingAndPeeking(
    const std::vector<std::string>& input, std::vector<int> advance_by,
    const PeekFunc& peek_func = [](absl::string_view) { return true; }) {
  std::string flattened = absl::StrJoin(input, "");
  // To fully traverse the input.
  int available = static_cast<int>(flattened.size());
  // To ensure that we can cover the entire data.
  advance_by.push_back(available);

  io::internal::TestZeroCopyInputStream stream(input);
  const char* ptr;
  ParseContext ctx(io::CodedInputStream::GetDefaultRecursionLimit(), false,
                   &ptr, &stream);
  if (ctx.Done(&ptr)) return;

  WireFormatStringSink sink(ptr);
  sink.data.reserve(flattened.size());
  for (auto advance : advance_by) {
    int adv = std::min(available, advance);
    available -= adv;
    ABSL_CHECK_GE(adv, 0);
    ptr = ctx.AdvancePtrMaybeFlush<DataT>(ptr, adv, sink, peek_func);
    if (!ptr) break;

    if (available == 0) {
      sink.Flush(ptr);
      break;
    }
  }

  // Output must match the flattened input.
  EXPECT_EQ(sink.data, flattened);
}

void TestAdvancePtrPeeking(const std::vector<std::string>& input,
                           std::vector<int> advance_by) {
  // Peeked data must match the flattened input.
  std::string peeked;

  TestAdvancePtrSinkingAndPeeking<char>(input, advance_by,
                                        [&peeked](absl::string_view view) {
                                          absl::StrAppend(&peeked, view);
                                          return true;
                                        });

  EXPECT_EQ(peeked, absl::StrJoin(input, ""));
}

TEST(ParseAndFlushTest, SmallViewInput) {
  TestAdvancePtrPeeking({"hello world"}, {});
}

TEST(ParseAndFlushTest, SmallViewInputIncremental) {
  TestAdvancePtrPeeking({"hello world"}, {1, 2, 3, 4, 5});
}

TEST(ParseAndFlushTest, LargeViewInput) {
  constexpr absl::string_view kPayload =
      "hello world, a valid UTF8 payload #1"
      "hello world, a valid UTF8 payload #2"
      "hello world, a valid UTF8 payload #3"
      "hello world, a valid UTF8 payload #4"
      "hello world, a valid UTF8 payload #5";

  TestAdvancePtrPeeking({std::string{kPayload}}, {});
}

TEST(ParseAndFlushTest, LargeViewInputIncremental) {
  constexpr absl::string_view kPayload =
      "hello world, a valid UTF8 payload #1"
      "hello world, a valid UTF8 payload #2"
      "hello world, a valid UTF8 payload #3"
      "hello world, a valid UTF8 payload #4"
      "hello world, a valid UTF8 payload #5";

  TestAdvancePtrPeeking({std::string{kPayload}}, {1, 3, 5, 7, 9, 11});
}


TEST(ParseAndFlush, TestAdvancePtrPeekingRegression1) {
  // "ptr" from ParseContext may not be flush-ready if the fragmented input has
  // empty fragments. This is not plausible in practice.
  TestAdvancePtrPeeking({"", "", "", "", "0", "pppppp", ""}, {50});
}

TEST(ParseAndFlush, TestAdvancePtrPeekingRegression2) {
  // From fragmented input, advancing ptr by greater than kSlopBytes is not
  // safe.
  TestAdvancePtrPeeking(
      {"", "\364\364", "###", "\022", "\223\036", "\366", "\366", "\366",
       "\366", "\366", "\366", "\366", "\366", "\366", "\024", "\223"},
      {1, 50});
}

TEST(ParseAndFlush, TestAdvancePtrPeekingRegression3) {
  TestAdvancePtrPeeking({"\315\315", ""}, {3});
}

TEST(ParseAndFlush, TestAdvancePtrPeekingRegression4) {
  TestAdvancePtrPeeking({"\323", "\323", "", "\033", "", ""}, {});
}

TEST(ParseAndFlush, TestAdvancePtrPeekingRegression5) {
  TestAdvancePtrPeeking(
      {"9", "9", "9", "9", "9", "9", "9", "9", "9", "9", "9", "9", "9",
       "9", "9", "9", "9", "9", "9", "9", "9", "9", "9", "9", "9", "9"},
      {2});
}

TEST(ParseAndFlush, TestAdvancePtrPeekingRegression6) {
  TestAdvancePtrPeeking(
      {"\314", "\314", "\314", "\314", "\314", "\314", "\314", "\327", "\327",
       "\327", "\327", "\327", "\327", "\314", "\314", "\314", "\271", "\017"},
      {16, 16, 16, 6, 16, 12, 16, 16, 16, 16, 16,
       16, 16, 16, 2, 6,  16, 16, 16, 16, 16, 16});
}

TEST(ParseAndFlush, TestAdvancePtrPeekingRegression7) {
  TestAdvancePtrPeeking(
      {"", "\244\030",
       "\361\361\361\361\361\361\361\361\361\361\361\361\361\361\361\361\361"
       "\361\361\361\361\361\361\361",
       "\301", "9\300", "\332\332\332\332\332\330\330\300", "\250", "\235", "",
       "\026"},
      {13, 1, 1, 9,  1,  12, 16, 14, 8, 7,  6, 8, 1,  14, 16, 1, 16, 13, 16,
       1,  1, 1, 14, 16, 4,  1,  1,  9, 12, 1, 1, 16, 1,  1,  1, 16, 7});
}

TEST(ParseAndFlush, TestAdvancePtrPeekingRegression8) {
  TestAdvancePtrPeeking(
      {"", "\343\343\343\343\343\343\343",
       "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||", "&",
       "\360", "\207\207\207\207\207\207\207\207\207\207", "\352", "\340", ",",
       "((((((", "\275", ""},
      {1,  1,  8,  4,  1,  1,  4,  4,  4,  4,  4,  10, 10, 14,
       13, 7,  3,  16, 11, 12, 16, 15, 16, 10, 15, 15, 14, 14,
       11, 14, 10, 2,  16, 14, 14, 14, 8,  14, 14, 14});
}

TEST(ParseAndFlush, TestAdvancePtrPeekingRegression9) {
  TestAdvancePtrPeeking(
      {"D", "", "NNN", "J", "\270", "\200\200", "\021\021", "\320\320\320",
       "\227",
       "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
       "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
       "ff",
       "\331", "&", "L", "", "\237", "", "\311", "\353"},
      {96, 96, 96, 96, 96, 96, 96, 96, 96, 96,  96, 96, 96, 96, 96, 96,
       96, 30, 30, 30, 30, 30, 30, 30, 28, 30,  30, 30, 30, 30, 30, 1,
       26, 9,  31, 57, 38, 34, 97, 96, 96, 100, 96, 96, 96, 96, 96, 100,
       96, 96, 7,  99, 96, 96, 96, 96, 96, 96,  96, 96, 1,  96, 96, 96,
       96, 96, 96, 96, 96, 96, 96, 44, 96, 61,  66, 14});
}

void VerifyUTF8AndCopy(const std::vector<std::string>& input) {
  std::string flattened = absl::StrJoin(input, "");
  // To fully traverse the input.
  io::internal::TestZeroCopyInputStream stream(input);
  const char* ptr;
  ParseContext ctx(io::CodedInputStream::GetDefaultRecursionLimit(), false,
                   &ptr, &stream);

  WireFormatStringSink sink(ptr);
  sink.data.reserve(flattened.size());
  ptr = ctx.VerifyUTF8MaybeFlush(ptr, static_cast<int64_t>(flattened.size()),
                                 sink);
  if (ptr == nullptr) {
    EXPECT_FALSE(utf8_range::IsStructurallyValid(flattened));
    return;
  }

  sink.Flush(ptr);

  // UTF8 validation must match the flattened input.
  EXPECT_TRUE(utf8_range::IsStructurallyValid(flattened));
  // If successful, the output must match the flattened input.
  EXPECT_EQ(sink.data, flattened);
}

TEST(VerifyUTF8AndFlush, ValidInput) { VerifyUTF8AndCopy({"hello world!"}); }

TEST(VerifyUTF8AndFlush, InvalidInput) {
  constexpr absl::string_view kInvalidUtf8 = "\xe2\x82\x28";
  VerifyUTF8AndCopy({std::string(kInvalidUtf8)});
}

TEST(VerifyUTF8AndFlush, InvalidSplit) {
  VerifyUTF8AndCopy({std::string("\xe2\x82"), std::string("\x28")});
}

TEST(VerifyUTF8AndFlush, IncompleteUtf8) {
  ASSERT_FALSE(utf8_range::IsStructurallyValid("\xf0"));
  VerifyUTF8AndCopy({std::string("\xf0")});
}

TEST(VerifyUTF8AndFlush, InvalidSeparatelyButValidTogether) {
  ASSERT_TRUE(utf8_range::IsStructurallyValid("\xf0\x9f\x98\x80"));
  VerifyUTF8AndCopy({std::string("\xf0"), std::string("\x9f\x98\x80")});
}


}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
