// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Binary conformance tests for delimited (group-encoded) message fields under
// editions.  So far this holds the extension cases of the legacy suite-level
// BinaryAndJsonConformanceSuite::RunDelimitedFieldTests(); the test names and
// the requests sent to the testee are identical to the legacy ones.
//
// TODO: b/410122237 - The remaining RunDelimitedFieldTests() cases (which also
// check binary->JSON) move here once JSON support is migrated.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "conformance/binary_wireformat.h"
#include "conformance/matchers.h"
#include "conformance/message_type_fixtures.h"
#include "conformance/test_environment.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

// Only TestAllTypesEdition2023 declares delimited extensions: field 121 is
// the groupliketype extension (whose name matches its type, as a proto2 group
// would) and field 122 is delimited_ext (whose name doesn't).  Both are
// GroupLikeType, whose field 1 is c.
using DelimitedExtensionTest = Edition2023ConformanceTest;

TEST_F(DelimitedExtensionTest, GroupLike) {
  EXPECT_THAT(
      Testee("ValidDelimitedExtension.GroupLike")
          .ParseBinary(message(), DelimitedField(121, VarintField(1, 99)))
          .SerializeBinary(),
      Yields(ParsedPayload(
          EqualsTextProto(R"pb([protobuf_test_messages.editions.groupliketype] {
                                 c: 99
                               })pb"))));
}

TEST_F(DelimitedExtensionTest, NotGroupLike) {
  EXPECT_THAT(
      Testee("ValidDelimitedExtension.NotGroupLike")
          .ParseBinary(message(), DelimitedField(122, VarintField(1, 99)))
          .SerializeBinary(),
      Yields(ParsedPayload(
          EqualsTextProto(R"pb([protobuf_test_messages.editions.delimited_ext] {
                                 c: 99
                               })pb"))));
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
