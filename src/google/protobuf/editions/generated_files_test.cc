// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/editions/golden/test_messages_proto2.pb.h"
#include "google/protobuf/editions/golden/test_messages_proto3.pb.h"
#include "google/protobuf/editions/proto/test_editions_default_features.pb.h"
#include "google/protobuf/test_textproto.h"

// These tests provide some basic minimal coverage that protos work as expected.
// Full coverage will come as we migrate test protos to editions.

namespace google {
namespace protobuf {
namespace {

using ::protobuf_editions_test::EditionsDefaultMessage;
using ::protobuf_test_messages::proto2::TestAllRequiredTypesProto2;
using ::protobuf_test_messages::proto2::TestAllTypesProto2;
using ::protobuf_test_messages::proto3::TestAllTypesProto3;
using ::testing::NotNull;

TEST(Generated, Parsing) {
  TestAllTypesProto2 message = ParseTextOrDie(R"pb(
    Data { group_int32: 9 }
  )pb");

  EXPECT_EQ(message.data().group_int32(), 9);
}

TEST(Generated, GeneratedMethods) {
  TestAllTypesProto3 message;
  message.set_optional_int32(9);

  EXPECT_THAT(message, EqualsProto(R"pb(optional_int32: 9)pb"));
}

TEST(Generated, ExplicitPresence) {
  const FieldDescriptor* field =
      TestAllTypesProto2::descriptor()->FindFieldByName("default_int32");
  ASSERT_THAT(field, NotNull());
  EXPECT_TRUE(field->has_presence());
}

TEST(Generated, RequiredPresence) {
  const FieldDescriptor* field =
      TestAllRequiredTypesProto2::descriptor()->FindFieldByName(
          "required_int32");
  ASSERT_THAT(field, NotNull());
  EXPECT_TRUE(field->has_presence());
  EXPECT_TRUE(field->is_required());
  EXPECT_EQ(field->label(), FieldDescriptor::LABEL_REQUIRED);
}

TEST(Generated, ImplicitPresence) {
  const FieldDescriptor* field =
      TestAllTypesProto3::descriptor()->FindFieldByName("optional_int32");
  ASSERT_THAT(field, NotNull());
  EXPECT_FALSE(field->has_presence());
}

TEST(Generated, ClosedEnum) {
  const EnumDescriptor* enm =
      TestAllTypesProto2::descriptor()->FindEnumTypeByName("NestedEnum");
  ASSERT_THAT(enm, NotNull());
  EXPECT_TRUE(enm->is_closed());
}

TEST(Generated, OpenEnum) {
  const EnumDescriptor* enm =
      TestAllTypesProto3::descriptor()->FindEnumTypeByName("NestedEnum");
  ASSERT_THAT(enm, NotNull());
  EXPECT_FALSE(enm->is_closed());
}

TEST(Generated, PackedRepeated) {
  const FieldDescriptor* field =
      TestAllTypesProto2::descriptor()->FindFieldByName("packed_int32");
  ASSERT_THAT(field, NotNull());
  EXPECT_TRUE(field->is_packed());
}

TEST(Generated, ExpandedRepeated) {
  const FieldDescriptor* field =
      TestAllTypesProto2::descriptor()->FindFieldByName("repeated_int32");
  ASSERT_THAT(field, NotNull());
  EXPECT_FALSE(field->is_packed());
}

TEST(Generated, DoesNotEnforceUtf8) {
  const FieldDescriptor* field =
      TestAllTypesProto2::descriptor()->FindFieldByName("optional_string");
  ASSERT_THAT(field, NotNull());
  EXPECT_FALSE(field->requires_utf8_validation());
}

TEST(Generated, EnforceUtf8) {
  const FieldDescriptor* field =
      TestAllTypesProto3::descriptor()->FindFieldByName("optional_string");
  ASSERT_THAT(field, NotNull());
  EXPECT_TRUE(field->requires_utf8_validation());
}

TEST(Generated, DelimitedEncoding) {
  const FieldDescriptor* field =
      TestAllTypesProto2::descriptor()->FindFieldByName("data");
  ASSERT_THAT(field, NotNull());
  EXPECT_EQ(field->type(), FieldDescriptor::TYPE_GROUP);
}

TEST(Generated, LengthPrefixedEncoding) {
  const FieldDescriptor* field =
      TestAllTypesProto2::descriptor()->FindFieldByName(
          "optional_nested_message");
  ASSERT_THAT(field, NotNull());
  EXPECT_EQ(field->type(), FieldDescriptor::TYPE_MESSAGE);
}

TEST(Generated, EditionDefaults2023) {
  const Descriptor* desc = EditionsDefaultMessage::descriptor();
  EXPECT_TRUE(desc->FindFieldByName("int32_field")->has_presence());
  EXPECT_TRUE(
      desc->FindFieldByName("string_field")->requires_utf8_validation());
  EXPECT_FALSE(desc->FindFieldByName("enum_field")
                   ->legacy_enum_field_treated_as_closed());
  EXPECT_FALSE(desc->FindFieldByName("enum_field")->enum_type()->is_closed());
  EXPECT_TRUE(desc->FindFieldByName("repeated_int32_field")->is_packed());
  EXPECT_EQ(desc->FindFieldByName("sub_message_field")->type(),
            FieldDescriptor::TYPE_MESSAGE);
}

TEST(Generated, EditionDefaults2023InternalFeatures) {
  EXPECT_THAT(internal::InternalFeatureHelper::GetFeatures(
                  *EditionsDefaultMessage::descriptor()),
              google::protobuf::EqualsProto(R"pb(
                field_presence: EXPLICIT
                enum_type: OPEN
                repeated_field_encoding: PACKED
                utf8_validation: VERIFY
                message_encoding: LENGTH_PREFIXED
                json_format: ALLOW
                [pb.cpp] { legacy_closed_enum: false }
              )pb"));
}

}  // namespace
}  // namespace protobuf
}  // namespace google
