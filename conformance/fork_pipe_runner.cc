// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// This file contains a program for running the test suite in a separate
// process.  The other alternative is to run the suite in-process.  See
// conformance.proto for pros/cons of these two options.
//
// This program will fork the process under test and communicate with it over
// its stdin/stdout:
//
//     +--------+   pipe   +----------+
//     | tester | <------> | testee   |
//     |        |          |          |
//     |  C++   |          | any lang |
//     +--------+          +----------+
//
// The tester contains all of the test cases and their expected output.
// The testee is a simple program written in the target language that reads
// each test case and attempts to produce acceptable output for it.
//
// Every test consists of a ConformanceRequest/ConformanceResponse
// request/reply pair.  The protocol on the pipe is simply:
//
//   1. tester sends 4-byte length N (little endian)
//   2. tester sends N bytes representing a ConformanceRequest proto
//   3. testee sends 4-byte length M (little endian)
//   4. testee sends M bytes representing a ConformanceResponse proto

#include "fork_pipe_runner.h"

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>  // NOLINT(build/c++11)
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>  // NOLINT(build/c++11)
#include <vector>

#include "absl/log/absl_log.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "conformance/conformance.pb.h"
#include "google/protobuf/endian.h"

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

using Clock = std::chrono::steady_clock;

// Time left until `deadline`, in whole milliseconds, never negative.
std::chrono::milliseconds RemainingUntil(Clock::time_point deadline) {
  return std::max(std::chrono::duration_cast<std::chrono::milliseconds>(
                      deadline - Clock::now()),
                  std::chrono::milliseconds::zero());
}

// Waits until `fd` is readable (data available, EOF or error) or `timeout`
// elapses.  Returns false on timeout, and also if poll() itself fails with
// anything but EINTR (which can only be a programming error such as a bad fd):
// the caller then takes its timeout path rather than risking a read() that
// blocks indefinitely.
bool WaitReadable(int fd, std::chrono::milliseconds timeout) {
  const Clock::time_point deadline = Clock::now() + timeout;
  while (true) {
    const int timeout_ms = static_cast<int>(
        std::min<int64_t>(RemainingUntil(deadline).count(), INT_MAX));
    pollfd pfd = {};
    pfd.fd = fd;
    pfd.events = POLLIN;
    const int ready = poll(&pfd, 1, timeout_ms);
    if (ready > 0) return true;
    if (ready == 0) return false;
    if (errno != EINTR) {
      ABSL_LOG(ERROR) << "poll() on the testee's pipe failed: "
                      << strerror(errno);
      return false;
    }
  }
}

// Reads what the testee prints on its stdout in response to SIGQUIT (e.g. a
// JVM thread dump), for logging.  Every wait is bounded: reading stops at EOF,
// once the testee has been silent for `kIdleTimeout`, after `kTotalTimeout`
// overall, or when `kMaxBytes` have been read, so a testee that ignores
// SIGQUIT and never writes anything cannot hang the runner.
std::string ReadSigquitOutput(int fd) {
  constexpr size_t kMaxBytes = 5000;
  constexpr std::chrono::milliseconds kIdleTimeout = std::chrono::seconds(1);
  constexpr std::chrono::seconds kTotalTimeout(5);

  std::string out(kMaxBytes, '\0');
  size_t ofs = 0;
  const Clock::time_point deadline = Clock::now() + kTotalTimeout;
  while (ofs < out.size()) {
    const std::chrono::milliseconds remaining = RemainingUntil(deadline);
    if (remaining <= std::chrono::milliseconds::zero()) break;
    if (!WaitReadable(fd, std::min(remaining, kIdleTimeout))) break;
    const ssize_t bytes_read = read(fd, &out[ofs], out.size() - ofs);
    if (bytes_read < 0 && errno == EINTR) continue;
    if (bytes_read <= 0) break;  // EOF or error.
    ofs += static_cast<size_t>(bytes_read);
  }
  out.resize(ofs);
  return out;
}

}  // namespace

