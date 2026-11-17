// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/failure_list.h"

#include <algorithm>
#include <string>

#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "conformance/testee.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {
namespace {

constexpr int kFailureMessageLengthLimit = 128;

void Normalize(std::string& input) {
  input.erase(std::remove(input.begin(), input.end(), '\n'), input.end());
}

}  // namespace

std::string FormatFailureMessage(absl::string_view message) {
  std::string result(absl::StripAsciiWhitespace(message));
  // Remove newlines
  Normalize(result);
  // Truncate failure message if needed
  if (result.length() > kFailureMessageLengthLimit) {
    result = result.substr(0, kFailureMessageLengthLimit);
  }
  return result;
}

absl::Status FailureList::Add(absl::string_view test_name,
                              absl::string_view failure_message) {
  if (absl::Status status = entries_.Insert(test_name); !status.ok()) {
    return status;
  }
  // Normalize the message the same way reported failures are normalized so
  // that the two compare equal in VerdictOnFailure().
  ABSL_CHECK(expected_messages_
                 .emplace(test_name, FormatFailureMessage(failure_message))
                 .second);
  return absl::OkStatus();
}

absl::Status FailureList::VerdictOnSuccess(absl::string_view test_name) const {
  absl::optional<std::string> failure_match = MatchingEntry(test_name);
  if (!failure_match.has_value()) {
    return absl::OkStatus();
  }
  return absl::FailedPreconditionError(absl::StrCat(
      "test ", test_name, " (matched to ", *failure_match,
      ") is in the failure list, but test succeeded.  Remove its match from "
      "the failure list."));
}

absl::Status FailureList::VerdictOnFailure(
    absl::string_view test_name, TestPriority priority,
    absl::string_view failure_message) const {
  absl::optional<std::string> failure_match = MatchingEntry(test_name);
  if (!failure_match.has_value()) {
    if (priority > enforcement_level_) {
      return absl::OkStatus();  // Tolerated.
    }
    return absl::FailedPreconditionError(
        absl::StrCat("Unexpected failure for test: ", test_name));
  }

  // Mirror the legacy runner, which only requires the actual failure message to
  // start with the expected one.
  std::string formatted_failure_message = FormatFailureMessage(failure_message);
  const std::string& expected_failure_message =
      expected_messages_.at(*failure_match);
  if (!absl::StartsWith(formatted_failure_message, expected_failure_message)) {
    return absl::FailedPreconditionError(absl::StrCat(
        "Unexpected failure message for test: ", test_name, " expected: ",
        expected_failure_message, " actual: ", formatted_failure_message));
  }
  return absl::OkStatus();
}

absl::Status FailureList::VerdictOnSkip(absl::string_view test_name,
                                        absl::string_view skip_reason) const {
  absl::optional<std::string> failure_match = MatchingEntry(test_name);
  if (!failure_match.has_value()) {
    return absl::OkStatus();
  }
  return absl::FailedPreconditionError(absl::StrCat(
      "test ", test_name, " (matched to ", *failure_match,
      ") is in the failure list but was skipped by the testee: ", skip_reason,
      ".  Remove its match from the failure list."));
}

absl::optional<std::string> FailureList::MatchingEntry(
    absl::string_view test_name) const {
  return entries_.WalkDownMatch(test_name);
}

absl::optional<std::string> FailureList::ExpectedMessage(
    absl::string_view entry) const {
  auto it = expected_messages_.find(entry);
  if (it == expected_messages_.end()) return absl::nullopt;
  return it->second;
}

}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
