#include "conformance/testee.h"

#include <cstdlib>
#include <string>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest-spi.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/log/log_entry.h"
#include "absl/log/scoped_mock_log.h"
#include "absl/status/status.h"
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
using ::protobuf_test_messages::proto3::TestAllTypesProto3;
using ::testing::_;
using ::testing::AnyNumber;
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

// The wire form of the ConformanceResponse `textproto` describes.
std::string SerializedResponse(absl::string_view textproto) {
  ::conformance::ConformanceResponse response;
  ABSL_CHECK(TextFormat::ParseFromString(textproto, &response));
  return response.SerializeAsString();
}

auto RespondWith(absl::string_view textproto) {
  return Return(SerializedResponse(textproto));
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

// The name of the tests below, as if they came from TEST(TesteeTest, foo).
TestName Foo() { return {/*suite=*/"TesteeTest", /*test=*/"foo"}; }

TEST(TesteeTest, BinaryToBinary) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(
      mock,
      RunTest("Required.TesteeTest.foo.ProtobufInput.ProtobufOutput",
              RequestEquals(R"pb(
                protobuf_payload: "wire"
                requested_output_format: PROTOBUF
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: BINARY_TEST
              )pb")))
      .WillOnce(RespondWith(R"pb(runtime_error: "error")pb"));

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .SerializeBinary());

  EXPECT_EQ(result.name(),
            "Required.TesteeTest.foo.ProtobufInput.ProtobufOutput");
  EXPECT_EQ(result.priority(), TestPriority::kP0);
  EXPECT_EQ(result.type(), TestAllTypesProto2::descriptor());
  EXPECT_EQ(result.format(), ::conformance::PROTOBUF);
  EXPECT_THAT(result.request(), EqualsProto(R"pb(
                protobuf_payload: "wire"
                requested_output_format: PROTOBUF
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: BINARY_TEST
              )pb"));
  EXPECT_THAT(result.response(), EqualsProto(R"pb(runtime_error: "error")pb"));
}

// Names derived from a gtest test: <Level>.<Suite>.<Test> plus the input and
// output formats.
TEST(TesteeTest, DerivedName) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(mock,
              RunTest("Required.FooTest.Bar.ProtobufInput.ProtobufOutput", _))
      .WillOnce(RespondWith(R"pb(protobuf_payload: "")pb"));

  TestResult result =
      Checked(testee
                  .CreateTest(TestName{/*suite=*/"FooTest", /*test=*/"Bar"},
                              TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .SerializeBinary());

  EXPECT_EQ(result.name(), "Required.FooTest.Bar.ProtobufInput.ProtobufOutput");
}

TEST(TesteeTest, DerivedNameOfParseOnlyHasNoOutputFormat) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(mock, RunTest("Recommended.FooTest.Bar.ProtobufInput", _))
      .WillOnce(RespondWith(R"pb(parse_error: "")pb"));

  TestResult result =
      Checked(testee
                  .CreateTest(TestName{/*suite=*/"FooTest", /*test=*/"Bar"},
                              TestPriority::kP3)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .ParseOnly());

  EXPECT_EQ(result.name(), "Recommended.FooTest.Bar.ProtobufInput");
}

// The input format names the payload the request carries, whatever the
// message type: JSON and text-format inputs are told apart from binary ones.
TEST(TesteeTest, DerivedNameCarriesTheInputFormat) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(mock, RunTest("Required.FooTest.Bar.JsonInput.ProtobufOutput", _))
      .WillOnce(RespondWith(R"pb(protobuf_payload: "")pb"));
  EXPECT_CALL(mock,
              RunTest("Required.FooTest.Baz.TextFormatInput.JsonOutput", _))
      .WillOnce(RespondWith(R"pb(json_payload: "{}")pb"));

  TestResult json =
      Checked(testee
                  .CreateTest(TestName{/*suite=*/"FooTest", /*test=*/"Bar"},
                              TestPriority::kP0)
                  .ParseJson(TestAllTypesProto2::descriptor(), "{}")
                  .SerializeBinary());
  TestResult text = Checked(testee
                                .CreateTest(TestName{/*suite=*/"FooTest",
                                                     /*test=*/"Baz"},
                                            TestPriority::kP0)
                                .ParseText(TestAllTypesProto2::descriptor(), "")
                                .SerializeJson());

  EXPECT_EQ(json.name(), "Required.FooTest.Bar.JsonInput.ProtobufOutput");
  EXPECT_EQ(text.name(), "Required.FooTest.Baz.TextFormatInput.JsonOutput");
}

// gtest's parameter name follows the test after a "/", verbatim: the message
// type's token stays, and so do the underscores between tokens, so that the
// name matches the gtest test's own (as passed to --gtest_filter).
TEST(TesteeTest, DerivedNameKeepsTheParametersVerbatim) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(mock, RunTest(_, _))
      .Times(AnyNumber())
      .WillRepeatedly(RespondWith(R"pb(parse_error: "")pb"));
  auto name_with_params = [&](absl::string_view params) {
    TestResult result =
        Checked(testee
                    .CreateTest(TestName{/*suite=*/"FooTest",
                                         /*test=*/"Bar",
                                         /*params=*/std::string(params)},
                                TestPriority::kP0)
                    .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                    .ParseOnly());
    return std::string(result.name());
  };

  EXPECT_EQ(name_with_params(""), "Required.FooTest.Bar.ProtobufInput");
  EXPECT_EQ(name_with_params("Proto2"),
            "Required.FooTest.Bar/Proto2.ProtobufInput");
  EXPECT_EQ(name_with_params("Proto2_INT32"),
            "Required.FooTest.Bar/Proto2_INT32.ProtobufInput");
  EXPECT_EQ(name_with_params("Proto3_INT32_STRING"),
            "Required.FooTest.Bar/Proto3_INT32_STRING.ProtobufInput");
  EXPECT_EQ(name_with_params("6_0_0"),
            "Required.FooTest.Bar/6_0_0.ProtobufInput");
}

TEST(TesteeTest, DerivedNameSuffixComesBeforeTheInputFormat) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(mock, RunTest("Required.FooTest.Bar/Proto2_INT32.Print."
                            "ProtobufInput.ProtobufOutput",
                            _))
      .WillOnce(RespondWith(R"pb(protobuf_payload: "")pb"));

  TestResult result =
      Checked(testee
                  .CreateTest(TestName{/*suite=*/"FooTest",
                                       /*test=*/"Bar",
                                       /*params=*/"Proto2_INT32",
                                       /*suffix=*/"Print"},
                              TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .SerializeBinary());

  EXPECT_EQ(result.name(),
            "Required.FooTest.Bar/Proto2_INT32.Print.ProtobufInput."
            "ProtobufOutput");
}

TEST(TesteeDeathTest, DerivedNameNeedsASuiteAndATest) {
  MockTestRunner mock;
  Testee testee(&mock);
  // Spelled out as TestName{...}: a bare braced list is ambiguous between the
  // TestName and the string_view overloads.
  EXPECT_DEATH(testee.CreateTest(TestName{/*suite=*/"",
                                          /*test=*/"Bar"},
                                 TestPriority::kP0),
               "needs a gtest suite and test name");
  EXPECT_DEATH(
      testee.CreateTest(TestName{/*suite=*/"FooTest"}, TestPriority::kP0),
      "needs a gtest suite and test name");
}

// Anything but [A-Za-z0-9_] would break the segment structure of the name ("."
// and "/" are its separators) or look like a failure-list wildcard.
TEST(TesteeDeathTest, DerivedNamePiecesMustBeGtestIdentifiers) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_DEATH(
      testee.CreateTest(TestName{/*suite=*/"All/FooTest", /*test=*/"Bar"},
                        TestPriority::kP0),
      "may only contain letters, digits and '_': All/FooTest");
  EXPECT_DEATH(
      testee.CreateTest(TestName{/*suite=*/"FooTest", /*test=*/"Bar.Baz"},
                        TestPriority::kP0),
      "may only contain letters, digits and '_': Bar.Baz");
  EXPECT_DEATH(testee.CreateTest(TestName{/*suite=*/"FooTest", /*test=*/"Bar",
                                          /*params=*/"INT32/1"},
                                 TestPriority::kP0),
               "may only contain letters, digits and '_': INT32/1");
  EXPECT_DEATH(testee.CreateTest(
                   TestName{/*suite=*/"FooTest", /*test=*/"Bar", /*params=*/"",
                            /*suffix=*/"*"},
                   TestPriority::kP0),
               "may only contain letters, digits and '_': \\*");
}

TEST(TesteeTest, TextToText) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(
      mock,
      RunTest("Recommended.TesteeTest.foo.TextFormatInput.TextFormatOutput",
              RequestEquals(R"pb(
                text_payload: "text"
                requested_output_format: TEXT_FORMAT
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: TEXT_FORMAT_TEST
              )pb")))
      .WillOnce(RespondWith(R"pb(runtime_error: "error")pb"));

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP3)
                  .ParseText(TestAllTypesProto2::descriptor(), "text")
                  .SerializeText());

  EXPECT_EQ(result.name(),
            "Recommended.TesteeTest.foo.TextFormatInput.TextFormatOutput");
  EXPECT_EQ(result.priority(), TestPriority::kP3);
  EXPECT_EQ(result.type(), TestAllTypesProto2::descriptor());
  EXPECT_EQ(result.format(), ::conformance::TEXT_FORMAT);
  EXPECT_THAT(result.response(), EqualsProto(R"pb(runtime_error: "error")pb"));
}

TEST(TesteeTest, TextPrintUnknownFields) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(
      mock,
      RunTest("Required.TesteeTest.foo.ProtobufInput.TextFormatOutput",
              RequestEquals(R"pb(
                protobuf_payload: "wire"
                requested_output_format: TEXT_FORMAT
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: BINARY_TEST
                print_unknown_fields: true
              )pb")))
      .WillOnce(RespondWith(R"pb(runtime_error: "error")pb"));

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .SerializeText({/*print_unknown_fields=*/true}));

  EXPECT_EQ(result.name(),
            "Required.TesteeTest.foo.ProtobufInput.TextFormatOutput");
  EXPECT_EQ(result.priority(), TestPriority::kP0);
  EXPECT_EQ(result.type(), TestAllTypesProto2::descriptor());
  EXPECT_EQ(result.format(), ::conformance::TEXT_FORMAT);
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
      RunTest("Required.TesteeTest.foo.JsonInput.JsonOutput",
              RequestEquals(R"pb(
                json_payload: "json"
                requested_output_format: JSON
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: JSON_TEST
              )pb")))
      .WillOnce(RespondWith(R"pb(runtime_error: "error")pb"));

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseJson(TestAllTypesProto2::descriptor(), "json")
                  .SerializeJson());

  EXPECT_EQ(result.name(), "Required.TesteeTest.foo.JsonInput.JsonOutput");
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
      RunTest("Required.TesteeTest.foo.JsonInput.ProtobufOutput",
              RequestEquals(R"pb(
                json_payload: "json"
                requested_output_format: PROTOBUF
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: JSON_IGNORE_UNKNOWN_PARSING_TEST
              )pb")))
      .WillOnce(RespondWith(R"pb(runtime_error: "error")pb"));

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseJson(TestAllTypesProto2::descriptor(), "json",
                             {/*ignore_unknown_fields=*/true})
                  .SerializeBinary());

  EXPECT_EQ(result.name(), "Required.TesteeTest.foo.JsonInput.ProtobufOutput");
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
      RunTest("Required.TesteeTest.foo.ProtobufInput.ProtobufOutput",
              RequestEquals(R"pb(
                protobuf_payload: "wire"
                requested_output_format: PROTOBUF
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: BINARY_TEST
              )pb")))
      .WillOnce(Return(std::string("\004")));

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .SerializeBinary());

  EXPECT_EQ(result.name(),
            "Required.TesteeTest.foo.ProtobufInput.ProtobufOutput");
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
      RunTest("Required.TesteeTest.foo.ProtobufInput.ProtobufOutput",
              RequestEquals(R"pb(
                protobuf_payload: "wire"
                requested_output_format: PROTOBUF
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: BINARY_TEST
              )pb")))
      .WillRepeatedly(Return(std::string("\004")));

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .SerializeBinary());

  EXPECT_DEATH(testee.CreateTest(Foo(), TestPriority::kP0)
                   .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                   .SerializeBinary(),
               "Duplicated test name: "
               "Required.TesteeTest.foo.ProtobufInput.ProtobufOutput");
}

TEST(TesteeTest, ParseOnlyBinary) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(
      mock,
      RunTest("Required.TesteeTest.foo.ProtobufInput", RequestEquals(R"pb(
                protobuf_payload: "wire"
                requested_output_format: PROTOBUF
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: BINARY_TEST
              )pb")))
      .WillOnce(RespondWith(R"pb(parse_error: "error")pb"));

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .ParseOnly());

  EXPECT_EQ(result.name(), "Required.TesteeTest.foo.ProtobufInput");
  EXPECT_EQ(result.priority(), TestPriority::kP0);
  EXPECT_EQ(result.type(), TestAllTypesProto2::descriptor());
  EXPECT_EQ(result.format(), ::conformance::PROTOBUF);
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
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .SerializeBinary());
  TestResult parse_only =
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .ParseOnly());

  EXPECT_EQ(serialized.name(),
            "Required.TesteeTest.foo.ProtobufInput.ProtobufOutput");
  EXPECT_EQ(parse_only.name(), "Required.TesteeTest.foo.ProtobufInput");
  EXPECT_EQ(parse_only.request().SerializeAsString(),
            serialized.request().SerializeAsString());
}

