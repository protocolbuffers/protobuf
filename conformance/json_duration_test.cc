// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// JSON conformance tests for google.protobuf.Duration: its "<seconds>s"
// string form with up to nine fractional digits, its range, the rejection of
// out-of-range or malformed values in either direction, and the number of
// fractional digits a serializer emits.  This replaces the Duration part of
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
// JSON under "<S>.<E>.<name>.JsonOutput", categorized as JSON_TEST; here
// ParseBinary() categorizes them as BINARY_TEST, which no testee tells apart
// from JSON_TEST on a binary payload (b/563657722).  And the
// legacy "validators" (RunValidJsonTestWithValidator()) looked at the
// serialized JSON text itself under the name "<name>.Validator".

#include <cstdint>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "binary_test_util.h"
#include "binary_wireformat.h"
#include "conformance/conformance.pb.h"
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

// Names a JSON-output test "<name>.Validator" like the legacy validators, and
// "<S>.<E>.<name>.JsonOutput" like the legacy ExpectSerializeFailureForJson().
using LegacyName = internal::JsonSerializationOptions::LegacyName;

using JsonDurationTest = MessageTypeConformanceTest;

// optional_duration is field 301 of the test message types; Duration's
// seconds is field 1 and nanos field 2.
constexpr uint32_t kOptionalDuration = 301;
constexpr uint32_t kSeconds = 1;
constexpr uint32_t kNanos = 2;

