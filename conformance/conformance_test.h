// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// This file defines a protocol for running the conformance test suite
// in-process.  In other words, the suite itself will run in the same process as
// the code under test.
//
// For pros and cons of this approach, please see conformance.proto.

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_CONFORMANCE_TEST_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_CONFORMANCE_TEST_H__

#include <iosfwd>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include "absl/base/nullability.h"
#include "absl/container/btree_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "absl/types/span.h"
#include "conformance/conformance.pb.h"
#include "failure_list_trie_node.h"
#include "test_runner.h"
#include "google/protobuf/descriptor.h"

namespace conformance {
class ConformanceRequest;
class ConformanceResponse;
}  // namespace conformance

namespace protobuf_test_messages {
namespace proto3 {
class TestAllTypesProto3;
}  // namespace proto3
}  // namespace protobuf_test_messages

namespace google {
namespace protobuf {

class ConformanceTestSuite;

// Runs `suites` with the conformance_test_runner command line (see
// PrintConformanceRunnerUsage()) and returns the process exit code.  This is
// the legacy entry point; it is equivalent to ParseConformanceRunnerArgs()
// followed by RunConformanceSuites().
int RunConformanceTests(int argc, char* argv[],
                        const std::vector<ConformanceTestSuite*>& suites);

// The options accepted on the conformance_test_runner command line.
//
// Transitional API for the gtest migration; will be removed once all suites
// are migrated.
struct ConformanceRunnerOptions {
  // The first non-flag argument and everything after it.
  std::string testee;
  std::vector<std::string> testee_args;

  // Every failure list flag given, as (flag name, file) pairs in command line
  // order, e.g. ("--failure_list", "failure_list_cpp.txt").  A suite loads the
  // files given for its GetFailureListFlagName().
  std::vector<std::pair<std::string, std::string>> failure_list_files;

  bool performance = false;
  bool debug = false;
  bool verbose = false;
  bool enforce_recommended = false;
  // EDITION_UNKNOWN (the default) runs the proto2 and proto3 tests only.
  Edition maximum_edition = EDITION_UNKNOWN;
  std::string output_dir;
  // Where --record_requests should write, if given.
  std::string record_requests_file;
  // The tests named with --test, and whether to run only those (set by
  // ParseConformanceRunnerArgs() when --test was given).  They are separate so
  // that a caller that has already run some of the names elsewhere can hand
  // over the remainder, even if none is left.
  absl::flat_hash_set<std::string> names_to_test;
  bool isolated = false;

  // Arguments starting with --gtest_ or --gunit_ that appeared before the
  // testee, in order and as given.  The legacy runner rejects them like any
  // other unknown option; the merged runner (conformance_test_main.cc) passes
  // them on to gtest, after RewriteGtestFlagPrefix().
  std::vector<std::string> gtest_args;

