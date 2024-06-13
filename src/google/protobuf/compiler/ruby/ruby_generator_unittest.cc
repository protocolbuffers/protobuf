// Protocol Buffers - Google's data interchange format
// Copyright 2014 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/ruby/ruby_generator.h"

#include <list>
#include <memory>

#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/googletest.h"
#include <gtest/gtest.h>
#include "google/protobuf/compiler/command_line_interface.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/zero_copy_stream.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace ruby {
namespace {

std::string FindRubyTestDir() {
  return absl::StrCat(TestSourceDir(), "/google/protobuf/compiler/ruby");
}

// This test is a simple golden-file test over the output of the Ruby code
// generator. When we make changes to the Ruby extension and alter the Ruby code
// generator to use those changes, we should (i) manually test the output of the
// code generator with the extension, and (ii) update the golden output above.
// Some day, we may integrate build systems between protoc and the language
// extensions to the point where we can do this test in a more automated way.

void RubyTest(std::string proto_file, std::string import_proto_file = "") {
  std::string ruby_tests = FindRubyTestDir();

  google::protobuf::compiler::CommandLineInterface cli;
  cli.SetInputsAreProtoPathRelative(true);

  ruby::Generator ruby_generator;
  cli.RegisterGenerator("--ruby_out", &ruby_generator, "");

  // Copy generated_code.proto to the temporary test directory.
  std::string test_input;
  ABSL_CHECK_OK(File::GetContents(
      absl::StrCat(ruby_tests, proto_file, ".proto"), &test_input, true));
  ABSL_CHECK_OK(File::SetContents(
      absl::StrCat(TestTempDir(), proto_file, ".proto"), test_input, true));

  // Copy generated_code_import.proto to the temporary test directory.
  std::string test_import;
  if (!import_proto_file.empty()) {
    ABSL_CHECK_OK(
        File::GetContents(absl::StrCat(ruby_tests, import_proto_file, ".proto"),
                          &test_import, true));
    ABSL_CHECK_OK(File::SetContents(
        absl::StrCat(TestTempDir(), import_proto_file, ".proto"), test_import,
        true));
  }

  // Invoke the proto compiler (we will be inside TestTempDir() at this point).
  std::string ruby_out = absl::StrCat("--ruby_out=", TestTempDir());
  std::string proto_path = absl::StrCat("--proto_path=", TestTempDir());

  std::string proto_target = absl::StrCat(TestTempDir(), proto_file, ".proto");
  const char* argv[] = {
    "protoc",
    ruby_out.c_str(),
    proto_path.c_str(),
    proto_target.c_str(),
  };

  EXPECT_EQ(0, cli.Run(4, argv));

  // Load the generated output and compare to the expected result.
  std::string output;
  ABSL_CHECK_OK(File::GetContentsAsText(
      absl::StrCat(TestTempDir(), proto_file, "_pb.rb"), &output, true));
  std::string expected_output;
  ABSL_CHECK_OK(File::GetContentsAsText(
      absl::StrCat(ruby_tests, proto_file, "_pb.rb"), &expected_output, true));
  EXPECT_EQ(expected_output, output);
}

TEST(RubyGeneratorTest, Proto3GeneratorTest) {
  RubyTest("/ruby_generated_code", "/ruby_generated_code_proto2_import");
}

TEST(RubyGeneratorTest, Proto2GeneratorTest) {
    RubyTest("/ruby_generated_code_proto2", "/ruby_generated_code_proto2_import");
}

TEST(RubyGeneratorTest, Proto3ImplicitPackageTest) {
    RubyTest("/ruby_generated_pkg_implicit");
}

TEST(RubyGeneratorTest, Proto3ExplictPackageTest) {
    RubyTest("/ruby_generated_pkg_explicit");
}

TEST(RubyGeneratorTest, Proto3ExplictLegacyPackageTest) {
    RubyTest("/ruby_generated_pkg_explicit_legacy");
}

}  // namespace
}  // namespace ruby
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
