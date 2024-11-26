// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "text_format_conformance_suite.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "absl/log/absl_log.h"
#include "absl/log/die_if_null.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "conformance_test.h"
#include "conformance/test_protos/test_messages_edition2023.pb.h"
#include "editions/golden/test_messages_proto2_editions.pb.h"
#include "editions/golden/test_messages_proto3_editions.pb.h"
#include "google/protobuf/test_messages_proto2.pb.h"
#include "google/protobuf/test_messages_proto3.pb.h"
#include "google/protobuf/text_format.h"

using conformance::ConformanceRequest;
using conformance::ConformanceResponse;
using conformance::TestStatus;
using conformance::WireFormat;
using protobuf_test_messages::editions::TestAllTypesEdition2023;
using protobuf_test_messages::proto2::TestAllTypesProto2;
using protobuf_test_messages::proto2::UnknownToTestAllTypes;
using protobuf_test_messages::proto3::TestAllTypesProto3;
using TestAllTypesProto2Editions =
    protobuf_test_messages::editions::proto2::TestAllTypesProto2;
using TestAllTypesProto3Editions =
    protobuf_test_messages::editions::proto3::TestAllTypesProto3;

namespace google {
namespace protobuf {

// The number of repetitions to use for performance tests.
// Corresponds approx to 500KB wireformat bytes.
static const size_t kPerformanceRepeatCount = 50000;

TextFormatConformanceTestSuite::TextFormatConformanceTestSuite() {
  SetFailureListFlagName("--text_format_failure_list");
}

bool TextFormatConformanceTestSuite::ParseTextFormatResponse(
    const ConformanceResponse& response,
    const ConformanceRequestSetting& setting, Message* test_message) {
  TextFormat::Parser parser;
  const ConformanceRequest& request = setting.GetRequest();
  if (request.print_unknown_fields()) {
    parser.AllowFieldNumber(true);
  }
  if (!parser.ParseFromString(response.text_payload(), test_message)) {
    ABSL_LOG(ERROR) << "INTERNAL ERROR: internal text->protobuf transcode "
                    << "yielded unparseable proto. Text payload: "
                    << response.text_payload();
    return false;
  }

  return true;
}

bool TextFormatConformanceTestSuite::ParseResponse(
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
      if (requested_output != conformance::PROTOBUF) {
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

    case ConformanceResponse::kTextPayload: {
      if (requested_output != conformance::TEXT_FORMAT) {
        test.set_failure_message(absl::StrCat(
            "Test was asked for ", WireFormatToString(requested_output),
            " output but provided TEXT_FORMAT instead."));
        ReportFailure(test, level, request, response);
        return false;
      }

      if (!ParseTextFormatResponse(response, setting, test_message)) {
        test.set_failure_message(
            "TEXT_FORMAT output we received from test was unparseable.");
        ReportFailure(test, level, request, response);
        return false;
      }

      break;
    }

    default:
      ABSL_LOG(FATAL) << test_name
                      << ": unknown payload type: " << response.result_case();
  }

  return true;
}

void TextFormatConformanceTestSuite::RunSuiteImpl() {
  TextFormatConformanceTestSuiteImpl<TestAllTypesProto2>(this);
  TextFormatConformanceTestSuiteImpl<TestAllTypesProto3>(this);
  if (maximum_edition_ >= Edition::EDITION_2023) {
    TextFormatConformanceTestSuiteImpl<TestAllTypesProto2Editions>(this);
    TextFormatConformanceTestSuiteImpl<TestAllTypesProto3Editions>(this);
    TextFormatConformanceTestSuiteImpl<TestAllTypesEdition2023>(this);
  }
}

template <typename MessageType>
TextFormatConformanceTestSuiteImpl<MessageType>::
    TextFormatConformanceTestSuiteImpl(TextFormatConformanceTestSuite* suite)
    : suite_(*ABSL_DIE_IF_NULL(suite)) {
  // Flag control performance tests to keep them internal and opt-in only
  if (suite_.performance_) {
    if (MessageType::GetDescriptor()->name() == "TestAllTypesEdition2023") {
      // There are no editions-sensitive performance tests.
      return;
    }
    RunTextFormatPerformanceTests();
  } else {
    if (MessageType::GetDescriptor()->name() == "TestAllTypesProto2") {
      RunGroupTests();
      RunClosedEnumTests();
    }
    if (MessageType::GetDescriptor()->name() == "TestAllTypesEdition2023") {
      RunDelimitedTests();
    }
    if (MessageType::GetDescriptor()->name() == "TestAllTypesProto3") {
      RunAnyTests();
      RunOpenEnumTests();
      // TODO Run these over proto2 also.
      RunAllTests();
    }
  }
}

template <typename MessageType>
void TextFormatConformanceTestSuiteImpl<MessageType>::ExpectParseFailure(
    const std::string& test_name, ConformanceLevel level,
    const std::string& input) {
  MessageType prototype;
  // We don't expect output, but if the program erroneously accepts the protobuf
  // we let it send its response as this.  We must not leave it unspecified.
  ConformanceRequestSetting setting(
      level, conformance::TEXT_FORMAT, conformance::TEXT_FORMAT,
      conformance::TEXT_FORMAT_TEST, prototype, test_name, input);
  const ConformanceRequest& request = setting.GetRequest();
  ConformanceResponse response;
  std::string effective_test_name = absl::StrCat(
      setting.ConformanceLevelToString(level), ".",
      setting.GetSyntaxIdentifier(), ".TextFormatInput.", test_name);

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
void TextFormatConformanceTestSuiteImpl<MessageType>::RunValidTextFormatTest(
    const std::string& test_name, ConformanceLevel level,
    const std::string& input_text) {
  MessageType prototype;
  RunValidTextFormatTestWithMessage(test_name, level, input_text, prototype);
}

template <typename MessageType>
void TextFormatConformanceTestSuiteImpl<MessageType>::
    RunValidTextFormatTestWithMessage(const std::string& test_name,
                                      ConformanceLevel level,
                                      const std::string& input_text,
                                      const Message& message) {
  ConformanceRequestSetting setting1(
      level, conformance::TEXT_FORMAT, conformance::PROTOBUF,
      conformance::TEXT_FORMAT_TEST, message, test_name, input_text);
  suite_.RunValidInputTest(setting1, input_text);
  ConformanceRequestSetting setting2(
      level, conformance::TEXT_FORMAT, conformance::TEXT_FORMAT,
      conformance::TEXT_FORMAT_TEST, message, test_name, input_text);
  suite_.RunValidInputTest(setting2, input_text);
}

template <typename MessageType>
void TextFormatConformanceTestSuiteImpl<MessageType>::
    RunValidTextFormatTestWithExpected(const std::string& test_name,
                                       ConformanceLevel level,
                                       const std::string& input_text,
                                       const std::string& expected_text) {
  MessageType prototype;
  ConformanceRequestSetting setting1(
      level, conformance::TEXT_FORMAT, conformance::PROTOBUF,
      conformance::TEXT_FORMAT_TEST, prototype, test_name, input_text);
  suite_.RunValidInputTest(setting1, expected_text);
  ConformanceRequestSetting setting2(
      level, conformance::TEXT_FORMAT, conformance::TEXT_FORMAT,
      conformance::TEXT_FORMAT_TEST, prototype, test_name, input_text);
  suite_.RunValidInputTest(setting2, expected_text);
}

template <typename MessageType>
void TextFormatConformanceTestSuiteImpl<
    MessageType>::RunValidUnknownTextFormatTest(const std::string& test_name,
                                                const Message& message) {
  std::string serialized_input;
  message.SerializeToString(&serialized_input);
  MessageType prototype;
  ConformanceRequestSetting setting1(
      RECOMMENDED, conformance::PROTOBUF, conformance::TEXT_FORMAT,
      conformance::TEXT_FORMAT_TEST, prototype,
      absl::StrCat(test_name, "_Drop"), serialized_input);
  setting1.SetPrototypeMessageForCompare(message);
  suite_.RunValidBinaryInputTest(setting1, "");

  ConformanceRequestSetting setting2(
      RECOMMENDED, conformance::PROTOBUF, conformance::TEXT_FORMAT,
      conformance::TEXT_FORMAT_TEST, prototype,
      absl::StrCat(test_name, "_Print"), serialized_input);
  setting2.SetPrototypeMessageForCompare(message);
  setting2.SetPrintUnknownFields(true);
  suite_.RunValidBinaryInputTest(setting2, serialized_input);
}

template <typename MessageType>
void TextFormatConformanceTestSuiteImpl<MessageType>::RunDelimitedTests() {
  RunValidTextFormatTest("GroupFieldNoColon", REQUIRED,
                         "GroupLikeType { group_int32: 1 }");
  RunValidTextFormatTest("GroupFieldWithColon", REQUIRED,
                         "GroupLikeType: { group_int32: 1 }");
  RunValidTextFormatTest("GroupFieldEmpty", REQUIRED, "GroupLikeType {}");
  RunValidTextFormatTest(
      "GroupFieldExtension", REQUIRED,
      "[protobuf_test_messages.editions.groupliketype] { c: 1 }");
  RunValidTextFormatTest(
      "DelimitedFieldExtension", REQUIRED,
      "[protobuf_test_messages.editions.delimited_ext] { c: 1 }");


  // Test that lower-cased group name (i.e. implicit field name) are accepted.
  RunValidTextFormatTest("DelimitedFieldLowercased", REQUIRED,
                         "groupliketype { group_int32: 1 }");
  RunValidTextFormatTest("DelimitedFieldLowercasedDifferent", REQUIRED,
                         "delimited_field { group_int32: 1 }");

  // Extensions always used the field name, and should never accept the message
  // name.
  ExpectParseFailure(
      "DelimitedFieldExtensionMessageName", REQUIRED,
      "[protobuf_test_messages.editions.GroupLikeType] { group_int32: 1 }");
}

template <typename MessageType>
void TextFormatConformanceTestSuiteImpl<MessageType>::RunGroupTests() {
  RunValidTextFormatTest("GroupFieldNoColon", REQUIRED,
                         "Data { group_int32: 1 }");
  RunValidTextFormatTest("GroupFieldWithColon", REQUIRED,
                         "Data: { group_int32: 1 }");
  RunValidTextFormatTest("GroupFieldEmpty", REQUIRED, "Data {}");
  RunValidTextFormatTest("GroupFieldMultiWord", REQUIRED,
                         "MultiWordGroupField { group_int32: 1 }");

  // Test that lower-cased group name (i.e. implicit field name) is accepted
  RunValidTextFormatTest("GroupFieldLowercased", REQUIRED,
                         "data { group_int32: 1 }");
  RunValidTextFormatTest("GroupFieldLowercasedMultiWord", REQUIRED,
                         "multiwordgroupfield { group_int32: 1 }");

  // Test extensions of group type
  RunValidTextFormatTest("GroupFieldExtension", REQUIRED,
                         absl::StrFormat("[%s] { group_int32: 1 }",
                                         MessageType::GetDescriptor()
                                             ->file()
                                             ->FindExtensionByName("groupfield")
                                             ->PrintableNameForExtension()));
  ExpectParseFailure("GroupFieldExtensionGroupName", REQUIRED,
                     absl::StrFormat("[%s] { group_int32: 1 }",
                                     MessageType::GetDescriptor()
                                         ->file()
                                         ->FindMessageTypeByName("GroupField")
                                         ->full_name()));
}

template <typename MessageType>
void TextFormatConformanceTestSuiteImpl<MessageType>::RunAllTests() {
  RunValidTextFormatTest("HelloWorld", REQUIRED,
                         "optional_string: 'Hello, World!'");
  // Integer fields.
  RunValidTextFormatTest("Int32FieldMaxValue", REQUIRED,
                         "optional_int32: 2147483647");
  RunValidTextFormatTest("Int32FieldMinValue", REQUIRED,
                         "optional_int32: -2147483648");
  RunValidTextFormatTest("Uint32FieldMaxValue", REQUIRED,
                         "optional_uint32: 4294967295");
  RunValidTextFormatTest("Int64FieldMaxValue", REQUIRED,
                         "optional_int64: 9223372036854775807");
  RunValidTextFormatTest("Int64FieldMinValue", REQUIRED,
                         "optional_int64: -9223372036854775808");
  RunValidTextFormatTest("Uint64FieldMaxValue", REQUIRED,
                         "optional_uint64: 18446744073709551615");
  // Integer fields - Hex
  RunValidTextFormatTestWithExpected("Int32FieldMaxValueHex", REQUIRED,
                                     "optional_int32: 0x7FFFFFFF",
                                     "optional_int32: 2147483647");
  RunValidTextFormatTestWithExpected("Int32FieldMinValueHex", REQUIRED,
                                     "optional_int32: -0x80000000",
                                     "optional_int32: -2147483648");
  RunValidTextFormatTestWithExpected("Uint32FieldMaxValueHex", REQUIRED,
                                     "optional_uint32: 0xFFFFFFFF",
                                     "optional_uint32: 4294967295");
  RunValidTextFormatTestWithExpected("Int64FieldMaxValueHex", REQUIRED,
                                     "optional_int64: 0x7FFFFFFFFFFFFFFF",
                                     "optional_int64: 9223372036854775807");
  RunValidTextFormatTestWithExpected("Int64FieldMinValueHex", REQUIRED,
                                     "optional_int64: -0x8000000000000000",
                                     "optional_int64: -9223372036854775808");
  RunValidTextFormatTestWithExpected("Uint64FieldMaxValueHex", REQUIRED,
                                     "optional_uint64: 0xFFFFFFFFFFFFFFFF",
                                     "optional_uint64: 18446744073709551615");
  // Integer fields - Octal
  RunValidTextFormatTestWithExpected("Int32FieldMaxValueOctal", REQUIRED,
                                     "optional_int32: 017777777777",
                                     "optional_int32: 2147483647");
  RunValidTextFormatTestWithExpected("Int32FieldMinValueOctal", REQUIRED,
                                     "optional_int32: -020000000000",
                                     "optional_int32: -2147483648");
  RunValidTextFormatTestWithExpected("Uint32FieldMaxValueOctal", REQUIRED,
                                     "optional_uint32: 037777777777",
                                     "optional_uint32: 4294967295");
  RunValidTextFormatTestWithExpected("Int64FieldMaxValueOctal", REQUIRED,
                                     "optional_int64: 0777777777777777777777",
                                     "optional_int64: 9223372036854775807");
  RunValidTextFormatTestWithExpected("Int64FieldMinValueOctal", REQUIRED,
                                     "optional_int64: -01000000000000000000000",
                                     "optional_int64: -9223372036854775808");
  RunValidTextFormatTestWithExpected("Uint64FieldMaxValueOctal", REQUIRED,
                                     "optional_uint64: 01777777777777777777777",
                                     "optional_uint64: 18446744073709551615");

  // Parsers reject out-of-bound integer values.
  ExpectParseFailure("Int32FieldTooLarge", REQUIRED,
                     "optional_int32: 2147483648");
  ExpectParseFailure("Int32FieldTooSmall", REQUIRED,
                     "optional_int32: -2147483649");
  ExpectParseFailure("Uint32FieldTooLarge", REQUIRED,
                     "optional_uint32: 4294967296");
  ExpectParseFailure("Int64FieldTooLarge", REQUIRED,
                     "optional_int64: 9223372036854775808");
  ExpectParseFailure("Int64FieldTooSmall", REQUIRED,
                     "optional_int64: -9223372036854775809");
  ExpectParseFailure("Uint64FieldTooLarge", REQUIRED,
                     "optional_uint64: 18446744073709551616");
  // Parsers reject out-of-bound integer values - Hex
  ExpectParseFailure("Int32FieldTooLargeHex", REQUIRED,
                     "optional_int32: 0x80000000");
  ExpectParseFailure("Int32FieldTooSmallHex", REQUIRED,
                     "optional_int32: -0x80000001");
  ExpectParseFailure("Uint32FieldTooLargeHex", REQUIRED,
                     "optional_uint32: 0x100000000");
  ExpectParseFailure("Int64FieldTooLargeHex", REQUIRED,
                     "optional_int64: 0x8000000000000000");
  ExpectParseFailure("Int64FieldTooSmallHex", REQUIRED,
                     "optional_int64: -0x8000000000000001");
  ExpectParseFailure("Uint64FieldTooLargeHex", REQUIRED,
                     "optional_uint64: 0x10000000000000000");
  // Parsers reject out-of-bound integer values - Octal
  ExpectParseFailure("Int32FieldTooLargeOctal", REQUIRED,
                     "optional_int32: 020000000000");
  ExpectParseFailure("Int32FieldTooSmallOctal", REQUIRED,
                     "optional_int32: -020000000001");
  ExpectParseFailure("Uint32FieldTooLargeOctal", REQUIRED,
                     "optional_uint32: 040000000000");
  ExpectParseFailure("Int64FieldTooLargeOctal", REQUIRED,
                     "optional_int64: 01000000000000000000000");
  ExpectParseFailure("Int64FieldTooSmallOctal", REQUIRED,
                     "optional_int64: -01000000000000000000001");
  ExpectParseFailure("Uint64FieldTooLargeOctal", REQUIRED,
                     "optional_uint64: 02000000000000000000000");

  // Floating point fields
  for (const auto& suffix : std::vector<std::string>{"", "f", "F"}) {
    const std::string name_suffix =
        suffix.empty() ? "" : absl::StrCat("_", suffix);

    RunValidTextFormatTest(absl::StrCat("FloatField", name_suffix), REQUIRED,
                           absl::StrCat("optional_float: 3.192837", suffix));
    RunValidTextFormatTestWithExpected(
        absl::StrCat("FloatFieldZero", name_suffix), REQUIRED,
        absl::StrCat("optional_float: 0", suffix),
        "" /* implicit presence, so zero means unset*/);
    RunValidTextFormatTest(absl::StrCat("FloatFieldNegative", name_suffix),
                           REQUIRED,
                           absl::StrCat("optional_float: -3.192837", suffix));
    RunValidTextFormatTest(
        absl::StrCat("FloatFieldWithVeryPreciseNumber", name_suffix), REQUIRED,
        absl::StrCat("optional_float: 3.123456789123456789", suffix));
    RunValidTextFormatTest(
        absl::StrCat("FloatFieldMaxValue", name_suffix), REQUIRED,
        absl::StrCat("optional_float: 3.4028235e+38", suffix));
    RunValidTextFormatTest(absl::StrCat("FloatFieldMinValue", name_suffix),
                           REQUIRED,
                           absl::StrCat("optional_float: 1.17549e-38", suffix));
    RunValidTextFormatTest(absl::StrCat("FloatFieldWithInt32Max", name_suffix),
                           REQUIRED,
                           absl::StrCat("optional_float: 4294967296", suffix));
    RunValidTextFormatTest(
        absl::StrCat("FloatFieldLargerThanInt64", name_suffix), REQUIRED,
        absl::StrCat("optional_float: 9223372036854775808", suffix));
    RunValidTextFormatTest(
        absl::StrCat("FloatFieldTooLarge", name_suffix), REQUIRED,
        absl::StrCat("optional_float: 3.4028235e+39", suffix));
    RunValidTextFormatTest(absl::StrCat("FloatFieldTooSmall", name_suffix),
                           REQUIRED,
                           absl::StrCat("optional_float: 1.17549e-39", suffix));
    RunValidTextFormatTest(
        absl::StrCat("FloatFieldLargerThanUint64", name_suffix), REQUIRED,
        absl::StrCat("optional_float: 18446744073709551616", suffix));
    // https://protobuf.dev/reference/protobuf/textformat-spec/#literals says
    // "-0" is a valid float literal. -0 should be considered not the same as 0
    // when considering implicit presence, and so should round trip.
    RunValidTextFormatTest(absl::StrCat("FloatFieldNegativeZero", name_suffix),
                           REQUIRED,
                           absl::StrCat("optional_float: -0", suffix));
    // https://protobuf.dev/reference/protobuf/textformat-spec/#literals says
    // ".123", "-.123", ".123e2" are a valid float literal.
    RunValidTextFormatTest(absl::StrCat("FloatFieldNoLeadingZero", name_suffix),
                           REQUIRED,
                           absl::StrCat("optional_float: .123", suffix));
    RunValidTextFormatTest(
        absl::StrCat("FloatFieldNegativeNoLeadingZero", name_suffix), REQUIRED,
        absl::StrCat("optional_float: -.123", suffix));
    RunValidTextFormatTest(
        absl::StrCat("FloatFieldNoLeadingZeroWithExponent", name_suffix),
        REQUIRED, absl::StrCat("optional_float: .123e2", suffix));
  }
  // https://protobuf.dev/reference/protobuf/textformat-spec/#value say case
  // doesn't matter for special values, test a few
  for (const auto& value : std::vector<std::string>{"nan", "NaN", "nAn"}) {
    RunValidTextFormatTest(absl::StrCat("FloatFieldValue_", value), REQUIRED,
                           absl::StrCat("optional_float: ", value));
  }
  for (const auto& value : std::vector<std::string>{
           "inf", "infinity", "INF", "INFINITY", "iNF", "inFINITY"}) {
    RunValidTextFormatTest(absl::StrCat("FloatFieldValue_Pos", value), REQUIRED,
                           absl::StrCat("optional_float: ", value));
    RunValidTextFormatTest(absl::StrCat("FloatFieldValue_Neg", value), REQUIRED,
                           absl::StrCat("optional_float: -", value));
  }
  // https://protobuf.dev/reference/protobuf/textformat-spec/#numeric and
  // https://protobuf.dev/reference/protobuf/textformat-spec/#value says
  // hex or octal float literals are invalid.
  ExpectParseFailure("FloatFieldNoHex", REQUIRED, "optional_float: 0x1");
  ExpectParseFailure("FloatFieldNoNegativeHex", REQUIRED,
                     "optional_float: -0x1");
  ExpectParseFailure("FloatFieldNoOctal", REQUIRED, "optional_float: 012");
  ExpectParseFailure("FloatFieldNoNegativeOctal", REQUIRED,
                     "optional_float: -012");
  // https://protobuf.dev/reference/protobuf/textformat-spec/#value says
  // overflows are mapped to infinity/-infinity.
  RunValidTextFormatTestWithExpected("FloatFieldOverflowInfinity", REQUIRED,
                                     "optional_float: 1e50",
                                     "optional_float: inf");
  RunValidTextFormatTestWithExpected("FloatFieldOverflowNegativeInfinity",
                                     REQUIRED, "optional_float: -1e50",
                                     "optional_float: -inf");
  RunValidTextFormatTestWithExpected("DoubleFieldOverflowInfinity", REQUIRED,
                                     "optional_double: 1e9999",
                                     "optional_double: inf");
  RunValidTextFormatTestWithExpected("DoubleFieldOverflowNegativeInfinity",
                                     REQUIRED, "optional_double: -1e9999",
                                     "optional_double: -inf");
  // Exponent is one more than uint64 max.
  RunValidTextFormatTestWithExpected(
      "FloatFieldOverflowInfinityHugeExponent", REQUIRED,
      "optional_float: 1e18446744073709551616", "optional_float: inf");
  RunValidTextFormatTestWithExpected(
      "DoubleFieldOverflowInfinityHugeExponent", REQUIRED,
      "optional_double: 1e18446744073709551616", "optional_double: inf");
  RunValidTextFormatTestWithExpected(
      "DoubleFieldLargeNegativeExponentParsesAsZero", REQUIRED,
      "optional_double: 1e-18446744073709551616", "");
  RunValidTextFormatTestWithExpected(
      "NegDoubleFieldLargeNegativeExponentParsesAsNegZero", REQUIRED,
      "optional_double: -1e-18446744073709551616", "optional_double: -0");

  RunValidTextFormatTestWithExpected(
      "FloatFieldLargeNegativeExponentParsesAsZero", REQUIRED,
      "optional_float: 1e-50", "");
  RunValidTextFormatTestWithExpected(
      "NegFloatFieldLargeNegativeExponentParsesAsNegZero", REQUIRED,
      "optional_float: -1e-50", "optional_float: -0");

  // String literals x {Strings, Bytes}
  for (const auto& field_type : std::vector<std::string>{"String", "Bytes"}) {
    const std::string field_name =
        field_type == "String" ? "optional_string" : "optional_bytes";
    RunValidTextFormatTest(
        absl::StrCat("StringLiteralConcat", field_type), REQUIRED,
        absl::StrCat(field_name, ": 'first' \"second\"\n'third'"));
    RunValidTextFormatTest(
        absl::StrCat("StringLiteralBasicEscapes", field_type), REQUIRED,
        absl::StrCat(field_name, ": '\\a\\b\\f\\n\\r\\t\\v\\?\\\\\\'\\\"'"));
    RunValidTextFormatTest(
        absl::StrCat("StringLiteralOctalEscapes", field_type), REQUIRED,
        absl::StrCat(field_name, ": '\\341\\210\\264'"));
    RunValidTextFormatTest(absl::StrCat("StringLiteralHexEscapes", field_type),
                           REQUIRED,
                           absl::StrCat(field_name, ": '\\xe1\\x88\\xb4'"));
    RunValidTextFormatTest(
        absl::StrCat("StringLiteralShortUnicodeEscape", field_type),
        RECOMMENDED, absl::StrCat(field_name, ": '\\u1234'"));
    RunValidTextFormatTest(
        absl::StrCat("StringLiteralLongUnicodeEscapes", field_type),
        RECOMMENDED, absl::StrCat(field_name, ": '\\U00001234\\U00010437'"));
    // String literals don't include line feeds.
    ExpectParseFailure(absl::StrCat("StringLiteralIncludesLF", field_type),
                       REQUIRED,
                       absl::StrCat(field_name, ": 'first line\nsecond line'"));
    // Unicode escapes don't include code points that lie beyond the planes
    // (> 0x10ffff).
    ExpectParseFailure(
        absl::StrCat("StringLiteralLongUnicodeEscapeTooLarge", field_type),
        REQUIRED, absl::StrCat(field_name, ": '\\U00110000'"));
    // Unicode escapes don't include surrogates.
    ExpectParseFailure(
        absl::StrCat("StringLiteralShortUnicodeEscapeSurrogatePair",
                     field_type),
        RECOMMENDED, absl::StrCat(field_name, ": '\\ud801\\udc37'"));
    ExpectParseFailure(
        absl::StrCat("StringLiteralShortUnicodeEscapeSurrogateFirstOnly",
                     field_type),
        RECOMMENDED, absl::StrCat(field_name, ": '\\ud800'"));
    ExpectParseFailure(
        absl::StrCat("StringLiteralShortUnicodeEscapeSurrogateSecondOnly",
                     field_type),
        RECOMMENDED, absl::StrCat(field_name, ": '\\udc00'"));
    ExpectParseFailure(
        absl::StrCat("StringLiteralLongUnicodeEscapeSurrogateFirstOnly",
                     field_type),
        RECOMMENDED, absl::StrCat(field_name, ": '\\U0000d800'"));
    ExpectParseFailure(
        absl::StrCat("StringLiteralLongUnicodeEscapeSurrogateSecondOnly",
                     field_type),
        RECOMMENDED, absl::StrCat(field_name, ": '\\U0000dc00'"));
    ExpectParseFailure(
        absl::StrCat("StringLiteralLongUnicodeEscapeSurrogatePair", field_type),
        RECOMMENDED, absl::StrCat(field_name, ": '\\U0000d801\\U00000dc37'"));
    ExpectParseFailure(
        absl::StrCat("StringLiteralUnicodeEscapeSurrogatePairLongShort",
                     field_type),
        RECOMMENDED, absl::StrCat(field_name, ": '\\U0000d801\\udc37'"));
    ExpectParseFailure(
        absl::StrCat("StringLiteralUnicodeEscapeSurrogatePairShortLong",
                     field_type),
        RECOMMENDED, absl::StrCat(field_name, ": '\\ud801\\U0000dc37'"));

    // The following method depend on the type of field, as strings have extra
    // validation.
    const auto test_method =
        field_type == "String"
            ? &TextFormatConformanceTestSuiteImpl::ExpectParseFailure
            : &TextFormatConformanceTestSuiteImpl::RunValidTextFormatTest;

    // String fields reject invalid UTF-8 byte sequences; bytes fields don't.
    (this->*test_method)(absl::StrCat(field_type, "FieldBadUTF8Octal"),
                         REQUIRED, absl::StrCat(field_name, ": '\\300'"));
    (this->*test_method)(absl::StrCat(field_type, "FieldBadUTF8Hex"), REQUIRED,
                         absl::StrCat(field_name, ": '\\xc0'"));
  }

  // Separators
  for (const auto& test_case : std::vector<std::pair<std::string, std::string>>{
           {"string", "\"abc\""},
           {"bytes", "\"abc\""},
           {"int32", "123"},
           {"bool", "true"},
           {"double", "1.23"},
           {"fixed32", "0x123"},
       }) {
    // Optional Field Separators
    for (const auto& field_type :
         std::vector<std::string>{"Single", "Repeated"}) {
      std::string field_name, field_value;
      if (field_type == "Single") {
        field_name = absl::StrCat("optional_", test_case.first);
        field_value = test_case.second;
      } else {
        field_name = absl::StrCat("repeated_", test_case.first);
        field_value = absl::StrCat("[", test_case.second, "]");
      }

      RunValidTextFormatTest(absl::StrCat("FieldSeparatorCommaTopLevel",
                                          field_type, "_", test_case.first),
                             REQUIRED,
                             absl::StrCat(field_name, ": ", field_value, ","));
      RunValidTextFormatTest(absl::StrCat("FieldSeparatorSemiTopLevelSingle",
                                          field_type, "_", test_case.first),
                             REQUIRED,
                             absl::StrCat(field_name, ": ", field_value, ";"));

      ExpectParseFailure(
          absl::StrCat("FieldSeparatorCommaTopLevelDuplicatesFails", field_type,
                       "_", test_case.first),
          REQUIRED, absl::StrCat(field_name, ": ", field_value, ",,"));
      ExpectParseFailure(
          absl::StrCat("FieldSeparatorSemiTopLevelDuplicateFails", field_type,
                       "_", test_case.first),
          REQUIRED, absl::StrCat(field_name, ": ", field_value, ";;"));
    }

    // Required List Separators
    RunValidTextFormatTest(
        absl::StrCat("ListSeparator_", test_case.first), REQUIRED,
        absl::StrCat("repeated_", test_case.first, ": [", test_case.second, ",",
                     test_case.second, "]"));
    ExpectParseFailure(
        absl::StrCat("ListSeparatorSemiFails_", test_case.first), REQUIRED,
        absl::StrCat("repeated_", test_case.first, ": [", test_case.second, ";",
                     test_case.second, "]"));
    // For string and bytes, if we skip the separator, the parser will treat
    // the two values as a single value.
    if (test_case.first == "string" || test_case.first == "bytes") {
      RunValidTextFormatTest(
          absl::StrCat("ListSeparatorMissingIsOneValue_", test_case.first),
          REQUIRED,
          absl::StrCat("repeated_", test_case.first, ": [", test_case.second,
                       " ", test_case.second, "]"));
    } else {
      ExpectParseFailure(
          absl::StrCat("ListSeparatorMissingFails_", test_case.first), REQUIRED,
          absl::StrCat("repeated_", test_case.first, ": [", test_case.second,
                       " ", test_case.second, "]"));
    }
    ExpectParseFailure(
        absl::StrCat("ListSeparatorDuplicateFails_", test_case.first), REQUIRED,
        absl::StrCat("repeated_", test_case.first, ": [", test_case.second,
                     ",,", test_case.second, "]"));
    ExpectParseFailure(
        absl::StrCat("ListSeparatorSingleTrailingFails_", test_case.first),
        REQUIRED,
        absl::StrCat("repeated_", test_case.first, ": [", test_case.second,
                     ",]"));
    ExpectParseFailure(
        absl::StrCat("ListSeparatorTwoValuesTrailingFails_", test_case.first),
        REQUIRED,
        absl::StrCat("repeated_", test_case.first, ": [", test_case.second, ",",
                     test_case.second, ",]"));
  }
  // The test message don't really have all types nested, so just check one
  // data type for the nested field separator support
  RunValidTextFormatTest("FieldSeparatorCommaNested", REQUIRED,
                         "optional_nested_message: { a: 123, }");
  RunValidTextFormatTest("FieldSeparatorSemiNested", REQUIRED,
                         "optional_nested_message: { a: 123; }");
  ExpectParseFailure("FieldSeparatorCommaNestedDuplicates", REQUIRED,
                     "optional_nested_message: { a: 123,, }");
  ExpectParseFailure("FieldSeparatorSemiNestedDuplicates", REQUIRED,
                     "optional_nested_message: { a: 123;; }");

  // Unknown Fields
  UnknownToTestAllTypes message;
  // Unable to print unknown Fixed32/Fixed64 fields as if they are known.
  // Fixed32/Fixed64 fields are not added in the tests.
  message.set_optional_int32(123);
  message.set_optional_string("hello");
  message.set_optional_bool(true);
  RunValidUnknownTextFormatTest("ScalarUnknownFields", message);

  message.Clear();
  message.mutable_nested_message()->set_c(111);
  RunValidUnknownTextFormatTest("MessageUnknownFields", message);

  message.Clear();
  message.mutable_optionalgroup()->set_a(321);
  RunValidUnknownTextFormatTest("GroupUnknownFields", message);

  message.add_repeated_int32(1);
  message.add_repeated_int32(2);
  message.add_repeated_int32(3);
  RunValidUnknownTextFormatTest("RepeatedUnknownFields", message);

  // Map fields
  MessageType prototype;
  (*prototype.mutable_map_string_string())["c"] = "value";
  (*prototype.mutable_map_string_string())["b"] = "value";
  (*prototype.mutable_map_string_string())["a"] = "value";
  RunValidTextFormatTestWithMessage("AlphabeticallySortedMapStringKeys",
                                    REQUIRED,
                                    R"(
        map_string_string {
          key: "a"
          value: "value"
        }
        map_string_string {
          key: "b"
          value: "value"
        }
        map_string_string {
          key: "c"
          value: "value"
        }
        )",
                                    prototype);

  prototype.Clear();
  (*prototype.mutable_map_int32_int32())[3] = 0;
  (*prototype.mutable_map_int32_int32())[2] = 0;
  (*prototype.mutable_map_int32_int32())[1] = 0;
  RunValidTextFormatTestWithMessage("AlphabeticallySortedMapIntKeys", REQUIRED,
                                    R"(
        map_int32_int32 {
          key: 1
          value: 0
        }
        map_int32_int32 {
          key: 2
          value: 0
        }
        map_int32_int32 {
          key: 3
          value: 0
        }
        )",
                                    prototype);

