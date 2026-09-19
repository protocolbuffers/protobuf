// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Tests of upb_in_process_testee.cc.  Built once per variant library, with
// the same UPB_CONFORMANCE_REBUILD_MINITABLES define (see BUILD), so the
// expected variant name in the messages follows the library under test.

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "conformance/conformance.upb.h"
#include "conformance/test_runner.h"
#include "conformance/testee_runner.h"
#include "upb/base/string_view.h"
#include "upb/mem/arena.hpp"

#ifndef UPB_CONFORMANCE_REBUILD_MINITABLES
#error "Define UPB_CONFORMANCE_REBUILD_MINITABLES to 0 or 1 (see BUILD)."
#endif

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::HasSubstr;

#if UPB_CONFORMANCE_REBUILD_MINITABLES
constexpr char kTesteeName[] = "upb_dynamic_minitable_in_process_testee";
#else
constexpr char kTesteeName[] = "upb_in_process_testee";
#endif

constexpr char kProto3[] = "protobuf_test_messages.proto3.TestAllTypesProto3";

// TestAllTypesProto3 { optional_int32: 99 } on the wire (field 1, varint).
constexpr char kInt32Is99[] = "\x08\x63";

std::string ToString(upb_StringView view) {
  return std::string(view.data, view.size);
}

// A serialized ConformanceRequest for TestAllTypesProto3 with `payload` as
// the protobuf payload, asking for `output_format`.
std::string SerializedRequest(const std::string& payload, int output_format) {
  upb::Arena arena;
  conformance_ConformanceRequest* request =
      conformance_ConformanceRequest_new(arena.ptr());
  conformance_ConformanceRequest_set_message_type(
      request, upb_StringView_FromString(kProto3));
  conformance_ConformanceRequest_set_protobuf_payload(
      request, upb_StringView_FromDataAndSize(payload.data(), payload.size()));
  conformance_ConformanceRequest_set_requested_output_format(request,
                                                             output_format);
  size_t size;
  char* bytes =
      conformance_ConformanceRequest_serialize(request, arena.ptr(), &size);
  return std::string(bytes, size);
}

TEST(UpbInProcessTesteeTest, AnswersRequestsWithTheUpbTestee) {
  std::unique_ptr<ConformanceTestRunner> runner = MakeTesteeRunner("", {});
  ASSERT_NE(runner, nullptr);

  std::string serialized_response = runner->RunTest(
      "SomeTest", SerializedRequest(kInt32Is99, conformance_PROTOBUF));

  upb::Arena arena;
  const conformance_ConformanceResponse* response =
      conformance_ConformanceResponse_parse(
          serialized_response.data(), serialized_response.size(), arena.ptr());
  ASSERT_NE(response, nullptr);
  ASSERT_TRUE(conformance_ConformanceResponse_has_protobuf_payload(response));
  EXPECT_EQ(
      ToString(conformance_ConformanceResponse_protobuf_payload(response)),
      kInt32Is99);
}

// Each request gets its own arena; a runner answers any number of them.
TEST(UpbInProcessTesteeTest, AnswersRepeatedRequestsIndependently) {
  std::unique_ptr<ConformanceTestRunner> runner = MakeTesteeRunner("", {});
  ASSERT_NE(runner, nullptr);

  std::string first = runner->RunTest(
      "FirstTest", SerializedRequest(kInt32Is99, conformance_TEXT_FORMAT));
  std::string second = runner->RunTest(
      "SecondTest", SerializedRequest(kInt32Is99, conformance_TEXT_FORMAT));

  upb::Arena arena;
  const conformance_ConformanceResponse* first_response =
      conformance_ConformanceResponse_parse(first.data(), first.size(),
                                            arena.ptr());
  const conformance_ConformanceResponse* second_response =
      conformance_ConformanceResponse_parse(second.data(), second.size(),
                                            arena.ptr());
  ASSERT_NE(first_response, nullptr);
  ASSERT_NE(second_response, nullptr);
  EXPECT_EQ(
      ToString(conformance_ConformanceResponse_text_payload(first_response)),
      "optional_int32: 99\n");
  EXPECT_EQ(
      ToString(conformance_ConformanceResponse_text_payload(second_response)),
      "optional_int32: 99\n");
}

// A request the harness can't act on is answered, not fatal to the test
// process (which would take every test in the binary down with it).
TEST(UpbInProcessTesteeTest, AnswersAnUnparseableRequestWithARuntimeError) {
  std::unique_ptr<ConformanceTestRunner> runner = MakeTesteeRunner("", {});
  ASSERT_NE(runner, nullptr);

  std::string serialized_response =
      runner->RunTest("SomeTest", "\xff\xff\xff\xff");

  upb::Arena arena;
  const conformance_ConformanceResponse* response =
      conformance_ConformanceResponse_parse(
          serialized_response.data(), serialized_response.size(), arena.ptr());
  ASSERT_NE(response, nullptr);
  ASSERT_TRUE(conformance_ConformanceResponse_has_runtime_error(response));
  EXPECT_THAT(ToString(conformance_ConformanceResponse_runtime_error(response)),
              HasSubstr("parse of ConformanceRequest failed"));
}

TEST(UpbInProcessTesteeDeathTest, RejectsATesteeBinary) {
  EXPECT_DEATH(MakeTesteeRunner("/some/testee", {}),
               std::string("--testee_binary=/some/testee was given, but this "
                           "test links the upb testee in-process \\(") +
                   kTesteeName + "\\)");
}

TEST(UpbInProcessTesteeDeathTest, RejectsTesteeArgs) {
  const std::vector<std::string> args = {"--foo", "bar"};
  EXPECT_DEATH(
      MakeTesteeRunner("", args),
      std::string("positional arguments \\[--foo bar\\] were given, "
                  "but this test links the upb testee in-process \\(") +
          kTesteeName + "\\)");
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
