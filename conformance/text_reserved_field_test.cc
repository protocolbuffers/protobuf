// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
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
#include "binary_test_util.h"
#include "matchers.h"
#include "message_type_fixtures.h"
#include "test_environment.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::ValuesIn;

// The expected message of every test is the input itself, i.e. an empty
// message once the reserved field has been skipped.
using TextReservedFieldTest = MessageTypeConformanceTest;

TEST_P(TextReservedFieldTest, Boolean) {
  EXPECT_THAT(RequiredTest("ReservedFieldName.Boolean")
                  .ParseText(message(), "reserved_field: true")
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto("reserved_field: true"))));
  EXPECT_THAT(RequiredTest("ReservedFieldName.Boolean")
                  .ParseText(message(), "reserved_field: true")
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto("reserved_field: true"))));
}

TEST_P(TextReservedFieldTest, Integer) {
  EXPECT_THAT(RequiredTest("ReservedFieldName.Integer")
                  .ParseText(message(), "reserved_field: -123")
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto("reserved_field: -123"))));
  EXPECT_THAT(RequiredTest("ReservedFieldName.Integer")
                  .ParseText(message(), "reserved_field: -123")
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto("reserved_field: -123"))));
}

TEST_P(TextReservedFieldTest, Float) {
  EXPECT_THAT(RequiredTest("ReservedFieldName.Float")
                  .ParseText(message(), "reserved_field: 0.123")
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto("reserved_field: 0.123"))));
  EXPECT_THAT(RequiredTest("ReservedFieldName.Float")
                  .ParseText(message(), "reserved_field: 0.123")
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto("reserved_field: 0.123"))));
}

TEST_P(TextReservedFieldTest, Enum) {
  EXPECT_THAT(
      RequiredTest("ReservedFieldName.Enum")
          .ParseText(message(), "reserved_field: ENUM_VALUE")
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto("reserved_field: ENUM_VALUE"))));
  EXPECT_THAT(
      RequiredTest("ReservedFieldName.Enum")
          .ParseText(message(), "reserved_field: ENUM_VALUE")
          .SerializeText(),
      Yields(ParsedPayload(EqualsTextProto("reserved_field: ENUM_VALUE"))));
}

TEST_P(TextReservedFieldTest, String) {
  EXPECT_THAT(
      RequiredTest("ReservedFieldName.String")
          .ParseText(message(), "reserved_field: \"hello\"")
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto("reserved_field: \"hello\""))));
  EXPECT_THAT(
      RequiredTest("ReservedFieldName.String")
          .ParseText(message(), "reserved_field: \"hello\"")
          .SerializeText(),
      Yields(ParsedPayload(EqualsTextProto("reserved_field: \"hello\""))));
}

TEST_P(TextReservedFieldTest, Message) {
  EXPECT_THAT(
      RequiredTest("ReservedFieldName.Message")
          .ParseText(message(), "reserved_field: { a: 123 }")
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto("reserved_field: { a: 123 }"))));
  EXPECT_THAT(
      RequiredTest("ReservedFieldName.Message")
          .ParseText(message(), "reserved_field: { a: 123 }")
          .SerializeText(),
      Yields(ParsedPayload(EqualsTextProto("reserved_field: { a: 123 }"))));
}

TEST_P(TextReservedFieldTest, MessageAngleBrackets) {
  EXPECT_THAT(
      RequiredTest("ReservedFieldName.MessageAngleBrackets")
          .ParseText(message(), "reserved_field: < a: 123 >")
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto("reserved_field: < a: 123 >"))));
  EXPECT_THAT(
      RequiredTest("ReservedFieldName.MessageAngleBrackets")
          .ParseText(message(), "reserved_field: < a: 123 >")
          .SerializeText(),
      Yields(ParsedPayload(EqualsTextProto("reserved_field: < a: 123 >"))));
}

TEST_P(TextReservedFieldTest, RepeatedInteger) {
  EXPECT_THAT(
      RequiredTest("ReservedFieldName.RepeatedInteger")
          .ParseText(message(), "reserved_field: [-123, 456]")
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto("reserved_field: [-123, 456]"))));
  EXPECT_THAT(
      RequiredTest("ReservedFieldName.RepeatedInteger")
          .ParseText(message(), "reserved_field: [-123, 456]")
          .SerializeText(),
      Yields(ParsedPayload(EqualsTextProto("reserved_field: [-123, 456]"))));
}

// The trailing space of the legacy input is part of the request bytes and is
// kept.
TEST_P(TextReservedFieldTest, RepeatedFloat) {
  EXPECT_THAT(RequiredTest("ReservedFieldName.RepeatedFloat")
                  .ParseText(message(), "reserved_field: [0.123, 1e-10] ")
                  .SerializeBinary(),
              Yields(ParsedPayload(
                  EqualsTextProto("reserved_field: [0.123, 1e-10] "))));
  EXPECT_THAT(RequiredTest("ReservedFieldName.RepeatedFloat")
                  .ParseText(message(), "reserved_field: [0.123, 1e-10] ")
                  .SerializeText(),
              Yields(ParsedPayload(
                  EqualsTextProto("reserved_field: [0.123, 1e-10] "))));
}

TEST_P(TextReservedFieldTest, RepeatedString) {
  EXPECT_THAT(RequiredTest("ReservedFieldName.RepeatedString")
                  .ParseText(message(), R"(reserved_field: ["hello", "world"])")
                  .SerializeBinary(),
              Yields(ParsedPayload(
                  EqualsTextProto(R"(reserved_field: ["hello", "world"])"))));
  EXPECT_THAT(RequiredTest("ReservedFieldName.RepeatedString")
                  .ParseText(message(), R"(reserved_field: ["hello", "world"])")
                  .SerializeText(),
              Yields(ParsedPayload(
                  EqualsTextProto(R"(reserved_field: ["hello", "world"])"))));
}

INSTANTIATE_TEST_SUITE_P(All, TextReservedFieldTest,
                         ValuesIn(Proto3TestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
