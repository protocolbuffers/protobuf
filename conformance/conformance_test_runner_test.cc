// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Tests of the conformance_test_runner command line parsing and suite driving
// in conformance_test_runner.cc, and of the report helpers in
// conformance_test.cc that the merged runner shares with the legacy suites.

#ifdef _WIN32
#include <direct.h>  // for _chdir, _getcwd, _mkdir
#else
#include <sys/stat.h>  // for mkdir
#include <unistd.h>    // for chdir, getcwd
#endif

#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "absl/status/statusor.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "conformance/conformance.pb.h"
#include "conformance/conformance_test.h"
#include "conformance/test_runner.h"
#include "google/protobuf/message.h"

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

// A suite with no tests.  Besides supplying its failure list flag name to the
// parser, it records what RunSuite() configured by the time RunSuiteImpl()
// runs, and can pretend to have run some of the --test names (which the real
// RunTest() would erase from names_to_test_ before contacting the testee).
class FakeSuite : public ConformanceTestSuite {
 public:
  explicit FakeSuite(absl::string_view failure_list_flag_name) {
    SetFailureListFlagName(std::string(failure_list_flag_name));
  }

  // Names RunSuiteImpl() pretends to run.
  void PretendToRun(std::vector<std::string> names) {
    names_to_pretend_to_run_ = std::move(names);
  }

  int run_count() const { return run_count_; }
  // The failure list file name RunSuite() was given, as seen by RunSuiteImpl().
  const std::string& failure_list_filename_seen() const {
    return failure_list_filename_seen_;
  }
  // The failure list entries loaded by the time RunSuiteImpl() ran.
  const std::vector<std::string>& failure_list_entries_seen() const {
    return failure_list_entries_seen_;
  }
  // The --test names still to run by the time RunSuiteImpl() ran.
  const absl::flat_hash_set<std::string>& names_to_test_seen() const {
    return names_to_test_seen_;
  }

 private:
  bool ParseResponse(const ::conformance::ConformanceResponse& response,
                     const ConformanceRequestSetting& setting,
                     Message* test_message) override {
    return false;
  }
  void RunSuiteImpl() override {
    ++run_count_;
    failure_list_filename_seen_ = failure_list_filename_;
    failure_list_entries_seen_.clear();
    for (const auto& [name, status] : unmatched_) {
      failure_list_entries_seen_.push_back(name);
    }
    names_to_test_seen_ = names_to_test_;
    for (const std::string& name : names_to_pretend_to_run_) {
      names_to_test_.erase(name);
    }
  }

  std::vector<std::string> names_to_pretend_to_run_;
  int run_count_ = 0;
  std::string failure_list_filename_seen_;
  std::vector<std::string> failure_list_entries_seen_;
  absl::flat_hash_set<std::string> names_to_test_seen_;
};

// A runner for suites that must never contact the testee.
class NeverCalledRunner : public ConformanceTestRunner {
 public:
  std::string RunTest(absl::string_view test_name,
                      absl::string_view input) override {
    ADD_FAILURE() << "Unexpected request for " << test_name;
    return "";
  }
};

// Parses `args` (without argv[0], which is added) with the given suites, or,
// by default, with one fake legacy suite whose failure list flag is
// --failure_list.  The real runner has no legacy suite left and accepts both
// --failure_list and --text_format_failure_list without one (see
// kGtestOnlyFailureListFlags in conformance_test_runner.cc); the tests below
// cover both ways.
absl::StatusOr<ConformanceRunnerOptions> Parse(
    std::vector<std::string> args,
    absl::Span<ConformanceTestSuite* const> suites) {
  std::vector<std::string> owned_args = {"conformance_test_runner"};
  owned_args.insert(owned_args.end(), args.begin(), args.end());
  std::vector<char*> argv;
  for (std::string& arg : owned_args) argv.push_back(arg.data());
  argv.push_back(nullptr);
  return ParseConformanceRunnerArgs(static_cast<int>(owned_args.size()),
                                    argv.data(), suites);
}

// Thin wrappers over the POSIX calls and their MSVC CRT spellings.
int MakeDirectory(const std::string& dir) {
#ifdef _WIN32
  return _mkdir(dir.c_str());
#else
  return mkdir(dir.c_str(), 0755);
#endif
}

