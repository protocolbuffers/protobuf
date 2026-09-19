// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Text-format conformance tests for the separators of the syntax: the
// optional "," / ";" after a field, and the "," between the values of a list.
// This replaces the "Separators" block of the legacy
// TextFormatConformanceTestSuiteImpl<M>::RunAllTests(), which ran for the
// proto3-style message types only; the test names and the requests sent to
// the testee are identical to the legacy ones.

#include <string>
#include <tuple>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "binary_test_util.h"
#include "matchers.h"
#include "message_type_fixtures.h"
#include "test_environment.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::Combine;
using ::testing::Values;
using ::testing::ValuesIn;

// A field type the separator tests run over: the legacy suite ran every
// separator test once per entry, with "_<type>" appended to the test name and
// `value` as the literal of the optional_<type> / repeated_<type> field.
struct SeparatorCase {
  absl::string_view type;
  absl::string_view value;
  // Whether two adjacent values of the type are one value to the parser
  // (string literals concatenate) rather than a missing list separator.
  bool adjacent_values_concatenate;
};

constexpr SeparatorCase kSeparatorCases[] = {
    {"string", "\"abc\"", true}, {"bytes", "\"abc\"", true},
    {"int32", "123", false},     {"bool", "true", false},
    {"double", "1.23", false},   {"fixed32", "0x123", false},
};

std::string ParamName(const SeparatorCase& test_case) {
  // Qualified: the overloads in this namespace hide the outer ones.
  return conformance::ParamName(test_case.type);
}

// Whether the field separator tests use the optional_<type> field with a
// bare value ("Single") or the repeated_<type> field with a one-element list
// ("Repeated"); the legacy test names carry the same two words.
enum class FieldCardinality { kSingle, kRepeated };

std::string ParamName(FieldCardinality cardinality) {
  return cardinality == FieldCardinality::kSingle ? "Single" : "Repeated";
}

// The separators after a top-level field, parameterized over (test message
// type, field type, single/repeated).  Reporting the message type through
// MessageUnderTest() makes the base SetUp() skip the editions instance when
// --maximum_edition doesn't cover it.  TestName() and Input() only build
// values; every expectation is in the test bodies.
class TextFieldSeparatorTest
    : public ConformanceTest,
      public testing::WithParamInterface<
          std::tuple<const Descriptor*, SeparatorCase, FieldCardinality>> {
 protected:
  const Descriptor* MessageUnderTest() const override { return message(); }

  const Descriptor* message() const { return std::get<0>(GetParam()); }
  const SeparatorCase& test_case() const { return std::get<1>(GetParam()); }
  FieldCardinality cardinality() const { return std::get<2>(GetParam()); }

  // The legacy test name, e.g. "FieldSeparatorCommaTopLevelSingle_int32".
  std::string TestName(absl::string_view base) const {
    return absl::StrCat(base, ParamName(cardinality()), "_", test_case().type);
  }

  // The field set to its value, followed by `separator`, e.g.
  // "optional_int32: 123," or "repeated_int32: [123],".
  std::string Input(absl::string_view separator) const {
    const bool single = cardinality() == FieldCardinality::kSingle;
    return absl::StrCat(single ? "optional_" : "repeated_", test_case().type,
                        ": ", single ? "" : "[", test_case().value,
                        single ? "" : "]", separator);
  }
};

TEST_P(TextFieldSeparatorTest, Comma) {
  const std::string input = Input(",");
  EXPECT_THAT(RequiredTest(TestName("FieldSeparatorCommaTopLevel"))
                  .ParseText(message(), input)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(input))));
  EXPECT_THAT(RequiredTest(TestName("FieldSeparatorCommaTopLevel"))
                  .ParseText(message(), input)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(input))));
}

// Note: the legacy name has "Single" in it even for the repeated variant.
TEST_P(TextFieldSeparatorTest, Semi) {
  const std::string input = Input(";");
  EXPECT_THAT(RequiredTest(TestName("FieldSeparatorSemiTopLevelSingle"))
                  .ParseText(message(), input)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(input))));
  EXPECT_THAT(RequiredTest(TestName("FieldSeparatorSemiTopLevelSingle"))
                  .ParseText(message(), input)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(input))));
}

TEST_P(TextFieldSeparatorTest, CommaDuplicatesFails) {
  EXPECT_THAT(
      RequiredTest(TestName("FieldSeparatorCommaTopLevelDuplicatesFails"))
          .ParseText(message(), Input(",,"))
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(TextFieldSeparatorTest, SemiDuplicateFails) {
  EXPECT_THAT(RequiredTest(TestName("FieldSeparatorSemiTopLevelDuplicateFails"))
                  .ParseText(message(), Input(";;"))
                  .ParseOnly(),
              Yields(IsParseError()));
}

INSTANTIATE_TEST_SUITE_P(
    All, TextFieldSeparatorTest,
    Combine(ValuesIn(Proto3TestMessageTypes()), ValuesIn(kSeparatorCases),
            Values(FieldCardinality::kSingle, FieldCardinality::kRepeated)),
    TupleParamName<TextFieldSeparatorTest::ParamType>);

// The required "," between the values of a list, parameterized over (test
// message type, field type).
class TextListSeparatorTest
    : public ConformanceTest,
      public testing::WithParamInterface<
          std::tuple<const Descriptor*, SeparatorCase>> {
 protected:
  const Descriptor* MessageUnderTest() const override { return message(); }

  const Descriptor* message() const { return std::get<0>(GetParam()); }
  const SeparatorCase& test_case() const { return std::get<1>(GetParam()); }

  // The legacy test name, e.g. "ListSeparator_int32".
  std::string TestName(absl::string_view base) const {
    return absl::StrCat(base, "_", test_case().type);
  }

  // The repeated field set to a list whose body is `list_body`, e.g.
  // "repeated_int32: [123,123]".
  std::string Input(absl::string_view list_body) const {
    return absl::StrCat("repeated_", test_case().type, ": [", list_body, "]");
  }

  absl::string_view value() const { return test_case().value; }
};

TEST_P(TextListSeparatorTest, Comma) {
  const std::string input = Input(absl::StrCat(value(), ",", value()));
  EXPECT_THAT(RequiredTest(TestName("ListSeparator"))
                  .ParseText(message(), input)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(input))));
  EXPECT_THAT(RequiredTest(TestName("ListSeparator"))
                  .ParseText(message(), input)
                  .SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(input))));
}

