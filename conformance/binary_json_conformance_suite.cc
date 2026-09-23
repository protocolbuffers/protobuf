// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/binary_json_conformance_suite.h"

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/log/die_if_null.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "absl/types/span.h"
#include "json/config.h"
#include "json/reader.h"
#include "json/value.h"
#include "conformance/binary_test_util.h"
#include "conformance/binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "conformance/conformance_test.h"
#include "conformance/test_protos/test_messages_edition2023.pb.h"
#include "conformance/test_protos/test_messages_edition_unstable.pb.h"
#include "editions/golden/test_messages_proto2_editions.pb.h"
#include "editions/golden/test_messages_proto3_editions.pb.h"
#include "google/protobuf/json/json.h"
#include "google/protobuf/test_messages_proto2.pb.h"
#include "google/protobuf/test_messages_proto3.pb.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/util/message_differencer.h"
#include "google/protobuf/util/type_resolver_util.h"
#include "google/protobuf/wire_format_lite.h"

using ::conformance::ConformanceRequest;
using ::conformance::ConformanceResponse;
using ::conformance::TestStatus;
using ::conformance::WireFormat;
using google::protobuf::Descriptor;
using google::protobuf::FieldDescriptor;
using google::protobuf::internal::WireFormatLite;
using google::protobuf::util::NewTypeResolverForDescriptorPool;
using protobuf_test_messages::edition_unstable::TestAllTypesEditionUnstable;
using protobuf_test_messages::editions::TestAllTypesEdition2023;
using protobuf_test_messages::proto2::TestAllTypesProto2;
using protobuf_test_messages::proto3::TestAllTypesProto3;
using TestAllTypesProto2Editions =
    protobuf_test_messages::editions::proto2::TestAllTypesProto2;
using TestAllTypesProto3Editions =
    protobuf_test_messages::editions::proto3::TestAllTypesProto3;

namespace {

constexpr absl::string_view kTypeUrlPrefix = "type.googleapis.com";

std::string GetTypeUrl(const Descriptor* message) {
  return absl::StrCat(kTypeUrlPrefix, "/", message->full_name());
}

std::string str_repeat(absl::string_view s, int count) {
  std::string result;
  for (int i = 0; i < count; ++i) {
    absl::StrAppend(&result, s);
  }
  return result;
}

/* Routines for building arbitrary protos *************************************/

// We would use CodedOutputStream except that we want more freedom to build
// arbitrary protos (even invalid ones).

std::string varint(uint64_t x) { return google::protobuf::conformance::Varint(x).str(); }
std::string delim(const std::string& buf) {
  return google::protobuf::conformance::LengthPrefixed(buf).str();
}

std::string tag(uint32_t fieldnum, char wire_type) {
  return google::protobuf::conformance::Tag(
             fieldnum, static_cast<google::protobuf::conformance::WireType>(wire_type))
      .str();
}
std::string tag(int fieldnum, char wire_type) {
  return tag(static_cast<uint32_t>(fieldnum), wire_type);
}

std::string field(uint32_t fieldnum, char wire_type, std::string content) {
  return absl::StrCat(tag(fieldnum, wire_type), content);
}

std::string group(uint32_t fieldnum, absl::string_view content) {
  return google::protobuf::conformance::DelimitedField(fieldnum,
                                             google::protobuf::conformance::Wire(content))
      .str();
}

std::string len(uint32_t fieldnum, std::string content) {
  return google::protobuf::conformance::LengthPrefixedField(fieldnum, std::move(content))
      .str();
}

// The value encoders live in binary_test_util.h now; these wrappers keep the
// remaining callers' std::string plumbing unchanged.
std::string GetDefaultValue(FieldDescriptor::Type type) {
  return google::protobuf::conformance::GetDefaultValue(type).str();
}

std::string GetNonDefaultValue(FieldDescriptor::Type type) {
  return google::protobuf::conformance::GetNonDefaultValue(type).str();
}

std::string UpperCase(std::string str) {
  for (size_t i = 0; i < str.size(); i++) {
    str[i] = toupper(str[i]);
  }
  return str;
}

}  // anonymous namespace

