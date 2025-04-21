// -*- c++ -*-
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#ifndef GOOGLE_PROTOBUF_WIRE_FORMAT_UNITTEST_H__
#define GOOGLE_PROTOBUF_WIRE_FORMAT_UNITTEST_H__

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/base/casts.h"
#include "absl/base/log_severity.h"
#include "absl/flags/flag.h"
#include "absl/log/absl_check.h"
#include "absl/log/scoped_mock_log.h"
#include "absl/strings/cord.h"
#include "absl/strings/match.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/generated_message_tctable_impl.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/test_util.h"
#include "google/protobuf/test_util2.h"
#include "google/protobuf/wire_format.h"
#include "google/protobuf/wire_format_lite.h"

// clang-format off
#include "google/protobuf/port_def.inc"
// clang-format on

namespace google {
namespace protobuf {
namespace internal {

inline bool IsOptimizeForCodeSize(const Descriptor* descriptor) {
  return descriptor->file()->options().optimize_for() == FileOptions::CODE_SIZE;
}

template <typename T>
class WireFormatTest : public testing::Test,
                       protected TestUtil::TestUtilTraits<T> {
 protected:
  using TestMessageSet = typename TestUtil::TestUtilTraits<T>::TestMessageSet;
  using TestMessageSetExtension1 =
      typename TestUtil::TestUtilTraits<T>::TestMessageSetExtension1;

  std::string BuildMessageSetTestExtension1(int value = 123) {
    std::string data;
    {
      TestMessageSetExtension1 message;
      message.set_i(value);
      io::StringOutputStream output_stream(&data);
      io::CodedOutputStream coded_output(&output_stream);
      // Write the message content first.
      WireFormatLite::WriteTag(WireFormatLite::kMessageSetMessageNumber,
                               WireFormatLite::WIRETYPE_LENGTH_DELIMITED,
                               &coded_output);
      coded_output.WriteVarint32(message.ByteSizeLong());
      message.SerializeWithCachedSizes(&coded_output);
    }
    return data;
  }

  void ValidateTestMessageSet(const std::string& test_case,
                              const std::string& data) {
    SCOPED_TRACE(test_case);
    {
      TestMessageSet message_set;
      ASSERT_TRUE(message_set.ParseFromString(data));

      EXPECT_EQ(123, message_set
                         .GetExtension(
                             TestMessageSetExtension1::message_set_extension)
                         .i());

      // Make sure it does not contain anything else.
      message_set.ClearExtension(
          TestMessageSetExtension1::message_set_extension);
      EXPECT_EQ(message_set.SerializeAsString(), "");
    }
    {
      // Test parse the message via Reflection.
      TestMessageSet message_set;
      io::CodedInputStream input(reinterpret_cast<const uint8_t*>(data.data()),
                                 data.size());
      EXPECT_TRUE(WireFormat::ParseAndMergePartial(&input, &message_set));
      EXPECT_TRUE(input.ConsumedEntireMessage());

      EXPECT_EQ(123, message_set
                         .GetExtension(
                             TestMessageSetExtension1::message_set_extension)
                         .i());
    }
    {
      // Test parse the message via DynamicMessage.
      DynamicMessageFactory factory;
      std::unique_ptr<Message> msg(
          factory.GetPrototype(TestMessageSet::descriptor())->New());
      msg->ParseFromString(data);
      auto* reflection = msg->GetReflection();
      std::vector<const FieldDescriptor*> fields;
      reflection->ListFields(*msg, &fields);
      ASSERT_EQ(fields.size(), 1);
      const auto& sub = reflection->GetMessage(*msg, fields[0]);
      reflection = sub.GetReflection();
      EXPECT_EQ(123, reflection->GetInt32(
                         sub, sub.GetDescriptor()->FindFieldByName("i")));
    }
  }

  void SerializeReverseOrder(const TestMessageSetExtension1& message,
                             io::CodedOutputStream* coded_output) {
    WireFormatLite::WriteTag(15,  // i
                             WireFormatLite::WIRETYPE_VARINT, coded_output);
    coded_output->WriteVarint64(message.i());
    WireFormatLite::WriteTag(16,  // recursive
                             WireFormatLite::WIRETYPE_LENGTH_DELIMITED,
                             coded_output);
    coded_output->WriteVarint32(message.recursive().GetCachedSize());
    SerializeReverseOrder(message.recursive(), coded_output);
  }

  void SerializeReverseOrder(const TestMessageSet& mset,
                             io::CodedOutputStream* coded_output) {
    if (!mset.HasExtension(TestMessageSetExtension1::message_set_extension))
      return;
    coded_output->WriteTag(WireFormatLite::kMessageSetItemStartTag);
    // Write the message content first.
    WireFormatLite::WriteTag(WireFormatLite::kMessageSetMessageNumber,
                             WireFormatLite::WIRETYPE_LENGTH_DELIMITED,
                             coded_output);
    auto& message =
        mset.GetExtension(TestMessageSetExtension1::message_set_extension);
    coded_output->WriteVarint32(message.GetCachedSize());
    SerializeReverseOrder(message, coded_output);
    // Write the type id.
    uint32_t type_id = message.GetDescriptor()->extension(0)->number();
    WireFormatLite::WriteUInt32(WireFormatLite::kMessageSetTypeIdNumber,
                                type_id, coded_output);
    coded_output->WriteTag(WireFormatLite::kMessageSetItemEndTag);
  }
};

TYPED_TEST_SUITE_P(WireFormatTest);

TYPED_TEST_P(WireFormatTest, Parse) {
  typename TestFixture::TestAllTypes source, dest;
  std::string data;

  // Serialize using the generated code.
  TestUtil::SetAllFields(&source);
  source.SerializeToString(&data);

  // Parse using WireFormat.
  io::ArrayInputStream raw_input(data.data(), data.size());
  io::CodedInputStream input(&raw_input);
  WireFormat::ParseAndMergePartial(&input, &dest);

  // Check.
  TestUtil::ExpectAllFieldsSet(dest);
}

TYPED_TEST_P(WireFormatTest, ParseExtensions) {
  typename TestFixture::TestAllExtensions source, dest;
  std::string data;

  // Serialize using the generated code.
  TestUtil::SetAllExtensions(&source);
  source.SerializeToString(&data);

  // Parse using WireFormat.
  io::ArrayInputStream raw_input(data.data(), data.size());
  io::CodedInputStream input(&raw_input);
  WireFormat::ParseAndMergePartial(&input, &dest);

  // Check.
  TestUtil::ExpectAllExtensionsSet(dest);
}

TYPED_TEST_P(WireFormatTest, ParsePacked) {
  typename TestFixture::TestPackedTypes source, dest;
  std::string data;

  // Serialize using the generated code.
  TestUtil::SetPackedFields(&source);
  source.SerializeToString(&data);

  // Parse using WireFormat.
  io::ArrayInputStream raw_input(data.data(), data.size());
  io::CodedInputStream input(&raw_input);
  WireFormat::ParseAndMergePartial(&input, &dest);

  // Check.
  TestUtil::ExpectPackedFieldsSet(dest);
}

TYPED_TEST_P(WireFormatTest, ParsePackedFromUnpacked) {
  // Serialize using the generated code.
  typename TestFixture::TestUnpackedTypes source;
  TestUtil::SetUnpackedFields(&source);
  std::string data = source.SerializeAsString();

  // Parse using WireFormat.
  typename TestFixture::TestPackedTypes dest;
  io::ArrayInputStream raw_input(data.data(), data.size());
  io::CodedInputStream input(&raw_input);
  WireFormat::ParseAndMergePartial(&input, &dest);

  // Check.
  TestUtil::ExpectPackedFieldsSet(dest);
}

TYPED_TEST_P(WireFormatTest, ParseUnpackedFromPacked) {
  // Serialize using the generated code.
  typename TestFixture::TestPackedTypes source;
  TestUtil::SetPackedFields(&source);
  std::string data = source.SerializeAsString();

  // Parse using WireFormat.
  typename TestFixture::TestUnpackedTypes dest;
  io::ArrayInputStream raw_input(data.data(), data.size());
  io::CodedInputStream input(&raw_input);
  WireFormat::ParseAndMergePartial(&input, &dest);

  // Check.
  TestUtil::ExpectUnpackedFieldsSet(dest);
}

TYPED_TEST_P(WireFormatTest, ParsePackedExtensions) {
  typename TestFixture::TestPackedExtensions source, dest;
  std::string data;

  // Serialize using the generated code.
  TestUtil::SetPackedExtensions(&source);
  source.SerializeToString(&data);

  // Parse using WireFormat.
  io::ArrayInputStream raw_input(data.data(), data.size());
  io::CodedInputStream input(&raw_input);
  WireFormat::ParseAndMergePartial(&input, &dest);

  // Check.
  TestUtil::ExpectPackedExtensionsSet(dest);
}

TYPED_TEST_P(WireFormatTest, ParseOneof) {
  typename TestFixture::TestOneof2 source, dest;
  std::string data;

  // Serialize using the generated code.
  TestUtil::SetOneof1(&source);
  source.SerializeToString(&data);

  // Parse using WireFormat.
  io::ArrayInputStream raw_input(data.data(), data.size());
  io::CodedInputStream input(&raw_input);
  WireFormat::ParseAndMergePartial(&input, &dest);

  // Check.
  TestUtil::ExpectOneofSet1(dest);
}

TYPED_TEST_P(WireFormatTest, OneofOnlySetLast) {
  typename TestFixture::TestOneofBackwardsCompatible source;
  typename TestFixture::TestOneof oneof_dest;
  std::string data;

  // Set two fields
  source.set_foo_int(100);
  source.set_foo_string("101");

  // Serialize and parse to oneof message. Generated serializer may not order
  // fields in tag order. Use WireFormat::SerializeWithCachedSizes instead as
  // it sorts fields beforehand.
  {
    io::StringOutputStream raw_output(&data);
    io::CodedOutputStream output(&raw_output);
    WireFormat::SerializeWithCachedSizes(source, source.ByteSizeLong(),
                                         &output);
    ASSERT_FALSE(output.HadError());
  }
  io::ArrayInputStream raw_input(data.data(), data.size());
  io::CodedInputStream input(&raw_input);
  WireFormat::ParseAndMergePartial(&input, &oneof_dest);

  // Only the last field is set.
  EXPECT_FALSE(oneof_dest.has_foo_int());
  EXPECT_TRUE(oneof_dest.has_foo_string());
}

TYPED_TEST_P(WireFormatTest, ByteSize) {
  typename TestFixture::TestAllTypes message;
  TestUtil::SetAllFields(&message);

  EXPECT_EQ(message.ByteSizeLong(), WireFormat::ByteSize(message));
  message.Clear();
  EXPECT_EQ(0, message.ByteSizeLong());
  EXPECT_EQ(0, WireFormat::ByteSize(message));
}

TYPED_TEST_P(WireFormatTest, ByteSizeExtensions) {
  typename TestFixture::TestAllExtensions message;
  TestUtil::SetAllExtensions(&message);

  EXPECT_EQ(message.ByteSizeLong(), WireFormat::ByteSize(message));
  message.Clear();
  EXPECT_EQ(0, message.ByteSizeLong());
  EXPECT_EQ(0, WireFormat::ByteSize(message));
}

TYPED_TEST_P(WireFormatTest, ByteSizePacked) {
  typename TestFixture::TestPackedTypes message;
  TestUtil::SetPackedFields(&message);

  EXPECT_EQ(message.ByteSizeLong(), WireFormat::ByteSize(message));
  message.Clear();
  EXPECT_EQ(0, message.ByteSizeLong());
  EXPECT_EQ(0, WireFormat::ByteSize(message));
}

TYPED_TEST_P(WireFormatTest, ByteSizePackedExtensions) {
  typename TestFixture::TestPackedExtensions message;
  TestUtil::SetPackedExtensions(&message);

  EXPECT_EQ(message.ByteSizeLong(), WireFormat::ByteSize(message));
  message.Clear();
  EXPECT_EQ(0, message.ByteSizeLong());
  EXPECT_EQ(0, WireFormat::ByteSize(message));
}

TYPED_TEST_P(WireFormatTest, ByteSizeOneof) {
  typename TestFixture::TestOneof2 message;
  TestUtil::SetOneof1(&message);

  EXPECT_EQ(message.ByteSizeLong(), WireFormat::ByteSize(message));
  message.Clear();

  EXPECT_EQ(0, message.ByteSizeLong());
  EXPECT_EQ(0, WireFormat::ByteSize(message));
}

TYPED_TEST_P(WireFormatTest, Serialize) {
  typename TestFixture::TestAllTypes message;
  std::string generated_data;
  std::string dynamic_data;

  TestUtil::SetAllFields(&message);
  size_t size = message.ByteSizeLong();

  // Serialize using the generated code.
  {
    io::StringOutputStream raw_output(&generated_data);
    io::CodedOutputStream output(&raw_output);
    message.SerializeWithCachedSizes(&output);
    ASSERT_FALSE(output.HadError());
  }

  // Serialize using WireFormat.
  {
    io::StringOutputStream raw_output(&dynamic_data);
    io::CodedOutputStream output(&raw_output);
    WireFormat::SerializeWithCachedSizes(message, size, &output);
    ASSERT_FALSE(output.HadError());
  }

  // Should parse to the same message.
  EXPECT_TRUE(TestUtil::EqualsToSerialized(message, generated_data));
  EXPECT_TRUE(TestUtil::EqualsToSerialized(message, dynamic_data));
}

TYPED_TEST_P(WireFormatTest, SerializeExtensions) {
  typename TestFixture::TestAllExtensions message;
  std::string generated_data;
  std::string dynamic_data;

  TestUtil::SetAllExtensions(&message);
  size_t size = message.ByteSizeLong();

  // Serialize using the generated code.
  {
    io::StringOutputStream raw_output(&generated_data);
    io::CodedOutputStream output(&raw_output);
    message.SerializeWithCachedSizes(&output);
    ASSERT_FALSE(output.HadError());
  }

  // Serialize using WireFormat.
  {
    io::StringOutputStream raw_output(&dynamic_data);
    io::CodedOutputStream output(&raw_output);
    WireFormat::SerializeWithCachedSizes(message, size, &output);
    ASSERT_FALSE(output.HadError());
  }

  // Should parse to the same message.
  EXPECT_TRUE(TestUtil::EqualsToSerialized(message, generated_data));
  EXPECT_TRUE(TestUtil::EqualsToSerialized(message, dynamic_data));
}

TYPED_TEST_P(WireFormatTest, SerializeFieldsAndExtensions) {
  typename TestFixture::TestFieldOrderings message;
  std::string generated_data;
  std::string dynamic_data;

  TestUtil::SetAllFieldsAndExtensions(&message);
  size_t size = message.ByteSizeLong();

  // Serialize using the generated code.
  {
    io::StringOutputStream raw_output(&generated_data);
    io::CodedOutputStream output(&raw_output);
    message.SerializeWithCachedSizes(&output);
    ASSERT_FALSE(output.HadError());
  }

  // Serialize using WireFormat.
  {
    io::StringOutputStream raw_output(&dynamic_data);
    io::CodedOutputStream output(&raw_output);
    WireFormat::SerializeWithCachedSizes(message, size, &output);
    ASSERT_FALSE(output.HadError());
  }

  // Should parse to the same message.
  EXPECT_TRUE(TestUtil::EqualsToSerialized(message, generated_data));
  EXPECT_TRUE(TestUtil::EqualsToSerialized(message, dynamic_data));
}

TYPED_TEST_P(WireFormatTest, SerializeOneof) {
  typename TestFixture::TestOneof2 message;
  std::string generated_data;
  std::string dynamic_data;

  TestUtil::SetOneof1(&message);
  size_t size = message.ByteSizeLong();

  // Serialize using the generated code.
  {
    io::StringOutputStream raw_output(&generated_data);
    io::CodedOutputStream output(&raw_output);
    message.SerializeWithCachedSizes(&output);
    ASSERT_FALSE(output.HadError());
  }

  // Serialize using WireFormat.
  {
    io::StringOutputStream raw_output(&dynamic_data);
    io::CodedOutputStream output(&raw_output);
    WireFormat::SerializeWithCachedSizes(message, size, &output);
    ASSERT_FALSE(output.HadError());
  }

  // Should parse to the same message.
  EXPECT_TRUE(TestUtil::EqualsToSerialized(message, generated_data));
  EXPECT_TRUE(TestUtil::EqualsToSerialized(message, dynamic_data));
}

TYPED_TEST_P(WireFormatTest, ParseMultipleExtensionRanges) {
  // Make sure we can parse a message that contains multiple extensions ranges.
  typename TestFixture::TestFieldOrderings source;
  std::string data;

  TestUtil::SetAllFieldsAndExtensions(&source);
  source.SerializeToString(&data);

  {
    typename TestFixture::TestFieldOrderings dest;
    EXPECT_TRUE(dest.ParseFromString(data));
    EXPECT_EQ(source.DebugString(), dest.DebugString());
  }

  // Also test using reflection-based parsing.
  {
    typename TestFixture::TestFieldOrderings dest;
    io::ArrayInputStream raw_input(data.data(), data.size());
    io::CodedInputStream coded_input(&raw_input);
    EXPECT_TRUE(WireFormat::ParseAndMergePartial(&coded_input, &dest));
    EXPECT_EQ(source.DebugString(), dest.DebugString());
  }
}

const int kUnknownTypeId = 1550055;

TYPED_TEST_P(WireFormatTest, SerializeMessageSet) {
  // Set up a TestMessageSet with two known messages and an unknown one.
  typename TestFixture::TestMessageSet message_set;
  message_set
      .MutableExtension(
          TestFixture::TestMessageSetExtension1::message_set_extension)
      ->set_i(123);
  message_set
      .MutableExtension(
          TestFixture::TestMessageSetExtension2::message_set_extension)
      ->set_str("foo");
  message_set.mutable_unknown_fields()->AddLengthDelimited(kUnknownTypeId,
                                                           "bar");

  std::string data;
  ASSERT_TRUE(message_set.SerializeToString(&data));

  // Parse back using RawMessageSet and check the contents.
  typename TestFixture::RawMessageSet raw;
  ASSERT_TRUE(raw.ParseFromString(data));

  EXPECT_EQ(0, raw.unknown_fields().field_count());

  ASSERT_EQ(3, raw.item_size());
  EXPECT_EQ(TestFixture::TestMessageSetExtension1::descriptor()
                ->extension(0)
                ->number(),
            raw.item(0).type_id());
  EXPECT_EQ(TestFixture::TestMessageSetExtension2::descriptor()
                ->extension(0)
                ->number(),
            raw.item(1).type_id());
  EXPECT_EQ(kUnknownTypeId, raw.item(2).type_id());

  typename TestFixture::TestMessageSetExtension1 message1;
  EXPECT_TRUE(message1.ParseFromString(raw.item(0).message()));
  EXPECT_EQ(123, message1.i());

  typename TestFixture::TestMessageSetExtension2 message2;
  EXPECT_TRUE(message2.ParseFromString(raw.item(1).message()));
  EXPECT_EQ("foo", message2.str());

  EXPECT_EQ("bar", raw.item(2).message());
}

TYPED_TEST_P(WireFormatTest, SerializeMessageSetVariousWaysAreEqual) {
  // Serialize a MessageSet to a stream and to a flat array using generated
  // code, and also using WireFormat, and check that the results are equal.
  // Set up a TestMessageSet with two known messages and an unknown one, as
  // above.

  typename TestFixture::TestMessageSet message_set;
  message_set
      .MutableExtension(
          TestFixture::TestMessageSetExtension1::message_set_extension)
      ->set_i(123);
  message_set
      .MutableExtension(
          TestFixture::TestMessageSetExtension2::message_set_extension)
      ->set_str("foo");
  message_set.mutable_unknown_fields()->AddLengthDelimited(kUnknownTypeId,
                                                           "bar");

  size_t size = message_set.ByteSizeLong();
  EXPECT_EQ(size, message_set.GetCachedSize());
  ASSERT_EQ(size, WireFormat::ByteSize(message_set));

  std::string flat_data;
  std::string stream_data;
  std::string dynamic_data;
  flat_data.resize(size);
  stream_data.resize(size);

  // Serialize to flat array
  {
    uint8_t* target = reinterpret_cast<uint8_t*>(&flat_data[0]);
    uint8_t* end = message_set.SerializeWithCachedSizesToArray(target);
    EXPECT_EQ(size, end - target);
  }

  // Serialize to buffer
  {
    io::ArrayOutputStream array_stream(&stream_data[0], size, 1);
    io::CodedOutputStream output_stream(&array_stream);
    message_set.SerializeWithCachedSizes(&output_stream);
    ASSERT_FALSE(output_stream.HadError());
  }

  // Serialize to buffer with WireFormat.
  {
    io::StringOutputStream string_stream(&dynamic_data);
    io::CodedOutputStream output_stream(&string_stream);
    WireFormat::SerializeWithCachedSizes(message_set, size, &output_stream);
    ASSERT_FALSE(output_stream.HadError());
  }

  EXPECT_TRUE(flat_data == stream_data);
  EXPECT_TRUE(flat_data == dynamic_data);
}

TYPED_TEST_P(WireFormatTest, ParseMessageSet) {
  // Set up a RawMessageSet with two known messages and an unknown one.
  typename TestFixture::RawMessageSet raw;

  {
    typename TestFixture::RawMessageSet::Item* item = raw.add_item();
    item->set_type_id(TestFixture::TestMessageSetExtension1::descriptor()
                          ->extension(0)
                          ->number());
    typename TestFixture::TestMessageSetExtension1 message;
    message.set_i(123);
    message.SerializeToString(item->mutable_message());
  }

  {
    typename TestFixture::RawMessageSet::Item* item = raw.add_item();
    item->set_type_id(TestFixture::TestMessageSetExtension2::descriptor()
                          ->extension(0)
                          ->number());
    typename TestFixture::TestMessageSetExtension2 message;
    message.set_str("foo");
    message.SerializeToString(item->mutable_message());
  }

  {
    typename TestFixture::RawMessageSet::Item* item = raw.add_item();
    item->set_type_id(kUnknownTypeId);
    item->set_message("bar");
  }

  std::string data;
  ASSERT_TRUE(raw.SerializeToString(&data));

  // Parse as a TestMessageSet and check the contents.
  typename TestFixture::TestMessageSet message_set;
  ASSERT_TRUE(message_set.ParseFromString(data));

  EXPECT_EQ(
      123, message_set
               .GetExtension(
                   TestFixture::TestMessageSetExtension1::message_set_extension)
               .i());
  EXPECT_EQ(
      "foo",
      message_set
          .GetExtension(
              TestFixture::TestMessageSetExtension2::message_set_extension)
          .str());

  ASSERT_EQ(1, message_set.unknown_fields().field_count());
  ASSERT_EQ(UnknownField::TYPE_LENGTH_DELIMITED,
            message_set.unknown_fields().field(0).type());
  EXPECT_EQ("bar", message_set.unknown_fields().field(0).length_delimited());

  // Also parse using WireFormat.
  typename TestFixture::TestMessageSet dynamic_message_set;
  io::CodedInputStream input(reinterpret_cast<const uint8_t*>(data.data()),
                             data.size());
  ASSERT_TRUE(WireFormat::ParseAndMergePartial(&input, &dynamic_message_set));
  EXPECT_EQ(message_set.DebugString(), dynamic_message_set.DebugString());
}

TYPED_TEST_P(WireFormatTest, MessageSetUnknownButValidTypeId) {
  const char encoded[] = {
      013,     // 1: SGROUP
      032, 2,  // 3:LEN 2
      010, 0,  // 1:0
      020, 4,  // 2:4
      014      // 1: EGROUP
  };
  typename TestFixture::TestMessageSet message;
  EXPECT_TRUE(message.ParseFromArray(encoded, sizeof(encoded)));
}

TYPED_TEST_P(WireFormatTest, MessageSetInvalidTypeId) {
  // "type_id" is 0 and should fail to parse.
  const char encoded[] = {
      013,     // 1: SGROUP
      032, 2,  // 3:LEN 2
      010, 0,  // 1:0
      020, 0,  // 2:0
      014      // 1: EGROUP
  };
  typename TestFixture::TestMessageSet message;
  EXPECT_FALSE(message.ParseFromArray(encoded, sizeof(encoded)));
}

TYPED_TEST_P(WireFormatTest, MessageSetNonCanonInvalidTypeId) {
  // "type_id" is 0 and should fail to parse. uint8_t is used to silence
  // complaints about narrowing conversion.
  const uint8_t encoded[] = {
      013,                               // 1: SGROUP
      032, 0,                            // 3:LEN 0
      020, 0x80, 0x80, 0x80, 0x80, 020,  // 2: long-form:2 0
      014                                // 1: EGROUP
  };
  typename TestFixture::TestMessageSet message;
  EXPECT_FALSE(message.ParseFromArray(encoded, sizeof(encoded)));
}

inline std::string BuildMessageSetItemStart() {
  std::string data;
  {
    io::StringOutputStream output_stream(&data);
    io::CodedOutputStream coded_output(&output_stream);
    coded_output.WriteTag(WireFormatLite::kMessageSetItemStartTag);
  }
  return data;
}

inline std::string BuildMessageSetItemEnd() {
  std::string data;
  {
    io::StringOutputStream output_stream(&data);
    io::CodedOutputStream coded_output(&output_stream);
    coded_output.WriteTag(WireFormatLite::kMessageSetItemEndTag);
  }
  return data;
}

inline std::string BuildMessageSetItemTypeId(int extension_number) {
  std::string data;
  {
    io::StringOutputStream output_stream(&data);
    io::CodedOutputStream coded_output(&output_stream);
    WireFormatLite::WriteUInt32(WireFormatLite::kMessageSetTypeIdNumber,
                                extension_number, &coded_output);
  }
  return data;
}

TYPED_TEST_P(WireFormatTest, ParseMessageSetWithAnyTagOrder) {
  std::string start = BuildMessageSetItemStart();
  std::string end = BuildMessageSetItemEnd();
  std::string id = BuildMessageSetItemTypeId(
      TestFixture::TestMessageSetExtension1::descriptor()
          ->extension(0)
          ->number());
  std::string message = this->BuildMessageSetTestExtension1();

  this->ValidateTestMessageSet("id + message", start + id + message + end);
  this->ValidateTestMessageSet("message + id", start + message + id + end);
}

TYPED_TEST_P(WireFormatTest, ParseMessageSetWithDuplicateTags) {
  std::string start = BuildMessageSetItemStart();
  std::string end = BuildMessageSetItemEnd();
  std::string id = BuildMessageSetItemTypeId(
      TestFixture::TestMessageSetExtension1::descriptor()
          ->extension(0)
          ->number());
  std::string other_id = BuildMessageSetItemTypeId(123456);
  std::string message = this->BuildMessageSetTestExtension1();
  std::string other_message = this->BuildMessageSetTestExtension1(321);

  // Double id
  this->ValidateTestMessageSet("id + other_id + message",
                               start + id + other_id + message + end);
  this->ValidateTestMessageSet("id + message + other_id",
                               start + id + message + other_id + end);
  this->ValidateTestMessageSet("message + id + other_id",
                               start + message + id + other_id + end);
  // Double message
  this->ValidateTestMessageSet("id + message + other_message",
                               start + id + message + other_message + end);
  this->ValidateTestMessageSet("message + id + other_message",
                               start + message + id + other_message + end);
  this->ValidateTestMessageSet("message + other_message + id",
                               start + message + other_message + id + end);
}

TYPED_TEST_P(WireFormatTest, ParseMessageSetWithDeepRecReverseOrder) {
  std::string data;
  {
    typename TestFixture::TestMessageSet message_set;
    typename TestFixture::TestMessageSet* mset = &message_set;
    for (int i = 0; i < 200; i++) {
      auto m = mset->MutableExtension(
          TestFixture::TestMessageSetExtension1::message_set_extension);
      m->set_i(i);
      mset = m->mutable_recursive();
    }
    message_set.ByteSizeLong();
    // Serialize with reverse payload tag order
    io::StringOutputStream output_stream(&data);
    io::CodedOutputStream coded_output(&output_stream);
    this->SerializeReverseOrder(message_set, &coded_output);
  }
  typename TestFixture::TestMessageSet message_set;
  EXPECT_FALSE(message_set.ParseFromString(data));
}

TYPED_TEST_P(WireFormatTest, ParseFailMalformedMessageSet) {
  constexpr int kDepth = 5;
  std::string data;
  {
    typename TestFixture::TestMessageSet message_set;
    typename TestFixture::TestMessageSet* mset = &message_set;
    for (int i = 0; i < kDepth; i++) {
      auto m = mset->MutableExtension(
          TestFixture::TestMessageSetExtension1::message_set_extension);
      m->set_i(i);
      mset = m->mutable_recursive();
    }
    auto m = mset->MutableExtension(
        TestFixture::TestMessageSetExtension1::message_set_extension);
    // -1 becomes \xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x1
    m->set_i(-1);

    EXPECT_TRUE(message_set.SerializeToString(&data));
    // Make the proto mal-formed.
    data[data.size() - 2 - kDepth] = 0xFF;
  }

  typename TestFixture::TestMessageSet message_set;
  EXPECT_FALSE(message_set.ParseFromString(data));
}

TYPED_TEST_P(WireFormatTest, ParseFailMalformedMessageSetReverseOrder) {
  constexpr int kDepth = 5;
  std::string data;
  {
    typename TestFixture::TestMessageSet message_set;
    typename TestFixture::TestMessageSet* mset = &message_set;
    for (int i = 0; i < kDepth; i++) {
      auto m = mset->MutableExtension(
          TestFixture::TestMessageSetExtension1::message_set_extension);
      m->set_i(i);
      mset = m->mutable_recursive();
    }
    auto m = mset->MutableExtension(
        TestFixture::TestMessageSetExtension1::message_set_extension);
    // -1 becomes \xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x1
    m->set_i(-1);
    // SerializeReverseOrder() assumes "recursive" is always present.
    m->mutable_recursive();

    message_set.ByteSizeLong();

    // Serialize with reverse payload tag order
    io::StringOutputStream output_stream(&data);
    io::CodedOutputStream coded_output(&output_stream);
    this->SerializeReverseOrder(message_set, &coded_output);
  }

  // Make varint for -1 malformed.
  data[data.size() - 5 * (kDepth + 1) - 4] = 0xFF;

  typename TestFixture::TestMessageSet message_set;
  EXPECT_FALSE(message_set.ParseFromString(data));
}

TYPED_TEST_P(WireFormatTest, ParseBrokenMessageSet) {
  typename TestFixture::TestMessageSet message_set;
  std::string input("goodbye");  // Invalid wire format data.
  EXPECT_FALSE(message_set.ParseFromString(input));
}

TYPED_TEST_P(WireFormatTest, RecursionLimit) {
  typename TestFixture::TestRecursiveMessage message;
  message.mutable_a()->mutable_a()->mutable_a()->mutable_a()->set_i(1);
  std::string data;
  message.SerializeToString(&data);

  {
    io::ArrayInputStream raw_input(data.data(), data.size());
    io::CodedInputStream input(&raw_input);
    input.SetRecursionLimit(4);
    typename TestFixture::TestRecursiveMessage message2;
    EXPECT_TRUE(message2.ParseFromCodedStream(&input));
  }

  {
    io::ArrayInputStream raw_input(data.data(), data.size());
    io::CodedInputStream input(&raw_input);
    input.SetRecursionLimit(3);
    typename TestFixture::TestRecursiveMessage message2;
    EXPECT_FALSE(message2.ParseFromCodedStream(&input));
  }
}

TYPED_TEST_P(WireFormatTest, LargeRecursionLimit) {
  const int kLargeLimit = io::CodedInputStream::GetDefaultRecursionLimit() + 50;
  typename TestFixture::TestRecursiveMessage src, dst, *a;
  a = src.mutable_a();
  for (int i = 0; i < kLargeLimit - 1; i++) {
    a = a->mutable_a();
  }
  a->set_i(1);

  std::string data = src.SerializeAsString();
  {
    // Parse with default recursion limit. Should fail.
    io::ArrayInputStream raw_input(data.data(), data.size());
    io::CodedInputStream input(&raw_input);
    ASSERT_FALSE(dst.ParseFromCodedStream(&input));
  }

  {
    // Parse with custom recursion limit. Should pass.
    io::ArrayInputStream raw_input(data.data(), data.size());
    io::CodedInputStream input(&raw_input);
    input.SetRecursionLimit(kLargeLimit);
    ASSERT_TRUE(dst.ParseFromCodedStream(&input));
  }

  // Verifies the recursion depth.
  int depth = 1;
  a = dst.mutable_a();
  while (a->has_a()) {
    a = a->mutable_a();
    depth++;
  }

  EXPECT_EQ(a->i(), 1);
  EXPECT_EQ(depth, kLargeLimit);
}

TYPED_TEST_P(WireFormatTest, UnknownFieldRecursionLimit) {
  typename TestFixture::TestEmptyMessage message;
  message.mutable_unknown_fields()
      ->AddGroup(1234)
      ->AddGroup(1234)
      ->AddGroup(1234)
      ->AddGroup(1234)
      ->AddVarint(1234, 123);
  std::string data;
  message.SerializeToString(&data);

  {
    io::ArrayInputStream raw_input(data.data(), data.size());
    io::CodedInputStream input(&raw_input);
    input.SetRecursionLimit(4);
    typename TestFixture::TestEmptyMessage message2;
    EXPECT_TRUE(message2.ParseFromCodedStream(&input));
  }

  {
    io::ArrayInputStream raw_input(data.data(), data.size());
    io::CodedInputStream input(&raw_input);
    input.SetRecursionLimit(3);
    typename TestFixture::TestEmptyMessage message2;
    EXPECT_FALSE(message2.ParseFromCodedStream(&input));
  }
}

TYPED_TEST_P(WireFormatTest, ZigZag) {
  // shorthands to avoid excessive line-wrapping
  auto ZigZagEncode32 = [](int32_t x) {
    return WireFormatLite::ZigZagEncode32(x);
  };
  auto ZigZagDecode32 = [](uint32_t x) {
    return WireFormatLite::ZigZagDecode32(x);
  };
  auto ZigZagEncode64 = [](int64_t x) {
    return WireFormatLite::ZigZagEncode64(x);
  };
  auto ZigZagDecode64 = [](uint64_t x) {
    return WireFormatLite::ZigZagDecode64(x);
  };

  EXPECT_EQ(0u, ZigZagEncode32(0));
  EXPECT_EQ(1u, ZigZagEncode32(-1));
  EXPECT_EQ(2u, ZigZagEncode32(1));
  EXPECT_EQ(3u, ZigZagEncode32(-2));
  EXPECT_EQ(0x7FFFFFFEu, ZigZagEncode32(0x3FFFFFFF));
  EXPECT_EQ(0x7FFFFFFFu, ZigZagEncode32(0xC0000000));
  EXPECT_EQ(0xFFFFFFFEu, ZigZagEncode32(0x7FFFFFFF));
  EXPECT_EQ(0xFFFFFFFFu, ZigZagEncode32(0x80000000));

  EXPECT_EQ(0, ZigZagDecode32(0u));
  EXPECT_EQ(-1, ZigZagDecode32(1u));
  EXPECT_EQ(1, ZigZagDecode32(2u));
  EXPECT_EQ(-2, ZigZagDecode32(3u));
  EXPECT_EQ(0x3FFFFFFF, ZigZagDecode32(0x7FFFFFFEu));
  EXPECT_EQ(0xC0000000, ZigZagDecode32(0x7FFFFFFFu));
  EXPECT_EQ(0x7FFFFFFF, ZigZagDecode32(0xFFFFFFFEu));
  EXPECT_EQ(0x80000000, ZigZagDecode32(0xFFFFFFFFu));

  EXPECT_EQ(0u, ZigZagEncode64(0));
  EXPECT_EQ(1u, ZigZagEncode64(-1));
  EXPECT_EQ(2u, ZigZagEncode64(1));
  EXPECT_EQ(3u, ZigZagEncode64(-2));
  EXPECT_EQ(0x000000007FFFFFFEu, ZigZagEncode64(0x000000003FFFFFFF));
  EXPECT_EQ(0x000000007FFFFFFFu,
            ZigZagEncode64(absl::bit_cast<int64_t>(0xFFFFFFFFC0000000)));
  EXPECT_EQ(0x00000000FFFFFFFEu, ZigZagEncode64(0x000000007FFFFFFF));
  EXPECT_EQ(0x00000000FFFFFFFFu,
            ZigZagEncode64(absl::bit_cast<int64_t>(0xFFFFFFFF80000000)));
  EXPECT_EQ(0xFFFFFFFFFFFFFFFEu, ZigZagEncode64(0x7FFFFFFFFFFFFFFF));
  EXPECT_EQ(0xFFFFFFFFFFFFFFFFu,
            ZigZagEncode64(absl::bit_cast<int64_t>(0x8000000000000000)));

  EXPECT_EQ(0, ZigZagDecode64(0u));
  EXPECT_EQ(-1, ZigZagDecode64(1u));
  EXPECT_EQ(1, ZigZagDecode64(2u));
  EXPECT_EQ(-2, ZigZagDecode64(3u));
  EXPECT_EQ(0x000000003FFFFFFF, ZigZagDecode64(0x000000007FFFFFFEu));
  EXPECT_EQ(absl::bit_cast<int64_t>(0xFFFFFFFFC0000000),
            ZigZagDecode64(0x000000007FFFFFFFu));
  EXPECT_EQ(0x000000007FFFFFFF, ZigZagDecode64(0x00000000FFFFFFFEu));
  EXPECT_EQ(absl::bit_cast<int64_t>(0xFFFFFFFF80000000),
            ZigZagDecode64(0x00000000FFFFFFFFu));
  EXPECT_EQ(0x7FFFFFFFFFFFFFFF, ZigZagDecode64(0xFFFFFFFFFFFFFFFEu));
  EXPECT_EQ(absl::bit_cast<int64_t>(0x8000000000000000),
            ZigZagDecode64(0xFFFFFFFFFFFFFFFFu));

  // Some easier-to-verify round-trip tests.  The inputs (other than 0, 1, -1)
  // were chosen semi-randomly via keyboard bashing.
  EXPECT_EQ(0, ZigZagDecode32(ZigZagEncode32(0)));
  EXPECT_EQ(1, ZigZagDecode32(ZigZagEncode32(1)));
  EXPECT_EQ(-1, ZigZagDecode32(ZigZagEncode32(-1)));
  EXPECT_EQ(14927, ZigZagDecode32(ZigZagEncode32(14927)));
  EXPECT_EQ(-3612, ZigZagDecode32(ZigZagEncode32(-3612)));

  EXPECT_EQ(0, ZigZagDecode64(ZigZagEncode64(0)));
  EXPECT_EQ(1, ZigZagDecode64(ZigZagEncode64(1)));
  EXPECT_EQ(-1, ZigZagDecode64(ZigZagEncode64(-1)));
  EXPECT_EQ(14927, ZigZagDecode64(ZigZagEncode64(14927)));
  EXPECT_EQ(-3612, ZigZagDecode64(ZigZagEncode64(-3612)));

  EXPECT_EQ(856912304801416, ZigZagDecode64(ZigZagEncode64(856912304801416)));
  EXPECT_EQ(-75123905439571256,
            ZigZagDecode64(ZigZagEncode64(-75123905439571256)));
}

TYPED_TEST_P(WireFormatTest, RepeatedScalarsDifferentTagSizes) {
  // At one point checks would trigger when parsing repeated fixed scalar
  // fields.
  typename TestFixture::TestRepeatedScalarDifferentTagSizes msg1, msg2;
  for (int i = 0; i < 100; ++i) {
    msg1.add_repeated_fixed32(i);
    msg1.add_repeated_int32(i);
    msg1.add_repeated_fixed64(i);
    msg1.add_repeated_int64(i);
    msg1.add_repeated_float(i);
    msg1.add_repeated_uint64(i);
  }

  // Make sure that we have a variety of tag sizes.
  const Descriptor* desc = msg1.GetDescriptor();
  const FieldDescriptor* field;
  field = desc->FindFieldByName("repeated_fixed32");
  ASSERT_TRUE(field != nullptr);
  ASSERT_EQ(1, WireFormat::TagSize(field->number(), field->type()));
  field = desc->FindFieldByName("repeated_int32");
  ASSERT_TRUE(field != nullptr);
  ASSERT_EQ(1, WireFormat::TagSize(field->number(), field->type()));
  field = desc->FindFieldByName("repeated_fixed64");
  ASSERT_TRUE(field != nullptr);
  ASSERT_EQ(2, WireFormat::TagSize(field->number(), field->type()));
  field = desc->FindFieldByName("repeated_int64");
  ASSERT_TRUE(field != nullptr);
  ASSERT_EQ(2, WireFormat::TagSize(field->number(), field->type()));
  field = desc->FindFieldByName("repeated_float");
  ASSERT_TRUE(field != nullptr);
  ASSERT_EQ(3, WireFormat::TagSize(field->number(), field->type()));
  field = desc->FindFieldByName("repeated_uint64");
  ASSERT_TRUE(field != nullptr);
  ASSERT_EQ(3, WireFormat::TagSize(field->number(), field->type()));

  EXPECT_TRUE(msg2.ParseFromString(msg1.SerializeAsString()));
  EXPECT_EQ(msg1.DebugString(), msg2.DebugString());
}

TYPED_TEST_P(WireFormatTest, CompatibleTypes) {
  const int64_t data = 0x100000000LL;
  typename TestFixture::Int64Message msg1;
  msg1.set_data(data);
  std::string serialized;
  msg1.SerializeToString(&serialized);

  // Test int64 is compatible with bool
  typename TestFixture::BoolMessage msg2;
  ASSERT_TRUE(msg2.ParseFromString(serialized));
  ASSERT_EQ(static_cast<bool>(data), msg2.data());

  // Test int64 is compatible with uint64
  typename TestFixture::Uint64Message msg3;
  ASSERT_TRUE(msg3.ParseFromString(serialized));
  ASSERT_EQ(static_cast<uint64_t>(data), msg3.data());

  // Test int64 is compatible with int32
  typename TestFixture::Int32Message msg4;
  ASSERT_TRUE(msg4.ParseFromString(serialized));
  ASSERT_EQ(static_cast<int32_t>(data), msg4.data());

  // Test int64 is compatible with uint32
  typename TestFixture::Uint32Message msg5;
  ASSERT_TRUE(msg5.ParseFromString(serialized));
  ASSERT_EQ(static_cast<uint32_t>(data), msg5.data());
}


TYPED_TEST_P(WireFormatTest, MessageSetLargeTypeId) {
  std::string s;
  {
    typename TestFixture::RawMessageSet ms;
    auto item = ms.add_item();
    item->set_type_id((1 << 29) +
                      10);  // Type_id bigger than normal range of fieldnums
    item->set_message("");
    ms.SerializeToString(&s);
  }
  {
    typename TestFixture::TestMessageSet ms;
    ms.ParseFromString(s);
    EXPECT_FALSE(ms.unknown_fields().empty());
    // No truncation of type_id
    EXPECT_TRUE(TestUtil::EqualsToSerialized(ms, s));
  }
  {
    typename TestFixture::TestMessageSet ms;
    const char* ptr;
    google::protobuf::internal::ParseContext ctx(100, false, &ptr, s);
    ptr = WireFormat::_InternalParse(&ms, ptr, &ctx);
    ASSERT_TRUE(ptr != nullptr && ctx.EndedAtLimit());
    EXPECT_FALSE(ms.unknown_fields().empty());
    // No truncation of type_id
    EXPECT_TRUE(TestUtil::EqualsToSerialized(ms, s));
  }
}

REGISTER_TYPED_TEST_SUITE_P(
    WireFormatTest, Parse, ParseExtensions, ParsePacked,
    ParsePackedFromUnpacked, ParseUnpackedFromPacked, ParsePackedExtensions,
    ParseOneof, OneofOnlySetLast, ByteSize, ByteSizeExtensions, ByteSizePacked,
    ByteSizePackedExtensions, ByteSizeOneof, Serialize, SerializeExtensions,
    SerializeFieldsAndExtensions, SerializeOneof, ParseMultipleExtensionRanges,
    SerializeMessageSet, SerializeMessageSetVariousWaysAreEqual,
    ParseMessageSet, MessageSetUnknownButValidTypeId, MessageSetInvalidTypeId,
    MessageSetNonCanonInvalidTypeId, CompatibleTypes, LargeRecursionLimit,
    ParseBrokenMessageSet, ParseFailMalformedMessageSet,
    ParseFailMalformedMessageSetReverseOrder, ParseMessageSetWithAnyTagOrder,
    ParseMessageSetWithDeepRecReverseOrder, ParseMessageSetWithDuplicateTags,
    RecursionLimit, RepeatedScalarsDifferentTagSizes,
    UnknownFieldRecursionLimit, ZigZag,
    MessageSetLargeTypeId);

template <typename T>
class Proto3PrimitiveRepeatedWireFormatTest;

template <typename TestAllTypesT, typename TestUnpackedTypesT>
class Proto3PrimitiveRepeatedWireFormatTest<void(
    TestAllTypesT, TestUnpackedTypesT)> : public ::testing::Test {
 protected:
  using TestAllTypes = TestAllTypesT;
  using TestUnpackedTypes = TestUnpackedTypesT;

  Proto3PrimitiveRepeatedWireFormatTest()
      : packedTestAllTypes_(
            "\xFA\x01\x01\x01"
            "\x82\x02\x01\x01"
            "\x8A\x02\x01\x01"
            "\x92\x02\x01\x01"
            "\x9A\x02\x01\x02"
            "\xA2\x02\x01\x02"
            "\xAA\x02\x04\x01\x00\x00\x00"
            "\xB2\x02\x08\x01\x00\x00\x00\x00\x00\x00\x00"
            "\xBA\x02\x04\x01\x00\x00\x00"
            "\xC2\x02\x08\x01\x00\x00\x00\x00\x00\x00\x00"
            "\xCA\x02\x04\x00\x00\x80\x3f"
            "\xD2\x02\x08\x00\x00\x00\x00\x00\x00\xf0\x3f"
            "\xDA\x02\x01\x01"
            "\x9A\x03\x01\x01",
            86),
        packedTestUnpackedTypes_(
            "\x0A\x01\x01"
            "\x12\x01\x01"
            "\x1A\x01\x01"
            "\x22\x01\x01"
            "\x2A\x01\x02"
            "\x32\x01\x02"
            "\x3A\x04\x01\x00\x00\x00"
            "\x42\x08\x01\x00\x00\x00\x00\x00\x00\x00"
            "\x4A\x04\x01\x00\x00\x00"
            "\x52\x08\x01\x00\x00\x00\x00\x00\x00\x00"
            "\x5A\x04\x00\x00\x80\x3f"
            "\x62\x08\x00\x00\x00\x00\x00\x00\xf0\x3f"
            "\x6A\x01\x01"
            "\x72\x01\x01",
            72),
        unpackedTestAllTypes_(
            "\xF8\x01\x01"
            "\x80\x02\x01"
            "\x88\x02\x01"
            "\x90\x02\x01"
            "\x98\x02\x02"
            "\xA0\x02\x02"
            "\xAD\x02\x01\x00\x00\x00"
            "\xB1\x02\x01\x00\x00\x00\x00\x00\x00\x00"
            "\xBD\x02\x01\x00\x00\x00"
            "\xC1\x02\x01\x00\x00\x00\x00\x00\x00\x00"
            "\xCD\x02\x00\x00\x80\x3f"
            "\xD1\x02\x00\x00\x00\x00\x00\x00\xf0\x3f"
            "\xD8\x02\x01"
            "\x98\x03\x01",
            72),
        unpackedTestUnpackedTypes_(
            "\x08\x01"
            "\x10\x01"
            "\x18\x01"
            "\x20\x01"
            "\x28\x02"
            "\x30\x02"
            "\x3D\x01\x00\x00\x00"
            "\x41\x01\x00\x00\x00\x00\x00\x00\x00"
            "\x4D\x01\x00\x00\x00"
            "\x51\x01\x00\x00\x00\x00\x00\x00\x00"
            "\x5D\x00\x00\x80\x3f"
            "\x61\x00\x00\x00\x00\x00\x00\xf0\x3f"
            "\x68\x01"
            "\x70\x01",
            58) {}
  template <class Proto>
  void SetProto3PrimitiveRepeatedFields(Proto* message) {
    message->add_repeated_int32(1);
    message->add_repeated_int64(1);
    message->add_repeated_uint32(1);
    message->add_repeated_uint64(1);
    message->add_repeated_sint32(1);
    message->add_repeated_sint64(1);
    message->add_repeated_fixed32(1);
    message->add_repeated_fixed64(1);
    message->add_repeated_sfixed32(1);
    message->add_repeated_sfixed64(1);
    message->add_repeated_float(1.0);
    message->add_repeated_double(1.0);
    message->add_repeated_bool(true);
    message->add_repeated_nested_enum(TestAllTypes::FOO);
  }

  template <class Proto>
  void ExpectProto3PrimitiveRepeatedFieldsSet(const Proto& message) {
    EXPECT_EQ(1, message.repeated_int32(0));
    EXPECT_EQ(1, message.repeated_int64(0));
    EXPECT_EQ(1, message.repeated_uint32(0));
    EXPECT_EQ(1, message.repeated_uint64(0));
    EXPECT_EQ(1, message.repeated_sint32(0));
    EXPECT_EQ(1, message.repeated_sint64(0));
    EXPECT_EQ(1, message.repeated_fixed32(0));
    EXPECT_EQ(1, message.repeated_fixed64(0));
    EXPECT_EQ(1, message.repeated_sfixed32(0));
    EXPECT_EQ(1, message.repeated_sfixed64(0));
    EXPECT_EQ(1.0, message.repeated_float(0));
    EXPECT_EQ(1.0, message.repeated_double(0));
    EXPECT_EQ(true, message.repeated_bool(0));
    EXPECT_EQ(TestAllTypes::FOO, message.repeated_nested_enum(0));
  }

  template <class Proto>
  void TestSerialization(Proto* message, const std::string& expected) {
    SetProto3PrimitiveRepeatedFields(message);

    size_t size = message->ByteSizeLong();

    // Serialize using the generated code.
    std::string generated_data;
    {
      io::StringOutputStream raw_output(&generated_data);
      io::CodedOutputStream output(&raw_output);
      message->SerializeWithCachedSizes(&output);
      ASSERT_FALSE(output.HadError());
    }
    EXPECT_TRUE(TestUtil::EqualsToSerialized(*message, generated_data));

    // Serialize using the dynamic code.
    std::string dynamic_data;
    {
      io::StringOutputStream raw_output(&dynamic_data);
      io::CodedOutputStream output(&raw_output);
      WireFormat::SerializeWithCachedSizes(*message, size, &output);
      ASSERT_FALSE(output.HadError());
    }
    EXPECT_TRUE(expected == dynamic_data);
  }

  template <class Proto>
  void TestParsing(Proto* message, const std::string& compatible_data) {
    message->Clear();
    message->ParseFromString(compatible_data);
    ExpectProto3PrimitiveRepeatedFieldsSet(*message);

    message->Clear();
    io::CodedInputStream input(
        reinterpret_cast<const uint8_t*>(compatible_data.data()),
        compatible_data.size());
    WireFormat::ParseAndMergePartial(&input, message);
    ExpectProto3PrimitiveRepeatedFieldsSet(*message);
  }

  const std::string packedTestAllTypes_;
  const std::string packedTestUnpackedTypes_;
  const std::string unpackedTestAllTypes_;
  const std::string unpackedTestUnpackedTypes_;
};

TYPED_TEST_SUITE_P(Proto3PrimitiveRepeatedWireFormatTest);

TYPED_TEST_P(Proto3PrimitiveRepeatedWireFormatTest, Proto3PrimitiveRepeated) {
  typename TestFixture::TestAllTypes packed_message;
  typename TestFixture::TestUnpackedTypes unpacked_message;
  this->TestSerialization(&packed_message, this->packedTestAllTypes_);
  this->TestParsing(&packed_message, this->packedTestAllTypes_);
  this->TestParsing(&packed_message, this->unpackedTestAllTypes_);
  this->TestSerialization(&unpacked_message, this->unpackedTestUnpackedTypes_);
  this->TestParsing(&unpacked_message, this->packedTestUnpackedTypes_);
  this->TestParsing(&unpacked_message, this->unpackedTestUnpackedTypes_);
}

REGISTER_TYPED_TEST_SUITE_P(Proto3PrimitiveRepeatedWireFormatTest,
                            Proto3PrimitiveRepeated);

template <typename T>
class WireFormatInvalidInputTest : public testing::Test,
                                   protected TestUtil::TestUtilTraits<T> {
 protected:
  using TestAllTypes = typename TestUtil::TestUtilTraits<T>::TestAllTypes;

  // Make a serialized TestAllTypes in which the field optional_nested_message
  // contains exactly the given bytes, which may be invalid.
  std::string MakeInvalidEmbeddedMessage(const char* bytes, int size) {
    const FieldDescriptor* field =
        TestAllTypes::descriptor()->FindFieldByName("optional_nested_message");
    ABSL_CHECK(field != nullptr);

    std::string result;

    {
      io::StringOutputStream raw_output(&result);
      io::CodedOutputStream output(&raw_output);

      WireFormatLite::WriteBytes(field->number(), std::string(bytes, size),
                                 &output);
    }

    return result;
  }

  // Make a serialized TestAllTypes in which the field optionalgroup
  // contains exactly the given bytes -- which may be invalid -- and
  // possibly no end tag.
  std::string MakeInvalidGroup(const char* bytes, int size,
                               bool include_end_tag) {
    const FieldDescriptor* field =
        TestAllTypes::descriptor()->FindFieldByName("optionalgroup");
    ABSL_CHECK(field != nullptr);

    std::string result;

    {
      io::StringOutputStream raw_output(&result);
      io::CodedOutputStream output(&raw_output);

      output.WriteVarint32(WireFormat::MakeTag(field));
      output.WriteString(std::string(bytes, size));
      if (include_end_tag) {
        output.WriteVarint32(WireFormatLite::MakeTag(
            field->number(), WireFormatLite::WIRETYPE_END_GROUP));
      }
    }

    return result;
  }
};
TYPED_TEST_SUITE_P(WireFormatInvalidInputTest);

TYPED_TEST_P(WireFormatInvalidInputTest, InvalidSubMessage) {
  typename TestFixture::TestAllTypes message;

  // Control case.
  EXPECT_TRUE(message.ParseFromString(this->MakeInvalidEmbeddedMessage("", 0)));

  // The byte is a valid varint, but not a valid tag (zero).
  EXPECT_FALSE(
      message.ParseFromString(this->MakeInvalidEmbeddedMessage("\0", 1)));

  // The byte is a malformed varint.
  EXPECT_FALSE(
      message.ParseFromString(this->MakeInvalidEmbeddedMessage("\200", 1)));

  // The byte is an endgroup tag, but we aren't parsing a group.
  EXPECT_FALSE(
      message.ParseFromString(this->MakeInvalidEmbeddedMessage("\014", 1)));

  // The byte is a valid varint but not a valid tag (bad wire type).
  EXPECT_FALSE(
      message.ParseFromString(this->MakeInvalidEmbeddedMessage("\017", 1)));
}

TYPED_TEST_P(WireFormatInvalidInputTest, InvalidMessageWithExtraZero) {
  std::string data;
  {
    // Serialize a valid proto
    typename TestFixture::TestAllTypes message;
    message.set_optional_int32(1);
    message.SerializeToString(&data);
    data.push_back(0);  // Append invalid zero tag
  }

  // Control case.
  {
    io::ArrayInputStream ais(data.data(), data.size());
    io::CodedInputStream is(&ais);
    typename TestFixture::TestAllTypes message;
    // It should fail but currently passes.
    EXPECT_TRUE(message.MergePartialFromCodedStream(&is));
    // Parsing from the string should fail.
    EXPECT_FALSE(message.ParseFromString(data));
  }
}

TYPED_TEST_P(WireFormatInvalidInputTest, InvalidGroup) {
  typename TestFixture::TestAllTypes message;

  // Control case.
  EXPECT_TRUE(message.ParseFromString(this->MakeInvalidGroup("", 0, true)));

  // Missing end tag.  Groups cannot end at EOF.
  EXPECT_FALSE(message.ParseFromString(this->MakeInvalidGroup("", 0, false)));

  // The byte is a valid varint, but not a valid tag (zero).
  EXPECT_FALSE(message.ParseFromString(this->MakeInvalidGroup("\0", 1, false)));

  // The byte is a malformed varint.
  EXPECT_FALSE(
      message.ParseFromString(this->MakeInvalidGroup("\200", 1, false)));

  // The byte is an endgroup tag, but not the right one for this group.
  EXPECT_FALSE(
      message.ParseFromString(this->MakeInvalidGroup("\014", 1, false)));

  // The byte is a valid varint but not a valid tag (bad wire type).
  EXPECT_FALSE(
      message.ParseFromString(this->MakeInvalidGroup("\017", 1, true)));
}

TYPED_TEST_P(WireFormatInvalidInputTest, InvalidUnknownGroup) {
  // Use TestEmptyMessage so that the group made by MakeInvalidGroup will not
  // be a known tag number.
  typename TestFixture::TestEmptyMessage message;

  // Control case.
  EXPECT_TRUE(message.ParseFromString(this->MakeInvalidGroup("", 0, true)));

  // Missing end tag.  Groups cannot end at EOF.
  EXPECT_FALSE(message.ParseFromString(this->MakeInvalidGroup("", 0, false)));

  // The byte is a valid varint, but not a valid tag (zero).
  EXPECT_FALSE(message.ParseFromString(this->MakeInvalidGroup("\0", 1, false)));

  // The byte is a malformed varint.
  EXPECT_FALSE(
      message.ParseFromString(this->MakeInvalidGroup("\200", 1, false)));

  // The byte is an endgroup tag, but not the right one for this group.
  EXPECT_FALSE(
      message.ParseFromString(this->MakeInvalidGroup("\014", 1, false)));

  // The byte is a valid varint but not a valid tag (bad wire type).
  EXPECT_FALSE(
      message.ParseFromString(this->MakeInvalidGroup("\017", 1, true)));
}

TYPED_TEST_P(WireFormatInvalidInputTest, InvalidStringInUnknownGroup) {
  // Test a bug fix:  SkipMessage should fail if the message contains a
  // string whose length would extend beyond the message end.

  typename TestFixture::TestAllTypes message;
  message.set_optional_string("foo foo foo foo");
  std::string data;
  message.SerializeToString(&data);

  // Chop some bytes off the end.
  data.resize(data.size() - 4);

  // Try to skip it.  Note that the bug was only present when parsing to an
  // UnknownFieldSet.
  io::ArrayInputStream raw_input(data.data(), data.size());
  io::CodedInputStream coded_input(&raw_input);
  UnknownFieldSet unknown_fields;
  EXPECT_FALSE(WireFormat::SkipMessage(&coded_input, &unknown_fields));
}

REGISTER_TYPED_TEST_SUITE_P(WireFormatInvalidInputTest,
                            InvalidStringInUnknownGroup, InvalidUnknownGroup,
                            InvalidGroup, InvalidMessageWithExtraZero,
                            InvalidSubMessage);

// Test differences between string and bytes.
// Value of a string type must be valid UTF-8 string.  When UTF-8
// validation is enabled (GOOGLE_PROTOBUF_UTF8_VALIDATION_ENABLED):
// WriteInvalidUTF8String:  see error message.
// ReadInvalidUTF8String:  see error message.
// WriteValidUTF8String: fine.
// ReadValidUTF8String:  fine.
// WriteAnyBytes: fine.
// ReadAnyBytes: fine.
template <typename T>
bool WriteMessage(absl::string_view value, T* message,
                  std::string* wire_buffer) {
  message->set_data(value);
  wire_buffer->clear();
  message->AppendToString(wire_buffer);
  return (!wire_buffer->empty());
}

template <typename T>
bool ReadMessage(const std::string& wire_buffer, T* message) {
  return message->ParseFromArray(wire_buffer.data(), wire_buffer.size());
}

template <typename T>
class Utf8ValidationTest : public ::testing::Test,
                           protected TestUtil::TestUtilTraits<T> {
 protected:
  Utf8ValidationTest() {}
  ~Utf8ValidationTest() override {}

  static constexpr absl::string_view kInvalidUTF8String =
      "Invalid UTF-8: \xA0\xB0\xC0\xD0";
  // This used to be "Valid UTF-8: \x01\x02\u8C37\u6B4C", but MSVC seems to
  // interpret \u differently from GCC.
  static constexpr absl::string_view kValidUTF8String =
      "Valid UTF-8: \x01\x02\350\260\267\346\255\214";

  void SetUp() override {
  }

};
TYPED_TEST_SUITE_P(Utf8ValidationTest);

TYPED_TEST_P(Utf8ValidationTest, WriteInvalidUTF8String) {
  std::string wire_buffer;
  typename TestFixture::OneString input;
  std::vector<std::string> errors;
  {
    absl::ScopedMockLog log(absl::MockLogDefault::kDisallowUnexpected);
#ifdef GOOGLE_PROTOBUF_UTF8_VALIDATION_ENABLED
    EXPECT_CALL(
        log,
        Log(absl::LogSeverity::kError, testing::_,
            absl::StrCat("String field '",
                         TestFixture::OneString::descriptor()->full_name(),
                         ".data' contains invalid UTF-8 data when "
                         "serializing a protocol buffer. Use the "
                         "'bytes' type if you intend to send raw bytes. ")));
#else
    EXPECT_CALL(log, Log(absl::LogSeverity::kError, testing::_, testing::_))
        .Times(0);
#endif  // GOOGLE_PROTOBUF_UTF8_VALIDATION_ENABLED
    log.StartCapturingLogs();
    WriteMessage(this->kInvalidUTF8String, &input, &wire_buffer);
  }
}


TYPED_TEST_P(Utf8ValidationTest, ReadInvalidUTF8String) {
  std::string wire_buffer;
  typename TestFixture::OneString input;
  WriteMessage(this->kInvalidUTF8String, &input, &wire_buffer);
  typename TestFixture::OneString output;
  std::vector<std::string> errors;
  {
    absl::ScopedMockLog log(absl::MockLogDefault::kDisallowUnexpected);
#ifdef GOOGLE_PROTOBUF_UTF8_VALIDATION_ENABLED
    EXPECT_CALL(
        log,
        Log(absl::LogSeverity::kError, testing::_,
            absl::StrCat("String field '",
                         TestFixture::OneString::descriptor()->full_name(),
                         ".data' contains invalid UTF-8 data when "
                         "parsing a protocol buffer. Use the "
                         "'bytes' type if you intend to send raw bytes. ")));
#else
    EXPECT_CALL(log, Log(absl::LogSeverity::kError, testing::_, testing::_))
        .Times(0);
#endif  // GOOGLE_PROTOBUF_UTF8_VALIDATION_ENABLED
    log.StartCapturingLogs();
    ReadMessage(wire_buffer, &output);
  }
}


TYPED_TEST_P(Utf8ValidationTest, WriteValidUTF8String) {
  std::string wire_buffer;
  typename TestFixture::OneString input;
  {
    absl::ScopedMockLog log(absl::MockLogDefault::kDisallowUnexpected);
    EXPECT_CALL(log, Log(absl::LogSeverity::kError, testing::_, testing::_))
        .Times(0);
    log.StartCapturingLogs();
    WriteMessage(this->kValidUTF8String, &input, &wire_buffer);
  }
}

TYPED_TEST_P(Utf8ValidationTest, ReadValidUTF8String) {
  std::string wire_buffer;
  typename TestFixture::OneString input;
  WriteMessage(this->kValidUTF8String, &input, &wire_buffer);
  typename TestFixture::OneString output;
  {
    absl::ScopedMockLog log(absl::MockLogDefault::kDisallowUnexpected);
    EXPECT_CALL(log, Log(absl::LogSeverity::kError, testing::_, testing::_))
        .Times(0);
    log.StartCapturingLogs();
    ReadMessage(wire_buffer, &output);
  }
  EXPECT_EQ(input.data(), output.data());
}

// Bytes: anything can pass as bytes, use invalid UTF-8 string to test
TYPED_TEST_P(Utf8ValidationTest, WriteArbitraryBytes) {
  std::string wire_buffer;
  typename TestFixture::OneBytes input;
  {
    absl::ScopedMockLog log(absl::MockLogDefault::kDisallowUnexpected);
    EXPECT_CALL(log, Log(absl::LogSeverity::kError, testing::_, testing::_))
        .Times(0);
    log.StartCapturingLogs();
    WriteMessage(this->kInvalidUTF8String, &input, &wire_buffer);
  }
}

TYPED_TEST_P(Utf8ValidationTest, ReadArbitraryBytes) {
  std::string wire_buffer;
  typename TestFixture::OneBytes input;
  WriteMessage(this->kInvalidUTF8String, &input, &wire_buffer);
  typename TestFixture::OneBytes output;
  {
    absl::ScopedMockLog log(absl::MockLogDefault::kDisallowUnexpected);
    EXPECT_CALL(log, Log(absl::LogSeverity::kError, testing::_, testing::_))
        .Times(0);
    log.StartCapturingLogs();
    ReadMessage(wire_buffer, &output);
  }
  EXPECT_EQ(input.data(), output.data());
}

TYPED_TEST_P(Utf8ValidationTest, ParseRepeatedString) {
  typename TestFixture::MoreBytes input;
  input.add_data(this->kValidUTF8String);
  input.add_data(this->kInvalidUTF8String);
  input.add_data(this->kInvalidUTF8String);
  std::string wire_buffer = input.SerializeAsString();

  typename TestFixture::MoreString output;
  {
    absl::ScopedMockLog log(absl::MockLogDefault::kDisallowUnexpected);
#ifdef GOOGLE_PROTOBUF_UTF8_VALIDATION_ENABLED
    EXPECT_CALL(log, Log(absl::LogSeverity::kError, testing::_, testing::_))
        .Times(2);
#else
    EXPECT_CALL(log, Log(absl::LogSeverity::kError, testing::_, testing::_))
        .Times(0);
#endif  // GOOGLE_PROTOBUF_UTF8_VALIDATION_ENABLED
    log.StartCapturingLogs();
    ReadMessage(wire_buffer, &output);
  }
  EXPECT_EQ(wire_buffer, output.SerializeAsString());
}

// Test the old VerifyUTF8String() function, which may still be called by old
// generated code.
TYPED_TEST_P(Utf8ValidationTest, OldVerifyUTF8String) {
  std::string data(this->kInvalidUTF8String);

  {
    absl::ScopedMockLog log(absl::MockLogDefault::kDisallowUnexpected);
#ifdef GOOGLE_PROTOBUF_UTF8_VALIDATION_ENABLED
    EXPECT_CALL(log, Log(absl::LogSeverity::kError, testing::_,
                         testing::StartsWith(
                             "String field contains invalid UTF-8 data when "
                             "serializing a protocol buffer. Use the "
                             "'bytes' type if you intend to send raw bytes.")));
#else
    EXPECT_CALL(log, Log(absl::LogSeverity::kError, testing::_, testing::_))
        .Times(0);
#endif  // GOOGLE_PROTOBUF_UTF8_VALIDATION_ENABLED
    log.StartCapturingLogs();
    WireFormat::VerifyUTF8String(data.data(), data.size(),
                                 WireFormat::SERIALIZE);
  }
}


REGISTER_TYPED_TEST_SUITE_P(Utf8ValidationTest, WriteInvalidUTF8String,
                            ReadInvalidUTF8String, WriteValidUTF8String,
                            ReadValidUTF8String, WriteArbitraryBytes,
                            ReadArbitraryBytes, ParseRepeatedString,
                            OldVerifyUTF8String);


template <typename T>
class LazyMessageSetsTest;
template <typename Proto2T, typename Proto3T, typename eagerly_parse>
class LazyMessageSetsTest<void(Proto2T, Proto3T, eagerly_parse)>
    : public testing::Test, protected TestUtil::TestUtilTraits<Proto2T> {
 protected:
  using Proto3Arena_NestedTestAllTypes = Proto3T;

  static constexpr bool kEagerlyParseMessageSets = eagerly_parse::value;

  void SetUp() override {
    GTEST_SKIP() << "Disabled.";
  }
};
TYPED_TEST_SUITE_P(LazyMessageSetsTest);

TYPED_TEST_P(LazyMessageSetsTest, RedundantWire) {
  // This is intentionally non-canonical but legal encoding to stress test lazy
  // message parsing and serialization. Note that comments follow protoscope
  // conventions.
  const char encoded_chars[] = {
      "\x0b"              // 1:SGROUP
      "\x10\xb0\xa6\x5e"  // 2: 1545008
      "\x1a\x04"          // 3:LEN 4
      "\x78\x00"          // 15: 0
      "\x78\x00"          // 15: 0 # intentionally redundant
      "\x0c"              // 1:EGROUP
  };
  const absl::string_view encoded{encoded_chars, sizeof(encoded_chars) - 1};
  typename TestFixture::TestMessageSet src, dst;
  ASSERT_TRUE(src.ParsePartialFromString(encoded));
  ASSERT_TRUE(dst.ParsePartialFromString(src.SerializeAsString()));
  size_t byte_size = dst.ByteSizeLong();

  // Serialize using WireFormat.
  std::string result;
  {
    io::StringOutputStream raw_output(&result);
    io::CodedOutputStream output(&raw_output);
    WireFormat::SerializeWithCachedSizes(dst, byte_size, &output);
    ASSERT_FALSE(output.HadError());
  }
  EXPECT_TRUE(dst.ParsePartialFromString(result));
}


REGISTER_TYPED_TEST_SUITE_P(
    LazyMessageSetsTest,
    RedundantWire);

#define PROTOBUF_INSTANTIATE_WIRE_FORMAT_UNITTEST(name, ns_suffix)             \
  INSTANTIATE_TYPED_TEST_SUITE_P(WireFormatTest##name, WireFormatTest,         \
                                 ::proto2_unittest##ns_suffix::TestAllTypes);  \
  INSTANTIATE_TYPED_TEST_SUITE_P(WireFormatInvalidInputTest##name,             \
                                 WireFormatInvalidInputTest,                   \
                                 ::proto2_unittest##ns_suffix::TestAllTypes);  \
  INSTANTIATE_TYPED_TEST_SUITE_P(Utf8ValidationTest##name, Utf8ValidationTest, \
                                 ::proto2_unittest##ns_suffix::TestAllTypes);  \
  INSTANTIATE_TYPED_TEST_SUITE_P(                                              \
      Proto3PrimitiveRepeatedWireFormatTest##name,                             \
      Proto3PrimitiveRepeatedWireFormatTest,                                   \
      void(::proto3_arena_unittest##ns_suffix::TestAllTypes,                   \
           ::proto3_arena_unittest##ns_suffix::TestUnpackedTypes));            \
  INSTANTIATE_TYPED_TEST_SUITE_P(                                              \
      LazyMessageSetsTestEager##name, LazyMessageSetsTest,                     \
      void(::proto2_unittest##ns_suffix::TestAllTypes,                         \
           ::proto3_arena_unittest##ns_suffix::NestedTestAllTypes,             \
           std::true_type));                                                   \
  INSTANTIATE_TYPED_TEST_SUITE_P(                                              \
      LazyMessageSetsTestLazy##name, LazyMessageSetsTest,                      \
      void(::proto2_unittest##ns_suffix::TestAllTypes,                         \
           ::proto3_arena_unittest##ns_suffix::NestedTestAllTypes,             \
           std::false_type))

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_WIRE_FORMAT_UNITTEST_H__
