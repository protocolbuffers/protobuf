// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/conformance/conformance_upb_harness.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "conformance/binary_wireformat.h"
#include "conformance/conformance.upb.h"
#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb/mem/arena.hpp"

namespace {

using ::google::protobuf::conformance::LengthPrefixedField;
using ::google::protobuf::conformance::Tag;
using ::google::protobuf::conformance::Varint;
using ::google::protobuf::conformance::VarintField;
using ::google::protobuf::conformance::Wire;
using ::google::protobuf::conformance::WireType;
using ::testing::HasSubstr;
using ::testing::IsEmpty;

constexpr char kProto3[] = "protobuf_test_messages.proto3.TestAllTypesProto3";

std::string ToString(upb_StringView view) {
  return std::string(view.data, view.size);
}

upb_StringView FromString(const std::string& s) {
  return upb_StringView_FromDataAndSize(s.data(), s.size());
}

// `wire` copied into `arena`.  The upb setters keep a view of what they
// are given, so payloads built inline in a setter call have to live
// somewhere that outlives the request.
upb_StringView InArena(upb::Arena& arena, const Wire& wire) {
  char* copy = static_cast<char*>(upb_Arena_Malloc(arena.ptr(), wire.size()));
  memcpy(copy, wire.data().data(), wire.size());
  return upb_StringView_FromDataAndSize(copy, wire.size());
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
    // The request only has to live for the call: the response is parsed into
    // the caller's arena from the harness's output bytes with options = 0,
    // which copies strings rather than aliasing them.
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
      request, InArena(arena, VarintField(1, 99)));

  const conformance_ConformanceResponse* response = Run(request, arena);
  ASSERT_NE(response, nullptr);
  ASSERT_TRUE(conformance_ConformanceResponse_has_protobuf_payload(response));
  EXPECT_EQ(
      ToString(conformance_ConformanceResponse_protobuf_payload(response)),
      VarintField(1, 99).data());
}

TEST_P(ConformanceUpbHarnessTest, TextFormatOutputHidesUnknownFieldsByDefault) {
  upb::Arena arena;
  conformance_ConformanceRequest* request =
      NewRequest(arena, conformance_TEXT_FORMAT);
  conformance_ConformanceRequest_set_protobuf_payload(
      request, InArena(arena, Wire(VarintField(1, 99), VarintField(9999, 1))));

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
      request, InArena(arena, Wire(VarintField(1, 99), VarintField(9999, 1))));
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
      request, InArena(arena, VarintField(1, 99)));

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
      VarintField(1, 99).data());
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
      request, InArena(arena, Tag(1, WireType::kVarint)));

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
      request, InArena(arena, VarintField(1, 99)));

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

TEST_P(ConformanceUpbHarnessTest, DiscardUnknownFieldsIsSkipped) {
  upb::Arena arena;
  conformance_ConformanceRequest* request =
      NewRequest(arena, conformance_PROTOBUF);
  conformance_ConformanceRequest_set_protobuf_payload(
      request, InArena(arena, VarintField(1, 99)));
  conformance_ConformanceRequest_set_discard_unknown_fields(request, true);

  const conformance_ConformanceResponse* response = Run(request, arena);
  ASSERT_NE(response, nullptr);
  ASSERT_TRUE(conformance_ConformanceResponse_has_skipped(response));
  EXPECT_EQ(ToString(conformance_ConformanceResponse_skipped(response)),
            "discard_unknown_fields is not supported");
}

TEST_P(ConformanceUpbHarnessTest, MergePayloadIsSkipped) {
  upb::Arena arena;
  conformance_ConformanceRequest* request =
      NewRequest(arena, conformance_PROTOBUF);
  conformance_ConformanceRequest_set_protobuf_payload(
      request, InArena(arena, VarintField(1, 99)));
  conformance_ConformanceRequest_set_merge_protobuf_payload(
      request, InArena(arena, VarintField(2, 5)));

  const conformance_ConformanceResponse* response = Run(request, arena);
  ASSERT_NE(response, nullptr);
  ASSERT_TRUE(conformance_ConformanceResponse_has_skipped(response));
  EXPECT_EQ(ToString(conformance_ConformanceResponse_skipped(response)),
            "merge_payload is not supported");
}

