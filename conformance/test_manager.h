#ifndef GOOGLE_PROTOBUF_CONFORMANCE_TEST_MANAGER_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_TEST_MANAGER_H__

#include <string>
#include <utility>
#include <vector>

#include "absl/container/btree_map.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "conformance/conformance.pb.h"
#include "conformance/failure_list_trie_node.h"
#include "conformance/testee.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {

// A test whose result contradicted the failure list.  See
// TestManager::UnexpectedFailures() and TestManager::UnexpectedSuccesses().
// TODO: b/563707827 - Remove with conformance_test_runner.
struct UnexpectedResult {
  // The full name of the test, as reported to the TestManager.
  std::string test_name;
  // For an unexpected failure, the test's failure message as
  // TestManager::SaveFailureList() would write it.  For an unexpected success,
  // the message of the failure list entry that matched the test.
  std::string failure_message;
  // For an unexpected success, the failure list entry that matched
  // `test_name`, possibly a wildcard.  Unset for an unexpected failure, which
  // matched no entry or matched one with a different message.
  absl::optional<std::string> matched_entry;
};

// Tracks the expected failures and the actual results of a test suite.  The
// conformance matchers (see matchers.h) report the outcome of every test
// here.  The test environment reads the results back and reports them as test
// properties.  Under --fix the results also populate the new failure list.
class TestManager {
 public:
  TestManager() : expected_failure_list_("root") {}
  ~TestManager();

  // The highest priority level whose failures fail the suite, 0 for kP0 or 1
  // for kP1 (see TestPriority in testee.h).  ReportFailure() tolerates a
  // failing test above this level unless it is in the failure list.  Defaults
  // to kEnforceAllPriorities.
  void set_enforcement_level(int level) { enforcement_level_ = level; }

  // Loads a failure list from disk and adds its entries to the ones loaded so
  // far.  Each line that isn't blank or a comment names one expected failure,
  // optionally followed by `#` and the expected failure message.  Returns an
  // error if the file can't be opened.  Also returns an error if an entry is
  // already present, including one from an earlier file, or contains an
  // invalid wildcard.
  absl::Status LoadFailureList(absl::string_view filename);

  // Returns the failure message of the failure list entry `entry`, normalized
  // the way ReportFailure() normalizes messages.  `entry` is an entry exactly
  // as listed, wildcards included, not a test name to match.  Returns nullopt
  // if there is no such entry.  Records nothing.
  // TODO: b/563707827 - Remove with conformance_test_runner.
  absl::optional<std::string> ExpectedFailureMessage(
      absl::string_view entry) const;

  // Saves an updated failure list to disk based on the reported results.
  // Comment and blank lines stay where they are.  Entries are updated as
  // follows:
  //
  //   - An entry that no test matched with a verdict is dropped.  These are
  //     the entries Finalize() reports as unseen.
  //   - An entry whose matched tests all still fail with its message, or were
  //     skipped (see ReportSkip()), is kept verbatim, wildcards included.
  //   - An entry whose matched tests all failed with the same different
  //     message is kept with that message instead, wildcards included.
  //   - Any other entry is rewritten in place, one line per matched test in
  //     name order.  A test that succeeded is left out.  A test that failed
  //     with a different message gets that message.  A test that still fails
  //     with the entry's message, or was skipped, keeps it.  An exact entry
  //     therefore goes when its test succeeded.  A wildcard entry is expanded
  //     into the tests it matched, so that the tests that still fail stay
  //     listed.
  //   - Tests that failed but matched no entry are inserted in name order
  //     with their messages.
  //
  // Entry lines are aligned to the longest name.
  //
  // A copybara strip block (`# copybara` `:strip_begin` ... `:strip_end`) is
  // the region the open source export drops.  It is kept as one contiguous
  // region and aligned on its own.  Its entries are handled as above and stay
  // inside it.  No new failure is ever inserted into it.  A new failure goes
  // in front of the first entry outside any block that sorts after it, or at
  // the end of the file.  One that sorts between the last entry before a
  // block and the first entry after it lands right after the block's
  // strip_end.  Unbalanced markers are ordinary comments, as they are to
  // LoadFailureList().
  //
  // Returns an error if `filename` can't be opened for writing or the write
  // fails.
  absl::Status SaveFailureList(absl::string_view filename) const;

  // Reports a successful test run.  This will return an error if the test was
  // expected to fail.
  absl::Status ReportSuccess(absl::string_view test_name);

