#include "test_manager.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/container/btree_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "absl/types/optional.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {
namespace {

constexpr int kMaximumWildcardExpansions = 20;
constexpr int kFailureMessageLengthLimit = 128;

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
  std::string result(absl::StripAsciiWhitespace(input));
  // Remove newlines
  Normalize(result);
  // Truncate failure message if needed
  if (result.length() > kFailureMessageLengthLimit) {
    result = result.substr(0, kFailureMessageLengthLimit);
  }
  return result;
}

// Formats an entry line for SaveFailureList(): the test name padded to
// `alignment`, then " # " and the message, or the name alone if the line has
// no message.  A comment or blank line is only stripped.
std::string ReformatLine(size_t alignment, absl::string_view line) {
  size_t comment_pos = line.find('#');
  absl::string_view test_name =
      absl::StripAsciiWhitespace(line.substr(0, comment_pos));
  if (test_name.empty()) {
    return std::string(absl::StripAsciiWhitespace(line));
  }
  absl::string_view message;
  if (comment_pos != absl::string_view::npos) {
    message = absl::StripAsciiWhitespace(line.substr(comment_pos + 1));
  }
  if (message.empty()) {
    return std::string(test_name);
  }
  ABSL_CHECK_GE(alignment, test_name.length());
  std::string whitespace(alignment - test_name.length(), ' ');
  return absl::StrCat(test_name, whitespace, " # ", message);
}

// The failure list line for `test_name` failing with `message`, which may be
// empty; ReformatLine() aligns it.
std::string EntryLine(absl::string_view test_name, absl::string_view message) {
  if (message.empty()) return std::string(test_name);
  return absl::StrCat(test_name, " # ", message);
}

// The test name of a failure list line (an entry as listed), or empty for a
// comment or blank line.
absl::string_view EntryOf(absl::string_view line) {
  return absl::StripAsciiWhitespace(line.substr(0, line.find('#')));
}

// The copybara markers that delimit a region the open source export drops.
// They are spelled in two pieces so that the export's scrubber, which
// processes every file, doesn't take these literals for markers.
constexpr absl::string_view kStripBeginMarker =
    "copybara:"
    "strip_begin";
constexpr absl::string_view kStripEndMarker =
    "copybara:"
    "strip_end";

// Whether `line` is a comment whose text starts with `marker`, e.g. a
// `# <kStripBeginMarker>` line (copybara allows text after the marker).
bool IsCommentStartingWith(absl::string_view line, absl::string_view marker) {
  line = absl::StripAsciiWhitespace(line);
  if (!absl::ConsumePrefix(&line, "#")) return false;
  return absl::StartsWith(absl::StripLeadingAsciiWhitespace(line), marker);
}

bool IsStripBegin(absl::string_view line) {
  return IsCommentStartingWith(line, kStripBeginMarker);
}

// Also matches the `strip_end_and_replace_begin` marker, which closes the
// block as well.
bool IsStripEnd(absl::string_view line) {
  return IsCommentStartingWith(line, kStripEndMarker);
}

// A line SaveFailureList() writes, with the alignment scope it belongs to: 0
// for the lines outside copybara strip blocks, k > 0 for the lines of the k-th
// block (its marker lines included).
struct SavedLine {
  std::string text;
  size_t scope;
};

}  // namespace

TestManager::~TestManager() {
  if (!finalized_) {
    ABSL_LOG(FATAL)
        << "TestManager::Finalize() was not called before destruction.";
  }
}

