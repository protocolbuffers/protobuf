// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/filtering_test_runner.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "conformance/conformance.pb.h"
#include "conformance/mock_test_runner.h"
#include "conformance/test_runner.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::ElementsAre;
using ::testing::InSequence;
using ::testing::IsEmpty;
using ::testing::Return;
using ::testing::StrictMock;

// The delegate is a strict mock throughout: a call the filter should not
// have forwarded fails the test.
TEST(FilteringTestRunnerTest, ForwardsSelectedTests) {
  StrictMock<MockTestRunner> delegate;
  FilteringTestRunner runner(&delegate, {"Test.One", "Test.Two"});

  InSequence in_sequence;
  EXPECT_CALL(delegate, RunTest("Test.One", "abc")).WillOnce(Return("one"));
  EXPECT_CALL(delegate, RunTest("Test.Two", "")).WillOnce(Return("two"));
  EXPECT_EQ(runner.RunTest("Test.One", "abc"), "one");
  EXPECT_EQ(runner.RunTest("Test.Two", ""), "two");
}

TEST(FilteringTestRunnerTest, SkipsUnselectedTestsWithoutDelegating) {
  StrictMock<MockTestRunner> delegate;
  FilteringTestRunner runner(&delegate, {"Test.One"});

  ::conformance::ConformanceResponse response;
  ASSERT_TRUE(response.ParseFromString(runner.RunTest("Test.Other", "abc")));
  EXPECT_EQ(response.result_case(),
            ::conformance::ConformanceResponse::kSkipped);
  // The shared constant is what Yields() (matchers.h) recognizes.
  EXPECT_EQ(response.skipped(), kTestNotSelectedSkipReason);
  EXPECT_EQ(response.skipped(), "Not selected by --test.");
}

TEST(FilteringTestRunnerTest, EmptySelectionSkipsEverything) {
  StrictMock<MockTestRunner> delegate;
  FilteringTestRunner runner(&delegate, {});

  ::conformance::ConformanceResponse response;
  ASSERT_TRUE(response.ParseFromString(runner.RunTest("Test.One", "abc")));
  EXPECT_EQ(response.result_case(),
            ::conformance::ConformanceResponse::kSkipped);
  EXPECT_THAT(runner.NamesRun(), IsEmpty());
}

TEST(FilteringTestRunnerTest, TracksNamesRunInSortedOrder) {
  StrictMock<MockTestRunner> delegate;
  FilteringTestRunner runner(&delegate, {"Test.C", "Test.A", "Test.B"});

  EXPECT_THAT(runner.NamesRun(), IsEmpty());

  InSequence in_sequence;
  EXPECT_CALL(delegate, RunTest("Test.C", ""));
  EXPECT_CALL(delegate, RunTest("Test.B", ""));
  EXPECT_CALL(delegate, RunTest("Test.C", ""));
  runner.RunTest("Test.C", "");
  runner.RunTest("Test.Other", "");
  runner.RunTest("Test.B", "");
  // Running a test twice is fine and counts once.
  runner.RunTest("Test.C", "");

  EXPECT_THAT(runner.NamesRun(), ElementsAre("Test.B", "Test.C"));
}

TEST(FilteringTestRunnerTest, MatchesExactNamesOnly) {
  StrictMock<MockTestRunner> delegate;
  FilteringTestRunner runner(&delegate, {"Test.One"});

  runner.RunTest("Test.On", "");
  runner.RunTest("Test.One.More", "");
  runner.RunTest("test.one", "");

  EXPECT_THAT(runner.NamesRun(), IsEmpty());
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
