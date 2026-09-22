// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/testee.h"

#include <cstddef>
#include <exception>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include "absl/base/no_destructor.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/memory/memory.h"
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "conformance/binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "conformance/conformance_result.pb.h"
#include "conformance/naming.h"
#include "conformance/taxonomy.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/message.h"
#include "google/protobuf/text_format.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {
namespace {

using ::conformance::ConformanceRequest;
using ::conformance::ConformanceResponse;

// The format of a v1 request's input payload, or UNSPECIFIED if it has none.
// Requests built by Test::Parse*() always have one; only a TestResult built by
// ForTesting() can be missing it, and it must still be constructible.
::conformance::WireFormat GetInputFormat(
    const ::conformance::ConformanceRequest& request) {
  switch (request.payload_case()) {
    case ::conformance::ConformanceRequest::kProtobufPayload:
      return ::conformance::PROTOBUF;
    case ::conformance::ConformanceRequest::kJsonPayload:
      return ::conformance::JSON;
    case ::conformance::ConformanceRequest::kTextPayload:
      return ::conformance::TEXT_FORMAT;
    case ::conformance::ConformanceRequest::kJspbPayload:
      return ::conformance::JSPB;
    case ::conformance::ConformanceRequest::PAYLOAD_NOT_SET:
      return ::conformance::UNSPECIFIED;
  }
  ABSL_LOG(FATAL) << "Unsupported input format";
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
  std::string full_name = absl::StrCat(PriorityLevelName(priority), ".",
                                       name.suite, ".", name.test);
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

// The gtest test's own name, "<Suite>.<Test>[/<Params>][.<Suffix>]", the body
// of the full name.
std::string GetGtestName(const TestName& name) {
  std::string gtest_name = absl::StrCat(name.suite, ".", name.test);
  if (!name.params.empty()) absl::StrAppend(&gtest_name, "/", name.params);
  if (!name.suffix.empty()) absl::StrAppend(&gtest_name, ".", name.suffix);
  return gtest_name;
}

// The "_"-separated components of a test's parameters, e.g. {"Proto3",
// "INT32"} for "Proto3_INT32"; empty for an unparameterized test.
std::vector<absl::string_view> ParamComponents(const TestName& name) {
  if (name.params.empty()) return {};
  return absl::StrSplit(name.params, '_');
}

// The suite's contribution to a test case name: the suite without its "Test"
// suffix, in snake_case ("PrematureEofTest" -> "premature_eof"), or the suite
// as is if that would leave nothing.
std::string SuiteToken(absl::string_view suite) {
  absl::string_view stem = suite;
  if (absl::ConsumeSuffix(&stem, "Test") && stem.empty()) stem = suite;
  return ToSnakeCase(stem);
}

// Builds the test case name (see ResolveTestInfo() in testee.h):
//
//   "eof_before_known_non_repeated_value_double_parsefails"
//
// Note this deliberately carries no syntax and no payload format: those are
// the variant, and the same test case name is shared by every variant of the
// same logical test.  The suite is part of the name because several suites of
// one section have tests of the same name (e.g. TextFloatTest.Max and
// TextDoubleTest.Max); it is left out when it would only repeat the
// subsection or the start of the test's name (JsonHelloWorldTest.HelloWorld
// in the hello_world subsection is "hello_world_hello_world", not
// "hello_world_json_hello_world_hello_world").
std::string MakeTestCaseName(const TestName& name, absl::string_view subsection,
                             bool has_syntax_component, NameStyle name_style) {
  std::vector<std::string> tokens;
  if (!subsection.empty()) tokens.push_back(std::string(subsection));
  const std::string suite = SuiteToken(name.suite);
  const std::string test = ToSnakeCase(name.test);
  const bool suite_repeats_subsection =
      !subsection.empty() &&
      (suite == subsection ||
       absl::EndsWith(suite, absl::StrCat("_", subsection)));
  if (!suite_repeats_subsection && !absl::StartsWith(test, suite)) {
    tokens.push_back(suite);
  }
  tokens.push_back(test);
  std::vector<absl::string_view> params = ParamComponents(name);
  for (size_t i = has_syntax_component ? 1 : 0; i < params.size(); ++i) {
    std::string snake = ParamToSnakeCase(params[i]);
    if (!snake.empty()) tokens.push_back(std::move(snake));
  }
  if (!name.suffix.empty()) tokens.push_back(ToSnakeCase(name.suffix));
  if (name_style == NameStyle::kWithoutOutputFormat) {
    tokens.push_back("parsefails");
  }
  return absl::StrJoin(tokens, "_");
}

// A human readable description of a test:
// "Verify wire varint (eof) [EditionsProto2, pb2pb]:
//  PrematureEofTest.BeforeKnownNonRepeatedValue/EditionsProto2_DOUBLE (input
//  must be rejected)".
std::string GenerateDescription(const TestName& name,
                                absl::string_view syntax_component,
                                absl::string_view payloads,
                                NameStyle name_style, absl::string_view domain,
                                absl::string_view section,
                                absl::string_view subsection) {
  std::string qualifier(syntax_component);
  if (!payloads.empty()) {
    absl::StrAppend(&qualifier, qualifier.empty() ? "" : ", ", payloads);
  }
  return absl::StrCat(
      "Verify ", domain, " ", section,
      (subsection.empty() ? "" : absl::StrCat(" (", subsection, ")")),
      (qualifier.empty() ? "" : absl::StrCat(" [", qualifier, "]")), ": ",
      GetGtestName(name),
      name_style == NameStyle::kWithoutOutputFormat
          ? " (input must be rejected)"
          : "");
}

// Truncates a payload for debug output, exactly like the legacy runner.
void TruncateDebugPayload(std::string* payload) {
  constexpr size_t kMaxDebugPayloadSize = 200;
  if (payload->size() > kMaxDebugPayloadSize) {
    payload->resize(kMaxDebugPayloadSize);
    payload->append("...(truncated)");
  }
}

// Returns a copy of `request` whose payload is truncated for debug output.
ConformanceRequest TruncateRequest(const ConformanceRequest& request) {
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
      break;
  }
  return debug_request;
}

// Returns a copy of `response` whose payload is truncated for debug output.
ConformanceResponse TruncateResponse(const ConformanceResponse& response) {
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
      break;
  }
  return debug_response;
}

}  // namespace

