// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/testee.h"

#include <exception>
#include <string>
#include <utility>

#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "conformance/binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "conformance/naming.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {
namespace {

using ::conformance::ConformanceRequest;
using ::conformance::ConformanceResponse;

::conformance::WireFormat GetInputFormat(
    const ::conformance::ConformanceRequest& request) {
  switch (request.payload_case()) {
    case ::conformance::ConformanceRequest::kProtobufPayload:
      return ::conformance::PROTOBUF;
    case ::conformance::ConformanceRequest::kJsonPayload:
      return ::conformance::JSON;
    case ::conformance::ConformanceRequest::kTextPayload:
      return ::conformance::TEXT_FORMAT;
    default:
      ABSL_LOG(FATAL) << "Unsupported input format";
  }
  return ::conformance::UNSPECIFIED;
}

// Builds the full test name: "<Level>.<Edition>.<InputFormat>Input.<name>"
// followed, for NameStyle::kWithOutputFormat, by ".<OutputFormat>Output".
std::string GetTestName(absl::string_view test_name, TestPriority priority,
                        const ::conformance::ConformanceRequest& request,
                        const Descriptor& message, NameStyle name_style) {
  std::string syntax_identifier = GetEditionIdentifier(message);

  std::string full_name = absl::StrCat(
      PriorityLevelName(priority), ".", syntax_identifier, ".",
      GetFormatIdentifier(GetInputFormat(request)), "Input.", test_name);
  if (name_style == NameStyle::kWithOutputFormat) {
    absl::StrAppend(&full_name, ".",
                    GetFormatIdentifier(request.requested_output_format()),
                    "Output");
  }
  return full_name;
}

}  // namespace

TestResult::TestResult(TestResult&& other) noexcept
    : test_name_(std::move(other.test_name_)),
      priority_(other.priority_),
      type_(other.type_),
      format_(other.format_),
      print_unknown_fields_(other.print_unknown_fields_),
      response_(std::move(other.response_)),
      checked_(other.checked_),
      verdict_(std::move(other.verdict_)),
      moved_from_(other.moved_from_) {
  other.moved_from_ = true;
}

TestResult& TestResult::operator=(TestResult&& other) noexcept {
  if (this != &other) {
    ReportIfUnchecked();
    test_name_ = std::move(other.test_name_);
    priority_ = other.priority_;
    type_ = other.type_;
    format_ = other.format_;
    print_unknown_fields_ = other.print_unknown_fields_;
    response_ = std::move(other.response_);
    checked_ = other.checked_;
    verdict_ = std::move(other.verdict_);
    moved_from_ = other.moved_from_;
    other.moved_from_ = true;
  }
  return *this;
}

TestResult::~TestResult() { ReportIfUnchecked(); }

void TestResult::SetVerdict(Verdict verdict) const {
  // A moved-from result has given up its name and response; checking it would
  // record a bogus outcome.
  ABSL_DCHECK(!moved_from_) << "Checking a moved-from TestResult";
  ABSL_DCHECK(!checked_) << "TestResult for " << test_name_
                         << " was already checked";
  checked_ = true;
  verdict_ = std::move(verdict);
}

void TestResult::ReportIfUnchecked() const {
  if (moved_from_ || checked_) {
    return;
  }
  // Don't pile a second failure onto a test that is already unwinding, e.g.
  // after an ASSERT_* returned early before the result could be checked.
  if (std::uncaught_exceptions() > 0 || testing::Test::HasFatalFailure()) {
    return;
  }
  ADD_FAILURE() << "TestResult for " << test_name_
                << " was never checked; wrap the matcher in Yields()";
}

::conformance::ConformanceResponse Testee::Run(
    absl::string_view test_name, const ConformanceRequest& request) {
  ABSL_CHECK(test_names_ran_.emplace(test_name).second)
      << "Duplicated test name: " << test_name;

  std::string serialized_request;
  // TODO: Remove this suppression.
  (void)request.SerializeToString(&serialized_request);

  std::string serialized_response =
      runner_->RunTest(test_name, serialized_request);

  ConformanceResponse response;
  if (!response.ParseFromString(serialized_response)) {
    response.set_runtime_error("response proto could not be parsed.");
  }

  return response;
}

InMemoryMessage Test::ParseBinary(const Descriptor* type, Wire input) && {
  ::conformance::ConformanceRequest request;
  request.set_protobuf_payload(std::move(input).data());
  request.set_test_category(::conformance::BINARY_TEST);
  request.set_message_type(type->full_name());
  return InMemoryMessage(testee_, name_, priority_, type, std::move(request));
}

InMemoryMessage Test::ParseText(const Descriptor* type,
                                absl::string_view input) && {
  ::conformance::ConformanceRequest request;
  request.set_text_payload(input);
  request.set_test_category(::conformance::TEXT_FORMAT_TEST);
  request.set_message_type(type->full_name());
  return InMemoryMessage(testee_, name_, priority_, type, std::move(request));
}

InMemoryMessage Test::ParseJson(const Descriptor* type, absl::string_view input,
                                JsonParseOptions options) && {
  ::conformance::ConformanceRequest request;
  request.set_json_payload(input);
  if (options.ignore_unknown_fields) {
    request.set_test_category(::conformance::JSON_IGNORE_UNKNOWN_PARSING_TEST);
  } else {
    request.set_test_category(::conformance::JSON_TEST);
  }
  request.set_message_type(type->full_name());
  return InMemoryMessage(testee_, name_, priority_, type, std::move(request));
}

TestResult InMemoryMessage::SerializeBinary() && {
  return Finish(::conformance::PROTOBUF, NameStyle::kWithOutputFormat);
}

TestResult InMemoryMessage::SerializeText(TextSerializationOptions options) && {
  if (options.print_unknown_fields) {
    request_.set_print_unknown_fields(true);
  }

  return Finish(::conformance::TEXT_FORMAT, NameStyle::kWithOutputFormat);
}

TestResult InMemoryMessage::SerializeJson() && {
  return Finish(::conformance::JSON, NameStyle::kWithOutputFormat);
}

TestResult InMemoryMessage::ParseOnly() && {
  // We don't expect output, but if the testee erroneously accepts the input we
  // let it send its response in the input's format.  We must not leave it
  // unspecified.  This is exactly what the legacy runner did.
  return Finish(GetInputFormat(request_), NameStyle::kWithoutOutputFormat);
}

TestResult InMemoryMessage::Finish(
    const ::conformance::WireFormat output_format, const NameStyle name_style) {
  request_.set_requested_output_format(output_format);

  std::string full_name =
      GetTestName(name_, priority_, request_, *type_, name_style);

  ::conformance::ConformanceResponse response =
      testee_->Run(full_name, request_);

  return TestResult(full_name, priority_, type_, request_, std::move(response));
}

}  // namespace internal

absl::string_view PriorityName(TestPriority priority) {
  switch (priority) {
    case TestPriority::kP0:
      return "P0";
    case TestPriority::kP1:
      return "P1";
  }
  return "Unknown";
}

absl::string_view PriorityLevelName(TestPriority priority) {
  // TODO: b/564550230 - return PriorityName() once the tests are renamed.
  return priority == TestPriority::kP1 ? "Recommended" : "Required";
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google
