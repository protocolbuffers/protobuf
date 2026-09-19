// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_BINARY_JSON_CONFORMANCE_SUITE_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_BINARY_JSON_CONFORMANCE_SUITE_H__

#include "conformance/conformance_test.h"
#include "google/protobuf/message.h"

namespace google {
namespace protobuf {

// The legacy binary and JSON conformance suite.  Every test it used to send
// has moved to the gtest-based binary_*_test.cc and json_*_test.cc suites, so
// it no longer runs anything; it only keeps the --failure_list plumbing of the
// legacy runner alive until the runner learns to own that flag itself.
//
// TODO: b/410122158 - Delete this class together with the legacy runner phase.
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
};

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_BINARY_JSON_CONFORMANCE_SUITE_H__
