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
#include <type_traits>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest-spi.h>
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
#include "json/value.h"
#include "conformance/binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "conformance/failure_list.h"
#include "conformance/global_test_environment.h"
#include "conformance/mock_test_runner.h"
#include "conformance/result_ledger.h"
#include "conformance/testee.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/test_messages_proto2.pb.h"
#include "google/protobuf/testing/protobuf_test.h"
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
ResultLedger* global_ledger = nullptr;

}  // namespace

const FailureList& GetGlobalFailureList() {
  ABSL_CHECK(global_ledger != nullptr);
  return global_ledger->failure_list();
}

void ReportSuccess(absl::string_view test_name) {
  ABSL_CHECK(global_ledger != nullptr);
  global_ledger->ReportSuccess(test_name).IgnoreError();
}

void ReportFailure(absl::string_view test_name, TestPriority priority,
                   absl::string_view failure_message) {
  ABSL_CHECK(global_ledger != nullptr);
  global_ledger->ReportFailure(test_name, priority, failure_message)
      .IgnoreError();
}

void ReportSkip(absl::string_view test_name, absl::string_view skip_reason) {
  ABSL_CHECK(global_ledger != nullptr);
  global_ledger->ReportSkip(test_name, skip_reason).IgnoreError();
}

}  // namespace internal

