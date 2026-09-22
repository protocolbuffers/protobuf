// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Text-format conformance tests for string literals, run for a string and for
// a bytes field: concatenation, the escape sequences, the rejected line feeds
// and out-of-range or surrogate unicode escapes, and the invalid UTF-8 byte
// sequences that only string fields reject.  This replaces the
// "String literals x {Strings, Bytes}" section of the legacy
// TextFormatConformanceTestSuiteImpl<M>::RunAllTests(), which ran for the
// proto3-style message types only; the requests sent to the testee are
// identical to the legacy ones.

#include <ostream>
#include <string>
#include <tuple>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "conformance/binary_test_util.h"
#include "conformance/matchers.h"
#include "conformance/message_type_fixtures.h"
#include "conformance/test_environment.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::Combine;
using ::testing::ValuesIn;

// The field a string literal is tested with.  The legacy suite ran every
// literal test for the string field and for the bytes field, with "String" /
// "Bytes" appended to the test name.
struct StringField {
  absl::string_view type_name;   // "String" or "Bytes", as in the test names.
  absl::string_view field_name;  // The name of the field of that type.
};

constexpr StringField kStringFields[] = {
    {"String", "optional_string"},
    {"Bytes", "optional_bytes"},
};

std::string ParamName(const StringField& field) {
  return std::string(field.type_name);
}

// So a failing test prints the field by name instead of as raw bytes.
void PrintTo(const StringField& field, std::ostream* os) {
  *os << field.field_name;
}

// Parameterized over (test message type, string field).  Reporting the
// message type through MessageUnderTest() makes the base SetUp() skip the
// editions instance when --maximum_edition doesn't cover it.  Input() only
// builds values; every expectation is in the test bodies.
class TextStringLiteralTest : public ConformanceTest,
                              public testing::WithParamInterface<
                                  std::tuple<const Descriptor*, StringField>> {
 public:
  TestPriority DefaultPriority() const override { return kP3; }

 protected:
  const Descriptor* MessageUnderTest() const override { return message(); }

  const Descriptor* message() const { return std::get<0>(GetParam()); }
  const StringField& field() const { return std::get<1>(GetParam()); }

  // The text-format input setting the field to `literal`, e.g.
  // "optional_bytes: 'first' \"second\"".
  std::string Input(absl::string_view literal) const {
    return absl::StrCat(field().field_name, ": ", literal);
  }
};

TEST_P(TextStringLiteralTest, Concat) {
  const std::string input = Input("'first' \"second\"\n'third'");
  EXPECT_THAT(Testee(kP0).ParseText(message(), input).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(input))));
  EXPECT_THAT(Testee(kP0).ParseText(message(), input).SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(input))));
}

TEST_P(TextStringLiteralTest, BasicEscapes) {
  const std::string input = Input("'\\a\\b\\f\\n\\r\\t\\v\\?\\\\\\'\\\"'");
  EXPECT_THAT(Testee(kP0).ParseText(message(), input).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(input))));
  EXPECT_THAT(Testee(kP0).ParseText(message(), input).SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(input))));
}

TEST_P(TextStringLiteralTest, OctalEscapes) {
  const std::string input = Input("'\\341\\210\\264'");
  EXPECT_THAT(Testee(kP0).ParseText(message(), input).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(input))));
  EXPECT_THAT(Testee(kP0).ParseText(message(), input).SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(input))));
}

TEST_P(TextStringLiteralTest, HexEscapes) {
  const std::string input = Input("'\\xe1\\x88\\xb4'");
  EXPECT_THAT(Testee(kP0).ParseText(message(), input).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(input))));
  EXPECT_THAT(Testee(kP0).ParseText(message(), input).SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(input))));
}

TEST_P(TextStringLiteralTest, ShortUnicodeEscape) {
  const std::string input = Input("'\\u1234'");
  EXPECT_THAT(Testee().ParseText(message(), input).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(input))));
  EXPECT_THAT(Testee().ParseText(message(), input).SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(input))));
}

TEST_P(TextStringLiteralTest, LongUnicodeEscapes) {
  const std::string input = Input("'\\U00001234\\U00010437'");
  EXPECT_THAT(Testee().ParseText(message(), input).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(input))));
  EXPECT_THAT(Testee().ParseText(message(), input).SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(input))));
}

