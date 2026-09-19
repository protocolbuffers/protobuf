// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Tests of the conformance_test_runner command line parsing and of the report
// writer in conformance_test_runner.cc (see conformance_test_runner.h).

#include "conformance_test_runner.h"

#include <sys/stat.h>

#include <cerrno>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "absl/status/statusor.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"

namespace google {
namespace protobuf {
namespace {

using ::absl_testing::IsOk;
using ::absl_testing::StatusIs;
using ::testing::ElementsAre;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::Pair;
using ::testing::UnorderedElementsAre;

// Parses `args` (without argv[0], which is added).
absl::StatusOr<ConformanceRunnerOptions> Parse(std::vector<std::string> args) {
  std::vector<std::string> owned_args = {"conformance_test_runner"};
  owned_args.insert(owned_args.end(), args.begin(), args.end());
  std::vector<char*> argv;
  for (std::string& arg : owned_args) argv.push_back(arg.data());
  argv.push_back(nullptr);
  return ParseConformanceRunnerArgs(static_cast<int>(owned_args.size()),
                                    argv.data());
}

// Returns a fresh directory for one test's output files, with a trailing '/'.
std::string NewOutputDir() {
  const testing::TestInfo& test_info =
      *testing::UnitTest::GetInstance()->current_test_info();
  std::string dir =
      absl::StrCat(testing::TempDir(), test_info.test_suite_name(), "_",
                   test_info.name(), "/");
  ABSL_PCHECK(mkdir(dir.c_str(), 0755) == 0 || errno == EEXIST) << dir;
  return dir;
}

std::string ReadFile(absl::string_view path) {
  std::ifstream in{std::string(path)};
  ABSL_CHECK(in.is_open()) << path;
  return std::string(std::istreambuf_iterator<char>(in), {});
}

bool FileExists(absl::string_view path) {
  return std::ifstream(std::string(path)).is_open();
}

TEST(ParseConformanceRunnerArgsTest, Defaults) {
  absl::StatusOr<ConformanceRunnerOptions> options = Parse({"testee"});
  ASSERT_THAT(options, IsOk());

  EXPECT_EQ(options->testee, "testee");
  EXPECT_THAT(options->testee_args, IsEmpty());
  EXPECT_THAT(options->failure_list_files, IsEmpty());
  EXPECT_FALSE(options->performance);
  EXPECT_FALSE(options->enforce_recommended);
  EXPECT_EQ(options->maximum_edition, EDITION_UNKNOWN);
  EXPECT_THAT(options->output_dir, IsEmpty());
  EXPECT_THAT(options->record_requests_file, IsEmpty());
  EXPECT_THAT(options->names_to_test, IsEmpty());
  EXPECT_FALSE(options->isolated);
  EXPECT_THAT(options->gtest_args, IsEmpty());
}

TEST(ParseConformanceRunnerArgsTest, NoTesteeIsAccepted) {
  // The legacy runner never checked for one; the testee fails to spawn later.
  absl::StatusOr<ConformanceRunnerOptions> options = Parse({});
  ASSERT_THAT(options, IsOk());
  EXPECT_THAT(options->testee, IsEmpty());
}

TEST(ParseConformanceRunnerArgsTest, ParsesEveryFlag) {
  absl::StatusOr<ConformanceRunnerOptions> options =
      Parse({"--performance", "--debug", "--verbose", "--enforce_recommended",
             "--maximum_edition", "2023", "--output_dir", "out",
             "--record_requests", "requests.txt", "--test", "Some.Test",
             "--failure_list", "list.txt", "testee"});
  ASSERT_THAT(options, IsOk());

  EXPECT_TRUE(options->performance);
  EXPECT_TRUE(options->enforce_recommended);
  EXPECT_EQ(options->maximum_edition, EDITION_2023);
  EXPECT_EQ(options->output_dir, "out");
  EXPECT_EQ(options->record_requests_file, "requests.txt");
  EXPECT_THAT(options->names_to_test, ElementsAre("Some.Test"));
  EXPECT_THAT(options->failure_list_files,
              ElementsAre(Pair("--failure_list", "list.txt")));
  EXPECT_EQ(options->testee, "testee");
}

TEST(ParseConformanceRunnerArgsTest, EverythingAfterTesteeGoesToTestee) {
  absl::StatusOr<ConformanceRunnerOptions> options =
      Parse({"--verbose", "testee", "--verbose", "--failure_list", "x",
             "--gtest_filter=*", "plain", "--"});
  ASSERT_THAT(options, IsOk());

  EXPECT_EQ(options->testee, "testee");
  EXPECT_THAT(options->testee_args,
              ElementsAre("--verbose", "--failure_list", "x",
                          "--gtest_filter=*", "plain", "--"));
  EXPECT_THAT(options->gtest_args, IsEmpty());
  EXPECT_THAT(options->failure_list_files, IsEmpty());
}

TEST(ParseConformanceRunnerArgsTest, GtestFlagsBeforeTesteeAreCollected) {
  absl::StatusOr<ConformanceRunnerOptions> options =
      Parse({"--gtest_filter=Foo.*", "--verbose", "--gunit_brief=1",
             "--gtest_list_tests", "testee", "--gtest_after"});
  ASSERT_THAT(options, IsOk());

  EXPECT_THAT(options->gtest_args,
              ElementsAre("--gtest_filter=Foo.*", "--gunit_brief=1",
                          "--gtest_list_tests"));
  EXPECT_THAT(options->testee_args, ElementsAre("--gtest_after"));
}

// --debug and --verbose only ever configured the legacy suites; they are still
// accepted (conformance_test_wrapper.sh passes --debug on) and change nothing.
TEST(ParseConformanceRunnerArgsTest, NoEffectFlagsAreAccepted) {
  absl::StatusOr<ConformanceRunnerOptions> with_flags =
      Parse({"--verbose", "--debug", "--test", "Some.Test", "testee"});
  absl::StatusOr<ConformanceRunnerOptions> without_flags =
      Parse({"--test", "Some.Test", "testee"});
  ASSERT_THAT(with_flags, IsOk());
  ASSERT_THAT(without_flags, IsOk());

  EXPECT_EQ(with_flags->testee, without_flags->testee);
  EXPECT_EQ(with_flags->names_to_test, without_flags->names_to_test);
  EXPECT_EQ(with_flags->isolated, without_flags->isolated);
  EXPECT_EQ(with_flags->performance, without_flags->performance);
  EXPECT_EQ(with_flags->enforce_recommended,
            without_flags->enforce_recommended);
  EXPECT_EQ(with_flags->gtest_args, without_flags->gtest_args);
}

TEST(ParseConformanceRunnerArgsTest, DoubleDashIsAnUnknownOption) {
  // Pins the legacy behavior: there is no way to end the runner's own flags
  // other than naming the testee.
  EXPECT_THAT(Parse({"--", "testee"}),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("Unknown option: --")));
}

TEST(ParseConformanceRunnerArgsTest, UnknownOption) {
  EXPECT_THAT(Parse({"--bogus", "testee"}),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("Unknown option: --bogus")));
  EXPECT_THAT(Parse({"-v", "testee"}),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("Unknown option: -v")));
}