int ChangeDirectory(const std::string& dir) {
#ifdef _WIN32
  return _chdir(dir.c_str());
#else
  return chdir(dir.c_str());
#endif
}

// Both glibc/macOS (as an extension) and the MSVC CRT malloc() a buffer of the
// right size when given a null one.
char* CurrentDirectory() {
#ifdef _WIN32
  return _getcwd(nullptr, 0);
#else
  return getcwd(nullptr, 0);
#endif
}

absl::StatusOr<ConformanceRunnerOptions> Parse(std::vector<std::string> args) {
  FakeSuite binary_suite("--failure_list");
  std::vector<ConformanceTestSuite*> suites = {&binary_suite};
  return Parse(std::move(args), suites);
}

// Returns a fresh directory for one test's output files, with a trailing '/'.
std::string NewOutputDir() {
  const testing::TestInfo& test_info =
      *testing::UnitTest::GetInstance()->current_test_info();
  std::string dir =
      absl::StrCat(testing::TempDir(), test_info.test_suite_name(), "_",
                   test_info.name(), "/");
  EXPECT_TRUE(MakeDirectory(dir) == 0 || errno == EEXIST) << dir;
  return dir;
}

// Writes `content` to a file called `name` in `dir` and returns its path.
std::string WriteFile(absl::string_view dir, absl::string_view name,
                      absl::string_view content) {
  std::string path = absl::StrCat(dir, name);
  std::ofstream out(path);
  out << content;
  EXPECT_TRUE(out.good()) << path;
  return path;
}

std::string ReadFile(absl::string_view path) {
  std::ifstream in{std::string(path)};
  EXPECT_TRUE(in.is_open()) << path;
  return std::string(std::istreambuf_iterator<char>(in), {});
}

bool FileExists(absl::string_view path) {
  return std::ifstream(std::string(path)).is_open();
}

::conformance::FailureSet FailureSetOf(
    std::vector<std::pair<std::string, std::string>> entries) {
  ::conformance::FailureSet failure_set;
  for (auto& [name, message] : entries) {
    ::conformance::TestStatus* test = failure_set.add_test();
    test->set_name(name);
    test->set_failure_message(message);
  }
  return failure_set;
}

TEST(ParseConformanceRunnerArgsTest, Defaults) {
  absl::StatusOr<ConformanceRunnerOptions> options = Parse({"testee"});
  ASSERT_THAT(options, IsOk());

  EXPECT_EQ(options->testee, "testee");
  EXPECT_THAT(options->testee_args, IsEmpty());
  EXPECT_THAT(options->failure_list_files, IsEmpty());
  EXPECT_FALSE(options->performance);
  EXPECT_FALSE(options->debug);
  EXPECT_FALSE(options->verbose);
  EXPECT_FALSE(options->enforce_recommended);
  EXPECT_EQ(options->maximum_edition, EDITION_UNKNOWN);
  EXPECT_THAT(options->output_dir, IsEmpty());
  EXPECT_THAT(options->record_requests_file, IsEmpty());
  EXPECT_THAT(options->names_to_test, IsEmpty());
  EXPECT_FALSE(options->isolated);
  EXPECT_THAT(options->gtest_args, IsEmpty());
}

