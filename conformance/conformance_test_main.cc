// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// The main of conformance_test_runner.
//
// This is a transitional runner: while tests migrate from the legacy suite
// (BinaryAndJsonConformanceSuite; the legacy text-format suite is gone, its
// tests all live in the gtest text suite now) to gtest-based conformance
// suites (see test_environment.h), it runs both kinds against the same testee,
// with the legacy command line and failure lists, and gives a single verdict.
// Nothing changes for users of the runner:
//
//   * The command line is the legacy one (see PrintConformanceRunnerUsage()).
//     gtest flags given before the testee are accepted too and only affect
//     the gtest suites.  Both spellings work everywhere: --gtest_* (the open
//     source one) and --gunit_* (the google3 one); a --gtest_ flag is rewritten
//     to the prefix this binary's gtest was built with (see
//     RewriteGtestFlagPrefix()), since google3's gtest rejects the other.
//   * --failure_list is loaded by the legacy suite as before, and additionally
//     into the gtest suites' TestManager.  --text_format_failure_list is
//     still accepted (see kGtestOnlyFailureListFlags in
//     conformance_test_runner.cc) and loaded into the same TestManager; no
//     legacy suite reads it any more.
//   * An entry that matches no test is only reported as unmatched if neither
//     side has a test for it, so failure lists don't have to change while a
//     test moves from one side to the other.  The gtest phase itself reports
//     the unmatched entries of the lists no legacy suite loads
//     (--text_format_failure_list), in the legacy suite's format and naming
//     that list, since the legacy phase never sees them.
//   * --test <name> isolates the named tests on both sides (see
//     FilteringTestRunner), and names that exist on neither side are reported
//     as before.
//   * --performance selects the performance tests on both sides: the legacy
//     suites switch to them as before, and on the gtest side the test suites
//     of the performance conformance suite, which are the ones named
//     *PerformanceTest (see test_environment.h), are selected through the
//     gtest filter, and deselected without the flag (see
//     PerformanceGtestFilter()).
//   * --record_requests writes the requests of both sides into one file.
//   * --output_dir receives the files update_failure_list consumes from both
//     sides: the gtest suites write failing_tests.txt (unexpected failures) and
//     succeeding_tests.txt (unexpected successes) in the legacy format, see
//     ReportTestStatusSet().  Like the legacy suites among themselves, a later
//     phase that has entries in the same category overwrites the file.
//
// The gtest suites run first (see RunGtestPhase()), then the legacy suite,
// whose report is printed exactly as before.  The gtest suites are
// the alwayslink libraries linked into conformance_test_main (see its BUILD
// deps, e.g. binary_conformance_tests); if none were linked in, the gtest
// phase would be skipped entirely and the runner would behave exactly like the
// legacy one.
//
// Behaviour notes, i.e. what does differ from the legacy runner:
//
//   * gtest flags are accepted before the testee, in their `--flag=value` form
//     only (the runner's own parser would take a separate value for the
//     testee).  Anything else starting with --gtest_ or --gunit_ is still
//     rejected, but by whoever sees it first: in google3 InitGoogle() exits
//     with its own "Unknown command line flag" error, in open source the
//     runner prints its usage text as before.
//   * The runner is a self-contained program: the environment variables
//     through which bazel and the test bridge configure a gtest binary
//     (kInheritedTestEnvironmentVariables: test filter, sharding, fail-fast,
//     result and status files, ...) are meant for the enclosing test and are
//     cleared before gtest initializes, since the legacy phase ignores them
//     too, and the gtest flags open source gtest has already defaulted from
//     them by then are reset (see ResetInheritedGtestFlags()).  Explicit
//     gtest flags on the command line are the only way to filter, shard or
//     get an XML/JSON report of the gtest phase.  As a consequence, the
//     testee doesn't see those variables either.
//   * Unless a gtest flag was given, the gtest phase prints gtest's brief
//     output (--gtest_brief=1): failures, skips and the summary, not every
//     test.
//   * --debug and --verbose only affect the legacy suites: `--debug --test
//     <name>` prints nothing for a test that only the gtest suites have (the
//     test just runs), and --verbose doesn't change the gtest phase's output
//     (--gtest_brief=0 lists every test's result).
//   * testing::InitGoogleTest() now always runs, even when no gtest suite is
//     linked in.  In google3 that also runs InitGoogle(): absl logging is
//     initialized (ABSL_LOG(INFO) lines, e.g. the testee command line logged
//     by ForkPipeRunner, go to the log files instead of stderr) and the
//     failure signal handler is installed.  Nothing else about the legacy
//     phase changes.
//   * --gtest_repeat is not supported (see test_environment.h).
//   * --gtest_filter, test sharding and --gtest_fail_fast make the gtest phase
//     a partial run, which disables the unmatched-entry check on both sides: a
//     failure list entry that neither side matched may simply belong to a test
//     that wasn't selected (or run).  The same applies when the filter selects
//     no test at all.  The selection --performance (or its absence) makes is
//     not partial: its baseline is the tests of the selected kind (see
//     IsPartialGtestPhase()), and when there is none the legacy suites check
//     their entries alone.  An explicit --gtest_filter applies within that
//     selection.
//   * --gtest_output only takes effect when given on the command line itself,
//     not through --gtest_flagfile (see GtestOutputRequested()).
//   * If writing the --record_requests file fails *and* --test named tests
//     that exist on neither side, the legacy "do not exist" report is printed
//     first (by the legacy phase) and the write error last.
//   * The cap on how many tests one wildcard failure list entry may match
//     applies per phase, so an entry may match up to the cap on each side.
//   * The gtest suites only report the two --output_dir categories above.  A
//     test that was expected to fail with a different message is an unexpected
//     failure to them (the legacy suites report it separately, in
//     unexpected_failure_messages.txt and expected_failure_messages.txt).
//     Since the gtest suites load every failure list into one TestManager,
//     their report comes in one block per failure list flag, each with the
//     hint naming that flag's last file: a result matched by a failure list
//     entry goes to the flag whose file lists the entry, a test in no list to
//     --text_format_failure_list if it is a text-format test (see
//     IsTextFormatTest()) and to --failure_list otherwise.  Like the legacy
//     suites among themselves, a later block overwrites the --output_dir
//     files of an earlier one.
//     TODO: b/563657875 - per-suite failure lists arrive with the
//     conformance_test() macros.

