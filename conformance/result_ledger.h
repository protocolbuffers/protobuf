#ifndef GOOGLE_PROTOBUF_CONFORMANCE_RESULT_LEDGER_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_RESULT_LEDGER_H__

#include <string>
#include <utility>
#include <vector>

#include "absl/container/btree_map.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "conformance/failure_list.h"
#include "conformance/result_record.h"
#include "conformance/testee.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {

// A test whose result contradicted the failure list.  See
// ResultLedger::UnexpectedFailures() and ResultLedger::UnexpectedSuccesses().
// TODO: b/563707827 - Remove with conformance_test_runner.
struct UnexpectedResult {
  // The full name of the test, as reported to the ResultLedger.
  std::string test_name;
  // For an unexpected failure, the test's failure message as
  // ResultLedger::SaveFailureList() would write it.  For an unexpected success,
  // the message of the failure list entry that matched the test.
  std::string failure_message;
  // For an unexpected success, the failure list entry that matched
  // `test_name`, possibly a wildcard.  Unset for an unexpected failure, which
  // matched no entry or matched one with a different message.
  absl::optional<std::string> matched_entry;
};

// How many tests one wildcard entry may cover with a verdict, that is an
// expected failure or an unexpected success.  See OverexpandedWildcards().
inline constexpr int kMaximumWildcardExpansions = 20;

// The ledger of a conformance run: tallies the results of its tests against
// its failure list.  It knows nothing of gtest; the test environment's
// ResultListener (result_listener.h) feeds it.  The failure list is loaded
// once (see LoadFailureList()) into a FailureList, which decides each result:
// Yields() (matchers.h) asks it for the verdict and records the outcome, and
// the listener reports the outcome here (see Report()) and reads the counters
// back as test properties.  The environment owns the ledger and, under --fix,
// writes the new failure list from it (see SaveFailureList()).
class ResultLedger {
 public:
  ResultLedger() = default;
  ~ResultLedger();

  // The lowest priority whose failures fail the suite (see TestPriority in
  // testee.h).  ReportFailure() tolerates a failing test of a lower priority
  // unless it is in the failure list.  Defaults to kLowestPriority, which
  // enforces every priority.
  void set_enforcement_level(TestPriority level) {
    failure_list_.set_enforcement_level(level);
  }

  // Loads a failure list from disk and adds its entries to the ones loaded so
  // far.  Each line that isn't blank or a comment names one expected failure,
  // optionally followed by `#` and the expected failure message.  Returns an
  // error if the file can't be opened.  Also returns an error if an entry is
  // already present, including one from an earlier file, or contains an
  // invalid wildcard.
  absl::Status LoadFailureList(absl::string_view filename);

  // The failure list as loaded so far, which decides each result.  Yields()
  // reads the global environment's through GetGlobalFailureList().
  const FailureList& failure_list() const { return failure_list_; }

  // Saves an updated failure list to disk based on the reported results.
  // Returns an error if `filename` can't be opened for writing or the write
  // fails.
  absl::Status SaveFailureList(absl::string_view filename) const;

  // Records a successful test run and returns
  // FailureList::VerdictOnSuccess().
  absl::Status ReportSuccess(absl::string_view test_name);

  // Records a failed test run along with the failure message and returns
  // FailureList::VerdictOnFailure().  A tolerated failure is only counted
  // (see tolerated_failures()).  An unexpected one is also kept for
  // UnexpectedFailures() and SaveFailureList().
  absl::Status ReportFailure(absl::string_view test_name, TestPriority priority,
                             absl::string_view failure_message);

  // Records a test that the testee skipped for `skip_reason` and returns
  // FailureList::VerdictOnSkip().  A skip is not a verdict on the test's
  // entry, if any.  The entry counts as seen and matched, so that Finalize()
  // doesn't report it and SaveFailureList() keeps it, but the skip is not an
  // expected failure.  A listed test that is skipped is also recorded for
  // ListedSkips().
  absl::Status ReportSkip(absl::string_view test_name,
                          absl::string_view skip_reason);

  // Reports a test the runner didn't run because it wasn't selected.
  // The original conformance_test_runner matches a test name against the
  // failure list before checking whether the test was selected.  Like it,
  // this only marks the entry the name matches, if any, as matched for
  // UnmatchedExpectedFailures().  The test is not counted by any statistic
  // and its entry stays unseen.
  // TODO: b/563707827 - Remove with conformance_test_runner.
  void ReportNotSelected(absl::string_view test_name);

  // Records `record`, the outcome Yields() recorded for `test_name`, through
  // the Report*() method for its status.  The verdict was applied when the
  // result was checked, so it is dropped here.
  void Report(absl::string_view test_name, const ResultRecord& record);

  // Whether any of the Report*() methods has been called for `test_name`.
  // The ResultListener checks this before it reports a recorded outcome (see
  // Report()), so that a test is counted once.
  bool WasReported(absl::string_view test_name) const {
    return seen_tests_.contains(test_name);
  }

  // The sorted failure list entries that more than kMaximumWildcardExpansions
  // reported tests matched with a verdict, that is as an expected failure or
  // an unexpected success.  Skips don't count.  Such a wildcard hides too
  // much; the test environment fails the run over it.
  std::vector<std::string> OverexpandedWildcards() const;

