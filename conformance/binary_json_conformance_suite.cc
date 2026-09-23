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
std::string longvarint(uint64_t x, int extra) {
  return google::protobuf::conformance::LongVarint(x, extra).str();
}
std::string delim(const std::string& buf) {
  return google::protobuf::conformance::LengthPrefixed(buf).str();
}
std::string u32(uint32_t u32) {
  return google::protobuf::conformance::Fixed32(u32).str();
}
std::string u64(uint64_t u64) {
  return google::protobuf::conformance::Fixed64(u64).str();
}
std::string flt(float f) { return google::protobuf::conformance::Float(f).str(); }
std::string dbl(double d) { return google::protobuf::conformance::Double(d).str(); }
std::string zz32(int32_t x) { return google::protobuf::conformance::SInt32(x).str(); }
std::string zz64(int64_t x) { return google::protobuf::conformance::SInt64(x).str(); }

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

std::string MismatchedWireTypeField(
    int field_number, WireFormatLite::WireType correct_wire_type) {
  if (correct_wire_type == WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
    return absl::StrCat(tag(field_number, WireFormatLite::WIRETYPE_VARINT),
                        varint(1));
  }
  return absl::StrCat(
      tag(field_number, WireFormatLite::WIRETYPE_LENGTH_DELIMITED), delim("a"));
}

