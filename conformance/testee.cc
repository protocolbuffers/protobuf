// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/testee.h"

#include <algorithm>
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
#include "absl/status/status.h"
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "absl/types/span.h"
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

using ::conformance::ConformanceAction;
using ::conformance::ConformanceRequest;
using ::conformance::ConformanceResponse;
using ::conformance::ParseAction;
using ::conformance::SerializeAction;
using ::conformance::SerializeResult;
using ::conformance::WireFormat;

// The protocol version of a request: 1 unless it says otherwise (see
// ConformanceRequest.protocol_version).  Anything at or below 1 (unset, an
// explicit 1, or a nonsensical negative value) is the flat request, which is
// also how the C++ harness reads it.
int ProtocolVersionOf(const ConformanceRequest& request) {
  return request.protocol_version() <= 1 ? 1 : request.protocol_version();
}

// The format of a v1 request's input payload, or UNSPECIFIED if it has none.
// Requests built by Test::Parse*() always have one; only a TestResult built by
// ForTesting() can be missing it, and it must still be constructible.
WireFormat GetV1InputFormat(const ConformanceRequest& request) {
  switch (request.payload_case()) {
    case ConformanceRequest::kProtobufPayload:
      return ::conformance::PROTOBUF;
    case ConformanceRequest::kJsonPayload:
      return ::conformance::JSON;
    case ConformanceRequest::kTextPayload:
      return ::conformance::TEXT_FORMAT;
    case ConformanceRequest::kJspbPayload:
      return ::conformance::JSPB;
    case ConformanceRequest::PAYLOAD_NOT_SET:
      return ::conformance::UNSPECIFIED;
  }
  ABSL_LOG(FATAL) << "Unsupported input format";
  return ::conformance::UNSPECIFIED;
}

// The format of a ParseAction's payload, or UNSPECIFIED if it has none.
WireFormat GetPayloadFormat(const ParseAction& parse) {
  switch (parse.payload_case()) {
    case ParseAction::kBinary:
      return ::conformance::PROTOBUF;
    case ParseAction::kJson:
      return ::conformance::JSON;
    case ParseAction::kText:
      return ::conformance::TEXT_FORMAT;
    case ParseAction::PAYLOAD_NOT_SET:
      return ::conformance::UNSPECIFIED;
  }
  ABSL_LOG(FATAL) << "Unsupported payload format";
  return ::conformance::UNSPECIFIED;
}

// The format a SerializeAction asks for, or UNSPECIFIED if it has none.
WireFormat GetSerializeFormat(const SerializeAction& serialize) {
  switch (serialize.format_case()) {
    case SerializeAction::kBinary:
      return ::conformance::PROTOBUF;
    case SerializeAction::kJson:
      return ::conformance::JSON;
    case SerializeAction::kText:
      return ::conformance::TEXT_FORMAT;
    case SerializeAction::FORMAT_NOT_SET:
      return ::conformance::UNSPECIFIED;
  }
  ABSL_LOG(FATAL) << "Unsupported output format";
  return ::conformance::UNSPECIFIED;
}

// The format of a SerializeResult's payload, or UNSPECIFIED if it has none.
WireFormat GetResultFormat(const SerializeResult& result) {
  switch (result.payload_case()) {
    case SerializeResult::kBinary:
      return ::conformance::PROTOBUF;
    case SerializeResult::kJson:
      return ::conformance::JSON;
    case SerializeResult::kText:
      return ::conformance::TEXT_FORMAT;
    case SerializeResult::PAYLOAD_NOT_SET:
      return ::conformance::UNSPECIFIED;
  }
  ABSL_LOG(FATAL) << "Unsupported payload format";
  return ::conformance::UNSPECIFIED;
}

// The payload of a SerializeResult, whatever its format; empty if it has none.
const std::string& GetResultPayload(const SerializeResult& result) {
  switch (result.payload_case()) {
    case SerializeResult::kBinary:
      return result.binary();
    case SerializeResult::kJson:
      return result.json();
    case SerializeResult::kText:
      return result.text();
    case SerializeResult::PAYLOAD_NOT_SET:
      break;
  }
  static const absl::NoDestructor<std::string> empty;
  return *empty;
}

// A SerializeAction asking for `format`, which must be one of the formats the
// test API can ask for.
SerializeAction SerializeIn(WireFormat format) {
  SerializeAction serialize;
  switch (format) {
    case ::conformance::PROTOBUF:
      serialize.mutable_binary();
      break;
    case ::conformance::JSON:
      serialize.mutable_json();
      break;
    case ::conformance::TEXT_FORMAT:
      serialize.mutable_text();
      break;
    default:
      ABSL_LOG(FATAL) << "Tests can only ask for PROTOBUF, JSON or TEXT_FORMAT "
                         "output, not "
                      << WireFormat_Name(format);
  }
  return serialize;
}

