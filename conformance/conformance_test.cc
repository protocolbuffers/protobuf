// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance_test.h"

#include <stdarg.h>

#include <fstream>
#include <string>

#include "google/protobuf/util/field_comparator.h"
#include "google/protobuf/util/json_util.h"
#include "google/protobuf/util/message_differencer.h"
#include "absl/log/absl_log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "conformance/conformance.pb.h"
#include "conformance/conformance.pb.h"
#include "google/protobuf/descriptor_legacy.h"
#include "google/protobuf/message.h"
#include "google/protobuf/text_format.h"

using conformance::ConformanceRequest;
using conformance::ConformanceResponse;
using conformance::WireFormat;
using google::protobuf::TextFormat;
using google::protobuf::util::DefaultFieldComparator;
using google::protobuf::util::MessageDifferencer;
using std::string;

namespace {

static string ToOctString(const string& binary_string) {
  string oct_string;
  for (size_t i = 0; i < binary_string.size(); i++) {
    uint8_t c = binary_string.at(i);
    uint8_t high = c / 64;
    uint8_t mid = (c % 64) / 8;
    uint8_t low = c % 8;
    oct_string.push_back('\\');
    oct_string.push_back('0' + high);
    oct_string.push_back('0' + mid);
    oct_string.push_back('0' + low);
  }
  return oct_string;
}

template <typename SetT>
bool CheckSetEmpty(const SetT& set_to_check, absl::string_view write_to_file,
                   absl::string_view msg, absl::string_view output_dir,
                   std::string* output) {
  if (set_to_check.empty()) return true;

  absl::StrAppendFormat(output, "\n");
  absl::StrAppendFormat(output, "%s\n\n", msg);
  for (absl::string_view v : set_to_check) {
    absl::StrAppendFormat(output, "  %s\n", v);
  }
  absl::StrAppendFormat(output, "\n");

  if (!write_to_file.empty()) {
    std::string full_filename;
    absl::string_view filename = write_to_file;
    if (!output_dir.empty()) {
      full_filename = std::string(output_dir);
      if (*output_dir.rbegin() != '/') {
        full_filename.push_back('/');
      }
      absl::StrAppend(&full_filename, write_to_file);
      filename = full_filename;
    }
    std::ofstream os{std::string(filename)};
    if (os) {
      for (absl::string_view v : set_to_check) {
        os << v << "\n";
      }
    } else {
      absl::StrAppendFormat(output, "Failed to open file: %s\n", filename);
    }
  }

  return false;
}

}  // namespace

