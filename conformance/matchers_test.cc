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
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "conformance/binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "conformance/mock_test_runner.h"
#include "conformance/testee.h"
#include "google/protobuf/descriptor.h"
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
using ::protobuf_test_messages::proto2::UnknownToTestAllTypes;
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
// its format, so the input is empty.
TestResult CreateResult(absl::string_view test_name, WireFormat input_format,
                        WireFormat output_format,
                        ConformanceResponse response) {
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
        return std::move(message).SerializeText();
      case ::conformance::JSON:
        return std::move(message).SerializeJson();
      default:
        return std::move(message).SerializeBinary();
    }
  }();
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
// that answers `response_textproto`.
TestResult ResultWithResponse(absl::string_view response_textproto,
                              TestPriority priority = TestPriority::kP0) {
  NiceMock<MockTestRunner> runner;
  ON_CALL(runner, RunTest)
      .WillByDefault(
          Return(ResponseFromText(response_textproto).SerializeAsString()));
  internal::Testee testee(&runner);
  return testee.CreateTest("foo", priority)
      .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
      .SerializeBinary();
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

TEST(PrintTestResultTest, TruncatedProtobufPayloadIsDecodedInFull) {
  // The raw bytes are cut after 200 bytes, but the message they decode to is
  // printed whole.
  TestAllTypesProto2 message;
  message.set_optional_string(std::string(300, 'a'));
  const std::string payload = message.SerializeAsString();

  EXPECT_EQ(
      PrintToString(ResultWithResponse(
          absl::StrCat("protobuf_payload: \"", absl::CEscape(payload), "\""))),
      absl::StrCat(kPrintedRequiredTest, " with response {protobuf_payload: \"",
                   absl::CEscape(payload.substr(0, 200)),
                   "...(truncated)\"} (decoded: {optional_string: \"",
                   std::string(300, 'a'), "\"})"));
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

TEST(MatcherDescriptionTest, WhenParsed) {
  EXPECT_EQ(Describe(WhenParsed(EqualsTextProto(R"pb(optional_int32: 9)pb"))),
            R"(when parsed, equals text proto "optional_int32: 9")");
  EXPECT_EQ(
      DescribeNegation(WhenParsed(EqualsTextProto(R"pb(optional_int32: 9)pb"))),
      R"(when parsed, doesn't equal text proto "optional_int32: 9")");
}

TEST(MatcherDescriptionTest, WhenParsedWithGenericMatcher) {
  EXPECT_EQ(Describe(WhenParsed(_)), "when parsed, is anything");
  EXPECT_EQ(
      Describe(WhenParsed(Not(EqualsTextProto(R"pb(optional_int32: 9)pb")))),
      R"(when parsed, doesn't equal text proto "optional_int32: 9")");
}

TEST(MatcherDescriptionTest, RawPayload) {
  EXPECT_EQ(Describe(RawPayload(Wire("foo"))), R"(payload is equal to "foo")");
  EXPECT_EQ(DescribeNegation(RawPayload(Wire("foo"))),
            R"(payload isn't equal to "foo")");
  EXPECT_EQ(Describe(RawPayload(VarintField(1, 2))),
            R"(payload is equal to "\010\002")");
  EXPECT_EQ(DescribeNegation(RawPayload(VarintField(1, 2))),
            R"(payload isn't equal to "\010\002")");
}

TEST(MatcherDescriptionTest, EqualsTextProto) {
  EXPECT_EQ(testing::DescribeMatcher<const Message&>(
                EqualsTextProto(R"pb(optional_int32: 9)pb")),
            "equals text proto \"optional_int32: 9\"");
  EXPECT_EQ(testing::DescribeMatcher<const Message&>(
                EqualsTextProto(R"pb(optional_int32: 9)pb"), /*negation=*/true),
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
// Response handling shared by WhenParsed() and RawPayload()
// ---------------------------------------------------------------------------

// These responses fail before any payload is compared, so the expected payload
// is irrelevant.

TEST(PayloadMatcherTest, EmptyResponse) {
  TestResult result =
      CreateResult("foo", ::conformance::PROTOBUF, ConformanceResponse());

  EXPECT_THAT(Explain(WhenParsed(_), result),
              Rejects("Response didn't have any field in the Response."));
  EXPECT_THAT(Explain(RawPayload(Wire()), result),
              Rejects("Response didn't have any field in the Response."));
}

struct ErrorResponseCase {
  absl::string_view name;
  absl::string_view response;
};

class PayloadMatcherErrorResponseTest
    : public testing::TestWithParam<ErrorResponseCase> {};

TEST_P(PayloadMatcherErrorResponseTest, IsAFailureForWhenParsed) {
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ResponseFromText(GetParam().response));

  EXPECT_THAT(Explain(WhenParsed(_), result),
              Rejects("Failed to parse input or produce output."));
}

TEST_P(PayloadMatcherErrorResponseTest, IsAFailureForPayload) {
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ResponseFromText(GetParam().response));

  EXPECT_THAT(Explain(RawPayload(Wire()), result),
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

  EXPECT_THAT(Explain(WhenParsed(_), result),
              Rejects("the testee skipped the test: skipped message"));
  EXPECT_THAT(Explain(RawPayload(Wire()), result),
              Rejects("the testee skipped the test: skipped message"));
}

TEST(PayloadMatcherTest, WrongOutputFormat) {
  TestResult result =
      CreateResult("foo", ::conformance::PROTOBUF,
                   ResponseFromText(R"pb(json_payload: "{}")pb"));

  EXPECT_THAT(
      Explain(WhenParsed(_), result),
      Rejects("Test was asked for PROTOBUF output but provided JSON instead."));
  EXPECT_THAT(
      Explain(RawPayload(Wire()), result),
      Rejects("Test was asked for PROTOBUF output but provided JSON instead."));
}

TEST(PayloadMatcherTest, WrongOutputFormatText) {
  TestResult result = CreateResult("foo", ::conformance::TEXT_FORMAT,
                                   ProtobufPayload(VarintField(1, 2)));

  EXPECT_THAT(
      Explain(WhenParsed(EqualsTextProto(R"pb(optional_int32: 2)pb")), result),
      Rejects("Test was asked for TEXT_FORMAT output but provided PROTOBUF "
              "instead."));
}

TEST(PayloadMatcherTest, WrongOutputFormatJspb) {
  TestResult result =
      CreateResult("foo", ::conformance::JSON,
                   ResponseFromText(R"pb(jspb_payload: "[]")pb"));

  EXPECT_THAT(
      Explain(RawPayload(Wire()), result),
      Rejects("Test was asked for JSON output but provided JSPB instead."));
}

// ---------------------------------------------------------------------------
// WhenParsed()
// ---------------------------------------------------------------------------

TEST(WhenParsedTest, Matches) {
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(VarintField(1, 2)));

  EXPECT_THAT(
      Explain(WhenParsed(EqualsTextProto(R"pb(optional_int32: 2)pb")), result),
      Accepts(IsEmpty()));
}

TEST(WhenParsedTest, UnparseableProtobufPayload) {
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(Wire("\001")));

  EXPECT_THAT(
      Explain(WhenParsed(EqualsTextProto("")), result),
      Rejects("Protobuf output we received from test was unparseable."));
}

TEST(WhenParsedTest, UnparseableTextPayload) {
  TestResult result =
      CreateResult("foo", ::conformance::TEXT_FORMAT,
                   ResponseFromText(R"pb(text_payload: "nonsense: 1")pb"));

  EXPECT_THAT(
      Explain(WhenParsed(EqualsTextProto("")), result),
      Rejects("TEXT_FORMAT output we received from test was unparseable."));
}

TEST(WhenParsedTest, UnparseablePayloadFailsEvenForAnything) {
  // The payload has to be decodable before any inner matcher gets a say.
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(Wire("\001")));

  EXPECT_THAT(
      Explain(WhenParsed(_), result),
      Rejects("Protobuf output we received from test was unparseable."));
}

