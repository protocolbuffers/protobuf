// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/in_process_test_runner.h"

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "conformance/binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "google/protobuf/test_textproto.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::conformance::ConformanceRequest;
using ::conformance::ConformanceResponse;
using ::testing::AllOf;
using ::testing::HasSubstr;

// Parses the serialized ConformanceResponse returned by a runner, aborting the
// test if the runner produced bytes that are not a valid response.
ConformanceResponse ParseResponse(absl::string_view serialized) {
  ConformanceResponse response;
  ABSL_CHECK(response.ParseFromString(serialized))
      << "runner returned an unparseable ConformanceResponse";
  return response;
}

TEST(InProcessTestRunnerTest, HandlerReceivesParsedRequest) {
  ConformanceRequest received;
  InProcessTestRunner runner([&received](const ConformanceRequest& request)
                                 -> absl::StatusOr<ConformanceResponse> {
    received = request;
    return ConformanceResponse();
  });
  ConformanceRequest request = ParseTextOrDie(R"pb(
    message_type: "some.Message"
    requested_output_format: TEXT_FORMAT
    protobuf_payload: "payload"
  )pb");

  runner.RunTest("Suite.Test", request.SerializeAsString());

  EXPECT_THAT(received, EqualsProto(R"pb(
                message_type: "some.Message"
                requested_output_format: TEXT_FORMAT
                protobuf_payload: "payload"
              )pb"));
}

TEST(InProcessTestRunnerTest, HandlerResponseIsSerialized) {
  InProcessTestRunner runner(
      [](const ConformanceRequest&) -> absl::StatusOr<ConformanceResponse> {
        ConformanceResponse response;
        response.set_text_payload("optional_int32: 1\n");
        return response;
      });

  std::string serialized =
      runner.RunTest("Suite.Test", ConformanceRequest().SerializeAsString());

  EXPECT_THAT(ParseResponse(serialized),
              EqualsProto(R"pb(text_payload: "optional_int32: 1\n")pb"));
}

TEST(InProcessTestRunnerTest, HandlerErrorBecomesRuntimeError) {
  InProcessTestRunner runner(
      [](const ConformanceRequest&) -> absl::StatusOr<ConformanceResponse> {
        return absl::NotFoundError("no such message type");
      });

  ConformanceResponse response = ParseResponse(
      runner.RunTest("Suite.Test", ConformanceRequest().SerializeAsString()));

  ASSERT_EQ(response.result_case(), ConformanceResponse::kRuntimeError);
  EXPECT_THAT(
      response.runtime_error(),
      AllOf(HasSubstr("Suite.Test"), HasSubstr("no such message type")));
}

TEST(InProcessTestRunnerTest, UnparseableRequestBecomesRuntimeError) {
  bool handler_called = false;
  InProcessTestRunner runner([&handler_called](const ConformanceRequest&)
                                 -> absl::StatusOr<ConformanceResponse> {
    handler_called = true;
    return ConformanceResponse();
  });

  // Field 1, length-delimited, claims 5 bytes but only 3 follow.
  ConformanceResponse response = ParseResponse(runner.RunTest(
      "Suite.Test",
      Wire(Tag(1, WireType::kLengthPrefixed), Varint(5), "abc").str()));

  EXPECT_FALSE(handler_called);
  ASSERT_EQ(response.result_case(), ConformanceResponse::kRuntimeError);
  EXPECT_THAT(response.runtime_error(),
              AllOf(HasSubstr("Suite.Test"),
                    HasSubstr("failed to parse ConformanceRequest")));
}

TEST(InProcessTestRunnerTest, HandlerMayBeStateful) {
  InProcessTestRunner runner([calls = 0](const ConformanceRequest&) mutable
                                 -> absl::StatusOr<ConformanceResponse> {
    ConformanceResponse response;
    response.set_text_payload(std::to_string(++calls));
    return response;
  });
  std::string request = ConformanceRequest().SerializeAsString();

  runner.RunTest("Suite.First", request);
  ConformanceResponse response =
      ParseResponse(runner.RunTest("Suite.Second", request));

  EXPECT_THAT(response, EqualsProto(R"pb(text_payload: "2")pb"));
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
