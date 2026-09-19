// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Binary conformance tests checking that invalid UTF-8 in a string field is
// rejected, and that a bytes field accepts it.  This replaces the legacy
// BinaryAndJsonConformanceSuiteImpl<M>::TestInvalidUtf8String() (which only
// ran for the proto3-style message types) and the suite-level
// BinaryAndJsonConformanceSuite::RunUtf8ValidationTests(); the test names and
// the requests sent to the testee are identical to the legacy ones.
//
// TODO: b/477413000 - Once conformance tests can express failures that are
// not expected to be fixed, add proto2 coverage here.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "binary_test_util.h"
#include "binary_wireformat.h"
#include "matchers.h"
#include "message_type_fixtures.h"
#include "test_environment.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::ValuesIn;

// Four bytes that aren't valid UTF-8.
constexpr absl::string_view kInvalidUtf8 = "\xA0\xB0\xC0\xD0";

// Instantiated over the proto3-style test message types only: proto3 rejects
// invalid UTF-8 in string fields, proto2 doesn't validate it.
using Utf8StringTest = MessageTypeConformanceTest;

// Field 14 is optional_string.
TEST_P(Utf8StringTest, Singular) {
  EXPECT_THAT(RecommendedTest("RejectInvalidUtf8.String.Singular")
                  .ParseBinary(message(), LengthPrefixedField(14, kInvalidUtf8))
                  .ParseOnly(),
              Yields(IsParseError()));
}

// Field 44 is repeated_string.
TEST_P(Utf8StringTest, Repeated) {
  EXPECT_THAT(RecommendedTest("RejectInvalidUtf8.String.Repeated")
                  .ParseBinary(message(), LengthPrefixedField(44, kInvalidUtf8))
                  .ParseOnly(),
              Yields(IsParseError()));
}

// Field 113 is oneof_string.
TEST_P(Utf8StringTest, Oneof) {
  EXPECT_THAT(
      RecommendedTest("RejectInvalidUtf8.String.Oneof")
          .ParseBinary(message(), LengthPrefixedField(113, kInvalidUtf8))
          .ParseOnly(),
      Yields(IsParseError()));
}

// Field 69 is map_string_string; its entries have key = 1 and value = 2.
TEST_P(Utf8StringTest, MapKey) {
  EXPECT_THAT(
      RecommendedTest("RejectInvalidUtf8.String.MapKey")
          .ParseBinary(
              message(),
              LengthPrefixedField(69, Wire(LengthPrefixedField(1, kInvalidUtf8),
                                           LengthPrefixedField(2, "foo"))))
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(Utf8StringTest, MapValue) {
  EXPECT_THAT(
      RecommendedTest("RejectInvalidUtf8.String.MapValue")
          .ParseBinary(message(),
                       LengthPrefixedField(
                           69, Wire(LengthPrefixedField(1, "foo"),
                                    LengthPrefixedField(2, kInvalidUtf8))))
          .ParseOnly(),
      Yields(IsParseError()));
}

INSTANTIATE_TEST_SUITE_P(All, Utf8StringTest,
                         ValuesIn(Proto3TestMessageTypes()),
                         MessageTypeParamName);

// The extension counterparts only exist for TestAllTypesEdition2023, where
// field 133 is the extension_string extension and field 134 is the
// extension_bytes extension.
using Utf8ExtensionTest = Edition2023ConformanceTest;

TEST_F(Utf8ExtensionTest, String) {
  EXPECT_THAT(
      RecommendedTest("RejectInvalidUtf8.String.Extension")
          .ParseBinary(message(), LengthPrefixedField(133, kInvalidUtf8))
          .ParseOnly(),
      Yields(IsParseError()));
}

// Bytes fields aren't validated, so the same bytes must be accepted.
TEST_F(Utf8ExtensionTest, Bytes) {
  EXPECT_THAT(
      RequiredTest("AcceptInvalidUtf8.Bytes.Extension")
          .ParseBinary(message(), LengthPrefixedField(134, kInvalidUtf8))
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(
          R"pb([protobuf_test_messages.editions
                    .extension_bytes]: "\xA0\xB0\xC0\xD0")pb"))));
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
