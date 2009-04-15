// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#endif
#include <vector>

#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/compiler/command_line_interface.h>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/unittest.pb.h>
#include <google/protobuf/testing/file.h>
#include <google/protobuf/stubs/strutil.h>

#include <google/protobuf/testing/googletest.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace compiler {

#if defined(_WIN32)
#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif
#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif
#endif

namespace {

class CommandLineInterfaceTest : public testing::Test {
 protected:
  virtual void SetUp();
  virtual void TearDown();

  // Runs the CommandLineInterface with the given command line.  The
  // command is automatically split on spaces, and the string "$tmpdir"
  // is replaced with TestTempDir().
  void Run(const string& command);

  // -----------------------------------------------------------------
  // Methods to set up the test (called before Run()).

  class MockCodeGenerator;
  class NullCodeGenerator;

  // Registers a MockCodeGenerator with the given name.
  MockCodeGenerator* RegisterGenerator(const string& generator_name,
                                       const string& flag_name,
                                       const string& filename,
                                       const string& help_text);
  MockCodeGenerator* RegisterErrorGenerator(const string& generator_name,
                                            const string& error_text,
                                            const string& flag_name,
                                            const string& filename,
                                            const string& help_text);

  // Registers a CodeGenerator which will not actually generate anything,
  // but records the parameter passed to the generator.
  NullCodeGenerator* RegisterNullGenerator(const string& flag_name);

  // Create a temp file within temp_directory_ with the given name.
  // The containing directory is also created if necessary.
  void CreateTempFile(const string& name, const string& contents);

  void SetInputsAreProtoPathRelative(bool enable) {
    cli_.SetInputsAreProtoPathRelative(enable);
  }

  // -----------------------------------------------------------------
  // Methods to check the test results (called after Run()).

  // Checks that no text was written to stderr during Run(), and Run()
  // returned 0.
  void ExpectNoErrors();

  // Checks that Run() returned non-zero and the stderr output is exactly
  // the text given.  expected_test may contain references to "$tmpdir",
  // which will be replaced by the temporary directory path.
  void ExpectErrorText(const string& expected_text);

  // Checks that Run() returned non-zero and the stderr contains the given
  // substring.
  void ExpectErrorSubstring(const string& expected_substring);

  // Returns true if ExpectErrorSubstring(expected_substring) would pass, but
  // does not fail otherwise.
  bool HasAlternateErrorSubstring(const string& expected_substring);

  // Checks that MockCodeGenerator::Generate() was called in the given
  // context.  That is, this tests if the generator with the given name
  // was called with the given parameter and proto file and produced the
  // given output file.  This is checked by reading the output file and
  // checking that it contains the content that MockCodeGenerator would
  // generate given these inputs.  message_name is the name of the first
  // message that appeared in the proto file; this is just to make extra
  // sure that the correct file was parsed.
  void ExpectGenerated(const string& generator_name,
                       const string& parameter,
                       const string& proto_name,
                       const string& message_name,
                       const string& output_file);

  void ReadDescriptorSet(const string& filename,
                         FileDescriptorSet* descriptor_set);

 private:
  // The object we are testing.
  CommandLineInterface cli_;

  // We create a directory within TestTempDir() in order to add extra
  // protection against accidentally deleting user files (since we recursively
  // delete this directory during the test).  This is the full path of that
  // directory.
  string temp_directory_;

  // The result of Run().
  int return_code_;

  // The captured stderr output.
  string error_text_;

  // Pointers which need to be deleted later.
  vector<CodeGenerator*> mock_generators_to_delete_;
};

// A mock CodeGenerator which outputs information about the context in which
// it was called, which can then be checked.  Output is written to a filename
// constructed by concatenating the filename_prefix (given to the constructor)
// with the proto file name, separated by a '.'.
class CommandLineInterfaceTest::MockCodeGenerator : public CodeGenerator {
 public:
  // Create a MockCodeGenerator whose Generate() method returns true.
  MockCodeGenerator(const string& name, const string& filename_prefix);

  // Create a MockCodeGenerator whose Generate() method returns false
  // and sets the error string to the given string.
  MockCodeGenerator(const string& name, const string& filename_prefix,
                    const string& error);

  ~MockCodeGenerator();

  void set_expect_write_error(bool value) {
    expect_write_error_ = value;
  }

  // implements CodeGenerator ----------------------------------------
  bool Generate(const FileDescriptor* file,
                const string& parameter,
                OutputDirectory* output_directory,
                string* error) const;

 private:
  string name_;
  string filename_prefix_;
  bool return_error_;
  string error_;
  bool expect_write_error_;
};

class CommandLineInterfaceTest::NullCodeGenerator : public CodeGenerator {
 public:
  NullCodeGenerator() : called_(false) {}
  ~NullCodeGenerator() {}

  mutable bool called_;
  mutable string parameter_;

