// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Binary conformance tests checking that every valid encoding of a singular
// field's value is parsed correctly: ValidDataScalar.<TYPE>[i] (REQUIRED,
// equivalence) and ValidDataScalarBinary.<TYPE>[i] (RECOMMENDED, byte-exact)
// for case i of ValidDataCases(type), and RepeatedScalarSelectsLast.<TYPE>,
// which sends all cases for the singular field at once and expects the last
// one.  This holds the binary-output leg of the singular-field part of the
// legacy BinaryAndJsonConformanceSuiteImpl<M>::TestValidDataForType(); the
// test names and the requests sent to the testee are identical to the legacy
// ones, and the legacy suite iterates the same value tables for the
// binary->JSON legs it still sends.
//
// TODO: The binary->JSON legs of these tests join the JSON suite
// once JSON matching is migrated.

#include <cstddef>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/base/nullability.h"
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

// AllFieldTypesExceptGroup() without TYPE_MESSAGE: repeated occurrences of a
// singular message field are merged, not replaced, which binary_merge_test.cc
// covers.
std::vector<FieldDescriptor::Type> SelectsLastTypes() {
  std::vector<FieldDescriptor::Type> types;
  for (FieldDescriptor::Type type : AllFieldTypesExceptGroup()) {
    if (type != FieldDescriptor::TYPE_MESSAGE) types.push_back(type);
  }
  return types;
}

// Common part of the fixtures below: a test message type and a field type,
// exercising the message's singular field of that type.  Reporting the
// message type through MessageUnderTest() makes the base SetUp() skip the
// editions instances when --maximum_edition doesn't cover them.
class ValidDataTestBase : public ConformanceTest {
 protected:
  PB_TEST_FIXTURE_BASE();

  const Descriptor* message() const { return message_; }
  virtual FieldDescriptor::Type type() const = 0;

  // `payload` (a value without a tag) as the message's singular field of
  // type().  This scans the descriptor (and check-fails if there is no such
  // field), hence the function-style name.
  Wire FieldWithValue(const Wire& payload) const {
    return Wire(Tag(FieldNumber(*GetFieldForType(*message(), type(),
                                                 /*repeated=*/false)),
                    WireTypeForFieldType(type())),
                payload);
  }

  // The legacy test name: "<prefix>.<TYPE><suffix>", e.g.
  // "ValidDataScalar.INT32[3]".
  std::string TestName(absl::string_view prefix,
                       absl::string_view suffix = "") const {
    return absl::StrCat(prefix, ".", UpperCaseTypeName(type()), suffix);
  }

 private:
  const Descriptor* MessageUnderTest() const override { return message(); }

  const Descriptor* absl_nonnull message_ =
      PB_TEST_CHOOSE_VALUE(AllTestMessageTypes(), MessageTypeName);
};

// A field type and the index of one of its ValidDataCases().
struct ScalarCase {
  FieldDescriptor::Type type;
  int index;
};

// AllFieldTypesExceptGroup() x the case indices of each type.
std::vector<ScalarCase> ScalarCases() {
  std::vector<ScalarCase> cases;
  for (FieldDescriptor::Type type : AllFieldTypesExceptGroup()) {
    const int count = static_cast<int>(ValidDataCases(type).size());
    for (int i = 0; i < count; ++i) cases.push_back({type, i});
  }
  return cases;
}

// "<TYPE>_<index>", e.g. "FLOAT_2".
std::string ScalarCaseName(const ScalarCase& scalar_case, size_t index) {
  return absl::StrCat(FieldTypeName(scalar_case.type, index), "_",
                      scalar_case.index);
}

// One gtest per (message type, field type, case index), so that every
// conformance test is its own gtest and no test body loops or skips.
class ValidDataScalarTest : public ValidDataTestBase {
 protected:
  FieldDescriptor::Type type() const override { return case_.type; }
  int index() const { return case_.index; }
  const ValidDataCase& value() const {
    return ValidDataCases(type())[static_cast<size_t>(index())];
  }

  // The canonical serialization of the parsed input.  A proto3 message
  // doesn't serialize a singular scalar field holding its default value, so
  // the expected output is then empty; this is the legacy suite's proto3
  // rule.
  Wire Expected() const {
    if (HasImplicitPresence(*message()) &&
        IsDefaultValue(type(), value().expected)) {
      return Wire();
    }
    return FieldWithValue(value().expected);
  }

  std::string TestName(absl::string_view prefix) const {
    return ValidDataTestBase::TestName(prefix, absl::StrCat("[", index(), "]"));
  }

 private:
  ScalarCase case_ = PB_TEST_CHOOSE_VALUE(ScalarCases(), ScalarCaseName);
};

// The parsed value must be equivalent to the case's canonical form.
PB_TEST_F(ValidDataScalarTest, Scalar) {
  EXPECT_THAT(Testee(TestName("ValidDataScalar"))
                  .ParseBinary(message(), FieldWithValue(value().input))
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsBinaryProto(Expected()))));
}

// ... and its serialization must be exactly the canonical form.
PB_TEST_F(ValidDataScalarTest, ScalarBinary) {
  EXPECT_THAT(Testee(kP1, TestName("ValidDataScalarBinary"))
                  .ParseBinary(message(), FieldWithValue(value().input))
                  .SerializeBinary(),
              Yields(RawPayload(Expected())));
}

// Several occurrences of a singular scalar field: the last one wins.  The
// input is every case of the type in order, so the last case's canonical form
// is expected.  Unlike the singular tests above, the legacy test didn't apply
// the proto3 default rule here (no type's last case is a default value
// anyway).
class RepeatedScalarSelectsLastTest : public ValidDataTestBase {
 protected:
  FieldDescriptor::Type type() const override { return type_; }

 private:
  FieldDescriptor::Type type_ =
      PB_TEST_CHOOSE_VALUE(SelectsLastTypes(), FieldTypeName);
};

PB_TEST_F(RepeatedScalarSelectsLastTest, SelectsLast) {
  Wire input;
  for (const ValidDataCase& value : ValidDataCases(type())) {
    input = Wire(input, FieldWithValue(value.input));
  }
  EXPECT_THAT(Testee(TestName("RepeatedScalarSelectsLast"))
                  .ParseBinary(message(), input)
                  .SerializeBinary(),
              Yields(WhenParsed(EqualsBinaryProto(
                  FieldWithValue(ValidDataCases(type()).back().expected)))));
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
