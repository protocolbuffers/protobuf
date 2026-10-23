// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/base/nullability.h"
#include "absl/log/absl_log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "conformance/binary_test_util.h"
#include "conformance/binary_wireformat.h"
#include "conformance/matchers.h"
#include "conformance/test_fixture.h"
#include "conformance/testee.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/testing/protobuf_test.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

// A field number no test message defines.
constexpr uint32_t kUnknownFieldNumber = 666;

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

// Common part of the fixtures below: a test message type and a field type.
// The three test suites only differ in the field types they choose, so that no
// test ever has to skip itself as "not applicable".  Reporting the message
// type through MessageUnderTest() makes the base SetUp() skip the editions
// instances when --maximum_edition doesn't cover them.
class PrematureEofTestBase : public ConformanceTest {
 protected:
  PB_TEST_FIXTURE_BASE();

  const Descriptor* message() const { return message_; }
  virtual FieldDescriptor::Type type() const = 0;
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

 private:
  const Descriptor* MessageUnderTest() const override { return message(); }

  const Descriptor* absl_nonnull message_ =
      PB_TEST_CHOOSE_VALUE(AllTestMessageTypes(), MessageTypeName);
};

// Every field type except groups.
class PrematureEofTest : public PrematureEofTestBase {
 protected:
  FieldDescriptor::Type type() const override { return type_; }

 private:
  FieldDescriptor::Type type_ =
      PB_TEST_CHOOSE_VALUE(AllFieldTypesExceptGroup(), FieldTypeName);
};

PB_TEST_F(PrematureEofTest, BeforeKnownNonRepeatedValue) {
  EXPECT_THAT(
      Testee(TestName("PrematureEofBeforeKnownNonRepeatedValue"))
          .ParseBinary(message(), Tag(FieldNumber(*Field()), wire_type()))
          .ParseOnly(),
      Yields(IsParseError()));
}

PB_TEST_F(PrematureEofTest, BeforeKnownRepeatedValue) {
  EXPECT_THAT(Testee(TestName("PrematureEofBeforeKnownRepeatedValue"))
                  .ParseBinary(message(),
                               Tag(FieldNumber(*RepeatedField()), wire_type()))
                  .ParseOnly(),
              Yields(IsParseError()));
}

PB_TEST_F(PrematureEofTest, BeforeUnknownValue) {
  EXPECT_THAT(Testee(TestName("PrematureEofBeforeUnknownValue"))
                  .ParseBinary(message(), Tag(kUnknownFieldNumber, wire_type()))
                  .ParseOnly(),
              Yields(IsParseError()));
}

PB_TEST_F(PrematureEofTest, InsideKnownNonRepeatedValue) {
  EXPECT_THAT(
      Testee(TestName("PrematureEofInsideKnownNonRepeatedValue"))
          .ParseBinary(message(), Wire(Tag(FieldNumber(*Field()), wire_type()),
                                       incomplete()))
          .ParseOnly(),
      Yields(IsParseError()));
}

PB_TEST_F(PrematureEofTest, InsideKnownRepeatedValue) {
  EXPECT_THAT(
      Testee(TestName("PrematureEofInsideKnownRepeatedValue"))
          .ParseBinary(message(),
                       Wire(Tag(FieldNumber(*RepeatedField()), wire_type()),
                            incomplete()))
          .ParseOnly(),
      Yields(IsParseError()));
}

PB_TEST_F(PrematureEofTest, InsideUnknownValue) {
  EXPECT_THAT(
      Testee(TestName("PrematureEofInsideUnknownValue"))
          .ParseBinary(message(), Wire(Tag(kUnknownFieldNumber, wire_type()),
                                       incomplete()))
          .ParseOnly(),
      Yields(IsParseError()));
}

// A length prefix that promises more data than follows.
class PrematureEofInDelimitedDataTest : public PrematureEofTestBase {
 protected:
  FieldDescriptor::Type type() const override { return type_; }

