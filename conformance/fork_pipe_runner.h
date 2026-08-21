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
                 absl::Span<const std::string> executable_args)
      : child_pid_(-1),
        executable_(executable),
        executable_args_(executable_args.begin(), executable_args.end()) {}

  explicit ForkPipeRunner(const std::string& executable)
      : child_pid_(-1), executable_(executable) {}

  ~ForkPipeRunner() override = default;

  std::string RunTest(absl::string_view test_name,
                      absl::string_view request) override;

 private:
  void SpawnTestProgram();

  void CheckedWrite(int fd, const void* buf, size_t len);
  bool TryRead(int fd, void* buf, size_t len);
  void CheckedRead(int fd, void* buf, size_t len);

  int write_fd_;
  int read_fd_;
  pid_t child_pid_;
  std::string executable_;
  const std::vector<std::string> executable_args_;
  std::string current_test_name_;
};

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_FORK_PIPE_RUNNER_H__
