// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// The command line parser and the report writer of conformance_test_runner;
// see conformance_test_runner.h and, for the runner itself,
// conformance_test_main.cc.

#include "conformance/conformance_test_runner.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/match.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"

namespace google {
namespace protobuf {

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
          "                              text-format conformance tests.  \n");
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
          "  --debug                     Accepted for compatibility; has\n"
          "                              no effect.  Requires --test, as\n"
          "                              it always did.\n\n");
  fprintf(stderr, "  --performance               Boolean option\n");
  fprintf(stderr, "                              for enabling run of\n");
  fprintf(stderr, "                              performance tests.\n\n");
  fprintf(stderr,
          "  --protocol_version <version> Speak the given conformance\n"
          "                              protocol version to the testee\n"
          "                              instead of detecting it, failing\n"
          "                              if the testee doesn't support it.\n"
          "                              0 (the default) detects it.\n\n");
  fprintf(stderr,
          "  --verbose                   Accepted for compatibility; has\n"
          "                              no effect (use --gtest_brief=0 to\n"
          "                              list every test's result).\n\n");
  fprintf(
      stderr,
      "  --output_result_file        <filename> Write the results as a\n"
      "                              ConformanceRunResult textproto to\n"
      "                              this file (for report_generator.py).\n\n");
  fprintf(
      stderr,
      "  --implementation_name       <name> The implementation name\n"
      "                              recorded in --output_result_file;\n"
      "                              defaults to the testee's basename.\n\n");
  fprintf(stderr,
          "  --gtest_<flag>=<value>      Passed on to the gtest-based\n"
          "                              suites (e.g. --gtest_filter,\n"
          "                              --gtest_output); --gunit_ is\n"
          "                              accepted too.  Must precede the\n"
          "                              testee and use the --flag=value\n"
          "                              form.\n");
}

absl::StatusOr<ConformanceRunnerOptions> ParseConformanceRunnerArgs(
    int argc, char* argv[]) {
  ConformanceRunnerOptions options;
  bool debug = false;

  // The error for a flag at `arg` that takes a value but is the last argument.
  auto missing_value = [&](int arg) {
    return absl::InvalidArgumentError(
        absl::StrCat("Missing value for ", argv[arg]));
  };

  for (int arg = 1; arg < argc; ++arg) {
    if (strcmp(argv[arg], "--performance") == 0) {
      options.performance = true;
    } else if (strcmp(argv[arg], "--debug") == 0) {
      // No effect (see ConformanceRunnerOptions); only its "requires --test"
      // check below survives.
      debug = true;
    } else if (strcmp(argv[arg], "--verbose") == 0) {
      // No effect (see ConformanceRunnerOptions).
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
    } else if (strcmp(argv[arg], "--protocol_version") == 0) {
      if (arg + 1 == argc) return missing_value(arg);
      ++arg;
      // Only the syntax is checked here; whether the version is one the runner
      // speaks is for ConformanceEnvironment::SetUp() to decide, which knows
      // the latest version.
      int protocol_version = 0;
      if (!absl::SimpleAtoi(argv[arg], &protocol_version) ||
          protocol_version < 0) {
        return absl::InvalidArgumentError(
            absl::StrCat("Invalid protocol version: ", argv[arg]));
      }
      options.protocol_version = protocol_version;

    } else if (strcmp(argv[arg], "--test") == 0) {
      if (arg + 1 == argc) return missing_value(arg);
      options.names_to_test.insert(argv[++arg]);

    } else if (strcmp(argv[arg], "--output_result_file") == 0) {
      if (arg + 1 == argc) return missing_value(arg);
      options.output_result_file = argv[++arg];
    } else if (strcmp(argv[arg], "--implementation_name") == 0) {
      if (arg + 1 == argc) return missing_value(arg);
      options.implementation_name = argv[++arg];

    } else if (argv[arg] == kFailureListFlag ||
               argv[arg] == kTextFormatFailureListFlag) {
      if (arg + 1 == argc) return missing_value(arg);
      options.failure_list_files.emplace_back(argv[arg], argv[arg + 1]);
      ++arg;

    } else if (absl::StartsWith(argv[arg], "--gtest_") ||
               absl::StartsWith(argv[arg], "--gunit_")) {
      options.gtest_args.push_back(argv[arg]);

    } else if (argv[arg][0] == '-') {
      return absl::InvalidArgumentError(
          absl::StrCat("Unknown option: ", argv[arg]));
    } else {
      options.testee = argv[arg++];
      while (arg < argc) {
        options.testee_args.push_back(argv[arg]);
        arg++;
      }
    }
  }

  if (debug && options.names_to_test.empty()) {
    return absl::InvalidArgumentError("--debug requires --test");
  }
  options.isolated = !options.names_to_test.empty();

  return options;
}

bool ReportTestStatusSet(absl::Span<const ReportedTestStatus> statuses,
                         absl::string_view file_name, absl::string_view message,
                         absl::string_view output_dir, std::string* output) {
  if (statuses.empty()) return true;

  absl::StrAppendFormat(output, "\n");
  absl::StrAppendFormat(output, "%s\n\n", message);
  for (const ReportedTestStatus& status : statuses) {
    absl::StrAppendFormat(output, "  %s # %s\n", status.test_name,
                          status.failure_message);
  }
  absl::StrAppendFormat(output, "\n");

  if (!file_name.empty()) {
    std::string full_filename;
    absl::string_view filename = file_name;
    if (!output_dir.empty()) {
      full_filename = std::string(output_dir);
      absl::StrAppend(&full_filename, file_name);
      filename = full_filename;
    }
    std::ofstream os{std::string(filename)};
    if (os) {
      for (const ReportedTestStatus& status : statuses) {
        // Additions will not have a 'matched_name' while removals will.
        absl::string_view potential_add_or_removal = status.matched_name.empty()
                                                         ? status.test_name
                                                         : status.matched_name;
        os << potential_add_or_removal << " # " << status.failure_message
           << "\n";
      }
    } else {
      absl::StrAppendFormat(output,
                            "Failed to open file: %s\n",
                            filename);
    }
  }

  return false;
}

}  // namespace protobuf
}  // namespace google
