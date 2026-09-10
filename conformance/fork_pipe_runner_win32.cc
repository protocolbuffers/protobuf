// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "fork_pipe_runner.h"

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <chrono>  // NOLINT(build/c++11)
#include <cstddef>
#include <future>  // NOLINT(build/c++11)
#include <limits>
#include <sstream>
#include <string>

#include "absl/log/absl_log.h"
#include "absl/strings/str_format.h"

namespace google {
namespace protobuf {

namespace {

std::string WindowsErrorMessage(DWORD error) {
  std::ostringstream oss;
  oss << "Windows error " << error;
  return oss.str();
}

std::string LastSystemError() { return WindowsErrorMessage(GetLastError()); }

std::string QuoteCommandLineArg(const std::string& arg) {
  if (arg.empty()) {
    return "\"\"";
  }

  bool needs_quotes = false;
  for (char c : arg) {
    if (c == ' ' || c == '\t' || c == '"' || c == '\\') {
      needs_quotes = true;
      break;
    }
  }
  if (!needs_quotes) {
    return arg;
  }

  std::string quoted = "\"";
  size_t backslashes = 0;
  for (char c : arg) {
    if (c == '\\') {
      ++backslashes;
    } else if (c == '"') {
      quoted.append(backslashes * 2 + 1, '\\');
      quoted.push_back(c);
      backslashes = 0;
    } else {
      quoted.append(backslashes, '\\');
      backslashes = 0;
      quoted.push_back(c);
    }
  }
  quoted.append(backslashes * 2, '\\');
  quoted.push_back('"');
  return quoted;
}

struct ReadChunk {
  std::ptrdiff_t bytes_read;
  DWORD error_code;
};

}  // namespace

struct ForkPipeRunner::State {
  HANDLE child_process = NULL;
  HANDLE write_handle = NULL;
  HANDLE read_handle = NULL;
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
  return state_->child_process != NULL;
}

void ForkPipeRunner::SpawnTestProgram() {
  SECURITY_ATTRIBUTES security_attributes;
  security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
  security_attributes.bInheritHandle = TRUE;
  security_attributes.lpSecurityDescriptor = NULL;

  HANDLE child_stdin_read = NULL;
  HANDLE child_stdin_write = NULL;
  HANDLE child_stdout_read = NULL;
  HANDLE child_stdout_write = NULL;

  if (!CreatePipe(&child_stdin_read, &child_stdin_write, &security_attributes,
                  0) ||
      !SetHandleInformation(child_stdin_write, HANDLE_FLAG_INHERIT, 0) ||
      !CreatePipe(&child_stdout_read, &child_stdout_write, &security_attributes,
                  0) ||
      !SetHandleInformation(child_stdout_read, HANDLE_FLAG_INHERIT, 0)) {
    ABSL_LOG(FATAL) << "pipe setup failed: " << LastSystemError();
  }

  std::string command_line = QuoteCommandLineArg(executable_);
  ABSL_LOG(INFO) << executable_;
  for (const std::string& arg : executable_args_) {
    command_line.push_back(' ');
    command_line.append(QuoteCommandLineArg(arg));
    ABSL_LOG(INFO) << arg;
  }

  STARTUPINFOA startup_info;
  ZeroMemory(&startup_info, sizeof(startup_info));
  startup_info.cb = sizeof(startup_info);
  startup_info.dwFlags = STARTF_USESTDHANDLES;
  startup_info.hStdInput = child_stdin_read;
  startup_info.hStdOutput = child_stdout_write;
  startup_info.hStdError = GetStdHandle(STD_ERROR_HANDLE);

  PROCESS_INFORMATION process_info;
  ZeroMemory(&process_info, sizeof(process_info));
  if (!CreateProcessA(NULL, &command_line[0], NULL, NULL, TRUE, 0, NULL, NULL,
                      &startup_info, &process_info)) {
    ABSL_LOG(FATAL) << "CreateProcess failed: " << LastSystemError();
  }

  CloseHandle(process_info.hThread);
  CloseHandle(child_stdin_read);
  CloseHandle(child_stdout_write);
  state_->write_handle = child_stdin_write;
  state_->read_handle = child_stdout_read;
  state_->child_process = process_info.hProcess;
}

void ForkPipeRunner::CheckedWrite(const void* buf, size_t len) {
  DWORD bytes_written = 0;
  if (len > static_cast<size_t>(std::numeric_limits<DWORD>::max()) ||
      !WriteFile(state_->write_handle, buf, static_cast<DWORD>(len),
                 &bytes_written, NULL) ||
      static_cast<size_t>(bytes_written) != len) {
    ABSL_LOG(FATAL) << current_test_name_
                    << ": error writing to test program: "
                    << LastSystemError();
  }
}

bool ForkPipeRunner::TryRead(void* buf, size_t len, bool* timed_out) {
  *timed_out = false;
  size_t offset = 0;
  while (len > 0) {
    std::future<ReadChunk> future = std::async(
        std::launch::async,
        [](HANDLE read_handle, void* buf, size_t offset, size_t len) {
          DWORD bytes_read = 0;
          if (len > static_cast<size_t>(std::numeric_limits<DWORD>::max())) {
            len = static_cast<size_t>(std::numeric_limits<DWORD>::max());
          }
          if (!ReadFile(read_handle, static_cast<char*>(buf) + offset,
                        static_cast<DWORD>(len), &bytes_read, NULL)) {
            return ReadChunk{-1, GetLastError()};
          }
          return ReadChunk{static_cast<std::ptrdiff_t>(bytes_read), 0};
        },
        state_->read_handle, buf, offset, len);
    std::future_status status = future.wait_for(std::chrono::seconds(30));
    if (status == std::future_status::timeout) {
      ABSL_LOG(ERROR) << current_test_name_ << ": timeout from test program";
      *timed_out = true;
      TerminateProcess(state_->child_process, 1);
      WaitForSingleObject(state_->child_process, 5000);
      return false;
    }

    ReadChunk chunk = future.get();
    if (chunk.bytes_read == 0) {
      ABSL_LOG(ERROR) << current_test_name_
                      << ": unexpected EOF from test program";
      return false;
    } else if (chunk.bytes_read < 0) {
      ABSL_LOG(ERROR) << current_test_name_
                      << ": error reading from test program: "
                      << WindowsErrorMessage(chunk.error_code);
      return false;
    }

    len -= static_cast<size_t>(chunk.bytes_read);
    offset += static_cast<size_t>(chunk.bytes_read);
  }

  return true;
}

void ForkPipeRunner::CheckedRead(void* buf, size_t len) {
  bool timed_out = false;
  if (!TryRead(buf, len, &timed_out)) {
    ABSL_LOG(FATAL) << current_test_name_
                    << ": error reading from test program: "
                    << LastSystemError();
  }
}

std::string ForkPipeRunner::GetTestProgramFailure(bool timed_out) {
  std::string error_msg;
  if (timed_out) {
    CloseTestProgram();
    return "child timed out";
  }

  DWORD status = 0;
  if (state_->child_process != NULL) {
    WaitForSingleObject(state_->child_process, 5000);
  }
  if (state_->child_process != NULL &&
      GetExitCodeProcess(state_->child_process, &status)) {
    if (status == STILL_ACTIVE) {
      error_msg = "child failed while still active";
    } else {
      absl::StrAppendFormat(&error_msg, "child exited, status=%d", status);
    }
  } else {
    absl::StrAppendFormat(&error_msg, "child failed: %s", LastSystemError());
  }
  CloseTestProgram();
  return error_msg;
}

void ForkPipeRunner::CloseTestProgram() {
  if (state_->write_handle != NULL) {
    CloseHandle(state_->write_handle);
    state_->write_handle = NULL;
  }
  if (state_->read_handle != NULL) {
    CloseHandle(state_->read_handle);
    state_->read_handle = NULL;
  }
  if (state_->child_process != NULL) {
    CloseHandle(state_->child_process);
    state_->child_process = NULL;
  }
}

}  // namespace protobuf
}  // namespace google

#endif  // _WIN32