  // Returns the files given for `failure_list_flag`, in command line order.
  // A member rather than a free function because it is a pure query over
  // `failure_list_files` and reads best at its call sites as
  // options.FailureListFilesFor(...).
  std::vector<std::string> FailureListFilesFor(
      absl::string_view failure_list_flag) const;
};

// Parses the conformance_test_runner command line.  `suites` only supplies
// the accepted failure list flags (each suite's GetFailureListFlagName()).
// On error, returns InvalidArgumentError with the message the runner prints
// before its usage text; see PrintConformanceRunnerUsage().
//
// Transitional API for the gtest migration; will be removed once all suites
// are migrated.
absl::StatusOr<ConformanceRunnerOptions> ParseConformanceRunnerArgs(
    int argc, char* argv[], absl::Span<ConformanceTestSuite* const> suites);

// Returns `arg` with a leading "--gtest_" replaced by "--" + `flag_prefix`;
// any other argument, including one already spelled with `flag_prefix`, is
// returned unchanged.  The merged runner (conformance_test_main.cc) applies
// this to ConformanceRunnerOptions::gtest_args with the prefix its gtest was
// built with (GTEST_FLAG_PREFIX_ from gtest-port.h: "gtest_" in open source,
// "gunit_" in google3, whose gtest rejects the --gtest_ spelling), so that the
// documented --gtest_ spelling works everywhere.
//
// Transitional API for the gtest migration; will be removed once all suites
// are migrated.
std::string RewriteGtestFlagPrefix(absl::string_view arg,
                                   absl::string_view flag_prefix);

// Prints `error_message` (if non-empty) followed by the usage text to stderr.
//
// Transitional API for the gtest migration; will be removed once all suites
// are migrated.
void PrintConformanceRunnerUsage(absl::string_view error_message);

// Runs `suites` as conformance_test_runner does: each suite runs against its
// own ForkPipeRunner for `options.testee`, its report is written to stderr,
// and finally the tests requested with --test that no suite ran are listed.
//
// `record_requests`, if non-null, receives one line per request sent to the
// testee (see RecordingTestRunner).  `unmatched_candidates`, if non-null, is
// passed to ConformanceTestSuite::SetUnmatchedCandidates() on every suite.
//
// Returns true if every suite passed.  Exits the process if a failure list
// cannot be read.
//
// Transitional API for the gtest migration; will be removed once all suites
// are migrated.
bool RunConformanceSuites(
    const ConformanceRunnerOptions& options,
    absl::Span<ConformanceTestSuite* const> suites,
    std::ostream* absl_nullable record_requests,
    const absl::flat_hash_set<std::string>* absl_nullable unmatched_candidates);

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

// Reports one category of test statuses the way every legacy suite does at the
// end of its run.  Does nothing and returns true if `statuses` is empty.
// Otherwise appends `message` followed by one "  <test name> # <failure
// message>" line per status to `output` and returns false; if `file_name` is
// non-empty, it also writes one "<name> # <failure message>" line per status to
// the file `output_dir` + `file_name`, in the form update_failure_list expects:
// <name> is the status's matched_name if set (an entry to remove from the
// failure list) and its test name otherwise (an entry to add).  A file that
// cannot be opened is reported in `output` too.
//
// Transitional API for the gtest migration: the merged runner
// (conformance_test_main.cc) uses it to report the gtest suites' results in
// the same format; will be removed once all suites are migrated.
bool ReportTestStatusSet(absl::Span<const ReportedTestStatus> statuses,
                         absl::string_view file_name, absl::string_view message,
                         absl::string_view output_dir, std::string* output);

// Class representing the test suite itself.  To run it, implement your own
// class derived from ConformanceTestRunner, class derived from
// ConformanceTestSuite and then write code like:
//
//    class MyConformanceTestSuite : public ConformanceTestSuite {
//     public:
//      void RunSuiteImpl() {
//        // INSERT ACTUAL TESTS.
//      }
//    };
//
//    class MyConformanceTestRunner : public ConformanceTestRunner {
//     public:
//      static int Run(int argc, char *argv[],
//                 ConformanceTestSuite* suite);
//
//     private:
//      virtual void RunTest(...) {
//        // INSERT YOUR FRAMEWORK-SPECIFIC CODE HERE.
//      }
//    };
//
//    int main() {
//      MyConformanceTestSuite suite;
//      MyConformanceTestRunner::Run(argc, argv, &suite);
//    }
//
class ConformanceTestSuite {
 public:
  ConformanceTestSuite() = default;
  virtual ~ConformanceTestSuite() = default;

  void SetPerformance(bool performance) { performance_ = performance; }
  void SetVerbose(bool verbose) { verbose_ = verbose; }

  // Whether to require the testee to pass RECOMMENDED tests. By default failing
  // a RECOMMENDED test case will not fail the entire suite but will only
  // generated a warning. If this flag is set to true, RECOMMENDED tests will
  // be treated the same way as REQUIRED tests and failing a RECOMMENDED test
  // case will cause the entire test suite to fail as well. An implementation
  // can enable this if it wants to be strictly conforming to protobuf spec.
  // See the comments about ConformanceLevel below to learn more about the
  // difference between REQUIRED and RECOMMENDED test cases.
  void SetEnforceRecommended(bool value) { enforce_recommended_ = value; }

  // Sets the maximum edition (inclusive) that should be tests for conformance.
  void SetMaximumEdition(Edition edition) { maximum_edition_ = edition; }

  // Gets the flag name to the failure list file.
  // By default, this would return --failure_list
  std::string GetFailureListFlagName() { return failure_list_flag_name_; }

  void SetFailureListFlagName(const std::string& failure_list_flag_name) {
    failure_list_flag_name_ = failure_list_flag_name;
  }

  // Sets the path of the output directory.
  void SetOutputDir(const std::string& output_dir) { output_dir_ = output_dir; }

