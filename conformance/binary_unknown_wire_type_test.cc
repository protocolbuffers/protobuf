// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Binary conformance tests checking that a tag with one of the two undefined
// wire types (6 and 7) is rejected.  This replaces the legacy
// BinaryAndJsonConformanceSuiteImpl<M>::TestUnknownWireType(); the test names
// and the requests sent to the testee are identical to the legacy ones.

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
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

// Parameterized over the test message type; the test chooses the wire type,
// field number and value byte itself.  Reporting the message type through
// MessageUnderTest() makes the base SetUp() skip the editions instances when
// --maximum_edition doesn't cover them.
class UnknownWireTypeTest : public ConformanceTest {
 protected:
  const Descriptor* message() const { return message_; }

 private:
  const Descriptor* MessageUnderTest() const override { return message_; }

  const Descriptor* message_ =
      PB_TEST_CHOOSE_VALUE(AllTestMessageTypes(), MessageTypeName);
};

PB_TEST_F(UnknownWireTypeTest, Rejected) {
  const int wire_type = PB_TEST_CHOOSE_VALUE((6, 7), "WireType");
  const int field_number = PB_TEST_CHOOSE_VALUE(testing::Range(0, 4), "Field");
  const int value = PB_TEST_CHOOSE_VALUE(testing::Range(0, 4), "Value");
  // Two bytes: a one-byte tag with the unknown wire type, then one byte of
  // "value".  Tag() can't encode wire type 7, so the tag byte is assembled by
  // hand exactly as the legacy test did.
  const std::string payload = {
      static_cast<char>((field_number << 3) | wire_type),
      static_cast<char>(value)};
  EXPECT_THAT(Testee(absl::StrCat("UnknownWireType", wire_type, "_Field",
                                  field_number, "_Version", value))
                  .ParseBinary(message(), Wire(payload))
                  .ParseOnly(),
              Yields(IsParseError()));
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
