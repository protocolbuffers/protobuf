// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/expected.h"

#include <memory>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace testing {
namespace {

enum class SampleError { kSampleError };

TEST(ExpectedTest, RawPtrSize) {
  using type = google::protobuf::expected<int*, SampleError>;
  EXPECT_LE(sizeof(type), 16);
}

TEST(ExpectedTest, GuaranteedTraits) {
  using type = google::protobuf::expected<int, SampleError>;
  EXPECT_TRUE(std::is_trivially_copy_constructible<type>::value);
  EXPECT_TRUE(std::is_trivially_copy_assignable<type>::value);
  EXPECT_TRUE(std::is_trivially_destructible<type>::value);
}

TEST(ExpectedTest, Moves) {
  auto i = std::make_unique<int>(100);
  google::protobuf::expected<std::unique_ptr<int>, SampleError> status(std::move(i));
  EXPECT_TRUE(status.has_value());
  EXPECT_EQ(*status.value(), 100);

  google::protobuf::expected<std::unique_ptr<int>, SampleError> move(std::move(status));
  EXPECT_TRUE(move.has_value());
  EXPECT_EQ(*move.value(), 100);
}

TEST(ExpectedTest, BasicUsage) {
  auto basic = google::protobuf::expected<int, SampleError>(100);
  EXPECT_TRUE(basic.has_value());
  EXPECT_EQ(basic.value(), 100);
  EXPECT_DEATH(basic.error(),
               "google::protobuf::expected an error, but detected a value");
}

TEST(ExpectedTest, Error) {
  auto nogood = google::protobuf::expected<int, SampleError>(SampleError::kSampleError);
  EXPECT_FALSE(nogood.has_value());
  EXPECT_EQ(nogood.error(), SampleError::kSampleError);
  EXPECT_DEATH(nogood.value(),
               "google::protobuf::expected a value, but detected an error");
}

}  // namespace
}  // namespace testing
}  // namespace protobuf
}  // namespace google
