// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// The smoke test of the text-format conformance suite: a single string field
// round-trips through the testee.  This replaces the legacy
// TextFormatConformanceTestSuiteImpl<M>::RunAllTests()'s "HelloWorld" test,
// which ran for the proto3-style message types only; the test names and the
// requests sent to the testee are identical to the legacy ones.
//
// Like the legacy RunValidTextFormatTest(), every valid-input test here and in
// the other text_*_test.cc files has two legs, in the legacy order: the input
// is parsed and serialized as binary ("<name>.ProtobufOutput"), then parsed
// and serialized as text format ("<name>.TextFormatOutput"), and each result
// must be equivalent to the expected message.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "binary_test_util.h"
#include "matchers.h"
#include "message_type_fixtures.h"
#include "test_environment.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::ValuesIn;

using TextHelloWorldTest = MessageTypeConformanceTest;

TEST_P(TextHelloWorldTest, HelloWorld) {
  EXPECT_THAT(RequiredTest("HelloWorld")
                  .ParseText(message(), "optional_string: 'Hello, World!'")
                  .SerializeBinary(),
              Yields(ParsedPayload(
                  EqualsTextProto("optional_string: 'Hello, World!'"))));
  EXPECT_THAT(RequiredTest("HelloWorld")
                  .ParseText(message(), "optional_string: 'Hello, World!'")
                  .SerializeText(),
              Yields(ParsedPayload(
                  EqualsTextProto("optional_string: 'Hello, World!'"))));
}

INSTANTIATE_TEST_SUITE_P(All, TextHelloWorldTest,
                         ValuesIn(Proto3TestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
