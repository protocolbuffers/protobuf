// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "matchers.h"

#include <cmath>
#include <fstream>
#include <iterator>
#include <ostream>
#include <string>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest-spi.h>
#include <gtest/gtest.h>
#include "absl/base/log_severity.h"
#include "absl/log/absl_check.h"
#include "absl/log/scoped_mock_log.h"
#include "absl/status/status_matchers.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "json/value.h"
#include "binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "global_test_environment.h"
#include "test_manager.h"
#include "test_runner.h"
#include "testee.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/test_messages_proto2.pb.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/unknown_field_set.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {
namespace {

// Backing state for the mocked-out global environment below.  In the real
// conformance binary this would be populated from CLI flags and the failure
// list on disk, but we don't want to drag that in for a unit test.
TestManager* global_test_manager = nullptr;

}  // namespace

TestManager& GetGlobalTestManager() {
  ABSL_CHECK(global_test_manager != nullptr);
  return *global_test_manager;
}

}  // namespace internal

namespace {

using ::absl_testing::IsOk;
using ::conformance::ConformanceRequest;
using ::conformance::ConformanceResponse;
using ::conformance::WireFormat;
using ::google::protobuf::conformance::internal::TestManager;
using ::google::protobuf::conformance::internal::TestResult;
using ::google::protobuf::conformance::internal::TestStrictness;
using ::protobuf_test_messages::proto2::TestAllTypesProto2;
using ::protobuf_test_messages::proto2::UnknownToTestAllTypes;
using ::testing::_;
using ::testing::AllOf;
using ::testing::AnyNumber;
using ::testing::AnyOf;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::FieldsAre;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::Pair;
using ::testing::StrEq;
using ::testing::TestParamInfo;
using ::testing::Truly;
using ::testing::Value;
using ::testing::Values;

static_assert(internal::kIsBytesLike<Wire>);
static_assert(internal::kIsBytesLike<std::string>);
static_assert(internal::kIsBytesLike<absl::string_view>);
static_assert(!internal::kIsBytesLike<const char*>);
static_assert(!internal::kIsBytesLike<char[4]>);

// Requests with the given kind of input and requested output format.  The
// matchers never look at the input payload itself, only at its kind.
ConformanceRequest BinaryInputRequest(WireFormat output_format) {
  ConformanceRequest request;
  request.set_protobuf_payload("");
  request.set_requested_output_format(output_format);
  return request;
}

ConformanceRequest TextInputRequest(WireFormat output_format) {
  ConformanceRequest request;
  request.set_text_payload("");
  request.set_requested_output_format(output_format);
  return request;
}

ConformanceRequest JsonInputRequest(WireFormat output_format) {
  ConformanceRequest request;
  request.set_json_payload("");
  request.set_requested_output_format(output_format);
  return request;
}

// Creates a result for the leaf matcher tests.  These only inspect the result,
// so it is marked checked right away.
//
// The leaf matchers never touch the TestManager; the tests make sure of it by
// not installing one at all while they run.  Only YieldsTest below installs
// one.
TestResult CreateResult(absl::string_view test_name, ConformanceRequest request,
                        ConformanceResponse response,
                        TestStrictness strictness = TestStrictness::kRequired) {
  TestResult result = TestResult::ForTesting(
      test_name, strictness, TestAllTypesProto2::descriptor(),
      std::move(request), std::move(response));
  result.MarkChecked();
  return result;
}

// Most tests only care about the requested output format; they use binary
// input.
TestResult CreateResult(absl::string_view test_name, WireFormat output_format,
                        ConformanceResponse response,
                        TestStrictness strictness = TestStrictness::kRequired) {
  return CreateResult(test_name, BinaryInputRequest(output_format),
                      std::move(response), strictness);
}

ConformanceResponse ProtobufPayload(Wire payload) {
  ConformanceResponse response;
  response.set_protobuf_payload(std::move(payload).str());
  return response;
}

// Note: we deliberately don't use test_textproto.h here, whose EqualsProto()
// matcher ADL would find alongside our matchers via google::protobuf::Message.
ConformanceResponse ResponseFromText(absl::string_view textproto) {
  ConformanceResponse response;
  ABSL_CHECK(TextFormat::ParseFromString(textproto, &response)) << textproto;
  return response;
}

template <class M>
std::string Describe(const M& m) {
  return testing::DescribeMatcher<const TestResult&>(m, /*negation=*/false);
}

template <class M>
std::string DescribeNegation(const M& m) {
  return testing::DescribeMatcher<const TestResult&>(m, /*negation=*/true);
}

// Runs `m` on `arg` once.  The returned AssertionResult succeeds iff `m`
// matched and its message is the matcher's explanation; check both with
// Accepts()/Rejects() below, so that a wrong verdict is reported at the caller.
template <class M, class T>
testing::AssertionResult Explain(const M& m, const T& arg) {
  testing::StringMatchResultListener listener;
  const bool matched = testing::ExplainMatchResult(m, arg, &listener);
  return (matched ? testing::AssertionSuccess() : testing::AssertionFailure())
         << listener.str();
}

// Matches an Explain() result whose matcher accepted the value and whose
// explanation matches `explanation` (a string or a matcher on std::string).
MATCHER_P(Accepts, explanation,
          absl::StrCat(negation ? "isn't accepted, or is" : "is",
                       " accepted with an explanation that ",
                       testing::DescribeMatcher<std::string>(explanation))) {
  if (!arg) {
    *result_listener << "the matcher rejected the value: " << arg.message();
    return false;
  }
  return testing::ExplainMatchResult(explanation, std::string(arg.message()),
                                     result_listener);
}

// Matches an Explain() result whose matcher rejected the value and whose
// explanation matches `explanation` (a string or a matcher on std::string).
MATCHER_P(Rejects, explanation,
          absl::StrCat(negation ? "isn't rejected, or is" : "is",
                       " rejected with an explanation that ",
                       testing::DescribeMatcher<std::string>(explanation))) {
  if (arg) {
    *result_listener << "the matcher accepted the value: " << arg.message();
    return false;
  }
  return testing::ExplainMatchResult(explanation, std::string(arg.message()),
                                     result_listener);
}

// ---------------------------------------------------------------------------
// Descriptions
// ---------------------------------------------------------------------------

TEST(MatcherDescriptionTest, ParsedPayload) {
  EXPECT_EQ(Describe(ParsedPayload(EqualsTextProto("optional_int32: 9"))),
            "parsed payload equals text proto \"optional_int32: 9\"");
  EXPECT_EQ(
      DescribeNegation(ParsedPayload(EqualsTextProto("optional_int32: 9"))),
      "parsed payload doesn't equal text proto \"optional_int32: 9\"");
}

TEST(MatcherDescriptionTest, ParsedPayloadWithGenericMatcher) {
  EXPECT_EQ(Describe(ParsedPayload(_)), "parsed payload is anything");
  EXPECT_EQ(Describe(ParsedPayload(Not(EqualsTextProto("optional_int32: 9")))),
            "parsed payload doesn't equal text proto \"optional_int32: 9\"");
}

TEST(MatcherDescriptionTest, Payload) {
  EXPECT_EQ(Describe(Payload(StrEq("foo"))), "payload is equal to \"foo\"");
  EXPECT_EQ(DescribeNegation(Payload(StrEq("foo"))),
            "payload isn't equal to \"foo\"");
}

TEST(MatcherDescriptionTest, PayloadWithBytes) {
  EXPECT_EQ(Describe(Payload(std::string("foo"))),
            "payload is equal to \"foo\"");
  EXPECT_EQ(DescribeNegation(Payload(absl::string_view("foo"))),
            "payload isn't equal to \"foo\"");
  EXPECT_EQ(Describe(Payload(VarintField(1, 2))),
            "payload is equal to \"\\010\\002\"");
  EXPECT_EQ(DescribeNegation(Payload(VarintField(1, 2))),
            "payload isn't equal to \"\\010\\002\"");
}

TEST(MatcherDescriptionTest, EqualsTextProto) {
  EXPECT_EQ(testing::DescribeMatcher<const Message&>(
                EqualsTextProto("optional_int32: 9")),
            "equals text proto \"optional_int32: 9\"");
  EXPECT_EQ(testing::DescribeMatcher<const Message&>(
                EqualsTextProto("optional_int32: 9"), /*negation=*/true),
            "doesn't equal text proto \"optional_int32: 9\"");
}

TEST(MatcherDescriptionTest, EqualsBinaryProto) {
  EXPECT_EQ(testing::DescribeMatcher<const Message&>(
                EqualsBinaryProto(VarintField(1, 9))),
            "equals binary proto \"\\010\\t\"");
  EXPECT_EQ(testing::DescribeMatcher<const Message&>(
                EqualsBinaryProto(std::string("\010\t")), /*negation=*/true),
            "doesn't equal binary proto \"\\010\\t\"");
}

TEST(MatcherDescriptionTest, HasUnknownFieldsInOrder) {
  UnknownFieldSet expected;
  expected.AddLengthDelimited(666, "abc");
  expected.AddVarint(666, 123);
  EXPECT_EQ(testing::DescribeMatcher<const Message&>(
                HasUnknownFieldsInOrder(expected)),
            "has exactly the unknown fields [666: \"abc\", 666: 123], in that "
            "order");
  EXPECT_EQ(testing::DescribeMatcher<const Message&>(
                HasUnknownFieldsInOrder(expected), /*negation=*/true),
            "doesn't have exactly the unknown fields [666: \"abc\", 666: 123] "
            "in that order");
}

TEST(MatcherDescriptionTest, HasUnknownFieldsInOrderCoversEveryFieldKind) {
  UnknownFieldSet expected;
  expected.AddVarint(1, 2);
  expected.AddFixed32(3, 4);
  expected.AddFixed64(5, 6);
  expected.AddLengthDelimited(7, "\001\n");
  expected.AddGroup(8)->AddVarint(9, 10);
  EXPECT_EQ(testing::DescribeMatcher<const Message&>(
                HasUnknownFieldsInOrder(expected)),
            "has exactly the unknown fields [1: 2, 3: fixed32(4), 5: "
            "fixed64(6), 7: \"\\001\\n\", 8: [9: 10]], in that order");
  EXPECT_EQ(testing::DescribeMatcher<const Message&>(
                HasUnknownFieldsInOrder(UnknownFieldSet())),
            "has exactly the unknown fields [], in that order");
}

TEST(MatcherDescriptionTest, FailureMatchers) {
  EXPECT_EQ(Describe(IsParseError()), "is a parse error");
  EXPECT_EQ(DescribeNegation(IsParseError()), "is not a parse error");
  EXPECT_EQ(Describe(IsSerializeError()), "is a serialize error");
  EXPECT_EQ(DescribeNegation(IsSerializeError()), "is not a serialize error");
}

TEST(MatcherDescriptionTest, Yields) {
  EXPECT_EQ(
      Describe(Yields(IsParseError())),
      "yields a result that is a parse error (or is an expected failure)");
  EXPECT_EQ(DescribeNegation(Yields(IsParseError())),
            "doesn't yield a result that is a parse error, nor an expected "
            "failure");
  EXPECT_EQ(Describe(Yields(AnyOf(IsParseError(), ParsedPayload(_)))),
            "yields a result that (is a parse error) or (parsed payload is "
            "anything) (or is an expected failure)");
}

// ---------------------------------------------------------------------------
// Response handling shared by ParsedPayload() and Payload()
// ---------------------------------------------------------------------------

TEST(PayloadMatcherTest, EmptyResponse) {
  TestResult result =
      CreateResult("foo", ::conformance::PROTOBUF, ConformanceResponse());

  EXPECT_THAT(Explain(ParsedPayload(_), result),
              Rejects("Response didn't have any field in the Response."));
  EXPECT_THAT(Explain(Payload(_), result),
              Rejects("Response didn't have any field in the Response."));
}

struct ErrorResponseCase {
  absl::string_view name;
  absl::string_view response;
};

class PayloadMatcherErrorResponseTest
    : public testing::TestWithParam<ErrorResponseCase> {};

TEST_P(PayloadMatcherErrorResponseTest, IsAFailureForParsedPayload) {
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ResponseFromText(GetParam().response));

