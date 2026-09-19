// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// The default testee of a conformance test binary: --testee_binary, run as a
// child process (see testee_runner.h).

#include <memory>
#include <string>

#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "fork_pipe_runner.h"
#include "test_runner.h"
#include "testee_runner.h"

namespace google {
namespace protobuf {
namespace conformance {

std::unique_ptr<ConformanceTestRunner> MakeTesteeRunner(
    absl::string_view testee_binary,
    absl::Span<const std::string> testee_args) {
  ABSL_QCHECK(!testee_binary.empty()) << "--testee_binary is required.";
  // Spawns the testee lazily, on the first request.
  return std::make_unique<ForkPipeRunner>(testee_binary, testee_args);
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google
