// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Binary conformance tests checking how several occurrences of the same field
// in one payload are combined: repeated submessages are merged, a repeated
// map entry replaces the previous value for its key, and repeated occurrences
// of a oneof message field are merged too.  This holds the binary-output leg
// of the legacy BinaryAndJsonConformanceSuiteImpl<M>::
// TestValidDataForRepeatedScalarMessage(), TestOverwriteMessageValueMap() and
// TestMergeOneofMessage(); the test names and the requests sent to the testee
// are identical to the legacy ones.  The inputs are shared with the legacy
// suite (see binary_test_util.h), which still sends them for its binary->JSON
// leg.
//
// TODO: b/410122158 - The binary->JSON legs of these tests join the JSON suite
// once JSON matching is migrated.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "conformance/binary_test_util.h"
#include "conformance/matchers.h"
#include "conformance/message_type_fixtures.h"
#include "conformance/test_environment.h"
#include "conformance/testee.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::ValuesIn;

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

// The same, but the serialized output must be exactly the merged bytes.
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
