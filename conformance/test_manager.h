#ifndef GOOGLE_PROTOBUF_CONFORMANCE_TEST_MANAGER_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_TEST_MANAGER_H__

#include <string>
#include <vector>

#include "absl/container/btree_map.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "conformance/conformance.pb.h"
#include "failure_list_trie_node.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {

// This class is used to manage the expected failures and actual results of
// a test suite.  Our matchers should all report their results into here so that
// results get properly tracked.  The results get reported to googletest via
// custom properties and the populate the new failure list when the --fix flag
// is provided.
class TestManager {
 public:
  TestManager() : expected_failure_list_("root") {}
  ~TestManager();

  // Loads a list of expected failures from disk.
  absl::Status LoadFailureList(absl::string_view filename);

  // Saves an updated list of failures to disk based on the reported results.
  absl::Status SaveFailureList(absl::string_view filename) const;

  // Reports a successful test run.  This will return an error if the test was
  // expected to fail.
  absl::Status ReportSuccess(absl::string_view test_name);

  // Reports a failed test run along with the failure message.  This will return
  // an error if the test wasn't expected to fail, or if it was expected to fail
  // with a different message.
  absl::Status ReportFailure(absl::string_view test_name,
                             absl::string_view failure_message);

  // Reports a test that was skipped.
  absl::Status ReportSkip(absl::string_view test_name);

  // Runs sanity checks over the failure list to make sure everything we
  // expected to run was reported.  Must be called before destruction.
  absl::Status Finalize();

  // The number of tests that were reported skipped.
  int skipped() const { return skipped_; }

  // The number of tests that were expected to fail and did.
  int expected_failures() const { return expected_failures_; }

  // The number of tests that were expected to fail and didn't.
  int unexpected_failures() const { return unexpected_failures_; }

  // The number of tests that were expected to succeed and did.
  int expected_successes() const { return expected_successes_; }

  // The number of tests that were expected to succeed and didn't.
  int unexpected_successes() const { return unexpected_successes_; }

 private:
  FailureListTrieNode expected_failure_list_;
  absl::flat_hash_map<std::string, std::string> expected_failure_messages_;

  absl::flat_hash_set<std::string> unseen_expected_failures_;
  absl::flat_hash_set<std::string> seen_unexpected_successes_;
  absl::flat_hash_map<std::string, int> number_of_matches_;

  // We have to track which tests we've already seen, because gtest will call
  // the matcher twice on failure.
  absl::flat_hash_set<std::string> seen_tests_;

  std::vector<std::string> failure_list_lines_;
  absl::btree_map<std::string, std::string> new_failures_;

  int skipped_ = 0;
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

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_TEST_MANAGER_H__