  // implements CodeGenerator ----------------------------------------
  bool Generate(const FileDescriptor* file,
                const string& parameter,
                OutputDirectory* output_directory,
                string* error) const {
    called_ = true;
    parameter_ = parameter;
    return true;
  }
};

// ===================================================================

void CommandLineInterfaceTest::SetUp() {
  // Most of these tests were written before this option was added, so we
  // run with the option on (which used to be the only way) except in certain
  // tests where we turn it off.
  cli_.SetInputsAreProtoPathRelative(true);

  temp_directory_ = TestTempDir() + "/proto2_cli_test_temp";

  // If the temp directory already exists, it must be left over from a
  // previous run.  Delete it.
  if (File::Exists(temp_directory_)) {
    File::DeleteRecursively(temp_directory_, NULL, NULL);
  }

  // Create the temp directory.
  GOOGLE_CHECK(File::CreateDir(temp_directory_.c_str(), DEFAULT_FILE_MODE));
}

void CommandLineInterfaceTest::TearDown() {
  // Delete the temp directory.
  File::DeleteRecursively(temp_directory_, NULL, NULL);

  // Delete all the MockCodeGenerators.
  for (int i = 0; i < mock_generators_to_delete_.size(); i++) {
    delete mock_generators_to_delete_[i];
  }
  mock_generators_to_delete_.clear();
}

void CommandLineInterfaceTest::Run(const string& command) {
  vector<string> args;
  SplitStringUsing(command, " ", &args);

  scoped_array<const char*> argv(new const char*[args.size()]);

  for (int i = 0; i < args.size(); i++) {
    args[i] = StringReplace(args[i], "$tmpdir", temp_directory_, true);
    argv[i] = args[i].c_str();
  }

  CaptureTestStderr();

  return_code_ = cli_.Run(args.size(), argv.get());

  error_text_ = GetCapturedTestStderr();
}

// -------------------------------------------------------------------

CommandLineInterfaceTest::MockCodeGenerator*
CommandLineInterfaceTest::RegisterGenerator(
    const string& generator_name,
    const string& flag_name,
    const string& filename,
    const string& help_text) {
  MockCodeGenerator* generator =
    new MockCodeGenerator(generator_name, filename);
  mock_generators_to_delete_.push_back(generator);

  cli_.RegisterGenerator(flag_name, generator, help_text);
  return generator;
}

CommandLineInterfaceTest::MockCodeGenerator*
CommandLineInterfaceTest::RegisterErrorGenerator(
    const string& generator_name,
    const string& error_text,
    const string& flag_name,
    const string& filename_prefix,
    const string& help_text) {
  MockCodeGenerator* generator =
    new MockCodeGenerator(generator_name, filename_prefix, error_text);
  mock_generators_to_delete_.push_back(generator);

  cli_.RegisterGenerator(flag_name, generator, help_text);
  return generator;
}

CommandLineInterfaceTest::NullCodeGenerator*
CommandLineInterfaceTest::RegisterNullGenerator(
    const string& flag_name) {
  NullCodeGenerator* generator = new NullCodeGenerator;
  mock_generators_to_delete_.push_back(generator);
  cli_.RegisterGenerator(flag_name, generator, "");
  return generator;
}

void CommandLineInterfaceTest::CreateTempFile(
    const string& name,
    const string& contents) {
  // Create parent directory, if necessary.
  string::size_type slash_pos = name.find_last_of('/');
  if (slash_pos != string::npos) {
    string dir = name.substr(0, slash_pos);
    File::RecursivelyCreateDir(temp_directory_ + "/" + dir, 0777);
  }

  // Write file.
  string full_name = temp_directory_ + "/" + name;
  File::WriteStringToFileOrDie(contents, full_name);
}

// -------------------------------------------------------------------

void CommandLineInterfaceTest::ExpectNoErrors() {
  EXPECT_EQ(0, return_code_);
  EXPECT_EQ("", error_text_);
}

void CommandLineInterfaceTest::ExpectErrorText(const string& expected_text) {
  EXPECT_NE(0, return_code_);
  EXPECT_EQ(StringReplace(expected_text, "$tmpdir", temp_directory_, true),
            error_text_);
}

void CommandLineInterfaceTest::ExpectErrorSubstring(
    const string& expected_substring) {
  EXPECT_NE(0, return_code_);
  EXPECT_PRED_FORMAT2(testing::IsSubstring, expected_substring, error_text_);
}

bool CommandLineInterfaceTest::HasAlternateErrorSubstring(
    const string& expected_substring) {
  EXPECT_NE(0, return_code_);
  return error_text_.find(expected_substring) != string::npos;
}

void CommandLineInterfaceTest::ExpectGenerated(
    const string& generator_name,
    const string& parameter,
    const string& proto_name,
    const string& message_name,
    const string& output_file_prefix) {
  // Open and read the file.
  string output_file = output_file_prefix + "." + proto_name;
  string file_contents;
  ASSERT_TRUE(File::ReadFileToString(temp_directory_ + "/" + output_file,
                                     &file_contents))
    << "Failed to open file: " + output_file;

  // Check that the contents are as we expect.
  string expected_contents =
    generator_name + ": " + parameter + ", " + proto_name + ", " +
    message_name + "\n";
  EXPECT_EQ(expected_contents, file_contents)
    << "Output file did not have expected contents: " + output_file;
}

void CommandLineInterfaceTest::ReadDescriptorSet(
    const string& filename, FileDescriptorSet* descriptor_set) {
  string path = temp_directory_ + "/" + filename;
  string file_contents;
  if (!File::ReadFileToString(path, &file_contents)) {
    FAIL() << "File not found: " << path;
  }
  if (!descriptor_set->ParseFromString(file_contents)) {
    FAIL() << "Could not parse file contents: " << path;
  }
}

// ===================================================================

CommandLineInterfaceTest::MockCodeGenerator::MockCodeGenerator(
    const string& name, const string& filename_prefix)
  : name_(name),
    filename_prefix_(filename_prefix),
    return_error_(false),
    expect_write_error_(false) {
}

CommandLineInterfaceTest::MockCodeGenerator::MockCodeGenerator(
    const string& name, const string& filename_prefix, const string& error)
  : name_(name),
    filename_prefix_(filename_prefix),
    return_error_(true),
    error_(error),
    expect_write_error_(false) {
}

CommandLineInterfaceTest::MockCodeGenerator::~MockCodeGenerator() {}

bool CommandLineInterfaceTest::MockCodeGenerator::Generate(
    const FileDescriptor* file,
    const string& parameter,
    OutputDirectory* output_directory,
    string* error) const {
  scoped_ptr<io::ZeroCopyOutputStream> output(
    output_directory->Open(filename_prefix_ + "." + file->name()));
  io::Printer printer(output.get(), '$');
  map<string, string> vars;
  vars["name"] = name_;
  vars["parameter"] = parameter;
  vars["proto_name"] = file->name();
  vars["message_name"] = file->message_type_count() > 0 ?
    file->message_type(0)->full_name().c_str() : "(none)";

  printer.Print(vars, "$name$: $parameter$, $proto_name$, $message_name$\n");

  if (expect_write_error_) {
    EXPECT_TRUE(printer.failed());
  } else {
    EXPECT_FALSE(printer.failed());
  }

  *error = error_;
  return !return_error_;
}

// ===================================================================

TEST_F(CommandLineInterfaceTest, BasicOutput) {
  // Test that the common case works.

  RegisterGenerator("test_generator", "--test_out",
                    "output.test", "Test output.");

  CreateTempFile("foo.proto",
    "syntax = \"proto2\";\n"
    "message Foo {}\n");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator", "", "foo.proto", "Foo", "output.test");
}

TEST_F(CommandLineInterfaceTest, MultipleInputs) {
  // Test parsing multiple input files.

  RegisterGenerator("test_generator", "--test_out",
                    "output.test", "Test output.");

  CreateTempFile("foo.proto",
    "syntax = \"proto2\";\n"
    "message Foo {}\n");
  CreateTempFile("bar.proto",
    "syntax = \"proto2\";\n"
    "message Bar {}\n");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir foo.proto bar.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator", "", "foo.proto", "Foo", "output.test");
  ExpectGenerated("test_generator", "", "bar.proto", "Bar", "output.test");
}

TEST_F(CommandLineInterfaceTest, CreateDirectory) {
  // Test that when we output to a sub-directory, it is created.

  RegisterGenerator("test_generator", "--test_out",
                    "bar/baz/output.test", "Test output.");

  CreateTempFile("foo.proto",
    "syntax = \"proto2\";\n"
    "message Foo {}\n");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator", "",
                  "foo.proto", "Foo", "bar/baz/output.test");
}

