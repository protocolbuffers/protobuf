// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include "conformance/conformance.pb.h"
#include "conformance/test_runner.h"
#include "conformance/testee_runner.h"
#include "google/protobuf/test_messages_proto3.pb.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::conformance::ConformanceRequest;
using ::conformance::ConformanceResponse;
using ::protobuf_test_messages::proto3::TestAllTypesProto3;

// A smoke test: MakeTesteeRunner() is MakeCppInProcessTestRunnerOrDie() over
// the generated classes (cpp_in_process_test_runner_test.cc covers that
// helper), so all there is to observe here is that the runner it returns
// answers requests.
TEST(CppInProcessTesteeTest, AnswersRequests) {
  std::unique_ptr<ConformanceTestRunner> runner = MakeTesteeRunner("", {});
  ASSERT_NE(runner, nullptr);

  TestAllTypesProto3 message;
  message.set_optional_int32(99);
  ConformanceRequest request;
  request.set_message_type(TestAllTypesProto3::descriptor()->full_name());
  request.set_protobuf_payload(message.SerializeAsString());
  request.set_requested_output_format(::conformance::TEXT_FORMAT);

  ConformanceResponse response;
  ASSERT_TRUE(response.ParseFromString(
      runner->RunTest("SomeTest", request.SerializeAsString())));
  EXPECT_EQ(response.text_payload(), "optional_int32: 99\n");
}

// The forked-testee flags are refused with a message naming this library.
TEST(CppInProcessTesteeDeathTest, RejectsATesteeBinary) {
  EXPECT_DEATH(MakeTesteeRunner("/some/testee", {}),
               "--testee_binary=/some/testee was given, but this test links "
               "the C\\+\\+ testee in-process \\(cpp_in_process_testee\\)");
}

TEST(CppInProcessTesteeDeathTest, RejectsTesteeArgs) {
  const std::vector<std::string> args = {"--foo", "bar"};
  EXPECT_DEATH(
      MakeTesteeRunner("", args),
      "positional arguments \\[--foo bar\\] were given, but this test "
      "links the C\\+\\+ testee in-process \\(cpp_in_process_testee\\)");
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