ForkPipeRunner::~ForkPipeRunner() { Shutdown(shutdown_grace_period_); }

ForkPipeRunner::ShutdownResult ForkPipeRunner::Shutdown(
    std::chrono::milliseconds grace_period) {
  // Closing our end of the testee's stdin makes it see EOF, which is how a
  // conformance testee learns that the run is over and exits.  Closing our end
  // of its stdout too means a testee blocked writing a response to a full pipe
  // gets SIGPIPE/EPIPE instead of hanging forever.  The fds are reset to -1 so
  // a second call (e.g. the destructor after RunTest()'s crash path) is a
  // no-op.
  if (write_fd_ >= 0) {
    close(write_fd_);
    write_fd_ = -1;
  }
  if (read_fd_ >= 0) {
    close(read_fd_);
    read_fd_ = -1;
  }

  ShutdownResult result;
  if (child_pid_ <= 0) return result;
  const pid_t pid = child_pid_;
  child_pid_ = -1;

  // Give the testee a bounded grace period to exit on its own after EOF.  A
  // blocking wait would let a misbehaving testee (one that ignores EOF) hang
  // the runner, so poll with WNOHANG instead and escalate to SIGKILL, which
  // cannot be ignored, once the deadline passes.  Only after a successful
  // SIGKILL do we wait synchronously, and only for the kill to be delivered.
  //
  // The SIGKILL goes to the testee's process group (it made itself the leader
  // of one in SpawnTestProgram()), so it also reaches anything the testee
  // forked, such as the real testee behind a wrapper script that does not
  // exec it.  A testee that exits on its own can leave such processes behind
  // too (e.g. a wrapper script that exits on EOF after backgrounding the real
  // testee), so the group is also swept once the testee has exited.  That has
  // to happen before the testee is reaped: until then its pid cannot be
  // reused, so the group id (which is that pid) still unambiguously names its
  // group, which is why the exit is detected with waitid(WNOWAIT) and the
  // reap is a separate step.
  constexpr std::chrono::milliseconds kPollInterval(10);
  // How long to keep polling for the testee to disappear if kill() failed.
  constexpr std::chrono::seconds kKillFailureWait(1);
  enum class Phase {
    kGrace,       // Waiting for the testee to exit on its own.
    kKilled,      // SIGKILL sent; a blocking waitid() is now safe.
    kKillFailed,  // kill() failed; polling a little longer, then giving up.
  };
  Phase phase = Phase::kGrace;
  auto deadline = std::chrono::steady_clock::now() + grace_period;
  while (true) {
    siginfo_t info = {};
    const int probed =
        waitid(P_PID, pid, &info,
               WEXITED | WNOWAIT | (phase == Phase::kKilled ? 0 : WNOHANG));
    if (probed < 0) {
      if (errno == EINTR) continue;
      // ECHILD means there is no such child to reap (e.g. something else in
      // the process already reaped it).  Anything else is unexpected, but
      // there is nothing useful left to do about it during shutdown.
      if (errno != ECHILD) {
        ABSL_LOG(WARNING) << "waitid(" << pid
                          << ") failed: " << strerror(errno);
      }
      return result;
    }
    if (info.si_pid == pid) {
      // The testee has exited but is not reaped yet.  Sweep whatever it left
      // behind in its process group (unless the group was SIGKILLed already):
      // the signal is a no-op for the zombie leader itself, and ESRCH means
      // there is no such group (the child died before its setpgid()).
      if (!result.killed && kill(-pid, SIGKILL) != 0 && errno != ESRCH) {
        ABSL_LOG(WARNING) << "kill(-" << pid
                          << ", SIGKILL) failed: " << strerror(errno);
      }
      int status = 0;
      pid_t reaped;
      do {
        reaped = waitpid(pid, &status, 0);
      } while (reaped < 0 && errno == EINTR);
      if (reaped == pid) {
        result.wait_status = status;
      } else {
        ABSL_LOG(WARNING) << "waitpid(" << pid
                          << ") failed: " << strerror(errno);
      }
      return result;
    }
    // si_pid == 0: the testee is still running.
    const auto now = std::chrono::steady_clock::now();
    if (now < deadline) {
      std::this_thread::sleep_for(kPollInterval);
      continue;
    }
    if (phase == Phase::kKillFailed) {
      ABSL_LOG(ERROR) << "giving up on child pid=" << pid
                      << ", which could not be killed and has not exited";
      return result;
    }
    if (grace_period > std::chrono::milliseconds::zero()) {
      ABSL_LOG(WARNING) << "child pid=" << pid << " did not exit within "
                        << std::chrono::duration<double>(grace_period).count()
                        << "s of its pipes being closed, sending SIGKILL";
    } else {
      // With no grace period the child may be exiting as we speak (its pipes
      // were closed microseconds ago); the SIGKILL is then merely redundant.
      ABSL_LOG(INFO) << "child pid=" << pid
                     << " has not exited yet, sending SIGKILL (no grace "
                        "period)";
    }
    // kill(-pid) fails with ESRCH if the group does not exist, which can only
    // happen if the child has not reached its setpgid() yet; then signal the
    // child alone.
    if (kill(-pid, SIGKILL) == 0 || kill(pid, SIGKILL) == 0) {
      phase = Phase::kKilled;  // The next waitid() blocks until it lands.
      result.killed = true;
      continue;
    }
    // ESRCH would mean the child is gone, in which case waitid() reports
    // ECHILD (or finds it exited) on the next iteration.  EPERM should be
    // impossible for our own child; rather than risk a blocking waitid() that
    // never returns, poll a little longer and then give up.
    ABSL_LOG(ERROR) << "kill(" << pid
                    << ", SIGKILL) failed: " << strerror(errno);
    phase = Phase::kKillFailed;
    deadline = now + kKillFailureWait;
  }
}

