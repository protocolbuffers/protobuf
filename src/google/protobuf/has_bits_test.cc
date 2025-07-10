// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/has_bits.h"

#include <cstdint>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "google/protobuf/map_unittest.pb.h"
#include "google/protobuf/unittest.pb.h"

namespace google {
namespace protobuf {
namespace internal {

using ::testing::Eq;

class HasBitsTest : public testing::Test {
 public:
  template <int n>
  void TestDefaultInit() {
    HasBits<n> bits;
    EXPECT_TRUE(bits.empty());
    for (int i = 0; i < n; ++i) {
      EXPECT_THAT(bits[i], Eq(0));
    }
  }

  bool HasBitSet(const Message& msg, const FieldDescriptor* field) {
    const auto* ref = msg.GetReflection();
    uint32_t has_bit_idx = ref->Schema().HasBitIndex(field);
    return ref->IsIndexInHasBitSet(ref->GetHasBits(msg), has_bit_idx);
  }

  bool HasBitSet(const Message& msg, absl::string_view field_name) {
    return HasBitSet(msg, msg.GetDescriptor()->FindFieldByName(field_name));
  }
};

namespace {

TEST_F(HasBitsTest, DefaultInit) {
  TestDefaultInit<1>();
  TestDefaultInit<2>();
  TestDefaultInit<3>();
  TestDefaultInit<4>();
}

TEST_F(HasBitsTest, ValueInit) {
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

TEST_F(HasBitsTest, ConstexprValueInit) {
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

TEST_F(HasBitsTest, operator_equal) {
  EXPECT_FALSE(HasBits<4>({1, 2, 3, 4}) == HasBits<4>({0, 2, 3, 4}));
  EXPECT_FALSE(HasBits<4>({1, 2, 3, 4}) == HasBits<4>({1, 0, 3, 4}));
  EXPECT_FALSE(HasBits<4>({1, 2, 3, 4}) == HasBits<4>({1, 2, 0, 4}));
  EXPECT_FALSE(HasBits<4>({1, 2, 3, 4}) == HasBits<4>({1, 2, 3, 0}));
  EXPECT_TRUE(HasBits<4>({1, 2, 3, 4}) == HasBits<4>({1, 2, 3, 4}));
}

TEST_F(HasBitsTest, Or) {
  HasBits<4> bits1({1, 2, 4, 8});
  HasBits<4> bits2({16, 32, 64, 128});
  bits1.Or(bits2);
  EXPECT_TRUE(bits1 == HasBits<4>({17, 34, 68, 136}));
}

TEST_F(HasBitsTest, Copy) {
  HasBits<4> bits1({1, 2, 4, 8});
  HasBits<4> bits2(bits1);
  EXPECT_TRUE(bits1 == bits2);
}

TEST_F(HasBitsTest, HasBitsUnsetForDefaultRepeatedField) {
  proto2_unittest::TestAllTypes msg;
  EXPECT_FALSE(HasBitSet(msg, "repeated_int32"));
}

TEST_F(HasBitsTest, HasBitsSetOnMutable) {
  proto2_unittest::TestAllTypes msg;
  msg.mutable_repeated_int32();
  EXPECT_TRUE(HasBitSet(msg, "repeated_int32"));
}

TEST_F(HasBitsTest, HasBitsClearedOnFieldClear) {
  proto2_unittest::TestAllTypes msg;
  msg.mutable_repeated_int32();
  msg.clear_repeated_int32();
  EXPECT_FALSE(HasBitSet(msg, "repeated_int32"));
}

TEST_F(HasBitsTest, HasBitsSetOnMutableWithReflection) {
  proto2_unittest::TestAllTypes msg;
  msg.GetReflection()->GetMutableRepeatedFieldRef<int32_t>(
      &msg, msg.GetDescriptor()->FindFieldByName("repeated_int32"));
  EXPECT_TRUE(HasBitSet(msg, "repeated_int32"));
}

TEST_F(HasBitsTest, HasBitsClearedOnFieldClearWithReflection) {
  proto2_unittest::TestAllTypes msg;
  msg.mutable_repeated_int32();
  msg.GetReflection()->ClearField(
      &msg, msg.GetDescriptor()->FindFieldByName("repeated_int32"));
  EXPECT_FALSE(HasBitSet(msg, "repeated_int32"));
}

TEST_F(HasBitsTest, HasBitsSetOnMutableMap) {
  proto2_unittest::TestMap msg;
  msg.mutable_map_int32_int32();
  EXPECT_TRUE(HasBitSet(msg, "map_int32_int32"));
}

TEST_F(HasBitsTest, HasBitsClearedOnMapFieldClear) {
  proto2_unittest::TestMap msg;
  msg.mutable_map_int32_int32();
  msg.clear_map_int32_int32();
  EXPECT_FALSE(HasBitSet(msg, "map_int32_int32"));
}

TEST_F(HasBitsTest, HasBitsSetOnMutableMapWithReflection) {
  proto2_unittest::TestMap msg;
  msg.GetReflection()->GetMutableRepeatedFieldRef<Message>(
      &msg, msg.GetDescriptor()->FindFieldByName("map_int32_int32"));
  EXPECT_TRUE(HasBitSet(msg, "map_int32_int32"));
}

TEST_F(HasBitsTest, HasBitsClearedOnMapFieldClearWithReflection) {
  proto2_unittest::TestMap msg;
  msg.mutable_map_int32_int32();
  msg.GetReflection()->ClearField(
      &msg, msg.GetDescriptor()->FindFieldByName("map_int32_int32"));
  EXPECT_FALSE(HasBitSet(msg, "map_int32_int32"));
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
