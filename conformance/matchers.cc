// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/matchers.h"

#include <cstddef>
#include <memory>
#include <ostream>
#include <string>
#include <utility>

#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/memory/memory.h"
#include "absl/strings/ascii.h"
#include "absl/strings/escaping.h"
#include "absl/strings/string_view.h"
#include "conformance/binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "conformance/testee.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/util/field_comparator.h"
#include "google/protobuf/util/message_differencer.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::conformance::ConformanceResponse;
using ::conformance::WireFormat;
using ::google::protobuf::conformance::internal::TestResult;

// Implements EqualsTextProto() and EqualsBinaryProto(): matches a message
// equivalent to the one obtained by decoding `expected` (in `format`) as the
// actual message's type.
class EquivalentMessageMatcher {
 public:
  using is_gtest_matcher = void;

  EquivalentMessageMatcher(WireFormat format, std::string expected)
      : format_(format), expected_(std::move(expected)) {
    ABSL_CHECK(format == ::conformance::PROTOBUF ||
               format == ::conformance::TEXT_FORMAT)
        << "Unsupported expected message format " << WireFormat_Name(format);
  }

  bool MatchAndExplain(const Message& actual,
                       testing::MatchResultListener* listener) const {
    std::unique_ptr<Message> expected = absl::WrapUnique(actual.New());
    if (format_ == ::conformance::PROTOBUF) {
      ABSL_CHECK(expected->ParseFromString(expected_))
          << "Failed to parse expected wire data for "
          << actual.GetDescriptor()->full_name() << ": "
          << absl::CEscape(expected_);
    } else {
      ABSL_CHECK(TextFormat::ParseFromString(expected_, expected.get()))
          << "Failed to parse expected text proto for "
          << actual.GetDescriptor()->full_name() << ": " << expected_;
    }

    util::MessageDifferencer differencer;
    util::DefaultFieldComparator field_comparator;
    field_comparator.set_treat_nan_as_equal(true);
    differencer.set_field_comparator(&field_comparator);
    std::string differences;
    differencer.ReportDifferencesToString(&differences);
    if (differencer.Compare(*expected, actual)) {
      return true;
    }
    *listener << "Output was not equivalent to reference message: "
              << differences;
    return false;
  }

  void DescribeTo(std::ostream* os) const {
    *os << "equals "
        << (format_ == ::conformance::PROTOBUF ? "binary proto" : "text proto")
        << " \"" << absl::CEscape(expected_) << "\"";
  }

  void DescribeNegationTo(std::ostream* os) const {
    *os << "doesn't equal "
        << (format_ == ::conformance::PROTOBUF ? "binary proto" : "text proto")
        << " \"" << absl::CEscape(expected_) << "\"";
  }

 private:
  WireFormat format_;
  std::string expected_;
};

// Matches a result whose response holds a specific kind of error.
class FailureMatcher {
 public:
  using is_gtest_matcher = void;

  // `name` is used for descriptions (e.g. "parse error").  `failure_message`
  // is the explanation when the response isn't the expected error, and
  // `runtime_error_failure_message` the one when it is a runtime error.
  FailureMatcher(absl::string_view name,
                 ConformanceResponse::ResultCase expected_result,
                 absl::string_view failure_message,
                 absl::string_view runtime_error_failure_message)
      : name_(name),
        expected_result_(expected_result),
        failure_message_(failure_message),
        runtime_error_failure_message_(runtime_error_failure_message) {}

  bool MatchAndExplain(const TestResult& result,
                       testing::MatchResultListener* listener) const {
    ConformanceResponse::ResultCase actual = result.response().result_case();
    if (actual == expected_result_) {
      return true;
    }
    if (actual == ConformanceResponse::kSkipped) {
      *listener << "the testee skipped the test: "
                << result.response().skipped();
      return false;
    }
    if (actual == ConformanceResponse::kRuntimeError) {
      *listener << runtime_error_failure_message_;
      return false;
    }
    *listener << failure_message_;
    return false;
  }
  void DescribeTo(std::ostream* os) const { *os << "is a " << name_; }
  void DescribeNegationTo(std::ostream* os) const {
    *os << "is not a " << name_;
  }

