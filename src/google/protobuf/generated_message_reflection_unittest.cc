// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// To test GeneratedMessageReflection, we actually let the protocol compiler
// generate a full protocol message implementation and then test its
// reflection interface.  This is much easier and more maintainable than
// trying to create our own Message class for GeneratedMessageReflection
// to wrap.
//
// The tests here closely mirror some of the tests in
// compiler/cpp/unittest, except using the reflection interface
// rather than generated accessors.

#include "google/protobuf/generated_message_reflection.h"

#include <memory>

#include "google/protobuf/arena.h"
#include "google/protobuf/descriptor.h"
#include <gmock/gmock.h>
#include "google/protobuf/testing/googletest.h"
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/cord.h"
#include "google/protobuf/map_test_util.h"
#include "google/protobuf/map_unittest.pb.h"
#include "google/protobuf/test_util.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest_mset.pb.h"
#include "google/protobuf/unittest_mset_wire_format.pb.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

class GeneratedMessageReflectionTestHelper {
 public:
  static void UnsafeShallowSwapFields(
      Message* lhs, Message* rhs,
      const std::vector<const FieldDescriptor*>& fields) {
    lhs->GetReflection()->UnsafeShallowSwapFields(lhs, rhs, fields);
  }
  static bool IsLazyExtension(const Message& msg, const FieldDescriptor* ext) {
    return msg.GetReflection()->IsLazyExtension(msg, ext);
  }
  static bool IsLazyField(const Message& msg, const FieldDescriptor* field) {
    return msg.GetReflection()->IsLazyField(field);
  }
  static bool IsEagerlyVerifiedLazyField(const Message& msg,
                                         const FieldDescriptor* field) {
    return msg.GetReflection()->IsEagerlyVerifiedLazyField(field);
  }
  static bool IsLazilyVerifiedLazyField(const Message& msg,
                                        const FieldDescriptor* field) {
    return msg.GetReflection()->IsLazilyVerifiedLazyField(field);
  }
};

