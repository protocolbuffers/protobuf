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
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/testing/googletest.h>
#include <gtest/gtest.h>
#include <google/protobuf/stubs/stl_util-inl.h>

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

  // Make sure setting stuff again after clearing works.  (This takes slightly
  // different code paths since the objects are reused.)
  TestUtil::SetAllExtensions(&message);
  TestUtil::ExpectAllExtensionsSet(message);
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

TEST(ExtensionSetTest, CopyFromUpcasted) {
  unittest::TestAllExtensions message1, message2;
  string data;
  const Message& upcasted_message = message1;

  TestUtil::SetAllExtensions(&message1);
  message2.CopyFrom(upcasted_message);
  TestUtil::ExpectAllExtensionsSet(message2);
}

TEST(ExtensionSetTest, SwapWithEmpty) {
  unittest::TestAllExtensions message1, message2;
  TestUtil::SetAllExtensions(&message1);

  TestUtil::ExpectAllExtensionsSet(message1);
  TestUtil::ExpectExtensionsClear(message2);
  message1.Swap(&message2);
  TestUtil::ExpectAllExtensionsSet(message2);
  TestUtil::ExpectExtensionsClear(message1);
}

TEST(ExtensionSetTest, SwapWithSelf) {
  unittest::TestAllExtensions message;
  TestUtil::SetAllExtensions(&message);

  TestUtil::ExpectAllExtensionsSet(message);
  message.Swap(&message);
  TestUtil::ExpectAllExtensionsSet(message);
}

TEST(ExtensionSetTest, SerializationToArray) {
  // Serialize as TestAllExtensions and parse as TestAllTypes to insure wire
  // compatibility of extensions.
  //
  // This checks serialization to a flat array by explicitly reserving space in
  // the string and calling the generated message's
  // SerializeWithCachedSizesToArray.
  unittest::TestAllExtensions source;
  unittest::TestAllTypes destination;
  TestUtil::SetAllExtensions(&source);
  int size = source.ByteSize();
  string data;
  data.resize(size);
  uint8* target = reinterpret_cast<uint8*>(string_as_array(&data));
  uint8* end = source.SerializeWithCachedSizesToArray(target);
  EXPECT_EQ(size, end - target);
  EXPECT_TRUE(destination.ParseFromString(data));
  TestUtil::ExpectAllFieldsSet(destination);
}

TEST(ExtensionSetTest, SerializationToStream) {
  // Serialize as TestAllExtensions and parse as TestAllTypes to insure wire
  // compatibility of extensions.
  //
  // This checks serialization to an output stream by creating an array output
  // stream that can only buffer 1 byte at a time - this prevents the message
  // from ever jumping to the fast path, ensuring that serialization happens via
  // the CodedOutputStream.
  unittest::TestAllExtensions source;
  unittest::TestAllTypes destination;
  TestUtil::SetAllExtensions(&source);
  int size = source.ByteSize();
  string data;
  data.resize(size);
  {
    io::ArrayOutputStream array_stream(string_as_array(&data), size, 1);
    io::CodedOutputStream output_stream(&array_stream);
    source.SerializeWithCachedSizes(&output_stream);
    ASSERT_FALSE(output_stream.HadError());
  }
  EXPECT_TRUE(destination.ParseFromString(data));
  TestUtil::ExpectAllFieldsSet(destination);
}

TEST(ExtensionSetTest, PackedSerializationToArray) {
  // Serialize as TestPackedExtensions and parse as TestPackedTypes to insure
  // wire compatibility of extensions.
  //
  // This checks serialization to a flat array by explicitly reserving space in
  // the string and calling the generated message's
  // SerializeWithCachedSizesToArray.
  unittest::TestPackedExtensions source;
  unittest::TestPackedTypes destination;
  TestUtil::SetPackedExtensions(&source);
  int size = source.ByteSize();
  string data;
  data.resize(size);
  uint8* target = reinterpret_cast<uint8*>(string_as_array(&data));
  uint8* end = source.SerializeWithCachedSizesToArray(target);
  EXPECT_EQ(size, end - target);
  EXPECT_TRUE(destination.ParseFromString(data));
  TestUtil::ExpectPackedFieldsSet(destination);
}

