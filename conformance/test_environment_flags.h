// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// The command-line flags of a conformance test binary and their translation
// into ConformanceEnvironmentOptions.  test_environment_main.cc is the usual
// consumer (an in-process testee needs no main of its own: it defines
// MakeTesteeRunner(), see testee_runner.h).  A binary with its own main can
// link this library instead to get the same flags, call OptionsFromFlags() and
// set `owned_runner` before installing the environment.

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_TEST_ENVIRONMENT_FLAGS_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_TEST_ENVIRONMENT_FLAGS_H__

#include <string>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include "absl/flags/declare.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "conformance/test_environment.h"

namespace google {
namespace protobuf {
namespace conformance {

// The type of --maximum_edition: an Edition that parses from both the
// conformance_test_runner command line's short form ("2023", "PROTO3") and full
// enum names ("EDITION_2023").
// An empty value means the default.  Values below EDITION_PROTO3 are accepted
// as is; ConformanceEnvironment clamps them.
struct MaximumEdition {
  Edition edition = EDITION_PROTO3;
};

bool AbslParseFlag(absl::string_view text, MaximumEdition* maximum_edition,
                   std::string* error);
std::string AbslUnparseFlag(const MaximumEdition& maximum_edition);

// Builds the environment options from the flags declared below.
// `positional_args` are the non-flag command-line arguments (excluding
// argv[0]), which are appended to --testee_args.
//
// This doesn't insist on --testee_binary: whether it is required (forked
// testee) or must be absent (in-process testee) is up to the linked
// MakeTesteeRunner() (testee_runner.h), which test_environment_main.cc hands
// it to.
ConformanceEnvironmentOptions OptionsFromFlags(
    absl::Span<char* const> positional_args = {});

}  // namespace conformance
}  // namespace protobuf
}  // namespace google

ABSL_DECLARE_FLAG(std::string, testee_binary);
ABSL_DECLARE_FLAG(std::vector<std::string>, testee_args);
ABSL_DECLARE_FLAG(std::vector<std::string>, failure_list);
ABSL_DECLARE_FLAG(bool, enforce_recommended);
ABSL_DECLARE_FLAG(google::protobuf::conformance::MaximumEdition, maximum_edition);
ABSL_DECLARE_FLAG(bool, fix);
ABSL_DECLARE_FLAG(std::string, fix_output_file);

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_TEST_ENVIRONMENT_FLAGS_H__
