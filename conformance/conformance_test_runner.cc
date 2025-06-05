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

#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "conformance/conformance.pb.h"
#include "conformance_test.h"
#include "fork_pipe_runner.h"

using google::protobuf::ConformanceTestSuite;

namespace google {
namespace protobuf {
namespace {

void ParseFailureList(const char *filename,
                      conformance::FailureSet *failure_list) {
  std::ifstream infile(filename);

  if (!infile.is_open()) {
    fprintf(stderr, "Couldn't open failure list file: %s\n", filename);
    exit(1);
  }

  for (std::string line; std::getline(infile, line);) {
    // Remove comments.
    std::string test_name = line.substr(0, line.find('#'));

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
      std::string message;
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

}  // namespace

int RunConformanceTests(int argc, char *argv[],
                        const std::vector<ConformanceTestSuite *> &suites) {
  if (suites.empty()) {
    fprintf(stderr, "No test suites found.\n");
    return EXIT_FAILURE;
  }

  std::string program;
  std::string testee;
  std::vector<std::string> program_args;
  bool performance = false;
  bool debug = false;
  absl::flat_hash_set<std::string> names_to_test;
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
    std::string failure_list_filename;
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

    ForkPipeRunner runner(program, program_args);

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
    for (const std::string &test_name : names_to_test) {
      fprintf(stderr, "  %s\n", test_name.c_str());
    }
    fprintf(stderr, "\n\n");
  }
  return all_ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

}  // namespace protobuf
}  // namespace google
