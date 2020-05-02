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

// Author: kenton@google.com (Kenton Varda)
//
// TODO(kenton):  Share code with the versions of this test in other languages?
//   It seemed like parameterizing it would add more complexity than it is
//   worth.

#include <memory>

#include <google/protobuf/compiler/command_line_interface.h>
#include <google/protobuf/compiler/python/python_generator.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>

#include <google/protobuf/testing/file.h>
#include <google/protobuf/testing/file.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/testing/googletest.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace python {
namespace {

class TestGenerator : public CodeGenerator {
 public:
  TestGenerator() {}
  ~TestGenerator() {}

  virtual bool Generate(const FileDescriptor* file,
                        const std::string& parameter, GeneratorContext* context,
                        std::string* error) const {
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

// This test verifies that all the expected insertion points exist.  It does
// not verify that they are correctly-placed; that would require actually
// compiling the output which is a bit more than I care to do for this test.
TEST(PythonPluginTest, PluginTest) {
  GOOGLE_CHECK_OK(File::SetContents(TestTempDir() + "/test.proto",
                             "syntax = \"proto2\";\n"
                             "package foo;\n"
                             "message Bar {\n"
                             "  message Baz {}\n"
                             "}\n",
                             true));

  compiler::CommandLineInterface cli;
  cli.SetInputsAreProtoPathRelative(true);

  python::Generator python_generator;
  TestGenerator test_generator;
  cli.RegisterGenerator("--python_out", &python_generator, "");
  cli.RegisterGenerator("--test_out", &test_generator, "");

  std::string proto_path = "-I" + TestTempDir();
  std::string python_out = "--python_out=" + TestTempDir();
  std::string test_out = "--test_out=" + TestTempDir();

  const char* argv[] = {"protoc", proto_path.c_str(), python_out.c_str(),
                        test_out.c_str(), "test.proto"};

  EXPECT_EQ(0, cli.Run(5, argv));
}

// This test verifies that the generated Python output uses regular imports (as
// opposed to importlib) in the usual case where the .proto file paths do not
// not contain any Python keywords.
TEST(PythonPluginTest, ImportTest) {
  // Create files test1.proto and test2.proto with the former importing the
  // latter.
  GOOGLE_CHECK_OK(File::SetContents(TestTempDir() + "/test1.proto",
                             "syntax = \"proto3\";\n"
                             "package foo;\n"
                             "import \"test2.proto\";"
                             "message Message1 {\n"
                             "  Message2 message_2 = 1;\n"
                             "}\n",
                             true));
  GOOGLE_CHECK_OK(File::SetContents(TestTempDir() + "/test2.proto",
                             "syntax = \"proto3\";\n"
                             "package foo;\n"
                             "message Message2 {}\n",
                             true));

  compiler::CommandLineInterface cli;
  cli.SetInputsAreProtoPathRelative(true);
  python::Generator python_generator;
  cli.RegisterGenerator("--python_out", &python_generator, "");
  std::string proto_path = "-I" + TestTempDir();
  std::string python_out = "--python_out=" + TestTempDir();
  const char* argv[] = {"protoc", proto_path.c_str(), "-I.", python_out.c_str(),
                        "test1.proto"};
  ASSERT_EQ(0, cli.Run(5, argv));

  // Loop over the lines of the generated code and verify that we find an
  // ordinary Python import but do not find the string "importlib".
  std::string output;
  GOOGLE_CHECK_OK(File::GetContents(TestTempDir() + "/test1_pb2.py", &output,
                             true));
  std::vector<std::string> lines = Split(output, "\n");
  std::string expected_import = "import test2_pb2";
  bool found_expected_import = false;
  for (int i = 0; i < lines.size(); ++i) {
    if (lines[i].find(expected_import) != std::string::npos) {
      found_expected_import = true;
    }
    EXPECT_EQ(std::string::npos, lines[i].find("importlib"));
  }
  EXPECT_TRUE(found_expected_import);
}

TEST(PythonPluginTest, ReplaceImportPackageBasicTest) {
/*
  Both tests recast the protoc generated Python statement:

    import b_bp2 ...

  into the form:

    from PACKAGE import b_pb2 ...

  where PACKAGE is either an absolute or relative package name.
*/
  GOOGLE_CHECK_OK(File::SetContents(TestTempDir() + "/a.proto",
                             "syntax = \"proto3\";\n"
                             "import \"b.proto\";"
                             "message A {\n"
                             "  B b = 1;\n"
                             "}\n",
                             true));
  GOOGLE_CHECK_OK(File::SetContents(TestTempDir() + "/b.proto",
                             "syntax = \"proto3\";\n"
                             "message B {}\n",
                             true));

  compiler::CommandLineInterface cli;
  python::Generator python_generator;
  cli.RegisterGenerator("--python_out", "--python_opt", &python_generator, "");
  std::string proto_path = "--proto_path=" + TestTempDir();
  std::string python_out = "--python_out=" + TestTempDir();

  // Check that package name supplied with option is inserted into generated
  // import statement
  std::string absolute_python_opt = "--python_opt=replace_import_package=|omega";
  const char* absolute_argv[] = {"protoc", proto_path.c_str(),
                             python_out.c_str(), absolute_python_opt.c_str(),
                             "a.proto"};
  ASSERT_EQ(0, cli.Run(5, absolute_argv));
  // Loop over the lines of the generated code and verify that we find
  // a Python import statement with a 'from' clause containing an absolute
  // package name.
  std::string absolute_output;
  GOOGLE_CHECK_OK(File::GetContents(TestTempDir() + "/a_pb2.py",
                             &absolute_output, true));
  std::vector<std::string> absolute_lines = Split(absolute_output, "\n");
  std::string expected_absolute_import = "from omega import b_pb2";
  bool found_expected_absolute_import = false;
  for (int i = 0; i < absolute_lines.size(); ++i) {
    if (absolute_lines[i].find(expected_absolute_import) != std::string::npos) {
      found_expected_absolute_import = true;
      break;
    }
  }
  EXPECT_TRUE(found_expected_absolute_import);

  // Check that default package name "." is inserted into generated
  // import statement
  std::string relative_python_opt = "--python_opt=replace_import_package";
  const char* relative_argv[] = {"protoc", proto_path.c_str(),
                             python_out.c_str(), relative_python_opt.c_str(),
                             "a.proto"};
  ASSERT_EQ(0, cli.Run(5, relative_argv));
  // Loop over the lines of the generated code and verify that we find
  // a Python import statement with a 'from' clause containing the default
  // explicit relative package name ".".
  std::string relative_output;
  GOOGLE_CHECK_OK(File::GetContents(TestTempDir() + "/a_pb2.py",
                             &relative_output, true));
  std::vector<std::string> relative_lines = Split(relative_output, "\n");
  std::string expected_relative_import = "from . import b_pb2";
  bool found_expected_relative_import = false;
  for (int i = 0; i < relative_lines.size(); ++i) {
    if (relative_lines[i].find(expected_relative_import) != std::string::npos) {
      found_expected_relative_import = true;
      break;
    }
  }
  EXPECT_TRUE(found_expected_relative_import);
}

TEST(PythonPluginTest, ReplaceImportPackageExtendedTest) {
/*
  Test creates a set of input message files arranged like:

  .
  ├── a
  │   └── b
  │       └── c.proto
  ├── d
  │   ├── e.proto
  │   └── f.proto
  └── g
      └── h.proto

  Where file ./d/e.proto imports the other three files.

  The python_opt replace_import_package mapping string "d|.;g|p.g"
  should convert the protoc generated statements:

    from a.b import c_pb2 ...
    from d import f_pb2 ...
    from g import h_pb2 ...

  into the user preferred statements:

    from a.b import c_pb2 ... # unchanged
    from . import f_pb2 ...
    from p.g import h_pb2 ...
*/
  std::string temp_dir_a = TestTempDir() + "/a";
  std::string temp_dir_a_b = temp_dir_a + "/b";
  std::string temp_dir_d = TestTempDir() + "/d";
  std::string temp_dir_g = TestTempDir() + "/g";
  std::string temp_dir_p = TestTempDir() + "/p";

  if (File::Exists(temp_dir_a)) {
    File::DeleteRecursively(temp_dir_a, NULL, NULL);
  }
  if (File::Exists(temp_dir_d)) {
    File::DeleteRecursively(temp_dir_d, NULL, NULL);
  }
  if (File::Exists(temp_dir_g)) {
    File::DeleteRecursively(temp_dir_g, NULL, NULL);
  }
  if (File::Exists(temp_dir_p)) {
    File::DeleteRecursively(temp_dir_p, NULL, NULL);
  }

  GOOGLE_CHECK_OK(File::CreateDir(temp_dir_a, 0777));
  GOOGLE_CHECK_OK(File::CreateDir(temp_dir_a_b, 0777));
  GOOGLE_CHECK_OK(File::CreateDir(temp_dir_d, 0777));
  GOOGLE_CHECK_OK(File::CreateDir(temp_dir_g, 0777));
  GOOGLE_CHECK_OK(File::CreateDir(temp_dir_p, 0777));

  GOOGLE_CHECK_OK(File::SetContents(
    temp_dir_a_b + "/c.proto",
    "syntax = \"proto3\";\n"
    "message C {}\n",
    true
  ));
  GOOGLE_CHECK_OK(File::SetContents(
    temp_dir_d + "/e.proto",
    "syntax = \"proto3\";\n"
    "import \"a/b/c.proto\";" // generated: from a.b import c_pb2 ...
    "import \"d/f.proto\";"   // generated: from d import f_pb2 ...
    "import \"g/h.proto\";"   // generated: from g import h_pb2 ...
    "message E {\n"
    "  C c = 1;\n"
    "  F f = 2;\n"
    "  H H = 3;\n"
    "}\n",
    true
  ));
  GOOGLE_CHECK_OK(File::SetContents(
    temp_dir_d + "/f.proto",
    "syntax = \"proto3\";\n"
    "message F {}\n",
    true
  ));
  GOOGLE_CHECK_OK(File::SetContents(
    temp_dir_g + "/h.proto",
    "syntax = \"proto3\";\n"
    "message H {}\n",
    true
  ));

  compiler::CommandLineInterface cli;
  python::Generator python_generator;
  cli.RegisterGenerator("--python_out", "--python_opt", &python_generator, "");
  std::string proto_path = "--proto_path=" + TestTempDir();
  std::string python_out = "--python_out=" + temp_dir_p;
  std::string python_opt = "--python_opt=replace_import_package=d|.;g|p.g";

  const char* argv[] = {
    "protoc",
    proto_path.c_str(),
    python_out.c_str(),
    python_opt.c_str(),
    "d/e.proto"
  };
  ASSERT_EQ(0, cli.Run(5, argv));
  // Loop over the lines of the generated code and verify that each
  // generated import package has been mapped to its preferred package
  // name.
  std::string output;
  GOOGLE_CHECK_OK(File::GetContents(temp_dir_p + "/d/e_pb2.py", &output, true));
  std::vector<std::string> lines = Split(output, "\n");
  std::string expected_import_a_dot_b = "from a.b import c_pb2";
  std::string expected_import_dot = "from . import f_pb2";
  std::string expected_import_p_dot_g = "from p.g import h_pb2";
  bool found_expected_import_a_dot_b = false;
  bool found_expected_import_dot = false;
  bool found_expected_import_p_dot_g = false;
  for (int i = 0; i < lines.size(); ++i) {
    if (lines[i].find(expected_import_a_dot_b) != std::string::npos) {
      found_expected_import_a_dot_b = true;
    }
    if (lines[i].find(expected_import_dot) != std::string::npos) {
      found_expected_import_dot = true;
    }
    if (lines[i].find(expected_import_p_dot_g) != std::string::npos) {
      found_expected_import_p_dot_g = true;
    }
  }
  EXPECT_TRUE(found_expected_import_a_dot_b);
  EXPECT_TRUE(found_expected_import_dot);
  EXPECT_TRUE(found_expected_import_p_dot_g);
}

TEST(PythonPluginTest, ReplaceImportPackageErrorTest) {
  GOOGLE_CHECK_OK(File::SetContents(TestTempDir() + "/a.proto",
                             "syntax = \"proto3\";\n"
                             "import \"b.proto\";"
                             "message A {\n"
                             "  B b = 1;\n"
                             "}\n",
                             true));
  GOOGLE_CHECK_OK(File::SetContents(TestTempDir() + "/b.proto",
                             "syntax = \"proto3\";\n"
                             "message B {}\n",
                             true));

  const std::string python_opt_prefix("--python_opt=replace_import_package=");

  struct ErrorMessage {
    std::string mapping_string;
    std::string expected_message;
  };

  const std::vector<ErrorMessage> cases = {
    {".|a",
     "invalid relative generated package name found at position 0"},
    {"a|p;b|q;..|r",
     "invalid relative generated package name found at position 8"},
    {";b|q",
     "invalid empty mapping found near position 0"},
    {"a|p;b|q;;c|r",
     "invalid empty mapping found near position 8"},
    {"?|p",
     "unexpected character '?' found while reading generated package name at position 0"},
    {"a|p;b|q;?|r",
     "unexpected character '?' found while reading generated package name at position 8"},
    {"a|p;b|q;",
     "unexpected end of input found while reading generated package name near position 8"},
    {"a|p;b;",
     "unexpected delimiter ';' found while reading generated package name at position 5"},
    {"a|p;b?",
     "unexpected character '?' found while reading generated package name at position 5"},
    {"a|p;b",
     "unexpected end of input found while reading generated package name near position 5"},
    {"a|;",
     "invalid empty preferred package name found near position 2"},
    {"a|",
     "invalid empty preferred package name found near position 2"},
    {"a||",
     "unexpected connector '|' found while reading preferred package name at position 2"},
    {"a|?",
     "unexpected character '?' found while reading preferred package name at position 2"},
    {"a|p|",
     "unexpected connector '|' found while reading preferred package name at position 3"},
    {"a|p?",
     "unexpected character '?' found while reading preferred package name at position 3"}
  };

  // In each case, pass in the grammatically flawed mapping string and
  // confirm that the parser output includes the corresponding error
  // message.
  for (const ErrorMessage& c : cases) {
    CaptureTestStderr();
    compiler::CommandLineInterface cli;
    python::Generator python_generator;
    cli.RegisterGenerator("--python_out", "--python_opt", &python_generator, "");
    std::string proto_path = "--proto_path=" + TestTempDir();
    std::string python_out = "--python_out=" + TestTempDir();
    std::string python_opt = python_opt_prefix + c.mapping_string;
    const char* argv[] = {
      "protoc",
      proto_path.c_str(),
      python_out.c_str(),
      python_opt.c_str(),
      "a.proto"
    };
    ASSERT_EQ(1, cli.Run(5, argv));
    std::string captured_message = GetCapturedTestStderr();
    ASSERT_PRED_FORMAT2(::testing::IsSubstring,
                        c.expected_message,
                        captured_message);
  }
}

TEST(PythonPluginTest, ReplaceImportPackagePlacementTest) {
  GOOGLE_CHECK_OK(File::SetContents(TestTempDir() + "/a.proto",
                             "syntax = \"proto3\";\n"
                             "import \"b.proto\";"
                             "message A {\n"
                             "  B b = 1;\n"
                             "}\n",
                             true));
  GOOGLE_CHECK_OK(File::SetContents(TestTempDir() + "/b.proto",
                             "syntax = \"proto3\";\n"
                             "message B {}\n",
                             true));

  compiler::CommandLineInterface cli;
  python::Generator python_generator;
  cli.RegisterGenerator("--python_out", &python_generator, "");
  std::string proto_path = "--proto_path=" + TestTempDir();
  // Check that replace_import_packkage flag works correctly when supplied as
  // prefix to the python_out value
  std::string python_out =
      "--python_out=replace_import_package=|alpha.beta;something|anything:" +
      TestTempDir();

  const char* argv[] = {
    "protoc",
    proto_path.c_str(),
    python_out.c_str(),
    "a.proto"
  };
  ASSERT_EQ(0, cli.Run(4, argv));
  // Loop over the lines of the generated code and verify that we find
  // a Python import statement with a 'from' clause containing an absolute
  // package name.
  std::string output;
  GOOGLE_CHECK_OK(File::GetContents(TestTempDir() + "/a_pb2.py", &output, true));
  std::vector<std::string> lines = Split(output, "\n");
  std::string expected_import = "from alpha.beta import b_pb2";
  bool found_expected_import = false;
  for (int i = 0; i < lines.size(); ++i) {
    if (lines[i].find(expected_import) != std::string::npos) {
      found_expected_import = true;
      break;
    }
  }
  EXPECT_TRUE(found_expected_import);
}

}  // namespace
}  // namespace python
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
