// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Binary conformance tests checking that a payload truncated before or inside
// a field's value is rejected.  This replaces the legacy
// BinaryAndJsonConformanceSuiteImpl<M>::TestPrematureEOFForType(), which
// RunAllTests() called for every field type except groups; the test names and
// the requests sent to the testee are identical to the legacy ones.

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "binary_test_util.h"
#include "binary_wireformat.h"
#include "matchers.h"
#include "message_type_fixtures.h"
#include "test_environment.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::Combine;
using ::testing::ValuesIn;

// A field number no test message defines.
constexpr uint32_t kUnknownFieldNumber = 666;

// Every field type except TYPE_GROUP, in declaration order.
constexpr FieldDescriptor::Type kAllTypesExceptGroup[] = {
    FieldDescriptor::TYPE_DOUBLE,   FieldDescriptor::TYPE_FLOAT,
    FieldDescriptor::TYPE_INT64,    FieldDescriptor::TYPE_UINT64,
    FieldDescriptor::TYPE_INT32,    FieldDescriptor::TYPE_FIXED64,
    FieldDescriptor::TYPE_FIXED32,  FieldDescriptor::TYPE_BOOL,
    FieldDescriptor::TYPE_STRING,   FieldDescriptor::TYPE_MESSAGE,
    FieldDescriptor::TYPE_BYTES,    FieldDescriptor::TYPE_UINT32,
    FieldDescriptor::TYPE_ENUM,     FieldDescriptor::TYPE_SFIXED32,
    FieldDescriptor::TYPE_SFIXED64, FieldDescriptor::TYPE_SINT32,
    FieldDescriptor::TYPE_SINT64,
};

// The length-delimited field types (string, message, bytes), in
// kAllTypesExceptGroup order.
std::vector<FieldDescriptor::Type> LengthDelimitedTypes() {
  std::vector<FieldDescriptor::Type> types;
  for (FieldDescriptor::Type type : kAllTypesExceptGroup) {
    if (WireTypeForFieldType(type) == WireType::kLengthPrefixed) {
      types.push_back(type);
    }
  }
  return types;
}

// The field types eligible for packing (everything that is neither
// length-delimited nor a group), in kAllTypesExceptGroup order.
std::vector<FieldDescriptor::Type> PackableTypes() {
  std::vector<FieldDescriptor::Type> types;
  for (FieldDescriptor::Type type : kAllTypesExceptGroup) {
    if (FieldDescriptor::IsTypePackable(type)) types.push_back(type);
  }
  return types;
}

// An incomplete value for each wire type, exactly as in the legacy suite.
absl::string_view IncompleteValue(WireType wire_type) {
  switch (wire_type) {
    case WireType::kVarint:
      return "\x80";
    case WireType::kFixed64:
      return "abcdefg";
    case WireType::kLengthPrefixed:
      return "\x80";  // Partial length.
    case WireType::kStartGroup:
    case WireType::kEndGroup:
      return "";  // No value required.
    case WireType::kFixed32:
      return "abc";
    case WireType::kInvalid:
      break;
  }
  ABSL_LOG(FATAL) << "No incomplete value for wire type " << wire_type;
}

// Parameterized over (test message type, field type).  The three test suites
// below only differ in the field types they are instantiated with, so that no
// test ever has to skip itself as "not applicable".  Reporting the message
// type through MessageUnderTest() makes the base SetUp() skip the editions
// instances when --maximum_edition doesn't cover them.
class PrematureEofTest
    : public ConformanceTest,
      public testing::WithParamInterface<
          std::tuple<const Descriptor*, FieldDescriptor::Type>> {
 protected:
  const Descriptor* MessageUnderTest() const override { return message(); }

  const Descriptor* message() const { return std::get<0>(GetParam()); }
  FieldDescriptor::Type type() const { return std::get<1>(GetParam()); }
  WireType wire_type() const { return WireTypeForFieldType(type()); }
  absl::string_view incomplete() const { return IncompleteValue(wire_type()); }

  // The message's singular and repeated field of type().  These scan the
  // descriptor (and check-fail if there is no such field), hence the
  // function-style names.
  const FieldDescriptor* Field() const {
    return GetFieldForType(*message(), type(), /*repeated=*/false);
  }
  const FieldDescriptor* RepeatedField() const {
    return GetFieldForType(*message(), type(), /*repeated=*/true);
  }

  // The legacy test name: "<prefix>.<TYPE>", e.g.
  // "PrematureEofBeforeKnownNonRepeatedValue.INT32".
  std::string TestName(absl::string_view prefix) const {
    return absl::StrCat(prefix, ".", UpperCaseTypeName(type()));
  }
};

