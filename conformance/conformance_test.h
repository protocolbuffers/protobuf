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


// This file defines a protocol for running the conformance test suite
// in-process.  In other words, the suite itself will run in the same process as
// the code under test.
//
// For pros and cons of this approach, please see conformance.proto.

#ifndef CONFORMANCE_CONFORMANCE_TEST_H
#define CONFORMANCE_CONFORMANCE_TEST_H

#include <string>
#include <google/protobuf/wire_format_lite.h>

namespace conformance {
class ConformanceRequest;
class ConformanceResponse;
}  // namespace conformance

namespace google {
namespace protobuf {

class ConformanceTestRunner {
 public:
  // Call to run a single conformance test.
  //
  // "input" is a serialized conformance.ConformanceRequest.
  // "output" should be set to a serialized conformance.ConformanceResponse.
  //
  // If there is any error in running the test itself, set "runtime_error" in
  // the response.
  virtual void RunTest(const std::string& input, std::string* output) = 0;
};

// Class representing the test suite itself.  To run it, implement your own
// class derived from ConformanceTestRunner and then write code like:
//
//    class MyConformanceTestRunner : public ConformanceTestRunner {
//     public:
//      virtual void RunTest(...) {
//        // INSERT YOUR FRAMEWORK-SPECIFIC CODE HERE.
//      }
//    };
//
//    int main() {
//      MyConformanceTestRunner runner;
//      google::protobuf::ConformanceTestSuite suite;
//
//      std::string output;
//      suite.RunSuite(&runner, &output);
//    }
//
class ConformanceTestSuite {
 public:
  ConformanceTestSuite() : verbose_(false) {}

  // Run all the conformance tests against the given test runner.
  // Test output will be stored in "output".
  void RunSuite(ConformanceTestRunner* runner, std::string* output);

 private:
  void ReportSuccess();
  void ReportFailure(const char* fmt, ...);
  void RunTest(const conformance::ConformanceRequest& request,
               conformance::ConformanceResponse* response);
  void DoExpectParseFailureForProto(const std::string& proto, int line);
  void TestPrematureEOFForType(
      google::protobuf::internal::WireFormatLite::FieldType type);

  ConformanceTestRunner* runner_;
  int successes_;
  int failures_;
  bool verbose_;
  std::string output_;
};

}  // namespace protobuf
}  // namespace google

#endif  // CONFORMANCE_CONFORMANCE_TEST_H
