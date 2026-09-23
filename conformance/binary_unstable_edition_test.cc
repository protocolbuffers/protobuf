// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Binary conformance tests for TestAllTypesEditionUnstable, the test message
// declared under the unstable edition.  This holds the binary-output leg of
// the legacy suite-level BinaryAndJsonConformanceSuite::RunUnstableTests();
// the test names and the requests sent to the testee are identical to the
// legacy ones.
//
// Like the legacy suite, these run whenever --maximum_edition is 2023 or
// newer (see ConformanceTest::IsSupported()), not only when it names the
// unstable edition itself.
//
// TODO: b/410122158 - The binary->JSON legs of these tests join the JSON suite
// once JSON matching is migrated.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "conformance/binary_wireformat.h"
#include "conformance/matchers.h"
#include "conformance/test_environment.h"
#include "conformance/test_protos/test_messages_edition_unstable.pb.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::protobuf_test_messages::edition_unstable::TestAllTypesEditionUnstable;

// Reporting TestAllTypesEditionUnstable through MessageUnderTest() makes the
// base SetUp() skip these tests when --maximum_edition doesn't cover it.
class UnstableEditionTest : public ConformanceTest {
 protected:
  const Descriptor* MessageUnderTest() const override { return message(); }

  static const Descriptor* message() {
    return TestAllTypesEditionUnstable::descriptor();
  }
};

// Field 13 is optional_bytes.
TEST_F(UnstableEditionTest, ValidBytes) {
  EXPECT_THAT(
      Testee("ValidBytes")
          .ParseBinary(message(), LengthPrefixedField(13, "foo"))
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(R"pb(optional_bytes: "foo")pb"))));
}

// Field 15 is map_string_bytes; its entries have key = 1 and value = 2.
TEST_F(UnstableEditionTest, ValidMapBytes) {
  EXPECT_THAT(Testee("ValidMap.Bytes")
                  .ParseBinary(message(),
                               LengthPrefixedField(
                                   15, Wire(LengthPrefixedField(1, "foo"),
                                            LengthPrefixedField(2, "barbaz"))))
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(
                  R"pb(map_string_bytes { key: "foo" value: "barbaz" })pb"))));
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
