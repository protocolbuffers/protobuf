// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/in_process_test_runner.h"

#include <string>
#include <utility>

#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "conformance/conformance.pb.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

// Returns a serialized ConformanceResponse whose `runtime_error` is `message`.
std::string RuntimeError(absl::string_view message) {
  ::conformance::ConformanceResponse response;
  response.set_runtime_error(message);
  return response.SerializeAsString();
}

}  // namespace

InProcessTestRunner::InProcessTestRunner(RequestHandler handler)
    : handler_(std::move(handler)) {}

InProcessTestRunner::~InProcessTestRunner() = default;

std::string InProcessTestRunner::RunTest(absl::string_view test_name,
                                         absl::string_view input) {
  ::conformance::ConformanceRequest request;
  if (!request.ParseFromString(input)) {
    return RuntimeError(absl::StrCat(
        "failed to parse ConformanceRequest for test ", test_name));
  }

  absl::StatusOr<::conformance::ConformanceResponse> response =
      handler_(request);
  if (!response.ok()) {
    return RuntimeError(absl::StrCat("request handler failed for test ",
                                     test_name, ": ",
                                     response.status().ToString()));
  }
  return response->SerializeAsString();
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google
