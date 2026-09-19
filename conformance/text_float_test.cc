// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Text-format conformance tests for float and double literals: the decimal
// forms with and without the "f"/"F" type suffix, the case-insensitive special
// values, the rejected hex and octal forms, and the literals that overflow or
// underflow the field's range.  This replaces the floating point section of
// the legacy TextFormatConformanceTestSuiteImpl<M>::RunAllTests(), which ran
// for the proto3-style message types only; the test names and the requests
// sent to the testee are identical to the legacy ones.

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
using ::testing::Values;
using ::testing::ValuesIn;

// The type suffix a float literal is tested with.  The legacy suite ran every
// literal test three times: bare, and with the "f" and "F" suffixes, the
// latter two with "_f" / "_F" appended to the test name.
struct FloatSuffix {
  absl::string_view suffix;
  // The component of the gtest parameter name (ParamName() requires a
  // non-empty one, so the bare case can't just use `suffix`).
  absl::string_view param_name;
};

constexpr FloatSuffix kFloatSuffixes[] = {
    {"", "NoSuffix"},
    {"f", "f"},
    {"F", "F"},
};

std::string ParamName(const FloatSuffix& suffix) {
  return std::string(suffix.param_name);
}

// So a failing test prints the suffix by name instead of as raw bytes.
void PrintTo(const FloatSuffix& suffix, std::ostream* os) {
  *os << suffix.param_name;
}

// Parameterized over (test message type, float suffix).  Reporting the
// message type through MessageUnderTest() makes the base SetUp() skip the
// editions instance when --maximum_edition doesn't cover it.  TestName() and
// Input() only build values; every expectation is in the test bodies.
class TextFloatLiteralTest : public ConformanceTest,
                             public testing::WithParamInterface<
                                 std::tuple<const Descriptor*, FloatSuffix>> {
 protected:
  const Descriptor* MessageUnderTest() const override { return message(); }

  const Descriptor* message() const { return std::get<0>(GetParam()); }
  absl::string_view suffix() const { return std::get<1>(GetParam()).suffix; }

  // The legacy test name: `base` as is for the bare literal, otherwise
  // "<base>_<suffix>", e.g. "FloatFieldMaxValue_f".
  std::string TestName(absl::string_view base) const {
    return suffix().empty() ? std::string(base)
                            : absl::StrCat(base, "_", suffix());
  }

  // The text-format input setting optional_float to `literal` with the
  // suffix appended, e.g. "optional_float: 3.192837f".
  std::string Input(absl::string_view literal) const {
    return absl::StrCat("optional_float: ", literal, suffix());
  }
};

TEST_P(TextFloatLiteralTest, Positive) {
  const std::string input = Input("3.192837");
  EXPECT_THAT(Testee(TestName("FloatField"))
                  .ParseText(message(), input)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(input))));
  EXPECT_THAT(Testee(TestName("FloatField"))
                  .ParseText(message(), input)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(input))));
}

// The field has implicit presence, so zero means unset.
TEST_P(TextFloatLiteralTest, Zero) {
  EXPECT_THAT(Testee(TestName("FloatFieldZero"))
                  .ParseText(message(), Input("0"))
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(""))));
  EXPECT_THAT(Testee(TestName("FloatFieldZero"))
                  .ParseText(message(), Input("0"))
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(""))));
}

TEST_P(TextFloatLiteralTest, Negative) {
  const std::string input = Input("-3.192837");
  EXPECT_THAT(Testee(TestName("FloatFieldNegative"))
                  .ParseText(message(), input)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(input))));
  EXPECT_THAT(Testee(TestName("FloatFieldNegative"))
                  .ParseText(message(), input)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(input))));
}

TEST_P(TextFloatLiteralTest, VeryPreciseNumber) {
  const std::string input = Input("3.123456789123456789");
  EXPECT_THAT(Testee(TestName("FloatFieldWithVeryPreciseNumber"))
                  .ParseText(message(), input)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(input))));
  EXPECT_THAT(Testee(TestName("FloatFieldWithVeryPreciseNumber"))
                  .ParseText(message(), input)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(input))));
}

TEST_P(TextFloatLiteralTest, MaxValue) {
  const std::string input = Input("3.4028235e+38");
  EXPECT_THAT(Testee(TestName("FloatFieldMaxValue"))
                  .ParseText(message(), input)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(input))));
  EXPECT_THAT(Testee(TestName("FloatFieldMaxValue"))
                  .ParseText(message(), input)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(input))));
}

