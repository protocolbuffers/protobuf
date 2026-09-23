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

#include <stdio.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <ostream>
#include <string>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include "absl/base/nullability.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "absl/types/span.h"
#include "conformance/conformance.pb.h"
#include "conformance/conformance_test.h"
#include "conformance/fork_pipe_runner.h"
#include "conformance/recording_test_runner.h"
#include "conformance/test_runner.h"

using google::protobuf::ConformanceTestSuite;

namespace google {
namespace protobuf {
namespace {

void ParseFailureList(const char* filename,
                      ::conformance::FailureSet* failure_list) {
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
      ::conformance::TestStatus* test = failure_list->add_test();
      test->set_name(test_name);
      test->set_failure_message(message);
    }
  }
}

}  // namespace

std::vector<std::string> ConformanceRunnerOptions::FailureListFilesFor(
    absl::string_view failure_list_flag) const {
  std::vector<std::string> files;
  for (const auto& [flag, file] : failure_list_files) {
    if (flag == failure_list_flag) files.push_back(file);
  }
  return files;
}

std::string RewriteGtestFlagPrefix(absl::string_view arg,
                                   absl::string_view flag_prefix) {
  constexpr absl::string_view kGtestPrefix = "--gtest_";
  if (!absl::StartsWith(arg, kGtestPrefix)) return std::string(arg);
  return absl::StrCat("--", flag_prefix, arg.substr(kGtestPrefix.size()));
}

bool IsPerformanceFixture(absl::string_view test_suite_name) {
  for (absl::string_view component : absl::StrSplit(test_suite_name, '/')) {
    if (absl::EndsWith(component, "PerformanceTest")) return true;
  }
  return false;
}

std::string PerformanceGtestFilter(
    absl::string_view filter, bool performance,
    absl::Span<const std::string> test_suite_names) {
  std::vector<std::string> excluded;
  for (const std::string& test_suite_name : test_suite_names) {
    if (IsPerformanceFixture(test_suite_name) != performance) {
      excluded.push_back(absl::StrCat(test_suite_name, ".*"));
    }
  }
  if (excluded.empty()) return std::string(filter);

  // As gtest reads a filter: everything before the first '-' is the positive
  // patterns (none means "*"), everything after it the negative ones.
  absl::string_view positive = filter;
  absl::string_view negative;
  if (size_t dash = filter.find('-'); dash != absl::string_view::npos) {
    positive = filter.substr(0, dash);
    negative = filter.substr(dash + 1);
  }
  if (positive.empty()) positive = "*";
  return absl::StrCat(positive, "-", negative, negative.empty() ? "" : ":",
                      absl::StrJoin(excluded, ":"));
}

void PrintConformanceRunnerUsage(absl::string_view error_message) {
  if (!error_message.empty()) {
    absl::FPrintF(stderr, "%s\n", error_message);
  }
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
  fprintf(stderr, "                              performance tests.\n\n");
  fprintf(stderr,
          "  --verbose                   Also report every request,\n"
          "                              response and skipped test.\n\n");
  fprintf(stderr,
          "  --record_requests <file>    Write one line per request sent\n"
          "                              to the testee, of the form\n"
          "                              '<test_name> <input_size> "
          "<fnv1a64_hex>', where the\n"
          "                              hash is of the request's\n"
          "                              canonical (text-format)\n"
          "                              rendering, for verifying that\n"
          "                              refactorings of the suites\n"
          "                              preserve behavior.\n\n");
  fprintf(stderr,
          "  --gtest_<flag>=<value>      Passed on to the gtest-based\n"
          "                              suites (e.g. --gtest_filter,\n"
          "                              --gtest_output); --gunit_ is\n"
          "                              accepted too.  Must precede the\n"
          "                              testee and use the --flag=value\n"
          "                              form.\n");
}

absl::StatusOr<ConformanceRunnerOptions> ParseConformanceRunnerArgs(
    int argc, char* argv[], absl::Span<ConformanceTestSuite* const> suites) {
  ConformanceRunnerOptions options;

  // The error for a flag at `arg` that takes a value but is the last argument.
  auto missing_value = [&](int arg) {
    return absl::InvalidArgumentError(
        absl::StrCat("Missing value for ", argv[arg]));
  };

  for (int arg = 1; arg < argc; ++arg) {
    if (strcmp(argv[arg], "--performance") == 0) {
      options.performance = true;
    } else if (strcmp(argv[arg], "--debug") == 0) {
      options.debug = true;
    } else if (strcmp(argv[arg], "--verbose") == 0) {
      options.verbose = true;
    } else if (strcmp(argv[arg], "--enforce_recommended") == 0) {
      options.enforce_recommended = true;
    } else if (strcmp(argv[arg], "--maximum_edition") == 0) {
      if (arg + 1 == argc) return missing_value(arg);
      ++arg;
      Edition edition = EDITION_UNKNOWN;
      if (!Edition_Parse(absl::StrCat("EDITION_", argv[arg]), &edition)) {
        return absl::InvalidArgumentError(
            absl::StrCat("Unknown edition: ", argv[arg]));
      }
      options.maximum_edition = edition;
    } else if (strcmp(argv[arg], "--output_dir") == 0) {
      if (arg + 1 == argc) return missing_value(arg);
      options.output_dir = argv[++arg];

    } else if (strcmp(argv[arg], "--record_requests") == 0) {
      if (arg + 1 == argc) return missing_value(arg);
      options.record_requests_file = argv[++arg];

    } else if (strcmp(argv[arg], "--test") == 0) {
      if (arg + 1 == argc) return missing_value(arg);
      options.names_to_test.insert(argv[++arg]);

    } else if (absl::StartsWith(argv[arg], "--gtest_") ||
               absl::StartsWith(argv[arg], "--gunit_")) {
      options.gtest_args.push_back(argv[arg]);

    } else if (argv[arg][0] == '-') {
      bool recognized_flag = false;
      for (ConformanceTestSuite* suite : suites) {
        if (strcmp(argv[arg], suite->GetFailureListFlagName().c_str()) == 0) {
          if (arg + 1 == argc) return missing_value(arg);
          options.failure_list_files.emplace_back(
              suite->GetFailureListFlagName(), argv[++arg]);
          recognized_flag = true;
        }
      }
      if (!recognized_flag) {
        return absl::InvalidArgumentError(
            absl::StrCat("Unknown option: ", argv[arg]));
      }
    } else {
      options.testee = argv[arg++];
      while (arg < argc) {
        options.testee_args.push_back(argv[arg]);
        arg++;
      }
    }
  }

  if (options.debug && options.names_to_test.empty()) {
    return absl::InvalidArgumentError("--debug requires --test");
  }
  options.isolated = !options.names_to_test.empty();

  return options;
}

