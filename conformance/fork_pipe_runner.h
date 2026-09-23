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

// Knobs of ForkPipeRunner; the defaults suit real testees, tests shorten them.
struct ForkPipeRunnerOptions {
  // How long the testee has to produce each response before the test is
  // failed as timed out.
  absl::Duration read_timeout = absl::Seconds(30);
  // How long the testee has to exit after its stdin is closed before it is
  // killed.
  absl::Duration shutdown_grace_period = absl::Seconds(5);
};

// Test runner that spawns the process being tested and communicates with it
// over a pipe.
//
// The testee is spawned by the first RunTest() and lives until the runner is
// destroyed.  The destructor closes the pipes (every conformance testee exits
// on EOF on its stdin), gives the testee a few seconds to exit, and kills it if
// it hasn't.  Leaving it to the end of the process is not an option: several
// runners can be created in one process (one per test phase), and some
// testees busy-poll stdin while idle.
//
// On POSIX the testee is made the leader of its own process group, so that the
// SIGKILL (and the diagnostic SIGQUIT sent when it stops answering) reaches
// anything it forked, e.g. the real testee behind a wrapper script that does
// not exec it; once the testee has exited, whatever it left behind in the
// group is killed too.  Two trade-offs: the testee is not in the terminal's
// foreground process group, so an interactive Ctrl-C reaches the runner but
// not the testee (which then only learns that the run is over from EOF on its
// stdin); and a wrapper script that does not exec the testee is terminated by
// the SIGQUIT, which is acceptable since the testee is shut down right after.
// On Windows the testee runs in a job object that is terminated as a whole.
class ForkPipeRunner : public ConformanceTestRunner {
 public:
  // The testee, `executable` run with `executable_args`, is spawned by the
  // first RunTest().
  ForkPipeRunner(absl::string_view executable,
                 absl::Span<const std::string> executable_args,
                 ForkPipeRunnerOptions options = {});
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
  // kills it if it hasn't, and reaps it.  Anything it spawned is killed with
  // it (its process group on POSIX, its job object on Windows).  A no-op if
  // there is no testee.
  ShutdownResult Shutdown(absl::Duration grace_period);

  // Shuts the testee down after a failed exchange and returns `what_failed`
  // (e.g. "child timed out") with how the testee ended appended, if known.
  // The next RunTest() call spawns a fresh testee.
  std::string GetTestProgramFailure(absl::string_view what_failed);
  // Shuts the testee down after a failed exchange and returns the serialized
  // ConformanceResponse reporting it: a timeout_error if `timed_out`, a
  // runtime_error otherwise.
  std::string ReportFailure(bool timed_out, absl::string_view what_failed);
  // ReportFailure() for a TryRead() that did not return kOk.
  std::string ReportReadFailure(ReadResult read_result);

  // Writes all `len` bytes of `buf` to the testee.  Returns false if the write
  // failed, in particular because the testee has exited (EPIPE on POSIX, where
  // SIGPIPE is blocked on the calling thread for the duration of the write; a
  // broken-pipe error on Windows), so that a testee that dies between two
  // requests is reported instead of killing the runner.
  bool TryWrite(const void* buf, size_t len);
  // Reads exactly `len` bytes from the testee, giving up if it produces
  // nothing for `options_.read_timeout`.  Never blocks indefinitely.
  ReadResult TryRead(void* buf, size_t len);

  std::string executable_;
  const std::vector<std::string> executable_args_;
  const ForkPipeRunnerOptions options_;
  std::string current_test_name_;
  std::unique_ptr<State> state_;
};

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_FORK_PIPE_RUNNER_H__
