// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Binary conformance tests for map fields: ValidDataMap.<KEY>.<VALUE>.<Case>
// (REQUIRED, equivalence) checks that a map entry is parsed correctly whatever
// the order, presence and repetition of its key and value, for every key/value
// type combination in ValidDataMapTypes(); and the RECOMMENDED
// .KeyWireTypeMismatch / .ValueWireTypeMismatch tests check that an entry
// whose key or value is encoded with the wrong wire type is either rejected or
// parsed as if that field were unknown.
//
// This replaces the legacy
// BinaryAndJsonConformanceSuiteImpl<M>::TestValidDataForMapType() and
// TestMapEntryWireTypeMismatch(); the requests sent to the testee are identical
// to the legacy ones.  Like the legacy RunValidProtobufTest(), each
// ValidDataMap input is sent twice: once to be serialized as binary
// ("<name>.ProtobufOutput") and once as JSON ("<name>.JsonOutput"), and both
// outputs must be equivalent to the same reference message.  The wire-type
// mismatch tests were binary-only.

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

using ::testing::AnyOf;
using ::testing::Combine;
using ::testing::ValuesIn;

// Map entries are messages with key = 1 and value = 2.
constexpr uint32_t kKeyField = 1;
constexpr uint32_t kValueField = 2;

// The five key/value type combinations the legacy suite ran the wire-type
// mismatch tests over, in its order: varint, fixed32 and bool scalars, string,
// and a message-valued map.
constexpr MapType kMapEntryWireTypeMismatchTypes[] = {
    {FieldDescriptor::TYPE_INT32, FieldDescriptor::TYPE_INT32},
    {FieldDescriptor::TYPE_FIXED32, FieldDescriptor::TYPE_FIXED32},
    {FieldDescriptor::TYPE_BOOL, FieldDescriptor::TYPE_BOOL},
    {FieldDescriptor::TYPE_STRING, FieldDescriptor::TYPE_STRING},
    {FieldDescriptor::TYPE_STRING, FieldDescriptor::TYPE_MESSAGE},
};

// Field `field_number` encoded with a wire type other than
// `correct_wire_type`: a one-byte length-delimited "a" for scalar fields, and
// the varint 1 for length-delimited ones.
Wire MismatchedWireTypeField(uint32_t field_number,
                             WireType correct_wire_type) {
  if (correct_wire_type == WireType::kLengthPrefixed) {
    return VarintField(field_number, 1);
  }
  return LengthPrefixedField(field_number, "a");
}

// Common part of the fixtures below: parameterized over (test message type,
// map type), exercising the message's map field with those key and value
// types.  Reporting the message type through MessageUnderTest() makes the base
// SetUp() skip the editions instances when --maximum_edition doesn't cover
// them.
class MapTestBase : public ConformanceTest,
                    public testing::WithParamInterface<
                        std::tuple<const Descriptor*, MapType>> {
 protected:
  const Descriptor* MessageUnderTest() const override { return message(); }

  const Descriptor* message() const { return std::get<0>(GetParam()); }
  FieldDescriptor::Type key_type() const { return std::get<1>(GetParam()).key; }
  FieldDescriptor::Type value_type() const {
    return std::get<1>(GetParam()).value;
  }

  // The key and value fields of an entry holding the default and the
  // non-default sample value of their types (GetDefaultValue() and
  // GetNonDefaultValue() with the entry field's tag in front).
  Wire DefaultKeyField() const { return Key(GetDefaultValue(key_type())); }
  Wire NonDefaultKeyField() const {
    return Key(GetNonDefaultValue(key_type()));
  }
  Wire DefaultValueField() const {
    return Value(GetDefaultValue(value_type()));
  }
  Wire NonDefaultValueField() const {
    return Value(GetNonDefaultValue(value_type()));
  }

  // The key and value fields of an entry encoded with the wrong wire type.
  Wire MismatchedKeyField() const {
    return MismatchedWireTypeField(kKeyField, WireTypeForFieldType(key_type()));
  }
  Wire MismatchedValueField() const {
    return MismatchedWireTypeField(kValueField,
                                   WireTypeForFieldType(value_type()));
  }

  // `entry` as one entry of the message's map field for (key_type(),
  // value_type()).  This scans the descriptor (and check-fails if there is no
  // such field), hence the function-style name.
  Wire MapEntry(const Wire& entry) const {
    return LengthPrefixedField(
        FieldNumber(*GetFieldForMapType(*message(), key_type(), value_type())),
        entry);
  }

 private:
  Wire Key(const Wire& payload) const {
    return Wire(Tag(kKeyField, WireTypeForFieldType(key_type())), payload);
  }
  Wire Value(const Wire& payload) const {
    return Wire(Tag(kValueField, WireTypeForFieldType(value_type())), payload);
  }
};

