// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)

#include <memory>
#include <string>
#include <vector>

#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/file.h"
#include "google/protobuf/compiler/command_line_interface.h"
#include "google/protobuf/compiler/python/generator.h"
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/str_split.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/zero_copy_stream.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace python {
namespace {

class TestGenerator : public CodeGenerator {
 public:
  TestGenerator() {}
  ~TestGenerator() override {}

  bool Generate(const FileDescriptor* file, const std::string& parameter,
                GeneratorContext* context, std::string* error) const override {
    TryInsert("test_pb2.py", "imports", context);
    TryInsert("test_pb2.py", "module_scope", context);
    TryInsert("test_pb2.py", "class_scope:foo.Bar", context);
    TryInsert("test_pb2.py", "class_scope:foo.Bar.Baz", context);
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
};

// opposed to importlib) in the usual case where the .proto file paths do not
// not contain any Python keywords.
TEST(PythonPluginTest, ImportTest) {
  // Create files test1.proto and test2.proto with the former importing the
  // latter.
  ABSL_CHECK_OK(
      File::SetContents(absl::StrCat(::testing::TempDir(), "/test1.proto"),
                        "syntax = \"proto3\";\n"
                        "package foo;\n"
                        "import \"test2.proto\";"
                        "message Message1 {\n"
                        "  Message2 message_2 = 1;\n"
                        "}\n",
                        true));
  ABSL_CHECK_OK(
      File::SetContents(absl::StrCat(::testing::TempDir(), "/test2.proto"),
                        "syntax = \"proto3\";\n"
                        "package foo;\n"
                        "message Message2 {}\n",
                        true));

  compiler::CommandLineInterface cli;
  cli.SetInputsAreProtoPathRelative(true);
  python::Generator python_generator;
  cli.RegisterGenerator("--python_out", &python_generator, "");
  std::string proto_path = absl::StrCat("-I", ::testing::TempDir());
  std::string python_out = absl::StrCat("--python_out=", ::testing::TempDir());
  const char* argv[] = {"protoc", proto_path.c_str(), "-I.", python_out.c_str(),
                        "test1.proto"};
  ASSERT_EQ(0, cli.Run(5, argv));

  // Loop over the lines of the generated code and verify that we find an
  // ordinary Python import but do not find the string "importlib".
  std::string output;
  ABSL_CHECK_OK(
      File::GetContents(absl::StrCat(::testing::TempDir(), "/test1_pb2.py"),
                        &output, true));
  std::vector<absl::string_view> lines = absl::StrSplit(output, '\n');
  std::string expected_import = "import test2_pb2";
  bool found_expected_import = false;
  for (absl::string_view line : lines) {
    if (absl::StrContains(line, expected_import)) {
      found_expected_import = true;
    }
    EXPECT_FALSE(absl::StrContains(line, "importlib"));
  }
  EXPECT_TRUE(found_expected_import);
}

}  // namespace
}  // namespace python
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
