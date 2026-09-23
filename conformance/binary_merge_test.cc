// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Binary conformance tests checking how several occurrences of the same field
// in one payload are combined: repeated submessages are merged, a repeated
// map entry replaces the previous value for its key, and repeated occurrences
// of a oneof message field are merged too.  This replaces the legacy
// BinaryAndJsonConformanceSuiteImpl<M>::TestValidDataForRepeatedScalarMessage(),
// TestOverwriteMessageValueMap() and TestMergeOneofMessage(); the test names
// and the requests sent to the testee are identical to the legacy ones.
//
// Like the legacy RunValidProtobufTest(), each input is sent twice: once to be
// serialized as binary ("<name>.ProtobufOutput") and once as JSON
// ("<name>.JsonOutput"), and both outputs must be equivalent to the same
// reference message.

#include <cstdint>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "conformance/binary_test_util.h"
#include "conformance/binary_wireformat.h"
#include "conformance/matchers.h"
#include "conformance/message_type_fixtures.h"
#include "conformance/test_environment.h"
#include "conformance/testee.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::ValuesIn;

// In every TestAllTypes message, NestedMessage's fields are a (1) and
// corecursive (2), and the TestAllTypes fields used inside the corecursive
// submessages of the merge-test inputs are optional_int32 (1),
// optional_int64 (2), optional_uint32 (3), repeated_int32 (31) and
// unpacked_int32 (89).
constexpr uint32_t kNestedMessageCorecursiveField = 2;
constexpr uint32_t kOptionalInt32Field = 1;
constexpr uint32_t kOptionalInt64Field = 2;
constexpr uint32_t kOptionalUint32Field = 3;
constexpr uint32_t kRepeatedInt32Field = 31;
constexpr uint32_t kUnpackedInt32Field = 89;

// A NestedMessage whose corecursive submessage is `corecursive`.
Wire NestedMessageWithCorecursive(const Wire& corecursive) {
  return LengthPrefixedField(kNestedMessageCorecursiveField, corecursive);
}

// The input of a merge test together with the single occurrence of the field
// that the parsed input must be equivalent to (and, for the byte-exact test,
// serialize to).
struct MergeTestData {
  Wire input;
  Wire expected;
};

// RepeatedScalarMessageMerge: `message`'s singular message field
// (optional_nested_message) twice, each holding a `corecursive` submessage
// that sets optional_int32, one of optional_int64 / optional_uint32, and
// repeated_int32.  A parser must merge the two submessages rather than
// replace the first with the second.
Wire RepeatedScalarMessageMergeInput(const Descriptor& message) {
  const uint32_t field = FieldNumber(*GetFieldForType(
      message, FieldDescriptor::TYPE_MESSAGE, /*repeated=*/false));
  return Wire(
      LengthPrefixedField(field, NestedMessageWithCorecursive(Wire(
                                     VarintField(kOptionalInt32Field, 1234),
                                     VarintField(kOptionalInt64Field, 1234),
                                     VarintField(kRepeatedInt32Field, 1234)))),
      LengthPrefixedField(field, NestedMessageWithCorecursive(Wire(
                                     VarintField(kOptionalInt32Field, 4321),
                                     VarintField(kOptionalUint32Field, 4321),
                                     VarintField(kRepeatedInt32Field, 4321)))));
}

// ValidDataMap.STRING.MESSAGE.MergeValue: two entries of `message`'s
// map_string_nested_message field with the same (empty) key and different
// message values.  A later map entry replaces an earlier one with the same
// key, so `expected` is the second entry alone.
MergeTestData MapMessageValueMergeData(const Descriptor& message) {
  // Map entries have key = 1 and value = 2.
  constexpr uint32_t kKeyField = 1;
  constexpr uint32_t kValueField = 2;
  const uint32_t field = FieldNumber(*GetFieldForMapType(
      message, FieldDescriptor::TYPE_STRING, FieldDescriptor::TYPE_MESSAGE));
  Wire key = LengthPrefixedField(kKeyField, "");
  Wire first_entry = LengthPrefixedField(
      field,
      Wire(key, LengthPrefixedField(
                    kValueField, NestedMessageWithCorecursive(Wire(
                                     VarintField(kOptionalInt32Field, 1),
                                     VarintField(kRepeatedInt32Field, 1))))));
  Wire second_entry = LengthPrefixedField(
      field,
      Wire(key, LengthPrefixedField(
                    kValueField, NestedMessageWithCorecursive(Wire(
                                     VarintField(kOptionalInt64Field, 1),
                                     VarintField(kRepeatedInt32Field, 1))))));
  return {/*input=*/Wire(first_entry, second_entry),
          /*expected=*/std::move(second_entry)};
}

