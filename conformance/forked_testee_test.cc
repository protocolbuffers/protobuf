// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include "test_runner.h"
#include "testee_runner.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

TEST(ForkedTesteeTest, RunsTheTesteeBinaryWithItsArgs) {
  // `cat` echoes the length-prefixed request straight back, so a round trip
  // shows that --testee_binary and --testee_args reached a child process.
  const std::vector<std::string> args = {"-c", "exec cat"};
  std::unique_ptr<ConformanceTestRunner> runner =
      MakeTesteeRunner("/bin/sh", args);
  ASSERT_NE(runner, nullptr);
  EXPECT_EQ(runner->RunTest("SomeTest", "request bytes"), "request bytes");
}

TEST(ForkedTesteeTest, DoesNotSpawnTheTesteeBeforeTheFirstRequest) {
  // The binary need not even exist until a test runs (--gtest_list_tests and
  // an all-skipped run never do).
  std::unique_ptr<ConformanceTestRunner> runner =
      MakeTesteeRunner("/no/such/testee", {});
  EXPECT_NE(runner, nullptr);
}

TEST(ForkedTesteeDeathTest, RequiresATesteeBinary) {
  // The message is what test_environment_integration_test.sh looks for.
  EXPECT_DEATH(MakeTesteeRunner("", {}), "--testee_binary is required");
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