#include <algorithm>
#include <cstddef>
#include <cstdio>  // IWYU pragma: keep
#include <cstdlib>
#include <fstream>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include <gtest/gtest.h>
#include "absl/algorithm/container.h"
#include "absl/base/nullability.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "absl/types/span.h"
#include "conformance/binary_json_conformance_suite.h"
#include "conformance/conformance_test.h"
#include "conformance/filtering_test_runner.h"
#include "conformance/fork_pipe_runner.h"
#include "conformance/recording_test_runner.h"
#include "conformance/test_environment.h"
#include "conformance/test_manager.h"
#include "conformance/test_runner.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

// The environment variables through which bazel and the test bridge configure
// the gtest binary they run.  Under `bazel test` they describe the *enclosing*
// test (typically an sh_test wrapping this runner), so the gtest phase must not
// act on them: it would run only that test's shard or filter, stop at its
// first failure, or overwrite its result file.  Cleared before
// testing::InitGoogleTest(); see the file comment and
// ResetInheritedGtestFlags().  Both the open source (GTEST_) and google3
// (GUNIT_) spellings are listed.
constexpr const char* kInheritedTestEnvironmentVariables[] = {
    // Test selection, sharding and shuffling.
    "TESTBRIDGE_TEST_ONLY",
    "TEST_TOTAL_SHARDS",
    "TEST_SHARD_INDEX",
    "TEST_SHARD_STATUS_FILE",
    "GTEST_FILTER",
    "GUNIT_FILTER",
    "GTEST_FAIL_IF_NO_TEST_SELECTED",
    "GUNIT_FAIL_IF_NO_TEST_SELECTED",
    "GTEST_ALSO_RUN_DISABLED_TESTS",
    "GUNIT_ALSO_RUN_DISABLED_TESTS",
    "GTEST_SHUFFLE",
    "GUNIT_SHUFFLE",
    "GTEST_RANDOM_SEED",
    "GUNIT_RANDOM_SEED",
    // How far the run goes and how it is reported.
    "TESTBRIDGE_TEST_RUNNER_FAIL_FAST",
    "GTEST_FAIL_FAST",
    "GUNIT_FAIL_FAST",
    "GTEST_REPEAT",
    "GUNIT_REPEAT",
    "GTEST_BRIEF",
    "GUNIT_BRIEF",
    "GTEST_FLAGFILE",
    "GUNIT_FLAGFILE",
    // Result and status files.
    "GTEST_OUTPUT",
    "GUNIT_OUTPUT",
    "XML_OUTPUT_FILE",
    "TEST_PREMATURE_EXIT_FILE",
    "TEST_WARNINGS_OUTPUT_FILE",
};