TEST(TesteeTest, ParseOnlyText) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(
      mock,
      RunTest("Recommended.TesteeTest.foo.TextFormatInput", RequestEquals(R"pb(
                text_payload: "text"
                requested_output_format: TEXT_FORMAT
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: TEXT_FORMAT_TEST
              )pb")))
      .WillOnce(RespondWith(R"pb(parse_error: "error")pb"));

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP3)
                  .ParseText(TestAllTypesProto2::descriptor(), "text")
                  .ParseOnly());

  EXPECT_EQ(result.name(), "Recommended.TesteeTest.foo.TextFormatInput");
  EXPECT_EQ(result.priority(), TestPriority::kP3);
  EXPECT_EQ(result.format(), ::conformance::TEXT_FORMAT);
  EXPECT_THAT(result.response(), EqualsProto(R"pb(parse_error: "error")pb"));
}

TEST(TesteeTest, ParseOnlyJson) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(
      mock,
      RunTest("Required.TesteeTest.foo.JsonInput", RequestEquals(R"pb(
                json_payload: "json"
                requested_output_format: JSON
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: JSON_TEST
              )pb")))
      .WillOnce(RespondWith(R"pb(parse_error: "error")pb"));

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseJson(TestAllTypesProto2::descriptor(), "json")
                  .ParseOnly());

  EXPECT_EQ(result.name(), "Required.TesteeTest.foo.JsonInput");
  EXPECT_EQ(result.format(), ::conformance::JSON);
  EXPECT_THAT(result.response(), EqualsProto(R"pb(parse_error: "error")pb"));
}

TEST(TesteeTest, ParseOnlyDuplicateTestName) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(mock, RunTest("Required.TesteeTest.foo.ProtobufInput", _))
      .WillRepeatedly(RespondWith(R"pb(parse_error: "error")pb"));

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .ParseOnly());

  EXPECT_DEATH(testee.CreateTest(Foo(), TestPriority::kP0)
                   .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                   .ParseOnly(),
               "Duplicated test name: Required.TesteeTest.foo.ProtobufInput");
}

