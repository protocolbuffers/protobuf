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
#include "google/protobuf/testing/file.h"
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/command_line_interface.h"
#include "google/protobuf/compiler/command_line_interface_tester.h"
#include "google/protobuf/compiler/cpp/generator.h"
#include "google/protobuf/compiler/python/generator.h"
#include "google/protobuf/compiler/python/pyi_generator.h"
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
  TestGenerator() = default;
  ~TestGenerator() override = default;

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

// `_ClassVar` is only a valid annotation inside a class body: at module level a
// type checker rejects the whole stub. Nothing in the generator's structure
// enforces that, so assert it over the generated file rather than pinning the
// annotation each declaration happens to carry today.
TEST(PythonPyiTest, ClassVarIsOnlyUsedInsideAClassBody) {
  ABSL_CHECK_OK(
      File::SetContents(absl::StrCat(::testing::TempDir(), "/extensions.proto"),
                        "syntax = \"proto2\";\n"
                        "package foo;\n"
                        "message Extendable {\n"
                        "  extensions 1000 to max;\n"
                        "}\n"
                        "extend Extendable {\n"
                        "  optional int32 top_level_ext = 1000;\n"
                        "}\n"
                        "message Holder {\n"
                        "  extend Extendable {\n"
                        "    optional int32 nested_ext = 1001;\n"
                        "  }\n"
                        "}\n",
                        true));

  compiler::CommandLineInterface cli;
  cli.SetInputsAreProtoPathRelative(true);
  python::PyiGenerator pyi_generator;
  cli.RegisterGenerator("--pyi_out", &pyi_generator, "");
  std::string proto_path = absl::StrCat("-I", ::testing::TempDir());
  std::string pyi_out = absl::StrCat("--pyi_out=", ::testing::TempDir());
  const char* argv[] = {"protoc", proto_path.c_str(), "-I.", pyi_out.c_str(),
                        "extensions.proto"};
  ASSERT_EQ(0, cli.Run(5, argv));

  std::string output;
  ABSL_CHECK_OK(File::GetContents(
      absl::StrCat(::testing::TempDir(), "/extensions_pb2.pyi"), &output,
      true));

  bool annotates_a_top_level_extension = false;
  bool annotates_a_nested_extension = false;
  for (absl::string_view line : absl::StrSplit(output, '\n')) {
    if (absl::StartsWith(line, "TOP_LEVEL_EXT_FIELD_NUMBER")) {
      annotates_a_top_level_extension = true;
    }
    if (absl::StrContains(line, "NESTED_EXT_FIELD_NUMBER")) {
      annotates_a_nested_extension = true;
    }
    if (absl::StrContains(line, "_ClassVar[")) {
      EXPECT_TRUE(absl::StartsWith(line, " "))
          << "module-level _ClassVar annotation: " << line;
    }
  }
  // Both extensions reach the stub, so neither branch of the assertion above is
  // vacuous.
  EXPECT_TRUE(annotates_a_top_level_extension);
  EXPECT_TRUE(annotates_a_nested_extension);
}

class PyiGeneratorTest : public CommandLineInterfaceTester {
 protected:
  PyiGeneratorTest() {
    RegisterGenerator("--pyi_out", "--pyi_opt",
                      std::make_unique<PyiGenerator>(), "Python pyi generator");
  }
};