// Resets the gtest flags that kInheritedTestEnvironmentVariables would have
// defaulted to their built-in values, and picks the brief output unless
// `gtest_args` (the explicit gtest flags) configure gtest.  Must be called
// after the variables are cleared and before testing::InitGoogleTest().
//
// Clearing the variables is enough in google3, whose gtest flags are absl
// flags that evaluate their environment-derived default lazily, on first
// access; that happens after the variables are gone.  Open source gtest
// evaluates them once, at static initialization, before main() runs: by now
// the enclosing test's filter, fail-fast, shuffle, ... settings are already
// baked into the flags, so they are reset here.  Explicit gtest flags are
// parsed afterwards, by InitGoogleTest(), and therefore still win.
//
// Not reset here: sharding, whose flags don't exist in every gtest version
// and whose variables gtest only reads in RUN_ALL_TESTS() (after they are
// cleared; a shard that got through anyway makes the run partial, see
// IsPartialRun()), and the output flag, which is handled after
// InitGoogleTest() (see GtestOutputRequested()).
void ResetInheritedGtestFlags(absl::Span<const std::string> gtest_args) {
  GTEST_FLAG_SET(filter, "*");
  GTEST_FLAG_SET(also_run_disabled_tests, false);
  GTEST_FLAG_SET(shuffle, false);
  GTEST_FLAG_SET(random_seed, 0);
  GTEST_FLAG_SET(fail_fast, false);
  GTEST_FLAG_SET(repeat, 1);
  // Keep the gtest phase's output as short as the legacy reports, unless the
  // caller configured gtest themselves.  Like the others, this has to happen
  // before InitGoogleTest(), which is where gtest picks its result printer
  // (in PostFlagParsingInit()).
  GTEST_FLAG_SET(brief, gtest_args.empty());
}

// The gtest output flag, spelled as this binary's gtest expects it, e.g.
// "--gunit_output=".  gtest only accepts the `--flag=value` form for it (and so
// does this runner, see the file comment), so that is the only form matched.
constexpr absl::string_view kGtestOutputFlagPrefix =
    "--" GTEST_FLAG_PREFIX_ "output=";

// Returns true if gtest output was asked for explicitly on the command line.
// `gtest_args` must already have been through RewriteGtestFlagPrefix().  The
// environment variables gtest would otherwise default the flag from are
// deliberately not consulted: they belong to the enclosing test (see
// kInheritedTestEnvironmentVariables).  Neither is --gtest_flagfile: an output
// flag given through it isn't seen here and so has no effect.
bool GtestOutputRequested(absl::Span<const std::string> gtest_args) {
  for (const std::string& arg : gtest_args) {
    if (absl::StartsWith(arg, kGtestOutputFlagPrefix)) return true;
  }
  return false;
}

// The names of every gtest test suite registered in this binary.
std::vector<std::string> RegisteredTestSuiteNames() {
  const testing::UnitTest& unit_test = *testing::UnitTest::GetInstance();
  const int count = unit_test.total_test_suite_count();
  std::vector<std::string> names;
  names.reserve(static_cast<size_t>(count));
  for (int i = 0; i < count; ++i) {
    names.push_back(unit_test.GetTestSuite(i)->name());
  }
  return names;
}

// Narrows the gtest filter to the performance test suites if `performance`,
// and to all the others otherwise; see PerformanceGtestFilter().  Must be
// called after testing::InitGoogleTest() (which parses an explicit
// --gtest_filter) and before the tests are listed or run.
void SelectPerformanceTests(bool performance) {
  GTEST_FLAG_SET(filter,
                 PerformanceGtestFilter(GTEST_FLAG_GET(filter), performance,
                                        RegisteredTestSuiteNames()));
}

// The number of tests a full gtest phase runs after SelectPerformanceTests():
// the tests of the selected kind, minus the DISABLED_ ones (like
// internal::IsPartialRun()'s baseline, which counts every registered test).
int SelectedTestCount(bool performance) {
  const testing::UnitTest& unit_test = *testing::UnitTest::GetInstance();
  int count = 0;
  for (int i = 0; i < unit_test.total_test_suite_count(); ++i) {
    const testing::TestSuite& test_suite = *unit_test.GetTestSuite(i);
    if (IsPerformanceFixture(test_suite.name()) != performance) continue;
    count += test_suite.total_test_count() - test_suite.disabled_test_count();
  }
  return count;
}

// internal::IsPartialRun() for the gtest phase: whether only some of the
// `selected_test_count` tests of the selected kind ran, because of an explicit
// --gtest_filter, sharding or --gtest_fail_fast.  IsPartialRun() itself would
// count the tests SelectPerformanceTests() deselected as not run, and so call
// every run partial.
bool IsPartialGtestPhase(int selected_test_count) {
  const testing::UnitTest& unit_test = *testing::UnitTest::GetInstance();
  if (GTEST_FLAG_GET(fail_fast) && unit_test.Failed()) return true;
  return unit_test.test_to_run_count() < selected_test_count;
}