TEST(TesteeTest, OverrideTestCategory) {
  MockTestRunner mock;
  Testee testee(&mock);
  // The category changes, the name (derived from the input format) doesn't.
  EXPECT_CALL(
      mock,
      RunTest("Required.TesteeTest.foo.ProtobufInput.TextFormatOutput",
              RequestEquals(R"pb(
                protobuf_payload: "wire"
                requested_output_format: TEXT_FORMAT
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: TEXT_FORMAT_TEST
              )pb")))
      .WillOnce(RespondWith(R"pb(text_payload: "")pb"));

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .OverrideTestCategory(::conformance::TEXT_FORMAT_TEST)
                  .SerializeText());

  EXPECT_EQ(result.name(),
            "Required.TesteeTest.foo.ProtobufInput.TextFormatOutput");
  EXPECT_THAT(result.request(), EqualsProto(R"pb(
                protobuf_payload: "wire"
                requested_output_format: TEXT_FORMAT
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: TEXT_FORMAT_TEST
              )pb"));
}

TEST(TesteeTest, OverrideTestCategoryParseOnly) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(
      mock,
      RunTest("Required.TesteeTest.foo.ProtobufInput", RequestEquals(R"pb(
                protobuf_payload: "wire"
                requested_output_format: PROTOBUF
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: TEXT_FORMAT_TEST
              )pb")))
      .WillOnce(RespondWith(R"pb(parse_error: "error")pb"));

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .OverrideTestCategory(::conformance::TEXT_FORMAT_TEST)
                  .ParseOnly());

  EXPECT_EQ(result.name(), "Required.TesteeTest.foo.ProtobufInput");
  EXPECT_EQ(result.request().test_category(), ::conformance::TEXT_FORMAT_TEST);
}

TEST(TesteeTest, DiscardUnknownFields) {
  MockTestRunner mock;
  Testee testee(&mock);
  // The request asks for the unknown fields to be discarded; the name doesn't
  // change.
  EXPECT_CALL(
      mock,
      RunTest("Required.TesteeTest.foo.ProtobufInput.ProtobufOutput",
              RequestEquals(R"pb(
                protobuf_payload: "wire"
                requested_output_format: PROTOBUF
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: BINARY_TEST
                discard_unknown_fields: true
              )pb")))
      .WillOnce(RespondWith(R"pb(protobuf_payload: "")pb"));

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .DiscardUnknownFields()
                  .SerializeBinary());

  EXPECT_EQ(result.name(),
            "Required.TesteeTest.foo.ProtobufInput.ProtobufOutput");
  EXPECT_TRUE(result.request().discard_unknown_fields());
}

TEST(TesteeTest, DiscardUnknownFieldsFromTextInputToTextOutput) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(
      mock,
      RunTest("Required.TesteeTest.foo.TextFormatInput.TextFormatOutput",
              RequestEquals(R"pb(
                text_payload: "text"
                requested_output_format: TEXT_FORMAT
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: TEXT_FORMAT_TEST
                discard_unknown_fields: true
                print_unknown_fields: true
              )pb")))
      .WillOnce(RespondWith(R"pb(text_payload: "")pb"));

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseText(TestAllTypesProto2::descriptor(), "text")
                  .DiscardUnknownFields()
                  .SerializeText({/*print_unknown_fields=*/true}));

  EXPECT_TRUE(result.request().discard_unknown_fields());
}

TEST(TesteeTest, MergeBinary) {
  MockTestRunner mock;
  Testee testee(&mock);
  // The merge payload rides along in the request; the name doesn't change.
  EXPECT_CALL(
      mock,
      RunTest("Required.TesteeTest.foo.ProtobufInput.ProtobufOutput",
              RequestEquals(R"pb(
                protobuf_payload: "wire"
                merge_protobuf_payload: "more"
                requested_output_format: PROTOBUF
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: BINARY_TEST
              )pb")))
      .WillOnce(RespondWith(R"pb(protobuf_payload: "")pb"));

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .MergeBinary(Wire("more"))
                  .SerializeBinary());

  EXPECT_EQ(result.name(),
            "Required.TesteeTest.foo.ProtobufInput.ProtobufOutput");
  EXPECT_EQ(result.request().merge_protobuf_payload(), "more");
}

TEST(TesteeTest, MergeTextIntoBinaryInput) {
  MockTestRunner mock;
  Testee testee(&mock);
  // The merge payload's format is independent of the input's and doesn't show
  // up in the name.
  EXPECT_CALL(
      mock,
      RunTest("Required.TesteeTest.foo.ProtobufInput.ProtobufOutput",
              RequestEquals(R"pb(
                protobuf_payload: "wire"
                merge_text_payload: "text"
                requested_output_format: PROTOBUF
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: BINARY_TEST
              )pb")))
      .WillOnce(RespondWith(R"pb(protobuf_payload: "")pb"));

  Checked(testee.CreateTest(Foo(), TestPriority::kP0)
              .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
              .MergeText("text")
              .SerializeBinary());
}

TEST(TesteeTest, MergeJsonIntoJsonInputKeepsTheCategory) {
  MockTestRunner mock;
  Testee testee(&mock);
  // A JSON merge payload is parsed with the input's ignore_unknown_fields
  // setting: the request has a single category.
  EXPECT_CALL(
      mock,
      RunTest("Required.TesteeTest.foo.JsonInput.JsonOutput",
              RequestEquals(R"pb(
                json_payload: "json"
                merge_json_payload: "more"
                requested_output_format: JSON
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: JSON_IGNORE_UNKNOWN_PARSING_TEST
              )pb")))
      .WillOnce(RespondWith(R"pb(json_payload: "{}")pb"));

  Checked(testee.CreateTest(Foo(), TestPriority::kP0)
              .ParseJson(TestAllTypesProto2::descriptor(), "json",
                         {/*ignore_unknown_fields=*/true})
              .MergeJson("more")
              .SerializeJson());
}

TEST(TesteeTest, MergeJsonIntoBinaryInputIsStrict) {
  MockTestRunner mock;
  Testee testee(&mock);
  // After a binary input the category is BINARY_TEST, so a JSON merge payload
  // is parsed strictly: there is no way to ask for ignore_unknown_fields.
  EXPECT_CALL(
      mock,
      RunTest("Required.TesteeTest.foo.ProtobufInput.ProtobufOutput",
              RequestEquals(R"pb(
                protobuf_payload: "wire"
                merge_json_payload: "{}"
                requested_output_format: PROTOBUF
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: BINARY_TEST
              )pb")))
      .WillOnce(RespondWith(R"pb(protobuf_payload: "")pb"));

  Checked(testee.CreateTest(Foo(), TestPriority::kP0)
              .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
              .MergeJson("{}")
              .SerializeBinary());
}

TEST(TesteeTest, MergeThenDiscardUnknownFields) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(
      mock,
      RunTest("Required.TesteeTest.foo.ProtobufInput.ProtobufOutput",
              RequestEquals(R"pb(
                protobuf_payload: "wire"
                merge_protobuf_payload: "more"
                requested_output_format: PROTOBUF
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: BINARY_TEST
                discard_unknown_fields: true
              )pb")))
      .WillOnce(RespondWith(R"pb(protobuf_payload: "")pb"));

  Checked(testee.CreateTest(Foo(), TestPriority::kP0)
              .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
              .MergeBinary(Wire("more"))
              .DiscardUnknownFields()
              .SerializeBinary());
}

// MergeFrom() of a merely parsed message of the same type, before anything
// else is done to either, is the very shape the Merge*() shortcut records, so
// a version 1 testee gets exactly the flat merge_protobuf_payload request the
// shortcut produces for the same payloads.
TEST(TesteeTest, MergeFromLowersToTheShortcutOnV1) {
  MockTestRunner mock;
  Testee testee(&mock);
  std::string shortcut_request;
  std::string merge_from_request;
  EXPECT_CALL(mock, RunTest("Required.TesteeTest.foo.ProtobufInput."
                            "ProtobufOutput",
                            _))
      .WillOnce([&](absl::string_view, absl::string_view input) {
        shortcut_request = std::string(input);
        return SerializedResponse(R"pb(protobuf_payload: "")pb");
      });
  EXPECT_CALL(mock, RunTest("Required.TesteeTest.bar.ProtobufInput."
                            "ProtobufOutput",
                            _))
      .WillOnce([&](absl::string_view, absl::string_view input) {
        merge_from_request = std::string(input);
        return SerializedResponse(R"pb(protobuf_payload: "")pb");
      });

  Checked(testee.CreateTest(Foo(), TestPriority::kP0)
              .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
              .MergeBinary(Wire("more"))
              .SerializeBinary());
  const TestName bar{/*suite=*/"TesteeTest", /*test=*/"bar"};
  TestResult result =
      Checked(testee.CreateTest(bar, TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .MergeFrom(testee.CreateTest(bar, TestPriority::kP0)
                                 .ParseBinary(TestAllTypesProto2::descriptor(),
                                              Wire("more")))
                  .SerializeBinary());

  EXPECT_EQ(result.required_protocol_version(), 1);
  EXPECT_THAT(result.request(), EqualsProto(R"pb(
                protobuf_payload: "wire"
                merge_protobuf_payload: "more"
                requested_output_format: PROTOBUF
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: BINARY_TEST
              )pb"));
  EXPECT_EQ(merge_from_request, shortcut_request);
}

TEST(TesteeDeathTest, MergeAtMostOnce) {
  MockTestRunner mock;
  Testee testee(&mock);
  // Without the DCHECK (opt builds) the request goes through.
  EXPECT_CALL(mock, RunTest)
      .Times(AnyNumber())
      .WillRepeatedly(RespondWith(R"pb(parse_error: "error")pb"));
  EXPECT_DEBUG_DEATH(
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .MergeBinary(Wire("more"))
                  .MergeText("text")
                  .SerializeBinary()),
      "may be called once per test");
}

TEST(TesteeDeathTest, MergeMustPrecedeDiscardUnknownFields) {
  MockTestRunner mock;
  Testee testee(&mock);
  // Without the DCHECK (opt builds) the request goes through.
  EXPECT_CALL(mock, RunTest)
      .Times(AnyNumber())
      .WillRepeatedly(RespondWith(R"pb(parse_error: "error")pb"));
  EXPECT_DEBUG_DEATH(
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .DiscardUnknownFields()
                  .MergeBinary(Wire("more"))
                  .SerializeBinary()),
      "must come before DiscardUnknownFields");
}

TEST(TesteeDeathTest, OverrideTestCategoryAcceptsOnlyTextFormatTest) {
  MockTestRunner mock;
  Testee testee(&mock);
  // Without the DCHECK (opt builds) the request goes through as relabeled.
  EXPECT_CALL(mock, RunTest)
      .Times(AnyNumber())
      .WillRepeatedly(RespondWith(R"pb(parse_error: "error")pb"));
  EXPECT_DEBUG_DEATH(
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .OverrideTestCategory(::conformance::JSON_TEST)
                  .ParseOnly()),
      "only relabels the text-format suite's binary-input tests");
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

// ---------------------------------------------------------------------------
// The protocol-neutral view of a v1 request and response
// ---------------------------------------------------------------------------

using Outcome = TestResult::Outcome;
using ::testing::IsEmpty;
using ::testing::IsNull;

// Builds a checked result straight from a v1 request and response, so that
// each test can pick exactly the response case it is about.
TestResult ViewOf(absl::string_view request_textproto,
                  absl::string_view response_textproto) {
  ::conformance::ConformanceRequest request;
  ABSL_CHECK(TextFormat::ParseFromString(request_textproto, &request));
  ::conformance::ConformanceResponse response;
  ABSL_CHECK(TextFormat::ParseFromString(response_textproto, &response));
  return Checked(TestResult::ForTesting(
      "some.name", TestPriority::kP0, TestAllTypesProto2::descriptor(),
      std::move(request), std::move(response)));
}

// A result whose response is an error or a skip carries no output, only the
// message.
MATCHER_P2(IsNonOutput, outcome, message, "") {
  return ExplainMatchResult(outcome, arg.outcome(), result_listener) &&
         ExplainMatchResult(message, arg.message(), result_listener) &&
         ExplainMatchResult(IsNull(), arg.output(), result_listener) &&
         ExplainMatchResult(IsEmpty(), arg.outputs(), result_listener);
}

// A result whose response is a payload carries exactly that output, in the
// format implied by the response field, and no message.
MATCHER_P2(IsSingleOutput, format, payload, "") {
  const TestResult::Output* output = arg.output();
  if (output == nullptr) {
    *result_listener << "which has no output";
    return false;
  }
  return ExplainMatchResult(Outcome::kOutput, arg.outcome(), result_listener) &&
         ExplainMatchResult(IsEmpty(), arg.message(), result_listener) &&
         ExplainMatchResult(format, output->format, result_listener) &&
         ExplainMatchResult(payload, output->payload, result_listener) &&
         arg.outputs().size() == 1 && &arg.outputs()[0] == output;
}

TEST(TestResultViewTest, NoResult) {
  EXPECT_THAT(ViewOf(R"pb(protobuf_payload: "wire")pb", ""),
              IsNonOutput(Outcome::kNoResult, ""));
}

TEST(TestResultViewTest, ParseError) {
  EXPECT_THAT(ViewOf(R"pb(protobuf_payload: "wire")pb",
                     R"pb(parse_error: "bad input")pb"),
              IsNonOutput(Outcome::kParseError, "bad input"));
}

TEST(TestResultViewTest, SerializeError) {
  EXPECT_THAT(ViewOf(R"pb(protobuf_payload: "wire")pb",
                     R"pb(serialize_error: "bad output")pb"),
              IsNonOutput(Outcome::kSerializeError, "bad output"));
}

TEST(TestResultViewTest, RuntimeError) {
  EXPECT_THAT(
      ViewOf(R"pb(protobuf_payload: "wire")pb", R"pb(runtime_error: "boom")pb"),
      IsNonOutput(Outcome::kRuntimeError, "boom"));
}

TEST(TestResultViewTest, TimeoutError) {
  EXPECT_THAT(ViewOf(R"pb(protobuf_payload: "wire")pb",
                     R"pb(timeout_error: "too slow")pb"),
              IsNonOutput(Outcome::kTimeoutError, "too slow"));
}

TEST(TestResultViewTest, Skipped) {
  EXPECT_THAT(ViewOf(R"pb(protobuf_payload: "wire")pb",
                     R"pb(skipped: "not supported")pb"),
              IsNonOutput(Outcome::kSkipped, "not supported"));
}

// An empty error or skip message is still that outcome, just with an empty
// message; it must not be mistaken for kNoResult.
TEST(TestResultViewTest, EmptyMessageKeepsTheOutcome) {
  EXPECT_THAT(ViewOf("", R"pb(parse_error: "")pb"),
              IsNonOutput(Outcome::kParseError, ""));
  EXPECT_THAT(ViewOf("", R"pb(skipped: "")pb"),
              IsNonOutput(Outcome::kSkipped, ""));
}

TEST(TestResultViewTest, ProtobufPayload) {
  EXPECT_THAT(ViewOf(R"pb(protobuf_payload: "wire"
                          requested_output_format: PROTOBUF)pb",
                     R"pb(protobuf_payload: "\010\t")pb"),
              IsSingleOutput(::conformance::PROTOBUF, "\010\t"));
}

// The raw strings live outside the macro because MSVC's preprocessor mangles
// escaped quotes inside raw string literals that are macro arguments.
constexpr absl::string_view kJsonPayloadResponse =
    R"pb(json_payload: "{\"optionalInt32\": 9}")pb";
constexpr absl::string_view kJsonPayloadOutput = R"({"optionalInt32": 9})";

TEST(TestResultViewTest, JsonPayload) {
  EXPECT_THAT(ViewOf(R"pb(json_payload: "{}" requested_output_format: JSON)pb",
                     kJsonPayloadResponse),
              IsSingleOutput(::conformance::JSON, kJsonPayloadOutput));
}

TEST(TestResultViewTest, TextPayload) {
  EXPECT_THAT(
      ViewOf(R"pb(text_payload: "" requested_output_format: TEXT_FORMAT)pb",
             R"pb(text_payload: "optional_int32: 9")pb"),
      IsSingleOutput(::conformance::TEXT_FORMAT, "optional_int32: 9"));
}

TEST(TestResultViewTest, JspbPayload) {
  EXPECT_THAT(ViewOf(R"pb(jspb_payload: "[]" requested_output_format: JSPB)pb",
                     R"pb(jspb_payload: "[9]")pb"),
              IsSingleOutput(::conformance::JSPB, "[9]"));
}

// An empty payload is still an output: a testee that serializes the default
// instance answers with an empty string.
TEST(TestResultViewTest, EmptyPayloadIsAnOutput) {
  EXPECT_THAT(ViewOf(R"pb(protobuf_payload: "wire"
                          requested_output_format: PROTOBUF)pb",
                     R"pb(protobuf_payload: "")pb"),
              IsSingleOutput(::conformance::PROTOBUF, ""));
}

// The output's format is the one the testee answered in, which the matchers
// compare against the requested format(); the view doesn't hide a mismatch.
TEST(TestResultViewTest, WrongFormatPayloadKeepsTheTesteesFormat) {
  TestResult result = ViewOf(R"pb(protobuf_payload: "wire"
                                  requested_output_format: TEXT_FORMAT)pb",
                             R"pb(json_payload: "{}")pb");
  EXPECT_EQ(result.format(), ::conformance::TEXT_FORMAT);
  EXPECT_THAT(result, IsSingleOutput(::conformance::JSON, "{}"));
}

TEST(TestResultViewTest, InputFormat) {
  EXPECT_EQ(ViewOf(R"pb(protobuf_payload: "wire")pb", "").input_format(),
            ::conformance::PROTOBUF);
  EXPECT_EQ(ViewOf(R"pb(json_payload: "{}")pb", "").input_format(),
            ::conformance::JSON);
  EXPECT_EQ(ViewOf(R"pb(text_payload: "")pb", "").input_format(),
            ::conformance::TEXT_FORMAT);
  EXPECT_EQ(ViewOf(R"pb(jspb_payload: "[]")pb", "").input_format(),
            ::conformance::JSPB);
  // Only a ForTesting() result can lack an input altogether.
  EXPECT_EQ(ViewOf("", "").input_format(), ::conformance::UNSPECIFIED);
}

// The merge payload is a second input and doesn't affect the input format.
TEST(TestResultViewTest, InputFormatIgnoresTheMergePayload) {
  EXPECT_EQ(
      ViewOf(R"pb(protobuf_payload: "wire" merge_json_payload: "{}")pb", "")
          .input_format(),
      ::conformance::PROTOBUF);
}

TEST(TestResultViewTest, PrintUnknownFields) {
  EXPECT_FALSE(
      ViewOf(R"pb(protobuf_payload: "wire")pb", "").print_unknown_fields());
  EXPECT_TRUE(
      ViewOf(R"pb(protobuf_payload: "wire" print_unknown_fields: true)pb", "")
          .print_unknown_fields());
}

// The view comes out the same whether the result was built by the Testee or
// by ForTesting(): both go through the same constructor.
TEST(TestResultViewTest, MatchesTheTesteesResult) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(mock, RunTest)
      .WillOnce(RespondWith(R"pb(text_payload: "optional_int32: 9")pb"));

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .SerializeText({/*print_unknown_fields=*/true}));

  EXPECT_EQ(result.input_format(), ::conformance::PROTOBUF);
  EXPECT_EQ(result.format(), ::conformance::TEXT_FORMAT);
  EXPECT_TRUE(result.print_unknown_fields());
  EXPECT_THAT(result,
              IsSingleOutput(::conformance::TEXT_FORMAT, "optional_int32: 9"));
}

TEST(TestResultViewTest, SurvivesMoves) {
  TestResult original = ViewOf(R"pb(json_payload: "{}"
                                    requested_output_format: JSON
                                    print_unknown_fields: true)pb",
                               R"pb(json_payload: "{}")pb");

  TestResult moved(std::move(original));
  EXPECT_EQ(moved.input_format(), ::conformance::JSON);
  EXPECT_TRUE(moved.print_unknown_fields());
  EXPECT_THAT(moved, IsSingleOutput(::conformance::JSON, "{}"));

  TestResult assigned = ViewOf("", R"pb(skipped: "no")pb");
  assigned = std::move(moved);
  EXPECT_EQ(assigned.input_format(), ::conformance::JSON);
  EXPECT_TRUE(assigned.print_unknown_fields());
  EXPECT_THAT(assigned, IsSingleOutput(::conformance::JSON, "{}"));

  TestResult error = ViewOf("", R"pb(runtime_error: "boom")pb");
  TestResult moved_error(std::move(error));
  EXPECT_THAT(moved_error, IsNonOutput(Outcome::kRuntimeError, "boom"));
}

TEST(TestResultViewTest, OutcomePrintsByName) {
  EXPECT_EQ(PrintToString(Outcome::kNoResult), "kNoResult");
  EXPECT_EQ(PrintToString(Outcome::kParseError), "kParseError");
  EXPECT_EQ(PrintToString(Outcome::kSerializeError), "kSerializeError");
  EXPECT_EQ(PrintToString(Outcome::kRuntimeError), "kRuntimeError");
  EXPECT_EQ(PrintToString(Outcome::kTimeoutError), "kTimeoutError");
  EXPECT_EQ(PrintToString(Outcome::kSkipped), "kSkipped");
  EXPECT_EQ(PrintToString(Outcome::kOutput), "kOutput");
  EXPECT_EQ(PrintToString(Outcome::kUnsupportedProtocol),
            "kUnsupportedProtocol");
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
      testee.CreateTest(Foo(), priority)
          .ParseBinary(TestAllTypesProto2::descriptor(), std::move(input))
          .SerializeBinary());
}

constexpr absl::string_view kPrintedRequiredRequest =
    R"(Required test "Required.TesteeTest.foo.ProtobufInput.ProtobufOutput" )"
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
      R"(Recommended test "Recommended.TesteeTest.foo.ProtobufInput.ProtobufOutput" )"
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
          R"(Required test "Required.TesteeTest.foo.ProtobufInput.ProtobufOutput" )"
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
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseText(TestAllTypesProto2::descriptor(), large_payload)
                  .SerializeText());

  EXPECT_EQ(
      PrintToString(result),
      absl::StrCat(
          R"(Required test "Required.TesteeTest.foo.TextFormatInput.TextFormatOutput" )"
          R"(with request {requested_output_format: TEXT_FORMAT )"
          R"(message_type: "protobuf_test_messages.proto2.TestAllTypesProto2" )"
          "test_category: TEXT_FORMAT_TEST text_payload: \"",
          std::string(200, 'a'), "...(truncated)",
          "\"} and response {runtime_error: \"error\"}"));
}

// ---------------------------------------------------------------------------
// JSON test names and ParseOnly() output override
// ---------------------------------------------------------------------------

TEST(TesteeTest, SerializeJsonNamesTheOutputFormat) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(mock, RunTest("Required.TesteeTest.foo.JsonInput.JsonOutput", _))
      .WillOnce(RespondWith(R"pb(json_payload: "{}")pb"));

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseJson(TestAllTypesProto2::descriptor(), "json")
                  .SerializeJson());

  EXPECT_EQ(result.name(), "Required.TesteeTest.foo.JsonInput.JsonOutput");
  EXPECT_EQ(result.format(), ::conformance::JSON);
}

TEST(TesteeTest, SerializeJsonFromBinaryInput) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(
      mock,
      RunTest("Recommended.TesteeTest.foo.ProtobufInput.JsonOutput",
              RequestEquals(R"pb(
                protobuf_payload: "wire"
                requested_output_format: JSON
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: BINARY_TEST
              )pb")))
      .WillOnce(RespondWith(R"pb(serialize_error: "error")pb"));

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP3)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .SerializeJson());

  EXPECT_EQ(result.name(),
            "Recommended.TesteeTest.foo.ProtobufInput.JsonOutput");
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
      RunTest("Required.TesteeTest.foo.JsonInput", RequestEquals(R"pb(
                json_payload: "json"
                requested_output_format: PROTOBUF
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: JSON_TEST
              )pb")))
      .WillOnce(RespondWith(R"pb(parse_error: "error")pb"));

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseJson(TestAllTypesProto2::descriptor(), "json")
                  .ParseOnly({/*output_format=*/::conformance::PROTOBUF}));

  EXPECT_EQ(result.name(), "Required.TesteeTest.foo.JsonInput");
  EXPECT_EQ(result.format(), ::conformance::PROTOBUF);
  EXPECT_THAT(result.request(), EqualsProto(R"pb(
                json_payload: "json"
                requested_output_format: PROTOBUF
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: JSON_TEST
              )pb"));
  EXPECT_THAT(result.response(), EqualsProto(R"pb(parse_error: "error")pb"));
}

