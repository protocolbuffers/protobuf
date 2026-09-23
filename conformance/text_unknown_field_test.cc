// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Text-format conformance tests for unknown fields: a binary payload whose
// fields are all unknown to the test message type is serialized as text
// format, once dropping the unknown fields ("<name>.Drop") and once printing
// them by field number ("<name>.Print").  This replaces the "Unknown Fields"
// block of the legacy TextFormatConformanceTestSuiteImpl<M>::RunAllTests() and
// its RunValidUnknownTextFormatTest() helper, which ran for the proto3-style
// message types only; the requests sent to the testee are identical to the
// legacy ones.
//
// The payloads are serializations of UnknownToTestAllTypes, whose fields have
// numbers no TestAllTypes message defines, and the testee's text output is
// decoded as that type too (see ParsedPayloadAs() in matchers.h): unknown
// fields printed by number become its known fields again, so the Print leg
// expects the whole payload back and the Drop leg an empty message.  Like the
// legacy tests these are RECOMMENDED and categorized as TEXT_FORMAT_TEST
// although their input is binary.

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "conformance/binary_test_util.h"
#include "conformance/binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "conformance/matchers.h"
#include "conformance/message_type_fixtures.h"
#include "conformance/test_environment.h"
#include "conformance/testee.h"
#include "google/protobuf/test_messages_proto2.pb.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::protobuf_test_messages::proto2::UnknownToTestAllTypes;
using ::testing::ValuesIn;

class TextUnknownFieldTest : public MessageTypeConformanceTest {
 public:
  TestPriority DefaultPriority() const override { return kP3; }
};

// Unable to print unknown Fixed32/Fixed64 fields as if they are known.
// Fixed32/Fixed64 fields are not added in the tests.
TEST_P(TextUnknownFieldTest, ScalarUnknownFields) {
  UnknownToTestAllTypes unknown;
  unknown.set_optional_int32(123);
  unknown.set_optional_string("hello");
  unknown.set_optional_bool(true);
  const std::string bytes = unknown.SerializeAsString();
  EXPECT_THAT(Testee("Drop")
                  .ParseBinary(message(), Wire(bytes))
                  .OverrideTestCategory(::conformance::TEXT_FORMAT_TEST)
                  .SerializeText(),
              Yields(ParsedPayloadAs(UnknownToTestAllTypes::descriptor(),
                                     EqualsTextProto(""))));
  EXPECT_THAT(Testee("Print")
                  .ParseBinary(message(), Wire(bytes))
                  .OverrideTestCategory(::conformance::TEXT_FORMAT_TEST)
                  .SerializeText({/*print_unknown_fields=*/true}),
              Yields(ParsedPayloadAs(UnknownToTestAllTypes::descriptor(),
                                     EqualsBinaryProto(bytes))));
}

TEST_P(TextUnknownFieldTest, MessageUnknownFields) {
  UnknownToTestAllTypes unknown;
  unknown.mutable_nested_message()->set_c(111);
  const std::string bytes = unknown.SerializeAsString();
  EXPECT_THAT(Testee("Drop")
                  .ParseBinary(message(), Wire(bytes))
                  .OverrideTestCategory(::conformance::TEXT_FORMAT_TEST)
                  .SerializeText(),
              Yields(ParsedPayloadAs(UnknownToTestAllTypes::descriptor(),
                                     EqualsTextProto(""))));
  EXPECT_THAT(Testee("Print")
                  .ParseBinary(message(), Wire(bytes))
                  .OverrideTestCategory(::conformance::TEXT_FORMAT_TEST)
                  .SerializeText({/*print_unknown_fields=*/true}),
              Yields(ParsedPayloadAs(UnknownToTestAllTypes::descriptor(),
                                     EqualsBinaryProto(bytes))));
}

TEST_P(TextUnknownFieldTest, GroupUnknownFields) {
  UnknownToTestAllTypes unknown;
  unknown.mutable_optionalgroup()->set_a(321);
  const std::string bytes = unknown.SerializeAsString();
  EXPECT_THAT(Testee("Drop")
                  .ParseBinary(message(), Wire(bytes))
                  .OverrideTestCategory(::conformance::TEXT_FORMAT_TEST)
                  .SerializeText(),
              Yields(ParsedPayloadAs(UnknownToTestAllTypes::descriptor(),
                                     EqualsTextProto(""))));
  EXPECT_THAT(Testee("Print")
                  .ParseBinary(message(), Wire(bytes))
                  .OverrideTestCategory(::conformance::TEXT_FORMAT_TEST)
                  .SerializeText({/*print_unknown_fields=*/true}),
              Yields(ParsedPayloadAs(UnknownToTestAllTypes::descriptor(),
                                     EqualsBinaryProto(bytes))));
}

// Note: the legacy test built this payload on top of the group one without
// clearing it, so the group is part of this payload too.
TEST_P(TextUnknownFieldTest, RepeatedUnknownFields) {
  UnknownToTestAllTypes unknown;
  unknown.mutable_optionalgroup()->set_a(321);
  unknown.add_repeated_int32(1);
  unknown.add_repeated_int32(2);
  unknown.add_repeated_int32(3);
  const std::string bytes = unknown.SerializeAsString();
  EXPECT_THAT(Testee("Drop")
                  .ParseBinary(message(), Wire(bytes))
                  .OverrideTestCategory(::conformance::TEXT_FORMAT_TEST)
                  .SerializeText(),
              Yields(ParsedPayloadAs(UnknownToTestAllTypes::descriptor(),
                                     EqualsTextProto(""))));
  EXPECT_THAT(Testee("Print")
                  .ParseBinary(message(), Wire(bytes))
                  .OverrideTestCategory(::conformance::TEXT_FORMAT_TEST)
                  .SerializeText({/*print_unknown_fields=*/true}),
              Yields(ParsedPayloadAs(UnknownToTestAllTypes::descriptor(),
                                     EqualsBinaryProto(bytes))));
}

INSTANTIATE_TEST_SUITE_P(All, TextUnknownFieldTest,
                         ValuesIn(Proto3TestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