std::unique_ptr<Message> NewMessage(const Descriptor* type) {
  // The factory delegates generated types to the generated factory, so the
  // returned message is a generated one whenever possible.  It is never
  // destroyed (go/totw/188) so that messages it created stay valid until the
  // process exits.
  struct SharedFactory {
    SharedFactory() { factory.SetDelegateToGeneratedFactory(true); }
    DynamicMessageFactory factory;
  };
  static absl::NoDestructor<SharedFactory> shared;
  return absl::WrapUnique(shared->factory.GetPrototype(type)->New());
}

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

ConformanceTestInfo ResolveTestInfo(const TestName& name, TestPriority priority,
                                    const ConformanceRequest& request,
                                    NameStyle name_style,
                                    TestNumberer& numberer) {
  ConformanceTestInfo info;
  info.priority = priority;
  info.legacy_test_name = GetTestName(name, priority, request, name_style);

  // The parameter components: an optional syntax first, then (for the suites
  // parameterized by field type) the field type.
  std::vector<absl::string_view> params = ParamComponents(name);
  absl::string_view syntax_component;
  if (!params.empty() && !SyntaxTag(params[0]).empty()) {
    syntax_component = params[0];
  }
  const size_t first_non_syntax = syntax_component.empty() ? 0 : 1;
  const absl::string_view field_type_component =
      params.size() > first_non_syntax ? params[first_non_syntax] : "";

  // Parts 1 and 2: the section, from the suite (and test and field type).
  int domain_num = 0;
  int section_num = 0;
  if (const SectionDescriptor* section =
          SectionOf(name.suite, name.test, field_type_component);
      section != nullptr) {
    domain_num = section->domain_num;
    section_num = section->section_num;
    info.domain = std::string(section->domain);
    info.section = std::string(section->section);
    info.subsection = std::string(section->subsection);
    info.is_informational = section->is_informational;
  } else {
    info.domain = "unmapped";
    info.section = SuiteToken(name.suite);
  }
  info.section_coordinate =
      absl::StrFormat("%02d.%02d", domain_num, section_num);

  // Part 4 before part 3, since the test case name depends on whether the
  // parameters start with a syntax: the variant, as <syntax digit><payload
  // digit>.
  info.syntax = std::string(SyntaxTag(syntax_component));
  info.payloads = PayloadTag(request);
  info.variant_number =
      internal::SyntaxDigit(info.syntax) * 10 + internal::PayloadDigit(request);
  if (info.syntax.empty()) {
    info.variant = info.payloads;
  } else if (info.payloads.empty()) {
    info.variant = info.syntax;
  } else {
    info.variant = absl::StrCat(info.syntax, "_", info.payloads);
  }

  // Part 3: the test case.  Its number is memoized on the name, so every
  // variant of the same logical test resolves to the same number.
  info.test_case = MakeTestCaseName(name, info.subsection,
                                    !syntax_component.empty(), name_style);
  info.test_number =
      numberer.NumberFor(info.section_coordinate, info.test_case);

  info.coordinate = absl::StrFormat("%s.%03d.%02d", info.section_coordinate,
                                    info.test_number, info.variant_number);
  info.test_name = absl::StrCat(info.domain, ".", info.section, ".",
                                info.test_case, ".", info.variant);
  info.description =
      GenerateDescription(name, syntax_component, info.payloads, name_style,
                          info.domain, info.section, info.subsection);
  return info;
}