// The format of the first input `actions` (a vector or repeated field of
// ConformanceAction) parse, i.e. the one the test's name is derived from;
// UNSPECIFIED if they parse nothing.
template <typename Actions>
WireFormat InputFormatOf(const Actions& actions) {
  for (const ConformanceAction& action : actions) {
    if (action.has_parse()) return GetPayloadFormat(action.parse());
  }
  return ::conformance::UNSPECIFIED;
}

// The format of the first input a request asks the testee to parse: the v1
// payload's, or the first ParseAction's of a v2 request.  UNSPECIFIED if there
// is none (a test built from Test::New() alone, or an empty ForTesting()
// request).
WireFormat GetInputFormat(const ConformanceRequest& request) {
  if (ProtocolVersionOf(request) == 1) return GetV1InputFormat(request);
  return InputFormatOf(request.actions());
}

// The terminal SerializeAction of a v2 request, i.e. its first one (Finish()
// appends the terminal one before the extras queued by Also*()), or null if
// it has none.
const SerializeAction* GetTerminalSerialize(const ConformanceRequest& request) {
  for (const ConformanceAction& action : request.actions()) {
    if (action.has_serialize()) return &action.serialize();
  }
  return nullptr;
}

// The output format a request asks for: the requested output format of a v1
// request, or the format of the terminal SerializeAction of a v2 request.
WireFormat GetRequestedOutputFormat(const ConformanceRequest& request) {
  if (ProtocolVersionOf(request) == 1) return request.requested_output_format();
  const SerializeAction* serialize = GetTerminalSerialize(request);
  return serialize == nullptr ? ::conformance::UNSPECIFIED
                              : GetSerializeFormat(*serialize);
}

// Whether a request asks for unknown fields to be printed in its (terminal)
// text output.
bool GetPrintUnknownFields(const ConformanceRequest& request) {
  if (ProtocolVersionOf(request) == 1) return request.print_unknown_fields();
  const SerializeAction* serialize = GetTerminalSerialize(request);
  return serialize != nullptr && serialize->has_text() &&
         serialize->text().print_unknown_fields();
}

// Adds `offset` to every handle an action creates or refers to; for splicing
// the actions of one message into another test (MergeFrom()).
void OffsetHandles(ConformanceAction& action, int offset) {
  switch (action.action_case()) {
    case ConformanceAction::kParse:
      action.mutable_parse()->set_id(action.parse().id() + offset);
      return;
    case ConformanceAction::kNewMessage:
      action.mutable_new_message()->set_id(action.new_message().id() + offset);
      return;
    case ConformanceAction::kMerge:
      action.mutable_merge()->set_from(action.merge().from() + offset);
      action.mutable_merge()->set_to(action.merge().to() + offset);
      return;
    case ConformanceAction::kDiscardUnknownFields:
      action.mutable_discard_unknown_fields()->set_id(
          action.discard_unknown_fields().id() + offset);
      return;
    case ConformanceAction::kSerialize:
      action.mutable_serialize()->set_id(action.serialize().id() + offset);
      return;
    case ConformanceAction::ACTION_NOT_SET:
      return;
  }
  ABSL_LOG(FATAL) << "Unknown action";
}