  EXPECT_THAT(Explain(ParsedPayload(_), result),
              Rejects("Failed to parse input or produce output."));
}

TEST_P(PayloadMatcherErrorResponseTest, IsAFailureForPayload) {
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ResponseFromText(GetParam().response));

  EXPECT_THAT(Explain(Payload(_), result),
              Rejects("Failed to parse input or produce output."));
}

INSTANTIATE_TEST_SUITE_P(
    ErrorResponses, PayloadMatcherErrorResponseTest,
    Values(ErrorResponseCase{"ParseError", R"pb(parse_error: "foo")pb"},
           ErrorResponseCase{"SerializeError", R"pb(serialize_error: "foo")pb"},
           ErrorResponseCase{"RuntimeError", R"pb(runtime_error: "foo")pb"},
           ErrorResponseCase{"TimeoutError", R"pb(timeout_error: "foo")pb"}),
    [](const TestParamInfo<ErrorResponseCase>& info) {
      return std::string(info.param.name);
    });

TEST(PayloadMatcherTest, Skipped) {
  TestResult result =
      CreateResult("foo", ::conformance::PROTOBUF,
                   ResponseFromText(R"pb(skipped: "skipped message")pb"));

  EXPECT_THAT(Explain(ParsedPayload(_), result),
              Rejects("the testee skipped the test: skipped message"));
  EXPECT_THAT(Explain(Payload(_), result),
              Rejects("the testee skipped the test: skipped message"));
}

TEST(PayloadMatcherTest, WrongOutputFormat) {
  TestResult result =
      CreateResult("foo", ::conformance::PROTOBUF,
                   ResponseFromText(R"pb(json_payload: "{}")pb"));

  EXPECT_THAT(
      Explain(ParsedPayload(_), result),
      Rejects("Test was asked for PROTOBUF output but provided JSON instead."));
  EXPECT_THAT(
      Explain(Payload(_), result),
      Rejects("Test was asked for PROTOBUF output but provided JSON instead."));
}

TEST(PayloadMatcherTest, WrongOutputFormatText) {
  TestResult result = CreateResult("foo", ::conformance::TEXT_FORMAT,
                                   ProtobufPayload(VarintField(1, 2)));

  EXPECT_THAT(
      Explain(ParsedPayload(EqualsTextProto("optional_int32: 2")), result),
      Rejects("Test was asked for TEXT_FORMAT output but provided PROTOBUF "
              "instead."));
}

TEST(PayloadMatcherTest, WrongOutputFormatJspb) {
  TestResult result =
      CreateResult("foo", ::conformance::JSON,
                   ResponseFromText(R"pb(jspb_payload: "[]")pb"));

  EXPECT_THAT(
      Explain(Payload(_), result),
      Rejects("Test was asked for JSON output but provided JSPB instead."));
}

TEST(PayloadMatcherTest, UnspecifiedOutputFormatNeverMatchesAPayload) {
  TestResult result = CreateResult("foo", ::conformance::UNSPECIFIED,
                                   ProtobufPayload(VarintField(1, 2)));

  EXPECT_THAT(
      Explain(ParsedPayload(_), result),
      Rejects("Test was asked for UNSPECIFIED output but provided PROTOBUF "
              "instead."));
  EXPECT_THAT(
      Explain(Payload(_), result),
      Rejects("Test was asked for UNSPECIFIED output but provided PROTOBUF "
              "instead."));
}

// ---------------------------------------------------------------------------
// ParsedPayload()
// ---------------------------------------------------------------------------

TEST(ParsedPayloadTest, Matches) {
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(VarintField(1, 2)));

  EXPECT_THAT(
      Explain(ParsedPayload(EqualsTextProto("optional_int32: 2")), result),
      Accepts(IsEmpty()));
}

TEST(ParsedPayloadTest, UnparseableProtobufPayload) {
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(Wire("\001")));

  EXPECT_THAT(
      Explain(ParsedPayload(EqualsTextProto("")), result),
      Rejects("Protobuf output we received from test was unparseable."));
}

TEST(ParsedPayloadTest, UnparseableTextPayload) {
  TestResult result =
      CreateResult("foo", ::conformance::TEXT_FORMAT,
                   ResponseFromText(R"pb(text_payload: "nonsense: 1")pb"));

  EXPECT_THAT(
      Explain(ParsedPayload(EqualsTextProto("")), result),
      Rejects("TEXT_FORMAT output we received from test was unparseable."));
}

TEST(ParsedPayloadTest, UnparseablePayloadFailsEvenForAnything) {
  // The payload has to be decodable before any inner matcher gets a say.
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(Wire("\001")));

  EXPECT_THAT(
      Explain(ParsedPayload(_), result),
      Rejects("Protobuf output we received from test was unparseable."));
}

TEST(ParsedPayloadTest, JsonOutput) {
  TestResult result = CreateResult(
      "foo", ::conformance::JSON,
      ResponseFromText(R"pb(json_payload: "{\"optionalInt32\": 9}")pb"));

  EXPECT_THAT(
      Explain(ParsedPayload(EqualsTextProto("optional_int32: 9")), result),
      Accepts(IsEmpty()));
  EXPECT_TRUE(
      Value(result, ParsedPayload(EqualsBinaryProto(VarintField(1, 9)))));
  EXPECT_THAT(
      Explain(ParsedPayload(EqualsTextProto("optional_int32: 1")), result),
      Rejects("Output was not equivalent to reference message: "
              "modified: optional_int32: 1 -> 9\n"));
}

TEST(ParsedPayloadTest, UnparseableJsonPayload) {
  TestResult result = CreateResult(
      "foo", ::conformance::JSON,
      ResponseFromText(R"pb(json_payload: "{\"nonsense\": 1}")pb"));

  EXPECT_THAT(Explain(ParsedPayload(_), result),
              Rejects("JSON output we received from test was unparseable."));
  EXPECT_THAT(
      Explain(ParsedPayload(_),
              CreateResult("bar", ::conformance::JSON,
                           ResponseFromText(R"pb(json_payload: "{")pb"))),
      Rejects("JSON output we received from test was unparseable."));
}

TEST(ParsedPayloadTest, JsonOutputIsDecodedWithLegacyLeniency) {
  // The legacy runner decoded JSON output with the default ParseOptions,
  // which tolerate some non-conformant output; so does ParsedPayload() until
  // b/563658359 tightens it.  Single-quoted strings are one such case; this
  // test documents the current behaviour rather than endorsing it.
  TestResult result = CreateResult(
      "foo", ::conformance::JSON,
      ResponseFromText(R"pb(json_payload: "{'optionalInt32': 9}")pb"));

  EXPECT_TRUE(
      Value(result, ParsedPayload(EqualsTextProto("optional_int32: 9"))));
}

TEST(ParsedPayloadTest, TextOutput) {
  TestResult result = CreateResult(
      "foo", ::conformance::TEXT_FORMAT,
      ResponseFromText(R"pb(text_payload: "optional_int32: 9")pb"));

  EXPECT_TRUE(
      Value(result, ParsedPayload(EqualsTextProto("optional_int32: 9"))));
}

TEST(ParsedPayloadTest, TextOutputFieldNumbersRejectedByDefault) {
  // Unless the testee was asked to print unknown fields, output by field
  // number is not valid text format (same as the legacy runner).
  TestResult result =
      CreateResult("foo", ::conformance::TEXT_FORMAT,
                   ResponseFromText(R"pb(text_payload: "1: 9")pb"));

  EXPECT_THAT(
      Explain(ParsedPayload(EqualsTextProto("optional_int32: 9")), result),
      Rejects("TEXT_FORMAT output we received from test was unparseable."));
}

TEST(ParsedPayloadTest, TextOutputFieldNumbersAcceptedForUnknownFields) {
  // Testees asked to print unknown fields emit them by number.
  ConformanceRequest request = BinaryInputRequest(::conformance::TEXT_FORMAT);
  request.set_print_unknown_fields(true);
  TestResult result = CreateResult(
      "foo", request, ResponseFromText(R"pb(text_payload: "1: 9")pb"));

  EXPECT_TRUE(
      Value(result, ParsedPayload(EqualsTextProto("optional_int32: 9"))));
}

TEST(ParsedPayloadTest, EqualsTextProtoMismatch) {
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(VarintField(1, 2)));

  EXPECT_THAT(
      Explain(ParsedPayload(EqualsTextProto("optional_int32: 1")), result),
      Rejects("Output was not equivalent to reference message: "
              "modified: optional_int32: 1 -> 2\n"));
}

TEST(ParsedPayloadTest, EqualsTextProtoMatchesNan) {
  TestAllTypesProto2 message;
  message.set_optional_float(std::nanf(""));
  message.set_optional_double(std::nan(""));
  TestResult result =
      CreateResult("foo", ::conformance::PROTOBUF,
                   ProtobufPayload(Wire(message.SerializeAsString())));

  EXPECT_TRUE(Value(result, ParsedPayload(EqualsTextProto(
                                "optional_float: nan optional_double: nan"))));
}

TEST(ParsedPayloadDeathTest, EqualsTextProtoUnparseableExpected) {
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(VarintField(1, 2)));

  EXPECT_DEATH(
      (void)Explain(ParsedPayload(EqualsTextProto("unknown: 1")), result),
      "Failed to parse expected text proto.*unknown: 1");
}

TEST(ParsedPayloadTest, EqualsBinaryProto) {
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(VarintField(1, 2)));

  // Equivalent, but not byte-identical (over-long varint encoding).
  EXPECT_TRUE(Value(
      result, ParsedPayload(EqualsBinaryProto(LongVarintField(1, 2, 1)))));
  EXPECT_TRUE(Value(result, ParsedPayload(EqualsBinaryProto(
                                std::string(VarintField(1, 2).data())))));
  EXPECT_TRUE(Value(
      result, ParsedPayload(EqualsBinaryProto(VarintField(1, 2).data()))));
}

TEST(ParsedPayloadTest, EqualsBinaryProtoWithTextOutput) {
  TestResult result = CreateResult(
      "foo", ::conformance::TEXT_FORMAT,
      ResponseFromText(R"pb(text_payload: "optional_int32: 9")pb"));

  EXPECT_TRUE(
      Value(result, ParsedPayload(EqualsBinaryProto(VarintField(1, 9)))));
}

TEST(ParsedPayloadTest, EqualsBinaryProtoMismatch) {
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(VarintField(1, 2)));

  EXPECT_THAT(
      Explain(ParsedPayload(EqualsBinaryProto(VarintField(1, 1))), result),
      Rejects("Output was not equivalent to reference message: "
              "modified: optional_int32: 1 -> 2\n"));
}

TEST(ParsedPayloadDeathTest, EqualsBinaryProtoUnparseableExpected) {
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(VarintField(1, 2)));

  EXPECT_DEATH(
      (void)Explain(ParsedPayload(EqualsBinaryProto(Wire("\001"))), result),
      "Failed to parse expected wire data");
}

