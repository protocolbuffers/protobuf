// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef CONFORMANCE_BINARY_JSON_CONFORMANCE_SUITE_H
#define CONFORMANCE_BINARY_JSON_CONFORMANCE_SUITE_H

#include "json/json.h"
#include "conformance_test.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {

class BinaryAndJsonConformanceSuite : public ConformanceTestSuite {
 public:
  BinaryAndJsonConformanceSuite() {}

 private:
  void RunSuiteImpl() override;
  void RunBinaryPerformanceTests();
  void RunJsonPerformanceTests();
  void RunJsonTests();
  void RunJsonTestsForFieldNameConvention();
  void RunJsonTestsForNonRepeatedTypes();
  void RunJsonTestsForRepeatedTypes();
  void RunJsonTestsForNullTypes();
  void RunJsonTestsForWrapperTypes();
  void RunJsonTestsForFieldMask();
  void RunJsonTestsForStruct();
  void RunJsonTestsForValue();
  void RunJsonTestsForAny();
  void RunJsonTestsForUnknownEnumStringValues();
  void RunValidJsonTest(const std::string& test_name, ConformanceLevel level,
                        const std::string& input_json,
                        const std::string& equivalent_text_format);
  void RunValidJsonTest(const std::string& test_name, ConformanceLevel level,
                        const std::string& input_json,
                        const std::string& equivalent_text_format,
                        bool is_proto3);
  void RunValidJsonTestWithMessage(const std::string& test_name,
                                   ConformanceLevel level,
                                   const std::string& input_json,
                                   const std::string& equivalent_text_forma,
                                   const Message& prototype);
  void RunValidJsonTestWithProtobufInput(
      const std::string& test_name, ConformanceLevel level,
      const protobuf_test_messages::proto3::TestAllTypesProto3& input,
      const std::string& equivalent_text_format);
  void RunValidJsonIgnoreUnknownTest(const std::string& test_name,
                                     ConformanceLevel level,
                                     const std::string& input_json,
                                     const std::string& equivalent_text_format);
  void RunValidProtobufTest(const std::string& test_name,
                            ConformanceLevel level,
                            const std::string& input_protobuf,
                            const std::string& equivalent_text_format,
                            bool is_proto3);
  void RunValidBinaryProtobufTest(const std::string& test_name,
                                  ConformanceLevel level,
                                  const std::string& input_protobuf,
                                  bool is_proto3);
  void RunValidBinaryProtobufTest(const std::string& test_name,
                                  ConformanceLevel level,
                                  const std::string& input_protobuf,
                                  const std::string& expected_protobuf,
                                  bool is_proto3);
  void RunBinaryPerformanceMergeMessageWithField(const std::string& test_name,
                                                 const std::string& field_proto,
                                                 bool is_proto3);

  void RunValidProtobufTestWithMessage(
      const std::string& test_name, ConformanceLevel level,
      const Message* input, const std::string& equivalent_text_format,
      bool is_proto3);

  bool ParseJsonResponse(const conformance::ConformanceResponse& response,
                         Message* test_message);
  bool ParseResponse(const conformance::ConformanceResponse& response,
                     const ConformanceRequestSetting& setting,
                     Message* test_message) override;

  typedef std::function<bool(const Json::Value&)> Validator;
  void RunValidJsonTestWithValidator(const std::string& test_name,
                                     ConformanceLevel level,
                                     const std::string& input_json,
                                     const Validator& validator,
                                     bool is_proto3);
  void ExpectParseFailureForJson(const std::string& test_name,
                                 ConformanceLevel level,
                                 const std::string& input_json);
  void ExpectSerializeFailureForJson(const std::string& test_name,
                                     ConformanceLevel level,
                                     const std::string& text_format);
  void ExpectParseFailureForProtoWithProtoVersion(const std::string& proto,
                                                  const std::string& test_name,
                                                  ConformanceLevel level,
                                                  bool is_proto3);
  void ExpectParseFailureForProto(const std::string& proto,
                                  const std::string& test_name,
                                  ConformanceLevel level);
  void ExpectHardParseFailureForProto(const std::string& proto,
                                      const std::string& test_name,
                                      ConformanceLevel level);
  void TestPrematureEOFForType(google::protobuf::FieldDescriptor::Type type);
  void TestIllegalTags();
  template <class MessageType>
  void TestOneofMessage(MessageType& message, bool is_proto3);
  template <class MessageType>
  void TestUnknownMessage(MessageType& message, bool is_proto3);
  void TestValidDataForType(
      google::protobuf::FieldDescriptor::Type,
      std::vector<std::pair<std::string, std::string>> values);
  void TestValidDataForRepeatedScalarMessage();
  void TestValidDataForMapType(google::protobuf::FieldDescriptor::Type,
                               google::protobuf::FieldDescriptor::Type);
  void TestValidDataForOneofType(google::protobuf::FieldDescriptor::Type);
  void TestMergeOneofMessage();
  void TestOverwriteMessageValueMap();
  void TestBinaryPerformanceForAlternatingUnknownFields();
  void TestBinaryPerformanceMergeMessageWithRepeatedFieldForType(
      google::protobuf::FieldDescriptor::Type);
  void TestBinaryPerformanceMergeMessageWithUnknownFieldForType(
      google::protobuf::FieldDescriptor::Type);
  void TestJsonPerformanceMergeMessageWithRepeatedFieldForType(
      google::protobuf::FieldDescriptor::Type, std::string field_value);

  std::unique_ptr<google::protobuf::util::TypeResolver> type_resolver_;
  std::string type_url_;
};

}  // namespace protobuf
}  // namespace google

#endif  // CONFORMANCE_BINARY_JSON_CONFORMANCE_SUITE_H
