#include "conformance/testee.h"

#include <string>

#include <gmock/gmock.h>
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
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::DoAll;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Return;
using ::testing::SaveArg;

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
      testee.CreateTest("foo", TestPriority::kP0)
          .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
          .SerializeBinary();

  EXPECT_EQ(result.name(), "Required.Proto2.ProtobufInput.foo.ProtobufOutput");
  EXPECT_EQ(result.priority(), TestPriority::kP0);
  EXPECT_EQ(result.type(), TestAllTypesProto2::descriptor());
  EXPECT_EQ(result.format(), ::conformance::PROTOBUF);
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

  TestResult result = testee.CreateTest("foo", TestPriority::kP1)
                          .ParseText(TestAllTypesProto2::descriptor(), "text")
                          .SerializeText();

  EXPECT_EQ(result.name(),
            "Recommended.Proto2.TextFormatInput.foo.TextFormatOutput");
  EXPECT_EQ(result.priority(), TestPriority::kP1);
  EXPECT_EQ(result.type(), TestAllTypesProto2::descriptor());
  EXPECT_EQ(result.format(), ::conformance::TEXT_FORMAT);
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
      testee.CreateTest("foo", TestPriority::kP0)
          .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
          .SerializeText({/*print_unknown_fields=*/true});

  EXPECT_EQ(result.name(),
            "Required.Proto2.ProtobufInput.foo.TextFormatOutput");
  EXPECT_EQ(result.priority(), TestPriority::kP0);
  EXPECT_EQ(result.type(), TestAllTypesProto2::descriptor());
  EXPECT_EQ(result.format(), ::conformance::TEXT_FORMAT);
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

  TestResult result = testee.CreateTest("foo", TestPriority::kP0)
                          .ParseJson(TestAllTypesProto2::descriptor(), "json")
                          .SerializeJson();

  EXPECT_EQ(result.name(), "Required.Proto2.JsonInput.foo.JsonOutput");
  EXPECT_EQ(result.priority(), TestPriority::kP0);
  EXPECT_EQ(result.type(), TestAllTypesProto2::descriptor());
  EXPECT_EQ(result.format(), ::conformance::JSON);
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

  TestResult result = testee.CreateTest("foo", TestPriority::kP0)
                          .ParseJson(TestAllTypesProto2::descriptor(), "json",
                                     {/*ignore_unknown_fields=*/true})
                          .SerializeBinary();

  EXPECT_EQ(result.name(), "Required.Proto2.JsonInput.foo.ProtobufOutput");
  EXPECT_EQ(result.priority(), TestPriority::kP0);
  EXPECT_EQ(result.type(), TestAllTypesProto2::descriptor());
  EXPECT_EQ(result.format(), ::conformance::PROTOBUF);
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
      testee.CreateTest("foo", TestPriority::kP0)
          .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
          .SerializeBinary();

  EXPECT_EQ(result.name(), "Required.Proto2.ProtobufInput.foo.ProtobufOutput");
  EXPECT_EQ(result.priority(), TestPriority::kP0);
  EXPECT_EQ(result.type(), TestAllTypesProto2::descriptor());
  EXPECT_EQ(result.format(), ::conformance::PROTOBUF);
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
      testee.CreateTest("foo", TestPriority::kP0)
          .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
          .SerializeBinary();

  EXPECT_DEATH(
      (void)testee.CreateTest("foo", TestPriority::kP0)
          .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
          .SerializeBinary(),
      "Duplicated test name: Required.Proto2.ProtobufInput.foo.ProtobufOutput");
}

TEST(TesteeTest, RecordsTheTestsRun) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(mock, RunTest).WillRepeatedly(Return(std::string("\004")));
  EXPECT_THAT(testee.tests_run(), IsEmpty());

  (void)testee.CreateTest("foo", TestPriority::kP0)
      .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
      .SerializeBinary();
  (void)testee.CreateTest("bar", TestPriority::kP1)
      .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
      .SerializeText();

  EXPECT_THAT(
      testee.tests_run(),
      ElementsAre("Required.Proto2.ProtobufInput.foo.ProtobufOutput",
                  "Recommended.Proto2.ProtobufInput.bar.TextFormatOutput"));
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
      testee.CreateTest("foo", TestPriority::kP0)
          .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
          .ParseOnly();

  EXPECT_EQ(result.name(), "Required.Proto2.ProtobufInput.foo");
  EXPECT_EQ(result.priority(), TestPriority::kP0);
  EXPECT_EQ(result.type(), TestAllTypesProto2::descriptor());
  EXPECT_EQ(result.format(), ::conformance::PROTOBUF);
  EXPECT_THAT(result.response(), EqualsProto(R"pb(parse_error: "error")pb"));
}

