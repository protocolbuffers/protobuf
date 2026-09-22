// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/cpp_in_process_test_runner.h"

#include <memory>
#include <string>

#include "absl/log/absl_check.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "conformance/conformance.pb.h"
#include "conformance/conformance_cpp_harness.h"
#include "conformance/in_process_test_runner.h"
#include "conformance/test_runner.h"

namespace google {
namespace protobuf {
namespace conformance {

std::unique_ptr<ConformanceTestRunner> NewCppInProcessTestRunner(
    CppConformanceHarness::MessageImplementation implementation) {
  return std::make_unique<InProcessTestRunner>(
      [harness = std::make_shared<CppConformanceHarness>(implementation)](
          const ::conformance::ConformanceRequest& request) {
        return harness->RunTest(request);
      });
}

std::unique_ptr<ConformanceTestRunner> MakeCppInProcessTestRunnerOrDie(
    CppConformanceHarness::MessageImplementation implementation,
    absl::string_view library_name, absl::string_view testee_binary,
    absl::Span<const std::string> testee_args) {
  ABSL_QCHECK(testee_binary.empty())
      << "--testee_binary=" << testee_binary
      << " was given, but this test links the C++ testee in-process ("
      << library_name
      << ") and can't run another binary; drop --testee_binary.";
  ABSL_QCHECK(testee_args.empty())
      << "--testee_args / positional arguments ["
      << absl::StrJoin(testee_args, " ")
      << "] were given, but this test links the C++ testee in-process ("
      << library_name << "), which takes no arguments; drop them.";
  return NewCppInProcessTestRunner(implementation);
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google
