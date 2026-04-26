// Copyright 2007 Google Inc.  All rights reserved.
// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "google/protobuf/unittest_force_split.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "google/protobuf/port.h"
#include "google/protobuf/test_util.h"

#define MESSAGE_TEST_NAME MessageTestForceSplit
#define MESSAGE_FACTORY_TEST_NAME MessageFactoryTestForceSplit
#define UNITTEST_PACKAGE_NAME "proto2_unittest_force_split"
#define UNITTEST ::proto2_unittest_force_split
#define UNITTEST_IMPORT ::proto2_unittest_import_force_split

// Must include after the above macros.
// clang-format off
#include "google/protobuf/message_unittest.inc"
#include "google/protobuf/message_unittest_legacy_apis.inc"
// clang-format on

namespace google {
namespace protobuf {

bool IsSplitForTest(const Message& message, const FieldDescriptor* field) {
  return message.GetReflection()->IsSplit(field);
}

namespace internal {
namespace {

using UNITTEST::TestAllTypes;
using UNITTEST::TestHugeFieldNumbers;

template <typename Proto>
struct OnStack {
  Proto* operator->() { return &msg; }
  Proto& operator*() { return *operator->(); }
  Proto* get() { return operator->(); }

  Proto msg;
};

template <typename Proto>
struct OnArena {
  OnArena() : msg(Arena::Create<Proto>(&arena)) {}
  Proto* operator->() { return msg; }
  Proto& operator*() { return *operator->(); }
  Proto* get() { return operator->(); }

