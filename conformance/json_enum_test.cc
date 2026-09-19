// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// JSON conformance tests for enum fields: values by name and by number,
// rejected unquoted names, arrays and booleans, and -- for proto3 message
// types only, like the legacy suite -- enum aliases and unknown values being
// serialized as numbers.  This replaces the "Enum fields" block of the legacy
// BinaryAndJsonConformanceSuiteImpl<M>::RunJsonTestsForNonRepeatedTypes(); the
// test names and the requests sent to the testee are identical to the legacy
// ones.
//
// Like the legacy RunValidJsonTest(), each valid input is sent twice: once to
// be serialized as binary ("<name>.ProtobufOutput") and once as JSON
// ("<name>.JsonOutput").  ExpectParseFailureForJson() asked for JSON output
// under a name without an output suffix, and the legacy "validators"
// (RunValidJsonTestWithValidator()) looked at the serialized JSON text itself.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "binary_test_util.h"
#include "json_test_util.h"
#include "matchers.h"
#include "message_type_fixtures.h"
#include "test_environment.h"
#include "testee.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::ValuesIn;

// ---------------------------------------------------------------------------
// Every message type.
// ---------------------------------------------------------------------------

using JsonEnumTest = MessageTypeConformanceTest;

TEST_P(JsonEnumTest, EnumField) {
  constexpr absl::string_view kInput = R"({"optionalNestedEnum": "FOO"})";
  constexpr absl::string_view kExpected = "optional_nested_enum: FOO";
  EXPECT_THAT(
      RequiredTest("EnumField").ParseJson(message(), kInput).SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      RequiredTest("EnumField").ParseJson(message(), kInput).SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// Enum values must be represented as strings.
TEST_P(JsonEnumTest, EnumFieldNotQuoted) {
  EXPECT_THAT(RequiredTest("EnumFieldNotQuoted")
                  .ParseJson(message(), R"({"optionalNestedEnum": FOO})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// Numeric values are allowed.
TEST_P(JsonEnumTest, EnumFieldNumericValueZero) {
  constexpr absl::string_view kInput = R"({"optionalNestedEnum": 0})";
  constexpr absl::string_view kExpected = "optional_nested_enum: FOO";
  EXPECT_THAT(RequiredTest("EnumFieldNumericValueZero")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("EnumFieldNumericValueZero")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonEnumTest, EnumFieldNumericValueNonZero) {
  constexpr absl::string_view kInput = R"({"optionalNestedEnum": 1})";
  constexpr absl::string_view kExpected = "optional_nested_enum: BAR";
  EXPECT_THAT(RequiredTest("EnumFieldNumericValueNonZero")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("EnumFieldNumericValueNonZero")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// Arrays are not allowed for non-repeated enum fields.
TEST_P(JsonEnumTest, EnumFieldSingleElementArrayEnumName) {
  EXPECT_THAT(RequiredTest("EnumFieldSingleElementArrayEnumName")
                  .ParseJson(message(), R"({"optionalNestedEnum": ["FOO"]})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonEnumTest, EnumFieldSingleElementArrayNumericValue) {
  EXPECT_THAT(RequiredTest("EnumFieldSingleElementArrayNumericValue")
                  .ParseJson(message(), R"({"optionalNestedEnum": [2]})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// Booleans are not allowed for enum fields.
TEST_P(JsonEnumTest, EnumFieldTrueValue) {
  EXPECT_THAT(RequiredTest("EnumFieldTrueValue")
                  .ParseJson(message(), R"({"optionalNestedEnum": true})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonEnumTest, EnumFieldFalseValue) {
  EXPECT_THAT(RequiredTest("EnumFieldFalseValue")
                  .ParseJson(message(), R"({"optionalNestedEnum": false})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

INSTANTIATE_TEST_SUITE_P(All, JsonEnumTest, ValuesIn(AllTestMessageTypes()),
                         MessageTypeParamName);

// ---------------------------------------------------------------------------
// Proto3 message types only: enum aliases and unknown (open enum) values.
// ---------------------------------------------------------------------------

using JsonProto3EnumTest = MessageTypeConformanceTest;

// Enum fields with alias.
TEST_P(JsonProto3EnumTest, EnumFieldWithAlias) {
  constexpr absl::string_view kInput =
      R"({"optionalAliasedEnum": "ALIAS_BAZ"})";
  constexpr absl::string_view kExpected = "optional_aliased_enum: ALIAS_BAZ";
  EXPECT_THAT(RequiredTest("EnumFieldWithAlias")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("EnumFieldWithAlias")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonProto3EnumTest, EnumFieldWithAliasUseAlias) {
  constexpr absl::string_view kInput = R"({"optionalAliasedEnum": "MOO"})";
  constexpr absl::string_view kExpected = "optional_aliased_enum: ALIAS_BAZ";
  EXPECT_THAT(RequiredTest("EnumFieldWithAliasUseAlias")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("EnumFieldWithAliasUseAlias")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonProto3EnumTest, EnumFieldWithAliasLowerCase) {
  constexpr absl::string_view kInput = R"({"optionalAliasedEnum": "moo"})";
  constexpr absl::string_view kExpected = "optional_aliased_enum: ALIAS_BAZ";
  EXPECT_THAT(RequiredTest("EnumFieldWithAliasLowerCase")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("EnumFieldWithAliasLowerCase")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonProto3EnumTest, EnumFieldWithAliasDifferentCase) {
  constexpr absl::string_view kInput = R"({"optionalAliasedEnum": "bAz"})";
  constexpr absl::string_view kExpected = "optional_aliased_enum: ALIAS_BAZ";
  EXPECT_THAT(RequiredTest("EnumFieldWithAliasDifferentCase")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("EnumFieldWithAliasDifferentCase")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// Unknown enum values are represented as numeric values.
TEST_P(JsonProto3EnumTest, EnumFieldUnknownValue) {
  EXPECT_THAT(RequiredTest("EnumFieldUnknownValue")
                  .ParseJson(message(), R"({"optionalNestedEnum": 123})")
                  .SerializeJson(),
              Yields(JsonPayload(
                  HasJsonMemberThat("optionalNestedEnum", IsJsonInt(123)))));
}

INSTANTIATE_TEST_SUITE_P(All, JsonProto3EnumTest,
                         ValuesIn(Proto3TestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