namespace google {
namespace protobuf {

bool BinaryAndJsonConformanceSuite::ParseJsonResponse(
    const ConformanceResponse& response, Message* test_message) {
  json::ParseOptions options;
  options.allow_legacy_nonconformant_behavior = false;
  absl::Status status =
      json::JsonStringToMessage(response.json_payload(), test_message);
  if (!status.ok()) {
    ABSL_LOG(ERROR) << status;
    return false;
  }
  return true;
}

bool BinaryAndJsonConformanceSuite::ParseResponse(
    const ConformanceResponse& response,
    const ConformanceRequestSetting& setting, Message* test_message) {
  const ConformanceRequest& request = setting.GetRequest();
  WireFormat requested_output = request.requested_output_format();
  const std::string& test_name = setting.GetTestName();
  ConformanceLevel level = setting.GetLevel();

  TestStatus test;
  test.set_name(test_name);
  switch (response.result_case()) {
    case ConformanceResponse::kProtobufPayload: {
      if (requested_output != ::conformance::PROTOBUF) {
        test.set_failure_message(absl::StrCat(
            "Test was asked for ", WireFormatToString(requested_output),
            " output but provided PROTOBUF instead."));
        ReportFailure(test, level, request, response);
        return false;
      }

      if (!test_message->ParseFromString(response.protobuf_payload())) {
        test.set_failure_message(
            "Protobuf output we received from test was unparseable.");
        ReportFailure(test, level, request, response);
        return false;
      }

      break;
    }

    case ConformanceResponse::kJsonPayload: {
      if (requested_output != ::conformance::JSON) {
        test.set_failure_message(absl::StrCat(
            "Test was asked for ", WireFormatToString(requested_output),
            " output but provided JSON instead."));
        ReportFailure(test, level, request, response);
        return false;
      }

      if (!ParseJsonResponse(response, test_message)) {
        test.set_failure_message(
            "JSON output we received from test was unparseable.");
        ReportFailure(test, level, request, response);
        return false;
      }

      break;
    }

    default:
      ABSL_LOG(FATAL) << test_name
                      << ": unknown payload type: " << response.result_case()
                      << ", response: " << response;
  }

  return true;
}

void BinaryAndJsonConformanceSuite::RunSuiteImpl() {
  type_resolver_.reset(NewTypeResolverForDescriptorPool(
      kTypeUrlPrefix, DescriptorPool::generated_pool()));

  BinaryAndJsonConformanceSuiteImpl<TestAllTypesProto3>(
      this, /*run_proto3_tests=*/true);
  BinaryAndJsonConformanceSuiteImpl<TestAllTypesProto2>(
      this, /*run_proto3_tests=*/false);
  // The message set and recursion limit tests live in the
  // binary_conformance_tests library (see BUILD).
  if (maximum_edition_ >= Edition::EDITION_2023) {
    BinaryAndJsonConformanceSuiteImpl<TestAllTypesProto3Editions>(
        this, /*run_proto3_tests=*/true);
    BinaryAndJsonConformanceSuiteImpl<TestAllTypesProto2Editions>(
        this, /*run_proto3_tests=*/false);
    if (!this->performance_) {
      RunDelimitedFieldTests();
      RunUnstableTests();
    }
  }
}

void BinaryAndJsonConformanceSuite::RunDelimitedFieldTests() {
  SetTypeUrl(GetTypeUrl(TestAllTypesEdition2023::GetDescriptor()));

  // Only the binary->JSON legs remain here; the binary->binary legs live in
  // binary_delimited_field_test.cc (see BUILD).

  RunValidProtobufToJsonTest<TestAllTypesEdition2023>(
      absl::StrCat("ValidNonMessage"), REQUIRED,
      field(1, WireFormatLite::WIRETYPE_VARINT, varint(99)),
      R"pb(optional_int32: 99)pb");

  RunValidProtobufToJsonTest<TestAllTypesEdition2023>(
      absl::StrCat("ValidLengthPrefixedField"), REQUIRED,
      len(18, field(1, WireFormatLite::WIRETYPE_VARINT, varint(99))),
      R"pb(optional_nested_message { a: 99 })pb");

  RunValidProtobufToJsonTest<TestAllTypesEdition2023>(
      absl::StrCat("ValidMap.Integer"), REQUIRED,
      len(56,
          absl::StrCat(field(1, WireFormatLite::WIRETYPE_VARINT, varint(99)),
                       field(2, WireFormatLite::WIRETYPE_VARINT, varint(87)))),
      R"pb(map_int32_int32 { key: 99 value: 87 })pb");

  RunValidProtobufToJsonTest<TestAllTypesEdition2023>(
      absl::StrCat("ValidMap.LengthPrefixed"), REQUIRED,
      len(71, absl::StrCat(len(1, "a"),
                           len(2, field(1, WireFormatLite::WIRETYPE_VARINT,
                                        varint(87))))),
      R"pb(map_string_nested_message {
             key: "a"
             value: { a: 87 }
           })pb");

  RunValidProtobufToJsonTest<TestAllTypesEdition2023>(
      absl::StrCat("ValidDelimitedField.GroupLike"), REQUIRED,
      group(201, field(202, WireFormatLite::WIRETYPE_VARINT, varint(99))),
      R"pb(groupliketype { group_int32: 99 })pb");

  RunValidProtobufToJsonTest<TestAllTypesEdition2023>(
      absl::StrCat("ValidDelimitedField.NotGroupLike"), REQUIRED,
      group(202, field(202, WireFormatLite::WIRETYPE_VARINT, varint(99))),
      R"pb(delimited_field { group_int32: 99 })pb");