TEST_P(PrematureEofTest, BeforeKnownNonRepeatedValue) {
  EXPECT_THAT(
      RequiredTest(TestName("PrematureEofBeforeKnownNonRepeatedValue"))
          .ParseBinary(message(), Tag(FieldNumber(*Field()), wire_type()))
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(PrematureEofTest, BeforeKnownRepeatedValue) {
  EXPECT_THAT(RequiredTest(TestName("PrematureEofBeforeKnownRepeatedValue"))
                  .ParseBinary(message(),
                               Tag(FieldNumber(*RepeatedField()), wire_type()))
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(PrematureEofTest, BeforeUnknownValue) {
  EXPECT_THAT(RequiredTest(TestName("PrematureEofBeforeUnknownValue"))
                  .ParseBinary(message(), Tag(kUnknownFieldNumber, wire_type()))
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(PrematureEofTest, InsideKnownNonRepeatedValue) {
  EXPECT_THAT(
      RequiredTest(TestName("PrematureEofInsideKnownNonRepeatedValue"))
          .ParseBinary(message(), Wire(Tag(FieldNumber(*Field()), wire_type()),
                                       incomplete()))
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(PrematureEofTest, InsideKnownRepeatedValue) {
  EXPECT_THAT(
      RequiredTest(TestName("PrematureEofInsideKnownRepeatedValue"))
          .ParseBinary(message(),
                       Wire(Tag(FieldNumber(*RepeatedField()), wire_type()),
                            incomplete()))
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(PrematureEofTest, InsideUnknownValue) {
  EXPECT_THAT(
      RequiredTest(TestName("PrematureEofInsideUnknownValue"))
          .ParseBinary(message(), Wire(Tag(kUnknownFieldNumber, wire_type()),
                                       incomplete()))
          .ParseOnly(),
      Yields(IsParseError()));
}

INSTANTIATE_TEST_SUITE_P(All, PrematureEofTest,
                         Combine(ValuesIn(AllTestMessageTypes()),
                                 ValuesIn(kAllTypesExceptGroup)),
                         TupleParamName<PrematureEofTest::ParamType>);

// A length prefix that promises more data than follows.
class PrematureEofInDelimitedDataTest : public PrematureEofTest {};

TEST_P(PrematureEofInDelimitedDataTest, ForKnownNonRepeatedValue) {
  EXPECT_THAT(
      RequiredTest(
          TestName("PrematureEofInDelimitedDataForKnownNonRepeatedValue"))
          .ParseBinary(message(), Wire(Tag(FieldNumber(*Field()),
                                           WireType::kLengthPrefixed),
                                       Varint(1)))
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(PrematureEofInDelimitedDataTest, ForKnownRepeatedValue) {
  EXPECT_THAT(
      RequiredTest(TestName("PrematureEofInDelimitedDataForKnownRepeatedValue"))
          .ParseBinary(message(), Wire(Tag(FieldNumber(*RepeatedField()),
                                           WireType::kLengthPrefixed),
                                       Varint(1)))
          .ParseOnly(),
      Yields(IsParseError()));
}

TEST_P(PrematureEofInDelimitedDataTest, ForUnknownValue) {
  EXPECT_THAT(
      RequiredTest(TestName("PrematureEofInDelimitedDataForUnknownValue"))
          .ParseBinary(message(),
                       Wire(Tag(kUnknownFieldNumber, WireType::kLengthPrefixed),
                            Varint(1)))
          .ParseOnly(),
      Yields(IsParseError()));
}

INSTANTIATE_TEST_SUITE_P(
    All, PrematureEofInDelimitedDataTest,
    Combine(ValuesIn(AllTestMessageTypes()), ValuesIn(LengthDelimitedTypes())),
    TupleParamName<PrematureEofInDelimitedDataTest::ParamType>);

// A packed repeated field whose payload is truncated.
class PrematureEofInPackedFieldTest : public PrematureEofTest {};

// The packed region ends in the middle of one of its values.
// TODO: b/563659437 - also run with valid data appended (legacy TODO).
TEST_P(PrematureEofInPackedFieldTest, PackedFieldValue) {
  EXPECT_THAT(RequiredTest(TestName("PrematureEofInPackedFieldValue"))
                  .ParseBinary(message(),
                               LengthPrefixedField(
                                   FieldNumber(*RepeatedField()), incomplete()))
                  .ParseOnly(),
              Yields(IsParseError()));
}

// The payload ends before the packed region does.
TEST_P(PrematureEofInPackedFieldTest, PackedField) {
  EXPECT_THAT(
      RequiredTest(TestName("PrematureEofInPackedField"))
          .ParseBinary(message(), Wire(Tag(FieldNumber(*RepeatedField()),
                                           WireType::kLengthPrefixed),
                                       Varint(1)))
          .ParseOnly(),
      Yields(IsParseError()));
}

INSTANTIATE_TEST_SUITE_P(
    All, PrematureEofInPackedFieldTest,
    Combine(ValuesIn(AllTestMessageTypes()), ValuesIn(PackableTypes())),
    TupleParamName<PrematureEofInPackedFieldTest::ParamType>);

// A submessage that ends in the middle of one of its own values.  The legacy
// test only ran this for TYPE_MESSAGE, so the suite is parameterized over the
// message type alone and, as in the other suites here, the gtest name is the
// rest of the conformance name: its legacy ".MESSAGE" suffix.
using PrematureEofInSubmessageValueTest = MessageTypeConformanceTest;

// TODO: b/563659437 - also run with valid data appended (legacy TODO).
TEST_P(PrematureEofInSubmessageValueTest, Message) {
  const FieldDescriptor* field = GetFieldForType(
      *message(), FieldDescriptor::TYPE_MESSAGE, /*repeated=*/false);
  // Field 5 is what the legacy test used: it (accidentally) passed
  // WireFormatLite::TYPE_INT32 == 5 as the field number.  The bytes are kept
  // identical.
  Wire incomplete_submessage(Tag(5, WireType::kVarint),
                             IncompleteValue(WireType::kVarint));
  EXPECT_THAT(
      RequiredTest("PrematureEofInSubmessageValue.MESSAGE")
          .ParseBinary(message(), LengthPrefixedField(FieldNumber(*field),
                                                      incomplete_submessage))
          .ParseOnly(),
      Yields(IsParseError()));
}

INSTANTIATE_TEST_SUITE_P(All, PrematureEofInSubmessageValueTest,
                         ValuesIn(AllTestMessageTypes()), MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
