// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// JSON conformance tests for repeated fields: JSON arrays of primitives,
// enums, strings, bytes and messages round-trip, array elements of the wrong
// type are rejected, a single value where an array was expected is rejected,
// and so is a trailing comma in an array.  This replaces the legacy
// BinaryAndJsonConformanceSuiteImpl<M>::RunJsonTestsForRepeatedTypes(); the
// requests sent to the testee are identical to the legacy ones.
//
// Like the legacy RunValidJsonTest(), each valid input is sent twice: once to
// be serialized as binary ("<name>.ProtobufOutput") and once as JSON
// ("<name>.JsonOutput").  ExpectParseFailureForJson() asked for JSON output
// under a name without an output suffix.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "conformance/binary_test_util.h"
#include "conformance/matchers.h"
#include "conformance/message_type_fixtures.h"
#include "conformance/test_environment.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::ValuesIn;

using JsonRepeatedTest = MessageTypeConformanceTest;

// ---------------------------------------------------------------------------
// Repeated fields of each kind round-trip.
// ---------------------------------------------------------------------------

TEST_P(JsonRepeatedTest, PrimitiveRepeatedField) {
  constexpr absl::string_view kInput = R"({"repeatedInt32": [1, 2, 3, 4]})";
  constexpr absl::string_view kExpected = "repeated_int32: [1, 2, 3, 4]";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonRepeatedTest, EnumRepeatedField) {
  constexpr absl::string_view kInput =
      R"({"repeatedNestedEnum": ["FOO", "BAR", "BAZ"]})";
  constexpr absl::string_view kExpected =
      "repeated_nested_enum: [FOO, BAR, BAZ]";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonRepeatedTest, StringRepeatedField) {
  constexpr absl::string_view kInput =
      R"({"repeatedString": ["Hello", "world"]})";
  constexpr absl::string_view kExpected =
      R"(repeated_string: ["Hello", "world"])";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonRepeatedTest, BytesRepeatedField) {
  constexpr absl::string_view kInput = R"({"repeatedBytes": ["AAEC", "AQI="]})";
  constexpr absl::string_view kExpected =
      R"(repeated_bytes: ["\x00\x01\x02", "\x01\x02"])";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonRepeatedTest, MessageRepeatedField) {
  constexpr absl::string_view kInput =
      R"({"repeatedNestedMessage": [{"a": 1234}, {"a": 5678}]})";
  constexpr absl::string_view kExpected =
      "repeated_nested_message: {a: 1234}"
      "repeated_nested_message: {a: 5678}";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// ---------------------------------------------------------------------------
// Repeated field elements of the wrong type are rejected.
// ---------------------------------------------------------------------------

TEST_P(JsonRepeatedTest,
       RepeatedFieldWrongElementTypeExpectingIntegersGotBool) {
  EXPECT_THAT(
      Testee()
          .ParseJson(message(), R"({"repeatedInt32": [1, false, 3, 4]})")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonRepeatedTest,
       RepeatedFieldWrongElementTypeExpectingIntegersGotString) {
  EXPECT_THAT(
      Testee()
          .ParseJson(message(), R"({"repeatedInt32": [1, 2, "name", 4]})")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonRepeatedTest,
       RepeatedFieldWrongElementTypeExpectingIntegersGotMessage) {
  EXPECT_THAT(
      Testee()
          .ParseJson(message(), R"({"repeatedInt32": [1, 2, 3, {"a": 4}]})")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonRepeatedTest, RepeatedFieldWrongElementTypeExpectingStringsGotInt) {
  EXPECT_THAT(
      Testee()
          .ParseJson(message(), R"({"repeatedString": ["1", 2, "3", "4"]})")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonRepeatedTest, RepeatedFieldWrongElementTypeExpectingStringsGotBool) {
  EXPECT_THAT(
      Testee()
          .ParseJson(message(), R"({"repeatedString": ["1", "2", false, "4"]})")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonRepeatedTest,
       RepeatedFieldWrongElementTypeExpectingStringsGotMessage) {
  EXPECT_THAT(Testee()
                  .ParseJson(message(),
                             R"({"repeatedString": ["1", 2, "3", {"a": 4}]})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonRepeatedTest, RepeatedFieldWrongElementTypeExpectingMessagesGotInt) {
  EXPECT_THAT(
      Testee()
          .ParseJson(message(), R"({"repeatedNestedMessage": [{"a": 1}, 2]})")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonRepeatedTest,
       RepeatedFieldWrongElementTypeExpectingMessagesGotBool) {
  EXPECT_THAT(Testee()
                  .ParseJson(message(),
                             R"({"repeatedNestedMessage": [{"a": 1}, false]})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonRepeatedTest,
       RepeatedFieldWrongElementTypeExpectingMessagesGotString) {
  EXPECT_THAT(
      Testee()
          .ParseJson(message(), R"({"repeatedNestedMessage": [{"a": 1}, "2"]})")
          .ParseOnly(),
      Yields(IsParseError()));
}

// ---------------------------------------------------------------------------
// A single value where a repeated field was expected is rejected, even if it
// is of the right type.
// ---------------------------------------------------------------------------

TEST_P(JsonRepeatedTest, SingleValueForRepeatedFieldInt32) {
  EXPECT_THAT(
      Testee().ParseJson(message(), R"({"repeatedInt32": 1})").ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonRepeatedTest, SingleValueForRepeatedFieldMessage) {
  EXPECT_THAT(
      Testee()
          .ParseJson(message(), R"({"repeatedNestedMessage": {"a": 1}})")
          .ParseOnly(),
      Yields(IsParseError()));
}

// ---------------------------------------------------------------------------
// A trailing comma in a repeated field is rejected.
// ---------------------------------------------------------------------------

TEST_P(JsonRepeatedTest, RepeatedFieldTrailingComma) {
  EXPECT_THAT(Testee(kP3)
                  .ParseJson(message(), R"({"repeatedInt32": [1, 2, 3, 4,]})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonRepeatedTest, RepeatedFieldTrailingCommaWithSpace) {
  EXPECT_THAT(Testee(kP3)
                  .ParseJson(message(), "{\"repeatedInt32\": [1, 2, 3, 4 ,]}")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonRepeatedTest, RepeatedFieldTrailingCommaWithSpaceCommaSpace) {
  EXPECT_THAT(Testee(kP3)
                  .ParseJson(message(), "{\"repeatedInt32\": [1, 2, 3, 4 , ]}")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonRepeatedTest, RepeatedFieldTrailingCommaWithNewlines) {
  EXPECT_THAT(
      Testee(kP3)
          .ParseJson(message(),
                     "{\"repeatedInt32\": [\n  1,\n  2,\n  3,\n  4,\n]}")
          .ParseOnly(),
      Yields(IsParseError()));
}

INSTANTIATE_TEST_SUITE_P(All, JsonRepeatedTest, ValuesIn(AllTestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
