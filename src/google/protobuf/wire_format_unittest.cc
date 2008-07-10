// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <google/protobuf/wire_format.h>
#include <google/protobuf/wire_format_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/unittest.pb.h>
#include <google/protobuf/unittest_mset.pb.h>
#include <google/protobuf/test_util.h>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/testing/googletest.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace internal {
namespace {

TEST(WireFormatTest, MaxFieldNumber) {
  // Make sure the max field number constant is accurate.
  EXPECT_EQ((1 << (32 - WireFormat::kTagTypeBits)) - 1,
            FieldDescriptor::kMaxNumber);
}

TEST(WireFormatTest, Parse) {
  unittest::TestAllTypes source, dest;
  string data;

  // Serialize using the generated code.
  TestUtil::SetAllFields(&source);
  source.SerializeToString(&data);

  // Parse using WireFormat.
  io::ArrayInputStream raw_input(data.data(), data.size());
  io::CodedInputStream input(&raw_input);
  WireFormat::ParseAndMergePartial(unittest::TestAllTypes::descriptor(),
                                   &input, dest.GetReflection());

  // Check.
  TestUtil::ExpectAllFieldsSet(dest);
}

TEST(WireFormatTest, ParseExtensions) {
  unittest::TestAllExtensions source, dest;
  string data;

  // Serialize using the generated code.
  TestUtil::SetAllExtensions(&source);
  source.SerializeToString(&data);

  // Parse using WireFormat.
  io::ArrayInputStream raw_input(data.data(), data.size());
  io::CodedInputStream input(&raw_input);
  WireFormat::ParseAndMergePartial(unittest::TestAllExtensions::descriptor(),
                                   &input, dest.GetReflection());

  // Check.
  TestUtil::ExpectAllExtensionsSet(dest);
}

TEST(WireFormatTest, ByteSize) {
  unittest::TestAllTypes message;
  TestUtil::SetAllFields(&message);

  EXPECT_EQ(message.ByteSize(),
            WireFormat::ByteSize(unittest::TestAllTypes::descriptor(),
                                 message.GetReflection()));
  message.Clear();
  EXPECT_EQ(0, message.ByteSize());
  EXPECT_EQ(0, WireFormat::ByteSize(unittest::TestAllTypes::descriptor(),
                                    message.GetReflection()));
}

TEST(WireFormatTest, ByteSizeExtensions) {
  unittest::TestAllExtensions message;
  TestUtil::SetAllExtensions(&message);

  EXPECT_EQ(message.ByteSize(),
            WireFormat::ByteSize(unittest::TestAllExtensions::descriptor(),
                                 message.GetReflection()));
  message.Clear();
  EXPECT_EQ(0, message.ByteSize());
  EXPECT_EQ(0, WireFormat::ByteSize(unittest::TestAllExtensions::descriptor(),
                                    message.GetReflection()));
}

TEST(WireFormatTest, Serialize) {
  unittest::TestAllTypes message;
  string generated_data;
  string dynamic_data;

  TestUtil::SetAllFields(&message);
  int size = message.ByteSize();

  // Serialize using the generated code.
  {
    io::StringOutputStream raw_output(&generated_data);
    io::CodedOutputStream output(&raw_output);
    message.SerializeWithCachedSizes(&output);
  }

  // Serialize using WireFormat.
  {
    io::StringOutputStream raw_output(&dynamic_data);
    io::CodedOutputStream output(&raw_output);
    WireFormat::SerializeWithCachedSizes(
      unittest::TestAllTypes::descriptor(),
      message.GetReflection(), size, &output);
  }

  // Should be the same.
  // Don't use EXPECT_EQ here because we're comparing raw binary data and
  // we really don't want it dumped to stdout on failure.
  EXPECT_TRUE(dynamic_data == generated_data);
}

TEST(WireFormatTest, SerializeExtensions) {
  unittest::TestAllExtensions message;
  string generated_data;
  string dynamic_data;

  TestUtil::SetAllExtensions(&message);
  int size = message.ByteSize();

  // Serialize using the generated code.
  {
    io::StringOutputStream raw_output(&generated_data);
    io::CodedOutputStream output(&raw_output);
    message.SerializeWithCachedSizes(&output);
  }

  // Serialize using WireFormat.
  {
    io::StringOutputStream raw_output(&dynamic_data);
    io::CodedOutputStream output(&raw_output);
    WireFormat::SerializeWithCachedSizes(
      unittest::TestAllExtensions::descriptor(),
      message.GetReflection(), size, &output);
  }

  // Should be the same.
  // Don't use EXPECT_EQ here because we're comparing raw binary data and
  // we really don't want it dumped to stdout on failure.
  EXPECT_TRUE(dynamic_data == generated_data);
}

TEST(WireFormatTest, SerializeFieldsAndExtensions) {
  unittest::TestFieldOrderings message;
  string generated_data;
  string dynamic_data;

  TestUtil::SetAllFieldsAndExtensions(&message);
  int size = message.ByteSize();

  // Serialize using the generated code.
  {
    io::StringOutputStream raw_output(&generated_data);
    io::CodedOutputStream output(&raw_output);
    message.SerializeWithCachedSizes(&output);
  }

  // Serialize using WireFormat.
  {
    io::StringOutputStream raw_output(&dynamic_data);
    io::CodedOutputStream output(&raw_output);
    WireFormat::SerializeWithCachedSizes(
      unittest::TestFieldOrderings::descriptor(),
      message.GetReflection(), size, &output);
  }

  // Should be the same.
  // Don't use EXPECT_EQ here because we're comparing raw binary data and
  // we really don't want it dumped to stdout on failure.
  EXPECT_TRUE(dynamic_data == generated_data);

  // Should output in canonical order.
  TestUtil::ExpectAllFieldsAndExtensionsInOrder(dynamic_data);
  TestUtil::ExpectAllFieldsAndExtensionsInOrder(generated_data);
}

const int kUnknownTypeId = 1550055;

TEST(WireFormatTest, SerializeMessageSet) {
  // Set up a TestMessageSet with two known messages and an unknown one.
  unittest::TestMessageSet message_set;
  message_set.MutableExtension(
    unittest::TestMessageSetExtension1::message_set_extension)->set_i(123);
  message_set.MutableExtension(
    unittest::TestMessageSetExtension2::message_set_extension)->set_str("foo");
  message_set.mutable_unknown_fields()->AddField(kUnknownTypeId)
                                      ->add_length_delimited("bar");

  string data;
  ASSERT_TRUE(message_set.SerializeToString(&data));

  // Parse back using RawMessageSet and check the contents.
  unittest::RawMessageSet raw;
  ASSERT_TRUE(raw.ParseFromString(data));

  EXPECT_EQ(0, raw.unknown_fields().field_count());

  ASSERT_EQ(3, raw.item_size());
  EXPECT_EQ(
    unittest::TestMessageSetExtension1::descriptor()->extension(0)->number(),
    raw.item(0).type_id());
  EXPECT_EQ(
    unittest::TestMessageSetExtension2::descriptor()->extension(0)->number(),
    raw.item(1).type_id());
  EXPECT_EQ(kUnknownTypeId, raw.item(2).type_id());

  unittest::TestMessageSetExtension1 message1;
  EXPECT_TRUE(message1.ParseFromString(raw.item(0).message()));
  EXPECT_EQ(123, message1.i());

  unittest::TestMessageSetExtension2 message2;
  EXPECT_TRUE(message2.ParseFromString(raw.item(1).message()));
  EXPECT_EQ("foo", message2.str());

  EXPECT_EQ("bar", raw.item(2).message());
}

TEST(WireFormatTest, ParseMessageSet) {
  // Set up a RawMessageSet with two known messages and an unknown one.
  unittest::RawMessageSet raw;

  {
    unittest::RawMessageSet::Item* item = raw.add_item();
    item->set_type_id(
      unittest::TestMessageSetExtension1::descriptor()->extension(0)->number());
    unittest::TestMessageSetExtension1 message;
    message.set_i(123);
    message.SerializeToString(item->mutable_message());
  }

  {
    unittest::RawMessageSet::Item* item = raw.add_item();
    item->set_type_id(
      unittest::TestMessageSetExtension2::descriptor()->extension(0)->number());
    unittest::TestMessageSetExtension2 message;
    message.set_str("foo");
    message.SerializeToString(item->mutable_message());
  }

  {
    unittest::RawMessageSet::Item* item = raw.add_item();
    item->set_type_id(kUnknownTypeId);
    item->set_message("bar");
  }

  string data;
  ASSERT_TRUE(raw.SerializeToString(&data));

  // Parse as a TestMessageSet and check the contents.
  unittest::TestMessageSet message_set;
  ASSERT_TRUE(message_set.ParseFromString(data));

  EXPECT_EQ(123, message_set.GetExtension(
    unittest::TestMessageSetExtension1::message_set_extension).i());
  EXPECT_EQ("foo", message_set.GetExtension(
    unittest::TestMessageSetExtension2::message_set_extension).str());

  ASSERT_EQ(1, message_set.unknown_fields().field_count());
  ASSERT_EQ(1, message_set.unknown_fields().field(0).length_delimited_size());
  EXPECT_EQ("bar", message_set.unknown_fields().field(0).length_delimited(0));
}

TEST(WireFormatTest, RecursionLimit) {
  unittest::TestRecursiveMessage message;
  message.mutable_a()->mutable_a()->mutable_a()->mutable_a()->set_i(1);
  string data;
  message.SerializeToString(&data);

  {
    io::ArrayInputStream raw_input(data.data(), data.size());
    io::CodedInputStream input(&raw_input);
    input.SetRecursionLimit(4);
    unittest::TestRecursiveMessage message2;
    EXPECT_TRUE(message2.ParseFromCodedStream(&input));
  }

  {
    io::ArrayInputStream raw_input(data.data(), data.size());
    io::CodedInputStream input(&raw_input);
    input.SetRecursionLimit(3);
    unittest::TestRecursiveMessage message2;
    EXPECT_FALSE(message2.ParseFromCodedStream(&input));
  }
}

TEST(WireFormatTest, UnknownFieldRecursionLimit) {
  unittest::TestEmptyMessage message;
  message.mutable_unknown_fields()
        ->AddField(1234)->add_group()
        ->AddField(1234)->add_group()
        ->AddField(1234)->add_group()
        ->AddField(1234)->add_group()
        ->AddField(1234)->add_varint(123);
  string data;
  message.SerializeToString(&data);

  {
    io::ArrayInputStream raw_input(data.data(), data.size());
    io::CodedInputStream input(&raw_input);
    input.SetRecursionLimit(4);
    unittest::TestEmptyMessage message2;
    EXPECT_TRUE(message2.ParseFromCodedStream(&input));
  }

  {
    io::ArrayInputStream raw_input(data.data(), data.size());
    io::CodedInputStream input(&raw_input);
    input.SetRecursionLimit(3);
    unittest::TestEmptyMessage message2;
    EXPECT_FALSE(message2.ParseFromCodedStream(&input));
  }
}

TEST(WireFormatTest, ZigZag) {
// avoid line-wrapping
#define LL(x) GOOGLE_LONGLONG(x)
#define ULL(x) GOOGLE_ULONGLONG(x)
#define ZigZagEncode32(x) WireFormat::ZigZagEncode32(x)
#define ZigZagDecode32(x) WireFormat::ZigZagDecode32(x)
#define ZigZagEncode64(x) WireFormat::ZigZagEncode64(x)
#define ZigZagDecode64(x) WireFormat::ZigZagDecode64(x)

  EXPECT_EQ(0u, ZigZagEncode32( 0));
  EXPECT_EQ(1u, ZigZagEncode32(-1));
  EXPECT_EQ(2u, ZigZagEncode32( 1));
  EXPECT_EQ(3u, ZigZagEncode32(-2));
  EXPECT_EQ(0x7FFFFFFEu, ZigZagEncode32(0x3FFFFFFF));
  EXPECT_EQ(0x7FFFFFFFu, ZigZagEncode32(0xC0000000));
  EXPECT_EQ(0xFFFFFFFEu, ZigZagEncode32(0x7FFFFFFF));
  EXPECT_EQ(0xFFFFFFFFu, ZigZagEncode32(0x80000000));

  EXPECT_EQ( 0, ZigZagDecode32(0u));
  EXPECT_EQ(-1, ZigZagDecode32(1u));
  EXPECT_EQ( 1, ZigZagDecode32(2u));
  EXPECT_EQ(-2, ZigZagDecode32(3u));
  EXPECT_EQ(0x3FFFFFFF, ZigZagDecode32(0x7FFFFFFEu));
  EXPECT_EQ(0xC0000000, ZigZagDecode32(0x7FFFFFFFu));
  EXPECT_EQ(0x7FFFFFFF, ZigZagDecode32(0xFFFFFFFEu));
  EXPECT_EQ(0x80000000, ZigZagDecode32(0xFFFFFFFFu));

  EXPECT_EQ(0u, ZigZagEncode64( 0));
  EXPECT_EQ(1u, ZigZagEncode64(-1));
  EXPECT_EQ(2u, ZigZagEncode64( 1));
  EXPECT_EQ(3u, ZigZagEncode64(-2));
  EXPECT_EQ(ULL(0x000000007FFFFFFE), ZigZagEncode64(LL(0x000000003FFFFFFF)));
  EXPECT_EQ(ULL(0x000000007FFFFFFF), ZigZagEncode64(LL(0xFFFFFFFFC0000000)));
  EXPECT_EQ(ULL(0x00000000FFFFFFFE), ZigZagEncode64(LL(0x000000007FFFFFFF)));
  EXPECT_EQ(ULL(0x00000000FFFFFFFF), ZigZagEncode64(LL(0xFFFFFFFF80000000)));
  EXPECT_EQ(ULL(0xFFFFFFFFFFFFFFFE), ZigZagEncode64(LL(0x7FFFFFFFFFFFFFFF)));
  EXPECT_EQ(ULL(0xFFFFFFFFFFFFFFFF), ZigZagEncode64(LL(0x8000000000000000)));

  EXPECT_EQ( 0, ZigZagDecode64(0u));
  EXPECT_EQ(-1, ZigZagDecode64(1u));
  EXPECT_EQ( 1, ZigZagDecode64(2u));
  EXPECT_EQ(-2, ZigZagDecode64(3u));
  EXPECT_EQ(LL(0x000000003FFFFFFF), ZigZagDecode64(ULL(0x000000007FFFFFFE)));
  EXPECT_EQ(LL(0xFFFFFFFFC0000000), ZigZagDecode64(ULL(0x000000007FFFFFFF)));
  EXPECT_EQ(LL(0x000000007FFFFFFF), ZigZagDecode64(ULL(0x00000000FFFFFFFE)));
  EXPECT_EQ(LL(0xFFFFFFFF80000000), ZigZagDecode64(ULL(0x00000000FFFFFFFF)));
  EXPECT_EQ(LL(0x7FFFFFFFFFFFFFFF), ZigZagDecode64(ULL(0xFFFFFFFFFFFFFFFE)));
  EXPECT_EQ(LL(0x8000000000000000), ZigZagDecode64(ULL(0xFFFFFFFFFFFFFFFF)));

  // Some easier-to-verify round-trip tests.  The inputs (other than 0, 1, -1)
  // were chosen semi-randomly via keyboard bashing.
  EXPECT_EQ(    0, ZigZagDecode32(ZigZagEncode32(    0)));
  EXPECT_EQ(    1, ZigZagDecode32(ZigZagEncode32(    1)));
  EXPECT_EQ(   -1, ZigZagDecode32(ZigZagEncode32(   -1)));
  EXPECT_EQ(14927, ZigZagDecode32(ZigZagEncode32(14927)));
  EXPECT_EQ(-3612, ZigZagDecode32(ZigZagEncode32(-3612)));

  EXPECT_EQ(    0, ZigZagDecode64(ZigZagEncode64(    0)));
  EXPECT_EQ(    1, ZigZagDecode64(ZigZagEncode64(    1)));
  EXPECT_EQ(   -1, ZigZagDecode64(ZigZagEncode64(   -1)));
  EXPECT_EQ(14927, ZigZagDecode64(ZigZagEncode64(14927)));
  EXPECT_EQ(-3612, ZigZagDecode64(ZigZagEncode64(-3612)));

  EXPECT_EQ(LL(856912304801416), ZigZagDecode64(ZigZagEncode64(
            LL(856912304801416))));
  EXPECT_EQ(LL(-75123905439571256), ZigZagDecode64(ZigZagEncode64(
            LL(-75123905439571256))));
}

class WireFormatInvalidInputTest : public testing::Test {
 protected:
  // Make a serialized TestAllTypes in which the field optional_nested_message
  // contains exactly the given bytes, which may be invalid.
  string MakeInvalidEmbeddedMessage(const char* bytes, int size) {
    const FieldDescriptor* field =
      unittest::TestAllTypes::descriptor()->FindFieldByName(
        "optional_nested_message");
    GOOGLE_CHECK(field != NULL);

    string result;

    {
      io::StringOutputStream raw_output(&result);
      io::CodedOutputStream output(&raw_output);

      EXPECT_TRUE(WireFormat::WriteString(
        field->number(), string(bytes, size), &output));
    }

    return result;
  }

