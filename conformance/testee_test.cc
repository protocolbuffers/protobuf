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

::conformance::ConformanceRequest JsonToTextRequest() {
  ::conformance::ConformanceRequest request;
  request.set_json_payload("{}");
  request.set_requested_output_format(::conformance::TEXT_FORMAT);
  return request;
}

TEST(ResolveTestInfoTest, ParseOnlyTestWithSyntaxAndFieldTypeParams) {
  TestNumberer numberer;
  ConformanceTestInfo info = ResolveTestInfo(
      TestName{"PrematureEofTest", "BeforeKnownNonRepeatedValue",
               "EditionsProto2_DOUBLE"},
      TestPriority::kP0, BinaryToBinaryRequest(),
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

  ConformanceTestInfo proto3 =
      ResolveTestInfo(name, TestPriority::kP0, BinaryToBinaryRequest(),
                      NameStyle::kWithoutOutputFormat, numberer);
  ConformanceTestInfo int32 =
      ResolveTestInfo(other, TestPriority::kP0, BinaryToBinaryRequest(),
                      NameStyle::kWithoutOutputFormat, numberer);
  name.params = "EditionsProto3_DOUBLE";
  ConformanceTestInfo editions =
      ResolveTestInfo(name, TestPriority::kP0, BinaryToBinaryRequest(),
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
      TestPriority::kP0, BinaryToBinaryRequest(), NameStyle::kWithOutputFormat,
      numberer);
  ConformanceTestInfo int32_case = ResolveTestInfo(
      TestName{"ValidDataScalarTest", "Scalar", "Proto3_INT32_0"},
      TestPriority::kP0, BinaryToBinaryRequest(), NameStyle::kWithOutputFormat,
      numberer);
  ConformanceTestInfo message_case =
      ResolveTestInfo(TestName{"ValidDataRepeatedNonPackableTest", "Repeated",
                               "EditionsProto2_MESSAGE"},
                      TestPriority::kP0, BinaryToBinaryRequest(),
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
  ::conformance::ConformanceRequest request;
  request.set_text_payload("optional_float: inf");
  request.set_requested_output_format(::conformance::PROTOBUF);
  TestNumberer numberer;
  ConformanceTestInfo lower = ResolveTestInfo(
      TestName{"TextFloatInfinityTest", "Positive", "Proto3_inf"},
      TestPriority::kP0, request, NameStyle::kWithOutputFormat, numberer);
  ConformanceTestInfo upper = ResolveTestInfo(
      TestName{"TextFloatInfinityTest", "Positive", "Proto3_INF"},
      TestPriority::kP0, request, NameStyle::kWithOutputFormat, numberer);
  ConformanceTestInfo mixed = ResolveTestInfo(
      TestName{"TextFloatInfinityTest", "Positive", "Proto3_iNF"},
      TestPriority::kP0, request, NameStyle::kWithOutputFormat, numberer);

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
      TestPriority::kP3, JsonToTextRequest(), NameStyle::kWithOutputFormat,
      numberer);

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
      BinaryToBinaryRequest(), NameStyle::kWithOutputFormat, numberer);
  EXPECT_EQ(starts_with.test_case, "submessages_merge_submessage");
  // Otherwise the suite tells apart same-named tests of different suites.
  ConformanceTestInfo kept = ResolveTestInfo(
      TestName{"TextFloatTest", "Max", "Proto3"}, TestPriority::kP0,
      BinaryToBinaryRequest(), NameStyle::kWithOutputFormat, numberer);
  EXPECT_EQ(kept.test_case, "literals_text_float_max");
}

TEST(ResolveTestInfoTest, UnmappedSuiteIsFiledUnderDomainZero) {
  TestNumberer numberer;
  ConformanceTestInfo info =
      ResolveTestInfo(Foo(), TestPriority::kP0, BinaryToBinaryRequest(),
                      NameStyle::kWithOutputFormat, numberer);

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
      TestPriority::kP0, BinaryToBinaryRequest(), NameStyle::kWithOutputFormat,
      numberer);

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
      TestPriority::kP3, BinaryToBinaryRequest(),
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
