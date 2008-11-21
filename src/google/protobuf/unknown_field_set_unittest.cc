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
// This test is testing a lot more than just the UnknownFieldSet class.  It
// tests handling of unknown fields throughout the system.

#include <google/protobuf/unknown_field_set.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/unittest.pb.h>
#include <google/protobuf/test_util.h>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/testing/googletest.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {

using internal::WireFormat;

namespace {

class UnknownFieldSetTest : public testing::Test {
 protected:
  virtual void SetUp() {
    descriptor_ = unittest::TestAllTypes::descriptor();
    TestUtil::SetAllFields(&all_fields_);
    all_fields_.SerializeToString(&all_fields_data_);
    ASSERT_TRUE(empty_message_.ParseFromString(all_fields_data_));
    unknown_fields_ = empty_message_.mutable_unknown_fields();
  }

  const UnknownField* GetField(const string& name) {
    const FieldDescriptor* field = descriptor_->FindFieldByName(name);
    if (field == NULL) return NULL;
    return unknown_fields_->FindFieldByNumber(field->number());
  }

  // Constructs a protocol buffer which contains fields with all the same
  // numbers as all_fields_data_ except that each field is some other wire
  // type.
  string GetBizarroData() {
    unittest::TestEmptyMessage bizarro_message;
    UnknownFieldSet* bizarro_unknown_fields =
      bizarro_message.mutable_unknown_fields();
    for (int i = 0; i < unknown_fields_->field_count(); i++) {
      const UnknownField& unknown_field = unknown_fields_->field(i);
      UnknownField* bizarro_field =
        bizarro_unknown_fields->AddField(unknown_field.number());
      if (unknown_field.varint_size() == 0) {
        bizarro_field->add_varint(1);
      } else {
        bizarro_field->add_fixed32(1);
      }
    }

    string data;
    EXPECT_TRUE(bizarro_message.SerializeToString(&data));
    return data;
  }

  const Descriptor* descriptor_;
  unittest::TestAllTypes all_fields_;
  string all_fields_data_;

