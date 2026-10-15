// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_FAILURE_LIST_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_FAILURE_LIST_H__

#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "conformance/failure_list_trie_node.h"
#include "conformance/testee.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {

// The form a failure message takes in a failure list: surrounding whitespace
// and newlines removed, truncated to 128 characters.  Expected messages are
// normalized when they are added and actual ones when they are judged, so
// that FailureList::VerdictOnFailure() compares like with like.
std::string FormatFailureMessage(absl::string_view message);

// The expected failures of a conformance run, as the failure list files list
// them, plus the enforcement level: everything needed to judge one test's
// outcome, and nothing that changes once the tests run.  The ResultLedger
// (result_ledger.h) builds it while loading the failure lists and tallies the
// outcomes against it.  Yields() (matchers.h) reads the run's list through
// GetGlobalFailureList() (global_test_environment.h) and changes nothing.
class FailureList {
 public:
  FailureList() : entries_("root") {}

  // The lowest priority whose failures fail the suite (see TestPriority in
  // testee.h).  VerdictOnFailure() tolerates a failing test of a lower priority
  // unless it is listed.  Defaults to kLowestPriority, which enforces every
  // priority.
  void set_enforcement_level(TestPriority level) { enforcement_level_ = level; }
  TestPriority enforcement_level() const { return enforcement_level_; }

  // Adds the entry `test_name`, a test name or a wildcard, as expected to fail
  // with `failure_message` (see VerdictOnFailure()).  Returns an error if the
  // entry is already present or contains an invalid wildcard.
  absl::Status Add(absl::string_view test_name,
                   absl::string_view failure_message);

  // The verdict on a test that succeeded: an error naming the entry it
  // matched if the test was expected to fail.
  absl::Status VerdictOnSuccess(absl::string_view test_name) const;

  // The verdict on a test that failed with `failure_message`.  OK if the
  // failure doesn't fail the suite.  That is the case when the test is in the
  // failure list with a matching message (an expected failure), or when it
  // isn't listed and `priority` is above the enforcement level (a tolerated
  // failure).  Otherwise the failure is unexpected and the error says why.  A
  // listed test is checked whatever its priority, so that the failure list
  // can't go stale unnoticed.  The normalized actual message only needs to
  // start with the expected message, so an empty expected message matches any
  // failure.
  absl::Status VerdictOnFailure(absl::string_view test_name,
                                TestPriority priority,
                                absl::string_view failure_message) const;

  // The verdict on a test the testee skipped for `skip_reason`: an error
  // naming the entry it matched if the test is in the failure list, since a
  // skipped test can't be an expected failure.  Whether that error fails the
  // test is the caller's policy; Yields() fails such a test.
  absl::Status VerdictOnSkip(absl::string_view test_name,
                             absl::string_view skip_reason) const;

  // The failure list entry `test_name` matches, possibly a wildcard, if any.
  absl::optional<std::string> MatchingEntry(absl::string_view test_name) const;

  // The expected failure message of the entry `entry`, normalized (see
  // FormatFailureMessage()).  `entry` is an entry exactly as listed, wildcards
  // included, not a test name to match.  Returns nullopt if there is no such
  // entry.
  absl::optional<std::string> ExpectedMessage(absl::string_view entry) const;

 private:
  FailureListTrieNode entries_;
  absl::flat_hash_map<std::string, std::string> expected_messages_;
  TestPriority enforcement_level_ = kLowestPriority;
};

}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_FAILURE_LIST_H__
