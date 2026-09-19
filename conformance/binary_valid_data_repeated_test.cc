// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Binary conformance tests checking that a repeated field accepts every valid
// encoding of its values, packed or unpacked, and that the output uses the
// packing the field declares.  For every packable type, ValidDataCases(type)
// is sent as a whole to the message's repeated field, in both encodings:
// ValidDataRepeated.<TYPE>.UnpackedInput / .PackedInput (REQUIRED,
// equivalence) and the byte-exact RECOMMENDED variants
// .<Un>PackedInput.DefaultOutput (the field with the message's default
// packing), .<Un>PackedInput.PackedOutput (the [packed = true] field) and
// .<Un>PackedInput.UnpackedOutput (the [packed = false] field).  The
// non-packable types (string, bytes, message) only have the REQUIRED
// ValidDataRepeated.<TYPE>.
//
// This replaces the repeated-field part of the legacy
// BinaryAndJsonConformanceSuiteImpl<M>::TestValidDataForType(); the test names
// and the requests sent to the testee are identical to the legacy ones.
//
// Like the legacy RunValidBinaryProtobufTest() + RunValidProtobufToJsonTest()
// pair, each REQUIRED input is sent twice: once to be serialized as binary
// ("<name>.ProtobufOutput") and once as JSON ("<name>.JsonOutput"), both
// expecting the same canonical message.  The byte-exact RECOMMENDED tests
// only have a binary leg, as before.

#include <string>
#include <tuple>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
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

// Parameterized over (test message type, field type).  Reporting the message
// type through MessageUnderTest() makes the base SetUp() skip the editions
// instances when --maximum_edition doesn't cover them.
class ValidDataRepeatedTest
    : public ConformanceTest,
      public testing::WithParamInterface<
          std::tuple<const Descriptor*, FieldDescriptor::Type>> {
 protected:
  const Descriptor* MessageUnderTest() const override { return message(); }

  const Descriptor* message() const { return std::get<0>(GetParam()); }
  FieldDescriptor::Type type() const { return std::get<1>(GetParam()); }

  // The message's repeated fields of type(): its first repeated field of
  // that type, with the message's default packing (e.g. repeated_int32, or
  // repeated_nested_enum for ENUM), and the ones declared [packed = true] and
  // [packed = false].  These scan the descriptor (and check-fail if
  // there is no such field), hence the function-style names.
  const FieldDescriptor* RepeatedField() const {
    return GetFieldForType(*message(), type(), /*repeated=*/true);
  }
  const FieldDescriptor* PackedField() const {
    return GetFieldForType(*message(), type(), /*repeated=*/true,
                           Packedness::kPacked);
  }
  const FieldDescriptor* UnpackedField() const {
    return GetFieldForType(*message(), type(), /*repeated=*/true,
                           Packedness::kUnpacked);
  }

  // All cases of type() as `field`, one tag per value, as sent to the testee
  // (the inputs) or as a conforming implementation encodes them (the canonical
  // forms).
  Wire UnpackedInput(const FieldDescriptor* field) const {
    return UnpackedCases(field, &ValidDataCase::input);
  }
  Wire UnpackedExpected(const FieldDescriptor* field) const {
    return UnpackedCases(field, &ValidDataCase::expected);
  }

  // The same values in one packed `field`.
  Wire PackedInput(const FieldDescriptor* field) const {
    return PackedCases(field, &ValidDataCase::input);
  }
  Wire PackedExpected(const FieldDescriptor* field) const {
    return PackedCases(field, &ValidDataCase::expected);
  }

  // The canonical forms encoded as RepeatedField() declares: packed by
  // default in proto3, unpacked in proto2.
  Wire DefaultExpected() const {
    return RepeatedField()->is_packed() ? PackedExpected(RepeatedField())
                                        : UnpackedExpected(RepeatedField());
  }

  // The legacy test name: "ValidDataRepeated.<TYPE><suffix>", e.g.
  // "ValidDataRepeated.INT32.PackedInput.DefaultOutput".
  std::string TestName(absl::string_view suffix = "") const {
    return absl::StrCat("ValidDataRepeated.", UpperCaseTypeName(type()),
                        suffix);
  }

 private:
  // One `column` of every case of type(), each as its own occurrence of
  // `field` ...
  Wire UnpackedCases(const FieldDescriptor* field,
                     Wire ValidDataCase::* column) const {
    const Wire tag = Tag(FieldNumber(*field), WireTypeForFieldType(type()));
    std::string bytes;
    for (const ValidDataCase& value : ValidDataCases(type())) {
      absl::StrAppend(&bytes, tag.data(), (value.*column).data());
    }
    return Wire(bytes);
  }

  // ... or concatenated into one packed `field`.
  Wire PackedCases(const FieldDescriptor* field,
                   Wire ValidDataCase::* column) const {
    std::string bytes;
    for (const ValidDataCase& value : ValidDataCases(type())) {
      absl::StrAppend(&bytes, (value.*column).data());
    }
    return LengthPrefixedField(FieldNumber(*field), bytes);
  }
};

// Both encodings of the default field must be accepted and yield the same
// values, whether serialized as binary or as JSON.
TEST_P(ValidDataRepeatedTest, UnpackedInput) {
  EXPECT_THAT(RequiredTest(TestName(".UnpackedInput"))
                  .ParseBinary(message(), UnpackedInput(RepeatedField()))
                  .SerializeBinary(),
              Yields(ParsedPayload(
                  EqualsBinaryProto(PackedExpected(RepeatedField())))));
}