  prototype.Clear();
  (*prototype.mutable_map_bool_bool())[true] = false;
  (*prototype.mutable_map_bool_bool())[false] = false;
  RunValidTextFormatTestWithMessage("AlphabeticallySortedMapBoolKeys", REQUIRED,
                                    R"(
        map_bool_bool {
          key: false
          value: false
        }
        map_bool_bool {
          key: true
          value: false
        }
        )",
                                    prototype);

  prototype.Clear();
  ConformanceRequestSetting setting_map(
      REQUIRED, conformance::TEXT_FORMAT, conformance::PROTOBUF,
      conformance::TEXT_FORMAT_TEST, prototype, "DuplicateMapKey", R"(
        map_string_nested_message {
          key: "duplicate"
          value: { a: 123 }
        }
        map_string_nested_message {
          key: "duplicate"
          value: { corecursive: {} }
        }
        )");
  // The last-specified value will be retained in a parsed map
  suite_.RunValidInputTest(setting_map, R"(
        map_string_nested_message {
          key: "duplicate"
          value: { corecursive: {} }
        }
        )");
}

template <typename MessageType>
void TextFormatConformanceTestSuiteImpl<MessageType>::RunAnyTests() {
  // Any fields
  RunValidTextFormatTest("AnyField", REQUIRED,
                         R"(
        optional_any: {
          [type.googleapis.com/protobuf_test_messages.proto3.TestAllTypesProto3]
  { optional_int32: 12345
          }
        }
        )");
  RunValidTextFormatTest("AnyFieldWithRawBytes", REQUIRED,
                         R"(
        optional_any: {
          type_url:
  "type.googleapis.com/protobuf_test_messages.proto3.TestAllTypesProto3" value:
  "\b\271`"
        }
        )");
  ExpectParseFailure("AnyFieldWithInvalidType", REQUIRED,
                     R"(
        optional_any: {
          [type.googleapis.com/unknown] {
            optional_int32: 12345
          }
        }
        )");
}

