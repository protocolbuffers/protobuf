// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// JSON conformance tests about field names and object syntax: the
// lowerCamelCase / original-name conventions, escaped and unquoted names,
// trailing commas, comments, whitespace, duplicate members, and whether
// default values of proto2 and proto3 fields are serialized.  This replaces
// the legacy BinaryAndJsonConformanceSuiteImpl<M>::
// RunJsonTestsForFieldNameConvention() and
// RunJsonTestsForStoresDefaultPrimitive(); the test names and the requests
// sent to the testee are identical to the legacy ones.
//
// Like the legacy RunValidJsonTest(), each valid input is sent twice: once to
// be serialized as binary ("<name>.ProtobufOutput") and once as JSON
// ("<name>.JsonOutput").  The legacy "validators"
// (RunValidJsonTestWithValidator()) looked at the serialized JSON text itself
// and are the tests that only serialize as JSON (FieldNameInLowerCamelCase,
// FieldNameWith*Validator, StoresDefaultPrimitive, ...).
// RunValidJsonTestOrParseFailure() asked for binary output under a name
// without an output suffix.

#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "conformance/binary_test_util.h"
#include "conformance/conformance.pb.h"
#include "conformance/json_test_util.h"
#include "conformance/matchers.h"
#include "conformance/message_type_fixtures.h"
#include "conformance/test_environment.h"
#include "conformance/testee.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::AllOf;
using ::testing::AnyOf;
using ::testing::Not;
using ::testing::ValuesIn;

// ---------------------------------------------------------------------------
// Parsing field names in either convention.
// ---------------------------------------------------------------------------

using JsonFieldNameTest = MessageTypeConformanceTest;

TEST_P(JsonFieldNameTest, FieldNameInSnakeCase) {
  constexpr absl::string_view kInput = R"({
        "fieldname1": 1,
        "fieldName2": 2,
        "FieldName3": 3,
        "fieldName4": 4
      })";
  constexpr absl::string_view kExpected = R"(
        fieldname1: 1
        field_name2: 2
        _field_name3: 3
        field__name4_: 4
      )";
  EXPECT_THAT(Testee("FieldNameInSnakeCase")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("FieldNameInSnakeCase")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonFieldNameTest, FieldNameWithNumbers) {
  constexpr absl::string_view kInput = R"({
        "field0name5": 5,
        "field0Name6": 6
      })";
  constexpr absl::string_view kExpected = R"(
        field0name5: 5
        field_0_name6: 6
      )";
  EXPECT_THAT(Testee("FieldNameWithNumbers")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("FieldNameWithNumbers")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonFieldNameTest, FieldNameWithMixedCases) {
  constexpr absl::string_view kInput = R"({
        "fieldName7": 7,
        "FieldName8": 8,
        "fieldName9": 9,
        "FieldName10": 10,
        "FIELDNAME11": 11,
        "FIELDName12": 12
      })";
  constexpr absl::string_view kExpected = R"(
        fieldName7: 7
        FieldName8: 8
        field_Name9: 9
        Field_Name10: 10
        FIELD_NAME11: 11
        FIELD_name12: 12
      )";
  EXPECT_THAT(Testee("FieldNameWithMixedCases")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("FieldNameWithMixedCases")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonFieldNameTest, FieldNameWithDoubleUnderscores) {
  constexpr absl::string_view kInput = R"({
        "FieldName13": 13,
        "FieldName14": 14,
        "fieldName15": 15,
        "fieldName16": 16,
        "fieldName17": 17,
        "FieldName18": 18
      })";
  constexpr absl::string_view kExpected = R"(
        __field_name13: 13
        __Field_name14: 14
        field__name15: 15
        field__Name16: 16
        field_name17__: 17
        Field_name18__: 18
      )";
  EXPECT_THAT(Testee(kP3, "FieldNameWithDoubleUnderscores")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee(kP3, "FieldNameWithDoubleUnderscores")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// Using the original proto field name in JSON is also allowed.
TEST_P(JsonFieldNameTest, OriginalProtoFieldName) {
  constexpr absl::string_view kInput = R"({
        "fieldname1": 1,
        "field_name2": 2,
        "_field_name3": 3,
        "field__name4_": 4,
        "field0name5": 5,
        "field_0_name6": 6,
        "fieldName7": 7,
        "FieldName8": 8,
        "field_Name9": 9,
        "Field_Name10": 10,
        "FIELD_NAME11": 11,
        "FIELD_name12": 12,
        "__field_name13": 13,
        "__Field_name14": 14,
        "field__name15": 15,
        "field__Name16": 16,
        "field_name17__": 17,
        "Field_name18__": 18
      })";
  constexpr absl::string_view kExpected = R"(
        fieldname1: 1
        field_name2: 2
        _field_name3: 3
        field__name4_: 4
        field0name5: 5
        field_0_name6: 6
        fieldName7: 7
        FieldName8: 8
        field_Name9: 9
        Field_Name10: 10
        FIELD_NAME11: 11
        FIELD_name12: 12
        __field_name13: 13
        __Field_name14: 14
        field__name15: 15
        field__Name16: 16
        field_name17__: 17
        Field_name18__: 18
      )";
  EXPECT_THAT(Testee("OriginalProtoFieldName")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee("OriginalProtoFieldName")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// Field names can be escaped.
TEST_P(JsonFieldNameTest, FieldNameEscaped) {
  constexpr absl::string_view kInput = R"({"fieldn\u0061me1": 1})";
  EXPECT_THAT(
      Testee("FieldNameEscaped").ParseJson(message(), kInput).SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto("fieldname1: 1"))));
  EXPECT_THAT(
      Testee("FieldNameEscaped").ParseJson(message(), kInput).SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto("fieldname1: 1"))));
}