ConformanceEnvironmentOptions EnvironmentOptionsFor(
    const ConformanceRunnerOptions& options,
    ConformanceTestRunner* absl_nonnull runner) {
  ConformanceEnvironmentOptions env_options;
  env_options.runner = runner;
  for (const auto& [flag, file] : options.failure_list_files) {
    env_options.failure_list_files.push_back(file);
  }
  env_options.enforce_recommended = options.enforce_recommended;
  // The legacy runner treats any maximum below PROTO3 (its EDITION_UNKNOWN
  // default, or an explicit PROTO2) as "proto2 and proto3 only", which is the
  // environment's EDITION_PROTO3.  ConformanceEnvironment clamps this itself;
  // the std::max here just keeps the intent visible.  Note that the two sides
  // gate editions tests differently: the legacy suites run *all* of them
  // (including the EDITION_UNSTABLE ones) once the maximum is >= 2023, whereas
  // the gtest suites compare against each test message's own edition.
  // Migrations preserve the legacy behavior test by test.
  env_options.maximum_edition =
      std::max(options.maximum_edition, EDITION_PROTO3);
  // --performance is applied through the gtest filter instead; see
  // SelectPerformanceTests().
  // Unseen entries are reconciled with the legacy suites below instead.
  env_options.check_unseen_expected_failures = false;
  // --debug and --verbose only affect the legacy suites; use gtest's own
  // output for the gtest suites (see the file comment).
  return env_options;
}

// Converts `results` for ReportTestStatusSet().
std::vector<ReportedTestStatus> ToReportedTestStatuses(
    absl::Span<const internal::UnexpectedResult> results) {
  std::vector<ReportedTestStatus> statuses;
  statuses.reserve(results.size());
  for (const internal::UnexpectedResult& result : results) {
    ReportedTestStatus status;
    status.test_name = result.test_name;
    status.failure_message = result.failure_message;
    status.matched_name = result.matched_entry.value_or("");
    statuses.push_back(std::move(status));
  }
  return statuses;
}

// The failure list flags the gtest suites' report is keyed by:
// kFailureListFlag is the legacy binary/JSON suite's flag (see
// ConformanceTestSuite::GetFailureListFlagName()), kTextFormatFailureListFlag
// the legacy text-format suite's (see kGtestOnlyFailureListFlags in
// conformance_test_runner.cc).
constexpr absl::string_view kFailureListFlag = "--failure_list";
constexpr absl::string_view kTextFormatFailureListFlag =
    "--text_format_failure_list";

// Whether `test_name` is a text-format conformance test, i.e. one the legacy
// text-format suite used to run: a test with text-format input or output.
// Test names read "<Level>.<Edition>.<Format>Input.<name>[.<Format>Output]"
// (see Testee).
bool IsTextFormatTest(absl::string_view test_name) {
  return absl::StrContains(test_name, ".TextFormatInput.") ||
         absl::EndsWith(test_name, ".TextFormatOutput");
}

// The failure list flag whose block reports a test that is in no failure
// list, with the hint to add it to that flag's last file.
absl::string_view FailureListFlagFor(absl::string_view test_name) {
  return IsTextFormatTest(test_name) ? kTextFormatFailureListFlag
                                     : kFailureListFlag;
}

// One block of the gtest suites' report: the results one failure list flag's
// files are responsible for, reported by ReportFailureListResults() with the
// hint naming the flag's last file (like each legacy suite names the last
// file of its flag).
struct FailureListReport {
  std::string flag;
  std::vector<std::string> files;
  // Whether a legacy suite loads the files too.  It then reports the entries
  // no test matched itself (see
  // ConformanceTestSuite::SetUnmatchedCandidates()); otherwise this block does.
  bool loaded_by_legacy_suite = false;
  std::vector<internal::UnexpectedResult> successes;
  std::vector<internal::UnexpectedResult> failures;
  // (test name, matched entry); see TestManager::ListedSkips().
  std::vector<std::pair<std::string, std::string>> listed_skips;
  // The entries no test matched, with their messages.
  std::vector<ReportedTestStatus> unmatched;
};

// Where a failure list entry (as listed, wildcards included) comes from.
struct FailureListEntry {
  size_t report = 0;  // An index into the FailureListReports.
  std::string message;
};

// The report block for `flag`, appended if there is none yet (with no files:
// the caller appends those).
FailureListReport& ReportFor(std::vector<FailureListReport>& reports,
                             absl::string_view flag) {
  for (FailureListReport& report : reports) {
    if (report.flag == flag) return report;
  }
  reports.push_back({/*flag=*/std::string(flag)});
  return reports.back();
}