TEST(ExtensionSetTest, PackedSerializationToStream) {
  // Serialize as TestPackedExtensions and parse as TestPackedTypes to insure
  // wire compatibility of extensions.
  //
  // This checks serialization to an output stream by creating an array output
  // stream that can only buffer 1 byte at a time - this prevents the message
  // from ever jumping to the fast path, ensuring that serialization happens via
  // the CodedOutputStream.
  unittest::TestPackedExtensions source;
  unittest::TestPackedTypes destination;
  TestUtil::SetPackedExtensions(&source);
  int size = source.ByteSize();
  string data;
  data.resize(size);
  {
    io::ArrayOutputStream array_stream(string_as_array(&data), size, 1);
    io::CodedOutputStream output_stream(&array_stream);
    source.SerializeWithCachedSizes(&output_stream);
    ASSERT_FALSE(output_stream.HadError());
  }
  EXPECT_TRUE(destination.ParseFromString(data));
  TestUtil::ExpectPackedFieldsSet(destination);
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

TEST(ExtensionSetTest, PackedParsing) {
  // Serialize as TestPackedTypes and parse as TestPackedExtensions.
  unittest::TestPackedTypes source;
  unittest::TestPackedExtensions destination;
  string data;

  TestUtil::SetPackedFields(&source);
  source.SerializeToString(&data);
  EXPECT_TRUE(destination.ParseFromString(data));
  TestUtil::ExpectPackedExtensionsSet(destination);
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

TEST(ExtensionSetTest, SpaceUsedExcludingSelf) {
  // Scalar primitive extensions should increase the extension set size by a
  // minimum of the size of the primitive type.
#define TEST_SCALAR_EXTENSIONS_SPACE_USED(type, value)                        \
  do {                                                                        \
    unittest::TestAllExtensions message;                                      \
    const int base_size = message.SpaceUsed();                                \
    message.SetExtension(unittest::optional_##type##_extension, value);       \
    int min_expected_size = base_size +                                       \
        sizeof(message.GetExtension(unittest::optional_##type##_extension));  \
    EXPECT_LE(min_expected_size, message.SpaceUsed());                        \
  } while (0)

  TEST_SCALAR_EXTENSIONS_SPACE_USED(int32   , 101);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(int64   , 102);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(uint32  , 103);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(uint64  , 104);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(sint32  , 105);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(sint64  , 106);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(fixed32 , 107);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(fixed64 , 108);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(sfixed32, 109);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(sfixed64, 110);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(float   , 111);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(double  , 112);
  TEST_SCALAR_EXTENSIONS_SPACE_USED(bool    , true);
#undef TEST_SCALAR_EXTENSIONS_SPACE_USED
  {
    unittest::TestAllExtensions message;
    const int base_size = message.SpaceUsed();
    message.SetExtension(unittest::optional_nested_enum_extension,
                         unittest::TestAllTypes::FOO);
    int min_expected_size = base_size +
        sizeof(message.GetExtension(unittest::optional_nested_enum_extension));
    EXPECT_LE(min_expected_size, message.SpaceUsed());
  }
  {
    // Strings may cause extra allocations depending on their length; ensure
    // that gets included as well.
    unittest::TestAllExtensions message;
    const int base_size = message.SpaceUsed();
    const string s("this is a fairly large string that will cause some "
                   "allocation in order to store it in the extension");
    message.SetExtension(unittest::optional_string_extension, s);
    int min_expected_size = base_size + s.length();
    EXPECT_LE(min_expected_size, message.SpaceUsed());
  }
  {
    // Messages also have additional allocation that need to be counted.
    unittest::TestAllExtensions message;
    const int base_size = message.SpaceUsed();
    unittest::ForeignMessage foreign;
    foreign.set_c(42);
    message.MutableExtension(unittest::optional_foreign_message_extension)->
        CopyFrom(foreign);
    int min_expected_size = base_size + foreign.SpaceUsed();
    EXPECT_LE(min_expected_size, message.SpaceUsed());
  }

  // Repeated primitive extensions will increase space used by at least a
  // RepeatedField<T>, and will cause additional allocations when the array
  // gets too big for the initial space.
  // This macro:
  //   - Adds a value to the repeated extension, then clears it, establishing
  //     the base size.
  //   - Adds a small number of values, testing that it doesn't increase the
  //     SpaceUsed()
  //   - Adds a large number of values (requiring allocation in the repeated
  //     field), and ensures that that allocation is included in SpaceUsed()
#define TEST_REPEATED_EXTENSIONS_SPACE_USED(type, cpptype, value)              \
  do {                                                                         \
    unittest::TestAllExtensions message;                                       \
    const int base_size = message.SpaceUsed();                                 \
    int min_expected_size = sizeof(RepeatedField<cpptype>) + base_size;        \
    message.AddExtension(unittest::repeated_##type##_extension, value);        \
    message.ClearExtension(unittest::repeated_##type##_extension);             \
    const int empty_repeated_field_size = message.SpaceUsed();                 \
    EXPECT_LE(min_expected_size, empty_repeated_field_size) << #type;          \
    message.AddExtension(unittest::repeated_##type##_extension, value);        \
    message.AddExtension(unittest::repeated_##type##_extension, value);        \
    EXPECT_EQ(empty_repeated_field_size, message.SpaceUsed()) << #type;        \
    message.ClearExtension(unittest::repeated_##type##_extension);             \
    for (int i = 0; i < 16; ++i) {                                             \
      message.AddExtension(unittest::repeated_##type##_extension, value);      \
    }                                                                          \
    int expected_size = sizeof(cpptype) * 16 + empty_repeated_field_size;      \
    EXPECT_EQ(expected_size, message.SpaceUsed()) << #type;                    \
  } while (0)

  TEST_REPEATED_EXTENSIONS_SPACE_USED(int32   , int32 , 101);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(int64   , int64 , 102);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(uint32  , uint32, 103);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(uint64  , uint64, 104);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(sint32  , int32 , 105);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(sint64  , int64 , 106);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(fixed32 , uint32, 107);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(fixed64 , uint64, 108);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(sfixed32, int32 , 109);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(sfixed64, int64 , 110);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(float   , float , 111);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(double  , double, 112);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(bool    , bool  , true);
  TEST_REPEATED_EXTENSIONS_SPACE_USED(nested_enum, int,
                                      unittest::TestAllTypes::FOO);
#undef TEST_REPEATED_EXTENSIONS_SPACE_USED
  // Repeated strings
  {
    unittest::TestAllExtensions message;
    const int base_size = message.SpaceUsed();
    int min_expected_size = sizeof(RepeatedPtrField<string>) + base_size;
    const string value(256, 'x');
    // Once items are allocated, they may stick around even when cleared so
    // without the hardcore memory management accessors there isn't a notion of
    // the empty repeated field memory usage as there is with primitive types.
    for (int i = 0; i < 16; ++i) {
      message.AddExtension(unittest::repeated_string_extension, value);
    }
    min_expected_size += (sizeof(value) + value.size()) * 16;
    EXPECT_LE(min_expected_size, message.SpaceUsed());
  }
  // Repeated messages
  {
    unittest::TestAllExtensions message;
    const int base_size = message.SpaceUsed();
    int min_expected_size = sizeof(RepeatedPtrField<unittest::ForeignMessage>) +
        base_size;
    unittest::ForeignMessage prototype;
    prototype.set_c(2);
    for (int i = 0; i < 16; ++i) {
      message.AddExtension(unittest::repeated_foreign_message_extension)->
          CopyFrom(prototype);
    }
    min_expected_size += 16 * prototype.SpaceUsed();
    EXPECT_LE(min_expected_size, message.SpaceUsed());
  }
}

TEST(ExtensionSetTest, InvalidEnumDeath) {
  unittest::TestAllExtensions message;
  EXPECT_DEBUG_DEATH(
    message.SetExtension(unittest::optional_foreign_enum_extension,
                         static_cast<unittest::ForeignEnum>(53)),
    "IsValid");
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
