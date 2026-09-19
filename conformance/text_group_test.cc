// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Text-format conformance tests for group fields (proto2-style message types)
// and for their editions successor, delimited-encoded message fields
// (TestAllTypesEdition2023).  This replaces the legacy
// TextFormatConformanceTestSuiteImpl<M>::RunGroupTests() and
// RunDelimitedTests(); the test names and the requests sent to the testee are
// identical to the legacy ones.

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
#include "binary_test_util.h"
#include "matchers.h"
#include "message_type_fixtures.h"
#include "test_environment.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::ValuesIn;

// Group fields.  In the test message types, "Data" is a group field with an
// int32 field group_int32, "MultiWordGroupField" is a group with the same
// field, and "groupfield" is an extension of group type GroupField.
using TextGroupTest = MessageTypeConformanceTest;

TEST_P(TextGroupTest, NoColon) {
  EXPECT_THAT(
      RequiredTest("GroupFieldNoColon")
          .ParseText(message(), "Data { group_int32: 1 }")
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto("Data { group_int32: 1 }"))));
  EXPECT_THAT(
      RequiredTest("GroupFieldNoColon")
          .ParseText(message(), "Data { group_int32: 1 }")
          .SerializeText(),
      Yields(ParsedPayload(EqualsTextProto("Data { group_int32: 1 }"))));
}

TEST_P(TextGroupTest, WithColon) {
  EXPECT_THAT(
      RequiredTest("GroupFieldWithColon")
          .ParseText(message(), "Data: { group_int32: 1 }")
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto("Data: { group_int32: 1 }"))));
  EXPECT_THAT(
      RequiredTest("GroupFieldWithColon")
          .ParseText(message(), "Data: { group_int32: 1 }")
          .SerializeText(),
      Yields(ParsedPayload(EqualsTextProto("Data: { group_int32: 1 }"))));
}

TEST_P(TextGroupTest, Empty) {
  EXPECT_THAT(RequiredTest("GroupFieldEmpty")
                  .ParseText(message(), "Data {}")
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto("Data {}"))));
  EXPECT_THAT(RequiredTest("GroupFieldEmpty")
                  .ParseText(message(), "Data {}")
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto("Data {}"))));
}

TEST_P(TextGroupTest, MultiWord) {
  EXPECT_THAT(
      RequiredTest("GroupFieldMultiWord")
          .ParseText(message(), "MultiWordGroupField { group_int32: 1 }")
          .SerializeBinary(),
      Yields(ParsedPayload(
          EqualsTextProto("MultiWordGroupField { group_int32: 1 }"))));
  EXPECT_THAT(
      RequiredTest("GroupFieldMultiWord")
          .ParseText(message(), "MultiWordGroupField { group_int32: 1 }")
          .SerializeText(),
      Yields(ParsedPayload(
          EqualsTextProto("MultiWordGroupField { group_int32: 1 }"))));
}

// The lower-cased group name (i.e. the implicit field name) is accepted.
TEST_P(TextGroupTest, Lowercased) {
  EXPECT_THAT(
      RequiredTest("GroupFieldLowercased")
          .ParseText(message(), "data { group_int32: 1 }")
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto("data { group_int32: 1 }"))));
  EXPECT_THAT(
      RequiredTest("GroupFieldLowercased")
          .ParseText(message(), "data { group_int32: 1 }")
          .SerializeText(),
      Yields(ParsedPayload(EqualsTextProto("data { group_int32: 1 }"))));
}

TEST_P(TextGroupTest, LowercasedMultiWord) {
  EXPECT_THAT(
      RequiredTest("GroupFieldLowercasedMultiWord")
          .ParseText(message(), "multiwordgroupfield { group_int32: 1 }")
          .SerializeBinary(),
      Yields(ParsedPayload(
          EqualsTextProto("multiwordgroupfield { group_int32: 1 }"))));
  EXPECT_THAT(
      RequiredTest("GroupFieldLowercasedMultiWord")
          .ParseText(message(), "multiwordgroupfield { group_int32: 1 }")
          .SerializeText(),
      Yields(ParsedPayload(
          EqualsTextProto("multiwordgroupfield { group_int32: 1 }"))));
}

