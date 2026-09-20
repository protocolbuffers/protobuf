// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/conformance_cpp_harness.h"

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "conformance/binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "google/protobuf/test_messages_proto3.pb.h"
#include "google/protobuf/test_textproto.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::absl_testing::IsOk;
using ::absl_testing::IsOkAndHolds;
using ::absl_testing::StatusIs;
using ::conformance::ConformanceRequest;
using ::conformance::ConformanceResponse;
using ::protobuf_test_messages::proto3::TestAllTypesProto3;
using ::testing::HasSubstr;
using ::testing::Property;
using ::testing::StartsWith;
using ::testing::TestParamInfo;
using ::testing::TestWithParam;
using ::testing::Values;

constexpr absl::string_view kProto3MessageType =
    "protobuf_test_messages.proto3.TestAllTypesProto3";

// Returns a request for TestAllTypesProto3 with the given output format and no
// payload. Tests add whichever payload they exercise, or leave it unset to
// exercise the missing-payload path.
ConformanceRequest Proto3Request(::conformance::WireFormat output_format) {
  ConformanceRequest request;
  request.set_message_type(kProto3MessageType);
  request.set_requested_output_format(output_format);
  return request;
}

// Serializes the TestAllTypesProto3 described by `textproto` to the wire
// format.
std::string SerializeProto3(absl::string_view textproto) {
  TestAllTypesProto3 message = ParseTextOrDie(textproto);
  return message.SerializeAsString();
}

// Serializes `optional_int32: 42` with an additional unknown varint field
// (number 9999, value 1) so that unknown-field handling can be observed.
std::string Proto3PayloadWithUnknownField() {
  TestAllTypesProto3 message;
  message.set_optional_int32(42);
  message.GetReflection()->MutableUnknownFields(&message)->AddVarint(9999, 1);
  return message.SerializeAsString();
}