TEST_F(CommandLineInterfaceTest, GeneratorParameters) {
  // Test that generator parameters are correctly parsed from the command line.

  RegisterGenerator("test_generator", "--test_out",
                    "output.test", "Test output.");

  CreateTempFile("foo.proto",
    "syntax = \"proto2\";\n"
    "message Foo {}\n");

  Run("protocol_compiler --test_out=TestParameter:$tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator", "TestParameter",
                  "foo.proto", "Foo", "output.test");
}

#if defined(_WIN32) || defined(__CYGWIN__)

TEST_F(CommandLineInterfaceTest, WindowsOutputPath) {
  // Test that the output path can be a Windows-style path.

  NullCodeGenerator* generator = RegisterNullGenerator("--test_out");

  CreateTempFile("foo.proto",
    "syntax = \"proto2\";\n");

  Run("protocol_compiler --test_out=C:\\ "
      "--proto_path=$tmpdir foo.proto");

  ExpectNoErrors();
  EXPECT_TRUE(generator->called_);
  EXPECT_EQ("", generator->parameter_);
}

TEST_F(CommandLineInterfaceTest, WindowsOutputPathAndParameter) {
  // Test that we can have a windows-style output path and a parameter.

  NullCodeGenerator* generator = RegisterNullGenerator("--test_out");

  CreateTempFile("foo.proto",
    "syntax = \"proto2\";\n");

  Run("protocol_compiler --test_out=bar:C:\\ "
      "--proto_path=$tmpdir foo.proto");

  ExpectNoErrors();
  EXPECT_TRUE(generator->called_);
  EXPECT_EQ("bar", generator->parameter_);
}

