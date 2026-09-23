// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
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
// the testee are identical to the legacy ones.
//
// Like the legacy RunValidJsonTest(), each valid input is sent twice: once to
// be serialized as binary ("<name>.ProtobufOutput") and once as JSON
// ("<name>.JsonOutput").  ExpectParseFailureForJson() asked for JSON output
// under a name without an output suffix.  The NaN normalization tests
// (RunValidJsonTestWithProtobufInput()) send a *binary* payload and ask for
// JSON output ("<S>.ProtobufInput.<name>.JsonOutput"), but the legacy suite
// filed them under the JSON_TEST category anyway, hence OverrideTestCategory().

#include <cstdint>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "conformance/binary_test_util.h"
#include "conformance/binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "conformance/matchers.h"
#include "conformance/message_type_fixtures.h"
#include "conformance/test_environment.h"
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
  constexpr absl::string_view kInput = R"({"optionalFloat": 1.175494e-38})";
  constexpr absl::string_view kExpected = "optional_float: 1.175494e-38";
  EXPECT_THAT(Testee("FloatFieldMinPositiveValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("FloatFieldMinPositiveValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonFloatTest, FloatFieldMaxNegativeValue) {
  constexpr absl::string_view kInput = R"({"optionalFloat": -1.175494e-38})";
  constexpr absl::string_view kExpected = "optional_float: -1.175494e-38";
  EXPECT_THAT(Testee("FloatFieldMaxNegativeValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("FloatFieldMaxNegativeValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonFloatTest, FloatFieldMaxPositiveValue) {
  constexpr absl::string_view kInput = R"({"optionalFloat": 3.402823e+38})";
  constexpr absl::string_view kExpected = "optional_float: 3.402823e+38";
  EXPECT_THAT(Testee("FloatFieldMaxPositiveValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("FloatFieldMaxPositiveValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// Sends the same (positive) value as FloatFieldMaxPositiveValue, like the
// legacy suite did.
TEST_P(JsonFloatTest, FloatFieldMinNegativeValue) {
  constexpr absl::string_view kInput = R"({"optionalFloat": 3.402823e+38})";
  constexpr absl::string_view kExpected = "optional_float: 3.402823e+38";
  EXPECT_THAT(Testee("FloatFieldMinNegativeValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("FloatFieldMinNegativeValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// Values can be quoted.
TEST_P(JsonFloatTest, FloatFieldQuotedValue) {
  constexpr absl::string_view kInput = R"({"optionalFloat": "1"})";
  constexpr absl::string_view kExpected = "optional_float: 1";
  EXPECT_THAT(Testee("FloatFieldQuotedValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("FloatFieldQuotedValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonFloatTest, FloatFieldQuotedExponentialValue) {
  constexpr absl::string_view kInput = R"({"optionalFloat": "1.175494e-38"})";
  constexpr absl::string_view kExpected = "optional_float: 1.175494e-38";
  EXPECT_THAT(Testee("FloatFieldQuotedExponentialValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("FloatFieldQuotedExponentialValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// Special values.
TEST_P(JsonFloatTest, FloatFieldNan) {
  constexpr absl::string_view kInput = R"({"optionalFloat": "NaN"})";
  constexpr absl::string_view kExpected = "optional_float: nan";
  EXPECT_THAT(
      Testee("FloatFieldNan").ParseJson(message(), kInput).SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("FloatFieldNan").ParseJson(message(), kInput).SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonFloatTest, FloatFieldInfinity) {
  constexpr absl::string_view kInput = R"({"optionalFloat": "Infinity"})";
  constexpr absl::string_view kExpected = "optional_float: inf";
  EXPECT_THAT(Testee("FloatFieldInfinity")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("FloatFieldInfinity").ParseJson(message(), kInput).SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonFloatTest, FloatFieldNegativeInfinity) {
  constexpr absl::string_view kInput = R"({"optionalFloat": "-Infinity"})";
  constexpr absl::string_view kExpected = "optional_float: -inf";
  EXPECT_THAT(Testee("FloatFieldNegativeInfinity")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("FloatFieldNegativeInfinity")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// Non-canonical NaNs are normalized: the binary payload carries a NaN with
// a non-canonical payload (0x7FA12345; the legacy names notwithstanding,
// bit 22 is clear in both patterns, so both are signaling NaNs), ...
TEST_P(JsonFloatTest, FloatFieldNormalizeQuietNan) {
  EXPECT_THAT(
      Testee("FloatFieldNormalizeQuietNan")
          .ParseBinary(message(),
                       Fixed32Field(kOptionalFloatFieldNumber, 0x7FA12345))
          .OverrideTestCategory(::conformance::JSON_TEST)
          .SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto("optional_float: nan"))));
}

// ... or a NaN with the sign bit set and another payload (0xFFB54321).
TEST_P(JsonFloatTest, FloatFieldNormalizeSignalingNan) {
  EXPECT_THAT(
      Testee("FloatFieldNormalizeSignalingNan")
          .ParseBinary(message(),
                       Fixed32Field(kOptionalFloatFieldNumber, 0xFFB54321))
          .OverrideTestCategory(::conformance::JSON_TEST)
          .SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto("optional_float: nan"))));
}

// Special values must be quoted.
TEST_P(JsonFloatTest, FloatFieldNanNotQuoted) {
  EXPECT_THAT(Testee(kP3, "FloatFieldNanNotQuoted")
                  .ParseJson(message(), R"({"optionalFloat": NaN})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonFloatTest, FloatFieldInfinityNotQuoted) {
  EXPECT_THAT(Testee(kP3, "FloatFieldInfinityNotQuoted")
                  .ParseJson(message(), R"({"optionalFloat": Infinity})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonFloatTest, FloatFieldNegativeInfinityNotQuoted) {
  EXPECT_THAT(Testee(kP3, "FloatFieldNegativeInfinityNotQuoted")
                  .ParseJson(message(), R"({"optionalFloat": -Infinity})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// Parsers should reject out-of-bound values.
TEST_P(JsonFloatTest, FloatFieldTooSmall) {
  EXPECT_THAT(Testee("FloatFieldTooSmall")
                  .ParseJson(message(), R"({"optionalFloat": -3.502823e+38})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonFloatTest, FloatFieldTooLarge) {
  EXPECT_THAT(Testee("FloatFieldTooLarge")
                  .ParseJson(message(), R"({"optionalFloat": 3.502823e+38})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// Parsers should reject empty string values.
TEST_P(JsonFloatTest, FloatFieldEmptyString) {
  EXPECT_THAT(Testee("FloatFieldEmptyString")
                  .ParseJson(message(), R"({"optionalFloat": ""})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// Parsers reject non-numeric string values.
TEST_P(JsonFloatTest, FloatFieldStringValuePartiallyNumeric) {
  EXPECT_THAT(Testee("FloatFieldStringValuePartiallyNumeric")
                  .ParseJson(message(), R"({"optionalFloat": "12abc"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonFloatTest, FloatFieldStringValueNonNumeric) {
  EXPECT_THAT(Testee("FloatFieldStringValueNonNumeric")
                  .ParseJson(message(), R"({"optionalFloat": "abc"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonFloatTest, FloatFieldStringValuePartiallyNumericSpace) {
  EXPECT_THAT(Testee("FloatFieldStringValuePartiallyNumericSpace")
                  .ParseJson(message(), R"({"optionalFloat": "12 34"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonFloatTest, FloatFieldStringValuePartiallyNumericComma) {
  EXPECT_THAT(Testee("FloatFieldStringValuePartiallyNumericComma")
                  .ParseJson(message(), R"({"optionalFloat": "12,34"})")
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

// Parsers reject boolean values for float fields.
TEST_P(JsonFloatTest, FloatFieldTrueValue) {
  EXPECT_THAT(Testee("FloatFieldTrueValue")
                  .ParseJson(message(), R"({"optionalFloat": true})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonFloatTest, FloatFieldFalseValue) {
  EXPECT_THAT(Testee("FloatFieldFalseValue")
                  .ParseJson(message(), R"({"optionalFloat": false})")
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
  constexpr absl::string_view kInput = R"({"optionalDouble": 2.22507e-308})";
  constexpr absl::string_view kExpected = "optional_double: 2.22507e-308";
  EXPECT_THAT(Testee("DoubleFieldMinPositiveValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("DoubleFieldMinPositiveValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonDoubleTest, DoubleFieldMaxNegativeValue) {
  constexpr absl::string_view kInput = R"({"optionalDouble": -2.22507e-308})";
  constexpr absl::string_view kExpected = "optional_double: -2.22507e-308";
  EXPECT_THAT(Testee("DoubleFieldMaxNegativeValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("DoubleFieldMaxNegativeValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonDoubleTest, DoubleFieldMaxPositiveValue) {
  constexpr absl::string_view kInput = R"({"optionalDouble": 1.79769e+308})";
  constexpr absl::string_view kExpected = "optional_double: 1.79769e+308";
  EXPECT_THAT(Testee("DoubleFieldMaxPositiveValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("DoubleFieldMaxPositiveValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonDoubleTest, DoubleFieldMinNegativeValue) {
  constexpr absl::string_view kInput = R"({"optionalDouble": -1.79769e+308})";
  constexpr absl::string_view kExpected = "optional_double: -1.79769e+308";
  EXPECT_THAT(Testee("DoubleFieldMinNegativeValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("DoubleFieldMinNegativeValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// Values can be quoted.
TEST_P(JsonDoubleTest, DoubleFieldQuotedValue) {
  constexpr absl::string_view kInput = R"({"optionalDouble": "1"})";
  constexpr absl::string_view kExpected = "optional_double: 1";
  EXPECT_THAT(Testee("DoubleFieldQuotedValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("DoubleFieldQuotedValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonDoubleTest, DoubleFieldQuotedExponentialValue) {
  constexpr absl::string_view kInput = R"({"optionalDouble": "2.22507e-308"})";
  constexpr absl::string_view kExpected = "optional_double: 2.22507e-308";
  EXPECT_THAT(Testee("DoubleFieldQuotedExponentialValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("DoubleFieldQuotedExponentialValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// Special values.
TEST_P(JsonDoubleTest, DoubleFieldNan) {
  constexpr absl::string_view kInput = R"({"optionalDouble": "NaN"})";
  constexpr absl::string_view kExpected = "optional_double: nan";
  EXPECT_THAT(
      Testee("DoubleFieldNan").ParseJson(message(), kInput).SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("DoubleFieldNan").ParseJson(message(), kInput).SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonDoubleTest, DoubleFieldInfinity) {
  constexpr absl::string_view kInput = R"({"optionalDouble": "Infinity"})";
  constexpr absl::string_view kExpected = "optional_double: inf";
  EXPECT_THAT(Testee("DoubleFieldInfinity")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("DoubleFieldInfinity")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonDoubleTest, DoubleFieldNegativeInfinity) {
  constexpr absl::string_view kInput = R"({"optionalDouble": "-Infinity"})";
  constexpr absl::string_view kExpected = "optional_double: -inf";
  EXPECT_THAT(Testee("DoubleFieldNegativeInfinity")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("DoubleFieldNegativeInfinity")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// Non-canonical NaNs are normalized, as for floats above.
TEST_P(JsonDoubleTest, DoubleFieldNormalizeQuietNan) {
  EXPECT_THAT(
      Testee("DoubleFieldNormalizeQuietNan")
          .ParseBinary(message(), Fixed64Field(kOptionalDoubleFieldNumber,
                                               0x7FFA123456789ABC))
          .OverrideTestCategory(::conformance::JSON_TEST)
          .SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto("optional_double: nan"))));
}

TEST_P(JsonDoubleTest, DoubleFieldNormalizeSignalingNan) {
  EXPECT_THAT(
      Testee("DoubleFieldNormalizeSignalingNan")
          .ParseBinary(message(), Fixed64Field(kOptionalDoubleFieldNumber,
                                               0xFFFBCBA987654321))
          .OverrideTestCategory(::conformance::JSON_TEST)
          .SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto("optional_double: nan"))));
}

// Special values must be quoted.
TEST_P(JsonDoubleTest, DoubleFieldNanNotQuoted) {
  EXPECT_THAT(Testee(kP3, "DoubleFieldNanNotQuoted")
                  .ParseJson(message(), R"({"optionalDouble": NaN})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonDoubleTest, DoubleFieldInfinityNotQuoted) {
  EXPECT_THAT(Testee(kP3, "DoubleFieldInfinityNotQuoted")
                  .ParseJson(message(), R"({"optionalDouble": Infinity})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonDoubleTest, DoubleFieldNegativeInfinityNotQuoted) {
  EXPECT_THAT(Testee(kP3, "DoubleFieldNegativeInfinityNotQuoted")
                  .ParseJson(message(), R"({"optionalDouble": -Infinity})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// Parsers should reject out-of-bound values.
TEST_P(JsonDoubleTest, DoubleFieldTooSmall) {
  EXPECT_THAT(Testee("DoubleFieldTooSmall")
                  .ParseJson(message(), R"({"optionalDouble": -1.89769e+308})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonDoubleTest, DoubleFieldTooLarge) {
  EXPECT_THAT(Testee("DoubleFieldTooLarge")
                  .ParseJson(message(), R"({"optionalDouble": 1.89769e+308})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// Parsers should reject empty string values.
TEST_P(JsonDoubleTest, DoubleFieldEmptyString) {
  EXPECT_THAT(Testee("DoubleFieldEmptyString")
                  .ParseJson(message(), R"({"optionalDouble": ""})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// Parsers reject non-numeric string values.
TEST_P(JsonDoubleTest, DoubleFieldStringValuePartiallyNumeric) {
  EXPECT_THAT(Testee("DoubleFieldStringValuePartiallyNumeric")
                  .ParseJson(message(), R"({"optionalDouble": "12abc"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonDoubleTest, DoubleFieldStringValueNonNumeric) {
  EXPECT_THAT(Testee("DoubleFieldStringValueNonNumeric")
                  .ParseJson(message(), R"({"optionalDouble": "abc"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// Parsers reject boolean values for double fields.
TEST_P(JsonDoubleTest, DoubleFieldTrueValue) {
  EXPECT_THAT(Testee("DoubleFieldTrueValue")
                  .ParseJson(message(), R"({"optionalDouble": true})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonDoubleTest, DoubleFieldFalseValue) {
  EXPECT_THAT(Testee("DoubleFieldFalseValue")
                  .ParseJson(message(), R"({"optionalDouble": false})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

INSTANTIATE_TEST_SUITE_P(All, JsonDoubleTest, ValuesIn(AllTestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
