// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/fork_pipe_runner.h"

#ifndef _WIN32

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

#include "absl/log/absl_log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "absl/types/optional.h"
#include "absl/types/span.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define CHECK_SYSCALL(call)                              \
  do {                                                   \
    if (call < 0) {                                      \
      perror(#call " " __FILE__ ":" TOSTRING(__LINE__)); \
      exit(1);                                           \
    }                                                    \
  } while (0)
// For use between fork() and execv() in the child of a multi-threaded process,
// where only async-signal-safe calls are allowed: no perror() (it uses stdio
// and may take locks held by another thread in the parent) and no exit() (it
// runs atexit handlers and flushes stdio), just a fixed message and _exit().
#define CHECK_SYSCALL_IN_CHILD(call)                                          \
  do {                                                                        \
    if (call < 0) {                                                           \
      static constexpr char kMessage[] =                                      \
          #call " failed in child " __FILE__ ":" TOSTRING(__LINE__) "\n";     \
      /* Best effort; there is nothing left to do if this fails too. */       \
      ssize_t written = write(STDERR_FILENO, kMessage, sizeof(kMessage) - 1); \
      (void)written;                                                          \
      _exit(1);                                                               \
    }                                                                         \
  } while (0)

namespace google {
namespace protobuf {
namespace {

// How often Shutdown() checks whether the testee has exited.
constexpr absl::Duration kPollInterval = absl::Milliseconds(10);
// How long Shutdown() waits for a SIGKILLed testee to be reapable before it
// gives up rather than block the runner.
constexpr absl::Duration kKillWait = absl::Seconds(5);
// How long a testee that failed to answer gets to finish dying before it is
// killed, so that one that has just crashed (or is dumping core on the SIGQUIT
// sent after a timeout) is reported with its own wait status.
constexpr absl::Duration kFailureGracePeriod = absl::Seconds(1);
// Bounds on the output logged after a read timeout, see ReadSigquitOutput().
constexpr absl::Duration kSigquitOutputTimeout = absl::Seconds(2);
constexpr size_t kSigquitOutputMaxBytes = 5000;

// Waits until `fd` is readable (data available, EOF or error) or `timeout`
// elapses.  Returns false on timeout, and also if poll() itself fails with
// anything but EINTR: the caller then treats the testee as hung rather than
// risk a read() that blocks indefinitely.
bool WaitReadable(int fd, absl::Duration timeout) {
  const absl::Time deadline = absl::Now() + timeout;
  while (true) {
    pollfd pfd = {};
    pfd.fd = fd;
    pfd.events = POLLIN;
    const int64_t remaining_ms =
        absl::ToInt64Milliseconds(deadline - absl::Now());
    const int ready =
        poll(&pfd, 1,
             static_cast<int>(std::clamp<int64_t>(remaining_ms, 0, INT_MAX)));
    if (ready > 0) return true;
    if (ready == 0) return false;
    if (errno == EINTR) continue;
    ABSL_LOG(ERROR) << "poll() on the testee's pipe failed: "
                    << strerror(errno);
    return false;
  }
}

// Reads what the testee prints in response to SIGQUIT (e.g. a JVM thread
// dump), for logging.  Stops at EOF, after `kSigquitOutputTimeout` or once
// `kSigquitOutputMaxBytes` have been read, so a testee that ignores SIGQUIT and
// keeps its stdout open cannot hang the runner.
std::string ReadSigquitOutput(int fd) {
  std::string out;
  const absl::Time deadline = absl::Now() + kSigquitOutputTimeout;
  char buf[1024];
  while (out.size() < kSigquitOutputMaxBytes &&
         WaitReadable(fd, deadline - absl::Now())) {
    const ssize_t bytes_read = read(
        fd, buf, std::min(sizeof(buf), kSigquitOutputMaxBytes - out.size()));
    if (bytes_read < 0 && errno == EINTR) continue;
    if (bytes_read <= 0) break;  // EOF or error.
    out.append(buf, static_cast<size_t>(bytes_read));
  }
  return out.empty() ? "(no output)" : out;
}

// Sends `sig` to the testee's process group (it made itself the leader of one
// in SpawnTestProgram()), so that it also reaches anything the testee forked,
// e.g. the real testee behind a wrapper script that does not exec it.
// kill(-pid) fails with ESRCH if the group does not exist, which can only
// happen if the child has not reached its setpgid() yet; the child alone is
// signalled then.
void SignalTestee(pid_t pid, int sig) {
  if (kill(-pid, sig) != 0 && kill(pid, sig) != 0) {
    ABSL_LOG(ERROR) << "kill(" << pid << ", " << sig
                    << ") failed: " << strerror(errno);
  }
}

// Polls for up to `timeout` for the testee `pid` to exit.  Once it has, kills
// whatever it left behind in its process group (e.g. a wrapper script that
// exits on EOF after backgrounding the real testee leaves that behind) and
// reaps it.  Returns its wait status, or nullopt if it is still running when
// the timeout expires (or if there is no such child to reap).
absl::optional<int> WaitForExit(pid_t pid, absl::Duration timeout) {
  const absl::Time deadline = absl::Now() + timeout;
  while (true) {
    // Look at the testee's state without reaping it: until it is reaped its
    // pid cannot be reused, so the group id (which is that pid) still
    // unambiguously names its group.
    siginfo_t info = {};
    if (waitid(P_PID, static_cast<id_t>(pid), &info,
               WEXITED | WNOHANG | WNOWAIT) != 0) {
      if (errno == EINTR) continue;
      // ECHILD: something else in this process already reaped it.
      if (errno != ECHILD) {
        ABSL_LOG(WARNING) << "waitid(" << pid
                          << ") failed: " << strerror(errno);
      }
      return absl::nullopt;
    }
    if (info.si_pid == pid) break;  // Exited, not yet reaped.
    if (absl::Now() >= deadline) return absl::nullopt;
    absl::SleepFor(kPollInterval);
  }
  // A no-op for the zombie leader itself; ESRCH means there is no such group
  // (the child died before its setpgid()).
  if (kill(-pid, SIGKILL) != 0 && errno != ESRCH) {
    ABSL_LOG(WARNING) << "kill(-" << pid
                      << ", SIGKILL) failed: " << strerror(errno);
  }
  int status = 0;
  pid_t reaped;
  do {
    reaped = waitpid(pid, &status, 0);
  } while (reaped < 0 && errno == EINTR);
  if (reaped != pid) {
    ABSL_LOG(WARNING) << "waitpid(" << pid << ") failed: " << strerror(errno);
    return absl::nullopt;
  }
  return status;
}

}  // namespace

struct ForkPipeRunner::State {
  // Ends of the to-testee and from-testee pipes owned by this process; -1 when
  // not open.
  int write_fd = -1;
  int read_fd = -1;
  // -1 when there is no live, unreaped testee.
  pid_t child_pid = -1;
};

ForkPipeRunner::ForkPipeRunner(absl::string_view executable,
                               absl::Span<const std::string> executable_args,
                               absl::Duration shutdown_grace_period,
                               absl::Duration read_timeout)
    : executable_(executable),
      executable_args_(executable_args.begin(), executable_args.end()),
      shutdown_grace_period_(shutdown_grace_period),
      read_timeout_(read_timeout),
      state_(std::make_unique<State>()) {}

ForkPipeRunner::ForkPipeRunner(absl::string_view executable)
    : ForkPipeRunner(executable, {}) {}

ForkPipeRunner::~ForkPipeRunner() { Shutdown(shutdown_grace_period_); }

bool ForkPipeRunner::IsTestProgramRunning() const {
  return state_->child_pid >= 0;
}

void ForkPipeRunner::SpawnTestProgram() {
  int toproc_pipe_fd[2];
  int fromproc_pipe_fd[2];
  if (pipe(toproc_pipe_fd) < 0 || pipe(fromproc_pipe_fd) < 0) {
    perror("pipe");
    exit(1);
  }
  // Keep the pipes out of any other program this process execs, such as the
  // testee of a second ForkPipeRunner: a stray copy of the write end would hold
  // this testee's stdin open after Shutdown() closes ours, so it would never
  // see EOF.  The child's dup2() below clears the flag on the ends it uses.
  // (pipe2(O_CLOEXEC) would be atomic, but macOS doesn't have it.)
  for (int fd : {toproc_pipe_fd[0], toproc_pipe_fd[1], fromproc_pipe_fd[0],
                 fromproc_pipe_fd[1]}) {
    CHECK_SYSCALL(fcntl(fd, F_SETFD, FD_CLOEXEC));
  }

  // Build argv and log it *before* fork(): this process is multi-threaded, so
  // the child must only make async-signal-safe calls (no logging) until execv.
  std::vector<const char*> argv;
  argv.reserve(executable_args_.size() + 2);
  argv.push_back(executable_.c_str());
  ABSL_LOG(INFO) << argv[0];
  for (size_t i = 0; i < executable_args_.size(); ++i) {
    argv.push_back(executable_args_[i].c_str());
    ABSL_LOG(INFO) << executable_args_[i];
  }
  argv.push_back(nullptr);

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    exit(1);
  }

  if (pid) {
    // Parent.
    CHECK_SYSCALL(close(toproc_pipe_fd[0]));
    CHECK_SYSCALL(close(fromproc_pipe_fd[1]));
    state_->write_fd = toproc_pipe_fd[1];
    state_->read_fd = fromproc_pipe_fd[0];
    state_->child_pid = pid;
  } else {
    // Child.
    CHECK_SYSCALL_IN_CHILD(close(STDIN_FILENO));
    CHECK_SYSCALL_IN_CHILD(close(STDOUT_FILENO));
    CHECK_SYSCALL_IN_CHILD(dup2(toproc_pipe_fd[0], STDIN_FILENO));
    CHECK_SYSCALL_IN_CHILD(dup2(fromproc_pipe_fd[1], STDOUT_FILENO));

    CHECK_SYSCALL_IN_CHILD(close(toproc_pipe_fd[0]));
    CHECK_SYSCALL_IN_CHILD(close(fromproc_pipe_fd[1]));
    CHECK_SYSCALL_IN_CHILD(close(toproc_pipe_fd[1]));
    CHECK_SYSCALL_IN_CHILD(close(fromproc_pipe_fd[0]));

    // Become the leader of a new process group (setpgid() is
    // async-signal-safe), so that Shutdown() can SIGKILL the group and reach
    // any process the testee forks.  See the class comment for the trade-offs.
    CHECK_SYSCALL_IN_CHILD(setpgid(0, 0));

    // Never returns.
    CHECK_SYSCALL_IN_CHILD(
        execv(executable_.c_str(), const_cast<char**>(argv.data())));
  }
}

ForkPipeRunner::ShutdownResult ForkPipeRunner::Shutdown(
    absl::Duration grace_period) {
  // Closing our end of the testee's stdin makes it see EOF, which is how a
  // conformance testee learns that the run is over.  Closing our end of its
  // stdout too means a testee blocked writing to a full pipe gets
  // SIGPIPE/EPIPE instead of hanging.  Resetting the fds and the pid makes a
  // second call (e.g. the destructor after RunTest()'s crash path) a no-op.
  if (state_->write_fd >= 0) {
    close(state_->write_fd);
    state_->write_fd = -1;
  }
  if (state_->read_fd >= 0) {
    close(state_->read_fd);
    state_->read_fd = -1;
  }

  ShutdownResult result;
  const pid_t pid = state_->child_pid;
  if (pid <= 0) return result;
  state_->child_pid = -1;

  result.wait_status = WaitForExit(pid, grace_period);
  if (!result.wait_status.has_value()) {
    // A blocking wait would let a testee that ignores EOF hang the runner;
    // SIGKILL cannot be ignored.
    ABSL_LOG(WARNING) << "child pid=" << pid << " has not exited "
                      << absl::FormatDuration(grace_period)
                      << " after its pipes were closed, sending SIGKILL";
    SignalTestee(pid, SIGKILL);
    result.killed = true;
    result.wait_status = WaitForExit(pid, kKillWait);
    if (!result.wait_status.has_value()) {
      ABSL_LOG(ERROR) << "giving up on child pid=" << pid
                      << ", which has not exited";
    }
  }
  return result;
}

std::string ForkPipeRunner::GetTestProgramFailure(
    absl::string_view what_failed) {
  ABSL_LOG(INFO) << "Trying to reap child, pid=" << state_->child_pid;
  const ShutdownResult shutdown = Shutdown(kFailureGracePeriod);

  std::string error_msg(what_failed);
  // Say how the testee ended, if known.  A kill by the runner is reported as
  // such so that it is not mistaken for the testee crashing on its own.
  if (shutdown.killed) {
    absl::StrAppend(&error_msg, " (killed by runner)");
  } else if (shutdown.wait_status.has_value()) {
    const int status = *shutdown.wait_status;
    if (WIFSIGNALED(status)) {
      absl::StrAppendFormat(&error_msg, " (killed by signal %d)",
                            WTERMSIG(status));
    } else if (WIFEXITED(status)) {
      absl::StrAppendFormat(&error_msg, " (exited with status=%d)",
                            WEXITSTATUS(status));
    }
  }
  return error_msg;
}

bool ForkPipeRunner::TryWrite(const void* buf, size_t len) {
  // Writing to a pipe whose reader has exited raises SIGPIPE, whose default
  // action would kill the runner.  The signal is directed at the writing
  // thread, so blocking it on this thread alone for the duration of the write
  // turns it into an EPIPE error return without changing the process-wide
  // disposition (which the testee would inherit across exec).  A blocked
  // SIGPIPE stays pending until it is consumed or unblocked, so it is consumed
  // before the mask is restored.
  sigset_t sigpipe;
  sigemptyset(&sigpipe);
  sigaddset(&sigpipe, SIGPIPE);
  sigset_t old_mask;
  pthread_sigmask(SIG_BLOCK, &sigpipe, &old_mask);

  const char* in = static_cast<const char*>(buf);
  size_t ofs = 0;
  int error = 0;
  while (ofs < len) {
    const ssize_t written = write(state_->write_fd, in + ofs, len - ofs);
    if (written < 0) {
      if (errno == EINTR) continue;
      error = errno;
      break;
    }
    ofs += static_cast<size_t>(written);
  }

  if (error == EPIPE && !sigismember(&old_mask, SIGPIPE)) {
    // Discard the SIGPIPE the failed write left pending for this thread.  This
    // cannot block: the signal is thread-directed and blocked here, so if
    // sigpending() reports it, sigwait() takes it at once; if the process
    // ignores SIGPIPE none was raised and sigwait() is not called.  (If the
    // caller had SIGPIPE blocked already the signal is left for the caller.)
    sigset_t pending;
    sigemptyset(&pending);
    sigpending(&pending);
    if (sigismember(&pending, SIGPIPE)) {
      int sig;
      while (sigwait(&sigpipe, &sig) == EINTR) {
      }
    }
  }
  pthread_sigmask(SIG_SETMASK, &old_mask, nullptr);

  if (error != 0) {
    ABSL_LOG(ERROR) << current_test_name_
                    << ": error writing to test program: " << strerror(error);
    return false;
  }
  return true;
}

ForkPipeRunner::ReadResult ForkPipeRunner::TryRead(void* buf, size_t len) {
  // The timeout is implemented with poll() on the calling thread rather than
  // by racing a blocking read() on a helper thread against a deadline: a
  // helper thread stuck in read() on a testee that never answers would have to
  // be joined (blocking us) or leaked.
  const int fd = state_->read_fd;
  char* out = static_cast<char*>(buf);
  size_t ofs = 0;
  while (ofs < len) {
    if (!WaitReadable(fd, read_timeout_)) {
      ABSL_LOG(ERROR) << current_test_name_ << ": timeout from test program";
      // Some runtimes (notably the JVM) react to SIGQUIT by dumping all their
      // threads' stacks to stdout, i.e. into our pipe; log whatever arrives,
      // for a bounded time, to help diagnose the hang.  The caller then shuts
      // the testee down.
      // TODO: Only log in flag-guarded mode, since reading output from
      // SIGQUIT is slow and verbose.
      SignalTestee(state_->child_pid, SIGQUIT);
      ABSL_LOG(ERROR) << "child pid=" << state_->child_pid << " SIGQUIT: \n"
                      << ReadSigquitOutput(fd);
      return ReadResult::kTimeout;
    }

    const ssize_t bytes_read = read(fd, out + ofs, len - ofs);
    if (bytes_read == 0) {
      ABSL_LOG(ERROR) << current_test_name_
                      << ": unexpected EOF from test program";
      return ReadResult::kEof;
    } else if (bytes_read < 0) {
      if (errno == EINTR) continue;
      ABSL_LOG(ERROR) << current_test_name_
                      << ": error reading from test program: "
                      << strerror(errno);
      return ReadResult::kError;
    }

    ofs += static_cast<size_t>(bytes_read);
  }

  return ReadResult::kOk;
}

}  // namespace protobuf
}  // namespace google

#endif  // !_WIN32
