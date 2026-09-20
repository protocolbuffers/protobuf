// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// JSON conformance tests for the well-known wrapper types
// (google.protobuf.BoolValue, Int32Value, ...): a wrapper is represented by
// its value alone, is present even when that value is the default, and is
// unset by `null`.  This replaces the wrapper type part of the legacy
// BinaryAndJsonConformanceSuiteImpl<M>::RunJsonTestsForWrapperTypes(), which
// ran for the proto3-style message types only; the requests sent to the testee
// are identical to the legacy ones.
//
// Like the legacy RunValidJsonTest(), each input is sent twice: once to be
// serialized as binary ("<name>.ProtobufOutput") and once as JSON
// ("<name>.JsonOutput").

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "conformance/binary_test_util.h"
#include "conformance/matchers.h"
#include "conformance/message_type_fixtures.h"
#include "conformance/test_environment.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::ValuesIn;

using JsonWrapperTest = MessageTypeConformanceTest;

// Singular wrappers holding their default value are present (a wrapper
// message is set even when its value is the default).
TEST_P(JsonWrapperTest, OptionalBoolWrapper) {
  constexpr absl::string_view kInput = R"({"optionalBoolWrapper": false})";
  constexpr absl::string_view kExpected =
      "optional_bool_wrapper: {value: false}";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonWrapperTest, OptionalInt32Wrapper) {
  constexpr absl::string_view kInput = R"({"optionalInt32Wrapper": 0})";
  constexpr absl::string_view kExpected = "optional_int32_wrapper: {value: 0}";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonWrapperTest, OptionalUint32Wrapper) {
  constexpr absl::string_view kInput = R"({"optionalUint32Wrapper": 0})";
  constexpr absl::string_view kExpected = "optional_uint32_wrapper: {value: 0}";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonWrapperTest, OptionalInt64Wrapper) {
  constexpr absl::string_view kInput = R"({"optionalInt64Wrapper": 0})";
  constexpr absl::string_view kExpected = "optional_int64_wrapper: {value: 0}";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonWrapperTest, OptionalUint64Wrapper) {
  constexpr absl::string_view kInput = R"({"optionalUint64Wrapper": 0})";
  constexpr absl::string_view kExpected = "optional_uint64_wrapper: {value: 0}";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonWrapperTest, OptionalFloatWrapper) {
  constexpr absl::string_view kInput = R"({"optionalFloatWrapper": 0})";
  constexpr absl::string_view kExpected = "optional_float_wrapper: {value: 0}";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonWrapperTest, OptionalDoubleWrapper) {
  constexpr absl::string_view kInput = R"({"optionalDoubleWrapper": 0})";
  constexpr absl::string_view kExpected = "optional_double_wrapper: {value: 0}";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonWrapperTest, OptionalStringWrapper) {
  constexpr absl::string_view kInput = R"({"optionalStringWrapper": ""})";
  constexpr absl::string_view kExpected =
      R"(optional_string_wrapper: {value: ""})";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonWrapperTest, OptionalBytesWrapper) {
  constexpr absl::string_view kInput = R"({"optionalBytesWrapper": ""})";
  constexpr absl::string_view kExpected =
      R"(optional_bytes_wrapper: {value: ""})";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonWrapperTest, OptionalWrapperTypesWithNonDefaultValue) {
  constexpr absl::string_view kInput = R"({
        "optionalBoolWrapper": true,
        "optionalInt32Wrapper": 1,
        "optionalUint32Wrapper": 1,
        "optionalInt64Wrapper": "1",
        "optionalUint64Wrapper": "1",
        "optionalFloatWrapper": 1,
        "optionalDoubleWrapper": 1,
        "optionalStringWrapper": "1",
        "optionalBytesWrapper": "AQI="
      })";
  constexpr absl::string_view kExpected = R"(
        optional_bool_wrapper: {value: true}
        optional_int32_wrapper: {value: 1}
        optional_uint32_wrapper: {value: 1}
        optional_int64_wrapper: {value: 1}
        optional_uint64_wrapper: {value: 1}
        optional_float_wrapper: {value: 1}
        optional_double_wrapper: {value: 1}
        optional_string_wrapper: {value: "1"}
        optional_bytes_wrapper: {value: "\x01\x02"}
      )";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// Repeated wrappers.