TEST(ParseConformanceRunnerArgsTest, NoTesteeIsAccepted) {
  // The legacy runner never checked for one; the suite fails to spawn later.
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
  EXPECT_TRUE(options->debug);
  EXPECT_TRUE(options->verbose);
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
  EXPECT_TRUE(options->verbose);
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

// --text_format_failure_list is accepted although no suite claims it, and is
// recorded under its own name.
TEST(ParseConformanceRunnerArgsTest, GtestOnlyFailureListFlagIsAccepted) {
  absl::StatusOr<ConformanceRunnerOptions> options =
      Parse({"--text_format_failure_list", "t.txt", "testee"});
  ASSERT_THAT(options, IsOk());
  EXPECT_THAT(options->failure_list_files,
              ElementsAre(Pair("--text_format_failure_list", "t.txt")));
  EXPECT_THAT(options->FailureListFilesFor("--failure_list"), IsEmpty());
}

// So is --failure_list, with no suite at all (the real runner's case now that
// no legacy suite is left): both flags are recorded, in command line order.
TEST(ParseConformanceRunnerArgsTest, FailureListFlagsAreAcceptedWithoutSuites) {
  absl::StatusOr<ConformanceRunnerOptions> options =
      Parse({"--text_format_failure_list", "t.txt", "--failure_list", "f.txt",
             "testee"},
            /*suites=*/{});
  ASSERT_THAT(options, IsOk());
  EXPECT_THAT(options->failure_list_files,
              ElementsAre(Pair("--text_format_failure_list", "t.txt"),
                          Pair("--failure_list", "f.txt")));
  EXPECT_THAT(options->FailureListFilesFor("--failure_list"),
              ElementsAre("f.txt"));
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

TEST(RunConformanceTestsTest, RejectsGtestArgs) {
  // Rejected while parsing, before any suite (or testee) runs.
  FakeSuite suite("--failure_list");
  std::vector<ConformanceTestSuite*> suites = {&suite};
  std::vector<std::string> owned_args = {"conformance_test_runner",
                                         "--gtest_filter=*", "testee"};
  std::vector<char*> argv;
  for (std::string& arg : owned_args) argv.push_back(arg.data());
  argv.push_back(nullptr);

  EXPECT_EQ(RunConformanceTests(static_cast<int>(owned_args.size()),
                                argv.data(), suites),
            EXIT_FAILURE);
}

TEST(RunConformanceTestsTest, RejectsNoSuites) {
  std::vector<std::string> owned_args = {"conformance_test_runner", "testee"};
  std::vector<char*> argv;
  for (std::string& arg : owned_args) argv.push_back(arg.data());
  argv.push_back(nullptr);

  EXPECT_EQ(
      RunConformanceTests(static_cast<int>(owned_args.size()), argv.data(), {}),
      EXIT_FAILURE);
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

// Tests of RunConformanceSuites(), which drives FakeSuites that never contact
// the testee, so the testee named on the command line is never started.
class RunConformanceSuitesTest : public testing::Test {
 protected:
  // Runs `suites` with the parsed `args` (plus an --output_dir and a testee)
  // and returns the result and everything written to stderr.
  std::pair<bool, std::string> Run(
      std::vector<std::string> args,
      absl::Span<ConformanceTestSuite* const> suites,
      const absl::flat_hash_set<std::string>* unmatched_candidates = nullptr) {
    args.push_back("--output_dir");
    args.push_back(output_dir_);
    args.push_back("never_started_testee");
    absl::StatusOr<ConformanceRunnerOptions> options = Parse(args);
    EXPECT_THAT(options, IsOk());
    if (!options.ok()) {
      return {false, std::string(options.status().message())};
    }
    testing::internal::CaptureStderr();
    bool ok = RunConformanceSuites(
        *options, suites, /*record_requests=*/nullptr, unmatched_candidates);
    return {ok, testing::internal::GetCapturedStderr()};
  }

  const std::string output_dir_ = NewOutputDir();
};

TEST_F(RunConformanceSuitesTest, PassesWithoutFailureLists) {
  FakeSuite binary_suite("--failure_list");
  FakeSuite text_format_suite("--text_format_failure_list");
  auto [ok, output] = Run({}, {&binary_suite, &text_format_suite});

  EXPECT_TRUE(ok);
  EXPECT_EQ(binary_suite.run_count(), 1);
  EXPECT_EQ(text_format_suite.run_count(), 1);
  EXPECT_THAT(binary_suite.failure_list_filename_seen(), IsEmpty());
  EXPECT_THAT(binary_suite.failure_list_entries_seen(), IsEmpty());
  EXPECT_THAT(output, Not(HasSubstr("do not exist")));
}

TEST_F(RunConformanceSuitesTest, RepeatedFailureListLoadsAllButNamesTheLast) {
  std::string binary = WriteFile(output_dir_, "binary.txt", "");
  std::string first = WriteFile(output_dir_, "first.txt", "Test.A # a\n");
  std::string second = WriteFile(output_dir_, "second.txt", "Test.B # b\n");
  FakeSuite binary_suite("--failure_list");
  FakeSuite text_format_suite("--text_format_failure_list");

  auto [ok, output] =
      Run({"--failure_list", binary, "--text_format_failure_list", first,
           "--text_format_failure_list", second},
          {&binary_suite, &text_format_suite});

  // Each suite only loads its own flag's files, all of them ...
  EXPECT_THAT(binary_suite.failure_list_entries_seen(), IsEmpty());
  EXPECT_THAT(text_format_suite.failure_list_entries_seen(),
              ElementsAre("Test.A", "Test.B"));
  // ... but names the last one in its report.
  EXPECT_EQ(binary_suite.failure_list_filename_seen(), binary);
  EXPECT_EQ(text_format_suite.failure_list_filename_seen(), second);
  // Neither entry matched a test (the fake suite has none), so the text
  // format suite fails and tells the user to update the last file.
  EXPECT_FALSE(ok);
  EXPECT_THAT(output, HasSubstr("didn't match any actual test name"));
  EXPECT_THAT(output, HasSubstr(absl::StrCat(" -- ", second, " --remove ")));
  EXPECT_THAT(output,
              Not(HasSubstr(absl::StrCat(" -- ", first, " --remove "))));
}

TEST_F(RunConformanceSuitesTest, ReportsTestNamesThatNoSuiteRan) {
  FakeSuite binary_suite("--failure_list");
  FakeSuite text_format_suite("--text_format_failure_list");
  binary_suite.PretendToRun({"Test.Binary"});
  text_format_suite.PretendToRun({"Test.TextFormat"});

  auto [ok, output] = Run({"--test", "Test.Binary", "--test", "Test.TextFormat",
                           "--test", "Test.Missing"},
                          {&binary_suite, &text_format_suite});

  EXPECT_TRUE(ok);  // Like the legacy runner, this doesn't fail the run.
  // Every suite is handed what the previous ones didn't run.
  EXPECT_THAT(
      binary_suite.names_to_test_seen(),
      UnorderedElementsAre("Test.Binary", "Test.TextFormat", "Test.Missing"));
  EXPECT_THAT(text_format_suite.names_to_test_seen(),
              UnorderedElementsAre("Test.TextFormat", "Test.Missing"));
  EXPECT_THAT(output, HasSubstr("they do not exist"));
  EXPECT_THAT(output, HasSubstr("  Test.Missing\n"));
  EXPECT_THAT(output, Not(HasSubstr("Test.Binary")));
  EXPECT_THAT(output, Not(HasSubstr("Test.TextFormat")));
}

TEST_F(RunConformanceSuitesTest, DoesNotReportTestNamesWhenAllRan) {
  FakeSuite suite("--failure_list");
  suite.PretendToRun({"Test.A"});

  auto [ok, output] = Run({"--test", "Test.A"}, {&suite});

  EXPECT_TRUE(ok);
  EXPECT_THAT(output, Not(HasSubstr("do not exist")));
}

TEST_F(RunConformanceSuitesTest, FailsIfAnySuiteFailsAndStopsThere) {
  std::string stale = WriteFile(output_dir_, "stale.txt", "Test.Stale\n");
  FakeSuite binary_suite("--failure_list");
  FakeSuite text_format_suite("--text_format_failure_list");

  // The second suite fails: its entry matches no test.
  auto [ok, output] = Run({"--text_format_failure_list", stale},
                          {&binary_suite, &text_format_suite});
  EXPECT_FALSE(ok);
  EXPECT_EQ(binary_suite.run_count(), 1);
  EXPECT_EQ(text_format_suite.run_count(), 1);
  EXPECT_THAT(output, HasSubstr("CONFORMANCE SUITE PASSED"));
  EXPECT_THAT(output, HasSubstr("CONFORMANCE SUITE FAILED"));

  // The first suite fails: as always, no further suite runs.
  FakeSuite failing_suite("--failure_list");
  FakeSuite never_run_suite("--text_format_failure_list");
  auto [ok_after_first_failed, output_after_first_failed] =
      Run({"--failure_list", stale}, {&failing_suite, &never_run_suite});
  EXPECT_FALSE(ok_after_first_failed);
  EXPECT_EQ(failing_suite.run_count(), 1);
  EXPECT_EQ(never_run_suite.run_count(), 0);
  EXPECT_THAT(output_after_first_failed, HasSubstr("CONFORMANCE SUITE FAILED"));
  EXPECT_THAT(output_after_first_failed,
              Not(HasSubstr("CONFORMANCE SUITE PASSED")));
}

TEST_F(RunConformanceSuitesTest, ExitCodeOfRunConformanceTests) {
  std::string stale = WriteFile(output_dir_, "stale.txt", "Test.Stale\n");
  auto run = [&](std::vector<std::string> args) {
    FakeSuite suite("--failure_list");
    std::vector<ConformanceTestSuite*> suites = {&suite};
    std::vector<std::string> owned_args = {"conformance_test_runner"};
    owned_args.insert(owned_args.end(), args.begin(), args.end());
    owned_args.insert(owned_args.end(),
                      {"--output_dir", output_dir_, "never_started_testee"});
    std::vector<char*> argv;
    for (std::string& arg : owned_args) argv.push_back(arg.data());
    argv.push_back(nullptr);
    testing::internal::CaptureStderr();
    int exit_code = RunConformanceTests(static_cast<int>(owned_args.size()),
                                        argv.data(), suites);
    testing::internal::GetCapturedStderr();
    return exit_code;
  };

  EXPECT_EQ(run({}), EXIT_SUCCESS);
  EXPECT_EQ(run({"--failure_list", stale}), EXIT_FAILURE);
}

TEST_F(RunConformanceSuitesTest, PassesUnmatchedCandidatesToEverySuite) {
  std::string binary_list =
      WriteFile(output_dir_, "binary.txt", "Test.Elsewhere\n");
  std::string text_format_list = WriteFile(output_dir_, "text_format.txt",
                                           "Test.Stale\nTest.Text.Elsewhere\n");
  FakeSuite binary_suite("--failure_list");
  FakeSuite text_format_suite("--text_format_failure_list");

  // Only Test.Stale is unmatched on the other side too, so it is the only
  // entry reported: the binary suite passes and the text format suite fails.
  const absl::flat_hash_set<std::string> candidates = {"Test.Stale"};
  auto [ok, output] = Run({"--failure_list", binary_list,
                           "--text_format_failure_list", text_format_list},
                          {&binary_suite, &text_format_suite}, &candidates);

  EXPECT_FALSE(ok);
  EXPECT_EQ(binary_suite.run_count(), 1);
  EXPECT_EQ(text_format_suite.run_count(), 1);
  EXPECT_THAT(output, HasSubstr("  Test.Stale # \n"));
  EXPECT_THAT(output, Not(HasSubstr("Test.Elsewhere")));
  EXPECT_THAT(output, Not(HasSubstr("Test.Text.Elsewhere")));
  EXPECT_THAT(output, HasSubstr("CONFORMANCE SUITE PASSED"));
  EXPECT_THAT(output, HasSubstr("CONFORMANCE SUITE FAILED"));
  EXPECT_EQ(ReadFile(absl::StrCat(output_dir_, "unmatched.txt")),
            "Test.Stale # \n");
}

TEST(SetUnmatchedCandidatesTest, ReportsEveryUnmatchedEntryByDefault) {
  const std::string output_dir = NewOutputDir();
  FakeSuite suite("--failure_list");
  suite.SetOutputDir(output_dir);
  NeverCalledRunner runner;
  ::conformance::FailureSet failure_list =
      FailureSetOf({{"Test.A", "a"}, {"Test.B", "b"}});

  std::string output;
  EXPECT_FALSE(suite.RunSuite(&runner, &output, "list.txt", &failure_list));
  EXPECT_THAT(output, HasSubstr("  Test.A # a\n"));
  EXPECT_THAT(output, HasSubstr("  Test.B # b\n"));
  EXPECT_THAT(output, HasSubstr(" -- list.txt --remove "));
  EXPECT_EQ(ReadFile(absl::StrCat(output_dir, "unmatched.txt")),
            "Test.A # a\nTest.B # b\n");
}

TEST(SetUnmatchedCandidatesTest, ReportsOnlyCandidates) {
  const std::string output_dir = NewOutputDir();
  FakeSuite suite("--failure_list");
  suite.SetOutputDir(output_dir);
  suite.SetUnmatchedCandidates({"Test.B", "Test.Unrelated"});
  NeverCalledRunner runner;
  ::conformance::FailureSet failure_list =
      FailureSetOf({{"Test.A", "a"}, {"Test.B", "b"}});

  std::string output;
  EXPECT_FALSE(suite.RunSuite(&runner, &output, "list.txt", &failure_list));
  EXPECT_THAT(output, Not(HasSubstr("Test.A")));
  EXPECT_THAT(output, HasSubstr("  Test.B # b\n"));
  EXPECT_THAT(output, Not(HasSubstr("Test.Unrelated")));
  EXPECT_EQ(ReadFile(absl::StrCat(output_dir, "unmatched.txt")),
            "Test.B # b\n");
}

TEST(SetUnmatchedCandidatesTest, NoCandidatesMeansNothingIsUnmatched) {
  const std::string output_dir = NewOutputDir();
  FakeSuite suite("--failure_list");
  suite.SetOutputDir(output_dir);
  suite.SetUnmatchedCandidates({});
  NeverCalledRunner runner;
  ::conformance::FailureSet failure_list =
      FailureSetOf({{"Test.A", "a"}, {"Test.B", "b"}});

  std::string output;
  EXPECT_TRUE(suite.RunSuite(&runner, &output, "list.txt", &failure_list));
  EXPECT_THAT(output, Not(HasSubstr("didn't match any actual test name")));
  EXPECT_THAT(output, HasSubstr("CONFORMANCE SUITE PASSED"));
  EXPECT_FALSE(FileExists(absl::StrCat(output_dir, "unmatched.txt")));
}

TEST(SetUnmatchedCandidatesTest, AppliesToTheNextRunOnly) {
  const std::string output_dir = NewOutputDir();
  FakeSuite suite("--failure_list");
  suite.SetOutputDir(output_dir);
  NeverCalledRunner runner;
  ::conformance::FailureSet failure_list =
      FailureSetOf({{"Test.A", "a"}, {"Test.B", "b"}});

  suite.SetUnmatchedCandidates({"Test.B"});
  std::string output;
  EXPECT_FALSE(suite.RunSuite(&runner, &output, "list.txt", &failure_list));
  EXPECT_THAT(output, Not(HasSubstr("Test.A")));
  EXPECT_THAT(output, HasSubstr("  Test.B # b\n"));

  // The next run starts afresh: every unmatched entry is reported again.
  EXPECT_FALSE(suite.RunSuite(&runner, &output, "list.txt", &failure_list));
  EXPECT_THAT(output, HasSubstr("  Test.A # a\n"));
  EXPECT_THAT(output, HasSubstr("  Test.B # b\n"));
  EXPECT_EQ(ReadFile(absl::StrCat(output_dir, "unmatched.txt")),
            "Test.A # a\nTest.B # b\n");
}

TEST(RunSuiteTest, EmptyOutputDirWritesToTheCurrentDirectory) {
  // Run in a fresh directory so that the file lands somewhere known.
  const std::string output_dir = NewOutputDir();
  std::string previous_dir;
  {
    char* cwd = CurrentDirectory();
    ASSERT_NE(cwd, nullptr);
    previous_dir = cwd;
    free(cwd);
  }
  ASSERT_EQ(ChangeDirectory(output_dir), 0) << output_dir;

  FakeSuite suite("--failure_list");
  NeverCalledRunner runner;
  ::conformance::FailureSet failure_list = FailureSetOf({{"Test.A", "a"}});
  std::string output;
  EXPECT_FALSE(suite.RunSuite(&runner, &output, "list.txt", &failure_list));

  EXPECT_EQ(ChangeDirectory(previous_dir), 0) << previous_dir;
  EXPECT_THAT(output, HasSubstr(" -- list.txt --remove unmatched.txt"));
  EXPECT_EQ(ReadFile(absl::StrCat(output_dir, "unmatched.txt")),
            "Test.A # a\n");
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
