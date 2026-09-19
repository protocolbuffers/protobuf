// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/result_listener.h"

#include <gtest/gtest.h>
#include "conformance/result_ledger.h"

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
  test_start_ = Statistics::From(*ledger_);
}

void ResultListener::OnTestEnd(const testing::TestInfo& /*test_info*/) {
  (Statistics::From(*ledger_) - test_start_).RecordProperties();
}

void ResultListener::OnTestSuiteEnd(const testing::TestSuite& /*suite*/) {
  (Statistics::From(*ledger_) - suite_start_).RecordProperties();
}

}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
