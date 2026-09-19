// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Binary conformance tests for the MessageSet wire format: well-formed items,
// items in unusual orders, duplicated entries, and the plain-submessage
// encoding of an item.  This replaces the legacy suite-level
// BinaryAndJsonConformanceSuite::RunMessageSetTests(); the test names and the
// requests sent to the testee are identical to the legacy ones.
//
// These tests only exist for TestAllTypesProto2, the one test message type
// with a message_set_wire_format field.

#include <cstdint>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "binary_wireformat.h"
#include "matchers.h"
#include "test_environment.h"
#include "google/protobuf/test_messages_proto2.pb.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::protobuf_test_messages::proto2::TestAllTypesProto2;

// Field 500 of TestAllTypesProto2 is message_set_correct, whose type has
// message_set_wire_format.  Such a message is a sequence of items, each a
// group (field 1) holding the extension's field number as its type_id (field
// 2, varint) and the extension message itself (field 3, length-prefixed).
constexpr uint32_t kMessageSetField = 500;
constexpr uint32_t kItemField = 1;
constexpr uint32_t kTypeIdField = 2;
constexpr uint32_t kMessageField = 3;

// The extension numbers, i.e. type_ids, of the MessageSetCorrect extensions
// declared in test_messages_proto2.proto, and one that isn't declared.
constexpr uint32_t kMessageSetCorrectExtension1TypeId = 1547769;
constexpr uint32_t kMessageSetCorrectExtension2TypeId = 4135312;
constexpr uint32_t kExtensionWithOneofTypeId = 123456789;
constexpr uint32_t kUnknownTypeId = 4135300;

// Field 9 of MessageSetCorrectExtension2 is i.
constexpr uint32_t kExtension2IField = 9;

// The expected result of every test whose payload decodes to
// MessageSetCorrectExtension2 { i: 99 }.
constexpr absl::string_view kMessageSetCorrectExtension2Text =
    R"pb(message_set_correct: {
           [protobuf_test_messages.proto2.TestAllTypesProto2
                .MessageSetCorrectExtension2]: { i: 99 }
         })pb";

// The message_set_correct field, holding the items `content`.
Wire MessageSet(const Wire& content) {
  return LengthPrefixedField(kMessageSetField, content);
}

// A MessageSet item (group 1) with the entries `content`, in that order.
Wire Item(const Wire& content) { return DelimitedField(kItemField, content); }

// The type_id entry of a MessageSet item.
Wire TypeId(uint32_t type_id) { return VarintField(kTypeIdField, type_id); }

// The message entry of a MessageSet item.
Wire ItemMessage(const Wire& message) {
  return LengthPrefixedField(kMessageField, message);
}

// A serialized MessageSetCorrectExtension2 with i set to `i`.
Wire Extension2Message(int i) { return VarintField(kExtension2IField, i); }

using MessageSetTest = ConformanceTest;

TEST_F(MessageSetTest, ValidMessageSetEncoding) {
  EXPECT_THAT(
      RequiredTest("ValidMessageSetEncoding")
          .ParseBinary(
              TestAllTypesProto2::descriptor(),
              MessageSet(Item(Wire(TypeId(kMessageSetCorrectExtension2TypeId),
                                   ItemMessage(Extension2Message(99))))))
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(kMessageSetCorrectExtension2Text))));
}

TEST_F(MessageSetTest, OutOfOrderGroupsEntries) {
  EXPECT_THAT(
      RequiredTest("ValidMessageSetEncoding.OutOfOrderGroupsEntries")
          .ParseBinary(TestAllTypesProto2::descriptor(),
                       MessageSet(Item(
                           Wire(ItemMessage(Extension2Message(99)),
                                TypeId(kMessageSetCorrectExtension2TypeId)))))
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(kMessageSetCorrectExtension2Text))));
}

