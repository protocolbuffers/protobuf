// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/has_bits.h"

#include <cstdint>
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
#include "google/protobuf/map_unittest.proto.static_reflection.h"
#include "google/protobuf/message.h"
#include "google/protobuf/message_static_reflection.h"
#include "google/protobuf/port.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/repeated_ptr_field.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest.proto.static_reflection.h"

namespace google {
namespace protobuf {
namespace internal {

using ::proto2_unittest::TestAllTypes;
using ::proto2_unittest::TestMap;
using ::testing::Eq;

class HasBitsTest : public testing::Test {
 public:
  static absl::Status VerifyHasBitConsistency(const MessageLite& msg) {
    return TcParser::VerifyHasBitConsistency(&msg, msg.GetTcParseTable());
  }

  static void ExpectHasBitIncorrect(const MessageLite& msg, int field_number) {
    EXPECT_THAT(
        VerifyHasBitConsistency(msg),
        testing::status::StatusIs(absl::StatusCode::kInternal,
                                  testing::HasSubstr(absl::StrFormat(
                                      "Has bits mismatch for Type=%s Field=%d",
                                      msg.GetTypeName(), field_number))));
  }

  template <int n>
  void TestDefaultInit() {
    HasBits<n> bits;
    EXPECT_TRUE(bits.empty());
    for (int i = 0; i < n; ++i) {
      EXPECT_THAT(bits[i], Eq(0));
    }
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

  template <google::protobuf::FieldId field, typename Src, typename T>
  static void ExpectHasBitForSetValue(T&& value) {
    static constexpr auto kFieldInfo = google::protobuf::FieldInfo<Src, field>();
    static constexpr auto kFieldInfoType = kFieldInfo.field_info_type;
    const auto* field_descriptor =
        Src::descriptor()->FindFieldByNumber(kFieldInfo.number);

    // Instantiate a new message and set the field.
    Src src;
    if constexpr (kFieldInfoType == FieldInfoType::kSingularMessage) {
      kFieldInfo.Mutable(src) = std::forward<T>(value);
    } else if constexpr (kFieldInfoType == FieldInfoType::kRepeated) {
      kFieldInfo.Mutable(src).Add(std::forward<T>(value));
    } else if constexpr (kFieldInfoType == FieldInfoType::kMap) {
      kFieldInfo.Mutable(src)[value.first] = value.second;
    } else {
      kFieldInfo.Set(src, std::forward<T>(value));
    }

    // Clear the field's hasbit, which will temporarily put the message into an
    // invalid state.
    ClearHasBit(&src, field_descriptor);

    // VerifyHasBitConsistency should fail, pointing to this field as the
    // culprit.
    ExpectHasBitIncorrect(src, kFieldInfo.number);

    // Clear the field so the real hasbit consistency check that fires on
    // destruction passes.
    if constexpr (kFieldInfoType == FieldInfoType::kMap) {
      kFieldInfo.Mutable(src).clear();
    } else {
      kFieldInfo.Clear(src);
    }
  }

  template <google::protobuf::FieldId field, typename Src>
  static void ExpectNoHasBitForClearedValue() {
    static constexpr auto kFieldInfo = google::protobuf::FieldInfo<Src, field>();
    const auto* field_descriptor =
        Src::descriptor()->FindFieldByNumber(kFieldInfo.number);

    // Set the empty field's hasbit, which will temporarily put the message into
    // an invalid state.
    Src src;
    SetHasBit(&src, field_descriptor);

    // VerifyHasBitConsistency should fail, pointing to this field as the
    // culprit.
    ExpectHasBitIncorrect(src, kFieldInfo.number);

    // Clear the field so the real hasbit consistency check that fires on
    // destruction passes.
    ClearHasBit(&src, field_descriptor);
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

TEST_F(HasBitsTest, DetectMissingBoolHasBit) {
  ExpectHasBitForSetValue<"optional_bool", TestAllTypes>(true);
}

TEST_F(HasBitsTest, DetectMissingNumericHasBit) {
  ExpectHasBitForSetValue<"optional_int32", TestAllTypes>(123);
}

TEST_F(HasBitsTest, DetectMissing64BitNumericHasBit) {
  ExpectHasBitForSetValue<"optional_int64", TestAllTypes>(456);
}

TEST_F(HasBitsTest, DetectIncorrectHasBitForDefaultString) {
  ExpectNoHasBitForClearedValue<"optional_string", TestAllTypes>();
}

TEST_F(HasBitsTest, DetectMissingCordHasBit) {
  ExpectHasBitForSetValue<"optional_cord", TestAllTypes>(absl::Cord("foo"));
}

TEST_F(HasBitsTest, DetectIncorrectHasBitForDefaultSubMessage) {
  ExpectNoHasBitForClearedValue<"optional_nested_message", TestAllTypes>();
}

TEST_F(HasBitsTest, DetectIncorrectHasBitForDefaultSubMessageGroup) {
  ExpectNoHasBitForClearedValue<"optionalgroup", TestAllTypes>();
}

TEST_F(HasBitsTest, DetectMissingRepeatedBoolHasBit) {
  if constexpr (!EnableExperimentalHintHasBitsForRepeatedFields()) {
    GTEST_SKIP()
        << "Test only applies with hasbits for repeated fields enabled.";
  }
  ExpectHasBitForSetValue<"repeated_bool", TestAllTypes>(true);
}

TEST_F(HasBitsTest, DetectMissingRepeatedInt32HasBit) {
  if constexpr (!EnableExperimentalHintHasBitsForRepeatedFields()) {
    GTEST_SKIP()
        << "Test only applies with hasbits for repeated fields enabled.";
  }
  ExpectHasBitForSetValue<"repeated_int32", TestAllTypes>(123);
}

TEST_F(HasBitsTest, DetectMissingRepeatedInt64HasBit) {
  if constexpr (!EnableExperimentalHintHasBitsForRepeatedFields()) {
    GTEST_SKIP()
        << "Test only applies with hasbits for repeated fields enabled.";
  }
  ExpectHasBitForSetValue<"repeated_int64", TestAllTypes>(456);
}

TEST_F(HasBitsTest, DetectMissingRepeatedStringHasBit) {
  if constexpr (!EnableExperimentalHintHasBitsForRepeatedFields()) {
    GTEST_SKIP()
        << "Test only applies with hasbits for repeated fields enabled.";
  }
  ExpectHasBitForSetValue<"repeated_string", TestAllTypes>("foo");
}

TEST_F(HasBitsTest, DetectMissingRepeatedCordHasBit) {
  if constexpr (!EnableExperimentalHintHasBitsForRepeatedFields()) {
    GTEST_SKIP()
        << "Test only applies with hasbits for repeated fields enabled.";
  }
  ExpectHasBitForSetValue<"repeated_cord", TestAllTypes>(absl::Cord("foo"));
}

TEST_F(HasBitsTest, DetectMissingRepeatedSubMessageHasBit) {
  if constexpr (!EnableExperimentalHintHasBitsForRepeatedFields()) {
    GTEST_SKIP()
        << "Test only applies with hasbits for repeated fields enabled.";
  }
  TestAllTypes::NestedMessage nested_message;
  nested_message.set_bb(123);
  ExpectHasBitForSetValue<"repeated_nested_message", TestAllTypes>(
      std::move(nested_message));
}

TEST_F(HasBitsTest, DetectMissingMapFieldHasBit) {
  if constexpr (!EnableExperimentalHintHasBitsForRepeatedFields()) {
    GTEST_SKIP()
        << "Test only applies with hasbits for repeated fields enabled.";
  }
  ExpectHasBitForSetValue<"map_int32_int32", TestMap>(std::make_pair(123, 456));
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