  // Sets if we are running the test in debug mode.
  void SetDebug(bool debug) { debug_ = debug; }

  // Sets if we are running ONLY the tests provided in the 'names_to_test_' set.
  void SetIsolated(bool isolated) { isolated_ = isolated; }

  // Sets the file path of the testee.
  void SetTestee(const std::string& testee) { testee_ = testee; }

  // Sets the names of tests to ONLY be run isolated from all the others.
  void SetNamesToTest(absl::flat_hash_set<std::string> names_to_test) {
    names_to_test_ = std::move(names_to_test);
  }

  absl::flat_hash_set<std::string> GetExpectedTestsNotRun() {
    return names_to_test_;
  }

  // Restricts which failure list entries the next RunSuite() may report as
  // unmatched (i.e. matching no test name in this suite): when set, such an
  // entry is only reported if it is also in `candidates`.  The merged runner
  // (conformance_test_main.cc) passes the entries that the gtest-based suites
  // running in the same process did not match either, so that an entry is only
  // flagged if neither side has a test for it.  Like the failure list, the
  // candidates are per run: RunSuite() consumes them, and a run without them
  // reports every unmatched entry (the default).
  void SetUnmatchedCandidates(absl::flat_hash_set<std::string> candidates) {
    unmatched_candidates_ = std::move(candidates);
  }

  // Run all the conformance tests against the given test runner.
  // Test output will be stored in "output".
  //
  // Returns true if the set of failing tests was exactly the same as the
  // failure list.
  // The filename here is *only* used to create/format useful error messages for
  // how to update the failure list.  We do NOT read this file at all.

  bool RunSuite(ConformanceTestRunner* runner, std::string* output,
                const std::string& filename,
                ::conformance::FailureSet* failure_list);

 protected:
  // Test cases are classified into a few categories:
  //   REQUIRED: the test case must be passed for an implementation to be
  //             interoperable with other implementations. For example, a
  //             parser implementation must accept both packed and unpacked
  //             form of repeated primitive fields.
  //   RECOMMENDED: the test case is not required for the implementation to
  //                be interoperable with other implementations, but is
  //                recommended for best performance and compatibility. For
  //                example, a proto3 serializer should serialize repeated
  //                primitive fields in packed form, but an implementation
  //                failing to do so will still be able to communicate with
  //                other implementations.
  enum ConformanceLevel {
    REQUIRED = 0,
    RECOMMENDED = 1,
  };

  class ConformanceRequestSetting {
   public:
    ConformanceRequestSetting(ConformanceLevel level,
                              ::conformance::WireFormat input_format,
                              ::conformance::WireFormat output_format,
                              ::conformance::TestCategory test_category,
                              const Message& prototype_message,
                              const std::string& test_name,
                              const std::string& input);
    virtual ~ConformanceRequestSetting() = default;

    std::unique_ptr<Message> NewTestMessage() const;

    std::string GetSyntaxIdentifier() const;

    std::string GetTestName() const;

    const ::conformance::ConformanceRequest& GetRequest() const {
      return request_;
    }

    ConformanceLevel GetLevel() const { return level_; }

    std::string ConformanceLevelToString(ConformanceLevel level) const;

    void SetPrintUnknownFields(bool print_unknown_fields) {
      request_.set_print_unknown_fields(true);
    }

    void SetPrototypeMessageForCompare(const Message& message) {
      prototype_message_for_compare_.reset(message.New());
    }

   protected:
    virtual std::string InputFormatString(
        ::conformance::WireFormat format) const;
    virtual std::string OutputFormatString(
        ::conformance::WireFormat format) const;
    ::conformance::ConformanceRequest request_;

   private:
    ConformanceLevel level_;
    ::conformance::WireFormat input_format_;
    ::conformance::WireFormat output_format_;
    const Message& prototype_message_;
    std::unique_ptr<Message> prototype_message_for_compare_;
    std::string test_name_;
  };

  std::string WireFormatToString(::conformance::WireFormat wire_format);

  // Parse payload in the response to the given message. Returns true on
  // success.
  virtual bool ParseResponse(const ::conformance::ConformanceResponse& response,
                             const ConformanceRequestSetting& setting,
                             Message* test_message) = 0;

  void VerifyResponse(const ConformanceRequestSetting& setting,
                      const std::string& equivalent_wire_format,
                      const ::conformance::ConformanceResponse& response,
                      bool need_report_success, bool require_same_wire_format);

