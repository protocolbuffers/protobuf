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
#include "absl/strings/string_view.h"
#include "conformance/conformance.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/message.h"

namespace google {
namespace protobuf {
namespace conformance {

// Implements the C++ conformance testee: answers ConformanceRequests of both
// protocol versions (see conformance.proto).  A version 1 request is the flat
// one: parse the payload into the requested test message, merge the merge
// payload into it and discard its unknown fields if the request asks for
// those, and serialize it in the requested output format.  A version 2
// request is a list of actions (parse, new, merge, discard unknown fields,
// serialize) over message handles, run in order.  Both go through the same
// parsers, serializers and options, so a test yields the same bytes whichever
// version carries it; the version 1 path is kept, although the runner speaks
// version 2 to this testee, to validate the runner's lowering of tests to
// version 1 against the same binary.  Shared by the conformance_cpp binary
// (stdin/stdout protocol) and, via cpp_in_process_test_runner.h, by
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
  // The newest conformance protocol version this testee implements; see
  // ConformanceRequest.protocol_version in conformance.proto.  RunTest() sets
  // it as ConformanceResponse.protocol_version on every response, which is
  // how the runner's discovery handshake learns it, and answers a request of a
  // newer version with a runtime_error.  This is the testee's version, not the
  // runner's (internal::kLatestProtocolVersion in testee.h): the two are
  // separate roles and move independently, e.g. a runner that speaks a newer
  // version than this testee falls back to this one for it.
  static constexpr int kProtocolVersion = 2;

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

  // Runs a single conformance test, of whichever protocol version the
  // request's `protocol_version` says: 2 or later is the action list (the
  // version 1 fields are ignored), anything else, unset included, is the flat
  // version 1 request (the actions are ignored).  Every response, errors
  // included, carries `protocol_version` = kProtocolVersion.
  //
  // For a version 1 request, returns a response with `parse_error` or
  // `serialize_error` set when the payload cannot be handled, and a non-OK
  // status when the request itself is malformed (unknown message type,
  // missing payload, or an unspecified output format).
  //
  // For a version 2 request, runs the actions in order and stops at the first
  // one that fails, answering with its error and its index in `failed_action`:
  // `parse_error` for a payload that doesn't parse, `serialize_error` for a
  // message that doesn't serialize, and `runtime_error` for a malformed action
  // (a handle created twice or never created, an unknown message type, a
  // merge between messages of different types or of a message into itself, a
  // member of the action oneof this testee doesn't know, a parse without a
  // payload or a serialize without a format).  If every action succeeds, the
  // answer is `results` with one SerializeResult per SerializeAction, in
  // order.  A request of a version newer than kProtocolVersion is answered
  // with the runtime_error "unsupported protocol version N (testee supports
  // M)" and no `failed_action`.  The status is always OK: unlike a version 1
  // request, a malformed version 2 request is reported in the response.
  absl::StatusOr<::conformance::ConformanceResponse> RunTest(
      const ::conformance::ConformanceRequest& request) const;

 private:
  // The version 1 path of RunTest(): the flat request.
  absl::StatusOr<::conformance::ConformanceResponse> RunLegacyTest(
      const ::conformance::ConformanceRequest& request) const;

  // The version 2 path of RunTest(): the action list.
  ::conformance::ConformanceResponse RunActions(
      const ::conformance::ConformanceRequest& request) const;

  // The prototype of the message type with the given full name, or null if
  // the pool this harness was built with doesn't have it.  This is the
  // message type resolution of both protocol versions
  // (ConformanceRequest.message_type, ParseAction.type, NewAction.type).
  const Message* FindPrototype(absl::string_view full_name) const;

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
