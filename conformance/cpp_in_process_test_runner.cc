// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/cpp_in_process_test_runner.h"

#include <memory>

#include "conformance/conformance.pb.h"
#include "conformance/conformance_cpp_harness.h"
#include "conformance/in_process_test_runner.h"
#include "conformance/test_runner.h"

namespace google {
namespace protobuf {
namespace conformance {

std::unique_ptr<ConformanceTestRunner> NewCppInProcessTestRunner() {
  return std::make_unique<InProcessTestRunner>(
      [harness = CppConformanceHarness()](
          const ::conformance::ConformanceRequest& request) {
        return harness.RunTest(request);
      });
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google