// Lowers `actions`, which RequiredProtocolVersion() found to be of the v1
// shape, into the flat v1 request, with `test_category_override` in place of
// the category the input's format implies (see OverrideTestCategory()).  The
// fields set here are exactly the ones the v1-only builder used to set, so
// the request is byte-for-byte the one it produced.
ConformanceRequest LowerToV1(
    absl::Span<const ConformanceAction> actions,
    absl::optional<::conformance::TestCategory> test_category_override) {
  ConformanceRequest request;
  const ParseAction& input = actions.front().parse();
  request.set_message_type(input.type());
  switch (input.payload_case()) {
    case ParseAction::kBinary:
      request.set_protobuf_payload(input.binary().data());
      request.set_test_category(::conformance::BINARY_TEST);
      break;
    case ParseAction::kJson:
      request.set_json_payload(input.json().data());
      request.set_test_category(
          input.json().ignore_unknown_fields()
              ? ::conformance::JSON_IGNORE_UNKNOWN_PARSING_TEST
              : ::conformance::JSON_TEST);
      break;
    case ParseAction::kText:
      request.set_text_payload(input.text().data());
      request.set_test_category(::conformance::TEXT_FORMAT_TEST);
      break;
    case ParseAction::PAYLOAD_NOT_SET:
      ABSL_LOG(FATAL) << "A v1 request needs an input payload";
  }
  if (test_category_override.has_value()) {
    request.set_test_category(*test_category_override);
  }

  for (const ConformanceAction& action : actions.subspan(1)) {
    switch (action.action_case()) {
      case ConformanceAction::kParse: {
        // The merge payload; its ignore_unknown_fields is the input's, which
        // the request's single test_category already says.
        const ParseAction& merge = action.parse();
        switch (merge.payload_case()) {
          case ParseAction::kBinary:
            request.set_merge_protobuf_payload(merge.binary().data());
            break;
          case ParseAction::kJson:
            request.set_merge_json_payload(merge.json().data());
            break;
          case ParseAction::kText:
            request.set_merge_text_payload(merge.text().data());
            break;
          case ParseAction::PAYLOAD_NOT_SET:
            ABSL_LOG(FATAL) << "A merge payload needs a payload";
        }
        break;
      }
      case ConformanceAction::kMerge:
        // Implied by the merge payload.
        break;
      case ConformanceAction::kDiscardUnknownFields:
        request.set_discard_unknown_fields(true);
        break;
      case ConformanceAction::kSerialize:
        request.set_requested_output_format(
            GetSerializeFormat(action.serialize()));
        if (action.serialize().has_text() &&
            action.serialize().text().print_unknown_fields()) {
          request.set_print_unknown_fields(true);
        }
        break;
      case ConformanceAction::kNewMessage:
      case ConformanceAction::ACTION_NOT_SET:
        ABSL_LOG(FATAL) << "Not a v1 action: " << ToShortString(action);
    }
  }
  return request;
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

// The <Input> piece of a test name for the given input format: the format's
// identifier, or "Empty" for a test that parses nothing.
absl::string_view GetInputIdentifier(WireFormat input_format) {
  if (input_format == ::conformance::UNSPECIFIED) return "Empty";
  return GetFormatIdentifier(input_format);
}

// Builds the full test name (see TestName in testee.h):
// "<Level>.<Suite>.<Test>[/<Params>][.<Suffix>].<Input>Input", followed by
// ".<Output>Output" unless `name_style` omits it.
std::string GetTestName(const TestName& name, TestPriority priority,
                        WireFormat input_format, WireFormat output_format,
                        NameStyle name_style) {
  std::string full_name = absl::StrCat(PriorityLevelName(priority), ".",
                                       name.suite, ".", name.test);
  if (!name.params.empty()) absl::StrAppend(&full_name, "/", name.params);
  if (!name.suffix.empty()) absl::StrAppend(&full_name, ".", name.suffix);
  absl::StrAppend(&full_name, ".", GetInputIdentifier(input_format), "Input");
  if (name_style == NameStyle::kWithOutputFormat) {
    absl::StrAppend(&full_name, ".", GetFormatIdentifier(output_format),
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

// Returns a copy of `request` whose payloads (the v1 payload, and the payload
// of every v2 ParseAction) are truncated for debug output.
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
  switch (debug_request.merge_payload_case()) {
    case ConformanceRequest::kMergeProtobufPayload:
      TruncateDebugPayload(debug_request.mutable_merge_protobuf_payload());
      break;
    case ConformanceRequest::kMergeJsonPayload:
      TruncateDebugPayload(debug_request.mutable_merge_json_payload());
      break;
    case ConformanceRequest::kMergeTextPayload:
      TruncateDebugPayload(debug_request.mutable_merge_text_payload());
      break;
    default:
      break;
  }
  for (ConformanceAction& action : *debug_request.mutable_actions()) {
    if (!action.has_parse()) continue;
    ParseAction& parse = *action.mutable_parse();
    switch (parse.payload_case()) {
      case ParseAction::kBinary:
        TruncateDebugPayload(parse.mutable_binary()->mutable_data());
        break;
      case ParseAction::kJson:
        TruncateDebugPayload(parse.mutable_json()->mutable_data());
        break;
      case ParseAction::kText:
        TruncateDebugPayload(parse.mutable_text()->mutable_data());
        break;
      case ParseAction::PAYLOAD_NOT_SET:
        break;
    }
  }
  return debug_request;
}

// Returns a copy of `response` whose payloads (the v1 payload, and every v2
// SerializeResult's) are truncated for debug output.
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
    case ConformanceResponse::kResults:
      for (SerializeResult& result :
           *debug_response.mutable_results()->mutable_serialized()) {
        switch (result.payload_case()) {
          case SerializeResult::kBinary:
            TruncateDebugPayload(result.mutable_binary());
            break;
          case SerializeResult::kJson:
            TruncateDebugPayload(result.mutable_json());
            break;
          case SerializeResult::kText:
            TruncateDebugPayload(result.mutable_text());
            break;
          case SerializeResult::PAYLOAD_NOT_SET:
            break;
        }
      }
      break;
    default:
      break;
  }
  return debug_response;
}

}  // namespace

