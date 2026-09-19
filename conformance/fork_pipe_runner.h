// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_FORK_PIPE_RUNNER_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_FORK_PIPE_RUNNER_H__

#include <sys/types.h>

#include <chrono>  // NOLINT(build/c++11)
#include <cstddef>
#include <string>
#include <vector>

#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "absl/types/span.h"
#include "conformance/conformance.pb.h"
#include "test_runner.h"

namespace google {
namespace protobuf {

// Test runner that spawns the process being tested and communicates with it
// over a pipe.
//
// The testee is spawned lazily on the first call to RunTest() and lives for
// the lifetime of the runner.  The destructor shuts it down: it closes the
// pipes (so the testee sees EOF on stdin, on which every conformance testee
// exits), waits a bounded amount of time for it to exit, and SIGKILLs it if
// it has not.  This matters when several runners are created in one process
// (e.g. one per test phase): some testees busy-poll stdin while idle and
// would otherwise keep burning a CPU until the parent process exits.  Once
// the testee has exited, whatever it left behind in its process group is
// SIGKILLed too, before the testee is reaped.
//
// The testee is made the leader of its own process group before it execs, so
// that the SIGKILL, and the diagnostic SIGQUIT sent when the testee stops
// answering (see RunTest()), can be sent to the whole group and reach anything
// the testee forked, e.g. the real testee behind a wrapper script that does
// not exec it.  Two trade-offs: the testee is no longer in the terminal's
// foreground process group, so an interactive Ctrl-C reaches the runner but
// not the testee, which then only learns that the run is over from EOF on its
// stdin once the runner has exited (conformance testees only ever read stdin,
// so being in a background process group otherwise makes no difference to
// them); and a wrapper script that does not exec the testee is terminated by
// the SIGQUIT (a shell's default action for it), which is acceptable since
// the testee is shut down right after anyway.
class ForkPipeRunner : public ConformanceTestRunner {
 public:
  ForkPipeRunner(absl::string_view executable,
                 absl::Span<const std::string> executable_args)
      : executable_(executable),
        executable_args_(executable_args.begin(), executable_args.end()) {}

  explicit ForkPipeRunner(const std::string& executable)
      : executable_(executable) {}

  ForkPipeRunner(const ForkPipeRunner&) = delete;
  ForkPipeRunner& operator=(const ForkPipeRunner&) = delete;

  ~ForkPipeRunner() override;

  std::string RunTest(absl::string_view test_name,
                      absl::string_view request) override;

 private:
  friend class ForkPipeRunnerPeer;

  // Outcome of an attempt to read a fixed number of bytes from the testee.
  enum class ReadResult {
    kOk,       // All requested bytes were read.
    kEof,      // The testee closed its end of the pipe (it exited or crashed).
    kError,    // read() failed.
    kTimeout,  // The testee produced nothing for the whole read timeout.
  };

  // What Shutdown() found out about the testee.
  struct ShutdownResult {
    // The testee's wait status (as filled in by waitpid()), or nullopt if
    // there was nothing to reap.
    absl::optional<int> wait_status;
    // True if the testee did not exit within the grace period and the runner
    // SIGKILLed it (in which case `wait_status` normally says so too).  Not
    // set by the sweep of the process group after the testee exited on its
    // own.
    bool killed = false;
  };

  void SpawnTestProgram();

  // Closes the pipes to the testee (if open) and reaps it (if still running).
  // The testee is given `grace_period` to exit on its own after its pipes are
  // closed, after which its process group is SIGKILLed; a zero grace period
  // kills it at once.  Either way, once the testee has exited the rest of its
  // process group (anything it forked and left behind) is SIGKILLed before the
  // testee is reaped.  Safe to call more than once and when no testee was ever
  // spawned.
  ShutdownResult Shutdown(std::chrono::milliseconds grace_period);

  // Writes all `len` bytes of `buf` to `fd`.  Returns false if the write
  // failed, in particular with EPIPE because the testee has exited: SIGPIPE
  // is blocked on the calling thread for the duration of the write (and the
  // one it raises is discarded), so a testee that dies between two requests
  // is reported instead of killing the runner.  The caller is expected to
  // ReportFailure() on failure.
  bool TryWrite(int fd, const void* buf, size_t len);
  // Reads exactly `len` bytes from `fd`, giving up if the testee produces
  // nothing for `read_timeout_` (in which case it is sent SIGQUIT and whatever
  // it prints in response is logged, for a bounded time).  Never blocks
  // indefinitely; the caller is expected to ReportReadFailure() on failure.
  ReadResult TryRead(int fd, void* buf, size_t len);

  // Shuts the testee down and returns a serialized ConformanceResponse that
  // reports why the exchange with it failed, classified from its wait status
  // (exited with a status, killed by a signal, killed by the runner).  With
  // `timed_out` the report is a timeout_error and the testee, which has just
  // been sent SIGQUIT, is given a moment to die of that before it is
  // SIGKILLed; otherwise the report is a runtime_error and the testee is
  // killed at once.  `fallback_error` is reported when there was no wait
  // status to classify.  The next RunTest() call spawns a fresh testee.
  std::string ReportFailure(bool timed_out, absl::string_view fallback_error);
  // ReportFailure() for a TryRead() that did not return kOk: a timeout_error
  // for kTimeout, otherwise a runtime_error whose fallback is "child closed
  // its output without responding" for kEof and "error reading from child"
  // for kError.
  std::string ReportReadFailure(ReadResult read_result);

  // Ends of the to-testee and from-testee pipes owned by this process; -1 when
  // not open.
  int write_fd_ = -1;
  int read_fd_ = -1;
  // -1 when there is no live, unreaped testee.
  pid_t child_pid_ = -1;
  // How long the destructor lets a testee exit on its own before SIGKILLing
  // it.  Overridable (via ForkPipeRunnerPeer) so tests need not wait it out.
  std::chrono::milliseconds shutdown_grace_period_ = std::chrono::seconds(5);
  // How long a testee may be silent while a response is being read before it
  // is declared hung.  Overridable (via ForkPipeRunnerPeer) so tests of the
  // timeout path need not wait it out.
  std::chrono::milliseconds read_timeout_ = std::chrono::seconds(30);
  std::string executable_;
  const std::vector<std::string> executable_args_;
  std::string current_test_name_;
};

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_FORK_PIPE_RUNNER_H__
