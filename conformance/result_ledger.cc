#include "conformance/result_ledger.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <fstream>
#include <ios>
#include <string>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "conformance/failure_list.h"
#include "conformance/result_record.h"
#include "conformance/testee.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {
namespace {

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

std::string ReformatLine(size_t alignment, absl::string_view line) {
  size_t comment_pos = line.find('#');
  absl::string_view test_name =
      absl::StripAsciiWhitespace(line.substr(0, comment_pos));
  if (test_name.empty()) {
    return std::string(absl::StripAsciiWhitespace(line));
  }
  absl::string_view message =
      absl::StripAsciiWhitespace(line.substr(comment_pos + 1));
  ABSL_CHECK_GE(alignment, test_name.length());
  std::string whitespace(alignment - test_name.length(), ' ');
  return absl::StrCat(test_name, whitespace, " # ", message);
}

}  // namespace

ResultLedger::~ResultLedger() {
  if (!finalized_) {
    ABSL_LOG(FATAL)
        << "ResultLedger::Finalize() was not called before destruction.";
  }
}

absl::Status ResultLedger::LoadFailureList(absl::string_view filename) {
  std::ifstream infile(std::string{filename});

  if (!infile.is_open()) {
    return absl::InternalError(
        absl::StrCat("Couldn't open failure list file: ", filename));
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
      size_t comment_pos = line.find('#');
      std::string message;
      if (comment_pos != std::string::npos) {
        message = line.substr(comment_pos + 1);  // +1 to skip the delimiter
        // If we had only whitespace after the delimiter, we will have an empty
        // failure message and the test will still pass.
        message = std::string(absl::StripAsciiWhitespace(message));
      }
      if (absl::Status status = AddExpectedFailure(test_name, message);
          !status.ok()) {
        return status;
      }
    }
  }

  return absl::OkStatus();
}

absl::Status ResultLedger::AddExpectedFailure(
    absl::string_view test_name, absl::string_view failure_message) {
  if (absl::Status status = failure_list_.Add(test_name, failure_message);
      !status.ok()) {
    return status;
  }
  ABSL_CHECK(unseen_expected_failures_.emplace(test_name).second);
  ABSL_CHECK(unmatched_expected_failures_.emplace(test_name).second);
  return absl::OkStatus();
}

absl::Status ResultLedger::SaveFailureList(absl::string_view filename) const {
  std::ofstream outfile(std::string{filename}, std::ios::binary);
  if (!outfile.is_open()) {
    return absl::InternalError(absl::StrCat(
        "Couldn't open failure list file for writing: ", filename));
  }

  // Calculate alignment.
  size_t alignment = 0;
  for (const std::string& line : failure_list_lines_) {
    absl::string_view test_name = absl::StripAsciiWhitespace(
        absl::string_view(line).substr(0, line.find('#')));
    alignment = std::max(alignment, test_name.length());
  }
  for (const auto& failure : new_failures_) {
    alignment = std::max(alignment, failure.first.length());
  }

  // Output existing failure list, stripping out unseen tests and inserting
  // new failures.
  auto to_add = new_failures_.begin();
  for (const std::string& line : failure_list_lines_) {
    absl::string_view test_name = absl::StripAsciiWhitespace(
        absl::string_view(line).substr(0, line.find('#')));
    if (unseen_expected_failures_.contains(test_name) ||
        seen_unexpected_successes_.contains(test_name)) {
      continue;
    }
    while (to_add != new_failures_.end() && test_name > to_add->first) {
      outfile << ReformatLine(alignment, absl::StrCat(to_add->first, " # ",
                                                      to_add->second))
              << "\n";
      ++to_add;
    }
    outfile << ReformatLine(alignment, line) << "\n";
  }

  // Add any remaining new failures.
  while (to_add != new_failures_.end()) {
    outfile << ReformatLine(alignment,
                            absl::StrCat(to_add->first, " # ", to_add->second))
            << "\n";
    ++to_add;
  }

  outfile.close();
  if (!outfile.good()) {
    return absl::InternalError(
        absl::StrCat("Failed to write failure list file: ", filename));
  }
  return absl::OkStatus();
}