TEST(CppConformanceHarnessTest, ProtobufRoundTrip) {
  CppConformanceHarness harness;
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_protobuf_payload(
      SerializeProto3(R"pb(optional_int32: 42 optional_string: "hello")pb"));

  absl::StatusOr<ConformanceResponse> response = harness.RunTest(request);

  ASSERT_THAT(response, IsOk());
  ASSERT_EQ(response->result_case(), ConformanceResponse::kProtobufPayload);
  TestAllTypesProto3 round_tripped;
  ASSERT_TRUE(round_tripped.ParseFromString(response->protobuf_payload()));
  EXPECT_THAT(round_tripped, EqualsProto(R"pb(optional_int32: 42
                                              optional_string: "hello")pb"));
}

TEST(CppConformanceHarnessTest, ProtobufParseErrorOnMalformedBytes) {
  CppConformanceHarness harness;
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  // Field 1, length-delimited, claims 5 bytes but only 3 follow.
  request.set_protobuf_payload(
      Wire(Tag(1, WireType::kLengthPrefixed), Varint(5), "abc").str());

  EXPECT_THAT(harness.RunTest(request),
              IsOkAndHolds(EqualsProto(
                  R"pb(parse_error: "parse error (no more details available)"
                  )pb")));
}

TEST(CppConformanceHarnessTest, JsonOutput) {
  CppConformanceHarness harness;
  ConformanceRequest request = Proto3Request(::conformance::JSON);
  request.set_protobuf_payload(SerializeProto3(R"pb(optional_int32: 42)pb"));

  // Kept out of the macro arguments: MSVC's traditional preprocessor mis-scans
  // raw string literals containing \" inside macro arguments.
  constexpr absl::string_view kExpected =
      R"pb(json_payload: "{\"optionalInt32\":42}")pb";
  EXPECT_THAT(harness.RunTest(request), IsOkAndHolds(EqualsProto(kExpected)));
}

TEST(CppConformanceHarnessTest, JsonSerializeErrorOnUnresolvableAny) {
  CppConformanceHarness harness;
  ConformanceRequest request = Proto3Request(::conformance::JSON);
  // The payload parses from the wire format, but JSON printing has to expand
  // the Any and cannot resolve its type URL.
  request.set_protobuf_payload(SerializeProto3(
      R"pb(optional_any { type_url: "type.googleapis.com/no.such.Message" }
      )pb"));

  absl::StatusOr<ConformanceResponse> response = harness.RunTest(request);

  ASSERT_THAT(response, IsOk());
  ASSERT_EQ(response->result_case(), ConformanceResponse::kSerializeError);
  EXPECT_THAT(response->serialize_error(),
              StartsWith("failed to serialize JSON output: "));
}

TEST(CppConformanceHarnessTest, JsonUnknownFieldIsParseErrorByDefault) {
  CppConformanceHarness harness;
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_json_payload(R"json({"noSuchField": 1})json");
  request.set_test_category(::conformance::JSON_TEST);

  EXPECT_THAT(harness.RunTest(request),
              IsOkAndHolds(Property(&ConformanceResponse::parse_error,
                                    StartsWith("parse error: "))));
}

TEST(CppConformanceHarnessTest, JsonUnknownFieldIsIgnoredInIgnoreUnknownTest) {
  CppConformanceHarness harness;
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_json_payload(
      R"json({"noSuchField": 1, "optionalInt32": 42})json");
  request.set_test_category(::conformance::JSON_IGNORE_UNKNOWN_PARSING_TEST);

  absl::StatusOr<ConformanceResponse> response = harness.RunTest(request);

  ASSERT_THAT(response, IsOk());
  ASSERT_EQ(response->result_case(), ConformanceResponse::kProtobufPayload);
  TestAllTypesProto3 parsed;
  ASSERT_TRUE(parsed.ParseFromString(response->protobuf_payload()));
  EXPECT_THAT(parsed, EqualsProto(R"pb(optional_int32: 42)pb"));
}

TEST(CppConformanceHarnessTest, TextPayloadRoundTrip) {
  CppConformanceHarness harness;
  ConformanceRequest request = Proto3Request(::conformance::TEXT_FORMAT);
  request.set_text_payload("optional_int32: 42 optional_string: 'hello'");

  constexpr absl::string_view kExpected =
      R"pb(text_payload: "optional_int32: 42\n"
                         "optional_string: \"hello\"\n")pb";
  EXPECT_THAT(harness.RunTest(request), IsOkAndHolds(EqualsProto(kExpected)));
}

TEST(CppConformanceHarnessTest, TextParseErrorOnMalformedInput) {
  CppConformanceHarness harness;
  ConformanceRequest request = Proto3Request(::conformance::TEXT_FORMAT);
  request.set_text_payload("optional_int32: not_a_number");

  EXPECT_THAT(harness.RunTest(request),
              IsOkAndHolds(EqualsProto(
                  R"pb(parse_error: "parse error (no more details available)"
                  )pb")));
}

TEST(CppConformanceHarnessTest, TextFormatOutputHidesUnknownFieldsByDefault) {
  CppConformanceHarness harness;
  ConformanceRequest request = Proto3Request(::conformance::TEXT_FORMAT);
  request.set_protobuf_payload(Proto3PayloadWithUnknownField());

  EXPECT_THAT(
      harness.RunTest(request),
      IsOkAndHolds(EqualsProto(R"pb(text_payload: "optional_int32: 42\n")pb")));
}

TEST(CppConformanceHarnessTest, TextFormatOutputPrintsUnknownFieldsOnRequest) {
  CppConformanceHarness harness;
  ConformanceRequest request = Proto3Request(::conformance::TEXT_FORMAT);
  request.set_protobuf_payload(Proto3PayloadWithUnknownField());
  request.set_print_unknown_fields(true);

  EXPECT_THAT(harness.RunTest(request),
              IsOkAndHolds(EqualsProto(
                  R"pb(text_payload: "optional_int32: 42\n9999: 1\n")pb")));
}

TEST(CppConformanceHarnessTest, UnknownFieldsAreKeptByDefault) {
  CppConformanceHarness harness;
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_protobuf_payload(Proto3PayloadWithUnknownField());

  EXPECT_THAT(harness.RunTest(request),
              IsOkAndHolds(Property(&ConformanceResponse::protobuf_payload,
                                    Proto3PayloadWithUnknownField())));
}

TEST(CppConformanceHarnessTest, DiscardUnknownFieldsDropsThemBeforeOutput) {
  CppConformanceHarness harness;
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_protobuf_payload(Proto3PayloadWithUnknownField());
  request.set_discard_unknown_fields(true);

  EXPECT_THAT(harness.RunTest(request),
              IsOkAndHolds(Property(&ConformanceResponse::protobuf_payload,
                                    SerializeProto3("optional_int32: 42"))));
}

TEST(CppConformanceHarnessTest, DiscardUnknownFieldsAppliesToTextOutput) {
  CppConformanceHarness harness;
  ConformanceRequest request = Proto3Request(::conformance::TEXT_FORMAT);
  request.set_protobuf_payload(Proto3PayloadWithUnknownField());
  request.set_discard_unknown_fields(true);
  // Would print "9999: 1" if the unknown field were still there.
  request.set_print_unknown_fields(true);

  EXPECT_THAT(
      harness.RunTest(request),
      IsOkAndHolds(EqualsProto(R"pb(text_payload: "optional_int32: 42\n")pb")));
}

TEST(CppConformanceHarnessTest, DiscardUnknownFieldsIsRecursive) {
  CppConformanceHarness harness;
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  TestAllTypesProto3 message;
  TestAllTypesProto3::NestedMessage* nested =
      message.mutable_optional_nested_message();
  nested->GetReflection()->MutableUnknownFields(nested)->AddVarint(9999, 1);
  request.set_protobuf_payload(message.SerializeAsString());
  request.set_discard_unknown_fields(true);

  EXPECT_THAT(
      harness.RunTest(request),
      IsOkAndHolds(Property(&ConformanceResponse::protobuf_payload,
                            SerializeProto3("optional_nested_message {}"))));
}

// Parses the PROTOBUF payload of `response` as a TestAllTypesProto3.
TestAllTypesProto3 ParseProto3Output(
    const absl::StatusOr<ConformanceResponse>& response) {
  ABSL_CHECK_OK(response.status());
  ABSL_CHECK_EQ(response->result_case(), ConformanceResponse::kProtobufPayload)
      << response->DebugString();
  TestAllTypesProto3 parsed;
  ABSL_CHECK(parsed.ParseFromString(response->protobuf_payload()));
  return parsed;
}

TEST(CppConformanceHarnessTest, MergeProtobufPayload) {
  CppConformanceHarness harness;
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_protobuf_payload(SerializeProto3(R"pb(optional_int32: 1
                                                    optional_string: "a"
                                                    repeated_int32: 1)pb"));
  request.set_merge_protobuf_payload(
      SerializeProto3(R"pb(optional_int32: 2 repeated_int32: 2)pb"));

  // Singular fields take the merged value, repeated fields append.
  EXPECT_THAT(ParseProto3Output(harness.RunTest(request)),
              EqualsProto(R"pb(optional_int32: 2
                               optional_string: "a"
                               repeated_int32: [ 1, 2 ])pb"));
}

TEST(CppConformanceHarnessTest, MergeTextPayload) {
  CppConformanceHarness harness;
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_protobuf_payload(SerializeProto3(R"pb(optional_int32: 1)pb"));
  request.set_merge_text_payload("optional_string: 'a'");

  EXPECT_THAT(ParseProto3Output(harness.RunTest(request)),
              EqualsProto(R"pb(optional_int32: 1 optional_string: "a")pb"));
}

TEST(CppConformanceHarnessTest, MergeJsonPayload) {
  CppConformanceHarness harness;
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_protobuf_payload(SerializeProto3(R"pb(optional_int32: 1)pb"));
  request.set_merge_json_payload(R"json({"optionalString": "a"})json");

  EXPECT_THAT(ParseProto3Output(harness.RunTest(request)),
              EqualsProto(R"pb(optional_int32: 1 optional_string: "a")pb"));
}

TEST(CppConformanceHarnessTest, MergeJsonPayloadFollowsTheRequestCategory) {
  CppConformanceHarness harness;
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_protobuf_payload(SerializeProto3(R"pb(optional_int32: 1)pb"));
  request.set_merge_json_payload(
      R"json({"noSuchField": 1, "optionalString": "a"})json");

  EXPECT_THAT(
      harness.RunTest(request),
      IsOkAndHolds(Property(&ConformanceResponse::parse_error,
                            StartsWith("merge_payload parse error: "))));

  request.set_test_category(::conformance::JSON_IGNORE_UNKNOWN_PARSING_TEST);
  EXPECT_THAT(ParseProto3Output(harness.RunTest(request)),
              EqualsProto(R"pb(optional_int32: 1 optional_string: "a")pb"));
}

TEST(CppConformanceHarnessTest, MergeProtobufPayloadParseError) {
  CppConformanceHarness harness;
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_protobuf_payload(SerializeProto3(R"pb(optional_int32: 1)pb"));
  // Field 1, length-delimited, claims 5 bytes but only 3 follow.
  request.set_merge_protobuf_payload(
      Wire(Tag(1, WireType::kLengthPrefixed), Varint(5), "abc").str());

  EXPECT_THAT(
      harness.RunTest(request),
      IsOkAndHolds(EqualsProto(
          R"pb(parse_error: "merge_payload parse error (no more details available)"
          )pb")));
}

TEST(CppConformanceHarnessTest, MergeTextPayloadParseError) {
  CppConformanceHarness harness;
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_protobuf_payload(SerializeProto3(R"pb(optional_int32: 1)pb"));
  request.set_merge_text_payload("optional_int32: not_a_number");

  EXPECT_THAT(
      harness.RunTest(request),
      IsOkAndHolds(EqualsProto(
          R"pb(parse_error: "merge_payload parse error (no more details available)"
          )pb")));
}

TEST(CppConformanceHarnessTest, MergeJsonPayloadParseError) {
  CppConformanceHarness harness;
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_protobuf_payload(SerializeProto3(R"pb(optional_int32: 1)pb"));
  request.set_merge_json_payload("{\"optionalInt32\": ");

  EXPECT_THAT(
      harness.RunTest(request),
      IsOkAndHolds(Property(&ConformanceResponse::parse_error,
                            StartsWith("merge_payload parse error: "))));
}

TEST(CppConformanceHarnessTest, MergePayloadIsNotAppliedWhenTheInputFails) {
  CppConformanceHarness harness;
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  // A truncated length-delimited field: the input itself doesn't parse.
  request.set_protobuf_payload(std::string("\x0a\x05") + "abc");
  request.set_merge_protobuf_payload(
      SerializeProto3(R"pb(optional_int32: 2)pb"));

  // The input's parse error is reported; the merge payload isn't looked at.
  EXPECT_THAT(harness.RunTest(request),
              IsOkAndHolds(Property(&ConformanceResponse::parse_error,
                                    StartsWith("parse error"))));
}

TEST(CppConformanceHarnessTest, MergeProtobufPayloadIntoJsonInput) {
  CppConformanceHarness harness;
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_json_payload(
      R"json({"optionalInt32": 1, "repeatedInt32": [1]})json");
  request.set_merge_protobuf_payload(
      SerializeProto3(R"pb(optional_string: "a" repeated_int32: 2)pb"));

  // The in-memory message doesn't depend on the input's format.
  EXPECT_THAT(ParseProto3Output(harness.RunTest(request)),
              EqualsProto(R"pb(optional_int32: 1
                               optional_string: "a"
                               repeated_int32: [ 1, 2 ])pb"));
}

TEST(CppConformanceHarnessTest, MergeHappensBeforeDiscardingUnknownFields) {
  CppConformanceHarness harness;
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_protobuf_payload(SerializeProto3(R"pb(optional_int32: 1)pb"));
  // Brings optional_int32: 42 and the unknown field 9999.
  request.set_merge_protobuf_payload(Proto3PayloadWithUnknownField());
  request.set_discard_unknown_fields(true);

  EXPECT_THAT(harness.RunTest(request),
              IsOkAndHolds(Property(&ConformanceResponse::protobuf_payload,
                                    SerializeProto3("optional_int32: 42"))));
}

TEST(CppConformanceHarnessTest, UnknownMessageTypeIsNotFound) {
  CppConformanceHarness harness;
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_message_type("no.such.Message");
  request.set_protobuf_payload("");

  EXPECT_THAT(harness.RunTest(request), StatusIs(absl::StatusCode::kNotFound,
                                                 HasSubstr("no.such.Message")));
}

TEST(CppConformanceHarnessTest, MissingPayloadIsInvalidArgument) {
  CppConformanceHarness harness;

  EXPECT_THAT(
      harness.RunTest(Proto3Request(::conformance::PROTOBUF)),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("payload")));
}

TEST(CppConformanceHarnessTest, UnspecifiedOutputFormatIsInvalidArgument) {
  CppConformanceHarness harness;
  ConformanceRequest request = Proto3Request(::conformance::UNSPECIFIED);
  request.set_protobuf_payload("");

  EXPECT_THAT(
      harness.RunTest(request),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("output format")));
}

TEST(CppConformanceHarnessTest, UnknownOutputFormatIsInvalidArgument) {
  CppConformanceHarness harness;
  ConformanceRequest request =
      Proto3Request(static_cast<::conformance::WireFormat>(99));
  request.set_protobuf_payload("");

  EXPECT_THAT(
      harness.RunTest(request),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("output format")));
}

// Message types the conformance suites may name in a request, other than
// TestAllTypesProto3 (which this file references directly). The harness
// constructor force-links the reflection data for each of them. This test
// deliberately does not reference the generated C++ types, so it is intended
// to fail if the harness stops pulling one of them in; it is only a weak
// guarantee, though, since the linker may keep the descriptors alive for other
// reasons (e.g. alwayslink or dynamic linking).
class LinkedMessageTypeTest : public TestWithParam<absl::string_view> {};

TEST_P(LinkedMessageTypeTest, EmptyPayloadRoundTrips) {
  CppConformanceHarness harness;
  ConformanceRequest request;
  request.set_message_type(GetParam());
  request.set_requested_output_format(::conformance::PROTOBUF);
  request.set_protobuf_payload("");

  EXPECT_THAT(harness.RunTest(request),
              IsOkAndHolds(EqualsProto(R"pb(protobuf_payload: "")pb")));
}

INSTANTIATE_TEST_SUITE_P(
    CppConformanceHarnessTest, LinkedMessageTypeTest,
    Values("protobuf_test_messages.proto2.TestAllTypesProto2",
           "protobuf_test_messages.editions.TestAllTypesEdition2023",
           "protobuf_test_messages.edition_unstable."
           "TestAllTypesEditionUnstable",
           "protobuf_test_messages.editions.proto2.TestAllTypesProto2",
           "protobuf_test_messages.editions.proto3.TestAllTypesProto3",
           "google.protobuf.Any", "google.protobuf.Timestamp"),
    [](const TestParamInfo<absl::string_view>& info) {
      return absl::StrReplaceAll(info.param, {{".", "_"}});
    });

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
