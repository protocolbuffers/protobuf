// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// JSON conformance tests for integer fields: the value ranges of the 32- and
// 64-bit types, integers as JSON strings and as float literals, rejected
// out-of-range, non-integer, boolean, non-numeric and malformed values, and
// the 64-bit types being serialized as strings.  This replaces the "Integer
// fields" block of the legacy BinaryAndJsonConformanceSuiteImpl<M>::
// RunJsonTestsForNonRepeatedTypes(); the test names and the requests sent to
// the testee are identical to the legacy ones.
//
// Like the legacy RunValidJsonTest(), each valid input is sent twice: once to
// be serialized as binary ("<name>.ProtobufOutput") and once as JSON
// ("<name>.JsonOutput").  ExpectParseFailureForJson() asked for JSON output
// under a name without an output suffix, and the legacy "validators"
// (RunValidJsonTestWithValidator()) looked at the serialized JSON text itself
// under the name "<name>.Validator".

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

// Names a JSON-output test "<name>.Validator" like the legacy validators.
using LegacyName = internal::JsonSerializationOptions::LegacyName;

using JsonIntegerTest = MessageTypeConformanceTest;

// ---------------------------------------------------------------------------
// Value ranges.  64-bit values are quoted in JSON.
// ---------------------------------------------------------------------------

