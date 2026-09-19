// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
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
#include "conformance/binary_test_util.h"
#include "conformance/matchers.h"
#include "conformance/message_type_fixtures.h"
#include "conformance/testee.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::ValuesIn;

using JsonMapTest = MessageTypeConformanceTest;

// Map keys are always JSON strings, even for integer and bool key types.
TEST_P(JsonMapTest, Int32MapField) {
  constexpr absl::string_view kInput =
      R"json({"mapInt32Int32": {"1": 2, "3": 4}})json";
  constexpr absl::string_view kExpected = R"pb(
    map_int32_int32: { key: 1 value: 2 }
    map_int32_int32: { key: 3 value: 4 }
  )pb";
  EXPECT_THAT(
      Testee("Int32MapField").ParseJson(message(), kInput).SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("Int32MapField").ParseJson(message(), kInput).SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonMapTest, Int32MapFieldKeyNotQuoted) {
  EXPECT_THAT(
      Testee(kP1, "Int32MapFieldKeyNotQuoted")
          .ParseJson(message(), R"json({"mapInt32Int32": {1: 2, 3: 4}})json")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonMapTest, Uint32MapField) {
  constexpr absl::string_view kInput =
      R"json({"mapUint32Uint32": {"1": 2, "3": 4}})json";
  constexpr absl::string_view kExpected = R"pb(
    map_uint32_uint32: { key: 1 value: 2 }
    map_uint32_uint32: { key: 3 value: 4 }
  )pb";
  EXPECT_THAT(
      Testee("Uint32MapField").ParseJson(message(), kInput).SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("Uint32MapField").ParseJson(message(), kInput).SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonMapTest, Uint32MapFieldKeyNotQuoted) {
  EXPECT_THAT(
      Testee(kP1, "Uint32MapFieldKeyNotQuoted")
          .ParseJson(message(), R"json({"mapUint32Uint32": {1: 2, 3: 4}})json")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonMapTest, Int64MapField) {
  constexpr absl::string_view kInput =
      R"json({"mapInt64Int64": {"1": 2, "3": 4}})json";
  constexpr absl::string_view kExpected = R"pb(
    map_int64_int64: { key: 1 value: 2 }
    map_int64_int64: { key: 3 value: 4 }
  )pb";
  EXPECT_THAT(
      Testee("Int64MapField").ParseJson(message(), kInput).SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("Int64MapField").ParseJson(message(), kInput).SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonMapTest, Int64MapFieldKeyNotQuoted) {
  EXPECT_THAT(
      Testee(kP1, "Int64MapFieldKeyNotQuoted")
          .ParseJson(message(), R"json({"mapInt64Int64": {1: 2, 3: 4}})json")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonMapTest, Uint64MapField) {
  constexpr absl::string_view kInput =
      R"json({"mapUint64Uint64": {"1": 2, "3": 4}})json";
  constexpr absl::string_view kExpected = R"pb(
    map_uint64_uint64: { key: 1 value: 2 }
    map_uint64_uint64: { key: 3 value: 4 }
  )pb";
  EXPECT_THAT(
      Testee("Uint64MapField").ParseJson(message(), kInput).SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("Uint64MapField").ParseJson(message(), kInput).SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonMapTest, Uint64MapFieldKeyNotQuoted) {
  EXPECT_THAT(
      Testee(kP1, "Uint64MapFieldKeyNotQuoted")
          .ParseJson(message(), R"json({"mapUint64Uint64": {1: 2, 3: 4}})json")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonMapTest, BoolMapField) {
  constexpr absl::string_view kInput =
      R"json({"mapBoolBool": {"true": true, "false": false}})json";
  constexpr absl::string_view kExpected = R"pb(
    map_bool_bool: { key: true value: true }
    map_bool_bool: { key: false value: false }
  )pb";
  EXPECT_THAT(
      Testee("BoolMapField").ParseJson(message(), kInput).SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("BoolMapField").ParseJson(message(), kInput).SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonMapTest, BoolMapFieldKeyNotQuoted) {
  EXPECT_THAT(
      Testee(kP1, "BoolMapFieldKeyNotQuoted")
          .ParseJson(message(),
                     R"json({"mapBoolBool": {true: true, false: false}})json")
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(JsonMapTest, MessageMapField) {
  constexpr absl::string_view kInput = R"json({
        "mapStringNestedMessage": {
          "hello": {"a": 1234},
          "world": {"a": 5678}
  }
      })json";
  constexpr absl::string_view kExpected = R"pb(
    map_string_nested_message: {
      key: "hello"
      value: { a: 1234 }
    }
    map_string_nested_message: {
      key: "world"
      value: { a: 5678 }
    }
  )pb";
  EXPECT_THAT(
      Testee("MessageMapField").ParseJson(message(), kInput).SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("MessageMapField").ParseJson(message(), kInput).SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
}

// Since Map keys are represented as JSON strings, escaping should be allowed.
TEST_P(JsonMapTest, Int32MapEscapedKey) {
  constexpr absl::string_view kInput =
      R"json({"mapInt32Int32": {"\u0031": 2}})json";
  constexpr absl::string_view kExpected =
      R"pb(map_int32_int32: { key: 1 value: 2 })pb";
  EXPECT_THAT(Testee("Int32MapEscapedKey")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("Int32MapEscapedKey").ParseJson(message(), kInput).SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonMapTest, Int64MapEscapedKey) {
  constexpr absl::string_view kInput =
      R"json({"mapInt64Int64": {"\u0031": 2}})json";
  constexpr absl::string_view kExpected =
      R"pb(map_int64_int64: { key: 1 value: 2 })pb";
  EXPECT_THAT(Testee("Int64MapEscapedKey")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("Int64MapEscapedKey").ParseJson(message(), kInput).SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonMapTest, BoolMapEscapedKey) {
  constexpr absl::string_view kInput =
      R"json({"mapBoolBool": {"tr\u0075e": true}})json";
  constexpr absl::string_view kExpected =
      R"pb(map_bool_bool: { key: true value: true })pb";
  EXPECT_THAT(Testee("BoolMapEscapedKey")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("BoolMapEscapedKey").ParseJson(message(), kInput).SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
}

INSTANTIATE_TEST_SUITE_P(All, JsonMapTest, ValuesIn(AllTestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
