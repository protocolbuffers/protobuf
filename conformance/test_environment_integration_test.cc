// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// End-to-end checks of test_environment_main and ConformanceEnvironment,
// running the deliberately failing sample_conformance_test against the real
// C++ testee.  Only what needs a real test process is checked here: the
// command line handling of the main, what gtest itself contributes to a run
// (partial-run detection, the XML report) and the exit code.  Everything else
// is covered in-process by test_environment_test.

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <string>
#include <vector>

#ifndef _WIN32
#include <sys/wait.h>  // for WEXITSTATUS
#endif

#include "tools/cpp/runfiles/runfiles.h"
#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/file.h"
#include <gmock/gmock.h>
#include "google/protobuf/testing/googletest.h"
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::AllOf;
using ::testing::HasSubstr;
using ::testing::Not;

// The names of the sample tests, as they appear in failure lists.
constexpr absl::string_view kPass =
    "Required.Proto3.ProtobufInput.SamplePass.ProtobufOutput";
constexpr absl::string_view kDeliberateFailure =
    "Required.Proto3.ProtobufInput.SampleDeliberateFailure.ProtobufOutput";
constexpr absl::string_view kP1Failure =
    "Recommended.Proto3.ProtobufInput.SampleP1Failure.ProtobufOutput";
constexpr absl::string_view kNeverRuns =
    "Required.Proto3.ProtobufInput.SampleNeverRuns.ProtobufOutput";
constexpr absl::string_view kFailureMessage =
    "Should have failed to parse, but didn't.";

// gtest spells its flags --gtest_*, Google's internal build --gunit_*.
constexpr absl::string_view kGtestFlag = "--" GTEST_FLAG_PREFIX_;

std::string Rlocation(const std::string& path) {
  static const bazel::tools::cpp::runfiles::Runfiles* const runfiles = [] {
    std::string error;
    bazel::tools::cpp::runfiles::Runfiles* runfiles =
        bazel::tools::cpp::runfiles::Runfiles::CreateForTest(&error);
    ABSL_CHECK(runfiles != nullptr) << error;
    return runfiles;
  }();
  return runfiles->Rlocation(path);
}

// The BUILD rule passes the sample test and the testee as $(rlocationpath)
// in these variables.
std::string RunfilesPathFromEnvironment(const char* name) {
  const char* path = std::getenv(name);
  ABSL_CHECK(path != nullptr) << name << " is not set";
  return path;
}

const std::string& SampleTest() {
  static const std::string* const path =
      new std::string(Rlocation(RunfilesPathFromEnvironment("SAMPLE_TEST")));
  return *path;
}

const std::string& TesteeBinary() {
  static const std::string* const path =
      new std::string(Rlocation(RunfilesPathFromEnvironment("TESTEE")));
  return *path;
}

// The path of sample_failure_list_<name>.txt, a data dependency from the
// sample test's package.
std::string FailureListPath(absl::string_view name) {
  std::string package = RunfilesPathFromEnvironment("SAMPLE_TEST");
  package.erase(package.rfind('/'));
  return Rlocation(
      absl::StrCat(package, "/sample_failure_list_", name, ".txt"));
}

std::string FailureListFlag(absl::string_view name) {
  return absl::StrCat("--failure_list=", FailureListPath(name));
}

std::string TmpPath(absl::string_view name) {
  return absl::StrCat(TestTempDir(), "/", name);
}

std::string WriteFile(absl::string_view name, absl::string_view content) {
  std::string path = TmpPath(name);
  ABSL_CHECK_OK(File::SetContents(path, content, true));
  return path;
}

// `path` is a std::string (not a string_view) for the open source
// File::GetContents() shim.
std::string ReadFile(const std::string& path) {
  std::string content;
  ABSL_CHECK_OK(File::GetContents(path, &content, true));
  return content;
}

// Quotes `arg` for the shell that popen() runs commands through.
std::string ShellQuote(absl::string_view arg) {
#ifdef _WIN32
  return absl::StrCat("\"", arg, "\"");
#else
  return absl::StrCat("'", absl::StrReplaceAll(arg, {{"'", "'\\''"}}), "'");
#endif
}

struct RunResult {
  int exit_code;
  std::string output;  // stdout and stderr
};

