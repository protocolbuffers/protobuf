// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/matchers.h"

#include <cmath>
#include <fstream>
#include <iterator>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/base/log_severity.h"
#include "absl/log/absl_check.h"
#include "absl/log/scoped_mock_log.h"
#include "absl/status/status_matchers.h"
#include "absl/strings/ascii.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "conformance/binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "conformance/failure_list.h"
#include "conformance/global_test_environment.h"
#include "conformance/matchers_testing.h"
#include "conformance/mock_test_runner.h"
#include "conformance/result_ledger.h"
#include "conformance/result_record.h"
#include "conformance/testee.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/test_messages_proto2.pb.h"
#include "google/protobuf/text_format.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {
namespace {

// Backing state for the mocked-out global environment below.  In the real
// conformance binary this would be populated from CLI flags and the failure
// list on disk, but we don't want to drag that in for a unit test.
const FailureList* global_failure_list = nullptr;

}  // namespace

const FailureList& GetGlobalFailureList() {
  ABSL_CHECK(global_failure_list != nullptr);
  return *global_failure_list;
}

}  // namespace internal

namespace {

using ::absl_testing::IsOk;
using ::conformance::ConformanceResponse;
using ::conformance::WireFormat;
using ::google::protobuf::conformance::TestPriority;
using ::google::protobuf::conformance::internal::ResultLedger;
using ::google::protobuf::conformance::internal::ResultRecord;
using ::google::protobuf::conformance::internal::TestResult;
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
using ::testing::NiceMock;
using ::testing::Not;
using ::testing::Optional;
using ::testing::Pair;
using ::testing::PrintToString;
using ::testing::Return;
using ::testing::TestParamInfo;
using ::testing::Truly;
using ::testing::Value;
using ::testing::Values;

// Creates a result for the leaf matcher tests by running a kP0 test with the
// given input format and requested output format (and, for text output,
// whether unknown fields are to be printed) against a testee that answers
// `response`.  The matchers never look at the input payload itself, only at
// its format, so the input is empty.
//
// The leaf matchers never touch the failure list; the tests make sure of it
// by not installing one at all while they run.  Only YieldsTest below
// installs one.
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

TEST(MatcherDescriptionTest, Yields) {
  EXPECT_EQ(
      Describe(Yields(IsParseError())),
      "yields a result that is a parse error (or is an expected failure)");
  EXPECT_EQ(DescribeNegation(Yields(IsParseError())),
            "doesn't yield a result that is a parse error, nor an expected "
            "failure");
  EXPECT_EQ(Describe(Yields(AnyOf(IsParseError(), WhenParsed(_)))),
            "yields a result that (is a parse error) or (when parsed, is "
            "anything) (or is an expected failure)");
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

// ---------------------------------------------------------------------------
// Yields()
// ---------------------------------------------------------------------------

// A snapshot of the ResultLedger counters, for concise assertions.
struct Counts {
  int expected_successes = 0;
  int expected_failures = 0;
  int unexpected_successes = 0;
  int unexpected_failures = 0;
  int skipped = 0;
  int listed_skips = 0;
  int tolerated_failures = 0;

  bool operator==(const Counts& other) const {
    return expected_successes == other.expected_successes &&
           expected_failures == other.expected_failures &&
           unexpected_successes == other.unexpected_successes &&
           unexpected_failures == other.unexpected_failures &&
           skipped == other.skipped && listed_skips == other.listed_skips &&
           tolerated_failures == other.tolerated_failures;
  }

  friend void PrintTo(const Counts& counts, std::ostream* os) {
    *os << "{expected_successes: " << counts.expected_successes
        << ", expected_failures: " << counts.expected_failures
        << ", unexpected_successes: " << counts.unexpected_successes
        << ", unexpected_failures: " << counts.unexpected_failures
        << ", skipped: " << counts.skipped
        << ", listed_skips: " << counts.listed_skips
        << ", tolerated_failures: " << counts.tolerated_failures << "}";
  }
};

Counts GetCounts(const ResultLedger& ledger) {
  return Counts{
      /*expected_successes=*/ledger.expected_successes(),
      /*expected_failures=*/ledger.expected_failures(),
      /*unexpected_successes=*/ledger.unexpected_successes(),
      /*unexpected_failures=*/ledger.unexpected_failures(),
      /*skipped=*/ledger.skipped(),
      /*listed_skips=*/ledger.listed_skips(),
      /*tolerated_failures=*/
      ledger.tolerated_failures(),
  };
}

// The names Run() below produces for the test named "foo".
constexpr absl::string_view kP0Foo =
    "Required.Proto2.ProtobufInput.foo.ProtobufOutput";
constexpr absl::string_view kP1Foo =
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

class YieldsTest : public testing::Test {
 protected:
  YieldsTest() { internal::global_failure_list = &ledger_.failure_list(); }
  ~YieldsTest() override {
    ledger_.Finalize().IgnoreError();
    internal::global_failure_list = nullptr;
  }

  // Makes the testee answer every test with `response`.
  void RespondWith(absl::string_view response) {
    ON_CALL(runner_, RunTest)
        .WillByDefault(Return(ResponseFromText(response).SerializeAsString()));
  }

  // Runs the binary-to-binary test `name` (of the given priority) against a
  // testee answering `response`.
  TestResult Run(absl::string_view response,
                 TestPriority priority = TestPriority::kP0,
                 absl::string_view name = "foo") {
    RespondWith(response);
    return testee_.CreateTest(name, priority)
        .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
        .SerializeBinary();
  }

  // Like Run(), but asks for text format output.
  TestResult RunText(absl::string_view response,
                     TestPriority priority = TestPriority::kP0,
                     absl::string_view name = "foo") {
    RespondWith(response);
    return testee_.CreateTest(name, priority)
        .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
        .SerializeText();
  }

  // Lists `test_name` in the failure list with `message`, as a line of a
  // failure list file would.
  void AddToFailureList(absl::string_view test_name,
                        absl::string_view message) {
    ASSERT_THAT(ledger_.LoadFailureList(WriteTempFile(
                    absl::StrCat("failure_list_", ++failure_lists_, ".txt"),
                    absl::StrCat(test_name, " # ", message, "\n"))),
                IsOk());
  }

  // Replays the outcomes Yields() recorded on the running gtest test into
  // ledger_, as the test environment's listener does as they arrive,
  // and returns the counters.  Tests the ledger has heard about are skipped,
  // so this can be called repeatedly.
  Counts ReplayedCounts() {
    for (const auto& [name, value] :
         internal::RecordedResults(internal::CurrentGtestResult())) {
      if (!ledger_.WasReported(name)) {
        ledger_.Report(name, *ResultRecord::Parse(value));
      }
    }
    return GetCounts(ledger_);
  }

  ResultLedger ledger_;
  NiceMock<MockTestRunner> runner_;
  internal::Testee testee_{&runner_};
  int failure_lists_ = 0;
};

TEST_F(YieldsTest, UnlistedSuccessPasses) {
  auto matcher =
      Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 2)pb")));

  EXPECT_THAT(Explain(matcher, Run(kPayload2)), Accepts(IsEmpty()));
  EXPECT_THAT(Run(kPayload2, TestPriority::kP0, "other"), matcher);
  EXPECT_EQ(ReplayedCounts(), (Counts{/*expected_successes=*/2}));
  EXPECT_THAT(ledger_.Finalize(), IsOk());
}

TEST_F(YieldsTest, UnlistedFailureFails) {
  auto matcher =
      Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 1)pb")));
  TestResult result = Run(kPayload2);

