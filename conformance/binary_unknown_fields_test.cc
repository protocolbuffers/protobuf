// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Binary conformance tests checking that unknown fields are preserved.  This
// replaces the legacy
// BinaryAndJsonConformanceSuiteImpl<M>::TestUnknownMessage(); the test names
// and the requests sent to the testee are identical to the legacy ones.
//
// TODO: b/410122237 - TestUnknownOrdering(), which inspects the testee's
// unknown field set directly, still lives in the legacy suite and joins this
// file once the framework can express that check.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "conformance/binary_test_util.h"
#include "conformance/binary_wireformat.h"
#include "conformance/matchers.h"
#include "conformance/message_type_fixtures.h"
#include "conformance/test_environment.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::ValuesIn;

using UnknownFieldsTest = MessageTypeConformanceTest;

// An unknown varint field must be round-tripped byte for byte.  Field 501
// isn't defined by any of the test messages; the legacy test spelled the input
// as the bytes "\xA8\x1F\x01".
TEST_P(UnknownFieldsTest, UnknownVarint) {
  Wire input = VarintField(501, 1);
  EXPECT_THAT(
      Testee("UnknownVarint").ParseBinary(message(), input).SerializeBinary(),
      Yields(Payload(input)));
}

INSTANTIATE_TEST_SUITE_P(All, UnknownFieldsTest,
                         ValuesIn(AllTestMessageTypes()), MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
