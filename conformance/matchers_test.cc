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
#include "conformance/binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "conformance/global_test_environment.h"
#include "conformance/mock_test_runner.h"
#include "conformance/test_manager.h"
#include "conformance/test_runner.h"
#include "conformance/testee.h"
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
TestManager* global_test_manager = nullptr;

}  // namespace

TestManager& GetGlobalTestManager() {
  ABSL_CHECK(global_test_manager != nullptr);
  return *global_test_manager;
}

}  // namespace internal

namespace {

using ::absl_testing::IsOk;
using ::conformance::ConformanceResponse;
using ::conformance::WireFormat;
using ::google::protobuf::conformance::TestPriority;
using ::google::protobuf::conformance::internal::TestManager;
using ::google::protobuf::conformance::internal::TestResult;
using ::protobuf_test_messages::proto2::TestAllTypesProto2;
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
// its format, so the input is empty.  These tests only inspect the result, so
// it is marked checked right away.
//
// The leaf matchers never touch the TestManager; the tests make sure of it by
// not installing one at all while they run.  Only YieldsTest below installs
// one.
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

// ---------------------------------------------------------------------------
// Yields()
// ---------------------------------------------------------------------------

// A snapshot of the TestManager counters, for concise assertions.
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

Counts GetCounts(const TestManager& manager) {
  return Counts{
      /*expected_successes=*/manager.expected_successes(),
      /*expected_failures=*/manager.expected_failures(),
      /*unexpected_successes=*/manager.unexpected_successes(),
      /*unexpected_failures=*/manager.unexpected_failures(),
      /*skipped=*/manager.skipped(),
      /*listed_skips=*/manager.listed_skips(),
      /*tolerated_failures=*/
      manager.tolerated_failures(),
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
  YieldsTest() { internal::global_test_manager = &test_manager_; }
  ~YieldsTest() override {
    test_manager_.Finalize().IgnoreError();
    internal::global_test_manager = nullptr;
  }

  // Makes the testee answer every test with `response`.
  void RespondWith(absl::string_view response) {
    ON_CALL(runner_, RunTest)
        .WillByDefault(Return(ResponseFromText(response).SerializeAsString()));
  }

  // Runs the binary-to-binary test `name` (of the given priority) against a
  // testee answering `response`.  Every result this produces must be checked
  // by the test, or the result's destructor fails it.
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
    ASSERT_THAT(test_manager_.LoadFailureList(WriteTempFile(
                    absl::StrCat("failure_list_", ++failure_lists_, ".txt"),
                    absl::StrCat(test_name, " # ", message, "\n"))),
                IsOk());
  }

  TestManager test_manager_;
  NiceMock<MockTestRunner> runner_;
  internal::Testee testee_{&runner_};
  int failure_lists_ = 0;
};

TEST_F(YieldsTest, UnlistedSuccessPasses) {
  auto matcher = Yields(ParsedPayload(EqualsTextProto("optional_int32: 2")));

  EXPECT_THAT(Explain(matcher, Run(kPayload2)), Accepts(IsEmpty()));
  EXPECT_THAT(Run(kPayload2, TestPriority::kP0, "other"), matcher);
  EXPECT_EQ(GetCounts(test_manager_), (Counts{/*expected_successes=*/2}));
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
  EXPECT_EQ(GetCounts(test_manager_), (Counts{/*expected_successes=*/0,
                                              /*expected_failures=*/0,
                                              /*unexpected_successes=*/0,
                                              /*unexpected_failures=*/1}));
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
  EXPECT_EQ(GetCounts(test_manager_), (Counts{/*expected_successes=*/0,
                                              /*expected_failures=*/0,
                                              /*unexpected_successes=*/0,
                                              /*unexpected_failures=*/1}));
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
    TestResult first = Run(kPayload2, TestPriority::kP0, "first");
    EXPECT_NONFATAL_FAILURE(EXPECT_THAT(first, first_matcher),
                            "Should have failed to parse, but didn't.");
  }
  TestResult second = Run(kParseError, TestPriority::kP0, "second");
  EXPECT_THAT(second, Yields(IsParseError()));

  EXPECT_THAT(Explain(first_matcher, second),
              Rejects(HasSubstr("was already checked")));
  EXPECT_EQ(GetCounts(test_manager_),
            (Counts{/*expected_successes=*/1, /*expected_failures=*/0,
                    /*unexpected_successes=*/0, /*unexpected_failures=*/1}));
}

