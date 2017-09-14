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

#include <string>
#include <iostream>

#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/arena_test_util.h>
#include <google/protobuf/map_lite_test_util.h>
#include <google/protobuf/map_lite_unittest.pb.h>
#include <google/protobuf/test_util_lite.h>
#include <google/protobuf/unittest_lite.pb.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <gtest/gtest.h>

#include <google/protobuf/stubs/strutil.h>

using std::string;

namespace {
// Helper methods to test parsing merge behavior.
void ExpectMessageMerged(const google::protobuf::unittest::TestAllTypesLite& message) {
  EXPECT_EQ(message.optional_int32(), 3);
  EXPECT_EQ(message.optional_int64(), 2);
  EXPECT_EQ(message.optional_string(), "hello");
}

void AssignParsingMergeMessages(
    google::protobuf::unittest::TestAllTypesLite* msg1,
    google::protobuf::unittest::TestAllTypesLite* msg2,
    google::protobuf::unittest::TestAllTypesLite* msg3) {
  msg1->set_optional_int32(1);
  msg2->set_optional_int64(2);
  msg3->set_optional_int32(3);
  msg3->set_optional_string("hello");
}

void SetAllTypesInEmptyMessageUnknownFields(
    google::protobuf::unittest::TestEmptyMessageLite* empty_message) {
  protobuf_unittest::TestAllTypesLite message;
  google::protobuf::TestUtilLite::ExpectClear(message);
  google::protobuf::TestUtilLite::SetAllFields(&message);
  string data = message.SerializeAsString();
  empty_message->ParseFromString(data);
}

void SetSomeTypesInEmptyMessageUnknownFields(
    google::protobuf::unittest::TestEmptyMessageLite* empty_message) {
  protobuf_unittest::TestAllTypesLite message;
  google::protobuf::TestUtilLite::ExpectClear(message);
  message.set_optional_int32(101);
  message.set_optional_int64(102);
  message.set_optional_uint32(103);
  message.set_optional_uint64(104);
  string data = message.SerializeAsString();
  empty_message->ParseFromString(data);
}

}  // namespace

TEST(Lite, AllLite1) {
  string data;

  {
    protobuf_unittest::TestAllTypesLite message, message2, message3;
    google::protobuf::TestUtilLite::ExpectClear(message);
    google::protobuf::TestUtilLite::SetAllFields(&message);
    message2.CopyFrom(message);
    data = message.SerializeAsString();
    message3.ParseFromString(data);
    google::protobuf::TestUtilLite::ExpectAllFieldsSet(message);
    google::protobuf::TestUtilLite::ExpectAllFieldsSet(message2);
    google::protobuf::TestUtilLite::ExpectAllFieldsSet(message3);
    google::protobuf::TestUtilLite::ModifyRepeatedFields(&message);
    google::protobuf::TestUtilLite::ExpectRepeatedFieldsModified(message);
    message.Clear();
    google::protobuf::TestUtilLite::ExpectClear(message);
  }
}

TEST(Lite, AllLite2) {
  string data;
  {
    protobuf_unittest::TestAllExtensionsLite message, message2, message3;
    google::protobuf::TestUtilLite::ExpectExtensionsClear(message);
    google::protobuf::TestUtilLite::SetAllExtensions(&message);
    message2.CopyFrom(message);
    string extensions_data = message.SerializeAsString();
    message3.ParseFromString(extensions_data);
    google::protobuf::TestUtilLite::ExpectAllExtensionsSet(message);
    google::protobuf::TestUtilLite::ExpectAllExtensionsSet(message2);
    google::protobuf::TestUtilLite::ExpectAllExtensionsSet(message3);
    google::protobuf::TestUtilLite::ModifyRepeatedExtensions(&message);
    google::protobuf::TestUtilLite::ExpectRepeatedExtensionsModified(message);
    message.Clear();
    google::protobuf::TestUtilLite::ExpectExtensionsClear(message);
  }
}

