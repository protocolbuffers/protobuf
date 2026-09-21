// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// The whole implementation is #ifdef'd out on other platforms, which makes
// this look unused to include checkers there.
#include "fork_pipe_runner.h"  // IWYU pragma: keep

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <chrono>  // NOLINT(build/c++11)
#include <cstddef>
#include <future>  // NOLINT(build/c++11)
#include <limits>
#include <memory>
#include <string>

#include "absl/log/absl_log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"

namespace google {
namespace protobuf {

namespace {

// How long to wait for the test program to exit once it has been asked to.
constexpr DWORD kExitWaitMs = 5000;

std::string WindowsErrorMessage(DWORD error) {
  std::string message;
  char* buffer = nullptr;
  DWORD length = FormatMessageA(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, error, 0, reinterpret_cast<LPSTR>(&buffer), 0, nullptr);
  if (length > 0 && buffer != nullptr) {
    message.assign(buffer, length);
    LocalFree(buffer);
    // System messages end with "\r\n"; strip that and any trailing period.
    while (!message.empty() &&
           (message.back() == '\r' || message.back() == '\n' ||
            message.back() == ' ' || message.back() == '.')) {
      message.pop_back();
    }
  }
  if (message.empty()) {
    return absl::StrCat("Windows error ", error);
  }
  return absl::StrCat("Windows error ", error, " (", message, ")");
}

std::string LastSystemError() { return WindowsErrorMessage(GetLastError()); }

std::string QuoteCommandLineArg(absl::string_view arg) {
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
    return std::string(arg);
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
  // Number of bytes read, or -1 if ReadFile failed.
  std::ptrdiff_t bytes_read;
  // GetLastError() captured on the reader thread right after ReadFile failed.
  // GetLastError() is thread-local, so it must not be queried from the main
  // thread afterwards.
  DWORD error_code;
};

// Kills the test program, along with any processes it spawned when it is in a
// job, and waits for it to exit.
void KillTestProgram(HANDLE job, HANDLE process) {
  if (job != nullptr) {
    TerminateJobObject(job, 1);
  } else if (process != nullptr) {
    TerminateProcess(process, 1);
  }
  if (process != nullptr) {
    WaitForSingleObject(process, kExitWaitMs);
  }
}

}  // namespace

struct ForkPipeRunner::State {
  HANDLE child_process = nullptr;
  // Job object containing the test program and all of its descendants, or
  // nullptr if the test program could not be assigned to a job.
  HANDLE job = nullptr;
  HANDLE write_handle = nullptr;
  HANDLE read_handle = nullptr;
  // Error code of the last failed read from the test program, captured on the
  // reader thread. Zero if the last read hit EOF instead.
  DWORD last_read_error = 0;
};

ForkPipeRunner::ForkPipeRunner(absl::string_view executable,
                               absl::Span<const std::string> executable_args)
    : executable_(executable),
      executable_args_(executable_args.begin(), executable_args.end()),
      state_(std::make_unique<State>()) {}

ForkPipeRunner::ForkPipeRunner(absl::string_view executable)
    : executable_(executable), state_(std::make_unique<State>()) {}

ForkPipeRunner::~ForkPipeRunner() { CloseTestProgram(); }

bool ForkPipeRunner::IsTestProgramRunning() const {
  return state_->child_process != nullptr;
}

void ForkPipeRunner::SpawnTestProgram() {
  SECURITY_ATTRIBUTES security_attributes;
  security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
  security_attributes.bInheritHandle = TRUE;
  security_attributes.lpSecurityDescriptor = nullptr;

  HANDLE child_stdin_read = nullptr;
  HANDLE child_stdin_write = nullptr;
  HANDLE child_stdout_read = nullptr;
  HANDLE child_stdout_write = nullptr;

  if (!CreatePipe(&child_stdin_read, &child_stdin_write, &security_attributes,
                  0) ||
      !SetHandleInformation(child_stdin_write, HANDLE_FLAG_INHERIT, 0) ||
      !CreatePipe(&child_stdout_read, &child_stdout_write, &security_attributes,
                  0) ||
      !SetHandleInformation(child_stdout_read, HANDLE_FLAG_INHERIT, 0)) {
    ABSL_LOG(FATAL) << "pipe setup failed: " << LastSystemError();
  }

  // Make sure the test program inherits our stderr, so that its crash
  // diagnostics are not silently lost. Failure here is not fatal.
  HANDLE stderr_handle = GetStdHandle(STD_ERROR_HANDLE);
  if (stderr_handle != nullptr && stderr_handle != INVALID_HANDLE_VALUE) {
    SetHandleInformation(stderr_handle, HANDLE_FLAG_INHERIT,
                         HANDLE_FLAG_INHERIT);
  }

  // Run the test program in a job object so that any processes it spawns (e.g.
  // a launcher script starting the real testee) are terminated together with
  // it. Otherwise a grandchild holding our stdout pipe open would keep the
  // reader thread blocked forever after a timeout.
  HANDLE job = CreateJobObjectA(nullptr, nullptr);
  if (job == nullptr) {
    ABSL_LOG(FATAL) << "CreateJobObject failed: " << LastSystemError();
  }
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION job_limits;
  ZeroMemory(&job_limits, sizeof(job_limits));
  job_limits.BasicLimitInformation.LimitFlags =
      JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
  if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation,
                               &job_limits,
                               static_cast<DWORD>(sizeof(job_limits)))) {
    ABSL_LOG(FATAL) << "SetInformationJobObject failed: " << LastSystemError();
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
  startup_info.hStdError = stderr_handle;

  PROCESS_INFORMATION process_info;
  ZeroMemory(&process_info, sizeof(process_info));
  // Start the process suspended so that it is assigned to the job before it
  // gets a chance to spawn children of its own.
  if (!CreateProcessA(nullptr, &command_line[0], nullptr, nullptr, TRUE,
                      CREATE_SUSPENDED, nullptr, nullptr, &startup_info,
                      &process_info)) {
    ABSL_LOG(FATAL) << "CreateProcess failed: " << LastSystemError();
  }
  if (!AssignProcessToJobObject(job, process_info.hProcess)) {
    // Older versions of Windows do not support nested jobs, so this fails if
    // the runner itself is already in a job. Carry on without one.
    ABSL_LOG(WARNING) << "AssignProcessToJobObject failed, processes spawned "
                         "by the test program will not be terminated on "
                         "timeout: "
                      << LastSystemError();
    CloseHandle(job);
    job = nullptr;
  }
  if (ResumeThread(process_info.hThread) == static_cast<DWORD>(-1)) {
    ABSL_LOG(FATAL) << "ResumeThread failed: " << LastSystemError();
  }

  CloseHandle(process_info.hThread);
  CloseHandle(child_stdin_read);
  CloseHandle(child_stdout_write);
  state_->write_handle = child_stdin_write;
  state_->read_handle = child_stdout_read;
  state_->child_process = process_info.hProcess;
  state_->job = job;
}

void ForkPipeRunner::CheckedWrite(const void* buf, size_t len) {
  DWORD bytes_written = 0;
  if (len > static_cast<size_t>(std::numeric_limits<DWORD>::max()) ||
      !WriteFile(state_->write_handle, buf, static_cast<DWORD>(len),
                 &bytes_written, nullptr) ||
      static_cast<size_t>(bytes_written) != len) {
    ABSL_LOG(FATAL) << current_test_name_
                    << ": error writing to test program: " << LastSystemError();
  }
}

bool ForkPipeRunner::TryRead(void* buf, size_t len, bool* timed_out) {
  *timed_out = false;
  state_->last_read_error = 0;
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
                        static_cast<DWORD>(len), &bytes_read, nullptr)) {
            return ReadChunk{-1, GetLastError()};
          }
          return ReadChunk{static_cast<std::ptrdiff_t>(bytes_read), 0};
        },
        state_->read_handle, buf, offset, len);
    std::future_status status = future.wait_for(std::chrono::seconds(30));
    if (status == std::future_status::timeout) {
      ABSL_LOG(ERROR) << current_test_name_ << ": timeout from test program";
      *timed_out = true;
      // Killing the test program and everything it spawned closes the write
      // end of the pipe, which unblocks the reader thread that the destructor
      // of `future` is going to join.
      KillTestProgram(state_->job, state_->child_process);
      return false;
    }

    ReadChunk chunk = future.get();
    state_->last_read_error = chunk.error_code;
    // A broken pipe is how the test program exiting normally shows up on
    // Windows, so treat it the same as a zero-byte read.
    if (chunk.bytes_read == 0 ||
        (chunk.bytes_read < 0 && chunk.error_code == ERROR_BROKEN_PIPE)) {
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
  if (TryRead(buf, len, &timed_out)) {
    return;
  }
  std::string reason;
  if (timed_out) {
    reason = "timed out";
  } else if (state_->last_read_error == 0) {
    reason = "unexpected EOF";
  } else {
    reason = WindowsErrorMessage(state_->last_read_error);
  }
  ABSL_LOG(FATAL) << current_test_name_
                  << ": error reading from test program: " << reason;
}

