// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_RESULT_RECORD_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_RESULT_RECORD_H__

#include <string>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "conformance/testee.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {

// The outcome of one conformance test, as Yields() (matchers.h) records it
// when it checks the test's result and as the test environment tallies it
// (see ResultLedger::Report() in result_ledger.h) and reports it, as a gtest
// property named after the test whose value is ToString(), e.g. "P0 PASS" or
// "P1 FAIL: Failed to parse input.".
struct ResultRecord {
  enum class Status {
    kPass,   // The result matched the expectation.
    kFail,   // It didn't.
    kCrash,  // The testee hit a runtime error or timed out.
    kSkip,   // The testee declined to run the test.
  };

  TestPriority priority;
  Status status;
  // The failure message or skip reason, if any.
  std::string message;

  // "<priority> <status>[: <message>]": the priority as PriorityName() spells
  // it, the status as PASS, FAIL, CRASH or SKIP, and the message stripped of
  // trailing whitespace and left out when empty.
  std::string ToString() const;

  // The inverse of ToString().  Returns nullopt for a value in any other
  // form.
  static absl::optional<ResultRecord> Parse(absl::string_view value);
};

// How Yields() hands a record to the test environment: as the message of a
// gtest success (SUCCEED()) in the running test.  gtest prints nothing for a
// success and reports it to the test event listeners at once, so the
// environment's listener sees the record as soon as the result is checked,
// while the record stays with the gtest result it belongs to.
//
// The first line that starts with kResultRecordMarker carries the record:
// the marker, the test name and ToString(), tab-separated and C-escaped onto
// that one line, so that what gtest appends to the message (SCOPED_TRACE
// output) doesn't get in the way.
inline constexpr absl::string_view kResultRecordMarker = "[conformance result]";

std::string ResultRecordMessage(absl::string_view test_name,
                                const ResultRecord& record);

// A record as ParseResultRecordMessage() finds it.
struct RecordedResult {
  std::string test_name;
  ResultRecord record;
};

// The inverse of ResultRecordMessage(): the record in `message`, the message
// of a test part result.  Returns kNotFound if there is none, that is if the
// part is an ordinary success, and kInvalidArgument if the message claims to
// carry a record that can't be decoded.  The test environment fails the test
// over the latter, so that no outcome goes missing quietly.
absl::StatusOr<RecordedResult> ParseResultRecordMessage(
    absl::string_view message);

}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_RESULT_RECORD_H__
