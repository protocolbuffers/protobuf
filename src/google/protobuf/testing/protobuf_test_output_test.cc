// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "google/protobuf/testing/protobuf_test.h"

namespace {

PB_TEST(StandaloneSuite, NoParams) {
  ADD_FAILURE() << "NoParams failed as expected";
}

PB_TEST(StandaloneSuite, ValueCartesianProduct) {
  int x = PB_TEST_CHOOSE_VALUE((10, 20), "x");
  bool flag = PB_TEST_CHOOSE_VALUE(testing::Bool(), "flag");
  if (x == 10 || flag) {
    ADD_FAILURE() << "x=" << x << " flag=" << flag;
  }
}

PB_TEST(StandaloneSuite, TypeConstexprAndValue) {
  using T = PB_TEST_CHOOSE_TYPE((int32_t, int64_t));
  constexpr int kMult = PB_TEST_CHOOSE_CONSTEXPR((2, 3), "mult");
  int v = PB_TEST_CHOOSE_VALUE((1, 5), "v");
  T result = static_cast<T>(v) * kMult;
  if (result > 4) {
    ADD_FAILURE() << "sizeof(T)=" << sizeof(T) << " result=" << result;
  }
}

PB_TEST(StandaloneSuite, ConditionalCompileTime) {
  using T = PB_TEST_CHOOSE_TYPE((int32_t, double));
  if constexpr (std::is_same_v<T, int32_t>) {
    constexpr int kShift = PB_TEST_CHOOSE_CONSTEXPR((1, 2), "shift");
    ADD_FAILURE() << "int32 shift=" << kShift;
  } else {
    ADD_FAILURE() << "double branch";
  }
}

PB_TEST(StandaloneSuite, UnusedRuntimeConditional) {
  int a = PB_TEST_CHOOSE_VALUE((1, 2), "a");
  if (a == 1) {
    int b = PB_TEST_CHOOSE_VALUE((10, 20), "b");
    (void)b;
  }
}

class OutputFixtureTest : public ::testing::Test {
 protected:
  void SetUp() override {
    setup_val_ = PB_TEST_CHOOSE_VALUE((100, 200), "setup");
  }