TEST(ParsedPayloadTest, AcceptsAnyMessageMatcher) {
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(VarintField(1, 2)));

  EXPECT_TRUE(Value(result, ParsedPayload(_)));
  EXPECT_TRUE(Value(
      result, ParsedPayload(AllOf(EqualsTextProto("optional_int32: 2"),
                                  Not(EqualsTextProto("optional_int32: 3"))))));
}

TEST(ParsedPayloadTest, InnerMatcherWithoutExplanation) {
  // Inner matchers that don't explain themselves still get a useful failure
  // message, including the decoded payload.
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(VarintField(1, 2)));

  EXPECT_THAT(
      Explain(ParsedPayload(Not(EqualsTextProto("optional_int32: 2"))), result),
      Rejects("Expect: parsed payload doesn't equal text proto "
              "\"optional_int32: 2\", but got: {optional_int32: 2}"));
}

TEST(ParsedPayloadTest, ComposesWithGMock) {
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(VarintField(1, 2)));

  EXPECT_TRUE(Value(result, Not(ParsedPayload(EqualsTextProto("")))));
  EXPECT_TRUE(Value(result, AnyOf(IsParseError(), ParsedPayload(EqualsTextProto(
                                                      "optional_int32: 2")))));
  EXPECT_TRUE(Value(result, AllOf(Not(IsParseError()), ParsedPayload(_))));
}

// ---------------------------------------------------------------------------
// ParsedPayloadAs()
// ---------------------------------------------------------------------------

TEST(ParsedPayloadAsTest, DecodesBinaryPayloadAsTheGivenType) {
  // Field 1001 is unknown to the test's type (TestAllTypesProto2) but is
  // optional_int32 of the shadow type UnknownToTestAllTypes.
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(VarintField(1001, 7)));

  EXPECT_THAT(Explain(ParsedPayloadAs(UnknownToTestAllTypes::descriptor(),
                                      EqualsTextProto("optional_int32: 7")),
                      result),
              Accepts(IsEmpty()));
  // Decoded as the test's own type the field stays unknown.
  EXPECT_THAT(
      Explain(ParsedPayload(EqualsTextProto("optional_int32: 7")), result),
      Rejects("Output was not equivalent to reference message: "
              "added: 1001[0]: 7\n"
              "deleted: optional_int32: 7\n"));
}

TEST(ParsedPayloadAsTest, TextOutputFieldNumbersAcceptedForUnknownFields) {
  // A testee asked to print unknown fields emits them by number; the shadow
  // type gives them names again.
  ConformanceRequest request = BinaryInputRequest(::conformance::TEXT_FORMAT);
  request.set_print_unknown_fields(true);
  TestResult result = CreateResult(
      "foo", request, ResponseFromText(R"pb(text_payload: "1001: 7")pb"));

  EXPECT_THAT(Explain(ParsedPayloadAs(UnknownToTestAllTypes::descriptor(),
                                      EqualsTextProto("optional_int32: 7")),
                      result),
              Accepts(IsEmpty()));
}

TEST(ParsedPayloadAsTest, TextOutputFieldNumbersRejectedByDefault) {
  // Without print_unknown_fields, field numbers aren't valid text format no
  // matter which type the payload is decoded as (same as the legacy runner).
  TestResult result =
      CreateResult("foo", ::conformance::TEXT_FORMAT,
                   ResponseFromText(R"pb(text_payload: "1001: 7")pb"));

  EXPECT_THAT(Explain(ParsedPayloadAs(UnknownToTestAllTypes::descriptor(),
                                      EqualsTextProto("optional_int32: 7")),
                      result),
              Rejects("TEXT_FORMAT output we received from test was "
                      "unparseable."));
}

TEST(ParsedPayloadAsTest, FailureMessagesMatchParsedPayload) {
  // With the test's own type as the override, ParsedPayloadAs() must behave
  // exactly like ParsedPayload(), failure messages included.
  const Descriptor* type = TestAllTypesProto2::descriptor();
  {
    TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                     ProtobufPayload(Wire("\001")));
    EXPECT_THAT(
        Explain(ParsedPayloadAs(type, EqualsTextProto("")), result),
        Rejects("Protobuf output we received from test was unparseable."));
  }
  {
    TestResult result =
        CreateResult("foo", ::conformance::TEXT_FORMAT,
                     ResponseFromText(R"pb(text_payload: "nonsense: 1")pb"));
    EXPECT_THAT(
        Explain(ParsedPayloadAs(type, EqualsTextProto("")), result),
        Rejects("TEXT_FORMAT output we received from test was unparseable."));
  }
  {
    TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                     ProtobufPayload(VarintField(1, 2)));
    EXPECT_THAT(
        Explain(ParsedPayloadAs(type, EqualsTextProto("optional_int32: 1")),
                result),
        Rejects("Output was not equivalent to reference message: "
                "modified: optional_int32: 1 -> 2\n"));
    EXPECT_THAT(Explain(ParsedPayloadAs(
                            type, Not(EqualsTextProto("optional_int32: 2"))),
                        result),
                Rejects("Expect: parsed payload doesn't equal text proto "
                        "\"optional_int32: 2\", but got: {optional_int32: 2}"));
  }
  {
    TestResult result =
        CreateResult("foo", ::conformance::PROTOBUF,
                     ResponseFromText(R"pb(parse_error: "foo")pb"));
    EXPECT_THAT(Explain(ParsedPayloadAs(type, _), result),
                Rejects("Failed to parse input or produce output."));
  }
}

TEST(MatcherDescriptionTest, ParsedPayloadAs) {
  // The description (gtest's "Expected:" line, never a failure-list message)
  // names the type the payload is decoded as.
  EXPECT_EQ(Describe(ParsedPayloadAs(UnknownToTestAllTypes::descriptor(),
                                     EqualsTextProto("optional_int32: 9"))),
            "parsed payload (as "
            "protobuf_test_messages.proto2.UnknownToTestAllTypes) equals text "
            "proto \"optional_int32: 9\"");
  EXPECT_EQ(
      DescribeNegation(ParsedPayloadAs(UnknownToTestAllTypes::descriptor(),
                                       EqualsTextProto("optional_int32: 9"))),
      "parsed payload (as protobuf_test_messages.proto2.UnknownToTestAllTypes) "
      "doesn't equal text proto \"optional_int32: 9\"");
}

// ---------------------------------------------------------------------------
// Payload()
// ---------------------------------------------------------------------------

TEST(RawPayloadTest, MatchesBytes) {
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(VarintField(1, 2)));

  EXPECT_TRUE(Value(result, Payload(VarintField(1, 2))));
  EXPECT_TRUE(Value(result, Payload(std::string("\010\002"))));
  EXPECT_TRUE(Value(result, Payload(absl::string_view("\010\002"))));
  EXPECT_THAT(Explain(Payload(VarintField(1, 2)), result), Accepts(IsEmpty()));
}

TEST(RawPayloadTest, BytesMismatch) {
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(VarintField(1, 2)));

  // Equivalent messages, but not byte-identical.  The failure message is the
  // legacy one.
  EXPECT_THAT(Explain(Payload(LongVarintField(1, 2, 1)), result),
              Rejects("Output was not equivalent to reference message: "
                      "Expect: \\010\\202\\000, but got: \\010\\002"));
}

// The generic-matcher tests use text output, which (unlike binary output)
// doesn't have to be parseable.
ConformanceResponse TextPayload(absl::string_view text) {
  ConformanceResponse response;
  response.set_text_payload(text);
  return response;
}

TEST(RawPayloadTest, MatchesGenericMatcher) {
  TestResult result =
      CreateResult("foo", ::conformance::TEXT_FORMAT, TextPayload("foo"));

  EXPECT_TRUE(Value(result, Payload(Eq("foo"))));
  EXPECT_TRUE(Value(result, Payload(StrEq("foo"))));
  EXPECT_TRUE(Value(result, Payload(HasSubstr("oo"))));
  EXPECT_TRUE(Value(result, Payload(Not(HasSubstr("bar")))));
  EXPECT_TRUE(Value(result, Payload(_)));
}

TEST(RawPayloadTest, GenericMatcherMismatch) {
  TestResult result =
      CreateResult("foo", ::conformance::TEXT_FORMAT, TextPayload("foo"));

  // Eq() doesn't explain itself, so the description and actual payload are
  // reported instead; a readable format is shown escaped.
  EXPECT_THAT(Explain(Payload(Eq(std::string("bar"))), result),
              Rejects("Expect: payload is equal to \"bar\", but got: \"foo\""));
}

TEST(RawPayloadTest, GenericMatcherMismatchShowsBinaryPayloadInOctal) {
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(VarintField(1, 2)));

  EXPECT_THAT(Explain(Payload(Eq(std::string("bar"))), result),
              Rejects("Expect: payload is equal to \"bar\", but got: "
                      "\\010\\002"));
}

TEST(RawPayloadTest, GenericMatcherExplanationIsUsed) {
  TestResult result =
      CreateResult("foo", ::conformance::TEXT_FORMAT, TextPayload("foo"));

  EXPECT_THAT(Explain(Payload(testing::SizeIs(2)), result),
              Rejects("whose size 3 doesn't match"));
}

TEST(RawPayloadTest, UnparseableProtobufOutputFailsWhateverTheMatcher) {
  // Like the legacy runner's require_same_wire_format, binary output must
  // decode as the test's message type.
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(Wire("\001")));

  EXPECT_THAT(
      Explain(Payload(_), result),
      Rejects("Protobuf output we received from test was unparseable."));
  EXPECT_THAT(
      Explain(Payload(Wire("\001")), result),
      Rejects("Protobuf output we received from test was unparseable."));
}

TEST(RawPayloadTest, TextOutput) {
  TestResult result = CreateResult(
      "foo", ::conformance::TEXT_FORMAT,
      ResponseFromText(R"pb(text_payload: "optional_int32: 9")pb"));

  EXPECT_TRUE(Value(result, Payload(std::string("optional_int32: 9"))));
  // Text output isn't required to be parseable.
  EXPECT_TRUE(Value(
      CreateResult("bar", ::conformance::TEXT_FORMAT,
                   ResponseFromText(R"pb(text_payload: "nonsense: 1")pb")),
      Payload(std::string("nonsense: 1"))));
}

TEST(RawPayloadTest, JsonOutput) {
  // Raw payloads don't need to be decoded, so JSON works already.
  TestResult result =
      CreateResult("foo", ::conformance::JSON,
                   ResponseFromText(R"pb(json_payload: "{}")pb"));

  EXPECT_TRUE(Value(result, Payload(std::string("{}"))));
}

// ---------------------------------------------------------------------------
// EqualsTextProto() / EqualsBinaryProto() used directly on messages
// ---------------------------------------------------------------------------

TEST(EqualsTextProtoTest, Success) {
  TestAllTypesProto2 message;
  message.set_optional_int32(9);
  EXPECT_THAT(message, EqualsTextProto("optional_int32: 9"));
}