  EXPECT_THAT(Explain(matcher, result),
              Rejects(absl::StrCat(kMismatch1,
                                   "\nUnexpected failure for test: ", kP0Foo)));
  EXPECT_YIELDS_FAILURE(EXPECT_THAT(result, matcher),
                        "modified: optional_int32: 1 -> 2");
  EXPECT_EQ(ReplayedCounts(), (Counts{/*expected_successes=*/0,
                                      /*expected_failures=*/0,
                                      /*unexpected_successes=*/0,
                                      /*unexpected_failures=*/1}));
  // The recorded message is the legacy one, as the failure list would get it.
  EXPECT_THAT(ledger_.UnexpectedFailures(),
              ElementsAre(FieldsAre(kP0Foo,
                                    "Output was not equivalent to reference "
                                    "message: modified: optional_int32: 1 -> 2",
                                    absl::nullopt)));
}

TEST_F(YieldsTest, UnlistedFailureIsCountedOnceDespiteGtestRetrying) {
  // gtest evaluates a matcher a second time to explain a failed assertion.
  EXPECT_YIELDS_FAILURE(EXPECT_THAT(Run(kPayload2), Yields(IsParseError())),
                        "Should have failed to parse, but didn't.");
  EXPECT_EQ(ReplayedCounts(), (Counts{/*expected_successes=*/0,
                                      /*expected_failures=*/0,
                                      /*unexpected_successes=*/0,
                                      /*unexpected_failures=*/1}));
  EXPECT_THAT(ledger_.UnexpectedFailures(),
              ElementsAre(FieldsAre(kP0Foo, kNotAParseError, absl::nullopt)));
}

