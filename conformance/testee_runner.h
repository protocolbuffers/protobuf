// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// The link-time seam that decides which testee a conformance test binary
// (one with test_environment_main.cc's main) runs against.
//
// Exactly one library linked into the binary defines MakeTesteeRunner():
//
//  * :forked_testee, which conformance_test() (conformance.bzl) links by
//    default, spawns --testee_binary with --testee_args and talks to it over
//    pipes (ForkPipeRunner).
//  * An in-process testee library answers the requests in this process, which
//    makes the testee debuggable from the test binary and needs no fork or
//    pipe; conformance_test(in_process = True) links it in place of
//    :forked_testee.

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_TESTEE_RUNNER_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_TESTEE_RUNNER_H__

#include <memory>
#include <string>

#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "test_runner.h"

namespace google {
namespace protobuf {
namespace conformance {

// Returns the runner the conformance suites of this binary talk to, given the
// values of --testee_binary and --testee_args (the latter including any
// positional arguments).  Never returns null; check-fails, with a message
// naming the flag, on a command line the linked testee can't make sense of:
// a forked testee needs `testee_binary`, whereas an in-process testee rejects
// a non-empty `testee_binary` or `testee_args` rather than silently running
// the linked testee instead of the requested one.
//
// Called once, before the tests run (see test_environment_main.cc), and the
// result is destroyed by ConformanceEnvironment::TearDown().
std::unique_ptr<ConformanceTestRunner> MakeTesteeRunner(
    absl::string_view testee_binary, absl::Span<const std::string> testee_args);

}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_TESTEE_RUNNER_H__
