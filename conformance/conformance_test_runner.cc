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

#include <algorithm>
#include <errno.h>
#include <unistd.h>
#include <fstream>
#include <vector>

#include "conformance.pb.h"
#include "conformance_test.h"

using conformance::ConformanceRequest;
using conformance::ConformanceResponse;
using google::protobuf::internal::scoped_array;
using std::string;
using std::vector;

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define CHECK_SYSCALL(call) \
  if (call < 0) { \
    perror(#call " " __FILE__ ":" TOSTRING(__LINE__)); \
    exit(1); \
  }

// Test runner that spawns the process being tested and communicates with it
// over a pipe.
class ForkPipeRunner : public google::protobuf::ConformanceTestRunner {
 public:
  ForkPipeRunner(const std::string &executable)
      : running_(false), executable_(executable) {}

  virtual ~ForkPipeRunner() {}

  void RunTest(const std::string& test_name,
               const std::string& request,
               std::string* response) {
    if (!running_) {
      SpawnTestProgram();
    }

    current_test_name_ = test_name;

    uint32_t len = request.size();
    CheckedWrite(write_fd_, &len, sizeof(uint32_t));
    CheckedWrite(write_fd_, request.c_str(), request.size());
    CheckedRead(read_fd_, &len, sizeof(uint32_t));
    response->resize(len);
    CheckedRead(read_fd_, (void*)response->c_str(), len);
  }

 private:
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
  void SpawnTestProgram() {
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
      running_ = true;
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

      scoped_array<char> executable(new char[executable_.size() + 1]);
      memcpy(executable.get(), executable_.c_str(), executable_.size());
      executable[executable_.size()] = '\0';

      char *const argv[] = {executable.get(), NULL};
      CHECK_SYSCALL(execv(executable.get(), argv));  // Never returns.
    }
  }

  void CheckedWrite(int fd, const void *buf, size_t len) {
    if (write(fd, buf, len) != len) {
      GOOGLE_LOG(FATAL) << current_test_name_
                        << ": error writing to test program: "
                        << strerror(errno);
    }
  }

  void CheckedRead(int fd, void *buf, size_t len) {
    size_t ofs = 0;
    while (len > 0) {
      ssize_t bytes_read = read(fd, (char*)buf + ofs, len);

      if (bytes_read == 0) {
        GOOGLE_LOG(FATAL) << current_test_name_
                          << ": unexpected EOF from test program";
      } else if (bytes_read < 0) {
        GOOGLE_LOG(FATAL) << current_test_name_
                          << ": error reading from test program: "
                          << strerror(errno);
      }

      len -= bytes_read;
      ofs += bytes_read;
    }
  }

  int write_fd_;
  int read_fd_;
  bool running_;
  std::string executable_;
  std::string current_test_name_;
};

void UsageError() {
  fprintf(stderr,
          "Usage: conformance-test-runner [options] <test-program>\n");
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
  exit(1);
}

void ParseFailureList(const char *filename, vector<string>* failure_list) {
  std::ifstream infile(filename);

  if (!infile.is_open()) {
    fprintf(stderr, "Couldn't open failure list file: %s\n", filename);
    exit(1);
  }

  for (string line; getline(infile, line);) {
    // Remove whitespace.
    line.erase(std::remove_if(line.begin(), line.end(), ::isspace),
               line.end());

    // Remove comments.
    line = line.substr(0, line.find("#"));

    if (!line.empty()) {
      failure_list->push_back(line);
    }
  }
}

int main(int argc, char *argv[]) {
  char *program;
  google::protobuf::ConformanceTestSuite suite;

  for (int arg = 1; arg < argc; ++arg) {
    if (strcmp(argv[arg], "--failure_list") == 0) {
      if (++arg == argc) UsageError();
      vector<string> failure_list;
      ParseFailureList(argv[arg], &failure_list);
      suite.SetFailureList(failure_list);
    } else if (strcmp(argv[arg], "--verbose") == 0) {
      suite.SetVerbose(true);
    } else if (argv[arg][0] == '-') {
      fprintf(stderr, "Unknown option: %s\n", argv[arg]);
      UsageError();
    } else {
      if (arg != argc - 1) {
        fprintf(stderr, "Too many arguments.\n");
        UsageError();
      }
      program = argv[arg];
    }
  }

  ForkPipeRunner runner(program);

  std::string output;
  bool ok = suite.RunSuite(&runner, &output);

  fwrite(output.c_str(), 1, output.size(), stderr);

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
