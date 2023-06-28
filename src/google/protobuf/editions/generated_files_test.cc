// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "google/protobuf/descriptor.h"
#include "google/protobuf/editions/golden/test_messages_proto2.pb.h"
#include "google/protobuf/editions/golden/test_messages_proto3.pb.h"
#include "google/protobuf/test_textproto.h"

// These tests provide some basic minimal coverage that protos work as expected.
// Full coverage will come as we migrate test protos to editions.

namespace google {
namespace protobuf {
namespace {

using ::protobuf_test_messages::proto2::TestAllRequiredTypesProto2;
using ::protobuf_test_messages::proto2::TestAllTypesProto2;
using ::protobuf_test_messages::proto3::TestAllTypesProto3;
using ::testing::NotNull;

TEST(Gemerated, Parsing) {
  TestAllTypesProto2 message = ParseTextOrDie(R"pb(
    Data { group_int32: 9 }
  )pb");

  EXPECT_EQ(message.data().group_int32(), 9);
}

TEST(Gemerated, GeneratedMethods) {
  TestAllTypesProto3 message;
  message.set_optional_int32(9);

  EXPECT_THAT(message, EqualsProto(R"pb(optional_int32: 9)pb"));
}

TEST(Gemerated, ExplicitPresence) {
  const FieldDescriptor* field =
      TestAllTypesProto2::descriptor()->FindFieldByName("default_int32");
  ASSERT_THAT(field, NotNull());
  EXPECT_TRUE(field->has_presence());
}

TEST(Gemerated, RequiredPresence) {
  const FieldDescriptor* field =
      TestAllRequiredTypesProto2::descriptor()->FindFieldByName(
          "required_int32");
  ASSERT_THAT(field, NotNull());
  EXPECT_TRUE(field->has_presence());
  EXPECT_TRUE(field->is_required());
  EXPECT_EQ(field->label(), FieldDescriptor::LABEL_REQUIRED);
}

TEST(Gemerated, ImplicitPresence) {
  const FieldDescriptor* field =
      TestAllTypesProto3::descriptor()->FindFieldByName("optional_int32");
  ASSERT_THAT(field, NotNull());
  EXPECT_FALSE(field->has_presence());
}

TEST(Gemerated, ClosedEnum) {
  const EnumDescriptor* enm =
      TestAllTypesProto2::descriptor()->FindEnumTypeByName("NestedEnum");
  ASSERT_THAT(enm, NotNull());
  EXPECT_TRUE(enm->is_closed());
}

TEST(Gemerated, OpenEnum) {
  const EnumDescriptor* enm =
      TestAllTypesProto3::descriptor()->FindEnumTypeByName("NestedEnum");
  ASSERT_THAT(enm, NotNull());
  EXPECT_FALSE(enm->is_closed());
}

TEST(Gemerated, PackedRepeated) {
  const FieldDescriptor* field =
      TestAllTypesProto2::descriptor()->FindFieldByName("packed_int32");
  ASSERT_THAT(field, NotNull());
  EXPECT_TRUE(field->is_packed());
}

TEST(Gemerated, ExpandedRepeated) {
  const FieldDescriptor* field =
      TestAllTypesProto2::descriptor()->FindFieldByName("repeated_int32");
  ASSERT_THAT(field, NotNull());
  EXPECT_FALSE(field->is_packed());
}

TEST(Gemerated, DoesNotEnforceUtf8) {
  const FieldDescriptor* field =
      TestAllTypesProto2::descriptor()->FindFieldByName("optional_string");
  ASSERT_THAT(field, NotNull());
  EXPECT_FALSE(field->requires_utf8_validation());
}

TEST(Gemerated, EnforceUtf8) {
  const FieldDescriptor* field =
      TestAllTypesProto3::descriptor()->FindFieldByName("optional_string");
  ASSERT_THAT(field, NotNull());
  EXPECT_TRUE(field->requires_utf8_validation());
}

TEST(Gemerated, DelimitedEncoding) {
  const FieldDescriptor* field =
      TestAllTypesProto2::descriptor()->FindFieldByName("data");
  ASSERT_THAT(field, NotNull());
  EXPECT_EQ(field->type(), FieldDescriptor::TYPE_GROUP);
}

TEST(Gemerated, LengthPrefixedEncoding) {
  const FieldDescriptor* field =
      TestAllTypesProto2::descriptor()->FindFieldByName(
          "optional_nested_message");
  ASSERT_THAT(field, NotNull());
  EXPECT_EQ(field->type(), FieldDescriptor::TYPE_MESSAGE);
}

}  // namespace
}  // namespace protobuf
}  // namespace google