  Arena arena;
  Proto* msg;
};

template <typename T>
class MessageTestForceSplit : public ::testing::Test {
 protected:
  const std::string str_ = std::string(100, 'x');
};

TYPED_TEST_SUITE_P(MessageTestForceSplit);

TYPED_TEST_P(MessageTestForceSplit, Write) {
  {
    TypeParam msg;
    TestUtil::SetAllFields(msg.get());
    TestUtil::ExpectAllFieldsSet(*msg);
  }
  {
    TypeParam msg;
    msg->set_optional_cord(this->str_);
    EXPECT_EQ(msg->optional_cord(), this->str_);
  }
  {
    TypeParam msg;
    auto* str = new std::string(this->str_);
    msg->set_allocated_optional_string(str);
    EXPECT_EQ(msg->optional_string(), this->str_);
  }
  {
    TypeParam msg;
    auto* nested = new UNITTEST::TestAllTypes_NestedMessage();
    msg->set_allocated_optional_nested_message(nested);
    EXPECT_TRUE(msg->has_optional_nested_message());
  }
  {
    TypeParam msg;
    msg->mutable_optional_lazy_message()->set_bb(100);
    EXPECT_EQ(msg->optional_lazy_message().bb(), 100);
  }
  {
    TypeParam msg;
    TestUtil::SetAllFields(msg.get());
    std::unique_ptr<UNITTEST::ForeignMessage> child_msg(
        msg->release_optional_foreign_message());
    EXPECT_NE(child_msg.get(), nullptr);
    EXPECT_FALSE(msg->has_optional_foreign_message());
  }
  // Make sure setting split fields doesn't affect the default values.
  TypeParam msg2;
  TestUtil::ExpectClear(*msg2);
}

TYPED_TEST_P(MessageTestForceSplit, WriteReflect) {
  {
    TypeParam msg;
    msg->GetReflection()->SetInt32(
        msg.get(), msg->GetDescriptor()->FindFieldByName("optional_int32"),
        11111);
    EXPECT_EQ(msg->optional_int32(), 11111);
  }
  {
    TypeParam msg;
    msg->GetReflection()->SetString(
        msg.get(), msg->GetDescriptor()->FindFieldByName("optional_bytes"),
        this->str_);
    EXPECT_EQ(msg->optional_bytes(), this->str_);
  }
  {
    TypeParam msg;
    msg->GetReflection()->SetString(
        msg.get(), msg->GetDescriptor()->FindFieldByName("optional_cord"),
        this->str_);
    EXPECT_EQ(msg->optional_cord(), this->str_);
  }
  {
    TypeParam msg;
    auto* nested = new UNITTEST::TestAllTypes_NestedMessage();
    msg->GetReflection()->SetAllocatedMessage(
        msg.get(), nested,
        msg->GetDescriptor()->FindFieldByName("optional_nested_message"));
    EXPECT_TRUE(msg->has_optional_nested_message());
  }
  {
    TypeParam msg;
    Message* lazy = msg->GetReflection()->MutableMessage(
        msg.get(),
        msg->GetDescriptor()->FindFieldByName("optional_lazy_message"));
    lazy->GetReflection()->SetInt32(
        lazy, lazy->GetDescriptor()->FindFieldByName("bb"), 100);
    EXPECT_EQ(msg->optional_lazy_message().bb(), 100);
  }
  // Make sure setting split fields doesn't affect the default values.
  TypeParam msg2;
  TestUtil::ExpectClear(*msg2);
}

TYPED_TEST_P(MessageTestForceSplit, Clear) {
  {
    TypeParam msg;
    TestUtil::SetAllFields(msg.get());
    msg->Clear();
    TestUtil::ExpectClear(*msg);
  }
  {
    TypeParam msg;
    *msg->mutable_optional_bytes() = this->str_;
    msg->clear_optional_bytes();
    msg->clear_optional_string();
    msg->clear_default_string();
    TestUtil::ExpectClear(*msg);
  }
}

TYPED_TEST_P(MessageTestForceSplit, ParseAndSerialize) {
  TypeParam msg;
  TestUtil::SetAllFields(msg.get());
  std::string str;
  EXPECT_TRUE(msg->SerializeToString(&str));
  TypeParam msg2;
  EXPECT_TRUE(msg2->ParseFromString(str));
  TestUtil::ExpectAllFieldsSet(*msg2);
}

TYPED_TEST_P(MessageTestForceSplit, RepeatedMessageArenaAllocated) {
  TypeParam msg;
  auto* repeated = msg->add_repeated_foreign_message();
  EXPECT_EQ(msg->GetArena(), repeated->GetArena());
}

TYPED_TEST_P(MessageTestForceSplit, IsSplit) {
  std::vector<absl::string_view> expected = {
      "optional_int32",
      "optional_int64",
      "optional_uint32",
      "optional_uint64",
      "optional_sint32",
      "optional_sint64",
      "optional_fixed32",
      "optional_fixed64",
      "optional_sfixed32",
      "optional_sfixed64",
      "optional_float",
      "optional_double",
      "optional_bool",
      "optionalgroup",
      "optional_nested_enum",
      "optional_foreign_enum",
      "optional_import_enum",
      "default_int32",
      "default_int64",
      "default_uint32",
      "default_uint64",
      "default_sint32",
      "default_sint64",
      "default_fixed32",
      "default_fixed64",
      "default_sfixed32",
      "default_sfixed64",
      "default_float",
      "default_double",
      "default_bool",
      "default_string",
      "default_bytes",
      "default_nested_enum",
      "default_foreign_enum",
      "default_import_enum",
      "repeated_bool",
      "repeated_double",
      "repeated_fixed32",
      "repeated_fixed64",
      "repeated_float",
      "repeated_int32",
      "repeated_int64",
      "repeated_sfixed32",
      "repeated_sfixed64",
      "repeated_sint32",
      "repeated_sint64",
      "repeated_uint32",
      "repeated_uint64",
      "repeated_foreign_enum",
      "repeated_import_enum",
      "repeated_nested_enum",
      "repeated_bytes",
      "repeated_string",
      "repeated_foreign_message",
      "repeated_import_message",
      "repeated_nested_message",
      "repeated_lazy_message",
      "repeatedgroup",
  };

  if (!internal::ForceInlineStringInProtoc()) {
    expected.push_back("optional_bytes");
    expected.push_back("optional_string");
  }

  if (!google::protobuf::internal::ForceEagerlyVerifiedLazyInProtoc()) {
    expected.push_back("optional_foreign_message");
    expected.push_back("optional_import_message");
    expected.push_back("optional_nested_message");
    expected.push_back("optional_public_import_message");
  }

  std::sort(expected.begin(), expected.end());

  TypeParam msg;
  const Descriptor* d = msg->GetDescriptor();
  std::vector<absl::string_view> actual;
  for (int i = 0; i < d->field_count(); i++) {
    if (IsSplitForTest(*msg, d->field(i))) {
      actual.push_back(d->field(i)->name());
    }
  }
  std::sort(actual.begin(), actual.end());

  EXPECT_THAT(actual, testing::ContainerEq(expected));
}

REGISTER_TYPED_TEST_SUITE_P(MessageTestForceSplit, Write, WriteReflect, Clear,
                            ParseAndSerialize, RepeatedMessageArenaAllocated,
                            IsSplit);
using AllocationTypes =
    ::testing::Types<OnStack<TestAllTypes>, OnArena<TestAllTypes>>;

INSTANTIATE_TYPED_TEST_SUITE_P(Do, MessageTestForceSplit, AllocationTypes);

template <typename T>
class MessageTestForceSplitPair : public ::testing::Test {};

TYPED_TEST_SUITE_P(MessageTestForceSplitPair);

TYPED_TEST_P(MessageTestForceSplitPair, Swap) {
  using LhsType = typename TypeParam::first_type;
  using RhsType = typename TypeParam::second_type;
  {
    LhsType lhs;
    {
      RhsType rhs;
      TestUtil::SetAllFields(rhs.get());
      lhs->Swap(rhs.get());
      TestUtil::ExpectClear(*rhs);
    }
    TestUtil::ExpectAllFieldsSet(*lhs);
  }
}

TYPED_TEST_P(MessageTestForceSplitPair, SwapReflect) {
  using LhsType = typename TypeParam::first_type;
  using RhsType = typename TypeParam::second_type;
  {
    LhsType lhs;
    {
      RhsType rhs;
      TestUtil::SetAllFields(rhs.get());
      const Reflection* reflection = lhs->GetReflection();
      reflection->Swap(lhs.get(), rhs.get());
      TestUtil::ExpectClear(*rhs);
    }
    TestUtil::ExpectAllFieldsSet(*lhs);
  }
}

TYPED_TEST_P(MessageTestForceSplitPair, Copy) {
  using LhsType = typename TypeParam::first_type;
  using RhsType = typename TypeParam::second_type;
  {
    LhsType lhs;
    {
      RhsType rhs;
      TestUtil::SetAllFields(rhs.get());
      *lhs = *rhs;
      TestUtil::ExpectAllFieldsSet(*rhs);
    }
    TestUtil::ExpectAllFieldsSet(*lhs);
  }
}

REGISTER_TYPED_TEST_SUITE_P(MessageTestForceSplitPair, Swap, SwapReflect, Copy);

using PairAllocationTypes = ::testing::Types<  //
    std::pair<OnStack<TestAllTypes>, OnStack<TestAllTypes>>,
    std::pair<OnStack<TestAllTypes>, OnArena<TestAllTypes>>,
    std::pair<OnArena<TestAllTypes>, OnStack<TestAllTypes>>,
    std::pair<OnArena<TestAllTypes>, OnArena<TestAllTypes>>>;

INSTANTIATE_TYPED_TEST_SUITE_P(Do, MessageTestForceSplitPair,
                               PairAllocationTypes);

TEST(MESSAGE_TEST_NAME, Map) {
  TestHugeFieldNumbers msg;
  (*msg.mutable_string_string_map())["123"] = "456";
  EXPECT_EQ("456", msg.string_string_map().at("123"));
}

TEST(MESSAGE_TEST_NAME, DeadModifyingDefault) {
#ifdef NDEBUG
  GTEST_SKIP() << "DeadModifyingDefault test is skipped since the check is "
                  "only performed in non-OPT build.";
#endif  // NDEBUG
  auto* default_msg =
      const_cast<TestAllTypes*>(&TestAllTypes::default_instance());
  EXPECT_DEATH(default_msg->set_optional_int32(111), "Check failed");
}

TEST(MESSAGE_TEST_NAME, DeadModifyingDefaultReflect) {
#ifdef NDEBUG
  GTEST_SKIP() << "DeadModifyingDefaultReflect test is skipped since the check "
                  "is only performed in non-OPT build.";
#endif  // NDEBUG
  auto* default_msg =
      const_cast<TestAllTypes*>(&TestAllTypes::default_instance());
  const Reflection* r = default_msg->GetReflection();
  const FieldDescriptor* field =
      default_msg->descriptor()->FindFieldByName("optional_int64");
  EXPECT_DEATH(r->SetInt64(default_msg, field, 111), "Check failed");
}

TEST(MessageTestForceSplit, CopyCtor) {
  TestAllTypes msg;
  TestUtil::SetAllFields(&msg);
  TestAllTypes copy(msg);
  EXPECT_THAT(copy, testing::EqualsProto(msg));
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
