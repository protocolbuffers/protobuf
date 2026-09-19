#include "conformance/result_ledger.h"

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <ios>
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

// The test name (or wildcard) of a failure list line, i.e. what precedes its
// comment, with all whitespace removed: the key entries are loaded and looked
// up by, so that LoadFailureList() and AppendSavedLines() agree on it.
std::string EntryKeyOf(absl::string_view line) {
  std::string key(line.substr(0, line.find('#')));
  key.erase(std::remove_if(key.begin(), key.end(), absl::ascii_isspace),
            key.end());
  return key;
}

// The test name of a failure list line as written (an entry as listed, see
// EntryKeyOf() for the key it is looked up by), or empty for a comment or
// blank line.
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

    std::string test_name = EntryKeyOf(line);

    if (test_name.empty()) {  // Skip empty lines and comments.
      continue;
    }

    // Find our failure message if it exists.  Will be set to an empty string
    // if no message is found.  Empty failure messages also pass our tests.
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

void ResultLedger::AppendSavedLines(absl::string_view line,
                                    std::vector<std::string>& lines) const {
  const std::string entry = EntryKeyOf(line);
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

  // Kept verbatim unless a matched test succeeded, was skipped or failed with
  // a different message.
  const bool changed = absl::c_any_of(matches, [](const auto& match) {
    return match.second.kind != MatchOutcome::kExpectedFailure;
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
  // fails (which for an exact entry that succeeded or was skipped is none).
  const std::string expected_message = *failure_list_.ExpectedMessage(entry);
  for (const auto& [test_name, outcome] : matches) {
    switch (outcome.kind) {
      case MatchOutcome::kSuccess:
      case MatchOutcome::kSkip:
        break;
      case MatchOutcome::kOtherFailure:
        lines.push_back(EntryLine(test_name, outcome.message));
        break;
      case MatchOutcome::kExpectedFailure:
        lines.push_back(EntryLine(test_name, expected_message));
        break;
    }
  }
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
    entry_matches_[*failure_match][test_name] = {
        /*kind=*/MatchOutcome::kSuccess};
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

  // The entry is seen either way: Finalize() is about entries no test
  // reached, and a message mismatch is already reported by the verdict, by
  // UnexpectedFailures() and by SaveFailureList().
  unseen_expected_failures_.erase(*failure_match);
  if (!absl::StartsWith(formatted_failure_message,
                        *failure_list_.ExpectedMessage(*failure_match))) {
    record_unexpected_failure();
    entry_matches_[*failure_match][test_name] = {
        /*kind=*/MatchOutcome::kOtherFailure,
        /*message=*/formatted_failure_message};
  } else {
    entry_matches_[*failure_match][test_name] = {
        /*kind=*/MatchOutcome::kExpectedFailure};
    IncrementIfUnique(unique, number_of_matches_[*failure_match]);
    IncrementIfUnique(unique, expected_failures_);
  }
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

  // A skipped test is a listed test that didn't fail, so its match is not an
  // unseen entry for Finalize() but is dropped by SaveFailureList(), like a
  // success (the verdict asks for the same by hand).  It doesn't count toward
  // the entry's wildcard expansion cap (number_of_matches_) either, unlike in
  // the legacy runner, which counted every test a wildcard matched: the cap
  // is about real outcomes hidden by a wildcard, and a skip isn't one.
  unseen_expected_failures_.erase(*failure_match);
  entry_matches_[*failure_match][test_name] = {/*kind=*/MatchOutcome::kSkip};
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
    case ResultRecord::Status::kNotSelected:
      ReportNotSelected(test_name);
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