  // An empty message that has been parsed from all_fields_data_.  So, it has
  // unknown fields of every type.
  unittest::TestEmptyMessage empty_message_;
  UnknownFieldSet* unknown_fields_;
};

TEST_F(UnknownFieldSetTest, Index) {
  for (int i = 0; i < unknown_fields_->field_count(); i++) {
    EXPECT_EQ(i, unknown_fields_->field(i).index());
  }
}

TEST_F(UnknownFieldSetTest, FindFieldByNumber) {
  // All fields of TestAllTypes should be present.  Fields that are not valid
  // field numbers of TestAllTypes should NOT be present.

  for (int i = 0; i < 1000; i++) {
    if (descriptor_->FindFieldByNumber(i) == NULL) {
      EXPECT_TRUE(unknown_fields_->FindFieldByNumber(i) == NULL);
    } else {
      EXPECT_TRUE(unknown_fields_->FindFieldByNumber(i) != NULL);
    }
  }
}

TEST_F(UnknownFieldSetTest, Varint) {
  const UnknownField* field = GetField("optional_int32");
  ASSERT_TRUE(field != NULL);

  ASSERT_EQ(1, field->varint_size());
  EXPECT_EQ(all_fields_.optional_int32(), field->varint(0));
}

TEST_F(UnknownFieldSetTest, Fixed32) {
  const UnknownField* field = GetField("optional_fixed32");
  ASSERT_TRUE(field != NULL);

  ASSERT_EQ(1, field->fixed32_size());
  EXPECT_EQ(all_fields_.optional_fixed32(), field->fixed32(0));
}

TEST_F(UnknownFieldSetTest, Fixed64) {
  const UnknownField* field = GetField("optional_fixed64");
  ASSERT_TRUE(field != NULL);

  ASSERT_EQ(1, field->fixed64_size());
  EXPECT_EQ(all_fields_.optional_fixed64(), field->fixed64(0));
}

TEST_F(UnknownFieldSetTest, LengthDelimited) {
  const UnknownField* field = GetField("optional_string");
  ASSERT_TRUE(field != NULL);

  ASSERT_EQ(1, field->length_delimited_size());
  EXPECT_EQ(all_fields_.optional_string(), field->length_delimited(0));
}

TEST_F(UnknownFieldSetTest, Group) {
  const UnknownField* field = GetField("optionalgroup");
  ASSERT_TRUE(field != NULL);

  ASSERT_EQ(1, field->group_size());
  EXPECT_EQ(1, field->group(0).field_count());

  const UnknownField& nested_field = field->group(0).field(0);
  const FieldDescriptor* nested_field_descriptor =
    unittest::TestAllTypes::OptionalGroup::descriptor()->FindFieldByName("a");
  ASSERT_TRUE(nested_field_descriptor != NULL);

  EXPECT_EQ(nested_field_descriptor->number(), nested_field.number());
  EXPECT_EQ(all_fields_.optionalgroup().a(), nested_field.varint(0));
}

TEST_F(UnknownFieldSetTest, Serialize) {
  // Check that serializing the UnknownFieldSet produces the original data
  // again.

  string data;
  empty_message_.SerializeToString(&data);

  // Don't use EXPECT_EQ because we don't want to dump raw binary data to
  // stdout.
  EXPECT_TRUE(data == all_fields_data_);
}

TEST_F(UnknownFieldSetTest, ParseViaReflection) {
  // Make sure fields are properly parsed to the UnknownFieldSet when parsing
  // via reflection.

  unittest::TestEmptyMessage message;
  io::ArrayInputStream raw_input(all_fields_data_.data(),
                                 all_fields_data_.size());
  io::CodedInputStream input(&raw_input);
  ASSERT_TRUE(WireFormat::ParseAndMergePartial(&input, &message));

  EXPECT_EQ(message.DebugString(), empty_message_.DebugString());
}

TEST_F(UnknownFieldSetTest, SerializeViaReflection) {
  // Make sure fields are properly written from the UnknownFieldSet when
  // serializing via reflection.

  string data;

  {
    io::StringOutputStream raw_output(&data);
    io::CodedOutputStream output(&raw_output);
    int size = WireFormat::ByteSize(empty_message_);
    ASSERT_TRUE(
      WireFormat::SerializeWithCachedSizes(empty_message_, size, &output));
  }

  // Don't use EXPECT_EQ because we don't want to dump raw binary data to
  // stdout.
  EXPECT_TRUE(data == all_fields_data_);
}

TEST_F(UnknownFieldSetTest, CopyFrom) {
  unittest::TestEmptyMessage message;

  message.CopyFrom(empty_message_);

  EXPECT_EQ(empty_message_.DebugString(), message.DebugString());
}

TEST_F(UnknownFieldSetTest, Swap) {
  unittest::TestEmptyMessage other_message;
  ASSERT_TRUE(other_message.ParseFromString(GetBizarroData()));

  EXPECT_GT(empty_message_.unknown_fields().field_count(), 0);
  EXPECT_GT(other_message.unknown_fields().field_count(), 0);
  const string debug_string = empty_message_.DebugString();
  const string other_debug_string = other_message.DebugString();
  EXPECT_NE(debug_string, other_debug_string);

  empty_message_.Swap(&other_message);
  EXPECT_EQ(debug_string, other_message.DebugString());
  EXPECT_EQ(other_debug_string, empty_message_.DebugString());
}

TEST_F(UnknownFieldSetTest, SwapWithSelf) {
  const string debug_string = empty_message_.DebugString();
  EXPECT_GT(empty_message_.unknown_fields().field_count(), 0);

  empty_message_.Swap(&empty_message_);
  EXPECT_GT(empty_message_.unknown_fields().field_count(), 0);
  EXPECT_EQ(debug_string, empty_message_.DebugString());
}

TEST_F(UnknownFieldSetTest, MergeFrom) {
  unittest::TestEmptyMessage source, destination;

  destination.mutable_unknown_fields()->AddField(1)->add_varint(1);
  destination.mutable_unknown_fields()->AddField(3)->add_varint(2);
  source.mutable_unknown_fields()->AddField(2)->add_varint(3);
  source.mutable_unknown_fields()->AddField(3)->add_varint(4);

  destination.MergeFrom(source);

  EXPECT_EQ(
    // Note:  The ordering of fields here depends on the ordering of adds
    //   and merging, above.
    "1: 1\n"
    "3: 2\n"
    "3: 4\n"
    "2: 3\n",
    destination.DebugString());
}

TEST_F(UnknownFieldSetTest, Clear) {
  // Get a pointer to a contained field object.
  const UnknownField* field = GetField("optional_int32");
  ASSERT_TRUE(field != NULL);
  ASSERT_EQ(1, field->varint_size());
  int number = field->number();

  // Clear the set.
  empty_message_.Clear();
  EXPECT_EQ(0, unknown_fields_->field_count());

  // If we add that field again we should get the same object.
  ASSERT_EQ(field, unknown_fields_->AddField(number));

  // But it should be cleared.
  EXPECT_EQ(0, field->varint_size());
}

TEST_F(UnknownFieldSetTest, ParseKnownAndUnknown) {
  // Test mixing known and unknown fields when parsing.

  unittest::TestEmptyMessage source;
  source.mutable_unknown_fields()->AddField(123456)->add_varint(654321);
  string data;
  ASSERT_TRUE(source.SerializeToString(&data));

  unittest::TestAllTypes destination;
  ASSERT_TRUE(destination.ParseFromString(all_fields_data_ + data));

  TestUtil::ExpectAllFieldsSet(destination);
  ASSERT_EQ(1, destination.unknown_fields().field_count());
  ASSERT_EQ(1, destination.unknown_fields().field(0).varint_size());
  EXPECT_EQ(654321, destination.unknown_fields().field(0).varint(0));
}

TEST_F(UnknownFieldSetTest, WrongTypeTreatedAsUnknown) {
  // Test that fields of the wrong wire type are treated like unknown fields
  // when parsing.

  unittest::TestAllTypes all_types_message;
  unittest::TestEmptyMessage empty_message;
  string bizarro_data = GetBizarroData();
  ASSERT_TRUE(all_types_message.ParseFromString(bizarro_data));
  ASSERT_TRUE(empty_message.ParseFromString(bizarro_data));

  // All fields should have been interpreted as unknown, so the debug strings
  // should be the same.
  EXPECT_EQ(empty_message.DebugString(), all_types_message.DebugString());
}

TEST_F(UnknownFieldSetTest, WrongTypeTreatedAsUnknownViaReflection) {
  // Same as WrongTypeTreatedAsUnknown but via the reflection interface.

  unittest::TestAllTypes all_types_message;
  unittest::TestEmptyMessage empty_message;
  string bizarro_data = GetBizarroData();
  io::ArrayInputStream raw_input(bizarro_data.data(), bizarro_data.size());
  io::CodedInputStream input(&raw_input);
  ASSERT_TRUE(WireFormat::ParseAndMergePartial(&input, &all_types_message));
  ASSERT_TRUE(empty_message.ParseFromString(bizarro_data));

  EXPECT_EQ(empty_message.DebugString(), all_types_message.DebugString());
}

TEST_F(UnknownFieldSetTest, UnknownExtensions) {
  // Make sure fields are properly parsed to the UnknownFieldSet even when
  // they are declared as extension numbers.

  unittest::TestEmptyMessageWithExtensions message;
  ASSERT_TRUE(message.ParseFromString(all_fields_data_));

  EXPECT_EQ(message.DebugString(), empty_message_.DebugString());
}

TEST_F(UnknownFieldSetTest, UnknownExtensionsReflection) {
  // Same as UnknownExtensions except parsing via reflection.

  unittest::TestEmptyMessageWithExtensions message;
  io::ArrayInputStream raw_input(all_fields_data_.data(),
                                 all_fields_data_.size());
  io::CodedInputStream input(&raw_input);
  ASSERT_TRUE(WireFormat::ParseAndMergePartial(&input, &message));

  EXPECT_EQ(message.DebugString(), empty_message_.DebugString());
}

TEST_F(UnknownFieldSetTest, WrongExtensionTypeTreatedAsUnknown) {
  // Test that fields of the wrong wire type are treated like unknown fields
  // when parsing extensions.

  unittest::TestAllExtensions all_extensions_message;
  unittest::TestEmptyMessage empty_message;
  string bizarro_data = GetBizarroData();
  ASSERT_TRUE(all_extensions_message.ParseFromString(bizarro_data));
  ASSERT_TRUE(empty_message.ParseFromString(bizarro_data));

  // All fields should have been interpreted as unknown, so the debug strings
  // should be the same.
  EXPECT_EQ(empty_message.DebugString(), all_extensions_message.DebugString());
}

TEST_F(UnknownFieldSetTest, UnknownEnumValue) {
  using unittest::TestAllTypes;
  using unittest::TestAllExtensions;
  using unittest::TestEmptyMessage;

  const FieldDescriptor* singular_field =
    TestAllTypes::descriptor()->FindFieldByName("optional_nested_enum");
  const FieldDescriptor* repeated_field =
    TestAllTypes::descriptor()->FindFieldByName("repeated_nested_enum");
  ASSERT_TRUE(singular_field != NULL);
  ASSERT_TRUE(repeated_field != NULL);

  string data;

  {
    TestEmptyMessage empty_message;
    UnknownFieldSet* unknown_fields = empty_message.mutable_unknown_fields();
    UnknownField* singular_unknown_field =
      unknown_fields->AddField(singular_field->number());
    singular_unknown_field->add_varint(TestAllTypes::BAR);
    singular_unknown_field->add_varint(5);  // not valid
    UnknownField* repeated_unknown_field =
      unknown_fields->AddField(repeated_field->number());
    repeated_unknown_field->add_varint(TestAllTypes::FOO);
    repeated_unknown_field->add_varint(4);  // not valid
    repeated_unknown_field->add_varint(TestAllTypes::BAZ);
    repeated_unknown_field->add_varint(6);  // not valid
    empty_message.SerializeToString(&data);
  }

  {
    TestAllTypes message;
    ASSERT_TRUE(message.ParseFromString(data));
    EXPECT_EQ(TestAllTypes::BAR, message.optional_nested_enum());
    ASSERT_EQ(2, message.repeated_nested_enum_size());
    EXPECT_EQ(TestAllTypes::FOO, message.repeated_nested_enum(0));
    EXPECT_EQ(TestAllTypes::BAZ, message.repeated_nested_enum(1));

    const UnknownFieldSet& unknown_fields = message.unknown_fields();
    ASSERT_EQ(2, unknown_fields.field_count());

    const UnknownField& singular_unknown_field = unknown_fields.field(0);
    ASSERT_EQ(singular_field->number(), singular_unknown_field.number());
    ASSERT_EQ(1, singular_unknown_field.varint_size());
    EXPECT_EQ(5, singular_unknown_field.varint(0));

    const UnknownField& repeated_unknown_field = unknown_fields.field(1);
    ASSERT_EQ(repeated_field->number(), repeated_unknown_field.number());
    ASSERT_EQ(2, repeated_unknown_field.varint_size());
    EXPECT_EQ(4, repeated_unknown_field.varint(0));
    EXPECT_EQ(6, repeated_unknown_field.varint(1));
  }

  {
    using unittest::optional_nested_enum_extension;
    using unittest::repeated_nested_enum_extension;

    TestAllExtensions message;
    ASSERT_TRUE(message.ParseFromString(data));
    EXPECT_EQ(TestAllTypes::BAR,
              message.GetExtension(optional_nested_enum_extension));
    ASSERT_EQ(2, message.ExtensionSize(repeated_nested_enum_extension));
    EXPECT_EQ(TestAllTypes::FOO,
              message.GetExtension(repeated_nested_enum_extension, 0));
    EXPECT_EQ(TestAllTypes::BAZ,
              message.GetExtension(repeated_nested_enum_extension, 1));

    const UnknownFieldSet& unknown_fields = message.unknown_fields();
    ASSERT_EQ(2, unknown_fields.field_count());

    const UnknownField& singular_unknown_field = unknown_fields.field(0);
    ASSERT_EQ(singular_field->number(), singular_unknown_field.number());
    ASSERT_EQ(1, singular_unknown_field.varint_size());
    EXPECT_EQ(5, singular_unknown_field.varint(0));

    const UnknownField& repeated_unknown_field = unknown_fields.field(1);
    ASSERT_EQ(repeated_field->number(), repeated_unknown_field.number());
    ASSERT_EQ(2, repeated_unknown_field.varint_size());
    EXPECT_EQ(4, repeated_unknown_field.varint(0));
    EXPECT_EQ(6, repeated_unknown_field.varint(1));
  }
}

TEST_F(UnknownFieldSetTest, SpaceUsedExcludingSelf) {
  {
    // Make sure an unknown field set has zero space used until a field is
    // actually added.
    unittest::TestEmptyMessage empty_message;
    const int empty_message_size = empty_message.SpaceUsed();
    UnknownFieldSet* unknown_fields = empty_message.mutable_unknown_fields();
    EXPECT_EQ(empty_message_size, empty_message.SpaceUsed());
    unknown_fields->AddField(1)->add_varint(0);
    EXPECT_LT(empty_message_size, empty_message.SpaceUsed());
  }
  {
    // Test varints.
    UnknownFieldSet unknown_fields;
    UnknownField* field = unknown_fields.AddField(1);
    const int base_size = unknown_fields.SpaceUsedExcludingSelf();
    for (int i = 0; i < 16; ++i) {
      field->add_varint(i);
    }
    // Should just defer computation to the RepeatedField.
    int expected_size = base_size + field->varint().SpaceUsedExcludingSelf();
    EXPECT_EQ(expected_size, unknown_fields.SpaceUsedExcludingSelf());
  }
  {
    // Test fixed32s.
    UnknownFieldSet unknown_fields;
    UnknownField* field = unknown_fields.AddField(1);
    const int base_size = unknown_fields.SpaceUsedExcludingSelf();
    for (int i = 0; i < 16; ++i) {
      field->add_fixed32(i);
    }
    int expected_size = base_size + field->fixed32().SpaceUsedExcludingSelf();
    EXPECT_EQ(expected_size, unknown_fields.SpaceUsedExcludingSelf());
  }
  {
    // Test fixed64s.
    UnknownFieldSet unknown_fields;
    UnknownField* field = unknown_fields.AddField(1);
    const int base_size = unknown_fields.SpaceUsedExcludingSelf();
    for (int i = 0; i < 16; ++i) {
      field->add_fixed64(i);
    }
    int expected_size = base_size + field->fixed64().SpaceUsedExcludingSelf();
    EXPECT_EQ(expected_size, unknown_fields.SpaceUsedExcludingSelf());
  }
  {
    // Test length-delimited types.
    UnknownFieldSet unknown_fields;
    UnknownField* field = unknown_fields.AddField(1);
    const int base_size = unknown_fields.SpaceUsedExcludingSelf();
    for (int i = 0; i < 16; ++i) {
      field->add_length_delimited()->assign("my length delimited string");
    }
    int expected_size = base_size +
        field->length_delimited().SpaceUsedExcludingSelf();
    EXPECT_EQ(expected_size, unknown_fields.SpaceUsedExcludingSelf());
  }
}

TEST_F(UnknownFieldSetTest, SpaceUsed) {
  UnknownFieldSet unknown_fields;
  const int expected_size = sizeof(unknown_fields) +
      unknown_fields.SpaceUsedExcludingSelf();
  EXPECT_EQ(expected_size, unknown_fields.SpaceUsed());
}

}  // namespace
}  // namespace protobuf
}  // namespace google