class MissingValueTest : public testing::TestWithParam<std::string> {};

TEST_P(MissingValueTest, LastArgumentIsAValueFlag) {
  EXPECT_THAT(Parse({GetParam()}),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("Missing value for " + GetParam())))
      << GetParam();
  EXPECT_THAT(Parse({"--verbose", GetParam()}),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("Missing value for " + GetParam())))
      << GetParam();
  // After the testee it is just an argument for the testee.
  absl::StatusOr<ConformanceRunnerOptions> options =
      Parse({"testee", GetParam()});
  ASSERT_THAT(options, IsOk()) << GetParam();
  EXPECT_THAT(options->testee_args, ElementsAre(GetParam()));
}

INSTANTIATE_TEST_SUITE_P(ValueFlags, MissingValueTest,
                         testing::Values("--maximum_edition", "--output_dir",
                                         "--record_requests", "--test",
                                         "--failure_list",
                                         "--text_format_failure_list"));

TEST(ParseConformanceRunnerArgsTest, DebugRequiresTest) {
  EXPECT_THAT(Parse({"--debug", "testee"}),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("--debug requires --test")));
  EXPECT_THAT(Parse({"--debug", "--test", "Some.Test", "testee"}), IsOk());
}

TEST(ParseConformanceRunnerArgsTest, MaximumEdition) {
  EXPECT_THAT(Parse({"--maximum_edition", "bogus", "testee"}),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("Unknown edition: bogus")));
  // The legacy runner prepends "EDITION_", so only the short form works.
  EXPECT_THAT(Parse({"--maximum_edition", "EDITION_2023", "testee"}),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("Unknown edition: EDITION_2023")));

  absl::StatusOr<ConformanceRunnerOptions> options =
      Parse({"--maximum_edition", "PROTO3", "testee"});
  ASSERT_THAT(options, IsOk());
  EXPECT_EQ(options->maximum_edition, EDITION_PROTO3);
}

