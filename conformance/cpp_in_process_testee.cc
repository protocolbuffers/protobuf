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
// the testee without forking conformance_cpp.  Other runtimes that can be
// linked into a C++ binary (e.g. upb) provide their own definition in their
// own library.

#include <memory>
#include <string>

#include "absl/log/absl_check.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "conformance/cpp_in_process_test_runner.h"
#include "conformance/test_runner.h"
#include "conformance/testee_runner.h"

namespace google {
namespace protobuf {
namespace conformance {

std::unique_ptr<ConformanceTestRunner> MakeTesteeRunner(
    absl::string_view testee_binary,
    absl::Span<const std::string> testee_args) {
  // The flags of a forked testee have no meaning here; refusing them keeps a
  // stale command line from silently testing the linked testee instead.
  ABSL_QCHECK(testee_binary.empty())
      << "--testee_binary=" << testee_binary
      << " was given, but this test links the C++ testee in-process "
         "(cpp_in_process_testee) and can't run another binary; drop "
         "--testee_binary.";
  ABSL_QCHECK(testee_args.empty())
      << "--testee_args / positional arguments ["
      << absl::StrJoin(testee_args, " ")
      << "] were given, but this test links the C++ testee in-process "
         "(cpp_in_process_testee), which takes no arguments; drop them.";
  return NewCppInProcessTestRunner();
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google