TEST_F(YieldsTest, PayloadBytesMismatchRecordsTheLegacyOctalMessage) {
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(Run(kPayload2), Yields(Payload(VarintField(1, 1)))),
      "Output was not equivalent to reference message: "
      "Expect: \\010\\001, but got: \\010\\002");
  EXPECT_EQ(GetCounts(test_manager_), (Counts{/*expected_successes=*/0,
                                              /*expected_failures=*/0,
                                              /*unexpected_successes=*/0,
                                              /*unexpected_failures=*/1}));
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
  EXPECT_THAT(RunText(kText2, TestPriority::kP0, "raw"),
              Yields(Payload(Wire("optional_int32: 2"))));
  EXPECT_EQ(GetCounts(test_manager_), (Counts{/*expected_successes=*/2}));

  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(RunText(kText2, TestPriority::kP0, "mismatch"),
                  Yields(ParsedPayload(EqualsTextProto("optional_int32: 1")))),
      "modified: optional_int32: 1 -> 2");
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(RunText(R"pb(text_payload: "nonsense: 1")pb",
                          TestPriority::kP0, "unparseable"),
                  Yields(ParsedPayload(_))),
      "TEXT_FORMAT output we received from test was unparseable.");
  EXPECT_EQ(GetCounts(test_manager_),
            (Counts{/*expected_successes=*/2, /*expected_failures=*/0,
                    /*unexpected_successes=*/0, /*unexpected_failures=*/2}));
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
  AddToFailureList(kRequiredFoo, kMismatch1);
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
              Accepts(absl::StrCat(
                  "which failed, but the failure is expected or tolerated: ",
                  kMismatch1)));
  EXPECT_EQ(GetCounts(test_manager_), (Counts{/*expected_successes=*/0,
                                              /*expected_failures=*/1}));
  EXPECT_THAT(test_manager_.UnexpectedFailures(), IsEmpty());
  EXPECT_THAT(test_manager_.Finalize(), IsOk());
}

TEST_F(YieldsTest, ListedFailureWithMessagePrefixPasses) {
  AddToFailureList(kRequiredFoo, "Output was not equivalent to reference");

  EXPECT_THAT(Run(kPayload2),
              Yields(ParsedPayload(EqualsTextProto("optional_int32: 1"))));
  EXPECT_EQ(GetCounts(test_manager_), (Counts{/*expected_successes=*/0,
                                              /*expected_failures=*/1}));
  EXPECT_THAT(test_manager_.Finalize(), IsOk());
}

TEST_F(YieldsTest, ListedFailureViaWildcardPasses) {
  AddToFailureList("Required.*.ProtobufInput.foo.ProtobufOutput",
                   kNotAParseError);

  EXPECT_THAT(Run(kPayload2), Yields(IsParseError()));
  EXPECT_EQ(GetCounts(test_manager_), (Counts{/*expected_successes=*/0,
                                              /*expected_failures=*/1}));
  EXPECT_THAT(test_manager_.Finalize(), IsOk());
}

TEST_F(YieldsTest, ListedFailureWithDifferentMessageFails) {
  AddToFailureList(kRequiredFoo, "Some other message");
  auto matcher = Yields(IsParseError());
  TestResult result = Run(kPayload2);

  EXPECT_THAT(Explain(matcher, result),
              Rejects(absl::StrCat(
                  kNotAParseError,
                  "\nUnexpected failure message for test: ", kRequiredFoo,
                  " expected: Some other message actual: ", kNotAParseError)));
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(result, matcher),
                          "Unexpected failure message for test");
  EXPECT_EQ(GetCounts(test_manager_), (Counts{/*expected_successes=*/0,
                                              /*expected_failures=*/0,
                                              /*unexpected_successes=*/0,
                                              /*unexpected_failures=*/1}));
  EXPECT_THAT(
      test_manager_.UnexpectedFailures(),
      ElementsAre(FieldsAre(kRequiredFoo, kNotAParseError, absl::nullopt)));
}

TEST_F(YieldsTest, ListedSuccessFails) {
  AddToFailureList(kRequiredFoo, kNotAParseError);
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
  EXPECT_EQ(GetCounts(test_manager_), (Counts{/*expected_successes=*/0,
                                              /*expected_failures=*/0,
                                              /*unexpected_successes=*/1}));
  EXPECT_THAT(test_manager_.UnexpectedSuccesses(),
              ElementsAre(FieldsAre(kRequiredFoo, kNotAParseError,
                                    testing::Optional(kRequiredFoo))));
}