TEST_F(YieldsTest, PayloadBytesMismatchRecordsTheLegacyOctalMessage) {
  EXPECT_YIELDS_FAILURE(
      EXPECT_THAT(Run(kPayload2), Yields(RawPayload(VarintField(1, 1)))),
      "Output was not equivalent to reference message: "
      "Expect: \\010\\001, but got: \\010\\002");
  EXPECT_EQ(ReplayedCounts(), (Counts{/*expected_successes=*/0,
                                      /*expected_failures=*/0,
                                      /*unexpected_successes=*/0,
                                      /*unexpected_failures=*/1}));
  EXPECT_THAT(
      ledger_.UnexpectedFailures(),
      ElementsAre(FieldsAre(kP0Foo,
                            "Output was not equivalent to reference message: "
                            "Expect: \\010\\001, but got: \\010\\002",
                            absl::nullopt)));
}

TEST_F(YieldsTest, TextFormatOutput) {
  constexpr absl::string_view kText2 =
      R"pb(text_payload: "optional_int32: 2")pb";

  EXPECT_THAT(RunText(kText2),
              Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 2)pb"))));
  EXPECT_THAT(RunText(kText2, TestPriority::kP0, "raw"),
              Yields(RawPayload(Wire("optional_int32: 2"))));
  EXPECT_EQ(ReplayedCounts(), (Counts{/*expected_successes=*/2}));

  EXPECT_YIELDS_FAILURE(
      EXPECT_THAT(
          RunText(kText2, TestPriority::kP0, "mismatch"),
          Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 1)pb")))),
      "modified: optional_int32: 1 -> 2");
  EXPECT_YIELDS_FAILURE(
      EXPECT_THAT(RunText(R"pb(text_payload: "nonsense: 1")pb",
                          TestPriority::kP0, "unparseable"),
                  Yields(WhenParsed(_))),
      "TEXT_FORMAT output we received from test was unparseable.");
  EXPECT_EQ(ReplayedCounts(),
            (Counts{/*expected_successes=*/2, /*expected_failures=*/0,
                    /*unexpected_successes=*/0, /*unexpected_failures=*/2}));
  EXPECT_THAT(
      ledger_.UnexpectedFailures(),
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
  AddToFailureList(kP0Foo, kMismatch1);
  absl::ScopedMockLog log;
  EXPECT_CALL(log, Log).Times(AnyNumber());
  // (The message is logged without its trailing newline.)
  EXPECT_CALL(
      log,
      Log(absl::LogSeverity::kInfo, _,
          Eq(absl::StrCat("Ignoring expected failure for test ", kP0Foo, ": ",
                          absl::StripTrailingAsciiWhitespace(kMismatch1)))))
      .Times(1);
  log.StartCapturingLogs();

  auto matcher =
      Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 1)pb")));
  TestResult result = Run(kPayload2);

  EXPECT_THAT(Explain(matcher, result), Accepts(IsEmpty()));
  EXPECT_EQ(ReplayedCounts(), (Counts{/*expected_successes=*/0,
                                      /*expected_failures=*/1}));
  EXPECT_THAT(ledger_.UnexpectedFailures(), IsEmpty());
  EXPECT_THAT(ledger_.Finalize(), IsOk());
}

TEST_F(YieldsTest, ListedFailureWithMessagePrefixPasses) {
  AddToFailureList(kP0Foo, "Output was not equivalent to reference");

  EXPECT_THAT(Run(kPayload2),
              Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 1)pb"))));
  EXPECT_EQ(ReplayedCounts(), (Counts{/*expected_successes=*/0,
                                      /*expected_failures=*/1}));
  EXPECT_THAT(ledger_.Finalize(), IsOk());
}

TEST_F(YieldsTest, ListedFailureViaWildcardPasses) {
  AddToFailureList("Required.*.ProtobufInput.foo.ProtobufOutput",
                   kNotAParseError);

  EXPECT_THAT(Run(kPayload2), Yields(IsParseError()));
  EXPECT_EQ(ReplayedCounts(), (Counts{/*expected_successes=*/0,
                                      /*expected_failures=*/1}));
  EXPECT_THAT(ledger_.Finalize(), IsOk());
}

TEST_F(YieldsTest, ListedFailureWithDifferentMessageFails) {
  AddToFailureList(kP0Foo, "Some other message");
  auto matcher = Yields(IsParseError());
  TestResult result = Run(kPayload2);

  EXPECT_THAT(
      Explain(matcher, result),
      Rejects(absl::StrCat(
          kNotAParseError, "\nUnexpected failure message for test: ", kP0Foo,
          " expected: Some other message actual: ", kNotAParseError)));
  EXPECT_YIELDS_FAILURE(EXPECT_THAT(result, matcher),
                        "Unexpected failure message for test");
  EXPECT_EQ(ReplayedCounts(), (Counts{/*expected_successes=*/0,
                                      /*expected_failures=*/0,
                                      /*unexpected_successes=*/0,
                                      /*unexpected_failures=*/1}));
  EXPECT_THAT(ledger_.UnexpectedFailures(),
              ElementsAre(FieldsAre(kP0Foo, kNotAParseError, absl::nullopt)));
}