TEST(EqualsTextProtoTest, MatchesNan) {
  TestAllTypesProto2 message;
  message.set_optional_float(std::nanf(""));
  EXPECT_THAT(message, EqualsTextProto("optional_float: nan"));
}

TEST(EqualsTextProtoTest, Failure) {
  TestAllTypesProto2 message;
  message.set_optional_float(std::nanf(""));
  auto matcher = EqualsTextProto("optional_float: 1.0");
  EXPECT_THAT(Explain(matcher, message),
              Rejects("Output was not equivalent to reference message: "
                      "modified: optional_float: 1 -> nan\n"));
  EXPECT_THAT(message, Not(matcher));
}

TEST(EqualsTextProtoTest, WorksOnBaseMessage) {
  TestAllTypesProto2 message;
  message.set_optional_int32(9);
  const Message& base = message;
  EXPECT_THAT(base, EqualsTextProto("optional_int32: 9"));
  EXPECT_THAT(base, Not(EqualsTextProto("optional_int32: 8")));
}

TEST(EqualsTextProtoDeathTest, ParseFailure) {
  TestAllTypesProto2 message;
  EXPECT_DEATH((void)Explain(EqualsTextProto("unknown: 1.0"), message),
               "Failed to parse expected text proto.*unknown: 1.0");
}

TEST(EqualsBinaryProtoTest, Success) {
  TestAllTypesProto2 message;
  message.set_optional_int32(9);
  EXPECT_THAT(message, EqualsBinaryProto(VarintField(1, 9)));
  EXPECT_THAT(message, EqualsBinaryProto(std::string("\010\t")));
  EXPECT_THAT(message, EqualsBinaryProto(absl::string_view("\010\t")));
}

TEST(EqualsBinaryProtoTest, Failure) {
  TestAllTypesProto2 message;
  message.set_optional_int32(9);
  auto matcher = EqualsBinaryProto(VarintField(1, 8));
  EXPECT_THAT(Explain(matcher, message),
              Rejects("Output was not equivalent to reference message: "
                      "modified: optional_int32: 8 -> 9\n"));
  EXPECT_THAT(message, Not(matcher));
}

TEST(EqualsBinaryProtoDeathTest, ParseFailure) {
  TestAllTypesProto2 message;
  EXPECT_DEATH((void)Explain(EqualsBinaryProto(Wire("\001")), message),
               "Failed to parse expected wire data");
}

// ---------------------------------------------------------------------------
// HasUnknownFieldsInOrder()
// ---------------------------------------------------------------------------

// Field 666 is defined by none of the test messages, so it always stays
// unknown.

TEST(HasUnknownFieldsInOrderTest, SameFieldsInSameOrder) {
  UnknownFieldSet expected;
  expected.AddLengthDelimited(666, "abc");
  expected.AddVarint(666, 123);
  TestAllTypesProto2 message;
  message.mutable_unknown_fields()->AddLengthDelimited(666, "abc");
  message.mutable_unknown_fields()->AddVarint(666, 123);

  EXPECT_THAT(Explain(HasUnknownFieldsInOrder(expected), message),
              Accepts(IsEmpty()));
}

TEST(HasUnknownFieldsInOrderTest, DifferentOrder) {
  UnknownFieldSet expected;
  expected.AddLengthDelimited(666, "abc");
  expected.AddVarint(666, 123);
  TestAllTypesProto2 message;
  message.mutable_unknown_fields()->AddVarint(666, 123);
  message.mutable_unknown_fields()->AddLengthDelimited(666, "abc");

  auto matcher = HasUnknownFieldsInOrder(expected);
  EXPECT_THAT(Explain(matcher, message), Rejects("Unknown field mismatch"));
  EXPECT_THAT(message, Not(matcher));
}

TEST(HasUnknownFieldsInOrderTest, MissingField) {
  UnknownFieldSet expected;
  expected.AddLengthDelimited(666, "abc");
  expected.AddVarint(666, 123);
  TestAllTypesProto2 message;
  message.mutable_unknown_fields()->AddLengthDelimited(666, "abc");

  EXPECT_THAT(Explain(HasUnknownFieldsInOrder(expected), message),
              Rejects("Unknown field mismatch"));
}

TEST(HasUnknownFieldsInOrderTest, ExtraField) {
  UnknownFieldSet expected;
  expected.AddLengthDelimited(666, "abc");
  TestAllTypesProto2 message;
  message.mutable_unknown_fields()->AddLengthDelimited(666, "abc");
  message.mutable_unknown_fields()->AddVarint(666, 123);

  EXPECT_THAT(Explain(HasUnknownFieldsInOrder(expected), message),
              Rejects("Unknown field mismatch"));
}

TEST(HasUnknownFieldsInOrderTest, DifferentValue) {
  UnknownFieldSet expected;
  expected.AddVarint(666, 123);
  TestAllTypesProto2 message;
  message.mutable_unknown_fields()->AddVarint(666, 124);

  EXPECT_THAT(Explain(HasUnknownFieldsInOrder(expected), message),
              Rejects("Unknown field mismatch"));
}

TEST(HasUnknownFieldsInOrderTest, DifferentFieldNumber) {
  UnknownFieldSet expected;
  expected.AddVarint(666, 123);
  TestAllTypesProto2 message;
  message.mutable_unknown_fields()->AddVarint(667, 123);

  EXPECT_THAT(Explain(HasUnknownFieldsInOrder(expected), message),
              Rejects("Unknown field mismatch"));
}

TEST(HasUnknownFieldsInOrderTest, DifferentWireType) {
  UnknownFieldSet expected;
  expected.AddVarint(666, 1);
  TestAllTypesProto2 message;
  message.mutable_unknown_fields()->AddFixed32(666, 1);

  EXPECT_THAT(Explain(HasUnknownFieldsInOrder(expected), message),
              Rejects("Unknown field mismatch"));
}

TEST(HasUnknownFieldsInOrderTest, GroupsAreComparedByContent) {
  UnknownFieldSet expected;
  expected.AddGroup(666)->AddVarint(1, 2);
  TestAllTypesProto2 same;
  same.mutable_unknown_fields()->AddGroup(666)->AddVarint(1, 2);
  TestAllTypesProto2 different;
  different.mutable_unknown_fields()->AddGroup(666)->AddVarint(1, 3);

  EXPECT_THAT(Explain(HasUnknownFieldsInOrder(expected), same),
              Accepts(IsEmpty()));
  EXPECT_THAT(Explain(HasUnknownFieldsInOrder(expected), different),
              Rejects("Unknown field mismatch"));
}

TEST(HasUnknownFieldsInOrderTest, EmptyExpectedMatchesOnlyNoUnknownFields) {
  TestAllTypesProto2 without;
  TestAllTypesProto2 with;
  with.mutable_unknown_fields()->AddVarint(666, 1);

  EXPECT_THAT(Explain(HasUnknownFieldsInOrder(UnknownFieldSet()), without),
              Accepts(IsEmpty()));
  EXPECT_THAT(Explain(HasUnknownFieldsInOrder(UnknownFieldSet()), with),
              Rejects("Unknown field mismatch"));
}

TEST(HasUnknownFieldsInOrderTest, IgnoresKnownFields) {
  UnknownFieldSet expected;
  expected.AddVarint(666, 123);
  TestAllTypesProto2 message;
  message.set_optional_int32(9);
  message.set_optional_string("known");
  message.mutable_unknown_fields()->AddVarint(666, 123);

  EXPECT_THAT(Explain(HasUnknownFieldsInOrder(expected), message),
              Accepts(IsEmpty()));
}

// The end-to-end case: the mismatch text is what ends up in failure lists.
TEST(ParsedPayloadTest, HasUnknownFieldsInOrderKeepsTheLegacyMessage) {
  UnknownFieldSet expected;
  expected.AddLengthDelimited(666, "abc");
  expected.AddVarint(666, 123);
  TestResult in_order =
      CreateResult("foo", ::conformance::PROTOBUF,
                   ProtobufPayload(Wire(LengthPrefixedField(666, "abc"),
                                        VarintField(666, 123))));
  TestResult reordered =
      CreateResult("foo", ::conformance::PROTOBUF,
                   ProtobufPayload(Wire(VarintField(666, 123),
                                        LengthPrefixedField(666, "abc"))));

  EXPECT_THAT(
      Explain(ParsedPayload(HasUnknownFieldsInOrder(expected)), in_order),
      Accepts(IsEmpty()));
  EXPECT_THAT(
      Explain(ParsedPayload(HasUnknownFieldsInOrder(expected)), reordered),
      Rejects("Unknown field mismatch"));
}

TEST(ParsedPayloadAsTest, HasUnknownFieldsInOrderSeesTheGivenTypesUnknowns) {
  // Field 1001 is unknown to the test's type (TestAllTypesProto2) but is
  // optional_int32 of the shadow type UnknownToTestAllTypes; 666 is unknown
  // to both.
  TestResult result = CreateResult(
      "foo", ::conformance::PROTOBUF,
      ProtobufPayload(Wire(VarintField(1001, 7), VarintField(666, 1))));
  UnknownFieldSet both;
  both.AddVarint(1001, 7);
  both.AddVarint(666, 1);
  UnknownFieldSet only_666;
  only_666.AddVarint(666, 1);

  EXPECT_THAT(Explain(ParsedPayload(HasUnknownFieldsInOrder(both)), result),
              Accepts(IsEmpty()));
  EXPECT_THAT(Explain(ParsedPayloadAs(UnknownToTestAllTypes::descriptor(),
                                      HasUnknownFieldsInOrder(only_666)),
                      result),
              Accepts(IsEmpty()));
  EXPECT_THAT(Explain(ParsedPayloadAs(UnknownToTestAllTypes::descriptor(),
                                      HasUnknownFieldsInOrder(both)),
                      result),
              Rejects("Unknown field mismatch"));
}

// ---------------------------------------------------------------------------
// Failure matchers
// ---------------------------------------------------------------------------

TEST(FailureMatcherTest, ParseErrorMatches) {
  TestResult result =
      CreateResult("foo", ::conformance::PROTOBUF,
                   ResponseFromText(R"pb(parse_error: "failed to parse")pb"));

  EXPECT_THAT(Explain(IsParseError(), result), Accepts(IsEmpty()));
  EXPECT_FALSE(Value(result, IsSerializeError()));
}

TEST(FailureMatcherTest, ParseErrorWithPayload) {
  TestResult result =
      CreateResult("foo", ::conformance::PROTOBUF, ProtobufPayload(Wire()));

  EXPECT_THAT(Explain(IsParseError(), result),
              Rejects("Should have failed to parse, but didn't."));
}

TEST(FailureMatcherTest, ParseErrorEmptyResponse) {
  TestResult result =
      CreateResult("foo", ::conformance::PROTOBUF, ConformanceResponse());

  EXPECT_THAT(Explain(IsParseError(), result),
              Rejects("Should have failed to parse, but didn't."));
}

TEST(FailureMatcherTest, ParseErrorSerializeError) {
  TestResult result =
      CreateResult("foo", ::conformance::PROTOBUF,
                   ResponseFromText(R"pb(serialize_error: "x")pb"));

  EXPECT_THAT(Explain(IsParseError(), result),
              Rejects("Should have failed to parse, but didn't."));
}