// String ends with escape character.
TEST_P(JsonFieldNameTest, StringEndsWithEscapeChar) {
  EXPECT_THAT(Testee(kP3, "StringEndsWithEscapeChar")
                  .ParseJson(message(), "{\"optionalString\": \"abc\\")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// Field names must be quoted (or it's not valid JSON).
TEST_P(JsonFieldNameTest, FieldNameNotQuoted) {
  EXPECT_THAT(Testee(kP3, "FieldNameNotQuoted")
                  .ParseJson(message(), "{fieldname1: 1}")
                  .ParseOnly(),
              Yields(IsParseError()));
}

INSTANTIATE_TEST_SUITE_P(All, JsonFieldNameTest,
                         ValuesIn(AllTestMessageTypes()), MessageTypeParamName);

// ---------------------------------------------------------------------------
// Object syntax: trailing commas, comments, whitespace, missing commas.
// ---------------------------------------------------------------------------

class JsonObjectSyntaxTest : public MessageTypeConformanceTest {
 public:
  TestPriority DefaultPriority() const override { return kP3; }
};

// Trailing comma is not allowed (not valid JSON).
TEST_P(JsonObjectSyntaxTest, TrailingCommaInAnObject) {
  EXPECT_THAT(Testee("TrailingCommaInAnObject")
                  .ParseJson(message(), R"({"fieldname1":1,})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonObjectSyntaxTest, TrailingCommaInAnObjectWithSpace) {
  EXPECT_THAT(Testee("TrailingCommaInAnObjectWithSpace")
                  .ParseJson(message(), R"({"fieldname1":1 ,})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonObjectSyntaxTest, TrailingCommaInAnObjectWithSpaceCommaSpace) {
  EXPECT_THAT(Testee("TrailingCommaInAnObjectWithSpaceCommaSpace")
                  .ParseJson(message(), R"({"fieldname1":1 , })")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonObjectSyntaxTest, TrailingCommaInAnObjectWithNewlines) {
  EXPECT_THAT(Testee("TrailingCommaInAnObjectWithNewlines")
                  .ParseJson(message(), R"({
        "fieldname1":1,
      })")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// JSON doesn't support comments.
TEST_P(JsonObjectSyntaxTest, JsonWithComments) {
  EXPECT_THAT(Testee("JsonWithComments")
                  .ParseJson(message(), R"({
        // This is a comment.
        "fieldname1": 1
      })")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// JSON spec says whitespace doesn't matter, so try a few spacings to be sure.
constexpr absl::string_view kTwoInt32sExpected = R"(
        optional_int32: 1
        optional_int64: 2
      )";

TEST_P(JsonObjectSyntaxTest, OneLineNoSpaces) {
  constexpr absl::string_view kInput =
      "{\"optionalInt32\":1,\"optionalInt64\":2}";
  EXPECT_THAT(
      Testee("OneLineNoSpaces").ParseJson(message(), kInput).SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(kTwoInt32sExpected))));
  EXPECT_THAT(
      Testee("OneLineNoSpaces").ParseJson(message(), kInput).SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(kTwoInt32sExpected))));
}

TEST_P(JsonObjectSyntaxTest, OneLineWithSpaces) {
  constexpr absl::string_view kInput =
      "{ \"optionalInt32\" : 1 , \"optionalInt64\" : 2 }";
  EXPECT_THAT(Testee("OneLineWithSpaces")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kTwoInt32sExpected))));
  EXPECT_THAT(
      Testee("OneLineWithSpaces").ParseJson(message(), kInput).SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(kTwoInt32sExpected))));
}