// An extension of group type is named by the extension field.
TEST_P(TextGroupTest, Extension) {
  const FieldDescriptor* extension =
      message()->file()->FindExtensionByName("groupfield");
  ASSERT_NE(extension, nullptr);
  const std::string input = absl::StrCat(
      "[", extension->PrintableNameForExtension(), "] { group_int32: 1 }");
  EXPECT_THAT(RequiredTest("GroupFieldExtension")
                  .ParseText(message(), input)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(input))));
  EXPECT_THAT(RequiredTest("GroupFieldExtension")
                  .ParseText(message(), input)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(input))));
}

// ... and never by the name of its group message type.
TEST_P(TextGroupTest, ExtensionGroupName) {
  const Descriptor* group =
      message()->file()->FindMessageTypeByName("GroupField");
  ASSERT_NE(group, nullptr);
  EXPECT_THAT(RequiredTest("GroupFieldExtensionGroupName")
                  .ParseText(message(), absl::StrCat("[", group->full_name(),
                                                     "] { group_int32: 1 }"))
                  .ParseOnly(),
              Yields(IsParseError()));
}

INSTANTIATE_TEST_SUITE_P(All, TextGroupTest, ValuesIn(Proto2TestMessageTypes()),
                         MessageTypeParamName);

// Delimited-encoded message fields of TestAllTypesEdition2023 (message()):
// groupliketype and delimited_field are fields of the nested message type
// GroupLikeType (whose group-like name may stand in for the field name), and
// the groupliketype and delimited_ext extensions are of the top-level
// GroupLikeType.  The fixture skips itself when --maximum_edition doesn't
// cover editions.
using TextDelimitedTest = Edition2023ConformanceTest;

TEST_F(TextDelimitedTest, GroupFieldNoColon) {
  EXPECT_THAT(RequiredTest("GroupFieldNoColon")
                  .ParseText(message(), "GroupLikeType { group_int32: 1 }")
                  .SerializeBinary(),
              Yields(ParsedPayload(
                  EqualsTextProto("GroupLikeType { group_int32: 1 }"))));
  EXPECT_THAT(RequiredTest("GroupFieldNoColon")
                  .ParseText(message(), "GroupLikeType { group_int32: 1 }")
                  .SerializeText(),
              Yields(ParsedPayload(
                  EqualsTextProto("GroupLikeType { group_int32: 1 }"))));
}

TEST_F(TextDelimitedTest, GroupFieldWithColon) {
  EXPECT_THAT(RequiredTest("GroupFieldWithColon")
                  .ParseText(message(), "GroupLikeType: { group_int32: 1 }")
                  .SerializeBinary(),
              Yields(ParsedPayload(
                  EqualsTextProto("GroupLikeType: { group_int32: 1 }"))));
  EXPECT_THAT(RequiredTest("GroupFieldWithColon")
                  .ParseText(message(), "GroupLikeType: { group_int32: 1 }")
                  .SerializeText(),
              Yields(ParsedPayload(
                  EqualsTextProto("GroupLikeType: { group_int32: 1 }"))));
}

TEST_F(TextDelimitedTest, GroupFieldEmpty) {
  EXPECT_THAT(RequiredTest("GroupFieldEmpty")
                  .ParseText(message(), "GroupLikeType {}")
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto("GroupLikeType {}"))));
  EXPECT_THAT(RequiredTest("GroupFieldEmpty")
                  .ParseText(message(), "GroupLikeType {}")
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto("GroupLikeType {}"))));
}

TEST_F(TextDelimitedTest, GroupFieldExtension) {
  EXPECT_THAT(
      RequiredTest("GroupFieldExtension")
          .ParseText(message(),
                     "[protobuf_test_messages.editions.groupliketype] { c: 1 }")
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(
          "[protobuf_test_messages.editions.groupliketype] { c: 1 }"))));
  EXPECT_THAT(
      RequiredTest("GroupFieldExtension")
          .ParseText(message(),
                     "[protobuf_test_messages.editions.groupliketype] { c: 1 }")
          .SerializeText(),
      Yields(ParsedPayload(EqualsTextProto(
          "[protobuf_test_messages.editions.groupliketype] { c: 1 }"))));
}

