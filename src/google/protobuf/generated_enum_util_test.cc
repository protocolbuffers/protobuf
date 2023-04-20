// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
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

#include "google/protobuf/generated_enum_util.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/container/flat_hash_set.h"
#include "absl/types/span.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

using testing::ElementsAre;
using testing::ElementsAreArray;
using testing::UnorderedElementsAreArray;

namespace google {
namespace protobuf {
namespace internal {
namespace {

void ValidValues(absl::Span<const uint16_t> data, int min, int max,
                 absl::flat_hash_set<int>& out) {
  for (int64_t i = min; i <= max; ++i) {
    if (ValidateEnum(i, data.begin())) out.insert(i);
  }
}

std::vector<int32_t> ValidValues(absl::Span<const uint16_t> data, int min,
                                 int max) {
  absl::flat_hash_set<int> s;
  ValidValues(data, min, max, s);
  std::vector<int32_t> out(s.begin(), s.end());
  std::sort(out.begin(), out.end());
  return out;
}

TEST(ValidateEnumTest, SequentialRangeTest) {
  EXPECT_THAT(ValidValues({0, 0, 0, 0}, -100, 100), ElementsAre());
  EXPECT_THAT(ValidValues({5, 3, 0, 0}, -100, 100), ElementsAre(5, 6, 7));
  EXPECT_THAT(ValidValues({static_cast<uint16_t>(-2), 10, 0, 0}, -100, 100),
              ElementsAre(-2, -1, 0, 1, 2, 3, 4, 5, 6, 7));
}

TEST(ValidateEnumTest, BitmapRangeTest) {
  EXPECT_THAT(ValidValues({0, 0, 16, 0, 0b10011010101}, -100, 100),
              ElementsAre(0, 2, 4, 6, 7, 10));
  EXPECT_THAT(ValidValues({0, 0, 48, 0, 1 << 4, 1 << 5, 1 << 6}, -100, 100),
              ElementsAre(4, 16 + 5, 32 + 6));
}

TEST(ValidateEnumTest, GenerateEnumDataSequential) {
  EXPECT_THAT(GenerateEnumData({0, 1, 2, 3}), ElementsAre(0, 4, 0, 0));
  EXPECT_THAT(GenerateEnumData({-2, -1, 0, 1, 2, 3}),
              ElementsAre(static_cast<uint16_t>(-2), 6, 0, 0));
}

void TestRoundTrip(const std::vector<int32_t>& values) {
  auto encoded = GenerateEnumData(values);

  absl::flat_hash_set<int> s;
  // Add a few test values in case `values` is empty.
  ValidValues(encoded, -100, 100, s);

  // We look at a few values around the expected ones.
  // We could in theory test the whole int32_t domain, but that takes too long
  // to run.
  for (int32_t v : values) {
    int32_t min =
        std::max(static_cast<int64_t>(v) - 100,
                 static_cast<int64_t>(std::numeric_limits<int32_t>::min()));
    int32_t max =
        std::min(static_cast<int64_t>(v) + 100,
                 static_cast<int64_t>(std::numeric_limits<int32_t>::max()));
    ValidValues(encoded, min, max, s);
  }

  EXPECT_THAT(s, UnorderedElementsAreArray(values))
      << testing::PrintToString(encoded);
}

TEST(ValidateEnumTest, GenerateEnumDataBitmap) {
  EXPECT_THAT(GenerateEnumData({0, 1, 2, 4, 8, 16}),
              ElementsAre(0, 3, 16, 0, 0b10000000100010));
  TestRoundTrip({});
  TestRoundTrip({0, 1, 2, 4, 8, 16});
  TestRoundTrip({0, 1, 2, 4, 8, 16, 32, 64, 128, 256});
  TestRoundTrip({10000, 10001, 10002, 10004, 10006, 10008, 10010});
  TestRoundTrip({std::numeric_limits<int32_t>::min(), -123123, -123, 213,
                 213213, std::numeric_limits<int32_t>::max()});
}


}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