TEST_P(JsonObjectSyntaxTest, MultilineNoSpaces) {
  constexpr absl::string_view kInput =
      "{\n\"optionalInt32\"\n:\n1\n,\n\"optionalInt64\"\n:\n2\n}";
  EXPECT_THAT(Testee("MultilineNoSpaces")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kTwoInt32sExpected))));
  EXPECT_THAT(
      Testee("MultilineNoSpaces").ParseJson(message(), kInput).SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(kTwoInt32sExpected))));
}

TEST_P(JsonObjectSyntaxTest, MultilineWithSpaces) {
  constexpr absl::string_view kInput =
      "{\n  \"optionalInt32\"  :  1\n  ,\n  \"optionalInt64\"  :  2\n}\n";
  EXPECT_THAT(Testee("MultilineWithSpaces")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kTwoInt32sExpected))));
  EXPECT_THAT(Testee("MultilineWithSpaces")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kTwoInt32sExpected))));
}

// Missing comma between key/value pairs.
TEST_P(JsonObjectSyntaxTest, MissingCommaOneLine) {
  EXPECT_THAT(
      Testee("MissingCommaOneLine")
          .ParseJson(message(), "{ \"optionalInt32\": 1 \"optionalInt64\": 2 }")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonObjectSyntaxTest, MissingCommaMultiline) {
  EXPECT_THAT(
      Testee("MissingCommaMultiline")
          .ParseJson(message(),
                     "{\n  \"optionalInt32\": 1\n  \"optionalInt64\": 2\n}")
          .ParseOnly(),
      Yields(IsParseError()));
}

INSTANTIATE_TEST_SUITE_P(All, JsonObjectSyntaxTest,
                         ValuesIn(AllTestMessageTypes()), MessageTypeParamName);

// ---------------------------------------------------------------------------
// Duplicated field names have either last-wins or parse failure.  Like the
// legacy RunValidJsonTestOrParseFailure(), binary output is requested under a
// name without an output suffix.
// ---------------------------------------------------------------------------

class JsonDuplicateFieldNameTest : public MessageTypeConformanceTest {
 public:
  TestPriority DefaultPriority() const override { return kP3; }
};

TEST_P(JsonDuplicateFieldNameTest, FieldNameDuplicate) {
  EXPECT_THAT(
      Testee("FieldNameDuplicate")
          .ParseJson(message(), R"({
                                   "optionalNestedMessage": {"a": 1},
                                   "optionalNestedMessage": {}
                                 })")
          .ParseOnly({/*output_format=*/::conformance::PROTOBUF}),
      Yields(AnyOf(IsParseError(), ParsedPayload(EqualsTextProto(
                                       "optional_nested_message: {}")))));
}

TEST_P(JsonDuplicateFieldNameTest, FieldNameDuplicateDifferentCasing1) {
  EXPECT_THAT(
      Testee("FieldNameDuplicateDifferentCasing1")
          .ParseJson(message(), R"({
                                   "optional_nested_message": {"a": 1},
                                   "optionalNestedMessage": {}
                                 })")
          .ParseOnly({/*output_format=*/::conformance::PROTOBUF}),
      Yields(AnyOf(IsParseError(), ParsedPayload(EqualsTextProto(
                                       "optional_nested_message: {}")))));
}

TEST_P(JsonDuplicateFieldNameTest, FieldNameDuplicateDifferentCasing2) {
  EXPECT_THAT(
      Testee("FieldNameDuplicateDifferentCasing2")
          .ParseJson(message(), R"({
                                   "optionalNestedMessage": {"a": 1},
                                   "optional_nested_message": {}
                                 })")
          .ParseOnly({/*output_format=*/::conformance::PROTOBUF}),
      Yields(AnyOf(IsParseError(), ParsedPayload(EqualsTextProto(
                                       "optional_nested_message: {}")))));
}

INSTANTIATE_TEST_SUITE_P(All, JsonDuplicateFieldNameTest,
                         ValuesIn(AllTestMessageTypes()), MessageTypeParamName);

// ---------------------------------------------------------------------------
// Serializers should use lowerCamelCase by default.  These look at the JSON
// text the testee produces, not at the message it decodes to.
// ---------------------------------------------------------------------------

using JsonFieldNameSerializationTest = MessageTypeConformanceTest;

TEST_P(JsonFieldNameSerializationTest, FieldNameInLowerCamelCase) {
  EXPECT_THAT(Testee("FieldNameInLowerCamelCase")
                  .ParseJson(message(), R"({
        "fieldname1": 1,
        "fieldName2": 2,
        "FieldName3": 3,
        "fieldName4": 4
      })")
                  .SerializeJson(),
              Yields(JsonPayload(AllOf(
                  HasJsonMember("fieldname1"), HasJsonMember("fieldName2"),
                  HasJsonMember("FieldName3"), HasJsonMember("fieldName4")))));
}

