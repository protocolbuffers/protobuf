// Copyright 2007 Google Inc.  All rights reserved.
// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <string>
#include <utility>

#include "google/protobuf/unittest_inlined_string.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "google/protobuf/arena_test_util.h"

#define MESSAGE_TEST_NAME MessageTestInlinedString
#define MESSAGE_FACTORY_TEST_NAME MessageFactoryTestInlinedString
#define UNITTEST_PACKAGE_NAME "proto2_unittest_inlined_string"
#define UNITTEST ::proto2_unittest_inlined_string
#define UNITTEST_IMPORT ::proto2_unittest_import_inlined_string

// Must include after the above macros.
// clang-format off
#include "google/protobuf/message_unittest.inc"
#include "google/protobuf/message_unittest_legacy_apis.inc"
// clang-format on

namespace google {
namespace protobuf {
namespace internal {
namespace {

using UNITTEST::ManyOptionalString;
using UNITTEST::TestAllTypes;

template <typename Proto, bool call_mutable>
struct OnStack {
  explicit OnStack(Arena* /*arena*/) {}
  Proto* operator->() { return &msg; }
  Proto& operator*() { return msg; }
  Proto* get() { return &msg; }
  bool use_mutable() { return call_mutable; }

  Proto msg;
};

template <typename Proto, bool call_mutable>
struct OnArena {
  explicit OnArena(Arena* arena) : msg(Arena::Create<Proto>(arena)) {}
  Proto* operator->() { return msg; }
  Proto& operator*() { return *msg; }
  Proto* get() { return msg; }
  bool use_mutable() { return call_mutable; }

