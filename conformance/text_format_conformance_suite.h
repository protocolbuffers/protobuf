// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef TEXT_FORMAT_CONFORMANCE_SUITE_H_
#define TEXT_FORMAT_CONFORMANCE_SUITE_H_

#include "conformance_test.h"

namespace google {
namespace protobuf {

class TextFormatConformanceTestSuite : public ConformanceTestSuite {
 public:
  TextFormatConformanceTestSuite();

 private:
  void RunSuiteImpl() override;
  void RunTextFormatPerformanceTests();
  void RunValidTextFormatTest(const std::string& test_name,
                              ConformanceLevel level, const std::string& input);
  void RunValidTextFormatTestProto2(const std::string& test_name,
                                    ConformanceLevel level,
                                    const std::string& input);
  void RunValidTextFormatTestWithExpected(const std::string& test_name,
                                          ConformanceLevel level,
                                          const std::string& input,
                                          const std::string& expected);
  void RunValidTextFormatTestProto2WithExpected(const std::string& test_name,
                                                ConformanceLevel level,
                                                const std::string& input,
                                                const std::string& expected);
  void RunValidTextFormatTestWithMessage(const std::string& test_name,
                                         ConformanceLevel level,
                                         const std::string& input_text,
                                         const Message& prototype);
  void RunValidTextFormatTestWithMessage(const std::string& test_name,
                                         ConformanceLevel level,
                                         const std::string& input_text,
                                         const std::string& expected_text,
                                         const Message& prototype);
  void RunValidUnknownTextFormatTest(const std::string& test_name,
                                     const Message& message);
  void ExpectParseFailure(const std::string& test_name, ConformanceLevel level,
                          const std::string& input);
  bool ParseTextFormatResponse(const conformance::ConformanceResponse& response,
                               const ConformanceRequestSetting& setting,
                               Message* test_message);
  bool ParseResponse(const conformance::ConformanceResponse& response,
                     const ConformanceRequestSetting& setting,
                     Message* test_message) override;
  void TestTextFormatPerformanceMergeMessageWithRepeatedField(
      const std::string& test_type_name, const std::string& message_field);
};

}  // namespace protobuf
}  // namespace google

#endif  // TEXT_FORMAT_CONFORMANCE_SUITE_H_