TEST(FailureMatcherTest, ParseErrorRuntimeErrorWithBinaryInput) {
  // The legacy binary runner reports runtime errors specially.
  TestResult result =
      CreateResult("foo", BinaryInputRequest(::conformance::PROTOBUF),
                   ResponseFromText(R"pb(runtime_error: "x")pb"));

  EXPECT_THAT(
      Explain(IsParseError(), result),
      Rejects("Should have failed to parse, but raised an error instead."));
}

TEST(FailureMatcherTest, ParseErrorRuntimeErrorWithTextInput) {
  // The legacy text format runner doesn't distinguish runtime errors.
  TestResult result =
      CreateResult("foo", TextInputRequest(::conformance::TEXT_FORMAT),
                   ResponseFromText(R"pb(runtime_error: "x")pb"));

  EXPECT_THAT(Explain(IsParseError(), result),
              Rejects("Should have failed to parse, but didn't."));
}

TEST(FailureMatcherTest, ParseErrorRuntimeErrorWithJsonInput) {
  // Neither does the legacy JSON runner.
  TestResult result =
      CreateResult("foo", JsonInputRequest(::conformance::PROTOBUF),
                   ResponseFromText(R"pb(runtime_error: "x")pb"));

  EXPECT_THAT(Explain(IsParseError(), result),
              Rejects("Should have failed to parse, but didn't."));
}

TEST(FailureMatcherTest, ParseErrorSkipped) {
  TestResult result =
      CreateResult("foo", ::conformance::PROTOBUF,
                   ResponseFromText(R"pb(skipped: "skipped message")pb"));

  EXPECT_THAT(Explain(IsParseError(), result),
              Rejects("the testee skipped the test: skipped message"));
}

TEST(FailureMatcherTest, SerializeErrorMatches) {
  TestResult result =
      CreateResult("foo", ::conformance::PROTOBUF,
                   ResponseFromText(R"pb(serialize_error: "failed")pb"));

  EXPECT_THAT(Explain(IsSerializeError(), result), Accepts(IsEmpty()));
  EXPECT_FALSE(Value(result, IsParseError()));
}

TEST(FailureMatcherTest, SerializeErrorWithPayload) {
  TestResult result =
      CreateResult("foo", ::conformance::PROTOBUF, ProtobufPayload(Wire()));

  EXPECT_THAT(Explain(IsSerializeError(), result),
              Rejects("Should have failed to serialize, but didn't."));
}

TEST(FailureMatcherTest, SerializeErrorRuntimeError) {
  // Unlike parse errors, there is no special message for runtime errors.
  TestResult result =
      CreateResult("foo", ::conformance::PROTOBUF,
                   ResponseFromText(R"pb(runtime_error: "x")pb"));

  EXPECT_THAT(Explain(IsSerializeError(), result),
              Rejects("Should have failed to serialize, but didn't."));
}

TEST(FailureMatcherTest, ComposesWithGMock) {
  TestResult result =
      CreateResult("foo", ::conformance::PROTOBUF,
                   ResponseFromText(R"pb(parse_error: "failed to parse")pb"));

  EXPECT_TRUE(Value(result, Not(IsSerializeError())));
  EXPECT_TRUE(Value(result, AnyOf(IsSerializeError(), IsParseError())));
  EXPECT_FALSE(Value(result, AllOf(IsSerializeError(), IsParseError())));
}

// ---------------------------------------------------------------------------
// Yields()
// ---------------------------------------------------------------------------

// A testee that answers every test with the same canned response.
class FakeTestRunner : public ConformanceTestRunner {
 public:
  void RespondWith(absl::string_view response_textproto) {
    response_ = ResponseFromText(response_textproto);
  }

  std::string RunTest(absl::string_view test_name,
                      absl::string_view input) override {
    return response_.SerializeAsString();
  }

 private:
  ConformanceResponse response_;
};

// A snapshot of the TestManager counters, for concise assertions.
struct Counts {
  int expected_successes = 0;
  int expected_failures = 0;
  int unexpected_successes = 0;
  int unexpected_failures = 0;
  int skipped = 0;
  int listed_skips = 0;
  int tolerated_recommended_failures = 0;

  bool operator==(const Counts& other) const {
    return expected_successes == other.expected_successes &&
           expected_failures == other.expected_failures &&
           unexpected_successes == other.unexpected_successes &&
           unexpected_failures == other.unexpected_failures &&
           skipped == other.skipped && listed_skips == other.listed_skips &&
           tolerated_recommended_failures ==
               other.tolerated_recommended_failures;
  }

  friend void PrintTo(const Counts& counts, std::ostream* os) {
    *os << "{expected_successes: " << counts.expected_successes
        << ", expected_failures: " << counts.expected_failures
        << ", unexpected_successes: " << counts.unexpected_successes
        << ", unexpected_failures: " << counts.unexpected_failures
        << ", skipped: " << counts.skipped
        << ", listed_skips: " << counts.listed_skips
        << ", tolerated_recommended_failures: "
        << counts.tolerated_recommended_failures << "}";
  }
};

Counts GetCounts(const TestManager& manager) {
  return Counts{
      .expected_successes = manager.expected_successes(),
      .expected_failures = manager.expected_failures(),
      .unexpected_successes = manager.unexpected_successes(),
      .unexpected_failures = manager.unexpected_failures(),
      .skipped = manager.skipped(),
      .listed_skips = manager.listed_skips(),
      .tolerated_recommended_failures =
          manager.tolerated_recommended_failures(),
  };
}

// The names Run() below produces for the test named "foo".
constexpr absl::string_view kRequiredFoo =
    "Required.Proto2.ProtobufInput.foo.ProtobufOutput";
constexpr absl::string_view kRecommendedFoo =
    "Recommended.Proto2.ProtobufInput.foo.ProtobufOutput";

// Canned testee responses.
constexpr absl::string_view kPayload2 = R"pb(protobuf_payload: "\010\002")pb";
constexpr absl::string_view kParseError = R"pb(parse_error: "bad input")pb";
constexpr absl::string_view kSerializeError =
    R"pb(serialize_error: "can't serialize")pb";
constexpr absl::string_view kRuntimeError = R"pb(runtime_error: "crashed")pb";
constexpr absl::string_view kTimeoutError = R"pb(timeout_error: "too slow")pb";
constexpr absl::string_view kSkipped = R"pb(skipped: "not supported")pb";

// The legacy failure messages the canned responses lead to.
constexpr absl::string_view kMismatch1 =
    "Output was not equivalent to reference message: "
    "modified: optional_int32: 1 -> 2\n";
constexpr absl::string_view kNotAParseError =
    "Should have failed to parse, but didn't.";

class YieldsTest : public testing::Test {
 protected:
  YieldsTest() { internal::global_test_manager = &test_manager_; }
  ~YieldsTest() override {
    test_manager_.Finalize().IgnoreError();
    internal::global_test_manager = nullptr;
  }

  // Runs the binary-to-binary test `name` (of the given strictness) against a
  // testee answering `response`.  Every result this produces must be checked
  // by the test, or the result's destructor fails it.
  TestResult Run(absl::string_view response,
                 TestStrictness strictness = TestStrictness::kRequired,
                 absl::string_view name = "foo") {
    runner_.RespondWith(response);
    return testee_.CreateTest(name, strictness)
        .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
        .SerializeBinary();
  }

  // Like Run(), but asks for text format output.
  TestResult RunText(absl::string_view response,
                     TestStrictness strictness = TestStrictness::kRequired,
                     absl::string_view name = "foo") {
    runner_.RespondWith(response);
    return testee_.CreateTest(name, strictness)
        .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
        .SerializeText();
  }

  TestManager test_manager_;
  FakeTestRunner runner_;
  internal::Testee testee_{&runner_};
};

TEST_F(YieldsTest, UnlistedSuccessPasses) {
  auto matcher = Yields(ParsedPayload(EqualsTextProto("optional_int32: 2")));

  EXPECT_THAT(Explain(matcher, Run(kPayload2)), Accepts(IsEmpty()));
  EXPECT_THAT(Run(kPayload2, TestStrictness::kRequired, "other"), matcher);
  EXPECT_EQ(GetCounts(test_manager_), (Counts{.expected_successes = 2}));
  EXPECT_THAT(test_manager_.Finalize(), IsOk());
}

TEST_F(YieldsTest, UnlistedFailureFails) {
  auto matcher = Yields(ParsedPayload(EqualsTextProto("optional_int32: 1")));
  TestResult result = Run(kPayload2);

  EXPECT_THAT(
      Explain(matcher, result),
      Rejects(absl::StrCat(kMismatch1,
                           "\nUnexpected failure for test: ", kRequiredFoo)));
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(result, matcher),
                          "modified: optional_int32: 1 -> 2");
  EXPECT_EQ(GetCounts(test_manager_), (Counts{.unexpected_failures = 1}));
  // The recorded message is the legacy one, as the failure list would get it.
  EXPECT_THAT(test_manager_.UnexpectedFailures(),
              ElementsAre(FieldsAre(kRequiredFoo,
                                    "Output was not equivalent to reference "
                                    "message: modified: optional_int32: 1 -> 2",
                                    absl::nullopt)));
}

TEST_F(YieldsTest, UnlistedFailureIsCountedOnceDespiteGtestRetrying) {
  // gtest evaluates a matcher a second time to explain a failed assertion.
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(Run(kPayload2), Yields(IsParseError())),
                          "Should have failed to parse, but didn't.");
  EXPECT_EQ(GetCounts(test_manager_), (Counts{.unexpected_failures = 1}));
  EXPECT_THAT(
      test_manager_.UnexpectedFailures(),
      ElementsAre(FieldsAre(kRequiredFoo, kNotAParseError, absl::nullopt)));
}

TEST_F(YieldsTest, StaleVerdictIsNeverReplayedForAnotherResult) {
  // A long-lived matcher that checked a result which has since been destroyed
  // must not replay that verdict for a different result, even one that reuses
  // the same address: the verdict lives in the result, not in the matcher.
  auto first_matcher = Yields(IsParseError());
  {
    TestResult first = Run(kPayload2, TestStrictness::kRequired, "first");
    EXPECT_NONFATAL_FAILURE(EXPECT_THAT(first, first_matcher),
                            "Should have failed to parse, but didn't.");
  }
  TestResult second = Run(kParseError, TestStrictness::kRequired, "second");
  EXPECT_THAT(second, Yields(IsParseError()));

  EXPECT_THAT(Explain(first_matcher, second),
              Rejects(HasSubstr("was already checked")));
  EXPECT_EQ(GetCounts(test_manager_),
            (Counts{.expected_successes = 1, .unexpected_failures = 1}));
}

TEST_F(YieldsTest, PayloadBytesMismatchRecordsTheLegacyOctalMessage) {
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(Run(kPayload2), Yields(Payload(VarintField(1, 1)))),
      "Output was not equivalent to reference message: "
      "Expect: \\010\\001, but got: \\010\\002");
  EXPECT_EQ(GetCounts(test_manager_), (Counts{.unexpected_failures = 1}));
  EXPECT_THAT(
      test_manager_.UnexpectedFailures(),
      ElementsAre(FieldsAre(kRequiredFoo,
                            "Output was not equivalent to reference message: "
                            "Expect: \\010\\001, but got: \\010\\002",
                            absl::nullopt)));
}