int RequiredProtocolVersion(absl::Span<const ConformanceAction> actions) {
  // Version 1 can express exactly one shape, the flat request's:
  //
  //   Parse(0) [Parse(1) Merge(1 -> 0)] [DiscardUnknownFields(0)] Serialize(0)
  //
  // with the merge payload of the input's type (the request has a single
  // message_type) and a JSON merge payload lenient about unknown fields iff
  // the input is lenient JSON (the request has a single test_category).
  // Anything else needs version 2.
  auto at = [&](size_t i) -> const ConformanceAction* {
    return i < actions.size() ? &actions[i] : nullptr;
  };
  if (at(0) == nullptr || !at(0)->has_parse() || at(0)->parse().id() != 0) {
    return 2;
  }
  const ParseAction& input = at(0)->parse();
  size_t i = 1;
  if (at(i) != nullptr && at(i)->has_parse()) {
    const ParseAction& merge = at(i)->parse();
    const bool lenient_input =
        input.has_json() && input.json().ignore_unknown_fields();
    if (merge.id() != 1 || merge.type() != input.type() ||
        (merge.has_json() &&
         merge.json().ignore_unknown_fields() != lenient_input)) {
      return 2;
    }
    ++i;
    if (at(i) == nullptr || !at(i)->has_merge() || at(i)->merge().from() != 1 ||
        at(i)->merge().to() != 0) {
      return 2;
    }
    ++i;
  }
  if (at(i) != nullptr && at(i)->has_discard_unknown_fields()) {
    if (at(i)->discard_unknown_fields().id() != 0) return 2;
    ++i;
  }
  if (at(i) == nullptr || !at(i)->has_serialize() ||
      at(i)->serialize().id() != 0) {
    return 2;
  }
  ++i;
  return i == actions.size() ? 1 : 2;
}

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
                                    WireFormat input_format,
                                    WireFormat output_format,
                                    NameStyle name_style,
                                    TestNumberer& numberer) {
  ConformanceTestInfo info;
  info.priority = priority;
  info.legacy_test_name =
      GetTestName(name, priority, input_format, output_format, name_style);

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
  info.payloads = PayloadTag(input_format, output_format);
  info.variant_number = internal::SyntaxDigit(info.syntax) * 10 +
                        internal::PayloadDigit(input_format, output_format);
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
      response_(std::move(response)) {
  ABSL_CHECK(type_ != nullptr)
      << "TestResult for " << info_.legacy_test_name << " has no message type";
  InitializeFromRequest();

  // Lower the response, whose `result` oneof mixes the error kinds with one
  // payload field per v1 output format and the v2 results, into the
  // protocol-neutral view.  A payload's format comes from the oneof case (or,
  // for v2, from the SerializeResult's payload case), not from the requested
  // output format, so that a testee answering in the wrong format is reported
  // as such rather than parsed as something it isn't.
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
    case ConformanceResponse::kResults: {
      // One output per SerializeAction, in order.  A testee that answers
      // with any other number didn't run the request it was given, which is
      // a runtime error rather than a wrong output (the matchers would only
      // look at the first one).  An output whose payload is missing has no
      // format (UNSPECIFIED), so that it can't pass as an empty message.
      int serialize_actions = 0;
      for (const ConformanceAction& action : request_.actions()) {
        if (action.has_serialize()) ++serialize_actions;
      }
      const int results = response_.results().serialized_size();
      if (results != serialize_actions) {
        outcome_ = Outcome::kRuntimeError;
        message_ = absl::StrCat("testee returned ", results, " outputs for ",
                                serialize_actions, " serialize actions");
        break;
      }
      outcome_ = Outcome::kOutput;
      for (const SerializeResult& result : response_.results().serialized()) {
        outputs_.push_back({GetResultFormat(result), GetResultPayload(result)});
      }
      break;
    }
  }
  if (response_.has_failed_action()) {
    failed_action_ = response_.failed_action();
  }
}

TestResult::TestResult(ConformanceTestInfo info, const Descriptor* type,
                       ::conformance::ConformanceRequest request,
                       UnsupportedProtocol unsupported)
    : info_(std::move(info)), type_(type), request_(std::move(request)) {
  ABSL_CHECK(type_ != nullptr)
      << "TestResult for " << info_.legacy_test_name << " has no message type";
  InitializeFromRequest();
  outcome_ = Outcome::kUnsupportedProtocol;
  message_ = absl::StrCat("requires conformance protocol v",
                          required_protocol_version_, " (the testee speaks v",
                          unsupported.testee_protocol_version, ")");
}

