// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// JSON conformance tests for oneof fields: setting more than one member of a
// oneof in the same object, `null` members, and explicitly zero/empty
// members (which must still mark the oneof as set).  This replaces the
// "Oneof fields" block of the legacy
// BinaryAndJsonConformanceSuiteImpl<M>::RunJsonTestsForNonRepeatedTypes(),
// for every test message type; the requests sent to the testee are identical to
// the legacy ones.
//
// Like the legacy RunValidJsonTest(), the valid input is sent twice: once to
// be serialized as binary ("<name>.ProtobufOutput") and once as JSON
// ("<name>.JsonOutput").  The legacy RunValidJsonTestOrParseFailure() asked
// for binary output under a name without an output suffix.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "conformance/binary_test_util.h"
#include "conformance/conformance.pb.h"
#include "conformance/matchers.h"
#include "conformance/message_type_fixtures.h"
#include "conformance/test_environment.h"
#include "conformance/testee.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::AnyOf;
using ::testing::ValuesIn;

class JsonOneofTest : public MessageTypeConformanceTest {
 public:
  TestPriority DefaultPriority() const override { return kP3; }
};

// Setting two members of the same oneof may either be rejected or resolved
// in favor of the last one.
TEST_P(JsonOneofTest, OneofFieldDuplicate) {
  EXPECT_THAT(
      Testee(kP0)
          .ParseJson(message(), R"({"oneofUint32": 1, "oneofString": "test"})")
          .ParseOnly({/*output_format=*/::conformance::PROTOBUF}),
      Yields(AnyOf(IsParseError(),
                   ParsedPayload(EqualsTextProto("oneof_string: \"test\"")))));
}

TEST_P(JsonOneofTest, OneofFieldDuplicate2) {
  EXPECT_THAT(
      Testee(kP0)
          .ParseJson(message(), R"({"oneofString": "test", "oneofUint32": 1})")
          .ParseOnly({/*output_format=*/::conformance::PROTOBUF}),
      Yields(AnyOf(IsParseError(),
                   ParsedPayload(EqualsTextProto("oneof_uint32: 1")))));
}

// A null oneof member does not count as setting the oneof, so a second,
// non-null member is not a duplicate.
TEST_P(JsonOneofTest, OneofFieldNullFirst) {
  constexpr absl::string_view kInput =
      R"({"oneofUint32": null, "oneofString": "test"})";
  constexpr absl::string_view kExpected = "oneof_string: \"test\"";
  EXPECT_THAT(Testee(kP0).ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee(kP0).ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonOneofTest, OneofFieldNullSecond) {
  constexpr absl::string_view kInput =
      R"({"oneofString": "test", "oneofUint32": null})";
  constexpr absl::string_view kExpected = "oneof_string: \"test\"";
  EXPECT_THAT(Testee(kP0).ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee(kP0).ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// A oneof member explicitly set to its default value is still set.
TEST_P(JsonOneofTest, OneofZeroUint32) {
  constexpr absl::string_view kInput = R"({"oneofUint32": 0})";
  constexpr absl::string_view kExpected = "oneof_uint32: 0";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonOneofTest, OneofZeroMessage) {
  constexpr absl::string_view kInput = R"({"oneofNestedMessage": {}})";
  constexpr absl::string_view kExpected = "oneof_nested_message: {}";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonOneofTest, OneofZeroString) {
  constexpr absl::string_view kInput = R"({"oneofString": ""})";
  constexpr absl::string_view kExpected = "oneof_string: \"\"";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonOneofTest, OneofZeroBytes) {
  constexpr absl::string_view kInput = R"({"oneofBytes": ""})";
  constexpr absl::string_view kExpected = "oneof_bytes: \"\"";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonOneofTest, OneofZeroBool) {
  constexpr absl::string_view kInput = R"({"oneofBool": false})";
  constexpr absl::string_view kExpected = "oneof_bool: false";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonOneofTest, OneofZeroUint64) {
  constexpr absl::string_view kInput = R"({"oneofUint64": 0})";
  constexpr absl::string_view kExpected = "oneof_uint64: 0";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonOneofTest, OneofZeroFloat) {
  constexpr absl::string_view kInput = R"({"oneofFloat": 0.0})";
  constexpr absl::string_view kExpected = "oneof_float: 0";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonOneofTest, OneofZeroDouble) {
  constexpr absl::string_view kInput = R"({"oneofDouble": 0.0})";
  constexpr absl::string_view kExpected = "oneof_double: 0";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonOneofTest, OneofZeroEnum) {
  constexpr absl::string_view kInput = R"({"oneofEnum":"FOO"})";
  constexpr absl::string_view kExpected = "oneof_enum: FOO";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

INSTANTIATE_TEST_SUITE_P(All, JsonOneofTest, ValuesIn(AllTestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