// Every entry below must parse to the same map as a conforming parser (the
// C++ implementation) reads from it.
class ValidDataMapTest : public MapTestBase {};

// An entry with the default key and value, both present.
TEST_P(ValidDataMapTest, Default) {
  const Wire input = MapEntry(Wire(DefaultKeyField(), DefaultValueField()));
  EXPECT_THAT(Testee().ParseBinary(message(), input).SerializeBinary(),
              Yields(ParsedPayload(EqualsBinaryProto(input))));
}

TEST_P(ValidDataMapTest, DefaultJson) {
  const Wire input = MapEntry(Wire(DefaultKeyField(), DefaultValueField()));
  EXPECT_THAT(Testee().ParseBinary(message(), input).SerializeJson(),
              Yields(ParsedPayload(EqualsBinaryProto(input))));
}

// An empty entry: the key and value are both missing and take their defaults.
TEST_P(ValidDataMapTest, MissingDefault) {
  const Wire input = MapEntry(Wire());
  EXPECT_THAT(Testee().ParseBinary(message(), input).SerializeBinary(),
              Yields(ParsedPayload(EqualsBinaryProto(input))));
}

TEST_P(ValidDataMapTest, MissingDefaultJson) {
  const Wire input = MapEntry(Wire());
  EXPECT_THAT(Testee().ParseBinary(message(), input).SerializeJson(),
              Yields(ParsedPayload(EqualsBinaryProto(input))));
}

// An entry with a non-default key and value.
TEST_P(ValidDataMapTest, NonDefault) {
  const Wire input =
      MapEntry(Wire(NonDefaultKeyField(), NonDefaultValueField()));
  EXPECT_THAT(Testee().ParseBinary(message(), input).SerializeBinary(),
              Yields(ParsedPayload(EqualsBinaryProto(input))));
}

TEST_P(ValidDataMapTest, NonDefaultJson) {
  const Wire input =
      MapEntry(Wire(NonDefaultKeyField(), NonDefaultValueField()));
  EXPECT_THAT(Testee().ParseBinary(message(), input).SerializeJson(),
              Yields(ParsedPayload(EqualsBinaryProto(input))));
}

// The value before the key.
TEST_P(ValidDataMapTest, Unordered) {
  const Wire input =
      MapEntry(Wire(NonDefaultValueField(), NonDefaultKeyField()));
  EXPECT_THAT(Testee().ParseBinary(message(), input).SerializeBinary(),
              Yields(ParsedPayload(EqualsBinaryProto(input))));
}

TEST_P(ValidDataMapTest, UnorderedJson) {
  const Wire input =
      MapEntry(Wire(NonDefaultValueField(), NonDefaultKeyField()));
  EXPECT_THAT(Testee().ParseBinary(message(), input).SerializeJson(),
              Yields(ParsedPayload(EqualsBinaryProto(input))));
}

// Two entries with the same key: the second one replaces the first.
TEST_P(ValidDataMapTest, DuplicateKey) {
  const Wire first = MapEntry(Wire(NonDefaultKeyField(), DefaultValueField()));
  const Wire second =
      MapEntry(Wire(NonDefaultKeyField(), NonDefaultValueField()));
  EXPECT_THAT(
      Testee().ParseBinary(message(), Wire(first, second)).SerializeBinary(),
      Yields(ParsedPayload(EqualsBinaryProto(second))));
}