// Groups the failure lists (`failure_list_files`, (flag, file) pairs in
// command line order) into one report block per flag, in order of the flags'
// first appearance, and indexes their entries in `entries`.  The files are
// loaded into a scratch TestManager exactly as ConformanceEnvironment will,
// so that a bad failure list is reported once and clearly instead of failing
// every gtest test in SetUp().  The environment loads all the files into one
// TestManager, so unlike with the legacy suites (each of which only loads its
// own flag's files) an entry may only appear in one of them: the same name in
// both --failure_list and --text_format_failure_list, or a wildcard in one
// covering an entry of the other, is an error.
// TODO: b/563657875 - per-suite failure lists arrive with the
// conformance_test() macros.
absl::Status LoadFailureListReports(
    absl::Span<const std::pair<std::string, std::string>> failure_list_files,
    absl::Span<const std::string> legacy_failure_list_flags,
    std::vector<FailureListReport>& reports,
    absl::flat_hash_map<std::string, FailureListEntry>& entries) {
  internal::TestManager manager;
  absl::Status status;
  std::vector<std::string> loaded;
  for (const auto& [flag, file] : failure_list_files) {
    status = manager.LoadFailureList(file);
    if (!status.ok()) {
      if (absl::IsAlreadyExists(status)) {
        status = absl::InvalidArgumentError(absl::StrCat(
            "While loading failure list ", file,
            loaded.empty()
                ? ""
                : absl::StrCat(" after ", absl::StrJoin(loaded, ", ")),
            ": ", status.message(),
            ".  The gtest conformance suites load all the failure lists into "
            "one set, so an entry (or a wildcard covering it) may only appear "
            "in one of them."));
      }
      break;
    }
    loaded.push_back(file);
    FailureListReport& report = ReportFor(reports, flag);
    report.files.push_back(file);
    report.loaded_by_legacy_suite =
        absl::c_find(legacy_failure_list_flags, flag) !=
        legacy_failure_list_flags.end();
    // The entries this file added are the ones not indexed yet.  (Nothing has
    // been reported, so no entry has been matched.)
    const size_t index = static_cast<size_t>(&report - reports.data());
    for (const std::string& entry : manager.UnmatchedExpectedFailures()) {
      if (entries.contains(entry)) continue;
      entries.emplace(entry,
                      FailureListEntry{
                          /*report=*/index,
                          /*message=*/*manager.ExpectedFailureMessage(entry),
                      });
    }
  }
  // TestManager insists on being finalized before destruction.
  manager.Finalize().IgnoreError();
  return status;
}

// Reports `report` (the gtest suites' unexpected successes, unexpected
// failures, listed skips, i.e. tests in the failure list that the testee
// skipped, and unmatched entries) to stderr and to succeeding_tests.txt /
// failing_tests.txt / listed_skips.txt / unmatched.txt in `output_dir` (which
// must end in '/' unless empty), exactly as each legacy suite reports its own;
// see ReportTestStatusSet().  The hints tell the user to update the report's
// last file.  Returns false if there were unmatched entries (the only category
// the gtest assertions haven't already failed for).
bool ReportFailureListResults(const FailureListReport& report,
                              absl::string_view output_dir) {
  // A block may have no file: a test in no list is routed to its flag's block
  // (see FailureListFlagFor()) whether or not that flag was given.
  const std::string failure_list =
      report.files.empty() ? absl::StrCat("<the ", report.flag, " file>")
                           : report.files.back();
  std::string output;
  ReportTestStatusSet(
      ToReportedTestStatuses(report.successes), "succeeding_tests.txt",
      absl::StrCat(
          "These tests succeeded, even though they were listed in "
          "the failure list (expanded from wildcard(s) or direct matches). "
          " Remove their match from the failure list by "
          "running from the root of your workspace:\n"
          "  bazel run "
          "//third_party/protobuf/conformance:update_failure_list -- ",
          failure_list, " --remove ", output_dir, "succeeding_tests.txt"),
      output_dir, &output);
  ReportTestStatusSet(
      ToReportedTestStatuses(report.failures), "failing_tests.txt",
      absl::StrCat("These tests failed.  If they can't be fixed right now, "
                   "you can add them to the failure list so the overall "
                   "suite can succeed.  Add them to the failure list by "
                   "running from the root of your workspace:\n"
                   "  bazel run "
                   "//third_party/protobuf/conformance:update_failure_list -- ",
                   failure_list, " --add ", output_dir, "failing_tests.txt"),
      output_dir, &output);
  std::vector<ReportedTestStatus> listed_skips;
  listed_skips.reserve(report.listed_skips.size());
  for (const auto& [test_name, matched_entry] : report.listed_skips) {
    listed_skips.push_back(
        {/*test_name=*/test_name,
         /*failure_message=*/
         absl::StrCat("skipped by the testee (matched to ", matched_entry, ")"),
         /*matched_name=*/matched_entry});
  }
  ReportTestStatusSet(
      listed_skips, "listed_skips.txt",
      absl::StrCat("These tests were skipped by the testee, even though they "
                   "were listed in the failure list (expanded from wildcard(s) "
                   "or direct matches).  A skipped test can't be an expected "
                   "failure.  Remove their match from the failure list by "
                   "running from the root of your workspace:\n"
                   "  bazel run "
                   "//third_party/protobuf/conformance:update_failure_list -- ",
                   failure_list, " --remove ", output_dir, "listed_skips.txt"),
      output_dir, &output);
  const bool no_unmatched = ReportTestStatusSet(
      report.unmatched, "unmatched.txt",
      absl::StrCat("These test names were listed in the failure list, but they "
                   "didn't match any actual test name.  Remove them from the "
                   "failure list by running from the root of your workspace:\n"
                   "  bazel run "
                   "//third_party/protobuf/conformance:update_failure_list -- ",
                   failure_list, " --remove ", output_dir, "unmatched.txt"),
      output_dir, &output);
  absl::FPrintF(stderr, "%s", output);
  return no_unmatched;
}

