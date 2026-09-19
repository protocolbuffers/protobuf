// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/conformance/conformance_upb_harness.h"

#include <cstddef>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "conformance/conformance.upb.h"
#include "upb/base/string_view.h"
#include "upb/mem/arena.hpp"

namespace {

using ::testing::HasSubstr;
using ::testing::IsEmpty;

constexpr char kProto3[] = "protobuf_test_messages.proto3.TestAllTypesProto3";

// TestAllTypesProto3 { optional_int32: 99 } on the wire (field 1, varint).
constexpr char kInt32Is99[] = "\x08\x63";
// ...followed by an unknown varint field, number 9999, value 1.
constexpr char kInt32Is99WithUnknownField[] = "\x08\x63\xf8\xf0\x04\x01";

std::string ToString(upb_StringView view) {
  return std::string(view.data, view.size);
}

upb_StringView FromString(const std::string& s) {
  return upb_StringView_FromDataAndSize(s.data(), s.size());
}

// The parameter is `rebuild_minitables`, so every case runs against both the
// generated-minitable and the dynamic-minitable testee.
class ConformanceUpbHarnessTest : public ::testing::TestWithParam<bool> {
 protected:
  ConformanceUpbHarnessTest()
      : harness_(
            upb_ConformanceHarness_New(/*rebuild_minitables=*/GetParam())) {}
  ~ConformanceUpbHarnessTest() override {
    upb_ConformanceHarness_Free(harness_);
  }

  // A request for TestAllTypesProto3 with no payload; tests add one.
  conformance_ConformanceRequest* NewRequest(upb::Arena& arena,
                                             int output_format) {
    conformance_ConformanceRequest* request =
        conformance_ConformanceRequest_new(arena.ptr());
    conformance_ConformanceRequest_set_message_type(
        request, upb_StringView_FromString(kProto3));
    conformance_ConformanceRequest_set_requested_output_format(request,
                                                               output_format);
    return request;
  }

  // The harness's response to `request`, decoded into `arena`; null if the
  // request doesn't serialize or the response bytes aren't a
  // ConformanceResponse (each test asserts on it).
  const conformance_ConformanceResponse* Run(
      const conformance_ConformanceRequest* request, upb::Arena& arena) {
    size_t size;
    char* bytes =
        conformance_ConformanceRequest_serialize(request, arena.ptr(), &size);
    if (bytes == nullptr) return nullptr;
    return RunBytes(std::string(bytes, size), arena);
  }

  // The same for raw request bytes, which need not be a ConformanceRequest.
  const conformance_ConformanceResponse* RunBytes(
      const std::string& serialized_request, upb::Arena& arena) {
    upb::Arena run_arena;
    upb_StringView bytes = upb_ConformanceHarness_Run(
        harness_, FromString(serialized_request), run_arena.ptr());
    return conformance_ConformanceResponse_parse(bytes.data, bytes.size,
                                                 arena.ptr());
  }

