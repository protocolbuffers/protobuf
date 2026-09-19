// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Binary conformance tests checking that a oneof member set to its default
// value (zero, empty, or an empty message) survives a round trip to binary and
// to JSON: unlike a plain proto3 field, a oneof member always has presence.
// This holds both legs of the legacy
// BinaryAndJsonConformanceSuiteImpl<M>::TestOneofMessage(); the test names and
// the requests sent to the testee are identical to the legacy ones.
//
// The legacy test set the members one after another on a single generated
// message and sent its serialization each time.  Since all of them belong to
// the same oneof, each step left only the member just set, so every test here
// sends the wire encoding of just that member.  The oneof members have the
// same field numbers (111-119) in every TestAllTypes message.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "conformance/binary_test_util.h"
#include "conformance/binary_wireformat.h"
#include "conformance/matchers.h"
#include "conformance/message_type_fixtures.h"
#include "conformance/test_environment.h"
#include "conformance/testee.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::ValuesIn;

class OneofZeroTest : public MessageTypeConformanceTest {
 public:
  TestPriority DefaultPriority() const override { return kP3; }

 protected:
  // Whether NestedMessage.a has explicit presence (proto2) rather than
  // implicit presence (proto3).
  bool NestedMessageFieldHasPresence() const {
    return message()
        ->FindFieldByName("oneof_nested_message")
        ->message_type()
        ->FindFieldByName("a")
        ->has_presence();
  }
};

// Field 111 = oneof_uint32.
TEST_P(OneofZeroTest, Uint32) {
  EXPECT_THAT(Testee("OneofZeroUint32")
                  .ParseBinary(message(), VarintField(111, 0))
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(R"pb(oneof_uint32: 0)pb"))));
}

TEST_P(OneofZeroTest, Uint32Json) {
  EXPECT_THAT(Testee("OneofZeroUint32")
                  .ParseBinary(message(), VarintField(111, 0))
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(R"pb(oneof_uint32: 0)pb"))));
}

// Field 112 = oneof_nested_message, whose field 1 is a.  The legacy test set a
// to 0: with explicit presence (proto2) a is serialized and must round trip;
// with implicit presence (proto3) it is indistinguishable from unset, so the
// input is the empty message and only that is expected back.
TEST_P(OneofZeroTest, Message) {
  const bool has_presence = NestedMessageFieldHasPresence();
  EXPECT_THAT(Testee("OneofZeroMessage")
                  .ParseBinary(message(),
                               has_presence
                                   ? LengthPrefixedField(112, VarintField(1, 0))
                                   : LengthPrefixedField(112, Wire()))
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(
                  has_presence ? R"pb(oneof_nested_message: { a: 0 })pb"
                               : R"pb(oneof_nested_message: {})pb"))));
}

TEST_P(OneofZeroTest, MessageJson) {
  const bool has_presence = NestedMessageFieldHasPresence();
  EXPECT_THAT(Testee("OneofZeroMessage")
                  .ParseBinary(message(),
                               has_presence
                                   ? LengthPrefixedField(112, VarintField(1, 0))
                                   : LengthPrefixedField(112, Wire()))
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(
                  has_presence ? R"pb(oneof_nested_message: { a: 0 })pb"
                               : R"pb(oneof_nested_message: {})pb"))));
}

// Field 112 = oneof_nested_message.  The legacy test set a to 0 and then to 1
// on the same nested message.
TEST_P(OneofZeroTest, MessageSetTwice) {
  EXPECT_THAT(
      Testee("OneofZeroMessageSetTwice")
          .ParseBinary(message(), LengthPrefixedField(112, VarintField(1, 1)))
          .SerializeBinary(),
      Yields(ParsedPayload(
          EqualsTextProto(R"pb(oneof_nested_message: { a: 1 })pb"))));
}

TEST_P(OneofZeroTest, MessageSetTwiceJson) {
  EXPECT_THAT(
      Testee("OneofZeroMessageSetTwice")
          .ParseBinary(message(), LengthPrefixedField(112, VarintField(1, 1)))
          .SerializeJson(),
      Yields(ParsedPayload(
          EqualsTextProto(R"pb(oneof_nested_message: { a: 1 })pb"))));
}

