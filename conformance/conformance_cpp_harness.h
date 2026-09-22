// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_CONFORMANCE_CPP_HARNESS_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_CONFORMANCE_CPP_HARNESS_H__

#include "absl/status/statusor.h"
#include "conformance/conformance.pb.h"

namespace google {
namespace protobuf {
namespace conformance {

// Implements the C++ conformance testee: given a ConformanceRequest, parses
// the payload into the requested test message, discards its unknown fields if
// the request asks for that, and serializes it in the requested output
// format. Shared by the conformance_cpp binary (stdin/stdout
// protocol) and, via cpp_in_process_test_runner.h, by in-process conformance
// tests.
//
// Instances are stateless and cheap to construct; RunTest is const, so a
// single instance may be shared across threads.
class CppConformanceHarness {
 public:
  // Force-links the reflection data of every test message and well-known type
  // into the binary so that they can be looked up by name in the generated
  // descriptor pool.
  CppConformanceHarness();

  // Runs a single conformance test.
  //
  // Returns a response with `parse_error` or `serialize_error` set when the
  // payload cannot be handled, and a non-OK status when the request itself is
  // malformed (unknown message type, missing payload, or an unspecified output
  // format).
  absl::StatusOr<::conformance::ConformanceResponse> RunTest(
      const ::conformance::ConformanceRequest& request) const;
};

}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_CONFORMANCE_CPP_HARNESS_H__
