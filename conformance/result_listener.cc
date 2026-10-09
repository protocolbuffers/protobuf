// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/result_listener.h"

#include <iterator>
#include <string>
#include <utility>

#include <gtest/gtest.h>
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "conformance/result_ledger.h"
#include "conformance/result_record.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {

Statistics Statistics::From(const ResultLedger& ledger) {
  Statistics statistics;
  statistics.skipped_tests = ledger.skipped();
  statistics.listed_skips = ledger.listed_skips();
  statistics.tolerated_failures = ledger.tolerated_failures();
  statistics.expected_failures = ledger.expected_failures();
  statistics.unexpected_failures = ledger.unexpected_failures();
  statistics.expected_successes = ledger.expected_successes();
  statistics.unexpected_successes = ledger.unexpected_successes();
  return statistics;
}

Statistics Statistics::operator-(const Statistics& other) const {
  Statistics delta;
  delta.skipped_tests = skipped_tests - other.skipped_tests;
  delta.listed_skips = listed_skips - other.listed_skips;
  delta.tolerated_failures = tolerated_failures - other.tolerated_failures;
  delta.expected_failures = expected_failures - other.expected_failures;
  delta.unexpected_failures = unexpected_failures - other.unexpected_failures;
  delta.expected_successes = expected_successes - other.expected_successes;
  delta.unexpected_successes =
      unexpected_successes - other.unexpected_successes;
  return delta;
}

Statistics& Statistics::operator+=(const Statistics& other) {
  skipped_tests += other.skipped_tests;
  listed_skips += other.listed_skips;
  tolerated_failures += other.tolerated_failures;
  expected_failures += other.expected_failures;
  unexpected_failures += other.unexpected_failures;
  expected_successes += other.expected_successes;
  unexpected_successes += other.unexpected_successes;
  return *this;
}

void Statistics::RecordProperties() const {
  testing::Test::RecordProperty("skipped_tests", skipped_tests);
  testing::Test::RecordProperty("listed_skips", listed_skips);
  testing::Test::RecordProperty("tolerated_failures", tolerated_failures);
  testing::Test::RecordProperty("expected_failures", expected_failures);
  testing::Test::RecordProperty("unexpected_failures", unexpected_failures);
  testing::Test::RecordProperty("expected_successes", expected_successes);
  testing::Test::RecordProperty("unexpected_successes", unexpected_successes);
}

void ResultListener::OnTestSuiteStart(const testing::TestSuite& /*suite*/) {
  suite_start_ = Statistics::From(*ledger_);
}

void ResultListener::OnTestStart(const testing::TestInfo& /*test_info*/) {
  // Results checked in SetUpTestSuite() are the suite's, not this test's:
  // hold them back until the suite ends.
  suite_results_.insert(suite_results_.end(),
                        std::make_move_iterator(recorded_results_.begin()),
                        std::make_move_iterator(recorded_results_.end()));
  recorded_results_.clear();
  test_start_ = Statistics::From(*ledger_);
}

void ResultListener::OnTestPartResult(const testing::TestPartResult& part) {
  if (part.type() != testing::TestPartResult::kSuccess) return;
  absl::StatusOr<RecordedResult> recorded =
      ParseResultRecordMessage(part.message());
  if (absl::IsNotFound(recorded.status())) return;  // An ordinary success.
  if (!recorded.ok()) {
    undecodable_results_.emplace_back(recorded.status().message());
    return;
  }
  // The first outcome recorded for a test is the one that counts.
  if (ledger_->WasReported(recorded->test_name)) return;
  ledger_->Report(recorded->test_name, recorded->record);
  recorded_results_.push_back(*std::move(recorded));
}

void ResultListener::OnTestEnd(const testing::TestInfo& /*test_info*/) {
  ReportRecordedResults();
  (Statistics::From(*ledger_) - test_start_).RecordProperties();
}

void ResultListener::OnTestSuiteEnd(const testing::TestSuite& /*suite*/) {
  // What SetUpTestSuite() checked, then what TearDownTestSuite() did.
  recorded_results_.insert(recorded_results_.begin(),
                           std::make_move_iterator(suite_results_.begin()),
                           std::make_move_iterator(suite_results_.end()));
  suite_results_.clear();
  ReportRecordedResults();
  (Statistics::From(*ledger_) - suite_start_).RecordProperties();
}

void ResultListener::ReportRecordedResults() {
  for (const RecordedResult& recorded : recorded_results_) {
    testing::Test::RecordProperty(recorded.test_name,
                                  recorded.record.ToString());
  }
  recorded_results_.clear();
  for (const std::string& error : undecodable_results_) {
    ADD_FAILURE() << error << "\nThe outcome it recorded is lost.";
  }
  undecodable_results_.clear();
}

}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
