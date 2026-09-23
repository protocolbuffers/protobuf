// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/text_format_conformance_suite.h"

#include <string>

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
void TextFormatConformanceTestSuiteImpl<MessageType>::RunAllTests() {
  // Every test has moved to the gtest-based text suite (text_*_test.cc); the
  // legacy suite itself is deleted in a follow-up CL.
}

}  // namespace protobuf
}  // namespace google
