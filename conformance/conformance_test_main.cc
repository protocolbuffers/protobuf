// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <string>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "binary_json_conformance_suite.h"
#include "conformance_test.h"
#include "text_format_conformance_suite.h"

int main(int argc, char *argv[]) {
  google::protobuf::BinaryAndJsonConformanceSuite binary_and_json_suite;
  google::protobuf::BinaryAndJsonConformanceSuite::SectionDescription
      binary_json_section_description;
  absl::flat_hash_set<std::string> expands_to = {
      "Proto3", "Proto2", "Editions_Proto2", "Editions_Proto3"};
  // Anything except '.' (one or more times) followed by '.' followed by
  // '*' followed by '.' followed by anything (one or more times)
  // Capture groups around the wildcard for replacement.
  binary_json_section_description.SetPattern("([^.]+\\.)\\*(\\..+)");
  binary_json_section_description.SetExpandsTo(expands_to);
  binary_and_json_suite.AddSectionDescription(binary_json_section_description);

  google::protobuf::TextFormatConformanceTestSuite text_format_suite;
  return google::protobuf::ForkPipeRunner::Run(
      argc, argv, {&binary_and_json_suite, &text_format_suite});
}
