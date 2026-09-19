// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
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
#include "conformance/binary_test_util.h"
#include "conformance/matchers.h"
#include "conformance/message_type_fixtures.h"
#include "conformance/testee.h"
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
      Testee("GroupFieldNoColon")
          .ParseText(message(), R"pb(Data { group_int32: 1 })pb")
          .SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(R"pb(Data { group_int32: 1 })pb"))));
  EXPECT_THAT(
      Testee("GroupFieldNoColon")
          .ParseText(message(), R"pb(Data { group_int32: 1 })pb")
          .SerializeText(),
      Yields(WhenParsed(EqualsTextProto(R"pb(Data { group_int32: 1 })pb"))));
}

TEST_P(TextGroupTest, WithColon) {
  EXPECT_THAT(
      Testee("GroupFieldWithColon")
          .ParseText(message(), R"pb(Data: { group_int32: 1 })pb")
          .SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(R"pb(Data: { group_int32: 1 })pb"))));
  EXPECT_THAT(
      Testee("GroupFieldWithColon")
          .ParseText(message(), R"pb(Data: { group_int32: 1 })pb")
          .SerializeText(),
      Yields(WhenParsed(EqualsTextProto(R"pb(Data: { group_int32: 1 })pb"))));
}

TEST_P(TextGroupTest, Empty) {
  EXPECT_THAT(Testee("GroupFieldEmpty")
                  .ParseText(message(), R"pb(Data {})pb")
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(R"pb(Data {})pb"))));
  EXPECT_THAT(Testee("GroupFieldEmpty")
                  .ParseText(message(), R"pb(Data {})pb")
                  .SerializeText(),
              Yields(WhenParsed(EqualsTextProto(R"pb(Data {})pb"))));
}

