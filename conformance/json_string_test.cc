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
#include "conformance/testee.h"

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
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// Non-ASCII characters in the JSON text itself, as opposed to the \uXXXX
// escapes tested below.  Note that kInput is deliberately not a raw string
// literal: the compiler resolves the \x escapes, so the payload holds the
// UTF-8 bytes of U+8C37 U+6B4C ("Google" in Chinese).  In the raw string
// literals below, the \u escapes reach the testee's JSON parser verbatim.
TEST_P(JsonStringTest, StringFieldUnicode) {
  constexpr absl::string_view kInput =
      "{\"optionalString\": \"\xE8\xB0\xB7\xE6\xAD\x8C\"}";
  constexpr absl::string_view kExpected =
      R"(optional_string: "\xE8\xB0\xB7\xE6\xAD\x8C")";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonStringTest, StringFieldEscape) {
  constexpr absl::string_view kInput =
      R"({"optionalString": "\"\\\/\b\f\n\r\t"})";
  constexpr absl::string_view kExpected =
      R"(optional_string: "\"\\/\b\f\n\r\t")";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonStringTest, StringFieldUnicodeEscape) {
  constexpr absl::string_view kInput = R"({"optionalString": "\u8C37\u6B4C"})";
  constexpr absl::string_view kExpected =
      R"(optional_string: "\xE8\xB0\xB7\xE6\xAD\x8C")";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// U+1F601 (grinning face with smiling eyes).
TEST_P(JsonStringTest, StringFieldSurrogatePair) {
  constexpr absl::string_view kInput = R"({"optionalString": "\uD83D\uDE01"})";
  constexpr absl::string_view kExpected =
      R"(optional_string: "\xF0\x9F\x98\x81")";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonStringTest, StringFieldEmbeddedNull) {
  constexpr absl::string_view kInput =
      R"({"optionalString": "Hello\u0000world!"})";
  constexpr absl::string_view kExpected =
      R"(optional_string: "Hello\000world!")";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// Unicode escapes must start with "\u" (lowercase u).
TEST_P(JsonStringTest, StringFieldUppercaseEscapeLetter) {
  EXPECT_THAT(Testee(kP3)
                  .ParseJson(message(), R"({"optionalString": "\U8C37\U6b4C"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonStringTest, StringFieldInvalidEscape) {
  EXPECT_THAT(Testee(kP3)
                  .ParseJson(message(), R"({"optionalString": "\uXXXX\u6B4C"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonStringTest, StringFieldUnterminatedEscape) {
  EXPECT_THAT(Testee(kP3)
                  .ParseJson(message(), R"({"optionalString": "\u8C3"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// The unpaired-surrogate inputs below are ordinary (not raw) string literals:
// MSVC rejects a lone surrogate written as \uXXXX even inside a raw string
// literal (error C3850), so the backslash is escaped to keep the \u escape
// out of the compiler's hands and deliver it to the testee's JSON parser.
TEST_P(JsonStringTest, StringFieldUnpairedHighSurrogate) {
  EXPECT_THAT(Testee(kP3)
                  .ParseJson(message(), "{\"optionalString\": \"\\uD800\"}")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonStringTest, StringFieldUnpairedLowSurrogate) {
  EXPECT_THAT(Testee(kP3)
                  .ParseJson(message(), "{\"optionalString\": \"\\uDC00\"}")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonStringTest, StringFieldSurrogateInWrongOrder) {
  EXPECT_THAT(
      Testee(kP3)
          .ParseJson(message(), "{\"optionalString\": \"\\uDE01\\uD83D\"}")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonStringTest, StringFieldNotAString) {
  EXPECT_THAT(
      Testee().ParseJson(message(), R"({"optionalString": 12345})").ParseOnly(),
      Yields(IsParseError()));
}

// http://www.rfc-editor.org/rfc/rfc7159.txt says strings have to use double
// quotes.
TEST_P(JsonStringTest, StringFieldSingleQuoteKey) {
  EXPECT_THAT(Testee(kP3)
                  .ParseJson(message(), R"({'optionalString': "Hello world!"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonStringTest, StringFieldSingleQuoteValue) {
  EXPECT_THAT(Testee(kP3)
                  .ParseJson(message(), R"({"optionalString": 'Hello world!'})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonStringTest, StringFieldSingleQuoteBoth) {
  EXPECT_THAT(Testee(kP3)
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
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonBytesTest, BytesFieldBase64Url) {
  constexpr absl::string_view kInput = R"({"optionalBytes": "-_"})";
  constexpr absl::string_view kExpected = R"(optional_bytes: "\xfb")";
  EXPECT_THAT(Testee(kP3).ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee(kP3).ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

INSTANTIATE_TEST_SUITE_P(All, JsonBytesTest, ValuesIn(AllTestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
