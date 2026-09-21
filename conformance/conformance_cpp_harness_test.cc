// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/conformance_cpp_harness.h"

#include <memory>
#include <ostream>
#include <string>
#include <tuple>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "conformance/binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "conformance/test_protos/test_messages_edition2023.pb.h"
#include "google/protobuf/json/json.h"
#include "google/protobuf/message.h"
#include "google/protobuf/test_messages_proto2.pb.h"
#include "google/protobuf/test_messages_proto3.pb.h"
#include "google/protobuf/test_textproto.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/util/field_comparator.h"
#include "google/protobuf/util/message_differencer.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::absl_testing::IsOk;
using ::absl_testing::IsOkAndHolds;
using ::absl_testing::StatusIs;
using ::conformance::ConformanceRequest;
using ::conformance::ConformanceResponse;
using ::conformance::WireFormat;
using ::protobuf_test_messages::editions::TestAllTypesEdition2023;
using ::protobuf_test_messages::proto2::TestAllTypesProto2;
using ::protobuf_test_messages::proto3::TestAllTypesProto3;
using ::testing::AnyOf;
using ::testing::Combine;
using ::testing::HasSubstr;
using ::testing::Not;
using ::testing::Property;
using ::testing::StartsWith;
using ::testing::TestParamInfo;
using ::testing::TestWithParam;
using ::testing::Values;
using ::testing::ValuesIn;
using MessageImplementation = CppConformanceHarness::MessageImplementation;

constexpr MessageImplementation kMessageImplementations[] = {
    MessageImplementation::kGenerated, MessageImplementation::kDynamic};

std::string MessageImplementationName(MessageImplementation implementation) {
  switch (implementation) {
    case MessageImplementation::kGenerated:
      return "Generated";
    case MessageImplementation::kDynamic:
      return "Dynamic";
  }
  return "Unknown";
}

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

// Every test runs against both message implementations: the harness must
// behave the same whether the test messages are the generated classes or
// DynamicMessages.
class CppConformanceHarnessTest : public TestWithParam<MessageImplementation> {
 protected:
  CppConformanceHarness harness_{GetParam()};
};

INSTANTIATE_TEST_SUITE_P(MessageImplementations, CppConformanceHarnessTest,
                         ValuesIn(kMessageImplementations),
                         [](const TestParamInfo<MessageImplementation>& info) {
                           return MessageImplementationName(info.param);
                         });

TEST_P(CppConformanceHarnessTest, ImplementationIsTheRequestedOne) {
  EXPECT_EQ(harness_.implementation(), GetParam());
}

