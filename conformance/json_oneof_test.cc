// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// JSON conformance tests for oneof fields: setting more than one member of a
// oneof in the same object, `null` members, and explicitly zero/empty
// members (which must still mark the oneof as set).  This replaces the
// "Oneof fields" block of the legacy
// BinaryAndJsonConformanceSuiteImpl<M>::RunJsonTestsForNonRepeatedTypes(),
// for every test message type; the test names and the requests sent to the
// testee are identical to the legacy ones.
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
#include "conformance/testee.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::AnyOf;
using ::testing::ValuesIn;

class JsonOneofTest : public MessageTypeConformanceTest {
 private:
  TestPriority DefaultPriority() const override { return kP1; }
};

// Setting two members of the same oneof may either be rejected or resolved
// in favor of the last one.
TEST_P(JsonOneofTest, OneofFieldDuplicate) {
  EXPECT_THAT(
      Testee(kP0, "OneofFieldDuplicate")
          .ParseJson(message(),
                     R"json({"oneofUint32": 1, "oneofString": "test"})json")
          .ParseOnly({/*output_format=*/::conformance::PROTOBUF}),
      Yields(AnyOf(IsParseError(), WhenParsed(EqualsTextProto(
                                       R"pb(oneof_string: "test")pb")))));
}

TEST_P(JsonOneofTest, OneofFieldDuplicate2) {
  EXPECT_THAT(
      Testee(kP0, "OneofFieldDuplicate2")
          .ParseJson(message(),
                     R"json({"oneofString": "test", "oneofUint32": 1})json")
          .ParseOnly({/*output_format=*/::conformance::PROTOBUF}),
      Yields(AnyOf(IsParseError(),
                   WhenParsed(EqualsTextProto(R"pb(oneof_uint32: 1)pb")))));
}

// A null oneof member does not count as setting the oneof, so a second,
// non-null member is not a duplicate.
TEST_P(JsonOneofTest, OneofFieldNullFirst) {
  constexpr absl::string_view kInput =
      R"json({"oneofUint32": null, "oneofString": "test"})json";
  constexpr absl::string_view kExpected = R"pb(oneof_string: "test")pb";
  EXPECT_THAT(Testee(kP0, "OneofFieldNullFirst")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee(kP0, "OneofFieldNullFirst")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonOneofTest, OneofFieldNullSecond) {
  constexpr absl::string_view kInput =
      R"json({"oneofString": "test", "oneofUint32": null})json";
  constexpr absl::string_view kExpected = R"pb(oneof_string: "test")pb";
  EXPECT_THAT(Testee(kP0, "OneofFieldNullSecond")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee(kP0, "OneofFieldNullSecond")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(WhenParsed(EqualsTextProto(kExpected))));
}

// A oneof member explicitly set to its default value is still set.
TEST_P(JsonOneofTest, OneofZeroUint32) {
  constexpr absl::string_view kInput = R"json({"oneofUint32": 0})json";
  constexpr absl::string_view kExpected = R"pb(oneof_uint32: 0)pb";
  EXPECT_THAT(
      Testee("OneofZeroUint32").ParseJson(message(), kInput).SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("OneofZeroUint32").ParseJson(message(), kInput).SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonOneofTest, OneofZeroMessage) {
  constexpr absl::string_view kInput = R"json({"oneofNestedMessage": {}})json";
  constexpr absl::string_view kExpected = R"pb(oneof_nested_message: {})pb";
  EXPECT_THAT(
      Testee("OneofZeroMessage").ParseJson(message(), kInput).SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("OneofZeroMessage").ParseJson(message(), kInput).SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonOneofTest, OneofZeroString) {
  constexpr absl::string_view kInput = R"json({"oneofString": ""})json";
  constexpr absl::string_view kExpected = R"pb(oneof_string: "")pb";
  EXPECT_THAT(
      Testee("OneofZeroString").ParseJson(message(), kInput).SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("OneofZeroString").ParseJson(message(), kInput).SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonOneofTest, OneofZeroBytes) {
  constexpr absl::string_view kInput = R"json({"oneofBytes": ""})json";
  constexpr absl::string_view kExpected = R"pb(oneof_bytes: "")pb";
  EXPECT_THAT(
      Testee("OneofZeroBytes").ParseJson(message(), kInput).SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("OneofZeroBytes").ParseJson(message(), kInput).SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonOneofTest, OneofZeroBool) {
  constexpr absl::string_view kInput = R"json({"oneofBool": false})json";
  constexpr absl::string_view kExpected = R"pb(oneof_bool: false)pb";
  EXPECT_THAT(
      Testee("OneofZeroBool").ParseJson(message(), kInput).SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("OneofZeroBool").ParseJson(message(), kInput).SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonOneofTest, OneofZeroUint64) {
  constexpr absl::string_view kInput = R"json({"oneofUint64": 0})json";
  constexpr absl::string_view kExpected = R"pb(oneof_uint64: 0)pb";
  EXPECT_THAT(
      Testee("OneofZeroUint64").ParseJson(message(), kInput).SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("OneofZeroUint64").ParseJson(message(), kInput).SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonOneofTest, OneofZeroFloat) {
  constexpr absl::string_view kInput = R"json({"oneofFloat": 0.0})json";
  constexpr absl::string_view kExpected = R"pb(oneof_float: 0)pb";
  EXPECT_THAT(
      Testee("OneofZeroFloat").ParseJson(message(), kInput).SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("OneofZeroFloat").ParseJson(message(), kInput).SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonOneofTest, OneofZeroDouble) {
  constexpr absl::string_view kInput = R"json({"oneofDouble": 0.0})json";
  constexpr absl::string_view kExpected = R"pb(oneof_double: 0)pb";
  EXPECT_THAT(
      Testee("OneofZeroDouble").ParseJson(message(), kInput).SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("OneofZeroDouble").ParseJson(message(), kInput).SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
}

TEST_P(JsonOneofTest, OneofZeroEnum) {
  constexpr absl::string_view kInput = R"json({"oneofEnum":"FOO"})json";
  constexpr absl::string_view kExpected = R"pb(oneof_enum: FOO)pb";
  EXPECT_THAT(
      Testee("OneofZeroEnum").ParseJson(message(), kInput).SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("OneofZeroEnum").ParseJson(message(), kInput).SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
}

INSTANTIATE_TEST_SUITE_P(All, JsonOneofTest, ValuesIn(AllTestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