TEST_F(YieldsTest, ListedSuccessFails) {
  AddToFailureList(kP0Foo, kNotAParseError);
  auto matcher =
      Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 2)pb")));
  TestResult result = Run(kPayload2);

  // The legacy runner's wording, including the matched entry.
  EXPECT_THAT(
      Explain(matcher, result),
      Rejects(absl::StrCat("test ", kP0Foo, " (matched to ", kP0Foo,
                           ") is in the failure list, but test succeeded.  "
                           "Remove its match from the failure list.")));
  EXPECT_YIELDS_FAILURE(EXPECT_THAT(result, matcher),
                        "is in the failure list, but test succeeded");
  EXPECT_EQ(ReplayedCounts(), (Counts{/*expected_successes=*/0,
                                      /*expected_failures=*/0,
                                      /*unexpected_successes=*/1}));
  EXPECT_THAT(
      ledger_.UnexpectedSuccesses(),
      ElementsAre(FieldsAre(kP0Foo, kNotAParseError, Optional(kP0Foo))));
}

TEST_F(YieldsTest, ListedSuccessViaWildcardNamesTheWildcard) {
  AddToFailureList("Required.*.ProtobufInput.foo.ProtobufOutput", "");

  EXPECT_YIELDS_FAILURE(
      EXPECT_THAT(Run(kParseError), Yields(IsParseError())),
      "test Required.Proto2.ProtobufInput.foo.ProtobufOutput (matched to "
      "Required.*.ProtobufInput.foo.ProtobufOutput) is in the failure list, "
      "but test succeeded.  Remove its match from the failure list.");
  EXPECT_EQ(ReplayedCounts(), (Counts{/*expected_successes=*/0,
                                      /*expected_failures=*/0,
                                      /*unexpected_successes=*/1}));
}

TEST_F(YieldsTest, P1FailureIsToleratedWhenNotEnforced) {
  ledger_.set_enforcement_level(kP0);
  absl::ScopedMockLog log;
  EXPECT_CALL(log, Log).Times(AnyNumber());
  EXPECT_CALL(
      log,
      Log(absl::LogSeverity::kWarning, _,
          Eq(absl::StrCat("WARNING, test=", kP1Foo, ": ", kNotAParseError))))
      .Times(1);
  log.StartCapturingLogs();

  auto matcher = Yields(IsParseError());
  TestResult result = Run(kPayload2, TestPriority::kP1);

  EXPECT_THAT(Explain(matcher, result), Accepts(IsEmpty()));
  // It's neither a skip nor a failure.
  EXPECT_EQ(ReplayedCounts(),
            (Counts{/*expected_successes=*/0, /*expected_failures=*/0,
                    /*unexpected_successes=*/0, /*unexpected_failures=*/0,
                    /*skipped=*/0, /*listed_skips=*/0,
                    /*tolerated_failures=*/1}));
  EXPECT_THAT(ledger_.UnexpectedFailures(), IsEmpty());
  EXPECT_THAT(ledger_.Finalize(), IsOk());
}

TEST_F(YieldsTest, P1FailureFailsWhenEnforced) {
  ledger_.set_enforcement_level(kP1);

  EXPECT_YIELDS_FAILURE(
      EXPECT_THAT(Run(kPayload2, TestPriority::kP1), Yields(IsParseError())),
      "Should have failed to parse, but didn't.");
  EXPECT_EQ(ReplayedCounts(), (Counts{/*expected_successes=*/0,
                                      /*expected_failures=*/0,
                                      /*unexpected_successes=*/0,
                                      /*unexpected_failures=*/1}));
  EXPECT_THAT(ledger_.UnexpectedFailures(),
              ElementsAre(FieldsAre(kP1Foo, kNotAParseError, absl::nullopt)));
}

TEST_F(YieldsTest, ListedP1FailureIsTrackedEvenWhenNotEnforced) {
  // A kP1 test that's already in the failure list keeps being tracked
  // there, so that the list can't go stale unnoticed.
  ledger_.set_enforcement_level(kP0);
  AddToFailureList(kP1Foo, kNotAParseError);

  EXPECT_THAT(Run(kPayload2, TestPriority::kP1), Yields(IsParseError()));
  EXPECT_EQ(ReplayedCounts(), (Counts{/*expected_successes=*/0,
                                      /*expected_failures=*/1}));
  EXPECT_THAT(ledger_.Finalize(), IsOk());
}