TEST(Lite, AllLite3) {
  string data, packed_data;

  {
    protobuf_unittest::TestPackedTypesLite message, message2, message3;
    google::protobuf::TestUtilLite::ExpectPackedClear(message);
    google::protobuf::TestUtilLite::SetPackedFields(&message);
    message2.CopyFrom(message);
    packed_data = message.SerializeAsString();
    message3.ParseFromString(packed_data);
    google::protobuf::TestUtilLite::ExpectPackedFieldsSet(message);
    google::protobuf::TestUtilLite::ExpectPackedFieldsSet(message2);
    google::protobuf::TestUtilLite::ExpectPackedFieldsSet(message3);
    google::protobuf::TestUtilLite::ModifyPackedFields(&message);
    google::protobuf::TestUtilLite::ExpectPackedFieldsModified(message);
    message.Clear();
    google::protobuf::TestUtilLite::ExpectPackedClear(message);
  }

  {
    protobuf_unittest::TestPackedExtensionsLite message, message2, message3;
    google::protobuf::TestUtilLite::ExpectPackedExtensionsClear(message);
    google::protobuf::TestUtilLite::SetPackedExtensions(&message);
    message2.CopyFrom(message);
    string packed_extensions_data = message.SerializeAsString();
    EXPECT_EQ(packed_extensions_data, packed_data);
    message3.ParseFromString(packed_extensions_data);
    google::protobuf::TestUtilLite::ExpectPackedExtensionsSet(message);
    google::protobuf::TestUtilLite::ExpectPackedExtensionsSet(message2);
    google::protobuf::TestUtilLite::ExpectPackedExtensionsSet(message3);
    google::protobuf::TestUtilLite::ModifyPackedExtensions(&message);
    google::protobuf::TestUtilLite::ExpectPackedExtensionsModified(message);
    message.Clear();
    google::protobuf::TestUtilLite::ExpectPackedExtensionsClear(message);
  }
}

TEST(Lite, AllLite5) {
  string data;

  {
    // Test that if an optional or required message/group field appears multiple
    // times in the input, they need to be merged.
    google::protobuf::unittest::TestParsingMergeLite::RepeatedFieldsGenerator generator;
    google::protobuf::unittest::TestAllTypesLite* msg1;
    google::protobuf::unittest::TestAllTypesLite* msg2;
    google::protobuf::unittest::TestAllTypesLite* msg3;

#define ASSIGN_REPEATED_FIELD(FIELD)                \
  msg1 = generator.add_##FIELD();                   \
  msg2 = generator.add_##FIELD();                   \
  msg3 = generator.add_##FIELD();                   \
  AssignParsingMergeMessages(msg1, msg2, msg3)

    ASSIGN_REPEATED_FIELD(field1);
    ASSIGN_REPEATED_FIELD(field2);
    ASSIGN_REPEATED_FIELD(field3);
    ASSIGN_REPEATED_FIELD(ext1);
    ASSIGN_REPEATED_FIELD(ext2);

#undef ASSIGN_REPEATED_FIELD
#define ASSIGN_REPEATED_GROUP(FIELD)                \
  msg1 = generator.add_##FIELD()->mutable_field1(); \
  msg2 = generator.add_##FIELD()->mutable_field1(); \
  msg3 = generator.add_##FIELD()->mutable_field1(); \
  AssignParsingMergeMessages(msg1, msg2, msg3)

    ASSIGN_REPEATED_GROUP(group1);
    ASSIGN_REPEATED_GROUP(group2);

#undef ASSIGN_REPEATED_GROUP

    string buffer;
    generator.SerializeToString(&buffer);
    google::protobuf::unittest::TestParsingMergeLite parsing_merge;
    parsing_merge.ParseFromString(buffer);

    // Required and optional fields should be merged.
    ExpectMessageMerged(parsing_merge.required_all_types());
    ExpectMessageMerged(parsing_merge.optional_all_types());
    ExpectMessageMerged(
        parsing_merge.optionalgroup().optional_group_all_types());
    ExpectMessageMerged(parsing_merge.GetExtension(
        google::protobuf::unittest::TestParsingMergeLite::optional_ext));

    // Repeated fields should not be merged.
    EXPECT_EQ(parsing_merge.repeated_all_types_size(), 3);
    EXPECT_EQ(parsing_merge.repeatedgroup_size(), 3);
    EXPECT_EQ(parsing_merge.ExtensionSize(
                  google::protobuf::unittest::TestParsingMergeLite::repeated_ext),
              3);
  }
}