// What the gtest phase leaves behind for the legacy phase.
struct GtestPhaseResult {
  // RUN_ALL_TESTS()'s result, or 1 if a failure list only the gtest suites
  // load had unmatched entries; 0 if no gtest suite is linked in.
  int exit_code = 0;
  // The failure list entries the gtest suites have no test for, if they ran;
  // see ConformanceTestSuite::SetUnmatchedCandidates().  Empty (but present)
  // after a partial run, so that the legacy suites report none at all; absent
  // if the gtest suites have no test of the selected kind, so that they report
  // their own.
  absl::optional<absl::flat_hash_set<std::string>> unmatched_candidates;
  // The tests --test named that the gtest suites ran.
  std::vector<std::string> names_run;
};

// Runs the gtest suites linked into this binary against `options.testee`,
// prints their report and returns what the legacy phase needs to know.  If no
// gtest suite is linked in, nothing runs and the default result is returned.
// `legacy_failure_list_flags` are the legacy suites' failure list flags: the
// files of any other flag (--text_format_failure_list) are loaded by the gtest
// suites alone, so this phase also checks them for unmatched entries, which
// fail the run like they failed the legacy text-format suite.  Returns an
// error, before anything runs, if the failure lists can't be loaded into one
// TestManager (see LoadFailureListReports()).  Must be called after
// testing::InitGoogleTest(), and at most once (it installs the
// ConformanceEnvironment and calls RUN_ALL_TESTS()).
absl::StatusOr<GtestPhaseResult> RunGtestPhase(
    const ConformanceRunnerOptions& options,
    absl::Span<const std::string> legacy_failure_list_flags,
    std::ostream* absl_nullable record_requests) {
  GtestPhaseResult result;
  testing::UnitTest* unit_test = testing::UnitTest::GetInstance();
  // The gtest phase only exists if some gtest suite is linked in; otherwise
  // the legacy suites decide on their own.  (The lists only they would check
  // for unmatched entries then go unchecked; no current binary gets here.)
  if (unit_test->total_test_count() == 0) return result;

  // Like each legacy suite, the phase gets its own ForkPipeRunner (which only
  // spawns the testee on first use), wrapped for recording and for --test
  // isolation.  The runners only need to live for this call: the environment
  // installed below outlives them, but it doesn't own the runner and drops its
  // reference to it in TearDown() (run by gtest, or below if gtest selected no
  // test); ~ConformanceEnvironment only finalizes its TestManager.
  ForkPipeRunner fork_pipe_runner(options.testee, options.testee_args);
  ConformanceTestRunner* gtest_runner = &fork_pipe_runner;
  absl::optional<RecordingTestRunner> recording_runner;
  if (record_requests != nullptr) {
    recording_runner.emplace(gtest_runner, record_requests);
    gtest_runner = &*recording_runner;
  }
  absl::optional<FilteringTestRunner> filtering_runner;
  if (options.isolated) {
    filtering_runner.emplace(gtest_runner, options.names_to_test);
    gtest_runner = &*filtering_runner;
  }

  ConformanceEnvironmentOptions env_options =
      EnvironmentOptionsFor(options, gtest_runner);
  std::vector<FailureListReport> reports;
  absl::flat_hash_map<std::string, FailureListEntry> entries;
  if (absl::Status status =
          LoadFailureListReports(options.failure_list_files,
                                 legacy_failure_list_flags, reports, entries);
      !status.ok()) {
    return status;
  }

  ConformanceEnvironment& environment =
      ConformanceEnvironment::Install(std::move(env_options));
  const int selected_test_count = SelectedTestCount(options.performance);
  result.exit_code = RUN_ALL_TESTS();
  if (unit_test->test_to_run_count() == 0) {
    // gtest only runs an environment's SetUp() and TearDown() if it selected
    // some test to run (which is only known now, so the runners above can't
    // be created conditionally).  Run TearDown() ourselves, so that the
    // environment (which outlives this call, see Install()) lets go of the
    // runners destroyed at the end of it instead of keeping a dangling
    // pointer.  There is nothing else for it to do: SetUp() never ran, so no
    // failure list was loaded, and no test ran either.
    environment.TearDown();
  }

  const internal::TestManager& manager = environment.test_manager();
  // The report block a result belongs to: the one of the entry it matched,
  // or, for a test in no list, the one FailureListFlagFor() says.
  auto report_for_entry = [&](absl::string_view entry) -> FailureListReport& {
    return reports[entries.at(entry).report];
  };
  if (IsPartialGtestPhase(selected_test_count)) {
    // Only some of the gtest tests of the selected kind ran (--gtest_filter,
    // sharding, --gtest_fail_fast), or none at all (in which case gtest
    // didn't even set the environment up), so an entry they didn't match may
    // well belong to a test that wasn't selected.  Let the legacy suites
    // report none at all rather than guess, and don't guess here either; a
    // full run checks them.
    result.unmatched_candidates.emplace();
  } else if (selected_test_count > 0) {
    result.unmatched_candidates.emplace();
    for (const std::string& entry : manager.UnmatchedExpectedFailures()) {
      FailureListReport& report = report_for_entry(entry);
      if (report.loaded_by_legacy_suite) {
        result.unmatched_candidates->insert(entry);
      } else {
        // No legacy suite will look for it: report it here.
        report.unmatched.push_back(
            {/*test_name=*/entry,
             /*failure_message=*/entries.at(entry).message});
      }
    }
  }
  // Otherwise the gtest suites have no test of the selected kind at all (e.g.
  // no performance test is linked in yet and --performance was given): a full
  // run of nothing, which matched no entry and loaded no list, so the legacy
  // suites check their entries alone, as if no gtest suite were linked in.
  if (filtering_runner.has_value()) {
    result.names_run = filtering_runner->NamesRun();
  }

  for (const internal::UnexpectedResult& success :
       manager.UnexpectedSuccesses()) {
    report_for_entry(*success.matched_entry).successes.push_back(success);
  }
  // An unexpected failure has no matched entry (see UnexpectedResult), also
  // when it was listed with a different message, so it is routed by name.
  // A text-format test listed in a --failure_list file with a stale message
  // therefore gets the --text_format_failure_list hint; the list it is in is
  // unaffected either way, since SaveFailureList() replaces the entry in
  // place.
  for (const internal::UnexpectedResult& failure :
       manager.UnexpectedFailures()) {
    ReportFor(reports, FailureListFlagFor(failure.test_name))
        .failures.push_back(failure);
  }
  for (const auto& listed_skip : manager.ListedSkips()) {
    report_for_entry(listed_skip.second).listed_skips.push_back(listed_skip);
  }

  // One report block per failure list flag, like the legacy suites' own
  // reports.
  std::string output_dir = options.output_dir;
  if (!output_dir.empty() && output_dir.back() != '/') {
    output_dir.push_back('/');
  }
  for (const FailureListReport& report : reports) {
    if (!ReportFailureListResults(report, output_dir) &&
        result.exit_code == 0) {
      result.exit_code = 1;
    }
  }
  absl::FPrintF(stderr,
                "CONFORMANCE GTEST SUITES %s: %d tests: %d successes, %d "
                "skipped, %d expected failures, %d unexpected failures, "
                "%d unexpected successes, %d listed skips\n\n",
                result.exit_code == 0 ? "PASSED" : "FAILED",
                unit_test->test_to_run_count(), manager.expected_successes(),
                manager.skipped(), manager.expected_failures(),
                manager.unexpected_failures(), manager.unexpected_successes(),
                manager.listed_skips());
  return result;
}