TEST_F(YieldsTest,
       ListedP1FailureWithDifferentMessageFailsEvenWhenNotEnforced) {
  ledger_.set_enforcement_level(kP0);
  AddToFailureList(kP1Foo, "Some other message");

  EXPECT_YIELDS_FAILURE(
      EXPECT_THAT(Run(kPayload2, TestPriority::kP1), Yields(IsParseError())),
      "Unexpected failure message for test");
  EXPECT_EQ(ReplayedCounts(), (Counts{/*expected_successes=*/0,
                                      /*expected_failures=*/0,
                                      /*unexpected_successes=*/0,
                                      /*unexpected_failures=*/1}));
}

TEST_F(YieldsTest, P1SuccessPasses) {
  ledger_.set_enforcement_level(kP0);

  EXPECT_THAT(Run(kParseError, TestPriority::kP1), Yields(IsParseError()));
  EXPECT_EQ(ReplayedCounts(), (Counts{/*expected_successes=*/1}));
}

TEST_F(YieldsTest, UnlistedSkipPasses) {
  absl::ScopedMockLog log;
  EXPECT_CALL(log, Log).Times(AnyNumber());
  EXPECT_CALL(
      log, Log(absl::LogSeverity::kInfo, _,
               Eq(absl::StrCat("Skipping test ", kP0Foo, ": not supported"))))
      .Times(1);
  log.StartCapturingLogs();

  // The inner matcher never gets to see a skipped result.
  bool inner_called = false;
  auto matcher = Yields(Truly([&](const TestResult&) {
    inner_called = true;
    return false;
  }));
  TestResult result = Run(kSkipped);

  EXPECT_THAT(Explain(matcher, result), Accepts(IsEmpty()));
  EXPECT_FALSE(inner_called);
  EXPECT_EQ(ReplayedCounts(), (Counts{/*expected_successes=*/0,
                                      /*expected_failures=*/0,
                                      /*unexpected_successes=*/0,
                                      /*unexpected_failures=*/0,
                                      /*skipped=*/1}));
  EXPECT_THAT(ledger_.Finalize(), IsOk());
}

TEST_F(YieldsTest, ListedSkipFailsOnce) {
  // Deliberately stricter than the legacy runner, which passed this case: a
  // listed test the testee skips fails, but only here.  The entry counts as
  // seen (a skip says nothing about whether it is still needed), so Finalize()
  // doesn't report it a second time and --fix keeps it.
  ASSERT_THAT(ledger_.LoadFailureList(WriteTempFile(
                  "failure_list.txt", absl::StrCat(kP0Foo, " # abc\n"))),
              IsOk());
  auto matcher = Yields(IsParseError());
  TestResult result = Run(kSkipped);

  // The failure names the matched entry and the remedy, like an unexpected
  // success does.
  EXPECT_THAT(Explain(matcher, result),
              Rejects(absl::StrCat(
                  "test ", kP0Foo, " (matched to ", kP0Foo,
                  ") is in the failure list but was skipped by the testee: "
                  "not supported.  Remove its match from the failure list.")));
  EXPECT_YIELDS_FAILURE(EXPECT_THAT(result, matcher),
                        "is in the failure list but was skipped");
  // The ledger records it as a listed skip, not as an unexpected failure.
  EXPECT_EQ(ReplayedCounts(),
            (Counts{/*expected_successes=*/0, /*expected_failures=*/0,
                    /*unexpected_successes=*/0, /*unexpected_failures=*/0,
                    /*skipped=*/1, /*listed_skips=*/1}));
  EXPECT_THAT(ledger_.ListedSkips(), ElementsAre(Pair(kP0Foo, kP0Foo)));
  EXPECT_THAT(ledger_.UnexpectedFailures(), IsEmpty());
  EXPECT_THAT(ledger_.Finalize(), IsOk());

  const std::string fixed = WriteTempFile("fixed.txt", "");
  ASSERT_THAT(ledger_.SaveFailureList(fixed), IsOk());
  EXPECT_EQ(ReadFile(fixed), absl::StrCat(kP0Foo, " # abc\n"));
}

TEST_F(YieldsTest, RuntimeErrorKeepsTheInnerMatchersFailureMessage) {
  // The legacy runner reports a runtime error where a parse error was expected
  // with a dedicated message, which the failure lists rely on.
  EXPECT_YIELDS_FAILURE(
      EXPECT_THAT(Run(kRuntimeError), Yields(IsParseError())),
      "Should have failed to parse, but raised an error instead.");
  EXPECT_EQ(ReplayedCounts(), (Counts{/*expected_successes=*/0,
                                      /*expected_failures=*/0,
                                      /*unexpected_successes=*/0,
                                      /*unexpected_failures=*/1}));
  EXPECT_THAT(
      ledger_.UnexpectedFailures(),
      ElementsAre(FieldsAre(
          kP0Foo, "Should have failed to parse, but raised an error instead.",
          absl::nullopt)));
}