namespace {

using ::testing::ElementsAre;
using ::testing::Pointee;
using ::testing::Property;

// Shorthand to get a FieldDescriptor for a field of unittest::TestAllTypes.
const FieldDescriptor* F(const std::string& name) {
  const FieldDescriptor* result =
      unittest::TestAllTypes::descriptor()->FindFieldByName(name);
  ABSL_CHECK(result != nullptr);
  return result;
}

TEST(GeneratedMessageReflectionTest, Defaults) {
  // Check that all default values are set correctly in the initial message.
  unittest::TestAllTypes message;
  TestUtil::ReflectionTester reflection_tester(
      unittest::TestAllTypes::descriptor());

  reflection_tester.ExpectClearViaReflection(message);

  const Reflection* reflection = message.GetReflection();

  // Messages should return pointers to default instances until first use.
  // (This is not checked by ExpectClear() since it is not actually true after
  // the fields have been set and then cleared.)
  EXPECT_EQ(&unittest::TestAllTypes::OptionalGroup::default_instance(),
            &reflection->GetMessage(message, F("optionalgroup")));
  EXPECT_EQ(&unittest::TestAllTypes::NestedMessage::default_instance(),
            &reflection->GetMessage(message, F("optional_nested_message")));
  EXPECT_EQ(&unittest::ForeignMessage::default_instance(),
            &reflection->GetMessage(message, F("optional_foreign_message")));
  EXPECT_EQ(&unittest_import::ImportMessage::default_instance(),
            &reflection->GetMessage(message, F("optional_import_message")));
}

TEST(GeneratedMessageReflectionTest, Accessors) {
  // Set every field to a unique value then go back and check all those
  // values.
  unittest::TestAllTypes message;
  TestUtil::ReflectionTester reflection_tester(
      unittest::TestAllTypes::descriptor());

  reflection_tester.SetAllFieldsViaReflection(&message);
  TestUtil::ExpectAllFieldsSet(message);
  reflection_tester.ExpectAllFieldsSetViaReflection(message);

  reflection_tester.ModifyRepeatedFieldsViaReflection(&message);
  TestUtil::ExpectRepeatedFieldsModified(message);
}

TEST(GeneratedMessageReflectionTest, GetStringReference) {
  // Test that GetStringReference() returns the underlying string when it
  // is a normal string field.
  unittest::TestAllTypes message;
  message.set_optional_string("foo");
  message.add_repeated_string("foo");

  const Reflection* reflection = message.GetReflection();
  std::string scratch;

  EXPECT_EQ(
      &message.optional_string(),
      &reflection->GetStringReference(message, F("optional_string"), &scratch))
      << "For simple string fields, GetStringReference() should return a "
         "reference to the underlying string.";
  EXPECT_EQ(&message.repeated_string(0),
            &reflection->GetRepeatedStringReference(
                message, F("repeated_string"), 0, &scratch))
      << "For simple string fields, GetRepeatedStringReference() should "
         "return "
         "a reference to the underlying string.";
}

TEST(GeneratedMessageReflectionTest, GetStringReferenceCopy) {
  // Test that GetStringReference() returns the scratch string when the
  // underlying representation is not a normal string.
  unittest::TestCord cord_message;
  cord_message.set_optional_bytes_cord("bytes_cord");

  const Reflection* cord_reflection = cord_message.GetReflection();
  const Descriptor* descriptor = unittest::TestCord::descriptor();
  std::string cord_scratch;
  EXPECT_EQ(
      &cord_scratch,
      &cord_reflection->GetStringReference(
          cord_message, descriptor->FindFieldByName("optional_bytes_cord"),
          &cord_scratch));
}


class GeneratedMessageReflectionSwapTest : public testing::TestWithParam<bool> {
 protected:
  void Swap(const Reflection* reflection, Message* lhs, Message* rhs) {
    if (GetParam()) {
      reflection->UnsafeArenaSwap(lhs, rhs);
    } else {
      reflection->Swap(lhs, rhs);
    }
  }
  void SwapFields(const Reflection* reflection, Message* lhs, Message* rhs,
                  const std::vector<const FieldDescriptor*>& fields) {
    if (GetParam()) {
      reflection->UnsafeArenaSwapFields(lhs, rhs, fields);
    } else {
      reflection->SwapFields(lhs, rhs, fields);
    }
  }
};

// unsafe_shallow_swap: true -> UnsafeArena* API.
INSTANTIATE_TEST_SUITE_P(ReflectionSwap, GeneratedMessageReflectionSwapTest,
                         testing::Bool());

TEST_P(GeneratedMessageReflectionSwapTest, LhsSet) {
  unittest::TestAllTypes lhs;
  unittest::TestAllTypes rhs;

  TestUtil::SetAllFields(&lhs);

  Swap(lhs.GetReflection(), &lhs, &rhs);

  TestUtil::ExpectClear(lhs);
  TestUtil::ExpectAllFieldsSet(rhs);
}

TEST_P(GeneratedMessageReflectionSwapTest, BothSet) {
  unittest::TestAllTypes lhs;
  unittest::TestAllTypes rhs;

  TestUtil::SetAllFields(&lhs);
  TestUtil::SetAllFields(&rhs);
  TestUtil::ModifyRepeatedFields(&rhs);

  const Reflection* reflection = lhs.GetReflection();
  Swap(reflection, &lhs, &rhs);

  TestUtil::ExpectRepeatedFieldsModified(lhs);
  TestUtil::ExpectAllFieldsSet(rhs);

  lhs.set_optional_int32(532819);

  Swap(reflection, &lhs, &rhs);

  EXPECT_EQ(532819, rhs.optional_int32());
}

TEST_P(GeneratedMessageReflectionSwapTest, LhsCleared) {
  unittest::TestAllTypes lhs;
  unittest::TestAllTypes rhs;

  TestUtil::SetAllFields(&lhs);

  // For proto2 message, for message field, Clear only reset hasbits, but
  // doesn't delete the underlying field.
  lhs.Clear();

  Swap(lhs.GetReflection(), &lhs, &rhs);

  TestUtil::ExpectClear(rhs);
}

TEST_P(GeneratedMessageReflectionSwapTest, RhsCleared) {
  unittest::TestAllTypes lhs;
  unittest::TestAllTypes rhs;

  TestUtil::SetAllFields(&rhs);

  // For proto2 message, for message field, Clear only reset hasbits, but
  // doesn't delete the underlying field.
  rhs.Clear();

  Swap(lhs.GetReflection(), &lhs, &rhs);

  TestUtil::ExpectClear(lhs);
}

TEST_P(GeneratedMessageReflectionSwapTest, Extensions) {
  unittest::TestAllExtensions lhs;
  unittest::TestAllExtensions rhs;

  TestUtil::SetAllExtensions(&lhs);

  Swap(lhs.GetReflection(), &lhs, &rhs);

  TestUtil::ExpectExtensionsClear(lhs);
  TestUtil::ExpectAllExtensionsSet(rhs);
}

TEST_P(GeneratedMessageReflectionSwapTest, Unknown) {
  unittest::TestEmptyMessage lhs, rhs;

  lhs.mutable_unknown_fields()->AddVarint(1234, 1);

  EXPECT_EQ(1, lhs.unknown_fields().field_count());
  EXPECT_EQ(0, rhs.unknown_fields().field_count());
  Swap(lhs.GetReflection(), &lhs, &rhs);
  EXPECT_EQ(0, lhs.unknown_fields().field_count());
  EXPECT_EQ(1, rhs.unknown_fields().field_count());
}

TEST_P(GeneratedMessageReflectionSwapTest, Oneof) {
  unittest::TestOneof2 lhs, rhs;
  TestUtil::SetOneof1(&lhs);

  Swap(lhs.GetReflection(), &lhs, &rhs);

  TestUtil::ExpectOneofClear(lhs);
  TestUtil::ExpectOneofSet1(rhs);
}

TEST_P(GeneratedMessageReflectionSwapTest, OneofBothSet) {
  unittest::TestOneof2 lhs, rhs;
  TestUtil::SetOneof1(&lhs);
  TestUtil::SetOneof2(&rhs);

  Swap(lhs.GetReflection(), &lhs, &rhs);

  TestUtil::ExpectOneofSet2(lhs);
  TestUtil::ExpectOneofSet1(rhs);
}

TEST_P(GeneratedMessageReflectionSwapTest, SwapFields) {
  std::unique_ptr<unittest::TestAllTypes> lhs(
      Arena::CreateMessage<unittest::TestAllTypes>(nullptr));
  std::unique_ptr<unittest::TestAllTypes> rhs(
      Arena::CreateMessage<unittest::TestAllTypes>(nullptr));
  lhs->set_optional_double(12.3);
  lhs->mutable_repeated_int32()->Add(10);
  lhs->mutable_repeated_int32()->Add(20);

  rhs->set_optional_string("hello");
  rhs->mutable_repeated_int64()->Add(30);

  std::vector<const FieldDescriptor*> fields;
  const Descriptor* descriptor = lhs->GetDescriptor();
  fields.push_back(descriptor->FindFieldByName("optional_double"));
  fields.push_back(descriptor->FindFieldByName("repeated_int32"));
  fields.push_back(descriptor->FindFieldByName("optional_string"));
  fields.push_back(descriptor->FindFieldByName("optional_uint64"));

  SwapFields(lhs->GetReflection(), lhs.get(), rhs.get(), fields);

  EXPECT_FALSE(lhs->has_optional_double());
  EXPECT_EQ(0, lhs->repeated_int32_size());
  EXPECT_TRUE(lhs->has_optional_string());
  EXPECT_EQ("hello", lhs->optional_string());
  EXPECT_EQ(0, lhs->repeated_int64_size());
  EXPECT_FALSE(lhs->has_optional_uint64());

  EXPECT_TRUE(rhs->has_optional_double());
  EXPECT_EQ(12.3, rhs->optional_double());
  EXPECT_EQ(2, rhs->repeated_int32_size());
  EXPECT_EQ(10, rhs->repeated_int32(0));
  EXPECT_EQ(20, rhs->repeated_int32(1));
  EXPECT_FALSE(rhs->has_optional_string());
  EXPECT_EQ(1, rhs->repeated_int64_size());
  EXPECT_FALSE(rhs->has_optional_uint64());
}

TEST_P(GeneratedMessageReflectionSwapTest, SwapFieldsAll) {
  std::unique_ptr<unittest::TestAllTypes> lhs(
      Arena::CreateMessage<unittest::TestAllTypes>(nullptr));
  std::unique_ptr<unittest::TestAllTypes> rhs(
      Arena::CreateMessage<unittest::TestAllTypes>(nullptr));

  TestUtil::SetAllFields(rhs.get());

  std::vector<const FieldDescriptor*> fields;
  const Reflection* reflection = lhs->GetReflection();
  reflection->ListFields(*rhs, &fields);
  SwapFields(reflection, lhs.get(), rhs.get(), fields);

  TestUtil::ExpectAllFieldsSet(*lhs);
  TestUtil::ExpectClear(*rhs);
}

TEST(GeneratedMessageReflectionTest, SwapFieldsAllOnDifferentArena) {
  Arena arena1, arena2;
  auto* message1 = Arena::CreateMessage<unittest::TestAllTypes>(&arena1);
  auto* message2 = Arena::CreateMessage<unittest::TestAllTypes>(&arena2);

  TestUtil::SetAllFields(message2);

  std::vector<const FieldDescriptor*> fields;
  const Reflection* reflection = message1->GetReflection();
  reflection->ListFields(*message2, &fields);
  reflection->SwapFields(message1, message2, fields);

  TestUtil::ExpectAllFieldsSet(*message1);
  TestUtil::ExpectClear(*message2);
}

TEST(GeneratedMessageReflectionTest, SwapFieldsAllOnArenaHeap) {
  Arena arena;
  auto* message1 = Arena::CreateMessage<unittest::TestAllTypes>(&arena);
  std::unique_ptr<unittest::TestAllTypes> message2(
      Arena::CreateMessage<unittest::TestAllTypes>(nullptr));

  TestUtil::SetAllFields(message2.get());

  std::vector<const FieldDescriptor*> fields;
  const Reflection* reflection = message1->GetReflection();
  reflection->ListFields(*message2, &fields);
  reflection->SwapFields(message1, message2.get(), fields);

  TestUtil::ExpectAllFieldsSet(*message1);
  TestUtil::ExpectClear(*message2);
}

TEST(GeneratedMessageReflectionTest, SwapFieldsAllExtension) {
  unittest::TestAllExtensions message1;
  unittest::TestAllExtensions message2;

  TestUtil::SetAllExtensions(&message1);

  std::vector<const FieldDescriptor*> fields;
  const Reflection* reflection = message1.GetReflection();
  reflection->ListFields(message1, &fields);
  reflection->SwapFields(&message1, &message2, fields);

  TestUtil::ExpectExtensionsClear(message1);
  TestUtil::ExpectAllExtensionsSet(message2);
}

TEST(GeneratedMessageReflectionTest, SwapFieldsAllExtensionArenaHeap) {
  Arena arena;

  std::unique_ptr<unittest::TestAllExtensions> message1(
      Arena::CreateMessage<unittest::TestAllExtensions>(nullptr));
  auto* message2 = Arena::CreateMessage<unittest::TestAllExtensions>(&arena);

  TestUtil::SetAllExtensions(message1.get());

  std::vector<const FieldDescriptor*> fields;
  const Reflection* reflection = message1->GetReflection();
  reflection->ListFields(*message1, &fields);
  reflection->SwapFields(message1.get(), message2, fields);

  TestUtil::ExpectExtensionsClear(*message1);
  TestUtil::ExpectAllExtensionsSet(*message2);
}

TEST(GeneratedMessageReflectionTest, UnsafeShallowSwapFieldsAll) {
  Arena arena;
  auto* message1 = Arena::CreateMessage<unittest::TestAllTypes>(&arena);
  auto* message2 = Arena::CreateMessage<unittest::TestAllTypes>(&arena);

  TestUtil::SetAllFields(message2);

  auto* kept_nested_message_ptr = message2->mutable_optional_nested_message();
  auto* kept_foreign_message_ptr = message2->mutable_optional_foreign_message();
  auto* kept_repeated_nested_message_ptr =
      message2->mutable_repeated_nested_message(0);
  auto* kept_repeated_foreign_message_ptr =
      message2->mutable_repeated_foreign_message(0);

  std::vector<const FieldDescriptor*> fields;
  const Reflection* reflection = message1->GetReflection();
  reflection->ListFields(*message2, &fields);
  GeneratedMessageReflectionTestHelper::UnsafeShallowSwapFields(
      message1, message2, fields);

  TestUtil::ExpectAllFieldsSet(*message1);
  TestUtil::ExpectClear(*message2);

  // Expects the swap to be shallow. Expects pointer stability to the element of
  // the repeated fields (not the container).
  EXPECT_EQ(kept_nested_message_ptr,
            message1->mutable_optional_nested_message());
  EXPECT_EQ(kept_foreign_message_ptr,
            message1->mutable_optional_foreign_message());
  EXPECT_EQ(kept_repeated_nested_message_ptr,
            message1->mutable_repeated_nested_message(0));
  EXPECT_EQ(kept_repeated_foreign_message_ptr,
            message1->mutable_repeated_foreign_message(0));
}

TEST(GeneratedMessageReflectionTest, UnsafeShallowSwapFieldsMap) {
  Arena arena;
  auto* message1 = Arena::CreateMessage<unittest::TestMap>(&arena);
  auto* message2 = Arena::CreateMessage<unittest::TestMap>(&arena);

  MapTestUtil::SetMapFields(message2);

  auto* kept_map_int32_fm_ptr =
      &(*message2->mutable_map_int32_foreign_message())[0];

  std::vector<const FieldDescriptor*> fields;
  const Reflection* reflection = message1->GetReflection();
  reflection->ListFields(*message2, &fields);
  GeneratedMessageReflectionTestHelper::UnsafeShallowSwapFields(
      message1, message2, fields);

  MapTestUtil::ExpectMapFieldsSet(*message1);
  MapTestUtil::ExpectClear(*message2);

  // Expects the swap to be shallow.
  EXPECT_EQ(kept_map_int32_fm_ptr,
            &(*message1->mutable_map_int32_foreign_message())[0]);
}

TEST(GeneratedMessageReflectionTest, UnsafeShallowSwapFieldsAllExtension) {
  Arena arena;
  auto* message1 = Arena::CreateMessage<unittest::TestAllExtensions>(&arena);
  auto* message2 = Arena::CreateMessage<unittest::TestAllExtensions>(&arena);

  TestUtil::SetAllExtensions(message1);

  auto* kept_nested_message_ext_ptr =
      message1->MutableExtension(unittest::optional_nested_message_extension);
  auto* kept_foreign_message_ext_ptr =
      message1->MutableExtension(unittest::optional_foreign_message_extension);
  auto* kept_repeated_nested_message_ext_ptr =
      message1->MutableRepeatedExtension(
          unittest::repeated_nested_message_extension);
  auto* kept_repeated_foreign_message_ext_ptr =
      message1->MutableRepeatedExtension(
          unittest::repeated_foreign_message_extension);

  std::vector<const FieldDescriptor*> fields;
  const Reflection* reflection = message1->GetReflection();
  reflection->ListFields(*message1, &fields);
  GeneratedMessageReflectionTestHelper::UnsafeShallowSwapFields(
      message1, message2, fields);

  TestUtil::ExpectExtensionsClear(*message1);
  TestUtil::ExpectAllExtensionsSet(*message2);

  // Expects the swap to be shallow.
  EXPECT_EQ(
      kept_nested_message_ext_ptr,
      message2->MutableExtension(unittest::optional_nested_message_extension));
  EXPECT_EQ(
      kept_foreign_message_ext_ptr,
      message2->MutableExtension(unittest::optional_foreign_message_extension));
  EXPECT_EQ(kept_repeated_nested_message_ext_ptr,
            message2->MutableRepeatedExtension(
                unittest::repeated_nested_message_extension));
  EXPECT_EQ(kept_repeated_foreign_message_ext_ptr,
            message2->MutableRepeatedExtension(
                unittest::repeated_foreign_message_extension));
}

TEST(GeneratedMessageReflectionTest, SwapFieldsOneof) {
  unittest::TestOneof2 message1, message2;
  TestUtil::SetOneof1(&message1);

  std::vector<const FieldDescriptor*> fields;
  const Descriptor* descriptor = message1.GetDescriptor();
  for (int i = 0; i < descriptor->field_count(); i++) {
    fields.push_back(descriptor->field(i));
  }
  const Reflection* reflection = message1.GetReflection();
  reflection->SwapFields(&message1, &message2, fields);

  TestUtil::ExpectOneofClear(message1);
  TestUtil::ExpectOneofSet1(message2);
}

TEST(GeneratedMessageReflectionTest, UnsafeShallowSwapFieldsOneof) {
  Arena arena;
  auto* message1 = Arena::CreateMessage<unittest::TestOneof2>(&arena);
  auto* message2 = Arena::CreateMessage<unittest::TestOneof2>(&arena);
  TestUtil::SetOneof1(message1);

  std::vector<const FieldDescriptor*> fields;
  const Descriptor* descriptor = message1->GetDescriptor();
  for (int i = 0; i < descriptor->field_count(); i++) {
    fields.push_back(descriptor->field(i));
  }
  GeneratedMessageReflectionTestHelper::UnsafeShallowSwapFields(
      message1, message2, fields);

  TestUtil::ExpectOneofClear(*message1);
  TestUtil::ExpectOneofSet1(*message2);
}

TEST(GeneratedMessageReflectionTest,
     UnsafeShallowSwapFieldsOneofExpectShallow) {
  Arena arena;
  auto* message1 = Arena::CreateMessage<unittest::TestOneof2>(&arena);
  auto* message2 = Arena::CreateMessage<unittest::TestOneof2>(&arena);
  TestUtil::SetOneof1(message1);
  message1->mutable_foo_message()->set_moo_int(1000);
  auto* kept_foo_ptr = message1->mutable_foo_message();

  std::vector<const FieldDescriptor*> fields;
  const Descriptor* descriptor = message1->GetDescriptor();
  for (int i = 0; i < descriptor->field_count(); i++) {
    fields.push_back(descriptor->field(i));
  }
  GeneratedMessageReflectionTestHelper::UnsafeShallowSwapFields(
      message1, message2, fields);

  EXPECT_TRUE(message2->has_foo_message());
  EXPECT_EQ(message2->foo_message().moo_int(), 1000);
  EXPECT_EQ(kept_foo_ptr, message2->mutable_foo_message());
}

TEST(GeneratedMessageReflectionTest, RemoveLast) {
  unittest::TestAllTypes message;
  TestUtil::ReflectionTester reflection_tester(
      unittest::TestAllTypes::descriptor());

  TestUtil::SetAllFields(&message);

  reflection_tester.RemoveLastRepeatedsViaReflection(&message);

  TestUtil::ExpectLastRepeatedsRemoved(message);
}

TEST(GeneratedMessageReflectionTest, RemoveLastExtensions) {
  unittest::TestAllExtensions message;
  TestUtil::ReflectionTester reflection_tester(
      unittest::TestAllExtensions::descriptor());

  TestUtil::SetAllExtensions(&message);

  reflection_tester.RemoveLastRepeatedsViaReflection(&message);

  TestUtil::ExpectLastRepeatedExtensionsRemoved(message);
}

TEST(GeneratedMessageReflectionTest, ReleaseLast) {
  unittest::TestAllTypes message;
  const Descriptor* descriptor = message.GetDescriptor();
  TestUtil::ReflectionTester reflection_tester(descriptor);

  TestUtil::SetAllFields(&message);

  reflection_tester.ReleaseLastRepeatedsViaReflection(&message, false);

  TestUtil::ExpectLastRepeatedsReleased(message);

  // Now test that we actually release the right message.
  message.Clear();
  TestUtil::SetAllFields(&message);
  ASSERT_EQ(2, message.repeated_foreign_message_size());
  const protobuf_unittest::ForeignMessage* expected =
      message.mutable_repeated_foreign_message(1);
  (void)expected;  // unused in somce configurations
  std::unique_ptr<Message> released(message.GetReflection()->ReleaseLast(
      &message, descriptor->FindFieldByName("repeated_foreign_message")));
#ifndef PROTOBUF_FORCE_COPY_IN_RELEASE
  EXPECT_EQ(expected, released.get());
#endif  // !PROTOBUF_FORCE_COPY_IN_RELEASE
}

TEST(GeneratedMessageReflectionTest, ReleaseLastExtensions) {
#ifdef PROTOBUF_FORCE_COPY_IN_RELEASE
  GTEST_SKIP() << "Won't work with FORCE_COPY_IN_RELEASE.";
#endif  // !PROTOBUF_FORCE_COPY_IN_RELEASE

  unittest::TestAllExtensions message;
  const Descriptor* descriptor = message.GetDescriptor();
  TestUtil::ReflectionTester reflection_tester(descriptor);

  TestUtil::SetAllExtensions(&message);

  reflection_tester.ReleaseLastRepeatedsViaReflection(&message, true);

  TestUtil::ExpectLastRepeatedExtensionsReleased(message);

  // Now test that we actually release the right message.
  message.Clear();
  TestUtil::SetAllExtensions(&message);
  ASSERT_EQ(
      2, message.ExtensionSize(unittest::repeated_foreign_message_extension));
  const protobuf_unittest::ForeignMessage* expected =
      message.MutableExtension(unittest::repeated_foreign_message_extension, 1);
  std::unique_ptr<Message> released(message.GetReflection()->ReleaseLast(
      &message, descriptor->file()->FindExtensionByName(
                    "repeated_foreign_message_extension")));
  EXPECT_EQ(expected, released.get());
}

TEST(GeneratedMessageReflectionTest, SwapRepeatedElements) {
  unittest::TestAllTypes message;
  TestUtil::ReflectionTester reflection_tester(
      unittest::TestAllTypes::descriptor());

  TestUtil::SetAllFields(&message);

  // Swap and test that fields are all swapped.
  reflection_tester.SwapRepeatedsViaReflection(&message);
  TestUtil::ExpectRepeatedsSwapped(message);

  // Swap back and test that fields are all back to original values.
  reflection_tester.SwapRepeatedsViaReflection(&message);
  TestUtil::ExpectAllFieldsSet(message);
}

TEST(GeneratedMessageReflectionTest, SwapRepeatedElementsExtension) {
  unittest::TestAllExtensions message;
  TestUtil::ReflectionTester reflection_tester(
      unittest::TestAllExtensions::descriptor());

  TestUtil::SetAllExtensions(&message);

  // Swap and test that fields are all swapped.
  reflection_tester.SwapRepeatedsViaReflection(&message);
  TestUtil::ExpectRepeatedExtensionsSwapped(message);

  // Swap back and test that fields are all back to original values.
  reflection_tester.SwapRepeatedsViaReflection(&message);
  TestUtil::ExpectAllExtensionsSet(message);
}

TEST(GeneratedMessageReflectionTest, Extensions) {
  // Set every extension to a unique value then go back and check all those
  // values.
  unittest::TestAllExtensions message;
  TestUtil::ReflectionTester reflection_tester(
      unittest::TestAllExtensions::descriptor());

  reflection_tester.SetAllFieldsViaReflection(&message);
  TestUtil::ExpectAllExtensionsSet(message);
  reflection_tester.ExpectAllFieldsSetViaReflection(message);

  reflection_tester.ModifyRepeatedFieldsViaReflection(&message);
  TestUtil::ExpectRepeatedExtensionsModified(message);
}

TEST(GeneratedMessageReflectionTest, FindExtensionTypeByNumber) {
  const Reflection* reflection =
      unittest::TestAllExtensions::default_instance().GetReflection();

  const FieldDescriptor* extension1 =
      unittest::TestAllExtensions::descriptor()->file()->FindExtensionByName(
          "optional_int32_extension");
  const FieldDescriptor* extension2 =
      unittest::TestAllExtensions::descriptor()->file()->FindExtensionByName(
          "repeated_string_extension");

  EXPECT_EQ(extension1,
            reflection->FindKnownExtensionByNumber(extension1->number()));
  EXPECT_EQ(extension2,
            reflection->FindKnownExtensionByNumber(extension2->number()));

  // Non-existent extension.
  EXPECT_TRUE(reflection->FindKnownExtensionByNumber(62341) == nullptr);

  // Extensions of TestAllExtensions should not show up as extensions of
  // other types.
  EXPECT_TRUE(unittest::TestAllTypes::default_instance()
                  .GetReflection()
                  ->FindKnownExtensionByNumber(extension1->number()) ==
              nullptr);
}

TEST(GeneratedMessageReflectionTest, FindKnownExtensionByName) {
  const Reflection* reflection =
      unittest::TestAllExtensions::default_instance().GetReflection();

  const FieldDescriptor* extension1 =
      unittest::TestAllExtensions::descriptor()->file()->FindExtensionByName(
          "optional_int32_extension");
  const FieldDescriptor* extension2 =
      unittest::TestAllExtensions::descriptor()->file()->FindExtensionByName(
          "repeated_string_extension");

  EXPECT_EQ(extension1,
            reflection->FindKnownExtensionByName(extension1->full_name()));
  EXPECT_EQ(extension2,
            reflection->FindKnownExtensionByName(extension2->full_name()));

  // Non-existent extension.
  EXPECT_TRUE(reflection->FindKnownExtensionByName("no_such_ext") == nullptr);

  // Extensions of TestAllExtensions should not show up as extensions of
  // other types.
  EXPECT_TRUE(unittest::TestAllTypes::default_instance()
                  .GetReflection()
                  ->FindKnownExtensionByName(extension1->full_name()) ==
              nullptr);
}


TEST(GeneratedMessageReflectionTest, SetAllocatedMessageTest) {
  unittest::TestAllTypes from_message1;
  unittest::TestAllTypes from_message2;
  unittest::TestAllTypes to_message;
  TestUtil::ReflectionTester reflection_tester(
      unittest::TestAllTypes::descriptor());
  reflection_tester.SetAllFieldsViaReflection(&from_message1);
  reflection_tester.SetAllFieldsViaReflection(&from_message2);

  // Before moving fields, we expect the nested messages to be nullptr.
  reflection_tester.ExpectMessagesReleasedViaReflection(
      &to_message, TestUtil::ReflectionTester::IS_NULL);

  // After fields are moved we should get non-nullptr releases.
  reflection_tester.SetAllocatedOptionalMessageFieldsToMessageViaReflection(
      &from_message1, &to_message);
  reflection_tester.ExpectMessagesReleasedViaReflection(
      &to_message, TestUtil::ReflectionTester::NOT_NULL);

  // Another move to make sure that we can SetAllocated several times.
  reflection_tester.SetAllocatedOptionalMessageFieldsToMessageViaReflection(
      &from_message2, &to_message);
  reflection_tester.ExpectMessagesReleasedViaReflection(
      &to_message, TestUtil::ReflectionTester::NOT_NULL);

  // After SetAllocatedOptionalMessageFieldsToNullViaReflection() we expect the
  // releases to be nullptr again.
  reflection_tester.SetAllocatedOptionalMessageFieldsToNullViaReflection(
      &to_message);
  reflection_tester.ExpectMessagesReleasedViaReflection(
      &to_message, TestUtil::ReflectionTester::IS_NULL);
}

TEST(GeneratedMessageReflectionTest, SetAllocatedMessageOnArenaTest) {
  unittest::TestAllTypes from_message1;
  unittest::TestAllTypes from_message2;
  Arena arena;
  unittest::TestAllTypes* to_message =
      Arena::CreateMessage<unittest::TestAllTypes>(&arena);
  TestUtil::ReflectionTester reflection_tester(
      unittest::TestAllTypes::descriptor());
  reflection_tester.SetAllFieldsViaReflection(&from_message1);
  reflection_tester.SetAllFieldsViaReflection(&from_message2);

  // Before moving fields, we expect the nested messages to be nullptr.
  reflection_tester.ExpectMessagesReleasedViaReflection(
      to_message, TestUtil::ReflectionTester::IS_NULL);

  // After fields are moved we should get non-nullptr releases.
  reflection_tester.SetAllocatedOptionalMessageFieldsToMessageViaReflection(
      &from_message1, to_message);
  reflection_tester.ExpectMessagesReleasedViaReflection(
      to_message, TestUtil::ReflectionTester::NOT_NULL);

  // Another move to make sure that we can SetAllocated several times.
  reflection_tester.SetAllocatedOptionalMessageFieldsToMessageViaReflection(
      &from_message2, to_message);
  reflection_tester.ExpectMessagesReleasedViaReflection(
      to_message, TestUtil::ReflectionTester::NOT_NULL);

  // After SetAllocatedOptionalMessageFieldsToNullViaReflection() we expect the
  // releases to be nullptr again.
  reflection_tester.SetAllocatedOptionalMessageFieldsToNullViaReflection(
      to_message);
  reflection_tester.ExpectMessagesReleasedViaReflection(
      to_message, TestUtil::ReflectionTester::IS_NULL);
}

TEST(GeneratedMessageReflectionTest, SetAllocatedExtensionMessageTest) {
  unittest::TestAllExtensions from_message1;
  unittest::TestAllExtensions from_message2;
  unittest::TestAllExtensions to_message;
  TestUtil::ReflectionTester reflection_tester(
      unittest::TestAllExtensions::descriptor());
  reflection_tester.SetAllFieldsViaReflection(&from_message1);
  reflection_tester.SetAllFieldsViaReflection(&from_message2);

  // Before moving fields, we expect the nested messages to be nullptr.
  reflection_tester.ExpectMessagesReleasedViaReflection(
      &to_message, TestUtil::ReflectionTester::IS_NULL);

  // After fields are moved we should get non-nullptr releases.
  reflection_tester.SetAllocatedOptionalMessageFieldsToMessageViaReflection(
      &from_message1, &to_message);
  reflection_tester.ExpectMessagesReleasedViaReflection(
      &to_message, TestUtil::ReflectionTester::NOT_NULL);

  // Another move to make sure that we can SetAllocated several times.
  reflection_tester.SetAllocatedOptionalMessageFieldsToMessageViaReflection(
      &from_message2, &to_message);
  reflection_tester.ExpectMessagesReleasedViaReflection(
      &to_message, TestUtil::ReflectionTester::NOT_NULL);

  // After SetAllocatedOptionalMessageFieldsToNullViaReflection() we expect the
  // releases to be nullptr again.
  reflection_tester.SetAllocatedOptionalMessageFieldsToNullViaReflection(
      &to_message);
  reflection_tester.ExpectMessagesReleasedViaReflection(
      &to_message, TestUtil::ReflectionTester::IS_NULL);
}

TEST(GeneratedMessageReflectionTest, SetAllocatedExtensionMessageOnArenaTest) {
  Arena arena;
  unittest::TestAllExtensions* to_message =
      Arena::CreateMessage<unittest::TestAllExtensions>(&arena);
  unittest::TestAllExtensions from_message1;
  unittest::TestAllExtensions from_message2;
  TestUtil::ReflectionTester reflection_tester(
      unittest::TestAllExtensions::descriptor());
  reflection_tester.SetAllFieldsViaReflection(&from_message1);
  reflection_tester.SetAllFieldsViaReflection(&from_message2);

  // Before moving fields, we expect the nested messages to be nullptr.
  reflection_tester.ExpectMessagesReleasedViaReflection(
      to_message, TestUtil::ReflectionTester::IS_NULL);

  // After fields are moved we should get non-nullptr releases.
  reflection_tester.SetAllocatedOptionalMessageFieldsToMessageViaReflection(
      &from_message1, to_message);
  reflection_tester.ExpectMessagesReleasedViaReflection(
      to_message, TestUtil::ReflectionTester::NOT_NULL);

  // Another move to make sure that we can SetAllocated several times.
  reflection_tester.SetAllocatedOptionalMessageFieldsToMessageViaReflection(
      &from_message2, to_message);
  reflection_tester.ExpectMessagesReleasedViaReflection(
      to_message, TestUtil::ReflectionTester::NOT_NULL);

  // After SetAllocatedOptionalMessageFieldsToNullViaReflection() we expect the
  // releases to be nullptr again.
  reflection_tester.SetAllocatedOptionalMessageFieldsToNullViaReflection(
      to_message);
  reflection_tester.ExpectMessagesReleasedViaReflection(
      to_message, TestUtil::ReflectionTester::IS_NULL);
}

TEST(GeneratedMessageReflectionTest, AddRepeatedMessage) {
  unittest::TestAllTypes message;

  const Reflection* reflection = message.GetReflection();
  const Reflection* nested_reflection =
      unittest::TestAllTypes::NestedMessage::default_instance().GetReflection();

  const FieldDescriptor* nested_bb =
      unittest::TestAllTypes::NestedMessage::descriptor()->FindFieldByName(
          "bb");

  Message* nested =
      reflection->AddMessage(&message, F("repeated_nested_message"));
  nested_reflection->SetInt32(nested, nested_bb, 11);

  EXPECT_EQ(11, message.repeated_nested_message(0).bb());
}

TEST(GeneratedMessageReflectionTest, MutableRepeatedMessage) {
  unittest::TestAllTypes message;

  const Reflection* reflection = message.GetReflection();
  const Reflection* nested_reflection =
      unittest::TestAllTypes::NestedMessage::default_instance().GetReflection();

  const FieldDescriptor* nested_bb =
      unittest::TestAllTypes::NestedMessage::descriptor()->FindFieldByName(
          "bb");

  message.add_repeated_nested_message()->set_bb(12);

  Message* nested = reflection->MutableRepeatedMessage(
      &message, F("repeated_nested_message"), 0);
  EXPECT_EQ(12, nested_reflection->GetInt32(*nested, nested_bb));
  nested_reflection->SetInt32(nested, nested_bb, 13);
  EXPECT_EQ(13, message.repeated_nested_message(0).bb());
}

TEST(GeneratedMessageReflectionTest, AddAllocatedMessage) {
  unittest::TestAllTypes message;

  const Reflection* reflection = message.GetReflection();

  unittest::TestAllTypes::NestedMessage* nested =
      new unittest::TestAllTypes::NestedMessage();
  nested->set_bb(11);
  reflection->AddAllocatedMessage(&message, F("repeated_nested_message"),
                                  nested);
  EXPECT_EQ(1, message.repeated_nested_message_size());
  EXPECT_EQ(11, message.repeated_nested_message(0).bb());
}

TEST(GeneratedMessageReflectionTest, ListFieldsOneOf) {
  unittest::TestOneof2 message;
  TestUtil::SetOneof1(&message);

  const Reflection* reflection = message.GetReflection();
  std::vector<const FieldDescriptor*> fields;
  reflection->ListFields(message, &fields);
  EXPECT_EQ(4, fields.size());
}

TEST(GeneratedMessageReflectionTest, Oneof) {
  unittest::TestOneof2 message;
  const Descriptor* descriptor = message.GetDescriptor();
  const Reflection* reflection = message.GetReflection();

  // Check default values.
  EXPECT_EQ(
      0, reflection->GetInt32(message, descriptor->FindFieldByName("foo_int")));
  EXPECT_EQ("", reflection->GetString(
                    message, descriptor->FindFieldByName("foo_string")));
  EXPECT_EQ("", reflection->GetString(message,
                                      descriptor->FindFieldByName("foo_cord")));
  EXPECT_EQ("", reflection->GetString(
                    message, descriptor->FindFieldByName("foo_string_piece")));
  EXPECT_EQ("", reflection->GetString(
                    message, descriptor->FindFieldByName("foo_bytes")));
  EXPECT_EQ(
      unittest::TestOneof2::FOO,
      reflection->GetEnum(message, descriptor->FindFieldByName("foo_enum"))
          ->number());
  EXPECT_EQ(&unittest::TestOneof2::NestedMessage::default_instance(),
            &reflection->GetMessage(
                message, descriptor->FindFieldByName("foo_message")));
  EXPECT_EQ(&unittest::TestOneof2::FooGroup::default_instance(),
            &reflection->GetMessage(message,
                                    descriptor->FindFieldByName("foogroup")));
  EXPECT_NE(&unittest::TestOneof2::FooGroup::default_instance(),
            &reflection->GetMessage(
                message, descriptor->FindFieldByName("foo_lazy_message")));
  EXPECT_EQ("", reflection->GetString(
                    message, descriptor->FindFieldByName("foo_bytes_cord")));
  EXPECT_EQ(
      5, reflection->GetInt32(message, descriptor->FindFieldByName("bar_int")));
  EXPECT_EQ("STRING", reflection->GetString(
                          message, descriptor->FindFieldByName("bar_string")));
  EXPECT_EQ("CORD", reflection->GetString(
                        message, descriptor->FindFieldByName("bar_cord")));
  EXPECT_EQ("SPIECE",
            reflection->GetString(
                message, descriptor->FindFieldByName("bar_string_piece")));
  EXPECT_EQ("BYTES", reflection->GetString(
                         message, descriptor->FindFieldByName("bar_bytes")));
  EXPECT_EQ(
      unittest::TestOneof2::BAR,
      reflection->GetEnum(message, descriptor->FindFieldByName("bar_enum"))
          ->number());

  // Check Set functions.
  reflection->SetInt32(&message, descriptor->FindFieldByName("foo_int"), 123);
  EXPECT_EQ(123, reflection->GetInt32(message,
                                      descriptor->FindFieldByName("foo_int")));
  reflection->SetString(&message, descriptor->FindFieldByName("foo_string"),
                        "abc");
  EXPECT_EQ("abc", reflection->GetString(
                       message, descriptor->FindFieldByName("foo_string")));
  reflection->SetString(&message, descriptor->FindFieldByName("foo_bytes"),
                        "bytes");
  EXPECT_EQ("bytes", reflection->GetString(
                         message, descriptor->FindFieldByName("foo_bytes")));
  reflection->SetString(&message, descriptor->FindFieldByName("foo_bytes_cord"),
                        "bytes_cord");
  EXPECT_EQ("bytes_cord",
            reflection->GetString(
                message, descriptor->FindFieldByName("foo_bytes_cord")));
  reflection->SetString(&message, descriptor->FindFieldByName("bar_cord"),
                        "change_cord");
  EXPECT_EQ(
      "change_cord",
      reflection->GetString(message, descriptor->FindFieldByName("bar_cord")));
  reflection->SetString(&message,
                        descriptor->FindFieldByName("bar_string_piece"),
                        "change_spiece");
  EXPECT_EQ("change_spiece",
            reflection->GetString(
                message, descriptor->FindFieldByName("bar_string_piece")));

  message.clear_foo();
  message.clear_bar();
  TestUtil::ExpectOneofClear(message);
}

TEST(GeneratedMessageReflectionTest, SetAllocatedOneofMessageTest) {
  unittest::TestOneof2 from_message1;
  unittest::TestOneof2 from_message2;
  unittest::TestOneof2 to_message;
  const Descriptor* descriptor = unittest::TestOneof2::descriptor();
  const Reflection* reflection = to_message.GetReflection();

  Message* released = reflection->ReleaseMessage(
      &to_message, descriptor->FindFieldByName("foo_lazy_message"));
  EXPECT_TRUE(released == nullptr);
  released = reflection->ReleaseMessage(
      &to_message, descriptor->FindFieldByName("foo_message"));
  EXPECT_TRUE(released == nullptr);

  TestUtil::ReflectionTester::SetOneofViaReflection(&from_message1);
  TestUtil::ReflectionTester::ExpectOneofSetViaReflection(from_message1);

  TestUtil::ReflectionTester::
      SetAllocatedOptionalMessageFieldsToMessageViaReflection(&from_message1,
                                                              &to_message);
  const Message& sub_message = reflection->GetMessage(
      to_message, descriptor->FindFieldByName("foo_lazy_message"));
  (void)sub_message;  // unused in somce configurations
  released = reflection->ReleaseMessage(
      &to_message, descriptor->FindFieldByName("foo_lazy_message"));
  EXPECT_TRUE(released != nullptr);
#ifndef PROTOBUF_FORCE_COPY_IN_RELEASE
  EXPECT_EQ(&sub_message, released);
#endif  // !PROTOBUF_FORCE_COPY_IN_RELEASE
  delete released;

  TestUtil::ReflectionTester::SetOneofViaReflection(&from_message2);

  reflection->MutableMessage(&from_message2,
                             descriptor->FindFieldByName("foo_message"));

  TestUtil::ReflectionTester::
      SetAllocatedOptionalMessageFieldsToMessageViaReflection(&from_message2,
                                                              &to_message);

  const Message& sub_message2 = reflection->GetMessage(
      to_message, descriptor->FindFieldByName("foo_message"));
  (void)sub_message2;  // unused in somce configurations
  released = reflection->ReleaseMessage(
      &to_message, descriptor->FindFieldByName("foo_message"));
  EXPECT_TRUE(released != nullptr);
#ifndef PROTOBUF_FORCE_COPY_IN_RELEASE
  EXPECT_EQ(&sub_message2, released);
#endif  // !PROTOBUF_FORCE_COPY_IN_RELEASE
  delete released;
}

TEST(GeneratedMessageReflectionTest, SetAllocatedOneofMessageOnArenaTest) {
  unittest::TestOneof2 from_message1;
  unittest::TestOneof2 from_message2;
  Arena arena;
  unittest::TestOneof2* to_message =
      Arena::CreateMessage<unittest::TestOneof2>(&arena);
  const Descriptor* descriptor = unittest::TestOneof2::descriptor();
  const Reflection* reflection = to_message->GetReflection();

  Message* released = reflection->ReleaseMessage(
      to_message, descriptor->FindFieldByName("foo_lazy_message"));
  EXPECT_TRUE(released == nullptr);
  released = reflection->ReleaseMessage(
      to_message, descriptor->FindFieldByName("foo_message"));
  EXPECT_TRUE(released == nullptr);

  TestUtil::ReflectionTester::SetOneofViaReflection(&from_message1);
  TestUtil::ReflectionTester::ExpectOneofSetViaReflection(from_message1);

  TestUtil::ReflectionTester::
      SetAllocatedOptionalMessageFieldsToMessageViaReflection(&from_message1,
                                                              to_message);
  const Message& sub_message = reflection->GetMessage(
      *to_message, descriptor->FindFieldByName("foo_lazy_message"));
  released = reflection->ReleaseMessage(
      to_message, descriptor->FindFieldByName("foo_lazy_message"));
  EXPECT_TRUE(released != nullptr);
  // Since sub_message is arena allocated, releasing it results in copying it
  // into new heap-allocated memory.
  EXPECT_NE(&sub_message, released);
  delete released;

  TestUtil::ReflectionTester::SetOneofViaReflection(&from_message2);

  reflection->MutableMessage(&from_message2,
                             descriptor->FindFieldByName("foo_message"));

  TestUtil::ReflectionTester::
      SetAllocatedOptionalMessageFieldsToMessageViaReflection(&from_message2,
                                                              to_message);

  const Message& sub_message2 = reflection->GetMessage(
      *to_message, descriptor->FindFieldByName("foo_message"));
  released = reflection->ReleaseMessage(
      to_message, descriptor->FindFieldByName("foo_message"));
  EXPECT_TRUE(released != nullptr);
  // Since sub_message2 is arena allocated, releasing it results in copying it
  // into new heap-allocated memory.
  EXPECT_NE(&sub_message2, released);
  delete released;
}

TEST(GeneratedMessageReflectionTest, ReleaseMessageTest) {
  unittest::TestAllTypes message;
  TestUtil::ReflectionTester reflection_tester(
      unittest::TestAllTypes::descriptor());

  // When nothing is set, we expect all released messages to be nullptr.
  reflection_tester.ExpectMessagesReleasedViaReflection(
      &message, TestUtil::ReflectionTester::IS_NULL);

  // After fields are set we should get non-nullptr releases.
  reflection_tester.SetAllFieldsViaReflection(&message);
  reflection_tester.ExpectMessagesReleasedViaReflection(
      &message, TestUtil::ReflectionTester::NOT_NULL);

  // After Clear() we may or may not get a message from ReleaseMessage().
  // This is implementation specific.
  reflection_tester.SetAllFieldsViaReflection(&message);
  message.Clear();
  reflection_tester.ExpectMessagesReleasedViaReflection(
      &message, TestUtil::ReflectionTester::CAN_BE_NULL);

  // Test a different code path for setting after releasing.
  TestUtil::SetAllFields(&message);
  TestUtil::ExpectAllFieldsSet(message);
}

TEST(GeneratedMessageReflectionTest, ReleaseExtensionMessageTest) {
  unittest::TestAllExtensions message;
  TestUtil::ReflectionTester reflection_tester(
      unittest::TestAllExtensions::descriptor());

  // When nothing is set, we expect all released messages to be nullptr.
  reflection_tester.ExpectMessagesReleasedViaReflection(
      &message, TestUtil::ReflectionTester::IS_NULL);

  // After fields are set we should get non-nullptr releases.
  reflection_tester.SetAllFieldsViaReflection(&message);
  reflection_tester.ExpectMessagesReleasedViaReflection(
      &message, TestUtil::ReflectionTester::NOT_NULL);

  // After Clear() we may or may not get a message from ReleaseMessage().
  // This is implementation specific.
  reflection_tester.SetAllFieldsViaReflection(&message);
  message.Clear();
  reflection_tester.ExpectMessagesReleasedViaReflection(
      &message, TestUtil::ReflectionTester::CAN_BE_NULL);

  // Test a different code path for setting after releasing.
  TestUtil::SetAllExtensions(&message);
  TestUtil::ExpectAllExtensionsSet(message);
}

TEST(GeneratedMessageReflectionTest, ReleaseOneofMessageTest) {
  unittest::TestOneof2 message;
  TestUtil::ReflectionTester::SetOneofViaReflection(&message);

  const Descriptor* descriptor = unittest::TestOneof2::descriptor();
  const Reflection* reflection = message.GetReflection();
  const Message& sub_message = reflection->GetMessage(
      message, descriptor->FindFieldByName("foo_lazy_message"));
  (void)sub_message;  // unused in somce configurations
  Message* released = reflection->ReleaseMessage(
      &message, descriptor->FindFieldByName("foo_lazy_message"));

  EXPECT_TRUE(released != nullptr);
#ifndef PROTOBUF_FORCE_COPY_IN_RELEASE
  EXPECT_EQ(&sub_message, released);
#endif  // !PROTOBUF_FORCE_COPY_IN_RELEASE
  delete released;

  released = reflection->ReleaseMessage(
      &message, descriptor->FindFieldByName("foo_lazy_message"));
  EXPECT_TRUE(released == nullptr);
}

TEST(GeneratedMessageReflectionTest, ArenaReleaseMessageTest) {
  Arena arena;
  unittest::TestAllTypes* message =
      Arena::CreateMessage<unittest::TestAllTypes>(&arena);
  TestUtil::ReflectionTester reflection_tester(
      unittest::TestAllTypes::descriptor());

  // When nothing is set, we expect all released messages to be nullptr.
  reflection_tester.ExpectMessagesReleasedViaReflection(
      message, TestUtil::ReflectionTester::IS_NULL);

  // After fields are set we should get non-nullptr releases.
  reflection_tester.SetAllFieldsViaReflection(message);
  reflection_tester.ExpectMessagesReleasedViaReflection(
      message, TestUtil::ReflectionTester::NOT_NULL);

  // After Clear() we may or may not get a message from ReleaseMessage().
  // This is implementation specific.
  reflection_tester.SetAllFieldsViaReflection(message);
  message->Clear();
  reflection_tester.ExpectMessagesReleasedViaReflection(
      message, TestUtil::ReflectionTester::CAN_BE_NULL);
}

TEST(GeneratedMessageReflectionTest, ArenaReleaseExtensionMessageTest) {
  Arena arena;
  unittest::TestAllExtensions* message =
      Arena::CreateMessage<unittest::TestAllExtensions>(&arena);
  TestUtil::ReflectionTester reflection_tester(
      unittest::TestAllExtensions::descriptor());

  // When nothing is set, we expect all released messages to be nullptr.
  reflection_tester.ExpectMessagesReleasedViaReflection(
      message, TestUtil::ReflectionTester::IS_NULL);

  // After fields are set we should get non-nullptr releases.
  reflection_tester.SetAllFieldsViaReflection(message);
  reflection_tester.ExpectMessagesReleasedViaReflection(
      message, TestUtil::ReflectionTester::NOT_NULL);

  // After Clear() we may or may not get a message from ReleaseMessage().
  // This is implementation specific.
  reflection_tester.SetAllFieldsViaReflection(message);
  message->Clear();
  reflection_tester.ExpectMessagesReleasedViaReflection(
      message, TestUtil::ReflectionTester::CAN_BE_NULL);
}

TEST(GeneratedMessageReflectionTest, ArenaReleaseOneofMessageTest) {
  Arena arena;
  unittest::TestOneof2* message =
      Arena::CreateMessage<unittest::TestOneof2>(&arena);
  TestUtil::ReflectionTester::SetOneofViaReflection(message);

  const Descriptor* descriptor = unittest::TestOneof2::descriptor();
  const Reflection* reflection = message->GetReflection();
  Message* released = reflection->ReleaseMessage(
      message, descriptor->FindFieldByName("foo_lazy_message"));

  EXPECT_TRUE(released != nullptr);
  delete released;

  released = reflection->ReleaseMessage(
      message, descriptor->FindFieldByName("foo_lazy_message"));
  EXPECT_TRUE(released == nullptr);
}

#if GTEST_HAS_DEATH_TEST

TEST(GeneratedMessageReflectionTest, UsageErrors) {
  unittest::TestAllTypes message;
  unittest::ForeignMessage foreign;
  const Reflection* reflection = message.GetReflection();
  const Descriptor* descriptor = message.GetDescriptor();

  // Testing every single failure mode would be too much work.  Let's just
  // check a few.
  EXPECT_DEATH(
      reflection->GetInt32(message,
                           descriptor->FindFieldByName("optional_int64")),
      "Protocol Buffer reflection usage error:\n"
      "  Method      : google::protobuf::Reflection::GetInt32\n"
      "  Message type: protobuf_unittest\\.TestAllTypes\n"
      "  Field       : protobuf_unittest\\.TestAllTypes\\.optional_int64\n"
      "  Problem     : Field is not the right type for this message:\n"
      "    Expected  : CPPTYPE_INT32\n"
      "    Field type: CPPTYPE_INT64");
  EXPECT_DEATH(reflection->GetInt32(
                   message, descriptor->FindFieldByName("repeated_int32")),
               "Protocol Buffer reflection usage error:\n"
               "  Method      : google::protobuf::Reflection::GetInt32\n"
               "  Message type: protobuf_unittest.TestAllTypes\n"
               "  Field       : protobuf_unittest.TestAllTypes.repeated_int32\n"
               "  Problem     : Field is repeated; the method requires a "
               "singular field.");
#ifndef NDEBUG
  EXPECT_DEATH(
      reflection->GetInt32(foreign,
                           descriptor->FindFieldByName("optional_int32")),
      "Protocol Buffer reflection usage error:\n"
      "  Method       : google::protobuf::Reflection::GetInt32\n"
      "  Expected type: protobuf_unittest.TestAllTypes\n"
      "  Actual type  : protobuf_unittest.ForeignMessage\n"
      "  Field        : protobuf_unittest.TestAllTypes.optional_int32\n"
      "  Problem      : Message is not the right object for reflection");
#endif
  EXPECT_DEATH(
      reflection->GetInt32(
          message,
          unittest::ForeignMessage::descriptor()->FindFieldByName("c")),
      "Protocol Buffer reflection usage error:\n"
      "  Method      : google::protobuf::Reflection::GetInt32\n"
      "  Message type: protobuf_unittest.TestAllTypes\n"
      "  Field       : protobuf_unittest.ForeignMessage.c\n"
      "  Problem     : Field does not match message type.");
  EXPECT_DEATH(
      reflection->HasField(
          message,
          unittest::ForeignMessage::descriptor()->FindFieldByName("c")),
      "Protocol Buffer reflection usage error:\n"
      "  Method      : google::protobuf::Reflection::HasField\n"
      "  Message type: protobuf_unittest.TestAllTypes\n"
      "  Field       : protobuf_unittest.ForeignMessage.c\n"
      "  Problem     : Field does not match message type.");
}

#endif  // GTEST_HAS_DEATH_TEST

class GeneratedMessageReflectionCordAccessorsTest : public testing::Test {
 protected:
  const FieldDescriptor* optional_string_;
  const FieldDescriptor* optional_string_piece_;
  const FieldDescriptor* optional_cord_;
  const FieldDescriptor* repeated_string_;
  const FieldDescriptor* repeated_string_piece_;
  const FieldDescriptor* repeated_cord_;
  const FieldDescriptor* default_string_;
  const FieldDescriptor* default_string_piece_;
  const FieldDescriptor* default_cord_;

