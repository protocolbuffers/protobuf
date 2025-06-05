// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "binary_json_conformance_suite.h"

#include <cassert>
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
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "json/config.h"
#include "json/reader.h"
#include "json/value.h"
#include "binary_wireformat.h"
#include "conformance/conformance.pb.h"
#include "conformance_test.h"
#include "conformance/test_protos/test_messages_edition2023.pb.h"
#include "editions/golden/test_messages_proto2_editions.pb.h"
#include "editions/golden/test_messages_proto3_editions.pb.h"
#include "google/protobuf/json/json.h"
#include "google/protobuf/test_messages_proto2.pb.h"
#include "google/protobuf/test_messages_proto3.pb.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/unknown_field_set.h"
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
using protobuf_test_messages::editions::TestAllTypesEdition2023;
using protobuf_test_messages::proto2::TestAllTypesProto2;
using protobuf_test_messages::proto3::TestAllTypesProto3;
using TestAllTypesProto2Editions =
    protobuf_test_messages::editions::proto2::TestAllTypesProto2;
using TestAllTypesProto3Editions =
    protobuf_test_messages::editions::proto3::TestAllTypesProto3;

namespace {

constexpr absl::string_view kTypeUrlPrefix = "type.googleapis.com";

// The number of repetitions to use for performance tests.
// Corresponds approx to 500KB wireformat bytes.
const size_t kPerformanceRepeatCount = 50000;

std::string GetTypeUrl(const Descriptor* message) {
  return absl::StrCat(kTypeUrlPrefix, "/", message->full_name());
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

std::string GetDefaultValue(FieldDescriptor::Type type) {
  switch (type) {
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_ENUM:
    case FieldDescriptor::TYPE_BOOL:
      return varint(0);
    case FieldDescriptor::TYPE_SINT32:
      return zz32(0);
    case FieldDescriptor::TYPE_SINT64:
      return zz64(0);
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_SFIXED32:
      return u32(0);
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_SFIXED64:
      return u64(0);
    case FieldDescriptor::TYPE_FLOAT:
      return flt(0);
    case FieldDescriptor::TYPE_DOUBLE:
      return dbl(0);
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES:
    case FieldDescriptor::TYPE_MESSAGE:
      return delim("");
    default:
      return "";
  }
  return "";
}

std::string GetNonDefaultValue(FieldDescriptor::Type type) {
  switch (type) {
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_ENUM:
    case FieldDescriptor::TYPE_BOOL:
      return varint(1);
    case FieldDescriptor::TYPE_SINT32:
      return zz32(1);
    case FieldDescriptor::TYPE_SINT64:
      return zz64(1);
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_SFIXED32:
      return u32(1);
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_SFIXED64:
      return u64(1);
    case FieldDescriptor::TYPE_FLOAT:
      return flt(1);
    case FieldDescriptor::TYPE_DOUBLE:
      return dbl(1);
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES:
      return delim("a");
    case FieldDescriptor::TYPE_MESSAGE:
      return delim(
          absl::StrCat(tag(1, WireFormatLite::WIRETYPE_VARINT), varint(1234)));
    default:
      return "";
  }
  return "";
}

#define UNKNOWN_FIELD 666

std::string UpperCase(std::string str) {
  for (size_t i = 0; i < str.size(); i++) {
    str[i] = toupper(str[i]);
  }
  return str;
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
  std::string binary_protobuf;
  absl::Status status =
      json::JsonToBinaryString(type_resolver_.get(), type_url_,
                               response.json_payload(), &binary_protobuf);

  if (!status.ok()) {
    ABSL_LOG(ERROR) << status;
    return false;
  }

  if (!test_message->ParseFromString(binary_protobuf)) {
    ABSL_LOG(FATAL) << "INTERNAL ERROR: internal JSON->protobuf transcode "
                    << "yielded unparseable proto.";
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
  if (!this->performance_) {
    RunMessageSetTests();
  }
  if (maximum_edition_ >= Edition::EDITION_2023) {
    BinaryAndJsonConformanceSuiteImpl<TestAllTypesProto3Editions>(
        this, /*run_proto3_tests=*/true);
    BinaryAndJsonConformanceSuiteImpl<TestAllTypesProto2Editions>(
        this, /*run_proto3_tests=*/false);
    RunDelimitedFieldTests();
  }
}

void BinaryAndJsonConformanceSuite::RunDelimitedFieldTests() {
  TestAllTypesEdition2023 prototype;
  SetTypeUrl(GetTypeUrl(TestAllTypesEdition2023::GetDescriptor()));

  RunValidProtobufTest<TestAllTypesEdition2023>(
      absl::StrCat("ValidNonMessage"), REQUIRED,
      field(1, WireFormatLite::WIRETYPE_VARINT, varint(99)),
      R"pb(optional_int32: 99)pb");

  RunValidProtobufTest<TestAllTypesEdition2023>(
      absl::StrCat("ValidLengthPrefixedField"), REQUIRED,
      len(18, field(1, WireFormatLite::WIRETYPE_VARINT, varint(99))),
      R"pb(optional_nested_message { a: 99 })pb");

  RunValidProtobufTest<TestAllTypesEdition2023>(
      absl::StrCat("ValidMap.Integer"), REQUIRED,
      len(56,
          absl::StrCat(field(1, WireFormatLite::WIRETYPE_VARINT, varint(99)),
                       field(2, WireFormatLite::WIRETYPE_VARINT, varint(87)))),
      R"pb(map_int32_int32 { key: 99 value: 87 })pb");

  RunValidProtobufTest<TestAllTypesEdition2023>(
      absl::StrCat("ValidMap.LengthPrefixed"), REQUIRED,
      len(71, absl::StrCat(len(1, "a"),
                           len(2, field(1, WireFormatLite::WIRETYPE_VARINT,
                                        varint(87))))),
      R"pb(map_string_nested_message {
             key: "a"
             value: { a: 87 }
           })pb");

  RunValidProtobufTest<TestAllTypesEdition2023>(
      absl::StrCat("ValidDelimitedField.GroupLike"), REQUIRED,
      group(201, field(202, WireFormatLite::WIRETYPE_VARINT, varint(99))),
      R"pb(groupliketype { group_int32: 99 })pb");

  RunValidProtobufTest<TestAllTypesEdition2023>(
      absl::StrCat("ValidDelimitedField.NotGroupLike"), REQUIRED,
      group(202, field(202, WireFormatLite::WIRETYPE_VARINT, varint(99))),
      R"pb(delimited_field { group_int32: 99 })pb");

  // Note: extensions don't work with TypeResolver, which is used by
  // binary->JSON tests.
  RunValidBinaryProtobufTest<TestAllTypesEdition2023>(
      absl::StrCat("ValidDelimitedExtension.GroupLike"), REQUIRED,
      group(121, field(1, WireFormatLite::WIRETYPE_VARINT, varint(99))),
      R"pb([protobuf_test_messages.editions.groupliketype] { c: 99 })pb");

  RunValidBinaryProtobufTest<TestAllTypesEdition2023>(
      absl::StrCat("ValidDelimitedExtension.NotGroupLike"), REQUIRED,
      group(122, field(1, WireFormatLite::WIRETYPE_VARINT, varint(99))),
      R"pb([protobuf_test_messages.editions.delimited_ext] { c: 99 })pb");
}

void BinaryAndJsonConformanceSuite::RunMessageSetTests() {
  RunValidBinaryProtobufTest<TestAllTypesProto2>(
      absl::StrCat("ValidMessageSetEncoding"), REQUIRED,
      len(500,
          group(1, absl::StrCat(field(2, WireFormatLite::WIRETYPE_VARINT,
                                      varint(4135312)),
                                len(3, field(9, WireFormatLite::WIRETYPE_VARINT,
                                             varint(99)))))),
      // clang-format off
      R"pb(message_set_correct: {
             [protobuf_test_messages.proto2
                  .TestAllTypesProto2.MessageSetCorrectExtension2]: { i: 99 }
           })pb"
      // clang-format on
  );
  RunValidBinaryProtobufTest<TestAllTypesProto2>(
      absl::StrCat("ValidMessageSetEncoding.OutOfOrderGroupsEntries"), REQUIRED,
      len(500,
          group(1, absl::StrCat(len(3, field(9, WireFormatLite::WIRETYPE_VARINT,
                                             varint(99))),
                                field(2, WireFormatLite::WIRETYPE_VARINT,
                                      varint(4135312))))),
      // clang-format off
      R"pb(message_set_correct: {
             [protobuf_test_messages.proto2
                  .TestAllTypesProto2.MessageSetCorrectExtension2]: { i: 99 }
           })pb"
      // clang-format on
  );

  // Test that an unknown message set extension always goes to unknown fields.
  // This is done by poisoning the extension payload with an entry for field 0.
  RunValidRoundtripProtobufTest<TestAllTypesProto2>(
      "MessageSetEncoding.UnknownExtension", REQUIRED,
      len(500,
          group(1, absl::StrCat(field(2, WireFormatLite::WIRETYPE_VARINT,
                                      varint(4135300)),
                                len(3, field(0, WireFormatLite::WIRETYPE_VARINT,
                                             varint(99)))))));

  // If an encoder is unaware of the message_set_wire_format option it will be
  // encoded like any other extension submessage. Decoders should be able to
  // tolerate this format as well.
  RunValidBinaryProtobufTest<TestAllTypesProto2>(
      absl::StrCat("ValidMessageSetEncoding.SubmessageEncoding"), RECOMMENDED,
      len(500,
          len(4135312, field(9, WireFormatLite::WIRETYPE_VARINT, varint(99)))),
      // clang-format off
      R"pb(message_set_correct: {
             [protobuf_test_messages.proto2
                  .TestAllTypesProto2.MessageSetCorrectExtension2]: { i: 99 }
           })pb"
      // clang-format on
  );

  // Test again, but this time we'll try to detect if the implementation put the
  // submessage encoded entry into the unknown field set. We'll do this by using
  // conflicting oneof entries where order matters when the messages are merged.
  //
  // In a non-compliant implementation submessage encoded messageset entry will
  // be moved to unknown fields and then tacked onto the end of the payload.
  // Thus we'll see field b set first, and then field a.
  //
  // In a compliant implementation we expect the submessage encoded messageset
  // to be read first with field a set, and then the normal message set entry
  // will be read with field b will be set -- thus field b will win.
  RunValidBinaryProtobufTest<TestAllTypesProto2>(
      absl::StrCat("ValidMessageSetEncoding.SubmessageEncoding.NotUnknown"),
      RECOMMENDED,
      len(500, absl::StrCat(
                   len(123456789,
                       field(1, WireFormatLite::WIRETYPE_VARINT, varint(42))),
                   group(1, absl::StrCat(
                                field(2, WireFormatLite::WIRETYPE_VARINT,
                                      varint(123456789)),
                                len(3, field(2, WireFormatLite::WIRETYPE_VARINT,
                                             varint(99))))))),
      // clang-format off
      R"pb(message_set_correct: {
             [protobuf_test_messages.proto2
                  .TestAllTypesProto2.ExtensionWithOneof]: { b: 99 }
           })pb"
      // clang-format on
  );
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::
    ExpectParseFailureForProtoWithProtoVersion(const std::string& proto,
                                               const std::string& test_name,
                                               ConformanceLevel level) {
  MessageType prototype;
  // We don't expect output, but if the program erroneously accepts the protobuf
  // we let it send its response as this.  We must not leave it unspecified.
  ConformanceRequestSetting setting(
      level, ::conformance::PROTOBUF, ::conformance::PROTOBUF,
      ::conformance::BINARY_TEST, prototype, test_name, proto);

  const ConformanceRequest& request = setting.GetRequest();
  ConformanceResponse response;
  std::string effective_test_name =
      absl::StrCat(setting.ConformanceLevelToString(level), ".",
                   setting.GetSyntaxIdentifier(), ".ProtobufInput.", test_name);

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

// Expect that this precise protobuf will cause a parse error.
template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::ExpectParseFailureForProto(
    const std::string& proto, const std::string& test_name,
    ConformanceLevel level) {
  ExpectParseFailureForProtoWithProtoVersion(proto, test_name, level);
}

// Expect that this protobuf will cause a parse error, even if it is followed
// by valid protobuf data.  We can try running this twice: once with this
// data verbatim and once with this data followed by some valid data.
//
// TODO: implement the second of these.
template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<
    MessageType>::ExpectHardParseFailureForProto(const std::string& proto,
                                                 const std::string& test_name,
                                                 ConformanceLevel level) {
  return ExpectParseFailureForProto(proto, test_name, level);
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
void BinaryAndJsonConformanceSuiteImpl<MessageType>::
    RunValidJsonIgnoreUnknownTest(const std::string& test_name,
                                  ConformanceLevel level,
                                  const std::string& input_json,
                                  const std::string& equivalent_text_format) {
  MessageType prototype;
  ConformanceRequestSetting setting(
      level, ::conformance::JSON, ::conformance::PROTOBUF,
      ::conformance::JSON_IGNORE_UNKNOWN_PARSING_TEST, prototype, test_name,
      input_json);
  suite_.RunValidInputTest(setting, equivalent_text_format);
}

template <typename MessageType>
void BinaryAndJsonConformanceSuite::RunValidBinaryProtobufTest(
    const std::string& test_name, ConformanceLevel level,
    const std::string& input_protobuf,
    const std::string& equivalent_text_format) {
  MessageType prototype;

  ConformanceRequestSetting binary_to_binary(
      level, ::conformance::PROTOBUF, ::conformance::PROTOBUF,
      ::conformance::BINARY_TEST, prototype, test_name, input_protobuf);
  RunValidInputTest(binary_to_binary, equivalent_text_format);
}

template <typename MessageType>
void BinaryAndJsonConformanceSuite::RunValidRoundtripProtobufTest(
    const std::string& test_name, ConformanceLevel level,
    const std::string& input_protobuf) {
  MessageType prototype;

  ConformanceRequestSetting binary_to_binary(
      level, ::conformance::PROTOBUF, ::conformance::PROTOBUF,
      ::conformance::BINARY_TEST, prototype, test_name, input_protobuf);
  RunValidBinaryInputTest(binary_to_binary, input_protobuf);
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
    const std::string& input_protobuf) {
  RunValidBinaryProtobufTest(test_name, level, input_protobuf, input_protobuf);
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
void BinaryAndJsonConformanceSuiteImpl<MessageType>::
    RunBinaryPerformanceMergeMessageWithField(const std::string& test_name,
                                              const std::string& field_proto) {
  std::string message_tag = tag(27, WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
  std::string message_proto = absl::StrCat(message_tag, delim(field_proto));

  std::string proto;
  for (size_t i = 0; i < kPerformanceRepeatCount; i++) {
    proto.append(message_proto);
  }

  std::string multiple_repeated_field_proto;
  for (size_t i = 0; i < kPerformanceRepeatCount; i++) {
    multiple_repeated_field_proto.append(field_proto);
  }
  std::string expected_proto =
      absl::StrCat(message_tag, delim(multiple_repeated_field_proto));

  RunValidBinaryProtobufTest(test_name, RECOMMENDED, proto, expected_proto);
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::
    RunValidProtobufTestWithMessage(const std::string& test_name,
                                    ConformanceLevel level,
                                    const Message* input,
                                    const std::string& equivalent_text_format) {
  RunValidProtobufTest(test_name, level, input->SerializeAsString(),
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
  std::string effective_test_name = absl::StrCat(
      setting.ConformanceLevelToString(level), ".",
      setting.GetSyntaxIdentifier(), ".JsonInput.", test_name, ".Validator");

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
  std::string effective_test_name =
      absl::StrCat(setting.ConformanceLevelToString(level), ".",
                   SyntaxIdentifier(), ".", test_name, ".JsonOutput");

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
void BinaryAndJsonConformanceSuiteImpl<MessageType>::TestPrematureEOFForType(
    FieldDescriptor::Type type) {
  // Incomplete values for each wire type.
  static constexpr absl::string_view incompletes[6] = {
      "\x80",     // VARINT
      "abcdefg",  // 64BIT
      "\x80",     // DELIMITED (partial length)
      "",         // START_GROUP (no value required)
      "",         // END_GROUP (no value required)
      "abc"       // 32BIT
  };

  const FieldDescriptor* field = GetFieldForType(type, false);
  const FieldDescriptor* rep_field = GetFieldForType(type, true);
  WireFormatLite::WireType wire_type = WireFormatLite::WireTypeForFieldType(
      static_cast<WireFormatLite::FieldType>(type));
  absl::string_view incomplete = incompletes[wire_type];
  const std::string type_name =
      UpperCase(absl::StrCat(".", FieldDescriptor::TypeName(type)));

  ExpectParseFailureForProto(
      tag(field->number(), wire_type),
      absl::StrCat("PrematureEofBeforeKnownNonRepeatedValue", type_name),
      REQUIRED);

  ExpectParseFailureForProto(
      tag(rep_field->number(), wire_type),
      absl::StrCat("PrematureEofBeforeKnownRepeatedValue", type_name),
      REQUIRED);

  ExpectParseFailureForProto(
      tag(UNKNOWN_FIELD, wire_type),
      absl::StrCat("PrematureEofBeforeUnknownValue", type_name), REQUIRED);

  ExpectParseFailureForProto(
      absl::StrCat(tag(field->number(), wire_type), incomplete),
      absl::StrCat("PrematureEofInsideKnownNonRepeatedValue", type_name),
      REQUIRED);

  ExpectParseFailureForProto(
      absl::StrCat(tag(rep_field->number(), wire_type), incomplete),
      absl::StrCat("PrematureEofInsideKnownRepeatedValue", type_name),
      REQUIRED);

  ExpectParseFailureForProto(
      absl::StrCat(tag(UNKNOWN_FIELD, wire_type), incomplete),
      absl::StrCat("PrematureEofInsideUnknownValue", type_name), REQUIRED);

  if (wire_type == WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
    ExpectParseFailureForProto(
        absl::StrCat(tag(field->number(), wire_type), varint(1)),
        absl::StrCat("PrematureEofInDelimitedDataForKnownNonRepeatedValue",
                     type_name),
        REQUIRED);

    ExpectParseFailureForProto(
        absl::StrCat(tag(rep_field->number(), wire_type), varint(1)),
        absl::StrCat("PrematureEofInDelimitedDataForKnownRepeatedValue",
                     type_name),
        REQUIRED);

    // EOF in the middle of delimited data for unknown value.
    ExpectParseFailureForProto(
        absl::StrCat(tag(UNKNOWN_FIELD, wire_type), varint(1)),
        absl::StrCat("PrematureEofInDelimitedDataForUnknownValue", type_name),
        REQUIRED);

    if (type == FieldDescriptor::TYPE_MESSAGE) {
      // Submessage ends in the middle of a value.
      std::string incomplete_submsg = absl::StrCat(
          tag(WireFormatLite::TYPE_INT32, WireFormatLite::WIRETYPE_VARINT),
          incompletes[WireFormatLite::WIRETYPE_VARINT]);
      ExpectHardParseFailureForProto(
          absl::StrCat(
              tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
              varint(incomplete_submsg.size()), incomplete_submsg),
          absl::StrCat("PrematureEofInSubmessageValue", type_name), REQUIRED);
    }
  } else if (type != FieldDescriptor::TYPE_GROUP) {
    // Non-delimited, non-group: eligible for packing.

    // Packed region ends in the middle of a value.
    ExpectHardParseFailureForProto(
        absl::StrCat(
            tag(rep_field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
            varint(incomplete.size()), incomplete),
        absl::StrCat("PrematureEofInPackedFieldValue", type_name), REQUIRED);

    // EOF in the middle of packed region.
    ExpectParseFailureForProto(
        absl::StrCat(
            tag(rep_field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
            varint(1)),
        absl::StrCat("PrematureEofInPackedField", type_name), REQUIRED);
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
    test_message.MergeFromString(expected_proto);
    std::string text;
    TextFormat::PrintToString(test_message, &text);

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
      proto += absl::StrCat(tag(field->number(), wire_type), values[i].first);
    }
    std::string expected_proto =
        absl::StrCat(tag(field->number(), wire_type), values.back().second);
    MessageType test_message;
    test_message.MergeFromString(expected_proto);
    std::string text;
    TextFormat::PrintToString(test_message, &text);

    RunValidProtobufTest(absl::StrCat("RepeatedScalarSelectsLast", type_name),
                         REQUIRED, proto, text);
  }

  // Test repeated fields.
  if (FieldDescriptor::IsTypePackable(type)) {
    const FieldDescriptor* packed_field =
        GetFieldForType(type, true, Packed::kTrue);
    const FieldDescriptor* unpacked_field =
        GetFieldForType(type, true, Packed::kFalse);

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
      default_proto_unpacked +=
          absl::StrCat(tag(rep_field->number(), wire_type), values[i].first);
      default_proto_unpacked_expected +=
          absl::StrCat(tag(rep_field->number(), wire_type), values[i].second);
      default_proto_packed += values[i].first;
      default_proto_packed_expected += values[i].second;
      packed_proto_unpacked +=
          absl::StrCat(tag(packed_field->number(), wire_type), values[i].first);
      packed_proto_packed += values[i].first;
      packed_proto_expected += values[i].second;
      unpacked_proto_unpacked += absl::StrCat(
          tag(unpacked_field->number(), wire_type), values[i].first);
      unpacked_proto_packed += values[i].first;
      unpacked_proto_expected += absl::StrCat(
          tag(unpacked_field->number(), wire_type), values[i].second);
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
    test_message.MergeFromString(default_proto_packed_expected);
    std::string text;
    TextFormat::PrintToString(test_message, &text);

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
    test_message.MergeFromString(expected_proto);
    std::string text;
    TextFormat::PrintToString(test_message, &text);

    RunValidProtobufTest(absl::StrCat("ValidDataRepeated", type_name), REQUIRED,
                         proto, text);
  }
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<
    MessageType>::TestValidDataForRepeatedScalarMessage() {
  std::vector<std::string> values = {
      delim(absl::StrCat(
          tag(2, WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
          delim(absl::StrCat(
              tag(1, WireFormatLite::WIRETYPE_VARINT), varint(1234),
              tag(2, WireFormatLite::WIRETYPE_VARINT), varint(1234),
              tag(31, WireFormatLite::WIRETYPE_VARINT), varint(1234))))),
      delim(absl::StrCat(
          tag(2, WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
          delim(absl::StrCat(
              tag(1, WireFormatLite::WIRETYPE_VARINT), varint(4321),
              tag(3, WireFormatLite::WIRETYPE_VARINT), varint(4321),
              tag(31, WireFormatLite::WIRETYPE_VARINT), varint(4321))))),
  };

  const std::string expected =
      R"({
        corecursive: {
          optional_int32: 4321,
          optional_int64: 1234,
          optional_uint32: 4321,
          repeated_int32: [1234, 4321],
  }
      })";

  std::string proto;
  const FieldDescriptor* field =
      GetFieldForType(FieldDescriptor::TYPE_MESSAGE, false);
  for (size_t i = 0; i < values.size(); i++) {
    proto += absl::StrCat(
        tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
        values[i]);
  }

  RunValidProtobufTest("RepeatedScalarMessageMerge", REQUIRED, proto,
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
    test_message.MergeFromString(proto);
    std::string text;
    TextFormat::PrintToString(test_message, &text);
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
    test_message.MergeFromString(proto);
    std::string text;
    TextFormat::PrintToString(test_message, &text);
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
    test_message.MergeFromString(proto);
    std::string text;
    TextFormat::PrintToString(test_message, &text);
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
    test_message.MergeFromString(proto);
    std::string text;
    TextFormat::PrintToString(test_message, &text);
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
    test_message.MergeFromString(proto2);
    std::string text;
    TextFormat::PrintToString(test_message, &text);
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
    test_message.MergeFromString(proto);
    std::string text;
    TextFormat::PrintToString(test_message, &text);
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
    test_message.MergeFromString(proto);
    std::string text;
    TextFormat::PrintToString(test_message, &text);
    RunValidProtobufTest(
        absl::StrCat("ValidDataMap", key_type_name, value_type_name,
                     ".DuplicateValueInMapEntry"),
        REQUIRED, proto, text);
  }
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<
    MessageType>::TestOverwriteMessageValueMap() {
  std::string key_data = absl::StrCat(
      tag(1, WireFormatLite::WIRETYPE_LENGTH_DELIMITED), delim(""));
  std::string field1_data =
      absl::StrCat(tag(1, WireFormatLite::WIRETYPE_VARINT), varint(1));
  std::string field2_data =
      absl::StrCat(tag(2, WireFormatLite::WIRETYPE_VARINT), varint(1));
  std::string field31_data =
      absl::StrCat(tag(31, WireFormatLite::WIRETYPE_VARINT), varint(1));
  std::string submsg1_data = delim(absl::StrCat(field1_data, field31_data));
  std::string submsg2_data = delim(absl::StrCat(field2_data, field31_data));
  std::string value1_data = absl::StrCat(
      tag(2, WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
      delim(absl::StrCat(tag(2, WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
                         submsg1_data)));
  std::string value2_data = absl::StrCat(
      tag(2, WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
      delim(absl::StrCat(tag(2, WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
                         submsg2_data)));

  const FieldDescriptor* field = GetFieldForMapType(
      FieldDescriptor::TYPE_STRING, FieldDescriptor::TYPE_MESSAGE);

  std::string proto1 = absl::StrCat(
      tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
      delim(absl::StrCat(key_data, value1_data)));
  std::string proto2 = absl::StrCat(
      tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
      delim(absl::StrCat(key_data, value2_data)));
  std::string proto = absl::StrCat(proto1, proto2);
  MessageType test_message;
  test_message.MergeFromString(proto2);
  std::string text;
  TextFormat::PrintToString(test_message, &text);
  RunValidProtobufTest("ValidDataMap.STRING.MESSAGE.MergeValue", REQUIRED,
                       proto, text);
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
    test_message.MergeFromString(proto);
    std::string text;
    TextFormat::PrintToString(test_message, &text);

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
    test_message.MergeFromString(proto);
    std::string text;
    TextFormat::PrintToString(test_message, &text);

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
    test_message.MergeFromString(expected_proto);
    std::string text;
    TextFormat::PrintToString(test_message, &text);

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
    test_message.MergeFromString(expected_proto);
    std::string text;
    TextFormat::PrintToString(test_message, &text);

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
  std::string field1_data =
      absl::StrCat(tag(1, WireFormatLite::WIRETYPE_VARINT), varint(1));
  std::string field2a_data =
      absl::StrCat(tag(2, WireFormatLite::WIRETYPE_VARINT), varint(1));
  std::string field2b_data =
      absl::StrCat(tag(2, WireFormatLite::WIRETYPE_VARINT), varint(1));
  std::string field89_data =
      absl::StrCat(tag(89, WireFormatLite::WIRETYPE_VARINT), varint(1));
  std::string submsg1_data = absl::StrCat(
      tag(2, WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
      delim(absl::StrCat(field1_data, field2a_data, field89_data)));
  std::string submsg2_data =
      absl::StrCat(tag(2, WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
                   delim(absl::StrCat(field2b_data, field89_data)));
  std::string merged_data =
      absl::StrCat(tag(2, WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
                   delim(absl::StrCat(field1_data, field2b_data, field89_data,
                                      field89_data)));

  const FieldDescriptor* field =
      GetFieldForOneofType(FieldDescriptor::TYPE_MESSAGE);

  std::string proto1 = absl::StrCat(
      tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
      delim(submsg1_data));
  std::string proto2 = absl::StrCat(
      tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
      delim(submsg2_data));
  std::string proto = absl::StrCat(proto1, proto2);
  std::string expected_proto = absl::StrCat(
      tag(field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
      delim(merged_data));

  MessageType test_message;
  test_message.MergeFromString(expected_proto);
  std::string text;
  TextFormat::PrintToString(test_message, &text);
  RunValidProtobufTest("ValidDataOneof.MESSAGE.Merge", REQUIRED, proto, text);
  RunValidBinaryProtobufTest("ValidDataOneofBinary.MESSAGE.Merge", RECOMMENDED,
                             proto, expected_proto);
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::TestIllegalTags() {
  // field num 0 is illegal
  std::string nullfield[] = {"\1DEADBEEF", "\2\1\1", "\3\4", "\5DEAD"};
  for (int i = 0; i < 4; i++) {
    std::string name = "IllegalZeroFieldNum_Case_0";
    name.back() += i;
    ExpectParseFailureForProto(nullfield[i], name, REQUIRED);
  }
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::TestUnmatchedGroup() {
  ExpectParseFailureForProto(tag(201, WireFormatLite::WIRETYPE_END_GROUP),
                             "UnmatchedEndGroup", REQUIRED);
  ExpectParseFailureForProto(tag(1234, WireFormatLite::WIRETYPE_END_GROUP),
                             "UnmatchedEndGroupUnknown", REQUIRED);
  ExpectParseFailureForProto(tag(1, WireFormatLite::WIRETYPE_END_GROUP),
                             "UnmatchedEndGroupWrongType", REQUIRED);
  ExpectParseFailureForProto(
      len(18, tag(1234, WireFormatLite::WIRETYPE_END_GROUP)),
      "UnmatchedEndGroupNestedLen", REQUIRED);
  ExpectParseFailureForProto(
      group(201, tag(202, WireFormatLite::WIRETYPE_END_GROUP)),
      "UnmatchedEndGroupNested", REQUIRED);
  ExpectParseFailureForProto(
      absl::StrCat(tag(1, WireFormatLite::WIRETYPE_END_GROUP),
                   len(2, "hello world")),
      "UnmatchedEndGroupWithData", REQUIRED);

  ExpectParseFailureForProto(tag(201, WireFormatLite::WIRETYPE_START_GROUP),
                             "UnmatchedStartGroup", REQUIRED);
  ExpectParseFailureForProto(tag(1234, WireFormatLite::WIRETYPE_START_GROUP),
                             "UnmatchedStartGroupUnknown", REQUIRED);
  ExpectParseFailureForProto(tag(1, WireFormatLite::WIRETYPE_START_GROUP),
                             "UnmatchedStartGroupWrongType", REQUIRED);
  ExpectParseFailureForProto(
      len(18, tag(1234, WireFormatLite::WIRETYPE_START_GROUP)),
      "UnmatchedStartGroupNestedLen", REQUIRED);
  ExpectParseFailureForProto(
      group(201, tag(202, WireFormatLite::WIRETYPE_START_GROUP)),
      "UnmatchedStartGroupNested", REQUIRED);
  ExpectParseFailureForProto(
      absl::StrCat(tag(1, WireFormatLite::WIRETYPE_START_GROUP),
                   len(2, "hello world")),
      "UnmatchedStartGroupWithData", REQUIRED);

  ExpectParseFailureForProto(
      absl::StrCat(tag(201, WireFormatLite::WIRETYPE_START_GROUP),
                   len(2, "hello world"),
                   tag(202, WireFormatLite::WIRETYPE_END_GROUP)),
      "MismatchedGroupTags", REQUIRED);
  ExpectParseFailureForProto(
      group(201, absl::StrCat(tag(202, WireFormatLite::WIRETYPE_START_GROUP),
                              len(2, "hello world"),
                              tag(203, WireFormatLite::WIRETYPE_END_GROUP))),
      "MismatchedNestedGroupTags", REQUIRED);
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::TestUnknownWireType() {
  for (uint8_t type : {0x6, 0x7}) {
    for (uint8_t field = 0; field < 4; ++field) {
      for (uint8_t value = 0; value < 4; ++value) {
        std::string name = absl::StrFormat(
            "UnknownWireType%d_Field%d_Version%d", type, field, value);

        char data[2];
        data[0] = (field << 3) | type;  // unknown wire type.
        data[1] = value;
        std::string proto = {data, 2};
        ExpectParseFailureForProto(proto, name, REQUIRED);
      }
    }
  }
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::TestOneofMessage() {
  MessageType message;
  message.set_oneof_uint32(0);
  RunValidProtobufTestWithMessage("OneofZeroUint32", RECOMMENDED, &message,
                                  "oneof_uint32: 0");
  message.mutable_oneof_nested_message()->set_a(0);
  RunValidProtobufTestWithMessage("OneofZeroMessage", RECOMMENDED, &message,
                                  run_proto3_tests_
                                      ? "oneof_nested_message: {}"
                                      : "oneof_nested_message: {a: 0}");
  message.mutable_oneof_nested_message()->set_a(1);
  RunValidProtobufTestWithMessage("OneofZeroMessageSetTwice", RECOMMENDED,
                                  &message, "oneof_nested_message: {a: 1}");
  message.set_oneof_string("");
  RunValidProtobufTestWithMessage("OneofZeroString", RECOMMENDED, &message,
                                  "oneof_string: \"\"");
  message.set_oneof_bytes("");
  RunValidProtobufTestWithMessage("OneofZeroBytes", RECOMMENDED, &message,
                                  "oneof_bytes: \"\"");
  message.set_oneof_bool(false);
  RunValidProtobufTestWithMessage("OneofZeroBool", RECOMMENDED, &message,
                                  "oneof_bool: false");
  message.set_oneof_uint64(0);
  RunValidProtobufTestWithMessage("OneofZeroUint64", RECOMMENDED, &message,
                                  "oneof_uint64: 0");
  message.set_oneof_float(0.0f);
  RunValidProtobufTestWithMessage("OneofZeroFloat", RECOMMENDED, &message,
                                  "oneof_float: 0");
  message.set_oneof_double(0.0);
  RunValidProtobufTestWithMessage("OneofZeroDouble", RECOMMENDED, &message,
                                  "oneof_double: 0");
  message.set_oneof_enum(MessageType::FOO);
  RunValidProtobufTestWithMessage("OneofZeroEnum", RECOMMENDED, &message,
                                  "oneof_enum: FOO");
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::TestUnknownMessage() {
  MessageType message;
  message.ParseFromString("\xA8\x1F\x01");
  RunValidBinaryProtobufTest("UnknownVarint", REQUIRED,
                             message.SerializeAsString());
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::TestUnknownOrdering() {
  // Implementations must preserve the ordering of different unknown fields for
  // the same field number.  This is because some field types will accept
  // multiple wire types for the same field.  For example, repeated primitive
  // fields will accept both length-prefixed (packed) and
  // varint/fixed32/fixed64 (unpacked) wire types, and reordering these could
  // reorder the elements of the repeated field.
  MessageType message;
  MessageType prototype;
  message.mutable_unknown_fields()->AddLengthDelimited(UNKNOWN_FIELD, "abc");
  message.mutable_unknown_fields()->AddVarint(UNKNOWN_FIELD, 123);
  message.mutable_unknown_fields()->AddLengthDelimited(UNKNOWN_FIELD, "def");
  message.mutable_unknown_fields()->AddVarint(UNKNOWN_FIELD, 456);
  std::string serialized = message.SerializeAsString();

  ConformanceRequestSetting setting(
      REQUIRED, ::conformance::PROTOBUF, ::conformance::PROTOBUF,
      ::conformance::BINARY_TEST, prototype, "UnknownOrdering", serialized);
  const ConformanceRequest& request = setting.GetRequest();
  ConformanceResponse response;
  if (!suite_.RunTest(setting.GetTestName(), request, &response)) {
    return;
  }

  MessageType response_message;
  TestStatus test;
  test.set_name(setting.GetTestName());
  if (response.result_case() == ConformanceResponse::kSkipped) {
    suite_.ReportSkip(test, request, response);
    return;
  }

  suite_.ParseResponse(response, setting, &response_message);

  const UnknownFieldSet& ufs = response_message.unknown_fields();
  if (ufs.field_count() != 4 || ufs.field(0).number() != UNKNOWN_FIELD ||
      ufs.field(1).number() != UNKNOWN_FIELD ||
      ufs.field(2).number() != UNKNOWN_FIELD ||
      ufs.field(3).number() != UNKNOWN_FIELD ||
      ufs.field(0).type() != UnknownField::Type::TYPE_LENGTH_DELIMITED ||
      ufs.field(1).type() != UnknownField::Type::TYPE_VARINT ||
      ufs.field(2).type() != UnknownField::Type::TYPE_LENGTH_DELIMITED ||
      ufs.field(3).type() != UnknownField::Type::TYPE_VARINT ||
      ufs.field(0).length_delimited() != "abc" ||
      ufs.field(1).varint() != 123 ||
      ufs.field(2).length_delimited() != "def" ||
      ufs.field(3).varint() != 456) {
    test.set_failure_message("Unknown field mismatch");
    suite_.ReportFailure(test, setting.GetLevel(), request, response);
  } else {
    suite_.ReportSuccess(test);
  }
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<
    MessageType>::TestBinaryPerformanceForAlternatingUnknownFields() {
  std::string unknown_field_1 = absl::StrCat(
      tag(UNKNOWN_FIELD, WireFormatLite::WIRETYPE_VARINT), varint(1234));
  std::string unknown_field_2 = absl::StrCat(
      tag(UNKNOWN_FIELD + 1, WireFormatLite::WIRETYPE_VARINT), varint(5678));
  std::string proto;
  for (size_t i = 0; i < kPerformanceRepeatCount; i++) {
    proto.append(unknown_field_1);
    proto.append(unknown_field_2);
  }

  RunValidBinaryProtobufTest("TestBinaryPerformanceForAlternatingUnknownFields",
                             RECOMMENDED, proto);
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::
    TestBinaryPerformanceMergeMessageWithRepeatedFieldForType(
        FieldDescriptor::Type type) {
  const std::string type_name =
      UpperCase(absl::StrCat(".", FieldDescriptor::TypeName(type)));
  int field_number = GetFieldForType(type, true, Packed::kFalse)->number();
  std::string rep_field_proto = absl::StrCat(
      tag(field_number, WireFormatLite::WireTypeForFieldType(
                            static_cast<WireFormatLite::FieldType>(type))),
      GetNonDefaultValue(type));

  RunBinaryPerformanceMergeMessageWithField(
      absl::StrCat("TestBinaryPerformanceMergeMessageWithRepeatedFieldForType",
                   type_name),
      rep_field_proto);
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::
    TestBinaryPerformanceMergeMessageWithUnknownFieldForType(
        FieldDescriptor::Type type) {
  const std::string type_name =
      UpperCase(absl::StrCat(".", FieldDescriptor::TypeName(type)));
  std::string unknown_field_proto = absl::StrCat(
      tag(UNKNOWN_FIELD, WireFormatLite::WireTypeForFieldType(
                             static_cast<WireFormatLite::FieldType>(type))),
      GetNonDefaultValue(type));
  RunBinaryPerformanceMergeMessageWithField(
      absl::StrCat("TestBinaryPerformanceMergeMessageWithUnknownFieldForType",
                   type_name),
      unknown_field_proto);
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
  if (!suite_.performance_) {
    for (int i = 1; i <= FieldDescriptor::MAX_TYPE; i++) {
      if (i == FieldDescriptor::TYPE_GROUP) continue;
      TestPrematureEOFForType(static_cast<FieldDescriptor::Type>(i));
    }

    TestIllegalTags();
    TestUnmatchedGroup();
    TestUnknownWireType();

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
             delim("\'\"\?\\\a\b\f\n\r\t\v")},       // escape
            {delim("谷歌"), delim("谷歌")},          // Google in Chinese
            {delim("\u8C37\u6B4C"), delim("谷歌")},  // unicode escape
            {delim("\u8c37\u6b4c"), delim("谷歌")},  // lowercase unicode
            {delim("\xF0\x9F\x98\x81"),
             delim("\xF0\x9F\x98\x81")},  // emoji: 😁
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

    // Unknown fields.
    // TODO: update this behavior when unknown field's behavior
    // changed in open source. Also delete
    // Required.Proto3.ProtobufInput.UnknownVarint.ProtobufOutput
    // from failure list of python_cpp python java
    TestUnknownMessage();
    TestUnknownOrdering();
    TestOneofMessage();

    RunJsonTests();
  }
  // Flag control performance tests to keep them internal and opt-in only
  if (suite_.performance_) {
    RunBinaryPerformanceTests();
    RunJsonPerformanceTests();
  }
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<
    MessageType>::RunBinaryPerformanceTests() {
  TestBinaryPerformanceForAlternatingUnknownFields();

  TestBinaryPerformanceMergeMessageWithRepeatedFieldForType(
      FieldDescriptor::TYPE_BOOL);
  TestBinaryPerformanceMergeMessageWithRepeatedFieldForType(
      FieldDescriptor::TYPE_DOUBLE);
  TestBinaryPerformanceMergeMessageWithRepeatedFieldForType(
      FieldDescriptor::TYPE_FLOAT);
  TestBinaryPerformanceMergeMessageWithRepeatedFieldForType(
      FieldDescriptor::TYPE_UINT32);
  TestBinaryPerformanceMergeMessageWithRepeatedFieldForType(
      FieldDescriptor::TYPE_UINT64);
  TestBinaryPerformanceMergeMessageWithRepeatedFieldForType(
      FieldDescriptor::TYPE_STRING);
  TestBinaryPerformanceMergeMessageWithRepeatedFieldForType(
      FieldDescriptor::TYPE_BYTES);

  TestBinaryPerformanceMergeMessageWithUnknownFieldForType(
      FieldDescriptor::TYPE_BOOL);
  TestBinaryPerformanceMergeMessageWithUnknownFieldForType(
      FieldDescriptor::TYPE_DOUBLE);
  TestBinaryPerformanceMergeMessageWithUnknownFieldForType(
      FieldDescriptor::TYPE_FLOAT);
  TestBinaryPerformanceMergeMessageWithUnknownFieldForType(
      FieldDescriptor::TYPE_UINT32);
  TestBinaryPerformanceMergeMessageWithUnknownFieldForType(
      FieldDescriptor::TYPE_UINT64);
  TestBinaryPerformanceMergeMessageWithUnknownFieldForType(
      FieldDescriptor::TYPE_STRING);
  TestBinaryPerformanceMergeMessageWithUnknownFieldForType(
      FieldDescriptor::TYPE_BYTES);
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::RunJsonPerformanceTests() {
  TestJsonPerformanceMergeMessageWithRepeatedFieldForType(
      FieldDescriptor::TYPE_BOOL, "true");
  TestJsonPerformanceMergeMessageWithRepeatedFieldForType(
      FieldDescriptor::TYPE_DOUBLE, "123");
  TestJsonPerformanceMergeMessageWithRepeatedFieldForType(
      FieldDescriptor::TYPE_FLOAT, "123");
  TestJsonPerformanceMergeMessageWithRepeatedFieldForType(
      FieldDescriptor::TYPE_UINT32, "123");
  TestJsonPerformanceMergeMessageWithRepeatedFieldForType(
      FieldDescriptor::TYPE_UINT64, "123");
  TestJsonPerformanceMergeMessageWithRepeatedFieldForType(
      FieldDescriptor::TYPE_STRING, "\"foo\"");
  TestJsonPerformanceMergeMessageWithRepeatedFieldForType(
      FieldDescriptor::TYPE_BYTES, "\"foo\"");
}

// This is currently considered valid input by some languages but not others
template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::
    TestJsonPerformanceMergeMessageWithRepeatedFieldForType(
        FieldDescriptor::Type type, std::string field_value) {
  const std::string type_name =
      UpperCase(absl::StrCat(".", FieldDescriptor::TypeName(type)));
  const FieldDescriptor* field = GetFieldForType(type, true, Packed::kFalse);
  const absl::string_view field_name = field->name();

  std::string message_field =
      absl::StrCat("\"", field_name, "\": [", field_value, "]");
  std::string recursive_message =
      absl::StrCat("\"recursive_message\": { ", message_field, "}");
  std::string input = absl::StrCat("{", recursive_message);
  for (size_t i = 1; i < kPerformanceRepeatCount; i++) {
    absl::StrAppend(&input, ",", recursive_message);
  }
  absl::StrAppend(&input, "}");

  std::string textproto_message_field =
      absl::StrCat(field_name, ": ", field_value);
  std::string expected_textproto = "recursive_message { ";
  for (size_t i = 0; i < kPerformanceRepeatCount; i++) {
    absl::StrAppend(&expected_textproto, textproto_message_field, " ");
  }
  absl::StrAppend(&expected_textproto, "}");
  RunValidJsonTest(
      absl::StrCat("TestJsonPerformanceMergeMessageWithRepeatedFieldForType",
                   type_name),
      RECOMMENDED, input, expected_textproto);
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::RunJsonTests() {
  RunValidJsonTest("HelloWorld", REQUIRED,
                   "{\"optionalString\":\"Hello, World!\"}",
                   "optional_string: 'Hello, World!'");

  // NOTE: The spec for JSON support is still being sorted out, these may not
  // all be correct.
  RunJsonTestsForFieldNameConvention();
  RunJsonTestsForNonRepeatedTypes();
  RunJsonTestsForRepeatedTypes();
  RunJsonTestsForNullTypes();

  if (run_proto3_tests_) {
    RunJsonTestsForWrapperTypes();
    RunJsonTestsForFieldMask();
    RunJsonTestsForStruct();
    RunJsonTestsForValue();
    RunJsonTestsForAny();
  } else {
    // Currently Proto2 only, but should also be run on Proto3-optional.
    RunJsonTestsForStoresDefaultPrimitive();
  }

  RunJsonTestsForUnknownEnumStringValues();

  RunValidJsonIgnoreUnknownTest("IgnoreUnknownJsonNumber", REQUIRED,
                                R"({"unknown": 1})", "");
  RunValidJsonIgnoreUnknownTest("IgnoreUnknownJsonString", REQUIRED,
                                R"({"unknown": "a"})", "");
  RunValidJsonIgnoreUnknownTest("IgnoreUnknownJsonTrue", REQUIRED,
                                R"({"unknown": true})", "");
  RunValidJsonIgnoreUnknownTest("IgnoreUnknownJsonFalse", REQUIRED,
                                R"({"unknown": false})", "");
  RunValidJsonIgnoreUnknownTest("IgnoreUnknownJsonNull", REQUIRED,
                                R"({"unknown": null})", "");
  RunValidJsonIgnoreUnknownTest("IgnoreUnknownJsonObject", REQUIRED,
                                R"({"unknown": {"a": 1}})", "");

  ExpectParseFailureForJson("RejectTopLevelNull", REQUIRED, "null");
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<
    MessageType>::RunJsonTestsForStoresDefaultPrimitive() {
  RunValidJsonTestWithValidator(
      "StoresDefaultPrimitive", REQUIRED,
      R"({
          "FieldName13": 0
        })",
      [](const Json::Value& value) { return value.isMember("FieldName13"); });
  std::vector<const FieldDescriptor*> extensions;
  MessageType::GetDescriptor()->file()->pool()->FindAllExtensions(
      MessageType::GetDescriptor(), &extensions);
  RunValidJsonTestWithValidator("FieldNameExtension", RECOMMENDED,
                                absl::Substitute(R"({
          "[$0]": 1
        })",
                                                 extensions[0]->full_name()),
                                [&](const Json::Value& value) {
                                  return value.isMember(absl::StrCat(
                                      "[", extensions[0]->full_name(), "]"));
                                });
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
    MessageType>::RunJsonTestsForUnknownEnumStringValues() {
  // Tests the handling of unknown enum values when encoded as string labels.
  // The expected behavior depends on whether unknown fields are ignored:
  // * when ignored, the parser should ignore the unknown enum string value.
  // * when not ignored, the parser should fail.
  struct TestCase {
    // Used in the test name.
    std::string enum_location;
    // JSON input which will contain the unknown field.
    std::string input_json;
  };
  const std::vector<TestCase> test_cases = {
      {"InOptionalField", R"json({
      "optional_nested_enum": "UNKNOWN_ENUM_VALUE"
    })json"},
      {"InRepeatedField", R"json({
      "repeated_nested_enum": ["UNKNOWN_ENUM_VALUE"]
    })json"},
      {"InMapValue", R"json({
      "map_string_nested_enum": {"key": "UNKNOWN_ENUM_VALUE"}
    })json"},
  };
  for (const TestCase& test_case : test_cases) {
    // Unknown enum string value is a parse failure when not ignoring unknown
    // fields.
    ExpectParseFailureForJson(
        absl::StrCat("RejectUnknownEnumStringValue", test_case.enum_location),
        RECOMMENDED, test_case.input_json);
    // Unknown enum string value is ignored when ignoring unknown fields.
    RunValidJsonIgnoreUnknownTest(
        absl::StrCat("IgnoreUnknownEnumStringValue", test_case.enum_location),
        RECOMMENDED, test_case.input_json, "");
  }

  // This test is similar to "InRepeatedField" from above, but it highlights the
  // potentially unexpected behavior in an array with mixed known and unknown
  // enum string values.
  RunValidJsonIgnoreUnknownTest("IgnoreUnknownEnumStringValueInRepeatedPart",
                                RECOMMENDED,
                                R"json({
    "repeated_nested_enum": [
      "FOO",
      "UNKNOWN_ENUM_VALUE",
      "FOO"
    ]})json",
                                R"(
    repeated_nested_enum: FOO
    repeated_nested_enum: FOO
  )");

  // This test is similar to "InMapValue" from above with mixture of known and
  // unknown enum string values in the map.
  RunValidJsonIgnoreUnknownTest("IgnoreUnknownEnumStringValueInMapPart",
                                RECOMMENDED,
                                R"json({
    "map_string_nested_enum": {
      "key1": "FOO",
      "key2": "UNKNOWN_ENUM_VALUE"
    }})json",
                                R"(
    map_string_nested_enum: {
      key: "key1"
      value: FOO
    }
  )");
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<
    MessageType>::RunJsonTestsForFieldNameConvention() {
  RunValidJsonTest("FieldNameInSnakeCase", REQUIRED,
                   R"({
        "fieldname1": 1,
        "fieldName2": 2,
        "FieldName3": 3,
        "fieldName4": 4
      })",
                   R"(
        fieldname1: 1
        field_name2: 2
        _field_name3: 3
        field__name4_: 4
      )");
  RunValidJsonTest("FieldNameWithNumbers", REQUIRED,
                   R"({
        "field0name5": 5,
        "field0Name6": 6
      })",
                   R"(
        field0name5: 5
        field_0_name6: 6
      )");
  RunValidJsonTest("FieldNameWithMixedCases", REQUIRED,
                   R"({
        "fieldName7": 7,
        "FieldName8": 8,
        "fieldName9": 9,
        "FieldName10": 10,
        "FIELDNAME11": 11,
        "FIELDName12": 12
      })",
                   R"(
        fieldName7: 7
        FieldName8: 8
        field_Name9: 9
        Field_Name10: 10
        FIELD_NAME11: 11
        FIELD_name12: 12
      )");
  RunValidJsonTest("FieldNameWithDoubleUnderscores", RECOMMENDED,
                   R"({
        "FieldName13": 13,
        "FieldName14": 14,
        "fieldName15": 15,
        "fieldName16": 16,
        "fieldName17": 17,
        "FieldName18": 18
      })",
                   R"(
        __field_name13: 13
        __Field_name14: 14
        field__name15: 15
        field__Name16: 16
        field_name17__: 17
        Field_name18__: 18
      )");
  // Using the original proto field name in JSON is also allowed.
  RunValidJsonTest("OriginalProtoFieldName", REQUIRED,
                   R"({
        "fieldname1": 1,
        "field_name2": 2,
        "_field_name3": 3,
        "field__name4_": 4,
        "field0name5": 5,
        "field_0_name6": 6,
        "fieldName7": 7,
        "FieldName8": 8,
        "field_Name9": 9,
        "Field_Name10": 10,
        "FIELD_NAME11": 11,
        "FIELD_name12": 12,
        "__field_name13": 13,
        "__Field_name14": 14,
        "field__name15": 15,
        "field__Name16": 16,
        "field_name17__": 17,
        "Field_name18__": 18
      })",
                   R"(
        fieldname1: 1
        field_name2: 2
        _field_name3: 3
        field__name4_: 4
        field0name5: 5
        field_0_name6: 6
        fieldName7: 7
        FieldName8: 8
        field_Name9: 9
        Field_Name10: 10
        FIELD_NAME11: 11
        FIELD_name12: 12
        __field_name13: 13
        __Field_name14: 14
        field__name15: 15
        field__Name16: 16
        field_name17__: 17
        Field_name18__: 18
      )");
  // Field names can be escaped.
  RunValidJsonTest("FieldNameEscaped", REQUIRED, R"({"fieldn\u0061me1": 1})",
                   "fieldname1: 1");
  // String ends with escape character.
  ExpectParseFailureForJson("StringEndsWithEscapeChar", RECOMMENDED,
                            "{\"optionalString\": \"abc\\");
  // Field names must be quoted (or it's not valid JSON).
  ExpectParseFailureForJson("FieldNameNotQuoted", RECOMMENDED,
                            "{fieldname1: 1}");
  // Trailing comma is not allowed (not valid JSON).
  ExpectParseFailureForJson("TrailingCommaInAnObject", RECOMMENDED,
                            R"({"fieldname1":1,})");
  ExpectParseFailureForJson("TrailingCommaInAnObjectWithSpace", RECOMMENDED,
                            R"({"fieldname1":1 ,})");
  ExpectParseFailureForJson("TrailingCommaInAnObjectWithSpaceCommaSpace",
                            RECOMMENDED, R"({"fieldname1":1 , })");
  ExpectParseFailureForJson("TrailingCommaInAnObjectWithNewlines", RECOMMENDED,
                            R"({
        "fieldname1":1,
      })");
  // JSON doesn't support comments.
  ExpectParseFailureForJson("JsonWithComments", RECOMMENDED,
                            R"({
        // This is a comment.
        "fieldname1": 1
      })");
  // JSON spec says whitespace doesn't matter, so try a few spacings to be sure.
  RunValidJsonTest("OneLineNoSpaces", RECOMMENDED,
                   "{\"optionalInt32\":1,\"optionalInt64\":2}",
                   R"(
        optional_int32: 1
        optional_int64: 2
      )");
  RunValidJsonTest("OneLineWithSpaces", RECOMMENDED,
                   "{ \"optionalInt32\" : 1 , \"optionalInt64\" : 2 }",
                   R"(
        optional_int32: 1
        optional_int64: 2
      )");
  RunValidJsonTest("MultilineNoSpaces", RECOMMENDED,
                   "{\n\"optionalInt32\"\n:\n1\n,\n\"optionalInt64\"\n:\n2\n}",
                   R"(
        optional_int32: 1
        optional_int64: 2
      )");
  RunValidJsonTest(
      "MultilineWithSpaces", RECOMMENDED,
      "{\n  \"optionalInt32\"  :  1\n  ,\n  \"optionalInt64\"  :  2\n}\n",
      R"(
        optional_int32: 1
        optional_int64: 2
      )");
  // Missing comma between key/value pairs.
  ExpectParseFailureForJson("MissingCommaOneLine", RECOMMENDED,
                            "{ \"optionalInt32\": 1 \"optionalInt64\": 2 }");
  ExpectParseFailureForJson(
      "MissingCommaMultiline", RECOMMENDED,
      "{\n  \"optionalInt32\": 1\n  \"optionalInt64\": 2\n}");
  // Duplicated field names are not allowed.
  ExpectParseFailureForJson("FieldNameDuplicate", RECOMMENDED,
                            R"({
        "optionalNestedMessage": {a: 1},
        "optionalNestedMessage": {}
      })");
  ExpectParseFailureForJson("FieldNameDuplicateDifferentCasing1", RECOMMENDED,
                            R"({
        "optional_nested_message": {a: 1},
        "optionalNestedMessage": {}
      })");
  ExpectParseFailureForJson("FieldNameDuplicateDifferentCasing2", RECOMMENDED,
                            R"({
        "optionalNestedMessage": {a: 1},
        "optional_nested_message": {}
      })");
  // Serializers should use lowerCamelCase by default.
  RunValidJsonTestWithValidator("FieldNameInLowerCamelCase", REQUIRED,
                                R"({
        "fieldname1": 1,
        "fieldName2": 2,
        "FieldName3": 3,
        "fieldName4": 4
      })",
                                [](const Json::Value& value) {
                                  return value.isMember("fieldname1") &&
                                         value.isMember("fieldName2") &&
                                         value.isMember("FieldName3") &&
                                         value.isMember("fieldName4");
                                });
  RunValidJsonTestWithValidator("FieldNameWithNumbers", REQUIRED,
                                R"({
        "field0name5": 5,
        "field0Name6": 6
      })",
                                [](const Json::Value& value) {
                                  return value.isMember("field0name5") &&
                                         value.isMember("field0Name6");
                                });
  RunValidJsonTestWithValidator(
      "FieldNameWithMixedCases", REQUIRED,
      R"({
        "fieldName7": 7,
        "FieldName8": 8,
        "fieldName9": 9,
        "FieldName10": 10,
        "FIELDNAME11": 11,
        "FIELDName12": 12
      })",
      [](const Json::Value& value) {
        return value.isMember("fieldName7") && value.isMember("FieldName8") &&
               value.isMember("fieldName9") && value.isMember("FieldName10") &&
               value.isMember("FIELDNAME11") && value.isMember("FIELDName12");
      });
  RunValidJsonTestWithValidator(
      "FieldNameWithDoubleUnderscores", RECOMMENDED,
      R"({
        "FieldName13": 13,
        "FieldName14": 14,
        "fieldName15": 15,
        "fieldName16": 16,
        "fieldName17": 17,
        "FieldName18": 18
      })",
      [](const Json::Value& value) {
        return value.isMember("FieldName13") && value.isMember("FieldName14") &&
               value.isMember("fieldName15") && value.isMember("fieldName16") &&
               value.isMember("fieldName17") && value.isMember("FieldName18");
      });

  if (run_proto3_tests_) {
    RunValidJsonTestWithValidator("SkipsDefaultPrimitive", REQUIRED,
                                  R"({"FieldName13": 0})",
                                  [](const Json::Value& value) {
                                    return !value.isMember("FieldName13");
                                  });
  }
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<
    MessageType>::RunJsonTestsForNonRepeatedTypes() {
  // Integer fields.
  RunValidJsonTest("Int32FieldMaxValue", REQUIRED,
                   R"({"optionalInt32": 2147483647})",
                   "optional_int32: 2147483647");
  RunValidJsonTest("Int32FieldMinValue", REQUIRED,
                   R"({"optionalInt32": -2147483648})",
                   "optional_int32: -2147483648");
  RunValidJsonTest("Uint32FieldMaxValue", REQUIRED,
                   R"({"optionalUint32": 4294967295})",
                   "optional_uint32: 4294967295");
  RunValidJsonTest("Int64FieldMaxValue", REQUIRED,
                   R"({"optionalInt64": "9223372036854775807"})",
                   "optional_int64: 9223372036854775807");
  RunValidJsonTest("Int64FieldMinValue", REQUIRED,
                   R"({"optionalInt64": "-9223372036854775808"})",
                   "optional_int64: -9223372036854775808");
  RunValidJsonTest("Uint64FieldMaxValue", REQUIRED,
                   R"({"optionalUint64": "18446744073709551615"})",
                   "optional_uint64: 18446744073709551615");
  // While not the largest Int64, this is the largest
  // Int64 which can be exactly represented within an
  // IEEE-754 64-bit float, which is the expected level
  // of interoperability guarantee. Larger values may
  // work in some implementations, but should not be
  // relied upon.
  RunValidJsonTest("Int64FieldMaxValueNotQuoted", REQUIRED,
                   R"({"optionalInt64": 9223372036854774784})",
                   "optional_int64: 9223372036854774784");
  RunValidJsonTest("Int64FieldMinValueNotQuoted", REQUIRED,
                   R"({"optionalInt64": -9223372036854775808})",
                   "optional_int64: -9223372036854775808");
  // Largest interoperable Uint64; see comment above
  // for Int64FieldMaxValueNotQuoted.
  RunValidJsonTest("Uint64FieldMaxValueNotQuoted", REQUIRED,
                   R"({"optionalUint64": 18446744073709549568})",
                   "optional_uint64: 18446744073709549568");
  // Values can be represented as JSON strings.
  RunValidJsonTest("Int32FieldStringValue", REQUIRED,
                   R"({"optionalInt32": "2147483647"})",
                   "optional_int32: 2147483647");
  RunValidJsonTest("Int32FieldStringValueEscaped", REQUIRED,
                   R"({"optionalInt32": "2\u003147483647"})",
                   "optional_int32: 2147483647");
  RunValidJsonTest("Int32FieldStringValueZero", REQUIRED,
                   R"({"optionalInt32": "0"})", "optional_int32: 0");
  RunValidJsonTest("Int32FieldQuotedExponentialValue", REQUIRED,
                   R"({"optionalInt32": "1e5"})", "optional_int32: 100000");

  // Parsers reject out-of-bound integer values.
  ExpectParseFailureForJson("Int32FieldTooLarge", REQUIRED,
                            R"({"optionalInt32": 2147483648})");
  ExpectParseFailureForJson("Int32FieldTooSmall", REQUIRED,
                            R"({"optionalInt32": -2147483649})");
  ExpectParseFailureForJson("Uint32FieldTooLarge", REQUIRED,
                            R"({"optionalUint32": 4294967296})");
  ExpectParseFailureForJson("Int64FieldTooLarge", REQUIRED,
                            R"({"optionalInt64": "9223372036854775808"})");
  ExpectParseFailureForJson("Int64FieldTooSmall", REQUIRED,
                            R"({"optionalInt64": "-9223372036854775809"})");
  ExpectParseFailureForJson("Uint64FieldTooLarge", REQUIRED,
                            R"({"optionalUint64": "18446744073709551616"})");

  // Parser reject non-integer numeric values.
  ExpectParseFailureForJson("Int32FieldNotInteger", REQUIRED,
                            R"({"optionalInt32": 0.5})");
  ExpectParseFailureForJson("Uint32FieldNotInteger", REQUIRED,
                            R"({"optionalUint32": 0.5})");
  ExpectParseFailureForJson("Int64FieldNotInteger", REQUIRED,
                            R"({"optionalInt64": "0.5"})");
  ExpectParseFailureForJson("Uint64FieldNotInteger", REQUIRED,
                            R"({"optionalUint64": "0.5"})");

  // Parser reject non-numeric string values.
  ExpectParseFailureForJson("Int32FieldStringValuePartiallyNumeric", REQUIRED,
                            R"({"optionalInt32": "12abc"})");
  ExpectParseFailureForJson("Int32FieldStringValueNonNumeric", REQUIRED,
                            R"({"optionalInt32": "abc"})");

  // Parser reject empty string values.
  ExpectParseFailureForJson("Int32FieldEmptyString", REQUIRED,
                            R"({"optionalInt32": ""})");
  ExpectParseFailureForJson("Uint32FieldEmptyString", REQUIRED,
                            R"({"optionalUint32": ""})");
  ExpectParseFailureForJson("Int64FieldEmptyString", REQUIRED,
                            R"({"optionalInt64": ""})");
  ExpectParseFailureForJson("Uint64FieldEmptyString", REQUIRED,
                            R"({"optionalUint64": ""})");

  // Integers but represented as float values are accepted.
  RunValidJsonTest("Int32FieldFloatTrailingZero", REQUIRED,
                   R"({"optionalInt32": 100000.000})",
                   "optional_int32: 100000");
  RunValidJsonTest("Int32FieldExponentialFormat", REQUIRED,
                   R"({"optionalInt32": 1e5})", "optional_int32: 100000");
  RunValidJsonTest("Int32FieldMaxFloatValue", REQUIRED,
                   R"({"optionalInt32": 2.147483647e9})",
                   "optional_int32: 2147483647");
  RunValidJsonTest("Int32FieldMinFloatValue", REQUIRED,
                   R"({"optionalInt32": -2.147483648e9})",
                   "optional_int32: -2147483648");
  RunValidJsonTest("Uint32FieldMaxFloatValue", REQUIRED,
                   R"({"optionalUint32": 4.294967295e9})",
                   "optional_uint32: 4294967295");

  // Parser reject non-numeric values.
  ExpectParseFailureForJson("Int32FieldNotNumber", REQUIRED,
                            R"({"optionalInt32": "3x3"})");
  ExpectParseFailureForJson("Uint32FieldNotNumber", REQUIRED,
                            R"({"optionalUint32": "3x3"})");
  ExpectParseFailureForJson("Int64FieldNotNumber", REQUIRED,
                            R"({"optionalInt64": "3x3"})");
  ExpectParseFailureForJson("Uint64FieldNotNumber", REQUIRED,
                            R"({"optionalUint64": "3x3"})");
  // JSON does not allow "+" on numeric values.
  ExpectParseFailureForJson("Int32FieldPlusSign", REQUIRED,
                            R"({"optionalInt32": +1})");
  // JSON doesn't allow leading 0s.
  ExpectParseFailureForJson("Int32FieldLeadingZero", REQUIRED,
                            R"({"optionalInt32": 01})");
  ExpectParseFailureForJson("Int32FieldNegativeWithLeadingZero", REQUIRED,
                            R"({"optionalInt32": -01})");
  // String values must follow the same syntax rule. Specifically leading
  // or trailing spaces are not allowed.
  ExpectParseFailureForJson("Int32FieldLeadingSpace", REQUIRED,
                            R"({"optionalInt32": " 1"})");
  ExpectParseFailureForJson("Int32FieldTrailingSpace", REQUIRED,
                            R"({"optionalInt32": "1 "})");

  // 64-bit values are serialized as strings.
  RunValidJsonTestWithValidator(
      "Int64FieldBeString", RECOMMENDED, R"({"optionalInt64": 1})",
      [](const Json::Value& value) {
        return value["optionalInt64"].type() == Json::stringValue &&
               value["optionalInt64"].asString() == "1";
      });
  RunValidJsonTestWithValidator(
      "Uint64FieldBeString", RECOMMENDED, R"({"optionalUint64": 1})",
      [](const Json::Value& value) {
        return value["optionalUint64"].type() == Json::stringValue &&
               value["optionalUint64"].asString() == "1";
      });

  // Bool fields.
  RunValidJsonTest("BoolFieldTrue", REQUIRED, R"({"optionalBool":true})",
                   "optional_bool: true");
  RunValidJsonTest("BoolFieldFalse", REQUIRED, R"({"optionalBool":false})",
                   "optional_bool: false");

  // Other forms are not allowed.
  ExpectParseFailureForJson("BoolFieldIntegerZero", RECOMMENDED,
                            R"({"optionalBool":0})");
  ExpectParseFailureForJson("BoolFieldIntegerOne", RECOMMENDED,
                            R"({"optionalBool":1})");
  ExpectParseFailureForJson("BoolFieldCamelCaseTrue", RECOMMENDED,
                            R"({"optionalBool":True})");
  ExpectParseFailureForJson("BoolFieldCamelCaseFalse", RECOMMENDED,
                            R"({"optionalBool":False})");
  ExpectParseFailureForJson("BoolFieldAllCapitalTrue", RECOMMENDED,
                            R"({"optionalBool":TRUE})");
  ExpectParseFailureForJson("BoolFieldAllCapitalFalse", RECOMMENDED,
                            R"({"optionalBool":FALSE})");
  ExpectParseFailureForJson("BoolFieldDoubleQuotedTrue", RECOMMENDED,
                            R"({"optionalBool":"true"})");
  ExpectParseFailureForJson("BoolFieldDoubleQuotedFalse", RECOMMENDED,
                            R"({"optionalBool":"false"})");

  // Float fields.
  RunValidJsonTest("FloatFieldMinPositiveValue", REQUIRED,
                   R"({"optionalFloat": 1.175494e-38})",
                   "optional_float: 1.175494e-38");
  RunValidJsonTest("FloatFieldMaxNegativeValue", REQUIRED,
                   R"({"optionalFloat": -1.175494e-38})",
                   "optional_float: -1.175494e-38");
  RunValidJsonTest("FloatFieldMaxPositiveValue", REQUIRED,
                   R"({"optionalFloat": 3.402823e+38})",
                   "optional_float: 3.402823e+38");
  RunValidJsonTest("FloatFieldMinNegativeValue", REQUIRED,
                   R"({"optionalFloat": 3.402823e+38})",
                   "optional_float: 3.402823e+38");
  // Values can be quoted.
  RunValidJsonTest("FloatFieldQuotedValue", REQUIRED,
                   R"({"optionalFloat": "1"})", "optional_float: 1");
  RunValidJsonTest("FloatFieldQuotedExponentialValue", REQUIRED,
                   R"({"optionalFloat": "1.175494e-38"})",
                   "optional_float: 1.175494e-38");
  // Special values.
  RunValidJsonTest("FloatFieldNan", REQUIRED, R"({"optionalFloat": "NaN"})",
                   "optional_float: nan");
  RunValidJsonTest("FloatFieldInfinity", REQUIRED,
                   R"({"optionalFloat": "Infinity"})", "optional_float: inf");
  RunValidJsonTest("FloatFieldNegativeInfinity", REQUIRED,
                   R"({"optionalFloat": "-Infinity"})", "optional_float: -inf");
  // Non-canonical Nan will be correctly normalized.
  {
    MessageType message;
    // IEEE floating-point standard 32-bit quiet NaN:
    //   0111 1111 1xxx xxxx xxxx xxxx xxxx xxxx
    message.set_optional_float(WireFormatLite::DecodeFloat(0x7FA12345));
    RunValidJsonTestWithProtobufInput("FloatFieldNormalizeQuietNan", REQUIRED,
                                      message, "optional_float: nan");
    // IEEE floating-point standard 64-bit signaling NaN:
    //   1111 1111 1xxx xxxx xxxx xxxx xxxx xxxx
    message.set_optional_float(WireFormatLite::DecodeFloat(0xFFB54321));
    RunValidJsonTestWithProtobufInput("FloatFieldNormalizeSignalingNan",
                                      REQUIRED, message, "optional_float: nan");
  }

  // Special values must be quoted.
  ExpectParseFailureForJson("FloatFieldNanNotQuoted", RECOMMENDED,
                            R"({"optionalFloat": NaN})");
  ExpectParseFailureForJson("FloatFieldInfinityNotQuoted", RECOMMENDED,
                            R"({"optionalFloat": Infinity})");
  ExpectParseFailureForJson("FloatFieldNegativeInfinityNotQuoted", RECOMMENDED,
                            R"({"optionalFloat": -Infinity})");

  // Parsers should reject out-of-bound values.
  ExpectParseFailureForJson("FloatFieldTooSmall", REQUIRED,
                            R"({"optionalFloat": -3.502823e+38})");
  ExpectParseFailureForJson("FloatFieldTooLarge", REQUIRED,
                            R"({"optionalFloat": 3.502823e+38})");

  // Parsers should reject empty string values.
  ExpectParseFailureForJson("FloatFieldEmptyString", REQUIRED,
                            R"({"optionalFloat": ""})");

  // Parser reject non-numeric string values.
  ExpectParseFailureForJson("FloatFieldStringValuePartiallyNumeric", REQUIRED,
                            R"({"optionalFloat": "12abc"})");
  ExpectParseFailureForJson("FloatFieldStringValueNonNumeric", REQUIRED,
                            R"({"optionalFloat": "abc"})");

  // Double fields.
  RunValidJsonTest("DoubleFieldMinPositiveValue", REQUIRED,
                   R"({"optionalDouble": 2.22507e-308})",
                   "optional_double: 2.22507e-308");
  RunValidJsonTest("DoubleFieldMaxNegativeValue", REQUIRED,
                   R"({"optionalDouble": -2.22507e-308})",
                   "optional_double: -2.22507e-308");
  RunValidJsonTest("DoubleFieldMaxPositiveValue", REQUIRED,
                   R"({"optionalDouble": 1.79769e+308})",
                   "optional_double: 1.79769e+308");
  RunValidJsonTest("DoubleFieldMinNegativeValue", REQUIRED,
                   R"({"optionalDouble": -1.79769e+308})",
                   "optional_double: -1.79769e+308");
  // Values can be quoted.
  RunValidJsonTest("DoubleFieldQuotedValue", REQUIRED,
                   R"({"optionalDouble": "1"})", "optional_double: 1");
  RunValidJsonTest("DoubleFieldQuotedExponentialValue", REQUIRED,
                   R"({"optionalDouble": "2.22507e-308"})",
                   "optional_double: 2.22507e-308");
  // Special values.
  RunValidJsonTest("DoubleFieldNan", REQUIRED, R"({"optionalDouble": "NaN"})",
                   "optional_double: nan");
  RunValidJsonTest("DoubleFieldInfinity", REQUIRED,
                   R"({"optionalDouble": "Infinity"})", "optional_double: inf");
  RunValidJsonTest("DoubleFieldNegativeInfinity", REQUIRED,
                   R"({"optionalDouble": "-Infinity"})",
                   "optional_double: -inf");
  // Non-canonical Nan will be correctly normalized.
  {
    MessageType message;
    message.set_optional_double(
        WireFormatLite::DecodeDouble(int64_t{0x7FFA123456789ABC}));
    RunValidJsonTestWithProtobufInput("DoubleFieldNormalizeQuietNan", REQUIRED,
                                      message, "optional_double: nan");
    message.set_optional_double(
        WireFormatLite::DecodeDouble(uint64_t{0xFFFBCBA987654321}));
    RunValidJsonTestWithProtobufInput("DoubleFieldNormalizeSignalingNan",
                                      REQUIRED, message,
                                      "optional_double: nan");
  }

  // Special values must be quoted.
  ExpectParseFailureForJson("DoubleFieldNanNotQuoted", RECOMMENDED,
                            R"({"optionalDouble": NaN})");
  ExpectParseFailureForJson("DoubleFieldInfinityNotQuoted", RECOMMENDED,
                            R"({"optionalDouble": Infinity})");
  ExpectParseFailureForJson("DoubleFieldNegativeInfinityNotQuoted", RECOMMENDED,
                            R"({"optionalDouble": -Infinity})");

  // Parsers should reject out-of-bound values.
  ExpectParseFailureForJson("DoubleFieldTooSmall", REQUIRED,
                            R"({"optionalDouble": -1.89769e+308})");
  ExpectParseFailureForJson("DoubleFieldTooLarge", REQUIRED,
                            R"({"optionalDouble": +1.89769e+308})");

  // Parsers should reject empty string values.
  ExpectParseFailureForJson("DoubleFieldEmptyString", REQUIRED,
                            R"({"optionalDouble": ""})");

  // Parser reject non-numeric string values.
  ExpectParseFailureForJson("DoubleFieldStringValuePartiallyNumeric", REQUIRED,
                            R"({"optionalDouble": "12abc"})");
  ExpectParseFailureForJson("DoubleFieldStringValueNonNumeric", REQUIRED,
                            R"({"optionalDouble": "abc"})");

  // Enum fields.
  RunValidJsonTest("EnumField", REQUIRED, R"({"optionalNestedEnum": "FOO"})",
                   "optional_nested_enum: FOO");

  // Enum fields with alias
  if (run_proto3_tests_) {
    RunValidJsonTest("EnumFieldWithAlias", REQUIRED,
                     R"({"optionalAliasedEnum": "ALIAS_BAZ"})",
                     "optional_aliased_enum: ALIAS_BAZ");
    RunValidJsonTest("EnumFieldWithAliasUseAlias", REQUIRED,
                     R"({"optionalAliasedEnum": "MOO"})",
                     "optional_aliased_enum: ALIAS_BAZ");
    RunValidJsonTest("EnumFieldWithAliasLowerCase", REQUIRED,
                     R"({"optionalAliasedEnum": "moo"})",
                     "optional_aliased_enum: ALIAS_BAZ");
    RunValidJsonTest("EnumFieldWithAliasDifferentCase", REQUIRED,
                     R"({"optionalAliasedEnum": "bAz"})",
                     "optional_aliased_enum: ALIAS_BAZ");
  }

  // Enum values must be represented as strings.
  ExpectParseFailureForJson("EnumFieldNotQuoted", REQUIRED,
                            R"({"optionalNestedEnum": FOO})");
  // Numeric values are allowed.
  RunValidJsonTest("EnumFieldNumericValueZero", REQUIRED,
                   R"({"optionalNestedEnum": 0})", "optional_nested_enum: FOO");
  RunValidJsonTest("EnumFieldNumericValueNonZero", REQUIRED,
                   R"({"optionalNestedEnum": 1})", "optional_nested_enum: BAR");

  if (run_proto3_tests_) {
    // Unknown enum values are represented as numeric values.
    RunValidJsonTestWithValidator(
        "EnumFieldUnknownValue", REQUIRED, R"({"optionalNestedEnum": 123})",
        [](const Json::Value& value) {
          return value["optionalNestedEnum"].type() == Json::intValue &&
                 value["optionalNestedEnum"].asInt() == 123;
        });
  }

  // String fields.
  RunValidJsonTest("StringField", REQUIRED,
                   R"({"optionalString": "Hello world!"})",
                   R"(optional_string: "Hello world!")");
  RunValidJsonTest("StringFieldUnicode", REQUIRED,
                   // Google in Chinese.
                   R"({"optionalString": "谷歌"})",
                   R"(optional_string: "谷歌")");
  RunValidJsonTest("StringFieldEscape", REQUIRED,
                   R"({"optionalString": "\"\\\/\b\f\n\r\t"})",
                   R"(optional_string: "\"\\/\b\f\n\r\t")");
  RunValidJsonTest("StringFieldUnicodeEscape", REQUIRED,
                   R"({"optionalString": "\u8C37\u6B4C"})",
                   R"(optional_string: "谷歌")");
  RunValidJsonTest("StringFieldUnicodeEscapeWithLowercaseHexLetters", REQUIRED,
                   R"({"optionalString": "\u8c37\u6b4c"})",
                   R"(optional_string: "谷歌")");
  RunValidJsonTest(
      "StringFieldSurrogatePair", REQUIRED,
      // The character is an emoji: grinning face with smiling eyes. 😁
      R"({"optionalString": "\uD83D\uDE01"})",
      R"(optional_string: "\xF0\x9F\x98\x81")");
  RunValidJsonTest("StringFieldEmbeddedNull", REQUIRED,
                   R"({"optionalString": "Hello\u0000world!"})",
                   R"(optional_string: "Hello\000world!")");

  // Unicode escapes must start with "\u" (lowercase u).
  ExpectParseFailureForJson("StringFieldUppercaseEscapeLetter", RECOMMENDED,
                            R"({"optionalString": "\U8C37\U6b4C"})");
  ExpectParseFailureForJson("StringFieldInvalidEscape", RECOMMENDED,
                            R"({"optionalString": "\uXXXX\u6B4C"})");
  ExpectParseFailureForJson("StringFieldUnterminatedEscape", RECOMMENDED,
                            R"({"optionalString": "\u8C3"})");
  ExpectParseFailureForJson("StringFieldUnpairedHighSurrogate", RECOMMENDED,
                            R"({"optionalString": "\uD800"})");
  ExpectParseFailureForJson("StringFieldUnpairedLowSurrogate", RECOMMENDED,
                            R"({"optionalString": "\uDC00"})");
  ExpectParseFailureForJson("StringFieldSurrogateInWrongOrder", RECOMMENDED,
                            R"({"optionalString": "\uDE01\uD83D"})");
  ExpectParseFailureForJson("StringFieldNotAString", REQUIRED,
                            R"({"optionalString": 12345})");

  // Bytes fields.
  RunValidJsonTest("BytesField", REQUIRED, R"({"optionalBytes": "AQI="})",
                   R"(optional_bytes: "\x01\x02")");
  RunValidJsonTest("BytesFieldBase64Url", RECOMMENDED,
                   R"({"optionalBytes": "-_"})", R"(optional_bytes: "\xfb")");

  // Message fields.
  RunValidJsonTest("MessageField", REQUIRED,
                   R"({"optionalNestedMessage": {"a": 1234}})",
                   "optional_nested_message: {a: 1234}");

  // Oneof fields.
  ExpectParseFailureForJson("OneofFieldDuplicate", REQUIRED,
                            R"({"oneofUint32": 1, "oneofString": "test"})");
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

  // http://www.rfc-editor.org/rfc/rfc7159.txt says strings have to use double
  // quotes.
  ExpectParseFailureForJson("StringFieldSingleQuoteKey", RECOMMENDED,
                            R"({'optionalString': "Hello world!"})");
  ExpectParseFailureForJson("StringFieldSingleQuoteValue", RECOMMENDED,
                            R"({"optionalString": 'Hello world!'})");
  ExpectParseFailureForJson("StringFieldSingleQuoteBoth", RECOMMENDED,
                            R"({'optionalString': 'Hello world!'})");
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
    MessageType>::RunJsonTestsForNullTypes() {
  // "null" is accepted for all fields types.
  RunValidJsonTest("AllFieldAcceptNull", REQUIRED,
                   R"({
        "optionalInt32": null,
        "optionalInt64": null,
        "optionalUint32": null,
        "optionalUint64": null,
        "optionalSint32": null,
        "optionalSint64": null,
        "optionalFixed32": null,
        "optionalFixed64": null,
        "optionalSfixed32": null,
        "optionalSfixed64": null,
        "optionalFloat": null,
        "optionalDouble": null,
        "optionalBool": null,
        "optionalString": null,
        "optionalBytes": null,
        "optionalNestedEnum": null,
        "optionalNestedMessage": null,
        "repeatedInt32": null,
        "repeatedInt64": null,
        "repeatedUint32": null,
        "repeatedUint64": null,
        "repeatedSint32": null,
        "repeatedSint64": null,
        "repeatedFixed32": null,
        "repeatedFixed64": null,
        "repeatedSfixed32": null,
        "repeatedSfixed64": null,
        "repeatedFloat": null,
        "repeatedDouble": null,
        "repeatedBool": null,
        "repeatedString": null,
        "repeatedBytes": null,
        "repeatedNestedEnum": null,
        "repeatedNestedMessage": null,
        "mapInt32Int32": null,
        "mapBoolBool": null,
        "mapStringNestedMessage": null
      })",
                   "");

  // Repeated field elements cannot be null.
  ExpectParseFailureForJson("RepeatedFieldPrimitiveElementIsNull", RECOMMENDED,
                            R"({"repeatedInt32": [1, null, 2]})");
  ExpectParseFailureForJson(
      "RepeatedFieldMessageElementIsNull", RECOMMENDED,
      R"({"repeatedNestedMessage": [{"a":1}, null, {"a":2}]})");
  // Map field keys cannot be null.
  ExpectParseFailureForJson("MapFieldKeyIsNull", RECOMMENDED,
                            R"({"mapInt32Int32": {null: 1}})");
  // Map field values cannot be null.
  ExpectParseFailureForJson("MapFieldValueIsNull", RECOMMENDED,
                            R"({"mapInt32Int32": {"0": null}})");
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<
    MessageType>::RunJsonTestsForWrapperTypes() {
  RunValidJsonTest("OptionalBoolWrapper", REQUIRED,
                   R"({"optionalBoolWrapper": false})",
                   "optional_bool_wrapper: {value: false}");
  RunValidJsonTest("OptionalInt32Wrapper", REQUIRED,
                   R"({"optionalInt32Wrapper": 0})",
                   "optional_int32_wrapper: {value: 0}");
  RunValidJsonTest("OptionalUint32Wrapper", REQUIRED,
                   R"({"optionalUint32Wrapper": 0})",
                   "optional_uint32_wrapper: {value: 0}");
  RunValidJsonTest("OptionalInt64Wrapper", REQUIRED,
                   R"({"optionalInt64Wrapper": 0})",
                   "optional_int64_wrapper: {value: 0}");
  RunValidJsonTest("OptionalUint64Wrapper", REQUIRED,
                   R"({"optionalUint64Wrapper": 0})",
                   "optional_uint64_wrapper: {value: 0}");
  RunValidJsonTest("OptionalFloatWrapper", REQUIRED,
                   R"({"optionalFloatWrapper": 0})",
                   "optional_float_wrapper: {value: 0}");
  RunValidJsonTest("OptionalDoubleWrapper", REQUIRED,
                   R"({"optionalDoubleWrapper": 0})",
                   "optional_double_wrapper: {value: 0}");
  RunValidJsonTest("OptionalStringWrapper", REQUIRED,
                   R"({"optionalStringWrapper": ""})",
                   R"(optional_string_wrapper: {value: ""})");
  RunValidJsonTest("OptionalBytesWrapper", REQUIRED,
                   R"({"optionalBytesWrapper": ""})",
                   R"(optional_bytes_wrapper: {value: ""})");
  RunValidJsonTest("OptionalWrapperTypesWithNonDefaultValue", REQUIRED,
                   R"({
        "optionalBoolWrapper": true,
        "optionalInt32Wrapper": 1,
        "optionalUint32Wrapper": 1,
        "optionalInt64Wrapper": "1",
        "optionalUint64Wrapper": "1",
        "optionalFloatWrapper": 1,
        "optionalDoubleWrapper": 1,
        "optionalStringWrapper": "1",
        "optionalBytesWrapper": "AQI="
      })",
                   R"(
        optional_bool_wrapper: {value: true}
        optional_int32_wrapper: {value: 1}
        optional_uint32_wrapper: {value: 1}
        optional_int64_wrapper: {value: 1}
        optional_uint64_wrapper: {value: 1}
        optional_float_wrapper: {value: 1}
        optional_double_wrapper: {value: 1}
        optional_string_wrapper: {value: "1"}
        optional_bytes_wrapper: {value: "\x01\x02"}
      )");
  RunValidJsonTest("RepeatedBoolWrapper", REQUIRED,
                   R"({"repeatedBoolWrapper": [true, false]})",
                   "repeated_bool_wrapper: {value: true}"
                   "repeated_bool_wrapper: {value: false}");
  RunValidJsonTest("RepeatedInt32Wrapper", REQUIRED,
                   R"({"repeatedInt32Wrapper": [0, 1]})",
                   "repeated_int32_wrapper: {value: 0}"
                   "repeated_int32_wrapper: {value: 1}");
  RunValidJsonTest("RepeatedUint32Wrapper", REQUIRED,
                   R"({"repeatedUint32Wrapper": [0, 1]})",
                   "repeated_uint32_wrapper: {value: 0}"
                   "repeated_uint32_wrapper: {value: 1}");
  RunValidJsonTest("RepeatedInt64Wrapper", REQUIRED,
                   R"({"repeatedInt64Wrapper": [0, 1]})",
                   "repeated_int64_wrapper: {value: 0}"
                   "repeated_int64_wrapper: {value: 1}");
  RunValidJsonTest("RepeatedUint64Wrapper", REQUIRED,
                   R"({"repeatedUint64Wrapper": [0, 1]})",
                   "repeated_uint64_wrapper: {value: 0}"
                   "repeated_uint64_wrapper: {value: 1}");
  RunValidJsonTest("RepeatedFloatWrapper", REQUIRED,
                   R"({"repeatedFloatWrapper": [0, 1]})",
                   "repeated_float_wrapper: {value: 0}"
                   "repeated_float_wrapper: {value: 1}");
  RunValidJsonTest("RepeatedDoubleWrapper", REQUIRED,
                   R"({"repeatedDoubleWrapper": [0, 1]})",
                   "repeated_double_wrapper: {value: 0}"
                   "repeated_double_wrapper: {value: 1}");
  RunValidJsonTest("RepeatedStringWrapper", REQUIRED,
                   R"({"repeatedStringWrapper": ["", "AQI="]})",
                   R"(
        repeated_string_wrapper: {value: ""}
        repeated_string_wrapper: {value: "AQI="}
      )");
  RunValidJsonTest("RepeatedBytesWrapper", REQUIRED,
                   R"({"repeatedBytesWrapper": ["", "AQI="]})",
                   R"(
        repeated_bytes_wrapper: {value: ""}
        repeated_bytes_wrapper: {value: "\x01\x02"}
      )");
  RunValidJsonTest("WrapperTypesWithNullValue", REQUIRED,
                   R"({
        "optionalBoolWrapper": null,
        "optionalInt32Wrapper": null,
        "optionalUint32Wrapper": null,
        "optionalInt64Wrapper": null,
        "optionalUint64Wrapper": null,
        "optionalFloatWrapper": null,
        "optionalDoubleWrapper": null,
        "optionalStringWrapper": null,
        "optionalBytesWrapper": null,
        "repeatedBoolWrapper": null,
        "repeatedInt32Wrapper": null,
        "repeatedUint32Wrapper": null,
        "repeatedInt64Wrapper": null,
        "repeatedUint64Wrapper": null,
        "repeatedFloatWrapper": null,
        "repeatedDoubleWrapper": null,
        "repeatedStringWrapper": null,
        "repeatedBytesWrapper": null
      })",
                   "");

  // Duration
  RunValidJsonTest(
      "DurationMinValue", REQUIRED,
      R"({"optionalDuration": "-315576000000.999999999s"})",
      "optional_duration: {seconds: -315576000000 nanos: -999999999}");
  RunValidJsonTest(
      "DurationMaxValue", REQUIRED,
      R"({"optionalDuration": "315576000000.999999999s"})",
      "optional_duration: {seconds: 315576000000 nanos: 999999999}");
  RunValidJsonTest("DurationRepeatedValue", REQUIRED,
                   R"({"repeatedDuration": ["1.5s", "-1.5s"]})",
                   "repeated_duration: {seconds: 1 nanos: 500000000}"
                   "repeated_duration: {seconds: -1 nanos: -500000000}");
  RunValidJsonTest("DurationNull", REQUIRED, R"({"optionalDuration": null})",
                   "");
  RunValidJsonTest("DurationNegativeSeconds", REQUIRED,
                   R"({"optionalDuration": "-5s"})",
                   "optional_duration: {seconds: -5 nanos: 0}");
  RunValidJsonTest("DurationNegativeNanos", REQUIRED,
                   R"({"optionalDuration": "-0.5s"})",
                   "optional_duration: {seconds: 0 nanos: -500000000}");

  ExpectParseFailureForJson("DurationMissingS", REQUIRED,
                            R"({"optionalDuration": "1"})");
  ExpectParseFailureForJson(
      "DurationJsonInputTooSmall", REQUIRED,
      R"({"optionalDuration": "-315576000001.000000000s"})");
  ExpectParseFailureForJson(
      "DurationJsonInputTooLarge", REQUIRED,
      R"({"optionalDuration": "315576000001.000000000s"})");
  ExpectSerializeFailureForJson(
      "DurationProtoInputTooSmall", REQUIRED,
      "optional_duration: {seconds: -315576000001 nanos: 0}");
  ExpectSerializeFailureForJson(
      "DurationProtoInputTooLarge", REQUIRED,
      "optional_duration: {seconds: 315576000001 nanos: 0}");

  RunValidJsonTestWithValidator(
      "DurationHasZeroFractionalDigit", RECOMMENDED,
      R"({"optionalDuration": "1.000000000s"})", [](const Json::Value& value) {
        return value["optionalDuration"].asString() == "1s";
      });
  RunValidJsonTestWithValidator(
      "DurationHas3FractionalDigits", RECOMMENDED,
      R"({"optionalDuration": "1.010000000s"})", [](const Json::Value& value) {
        return value["optionalDuration"].asString() == "1.010s";
      });
  RunValidJsonTestWithValidator(
      "DurationHas6FractionalDigits", RECOMMENDED,
      R"({"optionalDuration": "1.000010000s"})", [](const Json::Value& value) {
        return value["optionalDuration"].asString() == "1.000010s";
      });
  RunValidJsonTestWithValidator(
      "DurationHas9FractionalDigits", RECOMMENDED,
      R"({"optionalDuration": "1.000000010s"})", [](const Json::Value& value) {
        return value["optionalDuration"].asString() == "1.000000010s";
      });

  // Timestamp
  RunValidJsonTest("TimestampMinValue", REQUIRED,
                   R"({"optionalTimestamp": "0001-01-01T00:00:00Z"})",
                   "optional_timestamp: {seconds: -62135596800}");
  RunValidJsonTest(
      "TimestampMaxValue", REQUIRED,
      R"({"optionalTimestamp": "9999-12-31T23:59:59.999999999Z"})",
      "optional_timestamp: {seconds: 253402300799 nanos: 999999999}");
  RunValidJsonTest(
      "TimestampRepeatedValue", REQUIRED,
      R"({
        "repeatedTimestamp": [
          "0001-01-01T00:00:00Z",
          "9999-12-31T23:59:59.999999999Z"
  ]
      })",
      "repeated_timestamp: {seconds: -62135596800}"
      "repeated_timestamp: {seconds: 253402300799 nanos: 999999999}");
  RunValidJsonTest("TimestampEpochValue", REQUIRED,
                   R"({"optionalTimestamp": "1970-01-01T00:00:00.000Z"})",
                   "optional_timestamp: {seconds: 0}");
  RunValidJsonTest("TimestampNanoAfterEpochlValue", REQUIRED,
                   R"({"optionalTimestamp": "1970-01-01T00:00:00.000000001Z"})",
                   "optional_timestamp: {seconds: 0 nanos: 1}");
  RunValidJsonTest("TimestampNanoBeforeEpochValue", REQUIRED,
                   R"({"optionalTimestamp": "1969-12-31T23:59:59.999999999Z"})",
                   "optional_timestamp: {seconds: -1 nanos: 999999999}");
  RunValidJsonTest("TimestampLittleAfterEpochlValue", REQUIRED,
                   R"({"optionalTimestamp": "1970-01-01T00:00:01.000000001Z"})",
                   "optional_timestamp: {seconds: 1 nanos: 1}");
  RunValidJsonTest("TimestampLittleBeforeEpochValue", REQUIRED,
                   R"({"optionalTimestamp": "1969-12-31T23:59:58.999999999Z"})",
                   "optional_timestamp: {seconds: -2 nanos: 999999999}");
  RunValidJsonTest("TimestampTenAndHalfSecondsAfterEpochValue", REQUIRED,
                   R"({"optionalTimestamp": "1970-01-01T00:00:10.500Z"})",
                   "optional_timestamp: {seconds: 10 nanos: 500000000}");
  RunValidJsonTest("TimestampTenAndHalfSecondsBeforeEpochValue", REQUIRED,
                   R"({"optionalTimestamp": "1969-12-31T23:59:49.500Z"})",
                   "optional_timestamp: {seconds: -11 nanos: 500000000}");
  RunValidJsonTest("TimestampLeap", REQUIRED,
                   R"({"optionalTimestamp": "1993-02-10T00:00:00.000Z"})",
                   "optional_timestamp: {seconds: 729302400}");
  RunValidJsonTest("TimestampWithPositiveOffset", REQUIRED,
                   R"({"optionalTimestamp": "1970-01-01T08:00:01+08:00"})",
                   "optional_timestamp: {seconds: 1}");
  RunValidJsonTest("TimestampWithNegativeOffset", REQUIRED,
                   R"({"optionalTimestamp": "1969-12-31T16:00:01-08:00"})",
                   "optional_timestamp: {seconds: 1}");
  RunValidJsonTest("TimestampNull", REQUIRED, R"({"optionalTimestamp": null})",
                   "");

  ExpectParseFailureForJson("TimestampJsonInputTooSmall", REQUIRED,
                            R"({"optionalTimestamp": "0000-01-01T00:00:00Z"})");
  ExpectParseFailureForJson(
      "TimestampJsonInputTooLarge", REQUIRED,
      R"({"optionalTimestamp": "10000-01-01T00:00:00Z"})");
  ExpectParseFailureForJson("TimestampJsonInputMissingZ", REQUIRED,
                            R"({"optionalTimestamp": "0001-01-01T00:00:00"})");
  ExpectParseFailureForJson("TimestampJsonInputMissingT", REQUIRED,
                            R"({"optionalTimestamp": "0001-01-01 00:00:00Z"})");
  ExpectParseFailureForJson("TimestampJsonInputLowercaseZ", REQUIRED,
                            R"({"optionalTimestamp": "0001-01-01T00:00:00z"})");
  ExpectParseFailureForJson("TimestampJsonInputLowercaseT", REQUIRED,
                            R"({"optionalTimestamp": "0001-01-01t00:00:00Z"})");
  ExpectParseFailureForJson(
      "TimestampWithMissingColonInOffset", REQUIRED,
      R"({"optionalTimestamp": "1970-01-01T08:00:01+0800"})");
  ExpectSerializeFailureForJson("TimestampProtoInputTooSmall", REQUIRED,
                                "optional_timestamp: {seconds: -62135596801}");
  ExpectSerializeFailureForJson("TimestampProtoInputTooLarge", REQUIRED,
                                "optional_timestamp: {seconds: 253402300800}");
  RunValidJsonTestWithValidator(
      "TimestampZeroNormalized", RECOMMENDED,
      R"({"optionalTimestamp": "1969-12-31T16:00:00-08:00"})",
      [](const Json::Value& value) {
        return value["optionalTimestamp"].asString() == "1970-01-01T00:00:00Z";
      });
  RunValidJsonTestWithValidator(
      "TimestampHasZeroFractionalDigit", RECOMMENDED,
      R"({"optionalTimestamp": "1970-01-01T00:00:00.000000000Z"})",
      [](const Json::Value& value) {
        return value["optionalTimestamp"].asString() == "1970-01-01T00:00:00Z";
      });
  RunValidJsonTestWithValidator(
      "TimestampHas3FractionalDigits", RECOMMENDED,
      R"({"optionalTimestamp": "1970-01-01T00:00:00.010000000Z"})",
      [](const Json::Value& value) {
        return value["optionalTimestamp"].asString() ==
               "1970-01-01T00:00:00.010Z";
      });
  RunValidJsonTestWithValidator(
      "TimestampHas6FractionalDigits", RECOMMENDED,
      R"({"optionalTimestamp": "1970-01-01T00:00:00.000010000Z"})",
      [](const Json::Value& value) {
        return value["optionalTimestamp"].asString() ==
               "1970-01-01T00:00:00.000010Z";
      });
  RunValidJsonTestWithValidator(
      "TimestampHas9FractionalDigits", RECOMMENDED,
      R"({"optionalTimestamp": "1970-01-01T00:00:00.000000010Z"})",
      [](const Json::Value& value) {
        return value["optionalTimestamp"].asString() ==
               "1970-01-01T00:00:00.000000010Z";
      });
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
}

