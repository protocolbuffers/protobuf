// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "cpp_in_process_test_runner.h"

#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/status/status_matchers.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "conformance/conformance.pb.h"
#include "conformance_cpp_harness.h"
#include "test_runner.h"
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

TEST(CppInProcessTestRunnerTest, RunsHarness) {
  std::unique_ptr<ConformanceTestRunner> runner = NewCppInProcessTestRunner();

  ConformanceResponse response = ParseResponse(
      runner->RunTest("Suite.Test", Proto3JsonRequest().SerializeAsString()));

  EXPECT_THAT(
      response, EqualsProto(R"pb(
        json_payload: "{\"optionalInt32\":42,\"optionalString\":\"hello\"}"
      )pb"));
}

TEST(CppInProcessTestRunnerTest, MatchesDirectHarnessResult) {
  std::unique_ptr<ConformanceTestRunner> runner = NewCppInProcessTestRunner();
  ConformanceRequest request = Proto3JsonRequest();

  absl::StatusOr<ConformanceResponse> direct =
      CppConformanceHarness().RunTest(request);
  ConformanceResponse via_runner =
      ParseResponse(runner->RunTest("Suite.Test", request.SerializeAsString()));

  ASSERT_THAT(
      direct, IsOkAndHolds(EqualsProto(R"pb(
        json_payload: "{\"optionalInt32\":42,\"optionalString\":\"hello\"}"
      )pb")));
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

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