 private:
  FieldDescriptor::Type type_ =
      PB_TEST_CHOOSE_VALUE(LengthDelimitedFieldTypes(), FieldTypeName);
};

PB_TEST_F(PrematureEofInDelimitedDataTest, ForKnownNonRepeatedValue) {
  EXPECT_THAT(
      Testee(TestName("PrematureEofInDelimitedDataForKnownNonRepeatedValue"))
          .ParseBinary(message(), Wire(Tag(FieldNumber(*Field()),
                                           WireType::kLengthPrefixed),
                                       Varint(1)))
          .ParseOnly(),
      Yields(IsParseError()));
}

PB_TEST_F(PrematureEofInDelimitedDataTest, ForKnownRepeatedValue) {
  EXPECT_THAT(
      Testee(TestName("PrematureEofInDelimitedDataForKnownRepeatedValue"))
          .ParseBinary(message(), Wire(Tag(FieldNumber(*RepeatedField()),
                                           WireType::kLengthPrefixed),
                                       Varint(1)))
          .ParseOnly(),
      Yields(IsParseError()));
}

PB_TEST_F(PrematureEofInDelimitedDataTest, ForUnknownValue) {
  EXPECT_THAT(Testee(TestName("PrematureEofInDelimitedDataForUnknownValue"))
                  .ParseBinary(message(), Wire(Tag(kUnknownFieldNumber,
                                                   WireType::kLengthPrefixed),
                                               Varint(1)))
                  .ParseOnly(),
              Yields(IsParseError()));
}

// A packed repeated field whose payload is truncated.
class PrematureEofInPackedFieldTest : public PrematureEofTestBase {
 protected:
  FieldDescriptor::Type type() const override { return type_; }

 private:
  FieldDescriptor::Type type_ =
      PB_TEST_CHOOSE_VALUE(PackableFieldTypes(), FieldTypeName);
};

// The packed region ends in the middle of one of its values.
// TODO: also run with valid data appended (legacy TODO).
PB_TEST_F(PrematureEofInPackedFieldTest, PackedFieldValue) {
  EXPECT_THAT(Testee(TestName("PrematureEofInPackedFieldValue"))
                  .ParseBinary(message(),
                               LengthPrefixedField(
                                   FieldNumber(*RepeatedField()), incomplete()))
                  .ParseOnly(),
              Yields(IsParseError()));
}

// The payload ends before the packed region does.
PB_TEST_F(PrematureEofInPackedFieldTest, PackedField) {
  EXPECT_THAT(
      Testee(TestName("PrematureEofInPackedField"))
          .ParseBinary(message(), Wire(Tag(FieldNumber(*RepeatedField()),
                                           WireType::kLengthPrefixed),
                                       Varint(1)))
          .ParseOnly(),
      Yields(IsParseError()));
}

// A submessage that ends in the middle of one of its own values.  The legacy
// test only ran this for TYPE_MESSAGE, so the suite is parameterized over the
// message type alone and, as in the other suites here, the gtest name is the
// rest of the conformance name: its legacy ".MESSAGE" suffix.
class PrematureEofInSubmessageValueTest : public PrematureEofTestBase {
 protected:
  FieldDescriptor::Type type() const override {
    return FieldDescriptor::TYPE_MESSAGE;
  }
};

// TODO: also run with valid data appended (legacy TODO).
PB_TEST_F(PrematureEofInSubmessageValueTest, Message) {
  // Field 5 is what the legacy test used: it (accidentally) passed
  // WireFormatLite::TYPE_INT32 == 5 as the field number.  The bytes are kept
  // identical.
  Wire incomplete_submessage(Tag(5, WireType::kVarint),
                             IncompleteValue(WireType::kVarint));
  EXPECT_THAT(
      Testee("PrematureEofInSubmessageValue.MESSAGE")
          .ParseBinary(message(), LengthPrefixedField(FieldNumber(*Field()),
                                                      incomplete_submessage))
          .ParseOnly(),
      Yields(IsParseError()));
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
