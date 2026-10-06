// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Helpers for unit tests of code that checks conformance results with
// Yields() (see matchers.h).

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_MATCHERS_TESTING_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_MATCHERS_TESTING_H__

#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest-spi.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {

// The outcomes Yields() recorded in `result` (see ResultRecordMessage() in
// result_record.h), in order, as (conformance test name,
// ResultRecord::ToString()) pairs.
std::vector<std::pair<std::string, std::string>> RecordedResults(
    const testing::TestResult& result);

// Implements EXPECT_YIELDS_FAILURE(): `results` is what the statement
// reported.
void ExpectYieldsFailure(const testing::TestPartResultArray& results,
                         absl::string_view substr);

}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google

// Like EXPECT_NONFATAL_FAILURE(statement, substr), for a statement that checks
// a conformance result with Yields(): `statement` must report exactly one
// non-fatal failure, whose message contains `substr`.  EXPECT_NONFATAL_FAILURE
// would also intercept the gtest success Yields() records the outcome with
// (see ResultRecordMessage() in result_record.h) and complain about it.  This
// records each outcome `statement` recorded on the running gtest test once it
// is done, once, so that the test environment still hears about the result.
#define EXPECT_YIELDS_FAILURE(statement, substr)                         \
  do {                                                                   \
    ::testing::TestPartResultArray gtest_failures;                       \
    {                                                                    \
      ::testing::ScopedFakeTestPartResultReporter gtest_reporter(        \
          ::testing::ScopedFakeTestPartResultReporter::                  \
              INTERCEPT_ONLY_CURRENT_THREAD,                             \
          &gtest_failures);                                              \
      statement;                                                         \
    }                                                                    \
    ::google::protobuf::conformance::internal::ExpectYieldsFailure(gtest_failures, \
                                                         substr);        \
  } while (false)

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_MATCHERS_TESTING_H__
