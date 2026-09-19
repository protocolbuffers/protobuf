// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_RESULT_LISTENER_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_RESULT_LISTENER_H__

#include <string>
#include <vector>

#include <gtest/gtest.h>
#include "absl/base/nullability.h"
#include "conformance/result_ledger.h"
#include "conformance/result_record.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {

// A snapshot of the ResultLedger's counters.  The ResultListener reports the
// per-test and per-suite deltas, and the environment the run's total, as test
// properties.
struct Statistics {
  int skipped_tests = 0;
  int listed_skips = 0;
  int tolerated_failures = 0;
  int expected_failures = 0;
  int unexpected_failures = 0;
  int expected_successes = 0;
  int unexpected_successes = 0;

  static Statistics From(const ResultLedger& ledger);
  Statistics operator-(const Statistics& other) const;
  Statistics& operator+=(const Statistics& other);

  // Records each statistic as a gtest property of the current test (or suite,
  // or run) under the names skipped_tests, listed_skips,
  // tolerated_failures, expected_failures, unexpected_failures,
  // expected_successes and unexpected_successes.
  void RecordProperties() const;
};

// How the conformance test environment hears what every conformance test did.
// Yields() (matchers.h) records each outcome as a gtest success in the running
// test (see ResultRecordMessage() in result_record.h).  This listener picks
// those out of the test part results as gtest reports them and tallies them in
// the ResultLedger at once.  At the end of every gtest test and test suite it
// records the outcomes checked there as properties named after the conformance
// tests (see ReportRecordedResults()) and the statistics the test or suite
// added as its properties.  gtest still attributes properties to the test at
// that point (it forgets the current test only after the listeners ran), and
// likewise for the suite.
//
// ConformanceEnvironment (test_environment.h) registers one with gtest.  The
// listener doesn't depend on the environment, so unit tests drive it directly.
class ResultListener : public testing::EmptyTestEventListener {
 public:
  explicit ResultListener(ResultLedger* absl_nonnull ledger)
      : ledger_(ledger) {}

  void OnTestSuiteStart(const testing::TestSuite& suite) override;
  void OnTestStart(const testing::TestInfo& test_info) override;
  // gtest holds its lock while the listeners see a test part result, so this
  // must not call back into it; it only tallies.
  void OnTestPartResult(const testing::TestPartResult& part) override;
  void OnTestEnd(const testing::TestInfo& test_info) override;
  void OnTestSuiteEnd(const testing::TestSuite& suite) override;

  // Records the outcome of every conformance test checked since the last call
  // as a property of the current gtest test (or suite, or run) named after the
  // conformance test, with ResultRecord::ToString() as the value, so that the
  // reports can tell what each test did.  The outcomes themselves were tallied
  // in the ledger as they were checked.  Also fails the current test for every
  // gtest success that claimed to carry an outcome but couldn't be decoded
  // (see ParseResultRecordMessage() in result_record.h), so that no outcome
  // goes missing quietly.  Runs when a test or a suite ends, and from
  // ConformanceEnvironment::TearDown() for anything checked outside a test
  // suite.
  void ReportRecordedResults();

 private:
  ResultLedger* const absl_nonnull ledger_;
  // The counters when the current suite and test started.
  Statistics suite_start_;
  Statistics test_start_;
  // The outcomes checked since ReportRecordedResults() last ran, in order, and
  // the gtest successes that claimed to carry an outcome but couldn't be
  // decoded.
  std::vector<RecordedResult> recorded_results_;
  std::vector<std::string> undecodable_results_;
  // The results checked in SetUpTestSuite(), held back while the tests run.
  std::vector<RecordedResult> suite_results_;
};

}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_RESULT_LISTENER_H__