TEST_F(YieldsTest, ListedSuccessViaWildcardNamesTheWildcard) {
  AddToFailureList("Required.*.ProtobufInput.foo.ProtobufOutput", "");

  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(Run(kParseError), Yields(IsParseError())),
      "test Required.Proto2.ProtobufInput.foo.ProtobufOutput (matched to "
      "Required.*.ProtobufInput.foo.ProtobufOutput) is in the failure list, "
      "but test succeeded.  Remove its match from the failure list.");
  EXPECT_EQ(GetCounts(test_manager_), (Counts{/*expected_successes=*/0,
                                              /*expected_failures=*/0,
                                              /*unexpected_successes=*/1}));
}

TEST_F(YieldsTest, RecommendedFailureIsToleratedWhenNotEnforced) {
  test_manager_.set_enforcement_level(0);
  absl::ScopedMockLog log;
  EXPECT_CALL(log, Log).Times(AnyNumber());
  EXPECT_CALL(log, Log(absl::LogSeverity::kWarning, _,
                       Eq(absl::StrCat("WARNING, test=", kRecommendedFoo, ": ",
                                       kNotAParseError))))
      .Times(1);
  log.StartCapturingLogs();

  auto matcher = Yields(IsParseError());
  TestResult result = Run(kPayload2, TestPriority::kP1);

  EXPECT_THAT(Explain(matcher, result),
              Accepts(absl::StrCat(
                  "which failed, but the failure is expected or tolerated: ",
                  kNotAParseError)));
  // It's neither a skip nor a failure.
  EXPECT_EQ(GetCounts(test_manager_),
            (Counts{/*expected_successes=*/0, /*expected_failures=*/0,
                    /*unexpected_successes=*/0, /*unexpected_failures=*/0,
                    /*skipped=*/0, /*listed_skips=*/0,
                    /*tolerated_failures=*/1}));
  EXPECT_THAT(test_manager_.UnexpectedFailures(), IsEmpty());
  EXPECT_THAT(test_manager_.Finalize(), IsOk());
}

TEST_F(YieldsTest, RecommendedFailureFailsWhenEnforced) {
  test_manager_.set_enforcement_level(1);

  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(Run(kPayload2, TestPriority::kP1), Yields(IsParseError())),
      "Should have failed to parse, but didn't.");
  EXPECT_EQ(GetCounts(test_manager_), (Counts{/*expected_successes=*/0,
                                              /*expected_failures=*/0,
                                              /*unexpected_successes=*/0,
                                              /*unexpected_failures=*/1}));
  EXPECT_THAT(
      test_manager_.UnexpectedFailures(),
      ElementsAre(FieldsAre(kRecommendedFoo, kNotAParseError, absl::nullopt)));
}

TEST_F(YieldsTest, ListedRecommendedFailureIsTrackedEvenWhenNotEnforced) {
  // A kP1 test that's already in the failure list keeps being tracked
  // there, so that the list can't go stale unnoticed.
  test_manager_.set_enforcement_level(0);
  AddToFailureList(kRecommendedFoo, kNotAParseError);

  EXPECT_THAT(Run(kPayload2, TestPriority::kP1), Yields(IsParseError()));
  EXPECT_EQ(GetCounts(test_manager_), (Counts{/*expected_successes=*/0,
                                              /*expected_failures=*/1}));
  EXPECT_THAT(test_manager_.Finalize(), IsOk());
}

TEST_F(YieldsTest,
       ListedRecommendedFailureWithDifferentMessageFailsEvenWhenNotEnforced) {
  test_manager_.set_enforcement_level(0);
  AddToFailureList(kRecommendedFoo, "Some other message");

  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(Run(kPayload2, TestPriority::kP1), Yields(IsParseError())),
      "Unexpected failure message for test");
  EXPECT_EQ(GetCounts(test_manager_), (Counts{/*expected_successes=*/0,
                                              /*expected_failures=*/0,
                                              /*unexpected_successes=*/0,
                                              /*unexpected_failures=*/1}));
}

TEST_F(YieldsTest, RecommendedSuccessPasses) {
  test_manager_.set_enforcement_level(0);

  EXPECT_THAT(Run(kParseError, TestPriority::kP1), Yields(IsParseError()));
  EXPECT_EQ(GetCounts(test_manager_), (Counts{/*expected_successes=*/1}));
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
  EXPECT_EQ(GetCounts(test_manager_), (Counts{/*expected_successes=*/0,
                                              /*expected_failures=*/0,
                                              /*unexpected_successes=*/0,
                                              /*unexpected_failures=*/0,
                                              /*skipped=*/1}));
  EXPECT_THAT(test_manager_.Finalize(), IsOk());
}

