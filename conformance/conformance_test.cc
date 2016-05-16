// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <stdarg.h>
#include <string>
#include <fstream>

#include "conformance.pb.h"
#include "conformance_test.h"
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/stringprintf.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/util/json_util.h>
#include <google/protobuf/util/field_comparator.h>
#include <google/protobuf/util/message_differencer.h>
#include <google/protobuf/util/type_resolver_util.h>
#include <google/protobuf/wire_format_lite.h>

#include "third_party/jsoncpp/json.h"

using conformance::ConformanceRequest;
using conformance::ConformanceResponse;
using conformance::TestAllTypes;
using conformance::WireFormat;
using google::protobuf::Descriptor;
using google::protobuf::FieldDescriptor;
using google::protobuf::internal::WireFormatLite;
using google::protobuf::TextFormat;
using google::protobuf::util::DefaultFieldComparator;
using google::protobuf::util::JsonToBinaryString;
using google::protobuf::util::MessageDifferencer;
using google::protobuf::util::NewTypeResolverForDescriptorPool;
using google::protobuf::util::Status;
using std::string;

namespace {

static const char kTypeUrlPrefix[] = "type.googleapis.com";

static string GetTypeUrl(const Descriptor* message) {
  return string(kTypeUrlPrefix) + "/" + message->full_name();
}

/* Routines for building arbitrary protos *************************************/

// We would use CodedOutputStream except that we want more freedom to build
// arbitrary protos (even invalid ones).

const string empty;

string cat(const string& a, const string& b,
           const string& c = empty,
           const string& d = empty,
           const string& e = empty,
           const string& f = empty,
           const string& g = empty,
           const string& h = empty,
           const string& i = empty,
           const string& j = empty,
           const string& k = empty,
           const string& l = empty) {
  string ret;
  ret.reserve(a.size() + b.size() + c.size() + d.size() + e.size() + f.size() +
              g.size() + h.size() + i.size() + j.size() + k.size() + l.size());
  ret.append(a);
  ret.append(b);
  ret.append(c);
  ret.append(d);
  ret.append(e);
  ret.append(f);
  ret.append(g);
  ret.append(h);
  ret.append(i);
  ret.append(j);
  ret.append(k);
  ret.append(l);
  return ret;
}

// The maximum number of bytes that it takes to encode a 64-bit varint.
#define VARINT_MAX_LEN 10

size_t vencode64(uint64_t val, char *buf) {
  if (val == 0) { buf[0] = 0; return 1; }
  size_t i = 0;
  while (val) {
    uint8_t byte = val & 0x7fU;
    val >>= 7;
    if (val) byte |= 0x80U;
    buf[i++] = byte;
  }
  return i;
}

string varint(uint64_t x) {
  char buf[VARINT_MAX_LEN];
  size_t len = vencode64(x, buf);
  return string(buf, len);
}

// TODO: proper byte-swapping for big-endian machines.
string fixed32(void *data) { return string(static_cast<char*>(data), 4); }
string fixed64(void *data) { return string(static_cast<char*>(data), 8); }

string delim(const string& buf) { return cat(varint(buf.size()), buf); }
string uint32(uint32_t u32) { return fixed32(&u32); }
string uint64(uint64_t u64) { return fixed64(&u64); }
string flt(float f) { return fixed32(&f); }
string dbl(double d) { return fixed64(&d); }
string zz32(int32_t x) { return varint(WireFormatLite::ZigZagEncode32(x)); }
string zz64(int64_t x) { return varint(WireFormatLite::ZigZagEncode64(x)); }

string tag(uint32_t fieldnum, char wire_type) {
  return varint((fieldnum << 3) | wire_type);
}

string submsg(uint32_t fn, const string& buf) {
  return cat( tag(fn, WireFormatLite::WIRETYPE_LENGTH_DELIMITED), delim(buf) );
}

#define UNKNOWN_FIELD 666

uint32_t GetFieldNumberForType(FieldDescriptor::Type type, bool repeated) {
  const Descriptor* d = TestAllTypes().GetDescriptor();
  for (int i = 0; i < d->field_count(); i++) {
    const FieldDescriptor* f = d->field(i);
    if (f->type() == type && f->is_repeated() == repeated) {
      return f->number();
    }
  }
  GOOGLE_LOG(FATAL) << "Couldn't find field with type " << (int)type;
  return 0;
}

string UpperCase(string str) {
  for (int i = 0; i < str.size(); i++) {
    str[i] = toupper(str[i]);
  }
  return str;
}

}  // anonymous namespace