int Main(int argc, char* argv[]) {
  BinaryAndJsonConformanceSuite binary_and_json_suite;
  const std::vector<ConformanceTestSuite*> suites = {&binary_and_json_suite};
  std::vector<std::string> legacy_failure_list_flags;
  legacy_failure_list_flags.reserve(suites.size());
  for (ConformanceTestSuite* suite : suites) {
    legacy_failure_list_flags.push_back(suite->GetFailureListFlagName());
  }

  // The legacy parser knows which of its flags take a value, so it is the one
  // that can tell the gtest flags (which it collects) from the testee's
  // arguments (which may well start with --gtest_ too).
  absl::StatusOr<ConformanceRunnerOptions> options =
      ParseConformanceRunnerArgs(argc, argv, suites);
  if (!options.ok()) {
    PrintConformanceRunnerUsage(options.status().message());
    return EXIT_FAILURE;
  }
  for (std::string& arg : options->gtest_args) {
    arg = RewriteGtestFlagPrefix(arg, GTEST_FLAG_PREFIX_);
  }

  // See the file comment: only the command line configures the gtest phase.
  // (In google3, unsetenv() alone does the job because the flags' defaults are
  // evaluated lazily; see ResetInheritedGtestFlags() for open source.)
  for (const char* variable : kInheritedTestEnvironmentVariables) {
#ifdef _WIN32
    _putenv_s(variable, "");  // the MSVC CRT has no unsetenv()
#else
    unsetenv(variable);
#endif
  }
  ResetInheritedGtestFlags(options->gtest_args);

  // In google3, InitGoogleTest() also runs InitGoogle(), which exits on any
  // flag it doesn't know, so it only gets to see the gtest flags.  (In open
  // source it's plain gtest, which leaves unknown flags in place.)  Whatever it
  // leaves behind is an unknown option, as it was for the legacy runner.
  std::vector<char*> gtest_argv = {argv[0]};
  gtest_argv.reserve(options->gtest_args.size() + 2);
  for (std::string& arg : options->gtest_args) {
    gtest_argv.push_back(arg.data());
  }
  // Like a real argv, the vector must be null-terminated: InitGoogleTest()
  // removes the flags it consumed by shifting the rest down and writing
  // argv[argc] = nullptr.
  gtest_argv.push_back(nullptr);
  int gtest_argc = static_cast<int>(gtest_argv.size()) - 1;
  testing::InitGoogleTest(&gtest_argc, gtest_argv.data());
  if (gtest_argc > 1) {
    PrintConformanceRunnerUsage(
        absl::StrCat("Unknown option: ", gtest_argv[1]));
    return EXIT_FAILURE;
  }

  // The gtest side of --performance; the legacy suites take it from
  // `options` (see RunConformanceSuites()).
  SelectPerformanceTests(options->performance);

  if (GTEST_FLAG_GET(list_tests)) return RUN_ALL_TESTS();

  // The environment variables gtest would default --gtest_output from are
  // cleared above, so by now the flag is only set if the command line asked
  // for a report.  Should gtest have found a default some other way, don't let
  // it write anywhere: InitGoogleTest() already installed the output listener
  // for the default, so clearing the flag isn't enough, release the listener
  // as well.  Despite its name, default_xml_generator() is the listener for
  // whichever format the flag selected, so this covers the JSON printer too.
  if (!GtestOutputRequested(options->gtest_args)) {
    GTEST_FLAG_SET(output, "");
    testing::TestEventListeners& listeners =
        testing::UnitTest::GetInstance()->listeners();
    std::unique_ptr<testing::TestEventListener> released_listener(
        listeners.Release(listeners.default_xml_generator()));
  }

  // Opened once (truncating) so that the requests of both phases end up in a
  // single file.
  std::ofstream record_requests_file;
  if (!options->record_requests_file.empty()) {
    record_requests_file.open(options->record_requests_file);
    if (!record_requests_file.is_open()) {
      absl::FPrintF(stderr, "Couldn't open record_requests file: %s\n",
                    options->record_requests_file);
      return EXIT_FAILURE;
    }
  }
  std::ostream* record_requests =
      record_requests_file.is_open() ? &record_requests_file : nullptr;

  absl::StatusOr<GtestPhaseResult> gtest_result =
      RunGtestPhase(*options, legacy_failure_list_flags, record_requests);
  if (!gtest_result.ok()) {
    absl::FPrintF(stderr, "%s\n", gtest_result.status().message());
    return EXIT_FAILURE;
  }

  // The legacy phase, exactly as before except that it doesn't report the
  // tests --test named that the gtest suites ran, nor failure list entries the
  // gtest suites have a test for (or that only they load).
  ConformanceRunnerOptions legacy_options = *options;
  for (const std::string& name : gtest_result->names_run) {
    legacy_options.names_to_test.erase(name);
  }
  bool legacy_ok =
      RunConformanceSuites(legacy_options, suites, record_requests,
                           gtest_result->unmatched_candidates.has_value()
                               ? &*gtest_result->unmatched_candidates
                               : nullptr);

  if (record_requests_file.is_open()) {
    record_requests_file.close();
    if (!record_requests_file) {
      absl::FPrintF(stderr, "Error writing record_requests file: %s\n",
                    options->record_requests_file);
      return EXIT_FAILURE;
    }
  }

  return legacy_ok && gtest_result->exit_code == 0 ? EXIT_SUCCESS
                                                   : EXIT_FAILURE;
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google

int main(int argc, char* argv[]) {
  return google::protobuf::conformance::Main(argc, argv);
}
