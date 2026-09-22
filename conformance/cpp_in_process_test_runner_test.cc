// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/cpp_in_process_test_runner.h"

#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/status/status_matchers.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "conformance/conformance.pb.h"
#include "conformance/conformance_cpp_harness.h"
#include "conformance/test_runner.h"
#include "google/protobuf/test_messages_proto3.pb.h"
#include "google/protobuf/test_textproto.h"
#include "google/protobuf/util/message_differencer.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::absl_testing::IsOkAndHolds;
using ::conformance::ConformanceRequest;
using ::conformance::ConformanceResponse;
using ::protobuf_test_messages::proto3::TestAllTypesProto3;
using ::testing::HasSubstr;
using MessageImplementation = CppConformanceHarness::MessageImplementation;

// Returns a request that exercises the harness end to end: a binary
// TestAllTypesProto3 payload to be re-serialized as JSON.
ConformanceRequest Proto3JsonRequest() {
  TestAllTypesProto3 message =
      ParseTextOrDie(R"pb(optional_int32: 42 optional_string: "hello")pb");

  ConformanceRequest request;
  request.set_message_type("protobuf_test_messages.proto3.TestAllTypesProto3");
  request.set_requested_output_format(::conformance::JSON);
  request.set_protobuf_payload(message.SerializeAsString());
  return request;
}

// Parses the serialized ConformanceResponse returned by a runner, aborting the
// test if the runner produced bytes that are not a valid response.
ConformanceResponse ParseResponse(absl::string_view serialized) {
  ConformanceResponse response;
  ABSL_CHECK(response.ParseFromString(serialized))
      << "runner returned an unparseable ConformanceResponse";
  return response;
}

// The response every Proto3JsonRequest() yields.  The harness stamps the
// protocol version it implements on every response, including answers to
// version 1 requests such as this one.  Kept out of the macro arguments:
// MSVC's traditional preprocessor mis-scans raw string literals containing \"
// inside macro arguments.
constexpr absl::string_view kProto3JsonResponse =
    R"pb(json_payload: "{\"optionalInt32\":42,\"optionalString\":\"hello\"}"
         protocol_version: 2)pb";

TEST(CppInProcessTestRunnerTest, RunsHarness) {
  std::unique_ptr<ConformanceTestRunner> runner = NewCppInProcessTestRunner();

  ConformanceResponse response = ParseResponse(
      runner->RunTest("Suite.Test", Proto3JsonRequest().SerializeAsString()));

  EXPECT_THAT(response, EqualsProto(kProto3JsonResponse));
}

TEST(CppInProcessTestRunnerTest, RunsDynamicHarness) {
  std::unique_ptr<ConformanceTestRunner> runner = NewCppInProcessTestRunner(
      CppConformanceHarness::MessageImplementation::kDynamic);

  ConformanceResponse response = ParseResponse(
      runner->RunTest("Suite.Test", Proto3JsonRequest().SerializeAsString()));

  EXPECT_THAT(response, EqualsProto(kProto3JsonResponse));
}

TEST(CppInProcessTestRunnerTest, MatchesDirectHarnessResult) {
  std::unique_ptr<ConformanceTestRunner> runner = NewCppInProcessTestRunner();
  ConformanceRequest request = Proto3JsonRequest();

  absl::StatusOr<ConformanceResponse> direct =
      CppConformanceHarness().RunTest(request);
  ConformanceResponse via_runner =
      ParseResponse(runner->RunTest("Suite.Test", request.SerializeAsString()));

  ASSERT_THAT(direct, IsOkAndHolds(EqualsProto(kProto3JsonResponse)));
  EXPECT_TRUE(util::MessageDifferencer::Equals(via_runner, *direct))
      << via_runner.ShortDebugString();
}

TEST(CppInProcessTestRunnerTest, ReportsHarnessErrorAsRuntimeError) {
  std::unique_ptr<ConformanceTestRunner> runner = NewCppInProcessTestRunner();
  ConformanceRequest request = Proto3JsonRequest();
  request.set_message_type("no.such.Message");

  ConformanceResponse response =
      ParseResponse(runner->RunTest("Suite.Test", request.SerializeAsString()));

  ASSERT_EQ(response.result_case(), ConformanceResponse::kRuntimeError);
  EXPECT_THAT(response.runtime_error(), HasSubstr("no.such.Message"));
  EXPECT_THAT(response.runtime_error(), HasSubstr("Suite.Test"));
}

TEST(CppInProcessTestRunnerTest, RunnerIsReusableAcrossRequests) {
  std::unique_ptr<ConformanceTestRunner> runner = NewCppInProcessTestRunner();
  std::string request = Proto3JsonRequest().SerializeAsString();

  ConformanceResponse first =
      ParseResponse(runner->RunTest("Suite.A", request));
  ConformanceResponse second =
      ParseResponse(runner->RunTest("Suite.B", request));

  EXPECT_TRUE(util::MessageDifferencer::Equals(first, second))
      << second.ShortDebugString();
  EXPECT_EQ(second.result_case(), ConformanceResponse::kJsonPayload);
}

TEST(MakeCppInProcessTestRunnerOrDieTest, ReturnsARunnerWithoutForkedFlags) {
  for (MessageImplementation implementation :
       {MessageImplementation::kGenerated, MessageImplementation::kDynamic}) {
    std::unique_ptr<ConformanceTestRunner> runner =
        MakeCppInProcessTestRunnerOrDie(implementation, "some_testee_library",
                                        /*testee_binary=*/"",
                                        /*testee_args=*/{});
    ASSERT_NE(runner, nullptr);
    EXPECT_THAT(ParseResponse(runner->RunTest(
                    "Suite.Test", Proto3JsonRequest().SerializeAsString())),
                EqualsProto(kProto3JsonResponse));
  }
}

TEST(MakeCppInProcessTestRunnerOrDieDeathTest, RejectsATesteeBinary) {
  EXPECT_DEATH(
      MakeCppInProcessTestRunnerOrDie(MessageImplementation::kGenerated,
                                      "some_testee_library", "/some/testee",
                                      /*testee_args=*/{}),
      "--testee_binary=/some/testee was given, but this test links the C\\+\\+ "
      "testee in-process \\(some_testee_library\\)");
}

TEST(MakeCppInProcessTestRunnerOrDieDeathTest, RejectsTesteeArgs) {
  const std::vector<std::string> args = {"--foo", "bar"};
  EXPECT_DEATH(
      MakeCppInProcessTestRunnerOrDie(MessageImplementation::kDynamic,
                                      "some_testee_library",
                                      /*testee_binary=*/"", args),
      "positional arguments \\[--foo bar\\] were given, but this test links "
      "the C\\+\\+ testee in-process \\(some_testee_library\\)");
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
