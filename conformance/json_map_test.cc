// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// JSON conformance tests for map fields: keys are always JSON strings (so
// unquoted integer and bool keys are rejected and escapes in keys are
// honored) and message values are nested objects.  This replaces the "Map
// fields" block of the legacy
// BinaryAndJsonConformanceSuiteImpl<M>::RunJsonTestsForNonRepeatedTypes(),
// for every test message type; the test names and the requests sent to the
// testee are identical to the legacy ones.
//
// Like the legacy RunValidJsonTest(), the valid input is sent twice: once to
// be serialized as binary ("<name>.ProtobufOutput") and once as JSON
// ("<name>.JsonOutput").  The legacy ExpectParseFailureForJson() asked for
// JSON output under a name without an output suffix.

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

using JsonMapTest = MessageTypeConformanceTest;

// Map keys are always JSON strings, even for integer and bool key types.
TEST_P(JsonMapTest, Int32MapField) {
  constexpr absl::string_view kInput = R"({"mapInt32Int32": {"1": 2, "3": 4}})";
  constexpr absl::string_view kExpected =
      "map_int32_int32: {key: 1 value: 2}"
      "map_int32_int32: {key: 3 value: 4}";
  EXPECT_THAT(RequiredTest("Int32MapField")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("Int32MapField")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonMapTest, Int32MapFieldKeyNotQuoted) {
  EXPECT_THAT(RecommendedTest("Int32MapFieldKeyNotQuoted")
                  .ParseJson(message(), R"({"mapInt32Int32": {1: 2, 3: 4}})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonMapTest, Uint32MapField) {
  constexpr absl::string_view kInput =
      R"({"mapUint32Uint32": {"1": 2, "3": 4}})";
  constexpr absl::string_view kExpected =
      "map_uint32_uint32: {key: 1 value: 2}"
      "map_uint32_uint32: {key: 3 value: 4}";
  EXPECT_THAT(RequiredTest("Uint32MapField")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("Uint32MapField")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonMapTest, Uint32MapFieldKeyNotQuoted) {
  EXPECT_THAT(RecommendedTest("Uint32MapFieldKeyNotQuoted")
                  .ParseJson(message(), R"({"mapUint32Uint32": {1: 2, 3: 4}})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonMapTest, Int64MapField) {
  constexpr absl::string_view kInput = R"({"mapInt64Int64": {"1": 2, "3": 4}})";
  constexpr absl::string_view kExpected =
      "map_int64_int64: {key: 1 value: 2}"
      "map_int64_int64: {key: 3 value: 4}";
  EXPECT_THAT(RequiredTest("Int64MapField")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("Int64MapField")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonMapTest, Int64MapFieldKeyNotQuoted) {
  EXPECT_THAT(RecommendedTest("Int64MapFieldKeyNotQuoted")
                  .ParseJson(message(), R"({"mapInt64Int64": {1: 2, 3: 4}})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonMapTest, Uint64MapField) {
  constexpr absl::string_view kInput =
      R"({"mapUint64Uint64": {"1": 2, "3": 4}})";
  constexpr absl::string_view kExpected =
      "map_uint64_uint64: {key: 1 value: 2}"
      "map_uint64_uint64: {key: 3 value: 4}";
  EXPECT_THAT(RequiredTest("Uint64MapField")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("Uint64MapField")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonMapTest, Uint64MapFieldKeyNotQuoted) {
  EXPECT_THAT(RecommendedTest("Uint64MapFieldKeyNotQuoted")
                  .ParseJson(message(), R"({"mapUint64Uint64": {1: 2, 3: 4}})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonMapTest, BoolMapField) {
  constexpr absl::string_view kInput =
      R"({"mapBoolBool": {"true": true, "false": false}})";
  constexpr absl::string_view kExpected =
      "map_bool_bool: {key: true value: true}"
      "map_bool_bool: {key: false value: false}";
  EXPECT_THAT(RequiredTest("BoolMapField")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      RequiredTest("BoolMapField").ParseJson(message(), kInput).SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonMapTest, BoolMapFieldKeyNotQuoted) {
  EXPECT_THAT(RecommendedTest("BoolMapFieldKeyNotQuoted")
                  .ParseJson(message(),
                             R"({"mapBoolBool": {true: true, false: false}})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonMapTest, MessageMapField) {
  constexpr absl::string_view kInput = R"({
        "mapStringNestedMessage": {
          "hello": {"a": 1234},
          "world": {"a": 5678}
  }
      })";
  constexpr absl::string_view kExpected = R"(
        map_string_nested_message: {
          key: "hello"
          value: {a: 1234}
  }
        map_string_nested_message: {
          key: "world"
          value: {a: 5678}
  }
      )";
  EXPECT_THAT(RequiredTest("MessageMapField")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("MessageMapField")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// Since Map keys are represented as JSON strings, escaping should be allowed.
TEST_P(JsonMapTest, Int32MapEscapedKey) {
  constexpr absl::string_view kInput = R"({"mapInt32Int32": {"\u0031": 2}})";
  constexpr absl::string_view kExpected = "map_int32_int32: {key: 1 value: 2}";
  EXPECT_THAT(RequiredTest("Int32MapEscapedKey")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("Int32MapEscapedKey")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonMapTest, Int64MapEscapedKey) {
  constexpr absl::string_view kInput = R"({"mapInt64Int64": {"\u0031": 2}})";
  constexpr absl::string_view kExpected = "map_int64_int64: {key: 1 value: 2}";
  EXPECT_THAT(RequiredTest("Int64MapEscapedKey")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("Int64MapEscapedKey")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonMapTest, BoolMapEscapedKey) {
  constexpr absl::string_view kInput =
      R"({"mapBoolBool": {"tr\u0075e": true}})";
  constexpr absl::string_view kExpected =
      "map_bool_bool: {key: true value: true}";
  EXPECT_THAT(RequiredTest("BoolMapEscapedKey")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("BoolMapEscapedKey")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

INSTANTIATE_TEST_SUITE_P(All, JsonMapTest, ValuesIn(AllTestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