TEST_P(JsonFieldNameSerializationTest, FieldNameWithNumbersValidator) {
  EXPECT_THAT(Testee("FieldNameWithNumbersValidator")
                  .ParseJson(message(), R"({
        "field0name5": 5,
        "field0Name6": 6
      })")
                  .SerializeJson(),
              Yields(JsonPayload(AllOf(HasJsonMember("field0name5"),
                                       HasJsonMember("field0Name6")))));
}

TEST_P(JsonFieldNameSerializationTest, FieldNameWithMixedCasesValidator) {
  EXPECT_THAT(
      Testee("FieldNameWithMixedCasesValidator")
          .ParseJson(message(), R"({
        "fieldName7": 7,
        "FieldName8": 8,
        "fieldName9": 9,
        "FieldName10": 10,
        "FIELDNAME11": 11,
        "FIELDName12": 12
      })")
          .SerializeJson(),
      Yields(JsonPayload(
          AllOf(HasJsonMember("fieldName7"), HasJsonMember("FieldName8"),
                HasJsonMember("fieldName9"), HasJsonMember("FieldName10"),
                HasJsonMember("FIELDNAME11"), HasJsonMember("FIELDName12")))));
}

TEST_P(JsonFieldNameSerializationTest,
       FieldNameWithDoubleUnderscoresValidator) {
  EXPECT_THAT(
      Testee(kP3, "FieldNameWithDoubleUnderscoresValidator")
          .ParseJson(message(), R"({
        "FieldName13": 13,
        "FieldName14": 14,
        "fieldName15": 15,
        "fieldName16": 16,
        "fieldName17": 17,
        "FieldName18": 18
      })")
          .SerializeJson(),
      Yields(JsonPayload(
          AllOf(HasJsonMember("FieldName13"), HasJsonMember("FieldName14"),
                HasJsonMember("fieldName15"), HasJsonMember("fieldName16"),
                HasJsonMember("fieldName17"), HasJsonMember("FieldName18")))));
}

INSTANTIATE_TEST_SUITE_P(All, JsonFieldNameSerializationTest,
                         ValuesIn(AllTestMessageTypes()), MessageTypeParamName);

// ---------------------------------------------------------------------------
// Default values: proto3 (implicit presence) skips them, proto2 (explicit
// presence) stores them.  Extensions are serialized under their bracketed
// full name.
// ---------------------------------------------------------------------------

using JsonSkipsDefaultTest = MessageTypeConformanceTest;

TEST_P(JsonSkipsDefaultTest, SkipsDefaultPrimitive) {
  EXPECT_THAT(Testee("SkipsDefaultPrimitive")
                  .ParseJson(message(), R"({"FieldName13": 0})")
                  .SerializeJson(),
              Yields(JsonPayload(Not(HasJsonMember("FieldName13")))));
}

INSTANTIATE_TEST_SUITE_P(All, JsonSkipsDefaultTest,
                         ValuesIn(Proto3TestMessageTypes()),
                         MessageTypeParamName);

// Only run on proto2 message types for now; proto3 fields with explicit
// presence would be a candidate as well.
using JsonStoresDefaultTest = MessageTypeConformanceTest;

TEST_P(JsonStoresDefaultTest, StoresDefaultPrimitive) {
  EXPECT_THAT(Testee("StoresDefaultPrimitive")
                  .ParseJson(message(), R"({
          "FieldName13": 0
        })")
                  .SerializeJson(),
              Yields(JsonPayload(HasJsonMember("FieldName13"))));
}

TEST_P(JsonStoresDefaultTest, FieldNameExtension) {
  // Like the legacy suite, the first extension of the message type in its
  // pool is used, whichever it is.
  std::vector<const FieldDescriptor*> extensions;
  message()->file()->pool()->FindAllExtensions(message(), &extensions);
  ASSERT_FALSE(extensions.empty());
  const absl::string_view extension_name = extensions[0]->full_name();
  EXPECT_THAT(Testee(kP3, "FieldNameExtension")
                  .ParseJson(message(), absl::Substitute(R"({
          "[$0]": 1
        })",
                                                         extension_name))
                  .SerializeJson(),
              Yields(JsonPayload(
                  HasJsonMember(absl::StrCat("[", extension_name, "]")))));
}

INSTANTIATE_TEST_SUITE_P(All, JsonStoresDefaultTest,
                         ValuesIn(Proto2TestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
