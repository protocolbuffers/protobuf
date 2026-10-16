// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"

namespace {

void UnsetEnvVar(const char* name) {
#ifdef _WIN32
  _putenv_s(name, "");
#else
  unsetenv(name);
#endif
}

FILE* OpenPipe(const char* cmd) {
#ifdef _WIN32
  std::string win_cmd = absl::StrReplaceAll(cmd, {{"/", "\\"}});
  return _popen(win_cmd.c_str(), "r");
#else
  return popen(cmd, "r");
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
    if (absl::StrContains(line, "protobuf_test_output_test_helper.cc") &&
        (absl::EndsWith(line, ": Failure") ||
         absl::StrContains(line, "): error:"))) {
      normalized_lines.push_back("FILE:#: Failure");
    } else {
      normalized_lines.emplace_back(line);
    }
  }
  return absl::StrJoin(normalized_lines, "\n");
}

constexpr absl::string_view kExpectedOutput =
    R"([==========] Running 28 tests from 2 test suites.
[----------] Global test environment set-up.
[----------] 16 tests from StandaloneSuite
[ RUN      ] StandaloneSuite.NoParams
FILE:#: Failure
Failed
NoParams failed as expected

[  FAILED  ] StandaloneSuite.NoParams
[ RUN      ] StandaloneSuite.ValueCartesianProduct/x_10/no_flag
FILE:#: Failure
Failed
x=10 flag=false

[  FAILED  ] StandaloneSuite.ValueCartesianProduct/x_10/no_flag, where GetParam() = x_10/no_flag
[ RUN      ] StandaloneSuite.ValueCartesianProduct/x_20/no_flag
[       OK ] StandaloneSuite.ValueCartesianProduct/x_20/no_flag
[ RUN      ] StandaloneSuite.ValueCartesianProduct/x_10/flag
FILE:#: Failure
Failed
x=10 flag=true

[  FAILED  ] StandaloneSuite.ValueCartesianProduct/x_10/flag, where GetParam() = x_10/flag
[ RUN      ] StandaloneSuite.ValueCartesianProduct/x_20/flag
FILE:#: Failure
Failed
x=20 flag=true

[  FAILED  ] StandaloneSuite.ValueCartesianProduct/x_20/flag, where GetParam() = x_20/flag
[ RUN      ] StandaloneSuite.TypeConstexprAndValue/int32_t/mult_2/v_1
[       OK ] StandaloneSuite.TypeConstexprAndValue/int32_t/mult_2/v_1
[ RUN      ] StandaloneSuite.TypeConstexprAndValue/int32_t/mult_2/v_5
FILE:#: Failure
Failed
sizeof(T)=4 result=10

[  FAILED  ] StandaloneSuite.TypeConstexprAndValue/int32_t/mult_2/v_5, where TypeParam = int32_t and GetParam() = mult_2/v_5
[ RUN      ] StandaloneSuite.TypeConstexprAndValue/int32_t/mult_3/v_1
[       OK ] StandaloneSuite.TypeConstexprAndValue/int32_t/mult_3/v_1
[ RUN      ] StandaloneSuite.TypeConstexprAndValue/int32_t/mult_3/v_5
FILE:#: Failure
Failed
sizeof(T)=4 result=15

[  FAILED  ] StandaloneSuite.TypeConstexprAndValue/int32_t/mult_3/v_5, where TypeParam = int32_t and GetParam() = mult_3/v_5
[ RUN      ] StandaloneSuite.TypeConstexprAndValue/int64_t/mult_2/v_1
[       OK ] StandaloneSuite.TypeConstexprAndValue/int64_t/mult_2/v_1
[ RUN      ] StandaloneSuite.TypeConstexprAndValue/int64_t/mult_2/v_5
FILE:#: Failure
Failed
sizeof(T)=8 result=10

[  FAILED  ] StandaloneSuite.TypeConstexprAndValue/int64_t/mult_2/v_5, where TypeParam = int64_t and GetParam() = mult_2/v_5
[ RUN      ] StandaloneSuite.TypeConstexprAndValue/int64_t/mult_3/v_1
[       OK ] StandaloneSuite.TypeConstexprAndValue/int64_t/mult_3/v_1
[ RUN      ] StandaloneSuite.TypeConstexprAndValue/int64_t/mult_3/v_5
FILE:#: Failure
Failed
sizeof(T)=8 result=15

[  FAILED  ] StandaloneSuite.TypeConstexprAndValue/int64_t/mult_3/v_5, where TypeParam = int64_t and GetParam() = mult_3/v_5
[ RUN      ] StandaloneSuite.ConditionalCompileTime/int32_t/shift_1
FILE:#: Failure
Failed
int32 shift=1

[  FAILED  ] StandaloneSuite.ConditionalCompileTime/int32_t/shift_1, where TypeParam = int32_t and GetParam() = shift_1
[ RUN      ] StandaloneSuite.ConditionalCompileTime/int32_t/shift_2
FILE:#: Failure
Failed
int32 shift=2