TEST_P(JsonIntegerTest, Int32FieldMaxValue) {
  constexpr absl::string_view kInput = R"({"optionalInt32": 2147483647})";
  constexpr absl::string_view kExpected = "optional_int32: 2147483647";
  EXPECT_THAT(RequiredTest("Int32FieldMaxValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("Int32FieldMaxValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonIntegerTest, Int32FieldMinValue) {
  constexpr absl::string_view kInput = R"({"optionalInt32": -2147483648})";
  constexpr absl::string_view kExpected = "optional_int32: -2147483648";
  EXPECT_THAT(RequiredTest("Int32FieldMinValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("Int32FieldMinValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonIntegerTest, Uint32FieldMaxValue) {
  constexpr absl::string_view kInput = R"({"optionalUint32": 4294967295})";
  constexpr absl::string_view kExpected = "optional_uint32: 4294967295";
  EXPECT_THAT(RequiredTest("Uint32FieldMaxValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("Uint32FieldMaxValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonIntegerTest, Int64FieldMaxValue) {
  constexpr absl::string_view kInput =
      R"({"optionalInt64": "9223372036854775807"})";
  constexpr absl::string_view kExpected = "optional_int64: 9223372036854775807";
  EXPECT_THAT(RequiredTest("Int64FieldMaxValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("Int64FieldMaxValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonIntegerTest, Int64FieldMinValue) {
  constexpr absl::string_view kInput =
      R"({"optionalInt64": "-9223372036854775808"})";
  constexpr absl::string_view kExpected =
      "optional_int64: -9223372036854775808";
  EXPECT_THAT(RequiredTest("Int64FieldMinValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("Int64FieldMinValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonIntegerTest, Uint64FieldMaxValue) {
  constexpr absl::string_view kInput =
      R"({"optionalUint64": "18446744073709551615"})";
  constexpr absl::string_view kExpected =
      "optional_uint64: 18446744073709551615";
  EXPECT_THAT(RequiredTest("Uint64FieldMaxValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("Uint64FieldMaxValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// While not the largest Int64, this is the largest Int64 which can be exactly
// represented within an IEEE-754 64-bit float, which is the expected level of
// interoperability guarantee.  Larger values may work in some implementations,
// but should not be relied upon.
TEST_P(JsonIntegerTest, Int64FieldMaxValueNotQuoted) {
  constexpr absl::string_view kInput =
      R"({"optionalInt64": 9223372036854774784})";
  constexpr absl::string_view kExpected = "optional_int64: 9223372036854774784";
  EXPECT_THAT(RequiredTest("Int64FieldMaxValueNotQuoted")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("Int64FieldMaxValueNotQuoted")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonIntegerTest, Int64FieldMinValueNotQuoted) {
  constexpr absl::string_view kInput =
      R"({"optionalInt64": -9223372036854775808})";
  constexpr absl::string_view kExpected =
      "optional_int64: -9223372036854775808";
  EXPECT_THAT(RequiredTest("Int64FieldMinValueNotQuoted")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("Int64FieldMinValueNotQuoted")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// Largest interoperable Uint64; see the comment above for
// Int64FieldMaxValueNotQuoted.
TEST_P(JsonIntegerTest, Uint64FieldMaxValueNotQuoted) {
  constexpr absl::string_view kInput =
      R"({"optionalUint64": 18446744073709549568})";
  constexpr absl::string_view kExpected =
      "optional_uint64: 18446744073709549568";
  EXPECT_THAT(RequiredTest("Uint64FieldMaxValueNotQuoted")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("Uint64FieldMaxValueNotQuoted")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// Values can be represented as JSON strings.
TEST_P(JsonIntegerTest, Int32FieldStringValue) {
  constexpr absl::string_view kInput = R"({"optionalInt32": "2147483647"})";
  constexpr absl::string_view kExpected = "optional_int32: 2147483647";
  EXPECT_THAT(RequiredTest("Int32FieldStringValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("Int32FieldStringValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonIntegerTest, Int32FieldStringValueEscaped) {
  constexpr absl::string_view kInput =
      R"({"optionalInt32": "2\u003147483647"})";
  constexpr absl::string_view kExpected = "optional_int32: 2147483647";
  EXPECT_THAT(RequiredTest("Int32FieldStringValueEscaped")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("Int32FieldStringValueEscaped")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonIntegerTest, Int32FieldStringValueZero) {
  constexpr absl::string_view kInput = R"({"optionalInt32": "0"})";
  constexpr absl::string_view kExpected = "optional_int32: 0";
  EXPECT_THAT(RequiredTest("Int32FieldStringValueZero")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("Int32FieldStringValueZero")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonIntegerTest, Int32FieldQuotedExponentialValue) {
  constexpr absl::string_view kInput = R"({"optionalInt32": "1e5"})";
  constexpr absl::string_view kExpected = "optional_int32: 100000";
  EXPECT_THAT(RequiredTest("Int32FieldQuotedExponentialValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("Int32FieldQuotedExponentialValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// ---------------------------------------------------------------------------
// Parsers reject out-of-bound integer values.
// ---------------------------------------------------------------------------

TEST_P(JsonIntegerTest, Int32FieldTooLarge) {
  EXPECT_THAT(RequiredTest("Int32FieldTooLarge")
                  .ParseJson(message(), R"({"optionalInt32": 2147483648})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Int32FieldTooSmall) {
  EXPECT_THAT(RequiredTest("Int32FieldTooSmall")
                  .ParseJson(message(), R"({"optionalInt32": -2147483649})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Uint32FieldTooLarge) {
  EXPECT_THAT(RequiredTest("Uint32FieldTooLarge")
                  .ParseJson(message(), R"({"optionalUint32": 4294967296})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Int64FieldTooLarge) {
  EXPECT_THAT(
      RequiredTest("Int64FieldTooLarge")
          .ParseJson(message(), R"({"optionalInt64": "9223372036854775808"})")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Int64FieldTooSmall) {
  EXPECT_THAT(
      RequiredTest("Int64FieldTooSmall")
          .ParseJson(message(), R"({"optionalInt64": "-9223372036854775809"})")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Uint64FieldTooLarge) {
  EXPECT_THAT(
      RequiredTest("Uint64FieldTooLarge")
          .ParseJson(message(), R"({"optionalUint64": "18446744073709551616"})")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Uint64QuotedExponentFieldTooLarge) {
  EXPECT_THAT(RequiredTest("Uint64QuotedExponentFieldTooLarge")
                  .ParseJson(message(), R"({"optionalUint64": "1e536870000"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// ---------------------------------------------------------------------------
// Parsers reject non-integer numeric values.
// ---------------------------------------------------------------------------

TEST_P(JsonIntegerTest, Int32FieldNotInteger) {
  EXPECT_THAT(RequiredTest("Int32FieldNotInteger")
                  .ParseJson(message(), R"({"optionalInt32": 0.5})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Uint32FieldNotInteger) {
  EXPECT_THAT(RequiredTest("Uint32FieldNotInteger")
                  .ParseJson(message(), R"({"optionalUint32": 0.5})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Int64FieldNotInteger) {
  EXPECT_THAT(RequiredTest("Int64FieldNotInteger")
                  .ParseJson(message(), R"({"optionalInt64": "0.5"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Uint64FieldNotInteger) {
  EXPECT_THAT(RequiredTest("Uint64FieldNotInteger")
                  .ParseJson(message(), R"({"optionalUint64": "0.5"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// ---------------------------------------------------------------------------
// Parsers reject boolean values for integer fields.
// ---------------------------------------------------------------------------

TEST_P(JsonIntegerTest, Int32FieldTrueValue) {
  EXPECT_THAT(RequiredTest("Int32FieldTrueValue")
                  .ParseJson(message(), R"({"optionalInt32": true})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Int32FieldFalseValue) {
  EXPECT_THAT(RequiredTest("Int32FieldFalseValue")
                  .ParseJson(message(), R"({"optionalInt32": false})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// ---------------------------------------------------------------------------
// Parsers reject non-numeric string values.
// ---------------------------------------------------------------------------

TEST_P(JsonIntegerTest, Int32FieldStringValuePartiallyNumeric) {
  EXPECT_THAT(RequiredTest("Int32FieldStringValuePartiallyNumeric")
                  .ParseJson(message(), R"({"optionalInt32": "12abc"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Int32FieldStringValuePartiallyNumericSpace) {
  EXPECT_THAT(RequiredTest("Int32FieldStringValuePartiallyNumericSpace")
                  .ParseJson(message(), R"({"optionalInt32": "12 34"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Int32FieldStringValuePartiallyNumericComma) {
  EXPECT_THAT(RequiredTest("Int32FieldStringValuePartiallyNumericComma")
                  .ParseJson(message(), R"({"optionalInt32": "12,34"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Int32FieldStringValuePartiallyNumericUnicode) {
  EXPECT_THAT(RequiredTest("Int32FieldStringValuePartiallyNumericUnicode")
                  .ParseJson(message(), R"({"optionalInt32": "12谷歌34"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Int32FieldStringValueNonNumeric) {
  EXPECT_THAT(RequiredTest("Int32FieldStringValueNonNumeric")
                  .ParseJson(message(), R"({"optionalInt32": "abc"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// ---------------------------------------------------------------------------
// Parsers reject empty string values.
// ---------------------------------------------------------------------------

TEST_P(JsonIntegerTest, Int32FieldEmptyString) {
  EXPECT_THAT(RequiredTest("Int32FieldEmptyString")
                  .ParseJson(message(), R"({"optionalInt32": ""})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Uint32FieldEmptyString) {
  EXPECT_THAT(RequiredTest("Uint32FieldEmptyString")
                  .ParseJson(message(), R"({"optionalUint32": ""})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Int64FieldEmptyString) {
  EXPECT_THAT(RequiredTest("Int64FieldEmptyString")
                  .ParseJson(message(), R"({"optionalInt64": ""})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Uint64FieldEmptyString) {
  EXPECT_THAT(RequiredTest("Uint64FieldEmptyString")
                  .ParseJson(message(), R"({"optionalUint64": ""})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// ---------------------------------------------------------------------------
// Integers represented as float values are accepted.
// ---------------------------------------------------------------------------

TEST_P(JsonIntegerTest, Int32FieldFloatTrailingZero) {
  constexpr absl::string_view kInput = R"({"optionalInt32": 100000.000})";
  constexpr absl::string_view kExpected = "optional_int32: 100000";
  EXPECT_THAT(RequiredTest("Int32FieldFloatTrailingZero")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("Int32FieldFloatTrailingZero")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonIntegerTest, Int32FieldExponentialFormat) {
  constexpr absl::string_view kInput = R"({"optionalInt32": 1e5})";
  constexpr absl::string_view kExpected = "optional_int32: 100000";
  EXPECT_THAT(RequiredTest("Int32FieldExponentialFormat")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("Int32FieldExponentialFormat")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonIntegerTest, Int32FieldMaxFloatValue) {
  constexpr absl::string_view kInput = R"({"optionalInt32": 2.147483647e9})";
  constexpr absl::string_view kExpected = "optional_int32: 2147483647";
  EXPECT_THAT(RequiredTest("Int32FieldMaxFloatValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("Int32FieldMaxFloatValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonIntegerTest, Int32FieldMinFloatValue) {
  constexpr absl::string_view kInput = R"({"optionalInt32": -2.147483648e9})";
  constexpr absl::string_view kExpected = "optional_int32: -2147483648";
  EXPECT_THAT(RequiredTest("Int32FieldMinFloatValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("Int32FieldMinFloatValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonIntegerTest, Uint32FieldMaxFloatValue) {
  constexpr absl::string_view kInput = R"({"optionalUint32": 4.294967295e9})";
  constexpr absl::string_view kExpected = "optional_uint32: 4294967295";
  EXPECT_THAT(RequiredTest("Uint32FieldMaxFloatValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("Uint32FieldMaxFloatValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// ---------------------------------------------------------------------------
// Parsers reject non-numeric and malformed numeric values.
// ---------------------------------------------------------------------------

TEST_P(JsonIntegerTest, Int32FieldNotNumber) {
  EXPECT_THAT(RequiredTest("Int32FieldNotNumber")
                  .ParseJson(message(), R"({"optionalInt32": "3x3"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Uint32FieldNotNumber) {
  EXPECT_THAT(RequiredTest("Uint32FieldNotNumber")
                  .ParseJson(message(), R"({"optionalUint32": "3x3"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Int64FieldNotNumber) {
  EXPECT_THAT(RequiredTest("Int64FieldNotNumber")
                  .ParseJson(message(), R"({"optionalInt64": "3x3"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Uint64FieldNotNumber) {
  EXPECT_THAT(RequiredTest("Uint64FieldNotNumber")
                  .ParseJson(message(), R"({"optionalUint64": "3x3"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// JSON does not allow "+" on numeric values.
TEST_P(JsonIntegerTest, Int32FieldPlusSign) {
  EXPECT_THAT(RequiredTest("Int32FieldPlusSign")
                  .ParseJson(message(), R"({"optionalInt32": +1})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// JSON doesn't allow leading 0s.
TEST_P(JsonIntegerTest, Int32FieldLeadingZero) {
  EXPECT_THAT(RequiredTest("Int32FieldLeadingZero")
                  .ParseJson(message(), R"({"optionalInt32": 01})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Int32FieldNegativeWithLeadingZero) {
  EXPECT_THAT(RequiredTest("Int32FieldNegativeWithLeadingZero")
                  .ParseJson(message(), R"({"optionalInt32": -01})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// String values must follow the same syntax rule.  Specifically leading or
// trailing spaces are not allowed.
TEST_P(JsonIntegerTest, Int32FieldLeadingSpace) {
  EXPECT_THAT(RequiredTest("Int32FieldLeadingSpace")
                  .ParseJson(message(), R"({"optionalInt32": " 1"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Int32FieldTrailingSpace) {
  EXPECT_THAT(RequiredTest("Int32FieldTrailingSpace")
                  .ParseJson(message(), R"({"optionalInt32": "1 "})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// ---------------------------------------------------------------------------
// 64-bit values are serialized as strings.
// ---------------------------------------------------------------------------

TEST_P(JsonIntegerTest, Int64FieldBeString) {
  EXPECT_THAT(RecommendedTest("Int64FieldBeString")
                  .ParseJson(message(), R"({"optionalInt64": 1})")
                  .SerializeJson({.legacy_name = LegacyName::kValidatorSuffix}),
              Yields(JsonPayload(
                  HasJsonMemberThat("optionalInt64", IsJsonString("1")))));
}

TEST_P(JsonIntegerTest, Uint64FieldBeString) {
  EXPECT_THAT(RecommendedTest("Uint64FieldBeString")
                  .ParseJson(message(), R"({"optionalUint64": 1})")
                  .SerializeJson({.legacy_name = LegacyName::kValidatorSuffix}),
              Yields(JsonPayload(
                  HasJsonMemberThat("optionalUint64", IsJsonString("1")))));
}

INSTANTIATE_TEST_SUITE_P(All, JsonIntegerTest, ValuesIn(AllTestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
