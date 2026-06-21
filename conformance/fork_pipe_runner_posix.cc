// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "fork_pipe_runner.h"

#ifndef _WIN32

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>  // NOLINT(build/c++11)
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <future>  // NOLINT(build/c++11)
#include <memory>
#include <string>
#include <vector>

#include "absl/log/absl_log.h"
#include "absl/strings/str_format.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define CHECK_SYSCALL(call)                            \
  if (call < 0) {                                      \
    perror(#call " " __FILE__ ":" TOSTRING(__LINE__)); \
    exit(1);                                           \
  }

namespace google {
namespace protobuf {

struct ForkPipeRunner::State {
  int write_fd = -1;
  int read_fd = -1;
  pid_t child_pid = -1;
};

ForkPipeRunner::ForkPipeRunner(
    absl::string_view executable,
    absl::Span<const std::string> executable_args)
    : executable_(executable),
      executable_args_(executable_args.begin(), executable_args.end()),
      state_(new State) {}

ForkPipeRunner::ForkPipeRunner(const std::string& executable)
    : executable_(executable), state_(new State) {}

ForkPipeRunner::~ForkPipeRunner() {
  CloseTestProgram();
  delete state_;
}

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
    CHECK_SYSCALL(close(STDIN_FILENO));
    CHECK_SYSCALL(close(STDOUT_FILENO));
    CHECK_SYSCALL(dup2(toproc_pipe_fd[0], STDIN_FILENO));
    CHECK_SYSCALL(dup2(fromproc_pipe_fd[1], STDOUT_FILENO));

    CHECK_SYSCALL(close(toproc_pipe_fd[0]));
    CHECK_SYSCALL(close(fromproc_pipe_fd[1]));
    CHECK_SYSCALL(close(toproc_pipe_fd[1]));
    CHECK_SYSCALL(close(fromproc_pipe_fd[0]));

    std::unique_ptr<char[]> executable(new char[executable_.size() + 1]);
    memcpy(executable.get(), executable_.c_str(), executable_.size());
    executable[executable_.size()] = '\0';

    std::vector<const char*> argv;
    argv.push_back(executable.get());
    ABSL_LOG(INFO) << argv[0];
    for (size_t i = 0; i < executable_args_.size(); ++i) {
      argv.push_back(executable_args_[i].c_str());
      ABSL_LOG(INFO) << executable_args_[i];
    }
    argv.push_back(nullptr);
    // Never returns.
    CHECK_SYSCALL(execv(executable.get(), const_cast<char**>(argv.data())));
  }
}

void ForkPipeRunner::CheckedWrite(const void* buf, size_t len) {
  if (static_cast<size_t>(write(state_->write_fd, buf, len)) != len) {
    ABSL_LOG(FATAL) << current_test_name_
                    << ": error writing to test program: " << strerror(errno);
  }
}

bool ForkPipeRunner::TryRead(void* buf, size_t len, bool* timed_out) {
  *timed_out = false;
  size_t ofs = 0;
  while (len > 0) {
    std::future<ssize_t> future = std::async(
        std::launch::async,
        [](int read_fd, void* buf, size_t ofs, size_t len) {
          return read(read_fd, (char*)buf + ofs, len);
        },
        state_->read_fd, buf, ofs, len);
    std::future_status status = future.wait_for(std::chrono::seconds(30));
    if (status == std::future_status::timeout) {
      ABSL_LOG(ERROR) << current_test_name_ << ": timeout from test program";
      *timed_out = true;
      kill(state_->child_pid, SIGQUIT);
      // TODO: Only log in flag-guarded mode, since reading output
      // from SIGQUIT is slow and verbose.
      std::vector<char> err;
      err.resize(5000);
      ssize_t err_bytes_read;
      size_t err_ofs = 0;
      do {
        err_bytes_read =
            read(state_->read_fd, (void*)&err[err_ofs],
                 err.size() - err_ofs);
        if (err_bytes_read > 0) {
          err_ofs += static_cast<size_t>(err_bytes_read);
        }
      } while (err_bytes_read > 0 && err_ofs < err.size());
      ABSL_LOG(ERROR) << "child_pid_=" << state_->child_pid << " SIGQUIT: \n"
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

void ForkPipeRunner::CheckedRead(void* buf, size_t len) {
  bool timed_out = false;
  if (!TryRead(buf, len, &timed_out)) {
    ABSL_LOG(FATAL) << current_test_name_
                    << ": error reading from test program: " << strerror(errno);
  }
}

std::string ForkPipeRunner::GetTestProgramFailure(bool timed_out) {
  // We failed to read from the child, assume a crash and try to reap.
  ABSL_LOG(INFO) << "Trying to reap child, pid=" << state_->child_pid;

  int status = 0;
  waitpid(state_->child_pid, &status, 0);

  std::string error_msg;
  if (timed_out) {
    if (WIFSIGNALED(status)) {
      absl::StrAppendFormat(&error_msg, "child timed out, killed by signal %d",
                            WTERMSIG(status));
    } else {
      error_msg = "child timed out";
    }
  } else if (WIFEXITED(status)) {
    absl::StrAppendFormat(&error_msg, "child exited, status=%d",
                          WEXITSTATUS(status));
  } else if (WIFSIGNALED(status)) {
    absl::StrAppendFormat(&error_msg, "child killed by signal %d",
                          WTERMSIG(status));
  }
  CloseTestProgram();
  return error_msg;
}

void ForkPipeRunner::CloseTestProgram() {
  if (state_->write_fd >= 0) {
    close(state_->write_fd);
    state_->write_fd = -1;
  }
  if (state_->read_fd >= 0) {
    close(state_->read_fd);
    state_->read_fd = -1;
  }
  state_->child_pid = -1;
}

}  // namespace protobuf
}  // namespace google

#endif  // !_WIN32