TEST_F(PyiGeneratorTest, KeywordMessageTypes) {
  CreateTempFile("keyword.proto", R"schema(
    syntax = "proto3";
    package keyword_test;

    message Unrelated {}
    message None { int32 count = 1; }
    message Outer {
      message None { keyword_test.None top_level = 1; }
      None value = 1;
      repeated None values = 2;
      map<string, None> by_name = 3;
    }
    message False {
      message True {}
      True value = 1;
      enum Kind { DEFAULT = 0; }
      Kind kind = 2;
    }
  )schema");
  CreateTempFile("facade.proto", R"schema(
    syntax = "proto3";
    import public "keyword.proto";
  )schema");
  CreateTempFile("bridge.proto", R"schema(
    syntax = "proto3";
    import public "facade.proto";
  )schema");
  CreateTempFile("consumer.proto", R"schema(
    syntax = "proto3";
    import "bridge.proto";
    message Holder { keyword_test.None value = 1; }
  )schema");
  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --pyi_out=$tmpdir "
      "keyword.proto facade.proto bridge.proto consumer.proto");
  ExpectNoErrors();

  for (absl::string_view declaration : {
           "type_check_only as _type_check_only\n",
           "@_type_check_only\nclass _None(_message.Message):",
           "@_type_check_only\n    class _None_(_message.Message):",
           "top_level: _None\n",
           "value: Outer._None_\n",
           "values: "
           "_containers.RepeatedCompositeFieldContainer[Outer._None_]\n",
           "by_name: _containers.MessageMap[str, Outer._None_]\n",
           "_Union[Outer._None_, _Mapping[_Any, _Any]]",
           "value: _False._True\n",
           "DEFAULT: _ClassVar[_False.Kind]\n",
           "kind: _False.Kind\n",
           "_Union[_False.Kind, str]",
       }) {
    ExpectFileContentContainsSubstring("keyword_pb2.pyi", declaration);
  }
  ExpectFileContentContainsSubstring("consumer_pb2.pyi",
                                     "import keyword_pb2 as _keyword_pb2\n");
  ExpectFileContentContainsSubstring("consumer_pb2.pyi",
                                     "value: _keyword_pb2._None\n");
  ExpectFileContentContainsSubstring(
      "consumer_pb2.pyi", "_Union[_keyword_pb2._None, _Mapping[_Any, _Any]]");
  ExpectFileContentNotContainsSubstring("facade_pb2.pyi",
                                        "from keyword_pb2 import None");
}

TEST_F(PyiGeneratorTest, KeywordMessageCollisions) {
  CreateTempFile("collision.proto", R"schema(
    syntax = "proto3";
    package collision;

    message _type_check_only {}
    message None {}
    message _None {}
    message Outer {
      message None {}
      message Inner {
        message _None_ {}
        oneof _None__ { collision.None top_level = 1; }
      }
      collision.None top_level = 1;
      None nested = 2;
    }
  )schema");
  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --pyi_out=$tmpdir "
      "collision.proto");
  ExpectNoErrors();

  for (absl::string_view declaration : {
           "type_check_only as _type_check_only_\n",
           "@_type_check_only_\nclass _None__(_message.Message):",
           "class _None(_message.Message):",
           "class _None_(_message.Message):",
           "\n    top_level: _None__\n",
           "\n        top_level: _None__\n",
           "nested: Outer._None___\n",
       }) {
    ExpectFileContentContainsSubstring("collision_pb2.pyi", declaration);
  }
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

TEST_P(PythonGeneratorTest, PyiMessageFieldsUseFullyTypedMappingInitType) {
  RegisterGenerator("--pyi_out", std::make_unique<PyiGenerator>(),
                    "Python pyi test generator");
  CreateTempFile("foo.proto",
                 R"schema(
    syntax = "proto3";

    package foo;

    message X {
      string s = 1;
    }

    message M {
      X x = 1;
      repeated X xs = 2;
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --pyi_out=$tmpdir "
      "foo.proto");

  ExpectNoErrors();
  ExpectFileContentContainsSubstring(
      "foo_pb2.pyi",
      "from typing import Any as _Any, ClassVar as _ClassVar, "
      "Optional as _Optional, Union as _Union");
  ExpectFileContentContainsSubstring(
      "foo_pb2.pyi",
      "def __init__(self, s: _Optional[str] = ...) -> None: ...");
  ExpectFileContentContainsSubstring(
      "foo_pb2.pyi",
      "def __init__(self, x: _Optional[_Union[X, _Mapping[_Any, _Any]]] = ..., "
      "xs: _Optional[_Iterable[_Union[X, _Mapping[_Any, _Any]]]] = ...)"
      " -> None: ...");
}

INSTANTIATE_TEST_SUITE_P(PythonGeneratorTest, PythonGeneratorTest,
                         testing::Bool());

}  // namespace
}  // namespace python
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