TEST_F(TextDelimitedTest, DelimitedFieldExtension) {
  EXPECT_THAT(
      RequiredTest("DelimitedFieldExtension")
          .ParseText(message(),
                     "[protobuf_test_messages.editions.delimited_ext] { c: 1 }")
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(
          "[protobuf_test_messages.editions.delimited_ext] { c: 1 }"))));
  EXPECT_THAT(
      RequiredTest("DelimitedFieldExtension")
          .ParseText(message(),
                     "[protobuf_test_messages.editions.delimited_ext] { c: 1 }")
          .SerializeText(),
      Yields(ParsedPayload(EqualsTextProto(
          "[protobuf_test_messages.editions.delimited_ext] { c: 1 }"))));
}


// The lower-cased group name (i.e. the implicit field name) is accepted.
TEST_F(TextDelimitedTest, DelimitedFieldLowercased) {
  EXPECT_THAT(RequiredTest("DelimitedFieldLowercased")
                  .ParseText(message(), "groupliketype { group_int32: 1 }")
                  .SerializeBinary(),
              Yields(ParsedPayload(
                  EqualsTextProto("groupliketype { group_int32: 1 }"))));
  EXPECT_THAT(RequiredTest("DelimitedFieldLowercased")
                  .ParseText(message(), "groupliketype { group_int32: 1 }")
                  .SerializeText(),
              Yields(ParsedPayload(
                  EqualsTextProto("groupliketype { group_int32: 1 }"))));
}

TEST_F(TextDelimitedTest, DelimitedFieldLowercasedDifferent) {
  EXPECT_THAT(RequiredTest("DelimitedFieldLowercasedDifferent")
                  .ParseText(message(), "delimited_field { group_int32: 1 }")
                  .SerializeBinary(),
              Yields(ParsedPayload(
                  EqualsTextProto("delimited_field { group_int32: 1 }"))));
  EXPECT_THAT(RequiredTest("DelimitedFieldLowercasedDifferent")
                  .ParseText(message(), "delimited_field { group_int32: 1 }")
                  .SerializeText(),
              Yields(ParsedPayload(
                  EqualsTextProto("delimited_field { group_int32: 1 }"))));
}

// Extensions always use the field name, and never accept the message name.
TEST_F(TextDelimitedTest, DelimitedFieldExtensionMessageName) {
  EXPECT_THAT(RequiredTest("DelimitedFieldExtensionMessageName")
                  .ParseText(message(),
                             "[protobuf_test_messages.editions.GroupLikeType] "
                             "{ group_int32: 1 }")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// Extension names can contain whitespace and comments.
TEST_F(TextDelimitedTest, ExtensionNameWithWhitespace) {
  EXPECT_THAT(
      RequiredTest("ExtensionNameWithWhitespace")
          .ParseText(
              message(),
              "[protobuf _test_messages.edit\tions.exten\nsion_int32]: 1")
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(
          "[protobuf _test_messages.edit\tions.exten\nsion_int32]: 1"))));
  EXPECT_THAT(
      RequiredTest("ExtensionNameWithWhitespace")
          .ParseText(
              message(),
              "[protobuf _test_messages.edit\tions.exten\nsion_int32]: 1")
          .SerializeText(),
      Yields(ParsedPayload(EqualsTextProto(
          "[protobuf _test_messages.edit\tions.exten\nsion_int32]: 1"))));
}

TEST_F(TextDelimitedTest, ExtensionNameWithComment) {
  EXPECT_THAT(RequiredTest("ExtensionNameWithComment")
                  .ParseText(message(),
                             "[protobuf_test_messages.edit # comment "
                             "\nions.extension_int32]: 1")
                  .SerializeBinary(),
              Yields(ParsedPayload(
                  EqualsTextProto("[protobuf_test_messages.edit # comment "
                                  "\nions.extension_int32]: 1"))));
  EXPECT_THAT(RequiredTest("ExtensionNameWithComment")
                  .ParseText(message(),
                             "[protobuf_test_messages.edit # comment "
                             "\nions.extension_int32]: 1")
                  .SerializeText(),
              Yields(ParsedPayload(
                  EqualsTextProto("[protobuf_test_messages.edit # comment "
                                  "\nions.extension_int32]: 1"))));
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