  Proto* msg;
};

void FillString1(TestAllTypes* msg, absl::string_view val, bool use_mutable) {
  if (use_mutable) {
    msg->set_optional_string(val);
  } else {
    msg->set_optional_string(val);
  }
}
void FillString2(TestAllTypes* msg, absl::string_view val, bool use_mutable) {
  if (use_mutable) {
    msg->set_optional_bytes(val);
  } else {
    msg->set_optional_bytes(val);
  }
}
absl::string_view GetString1(TestAllTypes* msg) {
  return msg->optional_string();
}
absl::string_view GetString2(TestAllTypes* msg) {
  return msg->optional_bytes();
}

void FillString1(ManyOptionalString* msg, absl::string_view val,
                 bool use_mutable) {
  if (use_mutable) {
    msg->set_str1(val);
  } else {
    msg->set_str1(val);
  }
}
void FillString2(ManyOptionalString* msg, absl::string_view val,
                 bool use_mutable) {
  if (use_mutable) {
    msg->set_str32(val);
  } else {
    msg->set_str32(val);
  }
}
absl::string_view GetString1(ManyOptionalString* msg) { return msg->str1(); }
absl::string_view GetString2(ManyOptionalString* msg) { return msg->str32(); }

using AllocationTypes = ::testing::Types<
    // UNITTEST::TestAllTypes
    std::pair<OnStack<TestAllTypes, false>, OnStack<TestAllTypes, false>>,
    std::pair<OnStack<TestAllTypes, false>, OnStack<TestAllTypes, true>>,
    std::pair<OnStack<TestAllTypes, true>, OnStack<TestAllTypes, false>>,
    std::pair<OnStack<TestAllTypes, true>, OnStack<TestAllTypes, true>>,
    std::pair<OnStack<TestAllTypes, false>, OnArena<TestAllTypes, false>>,
    std::pair<OnStack<TestAllTypes, false>, OnArena<TestAllTypes, true>>,
    std::pair<OnStack<TestAllTypes, true>, OnArena<TestAllTypes, false>>,
    std::pair<OnStack<TestAllTypes, true>, OnArena<TestAllTypes, true>>,
    std::pair<OnArena<TestAllTypes, false>, OnStack<TestAllTypes, false>>,
    std::pair<OnArena<TestAllTypes, false>, OnStack<TestAllTypes, true>>,
    std::pair<OnArena<TestAllTypes, true>, OnStack<TestAllTypes, false>>,
    std::pair<OnArena<TestAllTypes, true>, OnStack<TestAllTypes, true>>,
    std::pair<OnArena<TestAllTypes, false>, OnArena<TestAllTypes, false>>,
    std::pair<OnArena<TestAllTypes, false>, OnArena<TestAllTypes, true>>,
    std::pair<OnArena<TestAllTypes, true>, OnArena<TestAllTypes, false>>,
    std::pair<OnArena<TestAllTypes, true>, OnArena<TestAllTypes, true>>,
    // UNITTEST::ManyOptionalString
    std::pair<OnStack<ManyOptionalString, false>,
              OnStack<ManyOptionalString, false>>,
    std::pair<OnStack<ManyOptionalString, false>,
              OnStack<ManyOptionalString, true>>,
    std::pair<OnStack<ManyOptionalString, true>,
              OnStack<ManyOptionalString, false>>,
    std::pair<OnStack<ManyOptionalString, true>,
              OnStack<ManyOptionalString, true>>,
    std::pair<OnStack<ManyOptionalString, false>,
              OnArena<ManyOptionalString, false>>,
    std::pair<OnStack<ManyOptionalString, false>,
              OnArena<ManyOptionalString, true>>,
    std::pair<OnStack<ManyOptionalString, true>,
              OnArena<ManyOptionalString, false>>,
    std::pair<OnStack<ManyOptionalString, true>,
              OnArena<ManyOptionalString, true>>,
    std::pair<OnArena<ManyOptionalString, false>,
              OnStack<ManyOptionalString, false>>,
    std::pair<OnArena<ManyOptionalString, false>,
              OnStack<ManyOptionalString, true>>,
    std::pair<OnArena<ManyOptionalString, true>,
              OnStack<ManyOptionalString, false>>,
    std::pair<OnArena<ManyOptionalString, true>,
              OnStack<ManyOptionalString, true>>,
    std::pair<OnArena<ManyOptionalString, false>,
              OnArena<ManyOptionalString, false>>,
    std::pair<OnArena<ManyOptionalString, false>,
              OnArena<ManyOptionalString, true>>,
    std::pair<OnArena<ManyOptionalString, true>,
              OnArena<ManyOptionalString, false>>,
    std::pair<OnArena<ManyOptionalString, true>,
              OnArena<ManyOptionalString, true>>>;

template <typename T>
class MessageTestInlinedString : public ::testing::Test {
 protected:
  const std::string long_str1_l_ = std::string(100, 'x');
  const std::string long_str2_l_ = std::string(200, 'x');
  const std::string long_str1_r_ = std::string(300, 'y');
  const std::string long_str2_r_ = std::string(400, 'y');
};

TYPED_TEST_SUITE_P(MessageTestInlinedString);

TYPED_TEST_P(MessageTestInlinedString, Swap) {
  using LhsType = typename TypeParam::first_type;
  using RhsType = typename TypeParam::second_type;
  Arena arena;
  {
    LhsType lhs(&arena);
    FillString1(lhs.get(), this->long_str1_l_, lhs.use_mutable());
    FillString2(lhs.get(), this->long_str2_l_, lhs.use_mutable());
    {
      RhsType rhs(&arena);
      FillString1(rhs.get(), this->long_str1_r_, rhs.use_mutable());
      FillString2(rhs.get(), this->long_str2_r_, rhs.use_mutable());
      lhs->Swap(rhs.get());
      EXPECT_EQ(GetString1(rhs.get()), this->long_str1_l_);
      EXPECT_EQ(GetString2(rhs.get()), this->long_str2_l_);
      EXPECT_EQ(GetString1(lhs.get()), this->long_str1_r_);
      EXPECT_EQ(GetString2(lhs.get()), this->long_str2_r_);
    }
  }
}

TYPED_TEST_P(MessageTestInlinedString, SwapReflect) {
  using LhsType = typename TypeParam::first_type;
  using RhsType = typename TypeParam::second_type;
  Arena arena;
  {
    LhsType lhs(&arena);
    FillString1(lhs.get(), this->long_str1_l_, lhs.use_mutable());
    FillString2(lhs.get(), this->long_str2_l_, lhs.use_mutable());
    {
      RhsType rhs(&arena);
      FillString1(rhs.get(), this->long_str1_r_, rhs.use_mutable());
      FillString2(rhs.get(), this->long_str2_r_, rhs.use_mutable());
      const Reflection* reflection = lhs->GetReflection();
      reflection->Swap(lhs.get(), rhs.get());
      EXPECT_EQ(GetString1(rhs.get()), this->long_str1_l_);
      EXPECT_EQ(GetString2(rhs.get()), this->long_str2_l_);
      EXPECT_EQ(GetString1(lhs.get()), this->long_str1_r_);
      EXPECT_EQ(GetString2(lhs.get()), this->long_str2_r_);
    }
  }
}

TEST(MessageTestInlinedString,
     InlineStringFieldGetsProperlyRegisteredInTheArena) {
  Arena arena;
  auto* msg = Arena::Create<UNITTEST::TestString>(&arena);
  // Nothing in the cleanup yet.
  EXPECT_THAT(internal::ArenaTestPeer::PeekCleanupListForTesting(&arena),
              IsEmpty());
  // Very large input that needs allocation.
  msg->set_optional_string(std::string(1000, 'x'));
  // but now we should be registered from the on-demand call.
  EXPECT_THAT(internal::ArenaTestPeer::PeekCleanupListForTesting(&arena),
              Not(IsEmpty()));
  // We expect memory leaks here if the inline string was not properly
  // destroyed.
}

REGISTER_TYPED_TEST_SUITE_P(MessageTestInlinedString, Swap, SwapReflect);
INSTANTIATE_TYPED_TEST_SUITE_P(Do, MessageTestInlinedString, AllocationTypes);
}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
