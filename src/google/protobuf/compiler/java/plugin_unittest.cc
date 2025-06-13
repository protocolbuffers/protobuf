// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)

#include <memory>
#include <string>

#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/file.h"
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/str_split.h"
#include "google/protobuf/compiler/command_line_interface.h"
#include "google/protobuf/compiler/java/generator.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/zero_copy_stream.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {
namespace {

class TestGenerator : public CodeGenerator {
 public:
  TestGenerator() {}
  ~TestGenerator() override {}

  bool Generate(const FileDescriptor* file, const std::string& parameter,
                GeneratorContext* context, std::string* error) const override {
    std::string filename = "Test.java";
    TryInsert(filename, "outer_class_scope", context);
    TryInsert(filename, "class_scope:foo.Bar", context);
    TryInsert(filename, "class_scope:foo.Bar.Baz", context);
    TryInsert(filename, "builder_scope:foo.Bar", context);
    TryInsert(filename, "builder_scope:foo.Bar.Baz", context);
    TryInsert(filename, "enum_scope:foo.Qux", context);
    return true;
  }

  void TryInsert(const std::string& filename,
                 const std::string& insertion_point,
                 GeneratorContext* context) const {
    std::unique_ptr<io::ZeroCopyOutputStream> output(
        context->OpenForInsert(filename, insertion_point));
    io::Printer printer(output.get(), '$');
    printer.Print("// inserted $name$\n", "name", insertion_point);
  }

  uint64_t GetSupportedFeatures() const override {
    return CodeGenerator::Feature::FEATURE_SUPPORTS_EDITIONS;
  }

  Edition GetMinimumEdition() const override { return Edition::EDITION_PROTO2; }
  Edition GetMaximumEdition() const override { return Edition::EDITION_2023; }
};

// This test verifies that all the expected insertion points exist.  It does
// not verify that they are correctly-placed; that would require actually
// compiling the output which is a bit more than I care to do for this test.
TEST(JavaPluginTest, PluginTest) {
  ABSL_CHECK_OK(
      File::SetContents(absl::StrCat(::testing::TempDir(), "/test.proto"),
                        "edition = \"2023\";\n"
                        "package foo;\n"
                        "option java_package = \"\";\n"
                        "option java_outer_classname = \"Test\";\n"
                        "message Bar {\n"
                        "  message Baz {}\n"
                        "}\n"
                        "enum Qux {\n"
                        "  option features.enum_type = CLOSED;\n"
                        "  BLAH = 1;\n"
                        "}\n",
                        true));

  CommandLineInterface cli;
  cli.SetInputsAreProtoPathRelative(true);

  JavaGenerator java_generator;
  TestGenerator test_generator;
  cli.RegisterGenerator("--java_out", &java_generator, "");
  cli.RegisterGenerator("--test_out", &test_generator, "");

  std::string proto_path = absl::StrCat("-I", ::testing::TempDir());
  std::string java_out = absl::StrCat("--java_out=", ::testing::TempDir());
  std::string test_out = absl::StrCat("--test_out=", ::testing::TempDir());

  const char* argv[] = {"protoc", proto_path.c_str(), java_out.c_str(),
                        test_out.c_str(), "test.proto"};

  EXPECT_EQ(0, cli.Run(5, argv));

  // Loop over the lines of the generated code and verify that we find what we
  // expect

  std::string output;
  ABSL_CHECK_OK(
      File::GetContents(absl::StrCat(::testing::TempDir(), "/Test.java"),
                        &output, true));
  std::vector<std::string> lines = absl::StrSplit(output, '\n');
  bool found_generated_annotation = false;
  bool found_do_not_edit = false;
  for (const auto& line : lines) {
    if (line.find(" DO NOT EDIT!") != std::string::npos) {
      found_do_not_edit = true;
    }
    if (line.find("@com.google.protobuf.Generated") != std::string::npos) {
      found_generated_annotation = true;
    }
  }
  EXPECT_TRUE(found_do_not_edit);
  (void)found_generated_annotation;
}

}  // namespace
}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
