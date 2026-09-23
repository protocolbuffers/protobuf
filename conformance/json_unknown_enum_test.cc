// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// JSON conformance tests for unknown enum values given as string labels, in
// a singular field, a repeated field and a map value: a parser must reject
// them unless it is asked to ignore unknown fields, in which case it drops
// the unknown value (and, in a list or map mixing known and unknown labels,
// keeps the known ones).  This replaces the legacy
// BinaryAndJsonConformanceSuiteImpl<M>::RunJsonTestsForUnknownEnumStringValues(),
// which ran for every test message type; the requests sent to the testee are
// identical to the legacy ones.
//
// Like the legacy RunValidJsonIgnoreUnknownTest(), the ignore-unknown tests
// only have the binary-output leg ("<name>.ProtobufOutput", in the
// JSON_IGNORE_UNKNOWN_PARSING_TEST category), and like
// ExpectParseFailureForJson() the rejections ask for JSON output under a name
// without an output suffix.  The inputs are the legacy raw string literals
// verbatim, including their snake_case member names and whitespace.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
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

class JsonUnknownEnumTest : public MessageTypeConformanceTest {
 public:
  TestPriority DefaultPriority() const override { return kP3; }
};

// Unknown enum string values are a parse failure when not ignoring unknown
// fields.
TEST_P(JsonUnknownEnumTest, RejectUnknownEnumStringValueInOptionalField) {
  EXPECT_THAT(Testee()
                  .ParseJson(message(), R"json({
      "optional_nested_enum": "UNKNOWN_ENUM_VALUE"
    })json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonUnknownEnumTest, RejectUnknownEnumStringValueInRepeatedField) {
  EXPECT_THAT(Testee()
                  .ParseJson(message(), R"json({
      "repeated_nested_enum": ["UNKNOWN_ENUM_VALUE"]
    })json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonUnknownEnumTest, RejectUnknownEnumStringValueInMapValue) {
  EXPECT_THAT(Testee()
                  .ParseJson(message(), R"json({
      "map_string_nested_enum": {"key": "UNKNOWN_ENUM_VALUE"}
    })json")
                  .ParseOnly(),
              Yields(IsParseError()));
}

// ... and are ignored when ignoring unknown fields.
TEST_P(JsonUnknownEnumTest, IgnoreUnknownEnumStringValueInOptionalField) {
  EXPECT_THAT(Testee()
                  .ParseJson(message(), R"json({
      "optional_nested_enum": "UNKNOWN_ENUM_VALUE"
    })json",
                             {/*ignore_unknown_fields=*/true})
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(""))));
}

TEST_P(JsonUnknownEnumTest, IgnoreUnknownEnumStringValueInRepeatedField) {
  EXPECT_THAT(Testee()
                  .ParseJson(message(), R"json({
      "repeated_nested_enum": ["UNKNOWN_ENUM_VALUE"]
    })json",
                             {/*ignore_unknown_fields=*/true})
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(""))));
}

TEST_P(JsonUnknownEnumTest, IgnoreUnknownEnumStringValueInMapValue) {
  EXPECT_THAT(Testee()
                  .ParseJson(message(), R"json({
      "map_string_nested_enum": {"key": "UNKNOWN_ENUM_VALUE"}
    })json",
                             {/*ignore_unknown_fields=*/true})
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(""))));
}

// Like IgnoreUnknownEnumStringValueInRepeatedField, but highlights the
// potentially unexpected behavior in an array with mixed known and unknown
// enum string values.
TEST_P(JsonUnknownEnumTest, IgnoreUnknownEnumStringValueInRepeatedPart) {
  EXPECT_THAT(Testee()
                  .ParseJson(message(), R"json({
    "repeated_nested_enum": [
      "FOO",
      "UNKNOWN_ENUM_VALUE",
      "FOO"
    ]})json",
                             {/*ignore_unknown_fields=*/true})
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(
                  "repeated_nested_enum: FOO repeated_nested_enum: FOO"))));
}

// Like IgnoreUnknownEnumStringValueInMapValue, with a mixture of known and
// unknown enum string values in the map.
TEST_P(JsonUnknownEnumTest, IgnoreUnknownEnumStringValueInMapPart) {
  EXPECT_THAT(
      Testee()
          .ParseJson(message(), R"json({
    "map_string_nested_enum": {
      "key1": "FOO",
      "key2": "UNKNOWN_ENUM_VALUE"
    }})json",
                     {/*ignore_unknown_fields=*/true})
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(
          R"pb(map_string_nested_enum: { key: "key1" value: FOO })pb"))));
}

INSTANTIATE_TEST_SUITE_P(All, JsonUnknownEnumTest,
                         ValuesIn(AllTestMessageTypes()), MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
