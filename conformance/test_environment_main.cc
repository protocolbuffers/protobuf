// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// A gtest main for conformance test binaries.  It configures a
// ConformanceEnvironment from the command-line flags declared in
// test_environment_flags.h and installs it before running all tests.
// Conformance suites should depend on the test_environment_main library
// instead of a generic gtest main.
//
// The testee is whatever the library defining MakeTesteeRunner() (see
// testee_runner.h) makes of --testee_binary and --testee_args; the binary must
// link exactly one such library, :forked_testee by default.  Any positional
// arguments left after flag parsing are appended to --testee_args, so
// `conformance_test --testee_binary=foo -- --bar` passes `--bar` to a forked
// testee.

#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include "absl/types/span.h"
#include "test_environment.h"
#include "test_environment_flags.h"
#include "testee_runner.h"

#include "absl/flags/parse.h"

int main(int argc, char* argv[]) {
  // In google3, InitGoogleTest() also runs InitGoogle(), which parses absl
  // flags and leaves only positional arguments in argv.  In open source we
  // parse them ourselves, after gtest has removed its own flags from argv.
  testing::InitGoogleTest(&argc, argv);
  std::vector<char*> positional_args(argv, argv + argc);
  positional_args = absl::ParseCommandLine(argc, argv);

  // Listing tests doesn't need a testee, and shouldn't require any flags.
  if (!GTEST_FLAG_GET(list_tests)) {
    google::protobuf::conformance::ConformanceEnvironmentOptions options =
        google::protobuf::conformance::OptionsFromFlags(
            absl::MakeConstSpan(positional_args).subspan(1));
    // The linked testee decides what the flags mean, and the environment
    // takes its runner instead of the flags (it would otherwise spawn
    // --testee_binary itself).
    options.owned_runner = google::protobuf::conformance::MakeTesteeRunner(
        options.testee_binary, options.testee_args);
    options.testee_binary.clear();
    options.testee_args.clear();
    google::protobuf::conformance::ConformanceEnvironment::Install(std::move(options));
  }
  return RUN_ALL_TESTS();
}
