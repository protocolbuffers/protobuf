// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// JSON conformance tests for (singular) message fields: a nested message
// given as a JSON object.  This replaces the "Message fields" block of the
// legacy
// BinaryAndJsonConformanceSuiteImpl<M>::RunJsonTestsForNonRepeatedTypes(); the
// test name and the requests sent to the testee are identical to the legacy
// ones.
//
// Like the legacy RunValidJsonTest(), the valid input is sent twice: once to
// be serialized as binary ("<name>.ProtobufOutput") and once as JSON
// ("<name>.JsonOutput").

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

using JsonMessageTest = MessageTypeConformanceTest;

TEST_P(JsonMessageTest, MessageField) {
  constexpr absl::string_view kInput =
      R"json({"optionalNestedMessage": {"a": 1234}})json";
  constexpr absl::string_view kExpected =
      R"pb(optional_nested_message: { a: 1234 })pb";
  EXPECT_THAT(
      Testee("MessageField").ParseJson(message(), kInput).SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
  EXPECT_THAT(
      Testee("MessageField").ParseJson(message(), kInput).SerializeJson(),
      Yields(WhenParsed(EqualsTextProto(kExpected))));
}

INSTANTIATE_TEST_SUITE_P(All, JsonMessageTest, ValuesIn(AllTestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
