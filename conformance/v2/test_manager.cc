#include "conformance/v2/test_manager.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"

namespace google {
namespace protobuf {
namespace {

constexpr int kMaximumWildcardExpansions = 10;

bool InsertUniqueTest(absl::flat_hash_set<std::string>& set,
                      absl::string_view value) {
  if (!set.emplace(value).second) {
    return false;
  }
  return true;
}

void IncrementIfUnique(bool unique, int& counter) {
  if (unique) {
    ++counter;
  }
}

void Normalize(std::string& input) {
  input.erase(std::remove(input.begin(), input.end(), '\n'), input.end());
}

// Sets up a failure message properly for our failure lists.
std::string FormatFailureMessage(absl::string_view input) {
  // Make copy just this once, as we need to modify it for our failure lists.
  std::string result(input);
  // Remove newlines
  Normalize(result);
  // Truncate failure message if needed
  if (result.length() > 128) {
    result = result.substr(0, 128);
  }
  return result;
}

}  // namespace

void TestManager::LoadFailureList(absl::string_view filename) {
  std::ifstream infile(std::string{filename});

  if (!infile.is_open()) {
    ABSL_LOG(WARNING) << "Couldn't open failure list file: " << filename;
    return;
  }

  for (std::string line; std::getline(infile, line);) {
    failure_list_lines_.push_back(line);

    // Remove comments.
    std::string test_name = line.substr(0, line.find('#'));

    test_name.erase(
        std::remove_if(test_name.begin(), test_name.end(), ::isspace),
        test_name.end());

    if (test_name.empty()) {  // Skip empty lines.
      continue;
    }

    // If we remove whitespace from the beginning of a line, and what we have
    // left at first is a '#', then we have a comment.
    if (test_name[0] != '#') {
      // Find our failure message if it exists. Will be set to an empty string
      // if no message is found. Empty failure messages also pass our tests.
      size_t check_message = line.find('#');
      std::string message;
      if (check_message != std::string::npos) {
        message = line.substr(check_message + 1);  // +1 to skip the delimiter
        // If we had only whitespace after the delimiter, we will have an empty
        // failure message and the test will still pass.
        message = std::string(absl::StripAsciiWhitespace(message));
      }
      ABSL_CHECK_OK(expected_failure_list_.Insert(test_name));
      ABSL_CHECK(expected_failure_messages_.emplace(test_name, message).second);
      ABSL_CHECK(unseen_expected_failures_.emplace(test_name).second);
    }
  }
}

void TestManager::SaveFailureList(absl::string_view filename) const {
  std::ofstream outfile(std::string{filename});
  auto to_add = new_failures_.begin();

  for (const std::string& line : failure_list_lines_) {
    absl::string_view test_name = absl::StripAsciiWhitespace(
        absl::string_view(line).substr(0, line.find('#')));
    if (unseen_expected_failures_.contains(test_name)) {
      continue;
    }
    while (to_add != new_failures_.end() && test_name > to_add->first) {
      // TODO: Handle alignment of failure messages.
      outfile << to_add->first << " #" << to_add->second << "\n";
      ++to_add;
    }
    outfile << line << "\n";
  }

  while (to_add != new_failures_.end()) {
    outfile << to_add->first << " # " << to_add->second << "\n";
    ++to_add;
  }
}

absl::Status TestManager::ReportSuccess(absl::string_view test_name) {
  bool unique = InsertUniqueTest(seen_tests_, test_name);
  auto failure_match = expected_failure_list_.WalkDownMatch(test_name);

  if (failure_match.has_value()) {
    // This was expected to fail, but it succeeded.
    IncrementIfUnique(unique, number_of_matches_[*failure_match]);
    IncrementIfUnique(unique, unexpected_successes_);
    unseen_expected_failures_.erase(*failure_match);
    return absl::FailedPreconditionError(
        absl::StrCat("Unexpected success for test: ", test_name));
  }

  // This wasn't expected to fail.
  IncrementIfUnique(unique, expected_successes_);
  return absl::OkStatus();
}

absl::Status TestManager::ReportFailure(absl::string_view test_name,
                                        absl::string_view failure_message) {
  bool unique = InsertUniqueTest(seen_tests_, test_name);
  auto failure_match = expected_failure_list_.WalkDownMatch(test_name);

  std::string formatted_failure_message = FormatFailureMessage(failure_message);

  if (!failure_match.has_value()) {
    // This was not expected to fail.
    IncrementIfUnique(unique, unexpected_failures_);
    new_failures_[test_name] = formatted_failure_message;
    return absl::FailedPreconditionError(
        absl::StrCat("Unexpected failure for test: ", test_name));
  }

  if (expected_failure_messages_[*failure_match] != formatted_failure_message) {
    new_failures_[*failure_match] = formatted_failure_message;
    return absl::FailedPreconditionError(
        absl::StrCat("Unexpected failure message for test: ", test_name,
                     " expected: ", expected_failure_messages_[*failure_match],
                     " actual: ", formatted_failure_message));
  }

  unseen_expected_failures_.erase(*failure_match);

  if (number_of_matches_[*failure_match] > kMaximumWildcardExpansions) {
    return absl::FailedPreconditionError(
        absl::StrCat("The wildcard ", *failure_match,
                     " served as matches to too many test "
                     "names exceeding the max amount of ",
                     kMaximumWildcardExpansions, " for test: ", test_name));
  }

  IncrementIfUnique(unique, number_of_matches_[*failure_match]);
  IncrementIfUnique(unique, expected_failures_);

  return absl::OkStatus();
}

absl::Status TestManager::ReportSkip(absl::string_view test_name) {
  bool unique = InsertUniqueTest(seen_tests_, test_name);
  IncrementIfUnique(unique, skipped_);
  return absl::OkStatus();
}

absl::Status TestManager::Finalize() const {
  if (!unseen_expected_failures_.empty()) {
    return absl::FailedPreconditionError(
        absl::StrCat("The following expected failures were not seen: ",
                     absl::StrJoin(unseen_expected_failures_, ", ")));
  }
  return absl::OkStatus();
}

}  // namespace protobuf
}  // namespace google
