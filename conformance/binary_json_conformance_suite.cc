// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/binary_json_conformance_suite.h"

#include <string>

#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "conformance/conformance.pb.h"
#include "conformance/conformance_test.h"
#include "google/protobuf/json/json.h"
#include "google/protobuf/message.h"

using ::conformance::ConformanceRequest;
using ::conformance::ConformanceResponse;
using ::conformance::TestStatus;
using ::conformance::WireFormat;

namespace google {
namespace protobuf {

bool BinaryAndJsonConformanceSuite::ParseJsonResponse(
    const ConformanceResponse& response, Message* test_message) {
  absl::Status status =
      json::JsonStringToMessage(response.json_payload(), test_message);
  if (!status.ok()) {
    ABSL_LOG(ERROR) << status;
    return false;
  }
  return true;
}

bool BinaryAndJsonConformanceSuite::ParseResponse(
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
      if (requested_output != ::conformance::PROTOBUF) {
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

    case ConformanceResponse::kJsonPayload: {
      if (requested_output != ::conformance::JSON) {
        test.set_failure_message(absl::StrCat(
            "Test was asked for ", WireFormatToString(requested_output),
            " output but provided JSON instead."));
        ReportFailure(test, level, request, response);
        return false;
      }

      if (!ParseJsonResponse(response, test_message)) {
        test.set_failure_message(
            "JSON output we received from test was unparseable.");
        ReportFailure(test, level, request, response);
        return false;
      }

      break;
    }

    default:
      ABSL_LOG(FATAL) << test_name
                      << ": unknown payload type: " << response.result_case()
                      << ", response: " << response;
  }

  return true;
}

// Every test has moved to the gtest suites (see the binary_conformance_tests
// and json_conformance_tests libraries in BUILD); nothing is left to send.
void BinaryAndJsonConformanceSuite::RunSuiteImpl() {}

}  // namespace protobuf
}  // namespace google