TEST(Lite, AllLite6) {
  string data;

  // Test unknown fields support for lite messages.
  {
    protobuf_unittest::TestAllTypesLite message, message2;
    protobuf_unittest::TestEmptyMessageLite empty_message;
    google::protobuf::TestUtilLite::ExpectClear(message);
    google::protobuf::TestUtilLite::SetAllFields(&message);
    data = message.SerializeAsString();
    empty_message.ParseFromString(data);
    data.clear();
    data = empty_message.SerializeAsString();
    message2.ParseFromString(data);
    data = message2.SerializeAsString();
    google::protobuf::TestUtilLite::ExpectAllFieldsSet(message2);
    message.Clear();
    google::protobuf::TestUtilLite::ExpectClear(message);
  }
}

TEST(Lite, AllLite7) {
  string data;

  {
    protobuf_unittest::TestAllExtensionsLite message, message2;
    protobuf_unittest::TestEmptyMessageLite empty_message;
    google::protobuf::TestUtilLite::ExpectExtensionsClear(message);
    google::protobuf::TestUtilLite::SetAllExtensions(&message);
    data = message.SerializeAsString();
    empty_message.ParseFromString(data);
    data.clear();
    data = empty_message.SerializeAsString();
    message2.ParseFromString(data);
    data = message2.SerializeAsString();
    google::protobuf::TestUtilLite::ExpectAllExtensionsSet(message2);
    message.Clear();
    google::protobuf::TestUtilLite::ExpectExtensionsClear(message);
  }
}

TEST(Lite, AllLite8) {
  string data;

  {
    protobuf_unittest::TestPackedTypesLite message, message2;
    protobuf_unittest::TestEmptyMessageLite empty_message;
    google::protobuf::TestUtilLite::ExpectPackedClear(message);
    google::protobuf::TestUtilLite::SetPackedFields(&message);
    data = message.SerializeAsString();
    empty_message.ParseFromString(data);
    data.clear();
    data = empty_message.SerializeAsString();
    message2.ParseFromString(data);
    data = message2.SerializeAsString();
    google::protobuf::TestUtilLite::ExpectPackedFieldsSet(message2);
    message.Clear();
    google::protobuf::TestUtilLite::ExpectPackedClear(message);
  }
}

TEST(Lite, AllLite9) {
  string data;

  {
    protobuf_unittest::TestPackedExtensionsLite message, message2;
    protobuf_unittest::TestEmptyMessageLite empty_message;
    google::protobuf::TestUtilLite::ExpectPackedExtensionsClear(message);
    google::protobuf::TestUtilLite::SetPackedExtensions(&message);
    data = message.SerializeAsString();
    empty_message.ParseFromString(data);
    data.clear();
    data = empty_message.SerializeAsString();
    message2.ParseFromString(data);
    data = message2.SerializeAsString();
    google::protobuf::TestUtilLite::ExpectPackedExtensionsSet(message2);
    message.Clear();
    google::protobuf::TestUtilLite::ExpectPackedExtensionsClear(message);
  }
}

TEST(Lite, AllLite10) {
  string data;

  {
    // Test Unknown fields swap
    protobuf_unittest::TestEmptyMessageLite empty_message, empty_message2;
    SetAllTypesInEmptyMessageUnknownFields(&empty_message);
    SetSomeTypesInEmptyMessageUnknownFields(&empty_message2);
    data = empty_message.SerializeAsString();
    string data2 = empty_message2.SerializeAsString();
    empty_message.Swap(&empty_message2);
    EXPECT_EQ(data, empty_message2.SerializeAsString());
    EXPECT_EQ(data2, empty_message.SerializeAsString());
  }
}

TEST(Lite, AllLite11) {
  string data;

  {
    // Test unknown fields swap with self
    protobuf_unittest::TestEmptyMessageLite empty_message;
    SetAllTypesInEmptyMessageUnknownFields(&empty_message);
    data = empty_message.SerializeAsString();
    empty_message.Swap(&empty_message);
    EXPECT_EQ(data, empty_message.SerializeAsString());
  }
}