// Runs `binary` with `args` and returns its exit code and output.
RunResult RunBinary(const std::string& binary,
                    absl::Span<const std::string> args) {
  std::string command = ShellQuote(binary);
  for (const std::string& arg : args) {
    absl::StrAppend(&command, " ", ShellQuote(arg));
  }
  absl::StrAppend(&command, " 2>&1");
#ifdef _WIN32
  // cmd.exe strips the first and last quote of a command line that starts
  // with one, so give it a pair to strip.
  FILE* pipe = _popen(absl::StrCat("\"", command, "\"").c_str(), "r");
#else
  FILE* pipe = popen(command.c_str(), "r");
#endif
  ABSL_CHECK(pipe != nullptr) << "Failed to run " << command;

  RunResult result;
  char buffer[4096];
  size_t n;
  while ((n = fread(buffer, 1, sizeof(buffer), pipe)) > 0) {
    result.output.append(buffer, n);
  }
#ifdef _WIN32
  result.exit_code = _pclose(pipe);
#else
  int status = pclose(pipe);
  result.exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
#endif
  return result;
}

// Runs the sample test against the C++ testee with `args`.
RunResult RunSample(std::vector<std::string> args) {
  args.insert(args.begin(), absl::StrCat("--testee_binary=", TesteeBinary()));
  return RunBinary(SampleTest(), args);
}

// Returns the part of the gtest XML `report` from the element opening with
// `open` up to the first of `ends`.  gtest writes an element's properties
// before its children, so the right `ends` isolate one element's properties.
std::string XmlScope(const std::string& report, absl::string_view open,
                     std::initializer_list<absl::string_view> ends) {
  size_t begin = report.find(open);
  if (begin == std::string::npos) return "";
  size_t end = report.size();
  for (absl::string_view terminator : ends) {
    end = std::min(end, report.find(terminator, begin + open.size()));
  }
  return report.substr(begin, end - begin);
}

std::string XmlTestCase(const std::string& report, absl::string_view name) {
  return XmlScope(report, absl::StrCat("<testcase name=\"", name, "\""),
                  {"<testcase", "</testsuite>"});
}

std::string XmlTestSuite(const std::string& report, absl::string_view name) {
  return XmlScope(report, absl::StrCat("<testsuite name=\"", name, "\""),
                  {"<testcase", "</testsuite>"});
}

std::string XmlTestRun(const std::string& report) {
  return XmlScope(report, "<testsuites", {"<testsuite "});
}

// Sets `name` to `value` in this process's environment, which the binaries
// the tests run inherit.  The MSVC CRT has no setenv()/unsetenv().
void SetEnvironmentVariable(const char* name, const char* value) {
#ifdef _WIN32
  ABSL_CHECK_EQ(_putenv_s(name, value), 0);
#else
  ABSL_CHECK_EQ(setenv(name, value, /*overwrite=*/1), 0);
#endif
}

void UnsetEnvironmentVariable(const char* name) {
#ifdef _WIN32
  ABSL_CHECK_EQ(_putenv_s(name, ""), 0);  // An empty value removes it.
#else
  ABSL_CHECK_EQ(unsetenv(name), 0);
#endif
}

class TestEnvironmentIntegrationTest : public testing::Test {
 protected:
  static void SetUpTestSuite() {
    // The test runner configures this binary's gtest through these variables
    // (its filter, sharding, XML report, ...).  gtest has read them by now,
    // and they must not reach the binaries the tests run.
    for (const char* name :
         {"TESTBRIDGE_TEST_ONLY", "TESTBRIDGE_TEST_RUNNER_FAIL_FAST",
          "XML_OUTPUT_FILE", "TEST_PREMATURE_EXIT_FILE",
          "TEST_WARNINGS_OUTPUT_FILE", "TEST_TOTAL_SHARDS", "TEST_SHARD_INDEX",
          "TEST_SHARD_STATUS_FILE", "GTEST_TOTAL_SHARDS", "GTEST_SHARD_INDEX",
          "GTEST_SHARD_STATUS_FILE"}) {
      UnsetEnvironmentVariable(name);
    }
  }
};

TEST_F(TestEnvironmentIntegrationTest, PassesWithAMatchingFailureList) {
  RunResult run = RunSample({FailureListFlag("ok")});
  EXPECT_EQ(run.exit_code, 0) << run.output;
  EXPECT_THAT(run.output,
              AllOf(HasSubstr("[       OK ] SampleConformanceTest.SamplePass"),
                    HasSubstr("[       OK ] "
                              "SampleConformanceTest.SampleDeliberateFailure"),
                    HasSubstr("[       OK ] "
                              "SampleConformanceTest.SampleP1Failure")));
}

TEST_F(TestEnvironmentIntegrationTest, FailsWithAnEmptyFailureList) {
  RunResult run = RunSample({FailureListFlag("empty")});
  EXPECT_NE(run.exit_code, 0);
  EXPECT_THAT(run.output,
              AllOf(HasSubstr("[  FAILED  ] "
                              "SampleConformanceTest.SampleDeliberateFailure"),
                    HasSubstr(kDeliberateFailure), HasSubstr(kFailureMessage)));
}