  // The ValidDelimitedExtension tests live in the binary_conformance_tests
  // library (see BUILD).
}

void BinaryAndJsonConformanceSuite::RunUnstableTests() {
  SetTypeUrl(GetTypeUrl(TestAllTypesEditionUnstable::GetDescriptor()));

  // Only the binary->JSON legs remain here; the binary->binary legs live in
  // binary_unstable_edition_test.cc (see BUILD).

  RunValidProtobufToJsonTest<TestAllTypesEditionUnstable>(
      absl::StrCat("ValidBytes"), REQUIRED, len(13, "foo"),
      R"pb(optional_bytes: "foo")pb");

  RunValidProtobufToJsonTest<TestAllTypesEditionUnstable>(
      absl::StrCat("ValidMap.Bytes"), REQUIRED,
      len(15, absl::StrCat(len(1, "foo"), len(2, "barbaz"))),
      R"pb(map_string_bytes { key: "foo" value: "barbaz" })pb");
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::RunValidJsonTest(
    const std::string& test_name, ConformanceLevel level,
    const std::string& input_json, const std::string& equivalent_text_format) {
  MessageType prototype;
  RunValidJsonTestWithMessage(test_name, level, input_json,
                              equivalent_text_format, prototype);
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::
    RunValidJsonTestWithMessage(const std::string& test_name,
                                ConformanceLevel level,
                                const std::string& input_json,
                                const std::string& equivalent_text_format,
                                const Message& prototype) {
  ConformanceRequestSetting setting1(
      level, ::conformance::JSON, ::conformance::PROTOBUF,
      ::conformance::JSON_TEST, prototype, test_name, input_json);
  suite_.RunValidInputTest(setting1, equivalent_text_format);
  ConformanceRequestSetting setting2(
      level, ::conformance::JSON, ::conformance::JSON, ::conformance::JSON_TEST,
      prototype, test_name, input_json);
  suite_.RunValidInputTest(setting2, equivalent_text_format);
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::
    RunValidJsonTestWithProtobufInput(
        const std::string& test_name, ConformanceLevel level,
        const MessageType& input, const std::string& equivalent_text_format) {
  ConformanceRequestSetting setting(
      level, ::conformance::PROTOBUF, ::conformance::JSON,
      ::conformance::JSON_TEST, input, test_name, input.SerializeAsString());
  suite_.RunValidInputTest(setting, equivalent_text_format);
}

template <typename MessageType>
void BinaryAndJsonConformanceSuite::RunValidProtobufToJsonTest(
    const std::string& test_name, ConformanceLevel level,
    const std::string& input_protobuf,
    const std::string& equivalent_text_format) {
  MessageType prototype;
  ConformanceRequestSetting binary_to_json(
      level, ::conformance::PROTOBUF, ::conformance::JSON,
      ::conformance::BINARY_TEST, prototype, test_name, input_protobuf);
  RunValidInputTest(binary_to_json, equivalent_text_format);
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::RunValidProtobufToJsonTest(
    const std::string& test_name, ConformanceLevel level,
    const std::string& input_protobuf,
    const std::string& equivalent_text_format) {
  suite_.RunValidProtobufToJsonTest<MessageType>(
      test_name, level, input_protobuf, equivalent_text_format);
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::
    RunValidProtobufToJsonTestWithMessage(
        const std::string& test_name, ConformanceLevel level,
        const Message* input, const std::string& equivalent_text_format) {
  RunValidProtobufToJsonTest(test_name, level, input->SerializeAsString(),
                             equivalent_text_format);
}

// According to proto JSON specification, JSON serializers follow more strict
// rules than parsers (e.g., a serializer must serialize int32 values as JSON
// numbers while the parser is allowed to accept them as JSON strings). This
// method allows strict checking on a proto JSON serializer by inspecting

template <typename MessageType>  // the JSON output directly.
void BinaryAndJsonConformanceSuiteImpl<
    MessageType>::RunValidJsonTestWithValidator(const std::string& test_name,
                                                ConformanceLevel level,
                                                const std::string& input_json,
                                                const Validator& validator) {
  MessageType prototype;
  ConformanceRequestSetting setting(
      level, ::conformance::JSON, ::conformance::JSON, ::conformance::JSON_TEST,
      prototype, test_name, input_json);
  const ConformanceRequest& request = setting.GetRequest();
  ConformanceResponse response;
  const std::string& effective_test_name = setting.GetTestName();

  if (!suite_.RunTest(effective_test_name, request, &response)) {
    return;
  }

  TestStatus test;
  test.set_name(effective_test_name);
  if (response.result_case() == ConformanceResponse::kSkipped) {
    suite_.ReportSkip(test, request, response);
    return;
  }

  if (response.result_case() != ConformanceResponse::kJsonPayload) {
    test.set_failure_message(absl::StrCat("Expected JSON payload but got type ",
                                          response.result_case()));
    suite_.ReportFailure(test, level, request, response);
    return;
  }
  Json::CharReaderBuilder builder;
  Json::Value value;
  Json::String err;
  const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
  if (!reader->parse(
          response.json_payload().c_str(),
          response.json_payload().c_str() + response.json_payload().length(),
          &value, &err)) {
    test.set_failure_message(
        absl::StrCat("JSON payload cannot be parsed as valid JSON: ", err));
    suite_.ReportFailure(test, level, request, response);
    return;
  }
  if (!validator(value)) {
    test.set_failure_message("JSON payload validation failed.");
    suite_.ReportFailure(test, level, request, response);
    return;
  }
  suite_.ReportSuccess(test);
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::ExpectParseFailureForJson(
    const std::string& test_name, ConformanceLevel level,
    const std::string& input_json) {
  MessageType prototype;
  // We don't expect output, but if the program erroneously accepts the protobuf
  // we let it send its response as this.  We must not leave it unspecified.
  ConformanceRequestSetting setting(
      level, ::conformance::JSON, ::conformance::JSON, ::conformance::JSON_TEST,
      prototype, test_name, input_json);
  const ConformanceRequest& request = setting.GetRequest();
  ConformanceResponse response;
  std::string effective_test_name =
      absl::StrCat(setting.ConformanceLevelToString(level), ".",
                   SyntaxIdentifier(), ".JsonInput.", test_name);

  if (!suite_.RunTest(effective_test_name, request, &response)) {
    return;
  }

  TestStatus test;
  test.set_name(effective_test_name);
  if (response.result_case() == ConformanceResponse::kParseError) {
    suite_.ReportSuccess(test);
  } else if (response.result_case() == ConformanceResponse::kSkipped) {
    suite_.ReportSkip(test, request, response);
  } else {
    test.set_failure_message("Should have failed to parse, but didn't.");
    suite_.ReportFailure(test, level, request, response);
  }
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::
    RunValidJsonTestOrParseFailure(const std::string& test_name,
                                   ConformanceLevel level,
                                   const std::string& input_json,
                                   const std::string& equivalent_text_format) {
  MessageType prototype;
  ConformanceRequestSetting setting(
      level, ::conformance::JSON, ::conformance::PROTOBUF,
      ::conformance::JSON_TEST, prototype, test_name, input_json);
  const ConformanceRequest& request = setting.GetRequest();
  ConformanceResponse response;
  std::string effective_test_name =
      absl::StrCat(setting.ConformanceLevelToString(level), ".",
                   SyntaxIdentifier(), ".JsonInput.", test_name);

  if (!suite_.RunTest(effective_test_name, request, &response)) {
    return;
  }

  TestStatus test;
  test.set_name(effective_test_name);
  if (response.result_case() == ConformanceResponse::kParseError) {
    suite_.ReportSuccess(test);
  } else if (response.result_case() == ConformanceResponse::kSkipped) {
    suite_.ReportSkip(test, request, response);
  } else {
    std::unique_ptr<Message> reference_message(setting.NewTestMessage());
    ABSL_CHECK(TextFormat::ParseFromString(equivalent_text_format,
                                           reference_message.get()))
        << "Failed to parse data for test case: " << setting.GetTestName()
        << ", data: " << equivalent_text_format;
    std::unique_ptr<Message> test_message(setting.NewTestMessage());
    bool parsed = false;
    if (response.result_case() == ConformanceResponse::kProtobufPayload) {
      parsed = test_message->ParseFromString(response.protobuf_payload());
    }
    if (!parsed) {
      test.set_failure_message("Malformed protobuf response");
      suite_.ReportFailure(test, level, request, response);
      return;
    }

    util::MessageDifferencer differencer;
    util::DefaultFieldComparator field_comparator;
    field_comparator.set_treat_nan_as_equal(true);
    differencer.set_field_comparator(&field_comparator);
    std::string differences;
    differencer.ReportDifferencesToString(&differences);
    if (differencer.Compare(*reference_message, *test_message)) {
      suite_.ReportSuccess(test);
    } else {
      test.set_failure_message(
          "Should have failed to parse or matched expected output but did "
          "not.");
      suite_.ReportFailure(test, level, request, response);
    }
  }
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::
    ExpectSerializeFailureForJson(const std::string& test_name,
                                  ConformanceLevel level,
                                  const std::string& text_format) {
  MessageType payload_message;
  ABSL_CHECK(TextFormat::ParseFromString(text_format, &payload_message))
      << "Failed to parse: " << text_format;

  MessageType prototype;
  ConformanceRequestSetting setting(
      level, ::conformance::PROTOBUF, ::conformance::JSON,
      ::conformance::JSON_TEST, prototype, test_name,
      payload_message.SerializeAsString());
  const ConformanceRequest& request = setting.GetRequest();
  ConformanceResponse response;
  const std::string& effective_test_name = setting.GetTestName();

  if (!suite_.RunTest(effective_test_name, request, &response)) {
    return;
  }

  TestStatus test;
  test.set_name(effective_test_name);
  if (response.result_case() == ConformanceResponse::kSerializeError) {
    suite_.ReportSuccess(test);
  } else if (response.result_case() == ConformanceResponse::kSkipped) {
    suite_.ReportSkip(test, request, response);
  } else {
    test.set_failure_message("Should have failed to serialize, but didn't.");
    suite_.ReportFailure(test, level, request, response);
  }
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::TestValidDataForType(
    FieldDescriptor::Type type) {
  const std::string type_name =
      UpperCase(absl::StrCat(".", FieldDescriptor::TypeName(type)));
  WireFormatLite::WireType wire_type = WireFormatLite::WireTypeForFieldType(
      static_cast<WireFormatLite::FieldType>(type));
  const FieldDescriptor* field = GetFieldForType(type, false);
  const FieldDescriptor* rep_field = GetFieldForType(type, true);
  const absl::Span<const ::google::protobuf::conformance::ValidDataCase> values =
      ::google::protobuf::conformance::ValidDataCases(type);

  // Test singular data for singular fields.  Only the binary->JSON legs
  // remain here; the binary->binary legs (and the byte-exact
  // ValidDataScalarBinary tests) live in binary_valid_data_scalar_test.cc
  // (see BUILD), which shares the value tables.
  for (size_t i = 0; i < values.size(); i++) {
    std::string proto =
        absl::StrCat(tag(field->number(), wire_type), values[i].input);
    // In proto3, default primitive fields should not be encoded.
    std::string expected_proto =
        run_proto3_tests_ && ::google::protobuf::conformance::IsDefaultValue(
                                 field->type(), values[i].expected)
            ? ""
            : absl::StrCat(tag(field->number(), wire_type), values[i].expected);
    MessageType test_message;
    ABSL_CHECK(test_message.MergeFromString(expected_proto));
    std::string text;
    ABSL_CHECK(TextFormat::PrintToString(test_message, &text));

    RunValidProtobufToJsonTest(
        absl::StrCat("ValidDataScalar", type_name, "[", i, "]"), REQUIRED,
        proto, text);
  }

  // Test repeated data for singular fields (binary->JSON leg only, as above).
  // For scalar message fields, repeated values are merged, which is tested
  // separately.
  if (type != FieldDescriptor::TYPE_MESSAGE) {
    std::string proto;
    for (size_t i = 0; i < values.size(); i++) {
      absl::StrAppend(&proto, tag(field->number(), wire_type), values[i].input);
    }
    std::string expected_proto =
        absl::StrCat(tag(field->number(), wire_type), values.back().expected);
    MessageType test_message;
    ABSL_CHECK(test_message.MergeFromString(expected_proto));
    std::string text;
    ABSL_CHECK(TextFormat::PrintToString(test_message, &text));

    RunValidProtobufToJsonTest(
        absl::StrCat("RepeatedScalarSelectsLast", type_name), REQUIRED, proto,
        text);
  }

  // Test repeated fields (binary->JSON legs only; the binary->binary legs and
  // the byte-exact .*Output tests live in binary_valid_data_repeated_test.cc
  // (see BUILD), which shares the value tables).
  if (FieldDescriptor::IsTypePackable(type)) {
    std::string default_proto_packed;
    std::string default_proto_unpacked;
    std::string default_proto_packed_expected;
    for (size_t i = 0; i < values.size(); i++) {
      absl::StrAppend(&default_proto_unpacked,
                      tag(rep_field->number(), wire_type), values[i].input);
      absl::StrAppend(&default_proto_packed, values[i].input);
      absl::StrAppend(&default_proto_packed_expected, values[i].expected);
    }
    default_proto_packed = absl::StrCat(
        tag(rep_field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
        delim(default_proto_packed));
    default_proto_packed_expected = absl::StrCat(
        tag(rep_field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
        delim(default_proto_packed_expected));

    MessageType test_message;
    ABSL_CHECK(test_message.MergeFromString(default_proto_packed_expected));
    std::string text;
    ABSL_CHECK(TextFormat::PrintToString(test_message, &text));

    // Ensures both packed and unpacked data can be parsed.
    RunValidProtobufToJsonTest(
        absl::StrCat("ValidDataRepeated", type_name, ".UnpackedInput"),
        REQUIRED, default_proto_unpacked, text);
    RunValidProtobufToJsonTest(
        absl::StrCat("ValidDataRepeated", type_name, ".PackedInput"), REQUIRED,
        default_proto_packed, text);
  } else {
    std::string proto;
    std::string expected_proto;
    for (size_t i = 0; i < values.size(); i++) {
      absl::StrAppend(&proto, tag(rep_field->number(), wire_type),
                      values[i].input);
      absl::StrAppend(&expected_proto, tag(rep_field->number(), wire_type),
                      values[i].expected);
    }
    MessageType test_message;
    ABSL_CHECK(test_message.MergeFromString(expected_proto));
    std::string text;
    ABSL_CHECK(TextFormat::PrintToString(test_message, &text));

    RunValidProtobufToJsonTest(absl::StrCat("ValidDataRepeated", type_name),
                               REQUIRED, proto, text);
  }
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<
    MessageType>::TestValidDataForRepeatedScalarMessage() {
  // Only the binary->JSON leg remains here; the binary->binary leg lives in
  // binary_merge_test.cc (see BUILD), which shares the input.
  const std::string expected =
      R"({
        corecursive: {
          optional_int32: 4321,
          optional_int64: 1234,
          optional_uint32: 4321,
          repeated_int32: [1234, 4321],
  }
      })";

  const FieldDescriptor* field =
      GetFieldForType(FieldDescriptor::TYPE_MESSAGE, false);
  RunValidProtobufToJsonTest(
      "RepeatedScalarMessageMerge", REQUIRED,
      ::google::protobuf::conformance::RepeatedScalarMessageMergeInput(
          *MessageType::GetDescriptor())
          .str(),
      absl::StrCat(field->name(), ": ", expected));
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::TestValidDataForMapType(
    FieldDescriptor::Type key_type, FieldDescriptor::Type value_type) {
  // Only the binary->JSON legs remain here; the binary->binary legs live in
  // binary_valid_data_map_test.cc (see BUILD), which builds the same entries.
  const std::string key_type_name =
      UpperCase(absl::StrCat(".", FieldDescriptor::TypeName(key_type)));
  const std::string value_type_name =
      UpperCase(absl::StrCat(".", FieldDescriptor::TypeName(value_type)));
  WireFormatLite::WireType key_wire_type = WireFormatLite::WireTypeForFieldType(
      static_cast<WireFormatLite::FieldType>(key_type));
  WireFormatLite::WireType value_wire_type =
      WireFormatLite::WireTypeForFieldType(
          static_cast<WireFormatLite::FieldType>(value_type));

  std::string key1_data =
      absl::StrCat(tag(1, key_wire_type), GetDefaultValue(key_type));
  std::string value1_data =
      absl::StrCat(tag(2, value_wire_type), GetDefaultValue(value_type));
  std::string key2_data =
      absl::StrCat(tag(1, key_wire_type), GetNonDefaultValue(key_type));
  std::string value2_data =
      absl::StrCat(tag(2, value_wire_type), GetNonDefaultValue(value_type));

  const FieldDescriptor* field = GetFieldForMapType(key_type, value_type);

  {
    // Tests map with default key and value.
    std::string proto = absl::StrCat(
        tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
        delim(absl::StrCat(key1_data, value1_data)));
    MessageType test_message;
    ABSL_CHECK(test_message.MergeFromString(proto));
    std::string text;
    ABSL_CHECK(TextFormat::PrintToString(test_message, &text));
    RunValidProtobufToJsonTest(absl::StrCat("ValidDataMap", key_type_name,
                                            value_type_name, ".Default"),
                               REQUIRED, proto, text);
  }

  {
    // Tests map with missing default key and value.
    std::string proto = absl::StrCat(
        tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
        delim(""));
    MessageType test_message;
    ABSL_CHECK(test_message.MergeFromString(proto));
    std::string text;
    ABSL_CHECK(TextFormat::PrintToString(test_message, &text));
    RunValidProtobufToJsonTest(absl::StrCat("ValidDataMap", key_type_name,
                                            value_type_name, ".MissingDefault"),
                               REQUIRED, proto, text);
  }

  {
    // Tests map with non-default key and value.
    std::string proto = absl::StrCat(
        tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
        delim(absl::StrCat(key2_data, value2_data)));
    MessageType test_message;
    ABSL_CHECK(test_message.MergeFromString(proto));
    std::string text;
    ABSL_CHECK(TextFormat::PrintToString(test_message, &text));
    RunValidProtobufToJsonTest(absl::StrCat("ValidDataMap", key_type_name,
                                            value_type_name, ".NonDefault"),
                               REQUIRED, proto, text);
  }

  {
    // Tests map with unordered key and value.
    std::string proto = absl::StrCat(
        tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
        delim(absl::StrCat(value2_data, key2_data)));
    MessageType test_message;
    ABSL_CHECK(test_message.MergeFromString(proto));
    std::string text;
    ABSL_CHECK(TextFormat::PrintToString(test_message, &text));
    RunValidProtobufToJsonTest(absl::StrCat("ValidDataMap", key_type_name,
                                            value_type_name, ".Unordered"),
                               REQUIRED, proto, text);
  }

  {
    // Tests map with duplicate key.
    std::string proto1 = absl::StrCat(
        tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
        delim(absl::StrCat(key2_data, value1_data)));
    std::string proto2 = absl::StrCat(
        tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
        delim(absl::StrCat(key2_data, value2_data)));
    std::string proto = absl::StrCat(proto1, proto2);
    MessageType test_message;
    ABSL_CHECK(test_message.MergeFromString(proto2));
    std::string text;
    ABSL_CHECK(TextFormat::PrintToString(test_message, &text));
    RunValidProtobufToJsonTest(absl::StrCat("ValidDataMap", key_type_name,
                                            value_type_name, ".DuplicateKey"),
                               REQUIRED, proto, text);
  }

  {
    // Tests map with duplicate key in map entry.
    std::string proto = absl::StrCat(
        tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
        delim(absl::StrCat(key1_data, key2_data, value2_data)));
    MessageType test_message;
    ABSL_CHECK(test_message.MergeFromString(proto));
    std::string text;
    ABSL_CHECK(TextFormat::PrintToString(test_message, &text));
    RunValidProtobufToJsonTest(
        absl::StrCat("ValidDataMap", key_type_name, value_type_name,
                     ".DuplicateKeyInMapEntry"),
        REQUIRED, proto, text);
  }

  {
    // Tests map with duplicate value in map entry.
    std::string proto = absl::StrCat(
        tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
        delim(absl::StrCat(key2_data, value1_data, value2_data)));
    MessageType test_message;
    ABSL_CHECK(test_message.MergeFromString(proto));
    std::string text;
    ABSL_CHECK(TextFormat::PrintToString(test_message, &text));
    RunValidProtobufToJsonTest(
        absl::StrCat("ValidDataMap", key_type_name, value_type_name,
                     ".DuplicateValueInMapEntry"),
        REQUIRED, proto, text);
  }
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<
    MessageType>::TestOverwriteMessageValueMap() {
  // Only the binary->JSON leg remains here; the binary->binary leg lives in
  // binary_merge_test.cc (see BUILD), which shares the input.
  ::google::protobuf::conformance::MergeTestData data =
      ::google::protobuf::conformance::MapMessageValueMergeData(
          *MessageType::GetDescriptor());
  MessageType test_message;
  ABSL_CHECK(test_message.MergeFromString(data.expected.data()));
  std::string text;
  ABSL_CHECK(TextFormat::PrintToString(test_message, &text));
  RunValidProtobufToJsonTest("ValidDataMap.STRING.MESSAGE.MergeValue", REQUIRED,
                             std::move(data.input).str(), text);
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::TestValidDataForOneofType(
    FieldDescriptor::Type type) {
  // Only the binary->JSON legs remain here; the binary->binary legs and the
  // byte-exact ValidDataOneofBinary tests live in binary_oneof_test.cc (see
  // BUILD), which builds the same values.
  const std::string type_name =
      UpperCase(absl::StrCat(".", FieldDescriptor::TypeName(type)));
  WireFormatLite::WireType wire_type = WireFormatLite::WireTypeForFieldType(
      static_cast<WireFormatLite::FieldType>(type));

  const FieldDescriptor* field = ::google::protobuf::conformance::GetFieldForOneofType(
      *MessageType::GetDescriptor(), type);
  const std::string default_value =
      absl::StrCat(tag(field->number(), wire_type), GetDefaultValue(type));
  const std::string non_default_value =
      absl::StrCat(tag(field->number(), wire_type), GetNonDefaultValue(type));

  {
    // Tests oneof with default value.
    const std::string& proto = default_value;
    MessageType test_message;
    ABSL_CHECK(test_message.MergeFromString(proto));
    std::string text;
    ABSL_CHECK(TextFormat::PrintToString(test_message, &text));

    RunValidProtobufToJsonTest(
        absl::StrCat("ValidDataOneof", type_name, ".DefaultValue"), REQUIRED,
        proto, text);
  }

  {
    // Tests oneof with non-default value.
    const std::string& proto = non_default_value;
    MessageType test_message;
    ABSL_CHECK(test_message.MergeFromString(proto));
    std::string text;
    ABSL_CHECK(TextFormat::PrintToString(test_message, &text));

    RunValidProtobufToJsonTest(
        absl::StrCat("ValidDataOneof", type_name, ".NonDefaultValue"), REQUIRED,
        proto, text);
  }

  {
    // Tests oneof with multiple values of the same field.
    const std::string proto = absl::StrCat(default_value, non_default_value);
    const std::string& expected_proto = non_default_value;
    MessageType test_message;
    ABSL_CHECK(test_message.MergeFromString(expected_proto));
    std::string text;
    ABSL_CHECK(TextFormat::PrintToString(test_message, &text));

    RunValidProtobufToJsonTest(absl::StrCat("ValidDataOneof", type_name,
                                            ".MultipleValuesForSameField"),
                               REQUIRED, proto, text);
  }

  {
    // Tests oneof with multiple values of the different fields.
    const FieldDescriptor* other_field =
        ::google::protobuf::conformance::GetFieldForOneofType(
            *MessageType::GetDescriptor(), type,
            ::google::protobuf::conformance::OneofMember::kOfOtherType);
    FieldDescriptor::Type other_type = other_field->type();
    WireFormatLite::WireType other_wire_type =
        WireFormatLite::WireTypeForFieldType(
            static_cast<WireFormatLite::FieldType>(other_type));
    const std::string other_value =
        absl::StrCat(tag(other_field->number(), other_wire_type),
                     GetDefaultValue(other_type));

    const std::string proto = absl::StrCat(other_value, non_default_value);
    const std::string& expected_proto = non_default_value;
    MessageType test_message;
    ABSL_CHECK(test_message.MergeFromString(expected_proto));
    std::string text;
    ABSL_CHECK(TextFormat::PrintToString(test_message, &text));

    RunValidProtobufToJsonTest(absl::StrCat("ValidDataOneof", type_name,
                                            ".MultipleValuesForDifferentField"),
                               REQUIRED, proto, text);
  }
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::TestMergeOneofMessage() {
  // Only the binary->JSON leg remains here; the binary->binary leg and the
  // byte-exact ValidDataOneofBinary.MESSAGE.Merge live in binary_merge_test.cc
  // (see BUILD), which shares the input.
  ::google::protobuf::conformance::MergeTestData data =
      ::google::protobuf::conformance::OneofMessageMergeData(
          *MessageType::GetDescriptor());
  MessageType test_message;
  ABSL_CHECK(test_message.MergeFromString(data.expected.data()));
  std::string text;
  ABSL_CHECK(TextFormat::PrintToString(test_message, &text));
  RunValidProtobufToJsonTest("ValidDataOneof.MESSAGE.Merge", REQUIRED,
                             std::move(data.input).str(), text);
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::TestOneofMessage() {
  // Only the binary->JSON legs remain here; the binary->binary legs live in
  // binary_oneof_zero_test.cc (see BUILD).
  MessageType message;
  message.set_oneof_uint32(0);
  RunValidProtobufToJsonTestWithMessage("OneofZeroUint32", RECOMMENDED,
                                        &message, "oneof_uint32: 0");
  message.mutable_oneof_nested_message()->set_a(0);
  RunValidProtobufToJsonTestWithMessage(
      "OneofZeroMessage", RECOMMENDED, &message,
      run_proto3_tests_ ? "oneof_nested_message: {}"
                        : "oneof_nested_message: {a: 0}");
  message.mutable_oneof_nested_message()->set_a(1);
  RunValidProtobufToJsonTestWithMessage("OneofZeroMessageSetTwice", RECOMMENDED,
                                        &message,
                                        "oneof_nested_message: {a: 1}");
  message.set_oneof_string("");
  RunValidProtobufToJsonTestWithMessage("OneofZeroString", RECOMMENDED,
                                        &message, "oneof_string: \"\"");
  message.set_oneof_bytes("");
  RunValidProtobufToJsonTestWithMessage("OneofZeroBytes", RECOMMENDED, &message,
                                        "oneof_bytes: \"\"");
  message.set_oneof_bool(false);
  RunValidProtobufToJsonTestWithMessage("OneofZeroBool", RECOMMENDED, &message,
                                        "oneof_bool: false");
  message.set_oneof_uint64(0);
  RunValidProtobufToJsonTestWithMessage("OneofZeroUint64", RECOMMENDED,
                                        &message, "oneof_uint64: 0");
  message.set_oneof_float(0.0f);
  RunValidProtobufToJsonTestWithMessage("OneofZeroFloat", RECOMMENDED, &message,
                                        "oneof_float: 0");
  message.set_oneof_double(0.0);
  RunValidProtobufToJsonTestWithMessage("OneofZeroDouble", RECOMMENDED,
                                        &message, "oneof_double: 0");
  message.set_oneof_enum(MessageType::FOO);
  RunValidProtobufToJsonTestWithMessage("OneofZeroEnum", RECOMMENDED, &message,
                                        "oneof_enum: FOO");
}

template <typename MessageType>
BinaryAndJsonConformanceSuiteImpl<MessageType>::
    BinaryAndJsonConformanceSuiteImpl(BinaryAndJsonConformanceSuite* suite,
                                      bool run_proto3_tests)
    : suite_(*ABSL_DIE_IF_NULL(suite)), run_proto3_tests_(run_proto3_tests) {
  suite_.SetTypeUrl(GetTypeUrl(MessageType::GetDescriptor()));
  RunAllTests();
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::RunAllTests() {
  // Only the groups not yet migrated to gtest run from here; the migrated
  // ones live in the binary_conformance_tests library, and every performance
  // test in performance_conformance_tests (see BUILD), so under --performance
  // nothing runs from here.
  if (!suite_.performance_) {
    // The value tables live in binary_test_util.cc (ValidDataCases()).
    TestValidDataForType(FieldDescriptor::TYPE_DOUBLE);
    TestValidDataForType(FieldDescriptor::TYPE_FLOAT);
    TestValidDataForType(FieldDescriptor::TYPE_INT64);
    TestValidDataForType(FieldDescriptor::TYPE_UINT64);
    TestValidDataForType(FieldDescriptor::TYPE_INT32);
    TestValidDataForType(FieldDescriptor::TYPE_UINT32);
    TestValidDataForType(FieldDescriptor::TYPE_FIXED64);
    TestValidDataForType(FieldDescriptor::TYPE_FIXED32);
    TestValidDataForType(FieldDescriptor::TYPE_SFIXED64);
    TestValidDataForType(FieldDescriptor::TYPE_SFIXED32);
    TestValidDataForType(FieldDescriptor::TYPE_BOOL);
    TestValidDataForType(FieldDescriptor::TYPE_SINT32);
    TestValidDataForType(FieldDescriptor::TYPE_SINT64);
    TestValidDataForType(FieldDescriptor::TYPE_STRING);
    TestValidDataForType(FieldDescriptor::TYPE_BYTES);
    TestValidDataForType(FieldDescriptor::TYPE_ENUM);
    TestValidDataForRepeatedScalarMessage();
    TestValidDataForType(FieldDescriptor::TYPE_MESSAGE);

    for (const ::google::protobuf::conformance::MapType& map_type :
         ::google::protobuf::conformance::ValidDataMapTypes()) {
      TestValidDataForMapType(map_type.key, map_type.value);
    }
    // Additional test to check overwriting message value map.
    TestOverwriteMessageValueMap();

    for (FieldDescriptor::Type type :
         ::google::protobuf::conformance::ValidDataOneofTypes()) {
      TestValidDataForOneofType(type);
    }
    // Additional test to check merging oneof message.
    TestMergeOneofMessage();

    // TODO:
    // TestValidDataForType(FieldDescriptor::TYPE_GROUP

    // The unknown field tests (UnknownVarint, UnknownOrdering) live in the
    // binary_conformance_tests library (see BUILD).
    TestOneofMessage();

    RunJsonTests();
  }
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::RunJsonTests() {
  // The JSON tests that have been migrated to the gtest-based suite live in
  // json_*_test.cc (see conformance_suite(name = "json") in BUILD); only the
  // groups below are still run from here.

  if (run_proto3_tests_) {
    RunJsonTestsForFieldMask();
    RunJsonTestsForStruct();
    RunJsonTestsForValue();
  }
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<
    MessageType>::RunJsonTestsForReservedFields() {
  for (const auto& test_case : std::vector<std::pair<std::string, std::string>>{
           {"Boolean", "true"},
           {"Number", "1"},
           {"String", "\"hello\""},
           {"Message", R"json({ "a": 1 })json"},
       }) {
    ExpectParseFailureForJson(
        absl::StrCat("RejectReservedFieldName.", test_case.first), REQUIRED,
        absl::Substitute(R"json({
          "reserved_field": $0
        })json",
                         test_case.second));
  }
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<
    MessageType>::RunJsonTestsForFieldMask() {
  RunValidJsonTest("FieldMask", REQUIRED,
                   R"({"optionalFieldMask": "foo,barBaz"})",
                   R"(optional_field_mask: {paths: "foo" paths: "bar_baz"})");
  RunValidJsonTest("EmptyFieldMask", REQUIRED, R"({"optionalFieldMask": ""})",
                   R"(optional_field_mask: {})");
  ExpectParseFailureForJson("FieldMaskInvalidCharacter", RECOMMENDED,
                            R"({"optionalFieldMask": "foo,bar_bar"})");
  ExpectSerializeFailureForJson("FieldMaskPathsDontRoundTrip", RECOMMENDED,
                                R"(optional_field_mask: {paths: "fooBar"})");
  ExpectSerializeFailureForJson("FieldMaskNumbersDontRoundTrip", RECOMMENDED,
                                R"(optional_field_mask: {paths: "foo_3_bar"})");
  ExpectSerializeFailureForJson("FieldMaskTooManyUnderscore", RECOMMENDED,
                                R"(optional_field_mask: {paths: "foo__bar"})");
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::RunJsonTestsForStruct() {
  RunValidJsonTest("Struct", REQUIRED,
                   R"({
        "optionalStruct": {
          "nullValue": null,
          "intValue": 1234,
          "boolValue": true,
          "doubleValue": 1234.5678,
          "stringValue": "Hello world!",
          "listValue": [1234, "5678"],
          "objectValue": {
            "value": 0
    }
  }
      })",
                   R"(
        optional_struct: {
          fields: {
            key: "nullValue"
            value: {null_value: NULL_VALUE}
    }
          fields: {
            key: "intValue"
            value: {number_value: 1234}
    }
          fields: {
            key: "boolValue"
            value: {bool_value: true}
    }
          fields: {
            key: "doubleValue"
            value: {number_value: 1234.5678}
    }
          fields: {
            key: "stringValue"
            value: {string_value: "Hello world!"}
    }
          fields: {
            key: "listValue"
            value: {
              list_value: {
                values: {
                  number_value: 1234
          }
                values: {
                  string_value: "5678"
          }
        }
      }
    }
          fields: {
            key: "objectValue"
            value: {
              struct_value: {
                fields: {
                  key: "value"
                  value: {
                    number_value: 0
            }
          }
        }
      }
    }
  }
      )");
  RunValidJsonTest("StructWithEmptyListValue", REQUIRED,
                   R"({
        "optionalStruct": {
          "listValue": []
  }
      })",
                   R"(
        optional_struct: {
          fields: {
            key: "listValue"
            value: {
              list_value: {
        }
      }
    }
  }
      )");

  RunValidJsonTest(
      "StructDeepNesting25", RECOMMENDED,
      absl::StrCat(R"({"optionalStruct": {)", str_repeat(R"("n": {)", 25),
                   R"("value": 1)", std::string(25, '}'), "}}"),
      absl::StrCat("optional_struct: {\n",
                   str_repeat("  fields: {\n"
                              "    key: \"n\"\n"
                              "    value: {\n"
                              "      struct_value: {\n",
                              25),
                   "        fields: {\n"
                   "          key: \"value\"\n"
                   "          value: { number_value: 1 }\n"
                   "        }\n",
                   str_repeat("      }\n"
                              "    }\n"
                              "  }\n",
                              25),
                   "}\n"));

  ExpectParseFailureForJson(
      "StructDeepNesting200", RECOMMENDED,
      absl::StrCat(R"({"optionalStruct": {)", str_repeat(R"("n": {)", 200),
                   R"("value": 1)", std::string(200, '}'), "}}"));
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::RunJsonTestsForValue() {
  RunValidJsonTest("ValueAcceptInteger", REQUIRED, R"({"optionalValue": 1})",
                   "optional_value: { number_value: 1}");
  RunValidJsonTest("ValueAcceptFloat", REQUIRED, R"({"optionalValue": 1.5})",
                   "optional_value: { number_value: 1.5}");
  RunValidJsonTest("ValueAcceptBool", REQUIRED, R"({"optionalValue": false})",
                   "optional_value: { bool_value: false}");
  RunValidJsonTest("ValueAcceptNull", REQUIRED, R"({"optionalValue": null})",
                   "optional_value: { null_value: NULL_VALUE}");
  RunValidJsonTest("ValueAcceptString", REQUIRED,
                   R"({"optionalValue": "hello"})",
                   R"(optional_value: { string_value: "hello"})");
  RunValidJsonTest("ValueAcceptList", REQUIRED,
                   R"({"optionalValue": [0, "hello"]})",
                   R"(
        optional_value: {
          list_value: {
            values: {
              number_value: 0
      }
            values: {
              string_value: "hello"
      }
    }
  }
      )");
  RunValidJsonTest("ValueAcceptObject", REQUIRED,
                   R"({"optionalValue": {"value": 1}})",
                   R"(
        optional_value: {
          struct_value: {
            fields: {
              key: "value"
              value: {
                number_value: 1
        }
      }
    }
  }
      )");
  RunValidJsonTest("RepeatedValue", REQUIRED,
                   R"({
        "repeatedValue": [["a"]]
      })",
                   R"(
        repeated_value: [
  {
            list_value: {
              values: [
                { string_value: "a"}
        ]
      }
    }
  ]
      )");
  RunValidJsonTest("RepeatedListValue", REQUIRED,
                   R"({
        "repeatedListValue": [["a"]]
      })",
                   R"(
        repeated_list_value: [
  {
            values: [
              { string_value: "a"}
      ]
    }
  ]
      )");
  RunValidJsonTestWithValidator("NullValueInOtherOneofOldFormat", RECOMMENDED,
                                R"({"oneofNullValue": "NULL_VALUE"})",
                                [](const Json::Value& value) {
                                  return (value.isMember("oneofNullValue") &&
                                          value["oneofNullValue"].isNull());
                                });
  RunValidJsonTestWithValidator("NullValueInOtherOneofNewFormat", RECOMMENDED,
                                R"({"oneofNullValue": null})",
                                [](const Json::Value& value) {
                                  return (value.isMember("oneofNullValue") &&
                                          value["oneofNullValue"].isNull());
                                });
  RunValidJsonTestWithValidator(
      "NullValueInNormalMessage", RECOMMENDED, R"({"optionalNullValue": null})",
      [](const Json::Value& value) { return value.empty(); });
  ExpectSerializeFailureForJson("ValueRejectNanNumberValue", RECOMMENDED,
                                "optional_value: { number_value: nan}");
  ExpectSerializeFailureForJson("ValueRejectInfNumberValue", RECOMMENDED,
                                "optional_value: { number_value: inf}");
  RunValidJsonTest("ListValueDeepNesting25", RECOMMENDED,
                   absl::StrCat(R"({"optionalValue": )", std::string(25, '['),
                                "1", std::string(25, ']'), "}"),
                   absl::StrCat("optional_value: {\n",
                                str_repeat("  list_value: {\n"
                                           "    values: {\n",
                                           25),
                                "      number_value: 1\n",
                                str_repeat("    }\n"
                                           "  }\n",
                                           25),
                                "}\n"));

  ExpectParseFailureForJson(
      "ListValueDeepNesting200", RECOMMENDED,
      absl::StrCat(R"({"optionalValue": )", std::string(200, '['), "1",
                   std::string(200, ']'), "}"));
  RunValidJsonTest(
      "ValueDeepNesting25", RECOMMENDED,
      absl::StrCat(R"({"optionalValue": {)", str_repeat(R"("n": {)", 25),
                   R"("value": 1)", std::string(25, '}'), "}}"),
      absl::StrCat("optional_value: {\n", "  struct_value: {\n",
                   str_repeat("    fields: {\n"
                              "      key: \"n\"\n"
                              "      value: {\n"
                              "        struct_value: {\n",
                              25),
                   "          fields: {\n"
                   "            key: \"value\"\n"
                   "            value: { number_value: 1 }\n"
                   "          }\n",
                   str_repeat("        }\n"
                              "      }\n"
                              "    }\n",
                              25),
                   "  }\n", "}\n"));

  ExpectParseFailureForJson(
      "ValueDeepNesting200", RECOMMENDED,
      absl::StrCat(R"({"optionalValue": {)", str_repeat(R"("n": {)", 200),
                   R"("value": 1)", std::string(200, '}'), "}}"));
}

template <typename MessageType>
const FieldDescriptor*
BinaryAndJsonConformanceSuiteImpl<MessageType>::GetFieldForType(
    FieldDescriptor::Type type, bool repeated) const {
  return ::google::protobuf::conformance::GetFieldForType(*MessageType::GetDescriptor(),
                                                type, repeated);
}

template <typename MessageType>
const FieldDescriptor*
BinaryAndJsonConformanceSuiteImpl<MessageType>::GetFieldForMapType(
    FieldDescriptor::Type key_type, FieldDescriptor::Type value_type) const {
  return ::google::protobuf::conformance::GetFieldForMapType(
      *MessageType::GetDescriptor(), key_type, value_type);
}

template <typename MessageType>
std::string BinaryAndJsonConformanceSuiteImpl<MessageType>::SyntaxIdentifier()
    const {
  if (std::is_same_v<MessageType, TestAllTypesProto2>) {
    return "Proto2";
  } else if (std::is_same_v<MessageType, TestAllTypesProto3>) {
    return "Proto3";
  } else if (std::is_same_v<MessageType, TestAllTypesProto2Editions>) {
    return "Editions_Proto2";
  } else {
    return "Editions_Proto3";
  }
}

}  // namespace protobuf
}  // namespace google
