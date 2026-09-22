// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// The smoke tests of the JSON conformance suite: a single string field
// round-trips through the testee, unknown members of every JSON kind are
// ignored when asked to, and a top-level `null` is rejected.  This replaces
// the tests the legacy BinaryAndJsonConformanceSuiteImpl<M>::RunJsonTests()
// ran directly (rather than through one of its RunJsonTestsFor*() groups),
// for every test message type; the requests sent to the testee are identical to
// the legacy ones.
//
// Like the legacy RunValidJsonTest(), each valid input is sent twice: once to
// be serialized as binary ("<name>.ProtobufOutput") and once as JSON
// ("<name>.JsonOutput").  The legacy RunValidJsonIgnoreUnknownTest() only had
// the binary leg, and ExpectParseFailureForJson() asked for JSON output under
// a name without an output suffix.

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

using JsonHelloWorldTest = MessageTypeConformanceTest;

TEST_P(JsonHelloWorldTest, HelloWorld) {
  constexpr absl::string_view kInput = R"({"optionalString":"Hello, World!"})";
  constexpr absl::string_view kExpected = "optional_string: 'Hello, World!'";
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(Testee().ParseJson(message(), kInput).SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// Unknown members are ignored whatever kind of JSON value they hold.
TEST_P(JsonHelloWorldTest, IgnoreUnknownJsonNumber) {
  EXPECT_THAT(Testee()
                  .ParseJson(message(), R"({"unknown": 1})",
                             {/*ignore_unknown_fields=*/true})
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(""))));
}

TEST_P(JsonHelloWorldTest, IgnoreUnknownJsonString) {
  EXPECT_THAT(Testee()
                  .ParseJson(message(), R"({"unknown": "a"})",
                             {/*ignore_unknown_fields=*/true})
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(""))));
}

TEST_P(JsonHelloWorldTest, IgnoreUnknownJsonTrue) {
  EXPECT_THAT(Testee()
                  .ParseJson(message(), R"({"unknown": true})",
                             {/*ignore_unknown_fields=*/true})
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(""))));
}

TEST_P(JsonHelloWorldTest, IgnoreUnknownJsonFalse) {
  EXPECT_THAT(Testee()
                  .ParseJson(message(), R"({"unknown": false})",
                             {/*ignore_unknown_fields=*/true})
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(""))));
}

TEST_P(JsonHelloWorldTest, IgnoreUnknownJsonNull) {
  EXPECT_THAT(Testee()
                  .ParseJson(message(), R"({"unknown": null})",
                             {/*ignore_unknown_fields=*/true})
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(""))));
}

TEST_P(JsonHelloWorldTest, IgnoreUnknownJsonObject) {
  EXPECT_THAT(Testee()
                  .ParseJson(message(), R"({"unknown": {"a": 1}})",
                             {/*ignore_unknown_fields=*/true})
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(""))));
}

TEST_P(JsonHelloWorldTest, RejectTopLevelNull) {
  EXPECT_THAT(Testee().ParseJson(message(), "null").ParseOnly(),
              Yields(IsParseError()));
}

INSTANTIATE_TEST_SUITE_P(All, JsonHelloWorldTest,
                         ValuesIn(AllTestMessageTypes()), MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