namespace {

using ::absl_testing::IsOk;
using ::conformance::ConformanceResponse;
using ::conformance::WireFormat;
using ::google::protobuf::conformance::TestPriority;
using ::google::protobuf::conformance::internal::ResultLedger;
using ::google::protobuf::conformance::internal::TestResult;
using ::google::protobuf::conformance::internal::YieldedResult;
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
using ::testing::SizeIs;
using ::testing::StartsWith;
using ::testing::Truly;
using ::testing::Value;

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

template <class T>
std::string Describe(const testing::Matcher<T>& m) {
  return testing::DescribeMatcher<T>(m, /*negation=*/false);
}

template <class T>
std::string DescribeNegation(const testing::Matcher<T>& m) {
  return testing::DescribeMatcher<T>(m, /*negation=*/true);
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

// The leaf matchers match a YieldedResult; this overload wraps `result` in
// one, so that their tests can pass the TestResult.
testing::AssertionResult Explain(
    const testing::Matcher<const YieldedResult&>& m, const TestResult& result) {
  return Explain(m, YieldedResult(result));
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

TEST(PrintTestResultTest, DecodedProtobufPayloadIsTruncatedToo) {
  // The raw bytes and the text of the message they decode to are both cut
  // after 200 bytes.
  TestAllTypesProto2 message;
  message.set_optional_string(std::string(300, 'a'));
  const std::string payload = message.SerializeAsString();

  EXPECT_EQ(
      PrintToString(ResultWithResponse(
          absl::StrCat("protobuf_payload: \"", absl::CEscape(payload), "\""))),
      absl::StrCat(kPrintedRequiredTest, " with response {protobuf_payload: \"",
                   absl::CEscape(payload.substr(0, 200)),
                   "...(truncated)\"} (decoded: {optional_string: \"",
                   // 200 minus the 18 bytes of `optional_string: "`.
                   std::string(182, 'a'), "...(truncated)})"));
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
// YieldedResult
// ---------------------------------------------------------------------------

// What makes a forgotten Yields() a compile error: the leaf matchers match a
// YieldedResult, and a TestResult does not convert to one.
static_assert(!std::is_convertible_v<const TestResult&, YieldedResult>);
static_assert(!std::is_constructible_v<YieldedResult, TestResult>);

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

class PayloadMatcherErrorResponseTest : public testing::Test {
 protected:
  const absl::string_view response_ = PB_TEST_CHOOSE_VALUE(
      (R"pb(parse_error: "foo")pb", R"pb(serialize_error: "foo")pb",
       R"pb(runtime_error: "foo")pb", R"pb(timeout_error: "foo")pb"),
      {"ParseError", "SerializeError", "RuntimeError", "TimeoutError"});
};

PB_TEST_F(PayloadMatcherErrorResponseTest, IsAFailureForWhenParsed) {
  TestResult result =
      CreateResult("foo", ::conformance::PROTOBUF, ResponseFromText(response_));

  EXPECT_THAT(Explain(WhenParsed(_), result),
              Rejects("Failed to parse input or produce output."));
}

PB_TEST_F(PayloadMatcherErrorResponseTest, IsAFailureForPayload) {
  TestResult result =
      CreateResult("foo", ::conformance::PROTOBUF, ResponseFromText(response_));

  EXPECT_THAT(Explain(RawPayload(Wire()), result),
              Rejects("Failed to parse input or produce output."));
}

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

TEST(WhenParsedTest, JsonOutput) {
  TestResult result = CreateResult(
      "foo", ::conformance::JSON,
      ResponseFromText(R"pb(json_payload: "{\"optionalInt32\": 9}")pb"));

  EXPECT_THAT(
      Explain(WhenParsed(EqualsTextProto(R"pb(optional_int32: 9)pb")), result),
      Accepts(IsEmpty()));
  EXPECT_TRUE(Value(YieldedResult(result),
                    WhenParsed(EqualsBinaryProto(VarintField(1, 9)))));
  EXPECT_THAT(
      Explain(WhenParsed(EqualsTextProto(R"pb(optional_int32: 1)pb")), result),
      Rejects("Output was not equivalent to reference message: "
              "modified: optional_int32: 1 -> 9\n"));
}

TEST(WhenParsedTest, UnparseableJsonPayload) {
  TestResult result = CreateResult(
      "foo", ::conformance::JSON,
      ResponseFromText(R"pb(json_payload: "{\"nonsense\": 1}")pb"));

  EXPECT_THAT(Explain(WhenParsed(_), result),
              Rejects("JSON output we received from test was unparseable."));
  EXPECT_THAT(
      Explain(WhenParsed(_),
              CreateResult("bar", ::conformance::JSON,
                           ResponseFromText(R"pb(json_payload: "{")pb"))),
      Rejects("JSON output we received from test was unparseable."));
}

TEST(WhenParsedTest, JsonOutputIsDecodedWithLegacyLeniency) {
  // The legacy runner decoded JSON output with the default ParseOptions,
  // which tolerate some non-conformant output; so does WhenParsed() until
  // b/563658359 tightens it.  Single-quoted strings are one such case; this
  // test documents the current behaviour rather than endorsing it.
  TestResult result = CreateResult(
      "foo", ::conformance::JSON,
      ResponseFromText(R"pb(json_payload: "{'optionalInt32': 9}")pb"));

  EXPECT_TRUE(Value(YieldedResult(result),
                    WhenParsed(EqualsTextProto(R"pb(optional_int32: 9)pb"))));
}

TEST(WhenParsedTest, TextOutput) {
  TestResult result = CreateResult(
      "foo", ::conformance::TEXT_FORMAT,
      ResponseFromText(R"pb(text_payload: "optional_int32: 9")pb"));

  EXPECT_TRUE(Value(YieldedResult(result),
                    WhenParsed(EqualsTextProto(R"pb(optional_int32: 9)pb"))));
}

TEST(WhenParsedTest, TextOutputFieldNumbersAccepted) {
  // Testees asked to print unknown fields emit them by number, so field
  // numbers are always accepted.
  TestResult result =
      CreateResult("foo", ::conformance::TEXT_FORMAT,
                   ResponseFromText(R"pb(text_payload: "1: 9")pb"));

  EXPECT_TRUE(Value(YieldedResult(result),
                    WhenParsed(EqualsTextProto(R"pb(optional_int32: 9)pb"))));
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

  EXPECT_TRUE(Value(YieldedResult(result), WhenParsed(EqualsTextProto(R"pb(
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
  EXPECT_TRUE(Value(YieldedResult(result),
                    WhenParsed(EqualsBinaryProto(LongVarintField(1, 2, 1)))));
}

TEST(WhenParsedTest, EqualsBinaryProtoWithTextOutput) {
  TestResult result = CreateResult(
      "foo", ::conformance::TEXT_FORMAT,
      ResponseFromText(R"pb(text_payload: "optional_int32: 9")pb"));

  EXPECT_TRUE(Value(YieldedResult(result),
                    WhenParsed(EqualsBinaryProto(VarintField(1, 9)))));
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

  EXPECT_TRUE(Value(YieldedResult(result), WhenParsed(_)));
  EXPECT_TRUE(Value(
      YieldedResult(result),
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

  YieldedResult yielded(result);
  EXPECT_TRUE(Value(yielded, Not(WhenParsed(EqualsTextProto("")))));
  EXPECT_TRUE(Value(
      yielded, AnyOf(IsParseError(),
                     WhenParsed(EqualsTextProto(R"pb(optional_int32: 2)pb")))));
  EXPECT_TRUE(Value(yielded, AllOf(Not(IsParseError()), WhenParsed(_))));
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

  EXPECT_TRUE(Value(YieldedResult(result), RawPayload(VarintField(1, 2))));
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

  EXPECT_TRUE(
      Value(YieldedResult(result), RawPayload(Wire("optional_int32: 9"))));
  // Text output isn't required to be parseable.
  TestResult nonsense =
      CreateResult("bar", ::conformance::TEXT_FORMAT,
                   ResponseFromText(R"pb(text_payload: "nonsense: 1")pb"));
  EXPECT_TRUE(Value(YieldedResult(nonsense), RawPayload(Wire("nonsense: 1"))));
}

TEST(RawPayloadTest, JsonOutput) {
  // Raw payloads don't need to be decoded, so JSON works already.
  TestResult result =
      CreateResult("foo", ::conformance::JSON,
                   ResponseFromText(R"pb(json_payload: "{}")pb"));

  EXPECT_TRUE(Value(YieldedResult(result), RawPayload(Wire("{}"))));
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
TEST(WhenParsedTest, HasUnknownFieldsInOrderKeepsTheLegacyMessage) {
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

  EXPECT_THAT(Explain(WhenParsed(HasUnknownFieldsInOrder(expected)), in_order),
              Accepts(IsEmpty()));
  EXPECT_THAT(Explain(WhenParsed(HasUnknownFieldsInOrder(expected)), reordered),
              Rejects("Unknown field mismatch"));
}

TEST(WhenParsedAsTest, HasUnknownFieldsInOrderSeesTheGivenTypesUnknowns) {
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

  EXPECT_THAT(Explain(WhenParsed(HasUnknownFieldsInOrder(both)), result),
              Accepts(IsEmpty()));
  EXPECT_THAT(Explain(WhenParsedAs<UnknownToTestAllTypes>(
                          HasUnknownFieldsInOrder(only_666)),
                      result),
              Accepts(IsEmpty()));
  EXPECT_THAT(Explain(WhenParsedAs<UnknownToTestAllTypes>(
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
  EXPECT_FALSE(Value(YieldedResult(result), IsSerializeError()));
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
  EXPECT_FALSE(Value(YieldedResult(result), IsParseError()));
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

  EXPECT_TRUE(Value(YieldedResult(result), Not(IsSerializeError())));
  EXPECT_TRUE(
      Value(YieldedResult(result), AnyOf(IsSerializeError(), IsParseError())));
  EXPECT_FALSE(
      Value(YieldedResult(result), AllOf(IsSerializeError(), IsParseError())));
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
  YieldsTest() { internal::global_ledger = &ledger_; }
  ~YieldsTest() override {
    ledger_.Finalize().IgnoreError();
    internal::global_ledger = nullptr;
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
  EXPECT_EQ(GetCounts(ledger_), (Counts{/*expected_successes=*/2}));
  EXPECT_THAT(ledger_.Finalize(), IsOk());
}

TEST_F(YieldsTest, InnerMatcherSeesTheResultAsAYieldedResult) {
  const TestResult* seen = nullptr;
  auto matcher = Yields(Truly([&](const YieldedResult& yielded) {
    seen = &yielded.result();
    return true;
  }));
  TestResult result = Run(kPayload2);

  EXPECT_THAT(Explain(matcher, result), Accepts(IsEmpty()));
  EXPECT_EQ(seen, &result);
}

TEST_F(YieldsTest, UnlistedFailureFails) {
  auto matcher =
      Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 1)pb")));
  TestResult result = Run(kPayload2);

  EXPECT_THAT(Explain(matcher, result),
              Rejects(absl::StrCat(kMismatch1,
                                   "\nUnexpected failure for test: ", kP0Foo)));
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(result, matcher),
                          "modified: optional_int32: 1 -> 2");
  EXPECT_EQ(GetCounts(ledger_), (Counts{/*expected_successes=*/0,
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
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(Run(kPayload2), Yields(IsParseError())),
                          "Should have failed to parse, but didn't.");
  EXPECT_EQ(GetCounts(ledger_), (Counts{/*expected_successes=*/0,
                                        /*expected_failures=*/0,
                                        /*unexpected_successes=*/0,
                                        /*unexpected_failures=*/1}));
  EXPECT_THAT(ledger_.UnexpectedFailures(),
              ElementsAre(FieldsAre(kP0Foo, kNotAParseError, absl::nullopt)));
}

TEST_F(YieldsTest, PayloadBytesMismatchRecordsTheLegacyOctalMessage) {
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(Run(kPayload2), Yields(RawPayload(VarintField(1, 1)))),
      "Output was not equivalent to reference message: "
      "Expect: \\010\\001, but got: \\010\\002");
  EXPECT_EQ(GetCounts(ledger_), (Counts{/*expected_successes=*/0,
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
  EXPECT_EQ(GetCounts(ledger_), (Counts{/*expected_successes=*/2}));

  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(
          RunText(kText2, TestPriority::kP0, "mismatch"),
          Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 1)pb")))),
      "modified: optional_int32: 1 -> 2");
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(RunText(R"pb(text_payload: "nonsense: 1")pb",
                          TestPriority::kP0, "unparseable"),
                  Yields(WhenParsed(_))),
      "TEXT_FORMAT output we received from test was unparseable.");
  EXPECT_EQ(GetCounts(ledger_),
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
  EXPECT_EQ(GetCounts(ledger_), (Counts{/*expected_successes=*/0,
                                        /*expected_failures=*/1}));
  EXPECT_THAT(ledger_.UnexpectedFailures(), IsEmpty());
  EXPECT_THAT(ledger_.Finalize(), IsOk());
}

TEST_F(YieldsTest, ListedFailureWithMessagePrefixPasses) {
  AddToFailureList(kP0Foo, "Output was not equivalent to reference");

  EXPECT_THAT(Run(kPayload2),
              Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 1)pb"))));
  EXPECT_EQ(GetCounts(ledger_), (Counts{/*expected_successes=*/0,
                                        /*expected_failures=*/1}));
  EXPECT_THAT(ledger_.Finalize(), IsOk());
}

TEST_F(YieldsTest, ListedFailureViaWildcardPasses) {
  AddToFailureList("Required.*.ProtobufInput.foo.ProtobufOutput",
                   kNotAParseError);

  EXPECT_THAT(Run(kPayload2), Yields(IsParseError()));
  EXPECT_EQ(GetCounts(ledger_), (Counts{/*expected_successes=*/0,
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
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(result, matcher),
                          "Unexpected failure message for test");
  EXPECT_EQ(GetCounts(ledger_), (Counts{/*expected_successes=*/0,
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
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(result, matcher),
                          "is in the failure list, but test succeeded");
  EXPECT_EQ(GetCounts(ledger_), (Counts{/*expected_successes=*/0,
                                        /*expected_failures=*/0,
                                        /*unexpected_successes=*/1}));
  EXPECT_THAT(
      ledger_.UnexpectedSuccesses(),
      ElementsAre(FieldsAre(kP0Foo, kNotAParseError, Optional(kP0Foo))));
}

TEST_F(YieldsTest, ListedSuccessViaWildcardNamesTheWildcard) {
  AddToFailureList("Required.*.ProtobufInput.foo.ProtobufOutput", "");

  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(Run(kParseError), Yields(IsParseError())),
      "test Required.Proto2.ProtobufInput.foo.ProtobufOutput (matched to "
      "Required.*.ProtobufInput.foo.ProtobufOutput) is in the failure list, "
      "but test succeeded.  Remove its match from the failure list.");
  EXPECT_EQ(GetCounts(ledger_), (Counts{/*expected_successes=*/0,
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
  EXPECT_EQ(GetCounts(ledger_),
            (Counts{/*expected_successes=*/0, /*expected_failures=*/0,
                    /*unexpected_successes=*/0, /*unexpected_failures=*/0,
                    /*skipped=*/0, /*listed_skips=*/0,
                    /*tolerated_failures=*/1}));
  EXPECT_THAT(ledger_.UnexpectedFailures(), IsEmpty());
  EXPECT_THAT(ledger_.Finalize(), IsOk());
}

TEST_F(YieldsTest, P1FailureFailsWhenEnforced) {
  ledger_.set_enforcement_level(kP1);

  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(Run(kPayload2, TestPriority::kP1), Yields(IsParseError())),
      "Should have failed to parse, but didn't.");
  EXPECT_EQ(GetCounts(ledger_), (Counts{/*expected_successes=*/0,
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
  EXPECT_EQ(GetCounts(ledger_), (Counts{/*expected_successes=*/0,
                                        /*expected_failures=*/1}));
  EXPECT_THAT(ledger_.Finalize(), IsOk());
}

TEST_F(YieldsTest,
       ListedP1FailureWithDifferentMessageFailsEvenWhenNotEnforced) {
  ledger_.set_enforcement_level(kP0);
  AddToFailureList(kP1Foo, "Some other message");

  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(Run(kPayload2, TestPriority::kP1), Yields(IsParseError())),
      "Unexpected failure message for test");
  EXPECT_EQ(GetCounts(ledger_), (Counts{/*expected_successes=*/0,
                                        /*expected_failures=*/0,
                                        /*unexpected_successes=*/0,
                                        /*unexpected_failures=*/1}));
}

TEST_F(YieldsTest, P1SuccessPasses) {
  ledger_.set_enforcement_level(kP0);

  EXPECT_THAT(Run(kParseError, TestPriority::kP1), Yields(IsParseError()));
  EXPECT_EQ(GetCounts(ledger_), (Counts{/*expected_successes=*/1}));
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
  auto matcher = Yields(Truly([&](const YieldedResult&) {
    inner_called = true;
    return false;
  }));
  TestResult result = Run(kSkipped);

  EXPECT_THAT(Explain(matcher, result), Accepts(IsEmpty()));
  EXPECT_FALSE(inner_called);
  EXPECT_EQ(GetCounts(ledger_), (Counts{/*expected_successes=*/0,
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
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(result, matcher),
                          "is in the failure list but was skipped");
  // The ledger records it as a listed skip, not as an unexpected failure.
  EXPECT_EQ(GetCounts(ledger_),
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
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(Run(kRuntimeError), Yields(IsParseError())),
      "Should have failed to parse, but raised an error instead.");
  EXPECT_EQ(GetCounts(ledger_), (Counts{/*expected_successes=*/0,
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
  EXPECT_EQ(GetCounts(ledger_), (Counts{/*expected_successes=*/0,
                                        /*expected_failures=*/1}));
  EXPECT_THAT(ledger_.Finalize(), IsOk());
}

TEST_F(YieldsTest, AnyOfMatchesEitherLeg) {
  auto matcher = Yields(
      AnyOf(IsParseError(), WhenParsed(EqualsBinaryProto(VarintField(1, 2)))));

  EXPECT_THAT(Run(kParseError, TestPriority::kP0, "first"), matcher);
  EXPECT_THAT(Run(kPayload2, TestPriority::kP0, "second"), matcher);
  EXPECT_EQ(GetCounts(ledger_), (Counts{/*expected_successes=*/2}));
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
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(result, matcher), "Unexpected failure");
  EXPECT_EQ(GetCounts(ledger_), (Counts{/*expected_successes=*/0,
                                        /*expected_failures=*/0,
                                        /*unexpected_successes=*/0,
                                        /*unexpected_failures=*/1}));
}

TEST_F(YieldsTest, NotComposes) {
  EXPECT_THAT(Run(kPayload2, TestPriority::kP0, "first"),
              Yields(Not(IsParseError())));
  EXPECT_EQ(GetCounts(ledger_), (Counts{/*expected_successes=*/1}));

  auto matcher = Yields(Not(IsParseError()));
  TestResult result = Run(kParseError, TestPriority::kP0, "second");
  // Not() has no explanation of its own, so the description is used.
  EXPECT_THAT(Explain(matcher, result),
              Rejects(absl::StrCat(
                  "which doesn't match (is not a parse error)",
                  "\nUnexpected failure for test: ",
                  "Required.Proto2.ProtobufInput.second.ProtobufOutput")));
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(result, matcher),
                          "which doesn't match (is not a parse error)");
  EXPECT_EQ(GetCounts(ledger_),
            (Counts{/*expected_successes=*/1, /*expected_failures=*/0,
                    /*unexpected_successes=*/0, /*unexpected_failures=*/1}));
}

TEST_F(YieldsTest, CheckingAPassedResultAgainIsJudgedButNotCounted) {
  // Whatever the second matcher is, including the very same object.
  auto matcher = Yields(IsParseError());
  TestResult result = Run(kParseError);
  EXPECT_THAT(result, matcher);

  EXPECT_THAT(result, matcher);
  EXPECT_THAT(result, Yields(IsParseError()));
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(result, Yields(Not(IsParseError()))),
                          "which doesn't match (is not a parse error)");
  // The first check is the one that counts.
  EXPECT_EQ(GetCounts(ledger_), (Counts{/*expected_successes=*/1}));
  EXPECT_THAT(ledger_.UnexpectedFailures(), IsEmpty());
}

TEST_F(YieldsTest, CheckingAFailedResultAgainRepeatsTheFailure) {
  // gtest re-evaluates a failing matcher, through the same matcher object, to
  // explain the failure.  That evaluation reaches the same verdict, and the
  // ledger hears about the test once.  So does any other matcher: a second
  // check is judged on its own but never counted.
  auto matcher = Yields(IsParseError());
  TestResult result = Run(kPayload2);
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(result, matcher),
                          "Should have failed to parse, but didn't.");

  EXPECT_THAT(Explain(matcher, result),
              Rejects(absl::StrCat(kNotAParseError,
                                   "\nUnexpected failure for test: ", kP0Foo)));
  EXPECT_THAT(Explain(Yields(IsParseError()), result),
              Rejects(absl::StrCat(kNotAParseError,
                                   "\nUnexpected failure for test: ", kP0Foo)));
  EXPECT_THAT(Explain(Yields(WhenParsed(_)), result), Accepts(IsEmpty()));
  EXPECT_EQ(GetCounts(ledger_), (Counts{/*expected_successes=*/0,
                                        /*expected_failures=*/0,
                                        /*unexpected_successes=*/0,
                                        /*unexpected_failures=*/1}));
  EXPECT_THAT(ledger_.UnexpectedFailures(),
              ElementsAre(FieldsAre(kP0Foo, kNotAParseError, absl::nullopt)));
}

TEST_F(YieldsTest, ACopyOfACheckedResultIsNotCountedAgain) {
  TestResult original = Run(kPayload2);
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(original, Yields(IsParseError())),
                          "Should have failed to parse, but didn't.");

  // The ledger knows the test by name, so a copy of the result counts as
  // checked too: a check of it is judged on its own and the ledger hears
  // nothing more.
  TestResult copy = original;
  EXPECT_THAT(Explain(Yields(IsParseError()), copy),
              Rejects(HasSubstr("Unexpected failure for test")));
  EXPECT_THAT(Explain(Yields(WhenParsed(_)), copy), Accepts(IsEmpty()));
  EXPECT_EQ(GetCounts(ledger_), (Counts{/*expected_successes=*/0,
                                        /*expected_failures=*/0,
                                        /*unexpected_successes=*/0,
                                        /*unexpected_failures=*/1}));
}

TEST_F(YieldsTest, SameMatcherCanCheckSeveralResults) {
  auto matcher = Yields(IsParseError());
  EXPECT_THAT(Run(kParseError, TestPriority::kP0, "first"), matcher);
  EXPECT_THAT(Run(kParseError, TestPriority::kP0, "second"), matcher);
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(Run(kPayload2, TestPriority::kP0, "third"), matcher),
      "Should have failed to parse, but didn't.");
  EXPECT_EQ(GetCounts(ledger_),
            (Counts{/*expected_successes=*/2, /*expected_failures=*/0,
                    /*unexpected_successes=*/0, /*unexpected_failures=*/1}));
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
  return CreateResult("foo", /*input_format=*/::conformance::JSON,
                      /*output_format=*/::conformance::JSON,
                      std::move(response));
}

constexpr absl::string_view kJsonInt32 = R"json({"optionalInt32": 9})json";

TEST(JsonPayloadTest, Description) {
  EXPECT_EQ(Describe(JsonPayload(HasJsonMember("optionalInt32"))),
            R"(JSON payload has member "optionalInt32")");
  EXPECT_EQ(DescribeNegation(JsonPayload(HasJsonMember("optionalInt32"))),
            R"(JSON payload doesn't have member "optionalInt32")");
  EXPECT_EQ(Describe(JsonPayload(_)), "JSON payload is anything");
}

TEST(JsonPayloadTest, Matches) {
  TestResult result = JsonResult(kJsonInt32);

  EXPECT_THAT(Explain(JsonPayload(HasJsonMember("optionalInt32")), result),
              Accepts(IsEmpty()));
  YieldedResult yielded(result);
  EXPECT_TRUE(Value(yielded, JsonPayload(_)));
  EXPECT_TRUE(Value(yielded, JsonPayload(Truly([](const Json::Value& value) {
                      return value["optionalInt32"].asInt() == 9;
                    }))));
  EXPECT_TRUE(Value(yielded, JsonPayload(SizeIs(1))));
}

TEST(JsonPayloadTest, InnerMatcherWithoutExplanation) {
  // The description and the actual JSON text, quoted and escaped like
  // RawPayload() does, are reported instead.
  TestResult result = JsonResult(kJsonInt32);

  EXPECT_THAT(Explain(JsonPayload(HasJsonMember("optionalInt64")), result),
              Rejects("Expect: JSON payload has member \"optionalInt64\", "
                      "but got: \"{\\\"optionalInt32\\\": 9}\""));
}

TEST(JsonPayloadTest, InnerMatcherExplanationIsUsed) {
  TestResult result = JsonResult(kJsonInt32);

  EXPECT_THAT(Explain(JsonPayload(SizeIs(2)), result),
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

  EXPECT_THAT(
      Explain(JsonPayload(_), result),
      Rejects(StartsWith("JSON payload cannot be parsed as valid JSON: ")));
  EXPECT_THAT(
      Explain(JsonPayload(_), JsonResult("")),
      Rejects(StartsWith("JSON payload cannot be parsed as valid JSON: ")));
  EXPECT_THAT(
      Explain(JsonPayload(_), JsonResult("nonsense")),
      Rejects(StartsWith("JSON payload cannot be parsed as valid JSON: ")));
  // jsoncpp's defaults would accept these; the matcher parses strictly.
  EXPECT_THAT(
      Explain(JsonPayload(_), JsonResult("{\"optionalInt32\": 1,}")),
      Rejects(StartsWith("JSON payload cannot be parsed as valid JSON: ")));
  EXPECT_THAT(
      Explain(JsonPayload(_), JsonResult("{} trailing")),
      Rejects(StartsWith("JSON payload cannot be parsed as valid JSON: ")));
}

TEST(JsonPayloadTest, ParsesArraysAsWellAsObjects) {
  // A testee's answer is a message, so the root is normally an object, but
  // jsoncpp's strict mode also takes an array; whether it is acceptable is up
  // to the inner matcher.  A scalar root is rejected like other non-documents.
  TestResult array_result = JsonResult("[1, 2]");
  EXPECT_TRUE(Value(YieldedResult(array_result), JsonPayload(SizeIs(2))));
  EXPECT_THAT(
      Explain(JsonPayload(_), JsonResult("null")),
      Rejects(StartsWith("JSON payload cannot be parsed as valid JSON: ")));
}

TEST(JsonPayloadTest, EmptyResponse) {
  TestResult result =
      CreateResult("foo", ::conformance::JSON, ConformanceResponse());

  EXPECT_THAT(Explain(JsonPayload(_), result),
              Rejects("Response didn't have any field in the Response."));
}

PB_TEST_F(PayloadMatcherErrorResponseTest, IsAFailureForJsonPayload) {
  TestResult result =
      CreateResult("foo", ::conformance::JSON, ResponseFromText(response_));

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

  YieldedResult yielded(result);
  EXPECT_TRUE(Value(yielded, Not(JsonPayload(HasJsonMember("optionalInt64")))));
  EXPECT_TRUE(Value(
      yielded,
      AnyOf(IsParseError(), JsonPayload(HasJsonMember("optionalInt32")))));
  EXPECT_TRUE(Value(
      yielded, AllOf(JsonPayload(HasJsonMember("optionalInt32")),
                     WhenParsed(EqualsTextProto(R"pb(optional_int32: 9)pb")))));
}

TEST_F(YieldsTest, JsonOutput) {
  // JSON-input, JSON-output tests through the whole stack: the legacy
  // messages are what gets recorded for the failure list.
  auto run_json = [&](absl::string_view response, absl::string_view name) {
    RespondWith(response);
    return testee_.CreateTest(name, TestPriority::kP0)
        .ParseJson(TestAllTypesProto2::descriptor(), "{}")
        .SerializeJson();
  };
  constexpr absl::string_view kJsonResponse =
      R"pb(json_payload: "{\"optionalInt32\": 9}")pb";
  // Kept out of the EXPECT_NONFATAL_FAILURE() arguments below: MSVC's
  // traditional preprocessor mis-scans raw string literals containing \" or
  // an odd number of quotes when they appear inside macro arguments.
  constexpr absl::string_view kUnparseableJsonResponse =
      R"pb(json_payload: "{\"nonsense\": 1}")pb";
  constexpr absl::string_view kInvalidJsonResponse = R"pb(json_payload: "{")pb";

  EXPECT_THAT(run_json(kJsonResponse, "parsed"),
              Yields(WhenParsed(EqualsTextProto(R"pb(optional_int32: 9)pb"))));
  EXPECT_THAT(run_json(kJsonResponse, "validated"),
              Yields(JsonPayload(HasJsonMember("optionalInt32"))));
  EXPECT_EQ(GetCounts(ledger_), (Counts{/*expected_successes=*/2}));

  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(run_json(kUnparseableJsonResponse, "unparseable"),
                  Yields(WhenParsed(_))),
      "JSON output we received from test was unparseable.");
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(run_json(kInvalidJsonResponse, "invalid"),
                                      Yields(JsonPayload(_))),
                          "JSON payload cannot be parsed as valid JSON: ");
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(run_json(kJsonResponse, "mismatch"),
                  Yields(JsonPayload(HasJsonMember("optionalInt64")))),
      "Expect: JSON payload has member \"optionalInt64\", but got: "
      "\"{\\\"optionalInt32\\\": 9}\"");
  EXPECT_EQ(GetCounts(ledger_),
            (Counts{/*expected_successes=*/2, /*expected_failures=*/0,
                    /*unexpected_successes=*/0, /*unexpected_failures=*/3}));
  EXPECT_THAT(
      ledger_.UnexpectedFailures(),
      ElementsAre(
          FieldsAre("Required.Proto2.JsonInput.invalid.JsonOutput",
                    StartsWith("JSON payload cannot be parsed as valid JSON: "),
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