TEST_F(YieldsTest, ListedRuntimeErrorIsAnExpectedFailure) {
  AddToFailureList(kP0Foo,
                   "Should have failed to parse, but raised an error instead.");

  EXPECT_THAT(Run(kRuntimeError), Yields(IsParseError()));
  EXPECT_EQ(ReplayedCounts(), (Counts{/*expected_successes=*/0,
                                      /*expected_failures=*/1}));
  EXPECT_THAT(ledger_.Finalize(), IsOk());
}

TEST_F(YieldsTest, AnyOfMatchesEitherLeg) {
  auto matcher = Yields(
      AnyOf(IsParseError(), WhenParsed(EqualsBinaryProto(VarintField(1, 2)))));

  EXPECT_THAT(Run(kParseError, TestPriority::kP0, "first"), matcher);
  EXPECT_THAT(Run(kPayload2, TestPriority::kP0, "second"), matcher);
  EXPECT_EQ(ReplayedCounts(), (Counts{/*expected_successes=*/2}));
}

TEST_F(YieldsTest, AnyOfFailsWhenNoLegMatches) {
  auto matcher = Yields(
      AnyOf(IsParseError(), WhenParsed(EqualsBinaryProto(VarintField(1, 2)))));
  TestResult result = Run(kSerializeError);

  EXPECT_THAT(
      Explain(matcher, result),
      Rejects(AllOf(
          HasSubstr("Should have failed to parse, but didn't."),
          HasSubstr("Failed to parse input or produce output."),
          HasSubstr(absl::StrCat("Unexpected failure for test: ", kP0Foo)))));
  EXPECT_YIELDS_FAILURE(EXPECT_THAT(result, matcher), "Unexpected failure");
  EXPECT_EQ(ReplayedCounts(), (Counts{/*expected_successes=*/0,
                                      /*expected_failures=*/0,
                                      /*unexpected_successes=*/0,
                                      /*unexpected_failures=*/1}));
}

TEST_F(YieldsTest, NotComposes) {
  EXPECT_THAT(Run(kPayload2, TestPriority::kP0, "first"),
              Yields(Not(IsParseError())));
  EXPECT_EQ(ReplayedCounts(), (Counts{/*expected_successes=*/1}));

  auto matcher = Yields(Not(IsParseError()));
  TestResult result = Run(kParseError, TestPriority::kP0, "second");
  // Not() has no explanation of its own, so the description is used.
  EXPECT_THAT(Explain(matcher, result),
              Rejects(absl::StrCat(
                  "which doesn't match (is not a parse error)",
                  "\nUnexpected failure for test: ",
                  "Required.Proto2.ProtobufInput.second.ProtobufOutput")));
  EXPECT_YIELDS_FAILURE(EXPECT_THAT(result, matcher),
                        "which doesn't match (is not a parse error)");
  EXPECT_EQ(ReplayedCounts(),
            (Counts{/*expected_successes=*/1, /*expected_failures=*/0,
                    /*unexpected_successes=*/0, /*unexpected_failures=*/1}));
}

TEST_F(YieldsTest, CheckingAPassedResultAgainWithTheSameOutcomePasses) {
  // Whatever the second matcher is, as long as it reaches the same outcome;
  // the test is recorded and counted once.
  auto matcher = Yields(IsParseError());
  TestResult result = Run(kParseError);
  EXPECT_THAT(result, matcher);

  EXPECT_THAT(result, matcher);
  EXPECT_THAT(result, Yields(IsParseError()));
  EXPECT_THAT(result, Yields(Not(WhenParsed(_))));
  EXPECT_EQ(ReplayedCounts(), (Counts{/*expected_successes=*/1}));
}

TEST_F(YieldsTest, CheckingAResultAgainWithADifferentOutcomeFails) {
  TestResult result = Run(kParseError);
  EXPECT_THAT(result, Yields(IsParseError()));

  // The first outcome stays recorded; the ledger never hears of the second.
  EXPECT_YIELDS_FAILURE(
      EXPECT_THAT(result, Yields(Not(IsParseError()))),
      "TestResult for Required.Proto2.ProtobufInput.foo.ProtobufOutput was "
      "already checked, with a different outcome: the first check recorded "
      "\"P0 PASS\", this one would record \"P0 FAIL: which doesn't match (is "
      "not a parse error)\"; each result may be checked once");
  EXPECT_EQ(ReplayedCounts(), (Counts{/*expected_successes=*/1}));
  EXPECT_THAT(ledger_.UnexpectedFailures(), IsEmpty());
}

