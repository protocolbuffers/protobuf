// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// JSON conformance tests for google.protobuf.Value, ListValue and NullValue
// fields: every JSON kind a Value accepts, repeated Values and ListValues, how
// a NullValue is serialized inside and outside a oneof, the non-finite numbers
// a Value cannot express, and lists and Structs nested 25 deep (must be
// accepted) and 200 deep (must be rejected).  This replaces the legacy
// BinaryAndJsonConformanceSuiteImpl<M>::RunJsonTestsForValue(), which ran for
// the proto3-style message types only; the test names and the requests sent to
// the testee are identical to the legacy ones.
//
// Like the legacy RunValidJsonTest(), each valid input is sent twice: once to
// be serialized as binary ("<name>.ProtobufOutput") and once as JSON
// ("<name>.JsonOutput").  ExpectParseFailureForJson() asked for JSON output
// under a name without an output suffix; RunValidJsonTestWithValidator()
// asked for JSON output and inspected the JSON text.
// ExpectSerializeFailureForJson() sent the serialized text-format message as
// binary input, categorized as a JSON test, under
// "ProtobufInput.<name>.JsonOutput"; the same bytes are built here with the
// wire-format builders.

#include <cstdint>
#include <limits>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "conformance/binary_test_util.h"
#include "conformance/binary_wireformat.h"
#include "conformance/conformance.pb.h"
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

using JsonValueTest = MessageTypeConformanceTest;

// Field numbers of the binary inputs of the serialize-failure tests: the
// test message's optional_value and google.protobuf.Value.number_value.
constexpr uint32_t kOptionalValue = 306;
constexpr uint32_t kValueNumberValue = 2;

TEST_P(JsonValueTest, ValueAcceptInteger) {
  constexpr absl::string_view kInput = R"({"optionalValue": 1})";
  constexpr absl::string_view kExpected = "optional_value: { number_value: 1}";
  EXPECT_THAT(Testee("ValueAcceptInteger")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("ValueAcceptInteger").ParseJson(message(), kInput).SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonValueTest, ValueAcceptFloat) {
  constexpr absl::string_view kInput = R"({"optionalValue": 1.5})";
  constexpr absl::string_view kExpected =
      "optional_value: { number_value: 1.5}";
  EXPECT_THAT(
      Testee("ValueAcceptFloat").ParseJson(message(), kInput).SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("ValueAcceptFloat").ParseJson(message(), kInput).SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonValueTest, ValueAcceptBool) {
  constexpr absl::string_view kInput = R"({"optionalValue": false})";
  constexpr absl::string_view kExpected =
      "optional_value: { bool_value: false}";
  EXPECT_THAT(
      Testee("ValueAcceptBool").ParseJson(message(), kInput).SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("ValueAcceptBool").ParseJson(message(), kInput).SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonValueTest, ValueAcceptNull) {
  constexpr absl::string_view kInput = R"({"optionalValue": null})";
  constexpr absl::string_view kExpected =
      "optional_value: { null_value: NULL_VALUE}";
  EXPECT_THAT(
      Testee("ValueAcceptNull").ParseJson(message(), kInput).SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("ValueAcceptNull").ParseJson(message(), kInput).SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonValueTest, ValueAcceptString) {
  constexpr absl::string_view kInput = R"({"optionalValue": "hello"})";
  constexpr absl::string_view kExpected =
      R"(optional_value: { string_value: "hello"})";
  EXPECT_THAT(Testee("ValueAcceptString")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("ValueAcceptString").ParseJson(message(), kInput).SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonValueTest, ValueAcceptList) {
  constexpr absl::string_view kInput = R"({"optionalValue": [0, "hello"]})";
  constexpr absl::string_view kExpected = R"(
        optional_value: {
          list_value: {
            values: {
              number_value: 0
      }
            values: {
              string_value: "hello"
      }
    }
  }
      )";
  EXPECT_THAT(
      Testee("ValueAcceptList").ParseJson(message(), kInput).SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("ValueAcceptList").ParseJson(message(), kInput).SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonValueTest, ValueAcceptObject) {
  constexpr absl::string_view kInput = R"({"optionalValue": {"value": 1}})";
  constexpr absl::string_view kExpected = R"(
        optional_value: {
          struct_value: {
            fields: {
              key: "value"
              value: {
                number_value: 1
        }
      }
    }
  }
      )";
  EXPECT_THAT(Testee("ValueAcceptObject")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("ValueAcceptObject").ParseJson(message(), kInput).SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonValueTest, RepeatedValue) {
  constexpr absl::string_view kInput = R"({
        "repeatedValue": [["a"]]
      })";
  constexpr absl::string_view kExpected = R"(
        repeated_value: [
  {
            list_value: {
              values: [
                { string_value: "a"}
        ]
      }
    }
  ]
      )";
  EXPECT_THAT(
      Testee("RepeatedValue").ParseJson(message(), kInput).SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("RepeatedValue").ParseJson(message(), kInput).SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonValueTest, RepeatedListValue) {
  constexpr absl::string_view kInput = R"({
        "repeatedListValue": [["a"]]
      })";
  constexpr absl::string_view kExpected = R"(
        repeated_list_value: [
  {
            values: [
              { string_value: "a"}
      ]
    }
  ]
      )";
  EXPECT_THAT(Testee("RepeatedListValue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("RepeatedListValue").ParseJson(message(), kInput).SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// A NullValue set in a oneof is emitted as `null`, whether it was given as
// the enum name or as `null`.
TEST_P(JsonValueTest, NullValueInOtherOneofOldFormat) {
  EXPECT_THAT(
      Testee(kP3, "NullValueInOtherOneofOldFormat")
          .ParseJson(message(), R"({"oneofNullValue": "NULL_VALUE"})")
          .SerializeJson(),
      Yields(JsonPayload(HasJsonMemberThat("oneofNullValue", IsJsonNull()))));
}

TEST_P(JsonValueTest, NullValueInOtherOneofNewFormat) {
  EXPECT_THAT(
      Testee(kP3, "NullValueInOtherOneofNewFormat")
          .ParseJson(message(), R"({"oneofNullValue": null})")
          .SerializeJson(),
      Yields(JsonPayload(HasJsonMemberThat("oneofNullValue", IsJsonNull()))));
}

// A NullValue field outside a oneof has no presence, so it is omitted.
TEST_P(JsonValueTest, NullValueInNormalMessage) {
  EXPECT_THAT(Testee(kP3, "NullValueInNormalMessage")
                  .ParseJson(message(), R"({"optionalNullValue": null})")
                  .SerializeJson(),
              Yields(JsonPayload(IsEmptyJsonObject())));
}

// The binary form of the legacy input `optional_value: { number_value: nan}`.
TEST_P(JsonValueTest, ValueRejectNanNumberValue) {
  EXPECT_THAT(
      Testee(kP3, "ValueRejectNanNumberValue")
          .ParseBinary(
              message(),
              LengthPrefixedField(
                  kOptionalValue,
                  DoubleField(kValueNumberValue,
                              std::numeric_limits<double>::quiet_NaN())))
          .OverrideTestCategory(::conformance::JSON_TEST)
          .SerializeJson(),
      Yields(IsSerializeError()));
}

// The binary form of the legacy input `optional_value: { number_value: inf}`.
TEST_P(JsonValueTest, ValueRejectInfNumberValue) {
  EXPECT_THAT(Testee(kP3, "ValueRejectInfNumberValue")
                  .ParseBinary(
                      message(),
                      LengthPrefixedField(
                          kOptionalValue,
                          DoubleField(kValueNumberValue,
                                      std::numeric_limits<double>::infinity())))
                  .OverrideTestCategory(::conformance::JSON_TEST)
                  .SerializeJson(),
              Yields(IsSerializeError()));
}

TEST_P(JsonValueTest, ListValueDeepNesting25) {
  const std::string input =
      absl::StrCat(R"({"optionalValue": )", std::string(25, '['), "1",
                   std::string(25, ']'), "}");
  const std::string expected = absl::StrCat("optional_value: {\n",
                                            Repeat("  list_value: {\n"
                                                   "    values: {\n",
                                                   25),
                                            "      number_value: 1\n",
                                            Repeat("    }\n"
                                                   "  }\n",
                                                   25),
                                            "}\n");
  EXPECT_THAT(Testee(kP3, "ListValueDeepNesting25")
                  .ParseJson(message(), input)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(expected))));
  EXPECT_THAT(Testee(kP3, "ListValueDeepNesting25")
                  .ParseJson(message(), input)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(expected))));
}