TEST_P(TextListSeparatorTest, SemiFails) {
  EXPECT_THAT(
      RequiredTest(TestName("ListSeparatorSemiFails"))
          .ParseText(message(), Input(absl::StrCat(value(), ";", value())))
          .ParseOnly(),
      Yields(IsParseError()));
}

// For string and bytes, if we skip the separator, the parser will treat the
// two values as a single value.
TEST_P(TextListSeparatorTest, Missing) {
  const std::string input = Input(absl::StrCat(value(), " ", value()));
  if (test_case().adjacent_values_concatenate) {
    EXPECT_THAT(RequiredTest(TestName("ListSeparatorMissingIsOneValue"))
                    .ParseText(message(), input)
                    .SerializeBinary(),
                Yields(ParsedPayload(EqualsTextProto(input))));
    EXPECT_THAT(RequiredTest(TestName("ListSeparatorMissingIsOneValue"))
                    .ParseText(message(), input)
                    .SerializeText(),
                Yields(ParsedPayload(EqualsTextProto(input))));
  } else {
    EXPECT_THAT(RequiredTest(TestName("ListSeparatorMissingFails"))
                    .ParseText(message(), input)
                    .ParseOnly(),
                Yields(IsParseError()));
  }
}

TEST_P(TextListSeparatorTest, DuplicateFails) {
  EXPECT_THAT(
      RequiredTest(TestName("ListSeparatorDuplicateFails"))
          .ParseText(message(), Input(absl::StrCat(value(), ",,", value())))
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(TextListSeparatorTest, SingleTrailingFails) {
  EXPECT_THAT(RequiredTest(TestName("ListSeparatorSingleTrailingFails"))
                  .ParseText(message(), Input(absl::StrCat(value(), ",")))
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(TextListSeparatorTest, TwoValuesTrailingFails) {
  EXPECT_THAT(
      RequiredTest(TestName("ListSeparatorTwoValuesTrailingFails"))
          .ParseText(message(), Input(absl::StrCat(value(), ",", value(), ",")))
          .ParseOnly(),
      Yields(IsParseError()));
}

INSTANTIATE_TEST_SUITE_P(All, TextListSeparatorTest,
                         Combine(ValuesIn(Proto3TestMessageTypes()),
                                 ValuesIn(kSeparatorCases)),
                         TupleParamName<TextListSeparatorTest::ParamType>);

// The test messages don't have every type nested, so just check one data
// type for the nested field separator support.
using TextNestedSeparatorTest = MessageTypeConformanceTest;

TEST_P(TextNestedSeparatorTest, Comma) {
  EXPECT_THAT(RequiredTest("FieldSeparatorCommaNested")
                  .ParseText(message(), "optional_nested_message: { a: 123, }")
                  .SerializeBinary(),
              Yields(ParsedPayload(
                  EqualsTextProto("optional_nested_message: { a: 123, }"))));
  EXPECT_THAT(RequiredTest("FieldSeparatorCommaNested")
                  .ParseText(message(), "optional_nested_message: { a: 123, }")
                  .SerializeText(),
              Yields(ParsedPayload(
                  EqualsTextProto("optional_nested_message: { a: 123, }"))));
}

TEST_P(TextNestedSeparatorTest, Semi) {
  EXPECT_THAT(RequiredTest("FieldSeparatorSemiNested")
                  .ParseText(message(), "optional_nested_message: { a: 123; }")
                  .SerializeBinary(),
              Yields(ParsedPayload(
                  EqualsTextProto("optional_nested_message: { a: 123; }"))));
  EXPECT_THAT(RequiredTest("FieldSeparatorSemiNested")
                  .ParseText(message(), "optional_nested_message: { a: 123; }")
                  .SerializeText(),
              Yields(ParsedPayload(
                  EqualsTextProto("optional_nested_message: { a: 123; }"))));
}

TEST_P(TextNestedSeparatorTest, CommaDuplicates) {
  EXPECT_THAT(RequiredTest("FieldSeparatorCommaNestedDuplicates")
                  .ParseText(message(), "optional_nested_message: { a: 123,, }")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(TextNestedSeparatorTest, SemiDuplicates) {
  EXPECT_THAT(RequiredTest("FieldSeparatorSemiNestedDuplicates")
                  .ParseText(message(), "optional_nested_message: { a: 123;; }")
                  .ParseOnly(),
              Yields(IsParseError()));
}

INSTANTIATE_TEST_SUITE_P(All, TextNestedSeparatorTest,
                         ValuesIn(Proto3TestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
