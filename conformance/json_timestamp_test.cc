// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// JSON conformance tests for google.protobuf.Timestamp: its RFC 3339 string
// form, its range, UTC offsets, the rejection of out-of-range, malformed or
// invalid date-time components in either direction, and the number of
// fractional digits a serializer emits.  This replaces the Timestamp part of
// the legacy
// BinaryAndJsonConformanceSuiteImpl<M>::RunJsonTestsForWrapperTypes(), which
// ran for the proto3-style message types only; the test names and the requests
// sent to the testee are identical to the legacy ones, except that the
// binary-input tests below are BINARY_TEST requests (see below).
//
// Like the legacy RunValidJsonTest(), each valid input is sent twice: once to
// be serialized as binary ("<name>.ProtobufOutput") and once as JSON
// ("<name>.JsonOutput").  ExpectParseFailureForJson() asked for JSON output
// under a name without an output suffix; ExpectSerializeFailureForJson() sent
// a binary message (built here from its wire format) to be serialized as
// JSON ("ProtobufInput.<name>.JsonOutput"), categorized as JSON_TEST; here
// ParseBinary() categorizes them as BINARY_TEST, which no testee tells apart
// from JSON_TEST on a binary payload (b/563657722).  And the
// legacy "validators" (RunValidJsonTestWithValidator()) looked at the
// serialized JSON text itself.

#include <cstdint>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "conformance/binary_test_util.h"
#include "conformance/binary_wireformat.h"
#include "conformance/json_test_util.h"
#include "conformance/matchers.h"
#include "conformance/message_type_fixtures.h"
#include "conformance/test_environment.h"
#include "conformance/testee.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::ValuesIn;

using JsonTimestampTest = MessageTypeConformanceTest;

// optional_timestamp is field 302 of the test message types; Timestamp's
// seconds is field 1 and nanos field 2.
constexpr uint32_t kOptionalTimestamp = 302;
constexpr uint32_t kSeconds = 1;
constexpr uint32_t kNanos = 2;

