// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/generated_enum_util.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <ostream>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/container/btree_set.h"
#include "absl/strings/str_format.h"
#include "absl/types/span.h"
#include "google/protobuf/descriptor.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

using testing::_;
using testing::ElementsAre;
using testing::Gt;
using testing::IsEmpty;
using testing::SizeIs;

namespace google {
namespace protobuf {
namespace internal {
namespace {

TEST(GenerateEnumDataTest, DebugChecks) {
#if GTEST_HAS_DEATH_TEST
  // Not unique
  EXPECT_DEBUG_DEATH(GenerateEnumData({1, 1}), "sorted_and_unique");
  // Not sorted
  EXPECT_DEBUG_DEATH(GenerateEnumData({2, 1}), "sorted_and_unique");
#endif
}

uint32_t Make32(uint16_t a, uint16_t b) { return a | (b << 16); }
std::array<uint16_t, 2> Unmake32(uint32_t v) {
  return {static_cast<uint16_t>(v), static_cast<uint16_t>(v >> 16)};
}

struct Header {
  int16_t sequence_start;
  uint16_t sequence_length;
  uint16_t bitmap_length;
  uint16_t ordered_length;

  std::string ToString() const {
    return absl::StrFormat("(%d,%d,%d,%d)", sequence_start, sequence_length,
                           bitmap_length, ordered_length);
  }
  friend std::ostream& operator<<(std::ostream& os, Header header) {
    return os << header.ToString();
  }
};

MATCHER_P4(HeaderHas, sequence_start, sequence_length, bitmap_length,
           ordered_length, "") {
  return testing::ExplainMatchResult(sequence_start, arg.sequence_start,
                                     result_listener) &&
         testing::ExplainMatchResult(sequence_length, arg.sequence_length,
                                     result_listener) &&
         testing::ExplainMatchResult(bitmap_length, arg.bitmap_length,
                                     result_listener) &&
         testing::ExplainMatchResult(ordered_length, arg.ordered_length,
                                     result_listener);
}

Header ExtractHeader(absl::Span<const uint32_t> data) {
  return {
      static_cast<int16_t>(Unmake32(data[0])[0]),
      Unmake32(data[0])[1],
      Unmake32(data[1])[0],
      Unmake32(data[1])[1],
  };
}

TEST(GenerateEnumDataTest, BitmapSpaceOptimizationWorks) {
  std::vector<int32_t> values = {0};

  auto encoded = GenerateEnumData(values);
  EXPECT_THAT(ExtractHeader(encoded), HeaderHas(0, 1, 0, 0));
  EXPECT_THAT(encoded, SizeIs(2));

  // Adding one large value puts it on the fallback.
  values.push_back(100);
  encoded = GenerateEnumData(values);
  EXPECT_THAT(ExtractHeader(encoded), HeaderHas(0, 1, 0, 1));
  EXPECT_THAT(encoded, SizeIs(3));

  // Adding a second one still prefers the fallback.
  values.push_back(101);
  encoded = GenerateEnumData(values);
  EXPECT_THAT(ExtractHeader(encoded), HeaderHas(0, 1, 0, 2));
  EXPECT_THAT(encoded, SizeIs(4));

  // Adding two more now makes bitmap more efficient, so they are collapsed
  // to it. Because we can fit the bitmap in 128 bits, which is the same as the
  // ints.
  values.push_back(102);
  values.push_back(103);
  encoded = GenerateEnumData(values);
  EXPECT_THAT(ExtractHeader(encoded), HeaderHas(0, 1, 128, 0));
  EXPECT_THAT(encoded, SizeIs(6));

  // Add one value that falls into the existing bitmap, nothing changes.
  values.push_back(104);
  encoded = GenerateEnumData(values);
  EXPECT_THAT(ExtractHeader(encoded), HeaderHas(0, 1, 128, 0));
  EXPECT_THAT(encoded, SizeIs(6));

  // Add one value that is in the next 32 bits. It should grow the bitmap.
  values.push_back(130);
  encoded = GenerateEnumData(values);
  EXPECT_THAT(ExtractHeader(encoded), HeaderHas(0, 1, 160, 0));
  EXPECT_THAT(encoded, SizeIs(7));

  // Add one value far away, it should go into fallback.
  values.push_back(200);
  encoded = GenerateEnumData(values);
  EXPECT_THAT(ExtractHeader(encoded), HeaderHas(0, 1, 160, 1));
  EXPECT_THAT(encoded, SizeIs(8));

  // Another in the next 32-bit block will still make them fallback.
  values.push_back(230);
  encoded = GenerateEnumData(values);
  EXPECT_THAT(ExtractHeader(encoded), HeaderHas(0, 1, 160, 2));
  EXPECT_THAT(encoded, SizeIs(9));

  // One more in that same block should collapse them to bitmap.
  values.push_back(231);
  encoded = GenerateEnumData(values);
  EXPECT_THAT(ExtractHeader(encoded), HeaderHas(0, 1, 256, 0));
  EXPECT_THAT(encoded, SizeIs(10));
}

void GatherValidValues(absl::Span<const uint32_t> data, int32_t min,
                       int32_t max, absl::btree_set<int32_t>& out) {
  if (min >= max) return;
  for (int32_t i = min;; ++i) {
    if (ValidateEnum(i, data.begin())) out.insert(i);
    // We check the top limit before ++i to avoid overflows
    if (i == max) break;
  }
}

std::vector<int32_t> GetValidValues(absl::Span<const uint32_t> data,
                                    int32_t min, int32_t max) {
  // Btree to keep them sorted. Makes testing easier.
  absl::btree_set<int32_t> s;
  GatherValidValues(data, min, max, s);
  return std::vector<int32_t>(s.begin(), s.end());
}

TEST(ValidateEnumTest, SequentialRangeTest) {
  EXPECT_THAT(GetValidValues({0, 0}, -100, 100), ElementsAre());
  EXPECT_THAT(GetValidValues(
                  {// sequence start=3, length=3
                   Make32(5, 3),
                   // no bitmap, no fallback
                   Make32(0, 0)},
                  -100, 100),
              ElementsAre(5, 6, 7));
  EXPECT_THAT(GetValidValues(
                  {// sequence start=-2, length=10
                   Make32(-2, 10),
                   // no bitmap, no fallback
                   Make32(0, 0)},
                  -100, 100),
              ElementsAre(-2, -1, 0, 1, 2, 3, 4, 5, 6, 7));
}

TEST(ValidateEnumTest, BitmapRangeTest) {
  EXPECT_THAT(GetValidValues(
                  {// no sequence
                   Make32(0, 0),
                   // bitmap of 32 bits, no fallback
                   Make32(32, 0),
                   // bitmap
                   0b10011010101},
                  -100, 100),
              ElementsAre(0, 2, 4, 6, 7, 10));
  EXPECT_THAT(GetValidValues(
                  {// no sequence
                   Make32(0, 0),
                   // bitmap of 64 bits, no fallback
                   Make32(64, 0),
                   // bitmap
                   (1 << 4) | (1 << 21), 1 << 6},
                  -100, 100),
              ElementsAre(4, 21, 32 + 6));
}

TEST(ValidateEnumTest, GenerateEnumDataSequential) {
  EXPECT_THAT(GenerateEnumData({0, 1, 2, 3}), ElementsAre(
                                                  // sequence start=0, length=4
                                                  Make32(0, 4),
                                                  // no bitmap, no fallback.
                                                  Make32(0, 0)));
  EXPECT_THAT(GenerateEnumData({-2, -1, 0, 1, 2, 3}),
              ElementsAre(
                  // sequence start=-2, length=6
                  Make32(-2, 6),
                  // no bitmap, no fallback.
                  Make32(0, 0)));
}

void TestRoundTrip(absl::Span<const int32_t> values, int line) {
  auto encoded = GenerateEnumData(values);

  absl::btree_set<int32_t> s;

  // We test that all elements in `values` exist in the encoded data, and also
  // test a range of other values to verify that they do not exist in the
  // encoded data.

  // We keep track of the max seen to avoid testing the same values many times.
  int64_t max_seen = std::numeric_limits<int64_t>::min();
  const auto gather_valid_values_around = [&](int32_t v) {
    int32_t min = std::max({
        static_cast<int64_t>(v) - 100,
        static_cast<int64_t>(std::numeric_limits<int32_t>::min()),
        max_seen,
    });
    int32_t max =
        std::min(static_cast<int64_t>(v) + 100,
                 static_cast<int64_t>(std::numeric_limits<int32_t>::max()));
    max_seen = std::max(max_seen, int64_t{max});
    GatherValidValues(encoded, min, max, s);
  };

  // We look at a few values around the expected ones.
  // We could in theory test the whole int32_t domain, but that takes too long
  // to run.
  for (int32_t v : values) {
    gather_valid_values_around(v);
  }
  // Also gather some around 0, just to add more coverage, specially when
  // `values` is empty.
  gather_valid_values_around(0);

  // Skip the checks below if we are correct because they are expensive.
  if (std::equal(s.begin(), s.end(), values.begin(), values.end())) return;

  std::vector<int32_t> false_negatives;
  for (int32_t v : values) {
    if (!ValidateEnum(v, encoded.data())) false_negatives.push_back(v);
    s.erase(v);
  }
  const auto& false_positives = s;
  const auto print_data = [&] {
    auto header = ExtractHeader(encoded);
    return absl::StrFormat("line=%d header=%s", line, header.ToString());
  };
  EXPECT_THAT(false_negatives, IsEmpty())
      << "Missing values from the input. " << print_data()
      << "\nEncoded: " << testing::PrintToString(encoded);
  EXPECT_THAT(false_positives, IsEmpty())
      << "Found values not in input. " << print_data()
      << "\nEncoded: " << testing::PrintToString(encoded);
}

TEST(ValidateEnumTest, GenerateEnumDataSequentialWithOverflow) {
  std::vector<int32_t> values;
  for (int32_t i = -33000; i < 33000; ++i) {
    values.push_back(i);
  }
  const auto data = GenerateEnumData(values);
  EXPECT_THAT(ExtractHeader(data),
              HeaderHas(
                  // The sequence starts at the minimum possible value,
                  std::numeric_limits<int16_t>::min(),
                  // and it is as long as possible.
                  0xFFFF,
                  // we have some values in the bitmap
                  Gt(0),
                  // we have some in the fallback
                  Gt(0)));

  TestRoundTrip(values, __LINE__);
}

TEST(ValidateEnumTest, GenerateEnumDataBitmap) {
  EXPECT_THAT(GenerateEnumData({0, 1, 2, 4, 8, 16, 32}),
              ElementsAre(Make32(0, 3), Make32(32, 0),
                          0b100000000000000010000000100010));
  TestRoundTrip({}, __LINE__);
  TestRoundTrip({0, 1, 2, 4, 8, 16}, __LINE__);
  TestRoundTrip({0, 1, 2, 4, 8, 16, 32, 64, 128, 256}, __LINE__);
  TestRoundTrip({10000, 10001, 10002, 10004, 10006, 10008, 10010}, __LINE__);
  TestRoundTrip({std::numeric_limits<int32_t>::min(), -123123, -123, 213,
                 213213, std::numeric_limits<int32_t>::max()},
                __LINE__);
}

TEST(ValidateEnumTest, GenerateEnumDataBitmapWithOverflow) {
  std::vector<int32_t> values;
  // We step by 10 to guarantee each new value is more cost effective to add to
  // the bitmap, which would cause an overflow of the 16-bit bitmap size if we
  // didn't prevent it in the generator.
  for (int32_t i = -33000; i < 33000; i += 10) {
    values.push_back(i);
  }
  const auto data = GenerateEnumData(values);

  EXPECT_THAT(ExtractHeader(data),
              HeaderHas(_, _,
                        // we reached the maximum size for the bitmap.
                        0x10000 - 32,
                        // we have some in the fallback
                        Gt(0)));

  TestRoundTrip(values, __LINE__);
}

TEST(ValidateEnumTest, GenerateEnumDataWithOverflowOnBoth) {
  std::vector<int32_t> values;
  for (int32_t i = -33000; i < 100000; ++i) {
    values.push_back(i);
  }
  const auto data = GenerateEnumData(values);

  EXPECT_THAT(ExtractHeader(data),
              HeaderHas(
                  // The sequence starts at the minimum possible value,
                  std::numeric_limits<int16_t>::min(),
                  // and it is as long as possible.
                  0xFFFF,
                  // we reached the maximum size for the bitmap.
                  0x10000 - 32,
                  // we have some in the fallback
                  Gt(0)));

  TestRoundTrip(values, __LINE__);
}



}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
