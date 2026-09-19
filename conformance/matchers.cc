// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "matchers.h"

#include <cstdint>
#include <memory>
#include <ostream>
#include <string>
#include <utility>

#include <gmock/gmock.h>
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/strings/ascii.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "json/reader.h"
#include "json/value.h"
#include "conformance/conformance.pb.h"
#include "global_test_environment.h"
#include "test_manager.h"
#include "test_runner.h"
#include "testee.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/json/json.h"
#include "google/protobuf/message.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/unknown_field_set.h"
#include "google/protobuf/util/field_comparator.h"
#include "google/protobuf/util/message_differencer.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {
namespace {

using ::conformance::ConformanceRequest;
using ::conformance::ConformanceResponse;
using ::conformance::WireFormat;

// The legacy prefix of every payload mismatch.
constexpr absl::string_view kNotEquivalentPrefix =
    "Output was not equivalent to reference message: ";

// The legacy failure messages for responses that carry no payload.
constexpr absl::string_view kNoResultMessage =
    "Response didn't have any field in the Response.";
constexpr absl::string_view kErrorResultMessage =
    "Failed to parse input or produce output.";

// The response field that a successful test with the given output format is
// expected to populate.  UNSPECIFIED (and any unknown format) maps to
// RESULT_NOT_SET, which no payload ever matches.
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
      return ConformanceResponse::RESULT_NOT_SET;
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
      return "UNKNOWN";
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
      return "";
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
// into a message of type `type`.  Returns null and sets `*failure_message` if
// the payload is invalid or the format isn't supported.
std::unique_ptr<Message> ParsePayload(const TestResult& result,
                                      const Descriptor* type,
                                      std::string* failure_message) {
  std::unique_ptr<Message> message = NewMessage(type);
  switch (result.format()) {
    case ::conformance::PROTOBUF:
      if (!message->ParseFromString(result.response().protobuf_payload())) {
        *failure_message = UnparseableMessage(result.format());
        return nullptr;
      }
      return message;
    case ::conformance::TEXT_FORMAT: {
      TextFormat::Parser parser;
      // Testees asked to print unknown fields emit them by field number, so
      // (and only so) accept field numbers, like the legacy runner does.
      if (result.request().print_unknown_fields()) {
        parser.AllowFieldNumber(true);
      }
      if (!parser.ParseFromString(result.response().text_payload(),
                                  message.get())) {
        *failure_message = UnparseableMessage(result.format());
        return nullptr;
      }
      return message;
    }
    case ::conformance::JSON: {
      // Decoded with the default json::ParseOptions, exactly like the legacy
      // runner's ParseJsonResponse (which built stricter options but never
      // passed them).
      // TODO: b/563658359 - Set allow_legacy_nonconformant_behavior = false
      // once the failures that surfaces have been triaged across languages.
      absl::Status status = json::JsonStringToMessage(
          result.response().json_payload(), message.get());
      if (!status.ok()) {
        ABSL_LOG(INFO) << "JSON output of " << result.name()
                       << " is unparseable: " << status;
        *failure_message = UnparseableMessage(result.format());
        return nullptr;
      }
      return message;
    }
    case ::conformance::JSPB:
    case ::conformance::UNSPECIFIED:
    default:
      // Unreachable through PayloadMatcher, which only calls ParsePayload()
      // once the response carries the payload the test asked for, and tests
      // can only ask for PROTOBUF, TEXT_FORMAT or JSON output.
      *failure_message =
          absl::StrCat("ParsedPayload is not supported for ",
                       WireFormat_Name(result.format()), " output.");
      return nullptr;
  }
}

// Formats binary data the same way the legacy runner does in failure messages.
std::string ToOctString(absl::string_view binary_string) {
  std::string oct_string;
  for (char ch : binary_string) {
    uint8_t c = static_cast<uint8_t>(ch);
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

// The inner matcher for Payload(bytes).
class EqualsBytesMatcher {
 public:
  using is_gtest_matcher = void;

  explicit EqualsBytesMatcher(absl::string_view expected)
      : expected_(expected) {}

  bool MatchAndExplain(absl::string_view actual,
                       testing::MatchResultListener* listener) const {
    if (actual == expected_) {
      return true;
    }
    *listener << kNotEquivalentPrefix << "Expect: " << ToOctString(expected_)
              << ", but got: " << ToOctString(actual);
    return false;
  }
  void DescribeTo(std::ostream* os) const {
    *os << "is equal to \"" << absl::CEscape(expected_) << "\"";
  }
  void DescribeNegationTo(std::ostream* os) const {
    *os << "isn't equal to \"" << absl::CEscape(expected_) << "\"";
  }

 private:
  std::string expected_;
};

// The legacy failure message for a response that is a failure whatever the
// inner matcher of Yields() says: no result at all, or a runtime or timeout
// error.  Returns nullopt for any other response.
absl::optional<absl::string_view> ForcedFailureMessage(
    const ConformanceResponse& response) {
  switch (response.result_case()) {
    case ConformanceResponse::RESULT_NOT_SET:
      return kNoResultMessage;
    case ConformanceResponse::kRuntimeError:
    case ConformanceResponse::kTimeoutError:
      return kErrorResultMessage;
    default:
      return absl::nullopt;
  }
}

// Implements Yields().  This is the single matcher that talks to the global
// TestManager; see matchers.h for the policy it applies.
//
// The TestManager must only hear about each test once, so a result is
// evaluated the first time it is seen and the verdict is stored in the result
// itself.  gtest evaluates a failing matcher a second time to explain the
// failure; that replays the stored failure.  Anything else that finds the
// result already checked is a test bug and fails.
class YieldsMatcherImpl : public testing::MatcherInterface<const TestResult&> {
 public:
  explicit YieldsMatcherImpl(testing::Matcher<const TestResult&> inner)
      : inner_(std::move(inner)) {}

  bool MatchAndExplain(const TestResult& result,
                       testing::MatchResultListener* listener) const override {
    if (!result.checked()) {
      TestResult::Verdict verdict = Evaluate(result);
      *listener << verdict.explanation;
      const bool matched = verdict.matched;
      result.SetVerdict(std::move(verdict));
      return matched;
    }
    const absl::optional<TestResult::Verdict>& verdict = result.verdict();
    if (verdict.has_value() && !verdict->matched) {
      *listener << verdict->explanation;
      return false;
    }
    *listener << "TestResult for " << result.name()
              << " was already checked; each result may be checked exactly "
                 "once";
    return false;
  }

  void DescribeTo(std::ostream* os) const override {
    *os << "yields a result that ";
    inner_.DescribeTo(os);
    *os << " (or is an expected failure)";
  }

  void DescribeNegationTo(std::ostream* os) const override {
    *os << "doesn't yield a result that ";
    inner_.DescribeTo(os);
    *os << ", nor an expected failure";
  }

 private:
  // Reports `result` to the global TestManager and returns the gtest verdict.
  TestResult::Verdict Evaluate(const TestResult& result) const {
    TestManager& manager = GetGlobalTestManager();
    const std::string name(result.name());
    const ConformanceResponse& response = result.response();
    const bool listed = manager.IsExpectedToFail(name);

    // A test the runner filtered out (e.g. with --test) never ran; it is
    // neither a testee skip nor, if listed, a "listed but skipped" failure.
    // The manager only learns that the test exists, so that its failure list
    // entry isn't reported as matching no test.
    if (response.result_case() == ConformanceResponse::kSkipped &&
        response.skipped() == kTestNotSelectedSkipReason) {
      manager.ReportNotSelected(name);
      return {.matched = true, .explanation = "which was not selected to run"};
    }

    // A skip is decided here; the inner matcher never sees it.  The manager
    // marks a listed entry as seen and returns an error for it, so this is the
    // only place that reports a listed test the testee skipped (stricter than
    // legacy, see matchers.h).
    if (response.result_case() == ConformanceResponse::kSkipped) {
      absl::Status status = manager.ReportSkip(name, response.skipped());
      ABSL_LOG(INFO) << "Skipping test " << name << ": " << response.skipped();
      if (!status.ok()) {
        return {.matched = false, .explanation = std::string(status.message())};
      }
      return {.matched = true,
              .explanation = absl::StrCat("which was skipped by the testee: ",
                                          response.skipped())};
    }

    testing::StringMatchResultListener inner_listener;
    bool matched = inner_.MatchAndExplain(result, &inner_listener);
    std::string message = inner_listener.str();

    // A response without a usable result is a failure no matter what the
    // inner matcher says.  The failure message stays the inner matcher's,
    // since the legacy runner's message depends on the kind of test (e.g.
    // "Should have failed to parse, but raised an error instead."); the
    // generic legacy message is only used when the inner matcher has none.
    if (absl::optional<absl::string_view> forced =
            ForcedFailureMessage(response);
        forced.has_value()) {
      if (matched || message.empty()) {
        message = std::string(*forced);
      }
      matched = false;
    }

    if (matched) {
      absl::Status status = manager.ReportSuccess(name);
      if (!status.ok()) {
        return {.matched = false, .explanation = std::string(status.message())};
      }
      return {.matched = true, .explanation = std::move(message)};
    }

    if (message.empty()) {
      message = absl::StrCat(
          "which doesn't match (",
          testing::DescribeMatcher<const TestResult&>(inner_), ")");
    }

    // Failures of recommended tests are only tolerated when they aren't
    // already being tracked in the failure list.  Otherwise the failure list
    // would become stale without anybody noticing.
    if (!listed && result.strictness() == TestStrictness::kRecommended &&
        !manager.enforce_recommended()) {
      manager.ReportRecommendedFailure(name);
      ABSL_LOG(WARNING) << "WARNING, test=" << name << ": "
                        << absl::StripTrailingAsciiWhitespace(message);
      return {.matched = true,
              .explanation = absl::StrCat(
                  "which failed, but is only recommended: ", message)};
    }

    absl::Status status = manager.ReportFailure(name, message);
    if (status.ok()) {
      // This failure was expected.
      ABSL_LOG(INFO) << "Ignoring expected failure for test " << name << ": "
                     << absl::StripTrailingAsciiWhitespace(message);
      return {
          .matched = true,
          .explanation = absl::StrCat("which failed as expected: ", message)};
    }
    return {.matched = false,
            .explanation = absl::StrCat(message, "\n", status.message())};
  }

  testing::Matcher<const TestResult&> inner_;
};

// Formats `fields` for descriptions, e.g. `[666: "abc", 666: 123]`.
std::string DescribeUnknownFields(const UnknownFieldSet& fields) {
  std::string out = "[";
  for (int i = 0; i < fields.field_count(); ++i) {
    const UnknownField& field = fields.field(i);
    if (i > 0) absl::StrAppend(&out, ", ");
    absl::StrAppend(&out, field.number(), ": ");
    switch (field.type()) {
      case UnknownField::TYPE_VARINT:
        absl::StrAppend(&out, field.varint());
        break;
      case UnknownField::TYPE_FIXED32:
        absl::StrAppend(&out, "fixed32(", field.fixed32(), ")");
        break;
      case UnknownField::TYPE_FIXED64:
        absl::StrAppend(&out, "fixed64(", field.fixed64(), ")");
        break;
      case UnknownField::TYPE_LENGTH_DELIMITED:
        absl::StrAppend(&out, "\"", absl::CEscape(field.length_delimited()),
                        "\"");
        break;
      case UnknownField::TYPE_GROUP:
        absl::StrAppend(&out, DescribeUnknownFields(field.group()));
        break;
    }
  }
  return absl::StrCat(out, "]");
}

}  // namespace

bool PayloadMatcher::MatchAndExplain(
    const TestResult& result, testing::MatchResultListener* listener) const {
  const ConformanceResponse& response = result.response();
  switch (response.result_case()) {
    case ConformanceResponse::RESULT_NOT_SET:
      *listener << kNoResultMessage;
      return false;

    case ConformanceResponse::kParseError:
    case ConformanceResponse::kTimeoutError:
    case ConformanceResponse::kRuntimeError:
    case ConformanceResponse::kSerializeError:
      *listener << kErrorResultMessage;
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

bool ParsedPayloadMatcher::MatchPayload(
    const TestResult& result, testing::MatchResultListener* listener) const {
  std::string failure_message;
  std::unique_ptr<Message> actual = ParsePayload(
      result, type_override_ != nullptr ? type_override_ : result.type(),
      &failure_message);
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
    *listener << "Expect: parsed payload "
              << testing::DescribeMatcher<const Message&>(matcher_)
              << ", but got: {" << ToShortString(*actual) << "}";
  } else {
    *listener << inner_listener.str();
  }
  return false;
}

void ParsedPayloadMatcher::DescribeInnerTo(std::ostream* os,
                                           bool negation) const {
  *os << "parsed payload ";
  if (type_override_ != nullptr) {
    *os << "(as " << type_override_->full_name() << ") ";
  }
  *os << testing::DescribeMatcher<const Message&>(matcher_, negation);
}

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
  testing::StringMatchResultListener inner_listener;
  if (matcher_.MatchAndExplain(actual, &inner_listener)) {
    *listener << inner_listener.str();
    return true;
  }
  if (inner_listener.str().empty()) {
    // Binary payloads are shown in octal like the legacy runner does; readable
    // formats are merely escaped.
    *listener << "Expect: payload "
              << testing::DescribeMatcher<absl::string_view>(matcher_)
              << ", but got: "
              << (result.format() == ::conformance::PROTOBUF
                      ? ToOctString(actual)
                      : absl::StrCat("\"", absl::CEscape(actual), "\""));
  } else {
    *listener << inner_listener.str();
  }
  return false;
}

void RawPayloadMatcher::DescribeInnerTo(std::ostream* os, bool negation) const {
  *os << "payload "
      << testing::DescribeMatcher<absl::string_view>(matcher_, negation);
}

bool JsonPayloadMatcher::MatchPayload(
    const TestResult& result, testing::MatchResultListener* listener) const {
  // PayloadMatcher only checked that the payload is in the requested format;
  // a test that asked for anything but JSON can't be matched this way.  This
  // is an authoring bug, so it is deliberately checked only after the base
  // class has reported the "real" conformance failures (errors, skips, wrong
  // payload kind) that also apply to correctly written tests.
  if (result.format() != ::conformance::JSON) {
    *listener << "JsonPayload() needs JSON output, but the test asked for "
              << WireFormat_Name(result.format()) << " output.";
    return false;
  }

  // Parsed with jsoncpp's defaults, like the legacy runner's validators.
  const std::string& payload = result.response().json_payload();
  Json::CharReaderBuilder builder;
  const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
  Json::Value value;
  std::string error;
  if (!reader->parse(payload.data(), payload.data() + payload.size(), &value,
                     &error)) {
    *listener << "JSON payload cannot be parsed as valid JSON: " << error;
    return false;
  }

  testing::StringMatchResultListener inner_listener;
  if (matcher_.MatchAndExplain(value, &inner_listener)) {
    *listener << inner_listener.str();
    return true;
  }
  if (inner_listener.str().empty()) {
    // Quoted and escaped like Payload() does for readable formats.
    *listener << "Expect: JSON payload "
              << testing::DescribeMatcher<const Json::Value&>(matcher_)
              << ", but got: \"" << absl::CEscape(payload) << "\"";
  } else {
    *listener << inner_listener.str();
  }
  return false;
}

void JsonPayloadMatcher::DescribeInnerTo(std::ostream* os,
                                         bool negation) const {
  *os << "JSON payload "
      << testing::DescribeMatcher<const Json::Value&>(matcher_, negation);
}

testing::Matcher<absl::string_view> EqualsBytes(absl::string_view expected) {
  return EqualsBytesMatcher(expected);
}

EquivalentMessageMatcher::EquivalentMessageMatcher(WireFormat format,
                                                   std::string expected)
    : format_(format), expected_(std::move(expected)) {
  ABSL_CHECK(format == ::conformance::PROTOBUF ||
             format == ::conformance::TEXT_FORMAT)
      << "Unsupported expected message format " << WireFormat_Name(format);
}

bool EquivalentMessageMatcher::MatchAndExplain(
    const Message& actual, testing::MatchResultListener* listener) const {
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
  *listener << kNotEquivalentPrefix << differences;
  return false;
}

void EquivalentMessageMatcher::DescribeTo(std::ostream* os) const {
  *os << "equals "
      << (format_ == ::conformance::PROTOBUF ? "binary proto" : "text proto")
      << " \"" << absl::CEscape(expected_) << "\"";
}

void EquivalentMessageMatcher::DescribeNegationTo(std::ostream* os) const {
  *os << "doesn't equal "
      << (format_ == ::conformance::PROTOBUF ? "binary proto" : "text proto")
      << " \"" << absl::CEscape(expected_) << "\"";
}

UnknownFieldsInOrderMatcher::UnknownFieldsInOrderMatcher(
    const UnknownFieldSet& expected)
    : description_(DescribeUnknownFields(expected)) {
  if (!expected.SerializeToString(&expected_)) {
    ABSL_LOG(FATAL) << "Failed to serialize the expected unknown fields "
                    << description_;
  }
}

bool UnknownFieldsInOrderMatcher::MatchAndExplain(
    const Message& actual, testing::MatchResultListener* listener) const {
  std::string serialized;
  if (actual.GetReflection()->GetUnknownFields(actual).SerializeToString(
          &serialized) &&
      serialized == expected_) {
    return true;
  }
  *listener << "Unknown field mismatch";
  return false;
}

void UnknownFieldsInOrderMatcher::DescribeTo(std::ostream* os) const {
  *os << "has exactly the unknown fields " << description_ << ", in that order";
}

void UnknownFieldsInOrderMatcher::DescribeNegationTo(std::ostream* os) const {
  *os << "doesn't have exactly the unknown fields " << description_
      << " in that order";
}

bool FailureMatcher::MatchAndExplain(
    const TestResult& result, testing::MatchResultListener* listener) const {
  ConformanceResponse::ResultCase actual = result.response().result_case();
  if (actual == expected_result_) {
    return true;
  }
  if (actual == ConformanceResponse::kSkipped) {
    *listener << "the testee skipped the test: " << result.response().skipped();
    return false;
  }
  if (actual == ConformanceResponse::kRuntimeError &&
      result.request().payload_case() == ConformanceRequest::kProtobufPayload &&
      !binary_input_runtime_error_failure_message_.empty()) {
    *listener << binary_input_runtime_error_failure_message_;
    return false;
  }
  *listener << failure_message_;
  return false;
}

testing::Matcher<const TestResult&> MakeYieldsMatcher(
    testing::Matcher<const TestResult&> inner) {
  return testing::MakeMatcher(new YieldsMatcherImpl(std::move(inner)));
}

}  // namespace internal

internal::EquivalentMessageMatcher EqualsTextProto(absl::string_view text) {
  return internal::EquivalentMessageMatcher(::conformance::TEXT_FORMAT,
                                            std::string(text));
}

internal::UnknownFieldsInOrderMatcher HasUnknownFieldsInOrder(
    const UnknownFieldSet& expected) {
  return internal::UnknownFieldsInOrderMatcher(expected);
}

internal::FailureMatcher IsParseError() {
  return internal::FailureMatcher(
      "parse error", ::conformance::ConformanceResponse::kParseError,
      "Should have failed to parse, but didn't.",
      "Should have failed to parse, but raised an error instead.");
}

internal::FailureMatcher IsSerializeError() {
  return internal::FailureMatcher(
      "serialize error", ::conformance::ConformanceResponse::kSerializeError,
      "Should have failed to serialize, but didn't.");
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google