// String literals don't include line feeds.
TEST_P(TextStringLiteralTest, IncludesLF) {
  EXPECT_THAT(Testee(kP0)
                  .ParseText(message(), Input("'first line\nsecond line'"))
                  .ParseOnly(),
              Yields(IsParseError()));
}

// Unicode escapes don't include code points that lie beyond the planes
// (> 0x10ffff).
TEST_P(TextStringLiteralTest, LongUnicodeEscapeTooLarge) {
  EXPECT_THAT(
      Testee(kP0).ParseText(message(), Input("'\\U00110000'")).ParseOnly(),
      Yields(IsParseError()));
}

// Unicode escapes don't include surrogates.
TEST_P(TextStringLiteralTest, ShortUnicodeEscapeSurrogatePair) {
  EXPECT_THAT(
      Testee().ParseText(message(), Input("'\\ud801\\udc37'")).ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(TextStringLiteralTest, ShortUnicodeEscapeSurrogateFirstOnly) {
  EXPECT_THAT(Testee().ParseText(message(), Input("'\\ud800'")).ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(TextStringLiteralTest, ShortUnicodeEscapeSurrogateSecondOnly) {
  EXPECT_THAT(Testee().ParseText(message(), Input("'\\udc00'")).ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(TextStringLiteralTest, LongUnicodeEscapeSurrogateFirstOnly) {
  EXPECT_THAT(Testee().ParseText(message(), Input("'\\U0000d800'")).ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(TextStringLiteralTest, LongUnicodeEscapeSurrogateSecondOnly) {
  EXPECT_THAT(Testee().ParseText(message(), Input("'\\U0000dc00'")).ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(TextStringLiteralTest, LongUnicodeEscapeSurrogatePair) {
  EXPECT_THAT(Testee()
                  .ParseText(message(), Input("'\\U0000d801\\U00000dc37'"))
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(TextStringLiteralTest, UnicodeEscapeSurrogatePairLongShort) {
  EXPECT_THAT(
      Testee().ParseText(message(), Input("'\\U0000d801\\udc37'")).ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(TextStringLiteralTest, UnicodeEscapeSurrogatePairShortLong) {
  EXPECT_THAT(
      Testee().ParseText(message(), Input("'\\ud801\\U0000dc37'")).ParseOnly(),
      Yields(IsParseError()));
}

INSTANTIATE_TEST_SUITE_P(All, TextStringLiteralTest,
                         Combine(ValuesIn(Proto3TestMessageTypes()),
                                 ValuesIn(kStringFields)),
                         TupleParamName<TextStringLiteralTest::ParamType>);

// String fields reject invalid UTF-8 byte sequences; bytes fields don't.
using TextStringFieldTest = MessageTypeConformanceTest;

TEST_P(TextStringFieldTest, BadUtf8Octal) {
  EXPECT_THAT(
      Testee().ParseText(message(), "optional_string: '\\300'").ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(TextStringFieldTest, BadUtf8Hex) {
  EXPECT_THAT(
      Testee().ParseText(message(), "optional_string: '\\xc0'").ParseOnly(),
      Yields(IsParseError()));
}

INSTANTIATE_TEST_SUITE_P(All, TextStringFieldTest,
                         ValuesIn(Proto3TestMessageTypes()),
                         MessageTypeParamName);

using TextBytesFieldTest = MessageTypeConformanceTest;

TEST_P(TextBytesFieldTest, BadUtf8Octal) {
  EXPECT_THAT(
      Testee()
          .ParseText(message(), "optional_bytes: '\\300'")
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto("optional_bytes: '\\300'"))));
  EXPECT_THAT(
      Testee().ParseText(message(), "optional_bytes: '\\300'").SerializeText(),
      Yields(ParsedPayload(EqualsTextProto("optional_bytes: '\\300'"))));
}

TEST_P(TextBytesFieldTest, BadUtf8Hex) {
  EXPECT_THAT(
      Testee()
          .ParseText(message(), "optional_bytes: '\\xc0'")
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto("optional_bytes: '\\xc0'"))));
  EXPECT_THAT(
      Testee().ParseText(message(), "optional_bytes: '\\xc0'").SerializeText(),
      Yields(ParsedPayload(EqualsTextProto("optional_bytes: '\\xc0'"))));
}

INSTANTIATE_TEST_SUITE_P(All, TextBytesFieldTest,
                         ValuesIn(Proto3TestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