TEST_F(YieldsTest, TextFormatOutput) {
  constexpr absl::string_view kText2 =
      R"pb(text_payload: "optional_int32: 2")pb";

  EXPECT_THAT(RunText(kText2),
              Yields(ParsedPayload(EqualsTextProto("optional_int32: 2"))));
  EXPECT_THAT(RunText(kText2, TestStrictness::kRequired, "raw"),
              Yields(Payload(std::string("optional_int32: 2"))));
  EXPECT_EQ(GetCounts(test_manager_), (Counts{.expected_successes = 2}));

  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(RunText(kText2, TestStrictness::kRequired, "mismatch"),
                  Yields(ParsedPayload(EqualsTextProto("optional_int32: 1")))),
      "modified: optional_int32: 1 -> 2");
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(RunText(R"pb(text_payload: "nonsense: 1")pb",
                          TestStrictness::kRequired, "unparseable"),
                  Yields(ParsedPayload(_))),
      "TEXT_FORMAT output we received from test was unparseable.");
  EXPECT_EQ(GetCounts(test_manager_),
            (Counts{.expected_successes = 2, .unexpected_failures = 2}));
  EXPECT_THAT(
      test_manager_.UnexpectedFailures(),
      ElementsAre(
          FieldsAre("Required.Proto2.ProtobufInput.mismatch.TextFormatOutput",
                    "Output was not equivalent to reference message: "
                    "modified: optional_int32: 1 -> 2",
                    absl::nullopt),
          FieldsAre(
              "Required.Proto2.ProtobufInput.unparseable.TextFormatOutput",
              "TEXT_FORMAT output we received from test was unparseable.",
              absl::nullopt)));
}

TEST_F(YieldsTest, ListedFailureWithMatchingMessagePasses) {
  ASSERT_THAT(test_manager_.AddExpectedFailure(kRequiredFoo, kMismatch1),
              IsOk());
  absl::ScopedMockLog log;
  EXPECT_CALL(log, Log).Times(AnyNumber());
  // (The message is logged without its trailing newline.)
  EXPECT_CALL(log,
              Log(absl::LogSeverity::kInfo, _,
                  Eq(absl::StrCat(
                      "Ignoring expected failure for test ", kRequiredFoo, ": ",
                      absl::StripTrailingAsciiWhitespace(kMismatch1)))))
      .Times(1);
  log.StartCapturingLogs();

  auto matcher = Yields(ParsedPayload(EqualsTextProto("optional_int32: 1")));
  TestResult result = Run(kPayload2);

  EXPECT_THAT(Explain(matcher, result),
              Accepts(absl::StrCat("which failed as expected: ", kMismatch1)));
  EXPECT_EQ(GetCounts(test_manager_), (Counts{.expected_failures = 1}));
  EXPECT_THAT(test_manager_.UnexpectedFailures(), IsEmpty());
  EXPECT_THAT(test_manager_.Finalize(), IsOk());
}

TEST_F(YieldsTest, ListedFailureWithMessagePrefixPasses) {
  ASSERT_THAT(test_manager_.AddExpectedFailure(
                  kRequiredFoo, "Output was not equivalent to reference"),
              IsOk());

  EXPECT_THAT(Run(kPayload2),
              Yields(ParsedPayload(EqualsTextProto("optional_int32: 1"))));
  EXPECT_EQ(GetCounts(test_manager_), (Counts{.expected_failures = 1}));
  EXPECT_THAT(test_manager_.Finalize(), IsOk());
}

TEST_F(YieldsTest, ListedFailureViaWildcardPasses) {
  ASSERT_THAT(
      test_manager_.AddExpectedFailure(
          "Required.*.ProtobufInput.foo.ProtobufOutput", kNotAParseError),
      IsOk());

  EXPECT_THAT(Run(kPayload2), Yields(IsParseError()));
  EXPECT_EQ(GetCounts(test_manager_), (Counts{.expected_failures = 1}));
  EXPECT_THAT(test_manager_.Finalize(), IsOk());
}

TEST_F(YieldsTest, ListedFailureWithDifferentMessageFails) {
  ASSERT_THAT(
      test_manager_.AddExpectedFailure(kRequiredFoo, "Some other message"),
      IsOk());
  auto matcher = Yields(IsParseError());
  TestResult result = Run(kPayload2);

  EXPECT_THAT(Explain(matcher, result),
              Rejects(absl::StrCat(
                  kNotAParseError,
                  "\nUnexpected failure message for test: ", kRequiredFoo,
                  " expected: Some other message actual: ", kNotAParseError)));
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(result, matcher),
                          "Unexpected failure message for test");
  EXPECT_EQ(GetCounts(test_manager_), (Counts{.unexpected_failures = 1}));
  EXPECT_THAT(
      test_manager_.UnexpectedFailures(),
      ElementsAre(FieldsAre(kRequiredFoo, kNotAParseError, absl::nullopt)));
}

TEST_F(YieldsTest, ListedSuccessFails) {
  ASSERT_THAT(test_manager_.AddExpectedFailure(kRequiredFoo, kNotAParseError),
              IsOk());
  auto matcher = Yields(ParsedPayload(EqualsTextProto("optional_int32: 2")));
  TestResult result = Run(kPayload2);

  // The legacy runner's wording, including the matched entry.
  EXPECT_THAT(
      Explain(matcher, result),
      Rejects(absl::StrCat("test ", kRequiredFoo, " (matched to ", kRequiredFoo,
                           ") is in the failure list, but test succeeded.  "
                           "Remove its match from the failure list.")));
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(result, matcher),
                          "is in the failure list, but test succeeded");
  EXPECT_EQ(GetCounts(test_manager_), (Counts{.unexpected_successes = 1}));
  EXPECT_THAT(test_manager_.UnexpectedSuccesses(),
              ElementsAre(FieldsAre(kRequiredFoo, kNotAParseError,
                                    testing::Optional(kRequiredFoo))));
}

TEST_F(YieldsTest, ListedSuccessViaWildcardNamesTheWildcard) {
  ASSERT_THAT(test_manager_.AddExpectedFailure(
                  "Required.*.ProtobufInput.foo.ProtobufOutput", ""),
              IsOk());

  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(Run(kParseError), Yields(IsParseError())),
      "test Required.Proto2.ProtobufInput.foo.ProtobufOutput (matched to "
      "Required.*.ProtobufInput.foo.ProtobufOutput) is in the failure list, "
      "but test succeeded.  Remove its match from the failure list.");
  EXPECT_EQ(GetCounts(test_manager_), (Counts{.unexpected_successes = 1}));
}

TEST_F(YieldsTest, RecommendedFailureIsToleratedWhenNotEnforced) {
  test_manager_.set_enforce_recommended(false);
  absl::ScopedMockLog log;
  EXPECT_CALL(log, Log).Times(AnyNumber());
  EXPECT_CALL(log, Log(absl::LogSeverity::kWarning, _,
                       Eq(absl::StrCat("WARNING, test=", kRecommendedFoo, ": ",
                                       kNotAParseError))))
      .Times(1);
  log.StartCapturingLogs();

  auto matcher = Yields(IsParseError());
  TestResult result = Run(kPayload2, TestStrictness::kRecommended);

  EXPECT_THAT(Explain(matcher, result),
              Accepts(absl::StrCat("which failed, but is only recommended: ",
                                   kNotAParseError)));
  // It's neither a skip nor a failure.
  EXPECT_EQ(GetCounts(test_manager_),
            (Counts{.tolerated_recommended_failures = 1}));
  EXPECT_THAT(test_manager_.UnexpectedFailures(), IsEmpty());
  EXPECT_THAT(test_manager_.Finalize(), IsOk());
}

TEST_F(YieldsTest, RecommendedFailureFailsWhenEnforced) {
  test_manager_.set_enforce_recommended(true);

  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(Run(kPayload2, TestStrictness::kRecommended),
                  Yields(IsParseError())),
      "Should have failed to parse, but didn't.");
  EXPECT_EQ(GetCounts(test_manager_), (Counts{.unexpected_failures = 1}));
  EXPECT_THAT(
      test_manager_.UnexpectedFailures(),
      ElementsAre(FieldsAre(kRecommendedFoo, kNotAParseError, absl::nullopt)));
}

TEST_F(YieldsTest, ListedRecommendedFailureIsTrackedEvenWhenNotEnforced) {
  // A recommended test that's already in the failure list keeps being tracked
  // there, so that the list can't go stale unnoticed.
  test_manager_.set_enforce_recommended(false);
  ASSERT_THAT(
      test_manager_.AddExpectedFailure(kRecommendedFoo, kNotAParseError),
      IsOk());

  EXPECT_THAT(Run(kPayload2, TestStrictness::kRecommended),
              Yields(IsParseError()));
  EXPECT_EQ(GetCounts(test_manager_), (Counts{.expected_failures = 1}));
  EXPECT_THAT(test_manager_.Finalize(), IsOk());
}

TEST_F(YieldsTest,
       ListedRecommendedFailureWithDifferentMessageFailsEvenWhenNotEnforced) {
  test_manager_.set_enforce_recommended(false);
  ASSERT_THAT(
      test_manager_.AddExpectedFailure(kRecommendedFoo, "Some other message"),
      IsOk());

  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(Run(kPayload2, TestStrictness::kRecommended),
                  Yields(IsParseError())),
      "Unexpected failure message for test");
  EXPECT_EQ(GetCounts(test_manager_), (Counts{.unexpected_failures = 1}));
}

TEST_F(YieldsTest, RecommendedSuccessPasses) {
  test_manager_.set_enforce_recommended(false);

  EXPECT_THAT(Run(kParseError, TestStrictness::kRecommended),
              Yields(IsParseError()));
  EXPECT_EQ(GetCounts(test_manager_), (Counts{.expected_successes = 1}));
}

TEST_F(YieldsTest, UnlistedSkipPasses) {
  absl::ScopedMockLog log;
  EXPECT_CALL(log, Log).Times(AnyNumber());
  EXPECT_CALL(
      log,
      Log(absl::LogSeverity::kInfo, _,
          Eq(absl::StrCat("Skipping test ", kRequiredFoo, ": not supported"))))
      .Times(1);
  log.StartCapturingLogs();

  // The inner matcher never gets to see a skipped result.
  bool inner_called = false;
  auto matcher = Yields(Truly([&](const TestResult&) {
    inner_called = true;
    return false;
  }));
  TestResult result = Run(kSkipped);

  EXPECT_THAT(Explain(matcher, result),
              Accepts("which was skipped by the testee: not supported"));
  EXPECT_FALSE(inner_called);
  EXPECT_EQ(GetCounts(test_manager_), (Counts{.skipped = 1}));
  EXPECT_THAT(test_manager_.Finalize(), IsOk());
}

