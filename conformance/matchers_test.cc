// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/matchers.h"

#include <cmath>
#include <string>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "conformance/binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "conformance/mock_test_runner.h"
#include "conformance/testee.h"
#include "google/protobuf/message.h"
#include "google/protobuf/test_messages_proto2.pb.h"
#include "google/protobuf/text_format.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::conformance::ConformanceResponse;
using ::conformance::WireFormat;
using ::google::protobuf::conformance::TestPriority;
using ::google::protobuf::conformance::internal::TestResult;
using ::protobuf_test_messages::proto2::TestAllTypesProto2;
using ::testing::_;
using ::testing::AllOf;
using ::testing::AnyOf;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Not;
using ::testing::PrintToString;
using ::testing::Return;
using ::testing::TestParamInfo;
using ::testing::Value;
using ::testing::Values;

// Creates a result for the leaf matcher tests by running a kP0 test with the
// given input format and requested output format (and, for text output,
// whether unknown fields are to be printed) against a testee that answers
// `response`.  The matchers never look at the input payload itself, only at
// its format, so the input is empty.  These tests only inspect the result, so
// it is marked checked right away.
TestResult CreateResult(absl::string_view test_name, WireFormat input_format,
                        WireFormat output_format, ConformanceResponse response,
                        bool print_unknown_fields = false) {
  NiceMock<MockTestRunner> runner;
  ON_CALL(runner, RunTest).WillByDefault(Return(response.SerializeAsString()));
  internal::Testee testee(&runner);
  internal::Test test = testee.CreateTest(test_name, TestPriority::kP0);
  internal::InMemoryMessage message = [&] {
    switch (input_format) {
      case ::conformance::TEXT_FORMAT:
        return std::move(test).ParseText(TestAllTypesProto2::descriptor(), "");
      case ::conformance::JSON:
        return std::move(test).ParseJson(TestAllTypesProto2::descriptor(), "");
      default:
        return std::move(test).ParseBinary(TestAllTypesProto2::descriptor(),
                                           Wire());
    }
  }();
  TestResult result = [&] {
    switch (output_format) {
      case ::conformance::TEXT_FORMAT:
        return std::move(message).SerializeText({print_unknown_fields});
      case ::conformance::JSON:
        return std::move(message).SerializeJson();
      default:
        return std::move(message).SerializeBinary();
    }
  }();
  result.MarkChecked();
  return result;
}

// Most tests only care about the requested output format; they use binary
// input.
TestResult CreateResult(absl::string_view test_name, WireFormat output_format,
                        ConformanceResponse response) {
  return CreateResult(test_name, ::conformance::PROTOBUF, output_format,
                      std::move(response));
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
// PrintTo(TestResult)
// ---------------------------------------------------------------------------

// Runs the binary-to-binary test "foo" of the given priority against a testee
// that answers `response_textproto`.  The result is only printed, so it is
// marked checked right away.
TestResult ResultWithResponse(absl::string_view response_textproto,
                              TestPriority priority = TestPriority::kP0) {
  NiceMock<MockTestRunner> runner;
  ON_CALL(runner, RunTest)
      .WillByDefault(
          Return(ResponseFromText(response_textproto).SerializeAsString()));
  internal::Testee testee(&runner);
  TestResult result =
      testee.CreateTest("foo", priority)
          .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
          .SerializeBinary();
  result.MarkChecked();
  return result;
}

constexpr absl::string_view kPrintedRequiredTest =
    R"(Required test "Required.Proto2.ProtobufInput.foo.ProtobufOutput")";

TEST(PrintTestResultTest, ParseError) {
  EXPECT_EQ(PrintToString(
                ResultWithResponse(R"pb(parse_error: "failed to parse")pb")),
            absl::StrCat(kPrintedRequiredTest,
                         R"( with response {parse_error: "failed to parse"})"));
}

TEST(PrintTestResultTest, Skipped) {
  EXPECT_EQ(
      PrintToString(ResultWithResponse(R"pb(skipped: "skipped message")pb",
                                       TestPriority::kP1)),
      R"(Recommended test "Recommended.Proto2.ProtobufInput.foo.ProtobufOutput" )"
      R"(with response {skipped: "skipped message"})");
}

TEST(PrintTestResultTest, EmptyResponse) {
  EXPECT_EQ(PrintToString(ResultWithResponse("")),
            absl::StrCat(kPrintedRequiredTest, " with response {}"));
}

TEST(PrintTestResultTest, ProtobufPayloadIsDecoded) {
  EXPECT_EQ(
      PrintToString(ResultWithResponse(R"pb(protobuf_payload: "\010\t")pb")),
      absl::StrCat(kPrintedRequiredTest,
                   R"( with response {protobuf_payload: "\010\t"} )"
                   R"((decoded: {optional_int32: 9}))"));
}

TEST(PrintTestResultTest, UnparseableProtobufPayload) {
  EXPECT_EQ(
      PrintToString(ResultWithResponse(R"pb(protobuf_payload: "\001")pb")),
      absl::StrCat(
          kPrintedRequiredTest,
          R"( with response {protobuf_payload: "\001"} (unparseable))"));
}

TEST(PrintTestResultTest, LargePayloadsAreTruncated) {
  std::string large_payload(300, 'a');

  EXPECT_EQ(
      PrintToString(ResultWithResponse(
          absl::StrCat("text_payload: \"", large_payload, "\""))),
      absl::StrCat(kPrintedRequiredTest, " with response {text_payload: \"",
                   std::string(200, 'a'), "...(truncated)\"}"));
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
  EXPECT_EQ(Describe(Payload(Wire("foo"))), "payload is equal to \"foo\"");
  EXPECT_EQ(DescribeNegation(Payload(Wire("foo"))),
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
                EqualsBinaryProto(Wire("\010\t")), /*negation=*/true),
            "doesn't equal binary proto \"\\010\\t\"");
}

