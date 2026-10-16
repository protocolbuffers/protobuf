// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_MOCK_TEST_RUNNER_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_MOCK_TEST_RUNNER_H__

#include <string>

#include <gmock/gmock.h>
#include "absl/strings/string_view.h"
#include "conformance/test_runner.h"

namespace google {
namespace protobuf {
namespace conformance {

// gMock mock of the runner a Testee sends its requests through, for tests of
// the framework itself.  An expectation on RunTest() sees the test name and the
// serialized ConformanceRequest the framework sends, and answers with a
// serialized ConformanceResponse.
class MockTestRunner : public ConformanceTestRunner {
 public:
  MOCK_METHOD(std::string, RunTest,
              (absl::string_view test_name, absl::string_view input),
              (override));
};

}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_MOCK_TEST_RUNNER_H__
