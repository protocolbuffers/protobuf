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

#include <google/protobuf/extension_set.h>
#include <google/protobuf/unittest.pb.h>
#include <google/protobuf/test_util.h>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/testing/googletest.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace internal {
namespace {

// This test closely mirrors google/protobuf/compiler/cpp/unittest.cc
// except that it uses extensions rather than regular fields.

TEST(ExtensionSetTest, Defaults) {
  // Check that all default values are set correctly in the initial message.
  unittest::TestAllExtensions message;

  TestUtil::ExpectExtensionsClear(message);

  // Messages should return pointers to default instances until first use.
  // (This is not checked by ExpectClear() since it is not actually true after
  // the fields have been set and then cleared.)
  EXPECT_EQ(&unittest::OptionalGroup_extension::default_instance(),
            &message.GetExtension(unittest::optionalgroup_extension));
  EXPECT_EQ(&unittest::TestAllTypes::NestedMessage::default_instance(),
            &message.GetExtension(unittest::optional_nested_message_extension));
  EXPECT_EQ(&unittest::ForeignMessage::default_instance(),
            &message.GetExtension(
              unittest::optional_foreign_message_extension));
  EXPECT_EQ(&unittest_import::ImportMessage::default_instance(),
            &message.GetExtension(unittest::optional_import_message_extension));
}

TEST(ExtensionSetTest, Accessors) {
  // Set every field to a unique value then go back and check all those
  // values.
  unittest::TestAllExtensions message;

  TestUtil::SetAllExtensions(&message);
  TestUtil::ExpectAllExtensionsSet(message);

  TestUtil::ModifyRepeatedExtensions(&message);
  TestUtil::ExpectRepeatedExtensionsModified(message);
}

TEST(ExtensionSetTest, Clear) {
  // Set every field to a unique value, clear the message, then check that
  // it is cleared.
  unittest::TestAllExtensions message;

  TestUtil::SetAllExtensions(&message);
  message.Clear();
  TestUtil::ExpectExtensionsClear(message);

  // Unlike with the defaults test, we do NOT expect that requesting embedded
  // messages will return a pointer to the default instance.  Instead, they
  // should return the objects that were created when mutable_blah() was
  // called.
  EXPECT_NE(&unittest::OptionalGroup_extension::default_instance(),
            &message.GetExtension(unittest::optionalgroup_extension));
  EXPECT_NE(&unittest::TestAllTypes::NestedMessage::default_instance(),
            &message.GetExtension(unittest::optional_nested_message_extension));
  EXPECT_NE(&unittest::ForeignMessage::default_instance(),
            &message.GetExtension(
              unittest::optional_foreign_message_extension));
  EXPECT_NE(&unittest_import::ImportMessage::default_instance(),
            &message.GetExtension(unittest::optional_import_message_extension));
}

TEST(ExtensionSetTest, ClearOneField) {
  // Set every field to a unique value, then clear one value and insure that
  // only that one value is cleared.
  unittest::TestAllExtensions message;

  TestUtil::SetAllExtensions(&message);
  int64 original_value =
    message.GetExtension(unittest::optional_int64_extension);

  // Clear the field and make sure it shows up as cleared.
  message.ClearExtension(unittest::optional_int64_extension);
  EXPECT_FALSE(message.HasExtension(unittest::optional_int64_extension));
  EXPECT_EQ(0, message.GetExtension(unittest::optional_int64_extension));

  // Other adjacent fields should not be cleared.
  EXPECT_TRUE(message.HasExtension(unittest::optional_int32_extension));
  EXPECT_TRUE(message.HasExtension(unittest::optional_uint32_extension));

  // Make sure if we set it again, then all fields are set.
  message.SetExtension(unittest::optional_int64_extension, original_value);
  TestUtil::ExpectAllExtensionsSet(message);
}

TEST(ExtensionSetTest, CopyFrom) {
  unittest::TestAllExtensions message1, message2;
  string data;

  TestUtil::SetAllExtensions(&message1);
  message2.CopyFrom(message1);
  TestUtil::ExpectAllExtensionsSet(message2);
}

TEST(ExtensionSetTest, Serialization) {
  // Serialize as TestAllExtensions and parse as TestAllTypes to insure wire
  // compatibility of extensions.
  unittest::TestAllExtensions source;
  unittest::TestAllTypes destination;
  string data;

  TestUtil::SetAllExtensions(&source);
  source.SerializeToString(&data);
  EXPECT_TRUE(destination.ParseFromString(data));
  TestUtil::ExpectAllFieldsSet(destination);
}

TEST(ExtensionSetTest, Parsing) {
  // Serialize as TestAllTypes and parse as TestAllExtensions.
  unittest::TestAllTypes source;
  unittest::TestAllExtensions destination;
  string data;

  TestUtil::SetAllFields(&source);
  source.SerializeToString(&data);
  EXPECT_TRUE(destination.ParseFromString(data));
  TestUtil::ExpectAllExtensionsSet(destination);
}

TEST(ExtensionSetTest, IsInitialized) {
  // Test that IsInitialized() returns false if required fields in nested
  // extensions are missing.
  unittest::TestAllExtensions message;

  EXPECT_TRUE(message.IsInitialized());

  message.MutableExtension(unittest::TestRequired::single);
  EXPECT_FALSE(message.IsInitialized());

  message.MutableExtension(unittest::TestRequired::single)->set_a(1);
  EXPECT_FALSE(message.IsInitialized());
  message.MutableExtension(unittest::TestRequired::single)->set_b(2);
  EXPECT_FALSE(message.IsInitialized());
  message.MutableExtension(unittest::TestRequired::single)->set_c(3);
  EXPECT_TRUE(message.IsInitialized());

  message.AddExtension(unittest::TestRequired::multi);
  EXPECT_FALSE(message.IsInitialized());

  message.MutableExtension(unittest::TestRequired::multi, 0)->set_a(1);
  EXPECT_FALSE(message.IsInitialized());
  message.MutableExtension(unittest::TestRequired::multi, 0)->set_b(2);
  EXPECT_FALSE(message.IsInitialized());
  message.MutableExtension(unittest::TestRequired::multi, 0)->set_c(3);
  EXPECT_TRUE(message.IsInitialized());
}

TEST(ExtensionSetTest, MutableString) {
  // Test the mutable string accessors.
  unittest::TestAllExtensions message;

  message.MutableExtension(unittest::optional_string_extension)->assign("foo");
  EXPECT_TRUE(message.HasExtension(unittest::optional_string_extension));
  EXPECT_EQ("foo", message.GetExtension(unittest::optional_string_extension));

  message.AddExtension(unittest::repeated_string_extension)->assign("bar");
  ASSERT_EQ(1, message.ExtensionSize(unittest::repeated_string_extension));
  EXPECT_EQ("bar",
            message.GetExtension(unittest::repeated_string_extension, 0));
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
