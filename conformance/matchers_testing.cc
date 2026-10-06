// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/matchers_testing.h"

#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include "absl/container/flat_hash_set.h"
#include "absl/status/statusor.h"
#include "absl/strings/match.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "conformance/result_record.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {
namespace {

// The record in `part`, if it is a gtest success that carries one.
absl::optional<RecordedResult> RecordIn(const testing::TestPartResult& part) {
  if (part.type() != testing::TestPartResult::kSuccess) return absl::nullopt;
  absl::StatusOr<RecordedResult> recorded =
      ParseResultRecordMessage(part.message());
  if (!recorded.ok()) return absl::nullopt;
  return *std::move(recorded);
}

}  // namespace

std::vector<std::pair<std::string, std::string>> RecordedResults(
    const testing::TestResult& result) {
  std::vector<std::pair<std::string, std::string>> results;
  for (int i = 0; i < result.total_part_count(); ++i) {
    if (absl::optional<RecordedResult> recorded =
            RecordIn(result.GetTestPartResult(i));
        recorded.has_value()) {
      results.emplace_back(recorded->test_name, recorded->record.ToString());
    }
  }
  return results;
}

void ExpectYieldsFailure(const testing::TestPartResultArray& results,
                         absl::string_view substr) {
  int failures = 0;
  // gtest evaluates a failing matcher twice, and the second evaluation can't
  // see what the first one recorded while both are intercepted; record each
  // outcome once.
  absl::flat_hash_set<std::string> recorded;
  for (int i = 0; i < results.size(); ++i) {
    const testing::TestPartResult& part = results.GetTestPartResult(i);
    if (part.type() == testing::TestPartResult::kSuccess) {
      if (absl::optional<RecordedResult> record = RecordIn(part);
          record.has_value() &&
          recorded
              .insert(ResultRecordMessage(record->test_name, record->record))
              .second) {
        SUCCEED() << ResultRecordMessage(record->test_name, record->record);
      }
      continue;
    }
    ++failures;
    if (part.type() == testing::TestPartResult::kNonFatalFailure &&
        absl::StrContains(part.message(), substr)) {
      continue;
    }
    ADD_FAILURE() << "Expected: 1 non-fatal failure containing \"" << substr
                  << "\"\n  Actual:\n"
                  << part;
  }
  if (failures != 1) {
    ADD_FAILURE() << "Expected: 1 non-fatal failure containing \"" << substr
                  << "\"\n  Actual: " << failures << " failures";
  }
}

}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
