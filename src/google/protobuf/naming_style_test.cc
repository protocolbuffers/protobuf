// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/naming_style.h"

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"

namespace google {
namespace protobuf {
namespace internal {
namespace {

using ::absl_testing::IsOk;
using ::absl_testing::StatusIs;
using ::testing::HasSubstr;
using ::testing::Not;

TEST(NamingStyleTest, ContainsBadUnderscores) {
  EXPECT_TRUE(ContainsBadUnderscores("_foo"));
  EXPECT_TRUE(ContainsBadUnderscores("foo_"));
  EXPECT_TRUE(ContainsBadUnderscores("foo__bar"));
  EXPECT_TRUE(ContainsBadUnderscores("foo_1"));
  EXPECT_FALSE(ContainsBadUnderscores("foo_bar"));
  EXPECT_FALSE(ContainsBadUnderscores("foo_bar_baz"));
  EXPECT_FALSE(ContainsBadUnderscores(""));
}

TEST(NamingStyleTest, IsValidTitleCaseName) {
  EXPECT_THAT(IsValidTitleCaseName("Foo"), IsOk());
  EXPECT_THAT(IsValidTitleCaseName("FooBar"), IsOk());

  EXPECT_THAT(IsValidTitleCaseName("foo"), Not(IsOk()));
  EXPECT_THAT(IsValidTitleCaseName("Foo_Bar"), Not(IsOk()));
  EXPECT_THAT(IsValidTitleCaseName(""), Not(IsOk()));
}

TEST(NamingStyleTest, IsValidLowerSnakeCaseName) {
  EXPECT_THAT(IsValidLowerSnakeCaseName("foo"), IsOk());
  EXPECT_THAT(IsValidLowerSnakeCaseName("foo_bar"), IsOk());

  EXPECT_THAT(IsValidLowerSnakeCaseName("foo_bar_123"), Not(IsOk()));
  EXPECT_THAT(IsValidLowerSnakeCaseName("Foo"), Not(IsOk()));
  EXPECT_THAT(IsValidLowerSnakeCaseName("fooBar"), Not(IsOk()));
  EXPECT_THAT(IsValidLowerSnakeCaseName("foo__bar"), Not(IsOk()));
  EXPECT_THAT(IsValidLowerSnakeCaseName(""), Not(IsOk()));
}

TEST(NamingStyleTest, IsValidUpperSnakeCaseName) {
  EXPECT_THAT(IsValidUpperSnakeCaseName("FOO"), IsOk());
  EXPECT_THAT(IsValidUpperSnakeCaseName("FOO_BAR"), IsOk());

  EXPECT_THAT(IsValidUpperSnakeCaseName("FOO_BAR_123"), Not(IsOk()));
  EXPECT_THAT(IsValidUpperSnakeCaseName("foo"), Not(IsOk()));
  EXPECT_THAT(IsValidUpperSnakeCaseName("FOO_bar"), Not(IsOk()));
  EXPECT_THAT(IsValidUpperSnakeCaseName("FOO__BAR"), Not(IsOk()));
  EXPECT_THAT(IsValidUpperSnakeCaseName(""), Not(IsOk()));
}

TEST(NamingStyleTest, ToCamelCase) {
  EXPECT_EQ(ToCamelCase("foo_bar", /*lower_first=*/true), "fooBar");
  EXPECT_EQ(ToCamelCase("foo_bar", /*lower_first=*/false), "FooBar");
  EXPECT_EQ(ToCamelCase("Foo_bar", /*lower_first=*/true), "fooBar");
  EXPECT_EQ(ToCamelCase("foo_bar_baz", /*lower_first=*/true), "fooBarBaz");
  EXPECT_EQ(ToCamelCase("", /*lower_first=*/true), "");
}

TEST(NamingStyleTest, ToJsonName) {
  EXPECT_EQ(ToJsonName("foo_bar"), "fooBar");
  EXPECT_EQ(ToJsonName("foo_bar_baz"), "fooBarBaz");
  EXPECT_EQ(ToJsonName("fooBar"), "fooBar");
  EXPECT_EQ(ToJsonName(""), "");
}

TEST(NamingStyleTest, EnumValueToPascalCase) {
  EXPECT_EQ(EnumValueToPascalCase("FOO_BAR"), "FooBar");
  EXPECT_EQ(EnumValueToPascalCase("foo_bar"), "FooBar");
  EXPECT_EQ(EnumValueToPascalCase("FOO"), "Foo");
  EXPECT_EQ(EnumValueToPascalCase(""), "");
}

TEST(NamingStyleTest, IsValidFieldNonCollisionNameWithNullptrDescriptor) {
  EXPECT_THAT(IsValidFieldNonCollisionName("descriptor", nullptr),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("should not be named descriptor")));
  EXPECT_THAT(IsValidFieldNonCollisionName("foo", nullptr), IsOk());
  EXPECT_THAT(IsValidFieldNonCollisionName("has_foo", nullptr), IsOk());
  EXPECT_THAT(IsValidFieldNonCollisionName("get_foo", nullptr), IsOk());
  EXPECT_THAT(IsValidFieldNonCollisionName("set_foo", nullptr), IsOk());
  EXPECT_THAT(IsValidFieldNonCollisionName("clear_foo", nullptr), IsOk());
  EXPECT_THAT(IsValidFieldNonCollisionName("foo_value", nullptr), IsOk());
}

TEST(NamingStyleTest, IsValidFieldNonCollisionNameWithDescriptor) {
  FileDescriptorProto file_proto;
  file_proto.set_name("test.proto");
  DescriptorProto* message_proto = file_proto.add_message_type();
  message_proto->set_name("TestMessage");

  FieldDescriptorProto* field_proto = message_proto->add_field();
  field_proto->set_name("foo");
  field_proto->set_number(1);
  field_proto->set_label(FieldDescriptorProto::LABEL_OPTIONAL);
  field_proto->set_type(FieldDescriptorProto::TYPE_INT32);

  OneofDescriptorProto* oneof_proto = message_proto->add_oneof_decl();
  oneof_proto->set_name("bar");

  FieldDescriptorProto* oneof_field = message_proto->add_field();
  oneof_field->set_name("oneof_field");
  oneof_field->set_number(2);
  oneof_field->set_label(FieldDescriptorProto::LABEL_OPTIONAL);
  oneof_field->set_type(FieldDescriptorProto::TYPE_INT32);
  oneof_field->set_oneof_index(0);

  DescriptorPool pool;
  const FileDescriptor* file_desc = pool.BuildFile(file_proto);
  ASSERT_NE(file_desc, nullptr);
  const Descriptor* message_desc =
      file_desc->FindMessageTypeByName("TestMessage");
  ASSERT_NE(message_desc, nullptr);

  // "descriptor" is invalid with descriptor
  EXPECT_THAT(IsValidFieldNonCollisionName("descriptor", message_desc),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("should not be named descriptor")));