TEST_F(YieldsTest, CheckingAFailedResultAgainRepeatsTheFailure) {
  // gtest re-evaluates a failing matcher, through the same matcher object, to
  // explain the failure.  That evaluation reaches the same outcome, so it
  // only repeats the verdict, and the ledger hears about the test once.  So
  // does any other matcher that reaches the same outcome; one that doesn't
  // finds the result already checked.
  auto matcher = Yields(IsParseError());
  TestResult result = Run(kPayload2);
  EXPECT_YIELDS_FAILURE(EXPECT_THAT(result, matcher),
                        "Should have failed to parse, but didn't.");

  EXPECT_THAT(Explain(matcher, result),
              Rejects(absl::StrCat(kNotAParseError,
                                   "\nUnexpected failure for test: ", kP0Foo)));
  EXPECT_THAT(Explain(Yields(IsParseError()), result),
              Rejects(absl::StrCat(kNotAParseError,
                                   "\nUnexpected failure for test: ", kP0Foo)));
  EXPECT_THAT(Explain(Yields(WhenParsed(_)), result),
              Rejects(HasSubstr("was already checked, with a different "
                                "outcome: the first check recorded \"P0 FAIL: "
                                "Should have failed to parse, but didn't.\", "
                                "this one would record \"P0 PASS\"")));
  EXPECT_EQ(ReplayedCounts(), (Counts{/*expected_successes=*/0,
                                      /*expected_failures=*/0,
                                      /*unexpected_successes=*/0,
                                      /*unexpected_failures=*/1}));
  EXPECT_THAT(ledger_.UnexpectedFailures(),
              ElementsAre(FieldsAre(kP0Foo, kNotAParseError, absl::nullopt)));
}

TEST_F(YieldsTest, ACopyOfACheckedResultIsAlreadyChecked) {
  TestResult original = Run(kPayload2);
  EXPECT_YIELDS_FAILURE(EXPECT_THAT(original, Yields(IsParseError())),
                        "Should have failed to parse, but didn't.");

  // The record is keyed by the test's name, so a copy of the result counts as
  // checked too: the same outcome is repeated, a different one is refused,
  // and the ledger hears nothing more.
  TestResult copy = original;
  EXPECT_THAT(Explain(Yields(IsParseError()), copy),
              Rejects(HasSubstr("Unexpected failure for test")));
  EXPECT_THAT(Explain(Yields(WhenParsed(_)), copy),
              Rejects(HasSubstr("was already checked")));
  EXPECT_EQ(ReplayedCounts(), (Counts{/*expected_successes=*/0,
                                      /*expected_failures=*/0,
                                      /*unexpected_successes=*/0,
                                      /*unexpected_failures=*/1}));
}

TEST_F(YieldsTest, SameMatcherCanCheckSeveralResults) {
  auto matcher = Yields(IsParseError());
  EXPECT_THAT(Run(kParseError, TestPriority::kP0, "first"), matcher);
  EXPECT_THAT(Run(kParseError, TestPriority::kP0, "second"), matcher);
  EXPECT_YIELDS_FAILURE(
      EXPECT_THAT(Run(kPayload2, TestPriority::kP0, "third"), matcher),
      "Should have failed to parse, but didn't.");
  EXPECT_EQ(ReplayedCounts(),
            (Counts{/*expected_successes=*/2, /*expected_failures=*/0,
                    /*unexpected_successes=*/0, /*unexpected_failures=*/1}));
}

// ---------------------------------------------------------------------------
// The results Yields() records
// ---------------------------------------------------------------------------

// The outcomes recorded so far on the running gtest test, in order, as
// (test name, ResultRecord::ToString()) pairs.
std::vector<std::pair<std::string, std::string>> RecordedResults() {
  return internal::RecordedResults(internal::CurrentGtestResult());
}

TEST_F(YieldsTest, RecordsAPass) {
  EXPECT_THAT(Run(kPayload2),
              Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 2)pb"))));
  EXPECT_THAT(RecordedResults(), ElementsAre(Pair(kP0Foo, "P0 PASS")));
}

