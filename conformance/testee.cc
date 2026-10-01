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
#include "absl/strings/ascii.h"
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

// Whether `piece` may appear in a test name: only characters gtest allows in
// test and parameter names, so that every "." and "/" in a full name is a
// segment separator and "*" can never be mistaken for a failure-list wildcard.
bool IsValidNamePiece(absl::string_view piece) {
  for (char c : piece) {
    if (!absl::ascii_isalnum(c) && c != '_') return false;
  }
  return true;
}

// Builds the full test name (see TestName in testee.h):
// "<Level>.<Suite>.<Test>[/<Params>][.<Suffix>].<Input>Input", followed by
// ".<Output>Output" unless `name_style` omits it.
std::string GetTestName(const TestName& name, TestPriority priority,
                        const ::conformance::ConformanceRequest& request,
                        NameStyle name_style) {
  std::string full_name =
      absl::StrCat(PriorityName(priority), ".", name.suite, ".", name.test);
  if (!name.params.empty()) absl::StrAppend(&full_name, "/", name.params);
  if (!name.suffix.empty()) absl::StrAppend(&full_name, ".", name.suffix);
  absl::StrAppend(&full_name, ".", GetFormatIdentifier(GetInputFormat(request)),
                  "Input");
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

Test Testee::CreateTest(TestName name, TestPriority priority) {
  ABSL_CHECK(!name.suite.empty() && !name.test.empty())
      << "A conformance test needs a gtest suite and test name";
  for (absl::string_view piece :
       {name.suite, name.test, name.params, name.suffix}) {
    ABSL_CHECK(IsValidNamePiece(piece))
        << "Test name pieces may only contain letters, digits and '_': "
        << piece;
  }
  return Test(this, std::move(name), priority);
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
  return InMemoryMessage(testee_, std::move(name_), priority_, type,
                         std::move(request));
}

InMemoryMessage Test::ParseText(const Descriptor* type,
                                absl::string_view input) && {
  ::conformance::ConformanceRequest request;
  request.set_text_payload(input);
  request.set_test_category(::conformance::TEXT_FORMAT_TEST);
  request.set_message_type(type->full_name());
  return InMemoryMessage(testee_, std::move(name_), priority_, type,
                         std::move(request));
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
  return InMemoryMessage(testee_, std::move(name_), priority_, type,
                         std::move(request));
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

TestResult InMemoryMessage::ParseOnly(ParseOnlyOptions options) && {
  // We don't expect output, but if the testee erroneously accepts the input we
  // let it send its response in the input's format (unless the test says
  // otherwise).  We must not leave it unspecified.  This is exactly what the
  // legacy runner did.
  return Finish(options.output_format.value_or(GetInputFormat(request_)),
                NameStyle::kWithoutOutputFormat);
}

InMemoryMessage&& InMemoryMessage::OverrideTestCategory(
    ::conformance::TestCategory category) && {
  ABSL_DCHECK_EQ(category, ::conformance::TEXT_FORMAT_TEST)
      << "OverrideTestCategory() only relabels the text-format suite's "
         "binary-input tests as TEXT_FORMAT_TEST";
  ABSL_DCHECK(request_.has_protobuf_payload())
      << "OverrideTestCategory() is for PROTOBUF-input tests";
  request_.set_test_category(category);
  return std::move(*this);
}

TestResult InMemoryMessage::Finish(
    const ::conformance::WireFormat output_format, const NameStyle name_style) {
  request_.set_requested_output_format(output_format);

  std::string full_name = GetTestName(name_, priority_, request_, name_style);

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

}  // namespace conformance
}  // namespace protobuf
}  // namespace google
