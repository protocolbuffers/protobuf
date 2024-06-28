// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef TEXT_FORMAT_CONFORMANCE_SUITE_H_
#define TEXT_FORMAT_CONFORMANCE_SUITE_H_

#include <string>

#include "absl/strings/string_view.h"
#include "conformance_test.h"
#include "google/protobuf/message.h"

namespace google {
namespace protobuf {

class TextFormatConformanceTestSuite : public ConformanceTestSuite {
 public:
  TextFormatConformanceTestSuite();

 private:
  void RunSuiteImpl() override;

  bool ParseTextFormatResponse(const conformance::ConformanceResponse& response,
                               const ConformanceRequestSetting& setting,
                               Message* test_message);
  bool ParseResponse(const conformance::ConformanceResponse& response,
                     const ConformanceRequestSetting& setting,
                     Message* test_message) override;

  template <typename MessageType>
  friend class TextFormatConformanceTestSuiteImpl;
};

template <typename MessageType>
class TextFormatConformanceTestSuiteImpl {
 public:
  explicit TextFormatConformanceTestSuiteImpl(
      TextFormatConformanceTestSuite* suite);

 private:
  using ConformanceRequestSetting =
      TextFormatConformanceTestSuite::ConformanceRequestSetting;
  using ConformanceLevel = TextFormatConformanceTestSuite::ConformanceLevel;
  constexpr static ConformanceLevel RECOMMENDED = ConformanceLevel::RECOMMENDED;
  constexpr static ConformanceLevel REQUIRED = ConformanceLevel::REQUIRED;

  void RunAllTests();

  void RunDelimitedTests();
  void RunGroupTests();
  void RunAnyTests();

  void RunTextFormatPerformanceTests();
  void RunValidTextFormatTest(absl::string_view test_name,
                              ConformanceLevel level, absl::string_view input);
  void RunValidTextFormatTestWithExpected(absl::string_view test_name,
                                          ConformanceLevel level,
                                          absl::string_view input_text,
                                          absl::string_view expected_text);
  void RunValidUnknownTextFormatTest(absl::string_view test_name,
                                     const Message& message);
  void RunValidTextFormatTestWithMessage(absl::string_view test_name,
                                         ConformanceLevel level,
                                         absl::string_view input_text,
                                         const Message& message);
  void ExpectParseFailure(absl::string_view test_name, ConformanceLevel level,
                          absl::string_view input);
  void TestTextFormatPerformanceMergeMessageWithRepeatedField(
      absl::string_view test_type_name, absl::string_view message_field);

  TextFormatConformanceTestSuite& suite_;
};

}  // namespace protobuf
}  // namespace google

#endif  // TEXT_FORMAT_CONFORMANCE_SUITE_H_
