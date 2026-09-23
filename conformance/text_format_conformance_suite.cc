// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/text_format_conformance_suite.h"

#include <string>
#include <utility>
#include <vector>

#include "absl/log/absl_log.h"
#include "absl/log/die_if_null.h"
#include "absl/strings/str_cat.h"
#include "conformance/conformance_test.h"
#include "conformance/test_protos/test_messages_edition2023.pb.h"
#include "conformance/test_protos/test_messages_edition_unstable.pb.h"
#include "editions/golden/test_messages_proto2_editions.pb.h"
#include "editions/golden/test_messages_proto3_editions.pb.h"
#include "google/protobuf/test_messages_proto2.pb.h"
#include "google/protobuf/test_messages_proto3.pb.h"
#include "google/protobuf/text_format.h"

using conformance::ConformanceRequest;
using conformance::ConformanceResponse;
using conformance::TestStatus;
using conformance::WireFormat;
using protobuf_test_messages::editions::TestAllTypesEdition2023;
using protobuf_test_messages::proto2::TestAllTypesProto2;
using protobuf_test_messages::proto3::TestAllTypesProto3;
using TestAllTypesProto2Editions =
    protobuf_test_messages::editions::proto2::TestAllTypesProto2;
using TestAllTypesProto3Editions =
    protobuf_test_messages::editions::proto3::TestAllTypesProto3;

