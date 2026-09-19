// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Text-format conformance tests for reserved field names: a field name that
// the message reserves (reserved_field in the test message types) is skipped
// by the parser, whatever the shape of its value.  This replaces the legacy
// reserved-field block of TextFormatConformanceTestSuiteImpl<M>::RunAllTests(),
// which ran for the proto3-style message types only; the test names and the
// requests sent to the testee are identical to the legacy ones.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "conformance/binary_test_util.h"
#include "conformance/matchers.h"
#include "conformance/message_type_fixtures.h"
#include "conformance/testee.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::ValuesIn;

// The expected message of every test is the input itself, i.e. an empty
// message once the reserved field has been skipped.
using TextReservedFieldTest = MessageTypeConformanceTest;

TEST_P(TextReservedFieldTest, Boolean) {
  EXPECT_THAT(
      Testee("ReservedFieldName.Boolean")
          .ParseText(message(), R"pb(reserved_field: true)pb")
          .SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(R"pb(reserved_field: true)pb"))));
  EXPECT_THAT(
      Testee("ReservedFieldName.Boolean")
          .ParseText(message(), R"pb(reserved_field: true)pb")
          .SerializeText(),
      Yields(WhenParsed(EqualsTextProto(R"pb(reserved_field: true)pb"))));
}

TEST_P(TextReservedFieldTest, Integer) {
  EXPECT_THAT(
      Testee("ReservedFieldName.Integer")
          .ParseText(message(), R"pb(reserved_field: -123)pb")
          .SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(R"pb(reserved_field: -123)pb"))));
  EXPECT_THAT(
      Testee("ReservedFieldName.Integer")
          .ParseText(message(), R"pb(reserved_field: -123)pb")
          .SerializeText(),
      Yields(WhenParsed(EqualsTextProto(R"pb(reserved_field: -123)pb"))));
}

TEST_P(TextReservedFieldTest, Float) {
  EXPECT_THAT(
      Testee("ReservedFieldName.Float")
          .ParseText(message(), R"pb(reserved_field: 0.123)pb")
          .SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(R"pb(reserved_field: 0.123)pb"))));
  EXPECT_THAT(
      Testee("ReservedFieldName.Float")
          .ParseText(message(), R"pb(reserved_field: 0.123)pb")
          .SerializeText(),
      Yields(WhenParsed(EqualsTextProto(R"pb(reserved_field: 0.123)pb"))));
}

TEST_P(TextReservedFieldTest, Enum) {
  EXPECT_THAT(
      Testee("ReservedFieldName.Enum")
          .ParseText(message(), R"pb(reserved_field: ENUM_VALUE)pb")
          .SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(R"pb(reserved_field: ENUM_VALUE)pb"))));
  EXPECT_THAT(
      Testee("ReservedFieldName.Enum")
          .ParseText(message(), R"pb(reserved_field: ENUM_VALUE)pb")
          .SerializeText(),
      Yields(WhenParsed(EqualsTextProto(R"pb(reserved_field: ENUM_VALUE)pb"))));
}

TEST_P(TextReservedFieldTest, String) {
  EXPECT_THAT(
      Testee("ReservedFieldName.String")
          .ParseText(message(), R"pb(reserved_field: "hello")pb")
          .SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(R"pb(reserved_field: "hello")pb"))));
  EXPECT_THAT(
      Testee("ReservedFieldName.String")
          .ParseText(message(), R"pb(reserved_field: "hello")pb")
          .SerializeText(),
      Yields(WhenParsed(EqualsTextProto(R"pb(reserved_field: "hello")pb"))));
}

TEST_P(TextReservedFieldTest, Message) {
  EXPECT_THAT(
      Testee("ReservedFieldName.Message")
          .ParseText(message(), R"pb(reserved_field: { a: 123 })pb")
          .SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(R"pb(reserved_field: { a: 123 })pb"))));
  EXPECT_THAT(
      Testee("ReservedFieldName.Message")
          .ParseText(message(), R"pb(reserved_field: { a: 123 })pb")
          .SerializeText(),
      Yields(WhenParsed(EqualsTextProto(R"pb(reserved_field: { a: 123 })pb"))));
}

TEST_P(TextReservedFieldTest, MessageAngleBrackets) {
  EXPECT_THAT(
      Testee("ReservedFieldName.MessageAngleBrackets")
          .ParseText(message(), R"pb(reserved_field: < a: 123 >)pb")
          .SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(R"pb(reserved_field: < a: 123 >)pb"))));
  EXPECT_THAT(
      Testee("ReservedFieldName.MessageAngleBrackets")
          .ParseText(message(), R"pb(reserved_field: < a: 123 >)pb")
          .SerializeText(),
      Yields(WhenParsed(EqualsTextProto(R"pb(reserved_field: < a: 123 >)pb"))));
}

// TODO: Write the plain-string inputs below as R"pb(...)pb" raw
// strings once the migration's request goldens are gone.  The goldens pin the
// exact request bytes, and clang-format would reformat the whitespace of these
// inputs inside a pb raw string.
TEST_P(TextReservedFieldTest, RepeatedInteger) {
  EXPECT_THAT(
      Testee("ReservedFieldName.RepeatedInteger")
          .ParseText(message(), "reserved_field: [-123, 456]")
          .SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto("reserved_field: [-123, 456]"))));
  EXPECT_THAT(
      Testee("ReservedFieldName.RepeatedInteger")
          .ParseText(message(), "reserved_field: [-123, 456]")
          .SerializeText(),
      Yields(WhenParsed(EqualsTextProto("reserved_field: [-123, 456]"))));
}

// The trailing space of the legacy input is part of the request bytes and is
// kept.
TEST_P(TextReservedFieldTest, RepeatedFloat) {
  EXPECT_THAT(
      Testee("ReservedFieldName.RepeatedFloat")
          .ParseText(message(), "reserved_field: [0.123, 1e-10] ")
          .SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto("reserved_field: [0.123, 1e-10] "))));
  EXPECT_THAT(
      Testee("ReservedFieldName.RepeatedFloat")
          .ParseText(message(), "reserved_field: [0.123, 1e-10] ")
          .SerializeText(),
      Yields(WhenParsed(EqualsTextProto("reserved_field: [0.123, 1e-10] "))));
}

TEST_P(TextReservedFieldTest, RepeatedString) {
  EXPECT_THAT(Testee("ReservedFieldName.RepeatedString")
                  .ParseText(message(), R"(reserved_field: ["hello", "world"])")
                  .SerializeBinary(),
              Yields(WhenParsed(
                  EqualsTextProto(R"(reserved_field: ["hello", "world"])"))));
  EXPECT_THAT(Testee("ReservedFieldName.RepeatedString")
                  .ParseText(message(), R"(reserved_field: ["hello", "world"])")
                  .SerializeText(),
              Yields(WhenParsed(
                  EqualsTextProto(R"(reserved_field: ["hello", "world"])"))));
}

INSTANTIATE_TEST_SUITE_P(All, TextReservedFieldTest,
                         ValuesIn(Proto3TestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