// With the output format overridden, the request is byte-identical to the one
// SerializeBinary() sends for the same input; only the name differs.
TEST(TesteeTest, ParseOnlyWithProtobufOutputSendsSameRequestAsSerializeBinary) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(mock, RunTest)
      .WillRepeatedly(RespondWith(R"pb(parse_error: "error")pb"));

  TestResult serialized =
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseJson(TestAllTypesProto2::descriptor(), "json")
                  .SerializeBinary());
  TestResult parse_only =
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseJson(TestAllTypesProto2::descriptor(), "json")
                  .ParseOnly({/*output_format=*/::conformance::PROTOBUF}));

  EXPECT_EQ(serialized.name(),
            "Required.TesteeTest.foo.JsonInput.ProtobufOutput");
  EXPECT_EQ(parse_only.name(), "Required.TesteeTest.foo.JsonInput");
  EXPECT_EQ(parse_only.request().SerializeAsString(),
            serialized.request().SerializeAsString());
}

TEST(TesteeTest, ParseOnlyWithExplicitInputFormatIsTheDefault) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(
      mock,
      RunTest("Required.TesteeTest.foo.JsonInput", RequestEquals(R"pb(
                json_payload: "json"
                requested_output_format: JSON
                message_type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                test_category: JSON_TEST
              )pb")))
      .WillOnce(RespondWith(R"pb(parse_error: "error")pb"));

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseJson(TestAllTypesProto2::descriptor(), "json")
                  .ParseOnly({/*output_format=*/::conformance::JSON}));

  EXPECT_EQ(result.name(), "Required.TesteeTest.foo.JsonInput");
  EXPECT_EQ(result.format(), ::conformance::JSON);
}

// ---------------------------------------------------------------------------
// Protocol version 2: action lists
// ---------------------------------------------------------------------------

using ::testing::ElementsAre;
using ::testing::Field;
using ::testing::HasSubstr;
using ::testing::Optional;
using ::testing::SizeIs;
using ::testing::StrictMock;

// Parses `textproto` as a ConformanceRequest and returns its actions.
std::vector<::conformance::ConformanceAction> ActionsOf(
    absl::string_view textproto) {
  ::conformance::ConformanceRequest request;
  ABSL_CHECK(TextFormat::ParseFromString(textproto, &request));
  return std::vector<::conformance::ConformanceAction>(
      request.actions().begin(), request.actions().end());
}