TEST_P(JsonTimestampTest, TimestampMinValue) {
  constexpr absl::string_view kInput =
      R"({"optionalTimestamp": "0001-01-01T00:00:00Z"})";
  constexpr absl::string_view kExpected =
      "optional_timestamp: {seconds: -62135596800}";
  EXPECT_THAT(Testee("TimestampMinValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("TimestampMinValue").ParseJson(message(), kInput).SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonTimestampTest, TimestampMaxValue) {
  constexpr absl::string_view kInput =
      R"({"optionalTimestamp": "9999-12-31T23:59:59.999999999Z"})";
  constexpr absl::string_view kExpected =
      "optional_timestamp: {seconds: 253402300799 nanos: 999999999}";
  EXPECT_THAT(Testee("TimestampMaxValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("TimestampMaxValue").ParseJson(message(), kInput).SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonTimestampTest, TimestampRepeatedValue) {
  constexpr absl::string_view kInput = R"({
        "repeatedTimestamp": [
          "0001-01-01T00:00:00Z",
          "9999-12-31T23:59:59.999999999Z"
  ]
      })";
  constexpr absl::string_view kExpected =
      "repeated_timestamp: {seconds: -62135596800}"
      "repeated_timestamp: {seconds: 253402300799 nanos: 999999999}";
  EXPECT_THAT(Testee("TimestampRepeatedValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("TimestampRepeatedValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonTimestampTest, TimestampEpochValue) {
  constexpr absl::string_view kInput =
      R"({"optionalTimestamp": "1970-01-01T00:00:00.000Z"})";
  constexpr absl::string_view kExpected = "optional_timestamp: {seconds: 0}";
  EXPECT_THAT(Testee("TimestampEpochValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("TimestampEpochValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonTimestampTest, TimestampNanoAfterEpochlValue) {
  constexpr absl::string_view kInput =
      R"({"optionalTimestamp": "1970-01-01T00:00:00.000000001Z"})";
  constexpr absl::string_view kExpected =
      "optional_timestamp: {seconds: 0 nanos: 1}";
  EXPECT_THAT(Testee("TimestampNanoAfterEpochlValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("TimestampNanoAfterEpochlValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonTimestampTest, TimestampNanoBeforeEpochValue) {
  constexpr absl::string_view kInput =
      R"({"optionalTimestamp": "1969-12-31T23:59:59.999999999Z"})";
  constexpr absl::string_view kExpected =
      "optional_timestamp: {seconds: -1 nanos: 999999999}";
  EXPECT_THAT(Testee("TimestampNanoBeforeEpochValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("TimestampNanoBeforeEpochValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonTimestampTest, TimestampLittleAfterEpochlValue) {
  constexpr absl::string_view kInput =
      R"({"optionalTimestamp": "1970-01-01T00:00:01.000000001Z"})";
  constexpr absl::string_view kExpected =
      "optional_timestamp: {seconds: 1 nanos: 1}";
  EXPECT_THAT(Testee("TimestampLittleAfterEpochlValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("TimestampLittleAfterEpochlValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonTimestampTest, TimestampLittleBeforeEpochValue) {
  constexpr absl::string_view kInput =
      R"({"optionalTimestamp": "1969-12-31T23:59:58.999999999Z"})";
  constexpr absl::string_view kExpected =
      "optional_timestamp: {seconds: -2 nanos: 999999999}";
  EXPECT_THAT(Testee("TimestampLittleBeforeEpochValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("TimestampLittleBeforeEpochValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonTimestampTest, TimestampTenAndHalfSecondsAfterEpochValue) {
  constexpr absl::string_view kInput =
      R"({"optionalTimestamp": "1970-01-01T00:00:10.500Z"})";
  constexpr absl::string_view kExpected =
      "optional_timestamp: {seconds: 10 nanos: 500000000}";
  EXPECT_THAT(Testee("TimestampTenAndHalfSecondsAfterEpochValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("TimestampTenAndHalfSecondsAfterEpochValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonTimestampTest, TimestampTenAndHalfSecondsBeforeEpochValue) {
  constexpr absl::string_view kInput =
      R"({"optionalTimestamp": "1969-12-31T23:59:49.500Z"})";
  constexpr absl::string_view kExpected =
      "optional_timestamp: {seconds: -11 nanos: 500000000}";
  EXPECT_THAT(Testee("TimestampTenAndHalfSecondsBeforeEpochValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("TimestampTenAndHalfSecondsBeforeEpochValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonTimestampTest, TimestampLeap) {
  constexpr absl::string_view kInput =
      R"({"optionalTimestamp": "1993-02-10T00:00:00.000Z"})";
  constexpr absl::string_view kExpected =
      "optional_timestamp: {seconds: 729302400}";
  EXPECT_THAT(
      Testee("TimestampLeap").ParseJson(message(), kInput).SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("TimestampLeap").ParseJson(message(), kInput).SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonTimestampTest, TimestampWithPositiveOffset) {
  constexpr absl::string_view kInput =
      R"({"optionalTimestamp": "1970-01-01T08:00:01+08:00"})";
  constexpr absl::string_view kExpected = "optional_timestamp: {seconds: 1}";
  EXPECT_THAT(Testee("TimestampWithPositiveOffset")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("TimestampWithPositiveOffset")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonTimestampTest, TimestampWithNegativeOffset) {
  constexpr absl::string_view kInput =
      R"({"optionalTimestamp": "1969-12-31T16:00:01-08:00"})";
  constexpr absl::string_view kExpected = "optional_timestamp: {seconds: 1}";
  EXPECT_THAT(Testee("TimestampWithNegativeOffset")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("TimestampWithNegativeOffset")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonTimestampTest, TimestampNull) {
  constexpr absl::string_view kInput = R"({"optionalTimestamp": null})";
  constexpr absl::string_view kExpected = "";
  EXPECT_THAT(
      Testee("TimestampNull").ParseJson(message(), kInput).SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("TimestampNull").ParseJson(message(), kInput).SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// Out-of-range and malformed JSON values are rejected.
TEST_P(JsonTimestampTest, TimestampJsonInputTooSmall) {
  EXPECT_THAT(Testee("TimestampJsonInputTooSmall")
                  .ParseJson(message(),
                             R"({"optionalTimestamp": "0000-01-01T00:00:00Z"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonTimestampTest, TimestampJsonInputTooLarge) {
  EXPECT_THAT(
      Testee("TimestampJsonInputTooLarge")
          .ParseJson(message(),
                     R"({"optionalTimestamp": "10000-01-01T00:00:00Z"})")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonTimestampTest, TimestampJsonInputMissingZ) {
  EXPECT_THAT(Testee("TimestampJsonInputMissingZ")
                  .ParseJson(message(),
                             R"({"optionalTimestamp": "0001-01-01T00:00:00"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonTimestampTest, TimestampJsonInputMissingT) {
  EXPECT_THAT(Testee("TimestampJsonInputMissingT")
                  .ParseJson(message(),
                             R"({"optionalTimestamp": "0001-01-01 00:00:00Z"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonTimestampTest, TimestampJsonInputLowercaseZ) {
  EXPECT_THAT(Testee("TimestampJsonInputLowercaseZ")
                  .ParseJson(message(),
                             R"({"optionalTimestamp": "0001-01-01T00:00:00z"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonTimestampTest, TimestampJsonInputLowercaseT) {
  EXPECT_THAT(Testee("TimestampJsonInputLowercaseT")
                  .ParseJson(message(),
                             R"({"optionalTimestamp": "0001-01-01t00:00:00Z"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonTimestampTest, TimestampWithMissingColonInOffset) {
  EXPECT_THAT(
      Testee("TimestampWithMissingColonInOffset")
          .ParseJson(message(),
                     R"({"optionalTimestamp": "1970-01-01T08:00:01+0800"})")
          .ParseOnly(),
      Yields(IsParseError()));
}

// Out-of-range or inconsistent messages can't be serialized.  The inputs
// are the legacy text-format messages on the wire (zero fields omitted).
TEST_P(JsonTimestampTest, TimestampProtoInputTooSmall) {
  EXPECT_THAT(
      Testee("TimestampProtoInputTooSmall")
          .ParseBinary(message(),
                       LengthPrefixedField(kOptionalTimestamp,
                                           VarintField(kSeconds, -62135596801)))
          .SerializeJson(),
      Yields(IsSerializeError()));
}

TEST_P(JsonTimestampTest, TimestampProtoInputTooLarge) {
  EXPECT_THAT(
      Testee("TimestampProtoInputTooLarge")
          .ParseBinary(message(),
                       LengthPrefixedField(kOptionalTimestamp,
                                           VarintField(kSeconds, 253402300800)))
          .SerializeJson(),
      Yields(IsSerializeError()));
}

TEST_P(JsonTimestampTest, TimestampProtoNegativeNanos) {
  EXPECT_THAT(Testee("TimestampProtoNegativeNanos")
                  .ParseBinary(message(), LengthPrefixedField(
                                              kOptionalTimestamp,
                                              Wire(VarintField(kSeconds, 5000),
                                                   VarintField(kNanos, -1))))
                  .SerializeJson(),
              Yields(IsSerializeError()));
}

TEST_P(JsonTimestampTest, TimestampProtoNanoTooLarge) {
  EXPECT_THAT(
      Testee("TimestampProtoNanoTooLarge")
          .ParseBinary(message(), LengthPrefixedField(
                                      kOptionalTimestamp,
                                      Wire(VarintField(kSeconds, 5000),
                                           VarintField(kNanos, 1000000000))))
          .SerializeJson(),
      Yields(IsSerializeError()));
}

// Serializers normalize to UTC and emit 0, 3, 6 or 9 fractional digits,
// whichever is the fewest that represent the value exactly.
TEST_P(JsonTimestampTest, TimestampZeroNormalized) {
  EXPECT_THAT(
      Testee(kP3, "TimestampZeroNormalized")
          .ParseJson(message(),
                     R"({"optionalTimestamp": "1969-12-31T16:00:00-08:00"})")
          .SerializeJson(),
      Yields(JsonPayload(HasJsonMemberThat(
          "optionalTimestamp", IsJsonString("1970-01-01T00:00:00Z")))));
}

TEST_P(JsonTimestampTest, TimestampHasZeroFractionalDigit) {
  EXPECT_THAT(
      Testee(kP3, "TimestampHasZeroFractionalDigit")
          .ParseJson(
              message(),
              R"({"optionalTimestamp": "1970-01-01T00:00:00.000000000Z"})")
          .SerializeJson(),
      Yields(JsonPayload(HasJsonMemberThat(
          "optionalTimestamp", IsJsonString("1970-01-01T00:00:00Z")))));
}

TEST_P(JsonTimestampTest, TimestampHas3FractionalDigits) {
  EXPECT_THAT(
      Testee(kP3, "TimestampHas3FractionalDigits")
          .ParseJson(
              message(),
              R"({"optionalTimestamp": "1970-01-01T00:00:00.010000000Z"})")
          .SerializeJson(),
      Yields(JsonPayload(HasJsonMemberThat(
          "optionalTimestamp", IsJsonString("1970-01-01T00:00:00.010Z")))));
}

TEST_P(JsonTimestampTest, TimestampHas6FractionalDigits) {
  EXPECT_THAT(
      Testee(kP3, "TimestampHas6FractionalDigits")
          .ParseJson(
              message(),
              R"({"optionalTimestamp": "1970-01-01T00:00:00.000010000Z"})")
          .SerializeJson(),
      Yields(JsonPayload(HasJsonMemberThat(
          "optionalTimestamp", IsJsonString("1970-01-01T00:00:00.000010Z")))));
}

TEST_P(JsonTimestampTest, TimestampHas9FractionalDigits) {
  EXPECT_THAT(
      Testee(kP3, "TimestampHas9FractionalDigits")
          .ParseJson(
              message(),
              R"({"optionalTimestamp": "1970-01-01T00:00:00.000000010Z"})")
          .SerializeJson(),
      Yields(JsonPayload(
          HasJsonMemberThat("optionalTimestamp",
                            IsJsonString("1970-01-01T00:00:00.000000010Z")))));
}

// Out of bounds / invalid components should JSON parse fail.
TEST_P(JsonTimestampTest, TimestampJsonInputMonthTooLarge) {
  EXPECT_THAT(Testee("TimestampJsonInputMonthTooLarge")
                  .ParseJson(message(),
                             R"({"optionalTimestamp": "1970-13-01T00:00:00Z"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonTimestampTest, TimestampJsonInputMonthZero) {
  EXPECT_THAT(Testee("TimestampJsonInputMonthZero")
                  .ParseJson(message(),
                             R"({"optionalTimestamp": "1970-00-01T00:00:00Z"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonTimestampTest, TimestampJsonInputDayTooLarge) {
  EXPECT_THAT(Testee("TimestampJsonInputDayTooLarge")
                  .ParseJson(message(),
                             R"({"optionalTimestamp": "1970-01-32T00:00:00Z"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonTimestampTest, TimestampJsonInputDayZero) {
  EXPECT_THAT(Testee("TimestampJsonInputDayZero")
                  .ParseJson(message(),
                             R"({"optionalTimestamp": "1970-01-00T00:00:00Z"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonTimestampTest, TimestampJsonInputHourTooLarge) {
  EXPECT_THAT(Testee("TimestampJsonInputHourTooLarge")
                  .ParseJson(message(),
                             R"({"optionalTimestamp": "1970-01-01T24:00:00Z"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonTimestampTest, TimestampJsonInputHourTooLarge25) {
  EXPECT_THAT(Testee("TimestampJsonInputHourTooLarge25")
                  .ParseJson(message(),
                             R"({"optionalTimestamp": "1970-01-01T25:00:00Z"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonTimestampTest, TimestampJsonInputMinuteTooLarge) {
  EXPECT_THAT(Testee("TimestampJsonInputMinuteTooLarge")
                  .ParseJson(message(),
                             R"({"optionalTimestamp": "1970-01-01T00:60:00Z"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonTimestampTest, TimestampJsonInputSecondTooLarge) {
  EXPECT_THAT(Testee("TimestampJsonInputSecondTooLarge")
                  .ParseJson(message(),
                             R"({"optionalTimestamp": "1970-01-01T00:00:60Z"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonTimestampTest, TimestampJsonInputInvalidOffsetHour) {
  EXPECT_THAT(
      Testee("TimestampJsonInputInvalidOffsetHour")
          .ParseJson(message(),
                     R"({"optionalTimestamp": "1970-01-01T00:00:00+24:00"})")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonTimestampTest, TimestampJsonInputInvalidOffsetMinute) {
  EXPECT_THAT(
      Testee("TimestampJsonInputInvalidOffsetMinute")
          .ParseJson(message(),
                     R"({"optionalTimestamp": "1970-01-01T00:00:00+00:60"})")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonTimestampTest, TimestampJsonInputOffsetBoundaryUnderflow) {
  EXPECT_THAT(
      Testee("TimestampJsonInputOffsetBoundaryUnderflow")
          .ParseJson(message(),
                     R"({"optionalTimestamp": "0001-01-01T00:00:00+00:01"})")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonTimestampTest, TimestampJsonInputOffsetBoundaryOverflow) {
  EXPECT_THAT(
      Testee("TimestampJsonInputOffsetBoundaryOverflow")
          .ParseJson(message(),
                     R"({"optionalTimestamp": "9999-12-31T23:59:59-00:01"})")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonTimestampTest, TimestampJsonInputInvalidNanos) {
  EXPECT_THAT(
      Testee("TimestampJsonInputInvalidNanos")
          .ParseJson(
              message(),
              R"({"optionalTimestamp": "1970-01-01T00:00:00.1234567890Z"})")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonTimestampTest, TimestampJsonInputInvalidCharsInNanos) {
  EXPECT_THAT(
      Testee("TimestampJsonInputInvalidCharsInNanos")
          .ParseJson(message(),
                     R"({"optionalTimestamp": "1970-01-01T00:00:00.123aZ"})")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonTimestampTest, TimestampJsonInputYearTooShort) {
  EXPECT_THAT(Testee("TimestampJsonInputYearTooShort")
                  .ParseJson(message(),
                             R"({"optionalTimestamp": "999-01-01T00:00:00Z"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonTimestampTest, TimestampJsonInputYearTooLong) {
  EXPECT_THAT(
      Testee("TimestampJsonInputYearTooLong")
          .ParseJson(message(),
                     R"({"optionalTimestamp": "00001-01-01T00:00:00Z"})")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonTimestampTest, TimestampJsonInputMonthTooShort) {
  EXPECT_THAT(Testee("TimestampJsonInputMonthTooShort")
                  .ParseJson(message(),
                             R"({"optionalTimestamp": "1970-1-01T00:00:00Z"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonTimestampTest, TimestampJsonInputDayTooShort) {
  EXPECT_THAT(Testee("TimestampJsonInputDayTooShort")
                  .ParseJson(message(),
                             R"({"optionalTimestamp": "1970-01-1T00:00:00Z"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonTimestampTest, TimestampJsonInputNonLeapFeb29) {
  EXPECT_THAT(Testee("TimestampJsonInputNonLeapFeb29")
                  .ParseJson(message(),
                             R"({"optionalTimestamp": "2001-02-29T00:00:00Z"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// Honoring time zones correctly.
TEST_P(JsonTimestampTest, TimestampJsonInputLeapFeb29) {
  constexpr absl::string_view kInput =
      R"({"optionalTimestamp": "2000-02-29T00:00:00Z"})";
  constexpr absl::string_view kExpected =
      "optional_timestamp: {seconds: 951782400}";
  EXPECT_THAT(Testee("TimestampJsonInputLeapFeb29")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("TimestampJsonInputLeapFeb29")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonTimestampTest, TimestampWithOffsetShiftsDay) {
  constexpr absl::string_view kInput =
      R"({"optionalTimestamp": "1970-01-02T01:00:00+02:00"})";
  constexpr absl::string_view kExpected =
      "optional_timestamp: {seconds: 82800}";
  EXPECT_THAT(Testee("TimestampWithOffsetShiftsDay")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("TimestampWithOffsetShiftsDay")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonTimestampTest, TimestampWithOffsetBoundaryInBoundsMin) {
  constexpr absl::string_view kInput =
      R"({"optionalTimestamp": "0001-01-01T00:00:00-00:01"})";
  constexpr absl::string_view kExpected =
      "optional_timestamp: {seconds: -62135596740}";
  EXPECT_THAT(Testee("TimestampWithOffsetBoundaryInBoundsMin")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("TimestampWithOffsetBoundaryInBoundsMin")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonTimestampTest, TimestampWithOffsetBoundaryInBoundsMax) {
  constexpr absl::string_view kInput =
      R"({"optionalTimestamp": "9999-12-31T23:59:59+00:01"})";
  constexpr absl::string_view kExpected =
      "optional_timestamp: {seconds: 253402300739}";
  EXPECT_THAT(Testee("TimestampWithOffsetBoundaryInBoundsMax")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("TimestampWithOffsetBoundaryInBoundsMax")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonTimestampTest, TimestampWithComplexOffset) {
  constexpr absl::string_view kInput =
      R"({"optionalTimestamp": "1970-01-01T00:00:00-11:30"})";
  constexpr absl::string_view kExpected =
      "optional_timestamp: {seconds: 41400}";
  EXPECT_THAT(Testee("TimestampWithComplexOffset")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("TimestampWithComplexOffset")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

INSTANTIATE_TEST_SUITE_P(All, JsonTimestampTest,
                         ValuesIn(Proto3TestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