TEST_F(YieldsTest, RecordsAFailWithTheLegacyMessage) {
  TestResult result = Run(kPayload2);
  EXPECT_YIELDS_FAILURE(
      EXPECT_THAT(result, Yields(WhenParsed(EqualsTextProto(R"pb(
                    optional_int32: 1
                  )pb")))),
      "modified: optional_int32: 1 -> 2");
  EXPECT_THAT(
      RecordedResults(),
      ElementsAre(Pair(kP0Foo,
                       "P0 FAIL: Output was not equivalent to reference "
                       "message: modified: optional_int32: 1 -> 2")));
}

TEST_F(YieldsTest, RecordsAnExpectedFailureAsAFail) {
  // The record says what the testee did, not whether it was expected.
  AddToFailureList(kP0Foo, kNotAParseError);
  EXPECT_THAT(Run(kPayload2), Yields(IsParseError()));
  EXPECT_THAT(
      RecordedResults(),
      ElementsAre(Pair(kP0Foo, absl::StrCat("P0 FAIL: ", kNotAParseError))));
}

TEST_F(YieldsTest, RecordsAToleratedP1FailureAsAFail) {
  ledger_.set_enforcement_level(kP0);
  EXPECT_THAT(Run(kPayload2, TestPriority::kP1), Yields(IsParseError()));
  EXPECT_THAT(
      RecordedResults(),
      ElementsAre(Pair(kP1Foo, absl::StrCat("P1 FAIL: ", kNotAParseError))));
}

TEST_F(YieldsTest, RecordsARuntimeErrorAsACrash) {
  EXPECT_YIELDS_FAILURE(EXPECT_THAT(Run(kRuntimeError), Yields(WhenParsed(_))),
                        "Failed to parse input or produce output.");
  EXPECT_THAT(RecordedResults(),
              ElementsAre(Pair(kP0Foo,
                               "P0 CRASH: Failed to parse input or produce "
                               "output.")));
}

TEST_F(YieldsTest, RecordsATimeoutAsACrash) {
  EXPECT_YIELDS_FAILURE(EXPECT_THAT(Run(kTimeoutError), Yields(WhenParsed(_))),
                        "Failed to parse input or produce output.");
  EXPECT_THAT(RecordedResults(),
              ElementsAre(Pair(kP0Foo,
                               "P0 CRASH: Failed to parse input or produce "
                               "output.")));
}

TEST_F(YieldsTest, RecordsAnEmptyResponseAsAFail) {
  EXPECT_YIELDS_FAILURE(EXPECT_THAT(Run(""), Yields(WhenParsed(_))),
                        "Response didn't have any field in the Response.");
  EXPECT_THAT(RecordedResults(),
              ElementsAre(Pair(kP0Foo,
                               "P0 FAIL: Response didn't have any field in "
                               "the Response.")));
}

TEST_F(YieldsTest, RecordsASkipWithTheReason) {
  EXPECT_THAT(Run(kSkipped), Yields(_));
  EXPECT_THAT(RecordedResults(),
              ElementsAre(Pair(kP0Foo, "P0 SKIP: not supported")));
}

TEST_F(YieldsTest, RecordsAListedSkipAsASkip) {
  AddToFailureList(kP0Foo, "");
  EXPECT_YIELDS_FAILURE(EXPECT_THAT(Run(kSkipped), Yields(_)),
                        "is in the failure list but was skipped");
  EXPECT_THAT(RecordedResults(),
              ElementsAre(Pair(kP0Foo, "P0 SKIP: not supported")));
}

TEST_F(YieldsTest, RecordsOnceDespiteGtestRetrying) {
  // gtest evaluates a failing matcher a second time to explain the failure,
  // and a later check that reaches the same outcome repeats it.  None of that
  // records a second time.
  auto matcher = Yields(IsParseError());
  TestResult result = Run(kPayload2);
  EXPECT_YIELDS_FAILURE(EXPECT_THAT(result, matcher),
                        "Should have failed to parse, but didn't.");
  EXPECT_YIELDS_FAILURE(EXPECT_THAT(result, matcher),
                        "Should have failed to parse, but didn't.");
  EXPECT_YIELDS_FAILURE(EXPECT_THAT(result, Yields(IsParseError())),
                        "Should have failed to parse, but didn't.");
  EXPECT_THAT(
      RecordedResults(),
      ElementsAre(Pair(kP0Foo, absl::StrCat("P0 FAIL: ", kNotAParseError))));
}

TEST_F(YieldsTest, RecordsEveryResult) {
  auto matcher =
      Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 2)pb")));
  EXPECT_THAT(Run(kPayload2, TestPriority::kP0, "first"), matcher);
  EXPECT_THAT(Run(kSkipped, TestPriority::kP1, "second"), matcher);
  EXPECT_THAT(
      RecordedResults(),
      ElementsAre(
          Pair("Required.Proto2.ProtobufInput.first.ProtobufOutput", "P0 PASS"),
          Pair("Recommended.Proto2.ProtobufInput.second.ProtobufOutput",
               "P1 SKIP: not supported")));
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