  bool ctor_flag_ = PB_TEST_CHOOSE_VALUE(testing::Bool(), "Arena");
  int setup_val_ = 0;
};

PB_TEST_F(OutputFixtureTest, FixtureOnlyParams) {
  if (!ctor_flag_) {
    ADD_FAILURE() << "FixtureOnlyParams setup_val=" << setup_val_;
  }
}

PB_TEST_F(OutputFixtureTest, FixtureAndBodyParams) {
  int body_val = PB_TEST_CHOOSE_VALUE((1, 2), "body");
  if (ctor_flag_ != (body_val == 2)) {
    ADD_FAILURE() << "ctor_flag=" << ctor_flag_ << " setup_val=" << setup_val_
                  << " body_val=" << body_val;
  }
}

class UnusedHelperFixtureTest : public ::testing::Test {
 protected:
  int ChooseHelper() { return PB_TEST_CHOOSE_VALUE((1, 2), "helper"); }
};

PB_TEST_F(UnusedHelperFixtureTest, DoesNotCallHelper) {}

PB_TEST_F(UnusedHelperFixtureTest, AssertFailsBeforeHelper) {
  ASSERT_TRUE(false) << "assert failed before helper";
}

PB_TEST_F(UnusedHelperFixtureTest, SkipsBeforeHelper) {
  GTEST_SKIP() << "skipped before helper";
}

void UnsetEnvVar(const char* name) {
#ifdef _WIN32
  _putenv_s(name, "");
#else
  unsetenv(name);
#endif
}

FILE* OpenPipe(const char* argv0) {
#ifdef _WIN32
  std::string win_cmd = absl::StrCat(
      "\"", absl::StrReplaceAll(argv0, {{"/", "\\"}}), "\" --run_tests");
  return _popen(win_cmd.c_str(), "r");
#else
  std::string cmd = absl::StrCat("\"", argv0, "\" --run_tests");
  return popen(cmd.c_str(), "r");
#endif
}

int ClosePipe(FILE* pipe) {
#ifdef _WIN32
  return _pclose(pipe);
#else
  return pclose(pipe);
#endif
}

std::string NormalizeOutput(absl::string_view raw_output) {
  std::vector<std::string> normalized_lines;
  bool started = false;
  for (absl::string_view line : absl::StrSplit(raw_output, '\n')) {
    line = absl::StripSuffix(line, "\r");
    if (!started) {
      if (absl::StartsWith(line, "[==========]")) {
        started = true;
      } else {
        continue;
      }
    }
    constexpr absl::string_view kTestFile = "protobuf_test_output_test.cc";
    if (size_t pos = line.find(kTestFile); pos != absl::string_view::npos) {
      absl::string_view rest = line.substr(pos + kTestFile.size());
      if (absl::ConsumePrefix(&rest, ":")) {
        if (absl::ConsumeSuffix(&rest, ": Failure")) {
          normalized_lines.push_back(absl::StrCat("FILE:", rest, ": Failure"));
          continue;
        }
        if (absl::ConsumeSuffix(&rest, ": Skipped")) {
          normalized_lines.push_back(absl::StrCat("FILE:", rest, ": Skipped"));
          continue;
        }
      }
      if (absl::ConsumePrefix(&rest, "(")) {
        constexpr absl::string_view kMsvcError = "): error: ";
        if (size_t close = rest.find(kMsvcError);
            close != absl::string_view::npos) {
          normalized_lines.push_back(
              absl::StrCat("FILE:", rest.substr(0, close), ": Failure"));
          absl::string_view msg = rest.substr(close + kMsvcError.size());
          if (!msg.empty()) {
            normalized_lines.emplace_back(msg);
          }
          continue;
        }
        if (size_t close = rest.find("): Skipped");
            close != absl::string_view::npos) {
          normalized_lines.push_back(
              absl::StrCat("FILE:", rest.substr(0, close), ": Skipped"));
          continue;
        }
      }
    }
    normalized_lines.emplace_back(line);
  }
  return absl::StrJoin(normalized_lines, "\n");
}

constexpr absl::string_view kExpectedOutput =
    R"([==========] Running 38 tests from 3 test suites.
[----------] Global test environment set-up.
[----------] 20 tests from StandaloneSuite
[ RUN      ] StandaloneSuite.NoParams
FILE:30: Failure
Failed
NoParams failed as expected

[  FAILED  ] StandaloneSuite.NoParams
[ RUN      ] StandaloneSuite.ValueCartesianProduct/x_10/no_flag
FILE:37: Failure
Failed
x=10 flag=false

[  FAILED  ] StandaloneSuite.ValueCartesianProduct/x_10/no_flag, where GetParam() = x_10/no_flag
[ RUN      ] StandaloneSuite.ValueCartesianProduct/x_20/no_flag
[       OK ] StandaloneSuite.ValueCartesianProduct/x_20/no_flag
[ RUN      ] StandaloneSuite.ValueCartesianProduct/x_10/flag
FILE:37: Failure
Failed
x=10 flag=true

[  FAILED  ] StandaloneSuite.ValueCartesianProduct/x_10/flag, where GetParam() = x_10/flag
[ RUN      ] StandaloneSuite.ValueCartesianProduct/x_20/flag
FILE:37: Failure
Failed
x=20 flag=true

[  FAILED  ] StandaloneSuite.ValueCartesianProduct/x_20/flag, where GetParam() = x_20/flag
[ RUN      ] StandaloneSuite.TypeConstexprAndValue/int32_t/mult_2/v_1
[       OK ] StandaloneSuite.TypeConstexprAndValue/int32_t/mult_2/v_1
[ RUN      ] StandaloneSuite.TypeConstexprAndValue/int32_t/mult_2/v_5
FILE:47: Failure
Failed
sizeof(T)=4 result=10

[  FAILED  ] StandaloneSuite.TypeConstexprAndValue/int32_t/mult_2/v_5, where TypeParam = int32_t and GetParam() = mult_2/v_5
[ RUN      ] StandaloneSuite.TypeConstexprAndValue/int32_t/mult_3/v_1
[       OK ] StandaloneSuite.TypeConstexprAndValue/int32_t/mult_3/v_1
[ RUN      ] StandaloneSuite.TypeConstexprAndValue/int32_t/mult_3/v_5
FILE:47: Failure
Failed
sizeof(T)=4 result=15

[  FAILED  ] StandaloneSuite.TypeConstexprAndValue/int32_t/mult_3/v_5, where TypeParam = int32_t and GetParam() = mult_3/v_5
[ RUN      ] StandaloneSuite.TypeConstexprAndValue/int64_t/mult_2/v_1
[       OK ] StandaloneSuite.TypeConstexprAndValue/int64_t/mult_2/v_1
[ RUN      ] StandaloneSuite.TypeConstexprAndValue/int64_t/mult_2/v_5
FILE:47: Failure
Failed
sizeof(T)=8 result=10

[  FAILED  ] StandaloneSuite.TypeConstexprAndValue/int64_t/mult_2/v_5, where TypeParam = int64_t and GetParam() = mult_2/v_5
[ RUN      ] StandaloneSuite.TypeConstexprAndValue/int64_t/mult_3/v_1
[       OK ] StandaloneSuite.TypeConstexprAndValue/int64_t/mult_3/v_1
[ RUN      ] StandaloneSuite.TypeConstexprAndValue/int64_t/mult_3/v_5
FILE:47: Failure
Failed
sizeof(T)=8 result=15

[  FAILED  ] StandaloneSuite.TypeConstexprAndValue/int64_t/mult_3/v_5, where TypeParam = int64_t and GetParam() = mult_3/v_5
[ RUN      ] StandaloneSuite.ConditionalCompileTime/int32_t/shift_1
FILE:55: Failure
Failed
int32 shift=1

[  FAILED  ] StandaloneSuite.ConditionalCompileTime/int32_t/shift_1, where TypeParam = int32_t and GetParam() = shift_1
[ RUN      ] StandaloneSuite.ConditionalCompileTime/int32_t/shift_2
FILE:55: Failure
Failed
int32 shift=2

[  FAILED  ] StandaloneSuite.ConditionalCompileTime/int32_t/shift_2, where TypeParam = int32_t and GetParam() = shift_2
[ RUN      ] StandaloneSuite.ConditionalCompileTime/double
FILE:57: Failure
Failed
double branch

[  FAILED  ] StandaloneSuite.ConditionalCompileTime/double, where TypeParam = double
[ RUN      ] StandaloneSuite.UnusedRuntimeConditional/a_1/b_10
[       OK ] StandaloneSuite.UnusedRuntimeConditional/a_1/b_10
[ RUN      ] StandaloneSuite.UnusedRuntimeConditional/a_2/b_10
FILE:64: Failure
Failed
PB_TEST_CHOOSE_VALUE(...) dimension was registered for this test but never evaluated during the test run.

[  FAILED  ] StandaloneSuite.UnusedRuntimeConditional/a_2/b_10, where GetParam() = a_2/b_10
[ RUN      ] StandaloneSuite.UnusedRuntimeConditional/a_1/b_20
[       OK ] StandaloneSuite.UnusedRuntimeConditional/a_1/b_20
[ RUN      ] StandaloneSuite.UnusedRuntimeConditional/a_2/b_20
FILE:64: Failure
Failed
PB_TEST_CHOOSE_VALUE(...) dimension was registered for this test but never evaluated during the test run.

[  FAILED  ] StandaloneSuite.UnusedRuntimeConditional/a_2/b_20, where GetParam() = a_2/b_20
[----------] 12 tests from OutputFixtureTest
[ RUN      ] OutputFixtureTest.FixtureOnlyParams/setup_100/NoArena
FILE:81: Failure
Failed
FixtureOnlyParams setup_val=100

[  FAILED  ] OutputFixtureTest.FixtureOnlyParams/setup_100/NoArena, where GetParam() = setup_100/NoArena
[ RUN      ] OutputFixtureTest.FixtureOnlyParams/setup_200/NoArena
FILE:81: Failure
Failed
FixtureOnlyParams setup_val=200

[  FAILED  ] OutputFixtureTest.FixtureOnlyParams/setup_200/NoArena, where GetParam() = setup_200/NoArena
[ RUN      ] OutputFixtureTest.FixtureOnlyParams/setup_100/Arena
[       OK ] OutputFixtureTest.FixtureOnlyParams/setup_100/Arena
[ RUN      ] OutputFixtureTest.FixtureOnlyParams/setup_200/Arena
[       OK ] OutputFixtureTest.FixtureOnlyParams/setup_200/Arena
[ RUN      ] OutputFixtureTest.FixtureAndBodyParams/setup_100/NoArena/body_1
[       OK ] OutputFixtureTest.FixtureAndBodyParams/setup_100/NoArena/body_1
[ RUN      ] OutputFixtureTest.FixtureAndBodyParams/setup_200/NoArena/body_1
[       OK ] OutputFixtureTest.FixtureAndBodyParams/setup_200/NoArena/body_1
[ RUN      ] OutputFixtureTest.FixtureAndBodyParams/setup_100/Arena/body_1
FILE:88: Failure
Failed
ctor_flag=true setup_val=100 body_val=1

[  FAILED  ] OutputFixtureTest.FixtureAndBodyParams/setup_100/Arena/body_1, where GetParam() = setup_100/Arena/body_1
[ RUN      ] OutputFixtureTest.FixtureAndBodyParams/setup_200/Arena/body_1
FILE:88: Failure
Failed
ctor_flag=true setup_val=200 body_val=1

[  FAILED  ] OutputFixtureTest.FixtureAndBodyParams/setup_200/Arena/body_1, where GetParam() = setup_200/Arena/body_1
[ RUN      ] OutputFixtureTest.FixtureAndBodyParams/setup_100/NoArena/body_2
FILE:88: Failure
Failed
ctor_flag=false setup_val=100 body_val=2

[  FAILED  ] OutputFixtureTest.FixtureAndBodyParams/setup_100/NoArena/body_2, where GetParam() = setup_100/NoArena/body_2
[ RUN      ] OutputFixtureTest.FixtureAndBodyParams/setup_200/NoArena/body_2
FILE:88: Failure
Failed
ctor_flag=false setup_val=200 body_val=2

[  FAILED  ] OutputFixtureTest.FixtureAndBodyParams/setup_200/NoArena/body_2, where GetParam() = setup_200/NoArena/body_2
[ RUN      ] OutputFixtureTest.FixtureAndBodyParams/setup_100/Arena/body_2
[       OK ] OutputFixtureTest.FixtureAndBodyParams/setup_100/Arena/body_2
[ RUN      ] OutputFixtureTest.FixtureAndBodyParams/setup_200/Arena/body_2
[       OK ] OutputFixtureTest.FixtureAndBodyParams/setup_200/Arena/body_2
[----------] 6 tests from UnusedHelperFixtureTest
[ RUN      ] UnusedHelperFixtureTest.DoesNotCallHelper/helper_1
FILE:95: Failure
Failed
PB_TEST_CHOOSE_VALUE(...) dimension was registered for this test but never evaluated during the test run.

[  FAILED  ] UnusedHelperFixtureTest.DoesNotCallHelper/helper_1, where GetParam() = helper_1
[ RUN      ] UnusedHelperFixtureTest.DoesNotCallHelper/helper_2
FILE:95: Failure
Failed
PB_TEST_CHOOSE_VALUE(...) dimension was registered for this test but never evaluated during the test run.

[  FAILED  ] UnusedHelperFixtureTest.DoesNotCallHelper/helper_2, where GetParam() = helper_2
[ RUN      ] UnusedHelperFixtureTest.AssertFailsBeforeHelper/helper_1
FILE:101: Failure
Value of: false
  Actual: false
Expected: true
assert failed before helper

[  FAILED  ] UnusedHelperFixtureTest.AssertFailsBeforeHelper/helper_1, where GetParam() = helper_1
[ RUN      ] UnusedHelperFixtureTest.AssertFailsBeforeHelper/helper_2
FILE:101: Failure
Value of: false
  Actual: false
Expected: true
assert failed before helper

[  FAILED  ] UnusedHelperFixtureTest.AssertFailsBeforeHelper/helper_2, where GetParam() = helper_2
[ RUN      ] UnusedHelperFixtureTest.SkipsBeforeHelper/helper_1
FILE:105: Skipped
skipped before helper

[  SKIPPED ] UnusedHelperFixtureTest.SkipsBeforeHelper/helper_1
[ RUN      ] UnusedHelperFixtureTest.SkipsBeforeHelper/helper_2
FILE:105: Skipped
skipped before helper

[  SKIPPED ] UnusedHelperFixtureTest.SkipsBeforeHelper/helper_2
[----------] Global test environment tear-down
[==========] 38 tests from 3 test suites ran.
[  PASSED  ] 13 tests.
[  SKIPPED ] 2 tests, listed below:
[  SKIPPED ] UnusedHelperFixtureTest.SkipsBeforeHelper/helper_1
[  SKIPPED ] UnusedHelperFixtureTest.SkipsBeforeHelper/helper_2
[  FAILED  ] 23 tests, listed below:
[  FAILED  ] StandaloneSuite.NoParams
[  FAILED  ] StandaloneSuite.ValueCartesianProduct/x_10/no_flag, where GetParam() = x_10/no_flag
[  FAILED  ] StandaloneSuite.ValueCartesianProduct/x_10/flag, where GetParam() = x_10/flag
[  FAILED  ] StandaloneSuite.ValueCartesianProduct/x_20/flag, where GetParam() = x_20/flag
[  FAILED  ] StandaloneSuite.TypeConstexprAndValue/int32_t/mult_2/v_5, where TypeParam = int32_t and GetParam() = mult_2/v_5
[  FAILED  ] StandaloneSuite.TypeConstexprAndValue/int32_t/mult_3/v_5, where TypeParam = int32_t and GetParam() = mult_3/v_5
[  FAILED  ] StandaloneSuite.TypeConstexprAndValue/int64_t/mult_2/v_5, where TypeParam = int64_t and GetParam() = mult_2/v_5
[  FAILED  ] StandaloneSuite.TypeConstexprAndValue/int64_t/mult_3/v_5, where TypeParam = int64_t and GetParam() = mult_3/v_5
[  FAILED  ] StandaloneSuite.ConditionalCompileTime/int32_t/shift_1, where TypeParam = int32_t and GetParam() = shift_1
[  FAILED  ] StandaloneSuite.ConditionalCompileTime/int32_t/shift_2, where TypeParam = int32_t and GetParam() = shift_2
[  FAILED  ] StandaloneSuite.ConditionalCompileTime/double, where TypeParam = double
[  FAILED  ] StandaloneSuite.UnusedRuntimeConditional/a_2/b_10, where GetParam() = a_2/b_10
[  FAILED  ] StandaloneSuite.UnusedRuntimeConditional/a_2/b_20, where GetParam() = a_2/b_20
[  FAILED  ] OutputFixtureTest.FixtureOnlyParams/setup_100/NoArena, where GetParam() = setup_100/NoArena
[  FAILED  ] OutputFixtureTest.FixtureOnlyParams/setup_200/NoArena, where GetParam() = setup_200/NoArena
[  FAILED  ] OutputFixtureTest.FixtureAndBodyParams/setup_100/Arena/body_1, where GetParam() = setup_100/Arena/body_1
[  FAILED  ] OutputFixtureTest.FixtureAndBodyParams/setup_200/Arena/body_1, where GetParam() = setup_200/Arena/body_1
[  FAILED  ] OutputFixtureTest.FixtureAndBodyParams/setup_100/NoArena/body_2, where GetParam() = setup_100/NoArena/body_2
[  FAILED  ] OutputFixtureTest.FixtureAndBodyParams/setup_200/NoArena/body_2, where GetParam() = setup_200/NoArena/body_2
[  FAILED  ] UnusedHelperFixtureTest.DoesNotCallHelper/helper_1, where GetParam() = helper_1
[  FAILED  ] UnusedHelperFixtureTest.DoesNotCallHelper/helper_2, where GetParam() = helper_2
[  FAILED  ] UnusedHelperFixtureTest.AssertFailsBeforeHelper/helper_1, where GetParam() = helper_1
[  FAILED  ] UnusedHelperFixtureTest.AssertFailsBeforeHelper/helper_2, where GetParam() = helper_2

23 FAILED TESTS
)";

}  // namespace

