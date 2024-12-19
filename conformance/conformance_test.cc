// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance_test.h"

#include <stdarg.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <utility>

#include "google/protobuf/util/field_comparator.h"
#include "google/protobuf/util/message_differencer.h"
#include "absl/container/btree_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "conformance/conformance.pb.h"
#include "failure_list_trie_node.h"
#include "google/protobuf/descriptor_legacy.h"
#include "google/protobuf/endian.h"
#include "google/protobuf/message.h"
#include "google/protobuf/text_format.h"

using conformance::ConformanceRequest;
using conformance::ConformanceResponse;
using conformance::TestStatus;
using conformance::WireFormat;
using google::protobuf::util::DefaultFieldComparator;
using google::protobuf::util::MessageDifferencer;
using std::string;

namespace {

static void ReplaceAll(std::string& input, std::string replace_word,
                       std::string replace_by) {
  size_t pos = input.find(replace_word);
  while (pos != std::string::npos) {
    input.replace(pos, replace_word.length(), replace_by);
    pos = input.find(replace_word, pos + replace_by.length());
  }
}

static std::string ToOctString(const std::string& binary_string) {
  std::string oct_string;
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

// Returns full filename path of written .txt file if successful
static std::string ProduceOctalSerialized(const std::string& request,
                                          uint32_t len) {
  char* len_split_bytes = static_cast<char*>(static_cast<void*>(&len));

  std::string out;

  std::string hex_repr;
  for (int i = 0; i < 4; i++) {
    auto conversion = (unsigned int)static_cast<uint8_t>(len_split_bytes[i]);
    std::string hex = absl::StrFormat("\\x%x", conversion);
    absl::StrAppend(&hex_repr, hex);
  }

  absl::StrAppend(&out, hex_repr);

  absl::StrAppend(&out, ToOctString(request));

  return out;
}

static std::string WriteToFile(const std::string& octal_serialized,
                               const std::string& output_dir,
                               const std::string& test_name) {
  std::string test_name_txt = test_name;
  ReplaceAll(test_name_txt, ".", "_");
  absl::StrAppend(&test_name_txt, ".txt");
  std::string full_filename;
  if (!output_dir.empty()) {
    full_filename = output_dir;
    if (*output_dir.rbegin() != '/') {
      full_filename.push_back('/');
    }
    absl::StrAppend(&full_filename, test_name_txt);
  }
  std::ofstream os{std::string(full_filename)};
  if (os) {
    os << octal_serialized;
    return full_filename;
  } else {
    ABSL_LOG(INFO) << "Failed to open file for debugging: " << full_filename
                   << "\n";
    return "";
  }
}

// Removes all newlines.
static void Normalize(std::string& input) {
  input.erase(std::remove(input.begin(), input.end(), '\n'), input.end());
}

// Sets up a failure message properly for our failure lists.
static TestStatus FormatFailureMessage(const TestStatus& input) {
  // Make copy just this once, as we need to modify it for our failure lists.
  std::string formatted_failure_message = input.failure_message();
  // Remove newlines
  Normalize(formatted_failure_message);
  // Truncate failure message if needed
  if (formatted_failure_message.length() > 128) {
    formatted_failure_message = formatted_failure_message.substr(0, 128);
  }
  TestStatus properly_formatted;
  properly_formatted.set_name(input.name());
  properly_formatted.set_failure_message(formatted_failure_message);
  return properly_formatted;
}

bool CheckSetEmpty(const absl::btree_map<std::string, TestStatus>& set_to_check,
                   absl::string_view write_to_file, absl::string_view msg,
                   absl::string_view output_dir, std::string* output) {
  if (set_to_check.empty()) return true;

  absl::StrAppendFormat(output, "\n");
  absl::StrAppendFormat(output, "%s\n\n", msg);
  for (const auto& pair : set_to_check) {
    absl::StrAppendFormat(output, "  %s # %s\n", pair.first,
                          pair.second.failure_message());
  }
  absl::StrAppendFormat(output, "\n");

  if (!write_to_file.empty()) {
    std::string full_filename;
    absl::string_view filename = write_to_file;
    if (!output_dir.empty()) {
      full_filename = std::string(output_dir);
      absl::StrAppend(&full_filename, write_to_file);
      filename = full_filename;
    }
    std::ofstream os{std::string(filename)};
    if (os) {
      for (const auto& pair : set_to_check) {
        // Additions will not have a 'matched_name' while removals will.
        string potential_add_or_removal = pair.second.matched_name().empty()
                                              ? pair.first
                                              : pair.second.matched_name();
        os << potential_add_or_removal << " # " << pair.second.failure_message()
           << "\n";
      }
    } else {
      absl::StrAppendFormat(output,
                            "Failed to open file: %s\n",
                            filename);
    }
  }

  return false;
}

}  // namespace

namespace google {
namespace protobuf {

constexpr int kMaximumWildcardExpansions = 10;

ConformanceTestSuite::ConformanceRequestSetting::ConformanceRequestSetting(
    ConformanceLevel level, conformance::WireFormat input_format,
    conformance::WireFormat output_format,
    conformance::TestCategory test_category, const Message& prototype_message,
    const std::string& test_name, const std::string& input)
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

std::string
ConformanceTestSuite::ConformanceRequestSetting::GetSyntaxIdentifier() const {
  switch (FileDescriptorLegacy(prototype_message_.GetDescriptor()->file())
              .edition()) {
    case Edition::EDITION_PROTO3:
      return "Proto3";
    case Edition::EDITION_PROTO2:
      return "Proto2";
    default: {
      std::string id = "Editions";
      if (prototype_message_.GetDescriptor()->name() == "TestAllTypesProto2") {
        absl::StrAppend(&id, "_Proto2");
      } else if (prototype_message_.GetDescriptor()->name() ==
                 "TestAllTypesProto3") {
        absl::StrAppend(&id, "_Proto3");
      }
      return id;
    }
  }
}

string ConformanceTestSuite::ConformanceRequestSetting::GetTestName() const {
  return absl::StrCat(ConformanceLevelToString(level_), ".",
                      GetSyntaxIdentifier(), ".",
                      InputFormatString(input_format_), ".", test_name_, ".",
                      OutputFormatString(output_format_));
}

std::string
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

std::string ConformanceTestSuite::ConformanceRequestSetting::InputFormatString(
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

std::string ConformanceTestSuite::ConformanceRequestSetting::OutputFormatString(
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

ConformanceRequest ConformanceTestSuite::TruncateRequest(
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

ConformanceResponse ConformanceTestSuite::TruncateResponse(
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

void ConformanceTestSuite::ReportSuccess(const TestStatus& test) {
  if (expected_to_fail_.contains(test.name())) {
    absl::StrAppendFormat(&output_,
                          "ERROR: test %s (matched to %s) is in the failure "
                          "list, but test succeeded.  "
                          "Remove its match from the failure list.\n",
                          test.name(),
                          expected_to_fail_[test.name()].matched_name());
    unexpected_succeeding_tests_[test.name()] = expected_to_fail_[test.name()];
  }
  expected_to_fail_.erase(test.name());
  successes_++;
}

void ConformanceTestSuite::ReportFailure(TestStatus& test,
                                         ConformanceLevel level,
                                         const ConformanceRequest& request,
                                         const ConformanceResponse& response) {
  if (expected_to_fail_.contains(test.name())) {
    // Make copy just this once, as we need to modify them for comparison.
    // Failure message from the failure list.
    string expected_failure_message =
        expected_to_fail_[test.name()].failure_message();
    // Actual failure message from the test run.
    std::string actual_failure_message = test.failure_message();

    Normalize(actual_failure_message);
    if (actual_failure_message.rfind(expected_failure_message, 0) == 0) {
      // Our failure messages match.
      expected_failures_++;
    } else {
      // We want to add the test to the failure list with its correct failure
      // message.
      unexpected_failure_messages_[test.name()] = FormatFailureMessage(test);
      // We want to remove the test from the failure list. That means passing
      // to it the same failure message that was in the list.
      TestStatus incorrect_failure_message;
      incorrect_failure_message.set_name(test.name());
      incorrect_failure_message.set_failure_message(expected_failure_message);
      incorrect_failure_message.set_matched_name(
          expected_to_fail_[test.name()].matched_name());

      expected_failure_messages_[test.name()] = incorrect_failure_message;
    }
    expected_to_fail_.erase(test.name());
    if (!verbose_) return;
  } else if (level == RECOMMENDED && !enforce_recommended_) {
    absl::StrAppendFormat(&output_, "WARNING, test=%s: ", test.name());
  } else {
    absl::StrAppendFormat(&output_, "ERROR, test=%s: ", test.name());

    unexpected_failing_tests_[test.name()] = FormatFailureMessage(test);
  }

  absl::StrAppendFormat(&output_, "%s, request=%s, response=%s\n",
                        test.failure_message(),
                        TruncateRequest(request).ShortDebugString(),
                        TruncateResponse(response).ShortDebugString());
}

void ConformanceTestSuite::ReportSkip(const TestStatus& test,
                                      const ConformanceRequest& request,
                                      const ConformanceResponse& response) {
  if (verbose_) {
    absl::StrAppendFormat(
        &output_, "SKIPPED, test=%s request=%s, response=%s\n", test.name(),
        request.ShortDebugString(), response.ShortDebugString());
  }
  skipped_[test.name()] = test;
}

void ConformanceTestSuite::RunValidInputTest(
    const ConformanceRequestSetting& setting,
    const std::string& equivalent_text_format) {
  std::unique_ptr<Message> reference_message(setting.NewTestMessage());
  ABSL_CHECK(TextFormat::ParseFromString(equivalent_text_format,
                                         reference_message.get()))
      << "Failed to parse data for test case: " << setting.GetTestName()
      << ", data: " << equivalent_text_format;
  const std::string equivalent_wire_format =
      reference_message->SerializeAsString();
  RunValidBinaryInputTest(setting, equivalent_wire_format);
}

void ConformanceTestSuite::RunValidBinaryInputTest(
    const ConformanceRequestSetting& setting,
    const std::string& equivalent_wire_format, bool require_same_wire_format) {
  const ConformanceRequest& request = setting.GetRequest();
  ConformanceResponse response;
  if (!RunTest(setting.GetTestName(), request, &response)) {
    return;
  }

  VerifyResponse(setting, equivalent_wire_format, response, true,
                 require_same_wire_format);
}

void ConformanceTestSuite::VerifyResponse(
    const ConformanceRequestSetting& setting,
    const std::string& equivalent_wire_format,
    const ConformanceResponse& response, bool need_report_success,
    bool require_same_wire_format) {
  std::unique_ptr<Message> test_message(setting.NewTestMessage());
  const ConformanceRequest& request = setting.GetRequest();
  const std::string& test_name = setting.GetTestName();
  ConformanceLevel level = setting.GetLevel();
  std::unique_ptr<Message> reference_message = setting.NewTestMessage();

  ABSL_CHECK(reference_message->ParseFromString(equivalent_wire_format))
      << "Failed to parse wire data for test case: " << test_name;

  TestStatus test;
  test.set_name(test_name);

  switch (response.result_case()) {
    case ConformanceResponse::RESULT_NOT_SET:
      test.set_failure_message(
          "Response didn't have any field in the Response.");
      ReportFailure(test, level, request, response);
      return;

    case ConformanceResponse::kParseError:
    case ConformanceResponse::kTimeoutError:
    case ConformanceResponse::kRuntimeError:
    case ConformanceResponse::kSerializeError:
      test.set_failure_message("Failed to parse input or produce output.");
      ReportFailure(test, level, request, response);
      return;

    case ConformanceResponse::kSkipped:
      ReportSkip(test, request, response);
      return;

    default:
      if (!ParseResponse(response, setting, test_message.get())) return;
  }

  MessageDifferencer differencer;
  DefaultFieldComparator field_comparator;
  field_comparator.set_treat_nan_as_equal(true);
  differencer.set_field_comparator(&field_comparator);
  std::string differences;
  differencer.ReportDifferencesToString(&differences);

  bool check = false;

  if (require_same_wire_format) {
    ABSL_DCHECK_EQ(response.result_case(),
                   ConformanceResponse::kProtobufPayload);
    const std::string& protobuf_payload = response.protobuf_payload();
    check = equivalent_wire_format == protobuf_payload;
    differences = absl::StrCat("Expect: ", ToOctString(equivalent_wire_format),
                               ", but got: ", ToOctString(protobuf_payload));
  } else {
    check = differencer.Compare(*reference_message, *test_message);
  }
  if (check) {
    if (need_report_success) {
      ReportSuccess(test);
    }
  } else {
    test.set_failure_message(absl::StrCat(
        "Output was not equivalent to reference message: ", differences));
    ReportFailure(test, level, request, response);
  }
}

bool ConformanceTestSuite::RunTest(const std::string& test_name,
                                   const ConformanceRequest& request,
                                   ConformanceResponse* response) {
  if (test_names_ran_.insert(test_name).second == false) {
    ABSL_LOG(FATAL) << "Duplicated test name: " << test_name;
  }

  // In essence, find what wildcarded test names expand to or direct matches
  // (without wildcards).
  auto result = failure_list_root_.WalkDownMatch(test_name);
  if (result.has_value()) {
    string matched_equivalent = result.value();
    unmatched_.erase(matched_equivalent);
    TestStatus expansion;
    expansion.set_name(test_name);
    expansion.set_matched_name(matched_equivalent);
    expansion.set_failure_message(saved_failure_messages_[matched_equivalent]);
    expected_to_fail_[test_name] = expansion;

    if (number_of_matches_.contains(matched_equivalent)) {
      if (number_of_matches_[matched_equivalent] > kMaximumWildcardExpansions &&
          !exceeded_max_matches_.contains(matched_equivalent)) {
        exceeded_max_matches_[matched_equivalent] = expansion;
      }
      number_of_matches_[matched_equivalent]++;
    } else {
      number_of_matches_[matched_equivalent] = 1;
    }
  }

  std::string serialized_request;
  std::string serialized_response;
  request.SerializeToString(&serialized_request);

  uint32_t len = internal::little_endian::FromHost(
      static_cast<uint32_t>(serialized_request.size()));

  if (isolated_) {
    if (names_to_test_.erase(test_name) ==
        0) {  // Tests were asked to be run in isolated mode, but this test was
              // not asked to be run.
      expected_to_fail_.erase(test_name);
      return false;
    }
    if (debug_) {
      std::string octal = ProduceOctalSerialized(serialized_request, len);
      std::string full_filename = WriteToFile(octal, output_dir_, test_name);
      if (!full_filename.empty()) {
        absl::StrAppendFormat(
            &output_, "Produced octal serialized request file for test %s\n",
            test_name);
        absl::StrAppendFormat(
            &output_,
            "  To pipe the "
            "serialized request directly to "
            "the "
            "testee run from the root of your workspace:\n    printf $("
            "<\"%s\") | %s\n\n",
            full_filename, testee_);
        absl::StrAppendFormat(
            &output_,
            "  To inspect the wire format of the serialized request with "
            "protoscope run "
            "(Disclaimer: This may not work properly on non-Linux "
            "platforms):\n  "
            "  "
            "contents=$(<\"%s\"); sub=$(cut -d \\\\ -f 6- <<< "
            "$contents) ; printf \"\\\\${sub}\" | protoscope \n\n\n",
            full_filename);
      }
    }
  }

  response->set_protobuf_payload(serialized_request);

  runner_->RunTest(test_name, len, serialized_request, &serialized_response);

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
  return true;
}

std::string ConformanceTestSuite::WireFormatToString(WireFormat wire_format) {
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

bool ConformanceTestSuite::AddExpectedFailedTest(
    const TestStatus& expected_failure) {
  absl::Status attempt = failure_list_root_.Insert(expected_failure.name());
  if (!attempt.ok()) {
    absl::StrAppend(&output_, attempt.message(), "\n\n");
    return false;
  }
  unmatched_[expected_failure.name()] = expected_failure;
  saved_failure_messages_[expected_failure.name()] =
      expected_failure.failure_message();
  return true;
}

bool ConformanceTestSuite::RunSuite(ConformanceTestRunner* runner,
                                    std::string* output,
                                    const std::string& filename,
                                    conformance::FailureSet* failure_list) {
  runner_ = runner;
  failure_list_root_ = FailureListTrieNode("root");
  successes_ = 0;
  expected_failures_ = 0;
  skipped_.clear();
  test_names_ran_.clear();
  unexpected_failing_tests_.clear();
  unexpected_succeeding_tests_.clear();

  std::string mode = debug_ ? "DEBUG" : "TEST";
  absl::StrAppendFormat(
      &output_, "CONFORMANCE %s BEGIN ====================================\n\n",
      mode);

  failure_list_filename_ = filename;
  expected_to_fail_.clear();
  for (const TestStatus& expected_failure : failure_list->test()) {
    if (!AddExpectedFailedTest(expected_failure)) {
      output->assign(output_);
      return false;
    }
  }

  RunSuiteImpl();

  if (*output_dir_.rbegin() != '/') {
    output_dir_.push_back('/');
  }

  bool ok = true;
  if (!CheckSetEmpty(
          unmatched_, "unmatched.txt",
          absl::StrCat(
              "These test names were listed in the failure list, but they "
              "didn't match any actual test name.  Remove them from the "
              "failure list by running from the root of your workspace:\n"
              "  bazel run "
              "//google/protobuf/conformance:update_failure_list -- ",
              failure_list_filename_, " --remove ", output_dir_,
              "unmatched.txt"),
          output_dir_, &output_)) {
    ok = false;
  }

  if (!CheckSetEmpty(
          expected_failure_messages_, "expected_failure_messages.txt",
          absl::StrCat(
              "These tests (either expanded from wildcard(s) or direct "
              "matches) were listed in the failure list, but their "
              "failure messages do not match.  Remove their match from the "
              "failure list by running from the root of your workspace:\n"
              "  bazel run ",
              "//google/protobuf/conformance:update_failure_list -- ",
              failure_list_filename_, " --remove ", output_dir_,
              "expected_failure_messages.txt"),
          output_dir_, &output_)) {
    ok = false;
  }

  if (!CheckSetEmpty(
          unexpected_succeeding_tests_, "succeeding_tests.txt",
          absl::StrCat(
              "These tests succeeded, even though they were listed in "
              "the failure list (expanded from wildcard(s) or direct matches). "
              " Remove their match from the failure list by "
              "running from the root of your workspace:\n"
              "  bazel run "
              "//google/protobuf/conformance:update_failure_list -- ",
              failure_list_filename_, " --remove ", output_dir_,
              "succeeding_tests.txt"),
          output_dir_, &output_)) {
    ok = false;
  }

  if (!CheckSetEmpty(
          exceeded_max_matches_, "exceeded_max_matches.txt",
          absl::StrFormat(
              "These failure list entries served as matches to too many test "
              "names exceeding the max amount of %d.  "
              "Remove them from the failure list by running from the root of "
              "your workspace:\n"
              "  bazel run "
              "//google/protobuf/conformance:update_failure_list -- %s "
              "--remove %sexceeded_max_matches.txt",
              kMaximumWildcardExpansions, failure_list_filename_, output_dir_),
          output_dir_, &output_)) {
    ok = false;
  }

  if (!CheckSetEmpty(
          unexpected_failure_messages_, "unexpected_failure_messages.txt",
          absl::StrCat(
              "These tests (expanded from wildcard(s) or direct matches from "
              "the failure list) failed because their failure messages did "
              "not match.  If they can't be fixed right now, "
              "you can add them to the failure list so the overall "
              "suite can succeed.  Add them to the failure list by "
              "running from the root of your workspace:\n"
              "  bazel run "
              "//google/protobuf/conformance:update_failure_list -- ",
              failure_list_filename_, " --add ", output_dir_,
              "unexpected_failure_messages.txt"),
          output_dir_, &output_)) {
    ok = false;
  }

  if (!CheckSetEmpty(
          unexpected_failing_tests_, "failing_tests.txt",
          absl::StrCat(
              "These tests failed.  If they can't be fixed right now, "
              "you can add them to the failure list so the overall "
              "suite can succeed.  Add them to the failure list by "
              "running from the root of your workspace:\n"
              "  bazel run "
              "//google/protobuf/conformance:update_failure_list -- ",
              failure_list_filename_, " --add ", output_dir_,
              "failing_tests.txt"),
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