namespace google {
namespace protobuf {

TextFormatConformanceTestSuite::TextFormatConformanceTestSuite() {
  SetFailureListFlagName("--text_format_failure_list");
}

bool TextFormatConformanceTestSuite::ParseTextFormatResponse(
    const ConformanceResponse& response,
    const ConformanceRequestSetting& setting, Message* test_message) {
  TextFormat::Parser parser;
  const ConformanceRequest& request = setting.GetRequest();
  if (request.print_unknown_fields()) {
    parser.AllowFieldNumber(true);
  }
  if (!parser.ParseFromString(response.text_payload(), test_message)) {
    ABSL_LOG(ERROR) << "INTERNAL ERROR: internal text->protobuf transcode "
                    << "yielded unparseable proto. Text payload: "
                    << response.text_payload();
    return false;
  }

  return true;
}

bool TextFormatConformanceTestSuite::ParseResponse(
    const ConformanceResponse& response,
    const ConformanceRequestSetting& setting, Message* test_message) {
  const ConformanceRequest& request = setting.GetRequest();
  WireFormat requested_output = request.requested_output_format();
  const std::string& test_name = setting.GetTestName();
  ConformanceLevel level = setting.GetLevel();

  TestStatus test;
  test.set_name(test_name);
  switch (response.result_case()) {
    case ConformanceResponse::kProtobufPayload: {
      if (requested_output != conformance::PROTOBUF) {
        test.set_failure_message(absl::StrCat(
            "Test was asked for ", WireFormatToString(requested_output),
            " output but provided PROTOBUF instead."));
        ReportFailure(test, level, request, response);
        return false;
      }

      if (!test_message->ParseFromString(response.protobuf_payload())) {
        test.set_failure_message(
            "Protobuf output we received from test was unparseable.");
        ReportFailure(test, level, request, response);
        return false;
      }

      break;
    }

    case ConformanceResponse::kTextPayload: {
      if (requested_output != conformance::TEXT_FORMAT) {
        test.set_failure_message(absl::StrCat(
            "Test was asked for ", WireFormatToString(requested_output),
            " output but provided TEXT_FORMAT instead."));
        ReportFailure(test, level, request, response);
        return false;
      }

      if (!ParseTextFormatResponse(response, setting, test_message)) {
        test.set_failure_message(
            "TEXT_FORMAT output we received from test was unparseable.");
        ReportFailure(test, level, request, response);
        return false;
      }

      break;
    }

    default:
      ABSL_LOG(FATAL) << test_name
                      << ": unknown payload type: " << response.result_case();
  }

  return true;
}

void TextFormatConformanceTestSuite::RunSuiteImpl() {
  TextFormatConformanceTestSuiteImpl<TestAllTypesProto2>(this);
  TextFormatConformanceTestSuiteImpl<TestAllTypesProto3>(this);
  if (maximum_edition_ >= Edition::EDITION_2023) {
    TextFormatConformanceTestSuiteImpl<TestAllTypesProto2Editions>(this);
    TextFormatConformanceTestSuiteImpl<TestAllTypesProto3Editions>(this);
    TextFormatConformanceTestSuiteImpl<TestAllTypesEdition2023>(this);
  }
}

template <typename MessageType>
TextFormatConformanceTestSuiteImpl<MessageType>::
    TextFormatConformanceTestSuiteImpl(TextFormatConformanceTestSuite* suite)
    : suite_(*ABSL_DIE_IF_NULL(suite)) {
  // Flag control performance tests to keep them internal and opt-in only
  if (suite_.performance_) {
    // The performance tests have moved to the gtest suite
    // (text_performance_test.cc); a --performance run has nothing left here.
  } else {
    if (MessageType::GetDescriptor()->name() == "TestAllTypesProto3") {
      // TODO Run these over proto2 also.
      RunAllTests();
    }
  }
}

template <typename MessageType>
void TextFormatConformanceTestSuiteImpl<MessageType>::ExpectParseFailure(
    const std::string& test_name, ConformanceLevel level,
    const std::string& input) {
  MessageType prototype;
  // We don't expect output, but if the program erroneously accepts the protobuf
  // we let it send its response as this.  We must not leave it unspecified.
  ConformanceRequestSetting setting(
      level, conformance::TEXT_FORMAT, conformance::TEXT_FORMAT,
      conformance::TEXT_FORMAT_TEST, prototype, test_name, input);
  const ConformanceRequest& request = setting.GetRequest();
  ConformanceResponse response;
  std::string effective_test_name = absl::StrCat(
      setting.ConformanceLevelToString(level), ".",
      setting.GetSyntaxIdentifier(), ".TextFormatInput.", test_name);

  if (!suite_.RunTest(effective_test_name, request, &response)) {
    return;
  }

  TestStatus test;
  test.set_name(effective_test_name);
  if (response.result_case() == ConformanceResponse::kParseError) {
    suite_.ReportSuccess(test);
  } else if (response.result_case() == ConformanceResponse::kSkipped) {
    suite_.ReportSkip(test, request, response);
  } else {
    test.set_failure_message("Should have failed to parse, but didn't.");
    suite_.ReportFailure(test, level, request, response);
  }
}

template <typename MessageType>
void TextFormatConformanceTestSuiteImpl<MessageType>::RunValidTextFormatTest(
    const std::string& test_name, ConformanceLevel level,
    const std::string& input_text) {
  MessageType prototype;
  RunValidTextFormatTestWithMessage(test_name, level, input_text, prototype);
}

template <typename MessageType>
void TextFormatConformanceTestSuiteImpl<MessageType>::
    RunValidTextFormatTestWithMessage(const std::string& test_name,
                                      ConformanceLevel level,
                                      const std::string& input_text,
                                      const Message& message) {
  ConformanceRequestSetting setting1(
      level, conformance::TEXT_FORMAT, conformance::PROTOBUF,
      conformance::TEXT_FORMAT_TEST, message, test_name, input_text);
  suite_.RunValidInputTest(setting1, input_text);
  ConformanceRequestSetting setting2(
      level, conformance::TEXT_FORMAT, conformance::TEXT_FORMAT,
      conformance::TEXT_FORMAT_TEST, message, test_name, input_text);
  suite_.RunValidInputTest(setting2, input_text);
}

template <typename MessageType>
void TextFormatConformanceTestSuiteImpl<MessageType>::
    RunValidTextFormatTestWithExpected(const std::string& test_name,
                                       ConformanceLevel level,
                                       const std::string& input_text,
                                       const std::string& expected_text) {
  MessageType prototype;
  ConformanceRequestSetting setting1(
      level, conformance::TEXT_FORMAT, conformance::PROTOBUF,
      conformance::TEXT_FORMAT_TEST, prototype, test_name, input_text);
  suite_.RunValidInputTest(setting1, expected_text);
  ConformanceRequestSetting setting2(
      level, conformance::TEXT_FORMAT, conformance::TEXT_FORMAT,
      conformance::TEXT_FORMAT_TEST, prototype, test_name, input_text);
  suite_.RunValidInputTest(setting2, expected_text);
}

template <typename MessageType>
void TextFormatConformanceTestSuiteImpl<MessageType>::RunAllTests() {
  // Reserved field names
  for (const auto& test_case : std::vector<std::pair<std::string, std::string>>{
           {"Boolean", "true"},
           {"Integer", "-123"},
           {"Float", "0.123"},
           {"Enum", "ENUM_VALUE"},
           {"String", "\"hello\""},
           {"Message", "{ a: 123 }"},
           {"MessageAngleBrackets", "< a: 123 >"},
           {"RepeatedInteger", "[-123, 456]"},
           {"RepeatedFloat", "[0.123, 1e-10] "},
           {"RepeatedString", R"pb(["hello", "world"])pb"},
       }) {
    RunValidTextFormatTest(absl::StrCat("ReservedFieldName.", test_case.first),
                           REQUIRED,
                           absl::StrCat("reserved_field: ", test_case.second));
    // TODO: Add a test for reserved field numbers on 999999.
  }

  // Map fields
  MessageType prototype;
  (*prototype.mutable_map_string_string())["c"] = "value";
  (*prototype.mutable_map_string_string())["b"] = "value";
  (*prototype.mutable_map_string_string())["a"] = "value";
  RunValidTextFormatTestWithMessage("AlphabeticallySortedMapStringKeys",
                                    REQUIRED,
                                    R"(
        map_string_string {
          key: "a"
          value: "value"
        }
        map_string_string {
          key: "b"
          value: "value"
        }
        map_string_string {
          key: "c"
          value: "value"
        }
        )",
                                    prototype);

