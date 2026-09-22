// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Binary conformance tests for TestAllTypesEdition2023: length-prefixed and
// delimited (group-encoded) message fields, maps, and delimited extensions
// under editions.  This holds both the binary-output and the JSON-output leg
// of the legacy suite-level
// BinaryAndJsonConformanceSuite::RunDelimitedFieldTests(); the requests sent to
// the testee are identical to the legacy ones.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "conformance/binary_wireformat.h"
#include "conformance/matchers.h"
#include "conformance/message_type_fixtures.h"
#include "conformance/test_environment.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

// Every test here parses TestAllTypesEdition2023 (message()), so the fixtures
// skip themselves when --maximum_edition doesn't cover editions.
using DelimitedFieldTest = Edition2023ConformanceTest;

// Field 1 is optional_int32.
TEST_F(DelimitedFieldTest, ValidNonMessage) {
  EXPECT_THAT(
      Testee().ParseBinary(message(), VarintField(1, 99)).SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(R"pb(optional_int32: 99)pb"))));
}

TEST_F(DelimitedFieldTest, ValidNonMessageJson) {
  EXPECT_THAT(
      Testee().ParseBinary(message(), VarintField(1, 99)).SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(R"pb(optional_int32: 99)pb"))));
}

// Field 18 is optional_nested_message, whose field 1 is a.
TEST_F(DelimitedFieldTest, ValidLengthPrefixedField) {
  EXPECT_THAT(
      Testee()
          .ParseBinary(message(), LengthPrefixedField(18, VarintField(1, 99)))
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(R"pb(optional_nested_message {
                                                  a: 99
                                                })pb"))));
}

TEST_F(DelimitedFieldTest, ValidLengthPrefixedFieldJson) {
  EXPECT_THAT(
      Testee()
          .ParseBinary(message(), LengthPrefixedField(18, VarintField(1, 99)))
          .SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(R"pb(optional_nested_message {
                                                  a: 99
                                                })pb"))));
}

// Field 56 is map_int32_int32; map entries have key = 1 and value = 2.
TEST_F(DelimitedFieldTest, ValidMapInteger) {
  EXPECT_THAT(
      Testee()
          .ParseBinary(message(),
                       LengthPrefixedField(
                           56, Wire(VarintField(1, 99), VarintField(2, 87))))
          .SerializeBinary(),
      Yields(ParsedPayload(
          EqualsTextProto(R"pb(map_int32_int32 { key: 99 value: 87 })pb"))));
}

TEST_F(DelimitedFieldTest, ValidMapIntegerJson) {
  EXPECT_THAT(
      Testee()
          .ParseBinary(message(),
                       LengthPrefixedField(
                           56, Wire(VarintField(1, 99), VarintField(2, 87))))
          .SerializeJson(),
      Yields(ParsedPayload(
          EqualsTextProto(R"pb(map_int32_int32 { key: 99 value: 87 })pb"))));
}

// Field 71 is map_string_nested_message.
TEST_F(DelimitedFieldTest, ValidMapLengthPrefixed) {
  EXPECT_THAT(
      Testee()
          .ParseBinary(message(), LengthPrefixedField(
                                      71, Wire(LengthPrefixedField(1, "a"),
                                               LengthPrefixedField(
                                                   2, VarintField(1, 87)))))
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(R"pb(map_string_nested_message {
                                                  key: "a"
                                                  value: { a: 87 }
                                                })pb"))));
}

TEST_F(DelimitedFieldTest, ValidMapLengthPrefixedJson) {
  EXPECT_THAT(
      Testee()
          .ParseBinary(message(), LengthPrefixedField(
                                      71, Wire(LengthPrefixedField(1, "a"),
                                               LengthPrefixedField(
                                                   2, VarintField(1, 87)))))
          .SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(R"pb(map_string_nested_message {
                                                  key: "a"
                                                  value: { a: 87 }
                                                })pb"))));
}

// Field 201 is groupliketype (whose name matches its type, as a proto2 group
// would) and field 202 is delimited_field (whose name doesn't).  Both are
// GroupLikeType, whose field 202 is group_int32.
TEST_F(DelimitedFieldTest, GroupLike) {
  EXPECT_THAT(
      Testee()
          .ParseBinary(message(), DelimitedField(201, VarintField(202, 99)))
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(R"pb(groupliketype {
                                                  group_int32: 99
                                                })pb"))));
}

TEST_F(DelimitedFieldTest, GroupLikeJson) {
  EXPECT_THAT(
      Testee()
          .ParseBinary(message(), DelimitedField(201, VarintField(202, 99)))
          .SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(R"pb(groupliketype {
                                                  group_int32: 99
                                                })pb"))));
}

TEST_F(DelimitedFieldTest, NotGroupLike) {
  EXPECT_THAT(
      Testee()
          .ParseBinary(message(), DelimitedField(202, VarintField(202, 99)))
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(R"pb(delimited_field {
                                                  group_int32: 99
                                                })pb"))));
}

TEST_F(DelimitedFieldTest, NotGroupLikeJson) {
  EXPECT_THAT(
      Testee()
          .ParseBinary(message(), DelimitedField(202, VarintField(202, 99)))
          .SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(R"pb(delimited_field {
                                                  group_int32: 99
                                                })pb"))));
}

// Only TestAllTypesEdition2023 declares delimited extensions: field 121 is
// the groupliketype extension (whose name matches its type, as a proto2 group
// would) and field 122 is delimited_ext (whose name doesn't).  Both are
// GroupLikeType, whose field 1 is c.  Like the legacy suite, these have no
// JSON-output leg.
using DelimitedExtensionTest = Edition2023ConformanceTest;

TEST_F(DelimitedExtensionTest, GroupLike) {
  EXPECT_THAT(
      Testee()
          .ParseBinary(message(), DelimitedField(121, VarintField(1, 99)))
          .SerializeBinary(),
      Yields(ParsedPayload(
          EqualsTextProto(R"pb([protobuf_test_messages.editions.groupliketype] {
                                 c: 99
                               })pb"))));
}

TEST_F(DelimitedExtensionTest, NotGroupLike) {
  EXPECT_THAT(
      Testee()
          .ParseBinary(message(), DelimitedField(122, VarintField(1, 99)))
          .SerializeBinary(),
      Yields(ParsedPayload(
          EqualsTextProto(R"pb([protobuf_test_messages.editions.delimited_ext] {
                                 c: 99
                               })pb"))));
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