bool RunConformanceSuites(const ConformanceRunnerOptions& options,
                          absl::Span<ConformanceTestSuite* const> suites,
                          std::ostream* absl_nullable record_requests,
                          const absl::flat_hash_set<std::string>* absl_nullable
                              unmatched_candidates) {
  // In isolated mode only the named tests run and each suite removes the ones
  // it ran; whatever is left at the end does not exist.
  absl::flat_hash_set<std::string> names_to_test = options.names_to_test;

  bool all_ok = true;
  for (ConformanceTestSuite* suite : suites) {
    std::string failure_list_filename;
    ::conformance::FailureSet failure_list;
    for (const std::string& file :
         options.FailureListFilesFor(suite->GetFailureListFlagName())) {
      failure_list_filename = file;
      ParseFailureList(file.c_str(), &failure_list);
    }
    suite->SetPerformance(options.performance);
    suite->SetVerbose(options.verbose);
    suite->SetEnforceRecommended(options.enforce_recommended);
    suite->SetMaximumEdition(options.maximum_edition);
    suite->SetOutputDir(options.output_dir);
    suite->SetDebug(options.debug);
    suite->SetNamesToTest(names_to_test);
    suite->SetTestee(options.testee);
    suite->SetIsolated(options.isolated);
    if (unmatched_candidates != nullptr) {
      suite->SetUnmatchedCandidates(*unmatched_candidates);
    }

    ForkPipeRunner fork_pipe_runner(options.testee, options.testee_args);
    ConformanceTestRunner* runner = &fork_pipe_runner;
    absl::optional<conformance::RecordingTestRunner> recording_runner;
    if (record_requests != nullptr) {
      recording_runner.emplace(&fork_pipe_runner, record_requests);
      runner = &*recording_runner;
    }

    std::string output;
    // Note that this deliberately keeps the historical behavior of not running
    // any further suite once one has failed.
    all_ok = all_ok && suite->RunSuite(runner, &output, failure_list_filename,
                                       &failure_list);

    names_to_test = suite->GetExpectedTestsNotRun();
    fwrite(output.c_str(), 1, output.size(), stderr);
  }

  if (!names_to_test.empty()) {
    fprintf(stderr,
            "These tests were requested to be ran isolated, but they do "
            "not exist. Revise the test names:\n\n");
    for (const std::string& test_name : names_to_test) {
      fprintf(stderr, "  %s\n", test_name.c_str());
    }
    fprintf(stderr, "\n\n");
  }
  return all_ok;
}

int RunConformanceTests(int argc, char* argv[],
                        const std::vector<ConformanceTestSuite*>& suites) {
  if (suites.empty()) {
    fprintf(stderr, "No test suites found.\n");
    return EXIT_FAILURE;
  }

  absl::StatusOr<ConformanceRunnerOptions> options =
      ParseConformanceRunnerArgs(argc, argv, suites);
  if (!options.ok()) {
    PrintConformanceRunnerUsage(options.status().message());
    return EXIT_FAILURE;
  }
  // gtest flags are only meaningful to the merged runner.
  if (!options->gtest_args.empty()) {
    PrintConformanceRunnerUsage(
        absl::StrCat("Unknown option: ", options->gtest_args.front()));
    return EXIT_FAILURE;
  }

  // Opened once (truncating) so that requests from all suites are appended in
  // order to a single file.
  std::ofstream record_requests_file;
  if (!options->record_requests_file.empty()) {
    record_requests_file.open(options->record_requests_file);
    if (!record_requests_file.is_open()) {
      fprintf(stderr, "Couldn't open record_requests file: %s\n",
              options->record_requests_file.c_str());
      return EXIT_FAILURE;
    }
  }

  bool all_ok = RunConformanceSuites(
      *options, suites,
      record_requests_file.is_open() ? &record_requests_file : nullptr,
      /*unmatched_candidates=*/nullptr);

  if (record_requests_file.is_open()) {
    record_requests_file.close();
    if (!record_requests_file) {
      fprintf(stderr, "Error writing record_requests file: %s\n",
              options->record_requests_file.c_str());
      return EXIT_FAILURE;
    }
  }

  return all_ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

}  // namespace protobuf
}  // namespace google