  prototype.Clear();
  (*prototype.mutable_map_int32_int32())[3] = 0;
  (*prototype.mutable_map_int32_int32())[2] = 0;
  (*prototype.mutable_map_int32_int32())[1] = 0;
  RunValidTextFormatTestWithMessage("AlphabeticallySortedMapIntKeys", REQUIRED,
                                    R"(
        map_int32_int32 {
          key: 1
          value: 0
        }
        map_int32_int32 {
          key: 2
          value: 0
        }
        map_int32_int32 {
          key: 3
          value: 0
        }
        )",
                                    prototype);

  prototype.Clear();
  (*prototype.mutable_map_bool_bool())[true] = false;
  (*prototype.mutable_map_bool_bool())[false] = false;
  RunValidTextFormatTestWithMessage("AlphabeticallySortedMapBoolKeys", REQUIRED,
                                    R"(
        map_bool_bool {
          key: false
          value: false
        }
        map_bool_bool {
          key: true
          value: false
        }
        )",
                                    prototype);

  prototype.Clear();
  ConformanceRequestSetting setting_map(
      REQUIRED, conformance::TEXT_FORMAT, conformance::PROTOBUF,
      conformance::TEXT_FORMAT_TEST, prototype, "DuplicateMapKey", R"(
        map_string_nested_message {
          key: "duplicate"
          value: { a: 123 }
        }
        map_string_nested_message {
          key: "duplicate"
          value: { corecursive: {} }
        }
        )");
  // The last-specified value will be retained in a parsed map
  suite_.RunValidInputTest(setting_map, R"(
        map_string_nested_message {
          key: "duplicate"
          value: { corecursive: {} }
        }
        )");
}

}  // namespace protobuf
}  // namespace google