TEST(RequiredProtocolVersionTest, FlatShapesNeedVersion1) {
  EXPECT_EQ(RequiredProtocolVersion(ActionsOf(R"pb(
              actions {
                parse {
                  type: "T"
                  id: 0
                  binary { data: "a" }
                }
              }
              actions {
                serialize {
                  id: 0
                  binary {}
                }
              }
            )pb")),
            1);
  EXPECT_EQ(RequiredProtocolVersion(ActionsOf(R"pb(
              actions {
                parse {
                  type: "T"
                  id: 0
                  text { data: "a" }
                }
              }
              actions {
                parse {
                  type: "T"
                  id: 1
                  json { data: "b" }
                }
              }
              actions { merge { from: 1 to: 0 } }
              actions {
                serialize {
                  id: 0
                  text { print_unknown_fields: true }
                }
              }
            )pb")),
            1);
  EXPECT_EQ(RequiredProtocolVersion(ActionsOf(R"pb(
              actions {
                parse {
                  type: "T"
                  id: 0
                  binary { data: "a" }
                }
              }
              actions { discard_unknown_fields { id: 0 } }
              actions {
                serialize {
                  id: 0
                  json {}
                }
              }
            )pb")),
            1);
  EXPECT_EQ(RequiredProtocolVersion(ActionsOf(R"pb(
              actions {
                parse {
                  type: "T"
                  id: 0
                  binary { data: "a" }
                }
              }
              actions {
                parse {
                  type: "T"
                  id: 1
                  binary { data: "b" }
                }
              }
              actions { merge { from: 1 to: 0 } }
              actions { discard_unknown_fields { id: 0 } }
              actions {
                serialize {
                  id: 0
                  binary {}
                }
              }
            )pb")),
            1);
  // A lenient JSON merge payload after a lenient JSON input is what the flat
  // request's single test_category says.
  EXPECT_EQ(RequiredProtocolVersion(ActionsOf(R"pb(
              actions {
                parse {
                  type: "T"
                  id: 0
                  json { data: "a" ignore_unknown_fields: true }
                }
              }
              actions {
                parse {
                  type: "T"
                  id: 1
                  json { data: "b" ignore_unknown_fields: true }
                }
              }
              actions { merge { from: 1 to: 0 } }
              actions {
                serialize {
                  id: 0
                  json {}
                }
              }
            )pb")),
            1);
}

TEST(RequiredProtocolVersionTest, AnythingElseNeedsVersion2) {
  // Nothing at all.
  EXPECT_EQ(RequiredProtocolVersion({}), 2);
  // A message created empty.
  EXPECT_EQ(RequiredProtocolVersion(ActionsOf(R"pb(
              actions { new_message { type: "T" id: 0 } }
              actions {
                serialize {
                  id: 0
                  binary {}
                }
              }
            )pb")),
            2);
  // More than one output.
  EXPECT_EQ(RequiredProtocolVersion(ActionsOf(R"pb(
              actions {
                parse {
                  type: "T"
                  id: 0
                  binary { data: "a" }
                }
              }
              actions {
                serialize {
                  id: 0
                  binary {}
                }
              }
              actions {
                serialize {
                  id: 0
                  json {}
                }
              }
            )pb")),
            2);
  // No output.
  EXPECT_EQ(RequiredProtocolVersion(ActionsOf(R"pb(
              actions {
                parse {
                  type: "T"
                  id: 0
                  binary { data: "a" }
                }
              }
            )pb")),
            2);
  // A merge after discarding unknown fields.
  EXPECT_EQ(RequiredProtocolVersion(ActionsOf(R"pb(
              actions {
                parse {
                  type: "T"
                  id: 0
                  binary { data: "a" }
                }
              }
              actions { discard_unknown_fields { id: 0 } }
              actions {
                parse {
                  type: "T"
                  id: 1
                  binary { data: "b" }
                }
              }
              actions { merge { from: 1 to: 0 } }
              actions {
                serialize {
                  id: 0
                  binary {}
                }
              }
            )pb")),
            2);
  // Two merges.
  EXPECT_EQ(RequiredProtocolVersion(ActionsOf(R"pb(
              actions {
                parse {
                  type: "T"
                  id: 0
                  binary { data: "a" }
                }
              }
              actions {
                parse {
                  type: "T"
                  id: 1
                  binary { data: "b" }
                }
              }
              actions { merge { from: 1 to: 0 } }
              actions {
                parse {
                  type: "T"
                  id: 2
                  binary { data: "c" }
                }
              }
              actions { merge { from: 2 to: 0 } }
              actions {
                serialize {
                  id: 0
                  binary {}
                }
              }
            )pb")),
            2);
  // A merge the other way round.
  EXPECT_EQ(RequiredProtocolVersion(ActionsOf(R"pb(
              actions {
                parse {
                  type: "T"
                  id: 0
                  binary { data: "a" }
                }
              }
              actions {
                parse {
                  type: "T"
                  id: 1
                  binary { data: "b" }
                }
              }
              actions { merge { from: 0 to: 1 } }
              actions {
                serialize {
                  id: 0
                  binary {}
                }
              }
            )pb")),
            2);
  // Serializing the merge source.
  EXPECT_EQ(RequiredProtocolVersion(ActionsOf(R"pb(
              actions {
                parse {
                  type: "T"
                  id: 0
                  binary { data: "a" }
                }
              }
              actions {
                parse {
                  type: "T"
                  id: 1
                  binary { data: "b" }
                }
              }
              actions { merge { from: 1 to: 0 } }
              actions {
                serialize {
                  id: 1
                  binary {}
                }
              }
            )pb")),
            2);
  // A lenient JSON merge payload after a strict input (or the reverse) has no
  // single test_category.
  EXPECT_EQ(RequiredProtocolVersion(ActionsOf(R"pb(
              actions {
                parse {
                  type: "T"
                  id: 0
                  binary { data: "a" }
                }
              }
              actions {
                parse {
                  type: "T"
                  id: 1
                  json { data: "b" ignore_unknown_fields: true }
                }
              }
              actions { merge { from: 1 to: 0 } }
              actions {
                serialize {
                  id: 0
                  binary {}
                }
              }
            )pb")),
            2);
  EXPECT_EQ(RequiredProtocolVersion(ActionsOf(R"pb(
              actions {
                parse {
                  type: "T"
                  id: 0
                  json { data: "a" ignore_unknown_fields: true }
                }
              }
              actions {
                parse {
                  type: "T"
                  id: 1
                  json { data: "b" }
                }
              }
              actions { merge { from: 1 to: 0 } }
              actions {
                serialize {
                  id: 0
                  json {}
                }
              }
            )pb")),
            2);
}

// The flat request has a single message_type, so a merge payload of another
// type can't be lowered into it: LowerToV1 would drop the type and turn the
// test into a same-type merge.
TEST(RequiredProtocolVersionTest, CrossTypeMergeNeedsV2) {
  EXPECT_EQ(RequiredProtocolVersion(ActionsOf(R"pb(
              actions {
                parse {
                  type: "T"
                  id: 0
                  binary { data: "a" }
                }
              }
              actions {
                parse {
                  type: "U"
                  id: 1
                  binary { data: "b" }
                }
              }
              actions { merge { from: 1 to: 0 } }
              actions {
                serialize {
                  id: 0
                  binary {}
                }
              }
            )pb")),
            2);
}

// A test merging a message of another type is never sent to a version 1
// testee as a flat request: the runner skips it.
TEST(TesteeV2Test, CrossTypeMergeFromIsUnsupportedOnAV1Testee) {
  StrictMock<MockTestRunner> mock;
  Testee testee(&mock);
  EXPECT_CALL(mock, RunTest).Times(0);

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .MergeFrom(testee.CreateTest(Foo(), TestPriority::kP0)
                                 .ParseBinary(TestAllTypesProto3::descriptor(),
                                              Wire("more")))
                  .SerializeBinary());

  EXPECT_EQ(result.outcome(), TestResult::Outcome::kUnsupportedProtocol);
  EXPECT_EQ(result.required_protocol_version(), 2);
  // The request that would have been sent keeps both types.
  EXPECT_THAT(result.request(), EqualsProto(R"pb(
                protocol_version: 2
                actions {
                  parse {
                    type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                    id: 0
                    binary { data: "wire" }
                  }
                }
                actions {
                  parse {
                    type: "protobuf_test_messages.proto3.TestAllTypesProto3"
                    id: 1
                    binary { data: "more" }
                  }
                }
                actions { merge { from: 1 to: 0 } }
                actions {
                  serialize {
                    id: 0
                    binary {}
                  }
                }
              )pb"));
}

TEST(TesteeDeathTest, ProtocolVersionMustBeKnown) {
  MockTestRunner mock;
  EXPECT_DEATH(Testee(&mock, 0), "Unsupported conformance protocol version 0");
  EXPECT_DEATH(Testee(&mock, kLatestProtocolVersion + 1),
               "Unsupported conformance protocol version");
}

TEST(TesteeV2Test, DefaultsToVersion1) {
  MockTestRunner mock;
  EXPECT_EQ(Testee(&mock).protocol_version(), 1);
  EXPECT_EQ(Testee(&mock, 2).protocol_version(), 2);
}

// A version 2 testee gets every test as an action list, even one the flat
// request could carry; the version 1 fields stay unset.
TEST(TesteeV2Test, V2TesteeGetsActionLists) {
  MockTestRunner mock;
  Testee testee(&mock, 2);
  EXPECT_CALL(
      mock,
      RunTest("Required.TesteeTest.foo.ProtobufInput.TextFormatOutput",
              RequestEquals(R"pb(
                protocol_version: 2
                actions {
                  parse {
                    type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                    id: 0
                    binary { data: "wire" }
                  }
                }
                actions {
                  serialize {
                    id: 0
                    text { print_unknown_fields: true }
                  }
                }
              )pb")))
      .WillOnce(RespondWith(R"pb(results { serialized { text: "1: 2" } }
                                 protocol_version: 2)pb"));

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .SerializeText({/*print_unknown_fields=*/true}));

  EXPECT_EQ(result.required_protocol_version(), 2);
  EXPECT_EQ(result.input_format(), ::conformance::PROTOBUF);
  EXPECT_EQ(result.format(), ::conformance::TEXT_FORMAT);
  EXPECT_TRUE(result.print_unknown_fields());
  EXPECT_THAT(result, IsSingleOutput(::conformance::TEXT_FORMAT, "1: 2"));
  EXPECT_EQ(result.failed_action(), absl::nullopt);
}

// The Merge*() and DiscardUnknownFields() shortcuts record the actions the
// flat request implies, with the merge payload as handle 1.
TEST(TesteeV2Test, MergeShortcutsAsActions) {
  MockTestRunner mock;
  Testee testee(&mock, 2);
  // Kept out of the macro arguments: MSVC's traditional preprocessor mis-scans
  // raw string literals containing \" inside macro arguments.
  constexpr absl::string_view kExpectedRequest = R"pb(
    protocol_version: 2
    actions {
      parse {
        type: "protobuf_test_messages.proto2.TestAllTypesProto2"
        id: 0
        json { data: "{}" ignore_unknown_fields: true }
      }
    }
    actions {
      parse {
        type: "protobuf_test_messages.proto2.TestAllTypesProto2"
        id: 1
        json { data: "{\"a\":1}" ignore_unknown_fields: true }
      }
    }
    actions { merge { from: 1 to: 0 } }
    actions { discard_unknown_fields { id: 0 } }
    actions {
      serialize {
        id: 0
        json {}
      }
    }
  )pb";
  EXPECT_CALL(mock, RunTest("Required.TesteeTest.foo.JsonInput.JsonOutput",
                            RequestEquals(kExpectedRequest)))
      .WillOnce(RespondWith(R"pb(results { serialized { json: "{}" } })pb"));

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseJson(TestAllTypesProto2::descriptor(), "{}",
                             {/*ignore_unknown_fields=*/true})
                  .MergeJson("{\"a\":1}")
                  .DiscardUnknownFields()
                  .SerializeJson());

  EXPECT_THAT(result, IsSingleOutput(::conformance::JSON, "{}"));
}

// The test categories of the flat request don't exist in version 2, so an
// override has nothing to relabel.
TEST(TesteeV2Test, OverrideTestCategoryIsANoOpOnV2) {
  MockTestRunner mock;
  Testee testee(&mock, 2);
  EXPECT_CALL(
      mock,
      RunTest(_, RequestEquals(R"pb(
                protocol_version: 2
                actions {
                  parse {
                    type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                    id: 0
                    binary { data: "wire" }
                  }
                }
                actions {
                  serialize {
                    id: 0
                    text {}
                  }
                }
              )pb")))
      .WillOnce(RespondWith(R"pb(results { serialized { text: "" } })pb"));

  Checked(testee.CreateTest(Foo(), TestPriority::kP0)
              .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
              .OverrideTestCategory(::conformance::TEXT_FORMAT_TEST)
              .SerializeText());
}

TEST(TesteeV2Test, NewSerializesAnEmptyMessage) {
  MockTestRunner mock;
  Testee testee(&mock, 2);
  EXPECT_CALL(
      mock,
      RunTest("Recommended.TesteeTest.foo.EmptyInput.ProtobufOutput",
              RequestEquals(R"pb(
                protocol_version: 2
                actions {
                  new_message {
                    type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                    id: 0
                  }
                }
                actions {
                  serialize {
                    id: 0
                    binary {}
                  }
                }
              )pb")))
      .WillOnce(RespondWith(R"pb(results { serialized { binary: "" } })pb"));

  TestResult result = Checked(testee.CreateTest(Foo(), TestPriority::kP3)
                                  .New(TestAllTypesProto2::descriptor())
                                  .SerializeBinary());

  EXPECT_EQ(result.name(),
            "Recommended.TesteeTest.foo.EmptyInput.ProtobufOutput");
  EXPECT_EQ(result.required_protocol_version(), 2);
  EXPECT_EQ(result.input_format(), ::conformance::UNSPECIFIED);
  EXPECT_EQ(result.format(), ::conformance::PROTOBUF);
  EXPECT_THAT(result, IsSingleOutput(::conformance::PROTOBUF, ""));
}

TEST(TesteeV2Test, ParseOnlyOnNewTakesTheGivenOutputFormat) {
  MockTestRunner mock;
  Testee testee(&mock, 2);
  EXPECT_CALL(
      mock,
      RunTest("Required.TesteeTest.foo.EmptyInput", RequestEquals(R"pb(
                protocol_version: 2
                actions {
                  new_message {
                    type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                    id: 0
                  }
                }
                actions {
                  serialize {
                    id: 0
                    json {}
                  }
                }
              )pb")))
      .WillOnce(RespondWith(R"pb(results { serialized { json: "{}" } })pb"));

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .New(TestAllTypesProto2::descriptor())
                  .ParseOnly({/*output_format=*/::conformance::JSON}));

  EXPECT_EQ(result.name(), "Required.TesteeTest.foo.EmptyInput");
  EXPECT_EQ(result.format(), ::conformance::JSON);
}

TEST(TesteeDeathTest, ParseOnlyOnNewNeedsAnOutputFormat) {
  MockTestRunner mock;
  Testee testee(&mock, 2);
  EXPECT_DEATH(Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                           .New(TestAllTypesProto2::descriptor())
                           .ParseOnly()),
               "needs an explicit ParseOnlyOptions::output_format");
}

// MergeFrom() splices the other message's actions in ahead of the merge, its
// handles renumbered after this message's.  The name follows the first
// payload parsed, which is the merged message's when this one is New().
TEST(TesteeV2Test, MergeFromSplicesTheOtherMessage) {
  MockTestRunner mock;
  Testee testee(&mock, 2);
  EXPECT_CALL(
      mock,
      RunTest("Required.TesteeTest.foo.ProtobufInput.ProtobufOutput",
              RequestEquals(R"pb(
                protocol_version: 2
                actions {
                  new_message {
                    type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                    id: 0
                  }
                }
                actions {
                  parse {
                    type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                    id: 1
                    binary { data: "wire" }
                  }
                }
                actions { discard_unknown_fields { id: 1 } }
                actions { merge { from: 1 to: 0 } }
                actions {
                  serialize {
                    id: 0
                    binary {}
                  }
                }
              )pb")))
      .WillOnce(RespondWith(R"pb(results { serialized { binary: "" } })pb"));

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .New(TestAllTypesProto2::descriptor())
                  .MergeFrom(testee.CreateTest(Foo(), TestPriority::kP0)
                                 .ParseBinary(TestAllTypesProto2::descriptor(),
                                              Wire("wire"))
                                 .DiscardUnknownFields())
                  .SerializeBinary());

  EXPECT_EQ(result.input_format(), ::conformance::PROTOBUF);
  EXPECT_THAT(result, IsSingleOutput(::conformance::PROTOBUF, ""));
}

// Several merges, each with its own merged messages, keep their handles
// apart.
TEST(TesteeV2Test, MergeFromRenumbersNestedHandles) {
  MockTestRunner mock;
  Testee testee(&mock, 2);
  EXPECT_CALL(
      mock,
      RunTest("Required.TesteeTest.foo.ProtobufInput.ProtobufOutput",
              RequestEquals(R"pb(
                protocol_version: 2
                actions {
                  parse {
                    type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                    id: 0
                    binary { data: "a" }
                  }
                }
                actions {
                  parse {
                    type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                    id: 1
                    text { data: "b" }
                  }
                }
                actions { merge { from: 1 to: 0 } }
                actions {
                  parse {
                    type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                    id: 2
                    json { data: "c" }
                  }
                }
                actions {
                  parse {
                    type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                    id: 3
                    binary { data: "d" }
                  }
                }
                actions { merge { from: 3 to: 2 } }
                actions { merge { from: 2 to: 0 } }
                actions { discard_unknown_fields { id: 0 } }
                actions {
                  new_message {
                    type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                    id: 4
                  }
                }
                actions { merge { from: 4 to: 0 } }
                actions {
                  serialize {
                    id: 0
                    binary {}
                  }
                }
              )pb")))
      .WillOnce(RespondWith(R"pb(results { serialized { binary: "" } })pb"));

  TestResult result = Checked(
      testee.CreateTest(Foo(), TestPriority::kP0)
          .ParseBinary(TestAllTypesProto2::descriptor(), Wire("a"))
          .MergeText("b")
          .MergeFrom(testee.CreateTest(Foo(), TestPriority::kP0)
                         .ParseJson(TestAllTypesProto2::descriptor(), "c")
                         .MergeBinary(Wire("d")))
          .DiscardUnknownFields()
          .MergeFrom(testee.CreateTest(Foo(), TestPriority::kP0)
                         .New(TestAllTypesProto2::descriptor()))
          .SerializeBinary());

  EXPECT_EQ(result.required_protocol_version(), 2);
}

TEST(TesteeDeathTest, MergeFromNeedsTheSameTest) {
  MockTestRunner mock;
  Testee testee(&mock, 2);
  // Without the DCHECK (opt builds) the request goes through.
  EXPECT_CALL(mock, RunTest)
      .Times(AnyNumber())
      .WillRepeatedly(RespondWith(R"pb(parse_error: "error")pb"));
  EXPECT_DEBUG_DEATH(
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("a"))
                  .MergeFrom(testee
                                 .CreateTest(TestName{/*suite=*/"TesteeTest",
                                                      /*test=*/"bar"},
                                             TestPriority::kP0)
                                 .ParseBinary(TestAllTypesProto2::descriptor(),
                                              Wire("b")))
                  .SerializeBinary()),
      "merges a message of the same test");
}

TEST(TesteeDeathTest, MergeFromRejectsExtraOutputsOnTheSource) {
  MockTestRunner mock;
  Testee testee(&mock, 2);
  // Without the DCHECK (opt builds) the request goes through.
  EXPECT_CALL(mock, RunTest)
      .Times(AnyNumber())
      .WillRepeatedly(RespondWith(R"pb(parse_error: "error")pb"));
  EXPECT_DEBUG_DEATH(
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("a"))
                  .MergeFrom(testee.CreateTest(Foo(), TestPriority::kP0)
                                 .ParseBinary(TestAllTypesProto2::descriptor(),
                                              Wire("b"))
                                 .AlsoJson())
                  .SerializeBinary()),
      "Also\\*\\(\\) applies to the message a test serializes");
}

// The flat request's single category is the serialized message's; an
// override on the merged message would be dropped, so it is rejected.
TEST(TesteeDeathTest, MergeFromRejectsACategoryOverrideOnTheSource) {
  MockTestRunner mock;
  Testee testee(&mock);
  // Without the DCHECK (opt builds) the request goes through.
  EXPECT_CALL(mock, RunTest)
      .Times(AnyNumber())
      .WillRepeatedly(RespondWith(R"pb(parse_error: "error")pb"));
  EXPECT_DEBUG_DEATH(
      Checked(
          testee.CreateTest(Foo(), TestPriority::kP0)
              .ParseBinary(TestAllTypesProto2::descriptor(), Wire("a"))
              .MergeFrom(
                  testee.CreateTest(Foo(), TestPriority::kP0)
                      .ParseBinary(TestAllTypesProto2::descriptor(), Wire("b"))
                      .OverrideTestCategory(::conformance::TEXT_FORMAT_TEST))
              .SerializeBinary()),
      "OverrideTestCategory\\(\\) applies to the message a test serializes");
}

// The terminal output comes first, then the extras in the order queued; the
// name and format() only know about the terminal one.
TEST(TesteeV2Test, AlsoQueuesExtraOutputs) {
  MockTestRunner mock;
  Testee testee(&mock, 2);
  EXPECT_CALL(
      mock,
      RunTest("Required.TesteeTest.foo.ProtobufInput.ProtobufOutput",
              RequestEquals(R"pb(
                protocol_version: 2
                actions {
                  parse {
                    type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                    id: 0
                    binary { data: "wire" }
                  }
                }
                actions {
                  serialize {
                    id: 0
                    binary {}
                  }
                }
                actions {
                  serialize {
                    id: 0
                    json {}
                  }
                }
                actions {
                  serialize {
                    id: 0
                    text { print_unknown_fields: true }
                  }
                }
              )pb")))
      .WillOnce(RespondWith(R"pb(results {
                                   serialized { binary: "\010\t" }
                                   serialized { json: "{}" }
                                   serialized { text: "1: 9" }
                                 })pb"));

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .AlsoJson()
                  .AlsoText({/*print_unknown_fields=*/true})
                  .SerializeBinary());

  EXPECT_EQ(result.format(), ::conformance::PROTOBUF);
  EXPECT_FALSE(result.print_unknown_fields());
  EXPECT_EQ(result.outcome(), Outcome::kOutput);
  ASSERT_THAT(result.outputs(), SizeIs(3));
  EXPECT_EQ(result.output(), &result.outputs()[0]);
  EXPECT_THAT(
      result.outputs(),
      ElementsAre(
          Field(&TestResult::Output::format, ::conformance::PROTOBUF),
          Field(&TestResult::Output::format, ::conformance::JSON),
          Field(&TestResult::Output::format, ::conformance::TEXT_FORMAT)));
  EXPECT_THAT(result.outputs(),
              ElementsAre(Field(&TestResult::Output::payload, "\010\t"),
                          Field(&TestResult::Output::payload, "{}"),
                          Field(&TestResult::Output::payload, "1: 9")));
}

// A test that needs version 2 is not sent to a version 1 testee; the runner
// synthesizes the result and still takes the name.
TEST(TesteeV2Test, UnsupportedTestIsNotSentToAV1Testee) {
  StrictMock<MockTestRunner> mock;
  Testee testee(&mock);
  EXPECT_CALL(mock, RunTest).Times(0);

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .New(TestAllTypesProto2::descriptor())
                  .ParseOnly({/*output_format=*/::conformance::PROTOBUF}));

  EXPECT_EQ(result.name(), "Required.TesteeTest.foo.EmptyInput");
  EXPECT_EQ(result.priority(), TestPriority::kP0);
  EXPECT_EQ(result.type(), TestAllTypesProto2::descriptor());
  EXPECT_THAT(result,
              IsNonOutput(Outcome::kUnsupportedProtocol,
                          "requires conformance protocol v2 (the testee "
                          "speaks v1)"));
  EXPECT_EQ(result.required_protocol_version(), 2);
  EXPECT_EQ(result.input_format(), ::conformance::UNSPECIFIED);
  EXPECT_EQ(result.format(), ::conformance::PROTOBUF);
  EXPECT_EQ(result.failed_action(), absl::nullopt);
  // The request that would have been sent.
  EXPECT_THAT(result.request(), EqualsProto(R"pb(
                protocol_version: 2
                actions {
                  new_message {
                    type: "protobuf_test_messages.proto2.TestAllTypesProto2"
                    id: 0
                  }
                }
                actions {
                  serialize {
                    id: 0
                    binary {}
                  }
                }
              )pb"));
  EXPECT_THAT(result.response(), EqualsProto(""));
}

TEST(TesteeV2Test, AlsoIsUnsupportedOnAV1Testee) {
  StrictMock<MockTestRunner> mock;
  Testee testee(&mock);
  EXPECT_CALL(mock, RunTest).Times(0);

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP3)
                  .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
                  .AlsoJson()
                  .SerializeBinary());

  EXPECT_EQ(result.name(),
            "Recommended.TesteeTest.foo.ProtobufInput.ProtobufOutput");
  EXPECT_EQ(result.outcome(), Outcome::kUnsupportedProtocol);
  EXPECT_EQ(result.input_format(), ::conformance::PROTOBUF);
}

TEST(TesteeV2Test, UnsupportedTestStillTakesItsName) {
  StrictMock<MockTestRunner> mock;
  Testee testee(&mock);
  EXPECT_CALL(mock, RunTest).Times(0);
  Checked(testee.CreateTest(Foo(), TestPriority::kP0)
              .New(TestAllTypesProto2::descriptor())
              .SerializeBinary());
  EXPECT_DEATH(Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                           .New(TestAllTypesProto2::descriptor())
                           .SerializeBinary()),
               "Duplicated test name: "
               "Required.TesteeTest.foo.EmptyInput.ProtobufOutput");
}

TEST(TestResultTest, ForTestingUnsupportedProtocol) {
  constexpr absl::string_view kRequest = R"pb(
    protocol_version: 2
    actions { new_message { type: "T" id: 0 } }
    actions {
      serialize {
        id: 0
        text {}
      }
    }
  )pb";
  ::conformance::ConformanceRequest request;
  ABSL_CHECK(TextFormat::ParseFromString(kRequest, &request));
  TestResult result = Checked(TestResult::ForTestingUnsupportedProtocol(
      "some.name", TestPriority::kP3, TestAllTypesProto2::descriptor(),
      std::move(request),
      /*testee_protocol_version=*/1));
  EXPECT_THAT(result,
              IsNonOutput(Outcome::kUnsupportedProtocol,
                          "requires conformance protocol v2 (the testee "
                          "speaks v1)"));
  EXPECT_EQ(result.required_protocol_version(), 2);
  EXPECT_EQ(result.format(), ::conformance::TEXT_FORMAT);
  EXPECT_THAT(result.request(), EqualsProto(kRequest));
}

TEST(PrintTestResultTest, UnsupportedProtocolSaysWhyItWasNotSent) {
  MockTestRunner mock;
  Testee testee(&mock);
  TestResult result = Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                                  .New(TestAllTypesProto2::descriptor())
                                  .SerializeBinary());
  // (Handle 0 is the default value of `id` and isn't printed.)
  EXPECT_EQ(
      PrintToString(result),
      R"(Required test "Required.TesteeTest.foo.EmptyInput.ProtobufOutput" )"
      R"(with request {protocol_version: 2 )"
      R"(actions { new_message { type: "protobuf_test_messages.proto2.TestAllTypesProto2" } } )"
      R"(actions { serialize { binary { } } }} and response {} )"
      R"((not sent: requires conformance protocol v2 (the testee speaks v1)))");
}

TEST(PrintTestResultTest, V2PayloadsAreTruncated) {
  std::string large_payload(300, 'a');
  std::string truncated_payload =
      absl::StrCat(std::string(200, 'a'), "...(truncated)");
  MockTestRunner mock;
  Testee testee(&mock, 2);
  EXPECT_CALL(mock, RunTest)
      .WillOnce(RespondWith(absl::StrCat("results { serialized { text: \"",
                                         large_payload, "\" } }")));

  TestResult result =
      Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                  .ParseText(TestAllTypesProto2::descriptor(), large_payload)
                  .SerializeText());

  EXPECT_THAT(
      PrintToString(result),
      HasSubstr(absl::StrCat("text { data: \"", truncated_payload, "\" }")));
  EXPECT_THAT(PrintToString(result),
              HasSubstr(absl::StrCat("serialized { text: \"", truncated_payload,
                                     "\" }")));
}

// ---------------------------------------------------------------------------
// The protocol-neutral view of a v2 request and response
// ---------------------------------------------------------------------------

constexpr absl::string_view kV2Request = R"pb(
  protocol_version: 2
  actions {
    parse {
      type: "T"
      id: 0
      json { data: "{}" }
    }
  }
  actions {
    serialize {
      id: 0
      binary {}
    }
  }
  actions {
    serialize {
      id: 0
      text { print_unknown_fields: true }
    }
  }
)pb";

TEST(TestResultViewTest, V2Request) {
  TestResult result = ViewOf(kV2Request, "");
  EXPECT_EQ(result.required_protocol_version(), 2);
  EXPECT_EQ(result.input_format(), ::conformance::JSON);
  EXPECT_EQ(result.format(), ::conformance::PROTOBUF);
  // Only the terminal (first) SerializeAction counts.
  EXPECT_FALSE(result.print_unknown_fields());
  EXPECT_THAT(result, IsNonOutput(Outcome::kNoResult, ""));
}

// An explicit protocol_version of 1 (or a nonsensical value below it) is the
// flat request, whatever `actions` says: the same rule as the C++ harness's.
TEST(TestResultViewTest, VersionAtOrBelow1IsTheFlatRequest) {
  for (int version : {0, 1, -1}) {
    SCOPED_TRACE(version);
    TestResult result = ViewOf(absl::StrCat("protocol_version: ", version, R"pb(
                                 json_payload: "{}"
                                 requested_output_format: TEXT_FORMAT
                                 print_unknown_fields: true
                                 actions {
                                   parse {
                                     type: "T"
                                     id: 0
                                     binary { data: "" }
                                   }
                                 }
                                 actions {
                                   serialize {
                                     id: 0
                                     binary {}
                                   }
                                 }
                               )pb"),
                               R"pb(text_payload: "1: 2")pb");
    EXPECT_EQ(result.required_protocol_version(), 1);
    EXPECT_EQ(result.input_format(), ::conformance::JSON);
    EXPECT_EQ(result.format(), ::conformance::TEXT_FORMAT);
    EXPECT_TRUE(result.print_unknown_fields());
    EXPECT_THAT(result, IsSingleOutput(::conformance::TEXT_FORMAT, "1: 2"));
  }
}

TEST(TestResultViewTest, V2Results) {
  TestResult result = ViewOf(kV2Request, R"pb(
    results {
      serialized { binary: "bin" }
      serialized { text: "txt" }
    }
  )pb");
  EXPECT_EQ(result.outcome(), Outcome::kOutput);
  EXPECT_THAT(result.message(), IsEmpty());
  ASSERT_THAT(result.outputs(), SizeIs(2));
  EXPECT_EQ(result.outputs()[0].format, ::conformance::PROTOBUF);
  EXPECT_EQ(result.outputs()[0].payload, "bin");
  EXPECT_EQ(result.outputs()[1].format, ::conformance::TEXT_FORMAT);
  EXPECT_EQ(result.outputs()[1].payload, "txt");
  EXPECT_EQ(result.output(), &result.outputs()[0]);
}

// The format of an output is the testee's, not the request's, so that a
// wrong-format answer is reported as such.
TEST(TestResultViewTest, V2ResultKeepsTheTesteesFormat) {
  TestResult result = ViewOf(kV2Request, R"pb(
    results {
      serialized { json: "{}" }
      serialized { text: "txt" }
    }
  )pb");
  ASSERT_THAT(result.outputs(), SizeIs(2));
  EXPECT_EQ(result.outputs()[0].format, ::conformance::JSON);
  EXPECT_EQ(result.outputs()[0].payload, "{}");
}

TEST(TestResultViewTest, V2ResultWithoutPayloadHasNoFormat) {
  TestResult result = ViewOf(kV2Request, R"pb(
    results {
      serialized {}
      serialized { text: "" }
    }
  )pb");
  ASSERT_THAT(result.outputs(), SizeIs(2));
  EXPECT_EQ(result.outputs()[0].format, ::conformance::UNSPECIFIED);
  EXPECT_THAT(result.outputs()[0].payload, IsEmpty());
}

TEST(TestResultViewTest, V2ResultCountMismatchIsARuntimeError) {
  EXPECT_THAT(
      ViewOf(kV2Request, R"pb(results { serialized { binary: "" } })pb"),
      IsNonOutput(Outcome::kRuntimeError,
                  "testee returned 1 outputs for 2 serialize actions"));
  EXPECT_THAT(ViewOf(kV2Request, R"pb(results {})pb"),
              IsNonOutput(Outcome::kRuntimeError,
                          "testee returned 0 outputs for 2 serialize actions"));
  EXPECT_THAT(ViewOf(kV2Request, R"pb(results {
                                        serialized { binary: "" }
                                        serialized { text: "" }
                                        serialized { json: "" }
                                      })pb"),
              IsNonOutput(Outcome::kRuntimeError,
                          "testee returned 3 outputs for 2 serialize actions"));
}

TEST(TestResultViewTest, V2ErrorsCarryTheFailedAction) {
  TestResult result = ViewOf(kV2Request, R"pb(parse_error: "bad"
                                              failed_action: 0)pb");
  EXPECT_THAT(result, IsNonOutput(Outcome::kParseError, "bad"));
  EXPECT_THAT(result.failed_action(), Optional(0));

  result = ViewOf(kV2Request, R"pb(serialize_error: "bad" failed_action: 2)pb");
  EXPECT_THAT(result, IsNonOutput(Outcome::kSerializeError, "bad"));
  EXPECT_THAT(result.failed_action(), Optional(2));

  // Not about one action.
  result = ViewOf(kV2Request,
                  R"pb(runtime_error: "unsupported protocol version 2")pb");
  EXPECT_EQ(result.failed_action(), absl::nullopt);
  // A v1 response never has one.
  EXPECT_EQ(
      ViewOf(R"pb(protobuf_payload: "wire")pb", R"pb(parse_error: "bad")pb")
          .failed_action(),
      absl::nullopt);
}

TEST(TestResultViewTest, V2InputFormatIsTheFirstParseActions) {
  EXPECT_EQ(ViewOf(R"pb(protocol_version: 2
                        actions { new_message { type: "T" id: 0 } }
                        actions {
                          parse {
                            type: "T"
                            id: 1
                            text { data: "" }
                          }
                        }
                        actions { merge { from: 1 to: 0 } }
                        actions {
                          serialize {
                            id: 0
                            binary {}
                          }
                        })pb",
                   "")
                .input_format(),
            ::conformance::TEXT_FORMAT);
  EXPECT_EQ(ViewOf(R"pb(protocol_version: 2
                        actions { new_message { type: "T" id: 0 } }
                        actions {
                          serialize {
                            id: 0
                            binary {}
                          }
                        })pb",
                   "")
                .input_format(),
            ::conformance::UNSPECIFIED);
}

TEST(TestResultViewTest, UnsupportedProtocolSurvivesMoves) {
  MockTestRunner mock;
  Testee testee(&mock);
  TestResult original = Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                                    .New(TestAllTypesProto2::descriptor())
                                    .SerializeBinary());
  TestResult moved(std::move(original));
  EXPECT_EQ(moved.outcome(), Outcome::kUnsupportedProtocol);
  EXPECT_EQ(moved.required_protocol_version(), 2);
  EXPECT_EQ(moved.message(),
            "requires conformance protocol v2 (the testee speaks v1)");
}

// ---------------------------------------------------------------------------
// The discovery handshake
// ---------------------------------------------------------------------------

using ::testing::Eq;
using ::testing::Not;

// The probe request, whose version 1 shape is frozen forever: it is all a
// version 1 testee ever sees of the handshake.  It is sent under kProbeName
// (test_runner.h).
constexpr absl::string_view kProbeRequest = R"pb(
  protobuf_payload: ""
  requested_output_format: PROTOBUF
  message_type: "protobuf_test_messages.proto3.TestAllTypesProto3"
  test_category: BINARY_TEST
  protocol_version: 2
  actions {
    parse {
      type: "protobuf_test_messages.proto3.TestAllTypesProto3"
      id: 0
      binary { data: "" }
    }
  }
  actions {
    serialize {
      id: 0
      binary {}
    }
  }
)pb";

TEST(TesteeProbeTest, SendsTheFrozenProbe) {
  StrictMock<MockTestRunner> mock;
  Testee testee(&mock);
  EXPECT_CALL(mock, RunTest(kProbeName, RequestEquals(kProbeRequest)))
      .WillOnce(RespondWith(R"pb(protobuf_payload: "")pb"));

  EXPECT_EQ(testee.DetectProtocolVersion(), absl::OkStatus());
  EXPECT_EQ(testee.protocol_version(), 1);
}

// The probe's version 1 fields are exactly the flat request the builder
// lowers Test().ParseBinary(TestAllTypesProto3, "").SerializeBinary() to, so
// that a version 1 testee sees a request it already answers; this pins the
// two to each other, whatever LowerToV1() sets.
TEST(TesteeProbeTest, ProbeIsTheLoweredEmptyRoundTrip) {
  ::conformance::ConformanceRequest probe;
  ::conformance::ConformanceRequest lowered;
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(mock, RunTest(kProbeName, _))
      .WillOnce([&](absl::string_view, absl::string_view input) {
        ABSL_CHECK(probe.ParseFromString(input));
        return SerializedResponse(R"pb(protobuf_payload: "")pb");
      });
  EXPECT_CALL(mock, RunTest(Not(kProbeName), _))
      .WillOnce([&](absl::string_view, absl::string_view input) {
        ABSL_CHECK(lowered.ParseFromString(input));
        return SerializedResponse(R"pb(protobuf_payload: "")pb");
      });

  ASSERT_EQ(testee.DetectProtocolVersion(), absl::OkStatus());
  ASSERT_EQ(testee.protocol_version(), 1);
  Checked(testee.CreateTest(Foo(), TestPriority::kP0)
              .ParseBinary(TestAllTypesProto3::descriptor(), Wire(""))
              .SerializeBinary());

  // The version 2 fields are the probe's own; the rest must match.
  EXPECT_EQ(probe.protocol_version(), 2);
  EXPECT_EQ(probe.actions_size(), 2);
  probe.clear_protocol_version();
  probe.clear_actions();
  std::string probe_textproto;
  ASSERT_TRUE(TextFormat::PrintToString(probe, &probe_textproto));
  EXPECT_THAT(lowered, EqualsProto(probe_textproto));
  EXPECT_THAT(lowered, EqualsProto(R"pb(
                protobuf_payload: ""
                requested_output_format: PROTOBUF
                message_type: "protobuf_test_messages.proto3.TestAllTypesProto3"
                test_category: BINARY_TEST
              )pb"));
}

// Any answer without the field is a version 1 testee's, whatever it says.
TEST(TesteeProbeTest, NoFieldMeansVersion1) {
  for (absl::string_view response :
       {"", R"pb(protobuf_payload: "")pb", R"pb(skipped: "no")pb",
        R"pb(parse_error: "bad")pb", R"pb(runtime_error: "unknown request")pb",
        R"pb(results { serialized { binary: "" } })pb"}) {
    SCOPED_TRACE(response);
    MockTestRunner mock;
    Testee testee(&mock, 2);
    EXPECT_CALL(mock, RunTest).WillOnce(RespondWith(response));
    EXPECT_EQ(testee.DetectProtocolVersion(), absl::OkStatus());
    EXPECT_EQ(testee.protocol_version(), 1);
  }
}

// Any answer with the field is that version's, whatever else it says.
TEST(TesteeProbeTest, FieldMeansThatVersion) {
  for (absl::string_view response : {R"pb(results { serialized { binary: "" } }
                                          protocol_version: 2)pb",
                                     R"pb(skipped: "no" protocol_version: 2)pb",
                                     R"pb(protobuf_payload: ""
                                          protocol_version: 2)pb",
                                     R"pb(protocol_version: 2)pb"}) {
    SCOPED_TRACE(response);
    MockTestRunner mock;
    Testee testee(&mock);
    EXPECT_CALL(mock, RunTest).WillOnce(RespondWith(response));
    EXPECT_EQ(testee.DetectProtocolVersion(), absl::OkStatus());
    EXPECT_EQ(testee.protocol_version(), 2);
  }
}

TEST(TesteeProbeTest, NewerVersionsAreClampedWithAWarning) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(mock, RunTest)
      .WillOnce(RespondWith(R"pb(results { serialized { binary: "" } }
                                 protocol_version: 7)pb"));
  absl::ScopedMockLog log;
  EXPECT_CALL(log, Log).Times(AnyNumber());
  EXPECT_CALL(log, Log(absl::LogSeverity::kWarning, _,
                       HasSubstr("implements conformance protocol v7, newer "
                                 "than this runner's v2")))
      .Times(1);
  log.StartCapturingLogs();

  EXPECT_EQ(testee.DetectProtocolVersion(), absl::OkStatus());
  EXPECT_EQ(testee.protocol_version(), kLatestProtocolVersion);
}

TEST(TesteeProbeTest, LogsTheDetectedVersion) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(mock, RunTest)
      .WillOnce(RespondWith(R"pb(protocol_version: 2)pb"));
  absl::ScopedMockLog log;
  EXPECT_CALL(log, Log).Times(AnyNumber());
  EXPECT_CALL(log, Log(absl::LogSeverity::kInfo, _,
                       Eq("The testee answered the discovery handshake as "
                          "conformance protocol v2")))
      .Times(1);
  log.StartCapturingLogs();

  EXPECT_EQ(testee.DetectProtocolVersion(), absl::OkStatus());
}

TEST(TesteeProbeTest, PinSatisfied) {
  {
    // Pinned below what the testee speaks.
    MockTestRunner mock;
    Testee testee(&mock);
    EXPECT_CALL(mock, RunTest)
        .WillOnce(RespondWith(R"pb(protocol_version: 2)pb"));
    EXPECT_EQ(testee.DetectProtocolVersion(/*pinned_protocol_version=*/1),
              absl::OkStatus());
    EXPECT_EQ(testee.protocol_version(), 1);
  }
  {
    // Pinned to exactly what the testee speaks.
    MockTestRunner mock;
    Testee testee(&mock);
    EXPECT_CALL(mock, RunTest)
        .WillOnce(RespondWith(R"pb(protocol_version: 2)pb"));
    EXPECT_EQ(testee.DetectProtocolVersion(/*pinned_protocol_version=*/2),
              absl::OkStatus());
    EXPECT_EQ(testee.protocol_version(), 2);
  }
  {
    // A version 1 testee pinned to 1.
    MockTestRunner mock;
    Testee testee(&mock, 2);
    EXPECT_CALL(mock, RunTest)
        .WillOnce(RespondWith(R"pb(protobuf_payload: "")pb"));
    EXPECT_EQ(testee.DetectProtocolVersion(/*pinned_protocol_version=*/1),
              absl::OkStatus());
    EXPECT_EQ(testee.protocol_version(), 1);
  }
}

TEST(TesteeProbeTest, PinViolated) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(mock, RunTest)
      .WillOnce(RespondWith(R"pb(protobuf_payload: "")pb"));

  absl::Status status =
      testee.DetectProtocolVersion(/*pinned_protocol_version=*/2);
  EXPECT_EQ(status.code(), absl::StatusCode::kFailedPrecondition);
  EXPECT_EQ(status.message(),
            "--protocol_version=2 pins the testee to protocol v2, but it "
            "answered the probe as v1");
  // Unchanged.
  EXPECT_EQ(testee.protocol_version(), 1);
}

TEST(TesteeDeathTest, PinMustBeKnown) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_DEATH(
      testee.DetectProtocolVersion(kLatestProtocolVersion + 1).IgnoreError(),
      "Unsupported conformance protocol version");
  EXPECT_DEATH(testee.DetectProtocolVersion(-1).IgnoreError(),
               "Unsupported conformance protocol version");
}

TEST(TesteeProbeTest, TimeoutIsNotAnAnswer) {
  MockTestRunner mock;
  Testee testee(&mock, 2);
  EXPECT_CALL(mock, RunTest)
      .WillOnce(RespondWith(R"pb(timeout_error: "timed out")pb"));

  absl::Status status = testee.DetectProtocolVersion();
  EXPECT_EQ(status.code(), absl::StatusCode::kUnavailable);
  EXPECT_EQ(status.message(),
            "could not probe the testee's protocol version: timed out");
  EXPECT_EQ(testee.protocol_version(), 2);
}

// A filtering runner (FilteringTestRunner under --test) that filtered the
// probe out instead of forwarding it answers with the reserved not-selected
// skip: the testee never heard the probe, so this isn't a version 1 answer
// (which would make a --test run speak version 1 to a version 2 testee) but
// no answer at all.
TEST(TesteeProbeTest, NotSelectedSkipIsNotAnAnswer) {
  MockTestRunner mock;
  Testee testee(&mock, 2);
  ::conformance::ConformanceResponse not_selected;
  not_selected.set_skipped(kTestNotSelectedSkipReason);
  EXPECT_CALL(mock, RunTest).WillOnce(Return(not_selected.SerializeAsString()));

  absl::Status status = testee.DetectProtocolVersion();
  EXPECT_EQ(status.code(), absl::StatusCode::kUnavailable);
  EXPECT_EQ(status.message(),
            "could not probe the testee's protocol version: the test runner "
            "filtered the probe out instead of forwarding it to the testee");
  // Unchanged.
  EXPECT_EQ(testee.protocol_version(), 2);
}

TEST(TesteeProbeTest, GarbageIsNotAnAnswer) {
  MockTestRunner mock;
  Testee testee(&mock);
  // A truncated field tag.
  EXPECT_CALL(mock, RunTest).WillOnce(Return(std::string("\010", 1)));

  absl::Status status = testee.DetectProtocolVersion();
  EXPECT_EQ(status.code(), absl::StatusCode::kUnavailable);
  EXPECT_THAT(status.message(),
              HasSubstr("could not probe the testee's protocol version"));
}

// The version the probe detects is the one the tests then speak.
TEST(TesteeProbeTest, DetectedVersionIsSpoken) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(mock, RunTest(kProbeName, _))
      .WillOnce(RespondWith(R"pb(protocol_version: 2)pb"));
  EXPECT_CALL(mock,
              RunTest("Required.TesteeTest.foo.EmptyInput.ProtobufOutput", _))
      .WillOnce(RespondWith(R"pb(results { serialized { binary: "" } }
                                 protocol_version: 2)pb"));

  ASSERT_EQ(testee.DetectProtocolVersion(), absl::OkStatus());
  TestResult result = Checked(testee.CreateTest(Foo(), TestPriority::kP0)
                                  .New(TestAllTypesProto2::descriptor())
                                  .SerializeBinary());
  EXPECT_THAT(result, IsSingleOutput(::conformance::PROTOBUF, ""));
}

TEST(TesteeDeathTest, ProbeMustPrecedeTheFirstTest) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(mock, RunTest)
      .Times(AnyNumber())
      .WillRepeatedly(RespondWith(R"pb(protobuf_payload: "")pb"));
  Checked(testee.CreateTest(Foo(), TestPriority::kP0)
              .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
              .SerializeBinary());
  EXPECT_DEBUG_DEATH(testee.DetectProtocolVersion().IgnoreError(),
                     "must precede the first test");
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

::conformance::ConformanceRequest BinaryToBinaryRequest() {
  ::conformance::ConformanceRequest request;
  request.set_protobuf_payload("wire");
  request.set_requested_output_format(::conformance::PROTOBUF);
  return request;
}

TEST(ResolveTestInfoTest, ParseOnlyTestWithSyntaxAndFieldTypeParams) {
  TestNumberer numberer;
  ConformanceTestInfo info = ResolveTestInfo(
      TestName{"PrematureEofTest", "BeforeKnownNonRepeatedValue",
               "EditionsProto2_DOUBLE"},
      TestPriority::kP0, ::conformance::PROTOBUF, ::conformance::PROTOBUF,
      NameStyle::kWithoutOutputFormat, numberer);

  EXPECT_EQ(info.legacy_test_name,
            "Required.PrematureEofTest.BeforeKnownNonRepeatedValue/"
            "EditionsProto2_DOUBLE.ProtobufInput");
  EXPECT_EQ(info.domain, "wire");
  EXPECT_EQ(info.section, "varint");
  EXPECT_EQ(info.subsection, "eof");
  EXPECT_EQ(info.section_coordinate, "01.01");
  EXPECT_EQ(info.test_case,
            "eof_before_known_non_repeated_value_double_"
            "parsefails");
  EXPECT_EQ(info.test_number, 1);
  EXPECT_EQ(info.syntax, "ed_proto2");
  EXPECT_EQ(info.payloads, "pb2pb");
  EXPECT_EQ(info.variant, "ed_proto2_pb2pb");
  EXPECT_EQ(info.variant_number, 31);
  EXPECT_EQ(info.coordinate, "01.01.001.31");
  EXPECT_EQ(info.test_name,
            "wire.varint.eof_before_known_non_repeated_value_"
            "double_parsefails.ed_proto2_pb2pb");
  EXPECT_EQ(info.description,
            "Verify wire varint (eof) [EditionsProto2, pb2pb]: "
            "PrematureEofTest.BeforeKnownNonRepeatedValue/"
            "EditionsProto2_DOUBLE (input must be rejected)");
  EXPECT_FALSE(info.is_informational);
  EXPECT_EQ(info.priority, TestPriority::kP0);
}

TEST(ResolveTestInfoTest, VariantsOfOneTestShareTheTestNumber) {
  TestNumberer numberer;
  TestName name{"PrematureEofTest", "BeforeKnownNonRepeatedValue",
                "Proto3_DOUBLE"};
  TestName other{"PrematureEofTest", "BeforeKnownNonRepeatedValue",
                 "Proto3_INT32"};

  ConformanceTestInfo proto3 = ResolveTestInfo(
      name, TestPriority::kP0, ::conformance::PROTOBUF, ::conformance::PROTOBUF,
      NameStyle::kWithoutOutputFormat, numberer);
  ConformanceTestInfo int32 = ResolveTestInfo(
      other, TestPriority::kP0, ::conformance::PROTOBUF,
      ::conformance::PROTOBUF, NameStyle::kWithoutOutputFormat, numberer);
  name.params = "EditionsProto3_DOUBLE";
  ConformanceTestInfo editions = ResolveTestInfo(
      name, TestPriority::kP0, ::conformance::PROTOBUF, ::conformance::PROTOBUF,
      NameStyle::kWithoutOutputFormat, numberer);

  EXPECT_EQ(proto3.coordinate, "01.01.001.21");
  EXPECT_EQ(int32.coordinate, "01.01.002.21");
  EXPECT_EQ(editions.coordinate, "01.01.001.41");
  EXPECT_EQ(proto3.test_case, editions.test_case);
  EXPECT_NE(proto3.test_name, editions.test_name);
}

TEST(ResolveTestInfoTest, ValidDataSuitesAreFiledByFieldType) {
  TestNumberer numberer;
  ConformanceTestInfo double_case = ResolveTestInfo(
      TestName{"ValidDataScalarTest", "Scalar", "Proto3_DOUBLE_2"},
      TestPriority::kP0, ::conformance::PROTOBUF, ::conformance::PROTOBUF,
      NameStyle::kWithOutputFormat, numberer);
  ConformanceTestInfo int32_case = ResolveTestInfo(
      TestName{"ValidDataScalarTest", "Scalar", "Proto3_INT32_0"},
      TestPriority::kP0, ::conformance::PROTOBUF, ::conformance::PROTOBUF,
      NameStyle::kWithOutputFormat, numberer);
  ConformanceTestInfo message_case = ResolveTestInfo(
      TestName{"ValidDataRepeatedNonPackableTest", "Repeated",
               "EditionsProto2_MESSAGE"},
      TestPriority::kP0, ::conformance::PROTOBUF, ::conformance::PROTOBUF,
      NameStyle::kWithOutputFormat, numberer);

  EXPECT_EQ(double_case.section, "fixed");
  EXPECT_EQ(double_case.subsection, "floating_point");
  EXPECT_EQ(double_case.section_coordinate, "01.02");
  EXPECT_EQ(double_case.test_case,
            "floating_point_valid_data_scalar_scalar_double_2");
  EXPECT_EQ(double_case.test_name,
            "wire.fixed.floating_point_valid_data_scalar_scalar_double_2."
            "proto3_pb2pb");

  EXPECT_EQ(int32_case.section, "varint");
  EXPECT_EQ(int32_case.subsection, "integers");
  EXPECT_EQ(int32_case.section_coordinate, "01.01");
  EXPECT_EQ(int32_case.test_case, "integers_valid_data_scalar_scalar_int32_0");

  EXPECT_EQ(message_case.section, "length_delimited");
  EXPECT_EQ(message_case.subsection, "submessages");
  EXPECT_EQ(message_case.test_case,
            "submessages_valid_data_repeated_non_packable_repeated_message");
}

TEST(ResolveTestInfoTest, SpecialFloatSpellingsStayDistinct) {
  TestNumberer numberer;
  ConformanceTestInfo lower = ResolveTestInfo(
      TestName{"TextFloatInfinityTest", "Positive", "Proto3_inf"},
      TestPriority::kP0, ::conformance::TEXT_FORMAT, ::conformance::PROTOBUF,
      NameStyle::kWithOutputFormat, numberer);
  ConformanceTestInfo upper = ResolveTestInfo(
      TestName{"TextFloatInfinityTest", "Positive", "Proto3_INF"},
      TestPriority::kP0, ::conformance::TEXT_FORMAT, ::conformance::PROTOBUF,
      NameStyle::kWithOutputFormat, numberer);
  ConformanceTestInfo mixed = ResolveTestInfo(
      TestName{"TextFloatInfinityTest", "Positive", "Proto3_iNF"},
      TestPriority::kP0, ::conformance::TEXT_FORMAT, ::conformance::PROTOBUF,
      NameStyle::kWithOutputFormat, numberer);

  EXPECT_EQ(lower.test_case, "literals_text_float_infinity_positive_inf_lower");
  EXPECT_EQ(upper.test_case, "literals_text_float_infinity_positive_inf_upper");
  EXPECT_EQ(mixed.test_case,
            "literals_text_float_infinity_positive_inf_mixed_luu");
  EXPECT_EQ(lower.coordinate, "02.01.001.27");
  EXPECT_EQ(upper.coordinate, "02.01.002.27");
  EXPECT_EQ(mixed.coordinate, "02.01.003.27");
}

TEST(ResolveTestInfoTest, SuffixAndOutputFormatAreCarried) {
  TestNumberer numberer;
  ConformanceTestInfo info = ResolveTestInfo(
      TestName{"JsonHelloWorldTest", "HelloWorld", "", "WithSuffix"},
      TestPriority::kP3, ::conformance::JSON, ::conformance::TEXT_FORMAT,
      NameStyle::kWithOutputFormat, numberer);

  EXPECT_EQ(info.legacy_test_name,
            "Recommended.JsonHelloWorldTest.HelloWorld.WithSuffix.JsonInput."
            "TextFormatOutput");
  EXPECT_EQ(info.domain, "json");
  EXPECT_EQ(info.section, "field_names");
  EXPECT_EQ(info.subsection, "hello_world");
  // The suite (json_hello_world) ends in the subsection and is left out.
  EXPECT_EQ(info.test_case, "hello_world_hello_world_with_suffix");
  EXPECT_EQ(info.syntax, "");
  EXPECT_EQ(info.payloads, "json2text");
  EXPECT_EQ(info.variant, "json2text");
  EXPECT_EQ(info.variant_number, 6);
  EXPECT_EQ(info.coordinate, "03.01.001.06");
  EXPECT_EQ(info.description,
            "Verify json field_names (hello_world) [json2text]: "
            "JsonHelloWorldTest.HelloWorld.WithSuffix");
  EXPECT_EQ(info.priority, TestPriority::kP3);
}

TEST(ResolveTestInfoTest, SuiteIsLeftOutOfTheTestCaseWhenRedundant) {
  TestNumberer numberer;
  // The test's name starts with the suite's.
  ConformanceTestInfo starts_with = ResolveTestInfo(
      TestName{"MergeTest", "MergeSubmessage", "Proto2"}, TestPriority::kP0,
      ::conformance::PROTOBUF, ::conformance::PROTOBUF,
      NameStyle::kWithOutputFormat, numberer);
  EXPECT_EQ(starts_with.test_case, "submessages_merge_submessage");
  // Otherwise the suite tells apart same-named tests of different suites.
  ConformanceTestInfo kept = ResolveTestInfo(
      TestName{"TextFloatTest", "Max", "Proto3"}, TestPriority::kP0,
      ::conformance::PROTOBUF, ::conformance::PROTOBUF,
      NameStyle::kWithOutputFormat, numberer);
  EXPECT_EQ(kept.test_case, "literals_text_float_max");
}

TEST(ResolveTestInfoTest, UnmappedSuiteIsFiledUnderDomainZero) {
  TestNumberer numberer;
  ConformanceTestInfo info = ResolveTestInfo(
      Foo(), TestPriority::kP0, ::conformance::PROTOBUF,
      ::conformance::PROTOBUF, NameStyle::kWithOutputFormat, numberer);

  EXPECT_EQ(info.domain, "unmapped");
  EXPECT_EQ(info.section, "testee");
  EXPECT_EQ(info.subsection, "");
  EXPECT_EQ(info.section_coordinate, "00.00");
  EXPECT_EQ(info.test_case, "testee_foo");
  EXPECT_EQ(info.coordinate, "00.00.001.01");
  EXPECT_EQ(info.test_name, "unmapped.testee.testee_foo.pb2pb");
  EXPECT_EQ(info.description, "Verify unmapped testee [pb2pb]: TesteeTest.foo");
}

TEST(ResolveTestInfoTest, UnlistedPerformanceSuitesAreInformational) {
  TestNumberer numberer;
  ConformanceTestInfo info = ResolveTestInfo(
      TestName{"BinaryPerformanceTest", "RepeatedInt32", "Proto3"},
      TestPriority::kP0, ::conformance::PROTOBUF, ::conformance::PROTOBUF,
      NameStyle::kWithOutputFormat, numberer);

  EXPECT_TRUE(info.is_informational);
  EXPECT_EQ(info.domain, "informational");
  EXPECT_EQ(info.section, "performance");
  EXPECT_EQ(info.test_case, "benchmarks_binary_performance_repeated_int32");
}

TEST(ToCaseResultTest, CopiesTheIdentityAndPriority) {
  TestNumberer numberer;
  ConformanceTestInfo info = ResolveTestInfo(
      TestName{"PrematureEofTest", "BeforeKnownNonRepeatedValue",
               "EditionsProto2_DOUBLE"},
      TestPriority::kP3, ::conformance::PROTOBUF, ::conformance::PROTOBUF,
      NameStyle::kWithoutOutputFormat, numberer);

  EXPECT_THAT(
      ToCaseResult(info), EqualsProto(R"pb(
        test_name: "wire.varint.eof_before_known_non_repeated_value_double_parsefails.ed_proto2_pb2pb"
        coordinate: "01.01.001.31"
        section_coordinate: "01.01"
        domain: "wire"
        section: "varint"
        subsection: "eof"
        test_number: 1
        test_case: "eof_before_known_non_repeated_value_double_parsefails"
        variant_number: 31
        variant: "ed_proto2_pb2pb"
        syntax: "ed_proto2"
        payloads: "pb2pb"
        description: "Verify wire varint (eof) [EditionsProto2, pb2pb]: PrematureEofTest.BeforeKnownNonRepeatedValue/EditionsProto2_DOUBLE (input must be rejected)"
        legacy_test_name: "Recommended.PrematureEofTest.BeforeKnownNonRepeatedValue/EditionsProto2_DOUBLE.ProtobufInput"
        priority: "P3"
      )pb"));
}

TEST(TestResultTest, CarriesTheResolvedInfo) {
  MockTestRunner mock;
  Testee testee(&mock);
  EXPECT_CALL(mock, RunTest(_, _)).WillOnce(RespondWith(R"pb(
    protobuf_payload: "wire"
  )pb"));

  TestResult result = Checked(
      testee
          .CreateTest(TestName{"PrematureEofTest",
                               "BeforeKnownNonRepeatedValue", "Proto2_DOUBLE"},
                      TestPriority::kP0)
          .ParseBinary(TestAllTypesProto2::descriptor(), Wire("wire"))
          .ParseOnly());

  EXPECT_EQ(result.name(),
            "Required.PrematureEofTest.BeforeKnownNonRepeatedValue/"
            "Proto2_DOUBLE.ProtobufInput");
  EXPECT_EQ(result.info().legacy_test_name, result.name());
  EXPECT_EQ(result.info().coordinate, "01.01.001.11");
  EXPECT_EQ(result.info().priority, result.priority());
}

TEST(TestResultTest, ForTestingHasPlaceholderInfo) {
  ::conformance::ConformanceRequest request = BinaryToBinaryRequest();
  ::conformance::ConformanceResponse response;

  TestResult result = Checked(TestResult::ForTesting(
      "some.name", TestPriority::kP3, TestAllTypesProto2::descriptor(), request,
      response));

  EXPECT_EQ(result.info().test_name, "some.name");
  EXPECT_EQ(result.info().legacy_test_name, "some.name");
  EXPECT_EQ(result.info().coordinate, "00.00.000.00");
  EXPECT_EQ(result.info().domain, "unmapped");
  EXPECT_EQ(result.info().priority, TestPriority::kP3);
}

TEST(TestResultTest, ForTestingWithExplicitInfo) {
  ConformanceTestInfo info;
  info.test_name = "wire.varint.some_test.pb2pb";
  info.legacy_test_name = "Required.SomeTest.Some.ProtobufInput.ProtobufOutput";
  info.coordinate = "01.01.001.01";
  info.is_informational = true;
  info.priority = TestPriority::kP2;

  TestResult result = Checked(TestResult::ForTesting(
      info, TestAllTypesProto2::descriptor(), BinaryToBinaryRequest(),
      ::conformance::ConformanceResponse()));

  EXPECT_EQ(result.name(),
            "Required.SomeTest.Some.ProtobufInput.ProtobufOutput");
  EXPECT_EQ(result.priority(), TestPriority::kP2);
  EXPECT_EQ(result.info().test_name, "wire.varint.some_test.pb2pb");
  EXPECT_TRUE(result.info().is_informational);
}

}  // namespace
}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