TEST_P(TextFloatLiteralTest, MinValue) {
  const std::string input = Input("1.17549e-38");
  EXPECT_THAT(Testee(TestName("FloatFieldMinValue"))
                  .ParseText(message(), input)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(input))));
  EXPECT_THAT(Testee(TestName("FloatFieldMinValue"))
                  .ParseText(message(), input)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(input))));
}

TEST_P(TextFloatLiteralTest, WithInt32Max) {
  const std::string input = Input("4294967296");
  EXPECT_THAT(Testee(TestName("FloatFieldWithInt32Max"))
                  .ParseText(message(), input)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(input))));
  EXPECT_THAT(Testee(TestName("FloatFieldWithInt32Max"))
                  .ParseText(message(), input)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(input))));
}

TEST_P(TextFloatLiteralTest, LargerThanInt64) {
  const std::string input = Input("9223372036854775808");
  EXPECT_THAT(Testee(TestName("FloatFieldLargerThanInt64"))
                  .ParseText(message(), input)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(input))));
  EXPECT_THAT(Testee(TestName("FloatFieldLargerThanInt64"))
                  .ParseText(message(), input)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(input))));
}

TEST_P(TextFloatLiteralTest, TooLarge) {
  const std::string input = Input("3.4028235e+39");
  EXPECT_THAT(Testee(TestName("FloatFieldTooLarge"))
                  .ParseText(message(), input)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(input))));
  EXPECT_THAT(Testee(TestName("FloatFieldTooLarge"))
                  .ParseText(message(), input)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(input))));
}

TEST_P(TextFloatLiteralTest, TooSmall) {
  const std::string input = Input("1.17549e-39");
  EXPECT_THAT(Testee(TestName("FloatFieldTooSmall"))
                  .ParseText(message(), input)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(input))));
  EXPECT_THAT(Testee(TestName("FloatFieldTooSmall"))
                  .ParseText(message(), input)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(input))));
}

TEST_P(TextFloatLiteralTest, LargerThanUint64) {
  const std::string input = Input("18446744073709551616");
  EXPECT_THAT(Testee(TestName("FloatFieldLargerThanUint64"))
                  .ParseText(message(), input)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(input))));
  EXPECT_THAT(Testee(TestName("FloatFieldLargerThanUint64"))
                  .ParseText(message(), input)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(input))));
}

// https://protobuf.dev/reference/protobuf/textformat-spec/#literals says
// "-0" is a valid float literal. -0 should be considered not the same as 0
// when considering implicit presence, and so should round trip.
TEST_P(TextFloatLiteralTest, NegativeZero) {
  const std::string input = Input("-0");
  EXPECT_THAT(Testee(TestName("FloatFieldNegativeZero"))
                  .ParseText(message(), input)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(input))));
  EXPECT_THAT(Testee(TestName("FloatFieldNegativeZero"))
                  .ParseText(message(), input)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(input))));
}

// https://protobuf.dev/reference/protobuf/textformat-spec/#literals says
// ".123", "-.123", ".123e2" are a valid float literal.
TEST_P(TextFloatLiteralTest, NoLeadingZero) {
  const std::string input = Input(".123");
  EXPECT_THAT(Testee(TestName("FloatFieldNoLeadingZero"))
                  .ParseText(message(), input)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(input))));
  EXPECT_THAT(Testee(TestName("FloatFieldNoLeadingZero"))
                  .ParseText(message(), input)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(input))));
}

TEST_P(TextFloatLiteralTest, NegativeNoLeadingZero) {
  const std::string input = Input("-.123");
  EXPECT_THAT(Testee(TestName("FloatFieldNegativeNoLeadingZero"))
                  .ParseText(message(), input)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(input))));
  EXPECT_THAT(Testee(TestName("FloatFieldNegativeNoLeadingZero"))
                  .ParseText(message(), input)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(input))));
}

TEST_P(TextFloatLiteralTest, NoLeadingZeroWithExponent) {
  const std::string input = Input(".123e2");
  EXPECT_THAT(Testee(TestName("FloatFieldNoLeadingZeroWithExponent"))
                  .ParseText(message(), input)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(input))));
  EXPECT_THAT(Testee(TestName("FloatFieldNoLeadingZeroWithExponent"))
                  .ParseText(message(), input)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(input))));
}

INSTANTIATE_TEST_SUITE_P(All, TextFloatLiteralTest,
                         Combine(ValuesIn(Proto3TestMessageTypes()),
                                 ValuesIn(kFloatSuffixes)),
                         TupleParamName<TextFloatLiteralTest::ParamType>);

