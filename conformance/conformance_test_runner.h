// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// The command line of conformance_test_runner and the report format its users
// (the conformance test targets, their update_failure_list tooling and the
// scripts around them) rely on.  The runner itself is conformance_test_main.cc;
// it runs the gtest-based conformance suites (see test_environment.h) against
// the testee named on the command line.  See PrintConformanceRunnerUsage() for
// the options.
//
// The command line and the report format are those of the original
// conformance_test_runner, which ran hand-written ConformanceTestSuite classes
// instead of gtest suites; they are kept so that nothing changes for its users.
// TODO: b/563707827 - retire the failure list flags below together with the
// per-flag report blocks of conformance_test_main.cc once the remaining sh_test
// and script invocations have moved to the conformance_test() macro, which runs
// the gtest suites as ordinary test binaries.

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_CONFORMANCE_TEST_RUNNER_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_CONFORMANCE_TEST_RUNNER_H__

#include <string>
#include <utility>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"

namespace google {
namespace protobuf {

// The failure list flags the runner accepts.  Every file given for either is
// loaded into the gtest suites' one TestManager; the files are recorded under
// their flag (see ConformanceRunnerOptions::failure_list_files) so that the
// runner can tell which list an entry came from and report per list.
// --failure_list is the flag of the binary and JSON tests,
// --text_format_failure_list the one of the text-format tests; the test
// targets and scripts pass both.
inline constexpr absl::string_view kFailureListFlag = "--failure_list";
inline constexpr absl::string_view kTextFormatFailureListFlag =
    "--text_format_failure_list";

// The options accepted on the conformance_test_runner command line.
struct ConformanceRunnerOptions {
  // The first non-flag argument and everything after it.
  std::string testee;
  std::vector<std::string> testee_args;

  // Every failure list flag given, as (flag name, file) pairs in command line
  // order, e.g. ("--failure_list", "failure_list_cpp.txt").
  std::vector<std::pair<std::string, std::string>> failure_list_files;

  bool performance = false;
  bool enforce_recommended = false;
  // EDITION_UNKNOWN (the default) runs the proto2 and proto3 tests only.
  Edition maximum_edition = EDITION_UNKNOWN;
  std::string output_dir;
  // The tests named with --test, and whether to run only those (set by
  // ParseConformanceRunnerArgs() when --test was given).
  absl::flat_hash_set<std::string> names_to_test;
  bool isolated = false;

  // The conformance protocol version to speak to the testee (--protocol_version
  // <version>).  0, the default, detects it through the discovery handshake;
  // N pins it.  Forwarded as is to ConformanceEnvironmentOptions::
  // protocol_version, whose comment in test_environment.h has the details and
  // whose SetUp() rejects a version
  // outside 1..internal::kLatestProtocolVersion; the parser only rejects what
  // isn't a non-negative integer.
  int protocol_version = 0;

  // --output_result_file and --implementation_name: where to write the run's
  // results as a ConformanceRunResult textproto, and the implementation name
  // recorded in it (default: the testee's basename).  See
  // ConformanceEnvironmentOptions::result_file.
  std::string output_result_file;
  std::string implementation_name;

  // Arguments starting with --gtest_ or --gunit_ that appeared before the
  // testee, in order and as given.  conformance_test_main.cc passes them on to
  // gtest, after RewriteGtestFlagPrefix().
  std::vector<std::string> gtest_args;

  // (--debug and --verbose are accepted too, for the scripts that still pass
  // them, but have no effect: they only ever configured the original runner's
  // suites.  gtest's own flags take their place, e.g. --gtest_brief=0 lists
  // every test's result.)

  // Returns the files given for `failure_list_flag`, in command line order.
  std::vector<std::string> FailureListFilesFor(
      absl::string_view failure_list_flag) const;
};

// Parses the conformance_test_runner command line.  On error, returns
// InvalidArgumentError with the message the runner prints before its usage
// text; see PrintConformanceRunnerUsage().
absl::StatusOr<ConformanceRunnerOptions> ParseConformanceRunnerArgs(
    int argc, char* argv[]);

// Returns `arg` with a leading "--gtest_" replaced by "--" + `flag_prefix`;
// any other argument, including one already spelled with `flag_prefix`, is
// returned unchanged.  conformance_test_main.cc applies this to
// ConformanceRunnerOptions::gtest_args with the prefix its gtest was built
// with (GTEST_FLAG_PREFIX_ from gtest-port.h: "gtest_" in open source,
// "gunit_" in google3, whose gtest rejects the --gtest_ spelling), so that the
// documented --gtest_ spelling works everywhere.  GTEST_FLAG_PREFIX_ is one of
// gtest's internal macros, but it is what gtest itself parses its flags by,
// and merged_runner_test.sh checks end to end that a --gtest_ flag reaches
// gtest.
std::string RewriteGtestFlagPrefix(absl::string_view arg,
                                   absl::string_view flag_prefix);

// Returns whether the gtest test suite (fixture) named `test_suite_name`
// belongs to the performance conformance suite, i.e. whether its fixture is
// named `*PerformanceTest` (the naming rule in test_environment.h).  gtest
// names an instantiated fixture `<instantiation>/<fixture>` (value
// parameterized), `<fixture>/<index or type name>` (typed) or
// `<instantiation>/<fixture>/<index or type name>` (type parameterized), so
// every '/'-separated component is checked.
bool IsPerformanceFixture(absl::string_view test_suite_name);

// Returns the gtest filter (`positive patterns[-negative patterns]`, each
// list ':'-separated, see gtest's --gtest_filter) that narrows `filter` to the
// test suites of the kind `performance` selects: with `performance`, the
// performance fixtures among `test_suite_names` (the names of every registered
// gtest test suite, see IsPerformanceFixture()), otherwise all the others.
// The suites of the other kind are appended to the negative patterns as
// `<test suite name>.*`, so `filter` (typically an explicit --gtest_filter, or
// "*") still applies within the selected kind.  This is how the runner
// (conformance_test_main.cc) implements --performance.
std::string PerformanceGtestFilter(
    absl::string_view filter, bool performance,
    absl::Span<const std::string> test_suite_names);

// Prints `error_message` (if non-empty) followed by the usage text to stderr.
void PrintConformanceRunnerUsage(absl::string_view error_message);

// One entry of a report written by ReportTestStatusSet().
struct ReportedTestStatus {
  // The full name of the test.
  std::string test_name;
  // The failure message shown next to the test: the test's own for a failure,
  // the failure list entry's for an unexpected success.
  std::string failure_message;
  // The failure list entry (possibly a wildcard) the test matched, when the
  // report is about removing that entry from the list.  Empty when it is about
  // adding `test_name` to the list.
  std::string matched_name;
};

// Reports one category of test statuses in the runner's report format.  Does
// nothing and returns true if `statuses` is empty.  Otherwise appends
// `message` followed by one "  <test name> # <failure message>" line per
// status to `output` and returns false; if `file_name` is non-empty, it also
// writes one "<name> # <failure message>" line per status to the file
// `output_dir` + `file_name`, in the form update_failure_list expects: <name>
// is the status's matched_name if set (an entry to remove from the failure
// list) and its test name otherwise (an entry to add).  A file that cannot be
// opened is reported in `output` too.
bool ReportTestStatusSet(absl::Span<const ReportedTestStatus> statuses,
                         absl::string_view file_name, absl::string_view message,
                         absl::string_view output_dir, std::string* output);

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_CONFORMANCE_TEST_RUNNER_H__
