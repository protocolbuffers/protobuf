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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/memory/memory.h"
#include "absl/strings/ascii.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "conformance/binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "conformance/testee.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/port.h"
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

// The response field that a successful test with the given output format is
// expected to populate.  Tests always ask for a concrete format.
ConformanceResponse::ResultCase ExpectedResultCase(WireFormat format) {
  switch (format) {
    case ::conformance::PROTOBUF:
      return ConformanceResponse::kProtobufPayload;
    case ::conformance::JSON:
      return ConformanceResponse::kJsonPayload;
    case ::conformance::TEXT_FORMAT:
      return ConformanceResponse::kTextPayload;
    case ::conformance::JSPB:
      return ConformanceResponse::kJspbPayload;
    case ::conformance::UNSPECIFIED:
    default:
      google::protobuf::internal::Unreachable();
  }
}

// The name of the format of a payload, as used in legacy failure messages.
absl::string_view PayloadFormatName(ConformanceResponse::ResultCase result) {
  switch (result) {
    case ConformanceResponse::kProtobufPayload:
      return "PROTOBUF";
    case ConformanceResponse::kJsonPayload:
      return "JSON";
    case ConformanceResponse::kTextPayload:
      return "TEXT_FORMAT";
    case ConformanceResponse::kJspbPayload:
      return "JSPB";
    default:
      google::protobuf::internal::Unreachable();
  }
}

// The raw payload of a response, whichever format it is in.
absl::string_view RawPayload(const ConformanceResponse& response) {
  switch (response.result_case()) {
    case ConformanceResponse::kProtobufPayload:
      return response.protobuf_payload();
    case ConformanceResponse::kJsonPayload:
      return response.json_payload();
    case ConformanceResponse::kTextPayload:
      return response.text_payload();
    case ConformanceResponse::kJspbPayload:
      return response.jspb_payload();
    default:
      google::protobuf::internal::Unreachable();
  }
}

// The legacy failure message for a payload that couldn't be parsed.
std::string UnparseableMessage(WireFormat format) {
  switch (format) {
    case ::conformance::PROTOBUF:
      return "Protobuf output we received from test was unparseable.";
    default:
      return absl::StrCat(WireFormat_Name(format),
                          " output we received from test was unparseable.");
  }
}

// Parses the payload of `result` (according to its requested output format)
// into a message of the test's type.  Returns null and sets `failure_message`
// if the payload is invalid or the format isn't supported.
std::unique_ptr<Message> ParsePayload(const TestResult& result,
                                      std::string& failure_message) {
  std::unique_ptr<Message> message = NewMessage(result.type());
  switch (result.format()) {
    case ::conformance::PROTOBUF:
      if (!message->ParseFromString(result.response().protobuf_payload())) {
        failure_message = UnparseableMessage(result.format());
        return nullptr;
      }
      return message;
    case ::conformance::TEXT_FORMAT: {
      TextFormat::Parser parser;
      // Testees asked to print unknown fields emit them by field number, and
      // a known field named by number is the same field, so always accept
      // them.  Unknown numbers still fail to parse.
      parser.AllowFieldNumber(true);
      if (!parser.ParseFromString(result.response().text_payload(),
                                  message.get())) {
        failure_message = UnparseableMessage(result.format());
        return nullptr;
      }
      return message;
    }
    case ::conformance::JSON:
    case ::conformance::JSPB:
    case ::conformance::UNSPECIFIED:
    default:
      // TODO: b/410122158 - Support JSON once the JSON suite is migrated.
      failure_message = absl::StrCat(
          "WhenParsed is not supported for ", WireFormat_Name(result.format()),
          " output; use RawPayload() to match the raw JSON text until JSON "
          "matching is migrated to gtest (b/410122158).");
      return nullptr;
  }
}

// Formats binary data the same way the legacy runner does in failure messages.
std::string ToOctString(absl::string_view binary_string) {
  std::string oct_string;
  for (char ch : binary_string) {
    absl::StrAppendFormat(&oct_string, "\\%03o", ch);
  }
  return oct_string;
}

// Base class for the matchers that look at the payload of a successful
// response.  It handles all of the cases shared by every payload comparison
// (no payload, an error response, the wrong output format, a skipped test) and
// defers the actual comparison to MatchPayload().
class PayloadMatcher {
 public:
  using is_gtest_matcher = void;

  virtual ~PayloadMatcher() = default;

  bool MatchAndExplain(const TestResult& result,
                       testing::MatchResultListener* listener) const;
  void DescribeTo(std::ostream* os) const {
    DescribeInnerTo(os, /*negation=*/false);
  }
  void DescribeNegationTo(std::ostream* os) const {
    DescribeInnerTo(os, /*negation=*/true);
  }