TEST_F(TestEnvironmentIntegrationTest, AListedTestThatPassesFailsTheRun) {
  // The list also expects the test that passes to fail.
  std::string list = WriteFile(
      "pass_listed.txt", absl::StrCat(ReadFile(FailureListPath("ok")), kPass,
                                      " # Should have failed to parse\n"));
  RunResult run = RunSample({absl::StrCat("--failure_list=", list)});
  EXPECT_NE(run.exit_code, 0);
  EXPECT_THAT(run.output,
              AllOf(HasSubstr("[  FAILED  ] SampleConformanceTest.SamplePass"),
                    HasSubstr("is in the failure list, but test succeeded"),
                    HasSubstr("[       OK ] "
                              "SampleConformanceTest.SampleDeliberateFailure"),
                    HasSubstr("[       OK ] "
                              "SampleConformanceTest.SampleP1Failure")));
}

TEST_F(TestEnvironmentIntegrationTest, EnforcementLevelToleratesP1Failures) {
  // Only the P0 failure is listed.
  std::string list =
      WriteFile("p0_only.txt",
                absl::StrCat(kDeliberateFailure, " # ", kFailureMessage, "\n"));
  std::string xml = TmpPath("tolerated.xml");
  RunResult run =
      RunSample({absl::StrCat("--failure_list=", list), "--enforcement_level=0",
                 absl::StrCat(kGtestFlag, "output=xml:", xml)});
  EXPECT_EQ(run.exit_code, 0) << run.output;
  EXPECT_THAT(run.output,
              HasSubstr("[       OK ] SampleConformanceTest.SampleP1Failure"));
  EXPECT_THAT(XmlTestCase(ReadFile(xml), "SampleP1Failure"),
              HasSubstr("name=\"tolerated_failures\" value=\"1\""));

  // Every priority is enforced by default.
  run = RunSample({absl::StrCat("--failure_list=", list)});
  EXPECT_NE(run.exit_code, 0);
  EXPECT_THAT(
      run.output,
      AllOf(HasSubstr("[  FAILED  ] SampleConformanceTest.SampleP1Failure"),
            HasSubstr(
                absl::StrCat("Unexpected failure for test: ", kP1Failure))));
}

TEST_F(TestEnvironmentIntegrationTest, ListsTestsWithoutATestee) {
  RunResult run =
      RunBinary(SampleTest(), {absl::StrCat(kGtestFlag, "list_tests")});
  EXPECT_EQ(run.exit_code, 0) << run.output;
  EXPECT_THAT(run.output, HasSubstr("SampleDeliberateFailure"));
}

TEST_F(TestEnvironmentIntegrationTest, PassesPositionalArgumentsToTheTestee) {
  // The testee ignores them; this checks that flag parsing lets them through.
  RunResult run =
      RunSample({FailureListFlag("ok"), "--", "--ignored_by_testee"});
  EXPECT_EQ(run.exit_code, 0) << run.output;
}

// Partial runs are detected from gtest's own test counts and state (see
// IsPartialRun() in test_environment.cc), which only a real run sets up.

TEST_F(TestEnvironmentIntegrationTest, UnseenExpectedFailuresFailAFullRun) {
  // The DISABLED_ sample test doesn't run either; that mustn't make the run
  // count as partial.
  RunResult run = RunSample({FailureListFlag("unseen")});
  EXPECT_NE(run.exit_code, 0);
  EXPECT_THAT(
      run.output,
      HasSubstr(absl::StrCat("expected failures were not seen: ", kNeverRuns)));
}

TEST_F(TestEnvironmentIntegrationTest, UnseenExpectedFailuresPassAFilteredRun) {
  RunResult run =
      RunSample({FailureListFlag("unseen"),
                 absl::StrCat(kGtestFlag,
                              "filter=SampleConformanceTest.Sample*Failure")});
  EXPECT_EQ(run.exit_code, 0) << run.output;
  EXPECT_THAT(run.output, Not(HasSubstr("were not seen")));
}