TEST(Lite, AllLite12) {
  string data;

  {
    // Test MergeFrom with unknown fields
    protobuf_unittest::TestAllTypesLite message, message2;
    protobuf_unittest::TestEmptyMessageLite empty_message, empty_message2;
    message.set_optional_int32(101);
    message.add_repeated_int32(201);
    message.set_optional_nested_enum(google::protobuf::unittest::TestAllTypesLite::BAZ);
    message2.set_optional_int64(102);
    message2.add_repeated_int64(202);
    message2.set_optional_foreign_enum(google::protobuf::unittest::FOREIGN_LITE_BAZ);

    data = message.SerializeAsString();
    empty_message.ParseFromString(data);
    data = message2.SerializeAsString();
    empty_message2.ParseFromString(data);
    message.MergeFrom(message2);
    empty_message.MergeFrom(empty_message2);

    data = empty_message.SerializeAsString();
    message2.ParseFromString(data);
    // We do not compare the serialized output of a normal message and a lite
    // message because the order of fields do not match. We convert lite message
    // back into normal message, then compare.
    EXPECT_EQ(message.SerializeAsString(), message2.SerializeAsString());
  }
}

TEST(Lite, AllLite13) {
  string data;

  {
    // Test unknown enum value
    protobuf_unittest::TestAllTypesLite message;
    string buffer;
    {
      google::protobuf::io::StringOutputStream output_stream(&buffer);
      google::protobuf::io::CodedOutputStream coded_output(&output_stream);
      google::protobuf::internal::WireFormatLite::WriteTag(
          protobuf_unittest::TestAllTypesLite::kOptionalNestedEnumFieldNumber,
          google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT, &coded_output);
      coded_output.WriteVarint32(10);
      google::protobuf::internal::WireFormatLite::WriteTag(
          protobuf_unittest::TestAllTypesLite::kRepeatedNestedEnumFieldNumber,
          google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT, &coded_output);
      coded_output.WriteVarint32(20);
    }
    message.ParseFromString(buffer);
    data = message.SerializeAsString();
    EXPECT_EQ(data, buffer);
  }
}

TEST(Lite, AllLite14) {
  string data;

  {
    // Test Clear with unknown fields
    protobuf_unittest::TestEmptyMessageLite empty_message;
    SetAllTypesInEmptyMessageUnknownFields(&empty_message);
    empty_message.Clear();
    EXPECT_EQ(0, empty_message.unknown_fields().size());
  }
}

// Tests for map lite =============================================

TEST(Lite, AllLite15) {
  string data;

  {
    // Accessors
    protobuf_unittest::TestMapLite message;

    google::protobuf::MapLiteTestUtil::SetMapFields(&message);
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSet(message);

    google::protobuf::MapLiteTestUtil::ModifyMapFields(&message);
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsModified(message);
  }
}

TEST(Lite, AllLite16) {
  string data;

  {
    // SetMapFieldsInitialized
    protobuf_unittest::TestMapLite message;

    google::protobuf::MapLiteTestUtil::SetMapFieldsInitialized(&message);
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSetInitialized(message);
  }
}

TEST(Lite, AllLite17) {
  string data;

  {
    // Clear
    protobuf_unittest::TestMapLite message;

    google::protobuf::MapLiteTestUtil::SetMapFields(&message);
    message.Clear();
    google::protobuf::MapLiteTestUtil::ExpectClear(message);
  }
}

TEST(Lite, AllLite18) {
  string data;

  {
    // ClearMessageMap
    protobuf_unittest::TestMessageMapLite message;

    // Creates a TestAllTypes with default value
    google::protobuf::TestUtilLite::ExpectClear(
        (*message.mutable_map_int32_message())[0]);
  }
}

TEST(Lite, AllLite19) {
  string data;

  {
    // CopyFrom
    protobuf_unittest::TestMapLite message1, message2;

    google::protobuf::MapLiteTestUtil::SetMapFields(&message1);
    message2.CopyFrom(message1);
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSet(message2);

    // Copying from self should be a no-op.
    message2.CopyFrom(message2);
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSet(message2);
  }
}

TEST(Lite, AllLite20) {
  string data;

  {
    // CopyFromMessageMap
    protobuf_unittest::TestMessageMapLite message1, message2;

    (*message1.mutable_map_int32_message())[0].add_repeated_int32(100);
    (*message2.mutable_map_int32_message())[0].add_repeated_int32(101);

    message1.CopyFrom(message2);

    // Checks repeated field is overwritten.
    EXPECT_EQ(1, message1.map_int32_message().at(0).repeated_int32_size());
    EXPECT_EQ(101, message1.map_int32_message().at(0).repeated_int32(0));
  }
}