absl::Status ResultLedger::ReportSuccess(absl::string_view test_name) {
  bool unique = InsertUniqueTest(seen_tests_, test_name);
  absl::optional<std::string> failure_match = MarkMatched(test_name);

  if (failure_match.has_value()) {
    // This was expected to fail, but it succeeded.
    IncrementIfUnique(unique, number_of_matches_[*failure_match]);
    IncrementIfUnique(unique, unexpected_successes_);
    if (unique) {
      unexpected_success_matches_[test_name] = *failure_match;
    }
    unseen_expected_failures_.erase(*failure_match);
    seen_unexpected_successes_.insert(*failure_match);
  } else {
    // This wasn't expected to fail.
    IncrementIfUnique(unique, expected_successes_);
  }
  return failure_list_.VerdictOnSuccess(test_name);
}

absl::Status ResultLedger::ReportFailure(absl::string_view test_name,
                                         TestPriority priority,
                                         absl::string_view failure_message) {
  bool unique = InsertUniqueTest(seen_tests_, test_name);
  absl::optional<std::string> failure_match = MarkMatched(test_name);

  std::string formatted_failure_message = FormatFailureMessage(failure_message);
  // Counts the failure as unexpected and records it for UnexpectedFailures().
  auto record_unexpected_failure = [&] {
    IncrementIfUnique(unique, unexpected_failures_);
    if (unique) {
      unexpected_failure_messages_[test_name] = formatted_failure_message;
    }
  };

  if (!failure_match.has_value()) {
    if (priority > failure_list_.enforcement_level()) {
      // Tolerated: neither a failure nor a skip, and not written to the
      // failure list; only counted.
      IncrementIfUnique(unique, tolerated_failures_);
    } else {
      // This was not expected to fail.
      record_unexpected_failure();
      new_failures_[test_name] = formatted_failure_message;
    }
    return failure_list_.VerdictOnFailure(test_name, priority, failure_message);
  }

  // Mirror the legacy runner, which only requires the actual failure message to
  // start with the expected one.
  if (!absl::StartsWith(formatted_failure_message,
                        *failure_list_.ExpectedMessage(*failure_match))) {
    record_unexpected_failure();
    // TODO: b/563659620 - Keying the replacement by the (possibly wildcard)
    // entry duplicates the line if another test later matches the entry with
    // the expected message: with `foo.*.bar # abc` listed,
    // ReportFailure("foo.a.bar", "xyz") + ReportFailure("foo.b.bar", "abc")
    // makes SaveFailureList() keep the original line and add a second
    // `foo.*.bar` line, which LoadFailureList() then rejects.
    new_failures_[*failure_match] = formatted_failure_message;
    return failure_list_.VerdictOnFailure(test_name, priority, failure_message);
  }

  unseen_expected_failures_.erase(*failure_match);
  IncrementIfUnique(unique, number_of_matches_[*failure_match]);
  IncrementIfUnique(unique, expected_failures_);
  return failure_list_.VerdictOnFailure(test_name, priority, failure_message);
}

absl::Status ResultLedger::ReportSkip(absl::string_view test_name,
                                      absl::string_view skip_reason) {
  bool unique = InsertUniqueTest(seen_tests_, test_name);
  IncrementIfUnique(unique, skipped_);
  absl::optional<std::string> failure_match = MarkMatched(test_name);
  if (!failure_match.has_value()) {
    return absl::OkStatus();
  }

  // A skip says nothing about whether the entry is still needed, so it is
  // neither reported as unseen by Finalize() nor dropped by SaveFailureList().
  // Nor does it count toward the entry's wildcard expansion cap
  // (number_of_matches_), unlike in the legacy runner, which counted every
  // test a wildcard matched: the cap is about real outcomes hidden by a
  // wildcard, and a skip isn't one.
  unseen_expected_failures_.erase(*failure_match);
  IncrementIfUnique(unique, listed_skips_);
  if (unique) {
    listed_skip_matches_[test_name] = *failure_match;
  }
  return failure_list_.VerdictOnSkip(test_name, skip_reason);
}