TEST_P(TextGroupTest, MultiWord) {
  EXPECT_THAT(
      Testee("GroupFieldMultiWord")
          .ParseText(message(), R"pb(MultiWordGroupField { group_int32: 1 })pb")
          .SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(R"pb(
        MultiWordGroupField { group_int32: 1 }
      )pb"))));
  EXPECT_THAT(
      Testee("GroupFieldMultiWord")
          .ParseText(message(), R"pb(MultiWordGroupField { group_int32: 1 })pb")
          .SerializeText(),
      Yields(WhenParsed(EqualsTextProto(R"pb(
        MultiWordGroupField { group_int32: 1 }
      )pb"))));
}

// The lower-cased group name (i.e. the implicit field name) is accepted.
TEST_P(TextGroupTest, Lowercased) {
  EXPECT_THAT(
      Testee("GroupFieldLowercased")
          .ParseText(message(), R"pb(data { group_int32: 1 })pb")
          .SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(R"pb(data { group_int32: 1 })pb"))));
  EXPECT_THAT(
      Testee("GroupFieldLowercased")
          .ParseText(message(), R"pb(data { group_int32: 1 })pb")
          .SerializeText(),
      Yields(WhenParsed(EqualsTextProto(R"pb(data { group_int32: 1 })pb"))));
}

TEST_P(TextGroupTest, LowercasedMultiWord) {
  EXPECT_THAT(
      Testee("GroupFieldLowercasedMultiWord")
          .ParseText(message(), R"pb(multiwordgroupfield { group_int32: 1 })pb")
          .SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(R"pb(
        multiwordgroupfield { group_int32: 1 }
      )pb"))));
  EXPECT_THAT(
      Testee("GroupFieldLowercasedMultiWord")
          .ParseText(message(), R"pb(multiwordgroupfield { group_int32: 1 })pb")
          .SerializeText(),
      Yields(WhenParsed(EqualsTextProto(R"pb(
        multiwordgroupfield { group_int32: 1 }
      )pb"))));
}

// An extension of group type is named by the extension field.
TEST_P(TextGroupTest, Extension) {
  const FieldDescriptor* extension =
      message()->file()->FindExtensionByName("groupfield");
  ASSERT_NE(extension, nullptr);
  const std::string input = absl::StrCat(
      "[", extension->PrintableNameForExtension(), "] { group_int32: 1 }");
  EXPECT_THAT(Testee("GroupFieldExtension")
                  .ParseText(message(), input)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(input))));
  EXPECT_THAT(
      Testee("GroupFieldExtension").ParseText(message(), input).SerializeText(),
      Yields(WhenParsed(EqualsTextProto(input))));
}

// ... and never by the name of its group message type.
TEST_P(TextGroupTest, ExtensionGroupName) {
  const Descriptor* group =
      message()->file()->FindMessageTypeByName("GroupField");
  ASSERT_NE(group, nullptr);
  EXPECT_THAT(Testee("GroupFieldExtensionGroupName")
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
  EXPECT_THAT(
      Testee("GroupFieldNoColon")
          .ParseText(message(), R"pb(GroupLikeType { group_int32: 1 })pb")
          .SerializeBinary(),
      Yields(WhenParsed(
          EqualsTextProto(R"pb(GroupLikeType { group_int32: 1 })pb"))));
  EXPECT_THAT(
      Testee("GroupFieldNoColon")
          .ParseText(message(), R"pb(GroupLikeType { group_int32: 1 })pb")
          .SerializeText(),
      Yields(WhenParsed(
          EqualsTextProto(R"pb(GroupLikeType { group_int32: 1 })pb"))));
}

TEST_F(TextDelimitedTest, GroupFieldWithColon) {
  EXPECT_THAT(
      Testee("GroupFieldWithColon")
          .ParseText(message(), R"pb(GroupLikeType: { group_int32: 1 })pb")
          .SerializeBinary(),
      Yields(WhenParsed(
          EqualsTextProto(R"pb(GroupLikeType: { group_int32: 1 })pb"))));
  EXPECT_THAT(
      Testee("GroupFieldWithColon")
          .ParseText(message(), R"pb(GroupLikeType: { group_int32: 1 })pb")
          .SerializeText(),
      Yields(WhenParsed(
          EqualsTextProto(R"pb(GroupLikeType: { group_int32: 1 })pb"))));
}

TEST_F(TextDelimitedTest, GroupFieldEmpty) {
  EXPECT_THAT(Testee("GroupFieldEmpty")
                  .ParseText(message(), R"pb(GroupLikeType {})pb")
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(R"pb(GroupLikeType {})pb"))));
  EXPECT_THAT(Testee("GroupFieldEmpty")
                  .ParseText(message(), R"pb(GroupLikeType {})pb")
                  .SerializeText(),
              Yields(WhenParsed(EqualsTextProto(R"pb(GroupLikeType {})pb"))));
}

TEST_F(TextDelimitedTest, GroupFieldExtension) {
  EXPECT_THAT(
      Testee("GroupFieldExtension")
          .ParseText(
              message(),
              R"pb([protobuf_test_messages.editions.groupliketype] { c: 1 })pb")
          .SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(
          R"pb([protobuf_test_messages.editions.groupliketype] { c: 1 })pb"))));
  EXPECT_THAT(
      Testee("GroupFieldExtension")
          .ParseText(
              message(),
              R"pb([protobuf_test_messages.editions.groupliketype] { c: 1 })pb")
          .SerializeText(),
      Yields(WhenParsed(EqualsTextProto(
          R"pb([protobuf_test_messages.editions.groupliketype] { c: 1 })pb"))));
}

TEST_F(TextDelimitedTest, DelimitedFieldExtension) {
  EXPECT_THAT(
      Testee("DelimitedFieldExtension")
          .ParseText(
              message(),
              R"pb([protobuf_test_messages.editions.delimited_ext] { c: 1 })pb")
          .SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(
          R"pb([protobuf_test_messages.editions.delimited_ext] { c: 1 })pb"))));
  EXPECT_THAT(
      Testee("DelimitedFieldExtension")
          .ParseText(
              message(),
              R"pb([protobuf_test_messages.editions.delimited_ext] { c: 1 })pb")
          .SerializeText(),
      Yields(WhenParsed(EqualsTextProto(
          R"pb([protobuf_test_messages.editions.delimited_ext] { c: 1 })pb"))));
}