TEST_P(ConformanceUpbHarnessTest, UnspecifiedOutputFormatIsRuntimeError) {
  upb::Arena arena;
  conformance_ConformanceRequest* request =
      NewRequest(arena, conformance_UNSPECIFIED);
  conformance_ConformanceRequest_set_protobuf_payload(
      request, InArena(arena, VarintField(1, 99)));

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
  const conformance_ConformanceResponse* response = RunBytes(
      Wire(Tag(1, WireType::kLengthPrefixed), Varint(127)).str(), arena);
  ASSERT_NE(response, nullptr);
  ASSERT_TRUE(conformance_ConformanceResponse_has_runtime_error(response));
  EXPECT_EQ(ToString(conformance_ConformanceResponse_runtime_error(response)),
            "conformance_upb: parse of ConformanceRequest failed.");
}

TEST_P(ConformanceUpbHarnessTest, ProtobufOutputSerializeErrorIsReported) {
  upb::Arena arena;
  conformance_ConformanceRequest* request =
      NewRequest(arena, conformance_PROTOBUF);
  // recursive_message (27) nested 100 deep: upb's decoder accepts exactly its
  // depth limit (100) of nested messages, but its encoder rejects the 100th,
  // so this parses and then fails to serialize.
  Wire nested;
  for (int i = 0; i < 100; ++i) {
    nested = LengthPrefixedField(27, nested);
  }
  conformance_ConformanceRequest_set_protobuf_payload(request,
                                                      InArena(arena, nested));

  const conformance_ConformanceResponse* response = Run(request, arena);
  ASSERT_NE(response, nullptr);
  ASSERT_TRUE(conformance_ConformanceResponse_has_serialize_error(response));
  EXPECT_EQ(ToString(conformance_ConformanceResponse_serialize_error(response)),
            "Error serializing.");
}

TEST_P(ConformanceUpbHarnessTest, JsonOutputSerializeErrorIsReported) {
  upb::Arena arena;
  conformance_ConformanceRequest* request = NewRequest(arena, conformance_JSON);
  // optional_timestamp (302) one second past 9999-12-31T23:59:59Z, which
  // parses from the wire but has no JSON representation.
  conformance_ConformanceRequest_set_protobuf_payload(
      request,
      InArena(arena, LengthPrefixedField(
                         302, VarintField(1, int64_t{253402300799} + 1))));

  const conformance_ConformanceResponse* response = Run(request, arena);
  ASSERT_NE(response, nullptr);
  ASSERT_TRUE(conformance_ConformanceResponse_has_serialize_error(response));
  EXPECT_THAT(
      ToString(conformance_ConformanceResponse_serialize_error(response)),
      HasSubstr("maximum acceptable value is 9999-12-31T23:59:59Z"));
}

TEST_P(ConformanceUpbHarnessTest, UnknownOutputFormatIsSkipped) {
  upb::Arena arena;
  // An output format this harness doesn't know (a future WireFormat value).
  conformance_ConformanceRequest* request = NewRequest(arena, 99);
  conformance_ConformanceRequest_set_protobuf_payload(
      request, InArena(arena, VarintField(1, 99)));

  const conformance_ConformanceResponse* response = Run(request, arena);
  ASSERT_NE(response, nullptr);
  ASSERT_TRUE(conformance_ConformanceResponse_has_skipped(response));
  EXPECT_EQ(ToString(conformance_ConformanceResponse_skipped(response)),
            "Unsupported output format.");
}

INSTANTIATE_TEST_SUITE_P(GeneratedAndDynamicMinitables,
                         ConformanceUpbHarnessTest, ::testing::Bool(),
                         [](const ::testing::TestParamInfo<bool>& info) {
                           return info.param ? "DynamicMinitables"
                                             : "GeneratedMinitables";
                         });

}  // namespace
