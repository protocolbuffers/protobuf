// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_IN_PROCESS_TEST_RUNNER_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_IN_PROCESS_TEST_RUNNER_H__

#include <string>

#include "absl/functional/any_invocable.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "conformance/conformance.pb.h"
#include "conformance/test_runner.h"

namespace google {
namespace protobuf {
namespace conformance {

// A ConformanceTestRunner that dispatches each request to an in-process
// callback instead of a testee subprocess. Parses the serialized request,
// invokes `handler`, and serializes the response.
//
// If the request cannot be parsed, or `handler` returns a non-OK status, the
// returned response has `runtime_error` set describing the failure.
//
// Any state referenced by `handler` (e.g. a captured harness) must outlive the
// runner. Like every other ConformanceTestRunner, this class is meant for
// single-threaded use: RunTest is not thread-safe if `handler` mutates state.
class InProcessTestRunner : public ConformanceTestRunner {
 public:
  using RequestHandler =
      absl::AnyInvocable<absl::StatusOr<::conformance::ConformanceResponse>(
          const ::conformance::ConformanceRequest&)>;

  explicit InProcessTestRunner(RequestHandler handler);
  ~InProcessTestRunner() override;

  InProcessTestRunner(const InProcessTestRunner&) = delete;
  InProcessTestRunner& operator=(const InProcessTestRunner&) = delete;

  std::string RunTest(absl::string_view test_name,
                      absl::string_view input) override;

 private:
  RequestHandler handler_;
};

}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_IN_PROCESS_TEST_RUNNER_H__