[  FAILED  ] StandaloneSuite.ConditionalCompileTime/int32_t/shift_2, where TypeParam = int32_t and GetParam() = shift_2
[ RUN      ] StandaloneSuite.ConditionalCompileTime/double
FILE:#: Failure
Failed
double branch

[  FAILED  ] StandaloneSuite.ConditionalCompileTime/double, where TypeParam = double
[----------] 12 tests from OutputFixtureTest
[ RUN      ] OutputFixtureTest.FixtureOnlyParams/setup_100/NoArena
FILE:#: Failure
Failed
FixtureOnlyParams setup_val=100

[  FAILED  ] OutputFixtureTest.FixtureOnlyParams/setup_100/NoArena, where GetParam() = setup_100/NoArena
[ RUN      ] OutputFixtureTest.FixtureOnlyParams/setup_200/NoArena
FILE:#: Failure
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
FILE:#: Failure
Failed
ctor_flag=true setup_val=100 body_val=1

[  FAILED  ] OutputFixtureTest.FixtureAndBodyParams/setup_100/Arena/body_1, where GetParam() = setup_100/Arena/body_1
[ RUN      ] OutputFixtureTest.FixtureAndBodyParams/setup_200/Arena/body_1
FILE:#: Failure
Failed
ctor_flag=true setup_val=200 body_val=1

[  FAILED  ] OutputFixtureTest.FixtureAndBodyParams/setup_200/Arena/body_1, where GetParam() = setup_200/Arena/body_1
[ RUN      ] OutputFixtureTest.FixtureAndBodyParams/setup_100/NoArena/body_2
FILE:#: Failure
Failed
ctor_flag=false setup_val=100 body_val=2

[  FAILED  ] OutputFixtureTest.FixtureAndBodyParams/setup_100/NoArena/body_2, where GetParam() = setup_100/NoArena/body_2
[ RUN      ] OutputFixtureTest.FixtureAndBodyParams/setup_200/NoArena/body_2
FILE:#: Failure
Failed
ctor_flag=false setup_val=200 body_val=2

[  FAILED  ] OutputFixtureTest.FixtureAndBodyParams/setup_200/NoArena/body_2, where GetParam() = setup_200/NoArena/body_2
[ RUN      ] OutputFixtureTest.FixtureAndBodyParams/setup_100/Arena/body_2
[       OK ] OutputFixtureTest.FixtureAndBodyParams/setup_100/Arena/body_2
[ RUN      ] OutputFixtureTest.FixtureAndBodyParams/setup_200/Arena/body_2
[       OK ] OutputFixtureTest.FixtureAndBodyParams/setup_200/Arena/body_2
[----------] Global test environment tear-down
[==========] 28 tests from 2 test suites ran.
[  PASSED  ] 11 tests.
[  FAILED  ] 17 tests, listed below:
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
[  FAILED  ] OutputFixtureTest.FixtureOnlyParams/setup_100/NoArena, where GetParam() = setup_100/NoArena
[  FAILED  ] OutputFixtureTest.FixtureOnlyParams/setup_200/NoArena, where GetParam() = setup_200/NoArena
[  FAILED  ] OutputFixtureTest.FixtureAndBodyParams/setup_100/Arena/body_1, where GetParam() = setup_100/Arena/body_1
[  FAILED  ] OutputFixtureTest.FixtureAndBodyParams/setup_200/Arena/body_1, where GetParam() = setup_200/Arena/body_1
[  FAILED  ] OutputFixtureTest.FixtureAndBodyParams/setup_100/NoArena/body_2, where GetParam() = setup_100/NoArena/body_2
[  FAILED  ] OutputFixtureTest.FixtureAndBodyParams/setup_200/NoArena/body_2, where GetParam() = setup_200/NoArena/body_2

17 FAILED TESTS
)";

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <helper_binary_path>\n";
    return 1;
  }

  UnsetEnvVar("XML_OUTPUT_FILE");
  UnsetEnvVar("GTEST_OUTPUT");
  UnsetEnvVar("GUNIT_OUTPUT");
  UnsetEnvVar("TEST_PREMATURE_EXIT_FILE");
  UnsetEnvVar("TEST_SHARD_INDEX");
  UnsetEnvVar("TEST_TOTAL_SHARDS");
  UnsetEnvVar("TEST_SHARD_STATUS_FILE");
  UnsetEnvVar("GTEST_FILTER");
  UnsetEnvVar("GUNIT_FILTER");

  FILE* pipe = OpenPipe(argv[1]);
  if (pipe == nullptr) {
    std::cerr << "Failed to open pipe to command: " << argv[1] << "\n";
    return 1;
  }

  std::string raw_output;
  char buffer[4096];
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    raw_output.append(buffer);
  }
  int status = ClosePipe(pipe);
  if (status == 0) {
    std::cerr << "Expected helper binary to exit with non-zero status due to "
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