namespace google {
namespace protobuf {

void ConformanceTestSuite::ReportSuccess(const string& test_name) {
  if (expected_to_fail_.erase(test_name) != 0) {
    StringAppendF(&output_,
                  "ERROR: test %s is in the failure list, but test succeeded.  "
                  "Remove it from the failure list.\n",
                  test_name.c_str());
    unexpected_succeeding_tests_.insert(test_name);
  }
  successes_++;
}

void ConformanceTestSuite::ReportFailure(const string& test_name,
                                         const ConformanceRequest& request,
                                         const ConformanceResponse& response,
                                         const char* fmt, ...) {
  if (expected_to_fail_.erase(test_name) == 1) {
    expected_failures_++;
    if (!verbose_)
      return;
  } else {
    StringAppendF(&output_, "ERROR, test=%s: ", test_name.c_str());
    unexpected_failing_tests_.insert(test_name);
  }
  va_list args;
  va_start(args, fmt);
  StringAppendV(&output_, fmt, args);
  va_end(args);
  StringAppendF(&output_, " request=%s, response=%s\n",
                request.ShortDebugString().c_str(),
                response.ShortDebugString().c_str());
}

void ConformanceTestSuite::ReportSkip(const string& test_name,
                                      const ConformanceRequest& request,
                                      const ConformanceResponse& response) {
  if (verbose_) {
    StringAppendF(&output_, "SKIPPED, test=%s request=%s, response=%s\n",
                  test_name.c_str(), request.ShortDebugString().c_str(),
                  response.ShortDebugString().c_str());
  }
  skipped_.insert(test_name);
}

void ConformanceTestSuite::RunTest(const string& test_name,
                                   const ConformanceRequest& request,
                                   ConformanceResponse* response) {
  if (test_names_.insert(test_name).second == false) {
    GOOGLE_LOG(FATAL) << "Duplicated test name: " << test_name;
  }

  string serialized_request;
  string serialized_response;
  request.SerializeToString(&serialized_request);

  runner_->RunTest(test_name, serialized_request, &serialized_response);

  if (!response->ParseFromString(serialized_response)) {
    response->Clear();
    response->set_runtime_error("response proto could not be parsed.");
  }

  if (verbose_) {
    StringAppendF(&output_, "conformance test: name=%s, request=%s, response=%s\n",
                  test_name.c_str(),
                  request.ShortDebugString().c_str(),
                  response->ShortDebugString().c_str());
  }
}

void ConformanceTestSuite::RunValidInputTest(
    const string& test_name, const string& input, WireFormat input_format,
    const string& equivalent_text_format, WireFormat requested_output) {
  TestAllTypes reference_message;
  GOOGLE_CHECK(
      TextFormat::ParseFromString(equivalent_text_format, &reference_message))
          << "Failed to parse data for test case: " << test_name
          << ", data: " << equivalent_text_format;

  ConformanceRequest request;
  ConformanceResponse response;

  switch (input_format) {
    case conformance::PROTOBUF:
      request.set_protobuf_payload(input);
      break;

    case conformance::JSON:
      request.set_json_payload(input);
      break;

    default:
      GOOGLE_LOG(FATAL) << "Unspecified input format";
  }

  request.set_requested_output_format(requested_output);

  RunTest(test_name, request, &response);

  TestAllTypes test_message;

  switch (response.result_case()) {
    case ConformanceResponse::kParseError:
    case ConformanceResponse::kRuntimeError:
    case ConformanceResponse::kSerializeError:
      ReportFailure(test_name, request, response,
                    "Failed to parse JSON input or produce JSON output.");
      return;

    case ConformanceResponse::kSkipped:
      ReportSkip(test_name, request, response);
      return;

    case ConformanceResponse::kJsonPayload: {
      if (requested_output != conformance::JSON) {
        ReportFailure(
            test_name, request, response,
            "Test was asked for protobuf output but provided JSON instead.");
        return;
      }
      string binary_protobuf;
      Status status =
          JsonToBinaryString(type_resolver_.get(), type_url_,
                             response.json_payload(), &binary_protobuf);
      if (!status.ok()) {
        ReportFailure(test_name, request, response,
                      "JSON output we received from test was unparseable.");
        return;
      }

      if (!test_message.ParseFromString(binary_protobuf)) {
        ReportFailure(test_name, request, response,
                      "INTERNAL ERROR: internal JSON->protobuf transcode "
                      "yielded unparseable proto.");
        return;
      }

      break;
    }

    case ConformanceResponse::kProtobufPayload: {
      if (requested_output != conformance::PROTOBUF) {
        ReportFailure(
            test_name, request, response,
            "Test was asked for JSON output but provided protobuf instead.");
        return;
      }

      if (!test_message.ParseFromString(response.protobuf_payload())) {
        ReportFailure(test_name, request, response,
                      "Protobuf output we received from test was unparseable.");
        return;
      }

      break;
    }

    default:
      GOOGLE_LOG(FATAL) << test_name << ": unknown payload type: "
                        << response.result_case();
  }

  MessageDifferencer differencer;
  DefaultFieldComparator field_comparator;
  field_comparator.set_treat_nan_as_equal(true);
  differencer.set_field_comparator(&field_comparator);
  string differences;
  differencer.ReportDifferencesToString(&differences);

  if (differencer.Compare(reference_message, test_message)) {
    ReportSuccess(test_name);
  } else {
    ReportFailure(test_name, request, response,
                  "Output was not equivalent to reference message: %s.",
                  differences.c_str());
  }
}

// Expect that this precise protobuf will cause a parse error.
void ConformanceTestSuite::ExpectParseFailureForProto(
    const string& proto, const string& test_name) {
  ConformanceRequest request;
  ConformanceResponse response;
  request.set_protobuf_payload(proto);
  string effective_test_name = "ProtobufInput." + test_name;

  // We don't expect output, but if the program erroneously accepts the protobuf
  // we let it send its response as this.  We must not leave it unspecified.
  request.set_requested_output_format(conformance::PROTOBUF);

  RunTest(effective_test_name, request, &response);
  if (response.result_case() == ConformanceResponse::kParseError) {
    ReportSuccess(effective_test_name);
  } else if (response.result_case() == ConformanceResponse::kSkipped) {
    ReportSkip(effective_test_name, request, response);
  } else {
    ReportFailure(effective_test_name, request, response,
                  "Should have failed to parse, but didn't.");
  }
}

// Expect that this protobuf will cause a parse error, even if it is followed
// by valid protobuf data.  We can try running this twice: once with this
// data verbatim and once with this data followed by some valid data.
//
// TODO(haberman): implement the second of these.
void ConformanceTestSuite::ExpectHardParseFailureForProto(
    const string& proto, const string& test_name) {
  return ExpectParseFailureForProto(proto, test_name);
}

void ConformanceTestSuite::RunValidJsonTest(
    const string& test_name, const string& input_json,
    const string& equivalent_text_format) {
  RunValidInputTest("JsonInput." + test_name + ".ProtobufOutput", input_json,
                    conformance::JSON, equivalent_text_format,
                    conformance::PROTOBUF);
  RunValidInputTest("JsonInput." + test_name + ".JsonOutput", input_json,
                    conformance::JSON, equivalent_text_format,
                    conformance::JSON);
}

void ConformanceTestSuite::RunValidJsonTestWithProtobufInput(
    const string& test_name, const TestAllTypes& input,
    const string& equivalent_text_format) {
  RunValidInputTest("ProtobufInput." + test_name + ".JsonOutput",
                    input.SerializeAsString(), conformance::PROTOBUF,
                    equivalent_text_format, conformance::JSON);
}

// According to proto3 JSON specification, JSON serializers follow more strict
// rules than parsers (e.g., a serializer must serialize int32 values as JSON
// numbers while the parser is allowed to accept them as JSON strings). This
// method allows strict checking on a proto3 JSON serializer by inspecting
// the JSON output directly.
void ConformanceTestSuite::RunValidJsonTestWithValidator(
    const string& test_name, const string& input_json,
    const Validator& validator) {
  ConformanceRequest request;
  ConformanceResponse response;
  request.set_json_payload(input_json);
  request.set_requested_output_format(conformance::JSON);

  string effective_test_name = "JsonInput." + test_name + ".Validator";

  RunTest(effective_test_name, request, &response);

  if (response.result_case() == ConformanceResponse::kSkipped) {
    ReportSkip(effective_test_name, request, response);
    return;
  }

  if (response.result_case() != ConformanceResponse::kJsonPayload) {
    ReportFailure(effective_test_name, request, response,
                  "Expected JSON payload but got type %d.",
                  response.result_case());
    return;
  }
  Json::Reader reader;
  Json::Value value;
  if (!reader.parse(response.json_payload(), value)) {
    ReportFailure(effective_test_name, request, response,
                  "JSON payload cannot be parsed as valid JSON: %s",
                  reader.getFormattedErrorMessages().c_str());
    return;
  }
  if (!validator(value)) {
    ReportFailure(effective_test_name, request, response,
                  "JSON payload validation failed.");
    return;
  }
  ReportSuccess(effective_test_name);
}

void ConformanceTestSuite::ExpectParseFailureForJson(
    const string& test_name, const string& input_json) {
  ConformanceRequest request;
  ConformanceResponse response;
  request.set_json_payload(input_json);
  string effective_test_name = "JsonInput." + test_name;

  // We don't expect output, but if the program erroneously accepts the protobuf
  // we let it send its response as this.  We must not leave it unspecified.
  request.set_requested_output_format(conformance::JSON);

  RunTest(effective_test_name, request, &response);
  if (response.result_case() == ConformanceResponse::kParseError) {
    ReportSuccess(effective_test_name);
  } else if (response.result_case() == ConformanceResponse::kSkipped) {
    ReportSkip(effective_test_name, request, response);
  } else {
    ReportFailure(effective_test_name, request, response,
                  "Should have failed to parse, but didn't.");
  }
}

void ConformanceTestSuite::ExpectSerializeFailureForJson(
    const string& test_name, const string& text_format) {
  TestAllTypes payload_message;
  GOOGLE_CHECK(
      TextFormat::ParseFromString(text_format, &payload_message))
          << "Failed to parse: " << text_format;

  ConformanceRequest request;
  ConformanceResponse response;
  request.set_protobuf_payload(payload_message.SerializeAsString());
  string effective_test_name = test_name + ".JsonOutput";
  request.set_requested_output_format(conformance::JSON);

  RunTest(effective_test_name, request, &response);
  if (response.result_case() == ConformanceResponse::kSerializeError) {
    ReportSuccess(effective_test_name);
  } else if (response.result_case() == ConformanceResponse::kSkipped) {
    ReportSkip(effective_test_name, request, response);
  } else {
    ReportFailure(effective_test_name, request, response,
                  "Should have failed to serialize, but didn't.");
  }
}

void ConformanceTestSuite::TestPrematureEOFForType(FieldDescriptor::Type type) {
  // Incomplete values for each wire type.
  static const string incompletes[6] = {
    string("\x80"),     // VARINT
    string("abcdefg"),  // 64BIT
    string("\x80"),     // DELIMITED (partial length)
    string(),           // START_GROUP (no value required)
    string(),           // END_GROUP (no value required)
    string("abc")       // 32BIT
  };

  uint32_t fieldnum = GetFieldNumberForType(type, false);
  uint32_t rep_fieldnum = GetFieldNumberForType(type, true);
  WireFormatLite::WireType wire_type = WireFormatLite::WireTypeForFieldType(
      static_cast<WireFormatLite::FieldType>(type));
  const string& incomplete = incompletes[wire_type];
  const string type_name =
      UpperCase(string(".") + FieldDescriptor::TypeName(type));

  ExpectParseFailureForProto(
      tag(fieldnum, wire_type),
      "PrematureEofBeforeKnownNonRepeatedValue" + type_name);

  ExpectParseFailureForProto(
      tag(rep_fieldnum, wire_type),
      "PrematureEofBeforeKnownRepeatedValue" + type_name);

  ExpectParseFailureForProto(
      tag(UNKNOWN_FIELD, wire_type),
      "PrematureEofBeforeUnknownValue" + type_name);

  ExpectParseFailureForProto(
      cat( tag(fieldnum, wire_type), incomplete ),
      "PrematureEofInsideKnownNonRepeatedValue" + type_name);

  ExpectParseFailureForProto(
      cat( tag(rep_fieldnum, wire_type), incomplete ),
      "PrematureEofInsideKnownRepeatedValue" + type_name);

  ExpectParseFailureForProto(
      cat( tag(UNKNOWN_FIELD, wire_type), incomplete ),
      "PrematureEofInsideUnknownValue" + type_name);

  if (wire_type == WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
    ExpectParseFailureForProto(
        cat( tag(fieldnum, wire_type), varint(1) ),
        "PrematureEofInDelimitedDataForKnownNonRepeatedValue" + type_name);

    ExpectParseFailureForProto(
        cat( tag(rep_fieldnum, wire_type), varint(1) ),
        "PrematureEofInDelimitedDataForKnownRepeatedValue" + type_name);

    // EOF in the middle of delimited data for unknown value.
    ExpectParseFailureForProto(
        cat( tag(UNKNOWN_FIELD, wire_type), varint(1) ),
        "PrematureEofInDelimitedDataForUnknownValue" + type_name);

    if (type == FieldDescriptor::TYPE_MESSAGE) {
      // Submessage ends in the middle of a value.
      string incomplete_submsg =
          cat( tag(WireFormatLite::TYPE_INT32, WireFormatLite::WIRETYPE_VARINT),
                incompletes[WireFormatLite::WIRETYPE_VARINT] );
      ExpectHardParseFailureForProto(
          cat( tag(fieldnum, WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
               varint(incomplete_submsg.size()),
               incomplete_submsg ),
          "PrematureEofInSubmessageValue" + type_name);
    }
  } else if (type != FieldDescriptor::TYPE_GROUP) {
    // Non-delimited, non-group: eligible for packing.

    // Packed region ends in the middle of a value.
    ExpectHardParseFailureForProto(
        cat( tag(rep_fieldnum, WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
             varint(incomplete.size()),
             incomplete ),
        "PrematureEofInPackedFieldValue" + type_name);

    // EOF in the middle of packed region.
    ExpectParseFailureForProto(
        cat( tag(rep_fieldnum, WireFormatLite::WIRETYPE_LENGTH_DELIMITED),
             varint(1) ),
        "PrematureEofInPackedField" + type_name);
  }
}

void ConformanceTestSuite::SetFailureList(const string& filename,
                                          const vector<string>& failure_list) {
  failure_list_filename_ = filename;
  expected_to_fail_.clear();
  std::copy(failure_list.begin(), failure_list.end(),
            std::inserter(expected_to_fail_, expected_to_fail_.end()));
}

bool ConformanceTestSuite::CheckSetEmpty(const set<string>& set_to_check,
                                         const std::string& write_to_file,
                                         const std::string& msg) {
  if (set_to_check.empty()) {
    return true;
  } else {
    StringAppendF(&output_, "\n");
    StringAppendF(&output_, "%s\n\n", msg.c_str());
    for (set<string>::const_iterator iter = set_to_check.begin();
         iter != set_to_check.end(); ++iter) {
      StringAppendF(&output_, "  %s\n", iter->c_str());
    }
    StringAppendF(&output_, "\n");

    if (!write_to_file.empty()) {
      std::ofstream os(write_to_file);
      if (os) {
        for (set<string>::const_iterator iter = set_to_check.begin();
             iter != set_to_check.end(); ++iter) {
          os << *iter << "\n";
        }
      } else {
        StringAppendF(&output_, "Failed to open file: %s\n",
                      write_to_file.c_str());
      }
    }

    return false;
  }
}

bool ConformanceTestSuite::RunSuite(ConformanceTestRunner* runner,
                                    std::string* output) {
  runner_ = runner;
  successes_ = 0;
  expected_failures_ = 0;
  skipped_.clear();
  test_names_.clear();
  unexpected_failing_tests_.clear();
  unexpected_succeeding_tests_.clear();
  type_resolver_.reset(NewTypeResolverForDescriptorPool(
      kTypeUrlPrefix, DescriptorPool::generated_pool()));
  type_url_ = GetTypeUrl(TestAllTypes::descriptor());

  output_ = "\nCONFORMANCE TEST BEGIN ====================================\n\n";

  for (int i = 1; i <= FieldDescriptor::MAX_TYPE; i++) {
    if (i == FieldDescriptor::TYPE_GROUP) continue;
    TestPrematureEOFForType(static_cast<FieldDescriptor::Type>(i));
  }

  RunValidJsonTest("HelloWorld", "{\"optionalString\":\"Hello, World!\"}",
                   "optional_string: 'Hello, World!'");

  // Test field name conventions.
  RunValidJsonTest(
      "FieldNameInSnakeCase",
      R"({
        "fieldname1": 1,
        "fieldName2": 2,
        "FieldName3": 3
      })",
      R"(
        fieldname1: 1
        field_name2: 2
        _field_name3: 3
      )");
  RunValidJsonTest(
      "FieldNameWithNumbers",
      R"({
        "field0name5": 5,
        "field0Name6": 6
      })",
      R"(
        field0name5: 5
        field_0_name6: 6
      )");
  RunValidJsonTest(
      "FieldNameWithMixedCases",
      R"({
        "fieldName7": 7,
        "fieldName8": 8,
        "fieldName9": 9,
        "fieldName10": 10,
        "fIELDNAME11": 11,
        "fIELDName12": 12
      })",
      R"(
        fieldName7: 7
        FieldName8: 8
        field_Name9: 9
        Field_Name10: 10
        FIELD_NAME11: 11
        FIELD_name12: 12
      )");
  // Using the original proto field name in JSON is also allowed.
  RunValidJsonTest(
      "OriginalProtoFieldName",
      R"({
        "fieldname1": 1,
        "field_name2": 2,
        "_field_name3": 3,
        "field0name5": 5,
        "field_0_name6": 6,
        "fieldName7": 7,
        "FieldName8": 8,
        "field_Name9": 9,
        "Field_Name10": 10,
        "FIELD_NAME11": 11,
        "FIELD_name12": 12
      })",
      R"(
        fieldname1: 1
        field_name2: 2
        _field_name3: 3
        field0name5: 5
        field_0_name6: 6
        fieldName7: 7
        FieldName8: 8
        field_Name9: 9
        Field_Name10: 10
        FIELD_NAME11: 11
        FIELD_name12: 12
      )");
  // Field names can be escaped.
  RunValidJsonTest(
      "FieldNameEscaped",
      R"({"fieldn\u0061me1": 1})",
      "fieldname1: 1");
  // Field names must be quoted (or it's not valid JSON).
  ExpectParseFailureForJson(
      "FieldNameNotQuoted",
      "{fieldname1: 1}");
  // Trailing comma is not allowed (not valid JSON).
  ExpectParseFailureForJson(
      "TrailingCommaInAnObject",
      R"({"fieldname1":1,})");
  // JSON doesn't support comments.
  ExpectParseFailureForJson(
      "JsonWithComments",
      R"({
        // This is a comment.
        "fieldname1": 1
      })");
  // Duplicated field names are not allowed.
  ExpectParseFailureForJson(
      "FieldNameDuplicate",
      R"({
        "optionalNestedMessage": {a: 1},
        "optionalNestedMessage": {}
      })");
  ExpectParseFailureForJson(
      "FieldNameDuplicateDifferentCasing1",
      R"({
        "optional_nested_message": {a: 1},
        "optionalNestedMessage": {}
      })");
  ExpectParseFailureForJson(
      "FieldNameDuplicateDifferentCasing2",
      R"({
        "optionalNestedMessage": {a: 1},
        "optional_nested_message": {}
      })");
  // Serializers should use lowerCamelCase by default.
  RunValidJsonTestWithValidator(
      "FieldNameInLowerCamelCase",
      R"({
        "fieldname1": 1,
        "fieldName2": 2,
        "FieldName3": 3
      })",
      [](const Json::Value& value) {
        return value.isMember("fieldname1") &&
            value.isMember("fieldName2") &&
            value.isMember("FieldName3");
      });
  RunValidJsonTestWithValidator(
      "FieldNameWithNumbers",
      R"({
        "field0name5": 5,
        "field0Name6": 6
      })",
      [](const Json::Value& value) {
        return value.isMember("field0name5") &&
            value.isMember("field0Name6");
      });
  RunValidJsonTestWithValidator(
      "FieldNameWithMixedCases",
      R"({
        "fieldName7": 7,
        "fieldName8": 8,
        "fieldName9": 9,
        "fieldName10": 10,
        "fIELDNAME11": 11,
        "fIELDName12": 12
      })",
      [](const Json::Value& value) {
        return value.isMember("fieldName7") &&
            value.isMember("fieldName8") &&
            value.isMember("fieldName9") &&
            value.isMember("fieldName10") &&
            value.isMember("fIELDNAME11") &&
            value.isMember("fIELDName12");
      });

  // Integer fields.
  RunValidJsonTest(
      "Int32FieldMaxValue",
      R"({"optionalInt32": 2147483647})",
      "optional_int32: 2147483647");
  RunValidJsonTest(
      "Int32FieldMinValue",
      R"({"optionalInt32": -2147483648})",
      "optional_int32: -2147483648");
  RunValidJsonTest(
      "Uint32FieldMaxValue",
      R"({"optionalUint32": 4294967295})",
      "optional_uint32: 4294967295");
  RunValidJsonTest(
      "Int64FieldMaxValue",
      R"({"optionalInt64": "9223372036854775807"})",
      "optional_int64: 9223372036854775807");
  RunValidJsonTest(
      "Int64FieldMinValue",
      R"({"optionalInt64": "-9223372036854775808"})",
      "optional_int64: -9223372036854775808");
  RunValidJsonTest(
      "Uint64FieldMaxValue",
      R"({"optionalUint64": "18446744073709551615"})",
      "optional_uint64: 18446744073709551615");
  RunValidJsonTest(
      "Int64FieldMaxValueNotQuoted",
      R"({"optionalInt64": 9223372036854775807})",
      "optional_int64: 9223372036854775807");
  RunValidJsonTest(
      "Int64FieldMinValueNotQuoted",
      R"({"optionalInt64": -9223372036854775808})",
      "optional_int64: -9223372036854775808");
  RunValidJsonTest(
      "Uint64FieldMaxValueNotQuoted",
      R"({"optionalUint64": 18446744073709551615})",
      "optional_uint64: 18446744073709551615");
  // Values can be represented as JSON strings.
  RunValidJsonTest(
      "Int32FieldStringValue",
      R"({"optionalInt32": "2147483647"})",
      "optional_int32: 2147483647");
  RunValidJsonTest(
      "Int32FieldStringValueEscaped",
      R"({"optionalInt32": "2\u003147483647"})",
      "optional_int32: 2147483647");

  // Parsers reject out-of-bound integer values.
  ExpectParseFailureForJson(
      "Int32FieldTooLarge",
      R"({"optionalInt32": 2147483648})");
  ExpectParseFailureForJson(
      "Int32FieldTooSmall",
      R"({"optionalInt32": -2147483649})");
  ExpectParseFailureForJson(
      "Uint32FieldTooLarge",
      R"({"optionalUint32": 4294967296})");
  ExpectParseFailureForJson(
      "Int64FieldTooLarge",
      R"({"optionalInt64": "9223372036854775808"})");
  ExpectParseFailureForJson(
      "Int64FieldTooSmall",
      R"({"optionalInt64": "-9223372036854775809"})");
  ExpectParseFailureForJson(
      "Uint64FieldTooLarge",
      R"({"optionalUint64": "18446744073709551616"})");
  // Parser reject non-integer numeric values as well.
  ExpectParseFailureForJson(
      "Int32FieldNotInteger",
      R"({"optionalInt32": 0.5})");
  ExpectParseFailureForJson(
      "Uint32FieldNotInteger",
      R"({"optionalUint32": 0.5})");
  ExpectParseFailureForJson(
      "Int64FieldNotInteger",
      R"({"optionalInt64": "0.5"})");
  ExpectParseFailureForJson(
      "Uint64FieldNotInteger",
      R"({"optionalUint64": "0.5"})");

  // Integers but represented as float values are accepted.
  RunValidJsonTest(
      "Int32FieldFloatTrailingZero",
      R"({"optionalInt32": 100000.000})",
      "optional_int32: 100000");
  RunValidJsonTest(
      "Int32FieldExponentialFormat",
      R"({"optionalInt32": 1e5})",
      "optional_int32: 100000");
  RunValidJsonTest(
      "Int32FieldMaxFloatValue",
      R"({"optionalInt32": 2.147483647e9})",
      "optional_int32: 2147483647");
  RunValidJsonTest(
      "Int32FieldMinFloatValue",
      R"({"optionalInt32": -2.147483648e9})",
      "optional_int32: -2147483648");
  RunValidJsonTest(
      "Uint32FieldMaxFloatValue",
      R"({"optionalUint32": 4.294967295e9})",
      "optional_uint32: 4294967295");

  // Parser reject non-numeric values.
  ExpectParseFailureForJson(
      "Int32FieldNotNumber",
      R"({"optionalInt32": "3x3"})");
  ExpectParseFailureForJson(
      "Uint32FieldNotNumber",
      R"({"optionalUint32": "3x3"})");
  ExpectParseFailureForJson(
      "Int64FieldNotNumber",
      R"({"optionalInt64": "3x3"})");
  ExpectParseFailureForJson(
      "Uint64FieldNotNumber",
      R"({"optionalUint64": "3x3"})");
  // JSON does not allow "+" on numric values.
  ExpectParseFailureForJson(
      "Int32FieldPlusSign",
      R"({"optionalInt32": +1})");
  // JSON doesn't allow leading 0s.
  ExpectParseFailureForJson(
      "Int32FieldLeadingZero",
      R"({"optionalInt32": 01})");
  ExpectParseFailureForJson(
      "Int32FieldNegativeWithLeadingZero",
      R"({"optionalInt32": -01})");
  // String values must follow the same syntax rule. Specifically leading
  // or traling spaces are not allowed.
  ExpectParseFailureForJson(
      "Int32FieldLeadingSpace",
      R"({"optionalInt32": " 1"})");
  ExpectParseFailureForJson(
      "Int32FieldTrailingSpace",
      R"({"optionalInt32": "1 "})");

  // 64-bit values are serialized as strings.
  RunValidJsonTestWithValidator(
      "Int64FieldBeString",
      R"({"optionalInt64": 1})",
      [](const Json::Value& value) {
        return value["optionalInt64"].type() == Json::stringValue &&
            value["optionalInt64"].asString() == "1";
      });
  RunValidJsonTestWithValidator(
      "Uint64FieldBeString",
      R"({"optionalUint64": 1})",
      [](const Json::Value& value) {
        return value["optionalUint64"].type() == Json::stringValue &&
            value["optionalUint64"].asString() == "1";
      });

  // Bool fields.
  RunValidJsonTest(
      "BoolFieldTrue",
      R"({"optionalBool":true})",
      "optional_bool: true");
  RunValidJsonTest(
      "BoolFieldFalse",
      R"({"optionalBool":false})",
      "optional_bool: false");

  // Other forms are not allowed.
  ExpectParseFailureForJson(
      "BoolFieldIntegerZero",
      R"({"optionalBool":0})");
  ExpectParseFailureForJson(
      "BoolFieldIntegerOne",
      R"({"optionalBool":1})");
  ExpectParseFailureForJson(
      "BoolFieldCamelCaseTrue",
      R"({"optionalBool":True})");
  ExpectParseFailureForJson(
      "BoolFieldCamelCaseFalse",
      R"({"optionalBool":False})");
  ExpectParseFailureForJson(
      "BoolFieldAllCapitalTrue",
      R"({"optionalBool":TRUE})");
  ExpectParseFailureForJson(
      "BoolFieldAllCapitalFalse",
      R"({"optionalBool":FALSE})");
  ExpectParseFailureForJson(
      "BoolFieldDoubleQuotedTrue",
      R"({"optionalBool":"true"})");
  ExpectParseFailureForJson(
      "BoolFieldDoubleQuotedFalse",
      R"({"optionalBool":"false"})");

  // Float fields.
  RunValidJsonTest(
      "FloatFieldMinPositiveValue",
      R"({"optionalFloat": 1.175494e-38})",
      "optional_float: 1.175494e-38");
  RunValidJsonTest(
      "FloatFieldMaxNegativeValue",
      R"({"optionalFloat": -1.175494e-38})",
      "optional_float: -1.175494e-38");
  RunValidJsonTest(
      "FloatFieldMaxPositiveValue",
      R"({"optionalFloat": 3.402823e+38})",
      "optional_float: 3.402823e+38");
  RunValidJsonTest(
      "FloatFieldMinNegativeValue",
      R"({"optionalFloat": 3.402823e+38})",
      "optional_float: 3.402823e+38");
  // Values can be quoted.
  RunValidJsonTest(
      "FloatFieldQuotedValue",
      R"({"optionalFloat": "1"})",
      "optional_float: 1");
  // Special values.
  RunValidJsonTest(
      "FloatFieldNan",
      R"({"optionalFloat": "NaN"})",
      "optional_float: nan");
  RunValidJsonTest(
      "FloatFieldInfinity",
      R"({"optionalFloat": "Infinity"})",
      "optional_float: inf");
  RunValidJsonTest(
      "FloatFieldNegativeInfinity",
      R"({"optionalFloat": "-Infinity"})",
      "optional_float: -inf");
  // Non-cannonical Nan will be correctly normalized.
  {
    TestAllTypes message;
    // IEEE floating-point standard 32-bit quiet NaN:
    //   0111 1111 1xxx xxxx xxxx xxxx xxxx xxxx
    message.set_optional_float(
        WireFormatLite::DecodeFloat(0x7FA12345));
    RunValidJsonTestWithProtobufInput(
        "FloatFieldNormalizeQuietNan", message,
        "optional_float: nan");
    // IEEE floating-point standard 64-bit signaling NaN:
    //   1111 1111 1xxx xxxx xxxx xxxx xxxx xxxx
    message.set_optional_float(
        WireFormatLite::DecodeFloat(0xFFB54321));
    RunValidJsonTestWithProtobufInput(
        "FloatFieldNormalizeSignalingNan", message,
        "optional_float: nan");
  }

  // Special values must be quoted.
  ExpectParseFailureForJson(
      "FloatFieldNanNotQuoted",
      R"({"optionalFloat": NaN})");
  ExpectParseFailureForJson(
      "FloatFieldInfinityNotQuoted",
      R"({"optionalFloat": Infinity})");
  ExpectParseFailureForJson(
      "FloatFieldNegativeInfinityNotQuoted",
      R"({"optionalFloat": -Infinity})");
  // Parsers should reject out-of-bound values.
  ExpectParseFailureForJson(
      "FloatFieldTooSmall",
      R"({"optionalFloat": -3.502823e+38})");
  ExpectParseFailureForJson(
      "FloatFieldTooLarge",
      R"({"optionalFloat": 3.502823e+38})");

  // Double fields.
  RunValidJsonTest(
      "DoubleFieldMinPositiveValue",
      R"({"optionalDouble": 2.22507e-308})",
      "optional_double: 2.22507e-308");
  RunValidJsonTest(
      "DoubleFieldMaxNegativeValue",
      R"({"optionalDouble": -2.22507e-308})",
      "optional_double: -2.22507e-308");
  RunValidJsonTest(
      "DoubleFieldMaxPositiveValue",
      R"({"optionalDouble": 1.79769e+308})",
      "optional_double: 1.79769e+308");
  RunValidJsonTest(
      "DoubleFieldMinNegativeValue",
      R"({"optionalDouble": -1.79769e+308})",
      "optional_double: -1.79769e+308");
  // Values can be quoted.
  RunValidJsonTest(
      "DoubleFieldQuotedValue",
      R"({"optionalDouble": "1"})",
      "optional_double: 1");
  // Speical values.
  RunValidJsonTest(
      "DoubleFieldNan",
      R"({"optionalDouble": "NaN"})",
      "optional_double: nan");
  RunValidJsonTest(
      "DoubleFieldInfinity",
      R"({"optionalDouble": "Infinity"})",
      "optional_double: inf");
  RunValidJsonTest(
      "DoubleFieldNegativeInfinity",
      R"({"optionalDouble": "-Infinity"})",
      "optional_double: -inf");
  // Non-cannonical Nan will be correctly normalized.
  {
    TestAllTypes message;
    message.set_optional_double(
        WireFormatLite::DecodeDouble(0x7FFA123456789ABCLL));
    RunValidJsonTestWithProtobufInput(
        "DoubleFieldNormalizeQuietNan", message,
        "optional_double: nan");
    message.set_optional_double(
        WireFormatLite::DecodeDouble(0xFFFBCBA987654321LL));
    RunValidJsonTestWithProtobufInput(
        "DoubleFieldNormalizeSignalingNan", message,
        "optional_double: nan");
  }

  // Special values must be quoted.
  ExpectParseFailureForJson(
      "DoubleFieldNanNotQuoted",
      R"({"optionalDouble": NaN})");
  ExpectParseFailureForJson(
      "DoubleFieldInfinityNotQuoted",
      R"({"optionalDouble": Infinity})");
  ExpectParseFailureForJson(
      "DoubleFieldNegativeInfinityNotQuoted",
      R"({"optionalDouble": -Infinity})");

  // Parsers should reject out-of-bound values.
  ExpectParseFailureForJson(
      "DoubleFieldTooSmall",
      R"({"optionalDouble": -1.89769e+308})");
  ExpectParseFailureForJson(
      "DoubleFieldTooLarge",
      R"({"optionalDouble": +1.89769e+308})");

  // Enum fields.
  RunValidJsonTest(
      "EnumField",
      R"({"optionalNestedEnum": "FOO"})",
      "optional_nested_enum: FOO");
  // Enum values must be represented as strings.
  ExpectParseFailureForJson(
      "EnumFieldNotQuoted",
      R"({"optionalNestedEnum": FOO})");
  // Numeric values are allowed.
  RunValidJsonTest(
      "EnumFieldNumericValueZero",
      R"({"optionalNestedEnum": 0})",
      "optional_nested_enum: FOO");
  RunValidJsonTest(
      "EnumFieldNumericValueNonZero",
      R"({"optionalNestedEnum": 1})",
      "optional_nested_enum: BAR");
  // Unknown enum values are represented as numeric values.
  RunValidJsonTestWithValidator(
      "EnumFieldUnknownValue",
      R"({"optionalNestedEnum": 123})",
      [](const Json::Value& value) {
        return value["optionalNestedEnum"].type() == Json::intValue &&
            value["optionalNestedEnum"].asInt() == 123;
      });

  // String fields.
  RunValidJsonTest(
      "StringField",
      R"({"optionalString": "Hello world!"})",
      "optional_string: \"Hello world!\"");
  RunValidJsonTest(
      "StringFieldUnicode",
      // Google in Chinese.
      R"({"optionalString": "谷歌"})",
      R"(optional_string: "谷歌")");
  RunValidJsonTest(
      "StringFieldEscape",
      R"({"optionalString": "\"\\\/\b\f\n\r\t"})",
      R"(optional_string: "\"\\/\b\f\n\r\t")");
  RunValidJsonTest(
      "StringFieldUnicodeEscape",
      R"({"optionalString": "\u8C37\u6B4C"})",
      R"(optional_string: "谷歌")");
  RunValidJsonTest(
      "StringFieldUnicodeEscapeWithLowercaseHexLetters",
      R"({"optionalString": "\u8c37\u6b4c"})",
      R"(optional_string: "谷歌")");
  RunValidJsonTest(
      "StringFieldSurrogatePair",
      // The character is an emoji: grinning face with smiling eyes. 😁
      R"({"optionalString": "\uD83D\uDE01"})",
      R"(optional_string: "\xF0\x9F\x98\x81")");

  // Unicode escapes must start with "\u" (lowercase u).
  ExpectParseFailureForJson(
      "StringFieldUppercaseEscapeLetter",
      R"({"optionalString": "\U8C37\U6b4C"})");
  ExpectParseFailureForJson(
      "StringFieldInvalidEscape",
      R"({"optionalString": "\uXXXX\u6B4C"})");
  ExpectParseFailureForJson(
      "StringFieldUnterminatedEscape",
      R"({"optionalString": "\u8C3"})");
  ExpectParseFailureForJson(
      "StringFieldUnpairedHighSurrogate",
      R"({"optionalString": "\uD800"})");
  ExpectParseFailureForJson(
      "StringFieldUnpairedLowSurrogate",
      R"({"optionalString": "\uDC00"})");
  ExpectParseFailureForJson(
      "StringFieldSurrogateInWrongOrder",
      R"({"optionalString": "\uDE01\uD83D"})");
  ExpectParseFailureForJson(
      "StringFieldNotAString",
      R"({"optionalString": 12345})");

  // Bytes fields.
  RunValidJsonTest(
      "BytesField",
      R"({"optionalBytes": "AQI="})",
      R"(optional_bytes: "\x01\x02")");
  ExpectParseFailureForJson(
      "BytesFieldNoPadding",
      R"({"optionalBytes": "AQI"})");
  ExpectParseFailureForJson(
      "BytesFieldInvalidBase64Characters",
      R"({"optionalBytes": "-_=="})");

  // Message fields.
  RunValidJsonTest(
      "MessageField",
      R"({"optionalNestedMessage": {"a": 1234}})",
      "optional_nested_message: {a: 1234}");

  // Oneof fields.
  ExpectParseFailureForJson(
      "OneofFieldDuplicate",
      R"({"oneofUint32": 1, "oneofString": "test"})");

  // Repeated fields.
  RunValidJsonTest(
      "PrimitiveRepeatedField",
      R"({"repeatedInt32": [1, 2, 3, 4]})",
      "repeated_int32: [1, 2, 3, 4]");
  RunValidJsonTest(
      "EnumRepeatedField",
      R"({"repeatedNestedEnum": ["FOO", "BAR", "BAZ"]})",
      "repeated_nested_enum: [FOO, BAR, BAZ]");
  RunValidJsonTest(
      "StringRepeatedField",
      R"({"repeatedString": ["Hello", "world"]})",
      R"(repeated_string: ["Hello", "world"])");
  RunValidJsonTest(
      "BytesRepeatedField",
      R"({"repeatedBytes": ["AAEC", "AQI="]})",
      R"(repeated_bytes: ["\x00\x01\x02", "\x01\x02"])");
  RunValidJsonTest(
      "MessageRepeatedField",
      R"({"repeatedNestedMessage": [{"a": 1234}, {"a": 5678}]})",
      "repeated_nested_message: {a: 1234}"
      "repeated_nested_message: {a: 5678}");

  // Repeated field elements are of incorrect type.
  ExpectParseFailureForJson(
      "RepeatedFieldWrongElementTypeExpectingIntegersGotBool",
      R"({"repeatedInt32": [1, false, 3, 4]})");
  ExpectParseFailureForJson(
      "RepeatedFieldWrongElementTypeExpectingIntegersGotString",
      R"({"repeatedInt32": [1, 2, "name", 4]})");
  ExpectParseFailureForJson(
      "RepeatedFieldWrongElementTypeExpectingIntegersGotMessage",
      R"({"repeatedInt32": [1, 2, 3, {"a": 4}]})");
  ExpectParseFailureForJson(
      "RepeatedFieldWrongElementTypeExpectingStringsGotInt",
      R"({"repeatedString": ["1", 2, "3", "4"]})");
  ExpectParseFailureForJson(
      "RepeatedFieldWrongElementTypeExpectingStringsGotBool",
      R"({"repeatedString": ["1", "2", false, "4"]})");
  ExpectParseFailureForJson(
      "RepeatedFieldWrongElementTypeExpectingStringsGotMessage",
      R"({"repeatedString": ["1", 2, "3", {"a": 4}]})");
  ExpectParseFailureForJson(
      "RepeatedFieldWrongElementTypeExpectingMessagesGotInt",
      R"({"repeatedNestedMessage": [{"a": 1}, 2]})");
  ExpectParseFailureForJson(
      "RepeatedFieldWrongElementTypeExpectingMessagesGotBool",
      R"({"repeatedNestedMessage": [{"a": 1}, false]})");
  ExpectParseFailureForJson(
      "RepeatedFieldWrongElementTypeExpectingMessagesGotString",
      R"({"repeatedNestedMessage": [{"a": 1}, "2"]})");
  // Trailing comma in the repeated field is not allowed.
  ExpectParseFailureForJson(
      "RepeatedFieldTrailingComma",
      R"({"repeatedInt32": [1, 2, 3, 4,]})");

  // Map fields.
  RunValidJsonTest(
      "Int32MapField",
      R"({"mapInt32Int32": {"1": 2, "3": 4}})",
      "map_int32_int32: {key: 1 value: 2}"
      "map_int32_int32: {key: 3 value: 4}");
  ExpectParseFailureForJson(
      "Int32MapFieldKeyNotQuoted",
      R"({"mapInt32Int32": {1: 2, 3: 4}})");
  RunValidJsonTest(
      "Uint32MapField",
      R"({"mapUint32Uint32": {"1": 2, "3": 4}})",
      "map_uint32_uint32: {key: 1 value: 2}"
      "map_uint32_uint32: {key: 3 value: 4}");
  ExpectParseFailureForJson(
      "Uint32MapFieldKeyNotQuoted",
      R"({"mapUint32Uint32": {1: 2, 3: 4}})");
  RunValidJsonTest(
      "Int64MapField",
      R"({"mapInt64Int64": {"1": 2, "3": 4}})",
      "map_int64_int64: {key: 1 value: 2}"
      "map_int64_int64: {key: 3 value: 4}");
  ExpectParseFailureForJson(
      "Int64MapFieldKeyNotQuoted",
      R"({"mapInt64Int64": {1: 2, 3: 4}})");
  RunValidJsonTest(
      "Uint64MapField",
      R"({"mapUint64Uint64": {"1": 2, "3": 4}})",
      "map_uint64_uint64: {key: 1 value: 2}"
      "map_uint64_uint64: {key: 3 value: 4}");
  ExpectParseFailureForJson(
      "Uint64MapFieldKeyNotQuoted",
      R"({"mapUint64Uint64": {1: 2, 3: 4}})");
  RunValidJsonTest(
      "BoolMapField",
      R"({"mapBoolBool": {"true": true, "false": false}})",
      "map_bool_bool: {key: true value: true}"
      "map_bool_bool: {key: false value: false}");
  ExpectParseFailureForJson(
      "BoolMapFieldKeyNotQuoted",
      R"({"mapBoolBool": {true: true, false: false}})");
  RunValidJsonTest(
      "MessageMapField",
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
  RunValidJsonTest(
      "Int32MapEscapedKey",
      R"({"mapInt32Int32": {"\u0031": 2}})",
      "map_int32_int32: {key: 1 value: 2}");
  RunValidJsonTest(
      "Int64MapEscapedKey",
      R"({"mapInt64Int64": {"\u0031": 2}})",
      "map_int64_int64: {key: 1 value: 2}");
  RunValidJsonTest(
      "BoolMapEscapedKey",
      R"({"mapBoolBool": {"tr\u0075e": true}})",
      "map_bool_bool: {key: true value: true}");

  // "null" is accepted for all fields types.
  RunValidJsonTest(
      "AllFieldAcceptNull",
      R"({
        "optionalInt32": null,
        "optionalInt64": null,
        "optionalUint32": null,
        "optionalUint64": null,
        "optionalBool": null,
        "optionalString": null,
        "optionalBytes": null,
        "optionalNestedEnum": null,
        "optionalNestedMessage": null,
        "repeatedInt32": null,
        "repeatedInt64": null,
        "repeatedUint32": null,
        "repeatedUint64": null,
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
  ExpectParseFailureForJson(
      "RepeatedFieldPrimitiveElementIsNull",
      R"({"repeatedInt32": [1, null, 2]})");
  ExpectParseFailureForJson(
      "RepeatedFieldMessageElementIsNull",
      R"({"repeatedNestedMessage": [{"a":1}, null, {"a":2}]})");
  // Map field keys cannot be null.
  ExpectParseFailureForJson(
      "MapFieldKeyIsNull",
      R"({"mapInt32Int32": {null: 1}})");
  // Map field values cannot be null.
  ExpectParseFailureForJson(
      "MapFieldValueIsNull",
      R"({"mapInt32Int32": {"0": null}})");

  // Wrapper types.
  RunValidJsonTest(
      "OptionalBoolWrapper",
      R"({"optionalBoolWrapper": false})",
      "optional_bool_wrapper: {value: false}");
  RunValidJsonTest(
      "OptionalInt32Wrapper",
      R"({"optionalInt32Wrapper": 0})",
      "optional_int32_wrapper: {value: 0}");
  RunValidJsonTest(
      "OptionalUint32Wrapper",
      R"({"optionalUint32Wrapper": 0})",
      "optional_uint32_wrapper: {value: 0}");
  RunValidJsonTest(
      "OptionalInt64Wrapper",
      R"({"optionalInt64Wrapper": 0})",
      "optional_int64_wrapper: {value: 0}");
  RunValidJsonTest(
      "OptionalUint64Wrapper",
      R"({"optionalUint64Wrapper": 0})",
      "optional_uint64_wrapper: {value: 0}");
  RunValidJsonTest(
      "OptionalFloatWrapper",
      R"({"optionalFloatWrapper": 0})",
      "optional_float_wrapper: {value: 0}");
  RunValidJsonTest(
      "OptionalDoubleWrapper",
      R"({"optionalDoubleWrapper": 0})",
      "optional_double_wrapper: {value: 0}");
  RunValidJsonTest(
      "OptionalStringWrapper",
      R"({"optionalStringWrapper": ""})",
      R"(optional_string_wrapper: {value: ""})");
  RunValidJsonTest(
      "OptionalBytesWrapper",
      R"({"optionalBytesWrapper": ""})",
      R"(optional_bytes_wrapper: {value: ""})");
  RunValidJsonTest(
      "OptionalWrapperTypesWithNonDefaultValue",
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
  RunValidJsonTest(
      "RepeatedBoolWrapper",
      R"({"repeatedBoolWrapper": [true, false]})",
      "repeated_bool_wrapper: {value: true}"
      "repeated_bool_wrapper: {value: false}");
  RunValidJsonTest(
      "RepeatedInt32Wrapper",
      R"({"repeatedInt32Wrapper": [0, 1]})",
      "repeated_int32_wrapper: {value: 0}"
      "repeated_int32_wrapper: {value: 1}");
  RunValidJsonTest(
      "RepeatedUint32Wrapper",
      R"({"repeatedUint32Wrapper": [0, 1]})",
      "repeated_uint32_wrapper: {value: 0}"
      "repeated_uint32_wrapper: {value: 1}");
  RunValidJsonTest(
      "RepeatedInt64Wrapper",
      R"({"repeatedInt64Wrapper": [0, 1]})",
      "repeated_int64_wrapper: {value: 0}"
      "repeated_int64_wrapper: {value: 1}");
  RunValidJsonTest(
      "RepeatedUint64Wrapper",
      R"({"repeatedUint64Wrapper": [0, 1]})",
      "repeated_uint64_wrapper: {value: 0}"
      "repeated_uint64_wrapper: {value: 1}");
  RunValidJsonTest(
      "RepeatedFloatWrapper",
      R"({"repeatedFloatWrapper": [0, 1]})",
      "repeated_float_wrapper: {value: 0}"
      "repeated_float_wrapper: {value: 1}");
  RunValidJsonTest(
      "RepeatedDoubleWrapper",
      R"({"repeatedDoubleWrapper": [0, 1]})",
      "repeated_double_wrapper: {value: 0}"
      "repeated_double_wrapper: {value: 1}");
  RunValidJsonTest(
      "RepeatedStringWrapper",
      R"({"repeatedStringWrapper": ["", "AQI="]})",
      R"(
        repeated_string_wrapper: {value: ""}
        repeated_string_wrapper: {value: "AQI="}
      )");
  RunValidJsonTest(
      "RepeatedBytesWrapper",
      R"({"repeatedBytesWrapper": ["", "AQI="]})",
      R"(
        repeated_bytes_wrapper: {value: ""}
        repeated_bytes_wrapper: {value: "\x01\x02"}
      )");
  RunValidJsonTest(
      "WrapperTypesWithNullValue",
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
      "DurationMinValue",
      R"({"optionalDuration": "-315576000000.999999999s"})",
      "optional_duration: {seconds: -315576000000 nanos: -999999999}");
  RunValidJsonTest(
      "DurationMaxValue",
      R"({"optionalDuration": "315576000000.999999999s"})",
      "optional_duration: {seconds: 315576000000 nanos: 999999999}");
  RunValidJsonTest(
      "DurationRepeatedValue",
      R"({"repeatedDuration": ["1.5s", "-1.5s"]})",
      "repeated_duration: {seconds: 1 nanos: 500000000}"
      "repeated_duration: {seconds: -1 nanos: -500000000}");

  ExpectParseFailureForJson(
      "DurationMissingS",
      R"({"optionalDuration": "1"})");
  ExpectParseFailureForJson(
      "DurationJsonInputTooSmall",
      R"({"optionalDuration": "-315576000001.000000000s"})");
  ExpectParseFailureForJson(
      "DurationJsonInputTooLarge",
      R"({"optionalDuration": "315576000001.000000000s"})");
  ExpectSerializeFailureForJson(
      "DurationProtoInputTooSmall",
      "optional_duration: {seconds: -315576000001 nanos: 0}");
  ExpectSerializeFailureForJson(
      "DurationProtoInputTooLarge",
      "optional_duration: {seconds: 315576000001 nanos: 0}");

  RunValidJsonTestWithValidator(
      "DurationHasZeroFractionalDigit",
      R"({"optionalDuration": "1.000000000s"})",
      [](const Json::Value& value) {
        return value["optionalDuration"].asString() == "1s";
      });
  RunValidJsonTestWithValidator(
      "DurationHas3FractionalDigits",
      R"({"optionalDuration": "1.010000000s"})",
      [](const Json::Value& value) {
        return value["optionalDuration"].asString() == "1.010s";
      });
  RunValidJsonTestWithValidator(
      "DurationHas6FractionalDigits",
      R"({"optionalDuration": "1.000010000s"})",
      [](const Json::Value& value) {
        return value["optionalDuration"].asString() == "1.000010s";
      });
  RunValidJsonTestWithValidator(
      "DurationHas9FractionalDigits",
      R"({"optionalDuration": "1.000000010s"})",
      [](const Json::Value& value) {
        return value["optionalDuration"].asString() == "1.000000010s";
      });

  // Timestamp
  RunValidJsonTest(
      "TimestampMinValue",
      R"({"optionalTimestamp": "0001-01-01T00:00:00Z"})",
      "optional_timestamp: {seconds: -62135596800}");
  RunValidJsonTest(
      "TimestampMaxValue",
      R"({"optionalTimestamp": "9999-12-31T23:59:59.999999999Z"})",
      "optional_timestamp: {seconds: 253402300799 nanos: 999999999}");
  RunValidJsonTest(
      "TimestampRepeatedValue",
      R"({
        "repeatedTimestamp": [
          "0001-01-01T00:00:00Z",
          "9999-12-31T23:59:59.999999999Z"
        ]
      })",
      "repeated_timestamp: {seconds: -62135596800}"
      "repeated_timestamp: {seconds: 253402300799 nanos: 999999999}");
  RunValidJsonTest(
      "TimestampWithPositiveOffset",
      R"({"optionalTimestamp": "1970-01-01T08:00:00+08:00"})",
      "optional_timestamp: {seconds: 0}");
  RunValidJsonTest(
      "TimestampWithNegativeOffset",
      R"({"optionalTimestamp": "1969-12-31T16:00:00-08:00"})",
      "optional_timestamp: {seconds: 0}");

  ExpectParseFailureForJson(
      "TimestampJsonInputTooSmall",
      R"({"optionalTimestamp": "0000-01-01T00:00:00Z"})");
  ExpectParseFailureForJson(
      "TimestampJsonInputTooLarge",
      R"({"optionalTimestamp": "10000-01-01T00:00:00Z"})");
  ExpectParseFailureForJson(
      "TimestampJsonInputMissingZ",
      R"({"optionalTimestamp": "0001-01-01T00:00:00"})");
  ExpectParseFailureForJson(
      "TimestampJsonInputMissingT",
      R"({"optionalTimestamp": "0001-01-01 00:00:00Z"})");
  ExpectParseFailureForJson(
      "TimestampJsonInputLowercaseZ",
      R"({"optionalTimestamp": "0001-01-01T00:00:00z"})");
  ExpectParseFailureForJson(
      "TimestampJsonInputLowercaseT",
      R"({"optionalTimestamp": "0001-01-01t00:00:00Z"})");
  ExpectSerializeFailureForJson(
      "TimestampProtoInputTooSmall",
      "optional_timestamp: {seconds: -62135596801}");
  ExpectSerializeFailureForJson(
      "TimestampProtoInputTooLarge",
      "optional_timestamp: {seconds: 253402300800}");
  RunValidJsonTestWithValidator(
      "TimestampZeroNormalized",
      R"({"optionalTimestamp": "1969-12-31T16:00:00-08:00"})",
      [](const Json::Value& value) {
        return value["optionalTimestamp"].asString() ==
            "1970-01-01T00:00:00Z";
      });
  RunValidJsonTestWithValidator(
      "TimestampHasZeroFractionalDigit",
      R"({"optionalTimestamp": "1970-01-01T00:00:00.000000000Z"})",
      [](const Json::Value& value) {
        return value["optionalTimestamp"].asString() ==
            "1970-01-01T00:00:00Z";
      });
  RunValidJsonTestWithValidator(
      "TimestampHas3FractionalDigits",
      R"({"optionalTimestamp": "1970-01-01T00:00:00.010000000Z"})",
      [](const Json::Value& value) {
        return value["optionalTimestamp"].asString() ==
            "1970-01-01T00:00:00.010Z";
      });
  RunValidJsonTestWithValidator(
      "TimestampHas6FractionalDigits",
      R"({"optionalTimestamp": "1970-01-01T00:00:00.000010000Z"})",
      [](const Json::Value& value) {
        return value["optionalTimestamp"].asString() ==
            "1970-01-01T00:00:00.000010Z";
      });
  RunValidJsonTestWithValidator(
      "TimestampHas9FractionalDigits",
      R"({"optionalTimestamp": "1970-01-01T00:00:00.000000010Z"})",
      [](const Json::Value& value) {
        return value["optionalTimestamp"].asString() ==
            "1970-01-01T00:00:00.000000010Z";
      });

  // FieldMask
  RunValidJsonTest(
      "FieldMask",
      R"({"optionalFieldMask": "foo,barBaz"})",
      R"(optional_field_mask: {paths: "foo" paths: "bar_baz"})");
  ExpectParseFailureForJson(
      "FieldMaskInvalidCharacter",
      R"({"optionalFieldMask": "foo,bar_bar"})");
  ExpectSerializeFailureForJson(
      "FieldMaskPathsDontRoundTrip",
      R"(optional_field_mask: {paths: "fooBar"})");
  ExpectSerializeFailureForJson(
      "FieldMaskNumbersDontRoundTrip",
      R"(optional_field_mask: {paths: "foo_3_bar"})");
  ExpectSerializeFailureForJson(
      "FieldMaskTooManyUnderscore",
      R"(optional_field_mask: {paths: "foo__bar"})");

  // Struct
  RunValidJsonTest(
      "Struct",
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
  // Value
  RunValidJsonTest(
      "ValueAcceptInteger",
      R"({"optionalValue": 1})",
      "optional_value: { number_value: 1}");
  RunValidJsonTest(
      "ValueAcceptFloat",
      R"({"optionalValue": 1.5})",
      "optional_value: { number_value: 1.5}");
  RunValidJsonTest(
      "ValueAcceptBool",
      R"({"optionalValue": false})",
      "optional_value: { bool_value: false}");
  RunValidJsonTest(
      "ValueAcceptNull",
      R"({"optionalValue": null})",
      "optional_value: { null_value: NULL_VALUE}");
  RunValidJsonTest(
      "ValueAcceptString",
      R"({"optionalValue": "hello"})",
      R"(optional_value: { string_value: "hello"})");
  RunValidJsonTest(
      "ValueAcceptList",
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
  RunValidJsonTest(
      "ValueAcceptObject",
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

  // Any
  RunValidJsonTest(
      "Any",
      R"({
        "optionalAny": {
          "@type": "type.googleapis.com/conformance.TestAllTypes",
          "optionalInt32": 12345
        }
      })",
      R"(
        optional_any: {
          [type.googleapis.com/conformance.TestAllTypes] {
            optional_int32: 12345
          }
        }
      )");
  RunValidJsonTest(
      "AnyNested",
      R"({
        "optionalAny": {
          "@type": "type.googleapis.com/google.protobuf.Any",
          "value": {
            "@type": "type.googleapis.com/conformance.TestAllTypes",
            "optionalInt32": 12345
          }
        }
      })",
      R"(
        optional_any: {
          [type.googleapis.com/google.protobuf.Any] {
            [type.googleapis.com/conformance.TestAllTypes] {
              optional_int32: 12345
            }
          }
        }
      )");
  // The special "@type" tag is not required to appear first.
  RunValidJsonTest(
      "AnyUnorderedTypeTag",
      R"({
        "optionalAny": {
          "optionalInt32": 12345,
          "@type": "type.googleapis.com/conformance.TestAllTypes"
        }
      })",
      R"(
        optional_any: {
          [type.googleapis.com/conformance.TestAllTypes] {
            optional_int32: 12345
          }
        }
      )");
  // Well-known types in Any.
  RunValidJsonTest(
      "AnyWithInt32ValueWrapper",
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
  RunValidJsonTest(
      "AnyWithDuration",
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
  RunValidJsonTest(
      "AnyWithTimestamp",
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
  RunValidJsonTest(
      "AnyWithFieldMask",
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
  RunValidJsonTest(
      "AnyWithStruct",
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
  RunValidJsonTest(
      "AnyWithValueForJsonObject",
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
  RunValidJsonTest(
      "AnyWithValueForInteger",
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

  bool ok = true;
  if (!CheckSetEmpty(expected_to_fail_, "nonexistent_tests.txt",
                     "These tests were listed in the failure list, but they "
                     "don't exist.  Remove them from the failure list by "
                     "running:\n"
                     "  ./update_failure_list.py " + failure_list_filename_ +
                     " --remove nonexistent_tests.txt")) {
    ok = false;
  }
  if (!CheckSetEmpty(unexpected_failing_tests_, "failing_tests.txt",
                     "These tests failed.  If they can't be fixed right now, "
                     "you can add them to the failure list so the overall "
                     "suite can succeed.  Add them to the failure list by "
                     "running:\n"
                     "  ./update_failure_list.py " + failure_list_filename_ +
                     " --add failing_tests.txt")) {
    ok = false;
  }
  if (!CheckSetEmpty(unexpected_succeeding_tests_, "succeeding_tests.txt",
                     "These tests succeeded, even though they were listed in "
                     "the failure list.  Remove them from the failure list "
                     "by running:\n"
                     "  ./update_failure_list.py " + failure_list_filename_ +
                     " --remove succeeding_tests.txt")) {
    ok = false;
  }

  if (verbose_) {
    CheckSetEmpty(skipped_, "",
                  "These tests were skipped (probably because support for some "
                  "features is not implemented)");
  }

  StringAppendF(&output_,
                "CONFORMANCE SUITE %s: %d successes, %d skipped, "
                "%d expected failures, %d unexpected failures.\n",
                ok ? "PASSED" : "FAILED", successes_, skipped_.size(),
                expected_failures_, unexpected_failing_tests_.size());
  StringAppendF(&output_, "\n");

  output->assign(output_);

  return ok;
}

}  // namespace protobuf
}  // namespace google