namespace google {
namespace protobuf {

ConformanceTestSuite::ConformanceRequestSetting::ConformanceRequestSetting(
    ConformanceLevel level, conformance::WireFormat input_format,
    conformance::WireFormat output_format,
    conformance::TestCategory test_category, const Message& prototype_message,
    const string& test_name, const string& input)
    : level_(level),
      input_format_(input_format),
      output_format_(output_format),
      prototype_message_(prototype_message),
      prototype_message_for_compare_(prototype_message.New()),
      test_name_(test_name) {
  switch (input_format) {
    case conformance::PROTOBUF: {
      request_.set_protobuf_payload(input);
      break;
    }

    case conformance::JSON: {
      request_.set_json_payload(input);
      break;
    }

    case conformance::JSPB: {
      request_.set_jspb_payload(input);
      break;
    }

    case conformance::TEXT_FORMAT: {
      request_.set_text_payload(input);
      break;
    }

    default:
      ABSL_LOG(FATAL) << "Unspecified input format";
  }

  request_.set_test_category(test_category);

  request_.set_message_type(prototype_message.GetDescriptor()->full_name());
  request_.set_requested_output_format(output_format);
}

std::unique_ptr<Message>
ConformanceTestSuite::ConformanceRequestSetting::NewTestMessage() const {
  return std::unique_ptr<Message>(prototype_message_for_compare_->New());
}

string ConformanceTestSuite::ConformanceRequestSetting::GetTestName() const {
  string rname;
  switch (FileDescriptorLegacy(prototype_message_.GetDescriptor()->file())
              .syntax()) {
    case FileDescriptorLegacy::Syntax::SYNTAX_PROTO3:
      rname = ".Proto3.";
      break;
    case FileDescriptorLegacy::Syntax::SYNTAX_PROTO2:
      rname = ".Proto2.";
      break;
    default:
      break;
  }

  return absl::StrCat(ConformanceLevelToString(level_), rname,
                      InputFormatString(input_format_), ".", test_name_, ".",
                      OutputFormatString(output_format_));
}

string
ConformanceTestSuite::ConformanceRequestSetting::ConformanceLevelToString(
    ConformanceLevel level) const {
  switch (level) {
    case REQUIRED:
      return "Required";
    case RECOMMENDED:
      return "Recommended";
  }
  ABSL_LOG(FATAL) << "Unknown value: " << level;
  return "";
}

string ConformanceTestSuite::ConformanceRequestSetting::InputFormatString(
    conformance::WireFormat format) const {
  switch (format) {
    case conformance::PROTOBUF:
      return "ProtobufInput";
    case conformance::JSON:
      return "JsonInput";
    case conformance::TEXT_FORMAT:
      return "TextFormatInput";
    default:
      ABSL_LOG(FATAL) << "Unspecified output format";
  }
  return "";
}

string ConformanceTestSuite::ConformanceRequestSetting::OutputFormatString(
    conformance::WireFormat format) const {
  switch (format) {
    case conformance::PROTOBUF:
      return "ProtobufOutput";
    case conformance::JSON:
      return "JsonOutput";
    case conformance::TEXT_FORMAT:
      return "TextFormatOutput";
    default:
      ABSL_LOG(FATAL) << "Unspecified output format";
  }
  return "";
}

void ConformanceTestSuite::TruncateDebugPayload(string* payload) {
  if (payload != nullptr && payload->size() > 200) {
    payload->resize(200);
    payload->append("...(truncated)");
  }
}

const ConformanceRequest ConformanceTestSuite::TruncateRequest(
    const ConformanceRequest& request) {
  ConformanceRequest debug_request(request);
  switch (debug_request.payload_case()) {
    case ConformanceRequest::kProtobufPayload:
      TruncateDebugPayload(debug_request.mutable_protobuf_payload());
      break;
    case ConformanceRequest::kJsonPayload:
      TruncateDebugPayload(debug_request.mutable_json_payload());
      break;
    case ConformanceRequest::kTextPayload:
      TruncateDebugPayload(debug_request.mutable_text_payload());
      break;
    case ConformanceRequest::kJspbPayload:
      TruncateDebugPayload(debug_request.mutable_jspb_payload());
      break;
    default:
      // Do nothing.
      break;
  }
  return debug_request;
}

const ConformanceResponse ConformanceTestSuite::TruncateResponse(
    const ConformanceResponse& response) {
  ConformanceResponse debug_response(response);
  switch (debug_response.result_case()) {
    case ConformanceResponse::kProtobufPayload:
      TruncateDebugPayload(debug_response.mutable_protobuf_payload());
      break;
    case ConformanceResponse::kJsonPayload:
      TruncateDebugPayload(debug_response.mutable_json_payload());
      break;
    case ConformanceResponse::kTextPayload:
      TruncateDebugPayload(debug_response.mutable_text_payload());
      break;
    case ConformanceResponse::kJspbPayload:
      TruncateDebugPayload(debug_response.mutable_jspb_payload());
      break;
    default:
      // Do nothing.
      break;
  }
  return debug_response;
}

void ConformanceTestSuite::ReportSuccess(const string& test_name) {
  if (expected_to_fail_.erase(test_name) != 0) {
    absl::StrAppendFormat(
        &output_,
        "ERROR: test %s is in the failure list, but test succeeded.  "
        "Remove it from the failure list.\n",
        test_name);
    unexpected_succeeding_tests_.insert(test_name);
  }
  successes_++;
}

void ConformanceTestSuite::ReportFailure(const string& test_name,
                                         ConformanceLevel level,
                                         const ConformanceRequest& request,
                                         const ConformanceResponse& response,
                                         absl::string_view message) {
  if (expected_to_fail_.erase(test_name) == 1) {
    expected_failures_++;
    if (!verbose_) return;
  } else if (level == RECOMMENDED && !enforce_recommended_) {
    absl::StrAppendFormat(&output_, "WARNING, test=%s: ", test_name);
  } else {
    absl::StrAppendFormat(&output_, "ERROR, test=%s: ", test_name);
    unexpected_failing_tests_.insert(test_name);
  }

  absl::StrAppendFormat(&output_, "%s, request=%s, response=%s\n", message,
                        TruncateRequest(request).ShortDebugString(),
                        TruncateResponse(response).ShortDebugString());
}

void ConformanceTestSuite::ReportSkip(const string& test_name,
                                      const ConformanceRequest& request,
                                      const ConformanceResponse& response) {
  if (verbose_) {
    absl::StrAppendFormat(
        &output_, "SKIPPED, test=%s request=%s, response=%s\n", test_name,
        request.ShortDebugString(), response.ShortDebugString());
  }
  skipped_.insert(test_name);
}

void ConformanceTestSuite::RunValidInputTest(
    const ConformanceRequestSetting& setting,
    const string& equivalent_text_format) {
  std::unique_ptr<Message> reference_message(setting.NewTestMessage());
  ABSL_CHECK(TextFormat::ParseFromString(equivalent_text_format,
                                         reference_message.get()))
      << "Failed to parse data for test case: " << setting.GetTestName()
      << ", data: " << equivalent_text_format;
  const string equivalent_wire_format = reference_message->SerializeAsString();
  RunValidBinaryInputTest(setting, equivalent_wire_format);
}

void ConformanceTestSuite::RunValidBinaryInputTest(
    const ConformanceRequestSetting& setting,
    const string& equivalent_wire_format, bool require_same_wire_format) {
  const ConformanceRequest& request = setting.GetRequest();
  ConformanceResponse response;
  RunTest(setting.GetTestName(), request, &response);
  VerifyResponse(setting, equivalent_wire_format, response, true,
                 require_same_wire_format);
}

void ConformanceTestSuite::VerifyResponse(
    const ConformanceRequestSetting& setting,
    const string& equivalent_wire_format, const ConformanceResponse& response,
    bool need_report_success, bool require_same_wire_format) {
  std::unique_ptr<Message> test_message(setting.NewTestMessage());
  const ConformanceRequest& request = setting.GetRequest();
  const string& test_name = setting.GetTestName();
  ConformanceLevel level = setting.GetLevel();
  std::unique_ptr<Message> reference_message = setting.NewTestMessage();

  ABSL_CHECK(reference_message->ParseFromString(equivalent_wire_format))
      << "Failed to parse wire data for test case: " << test_name;

  switch (response.result_case()) {
    case ConformanceResponse::RESULT_NOT_SET:
      ReportFailure(test_name, level, request, response,
                    "Response didn't have any field in the Response.");
      return;

    case ConformanceResponse::kParseError:
    case ConformanceResponse::kTimeoutError:
    case ConformanceResponse::kRuntimeError:
    case ConformanceResponse::kSerializeError:
      ReportFailure(test_name, level, request, response,
                    "Failed to parse input or produce output.");
      return;

    case ConformanceResponse::kSkipped:
      ReportSkip(test_name, request, response);
      return;

    default:
      if (!ParseResponse(response, setting, test_message.get())) return;
  }

  MessageDifferencer differencer;
  DefaultFieldComparator field_comparator;
  field_comparator.set_treat_nan_as_equal(true);
  differencer.set_field_comparator(&field_comparator);
  string differences;
  differencer.ReportDifferencesToString(&differences);

  bool check = false;

  if (require_same_wire_format) {
    ABSL_DCHECK_EQ(response.result_case(),
                   ConformanceResponse::kProtobufPayload);
    const string& protobuf_payload = response.protobuf_payload();
    check = equivalent_wire_format == protobuf_payload;
    differences = absl::StrCat("Expect: ", ToOctString(equivalent_wire_format),
                               ", but got: ", ToOctString(protobuf_payload));
  } else {
    check = differencer.Compare(*reference_message, *test_message);
  }

  if (check) {
    if (need_report_success) {
      ReportSuccess(test_name);
    }
  } else {
    ReportFailure(
        test_name, level, request, response,
        absl::StrCat("Output was not equivalent to reference message: ",
                     differences));
  }
}

void ConformanceTestSuite::RunTest(const string& test_name,
                                   const ConformanceRequest& request,
                                   ConformanceResponse* response) {
  if (test_names_.insert(test_name).second == false) {
    ABSL_LOG(FATAL) << "Duplicated test name: " << test_name;
  }

  string serialized_request;
  string serialized_response;
  request.SerializeToString(&serialized_request);

  runner_->RunTest(test_name, serialized_request, &serialized_response);

  if (!response->ParseFromString(serialized_response)) {
    response->Clear();
    response->set_runtime_error("response proto could not be parsed.");
  }

  if (verbose_) {
    absl::StrAppendFormat(
        &output_, "conformance test: name=%s, request=%s, response=%s\n",
        test_name, TruncateRequest(request).ShortDebugString(),
        TruncateResponse(*response).ShortDebugString());
  }
}

string ConformanceTestSuite::WireFormatToString(WireFormat wire_format) {
  switch (wire_format) {
    case conformance::PROTOBUF:
      return "PROTOBUF";
    case conformance::JSON:
      return "JSON";
    case conformance::JSPB:
      return "JSPB";
    case conformance::TEXT_FORMAT:
      return "TEXT_FORMAT";
    case conformance::UNSPECIFIED:
      return "UNSPECIFIED";
    default:
      ABSL_LOG(FATAL) << "unknown wire type: " << wire_format;
  }
  return "";
}

void ConformanceTestSuite::AddExpectedFailedTest(const std::string& test_name) {
  expected_to_fail_.insert(test_name);
}

bool ConformanceTestSuite::RunSuite(ConformanceTestRunner* runner,
                                    std::string* output, const string& filename,
                                    conformance::FailureSet* failure_list) {
  runner_ = runner;
  successes_ = 0;
  expected_failures_ = 0;
  skipped_.clear();
  test_names_.clear();
  unexpected_failing_tests_.clear();
  unexpected_succeeding_tests_.clear();

  output_ = "\nCONFORMANCE TEST BEGIN ====================================\n\n";

  failure_list_filename_ = filename;
  expected_to_fail_.clear();
  for (const string& failure : failure_list->failure()) {
    AddExpectedFailedTest(failure);
  }
  RunSuiteImpl();

  bool ok = true;
  if (!CheckSetEmpty(
          expected_to_fail_, "nonexistent_tests.txt",
          absl::StrCat("These tests were listed in the failure list, but they "
                       "don't exist.  Remove them from the failure list by "
                       "running:\n"
                       "  ./update_failure_list.py ",
                       failure_list_filename_,
                       " --remove nonexistent_tests.txt"),
          output_dir_, &output_)) {
    ok = false;
  }
  if (!CheckSetEmpty(
          unexpected_failing_tests_, "failing_tests.txt",
          absl::StrCat("These tests failed.  If they can't be fixed right now, "
                       "you can add them to the failure list so the overall "
                       "suite can succeed.  Add them to the failure list by "
                       "running:\n"
                       "  ./update_failure_list.py ",
                       failure_list_filename_, " --add failing_tests.txt"),
          output_dir_, &output_)) {
    ok = false;
  }
  if (!CheckSetEmpty(
          unexpected_succeeding_tests_, "succeeding_tests.txt",
          absl::StrCat("These tests succeeded, even though they were listed in "
                       "the failure list.  Remove them from the failure list "
                       "by running:\n"
                       "  ./update_failure_list.py ",
                       failure_list_filename_,
                       " --remove succeeding_tests.txt"),
          output_dir_, &output_)) {
    ok = false;
  }

  if (verbose_) {
    CheckSetEmpty(skipped_, "",
                  "These tests were skipped (probably because support for some "
                  "features is not implemented)",
                  output_dir_, &output_);
  }

  absl::StrAppendFormat(&output_,
                        "CONFORMANCE SUITE %s: %d successes, %zu skipped, "
                        "%d expected failures, %zu unexpected failures.\n",
                        ok ? "PASSED" : "FAILED", successes_, skipped_.size(),
                        expected_failures_, unexpected_failing_tests_.size());
  absl::StrAppendFormat(&output_, "\n");

  output->assign(output_);

  return ok;
}

}  // namespace protobuf
}  // namespace google