TEST_P(JsonValueTest, ListValueDeepNesting200) {
  const std::string input =
      absl::StrCat(R"({"optionalValue": )", std::string(200, '['), "1",
                   std::string(200, ']'), "}");
  EXPECT_THAT(Testee(kP3, "ListValueDeepNesting200")
                  .ParseJson(message(), input)
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonValueTest, ValueDeepNesting25) {
  const std::string input =
      absl::StrCat(R"({"optionalValue": {)", Repeat(R"("n": {)", 25),
                   R"("value": 1)", std::string(25, '}'), "}}");
  const std::string expected =
      absl::StrCat("optional_value: {\n", "  struct_value: {\n",
                   Repeat("    fields: {\n"
                          "      key: \"n\"\n"
                          "      value: {\n"
                          "        struct_value: {\n",
                          25),
                   "          fields: {\n"
                   "            key: \"value\"\n"
                   "            value: { number_value: 1 }\n"
                   "          }\n",
                   Repeat("        }\n"
                          "      }\n"
                          "    }\n",
                          25),
                   "  }\n", "}\n");
  EXPECT_THAT(Testee(kP3, "ValueDeepNesting25")
                  .ParseJson(message(), input)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(expected))));
  EXPECT_THAT(Testee(kP3, "ValueDeepNesting25")
                  .ParseJson(message(), input)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(expected))));
}

TEST_P(JsonValueTest, ValueDeepNesting200) {
  const std::string input =
      absl::StrCat(R"({"optionalValue": {)", Repeat(R"("n": {)", 200),
                   R"("value": 1)", std::string(200, '}'), "}}");
  EXPECT_THAT(Testee(kP3, "ValueDeepNesting200")
                  .ParseJson(message(), input)
                  .ParseOnly(),
              Yields(IsParseError()));
}

INSTANTIATE_TEST_SUITE_P(All, JsonValueTest, ValuesIn(Proto3TestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
