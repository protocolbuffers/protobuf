// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/result_record.h"

#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/ascii.h"
#include "absl/strings/escaping.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "absl/types/optional.h"
#include "conformance/testee.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {
namespace {

// The status words of ResultRecord::ToString(), indexed by Status.
constexpr absl::string_view kStatusNames[] = {"PASS", "FAIL", "CRASH", "SKIP"};

}  // namespace

std::string ResultRecord::ToString() const {
  std::string value = absl::StrCat(PriorityName(priority), " ",
                                   kStatusNames[static_cast<int>(status)]);
  absl::string_view stripped = absl::StripTrailingAsciiWhitespace(message);
  if (!stripped.empty()) absl::StrAppend(&value, ": ", stripped);
  return value;
}

absl::optional<ResultRecord> ResultRecord::Parse(absl::string_view value) {
  ResultRecord record;
  bool found = false;
  for (TestPriority priority : {kP0, kP1}) {
    if (absl::ConsumePrefix(&value,
                            absl::StrCat(PriorityName(priority), " "))) {
      record.priority = priority;
      found = true;
      break;
    }
  }
  if (!found) return absl::nullopt;
  absl::string_view status = value.substr(0, value.find(':'));
  const absl::string_view* name = absl::c_find(kStatusNames, status);
  if (name == std::end(kStatusNames)) return absl::nullopt;
  record.status = static_cast<Status>(name - std::begin(kStatusNames));
  value.remove_prefix(status.size());
  if (!value.empty() && !absl::ConsumePrefix(&value, ": ")) {
    return absl::nullopt;
  }
  record.message = std::string(value);
  return record;
}

std::string ResultRecordMessage(absl::string_view test_name,
                                const ResultRecord& record) {
  return absl::StrCat(kResultRecordMarker, "\t", absl::CEscape(test_name), "\t",
                      absl::CEscape(record.ToString()));
}

absl::StatusOr<RecordedResult> ParseResultRecordMessage(
    absl::string_view message) {
  for (absl::string_view line : absl::StrSplit(message, '\n')) {
    if (!absl::StartsWith(line, kResultRecordMarker)) continue;
    auto invalid = [line] {
      return absl::InvalidArgumentError(
          absl::StrCat("Can't decode the conformance result record in a gtest "
                       "success: ",
                       line));
    };
    std::vector<absl::string_view> fields = absl::StrSplit(line, '\t');
    if (fields.size() != 3 || fields[0] != kResultRecordMarker) {
      return invalid();
    }
    RecordedResult result;
    std::string value;
    if (!absl::CUnescape(fields[1], &result.test_name) ||
        !absl::CUnescape(fields[2], &value)) {
      return invalid();
    }
    absl::optional<ResultRecord> record = ResultRecord::Parse(value);
    if (!record.has_value()) return invalid();
    result.record = *std::move(record);
    return result;
  }
  return absl::NotFoundError("Not a conformance result record.");
}

}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
