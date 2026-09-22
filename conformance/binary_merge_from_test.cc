// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Binary conformance tests of the explicit MergeFrom step (Merge*() on the
// testee API, `merge_payload` on the wire): a second payload merged into the
// parsed message before it is serialized.  binary_merge_test.cc covers the
// merge a parser performs on repeated occurrences of a field within one
// payload; these cover the public MergeFrom API, which no round trip
// exercises, with the same expectations, plus payloads in a different format
// than the input and the interplay with unknown fields.  Testees that don't
// support merge_payload skip them.

#include <cstdint>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "conformance/binary_test_util.h"
#include "conformance/binary_wireformat.h"
#include "conformance/matchers.h"
#include "conformance/message_type_fixtures.h"
#include "conformance/test_environment.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/unknown_field_set.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::ValuesIn;

// A field number no test message defines.
constexpr uint32_t kUnknownFieldNumber = 666;

// In every TestAllTypes message, NestedMessage's fields are a (1) and
// corecursive (2).
constexpr uint32_t kNestedMessageAField = 1;
constexpr uint32_t kNestedMessageCorecursiveField = 2;

class MergeFromTest : public MessageTypeConformanceTest {
 protected:
  // The number of `message()`'s first singular / repeated field of `type`.
  uint32_t SingularField(FieldDescriptor::Type type) {
    return FieldNumber(*GetFieldForType(*message(), type, /*repeated=*/false));
  }
  uint32_t RepeatedField(FieldDescriptor::Type type) {
    return FieldNumber(*GetFieldForType(*message(), type, /*repeated=*/true));
  }
};

// A singular scalar takes the merged value.
TEST_P(MergeFromTest, ScalarOverwritten) {
  const uint32_t field = SingularField(FieldDescriptor::TYPE_INT32);
  EXPECT_THAT(Testee()
                  .ParseBinary(message(), VarintField(field, 1))
                  .MergeBinary(VarintField(field, 2))
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto("optional_int32: 2"))));
}

// A field the merged payload doesn't set keeps its value.
TEST_P(MergeFromTest, UnsetFieldKept) {
  const uint32_t int32_field = SingularField(FieldDescriptor::TYPE_INT32);
  const uint32_t string_field = SingularField(FieldDescriptor::TYPE_STRING);
  EXPECT_THAT(
      Testee()
          .ParseBinary(message(), VarintField(int32_field, 1))
          .MergeBinary(LengthPrefixedField(string_field, "a"))
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(R"pb(optional_int32: 1
                                                optional_string: "a")pb"))));
}

// A repeated field appends the merged elements.
TEST_P(MergeFromTest, RepeatedAppended) {
  const uint32_t field = RepeatedField(FieldDescriptor::TYPE_INT32);
  EXPECT_THAT(
      Testee()
          .ParseBinary(message(), VarintField(field, 1))
          .MergeBinary(Wire(VarintField(field, 2), VarintField(field, 3)))
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto("repeated_int32: [ 1, 2, 3 ]"))));
}

// A message field is merged field by field rather than replaced.
TEST_P(MergeFromTest, MessageMerged) {
  const uint32_t field = SingularField(FieldDescriptor::TYPE_MESSAGE);
  EXPECT_THAT(
      Testee()
          .ParseBinary(
              message(),
              LengthPrefixedField(field, VarintField(kNestedMessageAField, 1)))
          .MergeBinary(LengthPrefixedField(
              field,
              LengthPrefixedField(
                  kNestedMessageCorecursiveField,
                  VarintField(SingularField(FieldDescriptor::TYPE_INT32), 2))))
          .SerializeBinary(),
      Yields(
          ParsedPayload(EqualsTextProto(R"pb(optional_nested_message {
                                               a: 1
                                               corecursive { optional_int32: 2 }
                                             })pb"))));
}

// Merging a different member of a oneof clears the one that was set.
TEST_P(MergeFromTest, OneofSwitched) {
  const uint32_t uint32_field = FieldNumber(
      *GetFieldForOneofType(*message(), FieldDescriptor::TYPE_UINT32));
  const uint32_t string_field = FieldNumber(
      *GetFieldForOneofType(*message(), FieldDescriptor::TYPE_STRING));
  EXPECT_THAT(
      Testee()
          .ParseBinary(message(), VarintField(uint32_field, 1))
          .MergeBinary(LengthPrefixedField(string_field, "a"))
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(R"pb(oneof_string: "a")pb"))));
}

