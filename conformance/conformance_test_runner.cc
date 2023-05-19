// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <future>
#include <vector>

#include "absl/log/absl_log.h"
#include "absl/strings/str_format.h"
#include "conformance/conformance.pb.h"
#include "conformance_test.h"

using conformance::ConformanceResponse;
using google::protobuf::ConformanceTestSuite;
using std::string;
using std::vector;

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define CHECK_SYSCALL(call)                            \
  if (call < 0) {                                      \
    perror(#call " " __FILE__ ":" TOSTRING(__LINE__)); \
    exit(1);                                           \
  }

namespace google {
namespace protobuf {

void ParseFailureList(const char *filename,
                      conformance::FailureSet *failure_list) {
  std::ifstream infile(filename);

  if (!infile.is_open()) {
    fprintf(stderr, "Couldn't open failure list file: %s\n", filename);
    exit(1);
  }

  for (string line; getline(infile, line);) {
    // Remove whitespace.
    line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());

    // Remove comments.
    line = line.substr(0, line.find("#"));

    if (!line.empty()) {
      failure_list->add_failure(line);
    }
  }
}

void UsageError() {
  fprintf(stderr, "Usage: conformance-test-runner [options] <test-program>\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr,
          "  --failure_list <filename>   Use to specify list of tests\n");
  fprintf(stderr,
          "                              that are expected to fail.  File\n");
  fprintf(stderr,
          "                              should contain one test name per\n");
  fprintf(stderr,
          "                              line.  Use '#' for comments.\n");
  fprintf(stderr,
          "  --text_format_failure_list <filename>   Use to specify list \n");
  fprintf(stderr,
          "                              of tests that are expected to \n");
  fprintf(stderr, "                              fail in the \n");
  fprintf(stderr,
          "                              text_format_conformance_suite.  \n");
  fprintf(stderr,
          "                              File should contain one test name \n");
  fprintf(stderr,
          "                              per line.  Use '#' for comments.\n");

  fprintf(stderr,
          "  --enforce_recommended       Enforce that recommended test\n");
  fprintf(stderr,
          "                              cases are also passing. Specify\n");
  fprintf(stderr,
          "                              this flag if you want to be\n");
  fprintf(stderr,
          "                              strictly conforming to protobuf\n");
  fprintf(stderr, "                              spec.\n");
  fprintf(stderr,
          "  --output_dir                <dirname> Directory to write\n"
          "                              output files.\n");
  exit(1);
}

void ForkPipeRunner::RunTest(const std::string &test_name,
                             const std::string &request,
                             std::string *response) {
  if (child_pid_ < 0) {
    SpawnTestProgram();
  }
  current_test_name_ = test_name;

  uint32_t len = request.size();
  CheckedWrite(write_fd_, &len, sizeof(uint32_t));
  CheckedWrite(write_fd_, request.c_str(), request.size());

  if (!TryRead(read_fd_, &len, sizeof(uint32_t))) {
    // We failed to read from the child, assume a crash and try to reap.
    ABSL_LOG(INFO) << "Trying to reap child, pid=" << child_pid_;

    int status = 0;
    waitpid(child_pid_, &status, WEXITED);

    string error_msg;
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

    response_obj.SerializeToString(response);
    return;
  }

  response->resize(len);
  CheckedRead(read_fd_, (void *)response->c_str(), len);
}

int ForkPipeRunner::Run(int argc, char *argv[],
                        const std::vector<ConformanceTestSuite *> &suites) {
  if (suites.empty()) {
    fprintf(stderr, "No test suites found.\n");
    return EXIT_FAILURE;
  }
  bool all_ok = true;
  for (ConformanceTestSuite *suite : suites) {
    string program;
    std::vector<string> program_args;
    string failure_list_filename;
    conformance::FailureSet failure_list;

    bool performance = false;
    for (int arg = 1; arg < argc; ++arg) {
      if (strcmp(argv[arg], suite->GetFailureListFlagName().c_str()) == 0) {
        if (++arg == argc) UsageError();
        failure_list_filename = argv[arg];
        ParseFailureList(argv[arg], &failure_list);
      } else if (strcmp(argv[arg], "--performance") == 0) {
        performance = true;
        suite->SetPerformance(true);
      } else if (strcmp(argv[arg], "--verbose") == 0) {
        suite->SetVerbose(true);
      } else if (strcmp(argv[arg], "--enforce_recommended") == 0) {
        suite->SetEnforceRecommended(true);
      } else if (strcmp(argv[arg], "--output_dir") == 0) {
        if (++arg == argc) UsageError();
        suite->SetOutputDir(argv[arg]);
      } else if (argv[arg][0] == '-') {
        bool recognized_flag = false;
        for (ConformanceTestSuite *suite : suites) {
          if (strcmp(argv[arg], suite->GetFailureListFlagName().c_str()) == 0) {
            if (++arg == argc) UsageError();
            recognized_flag = true;
          }
        }
        if (!recognized_flag) {
          fprintf(stderr, "Unknown option: %s\n", argv[arg]);
          UsageError();
        }
      } else {
        program += argv[arg++];
        while (arg < argc) {
          program_args.push_back(argv[arg]);
          arg++;
        }
      }
    }

    ForkPipeRunner runner(program, program_args, performance);

    std::string output;
    all_ok = all_ok && suite->RunSuite(&runner, &output, failure_list_filename,
                                       &failure_list);

    fwrite(output.c_str(), 1, output.size(), stderr);
  }
  return all_ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

// TODO(haberman): make this work on Windows, instead of using these
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
    std::future_status status;
    if (performance_) {
      status = future.wait_for(std::chrono::seconds(5));
      if (status == std::future_status::timeout) {
        ABSL_LOG(ERROR) << current_test_name_ << ": timeout from test program";
        kill(child_pid_, SIGQUIT);
        // TODO(sandyzhang): Only log in flag-guarded mode, since reading output
        // from SIGQUIT is slow and verbose.
        std::vector<char> err;
        err.resize(5000);
        ssize_t err_bytes_read;
        size_t err_ofs = 0;
        do {
          err_bytes_read =
              read(fd, (void *)&err[err_ofs], err.size() - err_ofs);
          err_ofs += err_bytes_read;
        } while (err_bytes_read > 0 && err_ofs < err.size());
        ABSL_LOG(ERROR) << "child_pid_=" << child_pid_ << " SIGQUIT: \n"
                        << &err[0];
        return false;
      }
    } else {
      future.wait();
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

    len -= bytes_read;
    ofs += bytes_read;
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
