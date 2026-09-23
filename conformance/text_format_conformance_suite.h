// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_TEXT_FORMAT_CONFORMANCE_SUITE_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_TEXT_FORMAT_CONFORMANCE_SUITE_H__

#include <string>

#include "conformance/conformance_test.h"
#include "google/protobuf/message.h"

namespace google {
namespace protobuf {

class TextFormatConformanceTestSuite : public ConformanceTestSuite {
 public:
  TextFormatConformanceTestSuite();

 private:
  void RunSuiteImpl() override;

  bool ParseTextFormatResponse(
      const ::conformance::ConformanceResponse& response,
      const ConformanceRequestSetting& setting, Message* test_message);
  bool ParseResponse(const ::conformance::ConformanceResponse& response,
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

  void RunAllTests();

  TextFormatConformanceTestSuite& suite_;
};

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_TEXT_FORMAT_CONFORMANCE_SUITE_H__
