// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// The command-line flags of a conformance test binary and their translation
// into ConformanceEnvironmentOptions.  test_environment_main.cc is the usual
// consumer.  A binary with its own main, for example one hosting an
// in-process testee, can link this library instead to get the same flags.  It
// then calls OptionsFromFlags() and sets `runner` or `owned_runner` before
// installing the environment.

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_TEST_ENVIRONMENT_FLAGS_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_TEST_ENVIRONMENT_FLAGS_H__

#include <string>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include "absl/flags/declare.h"
#include "absl/types/span.h"
#include "conformance/test_environment.h"
#include "conformance/testee.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {

// Builds the environment options from the flags declared below.
// `positional_args` are the non-flag command-line arguments, excluding
// argv[0].  They are appended to --testee_args.
//
//   ConformanceEnvironment::Install(OptionsFromFlags());
//
// This doesn't insist on --testee_binary.  A main hosting an in-process testee
// leaves it unset and fills in `runner` or `owned_runner` instead.
// test_environment_main.cc checks it.
ConformanceEnvironmentOptions OptionsFromFlags(
    absl::Span<char* const> positional_args = {});

}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google

ABSL_DECLARE_FLAG(std::string, testee_binary);
ABSL_DECLARE_FLAG(std::vector<std::string>, testee_args);
ABSL_DECLARE_FLAG(std::vector<std::string>, failure_list);
ABSL_DECLARE_FLAG(google::protobuf::conformance::TestPriority, enforcement_level);
ABSL_DECLARE_FLAG(google::protobuf::Edition, maximum_edition);
ABSL_DECLARE_FLAG(bool, performance);  // TODO: Transitional.
ABSL_DECLARE_FLAG(bool, fix);
ABSL_DECLARE_FLAG(std::string, fix_output_file);

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_TEST_ENVIRONMENT_FLAGS_H__