template <typename MessageType>
void BinaryAndJsonConformanceSuiteImpl<MessageType>::RunJsonTestsForAny() {
  std::string type_url = GetTypeUrl(MessageType::GetDescriptor());
  RunValidJsonTest("Any", REQUIRED,
                   absl::Substitute(R"({
        "optionalAny": {
          "@type": "$0",
          "optionalInt32": 12345
  }
      })",
                                    GetTypeUrl(MessageType::GetDescriptor())),
                   absl::Substitute(R"(
        optional_any: {
          [$0] {
            optional_int32: 12345
          }
        }
      )",
                                    type_url));
  RunValidJsonTest("AnyNested", REQUIRED,
                   absl::Substitute(R"({
        "optionalAny": {
          "@type": "type.googleapis.com/google.protobuf.Any",
          "value": {
            "@type": "$0",
            "optionalInt32": 12345
    }
  }
      })",
                                    type_url),
                   absl::Substitute(R"(
        optional_any: {
          [type.googleapis.com/google.protobuf.Any] {
            [$0] {
              optional_int32: 12345
            }
          }
        }
      )",
                                    type_url));
  // The special "@type" tag is not required to appear first.
  RunValidJsonTest("AnyUnorderedTypeTag", REQUIRED,
                   absl::Substitute(R"({
        "optionalAny": {
          "optionalInt32": 12345,
          "@type": "$0"
        }
      })",
                                    type_url),
                   absl::Substitute(R"(
        optional_any: {
          [$0] {
            optional_int32: 12345
          }
        }
      )",
                                    type_url));
  // Well-known types in Any.
  RunValidJsonTest("AnyWithInt32ValueWrapper", REQUIRED,
                   R"({
        "optionalAny": {
          "@type": "type.googleapis.com/google.protobuf.Int32Value",
          "value": 12345
  }
      })",
                   R"(
        optional_any: {
    [type.googleapis.com/google.protobuf.Int32Value] {
            value: 12345
    }
  }
      )");
  RunValidJsonTest("AnyWithDuration", REQUIRED,
                   R"({
        "optionalAny": {
          "@type": "type.googleapis.com/google.protobuf.Duration",
          "value": "1.5s"
  }
      })",
                   R"(
        optional_any: {
    [type.googleapis.com/google.protobuf.Duration] {
            seconds: 1
            nanos: 500000000
    }
  }
      )");
  RunValidJsonTest("AnyWithTimestamp", REQUIRED,
                   R"({
        "optionalAny": {
          "@type": "type.googleapis.com/google.protobuf.Timestamp",
          "value": "1970-01-01T00:00:00Z"
  }
      })",
                   R"(
        optional_any: {
    [type.googleapis.com/google.protobuf.Timestamp] {
            seconds: 0
            nanos: 0
    }
  }
      )");
  RunValidJsonTest("AnyWithFieldMask", REQUIRED,
                   R"({
        "optionalAny": {
          "@type": "type.googleapis.com/google.protobuf.FieldMask",
          "value": "foo,barBaz"
  }
      })",
                   R"(
        optional_any: {
    [type.googleapis.com/google.protobuf.FieldMask] {
            paths: ["foo", "bar_baz"]
    }
  }
      )");
  RunValidJsonTest("AnyWithStruct", REQUIRED,
                   R"({
        "optionalAny": {
          "@type": "type.googleapis.com/google.protobuf.Struct",
          "value": {
            "foo": 1
    }
  }
      })",
                   R"(
        optional_any: {
    [type.googleapis.com/google.protobuf.Struct] {
            fields: {
              key: "foo"
              value: {
                number_value: 1
        }
      }
    }
  }
      )");
  RunValidJsonTest("AnyWithValueForJsonObject", REQUIRED,
                   R"({
        "optionalAny": {
          "@type": "type.googleapis.com/google.protobuf.Value",
          "value": {
            "foo": 1
    }
  }
      })",
                   R"(
        optional_any: {
    [type.googleapis.com/google.protobuf.Value] {
            struct_value: {
              fields: {
                key: "foo"
                value: {
                  number_value: 1
          }
        }
      }
    }
  }
      )");
  RunValidJsonTest("AnyWithValueForInteger", REQUIRED,
                   R"({
        "optionalAny": {
          "@type": "type.googleapis.com/google.protobuf.Value",
          "value": 1
  }
      })",
                   R"(
        optional_any: {
    [type.googleapis.com/google.protobuf.Value] {
            number_value: 1
    }
  }
      )");
  // When the Any is in WKT form (with "@type"), the type_url must be present
  // and URL shaped, otherwise it should be a parse error (because it can't be
  // parsed into the Any schema).
  ExpectParseFailureForJson("AnyWktRepresentationWithEmptyTypeAndValue",
                            REQUIRED,
                            R"({
        "optionalAny": {
          "@type": "",
          "value": ""
        }
      })");
  ExpectParseFailureForJson("AnyWktRepresentationWithBadType", REQUIRED,
                            R"({
        "optionalAny": {
          "@type": "not_a_url",
          "value": ""
        }
      })");
  // When the Any can be parsed as non-WKT form, the type_url could be missing
  // or invalid, since that can still be parsed into the Any schema.
  RunValidJsonTest("AnyWithNoType", REQUIRED,
                   R"({
        "optionalAny": {}
      })",
                   R"(
        optional_any: {}
      )");
  // `null` where an Any exists should just result in the field being unset.
  RunValidJsonTest("AnyNull", REQUIRED,
                   R"({
        "optionalAny": null
      })",
                   R"(
      )");
}

