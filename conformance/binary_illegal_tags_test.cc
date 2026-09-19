// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Binary conformance tests checking that illegal field tags are rejected: a
// field number of zero, and tag varints whose field number is out of range or
// that are overlong.  This replaces the legacy
// BinaryAndJsonConformanceSuiteImpl<M>::TestIllegalTags(); the test names and
// the requests sent to the testee are identical to the legacy ones.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "binary_test_util.h"
#include "binary_wireformat.h"
#include "matchers.h"
#include "message_type_fixtures.h"
#include "test_environment.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::ValuesIn;

// The one-byte tag of field number 1 with wire type varint (0x08) with its
// continuation bit (0x80) set, i.e. the first byte of a longer tag varint.  The
// BadTag tests append further bytes to it.
constexpr absl::string_view kTagWithContinuationBit = "\x88";

// The last byte of an overlong varint: neither continuation bit nor value
// bits.  Spelled with an explicit length since it is a NUL.
constexpr absl::string_view kVarintTerminator("\0", 1);

using IllegalTagsTest = MessageTypeConformanceTest;

// Field number 0 is illegal, whatever the wire type.  (The leading "\1" etc.
// are octal escapes: the tag bytes 0x01, 0x02, 0x03 and 0x05.)
TEST_P(IllegalTagsTest, IllegalZeroFieldNumCase0) {
  EXPECT_THAT(RequiredTest("IllegalZeroFieldNum_Case_0")
                  .ParseBinary(message(), Wire("\1DEADBEEF"))
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(IllegalTagsTest, IllegalZeroFieldNumCase1) {
  EXPECT_THAT(RequiredTest("IllegalZeroFieldNum_Case_1")
                  .ParseBinary(message(), Wire("\2\1\1"))
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(IllegalTagsTest, IllegalZeroFieldNumCase2) {
  EXPECT_THAT(RequiredTest("IllegalZeroFieldNum_Case_2")
                  .ParseBinary(message(), Wire("\3\4"))
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_P(IllegalTagsTest, IllegalZeroFieldNumCase3) {
  EXPECT_THAT(RequiredTest("IllegalZeroFieldNum_Case_3")
                  .ParseBinary(message(), Wire("\5DEAD"))
                  .ParseOnly(),
              Yields(IsParseError()));
}

// A tag varint whose field number is far out of range of the maximum legal
// field number.  The lower 5 bytes of the varint do look like a well-formed
// tag for field number 1.
TEST_P(IllegalTagsTest, BadTagFieldNumberTooHigh) {
  EXPECT_THAT(RequiredTest("BadTag_FieldNumberTooHigh")
                  .ParseBinary(message(),
                               Wire(kTagWithContinuationBit,
                                    "\x80\x80\x80\x80\x80\x0F", Varint(1234)))
                  .ParseOnly(),
              Yields(IsParseError()));
}

// A 5-byte tag varint whose value is above UINT32_MAX (bit 35 is set).
TEST_P(IllegalTagsTest, BadTagFieldNumberSlightlyTooHigh) {
  EXPECT_THAT(
      RequiredTest("BadTag_FieldNumberSlightlyTooHigh")
          .ParseBinary(message(), Wire(kTagWithContinuationBit,
                                       "\x80\x80\x80\x40", Varint(1234)))
          .ParseOnly(),
      Yields(IsParseError()));
}

// A tag varint that is more than 5 bytes only because it is overlong (seven
// empty continuation bytes), so the decoded value is still below UINT32_MAX.
TEST_P(IllegalTagsTest, BadTagOverlongVarint) {
  EXPECT_THAT(RequiredTest("BadTag_OverlongVarint")
                  .ParseBinary(message(), Wire(kTagWithContinuationBit,
                                               "\x80\x80\x80\x80\x80\x80\x80",
                                               kVarintTerminator, Varint(1234)))
                  .ParseOnly(),
              Yields(IsParseError()));
}

// An overlong tag varint (eleven empty continuation bytes) that is even more
// than 10 bytes long.
TEST_P(IllegalTagsTest, BadTagVarintMoreThanTenBytes) {
  EXPECT_THAT(
      RequiredTest("BadTag_VarintMoreThanTenBytes")
          .ParseBinary(message(),
                       Wire(kTagWithContinuationBit,
                            "\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80",
                            kVarintTerminator, Varint(1234)))
          .ParseOnly(),
      Yields(IsParseError()));
}

INSTANTIATE_TEST_SUITE_P(All, IllegalTagsTest, ValuesIn(AllTestMessageTypes()),
                         MessageTypeParamName);

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
