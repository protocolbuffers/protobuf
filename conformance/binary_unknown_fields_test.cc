// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Binary conformance tests checking that unknown fields are preserved, and
// preserved in order.  This replaces the legacy
// BinaryAndJsonConformanceSuiteImpl<M>::TestUnknownMessage() and
// TestUnknownOrdering(); the test names and the requests sent to the testee
// are identical to the legacy ones.

#include <cstdint>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "binary_test_util.h"
#include "binary_wireformat.h"
#include "matchers.h"
#include "message_type_fixtures.h"
#include "test_environment.h"
#include "google/protobuf/unknown_field_set.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::ValuesIn;

// A field number no test message defines.
constexpr uint32_t kUnknownFieldNumber = 666;

using UnknownFieldsTest = MessageTypeConformanceTest;

// An unknown varint field must be round-tripped byte for byte.  Field 501
// isn't defined by any of the test messages; the legacy test spelled the input
// as the bytes "\xA8\x1F\x01".
TEST_P(UnknownFieldsTest, UnknownVarint) {
  Wire input = VarintField(501, 1);
  EXPECT_THAT(RequiredTest("UnknownVarint")
                  .ParseBinary(message(), input)
                  .SerializeBinary(),
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
  EXPECT_THAT(RequiredTest("UnknownOrdering")
                  .ParseBinary(message(), input)
                  .SerializeBinary(),
              Yields(ParsedPayload(HasUnknownFieldsInOrder(expected))));
}

INSTANTIATE_TEST_SUITE_P(All, UnknownFieldsTest,
                         ValuesIn(AllTestMessageTypes()), MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
