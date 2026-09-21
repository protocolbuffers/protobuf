// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Binary conformance tests checking that a oneof member is parsed correctly
// and that later occurrences of a oneof's members replace earlier ones, for
// every member type in ValidDataOneofTypes(): ValidDataOneof.<TYPE>.<Case>
// (REQUIRED, equivalence, with a binary-output and a JSON-output leg) and
// ValidDataOneofBinary.<TYPE>.<Case> (RECOMMENDED, byte-exact) for the cases
// DefaultValue, NonDefaultValue, MultipleValuesForSameField and
// MultipleValuesForDifferentField.
//
// This holds both legs of the legacy
// BinaryAndJsonConformanceSuiteImpl<M>::TestValidDataForOneofType(); the
// requests sent to the testee are identical to the legacy ones.  The oneof
// merge and zero-value tests live in binary_merge_test.cc and
// binary_oneof_zero_test.cc.

#include <string>
#include <tuple>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "conformance/binary_test_util.h"
#include "conformance/binary_wireformat.h"
#include "conformance/matchers.h"
#include "conformance/message_type_fixtures.h"
#include "conformance/test_environment.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::Combine;
using ::testing::ValuesIn;

// Parameterized over (test message type, member type), exercising the
// message's first oneof member of that type.  Reporting the message type
// through MessageUnderTest() makes the base SetUp() skip the editions
// instances when --maximum_edition doesn't cover them.
class ValidDataOneofTest
    : public ConformanceTest,
      public testing::WithParamInterface<
          std::tuple<const Descriptor*, FieldDescriptor::Type>> {
 protected:
  const Descriptor* MessageUnderTest() const override { return message(); }

  const Descriptor* message() const { return std::get<0>(GetParam()); }
  FieldDescriptor::Type type() const { return std::get<1>(GetParam()); }

  // The member holding the default and the non-default sample value of
  // type().  These scan the descriptor (and check-fail if there is no such
  // member), hence the function-style names.
  Wire DefaultMember() const {
    return FieldWithValue(Member(), GetDefaultValue(type()));
  }
  Wire NonDefaultMember() const {
    return FieldWithValue(Member(), GetNonDefaultValue(type()));
  }

  // Another member of the same oneof, of a different type, holding its
  // type's default value.
  Wire OtherMemberValue() const {
    const FieldDescriptor* other =
        GetFieldForOneofType(*message(), type(), OneofMember::kOfOtherType);
    return FieldWithValue(other, GetDefaultValue(other->type()));
  }

 private:
  const FieldDescriptor* Member() const {
    return GetFieldForOneofType(*message(), type());
  }

  // `payload` (a value without a tag) as `field`.
  static Wire FieldWithValue(const FieldDescriptor* field,
                             const Wire& payload) {
    return Wire(Tag(FieldNumber(*field), WireTypeForFieldType(field->type())),
                payload);
  }
};

// A member set to its default value has presence and must round trip.
TEST_P(ValidDataOneofTest, DefaultValue) {
  EXPECT_THAT(
      Testee().ParseBinary(message(), DefaultMember()).SerializeBinary(),
      Yields(ParsedPayload(EqualsBinaryProto(DefaultMember()))));
}

// ... in JSON as well as in binary.
TEST_P(ValidDataOneofTest, DefaultValueJson) {
  EXPECT_THAT(Testee().ParseBinary(message(), DefaultMember()).SerializeJson(),
              Yields(ParsedPayload(EqualsBinaryProto(DefaultMember()))));
}

// ... and its binary serialization must be exactly the input.
TEST_P(ValidDataOneofTest, DefaultValueBinary) {
  EXPECT_THAT(
      Testee(kP3).ParseBinary(message(), DefaultMember()).SerializeBinary(),
      Yields(Payload(DefaultMember())));
}

// So does a non-default value.
TEST_P(ValidDataOneofTest, NonDefaultValue) {
  EXPECT_THAT(
      Testee().ParseBinary(message(), NonDefaultMember()).SerializeBinary(),
      Yields(ParsedPayload(EqualsBinaryProto(NonDefaultMember()))));
}

// ... in JSON as well as in binary.
TEST_P(ValidDataOneofTest, NonDefaultValueJson) {
  EXPECT_THAT(
      Testee().ParseBinary(message(), NonDefaultMember()).SerializeJson(),
      Yields(ParsedPayload(EqualsBinaryProto(NonDefaultMember()))));
}

// ... and its binary serialization must be exactly the input.
TEST_P(ValidDataOneofTest, NonDefaultValueBinary) {
  EXPECT_THAT(
      Testee(kP3).ParseBinary(message(), NonDefaultMember()).SerializeBinary(),
      Yields(Payload(NonDefaultMember())));
}

// The same member twice: the last value wins.
TEST_P(ValidDataOneofTest, MultipleValuesForSameField) {
  EXPECT_THAT(
      Testee()
          .ParseBinary(message(), Wire(DefaultMember(), NonDefaultMember()))
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsBinaryProto(NonDefaultMember()))));
}

// ... in JSON as well as in binary.
TEST_P(ValidDataOneofTest, MultipleValuesForSameFieldJson) {
  EXPECT_THAT(
      Testee()
          .ParseBinary(message(), Wire(DefaultMember(), NonDefaultMember()))
          .SerializeJson(),
      Yields(ParsedPayload(EqualsBinaryProto(NonDefaultMember()))));
}

// ... and the binary serialization must be exactly the last value.
TEST_P(ValidDataOneofTest, MultipleValuesForSameFieldBinary) {
  EXPECT_THAT(
      Testee(kP3)
          .ParseBinary(message(), Wire(DefaultMember(), NonDefaultMember()))
          .SerializeBinary(),
      Yields(Payload(NonDefaultMember())));
}

// Two different members of the oneof: the last one set is the only one kept.
TEST_P(ValidDataOneofTest, MultipleValuesForDifferentField) {
  EXPECT_THAT(
      Testee()
          .ParseBinary(message(), Wire(OtherMemberValue(), NonDefaultMember()))
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsBinaryProto(NonDefaultMember()))));
}

// ... in JSON as well as in binary.
TEST_P(ValidDataOneofTest, MultipleValuesForDifferentFieldJson) {
  EXPECT_THAT(
      Testee()
          .ParseBinary(message(), Wire(OtherMemberValue(), NonDefaultMember()))
          .SerializeJson(),
      Yields(ParsedPayload(EqualsBinaryProto(NonDefaultMember()))));
}

// ... and the binary serialization must be exactly the last member.
TEST_P(ValidDataOneofTest, MultipleValuesForDifferentFieldBinary) {
  EXPECT_THAT(
      Testee(kP3)
          .ParseBinary(message(), Wire(OtherMemberValue(), NonDefaultMember()))
          .SerializeBinary(),
      Yields(Payload(NonDefaultMember())));
}

INSTANTIATE_TEST_SUITE_P(All, ValidDataOneofTest,
                         Combine(ValuesIn(AllTestMessageTypes()),
                                 ValuesIn(ValidDataOneofTypes())),
                         TupleParamName<ValidDataOneofTest::ParamType>);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
