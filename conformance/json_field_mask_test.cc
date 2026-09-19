// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// JSON conformance tests for google.protobuf.FieldMask fields: the
// comma-separated lowerCamelCase form, the empty mask, a path character the
// JSON form cannot express, and the paths that don't round-trip through
// lowerCamelCase and so must be rejected by the serializer.  This replaces the
// legacy BinaryAndJsonConformanceSuiteImpl<M>::RunJsonTestsForFieldMask(),
// which ran for the proto3-style message types only; the test names and the
// requests sent to the testee are identical to the legacy ones, except that
// the binary-input tests below are BINARY_TEST requests (see below).
//
// Like the legacy RunValidJsonTest(), each valid input is sent twice: once to
// be serialized as binary ("<name>.ProtobufOutput") and once as JSON
// ("<name>.JsonOutput").  ExpectParseFailureForJson() asked for JSON output
// under a name without an output suffix.  ExpectSerializeFailureForJson() sent
// the serialized text-format message as binary input, categorized as a JSON
// test, under "<name>.JsonOutput" without an input prefix; the same bytes are
// built here with the wire-format builders, and ParseBinary() categorizes
// them as BINARY_TEST, which no testee tells apart from JSON_TEST on a binary
// payload (b/563657722).

#include <cstdint>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "binary_test_util.h"
#include "binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "matchers.h"
#include "message_type_fixtures.h"
#include "test_environment.h"
#include "testee.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::ValuesIn;

using LegacyName = internal::JsonSerializationOptions::LegacyName;

using JsonFieldMaskTest = MessageTypeConformanceTest;

// Field numbers of the binary inputs of the serialize-failure tests: the
// test message's optional_field_mask and google.protobuf.FieldMask.paths.
constexpr uint32_t kOptionalFieldMask = 303;
constexpr uint32_t kFieldMaskPaths = 1;

TEST_P(JsonFieldMaskTest, FieldMask) {
  constexpr absl::string_view kInput = R"({"optionalFieldMask": "foo,barBaz"})";
  constexpr absl::string_view kExpected =
      R"(optional_field_mask: {paths: "foo" paths: "bar_baz"})";
  EXPECT_THAT(
      RequiredTest("FieldMask").ParseJson(message(), kInput).SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      RequiredTest("FieldMask").ParseJson(message(), kInput).SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonFieldMaskTest, EmptyFieldMask) {
  constexpr absl::string_view kInput = R"({"optionalFieldMask": ""})";
  constexpr absl::string_view kExpected = R"(optional_field_mask: {})";
  EXPECT_THAT(RequiredTest("EmptyFieldMask")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("EmptyFieldMask")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonFieldMaskTest, FieldMaskInvalidCharacter) {
  EXPECT_THAT(
      RecommendedTest("FieldMaskInvalidCharacter")
          .ParseJson(message(), R"({"optionalFieldMask": "foo,bar_bar"})")
          .ParseOnly(),
      Yields(IsParseError()));
}

// The binary form of the legacy input
// `optional_field_mask: {paths: "fooBar"}`.
TEST_P(JsonFieldMaskTest, FieldMaskPathsDontRoundTrip) {
  EXPECT_THAT(
      RecommendedTest("FieldMaskPathsDontRoundTrip")
          .ParseBinary(message(),
                       LengthPrefixedField(
                           kOptionalFieldMask,
                           LengthPrefixedField(kFieldMaskPaths, "fooBar")))
          .SerializeJson({.legacy_name = LegacyName::kWithoutInputFormat}),
      Yields(IsSerializeError()));
}

// The binary form of the legacy input `optional_field_mask: {paths:
// "foo_3_bar"}`.
TEST_P(JsonFieldMaskTest, FieldMaskNumbersDontRoundTrip) {
  EXPECT_THAT(
      RecommendedTest("FieldMaskNumbersDontRoundTrip")
          .ParseBinary(message(),
                       LengthPrefixedField(
                           kOptionalFieldMask,
                           LengthPrefixedField(kFieldMaskPaths, "foo_3_bar")))
          .SerializeJson({.legacy_name = LegacyName::kWithoutInputFormat}),
      Yields(IsSerializeError()));
}

// The binary form of the legacy input `optional_field_mask: {paths:
// "foo__bar"}`.
TEST_P(JsonFieldMaskTest, FieldMaskTooManyUnderscore) {
  EXPECT_THAT(
      RecommendedTest("FieldMaskTooManyUnderscore")
          .ParseBinary(message(),
                       LengthPrefixedField(
                           kOptionalFieldMask,
                           LengthPrefixedField(kFieldMaskPaths, "foo__bar")))
          .SerializeJson({.legacy_name = LegacyName::kWithoutInputFormat}),
      Yields(IsSerializeError()));
}

INSTANTIATE_TEST_SUITE_P(All, JsonFieldMaskTest,
                         ValuesIn(Proto3TestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
