#include "conformance/testee.h"

#include <cstdlib>
#include <string>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest-spi.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "conformance/binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "conformance/mock_test_runner.h"
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

// Marks `result` as checked so that inspecting it directly (instead of
// matching it with Yields()) doesn't trip the never-checked failure.
TestResult Checked(TestResult result) {
  result.MarkChecked();
  return result;
}

// Runs a binary-to-binary kP0 test named `name` against a testee that answers
// with an empty response, for tests of a result's bookkeeping.  The result is
// unchecked and named Required.Proto2.ProtobufInput.<name>.ProtobufOutput.
TestResult UncheckedResult(absl::string_view name) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(mock, RunTest).WillOnce(Return(std::string()));
  return testee.CreateTest(name, TestPriority::kP0)
      .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
      .SerializeBinary();
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
  EXPECT_FALSE(result.print_unknown_fields());
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
      Checked(testee.CreateTest("foo", TestPriority::kP1)
                  .ParseText(TestAllTypesProto2::descriptor(), "text")
                  .SerializeText());

  EXPECT_EQ(result.name(),
            "Recommended.Proto2.TextFormatInput.foo.TextFormatOutput");
  EXPECT_EQ(result.priority(), TestPriority::kP1);
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
  EXPECT_TRUE(result.print_unknown_fields());
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

TEST(TestResultTest, NeverCheckedResultFailsOnDestruction) {
  EXPECT_NONFATAL_FAILURE(
      {
        TestResult result = UncheckedResult("foo");
        EXPECT_FALSE(result.checked());
      },
      "TestResult for Required.Proto2.ProtobufInput.foo.ProtobufOutput was "
      "never checked; wrap the matcher in Yields()");
}

TEST(TestResultTest, CheckedResultIsSilentOnDestruction) {
  testing::TestPartResultArray failures;
  {
    testing::ScopedFakeTestPartResultReporter reporter(
        testing::ScopedFakeTestPartResultReporter::
            INTERCEPT_ONLY_CURRENT_THREAD,
        &failures);
    TestResult result = UncheckedResult("foo");
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
    TestResult original = UncheckedResult("original");
    {
      // Move-construction: the destination inherits the unchecked state.
      TestResult moved(std::move(original));
      EXPECT_EQ(moved.name(),
                "Required.Proto2.ProtobufInput.original.ProtobufOutput");
      EXPECT_FALSE(moved.checked());
      moved.MarkChecked();
    }
    // Move-assignment over a checked result.
    TestResult target = UncheckedResult("target");
    target.MarkChecked();
    TestResult source = UncheckedResult("source");
    source.MarkChecked();
    target = std::move(source);
    EXPECT_EQ(target.name(),
              "Required.Proto2.ProtobufInput.source.ProtobufOutput");
    // Neither `original` nor `source` reports anything when it goes away.
  }
  EXPECT_EQ(failures.size(), 0);
}

TEST(TestResultTest, MoveAssignmentReportsOverwrittenUncheckedResult) {
  EXPECT_NONFATAL_FAILURE(
      {
        TestResult target = UncheckedResult("target");
        TestResult source = UncheckedResult("source");
        source.MarkChecked();
        target = std::move(source);
        EXPECT_TRUE(target.checked());
      },
      "TestResult for Required.Proto2.ProtobufInput.target.ProtobufOutput was "
      "never checked");
}

TEST(TestResultTest, SetVerdictMarksCheckedAndIsKeptAcrossMoves) {
  TestResult result = UncheckedResult("foo");
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

  TestResult assigned = UncheckedResult("other");
  assigned.MarkChecked();
  EXPECT_FALSE(assigned.verdict().has_value());
  assigned = std::move(moved);
  ASSERT_TRUE(assigned.verdict().has_value());
  EXPECT_TRUE(assigned.verdict()->matched);
  EXPECT_EQ(assigned.verdict()->explanation, "because");
}

TEST(TestResultTest, MarkCheckedRecordsNoVerdict) {
  TestResult result = UncheckedResult("foo");
  result.MarkChecked();
  EXPECT_TRUE(result.checked());
  EXPECT_FALSE(result.verdict().has_value());
}

TEST(TestResultDeathTest, MovedFromResultCannotBeChecked) {
  TestResult result = UncheckedResult("foo");
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
          TestResult result = UncheckedResult("foo");
          EXPECT_FALSE(result.checked());
        }
        std::exit(failures.size());
      },
      testing::ExitedWithCode(0), "");
}

TEST(TestPriorityTest, PriorityName) {
  EXPECT_EQ(PriorityName(kP0), "P0");
  EXPECT_EQ(PriorityName(kP1), "P1");
}

TEST(TestPriorityTest, PriorityLevelName) {
  EXPECT_EQ(PriorityLevelName(kP0), "Required");
  EXPECT_EQ(PriorityLevelName(kP1), "Recommended");
}

}  // namespace
}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