// https://protobuf.dev/reference/protobuf/textformat-spec/#value say case
// doesn't matter for special values, test a few.
//
// Parameterized over (test message type, spelling of the special value); the
// two suites below only differ in the spellings they are instantiated with.
class TextFloatSpecialValueTest
    : public ConformanceTest,
      public testing::WithParamInterface<
          std::tuple<const Descriptor*, absl::string_view>> {
 protected:
  const Descriptor* MessageUnderTest() const override { return message(); }

  const Descriptor* message() const { return std::get<0>(GetParam()); }
  absl::string_view spelling() const { return std::get<1>(GetParam()); }
};

using TextFloatNanTest = TextFloatSpecialValueTest;

TEST_P(TextFloatNanTest, Spelling) {
  const std::string input = absl::StrCat("optional_float: ", spelling());
  EXPECT_THAT(Testee(absl::StrCat("FloatFieldValue_", spelling()))
                  .ParseText(message(), input)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(input))));
  EXPECT_THAT(Testee(absl::StrCat("FloatFieldValue_", spelling()))
                  .ParseText(message(), input)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(input))));
}

INSTANTIATE_TEST_SUITE_P(All, TextFloatNanTest,
                         Combine(ValuesIn(Proto3TestMessageTypes()),
                                 Values("nan", "NaN", "nAn")),
                         TupleParamName<TextFloatNanTest::ParamType>);

using TextFloatInfinityTest = TextFloatSpecialValueTest;

TEST_P(TextFloatInfinityTest, Positive) {
  const std::string input = absl::StrCat("optional_float: ", spelling());
  EXPECT_THAT(Testee(absl::StrCat("FloatFieldValue_Pos", spelling()))
                  .ParseText(message(), input)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(input))));
  EXPECT_THAT(Testee(absl::StrCat("FloatFieldValue_Pos", spelling()))
                  .ParseText(message(), input)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(input))));
}

TEST_P(TextFloatInfinityTest, Negative) {
  const std::string input = absl::StrCat("optional_float: -", spelling());
  EXPECT_THAT(Testee(absl::StrCat("FloatFieldValue_Neg", spelling()))
                  .ParseText(message(), input)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(input))));
  EXPECT_THAT(Testee(absl::StrCat("FloatFieldValue_Neg", spelling()))
                  .ParseText(message(), input)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(input))));
}

INSTANTIATE_TEST_SUITE_P(All, TextFloatInfinityTest,
                         Combine(ValuesIn(Proto3TestMessageTypes()),
                                 Values("inf", "infinity", "INF", "INFINITY",
                                        "iNF", "inFINITY")),
                         TupleParamName<TextFloatInfinityTest::ParamType>);

// The remaining float tests, and their double counterparts, run once per
// message type.
using TextFloatTest = MessageTypeConformanceTest;

// https://protobuf.dev/reference/protobuf/textformat-spec/#numeric and
// https://protobuf.dev/reference/protobuf/textformat-spec/#value says
// hex or octal float literals are invalid.
TEST_P(TextFloatTest, NoHex) {
  EXPECT_THAT(Testee("FloatFieldNoHex")
                  .ParseText(message(), "optional_float: 0x1")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(TextFloatTest, NoNegativeHex) {
  EXPECT_THAT(Testee("FloatFieldNoNegativeHex")
                  .ParseText(message(), "optional_float: -0x1")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(TextFloatTest, NoOctal) {
  EXPECT_THAT(Testee("FloatFieldNoOctal")
                  .ParseText(message(), "optional_float: 012")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(TextFloatTest, NoNegativeOctal) {
  EXPECT_THAT(Testee("FloatFieldNoNegativeOctal")
                  .ParseText(message(), "optional_float: -012")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// https://protobuf.dev/reference/protobuf/textformat-spec/#value says
// overflows are mapped to infinity/-infinity.
TEST_P(TextFloatTest, OverflowInfinity) {
  EXPECT_THAT(Testee("FloatFieldOverflowInfinity")
                  .ParseText(message(), "optional_float: 1e50")
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto("optional_float: inf"))));
  EXPECT_THAT(Testee("FloatFieldOverflowInfinity")
                  .ParseText(message(), "optional_float: 1e50")
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto("optional_float: inf"))));
}

TEST_P(TextFloatTest, OverflowNegativeInfinity) {
  EXPECT_THAT(Testee("FloatFieldOverflowNegativeInfinity")
                  .ParseText(message(), "optional_float: -1e50")
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto("optional_float: -inf"))));
  EXPECT_THAT(Testee("FloatFieldOverflowNegativeInfinity")
                  .ParseText(message(), "optional_float: -1e50")
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto("optional_float: -inf"))));
}