// The request of a ParseOnly() test must be byte-identical to the one
// SerializeBinary() sends for the same input; only the test name differs.
TEST(TesteeTest, ParseOnlyBinarySendsSameRequestAsSerializeBinary) {
  MockTestRunner mock;
  Testee testee(&mock);
  std::string serialized_request, parse_only_request;
  EXPECT_CALL(mock,
              RunTest("Required.Proto2.ProtobufInput.foo.ProtobufOutput", _))
      .WillOnce(DoAll(SaveArg<1>(&serialized_request),
                      RespondWith(R"pb(parse_error: "error")pb")));
  EXPECT_CALL(mock, RunTest("Required.Proto2.ProtobufInput.foo", _))
      .WillOnce(DoAll(SaveArg<1>(&parse_only_request),
                      RespondWith(R"pb(parse_error: "error")pb")));

  TestResult serialized =
      testee.CreateTest("foo", TestPriority::kP0)
          .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
          .SerializeBinary();
  TestResult parse_only =
      testee.CreateTest("foo", TestPriority::kP0)
          .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
          .ParseOnly();

  EXPECT_EQ(serialized.name(),
            "Required.Proto2.ProtobufInput.foo.ProtobufOutput");
  EXPECT_EQ(parse_only.name(), "Required.Proto2.ProtobufInput.foo");
  EXPECT_EQ(parse_only_request, serialized_request);
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

  TestResult result = testee.CreateTest("foo", TestPriority::kP1)
                          .ParseText(TestAllTypesProto2::descriptor(), "text")
                          .ParseOnly();

  EXPECT_EQ(result.name(), "Recommended.Proto2.TextFormatInput.foo");
  EXPECT_EQ(result.priority(), TestPriority::kP1);
  EXPECT_EQ(result.format(), ::conformance::TEXT_FORMAT);
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

  TestResult result = testee.CreateTest("foo", TestPriority::kP0)
                          .ParseJson(TestAllTypesProto2::descriptor(), "json")
                          .ParseOnly();

  EXPECT_EQ(result.name(), "Required.Proto2.JsonInput.foo");
  EXPECT_EQ(result.format(), ::conformance::JSON);
  EXPECT_THAT(result.response(), EqualsProto(R"pb(parse_error: "error")pb"));
}

TEST(TesteeDeathTest, ParseOnlyDuplicateTestName) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(mock, RunTest("Required.Proto2.ProtobufInput.foo", _))
      .WillRepeatedly(RespondWith(R"pb(parse_error: "error")pb"));

  TestResult result =
      testee.CreateTest("foo", TestPriority::kP0)
          .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
          .ParseOnly();

  EXPECT_DEATH((void)testee.CreateTest("foo", TestPriority::kP0)
                   .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                   .ParseOnly(),
               "Duplicated test name: Required.Proto2.ProtobufInput.foo");
}

