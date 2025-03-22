// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "google/protobuf/testing/file.h"
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/command_line_interface_tester.h"
#include "google/protobuf/compiler/cpp/generator.h"
#include "google/protobuf/compiler/python/generator.h"
#include "google/protobuf/cpp_features.pb.h"
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

class PythonGeneratorTest : public CommandLineInterfaceTester,
                            public testing::WithParamInterface<bool> {
 protected:
  PythonGeneratorTest() {
    auto generator = std::make_unique<Generator>();
    generator->set_opensource_runtime(GetParam());
    RegisterGenerator("--python_out", "--python_opt", std::move(generator),
                      "Python test generator");

    // Generate built-in protos.
    CreateTempFile(
        google::protobuf::DescriptorProto::descriptor()->file()->name(),
        google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  }
};

TEST_P(PythonGeneratorTest, PythonWithCppFeatures) {
  // Test that the presence of C++ features does not break Python generation.
  RegisterGenerator("--cpp_out", "--cpp_opt",
                    std::make_unique<cpp::CppGenerator>(),
                    "C++ test generator");
  CreateTempFile("google/protobuf/cpp_features.proto",
                 pb::CppFeatures::descriptor()->file()->DebugString());
  CreateTempFile("foo.proto",
                 R"schema(
    edition = "2023";

    import "google/protobuf/cpp_features.proto";

    package foo;
    
    enum Bar {
      AAA = 0;
      BBB = 1;
    }

    message Foo {
      Bar bar_enum = 1 [features.(pb.cpp).legacy_closed_enum = true];
    })schema");

  RunProtoc(absl::Substitute(
      "protocol_compiler --proto_path=$$tmpdir --cpp_out=$$tmpdir "
      "--python_out=$$tmpdir foo.proto $0 "
      "google/protobuf/cpp_features.proto",
      google::protobuf::DescriptorProto::descriptor()->file()->name()));

  ExpectNoErrors();
}

INSTANTIATE_TEST_SUITE_P(PythonGeneratorTest, PythonGeneratorTest,
                         testing::Bool());

}  // namespace
}  // namespace python
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