// Exponent is one more than uint64 max.
TEST_P(TextFloatTest, OverflowInfinityHugeExponent) {
  EXPECT_THAT(
      Testee("FloatFieldOverflowInfinityHugeExponent")
          .ParseText(message(), "optional_float: 1e18446744073709551616")
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto("optional_float: inf"))));
  EXPECT_THAT(
      Testee("FloatFieldOverflowInfinityHugeExponent")
          .ParseText(message(), "optional_float: 1e18446744073709551616")
          .SerializeText(),
      Yields(ParsedPayload(EqualsTextProto("optional_float: inf"))));
}

TEST_P(TextFloatTest, LargeNegativeExponentParsesAsZero) {
  EXPECT_THAT(Testee("FloatFieldLargeNegativeExponentParsesAsZero")
                  .ParseText(message(), "optional_float: 1e-50")
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(""))));
  EXPECT_THAT(Testee("FloatFieldLargeNegativeExponentParsesAsZero")
                  .ParseText(message(), "optional_float: 1e-50")
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(""))));
}

TEST_P(TextFloatTest, NegativeLargeNegativeExponentParsesAsNegativeZero) {
  EXPECT_THAT(Testee("NegFloatFieldLargeNegativeExponentParsesAsNegZero")
                  .ParseText(message(), "optional_float: -1e-50")
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto("optional_float: -0"))));
  EXPECT_THAT(Testee("NegFloatFieldLargeNegativeExponentParsesAsNegZero")
                  .ParseText(message(), "optional_float: -1e-50")
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto("optional_float: -0"))));
}

INSTANTIATE_TEST_SUITE_P(All, TextFloatTest, ValuesIn(Proto3TestMessageTypes()),
                         MessageTypeParamName);

using TextDoubleTest = MessageTypeConformanceTest;

// https://protobuf.dev/reference/protobuf/textformat-spec/#value says
// overflows are mapped to infinity/-infinity.
TEST_P(TextDoubleTest, OverflowInfinity) {
  EXPECT_THAT(Testee("DoubleFieldOverflowInfinity")
                  .ParseText(message(), "optional_double: 1e9999")
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto("optional_double: inf"))));
  EXPECT_THAT(Testee("DoubleFieldOverflowInfinity")
                  .ParseText(message(), "optional_double: 1e9999")
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto("optional_double: inf"))));
}

TEST_P(TextDoubleTest, OverflowNegativeInfinity) {
  EXPECT_THAT(Testee("DoubleFieldOverflowNegativeInfinity")
                  .ParseText(message(), "optional_double: -1e9999")
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto("optional_double: -inf"))));
  EXPECT_THAT(Testee("DoubleFieldOverflowNegativeInfinity")
                  .ParseText(message(), "optional_double: -1e9999")
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto("optional_double: -inf"))));
}

// Exponent is one more than uint64 max.
TEST_P(TextDoubleTest, OverflowInfinityHugeExponent) {
  EXPECT_THAT(
      Testee("DoubleFieldOverflowInfinityHugeExponent")
          .ParseText(message(), "optional_double: 1e18446744073709551616")
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto("optional_double: inf"))));
  EXPECT_THAT(
      Testee("DoubleFieldOverflowInfinityHugeExponent")
          .ParseText(message(), "optional_double: 1e18446744073709551616")
          .SerializeText(),
      Yields(ParsedPayload(EqualsTextProto("optional_double: inf"))));
}

TEST_P(TextDoubleTest, LargeNegativeExponentParsesAsZero) {
  EXPECT_THAT(
      Testee("DoubleFieldLargeNegativeExponentParsesAsZero")
          .ParseText(message(), "optional_double: 1e-18446744073709551616")
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(""))));
  EXPECT_THAT(
      Testee("DoubleFieldLargeNegativeExponentParsesAsZero")
          .ParseText(message(), "optional_double: 1e-18446744073709551616")
          .SerializeText(),
      Yields(ParsedPayload(EqualsTextProto(""))));
}

TEST_P(TextDoubleTest, NegativeLargeNegativeExponentParsesAsNegativeZero) {
  EXPECT_THAT(
      Testee("NegDoubleFieldLargeNegativeExponentParsesAsNegZero")
          .ParseText(message(), "optional_double: -1e-18446744073709551616")
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto("optional_double: -0"))));
  EXPECT_THAT(
      Testee("NegDoubleFieldLargeNegativeExponentParsesAsNegZero")
          .ParseText(message(), "optional_double: -1e-18446744073709551616")
          .SerializeText(),
      Yields(ParsedPayload(EqualsTextProto("optional_double: -0"))));
}

INSTANTIATE_TEST_SUITE_P(All, TextDoubleTest,
                         ValuesIn(Proto3TestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
