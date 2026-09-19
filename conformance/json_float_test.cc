// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// JSON conformance tests for float and double fields: the value ranges,
// quoted values, the special values NaN / Infinity / -Infinity (which must be
// quoted), the normalization of non-canonical NaN bit patterns when
// serializing to JSON, and rejected out-of-range, empty, non-numeric and
// boolean values.  This replaces the "Float fields" and "Double fields" blocks
// of the legacy BinaryAndJsonConformanceSuiteImpl<M>::
// RunJsonTestsForNonRepeatedTypes(); the test names and the requests sent to
// the testee are identical to the legacy ones, except that the NaN
// normalization tests are BINARY_TEST requests (see below).
//
// Like the legacy RunValidJsonTest(), each valid input is sent twice: once to
// be serialized as binary ("<name>.ProtobufOutput") and once as JSON
// ("<name>.JsonOutput").  ExpectParseFailureForJson() asked for JSON output
// under a name without an output suffix.  The NaN normalization tests
// (RunValidJsonTestWithProtobufInput()) send a *binary* payload and ask for
// JSON output ("<S>.<E>.ProtobufInput.<name>.JsonOutput"); the legacy suite
// filed them under the JSON_TEST category, here ParseBinary() categorizes them
// as BINARY_TEST, which no testee tells apart from JSON_TEST on a binary
// payload (b/563657722).

#include <cstdint>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "conformance/binary_test_util.h"
#include "conformance/binary_wireformat.h"
#include "conformance/matchers.h"
#include "conformance/message_type_fixtures.h"
#include "conformance/testee.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::ValuesIn;

// The field numbers of optional_float and optional_double, the same in every
// test message type.
constexpr uint32_t kOptionalFloatFieldNumber = 11;
constexpr uint32_t kOptionalDoubleFieldNumber = 12;

// ---------------------------------------------------------------------------
// Float fields.
// ---------------------------------------------------------------------------

using JsonFloatTest = MessageTypeConformanceTest;

