// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Binary conformance tests checking that group tags without a matching start
// or end tag, or with mismatched field numbers, are rejected.  This replaces
// the legacy BinaryAndJsonConformanceSuiteImpl<M>::TestUnmatchedGroup(); the
// test names and the requests sent to the testee are identical to the legacy
// ones.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
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

using UnmatchedGroupTest = MessageTypeConformanceTest;

TEST_P(UnmatchedGroupTest, UnmatchedEndGroup) {
  EXPECT_THAT(RequiredTest("UnmatchedEndGroup")
                  .ParseBinary(message(), Tag(201, WireType::kEndGroup))
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(UnmatchedGroupTest, UnmatchedEndGroupUnknown) {
  EXPECT_THAT(RequiredTest("UnmatchedEndGroupUnknown")
                  .ParseBinary(message(), Tag(1234, WireType::kEndGroup))
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(UnmatchedGroupTest, UnmatchedEndGroupWrongType) {
  EXPECT_THAT(RequiredTest("UnmatchedEndGroupWrongType")
                  .ParseBinary(message(), Tag(1, WireType::kEndGroup))
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(UnmatchedGroupTest, UnmatchedEndGroupNestedLen) {
  EXPECT_THAT(
      RequiredTest("UnmatchedEndGroupNestedLen")
          .ParseBinary(message(),
                       LengthPrefixedField(18, Tag(1234, WireType::kEndGroup)))
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(UnmatchedGroupTest, UnmatchedEndGroupNested) {
  EXPECT_THAT(
      RequiredTest("UnmatchedEndGroupNested")
          .ParseBinary(message(),
                       DelimitedField(201, Tag(202, WireType::kEndGroup)))
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(UnmatchedGroupTest, UnmatchedEndGroupWithData) {
  EXPECT_THAT(
      RequiredTest("UnmatchedEndGroupWithData")
          .ParseBinary(message(), Wire(Tag(1, WireType::kEndGroup),
                                       LengthPrefixedField(2, "hello world")))
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(UnmatchedGroupTest, UnmatchedStartGroup) {
  EXPECT_THAT(RequiredTest("UnmatchedStartGroup")
                  .ParseBinary(message(), Tag(201, WireType::kStartGroup))
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(UnmatchedGroupTest, UnmatchedStartGroupUnknown) {
  EXPECT_THAT(RequiredTest("UnmatchedStartGroupUnknown")
                  .ParseBinary(message(), Tag(1234, WireType::kStartGroup))
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(UnmatchedGroupTest, UnmatchedStartGroupWrongType) {
  EXPECT_THAT(RequiredTest("UnmatchedStartGroupWrongType")
                  .ParseBinary(message(), Tag(1, WireType::kStartGroup))
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(UnmatchedGroupTest, UnmatchedStartGroupNestedLen) {
  EXPECT_THAT(
      RequiredTest("UnmatchedStartGroupNestedLen")
          .ParseBinary(message(), LengthPrefixedField(
                                      18, Tag(1234, WireType::kStartGroup)))
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(UnmatchedGroupTest, UnmatchedStartGroupNested) {
  EXPECT_THAT(
      RequiredTest("UnmatchedStartGroupNested")
          .ParseBinary(message(),
                       DelimitedField(201, Tag(202, WireType::kStartGroup)))
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(UnmatchedGroupTest, UnmatchedStartGroupWithData) {
  EXPECT_THAT(
      RequiredTest("UnmatchedStartGroupWithData")
          .ParseBinary(message(), Wire(Tag(1, WireType::kStartGroup),
                                       LengthPrefixedField(2, "hello world")))
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(UnmatchedGroupTest, MismatchedGroupTags) {
  EXPECT_THAT(
      RequiredTest("MismatchedGroupTags")
          .ParseBinary(message(), Wire(Tag(201, WireType::kStartGroup),
                                       LengthPrefixedField(2, "hello world"),
                                       Tag(202, WireType::kEndGroup)))
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(UnmatchedGroupTest, MismatchedNestedGroupTags) {
  EXPECT_THAT(
      RequiredTest("MismatchedNestedGroupTags")
          .ParseBinary(
              message(),
              DelimitedField(201, Wire(Tag(202, WireType::kStartGroup),
                                       LengthPrefixedField(2, "hello world"),
                                       Tag(203, WireType::kEndGroup))))
          .ParseOnly(),
      Yields(IsParseError()));
}

INSTANTIATE_TEST_SUITE_P(All, UnmatchedGroupTest,
                         ValuesIn(AllTestMessageTypes()), MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