  void TruncateDebugPayload(std::string* payload);
  ::conformance::ConformanceRequest TruncateRequest(
      const ::conformance::ConformanceRequest& request);
  ::conformance::ConformanceResponse TruncateResponse(
      const ::conformance::ConformanceResponse& response);

  void ReportSuccess(const ::conformance::TestStatus& test);
  void ReportFailure(::conformance::TestStatus& test, ConformanceLevel level,
                     const ::conformance::ConformanceRequest& request,
                     const ::conformance::ConformanceResponse& response);
  void ReportSkip(const ::conformance::TestStatus& test,
                  const ::conformance::ConformanceRequest& request,
                  const ::conformance::ConformanceResponse& response);

  void RunValidInputTest(const ConformanceRequestSetting& setting,
                         const std::string& equivalent_text_format);
  void RunValidBinaryInputTest(const ConformanceRequestSetting& setting,
                               const std::string& equivalent_wire_format,
                               bool require_same_wire_format = false);

  // Returns true if our runner_ ran the test and false if it did not.
  bool RunTest(const std::string& test_name,
               const ::conformance::ConformanceRequest& request,
               ::conformance::ConformanceResponse* response);

  // Will return false if an entry from the failure list was either a
  // duplicate of an already added one to the trie or it contained invalid
  // wildcards; otherwise, returns true.
  bool AddExpectedFailedTest(const ::conformance::TestStatus& failure);

  virtual void RunSuiteImpl() = 0;

  ConformanceTestRunner* runner_;
  FailureListTrieNode failure_list_root_;
  std::string testee_;
  int successes_;
  int expected_failures_;
  bool verbose_ = false;
  bool performance_ = false;
  bool enforce_recommended_ = false;
  Edition maximum_edition_ = Edition::EDITION_PROTO3;
  std::string output_;
  std::string output_dir_;
  std::string failure_list_flag_name_ = "--failure_list";
  std::string failure_list_filename_;
  absl::flat_hash_set<std::string> names_to_test_;
  bool debug_ = false;
  // If names were given for names_to_test_, only those tests
  // will be run and this bool will be set to true.
  bool isolated_ = false;

  // The set of test names (expanded from wildcard(s) and non-expanded) that are
  // expected to fail in this run, but haven't failed yet.
  absl::btree_map<std::string, ::conformance::TestStatus> expected_to_fail_;

  // The set of tests that failed because their failure message did not match
  // the actual failure message. These are failure messages that may need to be
  // removed from our failure lists.
  absl::btree_map<std::string, ::conformance::TestStatus>
      expected_failure_messages_;

  // The set of test names that have been run.  Used to ensure that there are no
  // duplicate names in the suite.
  absl::flat_hash_set<std::string> test_names_ran_;

  // The set of tests that failed, but weren't expected to: They weren't
  // present in our failure lists.
  absl::btree_map<std::string, ::conformance::TestStatus>
      unexpected_failing_tests_;

  // The set of tests that succeeded, but weren't expected to: They were present
  // in our failure lists, but managed to succeed.
  absl::btree_map<std::string, ::conformance::TestStatus>
      unexpected_succeeding_tests_;

  // The set of tests that failed because their failure message did not match
  // the actual failure message. These are failure messages that may need to be
  // added to our failure lists.
  absl::btree_map<std::string, ::conformance::TestStatus>
      unexpected_failure_messages_;

  // The set of test names (wildcarded or not) from the failure list that did
  // not match any actual test name.
  absl::btree_map<std::string, ::conformance::TestStatus> unmatched_;

  // If set, only entries of unmatched_ that are also in here are reported.
  // Taken (and cleared) by RunSuite(); see SetUnmatchedCandidates().
  absl::optional<absl::flat_hash_set<std::string>> unmatched_candidates_;

  // The set of tests that the testee opted out of;
  absl::btree_map<std::string, ::conformance::TestStatus> skipped_;

  // Allows us to remove from unmatched_.
  absl::btree_map<std::string, std::string> saved_failure_messages_;

  // If a failure list entry served as a match for more than 'max_matches_',
  // those will be added here for removal.
  absl::btree_map<std::string, ::conformance::TestStatus> exceeded_max_matches_;

  // Keeps track of how many tests matched to each failure list entry.
  absl::btree_map<std::string, int> number_of_matches_;
};

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_CONFORMANCE_TEST_H__
