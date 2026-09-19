// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// The whole implementation is #ifdef'd out on other platforms, which makes
// this look unused to include checkers there.
#include "conformance/fork_pipe_runner.h"  // IWYU pragma: keep

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <chrono>  // NOLINT(build/c++11)
#include <cstddef>
#include <cstdint>
#include <future>  // NOLINT(build/c++11)
#include <limits>
#include <memory>
#include <string>

#include "absl/log/absl_log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "absl/types/optional.h"
#include "absl/types/span.h"

namespace google {
namespace protobuf {

namespace {

// How long to wait for a test program that has just been terminated to
// actually go away.  Bounded so that a failing TerminateProcess cannot hang
// the runner.
constexpr DWORD kKillWaitMs = 5000;
// How long a testee that failed to answer gets to finish dying before it is
// terminated, so that one that has just crashed is reported with its own exit
// code.
constexpr absl::Duration kFailureGracePeriod = absl::Seconds(1);

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

// Converts a duration to a WaitForSingleObject timeout, clamped to the
// non-negative values below INFINITE (which would wait forever).
DWORD ToWaitMs(absl::Duration duration) {
  return static_cast<DWORD>(
      std::clamp<int64_t>(absl::ToInt64Milliseconds(duration), 0,
                          static_cast<int64_t>(INFINITE) - 1));
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
// job, and waits (for a bounded time) for it to exit.
void KillTestProgram(HANDLE job, HANDLE process) {
  if (job != nullptr) {
    TerminateJobObject(job, 1);
  } else if (process != nullptr) {
    TerminateProcess(process, 1);
  }
  if (process != nullptr) {
    WaitForSingleObject(process, kKillWaitMs);
  }
}

}  // namespace

struct ForkPipeRunner::State {
  // nullptr when there is no live, unreaped testee.
  HANDLE child_process = nullptr;
  // Job object containing the test program and all of its descendants, or
  // nullptr if the test program could not be assigned to a job.
  HANDLE job = nullptr;
  // Ends of the to-testee and from-testee pipes owned by this process; nullptr
  // when not open.
  HANDLE write_handle = nullptr;
  HANDLE read_handle = nullptr;
  // Error code of the last failed read from the test program, captured on the
  // reader thread. Zero if the last read hit EOF instead.
  DWORD last_read_error = 0;
  // True if TryRead() terminated the test program after a read timeout, so
  // that Shutdown() can report the kill as the runner's own.
  bool killed_on_timeout = false;
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
  state_->last_read_error = 0;
  state_->killed_on_timeout = false;
}

ForkPipeRunner::ShutdownResult ForkPipeRunner::Shutdown(
    absl::Duration grace_period) {
  // Closing our end of the testee's stdin makes it see EOF, which is how a
  // conformance testee learns that the run is over.  Closing our end of its
  // stdout too means a testee blocked writing to a full pipe gets a
  // broken-pipe error instead of hanging.  Resetting the handles makes a
  // second call (e.g. the destructor after RunTest()'s crash path) a no-op.
  if (state_->write_handle != nullptr) {
    CloseHandle(state_->write_handle);
    state_->write_handle = nullptr;
  }
  if (state_->read_handle != nullptr) {
    CloseHandle(state_->read_handle);
    state_->read_handle = nullptr;
  }

  ShutdownResult result;
  result.killed = state_->killed_on_timeout;
  state_->killed_on_timeout = false;
  HANDLE process = state_->child_process;
  HANDLE job = state_->job;
  state_->child_process = nullptr;
  state_->job = nullptr;
  if (process == nullptr) {
    if (job != nullptr) CloseHandle(job);
    return result;
  }

  const DWORD wait = WaitForSingleObject(process, ToWaitMs(grace_period));
  if (wait == WAIT_TIMEOUT) {
    ABSL_LOG(WARNING) << "child has not exited "
                      << absl::FormatDuration(grace_period)
                      << " after its pipes were closed, terminating it";
    KillTestProgram(job, process);
    result.killed = true;
  } else if (wait != WAIT_OBJECT_0) {
    ABSL_LOG(WARNING) << "waiting for the child failed: " << LastSystemError();
  }

  DWORD exit_code = 0;
  if (!GetExitCodeProcess(process, &exit_code)) {
    ABSL_LOG(WARNING) << "GetExitCodeProcess failed: " << LastSystemError();
  } else if (exit_code == STILL_ACTIVE) {
    // Only possible if terminating the child failed or has not landed yet;
    // closing the job below is the last resort.
    ABSL_LOG(ERROR) << "giving up on child, which has not exited";
  } else {
    result.wait_status = static_cast<int>(exit_code);
  }
  CloseHandle(process);
  if (job != nullptr) {
    // Terminates the test program and anything it spawned, if still running
    // (JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE).
    CloseHandle(job);
  }
  return result;
}

std::string ForkPipeRunner::GetTestProgramFailure(
    absl::string_view what_failed) {
  ABSL_LOG(INFO) << "Trying to reap child";
  const ShutdownResult shutdown = Shutdown(kFailureGracePeriod);

  std::string error_msg(what_failed);
  // Say how the testee ended, if known.  A kill by the runner is reported as
  // such so that it is not mistaken for the testee crashing on its own.
  if (shutdown.killed) {
    absl::StrAppend(&error_msg, " (killed by runner)");
  } else if (shutdown.wait_status.has_value()) {
    // Hex keeps NTSTATUS crash codes such as 0xC0000005 recognizable.
    absl::StrAppend(&error_msg, " (exited with status=0x",
                    absl::Hex(static_cast<DWORD>(*shutdown.wait_status)), ")");
  }
  return error_msg;
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

ForkPipeRunner::ReadResult ForkPipeRunner::TryRead(void* buf, size_t len) {
  state_->last_read_error = 0;
  size_t offset = 0;
  while (offset < len) {
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
        state_->read_handle, buf, offset, len - offset);
    std::future_status status =
        future.wait_for(absl::ToChronoMilliseconds(read_timeout_));
    if (status == std::future_status::timeout) {
      ABSL_LOG(ERROR) << current_test_name_ << ": timeout from test program";
      // Killing the test program and everything it spawned closes the write
      // end of the pipe, which unblocks the reader thread that the destructor
      // of `future` is going to join.  Shutdown() reports the kill as ours.
      KillTestProgram(state_->job, state_->child_process);
      state_->killed_on_timeout = true;
      return ReadResult::kTimeout;
    }

    ReadChunk chunk = future.get();
    state_->last_read_error = chunk.error_code;
    // A broken pipe is how the test program exiting normally shows up on
    // Windows, so treat it the same as a zero-byte read.
    if (chunk.bytes_read == 0 ||
        (chunk.bytes_read < 0 && chunk.error_code == ERROR_BROKEN_PIPE)) {
      ABSL_LOG(ERROR) << current_test_name_
                      << ": unexpected EOF from test program";
      return ReadResult::kEof;
    } else if (chunk.bytes_read < 0) {
      ABSL_LOG(ERROR) << current_test_name_
                      << ": error reading from test program: "
                      << WindowsErrorMessage(chunk.error_code);
      return ReadResult::kError;
    }

    offset += static_cast<size_t>(chunk.bytes_read);
  }

  return ReadResult::kOk;
}

void ForkPipeRunner::CheckedRead(void* buf, size_t len) {
  // TODO: b/564149373 - classify mid-body read failures like header-read
  // failures instead of crashing the runner.
  const ReadResult read_result = TryRead(buf, len);
  if (read_result == ReadResult::kOk) {
    return;
  }
  std::string reason;
  if (read_result == ReadResult::kTimeout) {
    reason = "timed out";
  } else if (read_result == ReadResult::kEof) {
    reason = "unexpected EOF";
  } else {
    reason = WindowsErrorMessage(state_->last_read_error);
  }
  ABSL_LOG(FATAL) << current_test_name_
                  << ": error reading from test program: " << reason;
}

}  // namespace protobuf
}  // namespace google

#endif  // _WIN32