void ResultLedger::ReportNotSelected(absl::string_view test_name) {
  seen_tests_.emplace(test_name);
  MarkMatched(test_name);
}

void ResultLedger::Report(absl::string_view test_name,
                          const ResultRecord& record) {
  switch (record.status) {
    case ResultRecord::Status::kPass:
      ReportSuccess(test_name).IgnoreError();
      break;
    case ResultRecord::Status::kFail:
    case ResultRecord::Status::kCrash:
      ReportFailure(test_name, record.priority, record.message).IgnoreError();
      break;
    case ResultRecord::Status::kSkip:
      ReportSkip(test_name, record.message).IgnoreError();
      break;
  }
}

absl::optional<std::string> ResultLedger::MarkMatched(
    absl::string_view test_name) {
  absl::optional<std::string> failure_match =
      failure_list_.MatchingEntry(test_name);
  if (failure_match.has_value()) {
    unmatched_expected_failures_.erase(*failure_match);
  }
  return failure_match;
}

std::vector<std::string> ResultLedger::OverexpandedWildcards() const {
  std::vector<std::string> entries;
  for (const auto& [entry, matches] : number_of_matches_) {
    if (matches > kMaximumWildcardExpansions) entries.push_back(entry);
  }
  absl::c_sort(entries);
  return entries;
}

std::vector<std::string> ResultLedger::UnseenExpectedFailures() const {
  std::vector<std::string> unseen(unseen_expected_failures_.begin(),
                                  unseen_expected_failures_.end());
  absl::c_sort(unseen);
  return unseen;
}

std::vector<std::string> ResultLedger::UnmatchedExpectedFailures() const {
  std::vector<std::string> unmatched(unmatched_expected_failures_.begin(),
                                     unmatched_expected_failures_.end());
  absl::c_sort(unmatched);
  return unmatched;
}

std::vector<UnexpectedResult> ResultLedger::UnexpectedFailures() const {
  std::vector<UnexpectedResult> failures;
  failures.reserve(unexpected_failure_messages_.size());
  // The map is ordered by test name.
  for (const auto& [test_name, failure_message] :
       unexpected_failure_messages_) {
    failures.push_back(
        {/*test_name=*/test_name, /*failure_message=*/failure_message});
  }
  return failures;
}

std::vector<UnexpectedResult> ResultLedger::UnexpectedSuccesses() const {
  std::vector<UnexpectedResult> successes;
  successes.reserve(unexpected_success_matches_.size());
  // The map is ordered by test name.
  for (const auto& [test_name, matched_entry] : unexpected_success_matches_) {
    successes.push_back(
        {/*test_name=*/test_name,
         /*failure_message=*/*failure_list_.ExpectedMessage(matched_entry),
         /*matched_entry=*/matched_entry});
  }
  return successes;
}

std::vector<std::pair<std::string, std::string>> ResultLedger::ListedSkips()
    const {
  // The map is ordered by test name.
  return std::vector<std::pair<std::string, std::string>>(
      listed_skip_matches_.begin(), listed_skip_matches_.end());
}

absl::Status ResultLedger::Finalize() {
  finalized_ = true;
  std::vector<std::string> unseen = UnseenExpectedFailures();
  if (!unseen.empty()) {
    return absl::FailedPreconditionError(
        absl::StrCat("The following expected failures were not seen: ",
                     absl::StrJoin(unseen, ", ")));
  }
  return absl::OkStatus();
}

}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
