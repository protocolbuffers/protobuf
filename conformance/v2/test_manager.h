#ifndef GOOGLE_PROTOBUF_CONFORMANCE_V2_TEST_MANAGER_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_V2_TEST_MANAGER_H__

#include <optional>
#include <string>
#include <vector>

#include "absl/container/btree_map.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "conformance/conformance.pb.h"
#include "conformance/failure_list_trie_node.h"

namespace google {
namespace protobuf {

class TestManager {
 public:
  TestManager() : expected_failure_list_("root") {}

  // TODO: These should probably return absl::Status.
  // TODO: These probably shouldn't handle file IO directly.
  void LoadFailureList(absl::string_view filename);
  void SaveFailureList(absl::string_view filename) const;

  absl::Status ReportSuccess(absl::string_view test_name);
  absl::Status ReportFailure(absl::string_view test_name,
                             absl::string_view failure_message);
  absl::Status ReportSkip(absl::string_view test_name);

  absl::Status Finalize() const;

  int skipped() const { return skipped_; }
  int expected_failures() const { return expected_failures_; }
  int unexpected_failures() const { return unexpected_failures_; }
  int expected_successes() const { return expected_successes_; }
  int unexpected_successes() const { return unexpected_successes_; }

 private:
  FailureListTrieNode expected_failure_list_;
  absl::flat_hash_map<std::string, std::string> expected_failure_messages_;

  absl::flat_hash_set<std::string> unseen_expected_failures_;
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
};

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_V2_TEST_MANAGER_H__