// Both failure list flags are recorded under their own name, in command line
// order.
TEST(ParseConformanceRunnerArgsTest, FailureListFlagsAreRecordedByName) {
  absl::StatusOr<ConformanceRunnerOptions> options =
      Parse({"--text_format_failure_list", "t.txt", "--failure_list", "f.txt",
             "testee"});
  ASSERT_THAT(options, IsOk());
  EXPECT_THAT(options->failure_list_files,
              ElementsAre(Pair("--text_format_failure_list", "t.txt"),
                          Pair("--failure_list", "f.txt")));
  EXPECT_THAT(options->FailureListFilesFor("--failure_list"),
              ElementsAre("f.txt"));
  EXPECT_THAT(options->FailureListFilesFor("--text_format_failure_list"),
              ElementsAre("t.txt"));
}

TEST(ParseConformanceRunnerArgsTest, FailureListFlagsKeepCommandLineOrder) {
  absl::StatusOr<ConformanceRunnerOptions> options =
      Parse({"--text_format_failure_list", "t1.txt", "--failure_list", "b1.txt",
             "--text_format_failure_list", "t2.txt", "--failure_list", "b2.txt",
             "testee"});
  ASSERT_THAT(options, IsOk());

  EXPECT_THAT(options->failure_list_files,
              ElementsAre(Pair("--text_format_failure_list", "t1.txt"),
                          Pair("--failure_list", "b1.txt"),
                          Pair("--text_format_failure_list", "t2.txt"),
                          Pair("--failure_list", "b2.txt")));
  EXPECT_THAT(options->FailureListFilesFor("--failure_list"),
              ElementsAre("b1.txt", "b2.txt"));
  EXPECT_THAT(options->FailureListFilesFor("--text_format_failure_list"),
              ElementsAre("t1.txt", "t2.txt"));
  EXPECT_THAT(options->FailureListFilesFor("--other_failure_list"), IsEmpty());
}

TEST(ParseConformanceRunnerArgsTest, IsolatedFollowsFromTest) {
  absl::StatusOr<ConformanceRunnerOptions> options = Parse(
      {"--test", "Test.B", "--test", "Test.A", "--test", "Test.B", "testee"});
  ASSERT_THAT(options, IsOk());
  EXPECT_TRUE(options->isolated);
  EXPECT_THAT(options->names_to_test, UnorderedElementsAre("Test.A", "Test.B"));

  options = Parse({"testee"});
  ASSERT_THAT(options, IsOk());
  EXPECT_FALSE(options->isolated);
}