TEST(WhenParsedTest, JsonOutputIsNotSupported) {
  TestResult result =
      CreateResult("foo", ::conformance::JSON,
                   ResponseFromText(R"pb(json_payload: "{}")pb"));

  EXPECT_THAT(
      Explain(WhenParsed(_), result),
      Rejects("WhenParsed is not supported for JSON output; use "
              "RawPayload() to match the raw JSON text until JSON matching is "
              "migrated to gtest (b/410122158)."));
}

TEST(WhenParsedTest, TextOutput) {
  TestResult result = CreateResult(
      "foo", ::conformance::TEXT_FORMAT,
      ResponseFromText(R"pb(text_payload: "optional_int32: 9")pb"));

  EXPECT_TRUE(
      Value(result, WhenParsed(EqualsTextProto(R"pb(optional_int32: 9)pb"))));
}

TEST(WhenParsedTest, TextOutputFieldNumbersAccepted) {
  // Testees asked to print unknown fields emit them by number, so field
  // numbers are always accepted.
  TestResult result =
      CreateResult("foo", ::conformance::TEXT_FORMAT,
                   ResponseFromText(R"pb(text_payload: "1: 9")pb"));

  EXPECT_TRUE(
      Value(result, WhenParsed(EqualsTextProto(R"pb(optional_int32: 9)pb"))));
}