#endif  // defined(_WIN32) || defined(__CYGWIN__)

TEST_F(CommandLineInterfaceTest, PathLookup) {
  // Test that specifying multiple directories in the proto search path works.

  RegisterGenerator("test_generator", "--test_out",
                    "output.test", "Test output.");

  CreateTempFile("b/bar.proto",
    "syntax = \"proto2\";\n"
    "message Bar {}\n");
  CreateTempFile("a/foo.proto",
    "syntax = \"proto2\";\n"
    "import \"bar.proto\";\n"
    "message Foo {\n"
    "  optional Bar a = 1;\n"
    "}\n");
  CreateTempFile("b/foo.proto", "this should not be parsed\n");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir/a --proto_path=$tmpdir/b foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator", "", "foo.proto", "Foo", "output.test");
}

TEST_F(CommandLineInterfaceTest, ColonDelimitedPath) {
  // Same as PathLookup, but we provide the proto_path in a single flag.

  RegisterGenerator("test_generator", "--test_out",
                    "output.test", "Test output.");

  CreateTempFile("b/bar.proto",
    "syntax = \"proto2\";\n"
    "message Bar {}\n");
  CreateTempFile("a/foo.proto",
    "syntax = \"proto2\";\n"
    "import \"bar.proto\";\n"
    "message Foo {\n"
    "  optional Bar a = 1;\n"
    "}\n");
  CreateTempFile("b/foo.proto", "this should not be parsed\n");

#undef PATH_SEPARATOR
#if defined(_WIN32)
#define PATH_SEPARATOR ";"
#else
#define PATH_SEPARATOR ":"
#endif

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir/a"PATH_SEPARATOR"$tmpdir/b foo.proto");

#undef PATH_SEPARATOR

  ExpectNoErrors();
  ExpectGenerated("test_generator", "", "foo.proto", "Foo", "output.test");
}

TEST_F(CommandLineInterfaceTest, NonRootMapping) {
  // Test setting up a search path mapping a directory to a non-root location.

  RegisterGenerator("test_generator", "--test_out",
                    "output.test", "Test output.");

  CreateTempFile("foo.proto",
    "syntax = \"proto2\";\n"
    "message Foo {}\n");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=bar=$tmpdir bar/foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator", "", "bar/foo.proto", "Foo", "output.test");
}

TEST_F(CommandLineInterfaceTest, MultipleGenerators) {
  // Test that we can have multiple generators and use both in one invocation,
  // each with a different output directory.

  RegisterGenerator("test_generator_1", "--test1_out",
                    "output1.test", "Test output 1.");
  RegisterGenerator("test_generator_2", "--test2_out",
                    "output2.test", "Test output 2.");

  CreateTempFile("foo.proto",
    "syntax = \"proto2\";\n"
    "message Foo {}\n");
  // Create the "a" and "b" sub-directories.
  CreateTempFile("a/dummy", "");
  CreateTempFile("b/dummy", "");

  Run("protocol_compiler "
      "--test1_out=$tmpdir/a "
      "--test2_out=$tmpdir/b "
      "--proto_path=$tmpdir foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator_1", "", "foo.proto", "Foo", "a/output1.test");
  ExpectGenerated("test_generator_2", "", "foo.proto", "Foo", "b/output2.test");
}

TEST_F(CommandLineInterfaceTest, DisallowServicesNoServices) {
  // Test that --disallow_services doesn't cause a problem when there are no
  // services.

  RegisterGenerator("test_generator", "--test_out",
                    "output.test", "Test output.");

  CreateTempFile("foo.proto",
    "syntax = \"proto2\";\n"
    "message Foo {}\n");

  Run("protocol_compiler --disallow_services --test_out=$tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator", "", "foo.proto", "Foo", "output.test");
}

