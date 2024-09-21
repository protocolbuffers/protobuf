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

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <future>
#include <memory>
#include <string>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_log.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "conformance/conformance.pb.h"
#include "conformance_test.h"
#include "google/protobuf/endian.h"

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

  for (string line; std::getline(infile, line);) {
    // Remove comments.
    string test_name = line.substr(0, line.find('#'));

    test_name.erase(
        std::remove_if(test_name.begin(), test_name.end(), ::isspace),
        test_name.end());

    if (test_name.empty()) {  // Skip empty lines.
      continue;
    }

    // If we remove whitespace from the beginning of a line, and what we have
    // left at first is a '#', then we have a comment.
    if (test_name[0] != '#') {
      // Find our failure message if it exists. Will be set to an empty string
      // if no message is found. Empty failure messages also pass our tests.
      size_t check_message = line.find('#');
      string message;
      if (check_message != std::string::npos) {
        message = line.substr(check_message + 1);  // +1 to skip the delimiter
        // If we had only whitespace after the delimiter, we will have an empty
        // failure message and the test will still pass.
        message = std::string(absl::StripAsciiWhitespace(message));
      }
      conformance::TestStatus *test = failure_list->add_test();
      test->set_name(test_name);
      test->set_failure_message(message);
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
          "                              line.  Use '#' for comments.\n\n");
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
          "                              per line.  Use '#' for comments.\n\n");

  fprintf(stderr,
          "  --enforce_recommended       Enforce that recommended test\n");
  fprintf(stderr,
          "                              cases are also passing. Specify\n");
  fprintf(stderr,
          "                              this flag if you want to be\n");
  fprintf(stderr,
          "                              strictly conforming to protobuf\n");
  fprintf(stderr, "                              spec.\n\n");
  fprintf(stderr,
          "  --maximum_edition <edition> Only run conformance tests up to\n");
  fprintf(stderr,
          "                              and including the specified\n");
  fprintf(stderr, "                              edition.\n\n");
  fprintf(stderr,
          "  --output_dir                <dirname> Directory to write\n"
          "                              output files.\n\n");
  fprintf(stderr, "  --test <test_name>          Only run\n");
  fprintf(stderr,
          "                              the specified test. Multiple tests\n"
          "                              can be specified by repeating the \n"
          "                              flag.\n\n");
  fprintf(stderr,
          "  --debug                     Enable debug mode\n"
          "                              to produce octal serialized\n"
          "                              ConformanceRequest for the tests\n"
          "                              passed to --test (required)\n\n");
  fprintf(stderr, "  --performance               Boolean option\n");
  fprintf(stderr, "                              for enabling run of\n");
  fprintf(stderr, "                              performance tests.\n");
  exit(1);
}

void ForkPipeRunner::RunTest(const std::string &test_name, uint32_t len,
                             const std::string &request,
                             std::string *response) {
  if (child_pid_ < 0) {
    SpawnTestProgram();
  }
  current_test_name_ = test_name;

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

  len = internal::little_endian::ToHost(len);
  response->resize(len);
  CheckedRead(read_fd_, (void *)response->c_str(), len);
}

int ForkPipeRunner::Run(int argc, char *argv[],
                        const std::vector<ConformanceTestSuite *> &suites) {
  if (suites.empty()) {
    fprintf(stderr, "No test suites found.\n");
    return EXIT_FAILURE;
  }

  string program;
  string testee;
  std::vector<string> program_args;
  bool performance = false;
  bool debug = false;
  absl::flat_hash_set<string> names_to_test;
  bool enforce_recommended = false;
  Edition maximum_edition = EDITION_UNKNOWN;
  std::string output_dir;
  bool verbose = false;
  bool isolated = false;

  for (int arg = 1; arg < argc; ++arg) {
    if (strcmp(argv[arg], "--performance") == 0) {
      performance = true;
    } else if (strcmp(argv[arg], "--debug") == 0) {
      debug = true;
    } else if (strcmp(argv[arg], "--verbose") == 0) {
      verbose = true;
    } else if (strcmp(argv[arg], "--enforce_recommended") == 0) {
      enforce_recommended = true;
    } else if (strcmp(argv[arg], "--maximum_edition") == 0) {
      if (++arg == argc) UsageError();
      Edition edition = EDITION_UNKNOWN;
      if (!Edition_Parse(absl::StrCat("EDITION_", argv[arg]), &edition)) {
        fprintf(stderr, "Unknown edition: %s\n", argv[arg]);
        UsageError();
      }
      maximum_edition = edition;
    } else if (strcmp(argv[arg], "--output_dir") == 0) {
      if (++arg == argc) UsageError();
      output_dir = argv[arg];

    } else if (strcmp(argv[arg], "--test") == 0) {
      if (++arg == argc) UsageError();
      names_to_test.insert(argv[arg]);

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

  if (debug && names_to_test.empty()) {
    UsageError();
  }

  if (!names_to_test.empty()) {
    isolated = true;
  }

  bool all_ok = true;
  for (ConformanceTestSuite *suite : suites) {
    string failure_list_filename;
    conformance::FailureSet failure_list;
    for (int arg = 1; arg < argc; ++arg) {
      if (strcmp(argv[arg], suite->GetFailureListFlagName().c_str()) == 0) {
        if (++arg == argc) UsageError();
        failure_list_filename = argv[arg];
        ParseFailureList(argv[arg], &failure_list);
      }
    }
    suite->SetPerformance(performance);
    suite->SetVerbose(verbose);
    suite->SetEnforceRecommended(enforce_recommended);
    suite->SetMaximumEdition(maximum_edition);
    suite->SetOutputDir(output_dir);
    suite->SetDebug(debug);
    suite->SetNamesToTest(names_to_test);
    suite->SetTestee(program);
    suite->SetIsolated(isolated);

    ForkPipeRunner runner(program, program_args, performance);

    std::string output;
    all_ok = all_ok && suite->RunSuite(&runner, &output, failure_list_filename,
                                       &failure_list);

    names_to_test = suite->GetExpectedTestsNotRun();
    fwrite(output.c_str(), 1, output.size(), stderr);
  }

  if (!names_to_test.empty()) {
    fprintf(stderr,
            "These tests were requested to be ran isolated, but they do "
            "not exist. Revise the test names:\n\n");
    for (const string &test_name : names_to_test) {
      fprintf(stderr, "  %s\n", test_name.c_str());
    }
    fprintf(stderr, "\n\n");
  }
  return all_ok ? EXIT_SUCCESS : EXIT_FAILURE;
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
        // TODO: Only log in flag-guarded mode, since reading output
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