TEST(WhenParsedTest, EqualsTextProtoMismatch) {
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(VarintField(1, 2)));

  EXPECT_THAT(
      Explain(WhenParsed(EqualsTextProto(R"pb(optional_int32: 1)pb")), result),
      Rejects("Output was not equivalent to reference message: "
              "modified: optional_int32: 1 -> 2\n"));
}

TEST(WhenParsedTest, EqualsTextProtoMatchesNan) {
  TestAllTypesProto2 message;
  message.set_optional_float(std::nanf(""));
  message.set_optional_double(std::nan(""));
  TestResult result =
      CreateResult("foo", ::conformance::PROTOBUF,
                   ProtobufPayload(Wire(message.SerializeAsString())));

  EXPECT_TRUE(Value(result, WhenParsed(EqualsTextProto(R"pb(
                      optional_float: nan
                      optional_double: nan
                    )pb"))));
}

TEST(WhenParsedDeathTest, EqualsTextProtoUnparseableExpected) {
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(VarintField(1, 2)));

  EXPECT_DEATH(
      (void)Explain(WhenParsed(EqualsTextProto(R"pb(unknown: 1)pb")), result),
      "Failed to parse expected text proto.*unknown: 1");
}

TEST(WhenParsedTest, EqualsBinaryProto) {
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(VarintField(1, 2)));

  // Equivalent, but not byte-identical (over-long varint encoding).
  EXPECT_TRUE(
      Value(result, WhenParsed(EqualsBinaryProto(LongVarintField(1, 2, 1)))));
}

TEST(WhenParsedTest, EqualsBinaryProtoWithTextOutput) {
  TestResult result = CreateResult(
      "foo", ::conformance::TEXT_FORMAT,
      ResponseFromText(R"pb(text_payload: "optional_int32: 9")pb"));

  EXPECT_TRUE(Value(result, WhenParsed(EqualsBinaryProto(VarintField(1, 9)))));
}

TEST(WhenParsedTest, EqualsBinaryProtoMismatch) {
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(VarintField(1, 2)));

  EXPECT_THAT(Explain(WhenParsed(EqualsBinaryProto(VarintField(1, 1))), result),
              Rejects("Output was not equivalent to reference message: "
                      "modified: optional_int32: 1 -> 2\n"));
}

TEST(WhenParsedDeathTest, EqualsBinaryProtoUnparseableExpected) {
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(VarintField(1, 2)));

  EXPECT_DEATH(
      (void)Explain(WhenParsed(EqualsBinaryProto(Wire("\001"))), result),
      "Failed to parse expected wire data");
}

TEST(WhenParsedTest, AcceptsAnyMessageMatcher) {
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(VarintField(1, 2)));

  EXPECT_TRUE(Value(result, WhenParsed(_)));
  EXPECT_TRUE(Value(
      result,
      WhenParsed(AllOf(EqualsTextProto(R"pb(optional_int32: 2)pb"),
                       Not(EqualsTextProto(R"pb(optional_int32: 3)pb"))))));
}

TEST(WhenParsedTest, InnerMatcherWithoutExplanation) {
  // Inner matchers that don't explain themselves still get a useful failure
  // message, including the decoded payload.
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(VarintField(1, 2)));

  EXPECT_THAT(
      Explain(WhenParsed(Not(EqualsTextProto(R"pb(optional_int32: 2)pb"))),
              result),
      Rejects("Expect: when parsed, doesn't equal text proto "
              R"("optional_int32: 2", but got: {optional_int32: 2})"));
}

