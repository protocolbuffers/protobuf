// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/test_util.h>
#include <google/protobuf/unittest.pb.h>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/testing/googletest.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {

namespace {

// Shorthand to get a FieldDescriptor for a field of unittest::TestAllTypes.
const FieldDescriptor* F(const string& name) {
  const FieldDescriptor* result =
    unittest::TestAllTypes::descriptor()->FindFieldByName(name);
  GOOGLE_CHECK(result != NULL);
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
  // Test that GetStringReference() returns the underlying string when it is
  // a normal string field.
  unittest::TestAllTypes message;
  message.set_optional_string("foo");
  message.add_repeated_string("foo");

  const Reflection* reflection = message.GetReflection();
  string scratch;

  EXPECT_EQ(&message.optional_string(),
      &reflection->GetStringReference(message, F("optional_string"), &scratch))
    << "For simple string fields, GetStringReference() should return a "
       "reference to the underlying string.";
  EXPECT_EQ(&message.repeated_string(0),
      &reflection->GetRepeatedStringReference(message, F("repeated_string"),
                                              0, &scratch))
    << "For simple string fields, GetRepeatedStringReference() should return "
       "a reference to the underlying string.";
}


TEST(GeneratedMessageReflectionTest, DefaultsAfterClear) {
  // Check that after setting all fields and then clearing, getting an
  // embedded message does NOT return the default instance.
  unittest::TestAllTypes message;
  TestUtil::ReflectionTester reflection_tester(
    unittest::TestAllTypes::descriptor());

  TestUtil::SetAllFields(&message);
  message.Clear();

  const Reflection* reflection = message.GetReflection();

  EXPECT_NE(&unittest::TestAllTypes::OptionalGroup::default_instance(),
            &reflection->GetMessage(message, F("optionalgroup")));
  EXPECT_NE(&unittest::TestAllTypes::NestedMessage::default_instance(),
            &reflection->GetMessage(message, F("optional_nested_message")));
  EXPECT_NE(&unittest::ForeignMessage::default_instance(),
            &reflection->GetMessage(message, F("optional_foreign_message")));
  EXPECT_NE(&unittest_import::ImportMessage::default_instance(),
            &reflection->GetMessage(message, F("optional_import_message")));
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
  EXPECT_TRUE(reflection->FindKnownExtensionByNumber(62341) == NULL);

  // Extensions of TestAllExtensions should not show up as extensions of
  // other types.
  EXPECT_TRUE(unittest::TestAllTypes::default_instance().GetReflection()->
              FindKnownExtensionByNumber(extension1->number()) == NULL);
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
  EXPECT_TRUE(reflection->FindKnownExtensionByName("no_such_ext") == NULL);

  // Extensions of TestAllExtensions should not show up as extensions of
  // other types.
  EXPECT_TRUE(unittest::TestAllTypes::default_instance().GetReflection()->
              FindKnownExtensionByName(extension1->full_name()) == NULL);
}

#ifdef GTEST_HAS_DEATH_TEST

TEST(GeneratedMessageReflectionTest, UsageErrors) {
  unittest::TestAllTypes message;
  const Reflection* reflection = message.GetReflection();
  const Descriptor* descriptor = message.GetDescriptor();

#define f(NAME) descriptor->FindFieldByName(NAME)

  // Testing every single failure mode would be too much work.  Let's just
  // check a few.
  EXPECT_DEATH(
    reflection->GetInt32(
      message, descriptor->FindFieldByName("optional_int64")),
    "Protocol Buffer reflection usage error:\n"
    "  Method      : google::protobuf::Reflection::GetInt32\n"
    "  Message type: protobuf_unittest\\.TestAllTypes\n"
    "  Field       : protobuf_unittest\\.TestAllTypes\\.optional_int64\n"
    "  Problem     : Field is not the right type for this message:\n"
    "    Expected  : CPPTYPE_INT32\n"
    "    Field type: CPPTYPE_INT64");
  EXPECT_DEATH(
    reflection->GetInt32(
      message, descriptor->FindFieldByName("repeated_int32")),
    "Protocol Buffer reflection usage error:\n"
    "  Method      : google::protobuf::Reflection::GetInt32\n"
    "  Message type: protobuf_unittest.TestAllTypes\n"
    "  Field       : protobuf_unittest.TestAllTypes.repeated_int32\n"
    "  Problem     : Field is repeated; the method requires a singular field.");
  EXPECT_DEATH(
    reflection->GetInt32(
      message, unittest::ForeignMessage::descriptor()->FindFieldByName("c")),
    "Protocol Buffer reflection usage error:\n"
    "  Method      : google::protobuf::Reflection::GetInt32\n"
    "  Message type: protobuf_unittest.TestAllTypes\n"
    "  Field       : protobuf_unittest.ForeignMessage.c\n"
    "  Problem     : Field does not match message type.");
  EXPECT_DEATH(
    reflection->HasField(
      message, unittest::ForeignMessage::descriptor()->FindFieldByName("c")),
    "Protocol Buffer reflection usage error:\n"
    "  Method      : google::protobuf::Reflection::HasField\n"
    "  Message type: protobuf_unittest.TestAllTypes\n"
    "  Field       : protobuf_unittest.ForeignMessage.c\n"
    "  Problem     : Field does not match message type.");

#undef f
}

#endif  // GTEST_HAS_DEATH_TEST


}  // namespace
}  // namespace protobuf
}  // namespace google