 private:
  std::string name_;
  ConformanceResponse::ResultCase expected_result_;
  std::string failure_message_;
  std::string runtime_error_failure_message_;
};

// Truncates a payload for debug output, exactly like the legacy runner.
void TruncateDebugPayload(std::string& payload) {
  constexpr size_t kMaxDebugPayloadSize = 200;
  if (payload.size() > kMaxDebugPayloadSize) {
    payload.resize(kMaxDebugPayloadSize);
    payload.append("...(truncated)");
  }
}

// Returns a copy of `response` whose payload is truncated for debug output.
ConformanceResponse TruncateResponse(const ConformanceResponse& response) {
  ConformanceResponse debug_response(response);
  switch (debug_response.result_case()) {
    case ConformanceResponse::kProtobufPayload:
      TruncateDebugPayload(*debug_response.mutable_protobuf_payload());
      break;
    case ConformanceResponse::kJsonPayload:
      TruncateDebugPayload(*debug_response.mutable_json_payload());
      break;
    case ConformanceResponse::kTextPayload:
      TruncateDebugPayload(*debug_response.mutable_text_payload());
      break;
    case ConformanceResponse::kJspbPayload:
      TruncateDebugPayload(*debug_response.mutable_jspb_payload());
      break;
    default:
      break;
  }
  return debug_response;
}

// Creates an empty message of the given type, which must be a generated one.
std::unique_ptr<Message> NewMessage(const Descriptor* type) {
  const Message* prototype =
      MessageFactory::generated_factory()->GetPrototype(type);
  ABSL_CHECK(prototype != nullptr)
      << "Not a generated message type: " << type->full_name();
  return absl::WrapUnique(prototype->New());
}

// Prints a message on a single line (expanding Any, short repeated
// primitives) for failure output.  An explicit printer keeps the output
// stable, unlike DebugString()'s.
std::string ToShortString(const Message& message) {
  TextFormat::Printer printer;
  printer.SetSingleLineMode(true);
  printer.SetExpandAny(true);
  printer.SetUseShortRepeatedPrimitives(true);
  std::string text;
  ABSL_CHECK(printer.PrintToString(message, &text));
  absl::StripTrailingAsciiWhitespace(&text);
  return text;
}

}  // namespace

namespace internal {

void PrintTo(const TestResult& result, std::ostream* os) {
  *os << PriorityLevelName(result.priority()) << " test \"" << result.name()
      << "\" with response {"
      << ToShortString(TruncateResponse(result.response())) << "}";

  // Binary payloads are opaque, so also show what they decode to.
  if (result.response().has_protobuf_payload()) {
    std::unique_ptr<Message> decoded = NewMessage(result.type());
    if (decoded->ParseFromString(result.response().protobuf_payload())) {
      *os << " (decoded: {" << ToShortString(*decoded) << "})";
    } else {
      *os << " (unparseable)";
    }
  }
}

}  // namespace internal

testing::Matcher<const Message&> EqualsTextProto(absl::string_view text) {
  return EquivalentMessageMatcher(::conformance::TEXT_FORMAT,
                                  std::string(text));
}

testing::Matcher<const Message&> EqualsBinaryProto(Wire bytes) {
  return EquivalentMessageMatcher(::conformance::PROTOBUF,
                                  std::move(bytes).str());
}

testing::Matcher<const internal::TestResult&> IsParseError() {
  return FailureMatcher(
      "parse error", ConformanceResponse::kParseError,
      "Should have failed to parse, but didn't.",
      "Should have failed to parse, but raised an error instead.");
}

testing::Matcher<const internal::TestResult&> IsSerializeError() {
  return FailureMatcher(
      "serialize error", ConformanceResponse::kSerializeError,
      "Should have failed to serialize, but didn't.",
      "Should have failed to serialize, but raised an error instead.");
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google
