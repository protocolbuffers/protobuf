// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/has_bits.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace internal {
namespace {

using ::testing::Eq;

template <int n>
void TestDefaultInit() {
  HasBits<n> bits;
  EXPECT_TRUE(bits.empty());
  for (int i = 0; i < n; ++i) {
    EXPECT_THAT(bits[i], Eq(0));
  }
}

TEST(HasBits, DefaultInit) {
  TestDefaultInit<1>();
  TestDefaultInit<2>();
  TestDefaultInit<3>();
  TestDefaultInit<4>();
}

TEST(HasBits, ValueInit) {
  {
    HasBits<4> bits;
    EXPECT_TRUE(bits.empty());
  }
  {
    HasBits<4> bits({});
    EXPECT_TRUE(bits.empty());
  }
  {
    HasBits<4> bits({1});
    EXPECT_FALSE(bits.empty());
    EXPECT_THAT(bits[0], Eq(1));
  }
  {
    HasBits<4> bits({1, 2, 3, 4});
    EXPECT_FALSE(bits.empty());
    EXPECT_THAT(bits[0], Eq(1));
    EXPECT_THAT(bits[1], Eq(2));
    EXPECT_THAT(bits[2], Eq(3));
    EXPECT_THAT(bits[3], Eq(4));
  }
}

TEST(HasBits, ConstexprValueInit) {
  {
    constexpr HasBits<4> bits;
    EXPECT_TRUE(bits.empty());
  }
  {
    constexpr HasBits<4> bits({});
    EXPECT_TRUE(bits.empty());
  }
  {
    constexpr HasBits<4> bits({1});
    EXPECT_FALSE(bits.empty());
    EXPECT_THAT(bits[0], Eq(1));
  }
  {
    constexpr HasBits<4> bits({1, 2, 3, 4});
    EXPECT_FALSE(bits.empty());
    EXPECT_THAT(bits[0], Eq(1));
    EXPECT_THAT(bits[1], Eq(2));
    EXPECT_THAT(bits[2], Eq(3));
    EXPECT_THAT(bits[3], Eq(4));
  }
}

TEST(HasBits, operator_equal) {
  EXPECT_FALSE(HasBits<4>({1, 2, 3, 4}) == HasBits<4>({0, 2, 3, 4}));
  EXPECT_FALSE(HasBits<4>({1, 2, 3, 4}) == HasBits<4>({1, 0, 3, 4}));
  EXPECT_FALSE(HasBits<4>({1, 2, 3, 4}) == HasBits<4>({1, 2, 0, 4}));
  EXPECT_FALSE(HasBits<4>({1, 2, 3, 4}) == HasBits<4>({1, 2, 3, 0}));
  EXPECT_TRUE(HasBits<4>({1, 2, 3, 4}) == HasBits<4>({1, 2, 3, 4}));
}

TEST(HasBits, Or) {
  HasBits<4> bits1({1, 2, 4, 8});
  HasBits<4> bits2({16, 32, 64, 128});
  bits1.Or(bits2);
  EXPECT_TRUE(bits1 == HasBits<4>({17, 34, 68, 136}));
}

TEST(HasBits, Copy) {
  HasBits<4> bits1({1, 2, 4, 8});
  HasBits<4> bits2(bits1);
  EXPECT_TRUE(bits1 == bits2);
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
