// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Binary conformance tests checking that illegal length prefixes are
// rejected.  This replaces the legacy
// BinaryAndJsonConformanceSuiteImpl<M>::TestIllegalLengths(); the requests
// sent to the testee are identical to the legacy ones.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
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

using ::testing::ValuesIn;

using IllegalLengthsTest = MessageTypeConformanceTest;

// A 5-byte varint length whose bit 32 is set (0x10 in the last byte), so that
// it wraps to 0 in 32-bit arithmetic.  Parsers must not overflow and must
// reject the length.
TEST_P(IllegalLengthsTest, BadLengthVarint32BitOverflow) {
  const FieldDescriptor& string_field = *GetFieldForType(
      *message(), FieldDescriptor::TYPE_STRING, /*repeated=*/false);
  EXPECT_THAT(Testee()
                  .ParseBinary(message(), Wire(Tag(FieldNumber(string_field),
                                                   WireType::kLengthPrefixed),
                                               "\x80\x80\x80\x80\x10"))
                  .ParseOnly(),
              Yields(IsParseError()));
}

INSTANTIATE_TEST_SUITE_P(All, IllegalLengthsTest,
                         ValuesIn(AllTestMessageTypes()), MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
