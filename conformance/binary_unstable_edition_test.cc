// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Binary conformance tests for TestAllTypesEditionUnstable, the test message
// declared under the unstable edition.  This holds both the binary-output and
// the JSON-output leg of the legacy suite-level
// BinaryAndJsonConformanceSuite::RunUnstableTests(); the requests sent to the
// testee are identical to the legacy ones.
//
// Like the legacy suite, these run whenever --maximum_edition is 2023 or
// newer (see ConformanceTest::IsSupported()), not only when it names the
// unstable edition itself.

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
      Testee()
          .ParseBinary(message(), LengthPrefixedField(13, "foo"))
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto(R"pb(optional_bytes: "foo")pb"))));
}

TEST_F(UnstableEditionTest, ValidBytesJson) {
  EXPECT_THAT(
      Testee()
          .ParseBinary(message(), LengthPrefixedField(13, "foo"))
          .SerializeJson(),
      Yields(ParsedPayload(EqualsTextProto(R"pb(optional_bytes: "foo")pb"))));
}

// Field 15 is map_string_bytes; its entries have key = 1 and value = 2.
TEST_F(UnstableEditionTest, ValidMapBytes) {
  EXPECT_THAT(Testee()
                  .ParseBinary(message(),
                               LengthPrefixedField(
                                   15, Wire(LengthPrefixedField(1, "foo"),
                                            LengthPrefixedField(2, "barbaz"))))
                  .SerializeBinary(),
              Yields(ParsedPayload(EqualsTextProto(
                  R"pb(map_string_bytes { key: "foo" value: "barbaz" })pb"))));
}

TEST_F(UnstableEditionTest, ValidMapBytesJson) {
  EXPECT_THAT(Testee()
                  .ParseBinary(message(),
                               LengthPrefixedField(
                                   15, Wire(LengthPrefixedField(1, "foo"),
                                            LengthPrefixedField(2, "barbaz"))))
                  .SerializeJson(),
              Yields(ParsedPayload(EqualsTextProto(
                  R"pb(map_string_bytes { key: "foo" value: "barbaz" })pb"))));
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
