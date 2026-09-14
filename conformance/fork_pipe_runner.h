// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_FORK_PIPE_RUNNER_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_FORK_PIPE_RUNNER_H__

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "conformance/conformance.pb.h"
#include "test_runner.h"

namespace google {
namespace protobuf {

// Test runner that spawns the process being tested and communicates with it
// over a pipe.
class ForkPipeRunner : public ConformanceTestRunner {
 public:
  ForkPipeRunner(absl::string_view executable,
                 absl::Span<const std::string> executable_args);

  explicit ForkPipeRunner(const std::string& executable);

  ~ForkPipeRunner() override;

  ForkPipeRunner(const ForkPipeRunner&) = delete;
  ForkPipeRunner& operator=(const ForkPipeRunner&) = delete;

  std::string RunTest(absl::string_view test_name,
                      absl::string_view request) override;

 private:
  // Platform-specific process and pipe handles live in the implementation files.
  struct State;

  void SpawnTestProgram();

  bool IsTestProgramRunning() const;
  void CloseTestProgram();
  std::string GetTestProgramFailure(bool timed_out);
  void CheckedWrite(const void* buf, size_t len);
  bool TryRead(void* buf, size_t len, bool* timed_out);
  void CheckedRead(void* buf, size_t len);

  std::string executable_;
  const std::vector<std::string> executable_args_;
  std::string current_test_name_;
  State* state_;
};

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_FORK_PIPE_RUNNER_H__