TEST(RewriteGtestFlagPrefixTest, RewritesLeadingGtestPrefixOnly) {
  EXPECT_EQ(RewriteGtestFlagPrefix("--gtest_filter=Foo.*", "gunit_"),
            "--gunit_filter=Foo.*");
  EXPECT_EQ(RewriteGtestFlagPrefix("--gtest_list_tests", "gunit_"),
            "--gunit_list_tests");
  // A no-op when the prefix already is gtest's own, as in open source.
  EXPECT_EQ(RewriteGtestFlagPrefix("--gtest_filter=Foo.*", "gtest_"),
            "--gtest_filter=Foo.*");
  // Already spelled with the target prefix, or not a gtest flag at all.
  EXPECT_EQ(RewriteGtestFlagPrefix("--gunit_filter=Foo.*", "gunit_"),
            "--gunit_filter=Foo.*");
  EXPECT_EQ(RewriteGtestFlagPrefix("--verbose", "gunit_"), "--verbose");
  EXPECT_EQ(RewriteGtestFlagPrefix("gtest_filter=x", "gunit_"),
            "gtest_filter=x");
  EXPECT_EQ(RewriteGtestFlagPrefix("--gtest_", "gunit_"), "--gunit_");
  EXPECT_EQ(RewriteGtestFlagPrefix("", "gunit_"), "");
}

TEST(IsPerformanceFixtureTest, PlainFixture) {
  EXPECT_TRUE(IsPerformanceFixture("BinaryPerformanceTest"));
  EXPECT_TRUE(IsPerformanceFixture("PerformanceTest"));
  EXPECT_FALSE(IsPerformanceFixture("BinaryTest"));
  EXPECT_FALSE(IsPerformanceFixture("PerformanceTestOfSomething"));
  EXPECT_FALSE(IsPerformanceFixture("BinaryPerformance"));
  EXPECT_FALSE(IsPerformanceFixture(""));
}

TEST(IsPerformanceFixtureTest, InstantiatedFixtures) {
  // Value-parameterized: <instantiation>/<fixture>.
  EXPECT_TRUE(IsPerformanceFixture("Proto3/UnknownFieldsPerformanceTest"));
  EXPECT_FALSE(IsPerformanceFixture("Proto3/UnknownFieldsTest"));
  // Typed: <fixture>/<index or type name>.
  EXPECT_TRUE(IsPerformanceFixture("DepthLimitPerformanceTest/0"));
  EXPECT_TRUE(IsPerformanceFixture("DepthLimitPerformanceTest/TestAllTypes"));
  EXPECT_FALSE(IsPerformanceFixture("DepthLimitTest/0"));
  // Type-parameterized: <instantiation>/<fixture>/<index or type name>.
  EXPECT_TRUE(IsPerformanceFixture("Editions/DepthLimitPerformanceTest/1"));
  EXPECT_FALSE(IsPerformanceFixture("Editions/DepthLimitTest/1"));
  // A two-component name does not tell whether it is <instantiation>/<fixture>
  // or <fixture>/<type name>, so every component counts: an instantiation or
  // type name ending in the suffix marks the suite too (nothing else is
  // expected to be named that way), while the suffix inside a component or
  // in the test name does not.
  EXPECT_TRUE(IsPerformanceFixture("PerformanceTest/DepthLimitTest"));
  EXPECT_TRUE(IsPerformanceFixture("Inst/DepthLimitTest/PerformanceTest"));
  EXPECT_FALSE(IsPerformanceFixture("Inst/DepthLimitTest/PerformanceTestType"));
}

// Fixtures of the performance kind in every form gtest registers a fixture
// in, so that the test below can check the names gtest reports for them (the
// input of IsPerformanceFixture() in the merged runner) against real gtest
// rather than against this file's reading of gtest's documentation.
class ProbePerformanceTest : public testing::Test {};
TEST_F(ProbePerformanceTest, IsRegistered) {}

class ProbeParameterizedPerformanceTest : public testing::TestWithParam<int> {};
TEST_P(ProbeParameterizedPerformanceTest, IsRegistered) {}
INSTANTIATE_TEST_SUITE_P(Probe, ProbeParameterizedPerformanceTest,
                         testing::Values(0));

template <typename T>
class ProbeTypedPerformanceTest : public testing::Test {};
TYPED_TEST_SUITE(ProbeTypedPerformanceTest, testing::Types<int>);
TYPED_TEST(ProbeTypedPerformanceTest, IsRegistered) {}

