// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Binary performance conformance tests: payloads with tens of thousands of
// unknown fields, or of submessages that have to be merged into one.  Part of
// the performance suite (see conformance.bzl).  This replaces the legacy
// BinaryAndJsonConformanceSuiteImpl<M>::RunBinaryPerformanceTests()
// and its helpers; the test names and the requests sent to the testee are
// identical to the legacy ones.

#include <cstddef>
#include <cstdint>
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
#include "conformance/testee.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::Combine;
using ::testing::ValuesIn;

// The number of repetitions to use for performance tests.
// Corresponds approx to 500KB wireformat bytes.
constexpr int kPerformanceRepeatCount = 50000;

// Field numbers no test message defines.
constexpr uint32_t kUnknownFieldNumber = 666;
constexpr uint32_t kOtherUnknownFieldNumber = 667;

// Field 27 of every test message is recursive_message, a submessage of the
// message's own type.
constexpr uint32_t kRecursiveMessageField = 27;

// The field types the merge tests are instantiated with, in the legacy order.
constexpr FieldDescriptor::Type kMergeTypes[] = {
    FieldDescriptor::TYPE_BOOL,   FieldDescriptor::TYPE_DOUBLE,
    FieldDescriptor::TYPE_FLOAT,  FieldDescriptor::TYPE_UINT32,
    FieldDescriptor::TYPE_UINT64, FieldDescriptor::TYPE_STRING,
    FieldDescriptor::TYPE_BYTES,
};

// `wire` repeated `count` times.
Wire Repeat(const Wire& wire, int count) {
  std::string repeated;
  repeated.reserve(wire.size() * static_cast<size_t>(count));
  for (int i = 0; i < count; ++i) {
    absl::StrAppend(&repeated, wire.data());
  }
  return Wire(repeated);
}

// Parameterized over the test message type only.
class UnknownFieldsPerformanceTest : public MessageTypeConformanceTest {
 public:
  TestPriority DefaultPriority() const override { return kP3; }
};

// Many unknown fields alternating between two field numbers must be preserved
// byte for byte.
TEST_P(UnknownFieldsPerformanceTest, AlternatingUnknownFields) {
  Wire input = Repeat(Wire(VarintField(kUnknownFieldNumber, 1234),
                           VarintField(kOtherUnknownFieldNumber, 5678)),
                      kPerformanceRepeatCount);
  EXPECT_THAT(Testee("TestBinaryPerformanceForAlternatingUnknownFields")
                  .ParseBinary(message(), input)
                  .SerializeBinary(),
              Yields(Payload(input)));
}

INSTANTIATE_TEST_SUITE_P(All, UnknownFieldsPerformanceTest,
                         ValuesIn(AllTestMessageTypes()), MessageTypeParamName);

// Parameterized over (test message type, field type): many occurrences of
// recursive_message, each holding one field, must be merged into a single
// recursive_message holding all of them, in order.
class MergeMessagePerformanceTest
    : public ConformanceTest,
      public testing::WithParamInterface<
          std::tuple<const Descriptor*, FieldDescriptor::Type>> {
 public:
  TestPriority DefaultPriority() const override { return kP3; }

 protected:
  const Descriptor* MessageUnderTest() const override { return message(); }

  const Descriptor* message() const { return std::get<0>(GetParam()); }
  FieldDescriptor::Type type() const { return std::get<1>(GetParam()); }

  // The legacy test name: "<prefix>.<TYPE>", e.g.
  // "TestBinaryPerformanceMergeMessageWithRepeatedFieldForType.BOOL".
  std::string TestName(absl::string_view prefix) const {
    return absl::StrCat(prefix, ".", UpperCaseTypeName(type()));
  }
};

// The input of a merge performance test: recursive_message holding `field` (a
// single encoded field, tag and value), kPerformanceRepeatCount times.
Wire RepeatedRecursiveMessageInput(const Wire& field) {
  return Repeat(LengthPrefixedField(kRecursiveMessageField, field),
                kPerformanceRepeatCount);
}

// What a parser must merge RepeatedRecursiveMessageInput(field) into: one
// recursive_message holding kPerformanceRepeatCount occurrences of `field`.
Wire MergedRecursiveMessage(const Wire& field) {
  return LengthPrefixedField(kRecursiveMessageField,
                             Repeat(field, kPerformanceRepeatCount));
}

// The field is the message's unpacked repeated field of the type.
TEST_P(MergeMessagePerformanceTest, WithRepeatedField) {
  const FieldDescriptor* field = GetFieldForType(
      *message(), type(), /*repeated=*/true, Packedness::kUnpacked);
  Wire entry(Tag(FieldNumber(*field), WireTypeForFieldType(type())),
             GetNonDefaultValue(type()));
  EXPECT_THAT(
      Testee(
          TestName("TestBinaryPerformanceMergeMessageWithRepeatedFieldForType"))
          .ParseBinary(message(), RepeatedRecursiveMessageInput(entry))
          .SerializeBinary(),
      Yields(Payload(MergedRecursiveMessage(entry))));
}

// The field is unknown, with the wire type of the type.
TEST_P(MergeMessagePerformanceTest, WithUnknownField) {
  Wire entry(Tag(kUnknownFieldNumber, WireTypeForFieldType(type())),
             GetNonDefaultValue(type()));
  EXPECT_THAT(
      Testee(
          TestName("TestBinaryPerformanceMergeMessageWithUnknownFieldForType"))
          .ParseBinary(message(), RepeatedRecursiveMessageInput(entry))
          .SerializeBinary(),
      Yields(Payload(MergedRecursiveMessage(entry))));
}

INSTANTIATE_TEST_SUITE_P(
    All, MergeMessagePerformanceTest,
    Combine(ValuesIn(AllTestMessageTypes()), ValuesIn(kMergeTypes)),
    TupleParamName<MergeMessagePerformanceTest::ParamType>);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