TEST_F(CommandLineInterfaceTest, DisallowServicesHasService) {
  // Test that --disallow_services produces an error when there are services.

  RegisterGenerator("test_generator", "--test_out",
                    "output.test", "Test output.");

  CreateTempFile("foo.proto",
    "syntax = \"proto2\";\n"
    "message Foo {}\n"
    "service Bar {}\n");

  Run("protocol_compiler --disallow_services --test_out=$tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectErrorSubstring("foo.proto: This file contains services");
}

TEST_F(CommandLineInterfaceTest, AllowServicesHasService) {
  // Test that services work fine as long as --disallow_services is not used.

  RegisterGenerator("test_generator", "--test_out",
                    "output.test", "Test output.");

  CreateTempFile("foo.proto",
    "syntax = \"proto2\";\n"
    "message Foo {}\n"
    "service Bar {}\n");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator", "", "foo.proto", "Foo", "output.test");
}

TEST_F(CommandLineInterfaceTest, CwdRelativeInputs) {
  // Test that we can accept working-directory-relative input files.

  SetInputsAreProtoPathRelative(false);

  RegisterGenerator("test_generator", "--test_out",
                    "output.test", "Test output.");

  CreateTempFile("foo.proto",
    "syntax = \"proto2\";\n"
    "message Foo {}\n");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir $tmpdir/foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator", "", "foo.proto", "Foo", "output.test");
}

TEST_F(CommandLineInterfaceTest, WriteDescriptorSet) {
  CreateTempFile("foo.proto",
    "syntax = \"proto2\";\n"
    "message Foo {}\n");
  CreateTempFile("bar.proto",
    "syntax = \"proto2\";\n"
    "import \"foo.proto\";\n"
    "message Bar {\n"
    "  optional Foo foo = 1;\n"
    "}\n");

  Run("protocol_compiler --descriptor_set_out=$tmpdir/descriptor_set "
      "--proto_path=$tmpdir bar.proto");

  ExpectNoErrors();

  FileDescriptorSet descriptor_set;
  ReadDescriptorSet("descriptor_set", &descriptor_set);
  if (HasFatalFailure()) return;
  ASSERT_EQ(1, descriptor_set.file_size());
  EXPECT_EQ("bar.proto", descriptor_set.file(0).name());
}

TEST_F(CommandLineInterfaceTest, WriteTransitiveDescriptorSet) {
  CreateTempFile("foo.proto",
    "syntax = \"proto2\";\n"
    "message Foo {}\n");
  CreateTempFile("bar.proto",
    "syntax = \"proto2\";\n"
    "import \"foo.proto\";\n"
    "message Bar {\n"
    "  optional Foo foo = 1;\n"
    "}\n");

  Run("protocol_compiler --descriptor_set_out=$tmpdir/descriptor_set "
      "--include_imports --proto_path=$tmpdir bar.proto");

  ExpectNoErrors();

  FileDescriptorSet descriptor_set;
  ReadDescriptorSet("descriptor_set", &descriptor_set);
  if (HasFatalFailure()) return;
  ASSERT_EQ(2, descriptor_set.file_size());
  if (descriptor_set.file(0).name() == "bar.proto") {
    swap(descriptor_set.mutable_file()->mutable_data()[0],
         descriptor_set.mutable_file()->mutable_data()[1]);
  }
  EXPECT_EQ("foo.proto", descriptor_set.file(0).name());
  EXPECT_EQ("bar.proto", descriptor_set.file(1).name());
}

// -------------------------------------------------------------------

TEST_F(CommandLineInterfaceTest, ParseErrors) {
  // Test that parse errors are reported.

  RegisterGenerator("test_generator", "--test_out",
                    "output.test", "Test output.");

  CreateTempFile("foo.proto",
    "syntax = \"proto2\";\n"
    "badsyntax\n");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectErrorText(
    "foo.proto:2:1: Expected top-level statement (e.g. \"message\").\n");
}

TEST_F(CommandLineInterfaceTest, ParseErrorsMultipleFiles) {
  // Test that parse errors are reported from multiple files.

  RegisterGenerator("test_generator", "--test_out",
                    "output.test", "Test output.");

  // We set up files such that foo.proto actually depends on bar.proto in
  // two ways:  Directly and through baz.proto.  bar.proto's errors should
  // only be reported once.
  CreateTempFile("bar.proto",
    "syntax = \"proto2\";\n"
    "badsyntax\n");
  CreateTempFile("baz.proto",
    "syntax = \"proto2\";\n"
    "import \"bar.proto\";\n");
  CreateTempFile("foo.proto",
    "syntax = \"proto2\";\n"
    "import \"bar.proto\";\n"
    "import \"baz.proto\";\n");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectErrorText(
    "bar.proto:2:1: Expected top-level statement (e.g. \"message\").\n"
    "baz.proto: Import \"bar.proto\" was not found or had errors.\n"
    "foo.proto: Import \"bar.proto\" was not found or had errors.\n"
    "foo.proto: Import \"baz.proto\" was not found or had errors.\n");
}

TEST_F(CommandLineInterfaceTest, InputNotFoundError) {
  // Test what happens if the input file is not found.

  RegisterGenerator("test_generator", "--test_out",
                    "output.test", "Test output.");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectErrorText(
    "foo.proto: File not found.\n");
}

TEST_F(CommandLineInterfaceTest, CwdRelativeInputNotFoundError) {
  // Test what happens when a working-directory-relative input file is not
  // found.

  SetInputsAreProtoPathRelative(false);

  RegisterGenerator("test_generator", "--test_out",
                    "output.test", "Test output.");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir $tmpdir/foo.proto");

  ExpectErrorText(
    "$tmpdir/foo.proto: No such file or directory\n");
}

TEST_F(CommandLineInterfaceTest, CwdRelativeInputNotMappedError) {
  // Test what happens when a working-directory-relative input file is not
  // mapped to a virtual path.

  SetInputsAreProtoPathRelative(false);

  RegisterGenerator("test_generator", "--test_out",
                    "output.test", "Test output.");

  CreateTempFile("foo.proto",
    "syntax = \"proto2\";\n"
    "message Foo {}\n");

  // Create a directory called "bar" so that we can point --proto_path at it.
  CreateTempFile("bar/dummy", "");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir/bar $tmpdir/foo.proto");

  ExpectErrorText(
    "$tmpdir/foo.proto: File does not reside within any path "
      "specified using --proto_path (or -I).  You must specify a "
      "--proto_path which encompasses this file.\n");
}

TEST_F(CommandLineInterfaceTest, CwdRelativeInputNotFoundAndNotMappedError) {
  // Check what happens if the input file is not found *and* is not mapped
  // in the proto_path.

  SetInputsAreProtoPathRelative(false);

  RegisterGenerator("test_generator", "--test_out",
                    "output.test", "Test output.");

  // Create a directory called "bar" so that we can point --proto_path at it.
  CreateTempFile("bar/dummy", "");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir/bar $tmpdir/foo.proto");

  ExpectErrorText(
    "$tmpdir/foo.proto: No such file or directory\n");
}

TEST_F(CommandLineInterfaceTest, CwdRelativeInputShadowedError) {
  // Test what happens when a working-directory-relative input file is shadowed
  // by another file in the virtual path.

  SetInputsAreProtoPathRelative(false);

  RegisterGenerator("test_generator", "--test_out",
                    "output.test", "Test output.");

  CreateTempFile("foo/foo.proto",
    "syntax = \"proto2\";\n"
    "message Foo {}\n");
  CreateTempFile("bar/foo.proto",
    "syntax = \"proto2\";\n"
    "message Bar {}\n");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir/foo --proto_path=$tmpdir/bar "
      "$tmpdir/bar/foo.proto");

  ExpectErrorText(
    "$tmpdir/bar/foo.proto: Input is shadowed in the --proto_path "
    "by \"$tmpdir/foo/foo.proto\".  Either use the latter "
    "file as your input or reorder the --proto_path so that the "
    "former file's location comes first.\n");
}