void TestResult::InitializeFromRequest() {
  required_protocol_version_ = ProtocolVersionOf(request_);
  format_ = GetRequestedOutputFormat(request_);
  input_format_ = GetInputFormat(request_);
  print_unknown_fields_ = GetPrintUnknownFields(request_);
}

TestResult::TestResult(TestResult&& other) noexcept
    : info_(std::move(other.info_)),
      type_(other.type_),
      request_(std::move(other.request_)),
      response_(std::move(other.response_)),
      outcome_(other.outcome_),
      message_(std::move(other.message_)),
      outputs_(std::move(other.outputs_)),
      required_protocol_version_(other.required_protocol_version_),
      format_(other.format_),
      input_format_(other.input_format_),
      print_unknown_fields_(other.print_unknown_fields_),
      failed_action_(other.failed_action_),
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
    required_protocol_version_ = other.required_protocol_version_;
    format_ = other.format_;
    input_format_ = other.input_format_;
    print_unknown_fields_ = other.print_unknown_fields_;
    failed_action_ = other.failed_action_;
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

  // The empty response of a test the runner never sent would be misleading
  // on its own.
  if (result.outcome() == TestResult::Outcome::kUnsupportedProtocol) {
    *os << " (not sent: " << result.message() << ")";
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
    case TestResult::Outcome::kUnsupportedProtocol:
      *os << "kUnsupportedProtocol";
      return;
  }
  *os << "Outcome(" << static_cast<int>(outcome) << ")";
}