  // Runs sanity checks over the failure list to make sure everything we
  // expected to run was reported.  Returns an error naming the sorted expected
  // failure entries that have not been reported as a failure, an unexpected
  // success or a skip.  Must be called before destruction.
  absl::Status Finalize();

  // Returns the sorted expected failure entries that no reported test name has
  // matched so far, regardless of the outcome.  Names are reported through
  // ReportSuccess(), ReportFailure(), ReportSkip() or ReportNotSelected().  A
  // wildcard entry counts as matched once it has matched any test name.
  // Unlike the unseen entries Finalize() reports, an entry whose test was not
  // selected is not returned.  It is a real test, only not run here.
  // TODO: b/563707827 - Remove with conformance_test_runner.
  std::vector<std::string> UnmatchedExpectedFailures() const;

  // Returns the tests reported as unexpected failures so far, sorted by test
  // name.  These are the tests unexpected_failures() counts.  Each result's
  // `failure_message` is the message as SaveFailureList() would write it, and
  // its `matched_entry` is unset.  This includes tests that were expected to
  // fail, but with a different message.  SaveFailureList() replaces their
  // entry, so adding these lines to the failure list would duplicate it.
  // TODO: b/563707827 - Remove with conformance_test_runner.
  std::vector<UnexpectedResult> UnexpectedFailures() const;

  // Returns the tests reported as unexpected successes so far, sorted by test
  // name.  These are the tests unexpected_successes() counts.  Each result's
  // `matched_entry` is the failure list entry the test matched, and
  // `failure_message` is that entry's message.
  // TODO: b/563707827 - Remove with conformance_test_runner.
  std::vector<UnexpectedResult> UnexpectedSuccesses() const;

  // Returns the tests the testee skipped although they are in the failure
  // list, sorted by test name.  These are the tests listed_skips() counts.
  // Each pair is the test name and the failure list entry it matched.  Such a
  // test fails the gtest run: the matchers fail it with the error
  // ReportSkip() returns.  It is not an unexpected failure or success.  Its
  // entry is kept as is, also by SaveFailureList() under --fix, so removing
  // the entry is up to the user.
  // TODO: b/563707827 - Remove with conformance_test_runner.
  std::vector<std::pair<std::string, std::string>> ListedSkips() const;

  // The number of tests that were reported skipped.  This includes the listed
  // skips.
  int skipped() const { return skipped_; }

  // The number of skipped tests that are in the failure list.  See
  // ListedSkips().
  int listed_skips() const { return listed_skips_; }

  // The number of tests that failed but were tolerated because their priority
  // is above the enforcement level.
  int tolerated_failures() const { return tolerated_failures_; }

  // The number of tests that were in the failure list and failed with the
  // expected message.
  int expected_failures() const { return expected_failures_; }

  // The number of tests that failed but were not in the failure list, or
  // failed with a different message than the one listed.
  int unexpected_failures() const { return unexpected_failures_; }

  // The number of tests that were not in the failure list and succeeded.
  int expected_successes() const { return expected_successes_; }

  // The number of tests that were in the failure list but succeeded.
  int unexpected_successes() const { return unexpected_successes_; }

 private:
  // Adds one failure list entry, as LoadFailureList() does for each line.
  // Returns an error if the test name is already present or contains an
  // invalid wildcard.
  absl::Status AddExpectedFailure(absl::string_view test_name,
                                  absl::string_view failure_message);

  // The sorted expected failure entries Finalize() complains about: those not
  // reported as a failure, an unexpected success or a skip so far.
  std::vector<std::string> UnseenExpectedFailures() const;

  // Marks the failure list entry `test_name` matches, if any, as matched (see
  // UnmatchedExpectedFailures()) and returns it.
  absl::optional<std::string> MarkMatched(absl::string_view test_name);

  FailureList failure_list_;

  absl::flat_hash_set<std::string> unseen_expected_failures_;
  // Entries never matched by name by any Report*() call.
  absl::flat_hash_set<std::string> unmatched_expected_failures_;
  absl::flat_hash_set<std::string> seen_unexpected_successes_;
  // How many tests matched each entry as an expected failure or an unexpected
  // success.  See OverexpandedWildcards().
  absl::flat_hash_map<std::string, int> number_of_matches_;

  // Every test name reported so far (see WasReported()), so that a test
  // reported more than once is only counted once.
  absl::flat_hash_set<std::string> seen_tests_;

  // The tests counted by unexpected_failures_, mapped to their formatted
  // failure message, and the tests counted by unexpected_successes_, mapped to
  // the failure list entry they matched.
  absl::btree_map<std::string, std::string> unexpected_failure_messages_;
  absl::btree_map<std::string, std::string> unexpected_success_matches_;
  // The tests counted by listed_skips_, mapped to the entry they matched.
  absl::btree_map<std::string, std::string> listed_skip_matches_;

  std::vector<std::string> failure_list_lines_;
  absl::btree_map<std::string, std::string> new_failures_;

  int skipped_ = 0;
  int listed_skips_ = 0;
  int tolerated_failures_ = 0;
  int expected_failures_ = 0;
  int unexpected_failures_ = 0;
  int expected_successes_ = 0;
  int unexpected_successes_ = 0;
  bool finalized_ = false;
};

}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_RESULT_LEDGER_H__