TEST(TesteeTest, OverrideTestCategory) {
  MockTestRunner mock;
  Testee testee(&mock);
  // The category changes, the name (derived from the input format) doesn't.
  EXPECT_CALL(
      mock,
      RunTest("Required.Proto2.ProtobufInput.foo.TextFormatOutput",
              RequestEquals(R"pb(
                protobuf_payload: "wire"
                requested_output_format: TEXT_FORMAT
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: TEXT_FORMAT_TEST
              )pb")))
      .WillOnce(RespondWith(R"pb(text_payload: "")pb"));

  TestResult result =
      testee.CreateTest("foo", TestPriority::kP0)
          .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
          .OverrideTestCategory(::conformance::TEXT_FORMAT_TEST)
          .SerializeText();

  EXPECT_EQ(result.name(),
            "Required.Proto2.ProtobufInput.foo.TextFormatOutput");
}

TEST(TesteeTest, OverrideTestCategoryParseOnly) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(
      mock,
      RunTest("Required.Proto2.ProtobufInput.foo", RequestEquals(R"pb(
                protobuf_payload: "wire"
                requested_output_format: PROTOBUF
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: TEXT_FORMAT_TEST
              )pb")))
      .WillOnce(RespondWith(R"pb(parse_error: "error")pb"));

  TestResult result =
      testee.CreateTest("foo", TestPriority::kP0)
          .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
          .OverrideTestCategory(::conformance::TEXT_FORMAT_TEST)
          .ParseOnly();

  EXPECT_EQ(result.name(), "Required.Proto2.ProtobufInput.foo");
}

TEST(TesteeDeathTest, OverrideTestCategoryAcceptsOnlyTextFormatTest) {
  MockTestRunner mock;
  Testee testee(&mock);
  // Without the DCHECK (opt builds) the request goes through as relabeled.
  EXPECT_CALL(mock, RunTest)
      .Times(AnyNumber())
      .WillRepeatedly(RespondWith(R"pb(parse_error: "error")pb"));
  EXPECT_DEBUG_DEATH(
      (void)testee.CreateTest("foo", TestPriority::kP0)
          .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
          .OverrideTestCategory(::conformance::JSON_TEST)
          .ParseOnly(),
      "only relabels the text-format suite's binary-input tests");
}

TEST(TesteeDeathTest, OverrideTestCategoryNeedsBinaryInput) {
  MockTestRunner mock;
  Testee testee(&mock);
  // Without the DCHECK (opt builds) the request goes through as relabeled.
  EXPECT_CALL(mock, RunTest)
      .Times(AnyNumber())
      .WillRepeatedly(RespondWith(R"pb(parse_error: "error")pb"));
  EXPECT_DEBUG_DEATH((void)testee.CreateTest("foo", TestPriority::kP0)
                         .ParseJson(TestAllTypesProto2::descriptor(), "{}")
                         .OverrideTestCategory(::conformance::TEXT_FORMAT_TEST)
                         .ParseOnly(),
                     "is for PROTOBUF-input tests");
}

// ---------------------------------------------------------------------------
// JSON test names and ParseOnly() output override
// ---------------------------------------------------------------------------

TEST(TesteeTest, SerializeJsonNamesTheTestWithInputAndOutputFormat) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(mock, RunTest("Required.Proto2.JsonInput.foo.JsonOutput", _))
      .WillOnce(RespondWith(R"pb(json_payload: "{}")pb"));

  TestResult result = testee.CreateTest("foo", TestPriority::kP0)
                          .ParseJson(TestAllTypesProto2::descriptor(), "json")
                          .SerializeJson();

  EXPECT_EQ(result.name(), "Required.Proto2.JsonInput.foo.JsonOutput");
  EXPECT_EQ(result.format(), ::conformance::JSON);
}

TEST(TesteeTest, SerializeJsonFromBinaryInputNamesTheInputFormat) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(
      mock,
      RunTest("Recommended.Proto2.ProtobufInput.foo.JsonOutput",
              RequestEquals(R"pb(
                protobuf_payload: "wire"
                requested_output_format: JSON
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: BINARY_TEST
              )pb")))
      .WillOnce(RespondWith(R"pb(serialize_error: "error")pb"));

  TestResult result =
      testee.CreateTest("foo", TestPriority::kP1)
          .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
          .SerializeJson();

  EXPECT_EQ(result.name(), "Recommended.Proto2.ProtobufInput.foo.JsonOutput");
  EXPECT_EQ(result.format(), ::conformance::JSON);
  EXPECT_THAT(result.response(),
              EqualsProto(R"pb(serialize_error: "error")pb"));
}

TEST(TesteeTest, ParseOnlyJsonWithProtobufOutput) {
  // The legacy RunValidJsonTestOrParseFailure asked for PROTOBUF output but
  // named the test like a parse-failure test.
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(
      mock,
      RunTest("Required.Proto2.JsonInput.foo", RequestEquals(R"pb(
                json_payload: "json"
                requested_output_format: PROTOBUF
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: JSON_TEST
              )pb")))
      .WillOnce(RespondWith(R"pb(parse_error: "error")pb"));

  TestResult result =
      testee.CreateTest("foo", TestPriority::kP0)
          .ParseJson(TestAllTypesProto2::descriptor(), "json")
          .ParseOnly({/*output_format=*/::conformance::PROTOBUF});

  EXPECT_EQ(result.name(), "Required.Proto2.JsonInput.foo");
  EXPECT_EQ(result.format(), ::conformance::PROTOBUF);
  EXPECT_THAT(result.response(), EqualsProto(R"pb(parse_error: "error")pb"));
}

// With the output format overridden, the request is byte-identical to the one
// SerializeBinary() sends for the same input; only the name differs.
TEST(TesteeTest, ParseOnlyWithProtobufOutputSendsSameRequestAsSerializeBinary) {
  MockTestRunner mock;
  Testee testee(&mock);
  std::string serialized_request, parse_only_request;
  EXPECT_CALL(mock, RunTest("Required.Proto2.JsonInput.foo.ProtobufOutput", _))
      .WillOnce(DoAll(SaveArg<1>(&serialized_request),
                      RespondWith(R"pb(parse_error: "error")pb")));
  EXPECT_CALL(mock, RunTest("Required.Proto2.JsonInput.foo", _))
      .WillOnce(DoAll(SaveArg<1>(&parse_only_request),
                      RespondWith(R"pb(parse_error: "error")pb")));

  TestResult serialized =
      testee.CreateTest("foo", TestPriority::kP0)
          .ParseJson(TestAllTypesProto2::descriptor(), "json")
          .SerializeBinary();
  TestResult parse_only =
      testee.CreateTest("foo", TestPriority::kP0)
          .ParseJson(TestAllTypesProto2::descriptor(), "json")
          .ParseOnly({/*output_format=*/::conformance::PROTOBUF});

  EXPECT_EQ(serialized.name(), "Required.Proto2.JsonInput.foo.ProtobufOutput");
  EXPECT_EQ(parse_only.name(), "Required.Proto2.JsonInput.foo");
  EXPECT_EQ(parse_only_request, serialized_request);
}

TEST(TesteeTest, ParseOnlyWithExplicitInputFormatIsTheDefault) {
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

  TestResult result = testee.CreateTest("foo", TestPriority::kP0)
                          .ParseJson(TestAllTypesProto2::descriptor(), "json")
                          .ParseOnly({/*output_format=*/::conformance::JSON});

  EXPECT_EQ(result.name(), "Required.Proto2.JsonInput.foo");
  EXPECT_EQ(result.format(), ::conformance::JSON);
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
