// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// JSON conformance tests for string and bytes fields: Unicode text, JSON
// escapes (including \u escapes, surrogate pairs and embedded NUL), rejected
// malformed escapes, unpaired surrogates, non-string values and single-quoted
// strings, and base64 / base64url bytes.  This replaces the "String fields"
// and "Bytes fields" blocks (and the single-quote tests at the end) of the
// legacy
// BinaryAndJsonConformanceSuiteImpl<M>::RunJsonTestsForNonRepeatedTypes(); the
// test names and the requests sent to the testee are identical to the legacy
// ones.
//
// Like the legacy RunValidJsonTest(), each valid input is sent twice: once to
// be serialized as binary ("<name>.ProtobufOutput") and once as JSON
// ("<name>.JsonOutput").  ExpectParseFailureForJson() asked for JSON output
// under a name without an output suffix.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "binary_test_util.h"
#include "matchers.h"
#include "message_type_fixtures.h"
#include "test_environment.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::ValuesIn;

// ---------------------------------------------------------------------------
// String fields.
// ---------------------------------------------------------------------------

using JsonStringTest = MessageTypeConformanceTest;

TEST_P(JsonStringTest, StringField) {
  constexpr absl::string_view kInput = R"({"optionalString": "Hello world!"})";
  constexpr absl::string_view kExpected = R"(optional_string: "Hello world!")";
  EXPECT_THAT(RequiredTest("StringField")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      RequiredTest("StringField").ParseJson(message(), kInput).SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// Google in Chinese.
TEST_P(JsonStringTest, StringFieldUnicode) {
  constexpr absl::string_view kInput = R"({"optionalString": "谷歌"})";
  constexpr absl::string_view kExpected = R"(optional_string: "谷歌")";
  EXPECT_THAT(RequiredTest("StringFieldUnicode")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("StringFieldUnicode")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonStringTest, StringFieldEscape) {
  constexpr absl::string_view kInput =
      R"({"optionalString": "\"\\\/\b\f\n\r\t"})";
  constexpr absl::string_view kExpected =
      R"(optional_string: "\"\\/\b\f\n\r\t")";
  EXPECT_THAT(RequiredTest("StringFieldEscape")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("StringFieldEscape")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonStringTest, StringFieldUnicodeEscape) {
  constexpr absl::string_view kInput = R"({"optionalString": "\u8C37\u6B4C"})";
  constexpr absl::string_view kExpected = R"(optional_string: "谷歌")";
  EXPECT_THAT(RequiredTest("StringFieldUnicodeEscape")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("StringFieldUnicodeEscape")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonStringTest, StringFieldUnicodeEscapeWithLowercaseHexLetters) {
  constexpr absl::string_view kInput = R"({"optionalString": "\u8c37\u6b4c"})";
  constexpr absl::string_view kExpected = R"(optional_string: "谷歌")";
  EXPECT_THAT(RequiredTest("StringFieldUnicodeEscapeWithLowercaseHexLetters")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("StringFieldUnicodeEscapeWithLowercaseHexLetters")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// The character is an emoji: grinning face with smiling eyes. 😁
TEST_P(JsonStringTest, StringFieldSurrogatePair) {
  constexpr absl::string_view kInput = R"({"optionalString": "\uD83D\uDE01"})";
  constexpr absl::string_view kExpected =
      R"(optional_string: "\xF0\x9F\x98\x81")";
  EXPECT_THAT(RequiredTest("StringFieldSurrogatePair")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("StringFieldSurrogatePair")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonStringTest, StringFieldEmbeddedNull) {
  constexpr absl::string_view kInput =
      R"({"optionalString": "Hello\u0000world!"})";
  constexpr absl::string_view kExpected =
      R"(optional_string: "Hello\000world!")";
  EXPECT_THAT(RequiredTest("StringFieldEmbeddedNull")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("StringFieldEmbeddedNull")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// Unicode escapes must start with "\u" (lowercase u).
TEST_P(JsonStringTest, StringFieldUppercaseEscapeLetter) {
  EXPECT_THAT(RecommendedTest("StringFieldUppercaseEscapeLetter")
                  .ParseJson(message(), R"({"optionalString": "\U8C37\U6b4C"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonStringTest, StringFieldInvalidEscape) {
  EXPECT_THAT(RecommendedTest("StringFieldInvalidEscape")
                  .ParseJson(message(), R"({"optionalString": "\uXXXX\u6B4C"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonStringTest, StringFieldUnterminatedEscape) {
  EXPECT_THAT(RecommendedTest("StringFieldUnterminatedEscape")
                  .ParseJson(message(), R"({"optionalString": "\u8C3"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonStringTest, StringFieldUnpairedHighSurrogate) {
  EXPECT_THAT(RecommendedTest("StringFieldUnpairedHighSurrogate")
                  .ParseJson(message(), R"({"optionalString": "\uD800"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonStringTest, StringFieldUnpairedLowSurrogate) {
  EXPECT_THAT(RecommendedTest("StringFieldUnpairedLowSurrogate")
                  .ParseJson(message(), R"({"optionalString": "\uDC00"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonStringTest, StringFieldSurrogateInWrongOrder) {
  EXPECT_THAT(RecommendedTest("StringFieldSurrogateInWrongOrder")
                  .ParseJson(message(), R"({"optionalString": "\uDE01\uD83D"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonStringTest, StringFieldNotAString) {
  EXPECT_THAT(RequiredTest("StringFieldNotAString")
                  .ParseJson(message(), R"({"optionalString": 12345})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// http://www.rfc-editor.org/rfc/rfc7159.txt says strings have to use double
// quotes.
TEST_P(JsonStringTest, StringFieldSingleQuoteKey) {
  EXPECT_THAT(RecommendedTest("StringFieldSingleQuoteKey")
                  .ParseJson(message(), R"({'optionalString': "Hello world!"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonStringTest, StringFieldSingleQuoteValue) {
  EXPECT_THAT(RecommendedTest("StringFieldSingleQuoteValue")
                  .ParseJson(message(), R"({"optionalString": 'Hello world!'})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonStringTest, StringFieldSingleQuoteBoth) {
  EXPECT_THAT(RecommendedTest("StringFieldSingleQuoteBoth")
                  .ParseJson(message(), R"({'optionalString': 'Hello world!'})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

INSTANTIATE_TEST_SUITE_P(All, JsonStringTest, ValuesIn(AllTestMessageTypes()),
                         MessageTypeParamName);

// ---------------------------------------------------------------------------
// Bytes fields.
// ---------------------------------------------------------------------------

using JsonBytesTest = MessageTypeConformanceTest;

TEST_P(JsonBytesTest, BytesField) {
  constexpr absl::string_view kInput = R"({"optionalBytes": "AQI="})";
  constexpr absl::string_view kExpected = R"(optional_bytes: "\x01\x02")";
  EXPECT_THAT(
      RequiredTest("BytesField").ParseJson(message(), kInput).SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      RequiredTest("BytesField").ParseJson(message(), kInput).SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonBytesTest, BytesFieldBase64Url) {
  constexpr absl::string_view kInput = R"({"optionalBytes": "-_"})";
  constexpr absl::string_view kExpected = R"(optional_bytes: "\xfb")";
  EXPECT_THAT(RecommendedTest("BytesFieldBase64Url")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RecommendedTest("BytesFieldBase64Url")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

INSTANTIATE_TEST_SUITE_P(All, JsonBytesTest, ValuesIn(AllTestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
