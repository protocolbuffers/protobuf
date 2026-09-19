// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
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

using JsonIntegerTest = MessageTypeConformanceTest;

// ---------------------------------------------------------------------------
// Value ranges.  64-bit values are quoted in JSON.
// ---------------------------------------------------------------------------

TEST_P(JsonIntegerTest, Int32FieldMaxValue) {
  constexpr absl::string_view kInput =
      R"json({"optionalInt32": 2147483647})json";
  constexpr absl::string_view kExpected = R"pb(optional_int32: 2147483647)pb";
  EXPECT_THAT(Testee("Int32FieldMaxValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("Int32FieldMaxValue").ParseJson(message(), kInput).SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonIntegerTest, Int32FieldMinValue) {
  constexpr absl::string_view kInput =
      R"json({"optionalInt32": -2147483648})json";
  constexpr absl::string_view kExpected = R"pb(optional_int32: -2147483648)pb";
  EXPECT_THAT(Testee("Int32FieldMinValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("Int32FieldMinValue").ParseJson(message(), kInput).SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonIntegerTest, Uint32FieldMaxValue) {
  constexpr absl::string_view kInput =
      R"json({"optionalUint32": 4294967295})json";
  constexpr absl::string_view kExpected = R"pb(optional_uint32: 4294967295)pb";
  EXPECT_THAT(Testee("Uint32FieldMaxValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("Uint32FieldMaxValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonIntegerTest, Int64FieldMaxValue) {
  constexpr absl::string_view kInput =
      R"json({"optionalInt64": "9223372036854775807"})json";
  constexpr absl::string_view kExpected =
      R"pb(optional_int64: 9223372036854775807)pb";
  EXPECT_THAT(Testee("Int64FieldMaxValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("Int64FieldMaxValue").ParseJson(message(), kInput).SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonIntegerTest, Int64FieldMinValue) {
  constexpr absl::string_view kInput =
      R"json({"optionalInt64": "-9223372036854775808"})json";
  constexpr absl::string_view kExpected =
      R"pb(optional_int64: -9223372036854775808)pb";
  EXPECT_THAT(Testee("Int64FieldMinValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("Int64FieldMinValue").ParseJson(message(), kInput).SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonIntegerTest, Uint64FieldMaxValue) {
  constexpr absl::string_view kInput =
      R"json({"optionalUint64": "18446744073709551615"})json";
  constexpr absl::string_view kExpected =
      R"pb(optional_uint64: 18446744073709551615)pb";
  EXPECT_THAT(Testee("Uint64FieldMaxValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("Uint64FieldMaxValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

// While not the largest Int64, this is the largest Int64 which can be exactly
// represented within an IEEE-754 64-bit float, which is the expected level of
// interoperability guarantee.  Larger values may work in some implementations,
// but should not be relied upon.
TEST_P(JsonIntegerTest, Int64FieldMaxValueNotQuoted) {
  constexpr absl::string_view kInput =
      R"json({"optionalInt64": 9223372036854774784})json";
  constexpr absl::string_view kExpected =
      R"pb(optional_int64: 9223372036854774784)pb";
  EXPECT_THAT(Testee("Int64FieldMaxValueNotQuoted")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("Int64FieldMaxValueNotQuoted")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonIntegerTest, Int64FieldMinValueNotQuoted) {
  constexpr absl::string_view kInput =
      R"json({"optionalInt64": -9223372036854775808})json";
  constexpr absl::string_view kExpected =
      R"pb(optional_int64: -9223372036854775808)pb";
  EXPECT_THAT(Testee("Int64FieldMinValueNotQuoted")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("Int64FieldMinValueNotQuoted")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

// Largest interoperable Uint64; see the comment above for
// Int64FieldMaxValueNotQuoted.
TEST_P(JsonIntegerTest, Uint64FieldMaxValueNotQuoted) {
  constexpr absl::string_view kInput =
      R"json({"optionalUint64": 18446744073709549568})json";
  constexpr absl::string_view kExpected =
      R"pb(optional_uint64: 18446744073709549568)pb";
  EXPECT_THAT(Testee("Uint64FieldMaxValueNotQuoted")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("Uint64FieldMaxValueNotQuoted")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

// Values can be represented as JSON strings.
TEST_P(JsonIntegerTest, Int32FieldStringValue) {
  constexpr absl::string_view kInput =
      R"json({"optionalInt32": "2147483647"})json";
  constexpr absl::string_view kExpected = R"pb(optional_int32: 2147483647)pb";
  EXPECT_THAT(Testee("Int32FieldStringValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("Int32FieldStringValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonIntegerTest, Int32FieldStringValueEscaped) {
  constexpr absl::string_view kInput =
      R"json({"optionalInt32": "2\u003147483647"})json";
  constexpr absl::string_view kExpected = R"pb(optional_int32: 2147483647)pb";
  EXPECT_THAT(Testee("Int32FieldStringValueEscaped")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("Int32FieldStringValueEscaped")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonIntegerTest, Int32FieldStringValueZero) {
  constexpr absl::string_view kInput = R"json({"optionalInt32": "0"})json";
  constexpr absl::string_view kExpected = R"pb(optional_int32: 0)pb";
  EXPECT_THAT(Testee("Int32FieldStringValueZero")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("Int32FieldStringValueZero")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonIntegerTest, Int32FieldQuotedExponentialValue) {
  constexpr absl::string_view kInput = R"json({"optionalInt32": "1e5"})json";
  constexpr absl::string_view kExpected = R"pb(optional_int32: 100000)pb";
  EXPECT_THAT(Testee("Int32FieldQuotedExponentialValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("Int32FieldQuotedExponentialValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

// ---------------------------------------------------------------------------
// Parsers reject out-of-bound integer values.
// ---------------------------------------------------------------------------

TEST_P(JsonIntegerTest, Int32FieldTooLarge) {
  EXPECT_THAT(
      Testee("Int32FieldTooLarge")
          .ParseJson(message(), R"json({"optionalInt32": 2147483648})json")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Int32FieldTooSmall) {
  EXPECT_THAT(
      Testee("Int32FieldTooSmall")
          .ParseJson(message(), R"json({"optionalInt32": -2147483649})json")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Uint32FieldTooLarge) {
  EXPECT_THAT(
      Testee("Uint32FieldTooLarge")
          .ParseJson(message(), R"json({"optionalUint32": 4294967296})json")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Int64FieldTooLarge) {
  EXPECT_THAT(
      Testee("Int64FieldTooLarge")
          .ParseJson(message(),
                     R"json({"optionalInt64": "9223372036854775808"})json")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Int64FieldTooSmall) {
  EXPECT_THAT(
      Testee("Int64FieldTooSmall")
          .ParseJson(message(),
                     R"json({"optionalInt64": "-9223372036854775809"})json")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Uint64FieldTooLarge) {
  EXPECT_THAT(
      Testee("Uint64FieldTooLarge")
          .ParseJson(message(),
                     R"json({"optionalUint64": "18446744073709551616"})json")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Uint64QuotedExponentFieldTooLarge) {
  EXPECT_THAT(
      Testee("Uint64QuotedExponentFieldTooLarge")
          .ParseJson(message(), R"json({"optionalUint64": "1e536870000"})json")
          .ParseOnly(),
      Yields(IsParseError()));
}

// ---------------------------------------------------------------------------
// Parsers reject non-integer numeric values.
// ---------------------------------------------------------------------------

TEST_P(JsonIntegerTest, Int32FieldNotInteger) {
  EXPECT_THAT(Testee("Int32FieldNotInteger")
                  .ParseJson(message(), R"json({"optionalInt32": 0.5})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Uint32FieldNotInteger) {
  EXPECT_THAT(Testee("Uint32FieldNotInteger")
                  .ParseJson(message(), R"json({"optionalUint32": 0.5})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Int64FieldNotInteger) {
  EXPECT_THAT(Testee("Int64FieldNotInteger")
                  .ParseJson(message(), R"json({"optionalInt64": "0.5"})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Uint64FieldNotInteger) {
  EXPECT_THAT(Testee("Uint64FieldNotInteger")
                  .ParseJson(message(), R"json({"optionalUint64": "0.5"})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// ---------------------------------------------------------------------------
// Parsers reject boolean values for integer fields.
// ---------------------------------------------------------------------------

TEST_P(JsonIntegerTest, Int32FieldTrueValue) {
  EXPECT_THAT(Testee("Int32FieldTrueValue")
                  .ParseJson(message(), R"json({"optionalInt32": true})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Int32FieldFalseValue) {
  EXPECT_THAT(Testee("Int32FieldFalseValue")
                  .ParseJson(message(), R"json({"optionalInt32": false})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// ---------------------------------------------------------------------------
// Parsers reject non-numeric string values.
// ---------------------------------------------------------------------------

TEST_P(JsonIntegerTest, Int32FieldStringValuePartiallyNumeric) {
  EXPECT_THAT(Testee("Int32FieldStringValuePartiallyNumeric")
                  .ParseJson(message(), R"json({"optionalInt32": "12abc"})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Int32FieldStringValuePartiallyNumericSpace) {
  EXPECT_THAT(Testee("Int32FieldStringValuePartiallyNumericSpace")
                  .ParseJson(message(), R"json({"optionalInt32": "12 34"})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Int32FieldStringValuePartiallyNumericComma) {
  EXPECT_THAT(Testee("Int32FieldStringValuePartiallyNumericComma")
                  .ParseJson(message(), R"json({"optionalInt32": "12,34"})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Int32FieldStringValuePartiallyNumericUnicode) {
  // Not a raw string literal: the payload holds the UTF-8 bytes of U+8C37
  // U+6B4C between the digits.
  EXPECT_THAT(Testee("Int32FieldStringValuePartiallyNumericUnicode")
                  .ParseJson(message(),
                             "{\"optionalInt32\": \"12\xE8\xB0\xB7\xE6\xAD\x8C"
                             "34\"}")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Int32FieldStringValueNonNumeric) {
  EXPECT_THAT(Testee("Int32FieldStringValueNonNumeric")
                  .ParseJson(message(), R"json({"optionalInt32": "abc"})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// Only ASCII digits are accepted in quoted integers.  Other Unicode
// decimal digits, such as Arabic-Indic or fullwidth digits, are rejected.
// Not raw string literals: the payloads hold the UTF-8 bytes of U+0661
// U+0662 U+0663 and of U+FF11 U+FF12 U+FF13.
TEST_P(JsonIntegerTest, Int32FieldStringValueArabicIndicDigits) {
  constexpr absl::string_view kInput =
      "{\"optionalInt32\": \"\xD9\xA1\xD9\xA2\xD9\xA3\"}";
  EXPECT_THAT(Testee("Int32FieldStringValueArabicIndicDigits")
                  .ParseJson(message(), kInput)
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Int64FieldStringValueFullwidthDigits) {
  constexpr absl::string_view kInput =
      "{\"optionalInt64\": \"\xEF\xBC\x91\xEF\xBC\x92\xEF\xBC\x93\"}";
  EXPECT_THAT(Testee("Int64FieldStringValueFullwidthDigits")
                  .ParseJson(message(), kInput)
                  .ParseOnly(),
              Yields(IsParseError()));
}

// ---------------------------------------------------------------------------
// Parsers reject empty string values.
// ---------------------------------------------------------------------------

TEST_P(JsonIntegerTest, Int32FieldEmptyString) {
  EXPECT_THAT(Testee("Int32FieldEmptyString")
                  .ParseJson(message(), R"json({"optionalInt32": ""})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Uint32FieldEmptyString) {
  EXPECT_THAT(Testee("Uint32FieldEmptyString")
                  .ParseJson(message(), R"json({"optionalUint32": ""})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Int64FieldEmptyString) {
  EXPECT_THAT(Testee("Int64FieldEmptyString")
                  .ParseJson(message(), R"json({"optionalInt64": ""})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Uint64FieldEmptyString) {
  EXPECT_THAT(Testee("Uint64FieldEmptyString")
                  .ParseJson(message(), R"json({"optionalUint64": ""})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// ---------------------------------------------------------------------------
// Integers represented as float values are accepted.
// ---------------------------------------------------------------------------

TEST_P(JsonIntegerTest, Int32FieldFloatTrailingZero) {
  constexpr absl::string_view kInput =
      R"json({"optionalInt32": 100000.000})json";
  constexpr absl::string_view kExpected = R"pb(optional_int32: 100000)pb";
  EXPECT_THAT(Testee("Int32FieldFloatTrailingZero")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("Int32FieldFloatTrailingZero")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonIntegerTest, Int32FieldExponentialFormat) {
  constexpr absl::string_view kInput = R"json({"optionalInt32": 1e5})json";
  constexpr absl::string_view kExpected = R"pb(optional_int32: 100000)pb";
  EXPECT_THAT(Testee("Int32FieldExponentialFormat")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("Int32FieldExponentialFormat")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonIntegerTest, Int32FieldMaxFloatValue) {
  constexpr absl::string_view kInput =
      R"json({"optionalInt32": 2.147483647e9})json";
  constexpr absl::string_view kExpected = R"pb(optional_int32: 2147483647)pb";
  EXPECT_THAT(Testee("Int32FieldMaxFloatValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("Int32FieldMaxFloatValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonIntegerTest, Int32FieldMinFloatValue) {
  constexpr absl::string_view kInput =
      R"json({"optionalInt32": -2.147483648e9})json";
  constexpr absl::string_view kExpected = R"pb(optional_int32: -2147483648)pb";
  EXPECT_THAT(Testee("Int32FieldMinFloatValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("Int32FieldMinFloatValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonIntegerTest, Uint32FieldMaxFloatValue) {
  constexpr absl::string_view kInput =
      R"json({"optionalUint32": 4.294967295e9})json";
  constexpr absl::string_view kExpected = R"pb(optional_uint32: 4294967295)pb";
  EXPECT_THAT(Testee("Uint32FieldMaxFloatValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("Uint32FieldMaxFloatValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

// ---------------------------------------------------------------------------
// Parsers reject non-numeric and malformed numeric values.
// ---------------------------------------------------------------------------

TEST_P(JsonIntegerTest, Int32FieldNotNumber) {
  EXPECT_THAT(Testee("Int32FieldNotNumber")
                  .ParseJson(message(), R"json({"optionalInt32": "3x3"})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Uint32FieldNotNumber) {
  EXPECT_THAT(Testee("Uint32FieldNotNumber")
                  .ParseJson(message(), R"json({"optionalUint32": "3x3"})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Int64FieldNotNumber) {
  EXPECT_THAT(Testee("Int64FieldNotNumber")
                  .ParseJson(message(), R"json({"optionalInt64": "3x3"})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Uint64FieldNotNumber) {
  EXPECT_THAT(Testee("Uint64FieldNotNumber")
                  .ParseJson(message(), R"json({"optionalUint64": "3x3"})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// JSON does not allow "+" on numeric values.
TEST_P(JsonIntegerTest, Int32FieldPlusSign) {
  EXPECT_THAT(Testee("Int32FieldPlusSign")
                  .ParseJson(message(), R"json({"optionalInt32": +1})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// JSON doesn't allow leading 0s.
TEST_P(JsonIntegerTest, Int32FieldLeadingZero) {
  EXPECT_THAT(Testee("Int32FieldLeadingZero")
                  .ParseJson(message(), R"json({"optionalInt32": 01})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Int32FieldNegativeWithLeadingZero) {
  EXPECT_THAT(Testee("Int32FieldNegativeWithLeadingZero")
                  .ParseJson(message(), R"json({"optionalInt32": -01})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// String values must follow the same syntax rule.  Specifically leading or
// trailing spaces are not allowed.
TEST_P(JsonIntegerTest, Int32FieldLeadingSpace) {
  EXPECT_THAT(Testee("Int32FieldLeadingSpace")
                  .ParseJson(message(), R"json({"optionalInt32": " 1"})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonIntegerTest, Int32FieldTrailingSpace) {
  EXPECT_THAT(Testee("Int32FieldTrailingSpace")
                  .ParseJson(message(), R"json({"optionalInt32": "1 "})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// ---------------------------------------------------------------------------
// 64-bit values are serialized as strings.
// ---------------------------------------------------------------------------

TEST_P(JsonIntegerTest, Int64FieldBeString) {
  EXPECT_THAT(Testee(kP1, "Int64FieldBeString")
                  .ParseJson(message(), R"json({"optionalInt64": 1})json")
                  .SerializeJson(),
              Yields(JsonPayload(
                  HasJsonMemberThat("optionalInt64", IsJsonString("1")))));
}

TEST_P(JsonIntegerTest, Uint64FieldBeString) {
  EXPECT_THAT(Testee(kP1, "Uint64FieldBeString")
                  .ParseJson(message(), R"json({"optionalUint64": 1})json")
                  .SerializeJson(),
              Yields(JsonPayload(
                  HasJsonMemberThat("optionalUint64", IsJsonString("1")))));
}

INSTANTIATE_TEST_SUITE_P(All, JsonIntegerTest, ValuesIn(AllTestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
