// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_CONFORMANCE_CPP_HARNESS_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_CONFORMANCE_CPP_HARNESS_H__

#include <memory>

#include "absl/status/statusor.h"
#include "conformance/conformance.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/message.h"

namespace google {
namespace protobuf {
namespace conformance {

// Implements the C++ conformance testee: given a ConformanceRequest, parses
// the payload into the requested test message, merges the merge payload into
// it and discards its unknown fields if the request asks for those, and
// serializes it in the requested output format. Shared by the conformance_cpp
// binary (stdin/stdout protocol) and, via cpp_in_process_test_runner.h, by
// in-process conformance tests.
//
// The test messages are either the generated C++ classes or DynamicMessages
// (see MessageImplementation), so the same testee covers both message
// implementations of the runtime.
//
// RunTest is const and the state behind it (descriptor pool and message
// factory) is thread-safe, so a single instance may be shared across threads.
class CppConformanceHarness {
 public:
  // Which implementation of the test messages the testee exercises.
  enum class MessageImplementation {
    // The generated C++ classes, from the generated descriptor pool.
    kGenerated,
    // DynamicMessage: the test message files and their dependencies are
    // copied into a descriptor pool of their own, so the descriptors are built
    // at runtime from FileDescriptorProtos like those of a proto loaded
    // dynamically, and the messages come from a DynamicMessageFactory.  Since
    // the descriptors are not the generated pool's, the factory cannot fall
    // back on the generated classes for them, so sub-messages are dynamic too.
    kDynamic,
  };

  // Force-links the reflection data of every test message and well-known type
  // into the binary so that they can be looked up by name, and sets up the
  // descriptor pool and message factory for `implementation`.
  explicit CppConformanceHarness(
      MessageImplementation implementation = MessageImplementation::kGenerated);

  CppConformanceHarness(const CppConformanceHarness&) = delete;
  CppConformanceHarness& operator=(const CppConformanceHarness&) = delete;

  MessageImplementation implementation() const { return implementation_; }

  // Runs a single conformance test.
  //
  // Returns a response with `parse_error` or `serialize_error` set when the
  // payload cannot be handled, and a non-OK status when the request itself is
  // malformed (unknown message type, missing payload, or an unspecified output
  // format).
  absl::StatusOr<::conformance::ConformanceResponse> RunTest(
      const ::conformance::ConformanceRequest& request) const;

 private:
  MessageImplementation implementation_;

  // Only set for kDynamic; the factory is declared after the pool so that it
  // is destroyed before the pool whose descriptors its prototypes refer to.
  std::unique_ptr<DescriptorPool> dynamic_pool_;
  std::unique_ptr<DynamicMessageFactory> dynamic_factory_;

  // The pool the request's message type is looked up in and the factory its
  // prototype comes from: the generated ones, or the dynamic ones above.
  const DescriptorPool* pool_ = nullptr;
  MessageFactory* factory_ = nullptr;
};

}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_CONFORMANCE_CPP_HARNESS_H__
