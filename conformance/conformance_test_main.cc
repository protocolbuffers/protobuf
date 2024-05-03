// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "binary_json_conformance_suite.h"
#include "conformance_test.h"
#include "text_format_conformance_suite.h"

int main(int argc, char *argv[]) {
  google::protobuf::BinaryAndJsonConformanceSuite binary_and_json_suite;
  google::protobuf::TextFormatConformanceTestSuite text_format_suite;
  return google::protobuf::ForkPipeRunner::Run(
      argc, argv, {&binary_and_json_suite, &text_format_suite});
}
