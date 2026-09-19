// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// JSON conformance tests for enum fields: values by name and by number,
// rejected unquoted names, arrays and booleans, and, for proto3 message
// types only, like the legacy suite, enum aliases and unknown values being
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
#include "conformance/binary_test_util.h"
#include "conformance/json_test_util.h"
#include "conformance/matchers.h"
#include "conformance/message_type_fixtures.h"
#include "conformance/testee.h"

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
  constexpr absl::string_view kInput =
      R"json({"optionalNestedEnum": "FOO"})json";
  constexpr absl::string_view kExpected = R"pb(optional_nested_enum: FOO)pb";
  EXPECT_THAT(
      Testee("EnumField").ParseJson(message(), kInput).SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("EnumField").ParseJson(message(), kInput).SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

// Enum values must be represented as strings.
TEST_P(JsonEnumTest, EnumFieldNotQuoted) {
  EXPECT_THAT(
      Testee("EnumFieldNotQuoted")
          .ParseJson(message(), R"json({"optionalNestedEnum": FOO})json")
          .ParseOnly(),
      Yields(IsParseError()));
}

// Numeric values are allowed.
TEST_P(JsonEnumTest, EnumFieldNumericValueZero) {
  constexpr absl::string_view kInput = R"json({"optionalNestedEnum": 0})json";
  constexpr absl::string_view kExpected = R"pb(optional_nested_enum: FOO)pb";
  EXPECT_THAT(Testee("EnumFieldNumericValueZero")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("EnumFieldNumericValueZero")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonEnumTest, EnumFieldNumericValueNonZero) {
  constexpr absl::string_view kInput = R"json({"optionalNestedEnum": 1})json";
  constexpr absl::string_view kExpected = R"pb(optional_nested_enum: BAR)pb";
  EXPECT_THAT(Testee("EnumFieldNumericValueNonZero")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("EnumFieldNumericValueNonZero")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

// Arrays are not allowed for non-repeated enum fields.
TEST_P(JsonEnumTest, EnumFieldSingleElementArrayEnumName) {
  EXPECT_THAT(
      Testee("EnumFieldSingleElementArrayEnumName")
          .ParseJson(message(), R"json({"optionalNestedEnum": ["FOO"]})json")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonEnumTest, EnumFieldSingleElementArrayNumericValue) {
  EXPECT_THAT(
      Testee("EnumFieldSingleElementArrayNumericValue")
          .ParseJson(message(), R"json({"optionalNestedEnum": [2]})json")
          .ParseOnly(),
      Yields(IsParseError()));
}

// Booleans are not allowed for enum fields.
TEST_P(JsonEnumTest, EnumFieldTrueValue) {
  EXPECT_THAT(
      Testee("EnumFieldTrueValue")
          .ParseJson(message(), R"json({"optionalNestedEnum": true})json")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonEnumTest, EnumFieldFalseValue) {
  EXPECT_THAT(
      Testee("EnumFieldFalseValue")
          .ParseJson(message(), R"json({"optionalNestedEnum": false})json")
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
      R"json({"optionalAliasedEnum": "ALIAS_BAZ"})json";
  constexpr absl::string_view kExpected =
      R"pb(optional_aliased_enum: ALIAS_BAZ)pb";
  EXPECT_THAT(Testee("EnumFieldWithAlias")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("EnumFieldWithAlias").ParseJson(message(), kInput).SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonProto3EnumTest, EnumFieldWithAliasUseAlias) {
  constexpr absl::string_view kInput =
      R"json({"optionalAliasedEnum": "MOO"})json";
  constexpr absl::string_view kExpected =
      R"pb(optional_aliased_enum: ALIAS_BAZ)pb";
  EXPECT_THAT(Testee("EnumFieldWithAliasUseAlias")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("EnumFieldWithAliasUseAlias")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonProto3EnumTest, EnumFieldWithAliasLowerCase) {
  constexpr absl::string_view kInput =
      R"json({"optionalAliasedEnum": "moo"})json";
  constexpr absl::string_view kExpected =
      R"pb(optional_aliased_enum: ALIAS_BAZ)pb";
  EXPECT_THAT(Testee("EnumFieldWithAliasLowerCase")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("EnumFieldWithAliasLowerCase")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonProto3EnumTest, EnumFieldWithAliasDifferentCase) {
  constexpr absl::string_view kInput =
      R"json({"optionalAliasedEnum": "bAz"})json";
  constexpr absl::string_view kExpected =
      R"pb(optional_aliased_enum: ALIAS_BAZ)pb";
  EXPECT_THAT(Testee("EnumFieldWithAliasDifferentCase")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("EnumFieldWithAliasDifferentCase")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

// Unknown enum values are represented as numeric values.
TEST_P(JsonProto3EnumTest, EnumFieldUnknownValue) {
  EXPECT_THAT(
      Testee("EnumFieldUnknownValue")
          .ParseJson(message(), R"json({"optionalNestedEnum": 123})json")
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