// The lower-cased group name (i.e. the implicit field name) is accepted.
TEST_F(TextDelimitedTest, DelimitedFieldLowercased) {
  EXPECT_THAT(
      Testee("DelimitedFieldLowercased")
          .ParseText(message(), R"pb(groupliketype { group_int32: 1 })pb")
          .SerializeBinary(),
      Yields(WhenParsed(
          EqualsTextProto(R"pb(groupliketype { group_int32: 1 })pb"))));
  EXPECT_THAT(
      Testee("DelimitedFieldLowercased")
          .ParseText(message(), R"pb(groupliketype { group_int32: 1 })pb")
          .SerializeText(),
      Yields(WhenParsed(
          EqualsTextProto(R"pb(groupliketype { group_int32: 1 })pb"))));
}

TEST_F(TextDelimitedTest, DelimitedFieldLowercasedDifferent) {
  EXPECT_THAT(
      Testee("DelimitedFieldLowercasedDifferent")
          .ParseText(message(), R"pb(delimited_field { group_int32: 1 })pb")
          .SerializeBinary(),
      Yields(WhenParsed(
          EqualsTextProto(R"pb(delimited_field { group_int32: 1 })pb"))));
  EXPECT_THAT(
      Testee("DelimitedFieldLowercasedDifferent")
          .ParseText(message(), R"pb(delimited_field { group_int32: 1 })pb")
          .SerializeText(),
      Yields(WhenParsed(
          EqualsTextProto(R"pb(delimited_field { group_int32: 1 })pb"))));
}

// TODO: Write the plain-string inputs below as R"pb(...)pb" raw
// strings once the migration's request goldens are gone.  The goldens pin the
// exact request bytes, and clang-format would reformat the whitespace of these
// inputs inside a pb raw string.

// Extensions always use the field name, and never accept the message name.
TEST_F(TextDelimitedTest, DelimitedFieldExtensionMessageName) {
  EXPECT_THAT(Testee("DelimitedFieldExtensionMessageName")
                  .ParseText(message(),
                             "[protobuf_test_messages.editions.GroupLikeType] "
                             "{ group_int32: 1 }")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// Extension names can contain whitespace and comments.
TEST_F(TextDelimitedTest, ExtensionNameWithWhitespace) {
  EXPECT_THAT(
      Testee("ExtensionNameWithWhitespace")
          .ParseText(
              message(),
              "[protobuf _test_messages.edit\tions.exten\nsion_int32]: 1")
          .SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(
          "[protobuf _test_messages.edit\tions.exten\nsion_int32]: 1"))));
  EXPECT_THAT(
      Testee("ExtensionNameWithWhitespace")
          .ParseText(
              message(),
              "[protobuf _test_messages.edit\tions.exten\nsion_int32]: 1")
          .SerializeText(),
      Yields(WhenParsed(EqualsTextProto(
          "[protobuf _test_messages.edit\tions.exten\nsion_int32]: 1"))));
}

TEST_F(TextDelimitedTest, ExtensionNameWithComment) {
  EXPECT_THAT(Testee("ExtensionNameWithComment")
                  .ParseText(message(),
                             "[protobuf_test_messages.edit # comment "
                             "\nions.extension_int32]: 1")
                  .SerializeBinary(),
              Yields(WhenParsed(
                  EqualsTextProto("[protobuf_test_messages.edit # comment "
                                  "\nions.extension_int32]: 1"))));
  EXPECT_THAT(Testee("ExtensionNameWithComment")
                  .ParseText(message(),
                             "[protobuf_test_messages.edit # comment "
                             "\nions.extension_int32]: 1")
                  .SerializeText(),
              Yields(WhenParsed(
                  EqualsTextProto("[protobuf_test_messages.edit # comment "
                                  "\nions.extension_int32]: 1"))));
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