// Test that an unknown message set extension always goes to unknown fields.
// This is done by poisoning the extension payload with an entry for field 0.
TEST_F(MessageSetTest, UnknownExtension) {
  Wire input = MessageSet(
      Item(Wire(TypeId(kUnknownTypeId), ItemMessage(VarintField(0, 99)))));
  EXPECT_THAT(RequiredTest("MessageSetEncoding.UnknownExtension")
                  .ParseBinary(TestAllTypesProto2::descriptor(), input)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsBinaryProto(input))));
}

// If an encoder is unaware of the message_set_wire_format option it will be
// encoded like any other extension submessage. Decoders should be able to
// tolerate this format as well.
TEST_F(MessageSetTest, SubmessageEncoding) {
  EXPECT_THAT(
      RecommendedTest("ValidMessageSetEncoding.SubmessageEncoding")
          .ParseBinary(
              TestAllTypesProto2::descriptor(),
              MessageSet(LengthPrefixedField(kMessageSetCorrectExtension2TypeId,
                                             Extension2Message(99))))
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(kMessageSetCorrectExtension2Text))));
}

// Test again, but this time we'll try to detect if the implementation put the
// submessage encoded entry into the unknown field set. We'll do this by using
// conflicting oneof entries where order matters when the messages are merged.
//
// In a non-compliant implementation submessage encoded messageset entry will
// be moved to unknown fields and then tacked onto the end of the payload.
// Thus we'll see field b set first, and then field a.
//
// In a compliant implementation we expect the submessage encoded messageset
// to be read first with field a set, and then the normal message set entry
// will be read with field b will be set -- thus field b will win.
TEST_F(MessageSetTest, SubmessageEncodingNotUnknown) {
  // Fields 1 and 2 of ExtensionWithOneof are a and b.
  EXPECT_THAT(
      RecommendedTest("ValidMessageSetEncoding.SubmessageEncoding.NotUnknown")
          .ParseBinary(
              TestAllTypesProto2::descriptor(),
              MessageSet(Wire(LengthPrefixedField(kExtensionWithOneofTypeId,
                                                  VarintField(1, 42)),
                              Item(Wire(TypeId(kExtensionWithOneofTypeId),
                                        ItemMessage(VarintField(2, 99)))))))
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(
          R"pb(message_set_correct: {
                 [protobuf_test_messages.proto2.TestAllTypesProto2
                      .ExtensionWithOneof]: { b: 99 }
               })pb"))));
}

// [type_id, value, type_id (different)] -> first type_id and value honored.
TEST_F(MessageSetTest, DuplicateDifferentTypeId) {
  EXPECT_THAT(
      RecommendedTest("ValidMessageSetEncoding.DuplicateDifferentTypeId")
          .ParseBinary(TestAllTypesProto2::descriptor(),
                       MessageSet(Item(
                           Wire(TypeId(kMessageSetCorrectExtension2TypeId),
                                ItemMessage(Extension2Message(99)),
                                TypeId(kMessageSetCorrectExtension1TypeId)))))
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(kMessageSetCorrectExtension2Text))));
}

// [type_id, value, value] -> first value honored, no merge.
TEST_F(MessageSetTest, DuplicateValue) {
  EXPECT_THAT(
      RecommendedTest("ValidMessageSetEncoding.DuplicateValue")
          .ParseBinary(
              TestAllTypesProto2::descriptor(),
              MessageSet(Item(Wire(TypeId(kMessageSetCorrectExtension2TypeId),
                                   ItemMessage(Extension2Message(99)),
                                   ItemMessage(Extension2Message(88))))))
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(kMessageSetCorrectExtension2Text))));
}

// [value, type_id, value] -> first value honored, no merge.
TEST_F(MessageSetTest, DuplicateValueOutOfOrder) {
  EXPECT_THAT(
      RecommendedTest("ValidMessageSetEncoding.DuplicateValueOutOfOrder")
          .ParseBinary(
              TestAllTypesProto2::descriptor(),
              MessageSet(Item(Wire(ItemMessage(Extension2Message(99)),
                                   TypeId(kMessageSetCorrectExtension2TypeId),
                                   ItemMessage(Extension2Message(88))))))
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(kMessageSetCorrectExtension2Text))));
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
