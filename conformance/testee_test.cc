#include "testee.h"

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "binary_wireformat.h"
#include "test_runner.h"
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
  request.ParseFromString(arg);
  TextFormat::ParseFromString(expected_textproto, &expected);
  if (!util::MessageDifferencer::Equals(request, expected)) {
    std::string request_string;
    TextFormat::PrintToString(request, &request_string);
    *result_listener << "with equivalent text format:\n" << request_string;
    return false;
  }
  return true;
}

auto RespondWith(absl::string_view textproto) {
  ::conformance::ConformanceResponse response;
  TextFormat::ParseFromString(textproto, &response);
  return Return(response.SerializeAsString());
}

class MockTestRunner : public ConformanceTestRunner {
 public:
  MOCK_METHOD(std::string, RunTest,
              (absl::string_view test_name, absl::string_view input),
              (override));
};

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
      testee.CreateTest("foo", TestStrictness::kRequired)
          .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
          .SerializeBinary();

  EXPECT_EQ(result.name(), "Required.Proto2.ProtobufInput.foo.ProtobufOutput");
  EXPECT_EQ(result.strictness(), TestStrictness::kRequired);
  EXPECT_EQ(result.type(), TestAllTypesProto2::descriptor());
  EXPECT_THAT(result.format(), ::conformance::PROTOBUF);
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

  TestResult result = testee.CreateTest("foo", TestStrictness::kRecommended)
                          .ParseText(TestAllTypesProto2::descriptor(), "text")
                          .SerializeText();

  EXPECT_EQ(result.name(),
            "Recommended.Proto2.TextFormatInput.foo.TextFormatOutput");
  EXPECT_EQ(result.strictness(), TestStrictness::kRecommended);
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
      testee.CreateTest("foo", TestStrictness::kRequired)
          .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
          .SerializeText({.print_unknown_fields = true});

  EXPECT_EQ(result.name(),
            "Required.Proto2.ProtobufInput.foo.TextFormatOutput");
  EXPECT_EQ(result.strictness(), TestStrictness::kRequired);
  EXPECT_EQ(result.type(), TestAllTypesProto2::descriptor());
  EXPECT_THAT(result.format(), ::conformance::TEXT_FORMAT);
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

  TestResult result = testee.CreateTest("foo", TestStrictness::kRequired)
                          .ParseJson(TestAllTypesProto2::descriptor(), "json")
                          .SerializeJson();

  EXPECT_EQ(result.name(), "Required.Proto2.JsonInput.foo.JsonOutput");
  EXPECT_EQ(result.strictness(), TestStrictness::kRequired);
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

  TestResult result = testee.CreateTest("foo", TestStrictness::kRequired)
                          .ParseJson(TestAllTypesProto2::descriptor(), "json",
                                     {.ignore_unknown_fields = true})
                          .SerializeBinary();

  EXPECT_EQ(result.name(), "Required.Proto2.JsonInput.foo.ProtobufOutput");
  EXPECT_EQ(result.strictness(), TestStrictness::kRequired);
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
      testee.CreateTest("foo", TestStrictness::kRequired)
          .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
          .SerializeBinary();

  EXPECT_EQ(result.name(), "Required.Proto2.ProtobufInput.foo.ProtobufOutput");
  EXPECT_EQ(result.strictness(), TestStrictness::kRequired);
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
      testee.CreateTest("foo", TestStrictness::kRequired)
          .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
          .SerializeBinary();

  EXPECT_DEATH(
      testee.CreateTest("foo", TestStrictness::kRequired)
          .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
          .SerializeBinary(),
      "Duplicated test name: Required.Proto2.ProtobufInput.foo.ProtobufOutput");
}

}  // namespace
}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