TEST(Lite, AllLite21) {
  string data;

  {
    // SwapWithEmpty
    protobuf_unittest::TestMapLite message1, message2;

    google::protobuf::MapLiteTestUtil::SetMapFields(&message1);
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSet(message1);
    google::protobuf::MapLiteTestUtil::ExpectClear(message2);

    message1.Swap(&message2);
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSet(message2);
    google::protobuf::MapLiteTestUtil::ExpectClear(message1);
  }
}

TEST(Lite, AllLite22) {
  string data;

  {
    // SwapWithSelf
    protobuf_unittest::TestMapLite message;

    google::protobuf::MapLiteTestUtil::SetMapFields(&message);
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSet(message);

    message.Swap(&message);
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSet(message);
  }
}

TEST(Lite, AllLite23) {
  string data;

  {
    // SwapWithOther
    protobuf_unittest::TestMapLite message1, message2;

    google::protobuf::MapLiteTestUtil::SetMapFields(&message1);
    google::protobuf::MapLiteTestUtil::SetMapFields(&message2);
    google::protobuf::MapLiteTestUtil::ModifyMapFields(&message2);

    message1.Swap(&message2);
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsModified(message1);
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSet(message2);
  }
}

TEST(Lite, AllLite24) {
  string data;

  {
    // CopyConstructor
    protobuf_unittest::TestMapLite message1;
    google::protobuf::MapLiteTestUtil::SetMapFields(&message1);

    protobuf_unittest::TestMapLite message2(message1);
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSet(message2);
  }
}

TEST(Lite, AllLite25) {
  string data;

  {
    // CopyAssignmentOperator
    protobuf_unittest::TestMapLite message1;
    google::protobuf::MapLiteTestUtil::SetMapFields(&message1);

    protobuf_unittest::TestMapLite message2;
    message2 = message1;
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSet(message2);

    // Make sure that self-assignment does something sane.
    message2.operator=(message2);
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSet(message2);
  }
}

TEST(Lite, AllLite26) {
  string data;

  {
    // NonEmptyMergeFrom
    protobuf_unittest::TestMapLite message1, message2;

    google::protobuf::MapLiteTestUtil::SetMapFields(&message1);

    // This field will test merging into an empty spot.
    (*message2.mutable_map_int32_int32())[1] = 1;
    message1.mutable_map_int32_int32()->erase(1);

    // This tests overwriting.
    (*message2.mutable_map_int32_double())[1] = 1;
    (*message1.mutable_map_int32_double())[1] = 2;

    message1.MergeFrom(message2);
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSet(message1);
  }
}

TEST(Lite, AllLite27) {
  string data;

  {
    // MergeFromMessageMap
    protobuf_unittest::TestMessageMapLite message1, message2;

    (*message1.mutable_map_int32_message())[0].add_repeated_int32(100);
    (*message2.mutable_map_int32_message())[0].add_repeated_int32(101);

    message1.MergeFrom(message2);

    // Checks repeated field is overwritten.
    EXPECT_EQ(1, message1.map_int32_message().at(0).repeated_int32_size());
    EXPECT_EQ(101, message1.map_int32_message().at(0).repeated_int32(0));
  }
}

TEST(Lite, AllLite28) {
  string data;

  {
    // Test the generated SerializeWithCachedSizesToArray()
    protobuf_unittest::TestMapLite message1, message2;
    string data;
    google::protobuf::MapLiteTestUtil::SetMapFields(&message1);
    int size = message1.ByteSize();
    data.resize(size);
    ::google::protobuf::uint8* start = reinterpret_cast< ::google::protobuf::uint8*>(::google::protobuf::string_as_array(&data));
    ::google::protobuf::uint8* end = message1.SerializeWithCachedSizesToArray(start);
    EXPECT_EQ(size, end - start);
    EXPECT_TRUE(message2.ParseFromString(data));
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSet(message2);
  }
}

