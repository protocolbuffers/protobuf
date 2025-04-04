#include "conformance/v2/matchers.h"

#include <memory>
#include <ostream>
#include <string>

#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "conformance/v2/global_test_environment.h"
#include "conformance/v2/test_manager.h"
#include "conformance/v2/testee.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/message.h"

namespace google {
namespace protobuf {
namespace internal {

std::unique_ptr<Message> ParseBinary(const TestResult& result) {
  static DynamicMessageFactory* factory = new DynamicMessageFactory();
  auto message = absl::WrapUnique(factory->GetPrototype(result.type())->New());
  if (!message->ParseFromString(result.response().protobuf_payload())) {
    return nullptr;
  }
  return message;
}

std::unique_ptr<Message> ParseText(const TestResult& result) {
  static DynamicMessageFactory* factory = new DynamicMessageFactory();
  auto message = absl::WrapUnique(factory->GetPrototype(result.type())->New());
  if (!TextFormat::ParseFromString(result.response().text_payload(),
                                   message.get())) {
    return nullptr;
  }
  return message;
}

bool ReportSuccess(absl::string_view test_name,
                   testing::MatchResultListener* result_listener) {
  absl::Status status = GetGlobalTestManager().ReportSuccess(test_name);
  if (!status.ok()) {
    *result_listener << "\n" << status.message();
    return false;
  }
  return true;
}

bool ReportFailure(absl::string_view test_name,
                   absl::string_view failure_message,
                   testing::MatchResultListener* result_listener) {
  absl::Status status =
      GetGlobalTestManager().ReportFailure(test_name, failure_message);
  if (!status.ok()) {
    *result_listener << failure_message;
    *result_listener << "\n" << status.message();
    return false;
  }
  ABSL_LOG(INFO) << "Ignoring expected failure for test " << test_name;
  return true;
}

bool ReportSkip(absl::string_view test_name,
                testing::MatchResultListener* result_listener) {
  absl::Status status = GetGlobalTestManager().ReportSkip(test_name);
  if (!status.ok()) {
    *result_listener << "\n" << status.message();
    return false;
  }
  ABSL_LOG(WARNING) << "Skipping test " << test_name;
  return true;
}

void PrintTo(const internal::TestResult& result, std::ostream* os) {
  switch (result.response().result_case()) {
    case conformance::ConformanceResponse::kParseError:
      (*os) << absl::StreamFormat("parse error: \"%s\"",
                                  result.response().parse_error());
      break;
    case conformance::ConformanceResponse::kSerializeError:
      (*os) << absl::StreamFormat("serialize error: \"%s\"",
                                  result.response().serialize_error());
      break;
    case conformance::ConformanceResponse::kRuntimeError:
      (*os) << absl::StreamFormat("runtime error: \"%s\"",
                                  result.response().runtime_error());
      break;
    case conformance::ConformanceResponse::kSkipped:
      (*os) << absl::StreamFormat("skipped: \"%s\"",
                                  result.response().skipped());
      break;
    case conformance::ConformanceResponse::kTimeoutError:
      (*os) << absl::StreamFormat("timeout error: \"%s\"",
                                  result.response().timeout_error());
      break;
    case conformance::ConformanceResponse::kProtobufPayload: {
      auto message = internal::ParseBinary(result);
      if (message != nullptr) {
        TextFormat::Printer printer;
        printer.SetSingleLineMode(true);
        printer.SetExpandAny(true);
        printer.SetUseShortRepeatedPrimitives(true);
        std::string text;
        ABSL_CHECK(printer.PrintToString(*message, &text));
        (*os) << absl::StreamFormat("protobuf payload: %s", text);
      } else {
        (*os) << absl::StreamFormat(
            "protobuf payload: %s",
            absl::CEscape(result.response().protobuf_payload()));
      }
      break;
    }
    case conformance::ConformanceResponse::kTextPayload:
      (*os) << absl::StreamFormat("text payload: %s",
                                  result.response().text_payload());
      break;
    default:
      ABSL_CHECK(false) << "Unknown result case: "
                        << result.response().result_case();
      break;
  }
}

conformance::ConformanceResponse::ResultCase ExpectedResultCase(
    conformance::WireFormat format) {
  switch (format) {
    case conformance::PROTOBUF:
      return conformance::ConformanceResponse::kProtobufPayload;
    case conformance::TEXT_FORMAT:
      return conformance::ConformanceResponse::kTextPayload;
    case conformance::JSON:
      return conformance::ConformanceResponse::kJsonPayload;
    default:
      ABSL_CHECK(false) << "Unsupported output format "
                        << conformance::WireFormat_Name(format);
  }
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
