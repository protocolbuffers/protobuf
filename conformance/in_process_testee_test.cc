// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// A conformance test binary with an in-process testee: this file defines
// MakeTesteeRunner() (see testee_runner.h) and links :test_environment_main
// without :forked_testee.  It is run without --testee_binary (see BUILD),
// which a forked testee would refuse, and checks that the main hands the
// suites the linked testee.

#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "matchers.h"
#include "test_environment.h"
#include "test_runner.h"
#include "testee_runner.h"
#include "google/protobuf/test_messages_proto3.pb.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::conformance::ConformanceRequest;
using ::conformance::ConformanceResponse;
using ::protobuf_test_messages::proto3::TestAllTypesProto3;
using ::testing::IsEmpty;

// What test_environment_main passed to MakeTesteeRunner(), and how many
// requests the testee it got back has answered.
struct LinkedTesteeState {
  bool created = false;
  std::string testee_binary;
  std::vector<std::string> testee_args;
  int requests_answered = 0;
};

LinkedTesteeState& State() {
  static auto* const state = new LinkedTesteeState;
  return *state;
}

// An in-process testee that "parses" a binary payload by echoing it back, so
// the suites see a conforming response without any real parsing.
class EchoTestee : public ConformanceTestRunner {
 public:
  std::string RunTest(absl::string_view test_name,
                      absl::string_view serialized_request) override {
    ConformanceRequest request;
    ABSL_CHECK(request.ParseFromString(serialized_request)) << test_name;
    ConformanceResponse response;
    if (request.has_protobuf_payload() &&
        request.requested_output_format() == ::conformance::PROTOBUF) {
      response.set_protobuf_payload(request.protobuf_payload());
    } else {
      response.set_skipped("EchoTestee only echoes binary payloads.");
    }
    ++State().requests_answered;
    return response.SerializeAsString();
  }
};

}  // namespace

std::unique_ptr<ConformanceTestRunner> MakeTesteeRunner(
    absl::string_view testee_binary,
    absl::Span<const std::string> testee_args) {
  State().created = true;
  State().testee_binary = std::string(testee_binary);
  State().testee_args.assign(testee_args.begin(), testee_args.end());
  return std::make_unique<EchoTestee>();
}

namespace {

using InProcessTesteeTest = ConformanceTest;

TEST_F(InProcessTesteeTest, MainCreatesTheLinkedTesteeFromEmptyFlags) {
  // Run without --testee_binary or --testee_args, which is how a test with an
  // in-process testee is invoked (see conformance.bzl).
  EXPECT_TRUE(State().created);
  EXPECT_THAT(State().testee_binary, IsEmpty());
  EXPECT_THAT(State().testee_args, IsEmpty());
}

TEST_F(InProcessTesteeTest, SuitesTalkToTheLinkedTestee) {
  const int requests_before = State().requests_answered;
  EXPECT_THAT(
      RequiredTest()
          .ParseBinary(TestAllTypesProto3::descriptor(), VarintField(1, 99))
          .SerializeBinary(),
      Yields(ParsedPayload(EqualsTextProto("optional_int32: 99"))));
  EXPECT_EQ(State().requests_answered, requests_before + 1);
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