  // Collisions with field "foo"
  EXPECT_THAT(
      IsValidFieldNonCollisionName("has_foo", message_desc),
      StatusIs(
          absl::StatusCode::kInvalidArgument,
          HasSubstr("should not begin with has_ if a field named foo exists")));
  EXPECT_THAT(
      IsValidFieldNonCollisionName("get_foo", message_desc),
      StatusIs(
          absl::StatusCode::kInvalidArgument,
          HasSubstr("should not begin with get_ if a field named foo exists")));
  EXPECT_THAT(
      IsValidFieldNonCollisionName("set_foo", message_desc),
      StatusIs(
          absl::StatusCode::kInvalidArgument,
          HasSubstr("should not begin with set_ if a field named foo exists")));
  EXPECT_THAT(
      IsValidFieldNonCollisionName("clear_foo", message_desc),
      StatusIs(
          absl::StatusCode::kInvalidArgument,
          HasSubstr(
              "should not begin with clear_ if a field named foo exists")));
  EXPECT_THAT(
      IsValidFieldNonCollisionName("foo_value", message_desc),
      StatusIs(
          absl::StatusCode::kInvalidArgument,
          HasSubstr("should not end with _value if a field named foo exists")));

  // Collisions with oneof "bar"
  EXPECT_THAT(
      IsValidFieldNonCollisionName("has_bar", message_desc),
      StatusIs(
          absl::StatusCode::kInvalidArgument,
          HasSubstr("should not begin with has_ if a field named bar exists")));
  EXPECT_THAT(
      IsValidFieldNonCollisionName("get_bar", message_desc),
      StatusIs(
          absl::StatusCode::kInvalidArgument,
          HasSubstr("should not begin with get_ if a field named bar exists")));
  EXPECT_THAT(
      IsValidFieldNonCollisionName("set_bar", message_desc),
      StatusIs(
          absl::StatusCode::kInvalidArgument,
          HasSubstr("should not begin with set_ if a field named bar exists")));
  EXPECT_THAT(
      IsValidFieldNonCollisionName("clear_bar", message_desc),
      StatusIs(
          absl::StatusCode::kInvalidArgument,
          HasSubstr(
              "should not begin with clear_ if a field named bar exists")));
  EXPECT_THAT(
      IsValidFieldNonCollisionName("bar_value", message_desc),
      StatusIs(
          absl::StatusCode::kInvalidArgument,
          HasSubstr("should not end with _value if a field named bar exists")));