TEST(WhenParsedTest, ComposesWithGMock) {
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(VarintField(1, 2)));

  EXPECT_TRUE(Value(result, Not(WhenParsed(EqualsTextProto("")))));
  EXPECT_TRUE(Value(
      result, AnyOf(IsParseError(),
                    WhenParsed(EqualsTextProto(R"pb(optional_int32: 2)pb")))));
  EXPECT_TRUE(Value(result, AllOf(Not(IsParseError()), WhenParsed(_))));
}

// ---------------------------------------------------------------------------
// WhenParsedAs()
// ---------------------------------------------------------------------------

TEST(WhenParsedAsTest, DecodesBinaryPayloadAsTheGivenType) {
  // Field 1001 is unknown to the test's type (TestAllTypesProto2) but is
  // optional_int32 of the shadow type UnknownToTestAllTypes.
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(VarintField(1001, 7)));

  EXPECT_THAT(Explain(WhenParsedAs<UnknownToTestAllTypes>(
                          EqualsTextProto(R"pb(optional_int32: 7)pb")),
                      result),
              Accepts(IsEmpty()));
  // Decoded as the test's own type the field stays unknown.
  EXPECT_THAT(
      Explain(WhenParsed(EqualsTextProto(R"pb(optional_int32: 7)pb")), result),
      Rejects("Output was not equivalent to reference message: "
              "added: 1001[0]: 7\n"
              "deleted: optional_int32: 7\n"));
}

TEST(WhenParsedAsTest, TextOutputFieldNumbersAccepted) {
  // A testee asked to print unknown fields emits them by number.  The
  // shadow type gives them names again.
  TestResult result =
      CreateResult("foo", ::conformance::TEXT_FORMAT,
                   ResponseFromText(R"pb(text_payload: "1001: 7")pb"));

  EXPECT_THAT(Explain(WhenParsedAs<UnknownToTestAllTypes>(
                          EqualsTextProto(R"pb(optional_int32: 7)pb")),
                      result),
              Accepts(IsEmpty()));
}

TEST(WhenParsedAsTest, FailureMessagesMatchWhenParsed) {
  // With the test's own type as the override, WhenParsedAs() must behave
  // exactly like WhenParsed(), failure messages included.
  {
    TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                     ProtobufPayload(Wire("\001")));
    EXPECT_THAT(
        Explain(WhenParsedAs<TestAllTypesProto2>(EqualsTextProto("")), result),
        Rejects("Protobuf output we received from test was unparseable."));
  }
  {
    TestResult result =
        CreateResult("foo", ::conformance::TEXT_FORMAT,
                     ResponseFromText(R"pb(text_payload: "nonsense: 1")pb"));
    EXPECT_THAT(
        Explain(WhenParsedAs<TestAllTypesProto2>(EqualsTextProto("")), result),
        Rejects("TEXT_FORMAT output we received from test was unparseable."));
  }
  {
    TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                     ProtobufPayload(VarintField(1, 2)));
    EXPECT_THAT(Explain(WhenParsedAs<TestAllTypesProto2>(
                            EqualsTextProto(R"pb(optional_int32: 1)pb")),
                        result),
                Rejects("Output was not equivalent to reference message: "
                        "modified: optional_int32: 1 -> 2\n"));
    EXPECT_THAT(
        Explain(WhenParsedAs<TestAllTypesProto2>(
                    Not(EqualsTextProto(R"pb(optional_int32: 2)pb"))),
                result),
        Rejects("Expect: when parsed, doesn't equal text proto "
                R"("optional_int32: 2", but got: {optional_int32: 2})"));
  }
  {
    TestResult result =
        CreateResult("foo", ::conformance::PROTOBUF,
                     ResponseFromText(R"pb(parse_error: "foo")pb"));
    EXPECT_THAT(Explain(WhenParsedAs<TestAllTypesProto2>(_), result),
                Rejects("Failed to parse input or produce output."));
  }
}

TEST(MatcherDescriptionTest, WhenParsedAs) {
  // The description (gtest's "Expected:" line, never a failure-list message)
  // names the type the payload is decoded as.
  EXPECT_EQ(Describe(WhenParsedAs<UnknownToTestAllTypes>(
                EqualsTextProto(R"pb(optional_int32: 9)pb"))),
            "when parsed as "
            "protobuf_test_messages.proto2.UnknownToTestAllTypes, equals text "
            R"(proto "optional_int32: 9")");
  EXPECT_EQ(
      DescribeNegation(WhenParsedAs<UnknownToTestAllTypes>(
          EqualsTextProto(R"pb(optional_int32: 9)pb"))),
      "when parsed as protobuf_test_messages.proto2.UnknownToTestAllTypes, "
      R"(doesn't equal text proto "optional_int32: 9")");
}