std::string ForkPipeRunner::GetTestProgramFailure(bool timed_out) {
  if (timed_out) {
    // TryRead has already killed the test program.
    CloseTestProgram();
    return "child timed out";
  }

  std::string error_msg;
  if (state_->child_process == nullptr) {
    error_msg = "child failed: test program is not running";
  } else {
    DWORD status = 0;
    WaitForSingleObject(state_->child_process, kExitWaitMs);
    if (!GetExitCodeProcess(state_->child_process, &status)) {
      error_msg = absl::StrCat("child failed: GetExitCodeProcess failed: ",
                               LastSystemError());
    } else if (status == STILL_ACTIVE) {
      // Never leave the test program behind.
      KillTestProgram(state_->job, state_->child_process);
      error_msg = "child failed while still active";
    } else {
      // Hex keeps NTSTATUS crash codes such as 0xC0000005 recognizable.
      error_msg = absl::StrCat("child exited, status=0x", absl::Hex(status));
    }
  }
  CloseTestProgram();
  return error_msg;
}

void ForkPipeRunner::CloseTestProgram() {
  if (state_->write_handle != nullptr) {
    CloseHandle(state_->write_handle);
    state_->write_handle = nullptr;
  }
  if (state_->read_handle != nullptr) {
    CloseHandle(state_->read_handle);
    state_->read_handle = nullptr;
  }
  if (state_->child_process != nullptr) {
    // Closing the pipes signals EOF to a well-behaved test program; give it a
    // chance to exit on its own before closing the job kills it.
    WaitForSingleObject(state_->child_process, kExitWaitMs);
    CloseHandle(state_->child_process);
    state_->child_process = nullptr;
  }
  if (state_->job != nullptr) {
    // Terminates the test program and anything it spawned, if still running.
    CloseHandle(state_->job);
    state_->job = nullptr;
  }
}

}  // namespace protobuf
}  // namespace google

#endif  // _WIN32