  // Collisions with field inside oneof "oneof_field"
  EXPECT_THAT(IsValidFieldNonCollisionName("has_oneof_field", message_desc),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("should not begin with has_ if a field named "
                                 "oneof_field exists")));
  EXPECT_THAT(IsValidFieldNonCollisionName("get_oneof_field", message_desc),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("should not begin with get_ if a field named "
                                 "oneof_field exists")));
  EXPECT_THAT(IsValidFieldNonCollisionName("set_oneof_field", message_desc),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("should not begin with set_ if a field named "
                                 "oneof_field exists")));
  EXPECT_THAT(IsValidFieldNonCollisionName("clear_oneof_field", message_desc),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("should not begin with clear_ if a field "
                                 "named oneof_field exists")));
  EXPECT_THAT(IsValidFieldNonCollisionName("oneof_field_value", message_desc),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("should not end with _value if a field named "
                                 "oneof_field exists")));

  // Non-colliding names
  EXPECT_THAT(IsValidFieldNonCollisionName("foo", message_desc), IsOk());
  EXPECT_THAT(IsValidFieldNonCollisionName("bar", message_desc), IsOk());
  EXPECT_THAT(IsValidFieldNonCollisionName("oneof_field", message_desc),
              IsOk());
  EXPECT_THAT(IsValidFieldNonCollisionName("has_baz", message_desc), IsOk());
  EXPECT_THAT(IsValidFieldNonCollisionName("get_baz", message_desc), IsOk());
  EXPECT_THAT(IsValidFieldNonCollisionName("set_baz", message_desc), IsOk());
  EXPECT_THAT(IsValidFieldNonCollisionName("clear_baz", message_desc), IsOk());
  EXPECT_THAT(IsValidFieldNonCollisionName("baz_value", message_desc), IsOk());
}

TEST(NamingStyleTest, IsValidFieldNonCollisionNameBoundaryConditions) {
  EXPECT_THAT(IsValidFieldNonCollisionName("", nullptr), IsOk());
  EXPECT_THAT(IsValidFieldNonCollisionName("has_", nullptr), IsOk());
  EXPECT_THAT(IsValidFieldNonCollisionName("_value", nullptr), IsOk());

  FileDescriptorProto file_proto;
  file_proto.set_name("test.proto");
  DescriptorProto* message_proto = file_proto.add_message_type();
  message_proto->set_name("TestMessage");
  FieldDescriptorProto* field_proto = message_proto->add_field();
  field_proto->set_name("foo");
  field_proto->set_number(1);
  field_proto->set_label(FieldDescriptorProto::LABEL_OPTIONAL);
  field_proto->set_type(FieldDescriptorProto::TYPE_INT32);

  DescriptorPool pool;
  const FileDescriptor* file_desc = pool.BuildFile(file_proto);
  ASSERT_NE(file_desc, nullptr);
  const Descriptor* message_desc =
      file_desc->FindMessageTypeByName("TestMessage");
  ASSERT_NE(message_desc, nullptr);

  EXPECT_THAT(IsValidFieldNonCollisionName("", message_desc), IsOk());
  EXPECT_THAT(IsValidFieldNonCollisionName("has_", message_desc), IsOk());
  EXPECT_THAT(IsValidFieldNonCollisionName("_value", message_desc), IsOk());
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