template <typename MessageType>
const FieldDescriptor*
BinaryAndJsonConformanceSuiteImpl<MessageType>::GetFieldForType(
    FieldDescriptor::Type type, bool repeated, Packed packed) const {
  const Descriptor* d = MessageType::GetDescriptor();
  for (int i = 0; i < d->field_count(); i++) {
    const FieldDescriptor* f = d->field(i);
    if (f->type() == type && f->is_repeated() == repeated) {
      if ((packed == Packed::kTrue && !f->is_packed()) ||
          (packed == Packed::kFalse && f->is_packed())) {
        continue;
      }
      return f;
    }
  }

  absl::string_view packed_string = "";
  const absl::string_view repeated_string =
      repeated ? "Repeated " : "Singular ";
  if (packed == Packed::kTrue) {
    packed_string = "Packed ";
  }
  if (packed == Packed::kFalse) {
    packed_string = "Unpacked ";
  }
  ABSL_LOG(FATAL) << "Couldn't find field with type: " << repeated_string
                  << packed_string << FieldDescriptor::TypeName(type) << " for "
                  << d->full_name();
  return nullptr;
}

template <typename MessageType>
const FieldDescriptor*
BinaryAndJsonConformanceSuiteImpl<MessageType>::GetFieldForMapType(
    FieldDescriptor::Type key_type, FieldDescriptor::Type value_type) const {
  const Descriptor* d = MessageType::GetDescriptor();
  for (int i = 0; i < d->field_count(); i++) {
    const FieldDescriptor* f = d->field(i);
    if (f->is_map()) {
      const Descriptor* map_entry = f->message_type();
      const FieldDescriptor* key = map_entry->field(0);
      const FieldDescriptor* value = map_entry->field(1);
      if (key->type() == key_type && value->type() == value_type) {
        return f;
      }
    }
  }

  ABSL_LOG(FATAL) << "Couldn't find map field with type: "
                  << FieldDescriptor::TypeName(key_type) << " and "
                  << FieldDescriptor::TypeName(key_type) << " for "
                  << d->full_name();
  return nullptr;
}

template <typename MessageType>
const FieldDescriptor*
BinaryAndJsonConformanceSuiteImpl<MessageType>::GetFieldForOneofType(
    FieldDescriptor::Type type, bool exclusive) const {
  const Descriptor* d = MessageType::GetDescriptor();
  for (int i = 0; i < d->field_count(); i++) {
    const FieldDescriptor* f = d->field(i);
    if (f->containing_oneof() && ((f->type() == type) ^ exclusive)) {
      return f;
    }
  }

  ABSL_LOG(FATAL) << "Couldn't find oneof field with type: "
                  << FieldDescriptor::TypeName(type) << " for "
                  << d->full_name();
  return nullptr;
}

template <typename MessageType>
std::string BinaryAndJsonConformanceSuiteImpl<MessageType>::SyntaxIdentifier()
    const {
  if (std::is_same<MessageType, TestAllTypesProto2>::value) {
    return "Proto2";
  } else if (std::is_same<MessageType, TestAllTypesProto3>::value) {
    return "Proto3";
  } else if (std::is_same<MessageType, TestAllTypesProto2Editions>::value) {
    return "Editions_Proto2";
  } else {
    return "Editions_Proto3";
  }
}

}  // namespace protobuf
}  // namespace google
