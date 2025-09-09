// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/has_bits.h"

#include <cstdint>
#include <string>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/generated_message_tctable_impl.h"
#include "google/protobuf/map_unittest.pb.h"
#include "google/protobuf/message.h"
#include "google/protobuf/port.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/repeated_ptr_field.h"
#include "google/protobuf/unittest.pb.h"


namespace google {
namespace protobuf {
namespace internal {

namespace {

using ::proto2_unittest::TestAllTypes;
using ::proto2_unittest::TestMap;
using ::testing::Eq;

bool IsIndexInHasBitSet(const uint32_t* has_bit_set, uint32_t has_bit_index) {
  ABSL_DCHECK_NE(has_bit_index, static_cast<uint32_t>(kNoHasbit));
  return ((has_bit_set[has_bit_index / 32] >> (has_bit_index % 32)) &
          static_cast<uint32_t>(1)) != 0;
}

}  // namespace

class HasBitsTestPeer {
 public:
  static bool HasBitSet(const Message& msg, const FieldDescriptor* field) {
    const auto* ref = msg.GetReflection();
    uint32_t has_bit_idx = ref->Schema().HasBitIndex(field);
    return IsIndexInHasBitSet(ref->GetHasBits(msg), has_bit_idx);
  }

  static bool HasBitSet(const Message& msg, absl::string_view field_name) {
    return HasBitSet(msg, msg.GetDescriptor()->FindFieldByName(field_name));
  }

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

TEST(HasBitsTest, HasBitsUnsetForDefaultRepeatedField) {
  if constexpr (!EnableExperimentalHintHasBitsForRepeatedFields()) {
    GTEST_SKIP()
        << "Test only applies with hasbits for repeated fields enabled";
  }
  TestAllTypes msg;
  EXPECT_FALSE(HasBitsTestPeer::HasBitSet(msg, "repeated_int32"));
}

TEST(HasBitsTest, HasBitsSetOnMutable) {
  if constexpr (!EnableExperimentalHintHasBitsForRepeatedFields()) {
    GTEST_SKIP()
        << "Test only applies with hasbits for repeated fields enabled";
  }
  TestAllTypes msg;
  msg.mutable_repeated_int32();
  EXPECT_TRUE(HasBitsTestPeer::HasBitSet(msg, "repeated_int32"));
}

TEST(HasBitsTest, HasBitsClearedOnFieldClear) {
  if constexpr (!EnableExperimentalHintHasBitsForRepeatedFields()) {
    GTEST_SKIP()
        << "Test only applies with hasbits for repeated fields enabled";
  }
  TestAllTypes msg;
  msg.mutable_repeated_int32();
  msg.clear_repeated_int32();
  EXPECT_FALSE(HasBitsTestPeer::HasBitSet(msg, "repeated_int32"));
}

TEST(HasBitsTest, HasBitsSetOnMutableWithReflection) {
  if constexpr (!EnableExperimentalHintHasBitsForRepeatedFields()) {
    GTEST_SKIP()
        << "Test only applies with hasbits for repeated fields enabled";
  }
  TestAllTypes msg;
  msg.GetReflection()->GetMutableRepeatedFieldRef<int32_t>(
      &msg, msg.GetDescriptor()->FindFieldByName("repeated_int32"));
  EXPECT_TRUE(HasBitsTestPeer::HasBitSet(msg, "repeated_int32"));
}

TEST(HasBitsTest, HasBitsClearedOnFieldClearWithReflection) {
  if constexpr (!EnableExperimentalHintHasBitsForRepeatedFields()) {
    GTEST_SKIP()
        << "Test only applies with hasbits for repeated fields enabled";
  }
  TestAllTypes msg;
  msg.mutable_repeated_int32();
  msg.GetReflection()->ClearField(
      &msg, msg.GetDescriptor()->FindFieldByName("repeated_int32"));
  EXPECT_FALSE(HasBitsTestPeer::HasBitSet(msg, "repeated_int32"));
}

TEST(HasBitsTest, HasBitsSetOnMutableMap) {
  if constexpr (!EnableExperimentalHintHasBitsForRepeatedFields()) {
    GTEST_SKIP()
        << "Test only applies with hasbits for repeated fields enabled";
  }
  TestMap msg;
  msg.mutable_map_int32_int32();
  EXPECT_TRUE(HasBitsTestPeer::HasBitSet(msg, "map_int32_int32"));
}

TEST(HasBitsTest, HasBitsClearedOnMapFieldClear) {
  if constexpr (!EnableExperimentalHintHasBitsForRepeatedFields()) {
    GTEST_SKIP()
        << "Test only applies with hasbits for repeated fields enabled";
  }
  TestMap msg;
  msg.mutable_map_int32_int32();
  msg.clear_map_int32_int32();
  EXPECT_FALSE(HasBitsTestPeer::HasBitSet(msg, "map_int32_int32"));
}

TEST(HasBitsTest, HasBitsSetOnMutableMapWithReflection) {
  if constexpr (!EnableExperimentalHintHasBitsForRepeatedFields()) {
    GTEST_SKIP()
        << "Test only applies with hasbits for repeated fields enabled";
  }
  TestMap msg;
  msg.GetReflection()->GetMutableRepeatedFieldRef<Message>(
      &msg, msg.GetDescriptor()->FindFieldByName("map_int32_int32"));
  EXPECT_TRUE(HasBitsTestPeer::HasBitSet(msg, "map_int32_int32"));
}

TEST(HasBitsTest, HasBitsClearedOnMapFieldClearWithReflection) {
  if constexpr (!EnableExperimentalHintHasBitsForRepeatedFields()) {
    GTEST_SKIP()
        << "Test only applies with hasbits for repeated fields enabled";
  }
  TestMap msg;
  msg.mutable_map_int32_int32();
  msg.GetReflection()->ClearField(
      &msg, msg.GetDescriptor()->FindFieldByName("map_int32_int32"));
  EXPECT_FALSE(HasBitsTestPeer::HasBitSet(msg, "map_int32_int32"));
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
