#include "conformance/testee.h"

#include <cstdlib>
#include <string>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest-spi.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "conformance/binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "conformance/test_runner.h"
#include "google/protobuf/test_messages_proto2.pb.h"
#include "google/protobuf/test_messages_proto3.pb.h"
#include "google/protobuf/test_textproto.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/util/message_differencer.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {
namespace {

using ::protobuf_test_messages::proto2::TestAllTypesProto2;
using ::testing::_;
using ::testing::PrintToString;
using ::testing::Return;

MATCHER_P(RequestEquals, expected_textproto, "") {
  ::conformance::ConformanceRequest request, expected;
  ABSL_CHECK(request.ParseFromString(arg));
  ABSL_CHECK(TextFormat::ParseFromString(expected_textproto, &expected));
  if (!util::MessageDifferencer::Equals(request, expected)) {
    std::string request_string;
    ABSL_CHECK(TextFormat::PrintToString(request, &request_string));
    *result_listener << "with equivalent text format:\n" << request_string;
    return false;
  }
  return true;
}

auto RespondWith(absl::string_view textproto) {
  ::conformance::ConformanceResponse response;
  ABSL_CHECK(TextFormat::ParseFromString(textproto, &response));
  return Return(response.SerializeAsString());
}

class MockTestRunner : public ConformanceTestRunner {
 public:
  MOCK_METHOD(std::string, RunTest,
              (absl::string_view test_name, absl::string_view input),
              (override));
};

// Marks `result` as checked so that inspecting it directly (instead of
// matching it with Yields()) doesn't trip the never-checked failure.
TestResult Checked(TestResult result) {
  result.MarkChecked();
  return result;
}

TEST(TesteeTest, BinaryToBinary) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(
      mock,
      RunTest("Required.Proto2.ProtobufInput.foo.ProtobufOutput",
              RequestEquals(R"pb(
                protobuf_payload: "wire"
                requested_output_format: PROTOBUF
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: BINARY_TEST
              )pb")))
      .WillOnce(RespondWith(R"pb(runtime_error: "error")pb"));

  TestResult result =
      Checked(testee.CreateTest("foo", TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .SerializeBinary());

  EXPECT_EQ(result.name(), "Required.Proto2.ProtobufInput.foo.ProtobufOutput");
  EXPECT_EQ(result.priority(), TestPriority::kP0);
  EXPECT_EQ(result.type(), TestAllTypesProto2::descriptor());
  EXPECT_THAT(result.format(), ::conformance::PROTOBUF);
  EXPECT_THAT(result.request(), EqualsProto(R"pb(
                protobuf_payload: "wire"
                requested_output_format: PROTOBUF
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: BINARY_TEST
              )pb"));
  EXPECT_THAT(result.response(), EqualsProto(R"pb(runtime_error: "error")pb"));
}

TEST(TesteeTest, TextToText) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(
      mock,
      RunTest("Recommended.Proto2.TextFormatInput.foo.TextFormatOutput",
              RequestEquals(R"pb(
                text_payload: "text"
                requested_output_format: TEXT_FORMAT
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: TEXT_FORMAT_TEST
              )pb")))
      .WillOnce(RespondWith(R"pb(runtime_error: "error")pb"));

  TestResult result =
      Checked(testee.CreateTest("foo", TestPriority::kP3)
                  .ParseText(TestAllTypesProto2::descriptor(), "text")
                  .SerializeText());

  EXPECT_EQ(result.name(),
            "Recommended.Proto2.TextFormatInput.foo.TextFormatOutput");
  EXPECT_EQ(result.priority(), TestPriority::kP3);
  EXPECT_EQ(result.type(), TestAllTypesProto2::descriptor());
  EXPECT_THAT(result.format(), ::conformance::TEXT_FORMAT);
  EXPECT_THAT(result.response(), EqualsProto(R"pb(runtime_error: "error")pb"));
}

TEST(TesteeTest, TextPrintUnknownFields) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(
      mock,
      RunTest("Required.Proto2.ProtobufInput.foo.TextFormatOutput",
              RequestEquals(R"pb(
                protobuf_payload: "wire"
                requested_output_format: TEXT_FORMAT
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: BINARY_TEST
                print_unknown_fields: true
              )pb")))
      .WillOnce(RespondWith(R"pb(runtime_error: "error")pb"));

  TestResult result =
      Checked(testee.CreateTest("foo", TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .SerializeText({/*print_unknown_fields=*/true}));

  EXPECT_EQ(result.name(),
            "Required.Proto2.ProtobufInput.foo.TextFormatOutput");
  EXPECT_EQ(result.priority(), TestPriority::kP0);
  EXPECT_EQ(result.type(), TestAllTypesProto2::descriptor());
  EXPECT_THAT(result.format(), ::conformance::TEXT_FORMAT);
  EXPECT_THAT(result.request(), EqualsProto(R"pb(
                protobuf_payload: "wire"
                requested_output_format: TEXT_FORMAT
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: BINARY_TEST
                print_unknown_fields: true
              )pb"));
  EXPECT_THAT(result.response(), EqualsProto(R"pb(runtime_error: "error")pb"));
}

TEST(TesteeTest, JsonToJson) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(
      mock,
      RunTest("Required.Proto2.JsonInput.foo.JsonOutput", RequestEquals(R"pb(
                json_payload: "json"
                requested_output_format: JSON
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: JSON_TEST
              )pb")))
      .WillOnce(RespondWith(R"pb(runtime_error: "error")pb"));

  TestResult result =
      Checked(testee.CreateTest("foo", TestPriority::kP0)
                  .ParseJson(TestAllTypesProto2::descriptor(), "json")
                  .SerializeJson());

  EXPECT_EQ(result.name(), "Required.Proto2.JsonInput.foo.JsonOutput");
  EXPECT_EQ(result.priority(), TestPriority::kP0);
  EXPECT_EQ(result.type(), TestAllTypesProto2::descriptor());
  EXPECT_THAT(result.format(), ::conformance::JSON);
  EXPECT_THAT(result.response(), EqualsProto(R"pb(runtime_error: "error")pb"));
}

TEST(TesteeTest, JsonIgnoreUnknownParsing) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(
      mock,
      RunTest("Required.Proto2.JsonInput.foo.ProtobufOutput",
              RequestEquals(R"pb(
                json_payload: "json"
                requested_output_format: PROTOBUF
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: JSON_IGNORE_UNKNOWN_PARSING_TEST
              )pb")))
      .WillOnce(RespondWith(R"pb(runtime_error: "error")pb"));

  TestResult result =
      Checked(testee.CreateTest("foo", TestPriority::kP0)
                  .ParseJson(TestAllTypesProto2::descriptor(), "json",
                             {/*ignore_unknown_fields=*/true})
                  .SerializeBinary());

  EXPECT_EQ(result.name(), "Required.Proto2.JsonInput.foo.ProtobufOutput");
  EXPECT_EQ(result.priority(), TestPriority::kP0);
  EXPECT_EQ(result.type(), TestAllTypesProto2::descriptor());
  EXPECT_THAT(result.format(), ::conformance::PROTOBUF);
  EXPECT_THAT(result.response(), EqualsProto(R"pb(runtime_error: "error")pb"));
}

TEST(TesteeTest, InvalidResponse) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(
      mock,
      RunTest("Required.Proto2.ProtobufInput.foo.ProtobufOutput",
              RequestEquals(R"pb(
                protobuf_payload: "wire"
                requested_output_format: PROTOBUF
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: BINARY_TEST
              )pb")))
      .WillOnce(Return(std::string("\004")));

  TestResult result =
      Checked(testee.CreateTest("foo", TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .SerializeBinary());

  EXPECT_EQ(result.name(), "Required.Proto2.ProtobufInput.foo.ProtobufOutput");
  EXPECT_EQ(result.priority(), TestPriority::kP0);
  EXPECT_EQ(result.type(), TestAllTypesProto2::descriptor());
  EXPECT_THAT(result.format(), ::conformance::PROTOBUF);
  EXPECT_THAT(
      result.response(),
      EqualsProto(
          R"pb(runtime_error: "response proto could not be parsed.")pb"));
}

TEST(TesteeTest, DuplicateTestName) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(
      mock,
      RunTest("Required.Proto2.ProtobufInput.foo.ProtobufOutput",
              RequestEquals(R"pb(
                protobuf_payload: "wire"
                requested_output_format: PROTOBUF
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: BINARY_TEST
              )pb")))
      .WillRepeatedly(Return(std::string("\004")));

  TestResult result =
      Checked(testee.CreateTest("foo", TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .SerializeBinary());

  EXPECT_DEATH(
      testee.CreateTest("foo", TestPriority::kP0)
          .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
          .SerializeBinary(),
      "Duplicated test name: Required.Proto2.ProtobufInput.foo.ProtobufOutput");
}

TEST(TesteeTest, ParseOnlyBinary) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(
      mock,
      RunTest("Required.Proto2.ProtobufInput.foo", RequestEquals(R"pb(
                protobuf_payload: "wire"
                requested_output_format: PROTOBUF
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: BINARY_TEST
              )pb")))
      .WillOnce(RespondWith(R"pb(parse_error: "error")pb"));

  TestResult result =
      Checked(testee.CreateTest("foo", TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .ParseOnly());

  EXPECT_EQ(result.name(), "Required.Proto2.ProtobufInput.foo");
  EXPECT_EQ(result.priority(), TestPriority::kP0);
  EXPECT_EQ(result.type(), TestAllTypesProto2::descriptor());
  EXPECT_THAT(result.format(), ::conformance::PROTOBUF);
  EXPECT_THAT(result.request(), EqualsProto(R"pb(
                protobuf_payload: "wire"
                requested_output_format: PROTOBUF
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: BINARY_TEST
              )pb"));
  EXPECT_THAT(result.response(), EqualsProto(R"pb(parse_error: "error")pb"));
}

// The request of a ParseOnly() test must be byte-identical to the one
// SerializeBinary() sends for the same input; only the test name differs.
TEST(TesteeTest, ParseOnlyBinarySendsSameRequestAsSerializeBinary) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(mock, RunTest)
      .WillRepeatedly(RespondWith(R"pb(parse_error: "error")pb"));

  TestResult serialized =
      Checked(testee.CreateTest("foo", TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .SerializeBinary());
  TestResult parse_only =
      Checked(testee.CreateTest("foo", TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .ParseOnly());

  EXPECT_EQ(serialized.name(),
            "Required.Proto2.ProtobufInput.foo.ProtobufOutput");
  EXPECT_EQ(parse_only.name(), "Required.Proto2.ProtobufInput.foo");
  EXPECT_EQ(parse_only.request().SerializeAsString(),
            serialized.request().SerializeAsString());
}

TEST(TesteeTest, ParseOnlyText) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(
      mock,
      RunTest("Recommended.Proto2.TextFormatInput.foo", RequestEquals(R"pb(
                text_payload: "text"
                requested_output_format: TEXT_FORMAT
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: TEXT_FORMAT_TEST
              )pb")))
      .WillOnce(RespondWith(R"pb(parse_error: "error")pb"));

  TestResult result =
      Checked(testee.CreateTest("foo", TestPriority::kP3)
                  .ParseText(TestAllTypesProto2::descriptor(), "text")
                  .ParseOnly());

  EXPECT_EQ(result.name(), "Recommended.Proto2.TextFormatInput.foo");
  EXPECT_EQ(result.priority(), TestPriority::kP3);
  EXPECT_THAT(result.format(), ::conformance::TEXT_FORMAT);
  EXPECT_THAT(result.response(), EqualsProto(R"pb(parse_error: "error")pb"));
}

TEST(TesteeTest, ParseOnlyJson) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(
      mock,
      RunTest("Required.Proto2.JsonInput.foo", RequestEquals(R"pb(
                json_payload: "json"
                requested_output_format: JSON
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: JSON_TEST
              )pb")))
      .WillOnce(RespondWith(R"pb(parse_error: "error")pb"));

  TestResult result =
      Checked(testee.CreateTest("foo", TestPriority::kP0)
                  .ParseJson(TestAllTypesProto2::descriptor(), "json")
                  .ParseOnly());

  EXPECT_EQ(result.name(), "Required.Proto2.JsonInput.foo");
  EXPECT_THAT(result.format(), ::conformance::JSON);
  EXPECT_THAT(result.response(), EqualsProto(R"pb(parse_error: "error")pb"));
}

TEST(TesteeTest, ParseOnlyDuplicateTestName) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(mock, RunTest("Required.Proto2.ProtobufInput.foo", _))
      .WillRepeatedly(RespondWith(R"pb(parse_error: "error")pb"));

  TestResult result =
      Checked(testee.CreateTest("foo", TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .ParseOnly());

  EXPECT_DEATH(testee.CreateTest("foo", TestPriority::kP0)
                   .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                   .ParseOnly(),
               "Duplicated test name: Required.Proto2.ProtobufInput.foo");
}

TEST(TestResultTest, ForTesting) {
  ::conformance::ConformanceRequest request;
  request.set_protobuf_payload("wire");
  request.set_requested_output_format(::conformance::TEXT_FORMAT);
  ::conformance::ConformanceResponse response;
  response.set_text_payload("text");

  TestResult result = TestResult::ForTesting("some.name", TestPriority::kP3,
                                             TestAllTypesProto2::descriptor(),
                                             request, response);
  EXPECT_FALSE(result.checked());
  result.MarkChecked();
  EXPECT_TRUE(result.checked());

  EXPECT_EQ(result.name(), "some.name");
  EXPECT_EQ(result.priority(), TestPriority::kP3);
  EXPECT_EQ(result.type(), TestAllTypesProto2::descriptor());
  EXPECT_EQ(result.format(), ::conformance::TEXT_FORMAT);
  EXPECT_THAT(result.request(), EqualsProto(R"pb(
                protobuf_payload: "wire"
                requested_output_format: TEXT_FORMAT
              )pb"));
  EXPECT_THAT(result.response(), EqualsProto(R"pb(text_payload: "text")pb"));
}

TEST(TestResultTest, NeverCheckedResultFailsOnDestruction) {
  EXPECT_NONFATAL_FAILURE(
      {
        TestResult result =
            TestResult::ForTesting("some.name", TestPriority::kP0,
                                   TestAllTypesProto2::descriptor(), {}, {});
        EXPECT_FALSE(result.checked());
      },
      "TestResult for some.name was never checked; wrap the matcher in "
      "Yields()");
}

TEST(TestResultTest, CheckedResultIsSilentOnDestruction) {
  testing::TestPartResultArray failures;
  {
    testing::ScopedFakeTestPartResultReporter reporter(
        testing::ScopedFakeTestPartResultReporter::
            INTERCEPT_ONLY_CURRENT_THREAD,
        &failures);
    TestResult result =
        TestResult::ForTesting("some.name", TestPriority::kP0,
                               TestAllTypesProto2::descriptor(), {}, {});
    result.MarkChecked();
  }
  EXPECT_EQ(failures.size(), 0);
}

TEST(TestResultTest, MovedFromResultIsInert) {
  testing::TestPartResultArray failures;
  {
    testing::ScopedFakeTestPartResultReporter reporter(
        testing::ScopedFakeTestPartResultReporter::
            INTERCEPT_ONLY_CURRENT_THREAD,
        &failures);
    TestResult original =
        TestResult::ForTesting("some.name", TestPriority::kP0,
                               TestAllTypesProto2::descriptor(), {}, {});
    {
      // Move-construction: the destination inherits the unchecked state.
      TestResult moved(std::move(original));
      EXPECT_EQ(moved.name(), "some.name");
      EXPECT_FALSE(moved.checked());
      moved.MarkChecked();
    }
    // Move-assignment over a checked result.
    TestResult target = TestResult::ForTesting(
        "target", TestPriority::kP0, TestAllTypesProto2::descriptor(), {}, {});
    target.MarkChecked();
    TestResult source = TestResult::ForTesting(
        "source", TestPriority::kP0, TestAllTypesProto2::descriptor(), {}, {});
    source.MarkChecked();
    target = std::move(source);
    EXPECT_EQ(target.name(), "source");
    // Neither `original` nor `source` reports anything when it goes away.
  }
  EXPECT_EQ(failures.size(), 0);
}

TEST(TestResultTest, MoveAssignmentReportsOverwrittenUncheckedResult) {
  EXPECT_NONFATAL_FAILURE(
      {
        TestResult target =
            TestResult::ForTesting("target", TestPriority::kP0,
                                   TestAllTypesProto2::descriptor(), {}, {});
        TestResult source =
            TestResult::ForTesting("source", TestPriority::kP0,
                                   TestAllTypesProto2::descriptor(), {}, {});
        source.MarkChecked();
        target = std::move(source);
        EXPECT_TRUE(target.checked());
      },
      "TestResult for target was never checked");
}

TEST(TestResultDeathTest, RequiresAMessageType) {
  EXPECT_DEATH(TestResult::ForTesting("some.name", TestPriority::kP0,
                                      /*type=*/nullptr, {}, {}),
               "TestResult for some.name has no message type");
}

TEST(TestResultTest, SetVerdictMarksCheckedAndIsKeptAcrossMoves) {
  TestResult result = TestResult::ForTesting(
      "some.name", TestPriority::kP0, TestAllTypesProto2::descriptor(), {}, {});
  EXPECT_FALSE(result.checked());
  EXPECT_FALSE(result.verdict().has_value());

  result.SetVerdict({/*matched=*/true, /*explanation=*/"because"});
  EXPECT_TRUE(result.checked());
  ASSERT_TRUE(result.verdict().has_value());
  EXPECT_TRUE(result.verdict()->matched);
  EXPECT_EQ(result.verdict()->explanation, "because");

  TestResult moved = std::move(result);
  EXPECT_TRUE(moved.checked());
  ASSERT_TRUE(moved.verdict().has_value());
  EXPECT_EQ(moved.verdict()->explanation, "because");

  TestResult assigned = TestResult::ForTesting(
      "other", TestPriority::kP0, TestAllTypesProto2::descriptor(), {}, {});
  assigned.MarkChecked();
  EXPECT_FALSE(assigned.verdict().has_value());
  assigned = std::move(moved);
  ASSERT_TRUE(assigned.verdict().has_value());
  EXPECT_TRUE(assigned.verdict()->matched);
  EXPECT_EQ(assigned.verdict()->explanation, "because");
}

TEST(TestResultTest, MarkCheckedRecordsNoVerdict) {
  TestResult result = TestResult::ForTesting(
      "some.name", TestPriority::kP0, TestAllTypesProto2::descriptor(), {}, {});
  result.MarkChecked();
  EXPECT_TRUE(result.checked());
  EXPECT_FALSE(result.verdict().has_value());
}

TEST(TestResultDeathTest, MovedFromResultCannotBeChecked) {
  TestResult result = TestResult::ForTesting(
      "some.name", TestPriority::kP0, TestAllTypesProto2::descriptor(), {}, {});
  TestResult moved = std::move(result);
  moved.MarkChecked();
  EXPECT_DEBUG_DEATH(
      result.SetVerdict({/*matched=*/true}),  // NOLINT(bugprone-use-after-move)
      "moved-from TestResult");
}

TEST(TestResultDeathTest, NeverCheckedResultIsSilentAfterAFatalFailure) {
  // An ASSERT_* that returns early leaves the test's result unchecked; the
  // destructor must not pile a second failure onto the fatal one.  A real
  // (non-intercepted) fatal failure would fail this test, so the scenario runs
  // in a death-test child that exits with the number of stray failures.
  EXPECT_EXIT(
      {
        // What ASSERT_* records before returning.
        [] { FAIL() << "fatal failure"; }();
        ABSL_CHECK(testing::Test::HasFatalFailure());

        testing::TestPartResultArray failures;
        {
          testing::ScopedFakeTestPartResultReporter reporter(
              testing::ScopedFakeTestPartResultReporter::
                  INTERCEPT_ONLY_CURRENT_THREAD,
              &failures);
          TestResult result =
              TestResult::ForTesting("some.name", TestPriority::kP0,
                                     TestAllTypesProto2::descriptor(), {}, {});
          EXPECT_FALSE(result.checked());
        }
        std::exit(failures.size());
      },
      testing::ExitedWithCode(0), "");
}

// Runs a binary-to-binary test against a testee that answers with the given
// response.
TestResult ResultWithResponse(absl::string_view response_textproto,
                              TestPriority priority = TestPriority::kP0,
                              Wire input = Wire("wire")) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(mock, RunTest).WillOnce(RespondWith(response_textproto));
  return Checked(
      testee.CreateTest("foo", priority)
          .ParseBinary(TestAllTypesProto2::descriptor(), std::move(input))
          .SerializeBinary());
}

constexpr absl::string_view kPrintedRequiredRequest =
    R"(Required test "Required.Proto2.ProtobufInput.foo.ProtobufOutput" )"
    R"(with request {protobuf_payload: "wire" )"
    R"(requested_output_format: PROTOBUF )"
    R"(message_type: "protobuf_test_messages.proto2.TestAllTypesProto2" )"
    R"(test_category: BINARY_TEST})";

TEST(PrintTestResultTest, ParseError) {
  EXPECT_EQ(PrintToString(
                ResultWithResponse(R"pb(parse_error: "failed to parse")pb")),
            absl::StrCat(kPrintedRequiredRequest,
                         R"( and response {parse_error: "failed to parse"})"));
}

TEST(PrintTestResultTest, Skipped) {
  EXPECT_EQ(
      PrintToString(ResultWithResponse(R"pb(skipped: "skipped message")pb",
                                       TestPriority::kP3)),
      R"(Recommended test "Recommended.Proto2.ProtobufInput.foo.ProtobufOutput" )"
      R"(with request {protobuf_payload: "wire" )"
      R"(requested_output_format: PROTOBUF )"
      R"(message_type: "protobuf_test_messages.proto2.TestAllTypesProto2" )"
      R"(test_category: BINARY_TEST} and response {skipped: "skipped message"})");
}

TEST(PrintTestResultTest, EmptyResponse) {
  EXPECT_EQ(PrintToString(ResultWithResponse("")),
            absl::StrCat(kPrintedRequiredRequest, " and response {}"));
}

TEST(PrintTestResultTest, ProtobufPayloadIsDecoded) {
  EXPECT_EQ(
      PrintToString(ResultWithResponse(R"pb(protobuf_payload: "\010\t")pb")),
      absl::StrCat(kPrintedRequiredRequest,
                   R"( and response {protobuf_payload: "\010\t"} )"
                   R"((decoded: {optional_int32: 9}))"));
}

TEST(PrintTestResultTest, UnparseableProtobufPayload) {
  EXPECT_EQ(
      PrintToString(ResultWithResponse(R"pb(protobuf_payload: "\001")pb")),
      absl::StrCat(
          kPrintedRequiredRequest,
          R"( and response {protobuf_payload: "\001"} (unparseable))"));
}

TEST(PrintTestResultTest, LargePayloadsAreTruncated) {
  std::string large_payload(300, 'a');
  std::string truncated_payload =
      absl::StrCat(std::string(200, 'a'), "...(truncated)");

  EXPECT_EQ(
      PrintToString(ResultWithResponse(
          absl::StrCat("text_payload: \"", large_payload, "\""),
          TestPriority::kP0, Wire(large_payload))),
      absl::StrCat(
          R"(Required test "Required.Proto2.ProtobufInput.foo.ProtobufOutput" )"
          "with request {protobuf_payload: \"",
          truncated_payload,
          "\" requested_output_format: PROTOBUF "
          R"(message_type: "protobuf_test_messages.proto2.TestAllTypesProto2" )"
          "test_category: BINARY_TEST} and response {text_payload: \"",
          truncated_payload, "\"}"));
}

TEST(PrintTestResultTest, LargeTextInputIsTruncated) {
  std::string large_payload(300, 'a');
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(mock, RunTest)
      .WillOnce(RespondWith(R"pb(runtime_error: "error")pb"));

  TestResult result =
      Checked(testee.CreateTest("foo", TestPriority::kP0)
                  .ParseText(TestAllTypesProto2::descriptor(), large_payload)
                  .SerializeText());

  EXPECT_EQ(
      PrintToString(result),
      absl::StrCat(
          R"(Required test "Required.Proto2.TextFormatInput.foo.TextFormatOutput" )"
          R"(with request {requested_output_format: TEXT_FORMAT )"
          R"(message_type: "protobuf_test_messages.proto2.TestAllTypesProto2" )"
          "test_category: TEXT_FORMAT_TEST text_payload: \"",
          std::string(200, 'a'), "...(truncated)",
          "\"} and response {runtime_error: \"error\"}"));
}

TEST(TestPriorityTest, PriorityName) {
  EXPECT_EQ(PriorityName(kP0), "P0");
  EXPECT_EQ(PriorityName(kP1), "P1");
  EXPECT_EQ(PriorityName(kP2), "P2");
  EXPECT_EQ(PriorityName(kP3), "P3");
}

TEST(TestPriorityTest, PriorityLevelName) {
  EXPECT_EQ(PriorityLevelName(kP0), "Required");
  EXPECT_EQ(PriorityLevelName(kP1), "Required");
  EXPECT_EQ(PriorityLevelName(kP2), "Required");
  EXPECT_EQ(PriorityLevelName(kP3), "Recommended");
}

}  // namespace
}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
