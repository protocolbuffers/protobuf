// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// A tiny conformance suite used to exercise the gtest conformance framework end
// to end against the real C++ testee.  Some of these tests fail on purpose; see
// the sample_failure_list_*.txt files and test_environment_integration_test.cc.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "conformance/binary_wireformat.h"
#include "conformance/matchers.h"
#include "conformance/test_fixture.h"
#include "conformance/testee.h"
#include "google/protobuf/test_messages_proto3.pb.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::protobuf_test_messages::proto3::TestAllTypesProto3;

using SampleConformanceTest = ConformanceTest;

TEST_F(SampleConformanceTest, SamplePass) {
  EXPECT_THAT(
      Testee()
          .ParseBinary(TestAllTypesProto3::descriptor(), VarintField(1, 99))
          .SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 99)pb"))));
}

// Valid input is expected to parse, so this always fails against a conforming
// testee.
TEST_F(SampleConformanceTest, SampleDeliberateFailure) {
  EXPECT_THAT(
      Testee()
          .ParseBinary(TestAllTypesProto3::descriptor(), VarintField(1, 99))
          .SerializeBinary(),
      Yields(IsParseError()));
}

// Same as above, but only P1.
TEST_F(SampleConformanceTest, SampleP1Failure) {
  EXPECT_THAT(
      Testee(kP1)
          .ParseBinary(TestAllTypesProto3::descriptor(), VarintField(1, 99))
          .SerializeBinary(),
      Yields(IsParseError()));
}

// Only runs with gtest's also_run_disabled_tests flag.  Its presence must not
// make a normal run count as partial (see IsPartialRun() in
// test_environment.cc).
TEST_F(SampleConformanceTest, DISABLED_SampleDisabled) {
  EXPECT_THAT(
      Testee()
          .ParseBinary(TestAllTypesProto3::descriptor(), VarintField(1, 99))
          .SerializeBinary(),
      Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 99)pb"))));
}

// Runs a conformance test but never checks its result with Yields(), which
// fails the test.  Only runs when asked for by name (see
// test_environment_integration_test.cc), so that the other runs stay clean.
TEST_F(SampleConformanceTest, DISABLED_SampleUncheckedResult) {
  auto unchecked =
      Testee("SampleUncheckedResult")
          .ParseBinary(TestAllTypesProto3::descriptor(), VarintField(1, 99))
          .SerializeBinary();
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
