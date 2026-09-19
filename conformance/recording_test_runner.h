// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_RECORDING_TEST_RUNNER_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_RECORDING_TEST_RUNNER_H__

#include <cstdint>
#include <ostream>
#include <string>

#include "absl/base/nullability.h"
#include "absl/strings/string_view.h"
#include "test_runner.h"

namespace google {
namespace protobuf {
namespace conformance {

namespace internal {

// Exposed for testing:

// Returns the 64-bit FNV-1a hash of `data`.
uint64_t Fnv1a64(absl::string_view data);

// Returns a canonical rendering of a serialized ConformanceRequest: the
// text-format of the parsed message, whose fields are always printed in
// field-number order.  The wire bytes themselves are not canonical (a
// serializer may legitimately emit fields in any order), so hashing them
// directly would make the recording depend on the build configuration.
// Input that does not parse as a ConformanceRequest (proto3 rejects invalid
// UTF-8 in string fields) is canonicalized without the schema instead: its
// wire-format fields re-serialized in field-number order.  Only input that is
// not even valid wire format is returned verbatim.
std::string CanonicalizeRequest(absl::string_view input);

}  // namespace internal

// Decorates a ConformanceTestRunner, appending one line per request to an
// output stream: "<test_name> <input_size> <fnv1a64_hex>\n", where the size
// is that of the serialized request and the hash is of its canonical
// rendering (see CanonicalizeRequest).  Conformance test names never contain
// spaces, so the line splits unambiguously on them.
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