TEST(Lite, AllLite29) {
  string data;

  {
    // Test the generated SerializeWithCachedSizes()
    protobuf_unittest::TestMapLite message1, message2;
    google::protobuf::MapLiteTestUtil::SetMapFields(&message1);
    int size = message1.ByteSize();
    string data;
    data.resize(size);
    {
      // Allow the output stream to buffer only one byte at a time.
      google::protobuf::io::ArrayOutputStream array_stream(
          ::google::protobuf::string_as_array(&data), size, 1);
      google::protobuf::io::CodedOutputStream output_stream(&array_stream);
      message1.SerializeWithCachedSizes(&output_stream);
      EXPECT_FALSE(output_stream.HadError());
      EXPECT_EQ(size, output_stream.ByteCount());
    }
    EXPECT_TRUE(message2.ParseFromString(data));
    google::protobuf::MapLiteTestUtil::ExpectMapFieldsSet(message2);
  }
}


TEST(Lite, AllLite32) {
  string data;

  {
    // Proto2UnknownEnum
    protobuf_unittest::TestEnumMapPlusExtraLite from;
    (*from.mutable_known_map_field())[0] =
        protobuf_unittest::E_PROTO2_MAP_ENUM_FOO_LITE;
    (*from.mutable_unknown_map_field())[0] =
        protobuf_unittest::E_PROTO2_MAP_ENUM_EXTRA_LITE;
    string data;
    from.SerializeToString(&data);

    protobuf_unittest::TestEnumMapLite to;
    EXPECT_TRUE(to.ParseFromString(data));
    EXPECT_EQ(0, to.unknown_map_field().size());
    EXPECT_FALSE(to.mutable_unknown_fields()->empty());
    EXPECT_EQ(1, to.known_map_field().size());
    EXPECT_EQ(protobuf_unittest::PROTO2_MAP_ENUM_FOO_LITE,
              to.known_map_field().at(0));

    data.clear();
    from.Clear();
    to.SerializeToString(&data);
    EXPECT_TRUE(from.ParseFromString(data));
    EXPECT_EQ(1, from.known_map_field().size());
    EXPECT_EQ(protobuf_unittest::E_PROTO2_MAP_ENUM_FOO_LITE,
              from.known_map_field().at(0));
    EXPECT_EQ(1, from.unknown_map_field().size());
    EXPECT_EQ(protobuf_unittest::E_PROTO2_MAP_ENUM_EXTRA_LITE,
              from.unknown_map_field().at(0));
  }
}

TEST(Lite, AllLite33) {
  string data;

  {
    // StandardWireFormat
    protobuf_unittest::TestMapLite message;
    string data = "\x0A\x04\x08\x01\x10\x01";

    EXPECT_TRUE(message.ParseFromString(data));
    EXPECT_EQ(1, message.map_int32_int32().size());
    EXPECT_EQ(1, message.map_int32_int32().at(1));
  }
}

TEST(Lite, AllLite34) {
  string data;

  {
    // UnorderedWireFormat
    protobuf_unittest::TestMapLite message;

    // put value before key in wire format
    string data = "\x0A\x04\x10\x01\x08\x02";

    EXPECT_TRUE(message.ParseFromString(data));
    EXPECT_EQ(1, message.map_int32_int32().size());
    EXPECT_EQ(1, message.map_int32_int32().at(2));
  }
}

TEST(Lite, AllLite35) {
  string data;

  {
    // DuplicatedKeyWireFormat
    protobuf_unittest::TestMapLite message;

    // Two key fields in wire format
    string data = "\x0A\x06\x08\x01\x08\x02\x10\x01";

    EXPECT_TRUE(message.ParseFromString(data));
    EXPECT_EQ(1, message.map_int32_int32().size());
    EXPECT_EQ(1, message.map_int32_int32().at(2));
  }
}

TEST(Lite, AllLite36) {
  string data;

  {
    // DuplicatedValueWireFormat
    protobuf_unittest::TestMapLite message;

    // Two value fields in wire format
    string data = "\x0A\x06\x08\x01\x10\x01\x10\x02";

    EXPECT_TRUE(message.ParseFromString(data));
    EXPECT_EQ(1, message.map_int32_int32().size());
    EXPECT_EQ(2, message.map_int32_int32().at(1));
  }
}