  upb_ConformanceHarness* harness_;
};

TEST_P(ConformanceUpbHarnessTest, ProtobufRoundTrip) {
  upb::Arena arena;
  conformance_ConformanceRequest* request =
      NewRequest(arena, conformance_PROTOBUF);
  conformance_ConformanceRequest_set_protobuf_payload(
      request, upb_StringView_FromString(kInt32Is99));

  const conformance_ConformanceResponse* response = Run(request, arena);
  ASSERT_NE(response, nullptr);
  ASSERT_TRUE(conformance_ConformanceResponse_has_protobuf_payload(response));
  EXPECT_EQ(
      ToString(conformance_ConformanceResponse_protobuf_payload(response)),
      kInt32Is99);
}

TEST_P(ConformanceUpbHarnessTest, TextFormatOutputHidesUnknownFieldsByDefault) {
  upb::Arena arena;
  conformance_ConformanceRequest* request =
      NewRequest(arena, conformance_TEXT_FORMAT);
  conformance_ConformanceRequest_set_protobuf_payload(
      request, upb_StringView_FromString(kInt32Is99WithUnknownField));

  const conformance_ConformanceResponse* response = Run(request, arena);
  ASSERT_NE(response, nullptr);
  ASSERT_TRUE(conformance_ConformanceResponse_has_text_payload(response));
  EXPECT_EQ(ToString(conformance_ConformanceResponse_text_payload(response)),
            "optional_int32: 99\n");
}

TEST_P(ConformanceUpbHarnessTest,
       TextFormatOutputPrintsUnknownFieldsOnRequest) {
  upb::Arena arena;
  conformance_ConformanceRequest* request =
      NewRequest(arena, conformance_TEXT_FORMAT);
  conformance_ConformanceRequest_set_protobuf_payload(
      request, upb_StringView_FromString(kInt32Is99WithUnknownField));
  conformance_ConformanceRequest_set_print_unknown_fields(request, true);

  const conformance_ConformanceResponse* response = Run(request, arena);
  ASSERT_NE(response, nullptr);
  ASSERT_TRUE(conformance_ConformanceResponse_has_text_payload(response));
  EXPECT_THAT(ToString(conformance_ConformanceResponse_text_payload(response)),
              HasSubstr("9999: 1"));
}

TEST_P(ConformanceUpbHarnessTest, JsonOutput) {
  upb::Arena arena;
  conformance_ConformanceRequest* request = NewRequest(arena, conformance_JSON);
  conformance_ConformanceRequest_set_protobuf_payload(
      request, upb_StringView_FromString(kInt32Is99));

  const conformance_ConformanceResponse* response = Run(request, arena);
  ASSERT_NE(response, nullptr);
  ASSERT_TRUE(conformance_ConformanceResponse_has_json_payload(response));
  EXPECT_EQ(ToString(conformance_ConformanceResponse_json_payload(response)),
            R"({"optionalInt32":99})");
}

TEST_P(ConformanceUpbHarnessTest, JsonInputRoundTrip) {
  upb::Arena arena;
  conformance_ConformanceRequest* request =
      NewRequest(arena, conformance_PROTOBUF);
  conformance_ConformanceRequest_set_json_payload(
      request, upb_StringView_FromString(R"({"optionalInt32":99})"));

  const conformance_ConformanceResponse* response = Run(request, arena);
  ASSERT_NE(response, nullptr);
  ASSERT_TRUE(conformance_ConformanceResponse_has_protobuf_payload(response));
  EXPECT_EQ(
      ToString(conformance_ConformanceResponse_protobuf_payload(response)),
      kInt32Is99);
}

TEST_P(ConformanceUpbHarnessTest, JsonUnknownFieldIsParseErrorByDefault) {
  upb::Arena arena;
  conformance_ConformanceRequest* request =
      NewRequest(arena, conformance_PROTOBUF);
  conformance_ConformanceRequest_set_json_payload(
      request, upb_StringView_FromString(R"({"noSuchField":1})"));

  const conformance_ConformanceResponse* response = Run(request, arena);
  ASSERT_NE(response, nullptr);
  ASSERT_TRUE(conformance_ConformanceResponse_has_parse_error(response));
  EXPECT_THAT(ToString(conformance_ConformanceResponse_parse_error(response)),
              HasSubstr("noSuchField"));
}

TEST_P(ConformanceUpbHarnessTest,
       JsonUnknownFieldIsIgnoredInIgnoreUnknownTest) {
  upb::Arena arena;
  conformance_ConformanceRequest* request =
      NewRequest(arena, conformance_PROTOBUF);
  conformance_ConformanceRequest_set_json_payload(
      request, upb_StringView_FromString(R"({"noSuchField":1})"));
  conformance_ConformanceRequest_set_test_category(
      request, conformance_JSON_IGNORE_UNKNOWN_PARSING_TEST);

  const conformance_ConformanceResponse* response = Run(request, arena);
  ASSERT_NE(response, nullptr);
  ASSERT_TRUE(conformance_ConformanceResponse_has_protobuf_payload(response));
  EXPECT_THAT(
      ToString(conformance_ConformanceResponse_protobuf_payload(response)),
      IsEmpty());
}

TEST_P(ConformanceUpbHarnessTest, MalformedProtobufIsParseError) {
  upb::Arena arena;
  conformance_ConformanceRequest* request =
      NewRequest(arena, conformance_PROTOBUF);
  // A varint tag with no value.
  conformance_ConformanceRequest_set_protobuf_payload(
      request, upb_StringView_FromString("\x08"));

  const conformance_ConformanceResponse* response = Run(request, arena);
  ASSERT_NE(response, nullptr);
  ASSERT_TRUE(conformance_ConformanceResponse_has_parse_error(response));
  EXPECT_EQ(ToString(conformance_ConformanceResponse_parse_error(response)),
            "Parse error");
}

TEST_P(ConformanceUpbHarnessTest, UnknownMessageTypeIsSkipped) {
  upb::Arena arena;
  conformance_ConformanceRequest* request =
      NewRequest(arena, conformance_PROTOBUF);
  conformance_ConformanceRequest_set_message_type(
      request, upb_StringView_FromString("no.such.Message"));
  conformance_ConformanceRequest_set_protobuf_payload(
      request, upb_StringView_FromString(kInt32Is99));

  const conformance_ConformanceResponse* response = Run(request, arena);
  ASSERT_NE(response, nullptr);
  ASSERT_TRUE(conformance_ConformanceResponse_has_skipped(response));
  EXPECT_EQ(ToString(conformance_ConformanceResponse_skipped(response)),
            "Unknown message type.");
}

TEST_P(ConformanceUpbHarnessTest, TextInputIsSkipped) {
  upb::Arena arena;
  conformance_ConformanceRequest* request =
      NewRequest(arena, conformance_PROTOBUF);
  conformance_ConformanceRequest_set_text_payload(
      request, upb_StringView_FromString("optional_int32: 99"));

  const conformance_ConformanceResponse* response = Run(request, arena);
  ASSERT_NE(response, nullptr);
  ASSERT_TRUE(conformance_ConformanceResponse_has_skipped(response));
  EXPECT_EQ(ToString(conformance_ConformanceResponse_skipped(response)),
            "Unsupported input format.");
}

TEST_P(ConformanceUpbHarnessTest, UnspecifiedOutputFormatIsRuntimeError) {
  upb::Arena arena;
  conformance_ConformanceRequest* request =
      NewRequest(arena, conformance_UNSPECIFIED);
  conformance_ConformanceRequest_set_protobuf_payload(
      request, upb_StringView_FromString(kInt32Is99));

  const conformance_ConformanceResponse* response = Run(request, arena);
  ASSERT_NE(response, nullptr);
  ASSERT_TRUE(conformance_ConformanceResponse_has_runtime_error(response));
  EXPECT_EQ(ToString(conformance_ConformanceResponse_runtime_error(response)),
            "conformance_upb: Unspecified output format.");
}

TEST_P(ConformanceUpbHarnessTest, MissingPayloadIsRuntimeError) {
  upb::Arena arena;
  conformance_ConformanceRequest* request =
      NewRequest(arena, conformance_PROTOBUF);

  const conformance_ConformanceResponse* response = Run(request, arena);
  ASSERT_NE(response, nullptr);
  ASSERT_TRUE(conformance_ConformanceResponse_has_runtime_error(response));
  EXPECT_EQ(ToString(conformance_ConformanceResponse_runtime_error(response)),
            "conformance_upb: Request didn't have payload.");
}

TEST_P(ConformanceUpbHarnessTest, UnparseableRequestIsRuntimeError) {
  upb::Arena arena;
  // A length-delimited field claiming more bytes than there are.
  const conformance_ConformanceResponse* response =
      RunBytes(std::string("\x0a\x7f", 2), arena);
  ASSERT_NE(response, nullptr);
  ASSERT_TRUE(conformance_ConformanceResponse_has_runtime_error(response));
  EXPECT_EQ(ToString(conformance_ConformanceResponse_runtime_error(response)),
            "conformance_upb: parse of ConformanceRequest failed.");
}

INSTANTIATE_TEST_SUITE_P(GeneratedAndDynamicMinitables,
                         ConformanceUpbHarnessTest, ::testing::Bool(),
                         [](const ::testing::TestParamInfo<bool>& info) {
                           return info.param ? "DynamicMinitables"
                                             : "GeneratedMinitables";
                         });

}  // namespace