// Field 113 = oneof_string.
TEST_P(OneofZeroTest, String) {
  EXPECT_THAT(
      Testee("OneofZeroString")
          .ParseBinary(message(), LengthPrefixedField(113, ""))
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(R"pb(oneof_string: "")pb"))));
}

TEST_P(OneofZeroTest, StringJson) {
  EXPECT_THAT(
      Testee("OneofZeroString")
          .ParseBinary(message(), LengthPrefixedField(113, ""))
          .SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(R"pb(oneof_string: "")pb"))));
}

// Field 114 = oneof_bytes.
TEST_P(OneofZeroTest, Bytes) {
  EXPECT_THAT(Testee("OneofZeroBytes")
                  .ParseBinary(message(), LengthPrefixedField(114, ""))
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(R"pb(oneof_bytes: "")pb"))));
}

TEST_P(OneofZeroTest, BytesJson) {
  EXPECT_THAT(Testee("OneofZeroBytes")
                  .ParseBinary(message(), LengthPrefixedField(114, ""))
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(R"pb(oneof_bytes: "")pb"))));
}

// Field 115 = oneof_bool.
TEST_P(OneofZeroTest, Bool) {
  EXPECT_THAT(
      Testee("OneofZeroBool")
          .ParseBinary(message(), VarintField(115, 0))
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(R"pb(oneof_bool: false)pb"))));
}

TEST_P(OneofZeroTest, BoolJson) {
  EXPECT_THAT(
      Testee("OneofZeroBool")
          .ParseBinary(message(), VarintField(115, 0))
          .SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(R"pb(oneof_bool: false)pb"))));
}

// Field 116 = oneof_uint64.
TEST_P(OneofZeroTest, Uint64) {
  EXPECT_THAT(Testee("OneofZeroUint64")
                  .ParseBinary(message(), VarintField(116, 0))
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(R"pb(oneof_uint64: 0)pb"))));
}

TEST_P(OneofZeroTest, Uint64Json) {
  EXPECT_THAT(Testee("OneofZeroUint64")
                  .ParseBinary(message(), VarintField(116, 0))
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(R"pb(oneof_uint64: 0)pb"))));
}

// Field 117 = oneof_float.
TEST_P(OneofZeroTest, Float) {
  EXPECT_THAT(Testee("OneofZeroFloat")
                  .ParseBinary(message(), FloatField(117, 0))
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(R"pb(oneof_float: 0)pb"))));
}

TEST_P(OneofZeroTest, FloatJson) {
  EXPECT_THAT(Testee("OneofZeroFloat")
                  .ParseBinary(message(), FloatField(117, 0))
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(R"pb(oneof_float: 0)pb"))));
}

// Field 118 = oneof_double.
TEST_P(OneofZeroTest, Double) {
  EXPECT_THAT(Testee("OneofZeroDouble")
                  .ParseBinary(message(), DoubleField(118, 0))
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(R"pb(oneof_double: 0)pb"))));
}

TEST_P(OneofZeroTest, DoubleJson) {
  EXPECT_THAT(Testee("OneofZeroDouble")
                  .ParseBinary(message(), DoubleField(118, 0))
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(R"pb(oneof_double: 0)pb"))));
}

// Field 119 = oneof_enum; FOO is the zero value of NestedEnum.
TEST_P(OneofZeroTest, Enum) {
  EXPECT_THAT(Testee("OneofZeroEnum")
                  .ParseBinary(message(), VarintField(119, 0))
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(R"pb(oneof_enum: FOO)pb"))));
}

TEST_P(OneofZeroTest, EnumJson) {
  EXPECT_THAT(Testee("OneofZeroEnum")
                  .ParseBinary(message(), VarintField(119, 0))
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(R"pb(oneof_enum: FOO)pb"))));
}

INSTANTIATE_TEST_SUITE_P(All, OneofZeroTest, ValuesIn(AllTestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
