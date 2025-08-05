// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/has_bits.h"

#include <cstdint>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/generated_message_tctable_impl.h"
#include "google/protobuf/message.h"
#include "google/protobuf/port.h"
#include "google/protobuf/unittest.pb.h"


namespace google {
namespace protobuf {
namespace internal {

namespace {

using ::proto2_unittest::TestAllTypes;
using ::testing::Eq;

}  // namespace

class HasBitsTestPeer {
 public:
  static void SetHasBit(Message* msg, const FieldDescriptor* field) {
    const auto* ref = msg->GetReflection();
    ABSL_DCHECK_NE(ref->Schema().HasBitIndex(field),
                   static_cast<uint32_t>(kNoHasbit));
    ref->SetHasBit(msg, field);
  }

  static void ClearHasBit(Message* msg, const FieldDescriptor* field) {
    const auto* ref = msg->GetReflection();
    ABSL_DCHECK_NE(ref->Schema().HasBitIndex(field),
                   static_cast<uint32_t>(kNoHasbit));
    ref->ClearHasBit(msg, field);
  }
};

namespace {

template <typename HasBitsT>
class DefaultInitTest : public ::testing::Test {};

using DefaultInitTestTypes =
    testing::Types<HasBits<1>, HasBits<2>, HasBits<3>, HasBits<4>>;
TYPED_TEST_SUITE(DefaultInitTest, DefaultInitTestTypes);

TYPED_TEST(DefaultInitTest, DefaultInit) {
  TypeParam bits;
  EXPECT_TRUE(bits.empty());
  for (int i = 0; i < TypeParam::kNumHasWords; ++i) {
    EXPECT_THAT(bits[i], Eq(0));
  }
}

TEST(HasBitsTest, ValueInit) {
  {
    HasBits<4> bits;
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

TEST(HasBitsTest, ConstexprValueInit) {
  {
    constexpr HasBits<4> bits;
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

TEST(HasBitsTest, operator_equal) {
  EXPECT_FALSE(HasBits<4>({1, 2, 3, 4}) == HasBits<4>({0, 2, 3, 4}));
  EXPECT_FALSE(HasBits<4>({1, 2, 3, 4}) == HasBits<4>({1, 0, 3, 4}));
  EXPECT_FALSE(HasBits<4>({1, 2, 3, 4}) == HasBits<4>({1, 2, 0, 4}));
  EXPECT_FALSE(HasBits<4>({1, 2, 3, 4}) == HasBits<4>({1, 2, 3, 0}));
  EXPECT_TRUE(HasBits<4>({1, 2, 3, 4}) == HasBits<4>({1, 2, 3, 4}));
}

TEST(HasBitsTest, Or) {
  HasBits<4> bits1({1, 2, 4, 8});
  HasBits<4> bits2({16, 32, 64, 128});
  bits1.Or(bits2);
  EXPECT_TRUE(bits1 == HasBits<4>({17, 34, 68, 136}));
}

TEST(HasBitsTest, Copy) {
  HasBits<4> bits1({1, 2, 4, 8});
  HasBits<4> bits2(bits1);
  EXPECT_TRUE(bits1 == bits2);
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