  // Reports a failed test run along with the failure message.  Returns OK if
  // the failure doesn't fail the suite.  That is the case when the test is in
  // the failure list with a matching message (an expected failure, logged at
  // INFO), or when it isn't listed and `priority` is above the enforcement
  // level (a tolerated failure, logged as a WARNING and only counted by
  // tolerated_failures()).  Otherwise the failure is unexpected and the error
  // says why.  A listed test is checked whatever its priority, so that the
  // failure list can't go stale unnoticed.  The normalized actual message
  // only needs to start with the expected message, so an empty expected
  // message matches any failure.
  absl::Status ReportFailure(absl::string_view test_name, TestPriority priority,
                             absl::string_view failure_message);

  // Reports a test that the testee skipped for `skip_reason`.  A skip is not
  // a verdict on the test's failure list entry, if any.  The entry counts as
  // seen and matched, so that Finalize() doesn't report it and
  // SaveFailureList() keeps it, but the skip is not an expected failure.  A
  // listed test that is skipped is recorded (see ListedSkips()) and returns an
  // error that names the matched entry, like an unexpected success.  Whether
  // that error fails the test is the caller's policy.  The matchers fail such
  // a test.
  absl::Status ReportSkip(absl::string_view test_name,
                          absl::string_view skip_reason);

  // Reports a test the runner didn't run because it wasn't selected.  Its
  // response was skipped with kTestNotSelectedSkipReason (see test_runner.h).
  // The original conformance_test_runner matches a test name against the
  // failure list before checking whether the test was selected.  Like it,
  // this only marks the entry the name matches, if any, as matched for
  // UnmatchedExpectedFailures().  The test is not counted by any statistic
  // and its entry stays unseen.
  // TODO: b/563707827 - Remove with conformance_test_runner.
  void ReportNotSelected(absl::string_view test_name);

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
  // fail, but with a different message.  SaveFailureList() rewrites their
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
  // entry is kept, also by SaveFailureList() under --fix, so removing the
  // entry is up to the user.  It is kept as is, or as the test's own line
  // when a wildcard entry is expanded.
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

  // The verdict on a test that matched a failure list entry, as far as
  // SaveFailureList() is concerned.
  struct MatchOutcome {
    enum Kind {
      kExpectedFailure,  // Failed with the entry's message.
      kOtherFailure,     // Failed with a different message (see `message`).
      kSuccess,          // Succeeded (see ReportSuccess()).
      kSkip,             // Was skipped by the testee (see ReportSkip()).
    };
    Kind kind;
    // The formatted failure message, for kOtherFailure.
    std::string message;
  };

  // Appends to `lines` what SaveFailureList() writes for `line`, a line of the
  // failure list as loaded.  That is the line itself, nothing, or the entry's
  // rewritten lines, possibly expanded.  See SaveFailureList().
  void AppendSavedLines(absl::string_view line,
                        std::vector<std::string>& lines) const;

  FailureListTrieNode expected_failure_list_;
  absl::flat_hash_map<std::string, std::string> expected_failure_messages_;

  absl::flat_hash_set<std::string> unseen_expected_failures_;
  // Entries never matched by name by any Report*() call.
  absl::flat_hash_set<std::string> unmatched_expected_failures_;
  // The tests that matched each entry with a verdict, by test name, and how
  // they came out.  Entries are as listed, wildcards included.
  // ReportNotSelected() is not a verdict and doesn't record anything here.
  absl::flat_hash_map<std::string, absl::btree_map<std::string, MatchOutcome>>
      entry_matches_;
  absl::flat_hash_map<std::string, int> number_of_matches_;

  // Every test name reported so far, so that a test reported more than once
  // is only counted once.  The matchers prevent that, but nothing else does.
  absl::flat_hash_set<std::string> seen_tests_;

  // The tests counted by unexpected_failures_, mapped to their formatted
  // failure message, and the tests counted by unexpected_successes_, mapped to
  // the failure list entry they matched.
  absl::btree_map<std::string, std::string> unexpected_failure_messages_;
  absl::btree_map<std::string, std::string> unexpected_success_matches_;
  // The tests counted by listed_skips_, mapped to the entry they matched.
  absl::btree_map<std::string, std::string> listed_skip_matches_;

  std::vector<std::string> failure_list_lines_;
  // The tests that failed but matched no entry, with their formatted messages.
  absl::btree_map<std::string, std::string> new_failures_;

  int skipped_ = 0;
  int listed_skips_ = 0;
  int tolerated_failures_ = 0;
  int expected_failures_ = 0;
  int unexpected_failures_ = 0;
  int expected_successes_ = 0;
  int unexpected_successes_ = 0;
  bool finalized_ = false;
  int enforcement_level_ = kEnforceAllPriorities;
};

}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_TEST_MANAGER_H__
