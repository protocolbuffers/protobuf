// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Binary performance conformance tests checking that absurdly deeply nested
// messages are rejected rather than parsed.  They only run with --performance
// (see PerformanceConformanceTest).  This replaces the legacy suite-level
// BinaryAndJsonConformanceSuite::RunRecursionLimitTests(); the test names and
// the requests sent to the testee are identical to the legacy ones.
//
// Unlike the legacy suite, which ran the TestAllTypesEdition2023 tests here
// regardless of --maximum_edition, these are edition-gated like every other
// editions test (through MessageUnderTest(), see message_type_fixtures.h).

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "binary_wireformat.h"
#include "matchers.h"
#include "message_type_fixtures.h"
#include "test_environment.h"
#include "conformance/test_protos/test_messages_edition2023.pb.h"
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

// The TestAllTypesProto2 test, which every --maximum_edition covers.
using RecursionLimitTest = PerformanceConformanceTest;

// The TestAllTypesEdition2023 tests: performance tests that the base SetUp()
// also skips when --maximum_edition doesn't cover editions.
class Edition2023RecursionLimitTest : public Edition2023ConformanceTest {
 protected:
  bool IsPerformanceTest() const override { return true; }
};

TEST_F(Edition2023RecursionLimitTest, Map) {
  TestAllTypesEdition2023 message;
  TestAllTypesEdition2023* sub = &message;
  for (int i = 0; i < kMapDepth; i++) {
    sub = &(*sub->mutable_map_recursive())[0];
    sub->set_optional_int32(123);
  }
  EXPECT_THAT(RecommendedTest("EnforceDepthLimit.Map")
                  .ParseBinary(TestAllTypesEdition2023::descriptor(),
                               Wire(message.SerializeAsString()))
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_F(Edition2023RecursionLimitTest, MapStringKey) {
  TestAllTypesEdition2023 message;
  TestAllTypesEdition2023* sub = &message;
  for (int i = 0; i < kMapStringKeyDepth; i++) {
    sub = (*sub->mutable_map_string_nested_message())[""].mutable_corecursive();
    sub->set_optional_int32(123);
  }
  EXPECT_THAT(RecommendedTest("EnforceDepthLimit.MapStringKey")
                  .ParseBinary(TestAllTypesEdition2023::descriptor(),
                               Wire(message.SerializeAsString()))
                  .ParseOnly(),
              Yields(IsParseError()));
}

TEST_F(RecursionLimitTest, MessageSetExtension) {
  TestAllTypesProto2 message;
  TestAllTypesProto2::MessageSetCorrect* sub =
      message.mutable_message_set_correct();
  for (int i = 0; i < kMessageSetDepth; i++) {
    sub =
        sub->MutableExtension(TestAllTypesProto2::MessageSetCorrectExtension2::
                                  message_set_extension)
            ->mutable_sub_msg();
  }
  sub->MutableExtension(
         TestAllTypesProto2::MessageSetCorrectExtension2::message_set_extension)
      ->set_i(123);
  EXPECT_THAT(RecommendedTest("EnforceDepthLimit.MessageSetExtension")
                  .ParseBinary(TestAllTypesProto2::descriptor(),
                               Wire(message.SerializeAsString()))
                  .ParseOnly(),
              Yields(IsParseError()));
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
