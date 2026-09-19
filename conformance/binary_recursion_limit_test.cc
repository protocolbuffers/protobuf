// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Binary performance conformance tests checking that absurdly deeply nested
// messages are rejected rather than parsed.  Part of the performance suite
// (see conformance.bzl).  This replaces the legacy suite-level
// BinaryAndJsonConformanceSuite::RunRecursionLimitTests(); the test names and
// the requests sent to the testee are identical to the legacy ones.
//
// Unlike the legacy suite, which ran the TestAllTypesEdition2023 tests here
// regardless of --maximum_edition, these are edition-gated like every other
// editions test (through MessageUnderTest(), see message_type_fixtures.h).

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "conformance/matchers.h"
#include "conformance/message_type_fixtures.h"
#include "conformance/recursion_limit_payloads.h"
#include "conformance/test_environment.h"
#include "conformance/test_protos/test_messages_edition2023.pb.h"
#include "conformance/testee.h"
#include "google/protobuf/test_messages_proto2.pb.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::protobuf_test_messages::editions::TestAllTypesEdition2023;
using ::protobuf_test_messages::proto2::TestAllTypesProto2;

// The default recursion limit is 100 for most languages. 10,000 for golang. We
// use a larger number here for test.
constexpr int kMapDepth = 20000;
constexpr int kMapStringKeyDepth = 10000;
constexpr int kMessageSetDepth = 20000;

// The payloads are built directly on the wire, see recursion_limit_payloads.h.

// The TestAllTypesProto2 test, which every --maximum_edition covers.
class RecursionLimitPerformanceTest : public ConformanceTest {
 public:
  TestPriority DefaultPriority() const override { return kP3; }
};

// The TestAllTypesEdition2023 tests, which the base SetUp() skips when
// --maximum_edition doesn't cover editions.
class Edition2023RecursionLimitPerformanceTest
    : public Edition2023ConformanceTest {
 public:
  TestPriority DefaultPriority() const override { return kP3; }
};

TEST_F(Edition2023RecursionLimitPerformanceTest, Map) {
  EXPECT_THAT(Testee("EnforceDepthLimit.Map")
                  .ParseBinary(TestAllTypesEdition2023::descriptor(),
                               DeepMapPayload(kMapDepth))
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_F(Edition2023RecursionLimitPerformanceTest, MapStringKey) {
  EXPECT_THAT(Testee("EnforceDepthLimit.MapStringKey")
                  .ParseBinary(TestAllTypesEdition2023::descriptor(),
                               DeepMapStringKeyPayload(kMapStringKeyDepth))
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_F(RecursionLimitPerformanceTest, MessageSetExtension) {
  EXPECT_THAT(Testee("EnforceDepthLimit.MessageSetExtension")
                  .ParseBinary(TestAllTypesProto2::descriptor(),
                               DeepMessageSetPayload(kMessageSetDepth))
                  .ParseOnly(),
              Yields(IsParseError()));
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
