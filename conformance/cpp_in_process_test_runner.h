// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_CPP_IN_PROCESS_TEST_RUNNER_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_CPP_IN_PROCESS_TEST_RUNNER_H__

#include <memory>

#include "conformance/test_runner.h"

namespace google {
namespace protobuf {
namespace conformance {

// Returns a ConformanceTestRunner that runs every request through a
// CppConformanceHarness owned by the runner, i.e. the C++ testee hosted in the
// test process instead of behind a fork/pipe.
//
// This is the glue between the testee side (conformance_cpp_harness) and the
// tester side (in_process_test_runner); it lives in its own library so that
// the conformance_cpp binary does not link any runner code.
std::unique_ptr<ConformanceTestRunner> NewCppInProcessTestRunner();

}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_CPP_IN_PROCESS_TEST_RUNNER_H__