TEST_F(YieldsTest, NotSelectedByRunnerPassesSilently) {
  // A filtering runner answers tests that weren't selected with this exact
  // skip reason; that isn't a testee skip and must not be logged or counted.
  ASSERT_THAT(test_manager_.AddExpectedFailure(kRequiredFoo, ""), IsOk());
  absl::ScopedMockLog log;
  EXPECT_CALL(log, Log(_, _, HasSubstr(kRequiredFoo))).Times(0);
  log.StartCapturingLogs();

  bool inner_called = false;
  auto matcher = Yields(Truly([&](const TestResult&) {
    inner_called = true;
    return false;
  }));
  const std::string response =
      absl::StrCat("skipped: \"", kTestNotSelectedSkipReason, "\"");
  TestResult result = Run(response);
  ASSERT_EQ(result.response().skipped(), "Not selected by --test.");

  EXPECT_THAT(Explain(matcher, result),
              Accepts("which was not selected to run"));
  EXPECT_FALSE(inner_called);
  EXPECT_EQ(GetCounts(test_manager_), Counts{});
  // Even a listed test is simply unseen, not "listed but skipped".  Its entry
  // does count as matched, though: the test exists, it just didn't run.
  EXPECT_THAT(test_manager_.UnseenExpectedFailures(),
              ElementsAre(kRequiredFoo));
  EXPECT_THAT(test_manager_.UnmatchedExpectedFailures(), IsEmpty());

  // An unlisted one likewise.
  EXPECT_THAT(Run(response, TestStrictness::kRequired, "other"), matcher);
  EXPECT_EQ(GetCounts(test_manager_), Counts{});
}

// Writes `content` to a fresh file under the test's temporary directory and
// returns its path.
std::string WriteTempFile(absl::string_view name, absl::string_view content) {
  const std::string path = absl::StrCat(
      testing::TempDir(), "/",
      testing::UnitTest::GetInstance()->current_test_info()->name(), ".", name);
  std::ofstream file(path);
  file << content;
  file.close();
  ABSL_CHECK(file.good()) << path;
  return path;
}

std::string ReadFile(absl::string_view path) {
  std::ifstream file{std::string(path)};
  ABSL_CHECK(file.is_open()) << path;
  return std::string(std::istreambuf_iterator<char>(file), {});
}

TEST_F(YieldsTest, ListedSkipFailsOnce) {
  // Deliberately stricter than the legacy runner, which passed this case: a
  // listed test the testee skips fails, but only here.  The entry counts as
  // seen (a skip says nothing about whether it is still needed), so Finalize()
  // doesn't report it a second time and --fix keeps it.
  ASSERT_THAT(test_manager_.LoadFailureList(WriteTempFile(
                  "failure_list.txt", absl::StrCat(kRequiredFoo, " # abc\n"))),
              IsOk());
  auto matcher = Yields(IsParseError());
  TestResult result = Run(kSkipped);

  // The failure names the matched entry and the remedy, like an unexpected
  // success does.
  EXPECT_THAT(Explain(matcher, result),
              Rejects(absl::StrCat(
                  "test ", kRequiredFoo, " (matched to ", kRequiredFoo,
                  ") is in the failure list but was skipped by the testee: "
                  "not supported.  Remove its match from the failure list.")));
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(result, matcher),
                          "is in the failure list but was skipped");
  // The manager records it as a listed skip, not as an unexpected failure.
  EXPECT_EQ(GetCounts(test_manager_),
            (Counts{.skipped = 1, .listed_skips = 1}));
  EXPECT_THAT(test_manager_.ListedSkips(),
              ElementsAre(Pair(kRequiredFoo, kRequiredFoo)));
  EXPECT_THAT(test_manager_.UnexpectedFailures(), IsEmpty());
  EXPECT_THAT(test_manager_.UnseenExpectedFailures(), IsEmpty());
  EXPECT_THAT(test_manager_.Finalize(), IsOk());

  const std::string fixed = WriteTempFile("fixed.txt", "");
  ASSERT_THAT(test_manager_.SaveFailureList(fixed), IsOk());
  EXPECT_EQ(ReadFile(fixed), absl::StrCat(kRequiredFoo, " # abc\n"));
}

TEST_F(YieldsTest, RuntimeErrorIsAlwaysAFailure) {
  // Even if the inner matcher would accept it.
  auto matcher = Yields(Not(IsParseError()));
  TestResult result = Run(kRuntimeError);

  EXPECT_THAT(
      Explain(matcher, result),
      Rejects(absl::StrCat("Failed to parse input or produce output.",
                           "\nUnexpected failure for test: ", kRequiredFoo)));
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(result, matcher),
                          "Failed to parse input or produce output.");
  EXPECT_EQ(GetCounts(test_manager_), (Counts{.unexpected_failures = 1}));
  EXPECT_THAT(test_manager_.UnexpectedFailures(),
              ElementsAre(FieldsAre(kRequiredFoo,
                                    "Failed to parse input or produce output.",
                                    absl::nullopt)));
}

TEST_F(YieldsTest, RuntimeErrorKeepsTheInnerMatchersFailureMessage) {
  // The legacy binary runner reports a runtime error where a parse error was
  // expected with a dedicated message, which the failure lists rely on.
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(Run(kRuntimeError), Yields(IsParseError())),
      "Should have failed to parse, but raised an error instead.");
  EXPECT_THAT(test_manager_.UnexpectedFailures(),
              ElementsAre(FieldsAre(
                  kRequiredFoo,
                  "Should have failed to parse, but raised an error instead.",
                  absl::nullopt)));
}

TEST_F(YieldsTest, ListedRuntimeErrorIsAnExpectedFailure) {
  ASSERT_THAT(test_manager_.AddExpectedFailure(
                  kRequiredFoo,
                  "Should have failed to parse, but raised an error instead."),
              IsOk());

  EXPECT_THAT(Run(kRuntimeError), Yields(IsParseError()));
  EXPECT_EQ(GetCounts(test_manager_), (Counts{.expected_failures = 1}));
  EXPECT_THAT(test_manager_.Finalize(), IsOk());
}

TEST_F(YieldsTest, TimeoutErrorIsAlwaysAFailure) {
  auto matcher = Yields(_);
  TestResult result = Run(kTimeoutError);

  EXPECT_THAT(
      Explain(matcher, result),
      Rejects(absl::StrCat("Failed to parse input or produce output.",
                           "\nUnexpected failure for test: ", kRequiredFoo)));
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(result, matcher),
                          "Failed to parse input or produce output.");
  EXPECT_EQ(GetCounts(test_manager_), (Counts{.unexpected_failures = 1}));
}

TEST_F(YieldsTest, EmptyResponseIsAlwaysAFailure) {
  auto matcher = Yields(_);
  TestResult result = Run("");

  EXPECT_THAT(
      Explain(matcher, result),
      Rejects(absl::StrCat("Response didn't have any field in the Response.",
                           "\nUnexpected failure for test: ", kRequiredFoo)));
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(result, matcher),
                          "Response didn't have any field in the Response.");
  EXPECT_EQ(GetCounts(test_manager_), (Counts{.unexpected_failures = 1}));
}

TEST_F(YieldsTest, AnyOfMatchesEitherLeg) {
  auto matcher = Yields(AnyOf(
      IsParseError(), ParsedPayload(EqualsBinaryProto(VarintField(1, 2)))));

  EXPECT_THAT(Run(kParseError, TestStrictness::kRequired, "first"), matcher);
  EXPECT_THAT(Run(kPayload2, TestStrictness::kRequired, "second"), matcher);
  EXPECT_EQ(GetCounts(test_manager_), (Counts{.expected_successes = 2}));
}

TEST_F(YieldsTest, AnyOfFailsWhenNoLegMatches) {
  auto matcher = Yields(AnyOf(
      IsParseError(), ParsedPayload(EqualsBinaryProto(VarintField(1, 2)))));
  TestResult result = Run(kSerializeError);

  EXPECT_THAT(
      Explain(matcher, result),
      Rejects(AllOf(HasSubstr("Should have failed to parse, but didn't."),
                    HasSubstr("Failed to parse input or produce output."),
                    HasSubstr(absl::StrCat("Unexpected failure for test: ",
                                           kRequiredFoo)))));
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(result, matcher), "Unexpected failure");
  EXPECT_EQ(GetCounts(test_manager_), (Counts{.unexpected_failures = 1}));
}

TEST_F(YieldsTest, NotComposes) {
  EXPECT_THAT(Run(kPayload2, TestStrictness::kRequired, "first"),
              Yields(Not(IsParseError())));
  EXPECT_EQ(GetCounts(test_manager_), (Counts{.expected_successes = 1}));

  auto matcher = Yields(Not(IsParseError()));
  TestResult result = Run(kParseError, TestStrictness::kRequired, "second");
  // Not() has no explanation of its own, so the description is used.
  EXPECT_THAT(Explain(matcher, result),
              Rejects(absl::StrCat(
                  "which doesn't match (is not a parse error)",
                  "\nUnexpected failure for test: ",
                  "Required.Proto2.ProtobufInput.second.ProtobufOutput")));
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(result, matcher),
                          "which doesn't match (is not a parse error)");
  EXPECT_EQ(GetCounts(test_manager_),
            (Counts{.expected_successes = 1, .unexpected_failures = 1}));
}

TEST_F(YieldsTest, MarksTheResultChecked) {
  TestResult result = Run(kParseError);
  EXPECT_FALSE(result.checked());
  EXPECT_FALSE(result.verdict().has_value());
  EXPECT_THAT(result, Yields(IsParseError()));
  EXPECT_TRUE(result.checked());
  ASSERT_TRUE(result.verdict().has_value());
  EXPECT_TRUE(result.verdict()->matched);
}

TEST_F(YieldsTest, CheckingAPassedResultAgainFails) {
  // Whatever the second matcher is, including the very same object.
  auto matcher = Yields(IsParseError());
  TestResult result = Run(kParseError);
  EXPECT_THAT(result, matcher);

  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(result, matcher),
      "TestResult for Required.Proto2.ProtobufInput.foo.ProtobufOutput was "
      "already checked; each result may be checked exactly once");
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(result, Yields(IsParseError())),
                          "was already checked");
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(result, Yields(Not(IsParseError()))),
                          "was already checked");
  EXPECT_EQ(GetCounts(test_manager_), (Counts{.expected_successes = 1}));
}

TEST_F(YieldsTest, CheckingAFailedResultAgainReplaysTheFailure) {
  // gtest re-evaluates a failing matcher to explain the failure, so a failed
  // verdict is replayed from the result, and the manager hears about the test
  // only once.  The replay doesn't depend on the matcher: a stale verdict
  // can't be confused with another result's, because it lives in the result.
  TestResult result = Run(kPayload2);
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(result, Yields(IsParseError())),
                          "Should have failed to parse, but didn't.");
  ASSERT_TRUE(result.verdict().has_value());
  EXPECT_FALSE(result.verdict()->matched);

  EXPECT_THAT(
      Explain(Yields(IsParseError()), result),
      Rejects(absl::StrCat(kNotAParseError,
                           "\nUnexpected failure for test: ", kRequiredFoo)));
  EXPECT_THAT(Explain(Yields(ParsedPayload(_)), result),
              Rejects(HasSubstr(kNotAParseError)));
  EXPECT_EQ(GetCounts(test_manager_), (Counts{.unexpected_failures = 1}));
  EXPECT_THAT(
      test_manager_.UnexpectedFailures(),
      ElementsAre(FieldsAre(kRequiredFoo, kNotAParseError, absl::nullopt)));
}

TEST_F(YieldsTest, ResultMarkedCheckedByHandHasNoVerdictToReplay) {
  TestResult result = Run(kParseError);
  result.MarkChecked();

  EXPECT_THAT(Explain(Yields(IsParseError()), result),
              Rejects(HasSubstr("was already checked")));
  EXPECT_EQ(GetCounts(test_manager_), Counts{});
}

