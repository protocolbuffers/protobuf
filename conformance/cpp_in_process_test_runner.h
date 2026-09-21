// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_CPP_IN_PROCESS_TEST_RUNNER_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_CPP_IN_PROCESS_TEST_RUNNER_H__

#include <memory>
#include <string>

#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "conformance/conformance_cpp_harness.h"
#include "conformance/test_runner.h"

namespace google {
namespace protobuf {
namespace conformance {

// Returns a ConformanceTestRunner that runs every request through a
// CppConformanceHarness owned by the runner, i.e. the C++ testee hosted in the
// test process instead of behind a fork/pipe. `implementation` selects the
// message implementation the harness tests (generated classes by default, or
// DynamicMessage).
//
// This is the glue between the testee side (conformance_cpp_harness) and the
// tester side (in_process_test_runner); it lives in its own library so that
// the conformance_cpp binary does not link any runner code.
std::unique_ptr<ConformanceTestRunner> NewCppInProcessTestRunner(
    CppConformanceHarness::MessageImplementation implementation =
        CppConformanceHarness::MessageImplementation::kGenerated);

// The body of MakeTesteeRunner() (testee_runner.h) for the libraries that link
// the C++ testee into a conformance test (cpp_in_process_testee and
// cpp_dynamic_in_process_testee): returns
// NewCppInProcessTestRunner(implementation), once it has checked that
// `testee_binary` and `testee_args`, the forked-testee flags MakeTesteeRunner()
// is handed, are empty.  Those flags have no meaning for a linked-in testee;
// refusing them (with a QCHECK naming `library_name`, the library the test
// links, e.g. "cpp_in_process_testee") keeps a stale command line from silently
// testing the linked testee instead.
std::unique_ptr<ConformanceTestRunner> MakeCppInProcessTestRunnerOrDie(
    CppConformanceHarness::MessageImplementation implementation,
    absl::string_view library_name, absl::string_view testee_binary,
    absl::Span<const std::string> testee_args);

}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_CPP_IN_PROCESS_TEST_RUNNER_H__