int main(int argc, char** argv) {
  if (argc > 1 && absl::string_view(argv[1]) == "--run_tests") {
    for (int i = 1; i < argc - 1; ++i) {
      argv[i] = argv[i + 1];
    }
    --argc;
    argv[argc] = nullptr;
    testing::InitGoogleTest(&argc, argv);
    GTEST_FLAG_SET(print_time, false);
    GTEST_FLAG_SET(stack_trace_depth, 0);
    GTEST_FLAG_SET(color, "no");
    GTEST_FLAG_SET(brief, false);
    return RUN_ALL_TESTS();
  }

  for (const char* name :
       {"XML_OUTPUT_FILE", "GTEST_OUTPUT", "GUNIT_OUTPUT",
        "TEST_PREMATURE_EXIT_FILE", "TEST_SHARD_INDEX", "TEST_TOTAL_SHARDS",
        "TEST_SHARD_STATUS_FILE", "GTEST_SHARD_INDEX", "GTEST_TOTAL_SHARDS",
        "GTEST_SHARD_STATUS_FILE", "GTEST_FILTER", "GUNIT_FILTER",
        "TESTBRIDGE_TEST_ONLY", "TESTBRIDGE_TEST_RUNNER_FAIL_FAST"}) {
    UnsetEnvVar(name);
  }

  FILE* pipe = OpenPipe(argv[0]);
  if (pipe == nullptr) {
    std::cerr << "Failed to open pipe to child command: " << argv[0] << "\n";
    return 1;
  }

  std::string raw_output;
  char buffer[4096];
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    raw_output.append(buffer);
  }
  int status = ClosePipe(pipe);
  if (status == 0) {
    std::cerr << "Expected child process to exit with non-zero status due to "
                 "test failures, but got 0.\nOutput:\n"
              << raw_output;
    return 1;
  }

  std::string normalized = NormalizeOutput(raw_output);
  if (normalized != kExpectedOutput) {
    std::cerr << "Normalized output does not match golden output.\n"
              << "=== ACTUAL ===\n"
              << normalized << "\n=== EXPECTED ===\n"
              << kExpectedOutput << "\n=== END ===\n";
    return 1;
  }

  return 0;
}
