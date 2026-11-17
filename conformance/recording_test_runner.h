// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_RECORDING_TEST_RUNNER_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_RECORDING_TEST_RUNNER_H__

#include <ostream>
#include <string>

#include "absl/base/nullability.h"
#include "absl/strings/string_view.h"
#include "conformance/test_runner.h"

namespace google {
namespace protobuf {
namespace conformance {

// Decorates a ConformanceTestRunner, appending one line per request to an
// output stream: "<test_name> <input_size> <crc32c>\n", where the size is that
// of the serialized request and the CRC32C (eight hex digits) is of a canonical
// rendering of it.  Conformance test names never contain spaces, so the line
// splits unambiguously on them.
//
// The canonical rendering is one "name: value" line per set ConformanceRequest
// field, in field-number order, so that the hash does not depend on the order
// the fields were serialized in (a serializer may legitimately emit them in any
// order, and non-optimized builds do).  Input that does not parse as a
// ConformanceRequest, or carries a field the runner never sends, is a bug and
// aborts.
//
// Used to produce a golden of exactly which requests a suite sends, so that
// refactorings of the suites can be verified to be behavior-preserving.
//
// Neither the delegate nor the stream is owned; both must outlive this object.
// The stream is flushed after every request so the recording is intact even if
// the process aborts mid-suite.
class RecordingTestRunner : public ConformanceTestRunner {
 public:
  RecordingTestRunner(ConformanceTestRunner* absl_nonnull delegate,
                      std::ostream* absl_nonnull out);
  ~RecordingTestRunner() override;

  RecordingTestRunner(const RecordingTestRunner&) = delete;
  RecordingTestRunner& operator=(const RecordingTestRunner&) = delete;

  std::string RunTest(absl::string_view test_name,
                      absl::string_view input) override;

 private:
  ConformanceTestRunner* absl_nonnull delegate_;
  std::ostream* absl_nonnull out_;
};

}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_RECORDING_TEST_RUNNER_H__
