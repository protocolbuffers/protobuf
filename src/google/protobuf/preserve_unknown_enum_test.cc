// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <gtest/gtest.h>
#include "google/protobuf/descriptor.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest_preserve_unknown_enum.pb.h"
#include "google/protobuf/unittest_preserve_unknown_enum2.pb.h"

namespace google {
namespace protobuf {
namespace {

void FillMessage(
    proto3_preserve_unknown_enum_unittest::MyMessagePlusExtra* message) {
  message->set_e(proto3_preserve_unknown_enum_unittest::E_EXTRA);
  message->add_repeated_e(proto3_preserve_unknown_enum_unittest::E_EXTRA);
  message->add_repeated_packed_e(
      proto3_preserve_unknown_enum_unittest::E_EXTRA);
  message->add_repeated_packed_unexpected_e(
      proto3_preserve_unknown_enum_unittest::E_EXTRA);
  message->set_oneof_e_1(proto3_preserve_unknown_enum_unittest::E_EXTRA);
}

void CheckMessage(
    const proto3_preserve_unknown_enum_unittest::MyMessagePlusExtra& message) {
  EXPECT_EQ(proto3_preserve_unknown_enum_unittest::E_EXTRA, message.e());
  EXPECT_EQ(1, message.repeated_e_size());
  EXPECT_EQ(proto3_preserve_unknown_enum_unittest::E_EXTRA,
            message.repeated_e(0));
  EXPECT_EQ(1, message.repeated_packed_e_size());
  EXPECT_EQ(proto3_preserve_unknown_enum_unittest::E_EXTRA,
            message.repeated_packed_e(0));
  EXPECT_EQ(1, message.repeated_packed_unexpected_e_size());
  EXPECT_EQ(proto3_preserve_unknown_enum_unittest::E_EXTRA,
            message.repeated_packed_unexpected_e(0));
  EXPECT_EQ(proto3_preserve_unknown_enum_unittest::E_EXTRA,
            message.oneof_e_1());
}

void CheckMessage(
    const proto3_preserve_unknown_enum_unittest::MyMessage& message) {
  EXPECT_EQ(static_cast<int>(proto3_preserve_unknown_enum_unittest::E_EXTRA),
            static_cast<int>(message.e()));
  EXPECT_EQ(1, message.repeated_e_size());
  EXPECT_EQ(static_cast<int>(proto3_preserve_unknown_enum_unittest::E_EXTRA),
            static_cast<int>(message.repeated_e(0)));
  EXPECT_EQ(1, message.repeated_packed_e_size());
  EXPECT_EQ(static_cast<int>(proto3_preserve_unknown_enum_unittest::E_EXTRA),
            static_cast<int>(message.repeated_packed_e(0)));
  EXPECT_EQ(1, message.repeated_packed_unexpected_e_size());
  EXPECT_EQ(static_cast<int>(proto3_preserve_unknown_enum_unittest::E_EXTRA),
            static_cast<int>(message.repeated_packed_unexpected_e(0)));
  EXPECT_EQ(static_cast<int>(proto3_preserve_unknown_enum_unittest::E_EXTRA),
            static_cast<int>(message.oneof_e_1()));
}

}  // anonymous namespace

// Test that parsing preserves an unknown value in the enum field and does not
// punt it to the UnknownFieldSet.
TEST(PreserveUnknownEnumTest, PreserveParseAndSerialize) {
  proto3_preserve_unknown_enum_unittest::MyMessagePlusExtra orig_message;
  FillMessage(&orig_message);
  std::string serialized;
  orig_message.SerializeToString(&serialized);

  proto3_preserve_unknown_enum_unittest::MyMessage message;
  EXPECT_EQ(true, message.ParseFromString(serialized));
  CheckMessage(message);

  serialized.clear();
  message.SerializeToString(&serialized);
  EXPECT_EQ(true, orig_message.ParseFromString(serialized));
  CheckMessage(orig_message);
}

// Test that reflection based implementation also keeps unknown enum values and
// doesn't put them into UnknownFieldSet.
TEST(PreserveUnknownEnumTest, PreserveParseAndSerializeDynamicMessage) {
  proto3_preserve_unknown_enum_unittest::MyMessagePlusExtra orig_message;
  FillMessage(&orig_message);
  std::string serialized = orig_message.SerializeAsString();

  DynamicMessageFactory factory;
  std::unique_ptr<Message> message(
      factory
          .GetPrototype(
              proto3_preserve_unknown_enum_unittest::MyMessage::descriptor())
          ->New());
  EXPECT_EQ(true, message->ParseFromString(serialized));
  message->DiscardUnknownFields();

  serialized = message->SerializeAsString();
  EXPECT_EQ(true, orig_message.ParseFromString(serialized));
  CheckMessage(orig_message);
}

// Test that for proto2 messages, unknown values are in unknown fields.
TEST(PreserveUnknownEnumTest, Proto2HidesUnknownValues) {
  proto3_preserve_unknown_enum_unittest::MyMessagePlusExtra orig_message;
  FillMessage(&orig_message);

  std::string serialized;
  orig_message.SerializeToString(&serialized);

  proto2_preserve_unknown_enum_unittest::MyMessage message;
  EXPECT_EQ(true, message.ParseFromString(serialized));
  // The intermediate message has everything in its "unknown fields".
  proto2_preserve_unknown_enum_unittest::MyMessage message2 = message;
  message2.DiscardUnknownFields();
  EXPECT_EQ(0, message2.ByteSizeLong());

  // But when we pass it to the correct structure, all values are there.
  serialized.clear();
  message.SerializeToString(&serialized);
  EXPECT_EQ(true, orig_message.ParseFromString(serialized));
  CheckMessage(orig_message);
}

// Same as before, for a dynamic message.
TEST(PreserveUnknownEnumTest, DynamicProto2HidesUnknownValues) {
  proto3_preserve_unknown_enum_unittest::MyMessagePlusExtra orig_message;
  FillMessage(&orig_message);

  std::string serialized;
  orig_message.SerializeToString(&serialized);

  DynamicMessageFactory factory;
  std::unique_ptr<Message> message(
      factory
          .GetPrototype(
              proto2_preserve_unknown_enum_unittest::MyMessage::descriptor())
          ->New());
  EXPECT_EQ(true, message->ParseFromString(serialized));
  // The intermediate message has everything in its "unknown fields".
  proto2_preserve_unknown_enum_unittest::MyMessage message2;
  message2.CopyFrom(*message);
  message2.DiscardUnknownFields();
  EXPECT_EQ(0, message2.ByteSizeLong());

  // But when we pass it to the correct structure, all values are there.
  serialized.clear();
  message->SerializeToString(&serialized);
  EXPECT_EQ(true, orig_message.ParseFromString(serialized));
  CheckMessage(orig_message);
}

// Test that reflection provides EnumValueDescriptors for unknown values.
TEST(PreserveUnknownEnumTest, DynamicEnumValueDescriptors) {
  proto3_preserve_unknown_enum_unittest::MyMessagePlusExtra orig_message;
  FillMessage(&orig_message);
  std::string serialized;
  orig_message.SerializeToString(&serialized);

  proto3_preserve_unknown_enum_unittest::MyMessage message;
  EXPECT_EQ(true, message.ParseFromString(serialized));
  CheckMessage(message);

  const Reflection* r = message.GetReflection();
  const Descriptor* d = message.GetDescriptor();
  const FieldDescriptor* field = d->FindFieldByName("e");

  // This should dynamically create an EnumValueDescriptor.
  const EnumValueDescriptor* enum_value = r->GetEnum(message, field);
  EXPECT_EQ(enum_value->number(),
            static_cast<int>(proto3_preserve_unknown_enum_unittest::E_EXTRA));

  // Fetching value for a second time should return the same pointer.
  const EnumValueDescriptor* enum_value_second = r->GetEnum(message, field);
  EXPECT_EQ(enum_value, enum_value_second);

  // Check the repeated case too.
  const FieldDescriptor* repeated_field = d->FindFieldByName("repeated_e");
  enum_value = r->GetRepeatedEnum(message, repeated_field, 0);
  EXPECT_EQ(enum_value->number(),
            static_cast<int>(proto3_preserve_unknown_enum_unittest::E_EXTRA));
  // Should reuse the same EnumValueDescriptor, even for a different field.
  EXPECT_EQ(enum_value, enum_value_second);

  // We should be able to use the returned value descriptor to set a value on
  // another message.
  Message* m = message.New();
  r->SetEnum(m, field, enum_value);
  EXPECT_EQ(enum_value, r->GetEnum(*m, field));
  delete m;
}

// Test that the new integer-based enum reflection API works.
TEST(PreserveUnknownEnumTest, IntegerEnumReflectionAPI) {
  proto3_preserve_unknown_enum_unittest::MyMessage message;
  const Reflection* r = message.GetReflection();
  const Descriptor* d = message.GetDescriptor();

  const FieldDescriptor* singular_field = d->FindFieldByName("e");
  const FieldDescriptor* repeated_field = d->FindFieldByName("repeated_e");

  r->SetEnumValue(&message, singular_field, 42);
  EXPECT_EQ(42, r->GetEnumValue(message, singular_field));
  r->AddEnumValue(&message, repeated_field, 42);
  r->AddEnumValue(&message, repeated_field, 42);
  EXPECT_EQ(42, r->GetRepeatedEnumValue(message, repeated_field, 0));
  r->SetRepeatedEnumValue(&message, repeated_field, 1, 84);
  EXPECT_EQ(84, r->GetRepeatedEnumValue(message, repeated_field, 1));
  const EnumValueDescriptor* enum_value = r->GetEnum(message, singular_field);
  EXPECT_EQ(42, enum_value->number());
}

// Test that the EnumValue API works properly for proto2 messages as well.
TEST(PreserveUnknownEnumTest, Proto2CatchesUnknownValues) {
  proto2_unittest::TestAllTypes message;  // proto2 message
  const Reflection* r = message.GetReflection();
  const Descriptor* d = message.GetDescriptor();
  const FieldDescriptor* repeated_field =
      d->FindFieldByName("repeated_nested_enum");
  // Add one element to the repeated field so that we can test
  // SetRepeatedEnumValue.
  const EnumValueDescriptor* enum_value =
      repeated_field->enum_type()->FindValueByName("BAR");
  EXPECT_TRUE(enum_value != nullptr);
  r->AddEnum(&message, repeated_field, enum_value);

  const FieldDescriptor* singular_field =
      d->FindFieldByName("optional_nested_enum");
  // Enum-field integer-based setters treat as unknown integer values as
  // unknown fields.
  r->SetEnumValue(&message, singular_field, 4242);
  EXPECT_EQ(r->GetEnum(message, singular_field)->number(),
            proto2_unittest::TestAllTypes::FOO);
  r->SetRepeatedEnumValue(&message, repeated_field, 0, 4242);
  // repeated_nested_enum was set to bar above, this should not have changed.
  EXPECT_EQ(r->GetRepeatedEnum(message, repeated_field, 0)->number(),
            proto2_unittest::TestAllTypes::BAR);
  r->AddEnumValue(&message, repeated_field, 4242);
  // No element should be added
  EXPECT_EQ(message.repeated_nested_enum_size(), 1);

  // We require the enums to end up in unknown field set
  ASSERT_EQ(message.unknown_fields().field_count(), 3);
  EXPECT_EQ(message.unknown_fields().field(0).number(),
            singular_field->number());
  EXPECT_EQ(message.unknown_fields().field(0).varint(), 4242);
  EXPECT_EQ(message.unknown_fields().field(1).number(),
            repeated_field->number());
  EXPECT_EQ(message.unknown_fields().field(1).varint(), 4242);
  EXPECT_EQ(message.unknown_fields().field(2).number(),
            repeated_field->number());
  EXPECT_EQ(message.unknown_fields().field(2).varint(), 4242);
}

}  // namespace protobuf
}  // namespace google