Testee::Testee(ConformanceTestRunner* runner, int protocol_version)
    : runner_(runner), protocol_version_(protocol_version) {
  ABSL_CHECK(protocol_version >= 1 &&
             protocol_version <= kLatestProtocolVersion)
      << "Unsupported conformance protocol version " << protocol_version
      << ": this runner speaks versions 1 to " << kLatestProtocolVersion;
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

void Testee::RegisterTestName(absl::string_view test_name) {
  ABSL_CHECK(test_names_ran_.emplace(test_name).second)
      << "Duplicated test name: " << test_name;
}

namespace {

// Sends `request` to the testee under `test_name` and returns its response,
// or nullopt if what came back isn't a ConformanceResponse at all.
absl::optional<ConformanceResponse> SendRequest(
    ConformanceTestRunner& runner, absl::string_view test_name,
    const ConformanceRequest& request) {
  std::string serialized_request;
  // TODO: Remove this suppression.
  (void)request.SerializeToString(&serialized_request);

  std::string serialized_response =
      runner.RunTest(test_name, serialized_request);

  ConformanceResponse response;
  if (!response.ParseFromString(serialized_response)) return absl::nullopt;
  return response;
}

// The probe request of the discovery handshake; see
// ConformanceRequest.protocol_version in conformance.proto.  Its version 1
// shape is frozen: it is what a version 1 testee sees.  It is exactly the
// request LowerToV1() builds for
//
//   Test().ParseBinary(TestAllTypesProto3, "").SerializeBinary()
//
// (testee_test.cc pins the two to each other), with the version 2 fields on
// top.  It is sent under kProbeName (test_runner.h).
ConformanceRequest ProbeRequest() {
  constexpr absl::string_view kProbeType =
      "protobuf_test_messages.proto3.TestAllTypesProto3";
  ConformanceRequest probe;
  // The version 1 shape: round-trip an empty message.
  probe.set_message_type(kProbeType);
  probe.set_protobuf_payload("");
  probe.set_requested_output_format(::conformance::PROTOBUF);
  probe.set_test_category(::conformance::BINARY_TEST);
  // The same in version 2.
  probe.set_protocol_version(2);
  ParseAction& parse = *probe.add_actions()->mutable_parse();
  parse.set_type(kProbeType);
  parse.set_id(0);
  parse.mutable_binary()->set_data("");
  SerializeAction& serialize = *probe.add_actions()->mutable_serialize();
  serialize.set_id(0);
  serialize.mutable_binary();
  return probe;
}

}  // namespace

absl::Status Testee::DetectProtocolVersion(int pinned_protocol_version) {
  ABSL_CHECK(pinned_protocol_version == 0 ||
             (pinned_protocol_version >= 1 &&
              pinned_protocol_version <= kLatestProtocolVersion))
      << "Unsupported conformance protocol version " << pinned_protocol_version
      << ": this runner speaks versions 1 to " << kLatestProtocolVersion;
  ABSL_DCHECK(test_names_ran_.empty())
      << "The discovery handshake must precede the first test";

  absl::optional<ConformanceResponse> response =
      SendRequest(*runner_, kProbeName, ProbeRequest());
  if (!response.has_value()) {
    return absl::UnavailableError(
        "could not probe the testee's protocol version: the response could "
        "not be parsed as a ConformanceResponse");
  }
  // A timeout is the runner's word, not the testee's (nothing else sets
  // timeout_error): the testee never answered.  So is the not-selected skip
  // of a filtering runner (see kTestNotSelectedSkipReason): it means the
  // probe never reached the testee, and taking it for a version 1 answer
  // would make a --test run speak a different version than a full one.
  // Every other kind of response is an answer, whether or not the testee
  // understood the probe.
  if (response->has_timeout_error()) {
    return absl::UnavailableError(
        absl::StrCat("could not probe the testee's protocol version: ",
                     response->timeout_error()));
  }
  if (response->has_skipped() &&
      response->skipped() == kTestNotSelectedSkipReason) {
    return absl::UnavailableError(
        "could not probe the testee's protocol version: the test runner "
        "filtered the probe out instead of forwarding it to the testee");
  }

  int detected = response->protocol_version();
  if (detected <= 0) {
    detected = 1;
  } else if (detected > kLatestProtocolVersion) {
    ABSL_LOG(WARNING) << "The testee implements conformance protocol v"
                      << detected << ", newer than this runner's v"
                      << kLatestProtocolVersion << "; speaking v"
                      << kLatestProtocolVersion << " to it";
    detected = kLatestProtocolVersion;
  }

  if (pinned_protocol_version != 0) {
    if (detected < pinned_protocol_version) {
      return absl::FailedPreconditionError(absl::StrCat(
          "--protocol_version=", pinned_protocol_version,
          " pins the testee to protocol v", pinned_protocol_version,
          ", but it answered the probe as v", detected));
    }
    protocol_version_ = pinned_protocol_version;
    ABSL_LOG(INFO) << "The testee answered the discovery handshake as "
                      "conformance protocol v"
                   << detected << "; pinned to v" << protocol_version_;
  } else {
    protocol_version_ = detected;
    ABSL_LOG(INFO) << "The testee answered the discovery handshake as "
                      "conformance protocol v"
                   << protocol_version_;
  }
  return absl::OkStatus();
}

::conformance::ConformanceResponse Testee::Run(
    absl::string_view test_name, const ConformanceRequest& request) {
  RegisterTestName(test_name);
  absl::optional<ConformanceResponse> response =
      SendRequest(*runner_, test_name, request);
  if (!response.has_value()) {
    response.emplace();
    response->set_runtime_error("response proto could not be parsed.");
  }
  return *std::move(response);
}

namespace {

// A ParseAction of `type` for handle 0, the message a Test starts with; the
// caller sets the payload.
ConformanceAction ParseIntoRoot(const Descriptor* type) {
  ConformanceAction action;
  action.mutable_parse()->set_type(type->full_name());
  action.mutable_parse()->set_id(0);
  return action;
}

}  // namespace

InMemoryMessage Test::ParseBinary(const Descriptor* type, Wire input) && {
  ConformanceAction action = ParseIntoRoot(type);
  action.mutable_parse()->mutable_binary()->set_data(std::move(input).data());
  std::vector<ConformanceAction> actions;
  actions.push_back(std::move(action));
  return InMemoryMessage(testee_, std::move(name_), priority_, type,
                         std::move(actions));
}

InMemoryMessage Test::ParseText(const Descriptor* type,
                                absl::string_view input) && {
  ConformanceAction action = ParseIntoRoot(type);
  action.mutable_parse()->mutable_text()->set_data(input);
  std::vector<ConformanceAction> actions;
  actions.push_back(std::move(action));
  return InMemoryMessage(testee_, std::move(name_), priority_, type,
                         std::move(actions));
}

InMemoryMessage Test::ParseJson(const Descriptor* type, absl::string_view input,
                                JsonParseOptions options) && {
  ConformanceAction action = ParseIntoRoot(type);
  action.mutable_parse()->mutable_json()->set_data(input);
  if (options.ignore_unknown_fields) {
    action.mutable_parse()->mutable_json()->set_ignore_unknown_fields(true);
  }
  std::vector<ConformanceAction> actions;
  actions.push_back(std::move(action));
  return InMemoryMessage(testee_, std::move(name_), priority_, type,
                         std::move(actions));
}

InMemoryMessage Test::New(const Descriptor* type) && {
  ConformanceAction action;
  action.mutable_new_message()->set_type(type->full_name());
  action.mutable_new_message()->set_id(0);
  std::vector<ConformanceAction> actions;
  actions.push_back(std::move(action));
  return InMemoryMessage(testee_, std::move(name_), priority_, type,
                         std::move(actions));
}

TestResult InMemoryMessage::SerializeBinary() && {
  return Finish(SerializeIn(::conformance::PROTOBUF),
                NameStyle::kWithOutputFormat);
}

TestResult InMemoryMessage::SerializeText(TextSerializationOptions options) && {
  SerializeAction serialize = SerializeIn(::conformance::TEXT_FORMAT);
  if (options.print_unknown_fields) {
    serialize.mutable_text()->set_print_unknown_fields(true);
  }
  return Finish(std::move(serialize), NameStyle::kWithOutputFormat);
}

TestResult InMemoryMessage::SerializeJson() && {
  return Finish(SerializeIn(::conformance::JSON), NameStyle::kWithOutputFormat);
}

TestResult InMemoryMessage::ParseOnly(ParseOnlyOptions options) && {
  // We don't expect output, but if the testee erroneously accepts the input we
  // let it send its response in the input's format (unless the test says
  // otherwise).  We must not leave it unspecified.  This is exactly what the
  // legacy runner did.
  const WireFormat output_format =
      options.output_format.value_or(InputFormatOf(actions_));
  ABSL_CHECK_NE(output_format, ::conformance::UNSPECIFIED)
      << "ParseOnly() on a test that parses nothing (Test::New()) needs an "
         "explicit ParseOnlyOptions::output_format";
  return Finish(SerializeIn(output_format), NameStyle::kWithoutOutputFormat);
}

InMemoryMessage&& InMemoryMessage::MergeBinary(Wire input) && {
  ParseAction parse;
  parse.mutable_binary()->set_data(std::move(input).data());
  return MergeParsed(std::move(parse));
}

InMemoryMessage&& InMemoryMessage::MergeText(absl::string_view input) && {
  ParseAction parse;
  parse.mutable_text()->set_data(input);
  return MergeParsed(std::move(parse));
}

InMemoryMessage&& InMemoryMessage::MergeJson(absl::string_view input) && {
  ParseAction parse;
  parse.mutable_json()->set_data(input);
  return MergeParsed(std::move(parse));
}

InMemoryMessage&& InMemoryMessage::MergeParsed(ParseAction parse) {
  CheckCanMerge();
  // A JSON merge payload follows the input's ignore_unknown_fields (see the
  // header); with no JSON input it is strict.  CheckCanMerge() guarantees
  // that nothing but the input has been parsed so far.
  if (parse.has_json()) {
    const ParseAction& input = actions_.front().parse();
    parse.mutable_json()->set_ignore_unknown_fields(
        actions_.front().has_parse() && input.has_json() &&
        input.json().ignore_unknown_fields());
  }
  const int id = next_handle_++;
  parse.set_type(type_->full_name());
  parse.set_id(id);
  ConformanceAction parse_action;
  *parse_action.mutable_parse() = std::move(parse);
  actions_.push_back(std::move(parse_action));
  ConformanceAction merge_action;
  merge_action.mutable_merge()->set_from(id);
  merge_action.mutable_merge()->set_to(0);
  actions_.push_back(std::move(merge_action));
  return std::move(*this);
}

void InMemoryMessage::CheckCanMerge() const {
  for (const ConformanceAction& action : actions_) {
    // Not a protocol limit (version 2 carries any number of merges): the
    // shortcuts deliberately keep to the one merge shape version 1 can
    // express, so that a test written with them runs on either version.
    ABSL_DCHECK(!action.has_merge())
        << "Merge*() may be called once per test: the shortcuts keep to the "
           "single merge protocol version 1 can express; use MergeFrom() to "
           "merge more than one message";
    ABSL_DCHECK(!action.has_discard_unknown_fields())
        << "Merge*() must come before DiscardUnknownFields(): the testee "
           "merges before it discards unknown fields";
  }
}

InMemoryMessage&& InMemoryMessage::MergeFrom(InMemoryMessage&& other) && {
  ABSL_DCHECK(
      other.testee_ == testee_ && other.priority_ == priority_ &&
      other.name_.suite == name_.suite && other.name_.test == name_.test &&
      other.name_.params == name_.params && other.name_.suffix == name_.suffix)
      << "MergeFrom() merges a message of the same test: build it with the "
         "same Testee()/Testee(kP3) call as the message it is "
         "merged into";
  ABSL_DCHECK(other.extra_serializes_.empty())
      << "Also*() applies to the message a test serializes, not to the "
         "message MergeFrom() merges into it";
  // Only the flat request has a category, one per request, and it is this
  // message's (the input's): an override on the merged message has nothing to
  // relabel and would be dropped on the floor, so say so.
  ABSL_DCHECK(!other.test_category_override_.has_value())
      << "OverrideTestCategory() applies to the message a test serializes, "
         "not to the message MergeFrom() merges into it";
  // `other`'s handles come after the ones created so far.
  const int offset = next_handle_;
  for (ConformanceAction& action : other.actions_) {
    OffsetHandles(action, offset);
    actions_.push_back(std::move(action));
  }
  next_handle_ += other.next_handle_;
  ConformanceAction merge_action;
  merge_action.mutable_merge()->set_from(offset);
  merge_action.mutable_merge()->set_to(0);
  actions_.push_back(std::move(merge_action));
  return std::move(*this);
}

InMemoryMessage&& InMemoryMessage::DiscardUnknownFields() && {
  ConformanceAction action;
  action.mutable_discard_unknown_fields()->set_id(0);
  actions_.push_back(std::move(action));
  return std::move(*this);
}

InMemoryMessage&& InMemoryMessage::AlsoBinary() && {
  return QueueSerialize(SerializeIn(::conformance::PROTOBUF));
}

InMemoryMessage&& InMemoryMessage::AlsoText(
    TextSerializationOptions options) && {
  SerializeAction serialize = SerializeIn(::conformance::TEXT_FORMAT);
  if (options.print_unknown_fields) {
    serialize.mutable_text()->set_print_unknown_fields(true);
  }
  return QueueSerialize(std::move(serialize));
}

InMemoryMessage&& InMemoryMessage::AlsoJson() && {
  return QueueSerialize(SerializeIn(::conformance::JSON));
}

InMemoryMessage&& InMemoryMessage::QueueSerialize(SerializeAction serialize) {
  serialize.set_id(0);
  extra_serializes_.push_back(std::move(serialize));
  return std::move(*this);
}

InMemoryMessage&& InMemoryMessage::OverrideTestCategory(
    ::conformance::TestCategory category) && {
  ABSL_DCHECK_EQ(category, ::conformance::TEXT_FORMAT_TEST)
      << "OverrideTestCategory() only relabels the text-format suite's "
         "binary-input tests as TEXT_FORMAT_TEST";
  ABSL_DCHECK(actions_.front().has_parse() &&
              actions_.front().parse().has_binary())
      << "OverrideTestCategory() is for PROTOBUF-input tests";
  test_category_override_ = category;
  return std::move(*this);
}

TestResult InMemoryMessage::Finish(SerializeAction serialize,
                                   const NameStyle name_style) {
  // The terminal output first, then the extras, so that outputs()[0] is the
  // terminal Serialize*() call's.
  serialize.set_id(0);
  const WireFormat output_format = GetSerializeFormat(serialize);
  ConformanceAction terminal;
  *terminal.mutable_serialize() = std::move(serialize);
  actions_.push_back(std::move(terminal));
  for (SerializeAction& extra : extra_serializes_) {
    ConformanceAction action;
    *action.mutable_serialize() = std::move(extra);
    actions_.push_back(std::move(action));
  }
  extra_serializes_.clear();

  const int required_version = RequiredProtocolVersion(actions_);
  const int testee_version = testee_->protocol_version();
  // The identity comes from the formats the test asked for, before the
  // actions are lowered into a request, so that it is the same whichever
  // protocol version carries them.
  ConformanceTestInfo info =
      ResolveTestInfo(name_, priority_, InputFormatOf(actions_), output_format,
                      name_style, testee_->numberer_);

  // A v1 testee gets the flat request whenever the test fits it; anything
  // else travels as an action list, under the lowest version that can carry
  // it (never below 2, the first version with actions), so that a testee of
  // a later version still sees the request an older one would.  The two
  // shapes don't mix: a v2 request leaves every v1 field unset.
  ConformanceRequest request;
  if (required_version == 1 && testee_version == 1) {
    request = LowerToV1(actions_, test_category_override_);
  } else {
    request.set_protocol_version(std::max(required_version, 2));
    for (ConformanceAction& action : actions_) {
      *request.add_actions() = std::move(action);
    }
  }
  actions_.clear();

  if (required_version > testee_version) {
    // Not sent; the name is still taken, since the test exists and Yields()
    // reports it to the TestManager under it.
    testee_->RegisterTestName(info.legacy_test_name);
    return TestResult(std::move(info), type_, std::move(request),
                      TestResult::UnsupportedProtocol{testee_version});
  }

  ::conformance::ConformanceResponse response =
      testee_->Run(info.legacy_test_name, request);

  return TestResult(std::move(info), type_, std::move(request),
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