ConformanceTestInfo TestResult::UnmappedTestInfo(absl::string_view test_name,
                                                 TestPriority priority) {
  ConformanceTestInfo info;
  info.test_name = std::string(test_name);
  info.legacy_test_name = std::string(test_name);
  info.coordinate = "00.00.000.00";
  info.section_coordinate = "00.00";
  info.domain = "unmapped";
  info.section = "unknown";
  info.description = std::string(test_name);
  info.priority = priority;
  return info;
}

TestResult::TestResult(ConformanceTestInfo info, const Descriptor* type,
                       ::conformance::ConformanceRequest request,
                       ::conformance::ConformanceResponse response)
    : info_(std::move(info)),
      type_(type),
      request_(std::move(request)),
      response_(std::move(response)),
      input_format_(GetInputFormat(request_)),
      print_unknown_fields_(request_.print_unknown_fields()) {
  ABSL_CHECK(type_ != nullptr)
      << "TestResult for " << info_.legacy_test_name << " has no message type";

  // Lower the v1 response, whose `result` oneof mixes the error kinds with one
  // payload field per output format, into the protocol-neutral view.  A
  // payload's format comes from the oneof case, not from the requested output
  // format, so that a testee answering in the wrong format is reported as
  // such rather than parsed as something it isn't.
  switch (response_.result_case()) {
    case ConformanceResponse::RESULT_NOT_SET:
      outcome_ = Outcome::kNoResult;
      break;
    case ConformanceResponse::kParseError:
      outcome_ = Outcome::kParseError;
      message_ = response_.parse_error();
      break;
    case ConformanceResponse::kSerializeError:
      outcome_ = Outcome::kSerializeError;
      message_ = response_.serialize_error();
      break;
    case ConformanceResponse::kRuntimeError:
      outcome_ = Outcome::kRuntimeError;
      message_ = response_.runtime_error();
      break;
    case ConformanceResponse::kTimeoutError:
      outcome_ = Outcome::kTimeoutError;
      message_ = response_.timeout_error();
      break;
    case ConformanceResponse::kSkipped:
      outcome_ = Outcome::kSkipped;
      message_ = response_.skipped();
      break;
    case ConformanceResponse::kProtobufPayload:
      outcome_ = Outcome::kOutput;
      outputs_.push_back(
          {::conformance::PROTOBUF, response_.protobuf_payload()});
      break;
    case ConformanceResponse::kJsonPayload:
      outcome_ = Outcome::kOutput;
      outputs_.push_back({::conformance::JSON, response_.json_payload()});
      break;
    case ConformanceResponse::kTextPayload:
      outcome_ = Outcome::kOutput;
      outputs_.push_back(
          {::conformance::TEXT_FORMAT, response_.text_payload()});
      break;
    case ConformanceResponse::kJspbPayload:
      outcome_ = Outcome::kOutput;
      outputs_.push_back({::conformance::JSPB, response_.jspb_payload()});
      break;
  }
}

TestResult::TestResult(TestResult&& other) noexcept
    : info_(std::move(other.info_)),
      type_(other.type_),
      request_(std::move(other.request_)),
      response_(std::move(other.response_)),
      outcome_(other.outcome_),
      message_(std::move(other.message_)),
      outputs_(std::move(other.outputs_)),
      input_format_(other.input_format_),
      print_unknown_fields_(other.print_unknown_fields_),
      checked_(other.checked_),
      verdict_(std::move(other.verdict_)),
      moved_from_(other.moved_from_) {
  other.moved_from_ = true;
}

TestResult& TestResult::operator=(TestResult&& other) noexcept {
  if (this != &other) {
    ReportIfUnchecked();
    info_ = std::move(other.info_);
    type_ = other.type_;
    request_ = std::move(other.request_);
    response_ = std::move(other.response_);
    outcome_ = other.outcome_;
    message_ = std::move(other.message_);
    outputs_ = std::move(other.outputs_);
    input_format_ = other.input_format_;
    print_unknown_fields_ = other.print_unknown_fields_;
    checked_ = other.checked_;
    verdict_ = std::move(other.verdict_);
    moved_from_ = other.moved_from_;
    other.moved_from_ = true;
  }
  return *this;
}

TestResult::~TestResult() { ReportIfUnchecked(); }