TEST(MatcherDescriptionTest, FailureMatchers) {
  EXPECT_EQ(Describe(IsParseError()), "is a parse error");
  EXPECT_EQ(DescribeNegation(IsParseError()), "is not a parse error");
  EXPECT_EQ(Describe(IsSerializeError()), "is a serialize error");
  EXPECT_EQ(DescribeNegation(IsSerializeError()), "is not a serialize error");
}

// ---------------------------------------------------------------------------
// Response handling shared by ParsedPayload() and Payload()
// ---------------------------------------------------------------------------

// These responses fail before any payload is compared, so the expected payload
// is irrelevant.

TEST(PayloadMatcherTest, EmptyResponse) {
  TestResult result =
      CreateResult("foo", ::conformance::PROTOBUF, ConformanceResponse());

  EXPECT_THAT(Explain(ParsedPayload(_), result),
              Rejects("Response didn't have any field in the Response."));
  EXPECT_THAT(Explain(Payload(Wire()), result),
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

  EXPECT_THAT(Explain(Payload(Wire()), result),
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
  EXPECT_THAT(Explain(Payload(Wire()), result),
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
      Explain(Payload(Wire()), result),
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
      Explain(Payload(Wire()), result),
      Rejects("Test was asked for JSON output but provided JSPB instead."));
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

TEST(ParsedPayloadTest, JsonOutputIsNotSupported) {
  TestResult result =
      CreateResult("foo", ::conformance::JSON,
                   ResponseFromText(R"pb(json_payload: "{}")pb"));

  EXPECT_THAT(
      Explain(ParsedPayload(_), result),
      Rejects("ParsedPayload is not supported for JSON output; use "
              "Payload() to match the raw JSON text until JSON matching is "
              "migrated to gtest (b/410122158)."));
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
  TestResult result =
      CreateResult("foo", /*input_format=*/::conformance::PROTOBUF,
                   /*output_format=*/::conformance::TEXT_FORMAT,
                   ResponseFromText(R"pb(text_payload: "1: 9")pb"),
                   /*print_unknown_fields=*/true);

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
// Payload()
// ---------------------------------------------------------------------------

TEST(RawPayloadTest, MatchesBytes) {
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(VarintField(1, 2)));

  EXPECT_TRUE(Value(result, Payload(VarintField(1, 2))));
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

TEST(RawPayloadTest, UnparseableProtobufOutputFailsEvenForTheExactBytes) {
  // Like the legacy runner's require_same_wire_format, binary output must
  // decode as the test's message type.
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(Wire("\001")));

  EXPECT_THAT(
      Explain(Payload(Wire("\001")), result),
      Rejects("Protobuf output we received from test was unparseable."));
}

TEST(RawPayloadTest, TextOutput) {
  TestResult result = CreateResult(
      "foo", ::conformance::TEXT_FORMAT,
      ResponseFromText(R"pb(text_payload: "optional_int32: 9")pb"));

  EXPECT_TRUE(Value(result, Payload(Wire("optional_int32: 9"))));
  // Text output isn't required to be parseable.
  EXPECT_TRUE(Value(
      CreateResult("bar", ::conformance::TEXT_FORMAT,
                   ResponseFromText(R"pb(text_payload: "nonsense: 1")pb")),
      Payload(Wire("nonsense: 1"))));
}

TEST(RawPayloadTest, JsonOutput) {
  // Raw payloads don't need to be decoded, so JSON works already.
  TestResult result =
      CreateResult("foo", ::conformance::JSON,
                   ResponseFromText(R"pb(json_payload: "{}")pb"));

  EXPECT_TRUE(Value(result, Payload(Wire("{}"))));
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
  EXPECT_THAT(message, EqualsBinaryProto(Wire("\010\t")));
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

TEST(FailureMatcherTest, ParseErrorRuntimeError) {
  TestResult result =
      CreateResult("foo", /*input_format=*/::conformance::PROTOBUF,
                   /*output_format=*/::conformance::PROTOBUF,
                   ResponseFromText(R"pb(runtime_error: "x")pb"));

  EXPECT_THAT(
      Explain(IsParseError(), result),
      Rejects("Should have failed to parse, but raised an error instead."));
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
  TestResult result =
      CreateResult("foo", ::conformance::PROTOBUF,
                   ResponseFromText(R"pb(runtime_error: "x")pb"));

  EXPECT_THAT(
      Explain(IsSerializeError(), result),
      Rejects("Should have failed to serialize, but raised an error instead."));
}

TEST(FailureMatcherTest, ComposesWithGMock) {
  TestResult result =
      CreateResult("foo", ::conformance::PROTOBUF,
                   ResponseFromText(R"pb(parse_error: "failed to parse")pb"));

  EXPECT_TRUE(Value(result, Not(IsSerializeError())));
  EXPECT_TRUE(Value(result, AnyOf(IsSerializeError(), IsParseError())));
  EXPECT_FALSE(Value(result, AllOf(IsSerializeError(), IsParseError())));
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
