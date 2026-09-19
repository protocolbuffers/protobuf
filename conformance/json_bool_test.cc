// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// JSON conformance tests for bool fields: `true` / `false` round-trip, and
// every other spelling (0, 1, True, TRUE, "true", ...) is rejected.  This
// replaces the "Bool fields" block of the legacy
// BinaryAndJsonConformanceSuiteImpl<M>::RunJsonTestsForNonRepeatedTypes(); the
// test names and the requests sent to the testee are identical to the legacy
// ones.
//
// Like the legacy RunValidJsonTest(), each valid input is sent twice: once to
// be serialized as binary ("<name>.ProtobufOutput") and once as JSON
// ("<name>.JsonOutput").  ExpectParseFailureForJson() asked for JSON output
// under a name without an output suffix.

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

using JsonBoolTest = MessageTypeConformanceTest;

TEST_P(JsonBoolTest, BoolFieldTrue) {
  constexpr absl::string_view kInput = R"({"optionalBool":true})";
  constexpr absl::string_view kExpected = "optional_bool: true";
  EXPECT_THAT(RequiredTest("BoolFieldTrue")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("BoolFieldTrue")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

TEST_P(JsonBoolTest, BoolFieldFalse) {
  constexpr absl::string_view kInput = R"({"optionalBool":false})";
  constexpr absl::string_view kExpected = "optional_bool: false";
  EXPECT_THAT(RequiredTest("BoolFieldFalse")
                  .ParseJson(message(), kInput)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
  EXPECT_THAT(RequiredTest("BoolFieldFalse")
                  .ParseJson(message(), kInput)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(kExpected))));
}

// ---------------------------------------------------------------------------
// Other forms are not allowed.
// ---------------------------------------------------------------------------

TEST_P(JsonBoolTest, BoolFieldIntegerZero) {
  EXPECT_THAT(RecommendedTest("BoolFieldIntegerZero")
                  .ParseJson(message(), R"({"optionalBool":0})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonBoolTest, BoolFieldIntegerOne) {
  EXPECT_THAT(RecommendedTest("BoolFieldIntegerOne")
                  .ParseJson(message(), R"({"optionalBool":1})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonBoolTest, BoolFieldCamelCaseTrue) {
  EXPECT_THAT(RecommendedTest("BoolFieldCamelCaseTrue")
                  .ParseJson(message(), R"({"optionalBool":True})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonBoolTest, BoolFieldCamelCaseFalse) {
  EXPECT_THAT(RecommendedTest("BoolFieldCamelCaseFalse")
                  .ParseJson(message(), R"({"optionalBool":False})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonBoolTest, BoolFieldAllCapitalTrue) {
  EXPECT_THAT(RecommendedTest("BoolFieldAllCapitalTrue")
                  .ParseJson(message(), R"({"optionalBool":TRUE})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonBoolTest, BoolFieldAllCapitalFalse) {
  EXPECT_THAT(RecommendedTest("BoolFieldAllCapitalFalse")
                  .ParseJson(message(), R"({"optionalBool":FALSE})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonBoolTest, BoolFieldDoubleQuotedTrue) {
  EXPECT_THAT(RecommendedTest("BoolFieldDoubleQuotedTrue")
                  .ParseJson(message(), R"({"optionalBool":"true"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(JsonBoolTest, BoolFieldDoubleQuotedFalse) {
  EXPECT_THAT(RecommendedTest("BoolFieldDoubleQuotedFalse")
                  .ParseJson(message(), R"({"optionalBool":"false"})")
                  .ParseOnly(),
              Yields(IsParseError()));
}

INSTANTIATE_TEST_SUITE_P(All, JsonBoolTest, ValuesIn(AllTestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
