// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Binary conformance tests checking that a tag with one of the two undefined
// wire types (6 and 7) is rejected.  This replaces the legacy
// BinaryAndJsonConformanceSuiteImpl<M>::TestUnknownWireType(); the test names
// and the requests sent to the testee are identical to the legacy ones.

#include <string>
#include <tuple>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
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
using ::testing::Range;
using ::testing::Values;
using ::testing::ValuesIn;

// Parameterized over (test message type, wire type, field number, value byte).
// Reporting the message type through MessageUnderTest() makes the base SetUp()
// skip the editions instances when --maximum_edition doesn't cover them.
class UnknownWireTypeTest : public ConformanceTest,
                            public testing::WithParamInterface<
                                std::tuple<const Descriptor*, int, int, int>> {
 protected:
  const Descriptor* MessageUnderTest() const override { return message(); }

  const Descriptor* message() const { return std::get<0>(GetParam()); }
  int wire_type() const { return std::get<1>(GetParam()); }
  int field_number() const { return std::get<2>(GetParam()); }
  int value() const { return std::get<3>(GetParam()); }
};

TEST_P(UnknownWireTypeTest, Rejected) {
  // Two bytes: a one-byte tag with the unknown wire type, then one byte of
  // "value".  Tag() can't encode wire type 7, so the tag byte is assembled by
  // hand exactly as the legacy test did.
  const std::string payload = {
      static_cast<char>((field_number() << 3) | wire_type()),
      static_cast<char>(value())};
  EXPECT_THAT(Testee(absl::StrCat("UnknownWireType", wire_type(), "_Field",
                                  field_number(), "_Version", value()))
                  .ParseBinary(message(), Wire(payload))
                  .ParseOnly(),
              Yields(IsParseError()));
}

// Instance names are "<Message>_<wire type>_<field number>_<value>".
INSTANTIATE_TEST_SUITE_P(All, UnknownWireTypeTest,
                         Combine(ValuesIn(AllTestMessageTypes()), Values(6, 7),
                                 Range(0, 4), Range(0, 4)),
                         TupleParamName<UnknownWireTypeTest::ParamType>);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