TEST_P(ValidDataRepeatedTest, UnpackedInputJson) {
  EXPECT_THAT(RequiredTest(TestName(".UnpackedInput"))
                  .ParseBinary(message(), UnpackedInput(RepeatedField()))
                  .SerializeJson(),
              Yields(ParsedPayload(
                  EqualsBinaryProto(PackedExpected(RepeatedField())))));
}

TEST_P(ValidDataRepeatedTest, PackedInput) {
  EXPECT_THAT(RequiredTest(TestName(".PackedInput"))
                  .ParseBinary(message(), PackedInput(RepeatedField()))
                  .SerializeBinary(),
              Yields(ParsedPayload(
                  EqualsBinaryProto(PackedExpected(RepeatedField())))));
}

TEST_P(ValidDataRepeatedTest, PackedInputJson) {
  EXPECT_THAT(RequiredTest(TestName(".PackedInput"))
                  .ParseBinary(message(), PackedInput(RepeatedField()))
                  .SerializeJson(),
              Yields(ParsedPayload(
                  EqualsBinaryProto(PackedExpected(RepeatedField())))));
}

// The default field must be serialized with its declared packing whatever
// the input's encoding was ...
TEST_P(ValidDataRepeatedTest, UnpackedInputDefaultOutput) {
  EXPECT_THAT(RecommendedTest(TestName(".UnpackedInput.DefaultOutput"))
                  .ParseBinary(message(), UnpackedInput(RepeatedField()))
                  .SerializeBinary(),
              Yields(Payload(DefaultExpected())));
}

TEST_P(ValidDataRepeatedTest, PackedInputDefaultOutput) {
  EXPECT_THAT(RecommendedTest(TestName(".PackedInput.DefaultOutput"))
                  .ParseBinary(message(), PackedInput(RepeatedField()))
                  .SerializeBinary(),
              Yields(Payload(DefaultExpected())));
}

// ... and so must the explicitly packed ...
TEST_P(ValidDataRepeatedTest, UnpackedInputPackedOutput) {
  EXPECT_THAT(RecommendedTest(TestName(".UnpackedInput.PackedOutput"))
                  .ParseBinary(message(), UnpackedInput(PackedField()))
                  .SerializeBinary(),
              Yields(Payload(PackedExpected(PackedField()))));
}

TEST_P(ValidDataRepeatedTest, PackedInputPackedOutput) {
  EXPECT_THAT(RecommendedTest(TestName(".PackedInput.PackedOutput"))
                  .ParseBinary(message(), PackedInput(PackedField()))
                  .SerializeBinary(),
              Yields(Payload(PackedExpected(PackedField()))));
}

// ... and explicitly unpacked fields.
TEST_P(ValidDataRepeatedTest, UnpackedInputUnpackedOutput) {
  EXPECT_THAT(RecommendedTest(TestName(".UnpackedInput.UnpackedOutput"))
                  .ParseBinary(message(), UnpackedInput(UnpackedField()))
                  .SerializeBinary(),
              Yields(Payload(UnpackedExpected(UnpackedField()))));
}

TEST_P(ValidDataRepeatedTest, PackedInputUnpackedOutput) {
  EXPECT_THAT(RecommendedTest(TestName(".PackedInput.UnpackedOutput"))
                  .ParseBinary(message(), PackedInput(UnpackedField()))
                  .SerializeBinary(),
              Yields(Payload(UnpackedExpected(UnpackedField()))));
}

INSTANTIATE_TEST_SUITE_P(All, ValidDataRepeatedTest,
                         Combine(ValuesIn(AllTestMessageTypes()),
                                 ValuesIn(PackableFieldTypes())),
                         TupleParamName<ValidDataRepeatedTest::ParamType>);

// The non-packable types only have one encoding; the conformance name has no
// suffix.  Of the inherited helpers only RepeatedField(), UnpackedInput() and
// UnpackedExpected() apply here; the packed ones would check-fail.
class ValidDataRepeatedNonPackableTest : public ValidDataRepeatedTest {};

TEST_P(ValidDataRepeatedNonPackableTest, Repeated) {
  EXPECT_THAT(RequiredTest(TestName())
                  .ParseBinary(message(), UnpackedInput(RepeatedField()))
                  .SerializeBinary(),
              Yields(ParsedPayload(
                  EqualsBinaryProto(UnpackedExpected(RepeatedField())))));
}

TEST_P(ValidDataRepeatedNonPackableTest, RepeatedJson) {
  EXPECT_THAT(RequiredTest(TestName())
                  .ParseBinary(message(), UnpackedInput(RepeatedField()))
                  .SerializeJson(),
              Yields(ParsedPayload(
                  EqualsBinaryProto(UnpackedExpected(RepeatedField())))));
}

INSTANTIATE_TEST_SUITE_P(
    All, ValidDataRepeatedNonPackableTest,
    Combine(ValuesIn(AllTestMessageTypes()),
            ValuesIn(LengthDelimitedFieldTypes())),
    TupleParamName<ValidDataRepeatedNonPackableTest::ParamType>);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
