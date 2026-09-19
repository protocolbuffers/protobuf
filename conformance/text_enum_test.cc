// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Text-format conformance tests for enum fields given by number.  An open
// enum (proto3-style message types) keeps any number, a closed enum
// (proto2-style message types) rejects numbers that aren't one of its values.
// This replaces the legacy TextFormatConformanceTestSuiteImpl<M>::
// RunOpenEnumTests() and RunClosedEnumTests(); the test names and the
// requests sent to the testee are identical to the legacy ones.

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

// TODO: Write the plain-string inputs below as R"pb(...)pb" raw
// strings once the migration's request goldens are gone.  The goldens pin the
// exact request bytes, and clang-format would reformat the whitespace of these
// inputs inside a pb raw string.
constexpr absl::string_view kKnownNumberInput =
    "\n        optional_nested_enum: 1\n        ";
constexpr absl::string_view kUnknownNumberInput =
    "\n        optional_nested_enum: 42\n        ";

using TextOpenEnumTest = MessageTypeConformanceTest;

TEST_P(TextOpenEnumTest, ByNumber) {
  EXPECT_THAT(Testee("ClosedEnumFieldByNumber")
                  .ParseText(message(), kKnownNumberInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kKnownNumberInput))));
  EXPECT_THAT(Testee("ClosedEnumFieldByNumber")
                  .ParseText(message(), kKnownNumberInput)
                  .SerializeText(),
              Yields(WhenParsed(EqualsTextProto(kKnownNumberInput))));
}

TEST_P(TextOpenEnumTest, WithUnknownNumber) {
  EXPECT_THAT(Testee("ClosedEnumFieldWithUnknownNumber")
                  .ParseText(message(), kUnknownNumberInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kUnknownNumberInput))));
  EXPECT_THAT(Testee("ClosedEnumFieldWithUnknownNumber")
                  .ParseText(message(), kUnknownNumberInput)
                  .SerializeText(),
              Yields(WhenParsed(EqualsTextProto(kUnknownNumberInput))));
}

INSTANTIATE_TEST_SUITE_P(All, TextOpenEnumTest,
                         ValuesIn(Proto3TestMessageTypes()),
                         MessageTypeParamName);

using TextClosedEnumTest = MessageTypeConformanceTest;

TEST_P(TextClosedEnumTest, ByNumber) {
  EXPECT_THAT(Testee("ClosedEnumFieldByNumber")
                  .ParseText(message(), kKnownNumberInput)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsTextProto(kKnownNumberInput))));
  EXPECT_THAT(Testee("ClosedEnumFieldByNumber")
                  .ParseText(message(), kKnownNumberInput)
                  .SerializeText(),
              Yields(WhenParsed(EqualsTextProto(kKnownNumberInput))));
}

TEST_P(TextClosedEnumTest, WithUnknownNumber) {
  EXPECT_THAT(Testee("ClosedEnumFieldWithUnknownNumber")
                  .ParseText(message(), kUnknownNumberInput)
                  .ParseOnly(),
              Yields(IsParseError()));
}

INSTANTIATE_TEST_SUITE_P(All, TextClosedEnumTest,
                         ValuesIn(Proto2TestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
