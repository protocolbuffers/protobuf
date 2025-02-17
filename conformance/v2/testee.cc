#include "conformance/v2/testee.h"

#include <cstdint>
#include <string>
#include <utility>

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "conformance/conformance.pb.h"
#include "conformance/v2/binary_wireformat.h"
#include "conformance/v2/naming.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/endian.h"

namespace google {
namespace protobuf {
namespace internal {
namespace {

using ::conformance::ConformanceRequest;
using ::conformance::ConformanceResponse;

absl::string_view FormatToString(conformance::WireFormat format) {
  switch (format) {
    case conformance::PROTOBUF:
      return "Protobuf";
    case conformance::JSON:
      return "Json";
    case conformance::TEXT_FORMAT:
      return "TextFormat";
    default:
      ABSL_LOG(FATAL) << "Unspecified output format";
  }
  return "";
}

conformance::WireFormat GetInputFormat(
    const conformance::ConformanceRequest& request) {
  switch (request.payload_case()) {
    case conformance::ConformanceRequest::kProtobufPayload:
      return conformance::PROTOBUF;
    case conformance::ConformanceRequest::kJsonPayload:
      return conformance::JSON;
    case conformance::ConformanceRequest::kTextPayload:
      return conformance::TEXT_FORMAT;
    default:
      ABSL_LOG(FATAL) << "Unsupported input format";
  }
  return conformance::UNSPECIFIED;
}

std::string GetTestName(absl::string_view test_name, TestStrictness strictness,
                        const conformance::ConformanceRequest& request,
                        const Descriptor& message) {
  absl::string_view strictness_string =
      strictness == TestStrictness::kRequired ? "Required" : "Recommended";
  std::string syntax_identifier = GetEditionIdentifier(message);

  return absl::StrCat(
      strictness_string, ".", syntax_identifier, ".",
      FormatToString(GetInputFormat(request)), "Input.", test_name, ".",
      // TODO: GTest names can't match the old ones perfectly, but would be
      // easier to maintain moving forward.
      // ::testing::UnitTest::GetInstance()->current_test_info()->name(), ".",
      FormatToString(request.requested_output_format()), "Output");
}

}  // namespace

conformance::ConformanceResponse Testee::Run(
    absl::string_view test_name, const ConformanceRequest& request) {
  ABSL_CHECK(test_names_ran_.emplace(test_name).second)
      << "Duplicated test name: " << test_name;

  std::string serialized_request;
  std::string serialized_response;
  request.SerializeToString(&serialized_request);

  uint32_t len = internal::little_endian::FromHost(
      static_cast<uint32_t>(serialized_request.size()));

  runner_.RunTest(test_name, len, serialized_request, &serialized_response);

  ConformanceResponse response;
  if (!response.ParseFromString(serialized_response)) {
    response.Clear();
    response.set_runtime_error("response proto could not be parsed.");
  }

  return response;
}

InMemoryMessage Test::ParseBinary(const Descriptor* type, Wire input) {
  conformance::ConformanceRequest request;
  request.set_protobuf_payload(std::move(input).data());
  request.set_test_category(conformance::BINARY_TEST);
  request.set_message_type(type->full_name());
  return InMemoryMessage(this, type, std::move(request));
}

InMemoryMessage Test::ParseText(const Descriptor* type,
                                absl::string_view input) {
  conformance::ConformanceRequest request;
  request.set_text_payload(input);
  request.set_test_category(conformance::TEXT_FORMAT_TEST);
  request.set_message_type(type->full_name());
  return InMemoryMessage(this, type, std::move(request));
}

InMemoryMessage Test::ParseJson(const Descriptor* type, absl::string_view input,
                                JsonParseOptions options) {
  conformance::ConformanceRequest request;
  request.set_json_payload(input);
  if (options.ignore_unknown_fields) {
    request.set_test_category(conformance::JSON_IGNORE_UNKNOWN_PARSING_TEST);
  } else {
    request.set_test_category(conformance::JSON_TEST);
  }
  request.set_message_type(type->full_name());
  return InMemoryMessage(this, type, std::move(request));
}

TestResult InMemoryMessage::SerializeBinary() && {
  request_.set_requested_output_format(conformance::PROTOBUF);

  std::string full_name =
      GetTestName(test_->name(), test_->strictness(), request_, *type_);

  conformance::ConformanceResponse response =
      test_->testee().Run(full_name, request_);

  return TestResult(test_->name(), type_, std::move(request_),
                    std::move(response));
}

TestResult InMemoryMessage::SerializeText(TextFormatOptions options) && {
  request_.set_requested_output_format(conformance::TEXT_FORMAT);
  if (options.print_unknown_fields) {
    request_.set_print_unknown_fields(true);
  }

  std::string full_name =
      GetTestName(test_->name(), test_->strictness(), request_, *type_);

  conformance::ConformanceResponse response =
      test_->testee().Run(full_name, request_);

  return TestResult(test_->name(), type_, std::move(request_),
                    std::move(response));
}

TestResult InMemoryMessage::SerializeJson() && {
  request_.set_requested_output_format(conformance::JSON);

  std::string full_name =
      GetTestName(test_->name(), test_->strictness(), request_, *type_);

  conformance::ConformanceResponse response =
      test_->testee().Run(full_name, request_);

  return TestResult(test_->name(), type_, std::move(request_),
                    std::move(response));
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