  const FieldDescriptor* optional_string_extension_;
  const FieldDescriptor* repeated_string_extension_;

  unittest::TestAllTypes message_;
  const Reflection* reflection_;
  unittest::TestAllExtensions extensions_message_;
  const Reflection* extensions_reflection_;

  void SetUp() override {
    const Descriptor* descriptor = unittest::TestAllTypes::descriptor();

    optional_string_ = descriptor->FindFieldByName("optional_string");
    optional_string_piece_ =
        descriptor->FindFieldByName("optional_string_piece");
    optional_cord_ = descriptor->FindFieldByName("optional_cord");
    repeated_string_ = descriptor->FindFieldByName("repeated_string");
    repeated_string_piece_ =
        descriptor->FindFieldByName("repeated_string_piece");
    repeated_cord_ = descriptor->FindFieldByName("repeated_cord");
    default_string_ = descriptor->FindFieldByName("default_string");
    default_string_piece_ = descriptor->FindFieldByName("default_string_piece");
    default_cord_ = descriptor->FindFieldByName("default_cord");

    optional_string_extension_ =
        descriptor->file()->FindExtensionByName("optional_string_extension");
    repeated_string_extension_ =
        descriptor->file()->FindExtensionByName("repeated_string_extension");

    ASSERT_TRUE(optional_string_ != nullptr);
    ASSERT_TRUE(optional_string_piece_ != nullptr);
    ASSERT_TRUE(optional_cord_ != nullptr);
    ASSERT_TRUE(repeated_string_ != nullptr);
    ASSERT_TRUE(repeated_string_piece_ != nullptr);
    ASSERT_TRUE(repeated_cord_ != nullptr);
    ASSERT_TRUE(optional_string_extension_ != nullptr);
    ASSERT_TRUE(repeated_string_extension_ != nullptr);

    reflection_ = message_.GetReflection();
    extensions_reflection_ = extensions_message_.GetReflection();
  }
};

TEST_F(GeneratedMessageReflectionCordAccessorsTest, GetCord) {
  message_.set_optional_string("foo");

  extensions_message_.SetExtension(unittest::optional_string_extension, "moo");

  EXPECT_EQ("foo", reflection_->GetCord(message_, optional_string_));
  EXPECT_EQ("moo", extensions_reflection_->GetCord(extensions_message_,
                                                   optional_string_extension_));

  EXPECT_EQ("hello", reflection_->GetCord(message_, default_string_));
  EXPECT_EQ("abc", reflection_->GetCord(message_, default_string_piece_));
  EXPECT_EQ("123", reflection_->GetCord(message_, default_cord_));


  unittest::TestCord message;
  const Descriptor* descriptor = unittest::TestCord::descriptor();
  const Reflection* reflection = message.GetReflection();

  message.set_optional_bytes_cord("bytes_cord");
  EXPECT_EQ("bytes_cord",
            reflection->GetCord(
                message, descriptor->FindFieldByName("optional_bytes_cord")));
}

TEST_F(GeneratedMessageReflectionCordAccessorsTest, GetOneofCord) {
  unittest::TestOneof2 message;
  const Descriptor* descriptor = unittest::TestOneof2::descriptor();
  const Reflection* reflection = message.GetReflection();

  message.set_foo_bytes_cord("bytes_cord");
  EXPECT_EQ("bytes_cord",
            reflection->GetCord(message,
                                descriptor->FindFieldByName("foo_bytes_cord")));

  message.set_foo_string("foo");
  EXPECT_EQ("foo", reflection->GetCord(
                       message, descriptor->FindFieldByName("foo_string")));

  message.set_foo_bytes("bytes");
  EXPECT_EQ("bytes", reflection->GetCord(
                         message, descriptor->FindFieldByName("foo_bytes")));

}

TEST_F(GeneratedMessageReflectionCordAccessorsTest, SetStringFromCord) {
  reflection_->SetString(&message_, optional_string_, absl::Cord("foo"));
  reflection_->SetString(&message_, optional_string_piece_, absl::Cord("bar"));
  reflection_->SetString(&message_, optional_cord_, absl::Cord("baz"));
  extensions_reflection_->SetString(
      &extensions_message_, optional_string_extension_, absl::Cord("moo"));

  EXPECT_TRUE(message_.has_optional_string());
  EXPECT_TRUE(message_.has_optional_string_piece());
  EXPECT_TRUE(message_.has_optional_cord());
  EXPECT_TRUE(
      extensions_message_.HasExtension(unittest::optional_string_extension));

  EXPECT_EQ("foo", message_.optional_string());
  EXPECT_EQ("bar", std::string(
                       reflection_->GetCord(message_, optional_string_piece_)));
  EXPECT_EQ("baz", std::string(reflection_->GetCord(message_, optional_cord_)));
  EXPECT_EQ("moo", extensions_message_.GetExtension(
                       unittest::optional_string_extension));

  unittest::TestCord message;
  const Descriptor* descriptor = unittest::TestCord::descriptor();
  const Reflection* reflection = message.GetReflection();

  reflection->SetString(&message,
                        descriptor->FindFieldByName("optional_bytes_cord"),
                        absl::Cord("cord"));
  EXPECT_TRUE(message.has_optional_bytes_cord());
  EXPECT_EQ("cord", message.optional_bytes_cord());
}

TEST_F(GeneratedMessageReflectionCordAccessorsTest, SetOneofStringFromCord) {
  unittest::TestOneof2 message;
  const Descriptor* descriptor = unittest::TestOneof2::descriptor();
  const Reflection* reflection = message.GetReflection();

  reflection->SetString(&message, descriptor->FindFieldByName("foo_string"),
                        absl::Cord("foo"));
  EXPECT_TRUE(message.has_foo_string());
  EXPECT_EQ("foo", message.foo_string());

  reflection->SetString(&message, descriptor->FindFieldByName("foo_bytes"),
                        absl::Cord("bytes"));
  EXPECT_TRUE(message.has_foo_bytes());
  EXPECT_EQ("bytes", message.foo_bytes());

  reflection->SetString(&message, descriptor->FindFieldByName("foo_cord"),
                        absl::Cord("cord"));
  EXPECT_EQ("cord", std::string(reflection->GetCord(
                        message, descriptor->FindFieldByName("foo_cord"))));

  reflection->SetString(&message,
                        descriptor->FindFieldByName("foo_string_piece"),
                        absl::Cord("string_piece"));
  EXPECT_EQ("string_piece",
            reflection->GetCord(
                message, descriptor->FindFieldByName("foo_string_piece")));

  reflection->SetString(&message, descriptor->FindFieldByName("foo_bytes_cord"),
                        absl::Cord("bytes_cord"));
  EXPECT_TRUE(message.has_foo_bytes_cord());
  EXPECT_EQ("bytes_cord", message.foo_bytes_cord());
}

TEST_F(GeneratedMessageReflectionCordAccessorsTest, CordSingularBytes) {
  unittest::TestCord message;
  absl::Cord cord_value("test");
  message.set_optional_bytes_cord(cord_value);
  EXPECT_EQ("test", message.optional_bytes_cord());

  EXPECT_TRUE(message.has_optional_bytes_cord());
  message.clear_optional_bytes_cord();
  EXPECT_FALSE(message.has_optional_bytes_cord());

  std::string string_value = "test";
  message.set_optional_bytes_cord(string_value);
  EXPECT_EQ("test", message.optional_bytes_cord());
}

TEST_F(GeneratedMessageReflectionCordAccessorsTest, CordSingularBytesDefault) {
  unittest::TestCord message;
  EXPECT_EQ("hello", message.optional_bytes_cord_default());
  absl::Cord cord_value("world");
  message.set_optional_bytes_cord_default(cord_value);
  EXPECT_EQ("world", message.optional_bytes_cord_default());
  message.clear_optional_bytes_cord_default();
  EXPECT_EQ("hello", message.optional_bytes_cord_default());
}

TEST_F(GeneratedMessageReflectionCordAccessorsTest, CordSingularOneofBytes) {
  unittest::TestOneof2 message;
  absl::Cord cord_value("test");
  message.set_foo_bytes_cord(cord_value);
  EXPECT_EQ("test", message.foo_bytes_cord());

  EXPECT_TRUE(message.has_foo_bytes_cord());
  message.clear_foo();
  EXPECT_FALSE(message.has_foo_bytes_cord());

  std::string string_value = "test";
  message.set_foo_bytes_cord(string_value);
  EXPECT_EQ("test", message.foo_bytes_cord());
  EXPECT_TRUE(message.has_foo_bytes_cord());
}

TEST_F(GeneratedMessageReflectionCordAccessorsTest, ClearOneofCord) {
  unittest::TestOneof2 message;
  absl::Cord cord_value("test");
  message.set_foo_bytes_cord(cord_value);

  const Descriptor* descriptor = unittest::TestOneof2::descriptor();
  const Reflection* reflection = message.GetReflection();

  EXPECT_TRUE(message.has_foo_bytes_cord());
  reflection->ClearOneof(&message, descriptor->FindOneofByName("foo"));
  EXPECT_FALSE(message.has_foo_bytes_cord());
}


using internal::IsDescendant;

TEST(GeneratedMessageReflection, IsDescendantMessage) {
  unittest::TestAllTypes msg1, msg2;
  TestUtil::SetAllFields(&msg1);
  msg2 = msg1;

  EXPECT_TRUE(IsDescendant(msg1, msg1.optional_nested_message()));
  EXPECT_TRUE(IsDescendant(msg1, msg1.repeated_foreign_message(0)));

  EXPECT_FALSE(IsDescendant(msg1, msg2.optional_nested_message()));
  EXPECT_FALSE(IsDescendant(msg1, msg2.repeated_foreign_message(0)));
}

TEST(GeneratedMessageReflection, IsDescendantMap) {
  unittest::TestMap msg1, msg2;
  (*msg1.mutable_map_int32_foreign_message())[0].set_c(100);
  TestUtil::SetAllFields(&(*msg1.mutable_map_int32_all_types())[0]);
  msg2 = msg1;

  EXPECT_TRUE(IsDescendant(msg1, msg1.map_int32_foreign_message().at(0)));
  EXPECT_TRUE(IsDescendant(msg1, msg1.map_int32_all_types().at(0)));

  EXPECT_FALSE(IsDescendant(msg1, msg2.map_int32_foreign_message().at(0)));
  EXPECT_FALSE(IsDescendant(msg1, msg2.map_int32_all_types().at(0)));
}

TEST(GeneratedMessageReflection, IsDescendantExtension) {
  unittest::TestAllExtensions msg1, msg2;
  TestUtil::SetAllExtensions(&msg1);
  msg2 = msg1;

  EXPECT_TRUE(IsDescendant(
      msg1, msg1.GetExtension(unittest::optional_nested_message_extension)));
  EXPECT_TRUE(IsDescendant(
      msg1,
      msg1.GetExtension(unittest::repeated_foreign_message_extension, 0)));

  EXPECT_FALSE(IsDescendant(
      msg1, msg2.GetExtension(unittest::optional_nested_message_extension)));
  EXPECT_FALSE(IsDescendant(
      msg1,
      msg2.GetExtension(unittest::repeated_foreign_message_extension, 0)));
}

TEST(GeneratedMessageReflection, IsDescendantOneof) {
  unittest::TestOneof msg1, msg2;
  TestUtil::SetAllFields(msg1.mutable_foo_message());
  msg2 = msg1;

  EXPECT_TRUE(IsDescendant(msg1, msg1.foo_message().optional_nested_message()));
  EXPECT_TRUE(
      IsDescendant(msg1, msg1.foo_message().repeated_foreign_message(0)));

  EXPECT_FALSE(
      IsDescendant(msg1, msg2.foo_message().optional_nested_message()));
  EXPECT_FALSE(
      IsDescendant(msg1, msg2.foo_message().repeated_foreign_message(0)));
}

TEST(GeneratedMessageReflection, ListFieldsSorted) {
  unittest::TestFieldOrderings msg;
  const Reflection* reflection = msg.GetReflection();
  std::vector<const FieldDescriptor*> fields;
  msg.set_my_string("hello");             // tag 11
  msg.mutable_optional_nested_message();  // tag 200
  reflection->ListFields(msg, &fields);
  // No sorting, in order declaration.
  EXPECT_THAT(fields,
              ElementsAre(Pointee(Property(&FieldDescriptor::number, 11)),
                          Pointee(Property(&FieldDescriptor::number, 200))));
  msg.set_my_int(4242);  // tag 1
  reflection->ListFields(msg, &fields);
  // Sorting as fields are declared in order 11, 1, 200.
  EXPECT_THAT(fields,
              ElementsAre(Pointee(Property(&FieldDescriptor::number, 1)),
                          Pointee(Property(&FieldDescriptor::number, 11)),
                          Pointee(Property(&FieldDescriptor::number, 200))));
  msg.clear_optional_nested_message();  // tag 200
  msg.SetExtension(unittest::my_extension_int,
                   424242);  // tag 5 from extension
  reflection->ListFields(msg, &fields);
  // Sorting as extension tag is in between.
  EXPECT_THAT(fields,
              ElementsAre(Pointee(Property(&FieldDescriptor::number, 1)),
                          Pointee(Property(&FieldDescriptor::number, 5)),
                          Pointee(Property(&FieldDescriptor::number, 11))));
  msg.clear_my_string();  // tag 11.
  reflection->ListFields(msg, &fields);
  // No sorting as extension is bigger than tag 1.
  EXPECT_THAT(fields,
              ElementsAre(Pointee(Property(&FieldDescriptor::number, 1)),
                          Pointee(Property(&FieldDescriptor::number, 5))));
  msg.set_my_float(1.0);  // tag 101
  msg.SetExtension(unittest::my_extension_string,
                   "hello");  // tag 50 from extension
  reflection->ListFields(msg, &fields);
  // Sorting of all as extensions are out of order and fields are in between.
  EXPECT_THAT(fields,
              ElementsAre(Pointee(Property(&FieldDescriptor::number, 1)),
                          Pointee(Property(&FieldDescriptor::number, 5)),
                          Pointee(Property(&FieldDescriptor::number, 50)),
                          Pointee(Property(&FieldDescriptor::number, 101))));
}

}  // namespace
}  // namespace protobuf
}  // namespace google