std::string ForkPipeRunner::RunTest(absl::string_view test_name,
                                    absl::string_view request) {
  if (child_pid_ < 0) {
    SpawnTestProgram();
  }
  current_test_name_ = std::string(test_name);

  uint32_t len =
      internal::little_endian::FromHost(static_cast<uint32_t>(request.size()));
  if (!TryWrite(write_fd_, &len, sizeof(uint32_t)) ||
      !TryWrite(write_fd_, request.data(), request.size())) {
    return ReportFailure(/*timed_out=*/false, "error writing to child");
  }

  ReadResult read_result = TryRead(read_fd_, &len, sizeof(uint32_t));
  if (read_result != ReadResult::kOk) return ReportReadFailure(read_result);

  len = internal::little_endian::ToHost(len);
  std::string response(len, '\0');
  read_result = TryRead(read_fd_, response.data(), len);
  if (read_result != ReadResult::kOk) return ReportReadFailure(read_result);
  return response;
}

std::string ForkPipeRunner::ReportFailure(bool timed_out,
                                          absl::string_view fallback_error) {
  // The testee exited, crashed, or hung.  After EOF or a read or write error
  // it has most likely just died, but a process that has just closed its
  // pipes may not be reapable for another moment, so it is given a short
  // grace period before being SIGKILLed; that way its own wait status (e.g.
  // the signal it crashed with) is observed rather than masked by our
  // SIGKILL.  A testee that closed its output but keeps running is SIGKILLed
  // once the period passes.  After a timeout it has just been sent SIGQUIT;
  // a testee whose default action is to dump core is given longer to finish
  // dying, so that its wait status (and its core) reflect the SIGQUIT rather
  // than our SIGKILL.  Either way the outcome is classified from the wait
  // status, and the next RunTest() call will spawn a fresh testee.
  constexpr std::chrono::seconds kExitGracePeriod(1);
  constexpr std::chrono::seconds kTimeoutGracePeriod(2);
  ABSL_LOG(INFO) << "Trying to reap child, pid=" << child_pid_;
  const ShutdownResult shutdown =
      Shutdown(timed_out ? kTimeoutGracePeriod : kExitGracePeriod);
  const absl::optional<int>& status = shutdown.wait_status;
  const bool signaled = status.has_value() && WIFSIGNALED(*status);
  const bool exited = status.has_value() && WIFEXITED(*status);

  std::string error_msg;
  conformance::ConformanceResponse response;
  if (timed_out) {
    // Signals sent by the runner itself are reported as such, so that they
    // are not mistaken for the testee crashing on its own.
    error_msg = "child timed out";
    if (shutdown.killed && signaled && WTERMSIG(*status) == SIGKILL) {
      error_msg += " (killed by runner)";
    } else if (signaled && WTERMSIG(*status) == SIGQUIT) {
      error_msg += " (terminated by runner's SIGQUIT)";
    } else if (signaled) {
      absl::StrAppendFormat(&error_msg, ", died with signal %d",
                            WTERMSIG(*status));
    } else if (exited) {
      absl::StrAppendFormat(&error_msg, ", exited with status=%d",
                            WEXITSTATUS(*status));
    }
    response.set_timeout_error(error_msg);
  } else if (shutdown.killed && signaled && WTERMSIG(*status) == SIGKILL) {
    // The testee was still alive after the failed exchange (e.g. it closed
    // its stdout but kept running) and Shutdown() SIGKILLed it; as in the
    // timeout branch, the runner's own signal is reported as such rather than
    // as the testee's death.
    error_msg = std::string(fallback_error);
    error_msg += " (killed by runner)";
    response.set_runtime_error(error_msg);
  } else if (signaled) {
    absl::StrAppendFormat(&error_msg, "child killed by signal %d",
                          WTERMSIG(*status));
    response.set_runtime_error(error_msg);
  } else if (exited) {
    absl::StrAppendFormat(&error_msg, "child exited, status=%d",
                          WEXITSTATUS(*status));
    response.set_runtime_error(error_msg);
  } else {
    // Nothing to reap (or waitpid() failed); all we know is that the exchange
    // failed.
    error_msg = std::string(fallback_error);
    response.set_runtime_error(error_msg);
  }
  ABSL_LOG(INFO) << error_msg;

  std::string serialized;
  // TODO: Remove this suppression.
  (void)response.SerializeToString(&serialized);
  return serialized;
}