bool IsProto3Default(FieldDescriptor::Type type,
                     const std::string& binary_data) {
  switch (type) {
    case FieldDescriptor::TYPE_DOUBLE:
      return binary_data == dbl(0);
    case FieldDescriptor::TYPE_FLOAT:
      return binary_data == flt(0);
    case FieldDescriptor::TYPE_BOOL:
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_ENUM:
      return binary_data == varint(0);
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_SFIXED64:
      return binary_data == u64(0);
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_SFIXED32:
      return binary_data == u32(0);
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES:
      return binary_data == delim("");
    default:
      return false;
  }
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
void BinaryAndJsonConformanceSuite::RunValidProtobufTest(
    const std::string& test_name, ConformanceLevel level,
    const std::string& input_protobuf,
    const std::string& equivalent_text_format) {
  MessageType prototype;

  ConformanceRequestSetting binary_to_binary(
      level, ::conformance::PROTOBUF, ::conformance::PROTOBUF,
      ::conformance::BINARY_TEST, prototype, test_name, input_protobuf);
  RunValidInputTest(binary_to_binary, equivalent_text_format);

  ConformanceRequestSetting binary_to_json(
      level, ::conformance::PROTOBUF, ::conformance::JSON,
      ::conformance::BINARY_TEST, prototype, test_name, input_protobuf);
  RunValidInputTest(binary_to_json, equivalent_text_format);
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
void BinaryAndJsonConformanceSuiteImpl<MessageType>::RunValidProtobufTest(
    const std::string& test_name, ConformanceLevel level,
    const std::string& input_protobuf,
    const std::string& equivalent_text_format) {
  suite_.RunValidProtobufTest<MessageType>(test_name, level, input_protobuf,
                                           equivalent_text_format);
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::RunValidBinaryProtobufTest(
    const std::string& test_name, ConformanceLevel level,
    const std::string& input_protobuf, const std::string& expected_protobuf) {
  MessageType prototype;
  ConformanceRequestSetting setting(
      level, ::conformance::PROTOBUF, ::conformance::PROTOBUF,
      ::conformance::BINARY_TEST, prototype, test_name, input_protobuf);
  suite_.RunValidBinaryInputTest(setting, expected_protobuf, true);
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
    FieldDescriptor::Type type,
    std::vector<std::pair<std::string, std::string>> values) {
  const std::string type_name =
      UpperCase(absl::StrCat(".", FieldDescriptor::TypeName(type)));
  WireFormatLite::WireType wire_type = WireFormatLite::WireTypeForFieldType(
      static_cast<WireFormatLite::FieldType>(type));
  const FieldDescriptor* field = GetFieldForType(type, false);
  const FieldDescriptor* rep_field = GetFieldForType(type, true);

  // Test singular data for singular fields.
  for (size_t i = 0; i < values.size(); i++) {
    std::string proto =
        absl::StrCat(tag(field->number(), wire_type), values[i].first);
    // In proto3, default primitive fields should not be encoded.
    std::string expected_proto =
        run_proto3_tests_ && IsProto3Default(field->type(), values[i].second)
            ? ""
            : absl::StrCat(tag(field->number(), wire_type), values[i].second);
    MessageType test_message;
    ABSL_CHECK(test_message.MergeFromString(expected_proto));
    std::string text;
    ABSL_CHECK(TextFormat::PrintToString(test_message, &text));

    RunValidProtobufTest(
        absl::StrCat("ValidDataScalar", type_name, "[", i, "]"), REQUIRED,
        proto, text);
    RunValidBinaryProtobufTest(
        absl::StrCat("ValidDataScalarBinary", type_name, "[", i, "]"),
        RECOMMENDED, proto, expected_proto);
  }

  // Test repeated data for singular fields.
  // For scalar message fields, repeated values are merged, which is tested
  // separately.
  if (type != FieldDescriptor::TYPE_MESSAGE) {
    std::string proto;
    for (size_t i = 0; i < values.size(); i++) {
      absl::StrAppend(&proto, tag(field->number(), wire_type), values[i].first);
    }
    std::string expected_proto =
        absl::StrCat(tag(field->number(), wire_type), values.back().second);
    MessageType test_message;
    ABSL_CHECK(test_message.MergeFromString(expected_proto));
    std::string text;
    ABSL_CHECK(TextFormat::PrintToString(test_message, &text));

    RunValidProtobufTest(absl::StrCat("RepeatedScalarSelectsLast", type_name),
                         REQUIRED, proto, text);
  }

  // Test repeated fields.
  if (FieldDescriptor::IsTypePackable(type)) {
    const FieldDescriptor* packed_field =
        GetFieldForType(type, true, Packed::kPacked);
    const FieldDescriptor* unpacked_field =
        GetFieldForType(type, true, Packed::kUnpacked);

    std::string default_proto_packed;
    std::string default_proto_unpacked;
    std::string default_proto_packed_expected;
    std::string default_proto_unpacked_expected;
    std::string packed_proto_packed;
    std::string packed_proto_unpacked;
    std::string packed_proto_expected;
    std::string unpacked_proto_packed;
    std::string unpacked_proto_unpacked;
    std::string unpacked_proto_expected;

    for (size_t i = 0; i < values.size(); i++) {
      absl::StrAppend(&default_proto_unpacked,
                      tag(rep_field->number(), wire_type), values[i].first);
      absl::StrAppend(&default_proto_unpacked_expected,
                      tag(rep_field->number(), wire_type), values[i].second);
      default_proto_packed += values[i].first;
      default_proto_packed_expected += values[i].second;
      absl::StrAppend(&packed_proto_unpacked,
                      tag(packed_field->number(), wire_type), values[i].first);
      packed_proto_packed += values[i].first;
      packed_proto_expected += values[i].second;
      absl::StrAppend(&unpacked_proto_unpacked,
                      tag(unpacked_field->number(), wire_type),
                      values[i].first);
      unpacked_proto_packed += values[i].first;
      absl::StrAppend(&unpacked_proto_expected,
                      tag(unpacked_field->number(), wire_type),
                      values[i].second);
    }
    default_proto_packed = absl::StrCat(
        tag(rep_field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
        delim(default_proto_packed));
    default_proto_packed_expected = absl::StrCat(
        tag(rep_field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
        delim(default_proto_packed_expected));
    packed_proto_packed = absl::StrCat(
        tag(packed_field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
        delim(packed_proto_packed));
    packed_proto_expected = absl::StrCat(
        tag(packed_field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
        delim(packed_proto_expected));
    unpacked_proto_packed =
        absl::StrCat(tag(unpacked_field->number(),
                         WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
                     delim(unpacked_proto_packed));

    MessageType test_message;
    ABSL_CHECK(test_message.MergeFromString(default_proto_packed_expected));
    std::string text;
    ABSL_CHECK(TextFormat::PrintToString(test_message, &text));

    // Ensures both packed and unpacked data can be parsed.
    RunValidProtobufTest(
        absl::StrCat("ValidDataRepeated", type_name, ".UnpackedInput"),
        REQUIRED, default_proto_unpacked, text);
    RunValidProtobufTest(
        absl::StrCat("ValidDataRepeated", type_name, ".PackedInput"), REQUIRED,
        default_proto_packed, text);

    // proto2 should encode as unpacked by default and proto3 should encode as
    // packed by default.
    std::string expected_proto = rep_field->is_packed()
                                     ? default_proto_packed_expected
                                     : default_proto_unpacked_expected;
    RunValidBinaryProtobufTest(absl::StrCat("ValidDataRepeated", type_name,
                                            ".UnpackedInput.DefaultOutput"),
                               RECOMMENDED, default_proto_unpacked,
                               expected_proto);
    RunValidBinaryProtobufTest(absl::StrCat("ValidDataRepeated", type_name,
                                            ".PackedInput.DefaultOutput"),
                               RECOMMENDED, default_proto_packed,
                               expected_proto);
    RunValidBinaryProtobufTest(absl::StrCat("ValidDataRepeated", type_name,
                                            ".UnpackedInput.PackedOutput"),
                               RECOMMENDED, packed_proto_unpacked,
                               packed_proto_expected);
    RunValidBinaryProtobufTest(absl::StrCat("ValidDataRepeated", type_name,
                                            ".PackedInput.PackedOutput"),
                               RECOMMENDED, packed_proto_packed,
                               packed_proto_expected);
    RunValidBinaryProtobufTest(absl::StrCat("ValidDataRepeated", type_name,
                                            ".UnpackedInput.UnpackedOutput"),
                               RECOMMENDED, unpacked_proto_unpacked,
                               unpacked_proto_expected);
    RunValidBinaryProtobufTest(absl::StrCat("ValidDataRepeated", type_name,
                                            ".PackedInput.UnpackedOutput"),
                               RECOMMENDED, unpacked_proto_packed,
                               unpacked_proto_expected);
  } else {
    std::string proto;
    std::string expected_proto;
    for (size_t i = 0; i < values.size(); i++) {
      proto +=
          absl::StrCat(tag(rep_field->number(), wire_type), values[i].first);
      expected_proto +=
          absl::StrCat(tag(rep_field->number(), wire_type), values[i].second);
    }
    MessageType test_message;
    ABSL_CHECK(test_message.MergeFromString(expected_proto));
    std::string text;
    ABSL_CHECK(TextFormat::PrintToString(test_message, &text));

    RunValidProtobufTest(absl::StrCat("ValidDataRepeated", type_name), REQUIRED,
                         proto, text);
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
    RunValidProtobufTest(absl::StrCat("ValidDataMap", key_type_name,
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
    RunValidProtobufTest(absl::StrCat("ValidDataMap", key_type_name,
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
    RunValidProtobufTest(absl::StrCat("ValidDataMap", key_type_name,
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
    RunValidProtobufTest(absl::StrCat("ValidDataMap", key_type_name,
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
    RunValidProtobufTest(absl::StrCat("ValidDataMap", key_type_name,
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
    RunValidProtobufTest(
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
    RunValidProtobufTest(
        absl::StrCat("ValidDataMap", key_type_name, value_type_name,
                     ".DuplicateValueInMapEntry"),
        REQUIRED, proto, text);
  }
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<
    MessageType>::RunMapEntryWireTypeMismatchTest(const std::string& test_name,
                                                  const std::string& proto) {
  MessageType prototype;
  ConformanceRequestSetting setting(
      RECOMMENDED, ::conformance::PROTOBUF, ::conformance::PROTOBUF,
      ::conformance::BINARY_TEST, prototype, test_name, proto);
  const ConformanceRequest& request = setting.GetRequest();
  ConformanceResponse response;
  if (!suite_.RunTest(setting.GetTestName(), request, &response)) {
    return;
  }

  TestStatus test;
  test.set_name(setting.GetTestName());

  if (response.result_case() == ConformanceResponse::kSkipped) {
    suite_.ReportSkip(test, request, response);
  } else if (response.result_case() == ConformanceResponse::kParseError) {
    suite_.ReportSuccess(test);
  } else if (response.result_case() == ConformanceResponse::kProtobufPayload) {
    MessageType reference;
    ABSL_CHECK(reference.MergeFromString(proto));
    MessageType response_message;
    if (suite_.ParseResponse(response, setting, &response_message)) {
      if (google::protobuf::util::MessageDifferencer::Equals(reference,
                                                   response_message)) {
        suite_.ReportSuccess(test);
      } else {
        test.set_failure_message(
            "Output was not equivalent to reference message; the mismatched "
            "field appears to have been decoded as a value.");
        suite_.ReportFailure(test, setting.GetLevel(), request, response);
      }
    }
  } else {
    test.set_failure_message(
        "Should have failed to parse or matched expected output but did "
        "not.");
    suite_.ReportFailure(test, setting.GetLevel(), request, response);
  }
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::
    TestMapEntryWireTypeMismatch(FieldDescriptor::Type key_type,
                                 FieldDescriptor::Type value_type) {
  const std::string key_type_name =
      UpperCase(absl::StrCat(".", FieldDescriptor::TypeName(key_type)));
  const std::string value_type_name =
      UpperCase(absl::StrCat(".", FieldDescriptor::TypeName(value_type)));
  const WireFormatLite::WireType key_wire_type =
      WireFormatLite::WireTypeForFieldType(
          static_cast<WireFormatLite::FieldType>(key_type));
  const WireFormatLite::WireType value_wire_type =
      WireFormatLite::WireTypeForFieldType(
          static_cast<WireFormatLite::FieldType>(value_type));

  const std::string good_key_data =
      absl::StrCat(tag(1, key_wire_type), GetNonDefaultValue(key_type));
  const std::string good_value_data =
      absl::StrCat(tag(2, value_wire_type), GetNonDefaultValue(value_type));

  const FieldDescriptor* field = GetFieldForMapType(key_type, value_type);

  // A map entry whose key is encoded with the wrong wire type; the value is
  // still well formed.
  RunMapEntryWireTypeMismatchTest(
      absl::StrCat("ValidDataMap", key_type_name, value_type_name,
                   ".KeyWireTypeMismatch"),
      absl::StrCat(
          tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
          delim(absl::StrCat(MismatchedWireTypeField(1, key_wire_type),
                             good_value_data))));

  // A map entry whose value is encoded with the wrong wire type; the key is
  // still well formed.
  RunMapEntryWireTypeMismatchTest(
      absl::StrCat("ValidDataMap", key_type_name, value_type_name,
                   ".ValueWireTypeMismatch"),
      absl::StrCat(
          tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
          delim(absl::StrCat(good_key_data,
                             MismatchedWireTypeField(2, value_wire_type)))));
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
  const std::string type_name =
      UpperCase(absl::StrCat(".", FieldDescriptor::TypeName(type)));
  WireFormatLite::WireType wire_type = WireFormatLite::WireTypeForFieldType(
      static_cast<WireFormatLite::FieldType>(type));

  const FieldDescriptor* field = GetFieldForOneofType(type);
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

    RunValidProtobufTest(
        absl::StrCat("ValidDataOneof", type_name, ".DefaultValue"), REQUIRED,
        proto, text);
    RunValidBinaryProtobufTest(
        absl::StrCat("ValidDataOneofBinary", type_name, ".DefaultValue"),
        RECOMMENDED, proto, proto);
  }

  {
    // Tests oneof with non-default value.
    const std::string& proto = non_default_value;
    MessageType test_message;
    ABSL_CHECK(test_message.MergeFromString(proto));
    std::string text;
    ABSL_CHECK(TextFormat::PrintToString(test_message, &text));

    RunValidProtobufTest(
        absl::StrCat("ValidDataOneof", type_name, ".NonDefaultValue"), REQUIRED,
        proto, text);
    RunValidBinaryProtobufTest(
        absl::StrCat("ValidDataOneofBinary", type_name, ".NonDefaultValue"),
        RECOMMENDED, proto, proto);
  }

  {
    // Tests oneof with multiple values of the same field.
    const std::string proto = absl::StrCat(default_value, non_default_value);
    const std::string& expected_proto = non_default_value;
    MessageType test_message;
    ABSL_CHECK(test_message.MergeFromString(expected_proto));
    std::string text;
    ABSL_CHECK(TextFormat::PrintToString(test_message, &text));

    RunValidProtobufTest(absl::StrCat("ValidDataOneof", type_name,
                                      ".MultipleValuesForSameField"),
                         REQUIRED, proto, text);
    RunValidBinaryProtobufTest(absl::StrCat("ValidDataOneofBinary", type_name,
                                            ".MultipleValuesForSameField"),
                               RECOMMENDED, proto, expected_proto);
  }

  {
    // Tests oneof with multiple values of the different fields.
    const FieldDescriptor* other_field = GetFieldForOneofType(type, true);
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

    RunValidProtobufTest(absl::StrCat("ValidDataOneof", type_name,
                                      ".MultipleValuesForDifferentField"),
                         REQUIRED, proto, text);
    RunValidBinaryProtobufTest(absl::StrCat("ValidDataOneofBinary", type_name,
                                            ".MultipleValuesForDifferentField"),
                               RECOMMENDED, proto, expected_proto);
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
    int64_t kInt64Min = -9223372036854775808ULL;
    int64_t kInt64Max = 9223372036854775807ULL;
    uint64_t kUint64Max = 18446744073709551615ULL;
    int32_t kInt32Max = 2147483647;
    int32_t kInt32Min = -2147483648;
    uint32_t kUint32Max = 4294967295UL;

    TestValidDataForType(
        FieldDescriptor::TYPE_DOUBLE,
        {
            {dbl(0), dbl(0)},
            {dbl(0.1), dbl(0.1)},
            {dbl(1.7976931348623157e+308), dbl(1.7976931348623157e+308)},
            {dbl(2.22507385850720138309e-308),
             dbl(2.22507385850720138309e-308)},
        });
    TestValidDataForType(
        FieldDescriptor::TYPE_FLOAT,
        {
            {flt(0), flt(0)},
            {flt(0.1), flt(0.1)},
            {flt(1.00000075e-36), flt(1.00000075e-36)},
            {flt(3.402823e+38), flt(3.402823e+38)},  // 3.40282347e+38
            {flt(1.17549435e-38f), flt(1.17549435e-38)},
        });
    TestValidDataForType(FieldDescriptor::TYPE_INT64,
                         {
                             {varint(0), varint(0)},
                             {varint(12345), varint(12345)},
                             {varint(kInt64Max), varint(kInt64Max)},
                             {varint(kInt64Min), varint(kInt64Min)},
                         });
    TestValidDataForType(FieldDescriptor::TYPE_UINT64,
                         {
                             {varint(0), varint(0)},
                             {varint(12345), varint(12345)},
                             {varint(kUint64Max), varint(kUint64Max)},
                         });
    TestValidDataForType(FieldDescriptor::TYPE_INT32,
                         {
                             {varint(0), varint(0)},
                             {varint(12345), varint(12345)},
                             {longvarint(12345, 2), varint(12345)},
                             {longvarint(12345, 7), varint(12345)},
                             {varint(kInt32Max), varint(kInt32Max)},
                             {varint(kInt32Min), varint(kInt32Min)},
                             {varint(1LL << 33), varint(0)},
                             {varint((1LL << 33) - 1), varint(-1)},
                             {varint(kInt64Max), varint(-1)},
                             {varint(kInt64Min + 1), varint(1)},
                         });
    TestValidDataForType(
        FieldDescriptor::TYPE_UINT32,
        {
            {varint(0), varint(0)},
            {varint(12345), varint(12345)},
            {longvarint(12345, 2), varint(12345)},
            {longvarint(12345, 7), varint(12345)},
            {varint(kUint32Max), varint(kUint32Max)},  // UINT32_MAX
            {varint(1LL << 33), varint(0)},
            {varint((1LL << 33) + 1), varint(1)},
            {varint((1LL << 33) - 1), varint((1LL << 32) - 1)},
            {varint(kInt64Max), varint((1LL << 32) - 1)},
            {varint(kInt64Min + 1), varint(1)},
        });
    TestValidDataForType(FieldDescriptor::TYPE_FIXED64,
                         {
                             {u64(0), u64(0)},
                             {u64(12345), u64(12345)},
                             {u64(kUint64Max), u64(kUint64Max)},
                         });
    TestValidDataForType(FieldDescriptor::TYPE_FIXED32,
                         {
                             {u32(0), u32(0)},
                             {u32(12345), u32(12345)},
                             {u32(kUint32Max), u32(kUint32Max)},  // UINT32_MAX
                         });
    TestValidDataForType(FieldDescriptor::TYPE_SFIXED64,
                         {
                             {u64(0), u64(0)},
                             {u64(12345), u64(12345)},
                             {u64(kInt64Max), u64(kInt64Max)},
                             {u64(kInt64Min), u64(kInt64Min)},
                         });
    TestValidDataForType(FieldDescriptor::TYPE_SFIXED32,
                         {
                             {u32(0), u32(0)},
                             {u32(12345), u32(12345)},
                             {u32(kInt32Max), u32(kInt32Max)},
                             {u32(kInt32Min), u32(kInt32Min)},
                         });
    // Bools should be serialized as 0 for false and 1 for true. Parsers should
    // also interpret any nonzero value as true.
    TestValidDataForType(FieldDescriptor::TYPE_BOOL,
                         {
                             {varint(0), varint(0)},
                             {varint(1), varint(1)},
                             {varint(-1), varint(1)},
                             {varint(12345678), varint(1)},
                             {varint(1LL << 33), varint(1)},
                             {varint(kInt64Max), varint(1)},
                             {varint(kInt64Min), varint(1)},
                         });
    TestValidDataForType(FieldDescriptor::TYPE_SINT32,
                         {
                             {zz32(0), zz32(0)},
                             {zz32(12345), zz32(12345)},
                             {zz32(kInt32Max), zz32(kInt32Max)},
                             {zz32(kInt32Min), zz32(kInt32Min)},
                             {zz64(kInt32Max + 2LL), zz32(1)},
                         });
    TestValidDataForType(FieldDescriptor::TYPE_SINT64,
                         {
                             {zz64(0), zz64(0)},
                             {zz64(12345), zz64(12345)},
                             {zz64(kInt64Max), zz64(kInt64Max)},
                             {zz64(kInt64Min), zz64(kInt64Min)},
                         });
    TestValidDataForType(
        FieldDescriptor::TYPE_STRING,
        {
            {delim(""), delim("")},
            {delim("Hello world!"), delim("Hello world!")},
            {delim("\'\"\?\\\a\b\f\n\r\t\v"),
             delim("\'\"\?\\\a\b\f\n\r\t\v")},  // escape
            // U+8C37 U+6B4C ("Google" in Chinese), as UTF-8.
            {delim("\xE8\xB0\xB7\xE6\xAD\x8C"),
             delim("\xE8\xB0\xB7\xE6\xAD\x8C")},
            // U+1F601 (grinning face with smiling eyes), as UTF-8.
            {delim("\xF0\x9F\x98\x81"), delim("\xF0\x9F\x98\x81")},
        });
    TestValidDataForType(FieldDescriptor::TYPE_BYTES,
                         {
                             {delim(""), delim("")},
                             {delim("Hello world!"), delim("Hello world!")},
                             {delim("\x01\x02"), delim("\x01\x02")},
                             {delim("\xfb"), delim("\xfb")},
                         });
    TestValidDataForType(FieldDescriptor::TYPE_ENUM,
                         {
                             {varint(0), varint(0)},
                             {varint(1), varint(1)},
                             {varint(2), varint(2)},
                             {varint(-1), varint(-1)},
                             {varint(kInt64Max), varint(-1)},
                             {varint(kInt64Min + 1), varint(1)},
                         });
    TestValidDataForRepeatedScalarMessage();
    TestValidDataForType(
        FieldDescriptor::TYPE_MESSAGE,
        {
            {delim(""), delim("")},
            {delim(absl::StrCat(tag(1, WireFormatLite::WIRETYPE_VARINT),
                                varint(1234))),
             delim(absl::StrCat(tag(1, WireFormatLite::WIRETYPE_VARINT),
                                varint(1234)))},
        });

    TestValidDataForMapType(FieldDescriptor::TYPE_INT32,
                            FieldDescriptor::TYPE_INT32);
    TestValidDataForMapType(FieldDescriptor::TYPE_INT64,
                            FieldDescriptor::TYPE_INT64);
    TestValidDataForMapType(FieldDescriptor::TYPE_UINT32,
                            FieldDescriptor::TYPE_UINT32);
    TestValidDataForMapType(FieldDescriptor::TYPE_UINT64,
                            FieldDescriptor::TYPE_UINT64);
    TestValidDataForMapType(FieldDescriptor::TYPE_SINT32,
                            FieldDescriptor::TYPE_SINT32);
    TestValidDataForMapType(FieldDescriptor::TYPE_SINT64,
                            FieldDescriptor::TYPE_SINT64);
    TestValidDataForMapType(FieldDescriptor::TYPE_FIXED32,
                            FieldDescriptor::TYPE_FIXED32);
    TestValidDataForMapType(FieldDescriptor::TYPE_FIXED64,
                            FieldDescriptor::TYPE_FIXED64);
    TestValidDataForMapType(FieldDescriptor::TYPE_SFIXED32,
                            FieldDescriptor::TYPE_SFIXED32);
    TestValidDataForMapType(FieldDescriptor::TYPE_SFIXED64,
                            FieldDescriptor::TYPE_SFIXED64);
    TestValidDataForMapType(FieldDescriptor::TYPE_INT32,
                            FieldDescriptor::TYPE_FLOAT);
    TestValidDataForMapType(FieldDescriptor::TYPE_INT32,
                            FieldDescriptor::TYPE_DOUBLE);
    TestValidDataForMapType(FieldDescriptor::TYPE_BOOL,
                            FieldDescriptor::TYPE_BOOL);
    TestValidDataForMapType(FieldDescriptor::TYPE_STRING,
                            FieldDescriptor::TYPE_STRING);
    TestValidDataForMapType(FieldDescriptor::TYPE_STRING,
                            FieldDescriptor::TYPE_BYTES);
    TestValidDataForMapType(FieldDescriptor::TYPE_STRING,
                            FieldDescriptor::TYPE_ENUM);
    TestValidDataForMapType(FieldDescriptor::TYPE_STRING,
                            FieldDescriptor::TYPE_MESSAGE);
    // Additional test to check overwriting message value map.
    TestOverwriteMessageValueMap();

    TestMapEntryWireTypeMismatch(FieldDescriptor::TYPE_INT32,
                                 FieldDescriptor::TYPE_INT32);
    TestMapEntryWireTypeMismatch(FieldDescriptor::TYPE_FIXED32,
                                 FieldDescriptor::TYPE_FIXED32);
    TestMapEntryWireTypeMismatch(FieldDescriptor::TYPE_BOOL,
                                 FieldDescriptor::TYPE_BOOL);
    TestMapEntryWireTypeMismatch(FieldDescriptor::TYPE_STRING,
                                 FieldDescriptor::TYPE_STRING);
    TestMapEntryWireTypeMismatch(FieldDescriptor::TYPE_STRING,
                                 FieldDescriptor::TYPE_MESSAGE);

    TestValidDataForOneofType(FieldDescriptor::TYPE_UINT32);
    TestValidDataForOneofType(FieldDescriptor::TYPE_BOOL);
    TestValidDataForOneofType(FieldDescriptor::TYPE_UINT64);
    TestValidDataForOneofType(FieldDescriptor::TYPE_FLOAT);
    TestValidDataForOneofType(FieldDescriptor::TYPE_DOUBLE);
    TestValidDataForOneofType(FieldDescriptor::TYPE_STRING);
    TestValidDataForOneofType(FieldDescriptor::TYPE_BYTES);
    TestValidDataForOneofType(FieldDescriptor::TYPE_ENUM);
    TestValidDataForOneofType(FieldDescriptor::TYPE_MESSAGE);
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
  RunJsonTestsForNonRepeatedTypes();
  RunJsonTestsForRepeatedTypes();

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
    MessageType>::RunJsonTestsForNonRepeatedTypes() {
  // The integer field tests live in json_integer_test.cc, the bool field tests
  // in json_bool_test.cc, the float and double field tests in
  // json_float_test.cc, the enum field tests in json_enum_test.cc, the string
  // and bytes field tests in json_string_test.cc and the message field test in
  // json_message_test.cc (the gtest-based suite; see BUILD).

  // Oneof fields.
  RunValidJsonTestOrParseFailure("OneofFieldDuplicate", REQUIRED,
                                 R"({"oneofUint32": 1, "oneofString": "test"})",
                                 "oneof_string: \"test\"");
  RunValidJsonTestOrParseFailure("OneofFieldDuplicate2", REQUIRED,
                                 R"({"oneofString": "test", "oneofUint32": 1})",
                                 "oneof_uint32: 1");
  RunValidJsonTest("OneofFieldNullFirst", REQUIRED,
                   R"({"oneofUint32": null, "oneofString": "test"})",
                   "oneof_string: \"test\"");
  RunValidJsonTest("OneofFieldNullSecond", REQUIRED,
                   R"({"oneofString": "test", "oneofUint32": null})",
                   "oneof_string: \"test\"");
  RunValidJsonTest("OneofZeroUint32", RECOMMENDED, R"({"oneofUint32": 0})",
                   "oneof_uint32: 0");
  RunValidJsonTest("OneofZeroMessage", RECOMMENDED,
                   R"({"oneofNestedMessage": {}})", "oneof_nested_message: {}");
  RunValidJsonTest("OneofZeroString", RECOMMENDED, R"({"oneofString": ""})",
                   "oneof_string: \"\"");
  RunValidJsonTest("OneofZeroBytes", RECOMMENDED, R"({"oneofBytes": ""})",
                   "oneof_bytes: \"\"");
  RunValidJsonTest("OneofZeroBool", RECOMMENDED, R"({"oneofBool": false})",
                   "oneof_bool: false");
  RunValidJsonTest("OneofZeroUint64", RECOMMENDED, R"({"oneofUint64": 0})",
                   "oneof_uint64: 0");
  RunValidJsonTest("OneofZeroFloat", RECOMMENDED, R"({"oneofFloat": 0.0})",
                   "oneof_float: 0");
  RunValidJsonTest("OneofZeroDouble", RECOMMENDED, R"({"oneofDouble": 0.0})",
                   "oneof_double: 0");
  RunValidJsonTest("OneofZeroEnum", RECOMMENDED, R"({"oneofEnum":"FOO"})",
                   "oneof_enum: FOO");

  // Map fields.
  RunValidJsonTest("Int32MapField", REQUIRED,
                   R"({"mapInt32Int32": {"1": 2, "3": 4}})",
                   "map_int32_int32: {key: 1 value: 2}"
                   "map_int32_int32: {key: 3 value: 4}");
  ExpectParseFailureForJson("Int32MapFieldKeyNotQuoted", RECOMMENDED,
                            R"({"mapInt32Int32": {1: 2, 3: 4}})");
  RunValidJsonTest("Uint32MapField", REQUIRED,
                   R"({"mapUint32Uint32": {"1": 2, "3": 4}})",
                   "map_uint32_uint32: {key: 1 value: 2}"
                   "map_uint32_uint32: {key: 3 value: 4}");
  ExpectParseFailureForJson("Uint32MapFieldKeyNotQuoted", RECOMMENDED,
                            R"({"mapUint32Uint32": {1: 2, 3: 4}})");
  RunValidJsonTest("Int64MapField", REQUIRED,
                   R"({"mapInt64Int64": {"1": 2, "3": 4}})",
                   "map_int64_int64: {key: 1 value: 2}"
                   "map_int64_int64: {key: 3 value: 4}");
  ExpectParseFailureForJson("Int64MapFieldKeyNotQuoted", RECOMMENDED,
                            R"({"mapInt64Int64": {1: 2, 3: 4}})");
  RunValidJsonTest("Uint64MapField", REQUIRED,
                   R"({"mapUint64Uint64": {"1": 2, "3": 4}})",
                   "map_uint64_uint64: {key: 1 value: 2}"
                   "map_uint64_uint64: {key: 3 value: 4}");
  ExpectParseFailureForJson("Uint64MapFieldKeyNotQuoted", RECOMMENDED,
                            R"({"mapUint64Uint64": {1: 2, 3: 4}})");
  RunValidJsonTest("BoolMapField", REQUIRED,
                   R"({"mapBoolBool": {"true": true, "false": false}})",
                   "map_bool_bool: {key: true value: true}"
                   "map_bool_bool: {key: false value: false}");
  ExpectParseFailureForJson("BoolMapFieldKeyNotQuoted", RECOMMENDED,
                            R"({"mapBoolBool": {true: true, false: false}})");
  RunValidJsonTest("MessageMapField", REQUIRED,
                   R"({
        "mapStringNestedMessage": {
          "hello": {"a": 1234},
          "world": {"a": 5678}
  }
      })",
                   R"(
        map_string_nested_message: {
          key: "hello"
          value: {a: 1234}
  }
        map_string_nested_message: {
          key: "world"
          value: {a: 5678}
  }
      )");
  // Since Map keys are represented as JSON strings, escaping should be allowed.
  RunValidJsonTest("Int32MapEscapedKey", REQUIRED,
                   R"({"mapInt32Int32": {"\u0031": 2}})",
                   "map_int32_int32: {key: 1 value: 2}");
  RunValidJsonTest("Int64MapEscapedKey", REQUIRED,
                   R"({"mapInt64Int64": {"\u0031": 2}})",
                   "map_int64_int64: {key: 1 value: 2}");
  RunValidJsonTest("BoolMapEscapedKey", REQUIRED,
                   R"({"mapBoolBool": {"tr\u0075e": true}})",
                   "map_bool_bool: {key: true value: true}");
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<
    MessageType>::RunJsonTestsForRepeatedTypes() {
  // Repeated fields.
  RunValidJsonTest("PrimitiveRepeatedField", REQUIRED,
                   R"({"repeatedInt32": [1, 2, 3, 4]})",
                   "repeated_int32: [1, 2, 3, 4]");
  RunValidJsonTest("EnumRepeatedField", REQUIRED,
                   R"({"repeatedNestedEnum": ["FOO", "BAR", "BAZ"]})",
                   "repeated_nested_enum: [FOO, BAR, BAZ]");
  RunValidJsonTest("StringRepeatedField", REQUIRED,
                   R"({"repeatedString": ["Hello", "world"]})",
                   R"(repeated_string: ["Hello", "world"])");
  RunValidJsonTest("BytesRepeatedField", REQUIRED,
                   R"({"repeatedBytes": ["AAEC", "AQI="]})",
                   R"(repeated_bytes: ["\x00\x01\x02", "\x01\x02"])");
  RunValidJsonTest("MessageRepeatedField", REQUIRED,
                   R"({"repeatedNestedMessage": [{"a": 1234}, {"a": 5678}]})",
                   "repeated_nested_message: {a: 1234}"
                   "repeated_nested_message: {a: 5678}");

  // Repeated field elements are of incorrect type.
  ExpectParseFailureForJson(
      "RepeatedFieldWrongElementTypeExpectingIntegersGotBool", REQUIRED,
      R"({"repeatedInt32": [1, false, 3, 4]})");
  ExpectParseFailureForJson(
      "RepeatedFieldWrongElementTypeExpectingIntegersGotString", REQUIRED,
      R"({"repeatedInt32": [1, 2, "name", 4]})");
  ExpectParseFailureForJson(
      "RepeatedFieldWrongElementTypeExpectingIntegersGotMessage", REQUIRED,
      R"({"repeatedInt32": [1, 2, 3, {"a": 4}]})");
  ExpectParseFailureForJson(
      "RepeatedFieldWrongElementTypeExpectingStringsGotInt", REQUIRED,
      R"({"repeatedString": ["1", 2, "3", "4"]})");
  ExpectParseFailureForJson(
      "RepeatedFieldWrongElementTypeExpectingStringsGotBool", REQUIRED,
      R"({"repeatedString": ["1", "2", false, "4"]})");
  ExpectParseFailureForJson(
      "RepeatedFieldWrongElementTypeExpectingStringsGotMessage", REQUIRED,
      R"({"repeatedString": ["1", 2, "3", {"a": 4}]})");
  ExpectParseFailureForJson(
      "RepeatedFieldWrongElementTypeExpectingMessagesGotInt", REQUIRED,
      R"({"repeatedNestedMessage": [{"a": 1}, 2]})");
  ExpectParseFailureForJson(
      "RepeatedFieldWrongElementTypeExpectingMessagesGotBool", REQUIRED,
      R"({"repeatedNestedMessage": [{"a": 1}, false]})");
  ExpectParseFailureForJson(
      "RepeatedFieldWrongElementTypeExpectingMessagesGotString", REQUIRED,
      R"({"repeatedNestedMessage": [{"a": 1}, "2"]})");

  // A singular field where a repeated field was expected is not allowed, even
  // if it is the right type.
  ExpectParseFailureForJson("SingleValueForRepeatedFieldInt32", REQUIRED,
                            R"({"repeatedInt32": 1})");
  ExpectParseFailureForJson("SingleValueForRepeatedFieldMessage", REQUIRED,
                            R"({"repeatedNestedMessage": {"a": 1}})");

  // Trailing comma in the repeated field is not allowed.
  ExpectParseFailureForJson("RepeatedFieldTrailingComma", RECOMMENDED,
                            R"({"repeatedInt32": [1, 2, 3, 4,]})");
  ExpectParseFailureForJson("RepeatedFieldTrailingCommaWithSpace", RECOMMENDED,
                            "{\"repeatedInt32\": [1, 2, 3, 4 ,]}");
  ExpectParseFailureForJson("RepeatedFieldTrailingCommaWithSpaceCommaSpace",
                            RECOMMENDED,
                            "{\"repeatedInt32\": [1, 2, 3, 4 , ]}");
  ExpectParseFailureForJson(
      "RepeatedFieldTrailingCommaWithNewlines", RECOMMENDED,
      "{\"repeatedInt32\": [\n  1,\n  2,\n  3,\n  4,\n]}");
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
    FieldDescriptor::Type type, bool repeated, Packed packed) const {
  return ::google::protobuf::conformance::GetFieldForType(*MessageType::GetDescriptor(),
                                                type, repeated, packed);
}

template <typename MessageType>
const FieldDescriptor*
BinaryAndJsonConformanceSuiteImpl<MessageType>::GetFieldForMapType(
    FieldDescriptor::Type key_type, FieldDescriptor::Type value_type) const {
  return ::google::protobuf::conformance::GetFieldForMapType(
      *MessageType::GetDescriptor(), key_type, value_type);
}

template <typename MessageType>
const FieldDescriptor*
BinaryAndJsonConformanceSuiteImpl<MessageType>::GetFieldForOneofType(
    FieldDescriptor::Type type, bool exclusive) const {
  return ::google::protobuf::conformance::GetFieldForOneofType(
      *MessageType::GetDescriptor(), type, exclusive);
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