TEST_P(CppConformanceHarnessTest, ProtobufRoundTrip) {
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_protobuf_payload(
      SerializeProto3(R"pb(optional_int32: 42 optional_string: "hello")pb"));

  absl::StatusOr<ConformanceResponse> response = harness_.RunTest(request);

  ASSERT_THAT(response, IsOk());
  ASSERT_EQ(response->result_case(), ConformanceResponse::kProtobufPayload);
  TestAllTypesProto3 round_tripped;
  ASSERT_TRUE(round_tripped.ParseFromString(response->protobuf_payload()));
  EXPECT_THAT(round_tripped, EqualsProto(R"pb(optional_int32: 42
                                              optional_string: "hello")pb"));
}

TEST_P(CppConformanceHarnessTest, ProtobufParseErrorOnMalformedBytes) {
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  // Field 1, length-delimited, claims 5 bytes but only 3 follow.
  request.set_protobuf_payload(
      Wire(Tag(1, WireType::kLengthPrefixed), Varint(5), "abc").str());

  EXPECT_THAT(harness_.RunTest(request),
              IsOkAndHolds(EqualsProto(
                  R"pb(parse_error: "parse error (no more details available)"
                  )pb")));
}

TEST_P(CppConformanceHarnessTest, JsonOutput) {
  ConformanceRequest request = Proto3Request(::conformance::JSON);
  request.set_protobuf_payload(SerializeProto3(R"pb(optional_int32: 42)pb"));

  // Kept out of the macro arguments: MSVC's traditional preprocessor mis-scans
  // raw string literals containing \" inside macro arguments.
  constexpr absl::string_view kExpected =
      R"pb(json_payload: "{\"optionalInt32\":42}")pb";
  EXPECT_THAT(harness_.RunTest(request), IsOkAndHolds(EqualsProto(kExpected)));
}

TEST_P(CppConformanceHarnessTest, JsonSerializeErrorOnUnresolvableAny) {
  ConformanceRequest request = Proto3Request(::conformance::JSON);
  // The payload parses from the wire format, but JSON printing has to expand
  // the Any and cannot resolve its type URL.
  request.set_protobuf_payload(SerializeProto3(
      R"pb(optional_any { type_url: "type.googleapis.com/no.such.Message" }
      )pb"));

  absl::StatusOr<ConformanceResponse> response = harness_.RunTest(request);

  ASSERT_THAT(response, IsOk());
  ASSERT_EQ(response->result_case(), ConformanceResponse::kSerializeError);
  EXPECT_THAT(response->serialize_error(),
              StartsWith("failed to serialize JSON output: "));
}

TEST_P(CppConformanceHarnessTest, JsonUnknownFieldIsParseErrorByDefault) {
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_json_payload(R"json({"noSuchField": 1})json");
  request.set_test_category(::conformance::JSON_TEST);

  EXPECT_THAT(harness_.RunTest(request),
              IsOkAndHolds(Property(&ConformanceResponse::parse_error,
                                    StartsWith("parse error: "))));
}

TEST_P(CppConformanceHarnessTest,
       JsonUnknownFieldIsIgnoredInIgnoreUnknownTest) {
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_json_payload(
      R"json({"noSuchField": 1, "optionalInt32": 42})json");
  request.set_test_category(::conformance::JSON_IGNORE_UNKNOWN_PARSING_TEST);

  absl::StatusOr<ConformanceResponse> response = harness_.RunTest(request);

  ASSERT_THAT(response, IsOk());
  ASSERT_EQ(response->result_case(), ConformanceResponse::kProtobufPayload);
  TestAllTypesProto3 parsed;
  ASSERT_TRUE(parsed.ParseFromString(response->protobuf_payload()));
  EXPECT_THAT(parsed, EqualsProto(R"pb(optional_int32: 42)pb"));
}

TEST_P(CppConformanceHarnessTest, TextPayloadRoundTrip) {
  ConformanceRequest request = Proto3Request(::conformance::TEXT_FORMAT);
  request.set_text_payload("optional_int32: 42 optional_string: 'hello'");

  constexpr absl::string_view kExpected =
      R"pb(text_payload: "optional_int32: 42\n"
                         "optional_string: \"hello\"\n")pb";
  EXPECT_THAT(harness_.RunTest(request), IsOkAndHolds(EqualsProto(kExpected)));
}

TEST_P(CppConformanceHarnessTest, TextParseErrorOnMalformedInput) {
  ConformanceRequest request = Proto3Request(::conformance::TEXT_FORMAT);
  request.set_text_payload("optional_int32: not_a_number");

  EXPECT_THAT(harness_.RunTest(request),
              IsOkAndHolds(EqualsProto(
                  R"pb(parse_error: "parse error (no more details available)"
                  )pb")));
}

TEST_P(CppConformanceHarnessTest, TextFormatOutputHidesUnknownFieldsByDefault) {
  ConformanceRequest request = Proto3Request(::conformance::TEXT_FORMAT);
  request.set_protobuf_payload(Proto3PayloadWithUnknownField());

  EXPECT_THAT(
      harness_.RunTest(request),
      IsOkAndHolds(EqualsProto(R"pb(text_payload: "optional_int32: 42\n")pb")));
}

TEST_P(CppConformanceHarnessTest,
       TextFormatOutputPrintsUnknownFieldsOnRequest) {
  ConformanceRequest request = Proto3Request(::conformance::TEXT_FORMAT);
  request.set_protobuf_payload(Proto3PayloadWithUnknownField());
  request.set_print_unknown_fields(true);

  EXPECT_THAT(harness_.RunTest(request),
              IsOkAndHolds(EqualsProto(
                  R"pb(text_payload: "optional_int32: 42\n9999: 1\n")pb")));
}

TEST_P(CppConformanceHarnessTest, UnknownFieldsAreKeptByDefault) {
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_protobuf_payload(Proto3PayloadWithUnknownField());

  EXPECT_THAT(harness_.RunTest(request),
              IsOkAndHolds(Property(&ConformanceResponse::protobuf_payload,
                                    Proto3PayloadWithUnknownField())));
}

TEST_P(CppConformanceHarnessTest, DiscardUnknownFieldsDropsThemBeforeOutput) {
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_protobuf_payload(Proto3PayloadWithUnknownField());
  request.set_discard_unknown_fields(true);

  EXPECT_THAT(harness_.RunTest(request),
              IsOkAndHolds(Property(&ConformanceResponse::protobuf_payload,
                                    SerializeProto3("optional_int32: 42"))));
}

TEST_P(CppConformanceHarnessTest, DiscardUnknownFieldsAppliesToTextOutput) {
  ConformanceRequest request = Proto3Request(::conformance::TEXT_FORMAT);
  request.set_protobuf_payload(Proto3PayloadWithUnknownField());
  request.set_discard_unknown_fields(true);
  // Would print "9999: 1" if the unknown field were still there.
  request.set_print_unknown_fields(true);

  EXPECT_THAT(
      harness_.RunTest(request),
      IsOkAndHolds(EqualsProto(R"pb(text_payload: "optional_int32: 42\n")pb")));
}

TEST_P(CppConformanceHarnessTest, DiscardUnknownFieldsIsRecursive) {
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  TestAllTypesProto3 message;
  TestAllTypesProto3::NestedMessage* nested =
      message.mutable_optional_nested_message();
  nested->GetReflection()->MutableUnknownFields(nested)->AddVarint(9999, 1);
  request.set_protobuf_payload(message.SerializeAsString());
  request.set_discard_unknown_fields(true);

  EXPECT_THAT(
      harness_.RunTest(request),
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

TEST_P(CppConformanceHarnessTest, MergeProtobufPayload) {
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_protobuf_payload(SerializeProto3(R"pb(optional_int32: 1
                                                    optional_string: "a"
                                                    repeated_int32: 1)pb"));
  request.set_merge_protobuf_payload(
      SerializeProto3(R"pb(optional_int32: 2 repeated_int32: 2)pb"));

  // Singular fields take the merged value, repeated fields append.
  EXPECT_THAT(ParseProto3Output(harness_.RunTest(request)),
              EqualsProto(R"pb(optional_int32: 2
                               optional_string: "a"
                               repeated_int32: [ 1, 2 ])pb"));
}

TEST_P(CppConformanceHarnessTest, MergeTextPayload) {
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_protobuf_payload(SerializeProto3(R"pb(optional_int32: 1)pb"));
  request.set_merge_text_payload("optional_string: 'a'");

  EXPECT_THAT(ParseProto3Output(harness_.RunTest(request)),
              EqualsProto(R"pb(optional_int32: 1 optional_string: "a")pb"));
}

TEST_P(CppConformanceHarnessTest, MergeJsonPayload) {
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_protobuf_payload(SerializeProto3(R"pb(optional_int32: 1)pb"));
  request.set_merge_json_payload(R"json({"optionalString": "a"})json");

  EXPECT_THAT(ParseProto3Output(harness_.RunTest(request)),
              EqualsProto(R"pb(optional_int32: 1 optional_string: "a")pb"));
}

TEST_P(CppConformanceHarnessTest, MergeJsonPayloadFollowsTheRequestCategory) {
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_protobuf_payload(SerializeProto3(R"pb(optional_int32: 1)pb"));
  request.set_merge_json_payload(
      R"json({"noSuchField": 1, "optionalString": "a"})json");

  EXPECT_THAT(
      harness_.RunTest(request),
      IsOkAndHolds(Property(&ConformanceResponse::parse_error,
                            StartsWith("merge_payload parse error: "))));

  request.set_test_category(::conformance::JSON_IGNORE_UNKNOWN_PARSING_TEST);
  EXPECT_THAT(ParseProto3Output(harness_.RunTest(request)),
              EqualsProto(R"pb(optional_int32: 1 optional_string: "a")pb"));
}

TEST_P(CppConformanceHarnessTest, MergeProtobufPayloadParseError) {
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_protobuf_payload(SerializeProto3(R"pb(optional_int32: 1)pb"));
  // Field 1, length-delimited, claims 5 bytes but only 3 follow.
  request.set_merge_protobuf_payload(
      Wire(Tag(1, WireType::kLengthPrefixed), Varint(5), "abc").str());

  EXPECT_THAT(
      harness_.RunTest(request),
      IsOkAndHolds(EqualsProto(
          R"pb(parse_error: "merge_payload parse error (no more details available)"
          )pb")));
}

TEST_P(CppConformanceHarnessTest, MergeTextPayloadParseError) {
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_protobuf_payload(SerializeProto3(R"pb(optional_int32: 1)pb"));
  request.set_merge_text_payload("optional_int32: not_a_number");

  EXPECT_THAT(
      harness_.RunTest(request),
      IsOkAndHolds(EqualsProto(
          R"pb(parse_error: "merge_payload parse error (no more details available)"
          )pb")));
}

TEST_P(CppConformanceHarnessTest, MergeJsonPayloadParseError) {
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_protobuf_payload(SerializeProto3(R"pb(optional_int32: 1)pb"));
  request.set_merge_json_payload("{\"optionalInt32\": ");

  EXPECT_THAT(
      harness_.RunTest(request),
      IsOkAndHolds(Property(&ConformanceResponse::parse_error,
                            StartsWith("merge_payload parse error: "))));
}

TEST_P(CppConformanceHarnessTest, MergePayloadIsNotAppliedWhenTheInputFails) {
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  // A truncated length-delimited field: the input itself doesn't parse.
  request.set_protobuf_payload(std::string("\x0a\x05") + "abc");
  request.set_merge_protobuf_payload(
      SerializeProto3(R"pb(optional_int32: 2)pb"));

  // The input's parse error is reported; the merge payload isn't looked at.
  EXPECT_THAT(harness_.RunTest(request),
              IsOkAndHolds(Property(&ConformanceResponse::parse_error,
                                    StartsWith("parse error"))));
}

TEST_P(CppConformanceHarnessTest, MergeProtobufPayloadIntoJsonInput) {
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_json_payload(
      R"json({"optionalInt32": 1, "repeatedInt32": [1]})json");
  request.set_merge_protobuf_payload(
      SerializeProto3(R"pb(optional_string: "a" repeated_int32: 2)pb"));

  // The in-memory message doesn't depend on the input's format.
  EXPECT_THAT(ParseProto3Output(harness_.RunTest(request)),
              EqualsProto(R"pb(optional_int32: 1
                               optional_string: "a"
                               repeated_int32: [ 1, 2 ])pb"));
}

TEST_P(CppConformanceHarnessTest, MergeHappensBeforeDiscardingUnknownFields) {
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_protobuf_payload(SerializeProto3(R"pb(optional_int32: 1)pb"));
  // Brings optional_int32: 42 and the unknown field 9999.
  request.set_merge_protobuf_payload(Proto3PayloadWithUnknownField());
  request.set_discard_unknown_fields(true);

  EXPECT_THAT(harness_.RunTest(request),
              IsOkAndHolds(Property(&ConformanceResponse::protobuf_payload,
                                    SerializeProto3("optional_int32: 42"))));
}

TEST_P(CppConformanceHarnessTest, UnknownMessageTypeIsNotFound) {
  ConformanceRequest request = Proto3Request(::conformance::PROTOBUF);
  request.set_message_type("no.such.Message");
  request.set_protobuf_payload("");

  EXPECT_THAT(
      harness_.RunTest(request),
      StatusIs(absl::StatusCode::kNotFound, HasSubstr("no.such.Message")));
}

TEST_P(CppConformanceHarnessTest, MissingPayloadIsInvalidArgument) {
  EXPECT_THAT(
      harness_.RunTest(Proto3Request(::conformance::PROTOBUF)),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("payload")));
}

TEST_P(CppConformanceHarnessTest, UnspecifiedOutputFormatIsInvalidArgument) {
  ConformanceRequest request = Proto3Request(::conformance::UNSPECIFIED);
  request.set_protobuf_payload("");

  EXPECT_THAT(
      harness_.RunTest(request),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("output format")));
}

TEST_P(CppConformanceHarnessTest, UnknownOutputFormatIsInvalidArgument) {
  ConformanceRequest request =
      Proto3Request(static_cast<::conformance::WireFormat>(99));
  request.set_protobuf_payload("");

  EXPECT_THAT(
      harness_.RunTest(request),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("output format")));
}

// Message types the conformance suites may name in a request, other than
// TestAllTypesProto3 (which this file references directly). The harness
// constructor force-links the reflection data for each of them, and the
// dynamic harness copies their files into its own pool. This test
// deliberately does not reference the generated C++ types, so it is intended
// to fail if the harness stops pulling one of them in; it is only a weak
// guarantee, though, since the linker may keep the descriptors alive for other
// reasons (e.g. alwayslink or dynamic linking).
class LinkedMessageTypeTest
    : public TestWithParam<
          std::tuple<MessageImplementation, absl::string_view>> {};

TEST_P(LinkedMessageTypeTest, EmptyPayloadRoundTrips) {
  CppConformanceHarness linked_harness(std::get<0>(GetParam()));
  ConformanceRequest request;
  request.set_message_type(std::get<1>(GetParam()));
  request.set_requested_output_format(::conformance::PROTOBUF);
  request.set_protobuf_payload("");

  EXPECT_THAT(linked_harness.RunTest(request),
              IsOkAndHolds(EqualsProto(R"pb(protobuf_payload: "")pb")));
}

INSTANTIATE_TEST_SUITE_P(
    CppConformanceHarnessTest, LinkedMessageTypeTest,
    Combine(ValuesIn(kMessageImplementations),
            Values("protobuf_test_messages.proto2.TestAllTypesProto2",
                   "protobuf_test_messages.editions.TestAllTypesEdition2023",
                   "protobuf_test_messages.edition_unstable."
                   "TestAllTypesEditionUnstable",
                   "protobuf_test_messages.editions.proto2.TestAllTypesProto2",
                   "protobuf_test_messages.editions.proto3.TestAllTypesProto3",
                   "google.protobuf.Any", "google.protobuf.Timestamp")),
    [](const TestParamInfo<
        std::tuple<MessageImplementation, absl::string_view>>& info) {
      return absl::StrCat(
          MessageImplementationName(std::get<0>(info.param)), "_",
          absl::StrReplaceAll(std::get<1>(info.param), {{".", "_"}}));
    });

// The two implementations must agree with each other: a payload exercising
// every kind of field, given in each input format and asked back in each
// output format, yields the same message through DynamicMessage as through
// the generated class.  The reflection-based parsers (JSON, text format) and
// serializers into and out of DynamicMessage are all covered this way.

// A test message type, as the generated class's prototype (which decides how
// the payload is built and how the outputs are read back), and a payload for
// it, in text format, exercising every kind of field the type has.
struct ImplementationsTestMessage {
  const Message* prototype;
  absl::string_view payload;
};

constexpr absl::string_view kProto2Payload = R"pb(
  optional_int32: 1
  optional_int64: -1
  optional_uint64: 18446744073709551615
  optional_float: -inf
  optional_double: nan
  optional_bool: true
  optional_string: "s"
  optional_bytes: "\x00\xff"
  optional_nested_enum: BAR
  optional_foreign_enum: FOREIGN_BAZ
  optional_nested_message {
    a: 2
    corecursive { optional_int32: 3 }
  }
  optional_foreign_message { c: 12 }
  repeated_int32: [ 4, 5 ]
  repeated_double: [ 1.5, inf ]
  repeated_string: [ "a", "b" ]
  repeated_bytes: [ "\x01", "" ]
  repeated_nested_enum: [ FOO, BAZ ]
  repeated_nested_message { a: 13 }
  repeated_nested_message {}
  packed_int32: [ 14, 15 ]
  unpacked_int32: [ 16, 17 ]
  map_string_string { key: "k" value: "v" }
  map_int32_int32 { key: 6 value: 7 }
  oneof_uint32: 8
  [protobuf_test_messages.proto2.extension_int32]: 9
  [protobuf_test_messages.proto2.extension_string]: "e"
  Data { group_int32: 10 group_uint32: 11 }
  message_set_correct {
    [protobuf_test_messages.proto2.TestAllTypesProto2
         .MessageSetCorrectExtension1.message_set_extension] { str: "x" }
  }
)pb";

constexpr absl::string_view kProto3Payload = R"pb(
  optional_int32: 1
  optional_int64: -1
  optional_uint64: 18446744073709551615
  optional_float: -inf
  optional_double: nan
  optional_bool: true
  optional_string: "s"
  optional_bytes: "\x00\xff"
  optional_nested_enum: BAR
  optional_foreign_enum: FOREIGN_BAZ
  optional_nested_message {
    a: 2
    corecursive { optional_int32: 3 }
  }
  optional_foreign_message { c: 12 }
  repeated_int32: [ 4, 5 ]
  repeated_double: [ 1.5, inf ]
  repeated_string: [ "a", "b" ]
  repeated_bytes: [ "\x01", "" ]
  repeated_nested_enum: [ FOO, BAZ ]
  repeated_nested_message { a: 13 }
  repeated_nested_message {}
  packed_int32: [ 14, 15 ]
  unpacked_int32: [ 16, 17 ]
  map_string_string { key: "k" value: "v" }
  map_int32_int32 { key: 6 value: 7 }
  oneof_uint32: 8
  optional_any {
    [type.googleapis.com/protobuf_test_messages.proto3.TestAllTypesProto3] {
      optional_int32: 9
    }
  }
  optional_timestamp { seconds: 10 nanos: 11 }
  optional_struct {
    fields {
      key: "f"
      value { list_value { values { number_value: 12 } } }
    }
  }
  optional_duration { seconds: 13 nanos: 14 }
  optional_field_mask { paths: [ "foo_bar", "baz" ] }
  optional_value { string_value: "v" }
  optional_empty {}
  optional_bool_wrapper { value: false }
  optional_int64_wrapper { value: -15 }
  repeated_string_wrapper { value: "w" }
  repeated_string_wrapper {}
  repeated_value { null_value: NULL_VALUE }
  repeated_value { bool_value: true }
)pb";

constexpr absl::string_view kEdition2023Payload = R"pb(
  optional_int32: 1
  optional_int64: -1
  optional_uint64: 18446744073709551615
  optional_float: -inf
  optional_double: nan
  optional_bool: true
  optional_string: "s"
  optional_bytes: "\x00\xff"
  optional_nested_enum: BAR
  optional_foreign_enum: FOREIGN_BAZ
  optional_nested_message {
    a: 2
    corecursive { optional_int32: 3 }
  }
  optional_foreign_message { c: 12 }
  repeated_int32: [ 4, 5 ]
  repeated_double: [ 1.5, inf ]
  repeated_string: [ "a", "b" ]
  repeated_bytes: [ "\x01", "" ]
  repeated_nested_enum: [ FOO, BAZ ]
  repeated_nested_message { a: 13 }
  repeated_nested_message {}
  packed_int32: [ 14, 15 ]
  unpacked_int32: [ 16, 17 ]
  map_string_string { key: "k" value: "v" }
  map_int32_int32 { key: 6 value: 7 }
  oneof_uint32: 8
  [protobuf_test_messages.editions.extension_int32]: 9
  delimited_field { group_int32: 10 group_uint32: 11 }
)pb";

const ImplementationsTestMessage kImplementationsTestMessages[] = {
    {&TestAllTypesProto2::default_instance(), kProto2Payload},
    {&TestAllTypesProto3::default_instance(), kProto3Payload},
    {&TestAllTypesEdition2023::default_instance(), kEdition2023Payload},
};

constexpr WireFormat kWireFormats[] = {
    ::conformance::PROTOBUF, ::conformance::JSON, ::conformance::TEXT_FORMAT};

// Sets the payload of `request` to `message` in `format`, serialized by the
// generated class.
void SetPayload(const Message& message, WireFormat format,
                ConformanceRequest& request) {
  switch (format) {
    case ::conformance::PROTOBUF:
      request.set_protobuf_payload(message.SerializeAsString());
      break;
    case ::conformance::JSON:
      ABSL_CHECK_OK(
          json::MessageToJsonString(message, request.mutable_json_payload()));
      break;
    case ::conformance::TEXT_FORMAT:
      ABSL_CHECK(
          TextFormat::PrintToString(message, request.mutable_text_payload()));
      break;
    default:
      ABSL_LOG(FATAL) << "unexpected input format " << format;
  }
}

// Parses the payload of `response` (in `format`, the one the request asked
// for) into a new message of `prototype`'s type.
std::unique_ptr<Message> ParseOutput(const Message& prototype,
                                     WireFormat format,
                                     const ConformanceResponse& response) {
  std::unique_ptr<Message> message(prototype.New());
  switch (format) {
    case ::conformance::PROTOBUF:
      ABSL_CHECK_EQ(response.result_case(),
                    ConformanceResponse::kProtobufPayload)
          << response.ShortDebugString();
      ABSL_CHECK(message->ParseFromString(response.protobuf_payload()));
      break;
    case ::conformance::JSON:
      ABSL_CHECK_EQ(response.result_case(), ConformanceResponse::kJsonPayload)
          << response.ShortDebugString();
      ABSL_CHECK_OK(
          json::JsonStringToMessage(response.json_payload(), message.get()));
      break;
    case ::conformance::TEXT_FORMAT:
      ABSL_CHECK_EQ(response.result_case(), ConformanceResponse::kTextPayload)
          << response.ShortDebugString();
      ABSL_CHECK(
          TextFormat::ParseFromString(response.text_payload(), message.get()));
      break;
    default:
      ABSL_LOG(FATAL) << "unexpected output format " << format;
  }
  return message;
}

// Matches a message that MessageDifferencer finds equal to `expected`,
// explaining the differences otherwise.  (test_textproto.h's EqualsProto()
// only takes the expected message in text format.)
class EqualsMessageMatcher {
 public:
  using is_gtest_matcher = void;

  explicit EqualsMessageMatcher(const Message& expected)
      : expected_(expected) {}

  bool MatchAndExplain(const Message& actual,
                       testing::MatchResultListener* listener) const {
    std::string differences;
    util::MessageDifferencer differencer;
    util::DefaultFieldComparator field_comparator;
    field_comparator.set_treat_nan_as_equal(true);
    differencer.set_field_comparator(&field_comparator);
    differencer.ReportDifferencesToString(&differences);
    if (differencer.Compare(expected_, actual)) return true;
    *listener << "which differs from the expected message:\n" << differences;
    return false;
  }
  void DescribeTo(std::ostream* os) const {
    *os << "equals " << expected_.ShortDebugString();
  }
  void DescribeNegationTo(std::ostream* os) const {
    *os << "doesn't equal " << expected_.ShortDebugString();
  }

 private:
  const Message& expected_;
};

EqualsMessageMatcher EqualsMessage(const Message& expected) {
  return EqualsMessageMatcher(expected);
}

class CppConformanceHarnessImplementationsTest
    : public TestWithParam<
          std::tuple<ImplementationsTestMessage, WireFormat, WireFormat>> {
 protected:
  const Message& prototype() const {
    return *std::get<0>(GetParam()).prototype;
  }
  absl::string_view payload() const { return std::get<0>(GetParam()).payload; }
  WireFormat input_format() const { return std::get<1>(GetParam()); }
  WireFormat output_format() const { return std::get<2>(GetParam()); }
};

TEST_P(CppConformanceHarnessImplementationsTest, DynamicMatchesGenerated) {
  CppConformanceHarness generated(MessageImplementation::kGenerated);
  CppConformanceHarness dynamic(MessageImplementation::kDynamic);
  std::unique_ptr<Message> message(prototype().New());
  ASSERT_TRUE(TextFormat::ParseFromString(payload(), message.get()));
  ConformanceRequest request;
  request.set_message_type(prototype().GetDescriptor()->full_name());
  request.set_requested_output_format(output_format());
  SetPayload(*message, input_format(), request);

  absl::StatusOr<ConformanceResponse> expected = generated.RunTest(request);
  ASSERT_THAT(expected, IsOk());
  ASSERT_THAT(expected->result_case(),
              Not(AnyOf(ConformanceResponse::kParseError,
                        ConformanceResponse::kSerializeError,
                        ConformanceResponse::kRuntimeError)))
      << "the generated harness failed: " << expected->ShortDebugString();
  absl::StatusOr<ConformanceResponse> actual = dynamic.RunTest(request);
  ASSERT_THAT(actual, IsOk());
  ASSERT_EQ(actual->result_case(), expected->result_case())
      << "the dynamic harness answered: " << actual->ShortDebugString();

  // The two serializers may legitimately order fields differently (in
  // sub-messages in particular), so compare what the outputs mean rather
  // than the outputs; the generated harness's own output must mean what went
  // in, too.
  std::unique_ptr<Message> expected_message =
      ParseOutput(prototype(), output_format(), *expected);
  std::unique_ptr<Message> actual_message =
      ParseOutput(prototype(), output_format(), *actual);
  EXPECT_THAT(*expected_message, EqualsMessage(*message));
  EXPECT_THAT(*actual_message, EqualsMessage(*expected_message));
}

INSTANTIATE_TEST_SUITE_P(
    MessageTypesAndFormats, CppConformanceHarnessImplementationsTest,
    Combine(ValuesIn(kImplementationsTestMessages), ValuesIn(kWireFormats),
            ValuesIn(kWireFormats)),
    [](const TestParamInfo<
        std::tuple<ImplementationsTestMessage, WireFormat, WireFormat>>& info) {
      return absl::StrCat(
          std::get<0>(info.param).prototype->GetDescriptor()->name(), "_",
          ::conformance::WireFormat_Name(std::get<1>(info.param)), "Input_",
          ::conformance::WireFormat_Name(std::get<2>(info.param)), "Output");
    });

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