 protected:
  PayloadMatcher() = default;
  PayloadMatcher(const PayloadMatcher&) = default;
  PayloadMatcher& operator=(const PayloadMatcher&) = default;

  // Compares the payload of `result`, which is guaranteed to hold a payload of
  // the format the test requested.  Returns true on a match; otherwise the
  // explanation written to `listener` becomes the failure message recorded in
  // the failure list.
  virtual bool MatchPayload(const TestResult& result,
                            testing::MatchResultListener* listener) const = 0;

  // Describes the matcher (or its negation).
  virtual void DescribeInnerTo(std::ostream* os, bool negation) const = 0;
};

bool PayloadMatcher::MatchAndExplain(
    const TestResult& result, testing::MatchResultListener* listener) const {
  const ConformanceResponse& response = result.response();
  switch (response.result_case()) {
    case ConformanceResponse::RESULT_NOT_SET:
      *listener << "Response didn't have any field in the Response.";
      return false;

    case ConformanceResponse::kParseError:
    case ConformanceResponse::kTimeoutError:
    case ConformanceResponse::kRuntimeError:
    case ConformanceResponse::kSerializeError:
      *listener << "Failed to parse input or produce output.";
      return false;

    case ConformanceResponse::kSkipped:
      *listener << "the testee skipped the test: " << response.skipped();
      return false;

    default:
      break;
  }

  if (response.result_case() != ExpectedResultCase(result.format())) {
    *listener << "Test was asked for " << WireFormat_Name(result.format())
              << " output but provided "
              << PayloadFormatName(response.result_case()) << " instead.";
    return false;
  }

  return MatchPayload(result, listener);
}

// Implements WhenParsed().
class WhenParsedMatcher : public PayloadMatcher {
 public:
  explicit WhenParsedMatcher(testing::Matcher<const Message&> matcher)
      : matcher_(std::move(matcher)) {}

 private:
  bool MatchPayload(const TestResult& result,
                    testing::MatchResultListener* listener) const override;
  void DescribeInnerTo(std::ostream* os, bool negation) const override;

  testing::Matcher<const Message&> matcher_;
};

bool WhenParsedMatcher::MatchPayload(
    const TestResult& result, testing::MatchResultListener* listener) const {
  std::string failure_message;
  std::unique_ptr<Message> actual = ParsePayload(result, failure_message);
  if (actual == nullptr) {
    *listener << failure_message;
    return false;
  }

  testing::StringMatchResultListener inner_listener;
  if (matcher_.MatchAndExplain(*actual, &inner_listener)) {
    *listener << inner_listener.str();
    return true;
  }
  if (inner_listener.str().empty()) {
    *listener << "Expect: when parsed, "
              << testing::DescribeMatcher<const Message&>(matcher_)
              << ", but got: {" << ToShortString(*actual) << "}";
  } else {
    *listener << inner_listener.str();
  }
  return false;
}

void WhenParsedMatcher::DescribeInnerTo(std::ostream* os, bool negation) const {
  *os << "when parsed, "
      << testing::DescribeMatcher<const Message&>(matcher_, negation);
}

// Implements RawPayload().
class RawPayloadMatcher : public PayloadMatcher {
 public:
  explicit RawPayloadMatcher(std::string expected)
      : expected_(std::move(expected)) {}

 private:
  bool MatchPayload(const TestResult& result,
                    testing::MatchResultListener* listener) const override;
  void DescribeInnerTo(std::ostream* os, bool negation) const override {
    *os << "payload " << (negation ? "isn't" : "is") << " equal to \""
        << absl::CEscape(expected_) << "\"";
  }

  std::string expected_;
};

bool RawPayloadMatcher::MatchPayload(
    const TestResult& result, testing::MatchResultListener* listener) const {
  // Like the legacy runner's `require_same_wire_format`, binary output must at
  // least be parseable as the test's message type.
  if (result.format() == ::conformance::PROTOBUF &&
      !NewMessage(result.type())
           ->ParseFromString(result.response().protobuf_payload())) {
    *listener << UnparseableMessage(result.format());
    return false;
  }

  absl::string_view actual = RawPayload(result.response());
  if (actual == expected_) {
    return true;
  }
  // TODO: b/568362905 - Point at the first differing byte instead of dumping
  // both payloads, once the legacy runner this message mirrors is gone.
  *listener << "Output was not equivalent to reference message: Expect: "
            << ToOctString(expected_) << ", but got: " << ToOctString(actual);
  return false;
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

testing::Matcher<const TestResult&> MakeWhenParsedMatcher(
    testing::Matcher<const Message&> m) {
  return WhenParsedMatcher(std::move(m));
}

}  // namespace internal

testing::Matcher<const internal::TestResult&> RawPayload(Wire bytes) {
  return RawPayloadMatcher(std::move(bytes).str());
}

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