template <typename T>
class ProbeTypeParameterizedPerformanceTest : public testing::Test {};
TYPED_TEST_SUITE_P(ProbeTypeParameterizedPerformanceTest);
TYPED_TEST_P(ProbeTypeParameterizedPerformanceTest, IsRegistered) {}
REGISTER_TYPED_TEST_SUITE_P(ProbeTypeParameterizedPerformanceTest,
                            IsRegistered);
INSTANTIATE_TYPED_TEST_SUITE_P(Probe, ProbeTypeParameterizedPerformanceTest,
                               testing::Types<int>);

TEST(IsPerformanceFixtureTest, RecognizesEveryFormGtestRegisters) {
  const testing::UnitTest& unit_test = *testing::UnitTest::GetInstance();
  std::vector<std::string> probes;
  for (int i = 0; i < unit_test.total_test_suite_count(); ++i) {
    const std::string name = unit_test.GetTestSuite(i)->name();
    if (absl::StrContains(name, "Probe")) probes.push_back(name);
  }
  EXPECT_THAT(probes, UnorderedElementsAre(
                          "ProbePerformanceTest",
                          "Probe/ProbeParameterizedPerformanceTest",
                          "ProbeTypedPerformanceTest/0",
                          "Probe/ProbeTypeParameterizedPerformanceTest/0"));
  for (const std::string& name : probes) {
    EXPECT_TRUE(IsPerformanceFixture(name)) << name;
  }
  // The fixtures of this file's other tests are of the regular kind.
  EXPECT_FALSE(IsPerformanceFixture("IsPerformanceFixtureTest"));
  EXPECT_FALSE(IsPerformanceFixture("ValueFlags/MissingValueTest"));
}

TEST(PerformanceGtestFilterTest, ExcludesTheOtherKind) {
  const std::vector<std::string> suites = {
      "BinaryTest", "Proto3/UnknownFieldsPerformanceTest",
      "DepthLimitPerformanceTest/0", "Editions/DelimitedTest/1"};
  EXPECT_EQ(PerformanceGtestFilter("*", /*performance=*/true, suites),
            "*-BinaryTest.*:Editions/DelimitedTest/1.*");
  EXPECT_EQ(PerformanceGtestFilter("*", /*performance=*/false, suites),
            "*-Proto3/UnknownFieldsPerformanceTest.*:"
            "DepthLimitPerformanceTest/0.*");
}

TEST(PerformanceGtestFilterTest, KeepsAnExplicitFilter) {
  const std::vector<std::string> suites = {"BinaryTest",
                                           "BinaryPerformanceTest"};
  // Positive patterns only.
  EXPECT_EQ(PerformanceGtestFilter("Binary*.Foo:*.Bar", true, suites),
            "Binary*.Foo:*.Bar-BinaryTest.*");
  // Positive and negative patterns.
  EXPECT_EQ(PerformanceGtestFilter("*.Foo-*.Bar:*.Baz", false, suites),
            "*.Foo-*.Bar:*.Baz:BinaryPerformanceTest.*");
  // Negative patterns only: gtest takes the positive part as "*".
  EXPECT_EQ(PerformanceGtestFilter("-*.Bar", true, suites),
            "*-*.Bar:BinaryTest.*");
  EXPECT_EQ(PerformanceGtestFilter("", true, suites), "*-BinaryTest.*");
}

TEST(PerformanceGtestFilterTest, UnchangedWhenNothingToExclude) {
  EXPECT_EQ(PerformanceGtestFilter("*", true, {}), "*");
  EXPECT_EQ(PerformanceGtestFilter("Foo.*-Bar.*", false, {"BinaryTest"}),
            "Foo.*-Bar.*");
  EXPECT_EQ(PerformanceGtestFilter("Foo.*", true, {"BinaryPerformanceTest"}),
            "Foo.*");
}
TEST(PrintConformanceRunnerUsageTest, DocumentsEveryOption) {
  testing::internal::CaptureStderr();
  PrintConformanceRunnerUsage("Some error");
  const std::string usage = testing::internal::GetCapturedStderr();

  EXPECT_THAT(usage, HasSubstr("Some error\nUsage: conformance-test-runner"));
  for (absl::string_view option :
       {"--failure_list", "--text_format_failure_list", "--enforce_recommended",
        "--maximum_edition", "--output_dir", "--test", "--debug",
        "--performance", "--verbose", "--record_requests",
        "--gtest_<flag>=<value>"}) {
    EXPECT_THAT(usage, HasSubstr(absl::StrCat("\n  ", option, " "))) << option;
  }
  EXPECT_THAT(usage, HasSubstr("--gunit_"));
}