// ValidDataOneof.MESSAGE.Merge: `message`'s oneof message field
// (oneof_nested_message) twice, each holding a `corecursive` submessage.
// Repeated occurrences of a oneof message field are merged like any other
// message field, so `expected` is a single occurrence holding the merged
// submessage.
MergeTestData OneofMessageMergeData(const Descriptor& message) {
  const uint32_t field = FieldNumber(
      *GetFieldForOneofType(message, FieldDescriptor::TYPE_MESSAGE));
  Wire first = LengthPrefixedField(
      field,
      NestedMessageWithCorecursive(Wire(VarintField(kOptionalInt32Field, 1),
                                        VarintField(kOptionalInt64Field, 1),
                                        VarintField(kUnpackedInt32Field, 1))));
  Wire second = LengthPrefixedField(
      field,
      NestedMessageWithCorecursive(Wire(VarintField(kOptionalInt64Field, 1),
                                        VarintField(kUnpackedInt32Field, 1))));
  // The singular fields take the last value seen and the repeated field
  // accumulates both.
  Wire merged = LengthPrefixedField(
      field,
      NestedMessageWithCorecursive(Wire(VarintField(kOptionalInt32Field, 1),
                                        VarintField(kOptionalInt64Field, 1),
                                        VarintField(kUnpackedInt32Field, 1),
                                        VarintField(kUnpackedInt32Field, 1))));
  return {/*input=*/Wire(first, second), /*expected=*/std::move(merged)};
}

using MergeTest = MessageTypeConformanceTest;

// Two occurrences of optional_nested_message, each with a corecursive
// submessage, must be merged field by field.  optional_nested_message (18) is
// the first singular message field in every TestAllTypes message, which is
// what the legacy test's GetFieldForType(MESSAGE) lookup resolved to.
TEST_P(MergeTest, RepeatedScalarMessageMerge) {
  EXPECT_THAT(
      Testee("RepeatedScalarMessageMerge")
          .ParseBinary(message(), RepeatedScalarMessageMergeInput(*message()))
          .SerializeBinary(),
      Yields(
          ParsedPayload(EqualsTextProto(R"pb(optional_nested_message: {
                                               corecursive: {
                                                 optional_int32: 4321
                                                 optional_int64: 1234
                                                 optional_uint32: 4321
                                                 repeated_int32: [ 1234, 4321 ]
                                               }
                                             })pb"))));
}

TEST_P(MergeTest, RepeatedScalarMessageMergeJson) {
  EXPECT_THAT(
      Testee("RepeatedScalarMessageMerge")
          .ParseBinary(message(), RepeatedScalarMessageMergeInput(*message()))
          .SerializeJson(),
      Yields(
          ParsedPayload(EqualsTextProto(R"pb(optional_nested_message: {
                                               corecursive: {
                                                 optional_int32: 4321
                                                 optional_int64: 1234
                                                 optional_uint32: 4321
                                                 repeated_int32: [ 1234, 4321 ]
                                               }
                                             })pb"))));
}

// Two map_string_nested_message entries with the same key: the second entry's
// value replaces the first one's, it isn't merged into it.  The legacy test
// computed its expected text by round-tripping the second entry through the
// generated message class and TextFormat; matching the second entry's bytes
// directly is the same equivalence check without the detour.
TEST_P(MergeTest, MapMessageValue) {
  MergeTestData data = MapMessageValueMergeData(*message());
  EXPECT_THAT(Testee("ValidDataMap.STRING.MESSAGE.MergeValue")
                  .ParseBinary(message(), data.input)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsBinaryProto(data.expected))));
}

TEST_P(MergeTest, MapMessageValueJson) {
  MergeTestData data = MapMessageValueMergeData(*message());
  EXPECT_THAT(Testee("ValidDataMap.STRING.MESSAGE.MergeValue")
                  .ParseBinary(message(), data.input)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsBinaryProto(data.expected))));
}

// Two occurrences of oneof_nested_message are merged like any other message
// field.  The legacy test compared the parsed output against the merged
// message (via a TextFormat round trip); see MapMessageValue above.
TEST_P(MergeTest, OneofMessage) {
  MergeTestData data = OneofMessageMergeData(*message());
  EXPECT_THAT(Testee("ValidDataOneof.MESSAGE.Merge")
                  .ParseBinary(message(), data.input)
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsBinaryProto(data.expected))));
}

TEST_P(MergeTest, OneofMessageJson) {
  MergeTestData data = OneofMessageMergeData(*message());
  EXPECT_THAT(Testee("ValidDataOneof.MESSAGE.Merge")
                  .ParseBinary(message(), data.input)
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsBinaryProto(data.expected))));
}

// The same, but the serialized output must be exactly the merged bytes.  The
// legacy suite had no JSON leg for this one.
TEST_P(MergeTest, OneofMessageBinary) {
  MergeTestData data = OneofMessageMergeData(*message());
  EXPECT_THAT(Testee(kP3, "ValidDataOneofBinary.MESSAGE.Merge")
                  .ParseBinary(message(), data.input)
                  .SerializeBinary(),
              Yields(Payload(data.expected)));
}

INSTANTIATE_TEST_SUITE_P(All, MergeTest, ValuesIn(AllTestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