TEST_F(TestEnvironmentIntegrationTest, UnseenExpectedFailuresPassAShardedRun) {
  // gtest takes the shard from the environment: TEST_* in Google's internal
  // build, GTEST_* in open source.
  for (const char* name : {"TEST_TOTAL_SHARDS", "GTEST_TOTAL_SHARDS"}) {
    SetEnvironmentVariable(name, "2");
  }
  for (const char* name : {"TEST_SHARD_INDEX", "GTEST_SHARD_INDEX"}) {
    SetEnvironmentVariable(name, "0");
  }
  RunResult run = RunSample({FailureListFlag("unseen")});
  for (const char* name : {"TEST_TOTAL_SHARDS", "GTEST_TOTAL_SHARDS",
                           "TEST_SHARD_INDEX", "GTEST_SHARD_INDEX"}) {
    UnsetEnvironmentVariable(name);
  }
  EXPECT_EQ(run.exit_code, 0) << run.output;
  EXPECT_THAT(run.output, AllOf(HasSubstr("Note: This is test shard 1 of 2"),
                                Not(HasSubstr("were not seen"))));
}

TEST_F(TestEnvironmentIntegrationTest, AnEarlyStopUnderFailFastIsAPartialRun) {
  // With a list that only names the never-run test, the deliberate failure
  // stops the run before the other tests, and the unseen entry isn't reported
  // on top of it.
  std::string list =
      WriteFile("never_runs_only.txt", absl::StrCat(kNeverRuns, "\n"));
  RunResult run = RunSample({absl::StrCat("--failure_list=", list),
                             absl::StrCat(kGtestFlag, "fail_fast")});
  EXPECT_NE(run.exit_code, 0);
  EXPECT_THAT(run.output,
              AllOf(HasSubstr("[  FAILED  ] "
                              "SampleConformanceTest.SampleDeliberateFailure"),
                    Not(HasSubstr("were not seen"))));
}

TEST_F(TestEnvironmentIntegrationTest, FixRewritesTheFailureListInPlace) {
  std::string list = WriteFile("failure_list.txt", "");
  RunResult run = RunSample({absl::StrCat("--failure_list=", list), "--fix"});
  // The failures were unexpected in this run; the rewritten list expects them.
  EXPECT_NE(run.exit_code, 0);
  EXPECT_THAT(ReadFile(list),
              AllOf(HasSubstr(kDeliberateFailure), HasSubstr(kP1Failure),
                    HasSubstr(absl::StrCat("# ", kFailureMessage))));
  run = RunSample({absl::StrCat("--failure_list=", list)});
  EXPECT_EQ(run.exit_code, 0) << run.output;
}

TEST_F(TestEnvironmentIntegrationTest, RecordsStatisticsInTheXmlReport) {
  std::string xml = TmpPath("report.xml");
  RunResult run = RunSample(
      {FailureListFlag("ok"), absl::StrCat(kGtestFlag, "output=xml:", xml)});
  ASSERT_EQ(run.exit_code, 0) << run.output;
  std::string report = ReadFile(xml);
  // Each level gets its own numbers: the P0 and P1 failures are one expected
  // failure each, two on their suite and on the run.
  EXPECT_THAT(XmlTestCase(report, "SampleDeliberateFailure"),
              HasSubstr("name=\"expected_failures\" value=\"1\""));
  EXPECT_THAT(XmlTestCase(report, "SamplePass"),
              HasSubstr("name=\"expected_successes\" value=\"1\""));
  EXPECT_THAT(XmlTestSuite(report, "SampleConformanceTest"),
              HasSubstr("name=\"expected_failures\" value=\"2\""));
  EXPECT_THAT(XmlTestRun(report),
              AllOf(HasSubstr("name=\"expected_failures\" value=\"2\""),
                    HasSubstr("name=\"unexpected_failures\" value=\"0\"")));
}

TEST_F(TestEnvironmentIntegrationTest, RecordsEachOutcomeInTheXmlReport) {
  std::string xml = TmpPath("outcomes.xml");
  RunResult run = RunSample(
      {FailureListFlag("ok"), absl::StrCat(kGtestFlag, "output=xml:", xml)});
  ASSERT_EQ(run.exit_code, 0) << run.output;
  std::string report = ReadFile(xml);
  // Each conformance test's outcome is a property, named after it, of the
  // gtest test that checked it.  gtest writes ' as &apos; in attributes.
  EXPECT_THAT(
      XmlTestCase(report, "SamplePass"),
      HasSubstr(absl::StrCat("name=\"", kPass, "\" value=\"P0 PASS\"")));
  EXPECT_THAT(
      XmlTestCase(report, "SampleDeliberateFailure"),
      HasSubstr(absl::StrCat("name=\"", kDeliberateFailure,
                             "\" value=\"P0 FAIL: Should have failed to parse, "
                             "but didn&apos;t.\"")));
  EXPECT_THAT(
      XmlTestCase(report, "SampleP1Failure"),
      HasSubstr(absl::StrCat("name=\"", kP1Failure, "\" value=\"P1 FAIL: ")));
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
