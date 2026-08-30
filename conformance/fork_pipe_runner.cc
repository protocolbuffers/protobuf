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

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <fcntl.h>
#include <io.h>
#else  // _WIN32
#include <sys/wait.h>
#include <unistd.h>
#endif  // !_WIN32

#include <chrono>  // NOLINT(build/c++11)
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <future>  // NOLINT(build/c++11)
#include <memory>
#include <string>
#include <vector>

#include "absl/log/absl_log.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "conformance/conformance.pb.h"
#include "google/protobuf/endian.h"

#ifdef _WIN32
#include "google/protobuf/io/io_win32.h"
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define CHECK_SYSCALL(call)                            \
  if (call < 0) {                                      \
    perror(#call " " __FILE__ ":" TOSTRING(__LINE__)); \
    exit(1);                                           \
  }

namespace google {
namespace protobuf {

#ifdef _WIN32
// Define the posix I/O functions we use for Windows similar to protoc.
using google::protobuf::io::win32::read;
using google::protobuf::io::win32::write;

namespace {
// Exit code assigned via TerminateProcess() when the child times out, so the
// reap logic can distinguish a timeout from a crash.
constexpr DWORD kChildTimeoutExitCode = 0xC0FFEE;

void CloseHandleOrDie(HANDLE handle) {
  if (!CloseHandle(handle)) {
    ABSL_LOG(FATAL) << "CloseHandle: error " << GetLastError();
  }
}

// Quotes a command-line argument so the child process parses it back to
// exactly `arg`. Unlike protoc, conformance binaries will often need arguments
// (e.g., a python script invocation) so it is important to quote here.
// https://learn.microsoft.com/en-us/cpp/c-language/parsing-c-command-line-arguments
std::string QuoteArg(const std::string &arg) {
  if (!arg.empty() && arg.find_first_of(" \t\n\v\"") == std::string::npos) {
    return arg;
  }
  std::string result = "\"";
  size_t num_backslashes = 0;
  for (char c : arg) {
    if (c == '\\') {
      ++num_backslashes;
      continue;
    }
    if (c == '"') {
      // Escape the backslash run and the quote itself.
      result.append(2 * num_backslashes + 1, '\\');
    } else {
      // Backslashes not followed by a quote are literal.
      result.append(num_backslashes, '\\');
    }
    num_backslashes = 0;
    result.push_back(c);
  }
  // Double a trailing backslash run so it does not escape the closing quote.
  result.append(2 * num_backslashes, '\\');
  result.push_back('"');
  return result;
}
}  // namespace
#endif  // _WIN32

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

    std::string error_msg;
    conformance::ConformanceResponse response_obj;
#ifdef _WIN32
    HANDLE child_handle = reinterpret_cast<HANDLE>(child_pid_);
    DWORD exit_code = 0;
    if (WaitForSingleObject(child_handle, INFINITE) != WAIT_OBJECT_0 ||
        !GetExitCodeProcess(child_handle, &exit_code)) {
      absl::StrAppendFormat(&error_msg, "failed to wait for child: error %u",
                            GetLastError());
      response_obj.set_runtime_error(error_msg);
    } else if (exit_code == kChildTimeoutExitCode) {
      absl::StrAppendFormat(&error_msg, "child timed out and was terminated");
      response_obj.set_timeout_error(error_msg);
    } else {
      absl::StrAppendFormat(&error_msg, "child exited, status=%u", exit_code);
      response_obj.set_runtime_error(error_msg);
    }
    CloseHandleOrDie(child_handle);
#else  // _WIN32
    int status = 0;
    waitpid(child_pid_, &status, WEXITED);

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
#endif  // !_WIN32
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

// Note: there is a platform-agnostic subprocess API in
//    src/google/protobuf/compiler/subprocess.h
// that we deliberately do not use here.
//
// That API only supports sending a single message to the subprocess.
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
#ifdef _WIN32
void ForkPipeRunner::SpawnTestProgram() {
  // Create the pipes.
  HANDLE stdin_pipe_read;
  HANDLE stdin_pipe_write;
  HANDLE stdout_pipe_read;
  HANDLE stdout_pipe_write;

  if (!CreatePipe(&stdin_pipe_read, &stdin_pipe_write, nullptr, 0)) {
    ABSL_LOG(FATAL) << "CreatePipe: error " << GetLastError();
  }
  if (!CreatePipe(&stdout_pipe_read, &stdout_pipe_write, nullptr, 0)) {
    ABSL_LOG(FATAL) << "CreatePipe: error " << GetLastError();
  }

  // Make child side of the pipes inheritable.
  if (!SetHandleInformation(stdin_pipe_read, HANDLE_FLAG_INHERIT,
                            HANDLE_FLAG_INHERIT)) {
    ABSL_LOG(FATAL) << "SetHandleInformation: error " << GetLastError();
  }
  if (!SetHandleInformation(stdout_pipe_write, HANDLE_FLAG_INHERIT,
                            HANDLE_FLAG_INHERIT)) {
    ABSL_LOG(FATAL) << "SetHandleInformation: error " << GetLastError();
  }

  // Setup STARTUPINFO to redirect handles.
  STARTUPINFOW startup_info;
  ZeroMemory(&startup_info, sizeof(startup_info));
  startup_info.cb = sizeof(startup_info);
  startup_info.dwFlags = STARTF_USESTDHANDLES;
  startup_info.hStdInput = stdin_pipe_read;
  startup_info.hStdOutput = stdout_pipe_write;
  startup_info.hStdError = GetStdHandle(STD_ERROR_HANDLE);

  if (startup_info.hStdError == INVALID_HANDLE_VALUE) {
    ABSL_LOG(FATAL) << "GetStdHandle: error " << GetLastError();
  }

  // get wide string version of program as the path may contain non-ascii characters
  std::wstring wprogram;
  if (!io::win32::strings::utf8_to_wcs(executable_.c_str(), &wprogram)) {
    ABSL_LOG(FATAL) << "utf8_to_wcs: error " << GetLastError();
  }

  // Join program and args into a single command-line string, as CreateProcessW
  // expects it.
  std::string command_line = QuoteArg(executable_);
  for (const std::string &arg : executable_args_) {
    command_line += ' ';
    command_line += QuoteArg(arg);
  }
  ABSL_LOG(INFO) << command_line;

  // get wide string version of command line as the path may contain non-ascii characters
  std::wstring wcommand_line;
  if (!io::win32::strings::utf8_to_wcs(command_line.c_str(), &wcommand_line)) {
    ABSL_LOG(FATAL) << "utf8_to_wcs: error " << GetLastError();
  }

  PROCESS_INFORMATION process_info;

  if (CreateProcessW(wprogram.c_str(),
                     wcommand_line.data(),
                     nullptr,  // process security attributes
                     nullptr,  // thread security attributes
                     TRUE,     // inherit handles?
                     0,        // obscure creation flags
                     nullptr,  // environment (inherit from parent)
                     nullptr,  // current directory (inherit from parent)
                     &startup_info, &process_info)) {
    child_pid_ = reinterpret_cast<intptr_t>(process_info.hProcess);
    CloseHandleOrDie(process_info.hThread);
  } else {
    ABSL_LOG(FATAL) << "CreateProcess(" << executable_ << "): error "
                    << GetLastError();
  }

  CloseHandleOrDie(stdin_pipe_read);
  CloseHandleOrDie(stdout_pipe_write);

  // Wrap our ends of the pipes in file descriptors so the I/O code below is
  // shared with the POSIX implementation.
  write_fd_ = _open_osfhandle(reinterpret_cast<intptr_t>(stdin_pipe_write),
                              _O_BINARY | _O_NOINHERIT);
  read_fd_ = _open_osfhandle(reinterpret_cast<intptr_t>(stdout_pipe_read),
                             _O_RDONLY | _O_BINARY | _O_NOINHERIT);
  if (write_fd_ < 0 || read_fd_ < 0) {
    ABSL_LOG(FATAL) << "_open_osfhandle failed";
  }
}
#else  // _WIN32
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
    write_fd_ = toproc_pipe_fd[1];
    read_fd_ = fromproc_pipe_fd[0];
    child_pid_ = pid;
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

    std::vector<const char *> argv;
    argv.push_back(executable.get());
    ABSL_LOG(INFO) << argv[0];
    for (size_t i = 0; i < executable_args_.size(); ++i) {
      argv.push_back(executable_args_[i].c_str());
      ABSL_LOG(INFO) << executable_args_[i];
    }
    argv.push_back(nullptr);
    // Never returns.
    CHECK_SYSCALL(execv(executable.get(), const_cast<char **>(argv.data())));
  }
}
#endif  // !_WIN32

void ForkPipeRunner::CheckedWrite(int fd, const void *buf, size_t len) {
  if (static_cast<size_t>(write(fd, buf, len)) != len) {
    ABSL_LOG(FATAL) << current_test_name_
                    << ": error writing to test program: " << strerror(errno);
  }
}

bool ForkPipeRunner::TryRead(int fd, void *buf, size_t len) {
  size_t ofs = 0;
  while (len > 0) {
    std::future<int> future = std::async(
        std::launch::async,
        [](int fd, void *buf, size_t ofs, size_t len) -> int {
          return read(fd, (char *)buf + ofs, len);
        },
        fd, buf, ofs, len);
    std::future_status status = future.wait_for(std::chrono::seconds(30));
    if (status == std::future_status::timeout) {
      ABSL_LOG(ERROR) << current_test_name_ << ": timeout from test program";
#ifdef _WIN32
      TerminateProcess(reinterpret_cast<HANDLE>(child_pid_),
                       kChildTimeoutExitCode);
#else  // _WIN32
      kill(child_pid_, SIGQUIT);
      // TODO: Only log in flag-guarded mode, since reading output
      // from SIGQUIT is slow and verbose.
      std::vector<char> err;
      err.resize(5000);
      int err_bytes_read;
      size_t err_ofs = 0;
      do {
        err_bytes_read = read(fd, (void *)&err[err_ofs], err.size() - err_ofs);
        err_ofs += static_cast<size_t>(err_bytes_read);
      } while (err_bytes_read > 0 && err_ofs < err.size());
      ABSL_LOG(ERROR) << "child_pid_=" << child_pid_ << " SIGQUIT: \n"
                      << &err[0];
#endif  // !_WIN32
      return false;
    }

    int bytes_read = future.get();
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