void TestResult::SetVerdict(Verdict verdict) const {
  // A moved-from result holds an empty request and response; checking it would
  // report a bogus outcome under a real test's name.
  ABSL_DCHECK(!moved_from_) << "Checking a moved-from TestResult";
  ABSL_DCHECK(!checked_) << "TestResult for " << info_.legacy_test_name
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
  ADD_FAILURE() << "TestResult for " << info_.legacy_test_name
                << " was never checked; wrap the matcher in Yields()";
}

void PrintTo(const TestResult& result, std::ostream* os) {
  *os << PriorityLevelName(result.priority()) << " test \"" << result.name()
      << "\" with request {" << ToShortString(TruncateRequest(result.request()))
      << "} and response {"
      << ToShortString(TruncateResponse(result.response())) << "}";

  // Binary payloads are opaque, so also show what they decode to.
  const TestResult::Output* output = result.output();
  if (output != nullptr && output->format == ::conformance::PROTOBUF) {
    std::unique_ptr<Message> decoded = NewMessage(result.type());
    if (decoded->ParseFromString(output->payload)) {
      *os << " (decoded: {" << ToShortString(*decoded) << "})";
    } else {
      *os << " (unparseable)";
    }
  }
}

void PrintTo(TestResult::Outcome outcome, std::ostream* os) {
  switch (outcome) {
    case TestResult::Outcome::kNoResult:
      *os << "kNoResult";
      return;
    case TestResult::Outcome::kParseError:
      *os << "kParseError";
      return;
    case TestResult::Outcome::kSerializeError:
      *os << "kSerializeError";
      return;
    case TestResult::Outcome::kRuntimeError:
      *os << "kRuntimeError";
      return;
    case TestResult::Outcome::kTimeoutError:
      *os << "kTimeoutError";
      return;
    case TestResult::Outcome::kSkipped:
      *os << "kSkipped";
      return;
    case TestResult::Outcome::kOutput:
      *os << "kOutput";
      return;
  }
  *os << "Outcome(" << static_cast<int>(outcome) << ")";
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

InMemoryMessage&& InMemoryMessage::MergeBinary(Wire input) && {
  CheckCanMerge();
  request_.set_merge_protobuf_payload(std::move(input).data());
  return std::move(*this);
}

InMemoryMessage&& InMemoryMessage::MergeText(absl::string_view input) && {
  CheckCanMerge();
  request_.set_merge_text_payload(input);
  return std::move(*this);
}

InMemoryMessage&& InMemoryMessage::MergeJson(absl::string_view input) && {
  CheckCanMerge();
  request_.set_merge_json_payload(input);
  return std::move(*this);
}

void InMemoryMessage::CheckCanMerge() const {
  ABSL_DCHECK_EQ(request_.merge_payload_case(),
                 ::conformance::ConformanceRequest::MERGE_PAYLOAD_NOT_SET)
      << "a test merges at most one payload";
  ABSL_DCHECK(!request_.discard_unknown_fields())
      << "Merge*() must come before DiscardUnknownFields(): the testee merges "
         "before it discards unknown fields";
}

InMemoryMessage&& InMemoryMessage::DiscardUnknownFields() && {
  request_.set_discard_unknown_fields(true);
  return std::move(*this);
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

  ConformanceTestInfo info = ResolveTestInfo(name_, priority_, request_,
                                             name_style, testee_->numberer_);

  ::conformance::ConformanceResponse response =
      testee_->Run(info.legacy_test_name, request_);

  return TestResult(std::move(info), type_, std::move(request_),
                    std::move(response));
}

}  // namespace internal

absl::string_view PriorityName(TestPriority priority) {
  switch (priority) {
    case TestPriority::kP0:
      return "P0";
    case TestPriority::kP1:
      return "P1";
    case TestPriority::kP2:
      return "P2";
    case TestPriority::kP3:
      return "P3";
  }
  return "Unknown";
}

absl::string_view PriorityLevelName(TestPriority priority) {
  // TODO: b/564550230 - return PriorityName() once the tests are renamed.
  return priority == TestPriority::kP3 ? "Recommended" : "Required";
}

::conformance::ConformanceTestCaseResult ToCaseResult(
    const ConformanceTestInfo& info) {
  ::conformance::ConformanceTestCaseResult result;
  result.set_test_name(info.test_name);
  result.set_legacy_test_name(info.legacy_test_name);
  result.set_coordinate(info.coordinate);
  result.set_section_coordinate(info.section_coordinate);
  result.set_domain(info.domain);
  result.set_section(info.section);
  result.set_subsection(info.subsection);
  result.set_test_number(info.test_number);
  result.set_test_case(info.test_case);
  result.set_variant_number(info.variant_number);
  result.set_variant(info.variant);
  result.set_syntax(info.syntax);
  result.set_payloads(info.payloads);
  result.set_description(info.description);
  result.set_is_informational(info.is_informational);
  result.set_priority(std::string(PriorityName(info.priority)));
  return result;
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google