TEST(Lite, AllLite37) {
  string data;

  {
    // MissedKeyWireFormat
    protobuf_unittest::TestMapLite message;

    // No key field in wire format
    string data = "\x0A\x02\x10\x01";

    EXPECT_TRUE(message.ParseFromString(data));
    EXPECT_EQ(1, message.map_int32_int32().size());
    EXPECT_EQ(1, message.map_int32_int32().at(0));
  }
}

TEST(Lite, AllLite38) {
  string data;

  {
    // MissedValueWireFormat
    protobuf_unittest::TestMapLite message;

    // No value field in wire format
    string data = "\x0A\x02\x08\x01";

    EXPECT_TRUE(message.ParseFromString(data));
    EXPECT_EQ(1, message.map_int32_int32().size());
    EXPECT_EQ(0, message.map_int32_int32().at(1));
  }
}

TEST(Lite, AllLite39) {
  string data;

  {
    // UnknownFieldWireFormat
    protobuf_unittest::TestMapLite message;

    // Unknown field in wire format
    string data = "\x0A\x06\x08\x02\x10\x03\x18\x01";

    EXPECT_TRUE(message.ParseFromString(data));
    EXPECT_EQ(1, message.map_int32_int32().size());
    EXPECT_EQ(3, message.map_int32_int32().at(2));
  }
}

TEST(Lite, AllLite40) {
  string data;

  {
    // CorruptedWireFormat
    protobuf_unittest::TestMapLite message;

    // corrupted data in wire format
    string data = "\x0A\x06\x08\x02\x11\x03";

    EXPECT_FALSE(message.ParseFromString(data));
  }
}

TEST(Lite, AllLite41) {
  string data;

  {
    // IsInitialized
    protobuf_unittest::TestRequiredMessageMapLite map_message;

    // Add an uninitialized message.
    (*map_message.mutable_map_field())[0];
    EXPECT_FALSE(map_message.IsInitialized());

    // Initialize uninitialized message
    (*map_message.mutable_map_field())[0].set_a(0);
    (*map_message.mutable_map_field())[0].set_b(0);
    (*map_message.mutable_map_field())[0].set_c(0);
    EXPECT_TRUE(map_message.IsInitialized());
  }
}

TEST(Lite, AllLite42) {
  string data;

  {
      // Check that adding more values to enum does not corrupt message
      // when passed through an old client.
      protobuf_unittest::V2MessageLite v2_message;
      v2_message.set_int_field(800);
      // Set enum field to the value not understood by the old client.
      v2_message.set_enum_field(protobuf_unittest::V2_SECOND);
      string v2_bytes = v2_message.SerializeAsString();

      protobuf_unittest::V1MessageLite v1_message;
      v1_message.ParseFromString(v2_bytes);
      EXPECT_TRUE(v1_message.IsInitialized());
      EXPECT_EQ(v1_message.int_field(), v2_message.int_field());
      // V1 client does not understand V2_SECOND value, so it discards it and
      // uses default value instead.
      EXPECT_EQ(v1_message.enum_field(), protobuf_unittest::V1_FIRST);

      // However, when re-serialized, it should preserve enum value.
      string v1_bytes = v1_message.SerializeAsString();

      protobuf_unittest::V2MessageLite same_v2_message;
      same_v2_message.ParseFromString(v1_bytes);

      EXPECT_EQ(v2_message.int_field(), same_v2_message.int_field());
      EXPECT_EQ(v2_message.enum_field(), same_v2_message.enum_field());
  }
}

// Test that when parsing a oneof, we can successfully clear whatever already
// happened to be stored in the oneof.
TEST(Lite, AllLite43) {
  protobuf_unittest::TestOneofParsingLite message1;

  message1.set_oneof_int32(17);
  string serialized;
  EXPECT_TRUE(message1.SerializeToString(&serialized));

  // Submessage
  {
    protobuf_unittest::TestOneofParsingLite message2;
    message2.mutable_oneof_submessage();
    google::protobuf::io::CodedInputStream input_stream(
        reinterpret_cast<const ::google::protobuf::uint8*>(serialized.data()), serialized.size());
    EXPECT_TRUE(message2.MergeFromCodedStream(&input_stream));
    EXPECT_EQ(17, message2.oneof_int32());
  }

  // String
  {
    protobuf_unittest::TestOneofParsingLite message2;
    message2.set_oneof_string("string");
    google::protobuf::io::CodedInputStream input_stream(
        reinterpret_cast<const ::google::protobuf::uint8*>(serialized.data()), serialized.size());
    EXPECT_TRUE(message2.MergeFromCodedStream(&input_stream));
    EXPECT_EQ(17, message2.oneof_int32());
  }

  // Bytes
  {
    protobuf_unittest::TestOneofParsingLite message2;
    message2.set_oneof_bytes("bytes");
    google::protobuf::io::CodedInputStream input_stream(
        reinterpret_cast<const ::google::protobuf::uint8*>(serialized.data()), serialized.size());
    EXPECT_TRUE(message2.MergeFromCodedStream(&input_stream));
    EXPECT_EQ(17, message2.oneof_int32());
  }
}

