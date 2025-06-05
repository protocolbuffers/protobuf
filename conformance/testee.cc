#include "testee.h"

#include <string>
#include <utility>

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "naming.h"
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

std::string GetTestName(absl::string_view test_name, TestStrictness strictness,
                        const ::conformance::ConformanceRequest& request,
                        const Descriptor& message) {
  absl::string_view strictness_string =
      strictness == TestStrictness::kRequired ? "Required" : "Recommended";
  std::string syntax_identifier = GetEditionIdentifier(message);

  return absl::StrCat(
      strictness_string, ".", syntax_identifier, ".",
      GetFormatIdentifier(GetInputFormat(request)), "Input.", test_name, ".",
      GetFormatIdentifier(request.requested_output_format()), "Output");
}

}  // namespace

::conformance::ConformanceResponse Testee::Run(
    absl::string_view test_name, const ConformanceRequest& request) {
  ABSL_CHECK(test_names_ran_.emplace(test_name).second)
      << "Duplicated test name: " << test_name;

  std::string serialized_request;
  request.SerializeToString(&serialized_request);

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
  return InMemoryMessage(testee_, name_, strictness_, type, std::move(request));
}

InMemoryMessage Test::ParseText(const Descriptor* type,
                                absl::string_view input) && {
  ::conformance::ConformanceRequest request;
  request.set_text_payload(input);
  request.set_test_category(::conformance::TEXT_FORMAT_TEST);
  request.set_message_type(type->full_name());
  return InMemoryMessage(testee_, name_, strictness_, type, std::move(request));
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
  return InMemoryMessage(testee_, name_, strictness_, type, std::move(request));
}

TestResult InMemoryMessage::SerializeBinary() && {
  return SerializeImpl(::conformance::PROTOBUF);
}

TestResult InMemoryMessage::SerializeText(TextSerializationOptions options) && {
  if (options.print_unknown_fields) {
    request_.set_print_unknown_fields(true);
  }

  return SerializeImpl(::conformance::TEXT_FORMAT);
}

TestResult InMemoryMessage::SerializeJson() && {
  return SerializeImpl(::conformance::JSON);
}

TestResult InMemoryMessage::SerializeImpl(
    const ::conformance::WireFormat format) {
  request_.set_requested_output_format(format);

  std::string full_name = GetTestName(name_, strictness_, request_, *type_);

  ::conformance::ConformanceResponse response =
      testee_->Run(full_name, request_);

  return TestResult(full_name, strictness_, type_, format, std::move(response));
}

}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