TEST_P(ValidDataMapTest, DuplicateKeyJson) {
  const Wire first = MapEntry(Wire(NonDefaultKeyField(), DefaultValueField()));
  const Wire second =
      MapEntry(Wire(NonDefaultKeyField(), NonDefaultValueField()));
  EXPECT_THAT(
      Testee().ParseBinary(message(), Wire(first, second)).SerializeJson(),
      Yields(ParsedPayload(EqualsBinaryProto(second))));
}

// The key twice within one entry: the last one wins.
TEST_P(ValidDataMapTest, DuplicateKeyInMapEntry) {
  const Wire input = MapEntry(
      Wire(DefaultKeyField(), NonDefaultKeyField(), NonDefaultValueField()));
  EXPECT_THAT(Testee().ParseBinary(message(), input).SerializeBinary(),
              Yields(ParsedPayload(EqualsBinaryProto(input))));
}

TEST_P(ValidDataMapTest, DuplicateKeyInMapEntryJson) {
  const Wire input = MapEntry(
      Wire(DefaultKeyField(), NonDefaultKeyField(), NonDefaultValueField()));
  EXPECT_THAT(Testee().ParseBinary(message(), input).SerializeJson(),
              Yields(ParsedPayload(EqualsBinaryProto(input))));
}

// The value twice within one entry: the last one wins (or, for message
// values, they merge -- the same result here).
TEST_P(ValidDataMapTest, DuplicateValueInMapEntry) {
  const Wire input = MapEntry(
      Wire(NonDefaultKeyField(), DefaultValueField(), NonDefaultValueField()));
  EXPECT_THAT(Testee().ParseBinary(message(), input).SerializeBinary(),
              Yields(ParsedPayload(EqualsBinaryProto(input))));
}

TEST_P(ValidDataMapTest, DuplicateValueInMapEntryJson) {
  const Wire input = MapEntry(
      Wire(NonDefaultKeyField(), DefaultValueField(), NonDefaultValueField()));
  EXPECT_THAT(Testee().ParseBinary(message(), input).SerializeJson(),
              Yields(ParsedPayload(EqualsBinaryProto(input))));
}

INSTANTIATE_TEST_SUITE_P(All, ValidDataMapTest,
                         Combine(ValuesIn(AllTestMessageTypes()),
                                 ValuesIn(ValidDataMapTypes())),
                         TupleParamName<ValidDataMapTest::ParamType>);

// An entry whose key or value is encoded with the wrong wire type.  A parser
// may reject it or treat the mismatched field as unknown, so that the key or
// value takes its default (the C++ implementation does the latter); what it
// must not do is decode the bytes as a value of the declared type.
class MapEntryWireTypeMismatchTest : public MapTestBase {
 public:
  TestPriority DefaultPriority() const override { return kP3; }
};

// The key has the wrong wire type; the value is still well formed.
TEST_P(MapEntryWireTypeMismatchTest, KeyWireTypeMismatch) {
  const Wire input =
      MapEntry(Wire(MismatchedKeyField(), NonDefaultValueField()));
  EXPECT_THAT(
      Testee().ParseBinary(message(), input).SerializeBinary(),
      Yields(AnyOf(IsParseError(), ParsedPayload(EqualsBinaryProto(input)))));
}

// The value has the wrong wire type; the key is still well formed.
TEST_P(MapEntryWireTypeMismatchTest, ValueWireTypeMismatch) {
  const Wire input =
      MapEntry(Wire(NonDefaultKeyField(), MismatchedValueField()));
  EXPECT_THAT(
      Testee().ParseBinary(message(), input).SerializeBinary(),
      Yields(AnyOf(IsParseError(), ParsedPayload(EqualsBinaryProto(input)))));
}

INSTANTIATE_TEST_SUITE_P(
    All, MapEntryWireTypeMismatchTest,
    Combine(ValuesIn(AllTestMessageTypes()),
            ValuesIn(kMapEntryWireTypeMismatchTypes)),
    TupleParamName<MapEntryWireTypeMismatchTest::ParamType>);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
