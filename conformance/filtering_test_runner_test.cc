// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/filtering_test_runner.h"

#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "conformance/conformance.pb.h"
#include "conformance/test_runner.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Pair;

// Fake delegate that records every call and returns a canned response derived
// from the arguments, so pass-through can be verified.
class FakeTestRunner : public ConformanceTestRunner {
 public:
  std::string RunTest(absl::string_view test_name,
                      absl::string_view input) override {
    calls_.emplace_back(std::string(test_name), std::string(input));
    return absl::StrCat("response:", test_name, ":", input);
  }

  const std::vector<std::pair<std::string, std::string>>& calls() const {
    return calls_;
  }

 private:
  std::vector<std::pair<std::string, std::string>> calls_;
};

TEST(FilteringTestRunnerTest, ForwardsSelectedTests) {
  FakeTestRunner fake;
  FilteringTestRunner runner(&fake, {"Test.One", "Test.Two"});

  EXPECT_EQ(runner.RunTest("Test.One", "abc"), "response:Test.One:abc");
  EXPECT_EQ(runner.RunTest("Test.Two", ""), "response:Test.Two:");

  EXPECT_THAT(fake.calls(),
              ElementsAre(Pair("Test.One", "abc"), Pair("Test.Two", "")));
}

TEST(FilteringTestRunnerTest, SkipsUnselectedTestsWithoutDelegating) {
  FakeTestRunner fake;
  FilteringTestRunner runner(&fake, {"Test.One"});

  ::conformance::ConformanceResponse response;
  ASSERT_TRUE(response.ParseFromString(runner.RunTest("Test.Other", "abc")));
  EXPECT_EQ(response.result_case(),
            ::conformance::ConformanceResponse::kSkipped);
  // The shared constant is what Yields() (matchers.h) recognizes.
  EXPECT_EQ(response.skipped(), kTestNotSelectedSkipReason);
  EXPECT_EQ(response.skipped(), "Not selected by --test.");

  EXPECT_THAT(fake.calls(), IsEmpty());
}

TEST(FilteringTestRunnerTest, EmptySelectionSkipsEverything) {
  FakeTestRunner fake;
  FilteringTestRunner runner(&fake, {});

  ::conformance::ConformanceResponse response;
  ASSERT_TRUE(response.ParseFromString(runner.RunTest("Test.One", "abc")));
  EXPECT_EQ(response.result_case(),
            ::conformance::ConformanceResponse::kSkipped);

  EXPECT_THAT(fake.calls(), IsEmpty());
  EXPECT_THAT(runner.NamesRun(), IsEmpty());
}

TEST(FilteringTestRunnerTest, TracksNamesRunInSortedOrder) {
  FakeTestRunner fake;
  FilteringTestRunner runner(&fake, {"Test.C", "Test.A", "Test.B"});

  EXPECT_THAT(runner.NamesRun(), IsEmpty());

  runner.RunTest("Test.C", "");
  runner.RunTest("Test.Other", "");
  runner.RunTest("Test.B", "");
  // Running a test twice is fine and counts once.
  runner.RunTest("Test.C", "");

  EXPECT_THAT(runner.NamesRun(), ElementsAre("Test.B", "Test.C"));
  EXPECT_THAT(fake.calls(), ElementsAre(Pair("Test.C", ""), Pair("Test.B", ""),
                                        Pair("Test.C", "")));
}

TEST(FilteringTestRunnerTest, MatchesExactNamesOnly) {
  FakeTestRunner fake;
  FilteringTestRunner runner(&fake, {"Test.One"});

  runner.RunTest("Test.On", "");
  runner.RunTest("Test.One.More", "");
  runner.RunTest("test.one", "");

  EXPECT_THAT(fake.calls(), IsEmpty());
  EXPECT_THAT(runner.NamesRun(), IsEmpty());
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