absl::Status TestManager::LoadFailureList(absl::string_view filename) {
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

absl::Status TestManager::AddExpectedFailure(
    absl::string_view test_name, absl::string_view failure_message) {
  if (absl::Status status = expected_failure_list_.Insert(test_name);
      !status.ok()) {
    return status;
  }
  // Normalize the message the same way reported failures are normalized so
  // that the two compare equal in ReportFailure.
  ABSL_CHECK(expected_failure_messages_
                 .emplace(test_name, FormatFailureMessage(failure_message))
                 .second);
  ABSL_CHECK(unseen_expected_failures_.emplace(test_name).second);
  ABSL_CHECK(unmatched_expected_failures_.emplace(test_name).second);
  return absl::OkStatus();
}

bool TestManager::IsExpectedToFail(absl::string_view test_name) const {
  return expected_failure_list_.WalkDownMatch(test_name).has_value();
}

absl::optional<std::string> TestManager::ExpectedFailureMessage(
    absl::string_view entry) const {
  auto it = expected_failure_messages_.find(entry);
  if (it == expected_failure_messages_.end()) return absl::nullopt;
  return it->second;
}

absl::Status TestManager::SaveFailureList(absl::string_view filename) const {
  std::ofstream outfile(std::string{filename});
  if (!outfile.is_open()) {
    return absl::InternalError(absl::StrCat(
        "Couldn't open failure list file for writing: ", filename));
  }

  // The loaded lines with unseen entries dropped and changed entries rewritten
  // (see AppendSavedLines()), each tagged with its alignment scope; the new
  // failures are merged in below.  A copybara strip block runs from a
  // strip_begin line to the next strip_end line; a strip_begin with no
  // strip_end after it, a strip_end outside a block and a strip_begin inside
  // one are ordinary comments, as they are to LoadFailureList().
  std::vector<SavedLine> lines;
  size_t blocks = 0;
  size_t scope = 0;
  for (size_t i = 0; i < failure_list_lines_.size(); ++i) {
    const std::string& line = failure_list_lines_[i];
    if (scope == 0 && IsStripBegin(line) &&
        std::any_of(
            failure_list_lines_.begin() + static_cast<std::ptrdiff_t>(i),
            failure_list_lines_.end(), IsStripEnd)) {
      scope = ++blocks;
    }
    std::vector<std::string> saved;
    AppendSavedLines(line, saved);
    for (std::string& text : saved) {
      lines.push_back({std::move(text), scope});
    }
    if (scope != 0 && IsStripEnd(line)) {
      scope = 0;
    }
  }

  // Calculate the alignment of each scope: the longest name among its lines
  // and, outside the blocks, the new failures.
  std::vector<size_t> alignment(blocks + 1, 0);
  for (const SavedLine& line : lines) {
    alignment[line.scope] =
        std::max(alignment[line.scope], EntryOf(line.text).length());
  }
  for (const auto& failure : new_failures_) {
    alignment[0] = std::max(alignment[0], failure.first.length());
  }

  // Output the lines, inserting the new failures by name among the lines
  // outside the blocks.
  auto to_add = new_failures_.begin();
  for (const SavedLine& line : lines) {
    if (line.scope == 0) {
      absl::string_view test_name = EntryOf(line.text);
      while (to_add != new_failures_.end() && test_name > to_add->first) {
        outfile << ReformatLine(alignment[0],
                                EntryLine(to_add->first, to_add->second))
                << "\n";
        ++to_add;
      }
    }
    outfile << ReformatLine(alignment[line.scope], line.text) << "\n";
  }

  // Add any remaining new failures.
  while (to_add != new_failures_.end()) {
    outfile << ReformatLine(alignment[0],
                            EntryLine(to_add->first, to_add->second))
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

void TestManager::AppendSavedLines(absl::string_view line,
                                   std::vector<std::string>& lines) const {
  absl::string_view entry = EntryOf(line);
  if (entry.empty()) {
    // A comment or blank line.
    lines.emplace_back(line);
    return;
  }

  auto it = entry_matches_.find(entry);
  if (it == entry_matches_.end()) {
    // No test matched the entry with a verdict: it is unseen and goes.
    return;
  }
  const absl::btree_map<std::string, MatchOutcome>& matches = it->second;

  // Kept verbatim unless a matched test succeeded or failed with a different
  // message.
  const bool changed = absl::c_any_of(matches, [](const auto& match) {
    return match.second.kind == MatchOutcome::kSuccess ||
           match.second.kind == MatchOutcome::kOtherFailure;
  });
  if (!changed) {
    lines.emplace_back(line);
    return;
  }

  // Kept as the entry (a wildcard included) with the new message if every
  // matched test failed with that same message.
  const MatchOutcome& first = matches.begin()->second;
  const bool same_new_message =
      first.kind == MatchOutcome::kOtherFailure &&
      absl::c_all_of(matches, [&](const auto& match) {
        return match.second.kind == MatchOutcome::kOtherFailure &&
               match.second.message == first.message;
      });
  if (same_new_message) {
    lines.push_back(EntryLine(entry, first.message));
    return;
  }

  // Otherwise the matched set diverged: one line per matched test that still
  // fails or was skipped (which for an exact entry that succeeded is none).
  const std::string& expected_message = expected_failure_messages_.at(entry);
  for (const auto& [test_name, outcome] : matches) {
    switch (outcome.kind) {
      case MatchOutcome::kSuccess:
        break;
      case MatchOutcome::kOtherFailure:
        lines.push_back(EntryLine(test_name, outcome.message));
        break;
      case MatchOutcome::kExpectedFailure:
      case MatchOutcome::kSkip:
        lines.push_back(EntryLine(test_name, expected_message));
        break;
    }
  }
}

absl::Status TestManager::ReportSuccess(absl::string_view test_name) {
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
    entry_matches_[*failure_match][test_name] = {.kind =
                                                     MatchOutcome::kSuccess};
    return absl::FailedPreconditionError(absl::StrCat(
        "test ", test_name, " (matched to ", *failure_match,
        ") is in the failure list, but test succeeded.  Remove its match from "
        "the failure list."));
  }

  // This wasn't expected to fail.
  IncrementIfUnique(unique, expected_successes_);
  return absl::OkStatus();
}

absl::Status TestManager::ReportFailure(absl::string_view test_name,
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
    // This was not expected to fail.
    record_unexpected_failure();
    new_failures_[test_name] = formatted_failure_message;
    return absl::FailedPreconditionError(
        absl::StrCat("Unexpected failure for test: ", test_name));
  }

  // Like the legacy runner, only require the actual failure message to start
  // with the expected one.
  const std::string& expected_failure_message =
      expected_failure_messages_.at(*failure_match);
  // The entry is seen either way: Finalize() is about entries no test
  // reached, and the mismatch is already reported by the returned error, by
  // UnexpectedFailures() and by SaveFailureList().
  unseen_expected_failures_.erase(*failure_match);
  if (!absl::StartsWith(formatted_failure_message, expected_failure_message)) {
    record_unexpected_failure();
    entry_matches_[*failure_match][test_name] = {
        .kind = MatchOutcome::kOtherFailure,
        .message = formatted_failure_message};
    return absl::FailedPreconditionError(absl::StrCat(
        "Unexpected failure message for test: ", test_name, " expected: ",
        expected_failure_message, " actual: ", formatted_failure_message));
  }

  entry_matches_[*failure_match][test_name] = {
      .kind = MatchOutcome::kExpectedFailure};

  if (number_of_matches_[*failure_match] > kMaximumWildcardExpansions) {
    record_unexpected_failure();
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

void TestManager::ReportRecommendedFailure(absl::string_view test_name) {
  ABSL_DCHECK(!IsExpectedToFail(test_name))
      << "Recommended test " << test_name
      << " is in the failure list and must be reported with ReportFailure()";
  bool unique = InsertUniqueTest(seen_tests_, test_name);
  IncrementIfUnique(unique, tolerated_recommended_failures_);
}

absl::Status TestManager::ReportSkip(absl::string_view test_name,
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
  entry_matches_[*failure_match][test_name] = {.kind = MatchOutcome::kSkip};
  IncrementIfUnique(unique, listed_skips_);
  if (unique) {
    listed_skip_matches_[test_name] = *failure_match;
  }
  return absl::FailedPreconditionError(absl::StrCat(
      "test ", test_name, " (matched to ", *failure_match,
      ") is in the failure list but was skipped by the testee: ", skip_reason,
      ".  Remove its match from the failure list."));
}

void TestManager::ReportNotSelected(absl::string_view test_name) {
  MarkMatched(test_name);
}

absl::optional<std::string> TestManager::MarkMatched(
    absl::string_view test_name) {
  absl::optional<std::string> failure_match =
      expected_failure_list_.WalkDownMatch(test_name);
  if (failure_match.has_value()) {
    unmatched_expected_failures_.erase(*failure_match);
  }
  return failure_match;
}

std::vector<std::string> TestManager::UnseenExpectedFailures() const {
  std::vector<std::string> unseen(unseen_expected_failures_.begin(),
                                  unseen_expected_failures_.end());
  absl::c_sort(unseen);
  return unseen;
}

std::vector<std::string> TestManager::UnmatchedExpectedFailures() const {
  std::vector<std::string> unmatched(unmatched_expected_failures_.begin(),
                                     unmatched_expected_failures_.end());
  absl::c_sort(unmatched);
  return unmatched;
}

std::vector<UnexpectedResult> TestManager::UnexpectedFailures() const {
  std::vector<UnexpectedResult> failures;
  failures.reserve(unexpected_failure_messages_.size());
  // The map is ordered by test name.
  for (const auto& [test_name, failure_message] :
       unexpected_failure_messages_) {
    failures.push_back(
        {.test_name = test_name, .failure_message = failure_message});
  }
  return failures;
}

std::vector<UnexpectedResult> TestManager::UnexpectedSuccesses() const {
  std::vector<UnexpectedResult> successes;
  successes.reserve(unexpected_success_matches_.size());
  // The map is ordered by test name.
  for (const auto& [test_name, matched_entry] : unexpected_success_matches_) {
    successes.push_back(
        {.test_name = test_name,
         .failure_message = expected_failure_messages_.at(matched_entry),
         .matched_entry = matched_entry});
  }
  return successes;
}

std::vector<std::pair<std::string, std::string>> TestManager::ListedSkips()
    const {
  // The map is ordered by test name.
  return std::vector<std::pair<std::string, std::string>>(
      listed_skip_matches_.begin(), listed_skip_matches_.end());
}

absl::Status TestManager::Finalize() {
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