// Verify that we can successfully parse fields of various types within oneof
// fields. We also verify that we can parse the same data twice into the same
// message.
TEST(Lite, AllLite44) {
  // Int32
  {
    protobuf_unittest::TestOneofParsingLite original;
    original.set_oneof_int32(17);
    string serialized;
    EXPECT_TRUE(original.SerializeToString(&serialized));
    protobuf_unittest::TestOneofParsingLite parsed;
    for (int i = 0; i < 2; ++i) {
      google::protobuf::io::CodedInputStream input_stream(
          reinterpret_cast<const ::google::protobuf::uint8*>(serialized.data()),
          serialized.size());
      EXPECT_TRUE(parsed.MergeFromCodedStream(&input_stream));
      EXPECT_EQ(17, parsed.oneof_int32());
    }
  }

  // Submessage
  {
    protobuf_unittest::TestOneofParsingLite original;
    original.mutable_oneof_submessage()->set_optional_int32(5);
    string serialized;
    EXPECT_TRUE(original.SerializeToString(&serialized));
    protobuf_unittest::TestOneofParsingLite parsed;
    for (int i = 0; i < 2; ++i) {
      google::protobuf::io::CodedInputStream input_stream(
          reinterpret_cast<const ::google::protobuf::uint8*>(serialized.data()),
          serialized.size());
      EXPECT_TRUE(parsed.MergeFromCodedStream(&input_stream));
      EXPECT_EQ(5, parsed.oneof_submessage().optional_int32());
    }
  }

  // String
  {
    protobuf_unittest::TestOneofParsingLite original;
    original.set_oneof_string("string");
    string serialized;
    EXPECT_TRUE(original.SerializeToString(&serialized));
    protobuf_unittest::TestOneofParsingLite parsed;
    for (int i = 0; i < 2; ++i) {
      google::protobuf::io::CodedInputStream input_stream(
          reinterpret_cast<const ::google::protobuf::uint8*>(serialized.data()),
          serialized.size());
      EXPECT_TRUE(parsed.MergeFromCodedStream(&input_stream));
      EXPECT_EQ("string", parsed.oneof_string());
    }
  }

  // Bytes
  {
    protobuf_unittest::TestOneofParsingLite original;
    original.set_oneof_bytes("bytes");
    string serialized;
    EXPECT_TRUE(original.SerializeToString(&serialized));
    protobuf_unittest::TestOneofParsingLite parsed;
    for (int i = 0; i < 2; ++i) {
      google::protobuf::io::CodedInputStream input_stream(
          reinterpret_cast<const ::google::protobuf::uint8*>(serialized.data()),
          serialized.size());
      EXPECT_TRUE(parsed.MergeFromCodedStream(&input_stream));
      EXPECT_EQ("bytes", parsed.oneof_bytes());
    }
  }

  // Enum
  {
    protobuf_unittest::TestOneofParsingLite original;
    original.set_oneof_enum(protobuf_unittest::V2_SECOND);
    string serialized;
    EXPECT_TRUE(original.SerializeToString(&serialized));
    protobuf_unittest::TestOneofParsingLite parsed;
    for (int i = 0; i < 2; ++i) {
      google::protobuf::io::CodedInputStream input_stream(
          reinterpret_cast<const ::google::protobuf::uint8*>(serialized.data()),
          serialized.size());
      EXPECT_TRUE(parsed.MergeFromCodedStream(&input_stream));
      EXPECT_EQ(protobuf_unittest::V2_SECOND, parsed.oneof_enum());
    }
  }

  std::cout << "PASS" << std::endl;
}
