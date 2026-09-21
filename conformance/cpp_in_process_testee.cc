// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// The C++ runtime's in-process conformance testee: defines MakeTesteeRunner()
// (testee_runner.h) to host CppConformanceHarness in the test process.
// `conformance_test(testee = ":cpp_in_process_testee", in_process = True)`
// links it into the C++ suites' tests in place of :forked_testee, so they run
// the testee without forking conformance_cpp.  cpp_dynamic_in_process_testee
// is the same testee over DynamicMessage.  Other runtimes that can be linked
// into a C++ binary (e.g. upb) provide their own definition in their own
// library.

#include <memory>
#include <string>

#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "conformance/conformance_cpp_harness.h"
#include "conformance/cpp_in_process_test_runner.h"
#include "conformance/test_runner.h"
#include "conformance/testee_runner.h"

namespace google {
namespace protobuf {
namespace conformance {

std::unique_ptr<ConformanceTestRunner> MakeTesteeRunner(
    absl::string_view testee_binary,
    absl::Span<const std::string> testee_args) {
  return MakeCppInProcessTestRunnerOrDie(
      CppConformanceHarness::MessageImplementation::kGenerated,
      "cpp_in_process_testee", testee_binary, testee_args);
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google
