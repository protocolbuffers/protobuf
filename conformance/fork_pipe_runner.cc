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
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>  // NOLINT(build/c++11)
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <future>  // NOLINT(build/c++11)
#include <string>
#include <vector>

#include "absl/log/absl_log.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
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

std::string ForkPipeRunner::RunTest(absl::string_view test_name,
                                    absl::string_view request) {
  if (child_pid_ < 0) {
    SpawnTestProgram();
  }
  current_test_name_ = std::string(test_name);

  uint32_t len =
      internal::little_endian::FromHost(static_cast<uint32_t>(request.size()));

  CheckedWrite(write_fd_, &len, sizeof(uint32_t));
  CheckedWrite(write_fd_, request.data(), request.size());

  std::string response;
  if (!TryRead(read_fd_, &len, sizeof(uint32_t))) {
    // We failed to read from the child, assume a crash and try to reap.
    ABSL_LOG(INFO) << "Trying to reap child, pid=" << child_pid_;

    int status = 0;
    waitpid(child_pid_, &status, WEXITED);

    std::string error_msg;
    conformance::ConformanceResponse response_obj;
    if (WIFEXITED(status)) {
      if (WEXITSTATUS(status) == 0) {
        absl::StrAppendFormat(&error_msg,
                              "child timed out, killed by signal %d",
                              WTERMSIG(status));
        response_obj.set_timeout_error(error_msg);
      } else {
        absl::StrAppendFormat(&error_msg, "child exited, status=%d",
                              WEXITSTATUS(status));
        response_obj.set_runtime_error(error_msg);
      }
    } else if (WIFSIGNALED(status)) {
      absl::StrAppendFormat(&error_msg, "child killed by signal %d",
                            WTERMSIG(status));
    }
    ABSL_LOG(INFO) << error_msg;
    child_pid_ = -1;

    // TODO: Remove this suppression.
    (void)response_obj.SerializeToString(&response);
    return response;
  }

  len = internal::little_endian::ToHost(len);
  response.resize(len);
  CheckedRead(read_fd_, (void *)response.c_str(), len);
  return response;
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

    // Never returns.
    CHECK_SYSCALL_IN_CHILD(
        execv(executable_.c_str(), const_cast<char**>(argv.data())));
  }
}

void ForkPipeRunner::CheckedWrite(int fd, const void *buf, size_t len) {
  if (static_cast<size_t>(write(fd, buf, len)) != len) {
    ABSL_LOG(FATAL) << current_test_name_
                    << ": error writing to test program: " << strerror(errno);
  }
}

bool ForkPipeRunner::TryRead(int fd, void *buf, size_t len) {
  size_t ofs = 0;
  while (len > 0) {
    std::future<ssize_t> future = std::async(
        std::launch::async,
        [](int fd, void *buf, size_t ofs, size_t len) {
          return read(fd, (char *)buf + ofs, len);
        },
        fd, buf, ofs, len);
    std::future_status status = future.wait_for(std::chrono::seconds(30));
    if (status == std::future_status::timeout) {
      ABSL_LOG(ERROR) << current_test_name_ << ": timeout from test program";
      kill(child_pid_, SIGQUIT);
      // TODO: Only log in flag-guarded mode, since reading output
      // from SIGQUIT is slow and verbose.
      std::vector<char> err;
      err.resize(5000);
      ssize_t err_bytes_read;
      size_t err_ofs = 0;
      do {
        err_bytes_read = read(fd, (void *)&err[err_ofs], err.size() - err_ofs);
        err_ofs += static_cast<size_t>(err_bytes_read);
      } while (err_bytes_read > 0 && err_ofs < err.size());
      ABSL_LOG(ERROR) << "child_pid_=" << child_pid_ << " SIGQUIT: \n"
                      << &err[0];
      return false;
    }

    ssize_t bytes_read = future.get();
    if (bytes_read == 0) {
      ABSL_LOG(ERROR) << current_test_name_
                      << ": unexpected EOF from test program";
      return false;
    } else if (bytes_read < 0) {
      ABSL_LOG(ERROR) << current_test_name_
                      << ": error reading from test program: "
                      << strerror(errno);
      return false;
    }

    len -= static_cast<size_t>(bytes_read);
    ofs += static_cast<size_t>(bytes_read);
  }

  return true;
}

void ForkPipeRunner::CheckedRead(int fd, void *buf, size_t len) {
  if (!TryRead(fd, buf, len)) {
    ABSL_LOG(FATAL) << current_test_name_
                    << ": error reading from test program: " << strerror(errno);
  }
}

}  // namespace protobuf
}  // namespace google
