// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_BINARY_JSON_CONFORMANCE_SUITE_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_BINARY_JSON_CONFORMANCE_SUITE_H__

#include <functional>
#include <memory>
#include <string>

#include "absl/strings/string_view.h"
#include "json/json.h"
#include "conformance/conformance_test.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/util/type_resolver.h"

namespace google {
namespace protobuf {

class BinaryAndJsonConformanceSuite : public ConformanceTestSuite {
 public:
  BinaryAndJsonConformanceSuite() = default;

 private:
  void RunSuiteImpl() override;
  bool ParseJsonResponse(const ::conformance::ConformanceResponse& response,
                         Message* test_message);
  bool ParseResponse(const ::conformance::ConformanceResponse& response,
                     const ConformanceRequestSetting& setting,
                     Message* test_message) override;
  void SetTypeUrl(absl::string_view type_url) {
    type_url_ = std::string(type_url);
  }

  // Runs only the binary-input -> JSON-output leg of a valid-data test.  The
  // binary-output leg of these tests has moved to the gtest suites (see
  // binary_*_test.cc); the JSON leg stays here until JSON matching is
  // available there (b/410122158).
  template <typename MessageType>
  void RunValidProtobufToJsonTest(const std::string& test_name,
                                  ConformanceLevel level,
                                  const std::string& input_protobuf,
                                  const std::string& equivalent_text_format);

  void RunDelimitedFieldTests();

  void RunUnstableTests();

  template <typename MessageType>
  friend class BinaryAndJsonConformanceSuiteImpl;

  std::unique_ptr<google::protobuf::util::TypeResolver> type_resolver_;
  std::string type_url_;
};

template <typename MessageType>
class BinaryAndJsonConformanceSuiteImpl {
 public:
  explicit BinaryAndJsonConformanceSuiteImpl(
      BinaryAndJsonConformanceSuite* suite, bool run_proto3_tests);

 private:
  using ConformanceRequestSetting =
      BinaryAndJsonConformanceSuite::ConformanceRequestSetting;
  using ConformanceLevel = BinaryAndJsonConformanceSuite::ConformanceLevel;
  constexpr static ConformanceLevel RECOMMENDED = ConformanceLevel::RECOMMENDED;
  constexpr static ConformanceLevel REQUIRED = ConformanceLevel::REQUIRED;

  void RunAllTests();

  void RunJsonTestsForReservedFields();
  void RunValidJsonTest(const std::string& test_name, ConformanceLevel level,
                        const std::string& input_json,
                        const std::string& equivalent_text_format);
  void RunValidJsonTestWithMessage(const std::string& test_name,
                                   ConformanceLevel level,
                                   const std::string& input_json,
                                   const std::string& equivalent_text_forma,
                                   const Message& prototype);
  void RunValidJsonTestWithProtobufInput(
      const std::string& test_name, ConformanceLevel level,
      const MessageType& input, const std::string& equivalent_text_format);
  // The binary-input -> JSON-output legs only; see
  // BinaryAndJsonConformanceSuite::RunValidProtobufToJsonTest().
  void RunValidProtobufToJsonTest(const std::string& test_name,
                                  ConformanceLevel level,
                                  const std::string& input_protobuf,
                                  const std::string& equivalent_text_format);
  void RunValidProtobufToJsonTestWithMessage(
      const std::string& test_name, ConformanceLevel level,
      const Message* input, const std::string& equivalent_text_format);

  typedef std::function<bool(const Json::Value&)> Validator;
  void RunValidJsonTestWithValidator(const std::string& test_name,
                                     ConformanceLevel level,
                                     const std::string& input_json,
                                     const Validator& validator);
  void ExpectParseFailureForJson(const std::string& test_name,
                                 ConformanceLevel level,
                                 const std::string& input_json);
  void RunValidJsonTestOrParseFailure(
      const std::string& test_name, ConformanceLevel level,
      const std::string& input_json, const std::string& equivalent_text_format);
  void ExpectSerializeFailureForJson(const std::string& test_name,
                                     ConformanceLevel level,
                                     const std::string& text_format);
  void TestOneofMessage();
  // Iterates ::google::protobuf::conformance::ValidDataCases(type).  Only the
  // binary->JSON legs remain here; see binary_valid_data_scalar_test.cc and
  // binary_valid_data_repeated_test.cc.
  void TestValidDataForType(google::protobuf::FieldDescriptor::Type type);
  void TestValidDataForRepeatedScalarMessage();
  // Called for every entry of ::google::protobuf::conformance::ValidDataMapTypes().
  // Only the binary->JSON legs remain here; see binary_valid_data_map_test.cc.
  void TestValidDataForMapType(google::protobuf::FieldDescriptor::Type,
                               google::protobuf::FieldDescriptor::Type);
  // Called for every entry of ::google::protobuf::conformance::ValidDataOneofTypes().
  // Only the binary->JSON legs remain here; see binary_oneof_test.cc.
  void TestValidDataForOneofType(google::protobuf::FieldDescriptor::Type);
  void TestMergeOneofMessage();
  void TestOverwriteMessageValueMap();

  // TODO: b/410122158 - The field lookups live in binary_test_util.h now;
  // these wrappers go away once their remaining callers migrate.
  const FieldDescriptor* GetFieldForType(FieldDescriptor::Type type,
                                         bool repeated) const;
  const FieldDescriptor* GetFieldForMapType(
      FieldDescriptor::Type key_type, FieldDescriptor::Type value_type) const;
  std::string SyntaxIdentifier() const;

  BinaryAndJsonConformanceSuite& suite_;
  bool run_proto3_tests_;
};

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_BINARY_JSON_CONFORMANCE_SUITE_H__