TEST_F(CommandLineInterfaceTest, ProtoPathNotFoundError) {
  // Test what happens if the input file is not found.

  RegisterGenerator("test_generator", "--test_out",
                    "output.test", "Test output.");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir/foo foo.proto");

  ExpectErrorText(
    "$tmpdir/foo: warning: directory does not exist.\n"
    "foo.proto: File not found.\n");
}

TEST_F(CommandLineInterfaceTest, MissingInputError) {
  // Test that we get an error if no inputs are given.

  RegisterGenerator("test_generator", "--test_out",
                    "output.test", "Test output.");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir");

  ExpectErrorText("Missing input file.\n");
}

TEST_F(CommandLineInterfaceTest, MissingOutputError) {
  RegisterGenerator("test_generator", "--test_out",
                    "output.test", "Test output.");

  CreateTempFile("foo.proto",
    "syntax = \"proto2\";\n"
    "message Foo {}\n");

  Run("protocol_compiler --proto_path=$tmpdir foo.proto");

  ExpectErrorText("Missing output directives.\n");
}

TEST_F(CommandLineInterfaceTest, OutputWriteError) {
  MockCodeGenerator* generator =
    RegisterGenerator("test_generator", "--test_out",
                      "output.test", "Test output.");
  generator->set_expect_write_error(true);

  CreateTempFile("foo.proto",
    "syntax = \"proto2\";\n"
    "message Foo {}\n");

  // Create a directory blocking our output location.
  CreateTempFile("output.test.foo.proto/foo", "");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir foo.proto");

#if defined(_WIN32) && !defined(__CYGWIN__)
  // Windows with MSVCRT.dll produces EPERM instead of EISDIR.
  if (HasAlternateErrorSubstring("output.test.foo.proto: Permission denied")) {
    return;
  }
#endif

  ExpectErrorSubstring("output.test.foo.proto: Is a directory");
}

TEST_F(CommandLineInterfaceTest, OutputDirectoryNotFoundError) {
  RegisterGenerator("test_generator", "--test_out",
                    "output.test", "Test output.");

  CreateTempFile("foo.proto",
    "syntax = \"proto2\";\n"
    "message Foo {}\n");

  Run("protocol_compiler --test_out=$tmpdir/nosuchdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectErrorSubstring("nosuchdir/: "
                       "No such file or directory");
}

TEST_F(CommandLineInterfaceTest, OutputDirectoryIsFileError) {
  RegisterGenerator("test_generator", "--test_out",
                    "output.test", "Test output.");

  CreateTempFile("foo.proto",
    "syntax = \"proto2\";\n"
    "message Foo {}\n");

  Run("protocol_compiler --test_out=$tmpdir/foo.proto "
      "--proto_path=$tmpdir foo.proto");

#if defined(_WIN32) && !defined(__CYGWIN__)
  // Windows with MSVCRT.dll produces EINVAL instead of ENOTDIR.
  if (HasAlternateErrorSubstring("foo.proto/: Invalid argument")) {
    return;
  }
#endif

  ExpectErrorSubstring("foo.proto/: Not a directory");
}

