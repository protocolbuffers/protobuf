// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_FORK_PIPE_RUNNER_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_FORK_PIPE_RUNNER_H__

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "absl/types/optional.h"
#include "absl/types/span.h"
#include "conformance/conformance.pb.h"
#include "conformance/test_runner.h"

namespace google {
namespace protobuf {

// Test runner that spawns the process being tested and communicates with it
// over a pipe.
//
// The testee is spawned by the first RunTest() and lives until the runner is
// destroyed.  The destructor closes the pipes (every conformance testee exits
// on EOF on its stdin), gives the testee a few seconds to exit, and kills it if
// it hasn't.  Leaving it to the end of the process is not an option: several
// runners can be created in one process (one per test phase), and some
// testees busy-poll stdin while idle.
class ForkPipeRunner : public ConformanceTestRunner {
 public:
  static constexpr absl::Duration kDefaultShutdownGracePeriod =
      absl::Seconds(5);
  static constexpr absl::Duration kDefaultReadTimeout = absl::Seconds(30);

  // The testee, `executable` run with `executable_args`, is spawned by the
  // first RunTest().  It is given `read_timeout` to produce each response
  // before the test is failed as timed out, and `shutdown_grace_period` to
  // exit after its stdin is closed before it is killed.
  ForkPipeRunner(
      absl::string_view executable,
      absl::Span<const std::string> executable_args,
      absl::Duration shutdown_grace_period = kDefaultShutdownGracePeriod,
      absl::Duration read_timeout = kDefaultReadTimeout);

  explicit ForkPipeRunner(absl::string_view executable);

  ~ForkPipeRunner() override;

  ForkPipeRunner(const ForkPipeRunner&) = delete;
  ForkPipeRunner& operator=(const ForkPipeRunner&) = delete;

  std::string RunTest(absl::string_view test_name,
                      absl::string_view request) override;

 private:
  // Outcome of an attempt to read a fixed number of bytes from the testee.
  enum class ReadResult {
    kOk,       // All requested bytes were read.
    kEof,      // The testee closed its end of the pipe (it exited or crashed).
    kError,    // The read failed.
    kTimeout,  // The testee produced nothing for the whole read timeout.
  };

  struct ShutdownResult {
    // How the testee ended: its wait status (POSIX) or exit code (Windows).
    // nullopt if there was nothing to reap.
    absl::optional<int> wait_status;
    // True if the testee outlived the grace period and the runner killed it.
    bool killed = false;
  };

  // The process and pipe handles of the test program, defined by the
  // platform's implementation file (fork_pipe_runner_posix.cc or
  // fork_pipe_runner_win32.cc) along with the methods below.
  struct State;

  void SpawnTestProgram();

  bool IsTestProgramRunning() const;

  // Closes the pipes to the testee, waits up to `grace_period` for it to exit,
  // kills it if it hasn't, and reaps it.  A no-op if there is no testee.
  ShutdownResult Shutdown(absl::Duration grace_period);

  // Shuts the testee down after a failed exchange and returns `what_failed`
  // (e.g. "child timed out") with how the testee ended appended, if known.
  // The next RunTest() call spawns a fresh testee.
  std::string GetTestProgramFailure(absl::string_view what_failed);
  // Shuts the testee down after a TryRead() that did not return kOk and
  // returns the serialized ConformanceResponse reporting it: a timeout_error
  // for kTimeout, a runtime_error otherwise.
  std::string ReportReadFailure(ReadResult read_result);

  void CheckedWrite(const void* buf, size_t len);
  // Reads exactly `len` bytes from the testee, giving up if it produces
  // nothing for `read_timeout_`.  Never blocks indefinitely.
  ReadResult TryRead(void* buf, size_t len);
  void CheckedRead(void* buf, size_t len);

  std::string executable_;
  const std::vector<std::string> executable_args_;
  const absl::Duration shutdown_grace_period_;
  const absl::Duration read_timeout_;
  std::string current_test_name_;
  std::unique_ptr<State> state_;
};

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_FORK_PIPE_RUNNER_H__