template <typename MessageType>
void TextFormatConformanceTestSuiteImpl<
    MessageType>::RunTextFormatPerformanceTests() {
  TestTextFormatPerformanceMergeMessageWithRepeatedField("Bool",
                                                         "repeated_bool: true");
  TestTextFormatPerformanceMergeMessageWithRepeatedField(
      "Double", "repeated_double: 123");
  TestTextFormatPerformanceMergeMessageWithRepeatedField(
      "Int32", "repeated_uint32: 123");
  TestTextFormatPerformanceMergeMessageWithRepeatedField(
      "Int64", "repeated_uint64: 123");
  TestTextFormatPerformanceMergeMessageWithRepeatedField(
      "String", R"(repeated_string: "foo")");
  TestTextFormatPerformanceMergeMessageWithRepeatedField(
      "Bytes", R"(repeated_bytes: "foo")");
}

// This is currently considered valid input by some languages but not others
template <typename MessageType>
void TextFormatConformanceTestSuiteImpl<MessageType>::
    TestTextFormatPerformanceMergeMessageWithRepeatedField(
        const std::string& test_type_name, const std::string& message_field) {
  std::string recursive_message =
      absl::StrCat("recursive_message { ", message_field, " }");

  std::string input;
  for (size_t i = 0; i < kPerformanceRepeatCount; i++) {
    absl::StrAppend(&input, recursive_message);
  }

  std::string expected = "recursive_message { ";
  for (size_t i = 0; i < kPerformanceRepeatCount; i++) {
    absl::StrAppend(&expected, message_field, " ");
  }
  absl::StrAppend(&expected, "}");

  RunValidTextFormatTestWithExpected(
      absl::StrCat("TestTextFormatPerformanceMergeMessageWithRepeatedField",
                   test_type_name),
      RECOMMENDED, input, expected);
}

template <typename MessageType>
void TextFormatConformanceTestSuiteImpl<MessageType>::RunOpenEnumTests() {
  RunValidTextFormatTest("ClosedEnumFieldByNumber", REQUIRED,
                         R"(
        optional_nested_enum: 1
        )");
  RunValidTextFormatTest("ClosedEnumFieldWithUnknownNumber", REQUIRED,
                         R"(
        optional_nested_enum: 42
        )");
}

template <typename MessageType>
void TextFormatConformanceTestSuiteImpl<MessageType>::RunClosedEnumTests() {
  RunValidTextFormatTest("ClosedEnumFieldByNumber", REQUIRED,
                         R"(
        optional_nested_enum: 1
        )");
  ExpectParseFailure("ClosedEnumFieldWithUnknownNumber", REQUIRED,
                     R"(
        optional_nested_enum: 42
        )");
}

}  // namespace protobuf
}  // namespace google
