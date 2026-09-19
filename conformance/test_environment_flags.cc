// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Defines the global --testee_binary, --testee_args, --failure_list,
// --enforce_recommended, --maximum_edition, --performance, --fix and
// --fix_output_file flags.  Being process-global ABSL_FLAGs with short, generic
// names, they must only be linked into binaries with a test_environment_main-
// style main (see test_environment_flags.h), never into
// conformance_test_runner, whose command line parses flags of the same names
// itself.

#include "test_environment_flags.h"

#include <string>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include "absl/flags/flag.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "absl/types/span.h"
#include "test_environment.h"

ABSL_FLAG(std::string, testee_binary, "",
          "The testee program to run conformance tests against.");
ABSL_FLAG(std::vector<std::string>, testee_args, {},
          "Comma-separated arguments to pass to --testee_binary.  Positional "
          "arguments are appended to these.");
ABSL_FLAG(std::vector<std::string>, failure_list, {},
          "Comma-separated list of files containing the tests that are "
          "expected to fail, one test name per line.  Use '#' for comments.");
ABSL_FLAG(bool, enforce_recommended, false,
          "Whether failures in recommended tests fail the run.  Off by "
          "default, like the conformance_test_runner command line; recommended "
          "tests listed in the failure list are checked either way.");
ABSL_FLAG(google::protobuf::conformance::MaximumEdition, maximum_edition, {},
          "Only run conformance tests up to and including this edition, e.g. "
          "\"2023\" or \"EDITION_2023\".  Defaults to proto2 and proto3 only.");
ABSL_FLAG(bool, performance, false,
          "Run only the performance tests instead of the regular tests.  A "
          "performance run needs its own failure list.");
ABSL_FLAG(bool, fix, false,
          "Rewrite the failure list at the end of the run to match the "
          "observed results.  See --fix_output_file.  Refused when only some "
          "of the tests run (--gtest_filter, sharding).  Use it via `bazel run "
          "<target> -- --fix`: under `bazel test` the file is written inside "
          "the test sandbox and discarded.");
ABSL_FLAG(std::string, fix_output_file, "",
          "Where --fix writes the updated failure list.  Required when more "
          "than one --failure_list is given; otherwise defaults to the "
          "failure list itself.  A relative path (this flag's or the failure "
          "list's) is resolved under $BUILD_WORKSPACE_DIRECTORY when that is "
          "set, so that `bazel run ... -- --fix` creates or updates the file "
          "in your workspace.");

namespace google {
namespace protobuf {
namespace conformance {

bool AbslParseFlag(absl::string_view text, MaximumEdition* maximum_edition,
                   std::string* error) {
  if (text.empty()) {
    *maximum_edition = MaximumEdition();
    return true;
  }
  std::string name(text);
  if (!absl::StartsWith(name, "EDITION_")) {
    name = absl::StrCat("EDITION_", name);
  }
  if (!Edition_Parse(name, &maximum_edition->edition)) {
    *error = absl::StrCat("Unknown edition \"", text,
                          "\"; expected e.g. \"2023\" or \"EDITION_2023\".");
    return false;
  }
  return true;
}

std::string AbslUnparseFlag(const MaximumEdition& maximum_edition) {
  return std::string(
      absl::StripPrefix(Edition_Name(maximum_edition.edition), "EDITION_"));
}

ConformanceEnvironmentOptions OptionsFromFlags(
    absl::Span<char* const> positional_args) {
  ConformanceEnvironmentOptions options;
  options.testee_binary = absl::GetFlag(FLAGS_testee_binary);
  options.testee_args = absl::GetFlag(FLAGS_testee_args);
  for (const char* positional_arg : positional_args) {
    options.testee_args.push_back(positional_arg);
  }
  options.failure_list_files = absl::GetFlag(FLAGS_failure_list);
  options.enforce_recommended = absl::GetFlag(FLAGS_enforce_recommended);
  options.maximum_edition = absl::GetFlag(FLAGS_maximum_edition).edition;
  options.performance = absl::GetFlag(FLAGS_performance);
  options.fix = absl::GetFlag(FLAGS_fix);
  options.fix_output_file = absl::GetFlag(FLAGS_fix_output_file);
  return options;
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google