// ---------------------------------------------------------------------------
// RawPayload()
// ---------------------------------------------------------------------------

TEST(RawPayloadTest, MatchesBytes) {
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(VarintField(1, 2)));

  EXPECT_TRUE(Value(result, RawPayload(VarintField(1, 2))));
  EXPECT_THAT(Explain(RawPayload(VarintField(1, 2)), result),
              Accepts(IsEmpty()));
}

TEST(RawPayloadTest, BytesMismatch) {
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(VarintField(1, 2)));

  // Equivalent messages, but not byte-identical.  The failure message is the
  // legacy one.
  EXPECT_THAT(Explain(RawPayload(LongVarintField(1, 2, 1)), result),
              Rejects("Output was not equivalent to reference message: "
                      "Expect: \\010\\202\\000, but got: \\010\\002"));
}

TEST(RawPayloadTest, UnparseableProtobufOutputFailsEvenForTheExactBytes) {
  // Like the legacy runner's require_same_wire_format, binary output must
  // decode as the test's message type.
  TestResult result = CreateResult("foo", ::conformance::PROTOBUF,
                                   ProtobufPayload(Wire("\001")));

  EXPECT_THAT(
      Explain(RawPayload(Wire("\001")), result),
      Rejects("Protobuf output we received from test was unparseable."));
}

TEST(RawPayloadTest, TextOutput) {
  TestResult result = CreateResult(
      "foo", ::conformance::TEXT_FORMAT,
      ResponseFromText(R"pb(text_payload: "optional_int32: 9")pb"));

  EXPECT_TRUE(Value(result, RawPayload(Wire("optional_int32: 9"))));
  // Text output isn't required to be parseable.
  EXPECT_TRUE(Value(
      CreateResult("bar", ::conformance::TEXT_FORMAT,
                   ResponseFromText(R"pb(text_payload: "nonsense: 1")pb")),
      RawPayload(Wire("nonsense: 1"))));
}

TEST(RawPayloadTest, JsonOutput) {
  // Raw payloads don't need to be decoded, so JSON works already.
  TestResult result =
      CreateResult("foo", ::conformance::JSON,
                   ResponseFromText(R"pb(json_payload: "{}")pb"));

  EXPECT_TRUE(Value(result, RawPayload(Wire("{}"))));
}

// ---------------------------------------------------------------------------
// EqualsTextProto() / EqualsBinaryProto() used directly on messages
// ---------------------------------------------------------------------------

TEST(EqualsTextProtoTest, Success) {
  TestAllTypesProto2 message;
  message.set_optional_int32(9);
  EXPECT_THAT(message, EqualsTextProto(R"pb(optional_int32: 9)pb"));
}

TEST(EqualsTextProtoTest, MatchesNan) {
  TestAllTypesProto2 message;
  message.set_optional_float(std::nanf(""));
  EXPECT_THAT(message, EqualsTextProto(R"pb(optional_float: nan)pb"));
}

TEST(EqualsTextProtoTest, Failure) {
  TestAllTypesProto2 message;
  message.set_optional_float(std::nanf(""));
  auto matcher = EqualsTextProto(R"pb(optional_float: 1.0)pb");
  EXPECT_THAT(Explain(matcher, message),
              Rejects("Output was not equivalent to reference message: "
                      "modified: optional_float: 1 -> nan\n"));
  EXPECT_THAT(message, Not(matcher));
}

TEST(EqualsTextProtoTest, WorksOnBaseMessage) {
  TestAllTypesProto2 message;
  message.set_optional_int32(9);
  const Message& base = message;
  EXPECT_THAT(base, EqualsTextProto(R"pb(optional_int32: 9)pb"));
  EXPECT_THAT(base, Not(EqualsTextProto(R"pb(optional_int32: 8)pb")));
}

TEST(EqualsTextProtoDeathTest, ParseFailure) {
  TestAllTypesProto2 message;
  EXPECT_DEATH((void)Explain(EqualsTextProto(R"pb(unknown: 1.0)pb"), message),
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