TEST_P(JsonFloatTest, FloatFieldMinPositiveValue) {
  constexpr absl::string_view kInput =
      R"json({"optionalFloat": 1.175494e-38})json";
  constexpr absl::string_view kExpected = R"pb(optional_float: 1.175494e-38)pb";
  EXPECT_THAT(Testee("FloatFieldMinPositiveValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("FloatFieldMinPositiveValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonFloatTest, FloatFieldMaxNegativeValue) {
  constexpr absl::string_view kInput =
      R"json({"optionalFloat": -1.175494e-38})json";
  constexpr absl::string_view kExpected =
      R"pb(optional_float: -1.175494e-38)pb";
  EXPECT_THAT(Testee("FloatFieldMaxNegativeValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("FloatFieldMaxNegativeValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonFloatTest, FloatFieldMaxPositiveValue) {
  constexpr absl::string_view kInput =
      R"json({"optionalFloat": 3.402823e+38})json";
  constexpr absl::string_view kExpected = R"pb(optional_float: 3.402823e+38)pb";
  EXPECT_THAT(Testee("FloatFieldMaxPositiveValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("FloatFieldMaxPositiveValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

// Sends the same (positive) value as FloatFieldMaxPositiveValue, like the
// legacy suite did.
TEST_P(JsonFloatTest, FloatFieldMinNegativeValue) {
  constexpr absl::string_view kInput =
      R"json({"optionalFloat": 3.402823e+38})json";
  constexpr absl::string_view kExpected = R"pb(optional_float: 3.402823e+38)pb";
  EXPECT_THAT(Testee("FloatFieldMinNegativeValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("FloatFieldMinNegativeValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

// Values can be quoted.
TEST_P(JsonFloatTest, FloatFieldQuotedValue) {
  constexpr absl::string_view kInput = R"json({"optionalFloat": "1"})json";
  constexpr absl::string_view kExpected = R"pb(optional_float: 1)pb";
  EXPECT_THAT(Testee("FloatFieldQuotedValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("FloatFieldQuotedValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonFloatTest, FloatFieldQuotedExponentialValue) {
  constexpr absl::string_view kInput =
      R"json({"optionalFloat": "1.175494e-38"})json";
  constexpr absl::string_view kExpected = R"pb(optional_float: 1.175494e-38)pb";
  EXPECT_THAT(Testee("FloatFieldQuotedExponentialValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("FloatFieldQuotedExponentialValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

// Special values.
TEST_P(JsonFloatTest, FloatFieldNan) {
  constexpr absl::string_view kInput = R"json({"optionalFloat": "NaN"})json";
  constexpr absl::string_view kExpected = R"pb(optional_float: nan)pb";
  EXPECT_THAT(
      Testee("FloatFieldNan").ParseJson(message(), kInput).SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("FloatFieldNan").ParseJson(message(), kInput).SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonFloatTest, FloatFieldInfinity) {
  constexpr absl::string_view kInput =
      R"json({"optionalFloat": "Infinity"})json";
  constexpr absl::string_view kExpected = R"pb(optional_float: inf)pb";
  EXPECT_THAT(Testee("FloatFieldInfinity")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("FloatFieldInfinity").ParseJson(message(), kInput).SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonFloatTest, FloatFieldNegativeInfinity) {
  constexpr absl::string_view kInput =
      R"json({"optionalFloat": "-Infinity"})json";
  constexpr absl::string_view kExpected = R"pb(optional_float: -inf)pb";
  EXPECT_THAT(Testee("FloatFieldNegativeInfinity")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("FloatFieldNegativeInfinity")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

// Non-canonical NaNs are normalized: the binary payload carries a NaN with
// a non-canonical payload (0x7FA12345; the legacy names notwithstanding,
// bit 22 is clear in both patterns, so both are signaling NaNs), ...
TEST_P(JsonFloatTest, FloatFieldNormalizeQuietNan) {
  EXPECT_THAT(
      Testee("FloatFieldNormalizeQuietNan")
          .ParseBinary(message(),
                       Fixed32Field(kOptionalFloatFieldNumber, 0x7FA12345))
          .SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(R"pb(optional_float: nan)pb"))));
}

// ... or a NaN with the sign bit set and another payload (0xFFB54321).
TEST_P(JsonFloatTest, FloatFieldNormalizeSignalingNan) {
  EXPECT_THAT(
      Testee("FloatFieldNormalizeSignalingNan")
          .ParseBinary(message(),
                       Fixed32Field(kOptionalFloatFieldNumber, 0xFFB54321))
          .SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(R"pb(optional_float: nan)pb"))));
}

// Special values must be quoted.
TEST_P(JsonFloatTest, FloatFieldNanNotQuoted) {
  EXPECT_THAT(Testee(kP1, "FloatFieldNanNotQuoted")
                  .ParseJson(message(), R"json({"optionalFloat": NaN})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonFloatTest, FloatFieldInfinityNotQuoted) {
  EXPECT_THAT(
      Testee(kP1, "FloatFieldInfinityNotQuoted")
          .ParseJson(message(), R"json({"optionalFloat": Infinity})json")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonFloatTest, FloatFieldNegativeInfinityNotQuoted) {
  EXPECT_THAT(
      Testee(kP1, "FloatFieldNegativeInfinityNotQuoted")
          .ParseJson(message(), R"json({"optionalFloat": -Infinity})json")
          .ParseOnly(),
      Yields(IsParseError()));
}

// Parsers should reject out-of-bound values.
TEST_P(JsonFloatTest, FloatFieldTooSmall) {
  EXPECT_THAT(
      Testee("FloatFieldTooSmall")
          .ParseJson(message(), R"json({"optionalFloat": -3.502823e+38})json")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonFloatTest, FloatFieldTooLarge) {
  EXPECT_THAT(
      Testee("FloatFieldTooLarge")
          .ParseJson(message(), R"json({"optionalFloat": 3.502823e+38})json")
          .ParseOnly(),
      Yields(IsParseError()));
}

// Parsers should reject empty string values.
TEST_P(JsonFloatTest, FloatFieldEmptyString) {
  EXPECT_THAT(Testee("FloatFieldEmptyString")
                  .ParseJson(message(), R"json({"optionalFloat": ""})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// Parsers reject non-numeric string values.
TEST_P(JsonFloatTest, FloatFieldStringValuePartiallyNumeric) {
  EXPECT_THAT(Testee("FloatFieldStringValuePartiallyNumeric")
                  .ParseJson(message(), R"json({"optionalFloat": "12abc"})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonFloatTest, FloatFieldStringValueNonNumeric) {
  EXPECT_THAT(Testee("FloatFieldStringValueNonNumeric")
                  .ParseJson(message(), R"json({"optionalFloat": "abc"})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonFloatTest, FloatFieldStringValuePartiallyNumericSpace) {
  EXPECT_THAT(Testee("FloatFieldStringValuePartiallyNumericSpace")
                  .ParseJson(message(), R"json({"optionalFloat": "12 34"})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonFloatTest, FloatFieldStringValuePartiallyNumericComma) {
  EXPECT_THAT(Testee("FloatFieldStringValuePartiallyNumericComma")
                  .ParseJson(message(), R"json({"optionalFloat": "12,34"})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonFloatTest, FloatFieldStringValuePartiallyNumericUnicode) {
  // Not a raw string literal: the payload holds the UTF-8 bytes of U+8C37
  // U+6B4C between the digits.
  EXPECT_THAT(Testee("FloatFieldStringValuePartiallyNumericUnicode")
                  .ParseJson(message(),
                             "{\"optionalFloat\": \"12\xE8\xB0\xB7\xE6\xAD\x8C"
                             "34\"}")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// Only ASCII digits are accepted in quoted numbers.  Not raw string
// literals: the payloads hold the UTF-8 bytes of the Arabic-Indic digits
// U+0661 U+0662 U+0663 and of the fullwidth digits U+FF11 U+FF12 U+FF13.
TEST_P(JsonFloatTest, FloatFieldStringValueArabicIndicDigits) {
  constexpr absl::string_view kInput =
      "{\"optionalFloat\": \"\xD9\xA1\xD9\xA2\xD9\xA3\"}";
  EXPECT_THAT(Testee("FloatFieldStringValueArabicIndicDigits")
                  .ParseJson(message(), kInput)
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonFloatTest, FloatFieldStringValueFullwidthDigits) {
  constexpr absl::string_view kInput =
      "{\"optionalFloat\": \"\xEF\xBC\x91\xEF\xBC\x92\xEF\xBC\x93\"}";
  EXPECT_THAT(Testee("FloatFieldStringValueFullwidthDigits")
                  .ParseJson(message(), kInput)
                  .ParseOnly(),
              Yields(IsParseError()));
}

// Parsers reject boolean values for float fields.
TEST_P(JsonFloatTest, FloatFieldTrueValue) {
  EXPECT_THAT(Testee("FloatFieldTrueValue")
                  .ParseJson(message(), R"json({"optionalFloat": true})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonFloatTest, FloatFieldFalseValue) {
  EXPECT_THAT(Testee("FloatFieldFalseValue")
                  .ParseJson(message(), R"json({"optionalFloat": false})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

INSTANTIATE_TEST_SUITE_P(All, JsonFloatTest, ValuesIn(AllTestMessageTypes()),
                         MessageTypeParamName);

// ---------------------------------------------------------------------------
// Double fields.
// ---------------------------------------------------------------------------

using JsonDoubleTest = MessageTypeConformanceTest;

TEST_P(JsonDoubleTest, DoubleFieldMinPositiveValue) {
  constexpr absl::string_view kInput =
      R"json({"optionalDouble": 2.22507e-308})json";
  constexpr absl::string_view kExpected =
      R"pb(optional_double: 2.22507e-308)pb";
  EXPECT_THAT(Testee("DoubleFieldMinPositiveValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("DoubleFieldMinPositiveValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonDoubleTest, DoubleFieldMaxNegativeValue) {
  constexpr absl::string_view kInput =
      R"json({"optionalDouble": -2.22507e-308})json";
  constexpr absl::string_view kExpected =
      R"pb(optional_double: -2.22507e-308)pb";
  EXPECT_THAT(Testee("DoubleFieldMaxNegativeValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("DoubleFieldMaxNegativeValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonDoubleTest, DoubleFieldMaxPositiveValue) {
  constexpr absl::string_view kInput =
      R"json({"optionalDouble": 1.79769e+308})json";
  constexpr absl::string_view kExpected =
      R"pb(optional_double: 1.79769e+308)pb";
  EXPECT_THAT(Testee("DoubleFieldMaxPositiveValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("DoubleFieldMaxPositiveValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonDoubleTest, DoubleFieldMinNegativeValue) {
  constexpr absl::string_view kInput =
      R"json({"optionalDouble": -1.79769e+308})json";
  constexpr absl::string_view kExpected =
      R"pb(optional_double: -1.79769e+308)pb";
  EXPECT_THAT(Testee("DoubleFieldMinNegativeValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("DoubleFieldMinNegativeValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

// Values can be quoted.
TEST_P(JsonDoubleTest, DoubleFieldQuotedValue) {
  constexpr absl::string_view kInput = R"json({"optionalDouble": "1"})json";
  constexpr absl::string_view kExpected = R"pb(optional_double: 1)pb";
  EXPECT_THAT(Testee("DoubleFieldQuotedValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("DoubleFieldQuotedValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonDoubleTest, DoubleFieldQuotedExponentialValue) {
  constexpr absl::string_view kInput =
      R"json({"optionalDouble": "2.22507e-308"})json";
  constexpr absl::string_view kExpected =
      R"pb(optional_double: 2.22507e-308)pb";
  EXPECT_THAT(Testee("DoubleFieldQuotedExponentialValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("DoubleFieldQuotedExponentialValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

// Special values.
TEST_P(JsonDoubleTest, DoubleFieldNan) {
  constexpr absl::string_view kInput = R"json({"optionalDouble": "NaN"})json";
  constexpr absl::string_view kExpected = R"pb(optional_double: nan)pb";
  EXPECT_THAT(
      Testee("DoubleFieldNan").ParseJson(message(), kInput).SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("DoubleFieldNan").ParseJson(message(), kInput).SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonDoubleTest, DoubleFieldInfinity) {
  constexpr absl::string_view kInput =
      R"json({"optionalDouble": "Infinity"})json";
  constexpr absl::string_view kExpected = R"pb(optional_double: inf)pb";
  EXPECT_THAT(Testee("DoubleFieldInfinity")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("DoubleFieldInfinity")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonDoubleTest, DoubleFieldNegativeInfinity) {
  constexpr absl::string_view kInput =
      R"json({"optionalDouble": "-Infinity"})json";
  constexpr absl::string_view kExpected = R"pb(optional_double: -inf)pb";
  EXPECT_THAT(Testee("DoubleFieldNegativeInfinity")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("DoubleFieldNegativeInfinity")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

// Non-canonical NaNs are normalized, as for floats above.
TEST_P(JsonDoubleTest, DoubleFieldNormalizeQuietNan) {
  EXPECT_THAT(
      Testee("DoubleFieldNormalizeQuietNan")
          .ParseBinary(message(), Fixed64Field(kOptionalDoubleFieldNumber,
                                               0x7FFA123456789ABC))
          .SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(R"pb(optional_double: nan)pb"))));
}

TEST_P(JsonDoubleTest, DoubleFieldNormalizeSignalingNan) {
  EXPECT_THAT(
      Testee("DoubleFieldNormalizeSignalingNan")
          .ParseBinary(message(), Fixed64Field(kOptionalDoubleFieldNumber,
                                               0xFFFBCBA987654321))
          .SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(R"pb(optional_double: nan)pb"))));
}

// Special values must be quoted.
TEST_P(JsonDoubleTest, DoubleFieldNanNotQuoted) {
  EXPECT_THAT(Testee(kP1, "DoubleFieldNanNotQuoted")
                  .ParseJson(message(), R"json({"optionalDouble": NaN})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonDoubleTest, DoubleFieldInfinityNotQuoted) {
  EXPECT_THAT(
      Testee(kP1, "DoubleFieldInfinityNotQuoted")
          .ParseJson(message(), R"json({"optionalDouble": Infinity})json")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonDoubleTest, DoubleFieldNegativeInfinityNotQuoted) {
  EXPECT_THAT(
      Testee(kP1, "DoubleFieldNegativeInfinityNotQuoted")
          .ParseJson(message(), R"json({"optionalDouble": -Infinity})json")
          .ParseOnly(),
      Yields(IsParseError()));
}

// Parsers should reject out-of-bound values.
TEST_P(JsonDoubleTest, DoubleFieldTooSmall) {
  EXPECT_THAT(
      Testee("DoubleFieldTooSmall")
          .ParseJson(message(), R"json({"optionalDouble": -1.89769e+308})json")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonDoubleTest, DoubleFieldTooLarge) {
  EXPECT_THAT(
      Testee("DoubleFieldTooLarge")
          .ParseJson(message(), R"json({"optionalDouble": 1.89769e+308})json")
          .ParseOnly(),
      Yields(IsParseError()));
}

// Parsers should reject empty string values.
TEST_P(JsonDoubleTest, DoubleFieldEmptyString) {
  EXPECT_THAT(Testee("DoubleFieldEmptyString")
                  .ParseJson(message(), R"json({"optionalDouble": ""})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// Parsers reject non-numeric string values.
TEST_P(JsonDoubleTest, DoubleFieldStringValuePartiallyNumeric) {
  EXPECT_THAT(
      Testee("DoubleFieldStringValuePartiallyNumeric")
          .ParseJson(message(), R"json({"optionalDouble": "12abc"})json")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonDoubleTest, DoubleFieldStringValueNonNumeric) {
  EXPECT_THAT(Testee("DoubleFieldStringValueNonNumeric")
                  .ParseJson(message(), R"json({"optionalDouble": "abc"})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// Only ASCII digits are accepted in quoted numbers.  Not raw string
// literals: the payloads hold the UTF-8 bytes of the Arabic-Indic digits
// U+0661 U+0662 U+0663 and of the fullwidth digits U+FF11 U+FF12 U+FF13.
TEST_P(JsonDoubleTest, DoubleFieldStringValueArabicIndicDigits) {
  constexpr absl::string_view kInput =
      "{\"optionalDouble\": \"\xD9\xA1\xD9\xA2\xD9\xA3\"}";
  EXPECT_THAT(Testee("DoubleFieldStringValueArabicIndicDigits")
                  .ParseJson(message(), kInput)
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonDoubleTest, DoubleFieldStringValueFullwidthDigits) {
  constexpr absl::string_view kInput =
      "{\"optionalDouble\": \"\xEF\xBC\x91\xEF\xBC\x92\xEF\xBC\x93\"}";
  EXPECT_THAT(Testee("DoubleFieldStringValueFullwidthDigits")
                  .ParseJson(message(), kInput)
                  .ParseOnly(),
              Yields(IsParseError()));
}

// Parsers reject boolean values for double fields.
TEST_P(JsonDoubleTest, DoubleFieldTrueValue) {
  EXPECT_THAT(Testee("DoubleFieldTrueValue")
                  .ParseJson(message(), R"json({"optionalDouble": true})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonDoubleTest, DoubleFieldFalseValue) {
  EXPECT_THAT(Testee("DoubleFieldFalseValue")
                  .ParseJson(message(), R"json({"optionalDouble": false})json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

INSTANTIATE_TEST_SUITE_P(All, JsonDoubleTest, ValuesIn(AllTestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