TEST_F(CommandLineInterfaceTest, GeneratorError) {
  RegisterErrorGenerator("error_generator", "Test error message.",
                         "--error_out", "output.test", "Test error output.");

  CreateTempFile("foo.proto",
    "syntax = \"proto2\";\n"
    "message Foo {}\n");

  Run("protocol_compiler --error_out=$tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectErrorSubstring("--error_out: Test error message.");
}

TEST_F(CommandLineInterfaceTest, HelpText) {
  RegisterGenerator("test_generator", "--test_out",
                    "output.test", "Test output.");
  RegisterErrorGenerator("error_generator", "Test error message.",
                         "--error_out", "output.test", "Test error output.");

  CreateTempFile("foo.proto",
    "syntax = \"proto2\";\n"
    "message Foo {}\n");

  Run("test_exec_name --help");

  ExpectErrorSubstring("Usage: test_exec_name ");
  ExpectErrorSubstring("--test_out=OUT_DIR");
  ExpectErrorSubstring("Test output.");
  ExpectErrorSubstring("--error_out=OUT_DIR");
  ExpectErrorSubstring("Test error output.");
}

TEST_F(CommandLineInterfaceTest, GccFormatErrors) {
  // Test --error_format=gcc (which is the default, but we want to verify
  // that it can be set explicitly).

  RegisterGenerator("test_generator", "--test_out",
                    "output.test", "Test output.");

  CreateTempFile("foo.proto",
    "syntax = \"proto2\";\n"
    "badsyntax\n");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir --error_format=gcc foo.proto");

  ExpectErrorText(
    "foo.proto:2:1: Expected top-level statement (e.g. \"message\").\n");
}

TEST_F(CommandLineInterfaceTest, MsvsFormatErrors) {
  // Test --error_format=msvs

  RegisterGenerator("test_generator", "--test_out",
                    "output.test", "Test output.");

  CreateTempFile("foo.proto",
    "syntax = \"proto2\";\n"
    "badsyntax\n");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir --error_format=msvs foo.proto");

  ExpectErrorText(
    "foo.proto(2) : error in column=1: Expected top-level statement "
      "(e.g. \"message\").\n");
}

TEST_F(CommandLineInterfaceTest, InvalidErrorFormat) {
  // Test --error_format=msvs

  RegisterGenerator("test_generator", "--test_out",
                    "output.test", "Test output.");

  CreateTempFile("foo.proto",
    "syntax = \"proto2\";\n"
    "badsyntax\n");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir --error_format=invalid foo.proto");

  ExpectErrorText(
    "Unknown error format: invalid\n");
}

// -------------------------------------------------------------------
// Flag parsing tests

TEST_F(CommandLineInterfaceTest, ParseSingleCharacterFlag) {
  // Test that a single-character flag works.

  RegisterGenerator("test_generator", "-t",
                    "output.test", "Test output.");

  CreateTempFile("foo.proto",
    "syntax = \"proto2\";\n"
    "message Foo {}\n");

  Run("protocol_compiler -t$tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator", "", "foo.proto", "Foo", "output.test");
}

TEST_F(CommandLineInterfaceTest, ParseSpaceDelimitedValue) {
  // Test that separating the flag value with a space works.

  RegisterGenerator("test_generator", "--test_out",
                    "output.test", "Test output.");

  CreateTempFile("foo.proto",
    "syntax = \"proto2\";\n"
    "message Foo {}\n");

  Run("protocol_compiler --test_out $tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator", "", "foo.proto", "Foo", "output.test");
}

TEST_F(CommandLineInterfaceTest, ParseSingleCharacterSpaceDelimitedValue) {
  // Test that separating the flag value with a space works for
  // single-character flags.

  RegisterGenerator("test_generator", "-t",
                    "output.test", "Test output.");

  CreateTempFile("foo.proto",
    "syntax = \"proto2\";\n"
    "message Foo {}\n");

  Run("protocol_compiler -t $tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator", "", "foo.proto", "Foo", "output.test");
}

TEST_F(CommandLineInterfaceTest, MissingValueError) {
  // Test that we get an error if a flag is missing its value.

  RegisterGenerator("test_generator", "--test_out",
                    "output.test", "Test output.");

  Run("protocol_compiler --test_out --proto_path=$tmpdir foo.proto");

  ExpectErrorText("Missing value for flag: --test_out\n");
}

TEST_F(CommandLineInterfaceTest, MissingValueAtEndError) {
  // Test that we get an error if the last argument is a flag requiring a
  // value.

  RegisterGenerator("test_generator", "--test_out",
                    "output.test", "Test output.");

  Run("protocol_compiler --test_out");

  ExpectErrorText("Missing value for flag: --test_out\n");
}

// ===================================================================

// Test for --encode and --decode.  Note that it would be easier to do this
// test as a shell script, but we'd like to be able to run the test on
// platforms that don't have a Bourne-compatible shell available (especially
// Windows/MSVC).
class EncodeDecodeTest : public testing::Test {
 protected:
  virtual void SetUp() {
    duped_stdin_ = dup(STDIN_FILENO);
  }

  virtual void TearDown() {
    dup2(duped_stdin_, STDIN_FILENO);
    close(duped_stdin_);
  }

  void RedirectStdinFromText(const string& input) {
    string filename = TestTempDir() + "/test_stdin";
    File::WriteStringToFileOrDie(input, filename);
    GOOGLE_CHECK(RedirectStdinFromFile(filename));
  }

  bool RedirectStdinFromFile(const string& filename) {
    int fd = open(filename.c_str(), O_RDONLY);
    if (fd < 0) return false;
    dup2(fd, STDIN_FILENO);
    close(fd);
    return true;
  }

  // Remove '\r' characters from text.
  string StripCR(const string& text) {
    string result;

    for (int i = 0; i < text.size(); i++) {
      if (text[i] != '\r') {
        result.push_back(text[i]);
      }
    }

    return result;
  }

  enum Type { TEXT, BINARY };
  enum ReturnCode { SUCCESS, ERROR };

  bool Run(const string& command) {
    vector<string> args;
    args.push_back("protoc");
    SplitStringUsing(command, " ", &args);
    args.push_back("--proto_path=" + TestSourceDir());

    scoped_array<const char*> argv(new const char*[args.size()]);
    for (int i = 0; i < args.size(); i++) {
      argv[i] = args[i].c_str();
    }

    CommandLineInterface cli;
    cli.SetInputsAreProtoPathRelative(true);

    CaptureTestStdout();
    CaptureTestStderr();

    int result = cli.Run(args.size(), argv.get());

    captured_stdout_ = GetCapturedTestStdout();
    captured_stderr_ = GetCapturedTestStderr();

    return result == 0;
  }

  void ExpectStdoutMatchesBinaryFile(const string& filename) {
    string expected_output;
    ASSERT_TRUE(File::ReadFileToString(filename, &expected_output));

    // Don't use EXPECT_EQ because we don't want to print raw binary data to
    // stdout on failure.
    EXPECT_TRUE(captured_stdout_ == expected_output);
  }

  void ExpectStdoutMatchesTextFile(const string& filename) {
    string expected_output;
    ASSERT_TRUE(File::ReadFileToString(filename, &expected_output));

    ExpectStdoutMatchesText(expected_output);
  }

  void ExpectStdoutMatchesText(const string& expected_text) {
    EXPECT_EQ(StripCR(expected_text), StripCR(captured_stdout_));
  }

  void ExpectStderrMatchesText(const string& expected_text) {
    EXPECT_EQ(StripCR(expected_text), StripCR(captured_stderr_));
  }

 private:
  int duped_stdin_;
  string captured_stdout_;
  string captured_stderr_;
};

TEST_F(EncodeDecodeTest, Encode) {
  RedirectStdinFromFile(TestSourceDir() +
    "/google/protobuf/testdata/text_format_unittest_data.txt");
  EXPECT_TRUE(Run("google/protobuf/unittest.proto "
                  "--encode=protobuf_unittest.TestAllTypes"));
  ExpectStdoutMatchesBinaryFile(TestSourceDir() +
    "/google/protobuf/testdata/golden_message");
  ExpectStderrMatchesText("");
}

TEST_F(EncodeDecodeTest, Decode) {
  RedirectStdinFromFile(TestSourceDir() +
    "/google/protobuf/testdata/golden_message");
  EXPECT_TRUE(Run("google/protobuf/unittest.proto "
                  "--decode=protobuf_unittest.TestAllTypes"));
  ExpectStdoutMatchesTextFile(TestSourceDir() +
    "/google/protobuf/testdata/text_format_unittest_data.txt");
  ExpectStderrMatchesText("");
}

TEST_F(EncodeDecodeTest, Partial) {
  RedirectStdinFromText("");
  EXPECT_TRUE(Run("google/protobuf/unittest.proto "
                  "--encode=protobuf_unittest.TestRequired"));
  ExpectStdoutMatchesText("");
  ExpectStderrMatchesText(
    "warning:  Input message is missing required fields:  a, b, c\n");
}

TEST_F(EncodeDecodeTest, DecodeRaw) {
  protobuf_unittest::TestAllTypes message;
  message.set_optional_int32(123);
  message.set_optional_string("foo");
  string data;
  message.SerializeToString(&data);

  RedirectStdinFromText(data);
  EXPECT_TRUE(Run("--decode_raw"));
  ExpectStdoutMatchesText("1: 123\n"
                          "14: \"foo\"\n");
  ExpectStderrMatchesText("");
}

TEST_F(EncodeDecodeTest, UnknownType) {
  EXPECT_FALSE(Run("google/protobuf/unittest.proto "
                   "--encode=NoSuchType"));
  ExpectStdoutMatchesText("");
  ExpectStderrMatchesText("Type not defined: NoSuchType\n");
}

TEST_F(EncodeDecodeTest, ProtoParseError) {
  EXPECT_FALSE(Run("google/protobuf/no_such_file.proto "
                   "--encode=NoSuchType"));
  ExpectStdoutMatchesText("");
  ExpectStderrMatchesText(
    "google/protobuf/no_such_file.proto: File not found.\n");
}

}  // anonymous namespace

}  // namespace compiler
}  // namespace protobuf
}  // namespace google
