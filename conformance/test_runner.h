// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_TEST_RUNNER_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_TEST_RUNNER_H__

#include <cstdint>
#include <string>

#include "absl/strings/string_view.h"

namespace google {
namespace protobuf {
namespace conformance {

// The `skipped` reason a filtering runner answers with for every test that
// was not selected to run.  The merged runner's FilteringTestRunner does so
// under --test.  Such a response is not a testee skip.  The conformance
// matchers (see matchers.h) pass it silently, without logging or counting it,
// and only tell the TestManager that the test exists (see
// TestManager::ReportNotSelected()).  The value is therefore reserved.  A
// testee must never answer with it, or its skip would go unnoticed.
inline constexpr absl::string_view kTestNotSelectedSkipReason =
    "Not selected by --test.";

// Interface for the underlying test runner that runs a single conformance test.
class ConformanceTestRunner {
 public:
  virtual ~ConformanceTestRunner() = default;

  // Call to run a single conformance test.
  //
  // "test_name" is the name of the test to run.
  // "input" is a serialized conformance.ConformanceRequest.
  // The returned string should be a serialized conformance.ConformanceResponse.
  //
  // If there is any error in running the test itself, set "runtime_error" in
  // the response.
  virtual std::string RunTest(absl::string_view test_name,
                              absl::string_view input) = 0;
};

}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_TEST_RUNNER_H__
