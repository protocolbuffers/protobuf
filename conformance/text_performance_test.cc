// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Text-format performance conformance tests: tens of thousands of occurrences
// of recursive_message, each holding one repeated field entry, that have to be
// merged into a single recursive_message.  Part of the performance suite
// (see conformance.bzl).  This replaces the legacy
// TextFormatConformanceTestSuiteImpl<M>::RunTextFormatPerformanceTests() and
// its helper, which ran for the four TestAllTypes{Proto2,Proto3} message types
// (proto and editions flavours); the test names and the requests sent to the
// testee are identical to the legacy ones.

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
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

// The number of repetitions to use for performance tests.
// Corresponds approx to 500KB wireformat bytes.
constexpr int kPerformanceRepeatCount = 50000;

// The input of the legacy
// TestTextFormatPerformanceMergeMessageWithRepeatedField() test for `field`,
// one entry of a repeated field of recursive_message: "recursive_message {
// <field> }" repeated kPerformanceRepeatCount times.
std::string RepeatedRecursiveMessageInput(absl::string_view field) {
  const std::string recursive_message =
      absl::StrCat("recursive_message { ", field, " }");
  std::string input;
  for (int i = 0; i < kPerformanceRepeatCount; ++i) {
    absl::StrAppend(&input, recursive_message);
  }
  return input;
}

// The expected result of parsing RepeatedRecursiveMessageInput(field): a
// single recursive_message holding all kPerformanceRepeatCount entries.
std::string MergedRecursiveMessage(absl::string_view field) {
  std::string expected = "recursive_message { ";
  for (int i = 0; i < kPerformanceRepeatCount; ++i) {
    absl::StrAppend(&expected, field, " ");
  }
  absl::StrAppend(&expected, "}");
  return expected;
}

// Parameterized over the test message type only.
class TextPerformanceTest : public MessageTypeConformanceTest {
 public:
  TestPriority DefaultPriority() const override { return kP3; }
};

TEST_P(TextPerformanceTest, MergeMessageWithRepeatedFieldBool) {
  constexpr absl::string_view kName =
      "TestTextFormatPerformanceMergeMessageWithRepeatedFieldBool";
  const std::string input =
      RepeatedRecursiveMessageInput("repeated_bool: true");
  const std::string expected = MergedRecursiveMessage("repeated_bool: true");
  EXPECT_THAT(Testee(kName).ParseText(message(), input).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(expected))));
  EXPECT_THAT(Testee(kName).ParseText(message(), input).SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(expected))));
}

TEST_P(TextPerformanceTest, MergeMessageWithRepeatedFieldDouble) {
  constexpr absl::string_view kName =
      "TestTextFormatPerformanceMergeMessageWithRepeatedFieldDouble";
  const std::string input =
      RepeatedRecursiveMessageInput("repeated_double: 123");
  const std::string expected = MergedRecursiveMessage("repeated_double: 123");
  EXPECT_THAT(Testee(kName).ParseText(message(), input).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(expected))));
  EXPECT_THAT(Testee(kName).ParseText(message(), input).SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(expected))));
}

// The legacy Int32/Int64 tests actually exercise repeated_uint32 and
// repeated_uint64; the names are kept so failure-list entries keep matching.
TEST_P(TextPerformanceTest, MergeMessageWithRepeatedFieldInt32) {
  constexpr absl::string_view kName =
      "TestTextFormatPerformanceMergeMessageWithRepeatedFieldInt32";
  const std::string input =
      RepeatedRecursiveMessageInput("repeated_uint32: 123");
  const std::string expected = MergedRecursiveMessage("repeated_uint32: 123");
  EXPECT_THAT(Testee(kName).ParseText(message(), input).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(expected))));
  EXPECT_THAT(Testee(kName).ParseText(message(), input).SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(expected))));
}

TEST_P(TextPerformanceTest, MergeMessageWithRepeatedFieldInt64) {
  constexpr absl::string_view kName =
      "TestTextFormatPerformanceMergeMessageWithRepeatedFieldInt64";
  const std::string input =
      RepeatedRecursiveMessageInput("repeated_uint64: 123");
  const std::string expected = MergedRecursiveMessage("repeated_uint64: 123");
  EXPECT_THAT(Testee(kName).ParseText(message(), input).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(expected))));
  EXPECT_THAT(Testee(kName).ParseText(message(), input).SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(expected))));
}

TEST_P(TextPerformanceTest, MergeMessageWithRepeatedFieldString) {
  constexpr absl::string_view kName =
      "TestTextFormatPerformanceMergeMessageWithRepeatedFieldString";
  const std::string input =
      RepeatedRecursiveMessageInput(R"(repeated_string: "foo")");
  const std::string expected =
      MergedRecursiveMessage(R"(repeated_string: "foo")");
  EXPECT_THAT(Testee(kName).ParseText(message(), input).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(expected))));
  EXPECT_THAT(Testee(kName).ParseText(message(), input).SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(expected))));
}

TEST_P(TextPerformanceTest, MergeMessageWithRepeatedFieldBytes) {
  constexpr absl::string_view kName =
      "TestTextFormatPerformanceMergeMessageWithRepeatedFieldBytes";
  const std::string input =
      RepeatedRecursiveMessageInput(R"(repeated_bytes: "foo")");
  const std::string expected =
      MergedRecursiveMessage(R"(repeated_bytes: "foo")");
  EXPECT_THAT(Testee(kName).ParseText(message(), input).SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(expected))));
  EXPECT_THAT(Testee(kName).ParseText(message(), input).SerializeText(),
              Yields(ParsedPayload(EqualsTextProto(expected))));
}

INSTANTIATE_TEST_SUITE_P(All, TextPerformanceTest,
                         ValuesIn(AllTestMessageTypes()), MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