  // Make a serialized TestAllTypes in which the field optionalgroup
  // contains exactly the given bytes -- which may be invalid -- and
  // possibly no end tag.
  string MakeInvalidGroup(const char* bytes, int size, bool include_end_tag) {
    const FieldDescriptor* field =
      unittest::TestAllTypes::descriptor()->FindFieldByName(
        "optionalgroup");
    GOOGLE_CHECK(field != NULL);

    string result;

    {
      io::StringOutputStream raw_output(&result);
      io::CodedOutputStream output(&raw_output);

      EXPECT_TRUE(output.WriteVarint32(WireFormat::MakeTag(field)));
      EXPECT_TRUE(output.WriteString(string(bytes, size)));
      if (include_end_tag) {
        EXPECT_TRUE(
          output.WriteVarint32(WireFormat::MakeTag(
            field->number(), WireFormat::WIRETYPE_END_GROUP)));
      }
    }

    return result;
  }
};

TEST_F(WireFormatInvalidInputTest, InvalidSubMessage) {
  unittest::TestAllTypes message;

  // Control case.
  EXPECT_TRUE(message.ParseFromString(MakeInvalidEmbeddedMessage("", 0)));

  // The byte is a valid varint, but not a valid tag (zero).
  EXPECT_FALSE(message.ParseFromString(MakeInvalidEmbeddedMessage("\0", 1)));

  // The byte is a malformed varint.
  EXPECT_FALSE(message.ParseFromString(MakeInvalidEmbeddedMessage("\200", 1)));

  // The byte is an endgroup tag, but we aren't parsing a group.
  EXPECT_FALSE(message.ParseFromString(MakeInvalidEmbeddedMessage("\014", 1)));

  // The byte is a valid varint but not a valid tag (bad wire type).
  EXPECT_FALSE(message.ParseFromString(MakeInvalidEmbeddedMessage("\017", 1)));
}

TEST_F(WireFormatInvalidInputTest, InvalidGroup) {
  unittest::TestAllTypes message;

  // Control case.
  EXPECT_TRUE(message.ParseFromString(MakeInvalidGroup("", 0, true)));

  // Missing end tag.  Groups cannot end at EOF.
  EXPECT_FALSE(message.ParseFromString(MakeInvalidGroup("", 0, false)));

  // The byte is a valid varint, but not a valid tag (zero).
  EXPECT_FALSE(message.ParseFromString(MakeInvalidGroup("\0", 1, false)));

  // The byte is a malformed varint.
  EXPECT_FALSE(message.ParseFromString(MakeInvalidGroup("\200", 1, false)));

  // The byte is an endgroup tag, but not the right one for this group.
  EXPECT_FALSE(message.ParseFromString(MakeInvalidGroup("\014", 1, false)));

  // The byte is a valid varint but not a valid tag (bad wire type).
  EXPECT_FALSE(message.ParseFromString(MakeInvalidGroup("\017", 1, true)));
}

TEST_F(WireFormatInvalidInputTest, InvalidUnknownGroup) {
  // Use ForeignMessage so that the group made by MakeInvalidGroup will not
  // be a known tag number.
  unittest::ForeignMessage message;

  // Control case.
  EXPECT_TRUE(message.ParseFromString(MakeInvalidGroup("", 0, true)));

  // Missing end tag.  Groups cannot end at EOF.
  EXPECT_FALSE(message.ParseFromString(MakeInvalidGroup("", 0, false)));

  // The byte is a valid varint, but not a valid tag (zero).
  EXPECT_FALSE(message.ParseFromString(MakeInvalidGroup("\0", 1, false)));

  // The byte is a malformed varint.
  EXPECT_FALSE(message.ParseFromString(MakeInvalidGroup("\200", 1, false)));

  // The byte is an endgroup tag, but not the right one for this group.
  EXPECT_FALSE(message.ParseFromString(MakeInvalidGroup("\014", 1, false)));

  // The byte is a valid varint but not a valid tag (bad wire type).
  EXPECT_FALSE(message.ParseFromString(MakeInvalidGroup("\017", 1, true)));
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