TEST_P(JsonWrapperTest, RepeatedBoolWrapper) {
  constexpr absl::string_view kInput =
      R"({"repeatedBoolWrapper": [true, false]})";
  constexpr absl::string_view kExpected =
      "repeated_bool_wrapper: {value: true}"
      "repeated_bool_wrapper: {value: false}";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonWrapperTest, RepeatedInt32Wrapper) {
  constexpr absl::string_view kInput = R"({"repeatedInt32Wrapper": [0, 1]})";
  constexpr absl::string_view kExpected =
      "repeated_int32_wrapper: {value: 0}"
      "repeated_int32_wrapper: {value: 1}";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonWrapperTest, RepeatedUint32Wrapper) {
  constexpr absl::string_view kInput = R"({"repeatedUint32Wrapper": [0, 1]})";
  constexpr absl::string_view kExpected =
      "repeated_uint32_wrapper: {value: 0}"
      "repeated_uint32_wrapper: {value: 1}";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonWrapperTest, RepeatedInt64Wrapper) {
  constexpr absl::string_view kInput = R"({"repeatedInt64Wrapper": [0, 1]})";
  constexpr absl::string_view kExpected =
      "repeated_int64_wrapper: {value: 0}"
      "repeated_int64_wrapper: {value: 1}";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonWrapperTest, RepeatedUint64Wrapper) {
  constexpr absl::string_view kInput = R"({"repeatedUint64Wrapper": [0, 1]})";
  constexpr absl::string_view kExpected =
      "repeated_uint64_wrapper: {value: 0}"
      "repeated_uint64_wrapper: {value: 1}";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonWrapperTest, RepeatedFloatWrapper) {
  constexpr absl::string_view kInput = R"({"repeatedFloatWrapper": [0, 1]})";
  constexpr absl::string_view kExpected =
      "repeated_float_wrapper: {value: 0}"
      "repeated_float_wrapper: {value: 1}";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonWrapperTest, RepeatedDoubleWrapper) {
  constexpr absl::string_view kInput = R"({"repeatedDoubleWrapper": [0, 1]})";
  constexpr absl::string_view kExpected =
      "repeated_double_wrapper: {value: 0}"
      "repeated_double_wrapper: {value: 1}";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonWrapperTest, RepeatedStringWrapper) {
  constexpr absl::string_view kInput =
      R"({"repeatedStringWrapper": ["", "AQI="]})";
  constexpr absl::string_view kExpected = R"(
        repeated_string_wrapper: {value: ""}
        repeated_string_wrapper: {value: "AQI="}
      )";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonWrapperTest, RepeatedBytesWrapper) {
  constexpr absl::string_view kInput =
      R"({"repeatedBytesWrapper": ["", "AQI="]})";
  constexpr absl::string_view kExpected = R"(
        repeated_bytes_wrapper: {value: ""}
        repeated_bytes_wrapper: {value: "\x01\x02"}
      )";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// `null` unsets a wrapper (unlike a Value, for which it is a null value).
TEST_P(JsonWrapperTest, WrapperTypesWithNullValue) {
  constexpr absl::string_view kInput = R"({
        "optionalBoolWrapper": null,
        "optionalInt32Wrapper": null,
        "optionalUint32Wrapper": null,
        "optionalInt64Wrapper": null,
        "optionalUint64Wrapper": null,
        "optionalFloatWrapper": null,
        "optionalDoubleWrapper": null,
        "optionalStringWrapper": null,
        "optionalBytesWrapper": null,
        "repeatedBoolWrapper": null,
        "repeatedInt32Wrapper": null,
        "repeatedUint32Wrapper": null,
        "repeatedInt64Wrapper": null,
        "repeatedUint64Wrapper": null,
        "repeatedFloatWrapper": null,
        "repeatedDoubleWrapper": null,
        "repeatedStringWrapper": null,
        "repeatedBytesWrapper": null
      })";
  constexpr absl::string_view kExpected = "";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

INSTANTIATE_TEST_SUITE_P(All, JsonWrapperTest,
                         ValuesIn(Proto3TestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