std::string ForkPipeRunner::ReportReadFailure(ReadResult read_result) {
  return ReportFailure(read_result == ReadResult::kTimeout,
                       read_result == ReadResult::kEof
                           ? "child closed its output without responding"
                           : "error reading from child");
}

// TODO: make this work on Windows, instead of using these
// UNIX-specific APIs.
//
// There is a platform-agnostic API in
//    src/google/protobuf/compiler/subprocess.h
//
// However that API only supports sending a single message to the subprocess.
// We really want to be able to send messages and receive responses one at a
// time:
//
// 1. Spawning a new process for each test would take way too long for thousands
//    of tests and subprocesses like java that can take 100ms or more to start
//    up.
//
// 2. Sending all the tests in one big message and receiving all results in one
//    big message would take away our visibility about which test(s) caused a
//    crash or other fatal error.  It would also give us only a single failure
//    instead of all of them.
void ForkPipeRunner::SpawnTestProgram() {
  int toproc_pipe_fd[2];
  int fromproc_pipe_fd[2];
  if (pipe(toproc_pipe_fd) < 0 || pipe(fromproc_pipe_fd) < 0) {
    perror("pipe");
    exit(1);
  }

  // Build argv and log it *before* fork(): this process is multi-threaded, so
  // the child must only make async-signal-safe calls (no logging) until execv.
  std::vector<const char*> argv;
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
    write_fd_ = toproc_pipe_fd[1];
    read_fd_ = fromproc_pipe_fd[0];
    child_pid_ = pid;
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
    // any process the testee forks.  See the class comment for the trade-off.
    CHECK_SYSCALL_IN_CHILD(setpgid(0, 0));

    // Never returns.
    CHECK_SYSCALL_IN_CHILD(
        execv(executable_.c_str(), const_cast<char**>(argv.data())));
  }
}

