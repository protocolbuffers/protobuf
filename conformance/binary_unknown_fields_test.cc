// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Binary conformance tests checking that unknown fields are preserved, and
// preserved in order.  The first two replace the legacy
// BinaryAndJsonConformanceSuiteImpl<M>::TestUnknownMessage() and
// TestUnknownOrdering(); the requests sent to the testee are identical to the
// legacy ones.
//
// The *Discarded tests then ask the testee to discard its unknown fields
// before serializing (DiscardUnknownFields()), which is what tells a value the
// parser kept as an unknown field from one it parsed into a known field: a
// plain round trip re-emits both the same way.  Testees that don't support
// discard_unknown_fields skip them.

#include <cstdint>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_log.h"
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

// The first field number in one of `message`'s extension ranges that no
// extension known to its pool uses (the test messages define extensions at the
// start of their ranges, so this is a number well inside a range).
uint32_t UnclaimedExtensionNumber(const Descriptor& message) {
  for (int i = 0; i < message.extension_range_count(); ++i) {
    const Descriptor::ExtensionRange* range = message.extension_range(i);
    for (int number = range->start_number(); number < range->end_number();
         ++number) {
      if (message.file()->pool()->FindExtensionByNumber(&message, number) ==
          nullptr) {
        return number;
      }
    }
  }
  ABSL_LOG(FATAL) << message.full_name()
                  << " has no unclaimed extension number";
}

using UnknownFieldsTest = MessageTypeConformanceTest;

// An unknown varint field must be round-tripped byte for byte.  Field 501
// isn't defined by any of the test messages; the legacy test spelled the input
// as the bytes "\xA8\x1F\x01".
TEST_P(UnknownFieldsTest, UnknownVarint) {
  Wire input = VarintField(501, 1);
  EXPECT_THAT(Testee().ParseBinary(message(), input).SerializeBinary(),
              Yields(Payload(input)));
}

// Implementations must preserve the ordering of different unknown fields for
// the same field number.  This is because some field types will accept
// multiple wire types for the same field.  For example, repeated primitive
// fields will accept both length-prefixed (packed) and
// varint/fixed32/fixed64 (unpacked) wire types, and reordering these could
// reorder the elements of the repeated field.
TEST_P(UnknownFieldsTest, UnknownOrdering) {
  UnknownFieldSet expected;
  expected.AddLengthDelimited(kUnknownFieldNumber, "abc");
  expected.AddVarint(kUnknownFieldNumber, 123);
  expected.AddLengthDelimited(kUnknownFieldNumber, "def");
  expected.AddVarint(kUnknownFieldNumber, 456);
  // The legacy test sent the serialization of a message holding exactly
  // these unknown fields; these are its bytes.
  Wire input(LengthPrefixedField(kUnknownFieldNumber, "abc"),
             VarintField(kUnknownFieldNumber, 123),
             LengthPrefixedField(kUnknownFieldNumber, "def"),
             VarintField(kUnknownFieldNumber, 456));
  EXPECT_THAT(Testee().ParseBinary(message(), input).SerializeBinary(),
              Yields(ParsedPayload(HasUnknownFieldsInOrder(expected))));
}

// With the unknown fields discarded, nothing of the input is left: the field
// really was kept as an unknown field rather than parsed into something.
TEST_P(UnknownFieldsTest, UnknownVarintDiscarded) {
  EXPECT_THAT(Testee()
                  .ParseBinary(message(), VarintField(501, 1))
                  .DiscardUnknownFields()
                  .SerializeBinary(),
              Yields(Payload(Wire())));
}

// Discarding is recursive: an unknown field inside a submessage goes too, and
// the (now empty) submessage stays.
TEST_P(UnknownFieldsTest, NestedUnknownVarintDiscarded) {
  const uint32_t field = FieldNumber(*GetFieldForType(
      *message(), FieldDescriptor::TYPE_MESSAGE, /*repeated=*/false));
  EXPECT_THAT(Testee()
                  .ParseBinary(message(),
                               LengthPrefixedField(field, VarintField(501, 1)))
                  .DiscardUnknownFields()
                  .SerializeBinary(),
              Yields(Payload(LengthPrefixedField(field, Wire()))));
}

// A known field number with a wire type its field can't take (optional_int32,
// field 1, as a length-prefixed value) must be kept as an unknown field, not
// parsed into the field; so once discarded nothing is left.
TEST_P(UnknownFieldsTest, KnownFieldWithWrongWireTypeDiscarded) {
  const uint32_t field = FieldNumber(*GetFieldForType(
      *message(), FieldDescriptor::TYPE_INT32, /*repeated=*/false));
  EXPECT_THAT(Testee()
                  .ParseBinary(message(), LengthPrefixedField(field, "abc"))
                  .DiscardUnknownFields()
                  .SerializeBinary(),
              Yields(Payload(Wire())));
}

// An enum value the enum doesn't define is parsed into the field if the enum
// is open (proto3) and kept as an unknown field if it is closed (proto2), so
// discarding unknown fields keeps it in the first case and drops it in the
// second.  A round trip can't tell the two apart.
TEST_P(UnknownFieldsTest, UnknownEnumValueDiscarded) {
  const FieldDescriptor* field = GetFieldForType(
      *message(), FieldDescriptor::TYPE_ENUM, /*repeated=*/false);
  Wire input = VarintField(FieldNumber(*field), 42);
  EXPECT_THAT(
      Testee()
          .ParseBinary(message(), input)
          .DiscardUnknownFields()
          .SerializeBinary(),
      Yields(Payload(field->enum_type()->is_closed() ? Wire() : input)));
}

INSTANTIATE_TEST_SUITE_P(All, UnknownFieldsTest,
                         ValuesIn(AllTestMessageTypes()), MessageTypeParamName);

// Groups and extension ranges only exist in the proto2-style test messages.
using Proto2UnknownFieldsTest = MessageTypeConformanceTest;

// Discarding recurses into groups (delimited messages) like into
// length-prefixed submessages: the unknown field inside goes, the now empty
// group stays.
TEST_P(Proto2UnknownFieldsTest, UnknownFieldInGroupDiscarded) {
  const uint32_t field = FieldNumber(*GetFieldForType(
      *message(), FieldDescriptor::TYPE_GROUP, /*repeated=*/false));
  EXPECT_THAT(
      Testee()
          .ParseBinary(message(), DelimitedField(field, VarintField(501, 1)))
          .DiscardUnknownFields()
          .SerializeBinary(),
      Yields(Payload(DelimitedField(field, Wire()))));
}

// A field number inside the message's extension range that no extension
// claims is an unknown field like any other, not a (missing) extension that
// survives discarding.
TEST_P(Proto2UnknownFieldsTest, UnclaimedExtensionNumberDiscarded) {
  EXPECT_THAT(
      Testee()
          .ParseBinary(message(),
                       VarintField(UnclaimedExtensionNumber(*message()), 1))
          .DiscardUnknownFields()
          .SerializeBinary(),
      Yields(Payload(Wire())));
}

INSTANTIATE_TEST_SUITE_P(All, Proto2UnknownFieldsTest,
                         ValuesIn(Proto2TestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