TEST_F(YieldsTest, NotSelectedByRunnerPassesSilently) {
  // A filtering runner answers tests that weren't selected with this exact
  // skip reason; that isn't a testee skip and must not be logged or counted.
  AddToFailureList(kRequiredFoo, "");
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
  EXPECT_THAT(test_manager_.Finalize().message(),
              HasSubstr(absl::StrCat("were not seen: ", kRequiredFoo)));
  EXPECT_THAT(test_manager_.UnmatchedExpectedFailures(), IsEmpty());

  // An unlisted one likewise.
  EXPECT_THAT(Run(response, TestPriority::kP0, "other"), matcher);
  EXPECT_EQ(GetCounts(test_manager_), Counts{});
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
            (Counts{/*expected_successes=*/0, /*expected_failures=*/0,
                    /*unexpected_successes=*/0, /*unexpected_failures=*/0,
                    /*skipped=*/1, /*listed_skips=*/1}));
  EXPECT_THAT(test_manager_.ListedSkips(),
              ElementsAre(Pair(kRequiredFoo, kRequiredFoo)));
  EXPECT_THAT(test_manager_.UnexpectedFailures(), IsEmpty());
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
  EXPECT_EQ(GetCounts(test_manager_), (Counts{/*expected_successes=*/0,
                                              /*expected_failures=*/0,
                                              /*unexpected_successes=*/0,
                                              /*unexpected_failures=*/1}));
  EXPECT_THAT(test_manager_.UnexpectedFailures(),
              ElementsAre(FieldsAre(kRequiredFoo,
                                    "Failed to parse input or produce output.",
                                    absl::nullopt)));
}

TEST_F(YieldsTest, RuntimeErrorKeepsTheInnerMatchersFailureMessage) {
  // The legacy runner reports a runtime error where a parse error was expected
  // with a dedicated message, which the failure lists rely on.
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
  AddToFailureList(kRequiredFoo,
                   "Should have failed to parse, but raised an error instead.");

  EXPECT_THAT(Run(kRuntimeError), Yields(IsParseError()));
  EXPECT_EQ(GetCounts(test_manager_), (Counts{/*expected_successes=*/0,
                                              /*expected_failures=*/1}));
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
  EXPECT_EQ(GetCounts(test_manager_), (Counts{/*expected_successes=*/0,
                                              /*expected_failures=*/0,
                                              /*unexpected_successes=*/0,
                                              /*unexpected_failures=*/1}));
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
  EXPECT_EQ(GetCounts(test_manager_), (Counts{/*expected_successes=*/0,
                                              /*expected_failures=*/0,
                                              /*unexpected_successes=*/0,
                                              /*unexpected_failures=*/1}));
}

TEST_F(YieldsTest, AnyOfMatchesEitherLeg) {
  auto matcher = Yields(AnyOf(
      IsParseError(), ParsedPayload(EqualsBinaryProto(VarintField(1, 2)))));

  EXPECT_THAT(Run(kParseError, TestPriority::kP0, "first"), matcher);
  EXPECT_THAT(Run(kPayload2, TestPriority::kP0, "second"), matcher);
  EXPECT_EQ(GetCounts(test_manager_), (Counts{/*expected_successes=*/2}));
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
  EXPECT_EQ(GetCounts(test_manager_), (Counts{/*expected_successes=*/0,
                                              /*expected_failures=*/0,
                                              /*unexpected_successes=*/0,
                                              /*unexpected_failures=*/1}));
}

TEST_F(YieldsTest, NotComposes) {
  EXPECT_THAT(Run(kPayload2, TestPriority::kP0, "first"),
              Yields(Not(IsParseError())));
  EXPECT_EQ(GetCounts(test_manager_), (Counts{/*expected_successes=*/1}));

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
  EXPECT_EQ(GetCounts(test_manager_),
            (Counts{/*expected_successes=*/1, /*expected_failures=*/0,
                    /*unexpected_successes=*/0, /*unexpected_failures=*/1}));
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
  EXPECT_EQ(GetCounts(test_manager_), (Counts{/*expected_successes=*/1}));
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
  EXPECT_EQ(GetCounts(test_manager_), (Counts{/*expected_successes=*/0,
                                              /*expected_failures=*/0,
                                              /*unexpected_successes=*/0,
                                              /*unexpected_failures=*/1}));
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
  EXPECT_EQ(GetCounts(test_manager_), (Counts{/*expected_successes=*/0,
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
  EXPECT_EQ(GetCounts(test_manager_),
            (Counts{/*expected_successes=*/2, /*expected_failures=*/0,
                    /*unexpected_successes=*/0, /*unexpected_failures=*/1}));
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
  EXPECT_EQ(GetCounts(test_manager_), (Counts{/*expected_successes=*/1}));
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
