// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Declares the process-wide state the Yields() matcher works against (see
// matchers.h).  The definitions are deliberately not provided here.  They are
// the link-time seam between the matchers and the process that hosts them.
// The conformance test binary provides them from its global test environment,
// populated from command-line flags and the failure list on disk.  Unit tests
// of the matchers provide a lightweight in-memory stand-in.

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_GLOBAL_TEST_ENVIRONMENT_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_GLOBAL_TEST_ENVIRONMENT_H__

#include "absl/strings/string_view.h"
#include "conformance/failure_list.h"
#include "conformance/testee.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {

// Returns the process-wide expected-failure list.  Yields() asks it for the
// verdict on every test, which applies the expected failures and the
// enforcement level (see FailureList in failure_list.h).  The list never
// changes once the tests run; the test environment owns it as part of its
// ResultLedger (see ConformanceEnvironment in test_environment.h).
const FailureList& GetGlobalFailureList();

// Report the outcome Yields() decided for the conformance test `test_name` to
// the process-wide ResultLedger, which counts it; they mirror its Report*()
// methods (see result_ledger.h).  This is the one way the matchers change
// anything.  The ledger counts each test once, so reporting a test again
// changes nothing; gtest does evaluate a failing matcher a second time to
// explain the failure.
//
// A process-wide ledger is safe because gtest runs the tests of a binary one
// after the other; a sharded run is several processes, each with its own
// environment and ledger, and the environment knows it only saw a subset (see
// ConformanceEnvironment::TearDown()).  Checking results from several threads
// at once is not supported, and neither is running them: a testee connection
// is a single pipe (see Testee in testee.h).
void ReportSuccess(absl::string_view test_name);
void ReportFailure(absl::string_view test_name, TestPriority priority,
                   absl::string_view failure_message);
void ReportSkip(absl::string_view test_name, absl::string_view skip_reason);

}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_GLOBAL_TEST_ENVIRONMENT_H__