TEST_P(JsonDurationTest, DurationMinValue) {
  constexpr absl::string_view kInput =
      R"({"optionalDuration": "-315576000000.999999999s"})";
  constexpr absl::string_view kExpected =
      "optional_duration: {seconds: -315576000000 nanos: -999999999}";
  EXPECT_THAT(RequiredTest("DurationMinValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("DurationMinValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonDurationTest, DurationMaxValue) {
  constexpr absl::string_view kInput =
      R"({"optionalDuration": "315576000000.999999999s"})";
  constexpr absl::string_view kExpected =
      "optional_duration: {seconds: 315576000000 nanos: 999999999}";
  EXPECT_THAT(RequiredTest("DurationMaxValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("DurationMaxValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonDurationTest, DurationRepeatedValue) {
  constexpr absl::string_view kInput =
      R"({"repeatedDuration": ["1.5s", "-1.5s"]})";
  constexpr absl::string_view kExpected =
      "repeated_duration: {seconds: 1 nanos: 500000000}"
      "repeated_duration: {seconds: -1 nanos: -500000000}";
  EXPECT_THAT(RequiredTest("DurationRepeatedValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("DurationRepeatedValue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonDurationTest, DurationNull) {
  constexpr absl::string_view kInput = R"({"optionalDuration": null})";
  constexpr absl::string_view kExpected = "";
  EXPECT_THAT(RequiredTest("DurationNull")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      RequiredTest("DurationNull").ParseJson(message(), kInput).SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonDurationTest, DurationNegativeSeconds) {
  constexpr absl::string_view kInput = R"({"optionalDuration": "-5s"})";
  constexpr absl::string_view kExpected =
      "optional_duration: {seconds: -5 nanos: 0}";
  EXPECT_THAT(RequiredTest("DurationNegativeSeconds")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("DurationNegativeSeconds")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonDurationTest, DurationNegativeNanos) {
  constexpr absl::string_view kInput = R"({"optionalDuration": "-0.5s"})";
  constexpr absl::string_view kExpected =
      "optional_duration: {seconds: 0 nanos: -500000000}";
  EXPECT_THAT(RequiredTest("DurationNegativeNanos")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("DurationNegativeNanos")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// Malformed and out-of-range JSON values are rejected.
TEST_P(JsonDurationTest, DurationMissingS) {
  EXPECT_THAT(RequiredTest("DurationMissingS")
                  .ParseJson(message(), R"({"optionalDuration": "1"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonDurationTest, DurationJsonInputTooSmall) {
  EXPECT_THAT(
      RequiredTest("DurationJsonInputTooSmall")
          .ParseJson(message(),
                     R"({"optionalDuration": "-315576000001.000000000s"})")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonDurationTest, DurationJsonInputTooLarge) {
  EXPECT_THAT(
      RequiredTest("DurationJsonInputTooLarge")
          .ParseJson(message(),
                     R"({"optionalDuration": "315576000001.000000000s"})")
          .ParseOnly(),
      Yields(IsParseError()));
}

// Out-of-range or inconsistent messages can't be serialized.  The inputs
// are the legacy text-format messages on the wire (zero fields omitted).
TEST_P(JsonDurationTest, DurationProtoInputTooSmall) {
  EXPECT_THAT(
      RequiredTest("DurationProtoInputTooSmall")
          .ParseBinary(message(), LengthPrefixedField(
                                      kOptionalDuration,
                                      VarintField(kSeconds, -315576000001)))
          .SerializeJson({.legacy_name = LegacyName::kWithoutInputFormat}),
      Yields(IsSerializeError()));
}

TEST_P(JsonDurationTest, DurationProtoInputTooLarge) {
  EXPECT_THAT(
      RequiredTest("DurationProtoInputTooLarge")
          .ParseBinary(message(),
                       LengthPrefixedField(kOptionalDuration,
                                           VarintField(kSeconds, 315576000001)))
          .SerializeJson({.legacy_name = LegacyName::kWithoutInputFormat}),
      Yields(IsSerializeError()));
}

TEST_P(JsonDurationTest, DurationProtoNanosWrongSign) {
  EXPECT_THAT(
      RequiredTest("DurationProtoNanosWrongSign")
          .ParseBinary(message(),
                       LengthPrefixedField(kOptionalDuration,
                                           Wire(VarintField(kSeconds, 1),
                                                VarintField(kNanos, -1))))
          .SerializeJson({.legacy_name = LegacyName::kWithoutInputFormat}),
      Yields(IsSerializeError()));
}

TEST_P(JsonDurationTest, DurationProtoNanosWrongSignNegativeSecs) {
  EXPECT_THAT(
      RequiredTest("DurationProtoNanosWrongSignNegativeSecs")
          .ParseBinary(message(),
                       LengthPrefixedField(kOptionalDuration,
                                           Wire(VarintField(kSeconds, -1),
                                                VarintField(kNanos, 1))))
          .SerializeJson({.legacy_name = LegacyName::kWithoutInputFormat}),
      Yields(IsSerializeError()));
}

TEST_P(JsonDurationTest, DurationProtoNanosTooSmall) {
  EXPECT_THAT(
      RequiredTest("DurationProtoNanosTooSmall")
          .ParseBinary(message(), LengthPrefixedField(
                                      kOptionalDuration,
                                      Wire(VarintField(kSeconds, -1),
                                           VarintField(kNanos, -1000000000))))
          .SerializeJson({.legacy_name = LegacyName::kWithoutInputFormat}),
      Yields(IsSerializeError()));
}

TEST_P(JsonDurationTest, DurationProtoNanosTooLarge) {
  EXPECT_THAT(
      RequiredTest("DurationProtoNanosTooLarge")
          .ParseBinary(message(), LengthPrefixedField(
                                      kOptionalDuration,
                                      Wire(VarintField(kSeconds, 1),
                                           VarintField(kNanos, 1000000000))))
          .SerializeJson({.legacy_name = LegacyName::kWithoutInputFormat}),
      Yields(IsSerializeError()));
}

// Serializers emit 0, 3, 6 or 9 fractional digits, whichever is the fewest
// that represent the value exactly.
TEST_P(JsonDurationTest, DurationHasZeroFractionalDigit) {
  EXPECT_THAT(
      RecommendedTest("DurationHasZeroFractionalDigit")
          .ParseJson(message(), R"({"optionalDuration": "1.000000000s"})")
          .SerializeJson({.legacy_name = LegacyName::kValidatorSuffix}),
      Yields(JsonPayload(
          HasJsonMemberThat("optionalDuration", IsJsonString("1s")))));
}

TEST_P(JsonDurationTest, DurationHas3FractionalDigits) {
  EXPECT_THAT(
      RecommendedTest("DurationHas3FractionalDigits")
          .ParseJson(message(), R"({"optionalDuration": "1.010000000s"})")
          .SerializeJson({.legacy_name = LegacyName::kValidatorSuffix}),
      Yields(JsonPayload(
          HasJsonMemberThat("optionalDuration", IsJsonString("1.010s")))));
}

TEST_P(JsonDurationTest, DurationHas6FractionalDigits) {
  EXPECT_THAT(
      RecommendedTest("DurationHas6FractionalDigits")
          .ParseJson(message(), R"({"optionalDuration": "1.000010000s"})")
          .SerializeJson({.legacy_name = LegacyName::kValidatorSuffix}),
      Yields(JsonPayload(
          HasJsonMemberThat("optionalDuration", IsJsonString("1.000010s")))));
}

TEST_P(JsonDurationTest, DurationHas9FractionalDigits) {
  EXPECT_THAT(
      RecommendedTest("DurationHas9FractionalDigits")
          .ParseJson(message(), R"({"optionalDuration": "1.000000010s"})")
          .SerializeJson({.legacy_name = LegacyName::kValidatorSuffix}),
      Yields(JsonPayload(HasJsonMemberThat("optionalDuration",
                                           IsJsonString("1.000000010s")))));
}

INSTANTIATE_TEST_SUITE_P(All, JsonDurationTest,
                         ValuesIn(Proto3TestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
