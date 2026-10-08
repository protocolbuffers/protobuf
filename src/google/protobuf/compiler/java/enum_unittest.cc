// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <memory>
#include <string>
#include <vector>

#include "google/protobuf/testing/file.h"
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "google/protobuf/compiler/command_line_interface.h"
#include "google/protobuf/compiler/java/generator.h"
#include "google/protobuf/test_util2.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {
namespace {

// https://github.com/protocolbuffers/protobuf/issues/9612
TEST(JavaEnumTest, JavadocReturnTest) {
  CommandLineInterface cli;
  cli.SetInputsAreProtoPathRelative(true);

  JavaGenerator java_generator;
  cli.RegisterGenerator("--java_out", &java_generator, "");

  std::string proto_path = absl::StrCat(
      "-I", TestUtil::GetTestDataPath("google/protobuf/compiler/java"));
  std::string java_out = absl::StrCat("--java_out=", ::testing::TempDir());

  const char* argv[] = {"protoc", proto_path.c_str(), java_out.c_str(),
                        "enum_unittest.proto"};

  // cli.Run seems to leak memory. Why? and how to fix?
  EXPECT_EQ(0, cli.Run(4, argv));

  // Loop over the lines of the generated code and verify that no two
  // successive lines contain @return
  std::string output;
  ABSL_CHECK_OK(
      File::GetContents(absl::StrCat(::testing::TempDir(), "/Test.java"),
                        &output, true));
  std::vector<std::string> lines = absl::StrSplit(output, '\n');
  bool found_return_tag = false;
  bool previous_contained_return_tag = false;
  for (const auto& line : lines) {
    if (line.find("* @return ") != std::string::npos) {
      EXPECT_FALSE(previous_contained_return_tag);
      previous_contained_return_tag = true;
      found_return_tag = true;
    } else {
      previous_contained_return_tag = false;
    }
  }
  EXPECT_TRUE(found_return_tag);
}

}  // namespace
}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