TEST_F(YieldsTest, VerdictMovesWithTheResult) {
  TestResult original = Run(kPayload2);
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(original, Yields(IsParseError())),
                          "Should have failed to parse, but didn't.");

  // Whatever address `original` had, the moved-to result carries the verdict.
  TestResult moved = std::move(original);
  EXPECT_THAT(Explain(Yields(IsParseError()), moved),
              Rejects(HasSubstr(kNotAParseError)));
  EXPECT_EQ(GetCounts(test_manager_), (Counts{.unexpected_failures = 1}));
}

TEST_F(YieldsTest, SameMatcherCanCheckSeveralResults) {
  auto matcher = Yields(IsParseError());
  EXPECT_THAT(Run(kParseError, TestStrictness::kRequired, "first"), matcher);
  EXPECT_THAT(Run(kParseError, TestStrictness::kRequired, "second"), matcher);
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(Run(kPayload2, TestStrictness::kRequired, "third"), matcher),
      "Should have failed to parse, but didn't.");
  EXPECT_EQ(GetCounts(test_manager_),
            (Counts{.expected_successes = 2, .unexpected_failures = 1}));
}

TEST_F(YieldsTest, NeverCheckedResultFails) {
  EXPECT_NONFATAL_FAILURE(
      { TestResult result = Run(kParseError); },
      "TestResult for Required.Proto2.ProtobufInput.foo.ProtobufOutput was "
      "never checked; wrap the matcher in Yields()");
  // Nothing was reported.
  EXPECT_EQ(GetCounts(test_manager_), Counts{});
}

TEST_F(YieldsTest, BareLeafMatcherIsNotEnough) {
  // A leaf that happens to pass on its own still isn't a checked result.
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(Run(kParseError), IsParseError()),
                          "was never checked; wrap the matcher in Yields()");
  EXPECT_EQ(GetCounts(test_manager_), Counts{});
}

TEST_F(YieldsTest, MovedFromResultReportsNothing) {
  TestResult original = Run(kParseError);
  TestResult moved = std::move(original);
  EXPECT_THAT(moved, Yields(IsParseError()));
  // `original` is inert and silent; `moved` was checked.
  EXPECT_EQ(GetCounts(test_manager_), (Counts{.expected_successes = 1}));
}

// ---------------------------------------------------------------------------
// JsonPayload()
// ---------------------------------------------------------------------------

// A typical validator: the JSON object has a member called `name`.  Like most
// hand-written matchers it has a description but no explanation of its own.
MATCHER_P(HasJsonMember, name,
          absl::StrCat(negation ? "doesn't have" : "has", " member \"", name,
                       "\"")) {
  return arg.isMember(name);
}

// A JSON-input, JSON-output result carrying the given JSON payload.
TestResult JsonResult(absl::string_view json) {
  ConformanceResponse response;
  response.set_json_payload(json);
  return CreateResult("foo", JsonInputRequest(::conformance::JSON),
                      std::move(response));
}

constexpr absl::string_view kJsonInt32 = R"({"optionalInt32": 9})";

TEST(JsonPayloadTest, Description) {
  EXPECT_EQ(Describe(JsonPayload(HasJsonMember("optionalInt32"))),
            "JSON payload has member \"optionalInt32\"");
  EXPECT_EQ(DescribeNegation(JsonPayload(HasJsonMember("optionalInt32"))),
            "JSON payload doesn't have member \"optionalInt32\"");
  EXPECT_EQ(Describe(JsonPayload(_)), "JSON payload is anything");
}

TEST(JsonPayloadTest, Matches) {
  TestResult result = JsonResult(kJsonInt32);

  EXPECT_THAT(Explain(JsonPayload(HasJsonMember("optionalInt32")), result),
              Accepts(IsEmpty()));
  EXPECT_TRUE(Value(result, JsonPayload(_)));
  EXPECT_TRUE(Value(result, JsonPayload(Truly([](const Json::Value& value) {
                      return value["optionalInt32"].asInt() == 9;
                    }))));
  EXPECT_TRUE(Value(result, JsonPayload(testing::SizeIs(1))));
}

TEST(JsonPayloadTest, InnerMatcherWithoutExplanation) {
  // The description and the actual JSON text, quoted and escaped like
  // Payload() does, are reported instead.
  TestResult result = JsonResult(kJsonInt32);

  EXPECT_THAT(Explain(JsonPayload(HasJsonMember("optionalInt64")), result),
              Rejects("Expect: JSON payload has member \"optionalInt64\", "
                      "but got: \"{\\\"optionalInt32\\\": 9}\""));
}

TEST(JsonPayloadTest, InnerMatcherExplanationIsUsed) {
  TestResult result = JsonResult(kJsonInt32);

  EXPECT_THAT(Explain(JsonPayload(testing::SizeIs(2)), result),
              Rejects("whose size 1 doesn't match"));
  // Truly() explains itself, tersely; this is what a lambda validator ported
  // from the legacy suite records in the failure list.  Only the gist is
  // pinned, since the exact wording belongs to gMock.
  EXPECT_THAT(Explain(JsonPayload(Truly([](const Json::Value& value) {
                        return value.isNull();
                      })),
                      result),
              Rejects(HasSubstr("didn't satisfy")));
}

TEST(JsonPayloadTest, InvalidJson) {
  // The legacy validator's message, followed by jsoncpp's own diagnostic.
  // The payload has to be valid JSON before any inner matcher gets a say.
  TestResult result = JsonResult("{");

  EXPECT_THAT(Explain(JsonPayload(_), result),
              Rejects(testing::StartsWith(
                  "JSON payload cannot be parsed as valid JSON: ")));
  EXPECT_THAT(Explain(JsonPayload(_), JsonResult("")),
              Rejects(testing::StartsWith(
                  "JSON payload cannot be parsed as valid JSON: ")));
  EXPECT_THAT(Explain(JsonPayload(_), JsonResult("nonsense")),
              Rejects(testing::StartsWith(
                  "JSON payload cannot be parsed as valid JSON: ")));
}

TEST(JsonPayloadTest, ParsesAnyJsonValueNotJustObjects) {
  // Scalars and arrays are valid JSON documents to jsoncpp's reader (like the
  // legacy validator); whether they are acceptable is up to the inner
  // matcher.
  EXPECT_TRUE(
      Value(JsonResult("null"), JsonPayload(Truly([](const Json::Value& value) {
              return value.isNull();
            }))));
  EXPECT_TRUE(Value(JsonResult("[1, 2]"), JsonPayload(testing::SizeIs(2))));
}

TEST(JsonPayloadTest, EmptyResponse) {
  TestResult result =
      CreateResult("foo", ::conformance::JSON, ConformanceResponse());

  EXPECT_THAT(Explain(JsonPayload(_), result),
              Rejects("Response didn't have any field in the Response."));
}

TEST_P(PayloadMatcherErrorResponseTest, IsAFailureForJsonPayload) {
  TestResult result = CreateResult("foo", ::conformance::JSON,
                                   ResponseFromText(GetParam().response));

  EXPECT_THAT(Explain(JsonPayload(_), result),
              Rejects("Failed to parse input or produce output."));
}

TEST(JsonPayloadTest, Skipped) {
  TestResult result =
      CreateResult("foo", ::conformance::JSON,
                   ResponseFromText(R"pb(skipped: "skipped message")pb"));

  EXPECT_THAT(Explain(JsonPayload(_), result),
              Rejects("the testee skipped the test: skipped message"));
}

TEST(JsonPayloadTest, WrongOutputFormat) {
  // Same handling as the other payload matchers.
  TestResult result = CreateResult("foo", ::conformance::JSON,
                                   ProtobufPayload(VarintField(1, 9)));

  EXPECT_THAT(
      Explain(JsonPayload(_), result),
      Rejects("Test was asked for JSON output but provided PROTOBUF instead."));
  EXPECT_THAT(
      Explain(JsonPayload(_),
              CreateResult("bar", ::conformance::JSON,
                           ResponseFromText(R"pb(text_payload: "")pb"))),
      Rejects(
          "Test was asked for JSON output but provided TEXT_FORMAT instead."));
}

TEST(JsonPayloadTest, RequiresATestThatAskedForJsonOutput) {
  // A test that asked for (and got) some other format can't be matched by
  // JsonPayload() at all; that is a bug in the test.
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(VarintField(1, 9)));

  EXPECT_THAT(Explain(JsonPayload(_), result),
              Rejects("JsonPayload() needs JSON output, but the test asked "
                      "for PROTOBUF output."));
}

TEST(JsonPayloadTest, ComposesWithGMock) {
  TestResult result = JsonResult(kJsonInt32);

  EXPECT_TRUE(Value(result, Not(JsonPayload(HasJsonMember("optionalInt64")))));
  EXPECT_TRUE(Value(
      result,
      AnyOf(IsParseError(), JsonPayload(HasJsonMember("optionalInt32")))));
  EXPECT_TRUE(Value(
      result, AllOf(JsonPayload(HasJsonMember("optionalInt32")),
                    ParsedPayload(EqualsTextProto("optional_int32: 9")))));
}

TEST_F(YieldsTest, JsonOutput) {
  // JSON-input, JSON-output tests through the whole stack: the legacy
  // messages are what gets recorded for the failure list.
  auto run_json = [&](absl::string_view response, absl::string_view name) {
    runner_.RespondWith(response);
    return testee_.CreateTest(name, TestStrictness::kRequired)
        .ParseJson(TestAllTypesProto2::descriptor(), "{}")
        .SerializeJson();
  };
  constexpr absl::string_view kJsonResponse =
      R"pb(json_payload: "{\"optionalInt32\": 9}")pb";

  EXPECT_THAT(run_json(kJsonResponse, "parsed"),
              Yields(ParsedPayload(EqualsTextProto("optional_int32: 9"))));
  EXPECT_THAT(run_json(kJsonResponse, "validated"),
              Yields(JsonPayload(HasJsonMember("optionalInt32"))));
  EXPECT_EQ(GetCounts(test_manager_), (Counts{.expected_successes = 2}));

  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(
          run_json(R"pb(json_payload: "{\"nonsense\": 1}")pb", "unparseable"),
          Yields(ParsedPayload(_))),
      "JSON output we received from test was unparseable.");
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(run_json(R"pb(json_payload: "{")pb", "invalid"),
                  Yields(JsonPayload(_))),
      "JSON payload cannot be parsed as valid JSON: ");
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(run_json(kJsonResponse, "mismatch"),
                  Yields(JsonPayload(HasJsonMember("optionalInt64")))),
      "Expect: JSON payload has member \"optionalInt64\", but got: "
      "\"{\\\"optionalInt32\\\": 9}\"");
  EXPECT_EQ(GetCounts(test_manager_),
            (Counts{.expected_successes = 2, .unexpected_failures = 3}));
  EXPECT_THAT(
      test_manager_.UnexpectedFailures(),
      ElementsAre(
          FieldsAre("Required.Proto2.JsonInput.invalid.JsonOutput",
                    testing::StartsWith(
                        "JSON payload cannot be parsed as valid JSON: "),
                    absl::nullopt),
          FieldsAre("Required.Proto2.JsonInput.mismatch.JsonOutput",
                    "Expect: JSON payload has member \"optionalInt64\", but "
                    "got: \"{\\\"optionalInt32\\\": 9}\"",
                    absl::nullopt),
          FieldsAre("Required.Proto2.JsonInput.unparseable.JsonOutput",
                    "JSON output we received from test was unparseable.",
                    absl::nullopt)));
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