bool ForkPipeRunner::TryWrite(int fd, const void* buf, size_t len) {
  // Writing to a pipe whose reader has exited raises SIGPIPE, whose default
  // action would kill the runner.  The signal is directed at the writing
  // thread, so blocking it on this thread alone for the duration of the write
  // turns it into an EPIPE error return, without changing the process-wide
  // disposition (which the testee would inherit across exec).  A blocked
  // SIGPIPE stays pending until it is either consumed or unblocked, so it is
  // consumed before the mask is restored.
  sigset_t sigpipe;
  sigemptyset(&sigpipe);
  sigaddset(&sigpipe, SIGPIPE);
  sigset_t old_mask;
  pthread_sigmask(SIG_BLOCK, &sigpipe, &old_mask);

  const char* in = static_cast<const char*>(buf);
  size_t ofs = 0;
  int error = 0;
  while (ofs < len) {
    const ssize_t written = write(fd, in + ofs, len - ofs);
    if (written < 0) {
      if (errno == EINTR) continue;
      error = errno;
      break;
    }
    ofs += static_cast<size_t>(written);
  }

  if (error == EPIPE && !sigismember(&old_mask, SIGPIPE)) {
    // Discard the SIGPIPE the failed write left pending for this thread.
    // This cannot block: the signal is thread-directed and blocked here, so
    // if sigpending() reports it, it is there for sigwait() to take at once,
    // and if the process ignores SIGPIPE none was raised and sigwait() is not
    // called.  (Both calls are portable, macOS included.)  Consuming the
    // signal also means that a SIGPIPE handler the host process may have
    // installed does not run for a testee-induced EPIPE; the write's error
    // return takes its place.  If the caller had SIGPIPE blocked already the
    // signal is left for the caller, as it would be for any other blocked
    // signal.
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

ForkPipeRunner::ReadResult ForkPipeRunner::TryRead(int fd, void* buf,
                                                   size_t len) {
  // The timeout is implemented with poll() on the calling thread rather than
  // by racing a blocking read() on a helper thread against a deadline: a
  // helper thread stuck in read() on a testee that never answers would have to
  // be joined (blocking us) or leaked, whereas poll() leaves nothing behind.
  char* out = static_cast<char*>(buf);
  size_t ofs = 0;
  while (ofs < len) {
    if (!WaitReadable(fd, read_timeout_)) {
      ABSL_LOG(ERROR) << current_test_name_ << ": timeout from test program";
      if (child_pid_ > 0) {
        // Some runtimes (notably the JVM) react to SIGQUIT by dumping all
        // their threads' stacks to stdout, i.e. into our pipe; log whatever
        // arrives, for a bounded time, to help diagnose the hang.  The caller
        // then shuts the testee down.  The signal goes to the testee's process
        // group so that it reaches the real testee behind a wrapper script
        // too (see the class comment for the trade-off); as in Shutdown(),
        // kill(-pid) fails with ESRCH only if the child has not reached its
        // setpgid() yet, in which case the child alone is signaled.
        if (kill(-child_pid_, SIGQUIT) != 0) kill(child_pid_, SIGQUIT);
        // TODO: Only log in flag-guarded mode, since reading
        // output from SIGQUIT is slow and verbose.
        const std::string sigquit_output = ReadSigquitOutput(fd);
        ABSL_LOG(ERROR) << "child_pid_=" << child_pid_ << " SIGQUIT: \n"
                        << (sigquit_output.empty() ? "(no output)"
                                                   : sigquit_output);
      }
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