// A merged map entry replaces the value of an existing key and adds new keys.
TEST_P(MergeFromTest, MapEntriesOverwrittenAndAdded) {
  // Map entries have key = 1 and value = 2.
  constexpr uint32_t kKeyField = 1;
  constexpr uint32_t kValueField = 2;
  const uint32_t field = FieldNumber(*GetFieldForMapType(
      *message(), FieldDescriptor::TYPE_STRING, FieldDescriptor::TYPE_STRING));
  auto entry = [&](absl::string_view key, absl::string_view value) {
    return LengthPrefixedField(field,
                               Wire(LengthPrefixedField(kKeyField, key),
                                    LengthPrefixedField(kValueField, value)));
  };
  EXPECT_THAT(
      Testee()
          .ParseBinary(message(), Wire(entry("a", "1"), entry("b", "1")))
          .MergeBinary(Wire(entry("b", "2"), entry("c", "2")))
          .SerializeBinary(),
      Yields(ParsedPayload(
          EqualsTextProto(R"pb(map_string_string { key: "a" value: "1" }
                               map_string_string { key: "b" value: "2" }
                               map_string_string { key: "c" value: "2" }
          )pb"))));
}

// The merged payload's format is independent of the input's.
TEST_P(MergeFromTest, TextMergedIntoBinaryInput) {
  const uint32_t field = SingularField(FieldDescriptor::TYPE_INT32);
  EXPECT_THAT(
      Testee()
          .ParseBinary(message(), VarintField(field, 1))
          .MergeText("optional_string: 'a'")
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(R"pb(optional_int32: 1
                                                optional_string: "a")pb"))));
}

TEST_P(MergeFromTest, JsonMergedIntoBinaryInput) {
  const uint32_t field = SingularField(FieldDescriptor::TYPE_INT32);
  EXPECT_THAT(
      Testee()
          .ParseBinary(message(), VarintField(field, 1))
          .MergeJson(R"json({"optionalString": "a"})json")
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(R"pb(optional_int32: 1
                                                optional_string: "a")pb"))));
}

// A merge payload that doesn't parse is a parse error, like the input.
TEST_P(MergeFromTest, UnparseableMergePayload) {
  const uint32_t field = SingularField(FieldDescriptor::TYPE_INT32);
  EXPECT_THAT(Testee()
                  .ParseBinary(message(), VarintField(field, 1))
                  .MergeBinary(Wire(Tag(field, WireType::kLengthPrefixed),
                                    Varint(5), "abc"))
                  .SerializeBinary(),
              Yields(IsParseError()));
}

// Unknown fields of both payloads are kept, in order.
TEST_P(MergeFromTest, UnknownFieldsAccumulate) {
  UnknownFieldSet expected;
  expected.AddVarint(kUnknownFieldNumber, 1);
  expected.AddVarint(kUnknownFieldNumber, 2);
  EXPECT_THAT(Testee()
                  .ParseBinary(message(), VarintField(kUnknownFieldNumber, 1))
                  .MergeBinary(VarintField(kUnknownFieldNumber, 2))
                  .SerializeBinary(),
              Yields(ParsedPayload(HasUnknownFieldsInOrder(expected))));
}

// Discarding unknown fields happens after the merge, so it takes the merged
// payload's unknown fields with it.
TEST_P(MergeFromTest, MergedUnknownFieldsDiscarded) {
  const uint32_t field = SingularField(FieldDescriptor::TYPE_INT32);
  EXPECT_THAT(Testee()
                  .ParseBinary(message(), VarintField(field, 1))
                  .MergeBinary(VarintField(kUnknownFieldNumber, 2))
                  .DiscardUnknownFields()
                  .SerializeBinary(),
              Yields(Payload(VarintField(field, 1))));
}

INSTANTIATE_TEST_SUITE_P(All, MergeFromTest, ValuesIn(AllTestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