TEST(ReportTestStatusSetTest, EmptySetReportsNothing) {
  const std::string output_dir = NewOutputDir();
  std::string output = "before";
  EXPECT_TRUE(ReportTestStatusSet({}, "empty.txt", "Some message", output_dir,
                                  &output));
  EXPECT_EQ(output, "before");
  EXPECT_FALSE(FileExists(absl::StrCat(output_dir, "empty.txt")));
}

TEST(ReportTestStatusSetTest, WritesReportAndFile) {
  const std::string output_dir = NewOutputDir();
  ReportedTestStatus added;
  added.test_name = "Test.Added";
  added.failure_message = "it failed";
  ReportedTestStatus removed;
  removed.test_name = "Test.Removed";
  removed.failure_message = "it passed";
  removed.matched_name = "Test.*";
  const std::vector<ReportedTestStatus> statuses = {added, removed};

  std::string output = "before\n";
  EXPECT_FALSE(ReportTestStatusSet(statuses, "report.txt", "Some message",
                                   output_dir, &output));
  // The report always names the test itself ...
  EXPECT_EQ(output,
            "before\n"
            "\n"
            "Some message\n"
            "\n"
            "  Test.Added # it failed\n"
            "  Test.Removed # it passed\n"
            "\n");
  // ... whereas the file for update_failure_list has the failure list entry
  // to remove, if there is one, and the test name to add otherwise.
  EXPECT_EQ(ReadFile(absl::StrCat(output_dir, "report.txt")),
            "Test.Added # it failed\n"
            "Test.* # it passed\n");
}

TEST(ReportTestStatusSetTest, WritesRelativeToCurrentDirWithoutOutputDir) {
  // An empty output_dir means the file name is used as is.  Point it into the
  // temp dir to keep the test hermetic.
  const std::string output_dir = NewOutputDir();
  ReportedTestStatus status;
  status.test_name = "Test.A";
  const std::vector<ReportedTestStatus> statuses = {status};

  std::string output;
  EXPECT_FALSE(ReportTestStatusSet(statuses,
                                   absl::StrCat(output_dir, "plain.txt"),
                                   "Some message", "", &output));
  EXPECT_EQ(ReadFile(absl::StrCat(output_dir, "plain.txt")), "Test.A # \n");
}

TEST(ReportTestStatusSetTest, NoFileNameMeansNoFile) {
  const std::string output_dir = NewOutputDir();
  ReportedTestStatus status;
  status.test_name = "Test.A";
  status.failure_message = "message";
  const std::vector<ReportedTestStatus> statuses = {status};

  std::string output;
  EXPECT_FALSE(
      ReportTestStatusSet(statuses, "", "Some message", output_dir, &output));
  EXPECT_THAT(output, HasSubstr("  Test.A # message\n"));
  EXPECT_THAT(output, Not(HasSubstr("Failed to open file")));
}

TEST(ReportTestStatusSetTest, ReportsUnwritableFile) {
  const std::string output_dir =
      absl::StrCat(NewOutputDir(), "does_not_exist/");
  ReportedTestStatus status;
  status.test_name = "Test.A";
  const std::vector<ReportedTestStatus> statuses = {status};

  std::string output;
  EXPECT_FALSE(ReportTestStatusSet(statuses, "report.txt", "Some message",
                                   output_dir, &output));
  EXPECT_THAT(output, HasSubstr("  Test.A # \n"));
  EXPECT_THAT(output, HasSubstr(absl::StrCat(
                          "Failed to open file: ", output_dir, "report.txt")));
}

}  // namespace
}  // namespace protobuf
}  // namespace google
