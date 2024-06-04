// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cstddef>
#include <cstdint>

#include <gmock/gmock.h>
#include "absl/log/absl_check.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/types/span.h"
#include "google/protobuf/compiler/command_line_interface_tester.h"
#include "google/protobuf/unittest_features.pb.h"
#include "google/protobuf/unittest_invalid_features.pb.h"

#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/file.h"
#include "google/protobuf/any.pb.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/testing/googletest.h"
#include <gtest/gtest.h>
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/command_line_interface.h"
#include "google/protobuf/compiler/cpp/names.h"
#include "google/protobuf/compiler/mock_code_generator.h"
#include "google/protobuf/compiler/plugin.pb.h"
#include "google/protobuf/test_textproto.h"
#include "google/protobuf/test_util2.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest_custom_options.pb.h"

#ifdef GOOGLE_PROTOBUF_USE_BAZEL_GENERATED_PLUGIN_PATHS
// This is needed because of https://github.com/bazelbuild/bazel/issues/19124.
#include "google/protobuf/compiler/test_plugin_paths.h"
#endif  // GOOGLE_PROTOBUF_USE_BAZEL_GENERATED_PLUGIN_PATHS

#ifdef _WIN32
#include "google/protobuf/compiler/subprocess.h"
#include "google/protobuf/io/io_win32.h"
#endif

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {

#if defined(_WIN32)
// DO NOT include <io.h>, instead create functions in io_win32.{h,cc} and import
// them like we do below.
using google::protobuf::io::win32::access;
using google::protobuf::io::win32::close;
using google::protobuf::io::win32::dup;
using google::protobuf::io::win32::dup2;
using google::protobuf::io::win32::open;
using google::protobuf::io::win32::write;
#endif

// Disable the whole test when we use tcmalloc for "draconian" heap checks, in
// which case tcmalloc will print warnings that fail the plugin tests.
#if !GOOGLE_PROTOBUF_HEAP_CHECK_DRACONIAN


namespace {

std::string CreatePluginArg() {
  std::string plugin_path;
#ifdef GOOGLE_PROTOBUF_TEST_PLUGIN_PATH
  plugin_path = GOOGLE_PROTOBUF_TEST_PLUGIN_PATH;
#else
  const char* possible_paths[] = {
      // When building with shared libraries, libtool hides the real
      // executable
      // in .libs and puts a fake wrapper in the current directory.
      // Unfortunately, due to an apparent bug on Cygwin/MinGW, if one program
      // wrapped in this way (e.g. protobuf-tests.exe) tries to execute
      // another
      // program wrapped in this way (e.g. test_plugin.exe), the latter fails
      // with error code 127 and no explanation message.  Presumably the
      // problem
      // is that the wrapper for protobuf-tests.exe set some environment
      // variables that confuse the wrapper for test_plugin.exe.  Luckily, it
      // turns out that if we simply invoke the wrapped test_plugin.exe
      // directly, it works -- I guess the environment variables set by the
      // protobuf-tests.exe wrapper happen to be correct for it too.  So we do
      // that.
      ".libs/test_plugin.exe",  // Win32 w/autotool (Cygwin / MinGW)
      "test_plugin.exe",        // Other Win32 (MSVC)
      "test_plugin",            // Unix
  };
  for (int i = 0; i < ABSL_ARRAYSIZE(possible_paths); ++i) {
    if (access(possible_paths[i], F_OK) == 0) {
      plugin_path = possible_paths[i];
      break;
    }
  }
#endif

  if (plugin_path.empty() || !File::Exists(plugin_path)) {
    ABSL_LOG(ERROR)
        << "Plugin executable not found.  Plugin tests are likely to fail. "
        << plugin_path;
    return "";
  }
  return absl::StrCat("--plugin=prefix-gen-plug=", plugin_path);
}

class CommandLineInterfaceTest : public CommandLineInterfaceTester {
 protected:
  CommandLineInterfaceTest();

  // Runs the CommandLineInterface with the given command line.  The
  // command is automatically split on spaces, and the string "$tmpdir"
  // is replaced with TestTempDir().
  void Run(std::string command);
  void RunWithArgs(std::vector<std::string> args);

  // -----------------------------------------------------------------
  // Methods to set up the test (called before Run()).

  class NullCodeGenerator;

  // Normally plugins are allowed for all tests.  Call this to explicitly
  // disable them.
  void DisallowPlugins() { disallow_plugins_ = true; }
  // Checks that MockCodeGenerator::Generate() was called in the given
  // context (or the generator in test_plugin.cc, which produces the same
  // output).  That is, this tests if the generator with the given name
  // was called with the given parameter and proto file and produced the
  // given output file.  This is checked by reading the output file and
  // checking that it contains the content that MockCodeGenerator would
  // generate given these inputs.  message_name is the name of the first
  // message that appeared in the proto file; this is just to make extra
  // sure that the correct file was parsed.
  void ExpectGenerated(const std::string& generator_name,
                       const std::string& parameter,
                       const std::string& proto_name,
                       const std::string& message_name);
  void ExpectGenerated(const std::string& generator_name,
                       const std::string& parameter,
                       const std::string& proto_name,
                       const std::string& message_name,
                       const std::string& output_directory);
  void ExpectGeneratedWithMultipleInputs(const std::string& generator_name,
                                         const std::string& all_proto_names,
                                         const std::string& proto_name,
                                         const std::string& message_name);
  void ExpectGeneratedWithInsertions(const std::string& generator_name,
                                     const std::string& parameter,
                                     const std::string& insertions,
                                     const std::string& proto_name,
                                     const std::string& message_name);
  void CheckGeneratedAnnotations(const std::string& name,
                                 const std::string& file);

#if defined(_WIN32)
  void ExpectNullCodeGeneratorCalled(const std::string& parameter);
#endif  // _WIN32


  std::string ReadFile(absl::string_view filename);
  void ReadDescriptorSet(absl::string_view filename,
                         FileDescriptorSet* descriptor_set);

  void WriteDescriptorSet(absl::string_view filename,
                          const FileDescriptorSet* descriptor_set);

  FeatureSetDefaults ReadEditionDefaults(absl::string_view filename);

  // The default code generators support all features. Use this to create a
  // code generator that omits the given feature(s).
  void CreateGeneratorWithMissingFeatures(const std::string& name,
                                          const std::string& description,
                                          uint64_t features) {
    auto generator = std::make_unique<MockCodeGenerator>(name);
    generator->SuppressFeatures(features);
    RegisterGenerator(name, std::move(generator), description);
  }

  void SetMockGeneratorTestCase(absl::string_view name) {
#ifdef _WIN32
    ::_putenv(absl::StrCat("TEST_CASE", "=", name).c_str());
#else
    ::setenv("TEST_CASE", name.data(), 1);
#endif
  }

  MockCodeGenerator* mock_generator_ = nullptr;

 private:
  // Was DisallowPlugins() called?
  bool disallow_plugins_ = false;

  NullCodeGenerator* null_generator_ = nullptr;
};

class CommandLineInterfaceTest::NullCodeGenerator : public CodeGenerator {
 public:
  NullCodeGenerator() : called_(false) {}
  ~NullCodeGenerator() override = default;

  mutable bool called_;
  mutable std::string parameter_;

  // implements CodeGenerator ----------------------------------------
  bool Generate(const FileDescriptor* file, const std::string& parameter,
                GeneratorContext* context, std::string* error) const override {
    called_ = true;
    parameter_ = parameter;
    return true;
  }
};

// ===================================================================

CommandLineInterfaceTest::CommandLineInterfaceTest() {
  // Reset the mock generator's test case environment variable.
  SetMockGeneratorTestCase("");

  // Register generators.
  auto mock_generator = std::make_unique<MockCodeGenerator>("test_generator");
  mock_generator_ = mock_generator.get();
  RegisterGenerator("--test_out", "--test_opt", std::move(mock_generator),
                    "Test output.");
  RegisterGenerator("-t", std::make_unique<MockCodeGenerator>("test_generator"),
                    "Test output.");

  RegisterGenerator("--alt_out",
                    std::make_unique<MockCodeGenerator>("alt_generator"),
                    "Alt output.");

  auto null_generator = std::make_unique<NullCodeGenerator>();
  null_generator_ = null_generator.get();
  RegisterGenerator("--null_out", std::move(null_generator), "Null output.");

}

void CommandLineInterfaceTest::Run(std::string command) {
  if (!disallow_plugins_) {
    AllowPlugins("prefix-");
    absl::StrAppend(&command, " ", CreatePluginArg());
  }

  RunProtoc(command);
}

void CommandLineInterfaceTest::RunWithArgs(std::vector<std::string> args) {
  if (!disallow_plugins_) {
    AllowPlugins("prefix-");
    args.push_back(CreatePluginArg());
  }

  RunProtocWithArgs(std::move(args));
}

// -------------------------------------------------------------------

void CommandLineInterfaceTest::ExpectGenerated(
    const std::string& generator_name, const std::string& parameter,
    const std::string& proto_name, const std::string& message_name) {
  MockCodeGenerator::ExpectGenerated(generator_name, parameter, "", proto_name,
                                     message_name, proto_name,
                                     temp_directory());
}

void CommandLineInterfaceTest::ExpectGenerated(
    const std::string& generator_name, const std::string& parameter,
    const std::string& proto_name, const std::string& message_name,
    const std::string& output_directory) {
  MockCodeGenerator::ExpectGenerated(
      generator_name, parameter, "", proto_name, message_name, proto_name,
      absl::StrCat(temp_directory(), "/", output_directory));
}

void CommandLineInterfaceTest::ExpectGeneratedWithMultipleInputs(
    const std::string& generator_name, const std::string& all_proto_names,
    const std::string& proto_name, const std::string& message_name) {
  MockCodeGenerator::ExpectGenerated(generator_name, "", "", proto_name,
                                     message_name, all_proto_names,
                                     temp_directory());
}

void CommandLineInterfaceTest::ExpectGeneratedWithInsertions(
    const std::string& generator_name, const std::string& parameter,
    const std::string& insertions, const std::string& proto_name,
    const std::string& message_name) {
  MockCodeGenerator::ExpectGenerated(generator_name, parameter, insertions,
                                     proto_name, message_name, proto_name,
                                     temp_directory());
}

void CommandLineInterfaceTest::CheckGeneratedAnnotations(
    const std::string& name, const std::string& file) {
  MockCodeGenerator::CheckGeneratedAnnotations(name, file, temp_directory());
}

#if defined(_WIN32)
void CommandLineInterfaceTest::ExpectNullCodeGeneratorCalled(
    const std::string& parameter) {
  EXPECT_TRUE(null_generator_->called_);
  EXPECT_EQ(parameter, null_generator_->parameter_);
}
#endif  // _WIN32


std::string CommandLineInterfaceTest::ReadFile(absl::string_view filename) {
  std::string path = absl::StrCat(temp_directory(), "/", filename);
  std::string file_contents;
  ABSL_CHECK_OK(File::GetContents(path, &file_contents, true));
  return file_contents;
}

void CommandLineInterfaceTest::ReadDescriptorSet(
    absl::string_view filename, FileDescriptorSet* descriptor_set) {
  std::string file_contents = ReadFile(filename);
  if (!descriptor_set->ParseFromString(file_contents)) {
    FAIL() << "Could not parse file contents: " << filename;
  }
}

FeatureSetDefaults CommandLineInterfaceTest::ReadEditionDefaults(
    absl::string_view filename) {
  FeatureSetDefaults defaults;
  std::string file_contents = ReadFile(filename);
  ABSL_CHECK(defaults.ParseFromString(file_contents))
      << "Could not parse file contents: " << filename;
  return defaults;
}

void CommandLineInterfaceTest::WriteDescriptorSet(
    absl::string_view filename, const FileDescriptorSet* descriptor_set) {
  std::string binary_proto;
  ABSL_CHECK(descriptor_set->SerializeToString(&binary_proto));
  CreateTempFile(filename, binary_proto);
}

// ===================================================================

TEST_F(CommandLineInterfaceTest, BasicOutput) {
  // Test that the common case works.

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator", "", "foo.proto", "Foo");
}

TEST_F(CommandLineInterfaceTest, BasicOutput_DescriptorSetIn) {
  // Test that the common case works.
  FileDescriptorSet file_descriptor_set;
  FileDescriptorProto* file_descriptor_proto = file_descriptor_set.add_file();
  file_descriptor_proto->set_name("foo.proto");
  file_descriptor_proto->add_message_type()->set_name("Foo");

  WriteDescriptorSet("foo.bin", &file_descriptor_set);

  Run("protocol_compiler --test_out=$tmpdir "
      "--descriptor_set_in=$tmpdir/foo.bin foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator", "", "foo.proto", "Foo");
}

TEST_F(CommandLineInterfaceTest, BasicPlugin) {
  // Test that basic plugins work.

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");

  Run("protocol_compiler --plug_out=$tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_plugin", "", "foo.proto", "Foo");
}

TEST_F(CommandLineInterfaceTest, BasicPlugin_DescriptorSetIn) {
  // Test that basic plugins work.

  FileDescriptorSet file_descriptor_set;
  FileDescriptorProto* file_descriptor_proto = file_descriptor_set.add_file();
  file_descriptor_proto->set_name("foo.proto");
  file_descriptor_proto->add_message_type()->set_name("Foo");

  WriteDescriptorSet("foo.bin", &file_descriptor_set);

  Run("protocol_compiler --plug_out=$tmpdir "
      "--descriptor_set_in=$tmpdir/foo.bin foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_plugin", "", "foo.proto", "Foo");
}

TEST_F(CommandLineInterfaceTest, Plugin_OptionRetention) {
  CreateTempFile("foo.proto",
                 R"pb(syntax = "proto2"
                      ;
                      import "bar.proto";
                      package foo;
                      message Foo {
                        optional bar.Bar b = 1;
                        extensions 1000 to max [
                          declaration = {
                            number: 1000
                            full_name: ".foo.my_ext"
                            type: ".foo.MyType"
                          }
                        ];
                      })pb");
  CreateTempFile("bar.proto",
                 R"pb(syntax = "proto2"
                      ;
                      package bar;
                      message Bar {
                        extensions 1000 to max [
                          declaration = {
                            number: 1000
                            full_name: ".baz.my_ext"
                            type: ".baz.MyType"
                          }
                        ];
                      })pb");

#ifdef GOOGLE_PROTOBUF_FAKE_PLUGIN_PATH
  std::string plugin_path = GOOGLE_PROTOBUF_FAKE_PLUGIN_PATH;
#else
  std::string plugin_path = absl::StrCat(
      TestUtil::TestSourceDir(), "/google/protobuf/compiler/fake_plugin");
#endif

  // Invoke protoc with fake_plugin to get ahold of the CodeGeneratorRequest
  // sent by protoc.
  Run(absl::StrCat(
      "protocol_compiler --fake_plugin_out=$tmpdir --proto_path=$tmpdir "
      "foo.proto --plugin=prefix-gen-fake_plugin=",
      plugin_path));
  ExpectNoErrors();
  std::string base64_output = ReadFile("foo.proto.request");
  std::string binary_request;
  ASSERT_TRUE(absl::Base64Unescape(base64_output, &binary_request));
  CodeGeneratorRequest request;
  ASSERT_TRUE(request.ParseFromString(binary_request));

  // request.proto_file() should include source-retention options for bar.proto
  // but not for foo.proto. Protoc should strip source-retention options from
  // the immediate proto files being built, but not for all dependencies.
  ASSERT_EQ(request.proto_file_size(), 2);
  {
    EXPECT_EQ(request.proto_file(0).name(), "bar.proto");
    ASSERT_EQ(request.proto_file(0).message_type_size(), 1);
    const DescriptorProto& m = request.proto_file(0).message_type(0);
    ASSERT_EQ(m.extension_range_size(), 1);
    EXPECT_EQ(m.extension_range(0).options().declaration_size(), 1);
  }

  {
    EXPECT_EQ(request.proto_file(1).name(), "foo.proto");
    ASSERT_EQ(request.proto_file(1).message_type_size(), 1);
    const DescriptorProto& m = request.proto_file(1).message_type(0);
    ASSERT_EQ(m.extension_range_size(), 1);
    EXPECT_TRUE(m.extension_range(0).options().declaration().empty());
  }
}

TEST_F(CommandLineInterfaceTest, Plugin_SourceFileDescriptors) {
  CreateTempFile("foo.proto",
                 R"pb(syntax = "proto2"
                      ;
                      import "bar.proto";
                      package foo;
                      message Foo {
                        optional bar.Bar b = 1;
                        extensions 1000 to max [
                          declaration = {
                            number: 1000
                            full_name: ".foo.my_ext"
                            type: ".foo.MyType"
                          }
                        ];
                      })pb");
  CreateTempFile("bar.proto",
                 R"pb(syntax = "proto2"
                      ;
                      package bar;
                      message Bar {
                        extensions 1000 to max [
                          declaration = {
                            number: 1000
                            full_name: ".baz.my_ext"
                            type: ".baz.MyType"
                          }
                        ];
                      })pb");

#ifdef GOOGLE_PROTOBUF_FAKE_PLUGIN_PATH
  std::string plugin_path = GOOGLE_PROTOBUF_FAKE_PLUGIN_PATH;
#else
  std::string plugin_path = absl::StrCat(
      TestUtil::TestSourceDir(), "/google/protobuf/compiler/fake_plugin");
#endif

  // Invoke protoc with fake_plugin to get ahold of the CodeGeneratorRequest
  // sent by protoc.
  Run(absl::StrCat(
      "protocol_compiler --fake_plugin_out=$tmpdir --proto_path=$tmpdir "
      "foo.proto --plugin=prefix-gen-fake_plugin=",
      plugin_path));
  ExpectNoErrors();
  std::string base64_output = ReadFile("foo.proto.request");
  std::string binary_request;
  ASSERT_TRUE(absl::Base64Unescape(base64_output, &binary_request));
  CodeGeneratorRequest request;
  ASSERT_TRUE(request.ParseFromString(binary_request));

  // request.source_file_descriptors() should consist of a descriptor for
  // foo.proto that includes source-retention options.
  ASSERT_EQ(request.source_file_descriptors_size(), 1);
  EXPECT_EQ(request.source_file_descriptors(0).name(), "foo.proto");
  ASSERT_EQ(request.source_file_descriptors(0).message_type_size(), 1);
  const DescriptorProto& m = request.source_file_descriptors(0).message_type(0);
  ASSERT_EQ(m.extension_range_size(), 1);
  EXPECT_EQ(m.extension_range(0).options().declaration_size(), 1);
}

TEST_F(CommandLineInterfaceTest, GeneratorAndPlugin) {
  // Invoke a generator and a plugin at the same time.

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");

  Run("protocol_compiler --test_out=$tmpdir --plug_out=$tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator", "", "foo.proto", "Foo");
  ExpectGenerated("test_plugin", "", "foo.proto", "Foo");
}

TEST_F(CommandLineInterfaceTest, GeneratorAndPlugin_DescriptorSetIn) {
  // Invoke a generator and a plugin at the same time.

  FileDescriptorSet file_descriptor_set;
  FileDescriptorProto* file_descriptor_proto = file_descriptor_set.add_file();
  file_descriptor_proto->set_name("foo.proto");
  file_descriptor_proto->add_message_type()->set_name("Foo");

  WriteDescriptorSet("foo.bin", &file_descriptor_set);

  Run("protocol_compiler --test_out=$tmpdir --plug_out=$tmpdir "
      "--descriptor_set_in=$tmpdir/foo.bin foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator", "", "foo.proto", "Foo");
  ExpectGenerated("test_plugin", "", "foo.proto", "Foo");
}

TEST_F(CommandLineInterfaceTest, MultipleInputs) {
  // Test parsing multiple input files.

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");
  CreateTempFile("bar.proto",
                 "syntax = \"proto2\";\n"
                 "message Bar {}\n");

  Run("protocol_compiler --test_out=$tmpdir --plug_out=$tmpdir "
      "--proto_path=$tmpdir foo.proto bar.proto");

  ExpectNoErrors();
  ExpectGeneratedWithMultipleInputs("test_generator", "foo.proto,bar.proto",
                                    "foo.proto", "Foo");
  ExpectGeneratedWithMultipleInputs("test_generator", "foo.proto,bar.proto",
                                    "bar.proto", "Bar");
  ExpectGeneratedWithMultipleInputs("test_plugin", "foo.proto,bar.proto",
                                    "foo.proto", "Foo");
  ExpectGeneratedWithMultipleInputs("test_plugin", "foo.proto,bar.proto",
                                    "bar.proto", "Bar");
}

TEST_F(CommandLineInterfaceTest, MultipleInputs_DescriptorSetIn) {
  // Test parsing multiple input files.
  FileDescriptorSet file_descriptor_set;

  FileDescriptorProto* file_descriptor_proto = file_descriptor_set.add_file();
  file_descriptor_proto->set_name("foo.proto");
  file_descriptor_proto->add_message_type()->set_name("Foo");

  file_descriptor_proto = file_descriptor_set.add_file();
  file_descriptor_proto->set_name("bar.proto");
  file_descriptor_proto->add_message_type()->set_name("Bar");

  WriteDescriptorSet("foo.bin", &file_descriptor_set);

  Run("protocol_compiler --test_out=$tmpdir --plug_out=$tmpdir "
      "--descriptor_set_in=$tmpdir/foo.bin foo.proto bar.proto");

  ExpectNoErrors();
  ExpectGeneratedWithMultipleInputs("test_generator", "foo.proto,bar.proto",
                                    "foo.proto", "Foo");
  ExpectGeneratedWithMultipleInputs("test_generator", "foo.proto,bar.proto",
                                    "bar.proto", "Bar");
  ExpectGeneratedWithMultipleInputs("test_plugin", "foo.proto,bar.proto",
                                    "foo.proto", "Foo");
  ExpectGeneratedWithMultipleInputs("test_plugin", "foo.proto,bar.proto",
                                    "bar.proto", "Bar");
}

TEST_F(CommandLineInterfaceTest, MultipleInputs_UnusedImport_DescriptorSetIn) {
  // Test unused import warning is not raised when descriptor_set_in is called
  // and custom options are in unknown field instead of uninterpreted_options.
  FileDescriptorSet file_descriptor_set;

  const FileDescriptor* descriptor_file =
      FileDescriptorProto::descriptor()->file();
  descriptor_file->CopyTo(file_descriptor_set.add_file());

  FileDescriptorProto& any_proto = *file_descriptor_set.add_file();
  google::protobuf::Any::descriptor()->file()->CopyTo(&any_proto);

  const FileDescriptor* custom_file =
      protobuf_unittest::AggregateMessage::descriptor()->file();
  FileDescriptorProto* file_descriptor_proto = file_descriptor_set.add_file();
  custom_file->CopyTo(file_descriptor_proto);
  file_descriptor_proto->set_name("custom_options.proto");
  // Add a custom message option.
  FieldDescriptorProto* extension_option =
      file_descriptor_proto->add_extension();
  extension_option->set_name("unknown_option");
  extension_option->set_extendee(".google.protobuf.MessageOptions");
  extension_option->set_number(1111);
  extension_option->set_label(FieldDescriptorProto::LABEL_OPTIONAL);
  extension_option->set_type(FieldDescriptorProto::TYPE_INT64);

  file_descriptor_proto = file_descriptor_set.add_file();
  file_descriptor_proto->set_name("import_custom_unknown_options.proto");
  file_descriptor_proto->add_dependency("custom_options.proto");
  // Add custom message option to unknown field. This custom option is
  // not known in generated pool, thus option will be in unknown fields.
  file_descriptor_proto->add_message_type()->set_name("Bar");
  file_descriptor_proto->mutable_message_type(0)
      ->mutable_options()
      ->mutable_unknown_fields()
      ->AddVarint(1111, 2222);

  WriteDescriptorSet("foo.bin", &file_descriptor_set);

  Run("protocol_compiler --test_out=$tmpdir --plug_out=$tmpdir "
      "--descriptor_set_in=$tmpdir/foo.bin "
      "import_custom_unknown_options.proto");

  // TODO: Fix this test. This test case only happens when
  // CommandLineInterface::Run() is used instead of invoke protoc combined
  // with descriptor_set_in, and same custom options are defined in both
  // generated pool and descriptor_set_in. There's no such uages for now but
  // still need to be fixed.
  /*
  file_descriptor_proto = file_descriptor_set.add_file();
  file_descriptor_proto->set_name("import_custom_extension_options.proto");
  file_descriptor_proto->add_dependency("custom_options.proto");
  // Add custom message option to unknown field. This custom option is
  // also defined in generated pool, thus option will be in extensions.
  file_descriptor_proto->add_message_type()->set_name("Foo");
  file_descriptor_proto->mutable_message_type(0)
      ->mutable_options()
      ->mutable_unknown_fields()
      ->AddVarint(protobuf_unittest::message_opt1.number(), 2222);

  WriteDescriptorSet("foo.bin", &file_descriptor_set);

  Run("protocol_compiler --test_out=$tmpdir --plug_out=$tmpdir "
      "--descriptor_set_in=$tmpdir/foo.bin import_custom_unknown_options.proto "
      "import_custom_extension_options.proto");
  */

  ExpectNoErrors();
}

TEST_F(CommandLineInterfaceTest, MultipleInputsWithImport) {
  // Test parsing multiple input files with an import of a separate file.

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");
  CreateTempFile("bar.proto",
                 "syntax = \"proto2\";\n"
                 "import \"baz.proto\";\n"
                 "message Bar {\n"
                 "  optional Baz a = 1;\n"
                 "}\n");
  CreateTempFile("baz.proto",
                 "syntax = \"proto2\";\n"
                 "message Baz {}\n");

  Run("protocol_compiler --test_out=$tmpdir --plug_out=$tmpdir "
      "--proto_path=$tmpdir foo.proto bar.proto");

  ExpectNoErrors();
  ExpectGeneratedWithMultipleInputs("test_generator", "foo.proto,bar.proto",
                                    "foo.proto", "Foo");
  ExpectGeneratedWithMultipleInputs("test_generator", "foo.proto,bar.proto",
                                    "bar.proto", "Bar");
  ExpectGeneratedWithMultipleInputs("test_plugin", "foo.proto,bar.proto",
                                    "foo.proto", "Foo");
  ExpectGeneratedWithMultipleInputs("test_plugin", "foo.proto,bar.proto",
                                    "bar.proto", "Bar");
}


TEST_F(CommandLineInterfaceTest, MultipleInputsWithImport_DescriptorSetIn) {
  // Test parsing multiple input files with an import of a separate file.
  FileDescriptorSet file_descriptor_set;

  FileDescriptorProto* file_descriptor_proto = file_descriptor_set.add_file();
  file_descriptor_proto->set_name("foo.proto");
  file_descriptor_proto->add_message_type()->set_name("Foo");

  file_descriptor_proto = file_descriptor_set.add_file();
  file_descriptor_proto->set_name("bar.proto");
  file_descriptor_proto->add_dependency("baz.proto");
  DescriptorProto* message = file_descriptor_proto->add_message_type();
  message->set_name("Bar");
  FieldDescriptorProto* field = message->add_field();
  field->set_type_name("Baz");
  field->set_name("a");
  field->set_number(1);

  WriteDescriptorSet("foo_and_bar.bin", &file_descriptor_set);

  file_descriptor_set.clear_file();
  file_descriptor_proto = file_descriptor_set.add_file();
  file_descriptor_proto->set_name("baz.proto");
  file_descriptor_proto->add_message_type()->set_name("Baz");

  file_descriptor_proto = file_descriptor_set.add_file();
  file_descriptor_proto->set_name("bat.proto");
  file_descriptor_proto->add_dependency("baz.proto");
  message = file_descriptor_proto->add_message_type();
  message->set_name("Bat");
  field = message->add_field();
  field->set_type_name("Baz");
  field->set_name("a");
  field->set_number(1);

  WriteDescriptorSet("baz_and_bat.bin", &file_descriptor_set);
  Run(absl::Substitute(
      "protocol_compiler --test_out=$$tmpdir --plug_out=$$tmpdir "
      "--descriptor_set_in=$0 foo.proto bar.proto",
      absl::StrCat("$tmpdir/foo_and_bar.bin",
                   CommandLineInterface::kPathSeparator,
                   "$tmpdir/baz_and_bat.bin")));

  ExpectNoErrors();
  ExpectGeneratedWithMultipleInputs("test_generator", "foo.proto,bar.proto",
                                    "foo.proto", "Foo");
  ExpectGeneratedWithMultipleInputs("test_generator", "foo.proto,bar.proto",
                                    "bar.proto", "Bar");
  ExpectGeneratedWithMultipleInputs("test_plugin", "foo.proto,bar.proto",
                                    "foo.proto", "Foo");
  ExpectGeneratedWithMultipleInputs("test_plugin", "foo.proto,bar.proto",
                                    "bar.proto", "Bar");

  Run(absl::Substitute(
      "protocol_compiler --test_out=$$tmpdir --plug_out=$$tmpdir "
      "--descriptor_set_in=$0 baz.proto bat.proto",
      absl::StrCat("$tmpdir/foo_and_bar.bin",
                   CommandLineInterface::kPathSeparator,
                   "$tmpdir/baz_and_bat.bin")));

  ExpectNoErrors();
  ExpectGeneratedWithMultipleInputs("test_generator", "baz.proto,bat.proto",
                                    "baz.proto", "Baz");
  ExpectGeneratedWithMultipleInputs("test_generator", "baz.proto,bat.proto",
                                    "bat.proto", "Bat");
  ExpectGeneratedWithMultipleInputs("test_plugin", "baz.proto,bat.proto",
                                    "baz.proto", "Baz");
  ExpectGeneratedWithMultipleInputs("test_plugin", "baz.proto,bat.proto",
                                    "bat.proto", "Bat");
}

TEST_F(CommandLineInterfaceTest,
       MultipleInputsWithImport_DescriptorSetIn_DuplicateFileDescriptor) {
  // Test parsing multiple input files with an import of a separate file.
  FileDescriptorSet file_descriptor_set;

  FileDescriptorProto foo_file_descriptor_proto;
  foo_file_descriptor_proto.set_name("foo.proto");
  foo_file_descriptor_proto.add_message_type()->set_name("Foo");

  *file_descriptor_set.add_file() = foo_file_descriptor_proto;

  FileDescriptorProto* file_descriptor_proto = file_descriptor_set.add_file();
  file_descriptor_proto->set_name("bar.proto");
  file_descriptor_proto->add_dependency("baz.proto");
  file_descriptor_proto->add_dependency("foo.proto");
  DescriptorProto* message = file_descriptor_proto->add_message_type();
  message->set_name("Bar");
  FieldDescriptorProto* field = message->add_field();
  field->set_type_name("Baz");
  field->set_name("a");
  field->set_number(1);
  field = message->add_field();
  field->set_type_name("Foo");
  field->set_name("f");
  field->set_number(2);
  WriteDescriptorSet("foo_and_bar.bin", &file_descriptor_set);

  file_descriptor_set.clear_file();
  *file_descriptor_set.add_file() = foo_file_descriptor_proto;

  file_descriptor_proto = file_descriptor_set.add_file();
  file_descriptor_proto->set_name("baz.proto");
  file_descriptor_proto->add_dependency("foo.proto");
  message = file_descriptor_proto->add_message_type();
  message->set_name("Baz");
  field = message->add_field();
  field->set_type_name("Foo");
  field->set_name("f");
  field->set_number(1);
  WriteDescriptorSet("foo_and_baz.bin", &file_descriptor_set);

  Run(absl::Substitute(
      "protocol_compiler --test_out=$$tmpdir --plug_out=$$tmpdir "
      "--descriptor_set_in=$0 bar.proto",
      absl::StrCat("$tmpdir/foo_and_bar.bin",
                   CommandLineInterface::kPathSeparator,
                   "$tmpdir/foo_and_baz.bin")));

  ExpectNoErrors();
  ExpectGenerated("test_generator", "", "bar.proto", "Bar");
  ExpectGenerated("test_plugin", "", "bar.proto", "Bar");
}

TEST_F(CommandLineInterfaceTest,
       MultipleInputsWithImport_DescriptorSetIn_MissingImport) {
  // Test parsing multiple input files with an import of a separate file.
  FileDescriptorSet file_descriptor_set;

  FileDescriptorProto* file_descriptor_proto = file_descriptor_set.add_file();
  file_descriptor_proto->set_name("foo.proto");
  file_descriptor_proto->add_message_type()->set_name("Foo");

  file_descriptor_proto = file_descriptor_set.add_file();
  file_descriptor_proto->set_name("bar.proto");
  file_descriptor_proto->add_dependency("baz.proto");
  DescriptorProto* message = file_descriptor_proto->add_message_type();
  message->set_name("Bar");
  FieldDescriptorProto* field = message->add_field();
  field->set_type_name("Baz");
  field->set_name("a");
  field->set_number(1);

  WriteDescriptorSet("foo_and_bar.bin", &file_descriptor_set);

  file_descriptor_set.clear_file();
  file_descriptor_proto = file_descriptor_set.add_file();
  file_descriptor_proto->set_name("baz.proto");
  file_descriptor_proto->add_message_type()->set_name("Baz");

  WriteDescriptorSet("baz.bin", &file_descriptor_set);
  Run("protocol_compiler --test_out=$tmpdir --plug_out=$tmpdir "
      "--descriptor_set_in=$tmpdir/foo_and_bar.bin "
      "foo.proto bar.proto");
  ExpectErrorSubstring(
      "bar.proto: Import \"baz.proto\" was not found or had errors.");
  ExpectErrorSubstring("bar.proto: \"Baz\" is not defined.");
}

TEST_F(CommandLineInterfaceTest,
       InputsOnlyFromDescriptorSetIn_UnusedImportIsNotReported) {
  FileDescriptorSet file_descriptor_set;

  FileDescriptorProto* file_descriptor_proto = file_descriptor_set.add_file();
  file_descriptor_proto->set_name("unused.proto");
  file_descriptor_proto->add_message_type()->set_name("Unused");

  file_descriptor_proto = file_descriptor_set.add_file();
  file_descriptor_proto->set_name("bar.proto");
  file_descriptor_proto->add_dependency("unused.proto");
  file_descriptor_proto->add_message_type()->set_name("Bar");

  WriteDescriptorSet("unused_and_bar.bin", &file_descriptor_set);

  Run("protocol_compiler --test_out=$tmpdir --plug_out=$tmpdir "
      "--descriptor_set_in=$tmpdir/unused_and_bar.bin unused.proto bar.proto");
  ExpectNoErrors();
}

TEST_F(CommandLineInterfaceTest,
       InputsFromDescriptorSetInAndFileSystem_UnusedImportIsReported) {
  FileDescriptorSet file_descriptor_set;

  FileDescriptorProto* file_descriptor_proto = file_descriptor_set.add_file();
  file_descriptor_proto->set_name("unused.proto");
  file_descriptor_proto->add_message_type()->set_name("Unused");

  file_descriptor_proto = file_descriptor_set.add_file();
  file_descriptor_proto->set_name("bar.proto");
  file_descriptor_proto->add_dependency("unused.proto");
  file_descriptor_proto->add_message_type()->set_name("Bar");

  WriteDescriptorSet("unused_and_bar.bin", &file_descriptor_set);

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "import \"bar.proto\";\n"
                 "message Foo {\n"
                 "  optional Bar bar = 1;\n"
                 "}\n");

  Run("protocol_compiler --test_out=$tmpdir --plug_out=$tmpdir "
      "--descriptor_set_in=$tmpdir/unused_and_bar.bin "
      "--proto_path=$tmpdir unused.proto bar.proto foo.proto");
  // Reporting unused imports here is unfair, since it's unactionable. Notice
  // the lack of a line number.
  // TODO: If the file with unused import is from the descriptor
  // set and not from the file system, suppress the warning.
  ExpectWarningSubstring("bar.proto: warning: Import unused.proto is unused.");
}

TEST_F(CommandLineInterfaceTest,
       OnlyReportsUnusedImportsForFilesBeingGenerated) {
  CreateTempFile("unused.proto",
                 "syntax = \"proto2\";\n"
                 "message Unused {}\n");
  CreateTempFile("bar.proto",
                 "syntax = \"proto2\";\n"
                 "import \"unused.proto\";\n"
                 "message Bar {}\n");
  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "import \"bar.proto\";\n"
                 "message Foo {\n"
                 "  optional Bar bar = 1;\n"
                 "}\n");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir foo.proto");
  ExpectNoErrors();
}

TEST_F(CommandLineInterfaceTest, ReportsTransitiveMisingImports_LeafFirst) {
  CreateTempFile("unused.proto",
                 "syntax = \"proto2\";\n"
                 "message Unused {}\n");
  CreateTempFile("bar.proto",
                 "syntax = \"proto2\";\n"
                 "import \"unused.proto\";\n"
                 "message Bar {}\n");
  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "import \"bar.proto\";\n"
                 "message Foo {\n"
                 "  optional Bar bar = 1;\n"
                 "}\n");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir bar.proto foo.proto");
  ExpectWarningSubstring(
      "bar.proto:2:1: warning: Import unused.proto is unused.");
}

TEST_F(CommandLineInterfaceTest, ReportsTransitiveMisingImports_LeafLast) {
  CreateTempFile("unused.proto",
                 "syntax = \"proto2\";\n"
                 "message Unused {}\n");
  CreateTempFile("bar.proto",
                 "syntax = \"proto2\";\n"
                 "import \"unused.proto\";\n"
                 "message Bar {}\n");
  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "import \"bar.proto\";\n"
                 "message Foo {\n"
                 "  optional Bar bar = 1;\n"
                 "}\n");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir foo.proto bar.proto");
  ExpectWarningSubstring(
      "bar.proto:2:1: warning: Import unused.proto is unused.");
}
TEST_F(CommandLineInterfaceTest, CreateDirectory) {
  // Test that when we output to a sub-directory, it is created.

  CreateTempFile("bar/baz/foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");
  CreateTempDir("out");
  CreateTempDir("plugout");

  Run("protocol_compiler --test_out=$tmpdir/out --plug_out=$tmpdir/plugout "
      "--proto_path=$tmpdir bar/baz/foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator", "", "bar/baz/foo.proto", "Foo", "out");
  ExpectGenerated("test_plugin", "", "bar/baz/foo.proto", "Foo", "plugout");
}

TEST_F(CommandLineInterfaceTest, GeneratorParameters) {
  // Test that generator parameters are correctly parsed from the command line.

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");

  Run("protocol_compiler --test_out=TestParameter:$tmpdir "
      "--plug_out=TestPluginParameter:$tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator", "TestParameter", "foo.proto", "Foo");
  ExpectGenerated("test_plugin", "TestPluginParameter", "foo.proto", "Foo");
}

TEST_F(CommandLineInterfaceTest, ExtraGeneratorParameters) {
  // Test that generator parameters specified with the option flag are
  // correctly passed to the code generator.

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");
  // Create the "a" and "b" sub-directories.
  CreateTempDir("a");
  CreateTempDir("b");

  Run("protocol_compiler "
      "--test_opt=foo1 "
      "--test_out=bar:$tmpdir/a "
      "--test_opt=foo2 "
      "--test_out=baz:$tmpdir/b "
      "--test_opt=foo3 "
      "--proto_path=$tmpdir foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator", "bar,foo1,foo2,foo3", "foo.proto", "Foo",
                  "a");
  ExpectGenerated("test_generator", "baz,foo1,foo2,foo3", "foo.proto", "Foo",
                  "b");
}

TEST_F(CommandLineInterfaceTest, ExtraPluginParameters) {
  // Test that generator parameters specified with the option flag are
  // correctly passed to the code generator.

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");
  // Create the "a" and "b" sub-directories.
  CreateTempDir("a");
  CreateTempDir("b");

  Run("protocol_compiler "
      "--plug_opt=foo1 "
      "--plug_out=bar:$tmpdir/a "
      "--plug_opt=foo2 "
      "--plug_out=baz:$tmpdir/b "
      "--plug_opt=foo3 "
      "--proto_path=$tmpdir foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_plugin", "bar,foo1,foo2,foo3", "foo.proto", "Foo", "a");
  ExpectGenerated("test_plugin", "baz,foo1,foo2,foo3", "foo.proto", "Foo", "b");
}

TEST_F(CommandLineInterfaceTest, UnrecognizedExtraParameters) {
  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");

  Run("protocol_compiler --plug_out=TestParameter:$tmpdir "
      "--unknown_plug_a_opt=Foo "
      "--unknown_plug_b_opt=Bar "
      "--proto_path=$tmpdir foo.proto");

  ExpectErrorSubstring("Unknown flag: --unknown_plug_a_opt");
  ExpectErrorSubstring("Unknown flag: --unknown_plug_b_opt");
}

TEST_F(CommandLineInterfaceTest, ExtraPluginParametersForOutParameters) {
  // This doesn't rely on the plugin having been registered and instead that
  // the existence of --[name]_out is enough to make the --[name]_opt valid.
  // However, running out of process plugins found via the search path (i.e. -
  // not pre registered with --plugin) isn't support in this test suite, so we
  // list the options pre/post the _out directive, and then include _opt that
  // will be unknown, and confirm the failure output is about the expected
  // unknown directive, which means the other were accepted.
  // NOTE: UnrecognizedExtraParameters confirms that if two unknown _opt
  // directives appear, they both are reported.

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");

  Run("protocol_compiler --plug_out=TestParameter:$tmpdir "
      "--xyz_opt=foo=bar --xyz_out=$tmpdir "
      "--abc_out=$tmpdir --abc_opt=foo=bar "
      "--unknown_plug_opt=Foo "
      "--proto_path=$tmpdir foo.proto");

  ExpectErrorText("Unknown flag: --unknown_plug_opt\n");
}

TEST_F(CommandLineInterfaceTest, Insert) {
  // Test running a generator that inserts code into another's output.

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");

  Run("protocol_compiler "
      "--test_out=TestParameter:$tmpdir "
      "--plug_out=TestPluginParameter:$tmpdir "
      "--test_out=insert=test_generator,test_plugin:$tmpdir "
      "--plug_out=insert=test_generator,test_plugin:$tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectNoErrors();
  ExpectGeneratedWithInsertions("test_generator", "TestParameter",
                                "test_generator,test_plugin", "foo.proto",
                                "Foo");
  ExpectGeneratedWithInsertions("test_plugin", "TestPluginParameter",
                                "test_generator,test_plugin", "foo.proto",
                                "Foo");
}

TEST_F(CommandLineInterfaceTest, InsertWithAnnotationFixup) {
  // Check that annotation spans are updated after insertions.

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message MockCodeGenerator_Annotate {}\n");

  Run("protocol_compiler "
      "--test_out=TestParameter:$tmpdir "
      "--plug_out=TestPluginParameter:$tmpdir "
      "--test_out=insert_endlines=test_generator,test_plugin:$tmpdir "
      "--plug_out=insert_endlines=test_generator,test_plugin:$tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectNoErrors();
  CheckGeneratedAnnotations("test_generator", "foo.proto");
  CheckGeneratedAnnotations("test_plugin", "foo.proto");
}

#if defined(_WIN32)

TEST_F(CommandLineInterfaceTest, WindowsOutputPath) {
  // Test that the output path can be a Windows-style path.

  CreateTempFile("foo.proto", "syntax = \"proto2\";\n");

  Run("protocol_compiler --null_out=C:\\ "
      "--proto_path=$tmpdir foo.proto");

  ExpectNoErrors();
  ExpectNullCodeGeneratorCalled("");
}

TEST_F(CommandLineInterfaceTest, WindowsOutputPathAndParameter) {
  // Test that we can have a windows-style output path and a parameter.

  CreateTempFile("foo.proto", "syntax = \"proto2\";\n");

  Run("protocol_compiler --null_out=bar:C:\\ "
      "--proto_path=$tmpdir foo.proto");

  ExpectNoErrors();
  ExpectNullCodeGeneratorCalled("bar");
}

TEST_F(CommandLineInterfaceTest, TrailingBackslash) {
  // Test that the directories can end in backslashes.  Some users claim this
  // doesn't work on their system.

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");

  Run("protocol_compiler --test_out=$tmpdir\\ "
      "--proto_path=$tmpdir\\ foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator", "", "foo.proto", "Foo");
}

TEST_F(CommandLineInterfaceTest, Win32ErrorMessage) {
  EXPECT_EQ("The system cannot find the file specified.\r\n",
            Subprocess::Win32ErrorMessage(ERROR_FILE_NOT_FOUND));
}

#endif  // defined(_WIN32) || defined(__CYGWIN__)

TEST_F(CommandLineInterfaceTest, PathLookup) {
  // Test that specifying multiple directories in the proto search path works.

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
  ExpectGenerated("test_generator", "", "foo.proto", "Foo");
}

TEST_F(CommandLineInterfaceTest, ColonDelimitedPath) {
  // Same as PathLookup, but we provide the proto_path in a single flag.

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

  Run(absl::Substitute(
      "protocol_compiler --test_out=$$tmpdir --proto_path=$0 foo.proto",
      std::string("$tmpdir/a") + CommandLineInterface::kPathSeparator +
          "$tmpdir/b"));

  ExpectNoErrors();
  ExpectGenerated("test_generator", "", "foo.proto", "Foo");
}

TEST_F(CommandLineInterfaceTest, NonRootMapping) {
  // Test setting up a search path mapping a directory to a non-root location.

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=bar=$tmpdir bar/foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator", "", "bar/foo.proto", "Foo");
}

TEST_F(CommandLineInterfaceTest, PathWithEqualsSign) {
  // Test setting up a search path which happens to have '=' in it.

  CreateTempDir("with=sign");
  CreateTempFile("with=sign/foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir/with=sign foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator", "", "foo.proto", "Foo");
}

TEST_F(CommandLineInterfaceTest, MultipleGenerators) {
  // Test that we can have multiple generators and use both in one invocation,
  // each with a different output directory.

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");
  // Create the "a" and "b" sub-directories.
  CreateTempDir("a");
  CreateTempDir("b");

  Run("protocol_compiler "
      "--test_out=$tmpdir/a "
      "--alt_out=$tmpdir/b "
      "--proto_path=$tmpdir foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator", "", "foo.proto", "Foo", "a");
  ExpectGenerated("alt_generator", "", "foo.proto", "Foo", "b");
}

TEST_F(CommandLineInterfaceTest, DisallowServicesNoServices) {
  // Test that --disallow_services doesn't cause a problem when there are no
  // services.

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");

  Run("protocol_compiler --disallow_services --test_out=$tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator", "", "foo.proto", "Foo");
}

TEST_F(CommandLineInterfaceTest, DisallowServicesHasService) {
  // Test that --disallow_services produces an error when there are services.

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

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n"
                 "service Bar {}\n");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator", "", "foo.proto", "Foo");
}

TEST_F(CommandLineInterfaceTest, NonExperimentalEditions) {
  CreateTempFile("foo.proto",
                 "edition = \"2023\";\n"
                 "message FooRequest {}\n");

  Run("protocol_compiler --proto_path=$tmpdir --test_out=$tmpdir foo.proto");

  ExpectNoErrors();
}

TEST_F(CommandLineInterfaceTest, EditionsFlagExplicitTrue) {
  CreateTempFile("foo.proto",
                 "edition = \"2023\";\n"
                 "message FooRequest {}\n");

  Run("protocol_compiler --proto_path=$tmpdir --test_out=$tmpdir foo.proto");
  ExpectNoErrors();
}

TEST_F(CommandLineInterfaceTest, FeaturesEditionZero) {
  CreateTempFile("foo.proto",
                 R"schema(
    edition = "2023";
    option features.field_presence = IMPLICIT;
    message Foo {
      int32 bar = 1 [default = 5, features.field_presence = EXPLICIT];
      int32 baz = 2;
    })schema");

  Run("protocol_compiler --proto_path=$tmpdir --test_out=$tmpdir foo.proto");
  ExpectNoErrors();
}

TEST_F(CommandLineInterfaceTest, FeatureExtensions) {
  CreateTempFile("google/protobuf/descriptor.proto",
                 google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  CreateTempFile("features.proto",
                 R"schema(
    syntax = "proto2";
    package pb;
    import "google/protobuf/descriptor.proto";
    extend google.protobuf.FeatureSet {
      optional TestFeatures test = 9999;
    }
    message TestFeatures {
      optional int32 int_feature = 1 [
        retention = RETENTION_RUNTIME,
        targets = TARGET_TYPE_FIELD,
        edition_defaults = { edition: EDITION_2023, value: "3" }
      ];
    })schema");
  CreateTempFile("foo.proto",
                 R"schema(
    edition = "2023";
    import "features.proto";
    message Foo {
      int32 bar = 1;
      int32 baz = 2 [features.(pb.test).int_feature = 5];
    })schema");

  Run("protocol_compiler --proto_path=$tmpdir --test_out=$tmpdir foo.proto");
  ExpectNoErrors();
}

TEST_F(CommandLineInterfaceTest, FeatureValidationError) {
  CreateTempFile("foo.proto",
                 R"schema(
    edition = "2023";
    option features.field_presence = IMPLICIT;
    message Foo {
      int32 bar = 1 [default = 5, features.field_presence = FIELD_PRESENCE_UNKNOWN];
      int32 baz = 2;
    })schema");

  Run("protocol_compiler --proto_path=$tmpdir --test_out=$tmpdir foo.proto");
  ExpectErrorSubstring(
      "`field_presence` must resolve to a known value, found "
      "FIELD_PRESENCE_UNKNOWN");
}

TEST_F(CommandLineInterfaceTest, FeatureTargetError) {
  CreateTempFile("foo.proto",
                 R"schema(
    edition = "2023";
    message Foo {
      option features.field_presence = IMPLICIT;
      int32 bar = 1 [default = 5, features.field_presence = EXPLICIT];
      int32 baz = 2;
    })schema");

  Run("protocol_compiler --proto_path=$tmpdir --test_out=$tmpdir foo.proto");
  ExpectErrorSubstring(
      "FeatureSet.field_presence cannot be set on an entity of type `message`");
}

TEST_F(CommandLineInterfaceTest, FeatureExtensionError) {
  CreateTempFile("google/protobuf/descriptor.proto",
                 google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  CreateTempFile("features.proto",
                 pb::TestInvalidFeatures::descriptor()->file()->DebugString());
  CreateTempFile("foo.proto",
                 R"schema(
    edition = "2023";
    import "features.proto";
    message Foo {
      int32 bar = 1;
      int32 baz = 2 [features.(pb.test_invalid).repeated_feature = 5];
    })schema");

  mock_generator_->set_feature_extensions(
      {GetExtensionReflection(pb::test_invalid)});

  Run("protocol_compiler --proto_path=$tmpdir --test_out=$tmpdir foo.proto");
  ExpectErrorSubstring(
      "Feature field pb.TestInvalidFeatures.repeated_feature is an unsupported "
      "repeated field");
}

TEST_F(CommandLineInterfaceTest, InvalidMinimumEditionError) {
  CreateTempFile("foo.proto", R"schema(edition = "2023";)schema");

  mock_generator_->set_minimum_edition(EDITION_1_TEST_ONLY);

  Run("protocol_compiler --proto_path=$tmpdir --test_out=$tmpdir foo.proto");
  ExpectErrorSubstring(
      "generator --test_out specifies a minimum edition 1_TEST_ONLY which is "
      "not the protoc minimum PROTO2");
}

TEST_F(CommandLineInterfaceTest, InvalidMaximumEditionError) {
  CreateTempFile("foo.proto", R"schema(edition = "2023";)schema");

  mock_generator_->set_maximum_edition(EDITION_99999_TEST_ONLY);

  Run("protocol_compiler --proto_path=$tmpdir --test_out=$tmpdir foo.proto");
  ExpectErrorSubstring(
      "generator --test_out specifies a maximum edition 99999_TEST_ONLY which "
      "is not the protoc maximum 2023");
}

TEST_F(CommandLineInterfaceTest, InvalidFeatureExtensionError) {
  CreateTempFile("foo.proto", R"schema(edition = "2023";)schema");

  mock_generator_->set_feature_extensions({nullptr});

  Run("protocol_compiler --proto_path=$tmpdir --test_out=$tmpdir foo.proto");
  ExpectErrorSubstring(
      "generator --test_out specifies an unknown feature extension");
}

TEST_F(CommandLineInterfaceTest, Plugin_InvalidFeatureExtensionError) {
  CreateTempFile("foo.proto", R"schema(
    edition = "2023";
    message Foo {
      int32 i = 1;
    }
  )schema");

  SetMockGeneratorTestCase("invalid_features");
  Run("protocol_compiler --proto_path=$tmpdir foo.proto --plug_out=$tmpdir");

  ExpectErrorSubstring(
      "error generating feature defaults: Unknown extension of "
      "google.protobuf.FeatureSet");
}

TEST_F(CommandLineInterfaceTest, Plugin_DeprecatedEdition) {
  CreateTempFile("foo.proto", R"schema(
    edition = "2023";
    message Foo {
      int32 i = 1;
    }
  )schema");

  SetMockGeneratorTestCase("high_minimum");
  Run("protocol_compiler "
      "--proto_path=$tmpdir foo.proto --plug_out=$tmpdir");

  ExpectErrorSubstring(
      "foo.proto: is a file using edition 2023, which isn't supported by code "
      "generator prefix-gen-plug.  Please upgrade your file to at least "
      "edition 99997_TEST_ONLY.");
}

TEST_F(CommandLineInterfaceTest, Plugin_FutureEdition) {
  CreateTempFile("foo.proto", R"schema(
    edition = "2023";
    message Foo {
      int32 i = 1;
    }
  )schema");

  SetMockGeneratorTestCase("low_maximum");
  Run("protocol_compiler "
      "--proto_path=$tmpdir foo.proto --plug_out=$tmpdir");

  ExpectErrorSubstring(
      "foo.proto: is a file using edition 2023, which isn't supported by code "
      "generator prefix-gen-plug.  Please ask the owner of this code generator "
      "to add support or switch back to a maximum of edition PROTO2.");
}

TEST_F(CommandLineInterfaceTest, Plugin_VersionSkewFuture) {
  CreateTempFile("foo.proto", R"schema(
    edition = "99997_TEST_ONLY";
    message Foo {
      int32 i = 1;
    }
  )schema");

  SetMockGeneratorTestCase("high_maximum");
  Run("protocol_compiler "
      "--proto_path=$tmpdir foo.proto --plug_out=$tmpdir");

  ExpectErrorSubstring(
      "foo.proto:2:5: Edition 99997_TEST_ONLY is later than the maximum "
      "supported edition 2023");
}

TEST_F(CommandLineInterfaceTest, Plugin_VersionSkewPast) {
  CreateTempFile("foo.proto", R"schema(
    edition = "1_TEST_ONLY";
    message Foo {
      int32 i = 1;
    }
  )schema");

  SetMockGeneratorTestCase("low_minimum");
  Run("protocol_compiler "
      "--proto_path=$tmpdir foo.proto --plug_out=$tmpdir");

  ExpectErrorSubstring(
      "foo.proto:2:5: Edition 1_TEST_ONLY is earlier than the minimum "
      "supported edition PROTO2");
}

TEST_F(CommandLineInterfaceTest, Plugin_MissingFeatureExtensionError) {
  CreateTempFile("foo.proto", R"schema(
    edition = "2023";
    message Foo {
      int32 i = 1;
    }
  )schema");

  SetMockGeneratorTestCase("no_feature_defaults");
  Run("protocol_compiler --proto_path=$tmpdir foo.proto --plug_out=$tmpdir");

  ExpectErrorSubstring("Test features were not resolved properly");
}

TEST_F(CommandLineInterfaceTest, Plugin_TestFeatures) {
  CreateTempFile("foo.proto", R"schema(
    edition = "2023";
    message Foo {
      int32 i = 1;
    }
  )schema");

  Run("protocol_compiler --proto_path=$tmpdir foo.proto --plug_out=$tmpdir");

  ExpectNoErrors();
}

TEST_F(CommandLineInterfaceTest, Plugin_LegacyFeatures) {
  CreateTempFile("foo.proto",
                 R"schema(
                      syntax = "proto2";
                      package foo;
                      message Foo {
                        optional int32 b = 1;
                      })schema");

#ifdef GOOGLE_PROTOBUF_FAKE_PLUGIN_PATH
  std::string plugin_path = GOOGLE_PROTOBUF_FAKE_PLUGIN_PATH;
#else
  std::string plugin_path = absl::StrCat(
      TestUtil::TestSourceDir(), "/google/protobuf/compiler/fake_plugin");
#endif

  // Invoke protoc with fake_plugin to get ahold of the CodeGeneratorRequest
  // sent by protoc.
  Run(absl::StrCat(
      "protocol_compiler --fake_plugin_out=$tmpdir --proto_path=$tmpdir "
      "foo.proto --plugin=prefix-gen-fake_plugin=",
      plugin_path));
  ExpectNoErrors();
  std::string base64_output = ReadFile("foo.proto.request");
  std::string binary_request;
  ASSERT_TRUE(absl::Base64Unescape(base64_output, &binary_request));
  CodeGeneratorRequest request;
  ASSERT_TRUE(request.ParseFromString(binary_request));

  EXPECT_FALSE(
      request.proto_file(0).message_type(0).field(0).options().has_features());
  EXPECT_FALSE(request.source_file_descriptors(0)
                   .message_type(0)
                   .field(0)
                   .options()
                   .has_features());
}

TEST_F(CommandLineInterfaceTest, Plugin_RuntimeFeatures) {
  CreateTempFile("foo.proto",
                 R"schema(
                      edition = "2023";
                      package foo;
                      message Foo {
                        int32 b = 1 [features.field_presence = IMPLICIT];
                      })schema");

#ifdef GOOGLE_PROTOBUF_FAKE_PLUGIN_PATH
  std::string plugin_path = GOOGLE_PROTOBUF_FAKE_PLUGIN_PATH;
#else
  std::string plugin_path = absl::StrCat(
      TestUtil::TestSourceDir(), "/google/protobuf/compiler/fake_plugin");
#endif

  // Invoke protoc with fake_plugin to get ahold of the CodeGeneratorRequest
  // sent by protoc.
  Run(absl::StrCat(
      "protocol_compiler --fake_plugin_out=$tmpdir --proto_path=$tmpdir "
      "foo.proto --plugin=prefix-gen-fake_plugin=",
      plugin_path));
  ExpectNoErrors();
  std::string base64_output = ReadFile("foo.proto.request");
  std::string binary_request;
  ASSERT_TRUE(absl::Base64Unescape(base64_output, &binary_request));
  CodeGeneratorRequest request;
  ASSERT_TRUE(request.ParseFromString(binary_request));

  EXPECT_THAT(
      request.proto_file(0).message_type(0).field(0).options().features(),
      EqualsProto(R"pb(field_presence: IMPLICIT)pb"));
  EXPECT_THAT(request.source_file_descriptors(0)
                  .message_type(0)
                  .field(0)
                  .options()
                  .features(),
              EqualsProto(R"pb(field_presence: IMPLICIT)pb"));
}

TEST_F(CommandLineInterfaceTest, Plugin_SourceFeatures) {
  CreateTempFile("google/protobuf/descriptor.proto",
                 google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  CreateTempFile("google/protobuf/unittest_features.proto",
                 pb::TestFeatures::descriptor()->file()->DebugString());
  CreateTempFile("foo.proto",
                 R"schema(
    edition = "2023";
    import "google/protobuf/unittest_features.proto";
    package foo;
    message Foo {
      int32 b = 1 [
        features.(pb.test).field_feature = VALUE6,
        features.(pb.test).source_feature = VALUE5
      ];
    }
  )schema");

#ifdef GOOGLE_PROTOBUF_FAKE_PLUGIN_PATH
  std::string plugin_path = GOOGLE_PROTOBUF_FAKE_PLUGIN_PATH;
#else
  std::string plugin_path = absl::StrCat(
      TestUtil::TestSourceDir(), "/google/protobuf/compiler/fake_plugin");
#endif

  // Invoke protoc with fake_plugin to get ahold of the CodeGeneratorRequest
  // sent by protoc.
  Run(absl::StrCat(
      "protocol_compiler --fake_plugin_out=$tmpdir --proto_path=$tmpdir "
      "foo.proto --plugin=prefix-gen-fake_plugin=",
      plugin_path));
  ExpectNoErrors();
  std::string base64_output = ReadFile("foo.proto.request");
  std::string binary_request;
  ASSERT_TRUE(absl::Base64Unescape(base64_output, &binary_request));
  CodeGeneratorRequest request;
  ASSERT_TRUE(request.ParseFromString(binary_request));

  {
    ASSERT_EQ(request.proto_file(2).name(), "foo.proto");
    const FeatureSet& features =
        request.proto_file(2).message_type(0).field(0).options().features();
    EXPECT_THAT(features,
                EqualsProto(R"pb([pb.test] { field_feature: VALUE6 })pb"));
  }

  {
    ASSERT_EQ(request.source_file_descriptors(0).name(), "foo.proto");
    const FeatureSet& features = request.source_file_descriptors(0)
                                     .message_type(0)
                                     .field(0)
                                     .options()
                                     .features();
    EXPECT_THAT(features, EqualsProto(R"pb([pb.test] {
                                             field_feature: VALUE6
                                             source_feature: VALUE5
                                           })pb"));
  }
}

TEST_F(CommandLineInterfaceTest, GeneratorFeatureLifetimeError) {
  CreateTempFile("google/protobuf/descriptor.proto",
                 google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  CreateTempFile("google/protobuf/unittest_features.proto",
                 pb::TestFeatures::descriptor()->file()->DebugString());
  CreateTempFile("foo.proto",
                 R"schema(
    edition = "2024";
    import "google/protobuf/unittest_features.proto";
    package foo;
    message Foo {
      int32 b = 1 [
        features.(pb.test).removed_feature = VALUE6
      ];
    }
  )schema");

  Run("protocol_compiler --experimental_editions --proto_path=$tmpdir "
      "--test_out=$tmpdir foo.proto");
  ExpectErrorSubstring(
      "foo.proto:6:13: Feature pb.TestFeatures.removed_feature has been "
      "removed in edition 2024");
}

TEST_F(CommandLineInterfaceTest, PluginFeatureLifetimeError) {
  CreateTempFile("google/protobuf/descriptor.proto",
                 google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  CreateTempFile("google/protobuf/unittest_features.proto",
                 pb::TestFeatures::descriptor()->file()->DebugString());
  CreateTempFile("foo.proto",
                 R"schema(
    edition = "2023";
    import "google/protobuf/unittest_features.proto";
    package foo;
    message Foo {
      int32 b = 1 [
        features.(pb.test).future_feature = VALUE6
      ];
    }
  )schema");

#ifdef GOOGLE_PROTOBUF_FAKE_PLUGIN_PATH
  std::string plugin_path = GOOGLE_PROTOBUF_FAKE_PLUGIN_PATH;
#else
  std::string plugin_path = absl::StrCat(
      TestUtil::TestSourceDir(), "/google/protobuf/compiler/fake_plugin");
#endif

  Run(absl::StrCat(
      "protocol_compiler --fake_plugin_out=$tmpdir --proto_path=$tmpdir "
      "foo.proto --plugin=prefix-gen-fake_plugin=",
      plugin_path));
  ExpectErrorSubstring(
      "foo.proto:6:13: Feature pb.TestFeatures.future_feature wasn't "
      "introduced until edition 2024");
}

TEST_F(CommandLineInterfaceTest, GeneratorNoEditionsSupport) {
  CreateTempFile("foo.proto", R"schema(
    edition = "2023";
    message Foo {
      int32 i = 1;
    }
  )schema");

  CreateGeneratorWithMissingFeatures("--no_editions_out",
                                     "Doesn't support editions",
                                     CodeGenerator::FEATURE_SUPPORTS_EDITIONS);

  Run("protocol_compiler "
      "--proto_path=$tmpdir foo.proto --no_editions_out=$tmpdir");

  ExpectErrorSubstring(
      "code generator --no_editions_out hasn't been updated to support "
      "editions");
}

TEST_F(CommandLineInterfaceTest, PluginNoEditionsSupport) {
  CreateTempFile("foo.proto", R"schema(
    edition = "2023";
    message Foo {
      int32 i = 1;
    }
  )schema");

  SetMockGeneratorTestCase("no_editions");
  Run("protocol_compiler "
      "--proto_path=$tmpdir foo.proto --plug_out=$tmpdir");

  ExpectErrorSubstring(
      "code generator prefix-gen-plug hasn't been updated to support editions");
}

TEST_F(CommandLineInterfaceTest, PluginErrorAndNoEditionsSupport) {
  CreateTempFile("foo.proto", R"schema(
    edition = "2023";
    message MockCodeGenerator_Error { }
  )schema");

  SetMockGeneratorTestCase("no_editions");
  Run("protocol_compiler "
      "--proto_path=$tmpdir foo.proto --plug_out=$tmpdir");

  ExpectErrorSubstring(
      "code generator prefix-gen-plug hasn't been updated to support editions");
  ExpectErrorSubstring(
      "--plug_out: foo.proto: Saw message type MockCodeGenerator_Error.");
}

TEST_F(CommandLineInterfaceTest, EditionDefaults) {
  CreateTempFile("google/protobuf/descriptor.proto",
                 google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  Run("protocol_compiler --proto_path=$tmpdir "
      "--edition_defaults_out=$tmpdir/defaults "
      "google/protobuf/descriptor.proto");
  ExpectNoErrors();

  FeatureSetDefaults defaults = ReadEditionDefaults("defaults");
  EXPECT_THAT(defaults, EqualsProto(R"pb(
                defaults {
                  edition: EDITION_PROTO2
                  overridable_features {}
                  fixed_features {
                    field_presence: EXPLICIT
                    enum_type: CLOSED
                    repeated_field_encoding: EXPANDED
                    utf8_validation: NONE
                    message_encoding: LENGTH_PREFIXED
                    json_format: LEGACY_BEST_EFFORT
                  }
                }
                defaults {
                  edition: EDITION_PROTO3
                  overridable_features {}
                  fixed_features {
                    field_presence: IMPLICIT
                    enum_type: OPEN
                    repeated_field_encoding: PACKED
                    utf8_validation: VERIFY
                    message_encoding: LENGTH_PREFIXED
                    json_format: ALLOW
                  }
                }
                defaults {
                  edition: EDITION_2023
                  overridable_features {
                    field_presence: EXPLICIT
                    enum_type: OPEN
                    repeated_field_encoding: PACKED
                    utf8_validation: VERIFY
                    message_encoding: LENGTH_PREFIXED
                    json_format: ALLOW
                  }
                  fixed_features {}
                }
                minimum_edition: EDITION_PROTO2
                maximum_edition: EDITION_2023
              )pb"));
}

TEST_F(CommandLineInterfaceTest, EditionDefaultsWithMaximum) {
  CreateTempFile("google/protobuf/descriptor.proto",
                 google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  Run("protocol_compiler --proto_path=$tmpdir "
      "--edition_defaults_out=$tmpdir/defaults "
      "--edition_defaults_maximum=99997_TEST_ONLY "
      "google/protobuf/descriptor.proto");
  ExpectNoErrors();

  FeatureSetDefaults defaults = ReadEditionDefaults("defaults");
  EXPECT_THAT(defaults, EqualsProto(R"pb(
                defaults {
                  edition: EDITION_PROTO2
                  overridable_features {}
                  fixed_features {
                    field_presence: EXPLICIT
                    enum_type: CLOSED
                    repeated_field_encoding: EXPANDED
                    utf8_validation: NONE
                    message_encoding: LENGTH_PREFIXED
                    json_format: LEGACY_BEST_EFFORT
                  }
                }
                defaults {
                  edition: EDITION_PROTO3
                  overridable_features {}
                  fixed_features {
                    field_presence: IMPLICIT
                    enum_type: OPEN
                    repeated_field_encoding: PACKED
                    utf8_validation: VERIFY
                    message_encoding: LENGTH_PREFIXED
                    json_format: ALLOW
                  }
                }
                defaults {
                  edition: EDITION_2023
                  overridable_features {
                    field_presence: EXPLICIT
                    enum_type: OPEN
                    repeated_field_encoding: PACKED
                    utf8_validation: VERIFY
                    message_encoding: LENGTH_PREFIXED
                    json_format: ALLOW
                  }
                  fixed_features {}
                }
                minimum_edition: EDITION_PROTO2
                maximum_edition: EDITION_99997_TEST_ONLY
              )pb"));
}

TEST_F(CommandLineInterfaceTest, EditionDefaultsWithMinimum) {
  CreateTempFile("google/protobuf/descriptor.proto",
                 google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  Run("protocol_compiler --proto_path=$tmpdir "
      "--edition_defaults_out=$tmpdir/defaults "
      "--edition_defaults_minimum=99997_TEST_ONLY "
      "--edition_defaults_maximum=99999_TEST_ONLY "
      "google/protobuf/descriptor.proto");
  ExpectNoErrors();

  FeatureSetDefaults defaults = ReadEditionDefaults("defaults");
  EXPECT_THAT(defaults, EqualsProto(R"pb(
                defaults {
                  edition: EDITION_PROTO2
                  overridable_features {}
                  fixed_features {
                    field_presence: EXPLICIT
                    enum_type: CLOSED
                    repeated_field_encoding: EXPANDED
                    utf8_validation: NONE
                    message_encoding: LENGTH_PREFIXED
                    json_format: LEGACY_BEST_EFFORT
                  }
                }
                defaults {
                  edition: EDITION_PROTO3
                  overridable_features {}
                  fixed_features {
                    field_presence: IMPLICIT
                    enum_type: OPEN
                    repeated_field_encoding: PACKED
                    utf8_validation: VERIFY
                    message_encoding: LENGTH_PREFIXED
                    json_format: ALLOW
                  }
                }
                defaults {
                  edition: EDITION_2023
                  overridable_features {
                    field_presence: EXPLICIT
                    enum_type: OPEN
                    repeated_field_encoding: PACKED
                    utf8_validation: VERIFY
                    message_encoding: LENGTH_PREFIXED
                    json_format: ALLOW
                  }
                  fixed_features {}
                }
                minimum_edition: EDITION_99997_TEST_ONLY
                maximum_edition: EDITION_99999_TEST_ONLY
              )pb"));
}

TEST_F(CommandLineInterfaceTest, EditionDefaultsWithExtension) {
  CreateTempFile("google/protobuf/descriptor.proto",
                 google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  CreateTempFile("features.proto",
                 pb::TestFeatures::descriptor()->file()->DebugString());
  Run("protocol_compiler --proto_path=$tmpdir "
      "--edition_defaults_out=$tmpdir/defaults "
      "--edition_defaults_maximum=99999_TEST_ONLY "
      "features.proto google/protobuf/descriptor.proto");
  ExpectNoErrors();

  FeatureSetDefaults defaults = ReadEditionDefaults("defaults");
  EXPECT_EQ(defaults.minimum_edition(), EDITION_PROTO2);
  EXPECT_EQ(defaults.maximum_edition(), EDITION_99999_TEST_ONLY);
  ASSERT_EQ(defaults.defaults_size(), 6);
  EXPECT_EQ(defaults.defaults(0).edition(), EDITION_PROTO2);
  EXPECT_EQ(defaults.defaults(2).edition(), EDITION_2023);
  EXPECT_EQ(defaults.defaults(3).edition(), EDITION_2024);
  EXPECT_EQ(defaults.defaults(4).edition(), EDITION_99997_TEST_ONLY);
  EXPECT_EQ(defaults.defaults(5).edition(), EDITION_99998_TEST_ONLY);
  EXPECT_EQ(defaults.defaults(0)
                .fixed_features()
                .GetExtension(pb::test)
                .file_feature(),
            pb::EnumFeature::VALUE1);
  EXPECT_EQ(defaults.defaults(2)
                .overridable_features()
                .GetExtension(pb::test)
                .file_feature(),
            pb::EnumFeature::VALUE3);
  EXPECT_EQ(defaults.defaults(3)
                .overridable_features()
                .GetExtension(pb::test)
                .file_feature(),
            pb::EnumFeature::VALUE3);
  EXPECT_EQ(defaults.defaults(4)
                .overridable_features()
                .GetExtension(pb::test)
                .file_feature(),
            pb::EnumFeature::VALUE4);
  EXPECT_EQ(defaults.defaults(5)
                .overridable_features()
                .GetExtension(pb::test)
                .file_feature(),
            pb::EnumFeature::VALUE5);
}

#ifndef _WIN32
TEST_F(CommandLineInterfaceTest, EditionDefaultsDependencyManifest) {
  CreateTempFile("google/protobuf/descriptor.proto",
                 google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  CreateTempFile("features.proto",
                 pb::TestFeatures::descriptor()->file()->DebugString());

  Run("protocol_compiler --dependency_out=$tmpdir/manifest "
      "--edition_defaults_out=$tmpdir/defaults "
      "--proto_path=$tmpdir features.proto");

  ExpectNoErrors();

  ExpectFileContent(
      "manifest",
      "$tmpdir/defaults: "
      "$tmpdir/google/protobuf/descriptor.proto\\\n $tmpdir/features.proto");
}
#endif  // _WIN32

TEST_F(CommandLineInterfaceTest, EditionDefaultsInvalidMissingDescriptor) {
  CreateTempFile("features.proto", R"schema(
    syntax = "proto2";
    message Foo {}
  )schema");
  Run("protocol_compiler --proto_path=$tmpdir "
      "--edition_defaults_out=$tmpdir/defaults "
      "features.proto");
  ExpectErrorSubstring("Could not find FeatureSet in descriptor pool");
}

TEST_F(CommandLineInterfaceTest, EditionDefaultsInvalidTwice) {
  CreateTempFile("google/protobuf/descriptor.proto",
                 google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  Run("protocol_compiler --proto_path=$tmpdir "
      "--edition_defaults_out=$tmpdir/defaults "
      "--edition_defaults_out=$tmpdir/defaults "
      "google/protobuf/descriptor.proto");
  ExpectErrorSubstring("edition_defaults_out may only be passed once");
}

TEST_F(CommandLineInterfaceTest, EditionDefaultsInvalidEmpty) {
  CreateTempFile("google/protobuf/descriptor.proto",
                 google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  Run("protocol_compiler --proto_path=$tmpdir "
      "--edition_defaults_out= "
      "google/protobuf/descriptor.proto");
  ExpectErrorSubstring("edition_defaults_out requires a non-empty value");
}

TEST_F(CommandLineInterfaceTest, EditionDefaultsInvalidCompile) {
  CreateTempFile("google/protobuf/descriptor.proto",
                 google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  Run("protocol_compiler --proto_path=$tmpdir "
      "--encode=pb.CppFeatures "
      "--edition_defaults_out=$tmpdir/defaults "
      "google/protobuf/descriptor.proto");
  ExpectErrorSubstring("Cannot use --encode or --decode and generate defaults");
}

TEST_F(CommandLineInterfaceTest, EditionDefaultsInvalidMinimumTwice) {
  CreateTempFile("google/protobuf/descriptor.proto",
                 google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  Run("protocol_compiler --proto_path=$tmpdir "
      "--edition_defaults_minimum=2023 "
      "--edition_defaults_minimum=2023 "
      "google/protobuf/descriptor.proto");
  ExpectErrorSubstring("edition_defaults_minimum may only be passed once");
}

TEST_F(CommandLineInterfaceTest, EditionDefaultsInvalidMinimumEmpty) {
  CreateTempFile("google/protobuf/descriptor.proto",
                 google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  Run("protocol_compiler --proto_path=$tmpdir "
      "--edition_defaults_minimum= "
      "google/protobuf/descriptor.proto");
  ExpectErrorSubstring("unknown edition \"\"");
}

TEST_F(CommandLineInterfaceTest, EditionDefaultsInvalidMinimumUnknown) {
  CreateTempFile("google/protobuf/descriptor.proto",
                 google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  Run("protocol_compiler --proto_path=$tmpdir "
      "--edition_defaults_minimum=2022 "
      "google/protobuf/descriptor.proto");
  ExpectErrorSubstring("unknown edition \"2022\"");
}

TEST_F(CommandLineInterfaceTest, EditionDefaultsInvalidMaximumTwice) {
  CreateTempFile("google/protobuf/descriptor.proto",
                 google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  Run("protocol_compiler --proto_path=$tmpdir "
      "--edition_defaults_maximum=2023 "
      "--edition_defaults_maximum=2023 "
      "google/protobuf/descriptor.proto");
  ExpectErrorSubstring("edition_defaults_maximum may only be passed once");
}

TEST_F(CommandLineInterfaceTest, EditionDefaultsInvalidMaximumEmpty) {
  CreateTempFile("google/protobuf/descriptor.proto",
                 google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  Run("protocol_compiler --proto_path=$tmpdir "
      "--edition_defaults_maximum= "
      "google/protobuf/descriptor.proto");
  ExpectErrorSubstring("unknown edition \"\"");
}

TEST_F(CommandLineInterfaceTest, EditionDefaultsInvalidMaximumUnknown) {
  CreateTempFile("google/protobuf/descriptor.proto",
                 google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  Run("protocol_compiler --proto_path=$tmpdir "
      "--edition_defaults_maximum=2022 "
      "google/protobuf/descriptor.proto");
  ExpectErrorSubstring("unknown edition \"2022\"");
}


TEST_F(CommandLineInterfaceTest, DirectDependencies_Missing_EmptyList) {
  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "import \"bar.proto\";\n"
                 "message Foo { optional Bar bar = 1; }");
  CreateTempFile("bar.proto",
                 "syntax = \"proto2\";\n"
                 "message Bar { optional string text = 1; }");

  Run("protocol_compiler --test_out=$tmpdir --proto_path=$tmpdir "
      "--direct_dependencies= foo.proto");

  ExpectErrorText(
      "foo.proto: File is imported but not declared in --direct_dependencies: "
      "bar.proto\n");
}

TEST_F(CommandLineInterfaceTest, DirectDependencies_Missing) {
  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "import \"bar.proto\";\n"
                 "import \"bla.proto\";\n"
                 "message Foo { optional Bar bar = 1; optional Bla bla = 2; }");
  CreateTempFile("bar.proto",
                 "syntax = \"proto2\";\n"
                 "message Bar { optional string text = 1; }");
  CreateTempFile("bla.proto",
                 "syntax = \"proto2\";\n"
                 "message Bla { optional int64 number = 1; }");

  Run("protocol_compiler --test_out=$tmpdir --proto_path=$tmpdir "
      "--direct_dependencies=bla.proto foo.proto");

  ExpectErrorText(
      "foo.proto: File is imported but not declared in --direct_dependencies: "
      "bar.proto\n");
}

TEST_F(CommandLineInterfaceTest, DirectDependencies_NoViolation) {
  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "import \"bar.proto\";\n"
                 "message Foo { optional Bar bar = 1; }");
  CreateTempFile("bar.proto",
                 "syntax = \"proto2\";\n"
                 "message Bar { optional string text = 1; }");

  Run("protocol_compiler --test_out=$tmpdir --proto_path=$tmpdir "
      "--direct_dependencies=bar.proto foo.proto");

  ExpectNoErrors();
}

TEST_F(CommandLineInterfaceTest, DirectDependencies_NoViolation_MultiImports) {
  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "import \"bar.proto\";\n"
                 "import \"bla.proto\";\n"
                 "message Foo { optional Bar bar = 1; optional Bla bla = 2; }");
  CreateTempFile("bar.proto",
                 "syntax = \"proto2\";\n"
                 "message Bar { optional string text = 1; }");
  CreateTempFile("bla.proto",
                 "syntax = \"proto2\";\n"
                 "message Bla { optional int64 number = 1; }");

  Run("protocol_compiler --test_out=$tmpdir --proto_path=$tmpdir "
      "--direct_dependencies=bar.proto:bla.proto foo.proto");

  ExpectNoErrors();
}

TEST_F(CommandLineInterfaceTest, DirectDependencies_ProvidedMultipleTimes) {
  CreateTempFile("foo.proto", "syntax = \"proto2\";\n");

  Run("protocol_compiler --test_out=$tmpdir --proto_path=$tmpdir "
      "--direct_dependencies=bar.proto --direct_dependencies=bla.proto "
      "foo.proto");

  ExpectErrorText(
      "--direct_dependencies may only be passed once. To specify multiple "
      "direct dependencies, pass them all as a single parameter separated by "
      "':'.\n");
}

TEST_F(CommandLineInterfaceTest, DirectDependencies_CustomErrorMessage) {
  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "import \"bar.proto\";\n"
                 "message Foo { optional Bar bar = 1; }");
  CreateTempFile("bar.proto",
                 "syntax = \"proto2\";\n"
                 "message Bar { optional string text = 1; }");

  std::vector<std::string> commands;
  commands.push_back("protocol_compiler");
  commands.push_back("--test_out=$tmpdir");
  commands.push_back("--proto_path=$tmpdir");
  commands.push_back("--direct_dependencies=");
  commands.push_back("--direct_dependencies_violation_msg=Bla \"%s\" Bla");
  commands.push_back("foo.proto");
  RunWithArgs(commands);

  ExpectErrorText("foo.proto: Bla \"bar.proto\" Bla\n");
}

TEST_F(CommandLineInterfaceTest, CwdRelativeInputs) {
  // Test that we can accept working-directory-relative input files.

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir $tmpdir/foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator", "", "foo.proto", "Foo");
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
  EXPECT_EQ(1, descriptor_set.file_size());
  EXPECT_EQ("bar.proto", descriptor_set.file(0).name());
  // Descriptor set should not have source code info.
  EXPECT_FALSE(descriptor_set.file(0).has_source_code_info());
  // Descriptor set should have json_name.
  EXPECT_EQ("Bar", descriptor_set.file(0).message_type(0).name());
  EXPECT_EQ("foo", descriptor_set.file(0).message_type(0).field(0).name());
  EXPECT_TRUE(descriptor_set.file(0).message_type(0).field(0).has_json_name());
}

TEST_F(CommandLineInterfaceTest, WriteDescriptorSetWithDuplicates) {
  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");
  CreateTempFile("bar.proto",
                 "syntax = \"proto2\";\n"
                 "import \"foo.proto\";\n"
                 "message Bar {\n"
                 "  optional Foo foo = 1;\n"
                 "}\n");
  CreateTempFile("baz.proto",
                 "syntax = \"proto2\";\n"
                 "import \"foo.proto\";\n"
                 "message Baz {\n"
                 "  optional Foo foo = 1;\n"
                 "}\n");

  Run("protocol_compiler --descriptor_set_out=$tmpdir/descriptor_set "
      "--proto_path=$tmpdir bar.proto foo.proto bar.proto baz.proto");

  ExpectNoErrors();

  FileDescriptorSet descriptor_set;
  ReadDescriptorSet("descriptor_set", &descriptor_set);
  if (HasFatalFailure()) return;
  EXPECT_EQ(3, descriptor_set.file_size());
  // foo should come first since the output is in dependency order.
  // since bar and baz are unordered, they should be in command line order.
  EXPECT_EQ("foo.proto", descriptor_set.file(0).name());
  EXPECT_EQ("bar.proto", descriptor_set.file(1).name());
  EXPECT_EQ("baz.proto", descriptor_set.file(2).name());
  // Descriptor set should not have source code info.
  EXPECT_FALSE(descriptor_set.file(0).has_source_code_info());
  // Descriptor set should have json_name.
  EXPECT_EQ("Bar", descriptor_set.file(1).message_type(0).name());
  EXPECT_EQ("foo", descriptor_set.file(1).message_type(0).field(0).name());
  EXPECT_TRUE(descriptor_set.file(1).message_type(0).field(0).has_json_name());
}

TEST_F(CommandLineInterfaceTest, WriteDescriptorSetWithSourceInfo) {
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
      "--include_source_info --proto_path=$tmpdir bar.proto");

  ExpectNoErrors();

  FileDescriptorSet descriptor_set;
  ReadDescriptorSet("descriptor_set", &descriptor_set);
  if (HasFatalFailure()) return;
  EXPECT_EQ(1, descriptor_set.file_size());
  EXPECT_EQ("bar.proto", descriptor_set.file(0).name());
  // Source code info included.
  EXPECT_TRUE(descriptor_set.file(0).has_source_code_info());
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
  EXPECT_EQ(2, descriptor_set.file_size());
  if (descriptor_set.file(0).name() == "bar.proto") {
    std::swap(descriptor_set.mutable_file()->mutable_data()[0],
              descriptor_set.mutable_file()->mutable_data()[1]);
  }
  EXPECT_EQ("foo.proto", descriptor_set.file(0).name());
  EXPECT_EQ("bar.proto", descriptor_set.file(1).name());
  // Descriptor set should not have source code info.
  EXPECT_FALSE(descriptor_set.file(0).has_source_code_info());
  EXPECT_FALSE(descriptor_set.file(1).has_source_code_info());
}

TEST_F(CommandLineInterfaceTest, WriteTransitiveDescriptorSetWithSourceInfo) {
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
      "--include_imports --include_source_info --proto_path=$tmpdir bar.proto");

  ExpectNoErrors();

  FileDescriptorSet descriptor_set;
  ReadDescriptorSet("descriptor_set", &descriptor_set);
  if (HasFatalFailure()) return;
  EXPECT_EQ(2, descriptor_set.file_size());
  if (descriptor_set.file(0).name() == "bar.proto") {
    std::swap(descriptor_set.mutable_file()->mutable_data()[0],
              descriptor_set.mutable_file()->mutable_data()[1]);
  }
  EXPECT_EQ("foo.proto", descriptor_set.file(0).name());
  EXPECT_EQ("bar.proto", descriptor_set.file(1).name());
  // Source code info included.
  EXPECT_TRUE(descriptor_set.file(0).has_source_code_info());
  EXPECT_TRUE(descriptor_set.file(1).has_source_code_info());
}

TEST_F(CommandLineInterfaceTest, DescriptorSetOptionRetention) {
  // clang-format off
  CreateTempFile(
      "foo.proto",
      absl::Substitute(R"pb(
          syntax = "proto2";
          import "$0";
          extend google.protobuf.FileOptions {
            optional int32 runtime_retention_option = 50001
                [retention = RETENTION_RUNTIME];
            optional int32 source_retention_option = 50002
                [retention = RETENTION_SOURCE];
          }
          option (runtime_retention_option) = 2;
          option (source_retention_option) = 3;)pb",
          DescriptorProto::descriptor()->file()->name()));
  // clang-format on
  std::string descriptor_proto_base_dir = "src";
  Run(absl::Substitute(
      "protocol_compiler --descriptor_set_out=$$tmpdir/descriptor_set "
      "--proto_path=$$tmpdir --proto_path=$0 foo.proto",
      descriptor_proto_base_dir));
  ExpectNoErrors();

  FileDescriptorSet descriptor_set;
  ReadDescriptorSet("descriptor_set", &descriptor_set);
  ASSERT_EQ(descriptor_set.file_size(), 1);
  const UnknownFieldSet& unknown_fields =
      descriptor_set.file(0).options().unknown_fields();

  // We expect runtime_retention_option to be present while
  // source_retention_option should have been stripped.
  ASSERT_EQ(unknown_fields.field_count(), 1);
  EXPECT_EQ(unknown_fields.field(0).number(), 50001);
  EXPECT_EQ(unknown_fields.field(0).varint(), 2);
}

TEST_F(CommandLineInterfaceTest, DescriptorSetOptionRetentionOverride) {
  // clang-format off
  CreateTempFile(
      "foo.proto",
      absl::Substitute(R"pb(
          syntax = "proto2";
          import "$0";
          extend google.protobuf.FileOptions {
            optional int32 runtime_retention_option = 50001
                [retention = RETENTION_RUNTIME];
            optional int32 source_retention_option = 50002
                [retention = RETENTION_SOURCE];
          }
          option (runtime_retention_option) = 2;
          option (source_retention_option) = 3;)pb",
          DescriptorProto::descriptor()->file()->name()));
  // clang-format on
  std::string descriptor_proto_base_dir = "src";
  Run(absl::Substitute(
      "protocol_compiler --descriptor_set_out=$$tmpdir/descriptor_set "
      "--proto_path=$$tmpdir --retain_options --proto_path=$0 foo.proto",
      descriptor_proto_base_dir));
  ExpectNoErrors();

  FileDescriptorSet descriptor_set;
  ReadDescriptorSet("descriptor_set", &descriptor_set);
  ASSERT_EQ(descriptor_set.file_size(), 1);
  const UnknownFieldSet& unknown_fields =
      descriptor_set.file(0).options().unknown_fields();

  // We expect all options to be present.
  ASSERT_EQ(unknown_fields.field_count(), 2);
  EXPECT_EQ(unknown_fields.field(0).number(), 50001);
  EXPECT_EQ(unknown_fields.field(1).number(), 50002);
  EXPECT_EQ(unknown_fields.field(0).varint(), 2);
  EXPECT_EQ(unknown_fields.field(1).varint(), 3);
}

#ifdef _WIN32
// TODO: Figure out how to write test on windows.
#else
TEST_F(CommandLineInterfaceTest, WriteDependencyManifestFileGivenTwoInputs) {
  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");
  CreateTempFile("bar.proto",
                 "syntax = \"proto2\";\n"
                 "import \"foo.proto\";\n"
                 "message Bar {\n"
                 "  optional Foo foo = 1;\n"
                 "}\n");

  Run("protocol_compiler --dependency_out=$tmpdir/manifest "
      "--test_out=$tmpdir --proto_path=$tmpdir bar.proto foo.proto");

  ExpectErrorText(
      "Can only process one input file when using --dependency_out=FILE.\n");
}

#ifdef PROTOBUF_OPENSOURCE
TEST_F(CommandLineInterfaceTest, WriteDependencyManifestFile) {
  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");
  CreateTempFile("bar.proto",
                 "syntax = \"proto2\";\n"
                 "import \"foo.proto\";\n"
                 "message Bar {\n"
                 "  optional Foo foo = 1;\n"
                 "}\n");

  std::string current_working_directory = getcwd(nullptr, 0);
  SwitchToTempDirectory();

  Run("protocol_compiler --dependency_out=manifest --test_out=. "
      "bar.proto");

  ExpectNoErrors();

  ExpectFileContent("manifest",
                    "bar.proto.MockCodeGenerator.test_generator: "
                    "foo.proto\\\n bar.proto");

  File::ChangeWorkingDirectory(current_working_directory);
}
#else  // !PROTOBUF_OPENSOURCE
// TODO: Figure out how to change and get working directory in
// google3.
#endif  // !PROTOBUF_OPENSOURCE

TEST_F(CommandLineInterfaceTest, WriteDependencyManifestFileForAbsolutePath) {
  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");
  CreateTempFile("bar.proto",
                 "syntax = \"proto2\";\n"
                 "import \"foo.proto\";\n"
                 "message Bar {\n"
                 "  optional Foo foo = 1;\n"
                 "}\n");

  Run("protocol_compiler --dependency_out=$tmpdir/manifest "
      "--test_out=$tmpdir --proto_path=$tmpdir bar.proto");

  ExpectNoErrors();

  ExpectFileContent("manifest",
                    "$tmpdir/bar.proto.MockCodeGenerator.test_generator: "
                    "$tmpdir/foo.proto\\\n $tmpdir/bar.proto");
}

TEST_F(CommandLineInterfaceTest,
       WriteDependencyManifestFileWithDescriptorSetOut) {
  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");
  CreateTempFile("bar.proto",
                 "syntax = \"proto2\";\n"
                 "import \"foo.proto\";\n"
                 "message Bar {\n"
                 "  optional Foo foo = 1;\n"
                 "}\n");

  Run("protocol_compiler --dependency_out=$tmpdir/manifest "
      "--descriptor_set_out=$tmpdir/bar.pb --proto_path=$tmpdir bar.proto");

  ExpectNoErrors();

  ExpectFileContent("manifest",
                    "$tmpdir/bar.pb: "
                    "$tmpdir/foo.proto\\\n $tmpdir/bar.proto");
}
#endif  // !_WIN32

TEST_F(CommandLineInterfaceTest, TestArgumentFile) {
  // Test parsing multiple input files using an argument file.

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");
  CreateTempFile("bar.proto",
                 "syntax = \"proto2\";\n"
                 "message Bar {}\n");
  CreateTempFile("arguments.txt",
                 "--test_out=$tmpdir\n"
                 "--plug_out=$tmpdir\n"
                 "--proto_path=$tmpdir\n"
                 "--direct_dependencies_violation_msg=%s is not imported\n"
                 "foo.proto\n"
                 "bar.proto");

  Run("protocol_compiler @$tmpdir/arguments.txt");

  ExpectNoErrors();
  ExpectGeneratedWithMultipleInputs("test_generator", "foo.proto,bar.proto",
                                    "foo.proto", "Foo");
  ExpectGeneratedWithMultipleInputs("test_generator", "foo.proto,bar.proto",
                                    "bar.proto", "Bar");
  ExpectGeneratedWithMultipleInputs("test_plugin", "foo.proto,bar.proto",
                                    "foo.proto", "Foo");
  ExpectGeneratedWithMultipleInputs("test_plugin", "foo.proto,bar.proto",
                                    "bar.proto", "Bar");
}


// -------------------------------------------------------------------

TEST_F(CommandLineInterfaceTest, ParseErrors) {
  // Test that parse errors are reported.

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "badsyntax\n");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectErrorText(
      "foo.proto:2:1: Expected top-level statement (e.g. \"message\").\n");
}

TEST_F(CommandLineInterfaceTest, ParseErrors_DescriptorSetIn) {
  // Test that parse errors are reported.
  CreateTempFile("foo.bin", "not a FileDescriptorSet");

  Run("protocol_compiler --test_out=$tmpdir "
      "--descriptor_set_in=$tmpdir/foo.bin foo.proto");

  ExpectErrorText("$tmpdir/foo.bin: Unable to parse.\n");
}

TEST_F(CommandLineInterfaceTest, ParseErrorsMultipleFiles) {
  // Test that parse errors are reported from multiple files.

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
      "baz.proto:2:1: Import \"bar.proto\" was not found or had errors.\n"
      "foo.proto:2:1: Import \"bar.proto\" was not found or had errors.\n"
      "foo.proto:3:1: Import \"baz.proto\" was not found or had errors.\n");
}

TEST_F(CommandLineInterfaceTest, RecursiveImportFails) {
  // Create a proto file that imports itself.
  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "import \"foo.proto\";\n");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectErrorSubstring(
      "foo.proto:2:1: File recursively imports itself: "
      "foo.proto -> foo.proto\n");
}

TEST_F(CommandLineInterfaceTest, InputNotFoundError) {
  // Test what happens if the input file is not found.

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectErrorText(
      "Could not make proto path relative: foo.proto: No such file or "
      "directory\n");
}

TEST_F(CommandLineInterfaceTest, InputNotFoundError_DescriptorSetIn) {
  // Test what happens if the input file is not found.

  Run("protocol_compiler --test_out=$tmpdir "
      "--descriptor_set_in=$tmpdir/foo.bin foo.proto");

  ExpectErrorText("$tmpdir/foo.bin: No such file or directory\n");
}

TEST_F(CommandLineInterfaceTest, CwdRelativeInputNotFoundError) {
  // Test what happens when a working-directory-relative input file is not
  // found.

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir $tmpdir/foo.proto");

  ExpectErrorText(
      "Could not make proto path relative: $tmpdir/foo.proto: No such file or "
      "directory\n");
}

TEST_F(CommandLineInterfaceTest, CwdRelativeInputNotMappedError) {
  // Test what happens when a working-directory-relative input file is not
  // mapped to a virtual path.

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
      "--proto_path which encompasses this file.  Note that the "
      "proto_path must be an exact prefix of the .proto file "
      "names -- protoc is too dumb to figure out when two paths "
      "(e.g. absolute and relative) are equivalent (it's harder "
      "than you think).\n");
}

TEST_F(CommandLineInterfaceTest, CwdRelativeInputNotFoundAndNotMappedError) {
  // Check what happens if the input file is not found *and* is not mapped
  // in the proto_path.

  // Create a directory called "bar" so that we can point --proto_path at it.
  CreateTempFile("bar/dummy", "");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir/bar $tmpdir/foo.proto");

  ExpectErrorText(
      "Could not make proto path relative: $tmpdir/foo.proto: No such file or "
      "directory\n");
}

TEST_F(CommandLineInterfaceTest, CwdRelativeInputShadowedError) {
  // Test what happens when a working-directory-relative input file is shadowed
  // by another file in the virtual path.

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

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir/foo foo.proto");

  ExpectErrorText(
      "$tmpdir/foo: warning: directory does not exist.\n"
      "Could not make proto path relative: foo.proto: No such file or "
      "directory\n");
}

TEST_F(CommandLineInterfaceTest, ProtoPathAndDescriptorSetIn) {
  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir --descriptor_set_in=$tmpdir/foo.bin foo.proto");
  ExpectErrorText("$tmpdir/foo.bin: No such file or directory\n");

  Run("protocol_compiler --test_out=$tmpdir "
      "--descriptor_set_in=$tmpdir/foo.bin --proto_path=$tmpdir foo.proto");
  ExpectErrorText("$tmpdir/foo.bin: No such file or directory\n");
}

TEST_F(CommandLineInterfaceTest, ProtoPathAndDescriptorSetIn_CompileFiles) {
  // Test what happens if a proto is in a --descriptor_set_in and also exists
  // on disk.
  FileDescriptorSet file_descriptor_set;

  // NOTE: This file desc SHOULD be different from the one created as a temp
  //       to make it easier to test that the file was output instead of the
  //       contents of the --descriptor_set_in file.
  FileDescriptorProto* file_descriptor_proto = file_descriptor_set.add_file();
  file_descriptor_proto->set_name("foo.proto");
  file_descriptor_proto->add_message_type()->set_name("Foo");

  WriteDescriptorSet("foo.bin", &file_descriptor_set);

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message FooBar { required string foo_message = 1; }\n");

  Run("protocol_compiler --descriptor_set_out=$tmpdir/descriptor_set "
      "--descriptor_set_in=$tmpdir/foo.bin "
      "--include_source_info "
      "--proto_path=$tmpdir foo.proto");

  ExpectNoErrors();

  FileDescriptorSet descriptor_set;
  ReadDescriptorSet("descriptor_set", &descriptor_set);

  EXPECT_EQ(1, descriptor_set.file_size());
  EXPECT_EQ("foo.proto", descriptor_set.file(0).name());
  // Descriptor set SHOULD have source code info.
  EXPECT_TRUE(descriptor_set.file(0).has_source_code_info());

  EXPECT_EQ("FooBar", descriptor_set.file(0).message_type(0).name());
  EXPECT_EQ("foo_message",
            descriptor_set.file(0).message_type(0).field(0).name());
}

TEST_F(CommandLineInterfaceTest, ProtoPathAndDependencyOut) {
  Run("protocol_compiler --test_out=$tmpdir "
      "--dependency_out=$tmpdir/manifest "
      "--descriptor_set_in=$tmpdir/foo.bin foo.proto");
  ExpectErrorText(
      "--descriptor_set_in cannot be used with --dependency_out.\n");

  Run("protocol_compiler --test_out=$tmpdir "
      "--descriptor_set_in=$tmpdir/foo.bin "
      "--dependency_out=$tmpdir/manifest foo.proto");
  ExpectErrorText(
      "--dependency_out cannot be used with --descriptor_set_in.\n");
}

TEST_F(CommandLineInterfaceTest, MissingInputError) {
  // Test that we get an error if no inputs are given.

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir");

  ExpectErrorText("Missing input file.\n");
}

TEST_F(CommandLineInterfaceTest, MissingOutputError) {
  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");

  Run("protocol_compiler --proto_path=$tmpdir foo.proto");

  ExpectErrorText("Missing output directives.\n");
}

TEST_F(CommandLineInterfaceTest, OutputWriteError) {
  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");

  std::string output_file =
      MockCodeGenerator::GetOutputFileName("test_generator", "foo.proto");

  // Create a directory blocking our output location.
  CreateTempDir(output_file);

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir foo.proto");

  // MockCodeGenerator no longer detects an error because we actually write to
  // an in-memory location first, then dump to disk at the end.  This is no
  // big deal.
  //   ExpectErrorSubstring("MockCodeGenerator detected write error.");

#if defined(_WIN32) && !defined(__CYGWIN__)
  // Windows with MSVCRT.dll produces EPERM instead of EISDIR.
  if (HasAlternateErrorSubstring(
          absl::StrCat(output_file, ": Permission denied"))) {
    return;
  }
#endif

  ExpectErrorSubstring(absl::StrCat(output_file, ": Is a directory"));
}

TEST_F(CommandLineInterfaceTest, PluginOutputWriteError) {
  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");

  std::string output_file =
      MockCodeGenerator::GetOutputFileName("test_plugin", "foo.proto");

  // Create a directory blocking our output location.
  CreateTempDir(output_file);

  Run("protocol_compiler --plug_out=$tmpdir "
      "--proto_path=$tmpdir foo.proto");

#if defined(_WIN32) && !defined(__CYGWIN__)
  // Windows with MSVCRT.dll produces EPERM instead of EISDIR.
  if (HasAlternateErrorSubstring(
          absl::StrCat(output_file, ": Permission denied"))) {
    return;
  }
#endif

  ExpectErrorSubstring(absl::StrCat(output_file, ": Is a directory"));
}

TEST_F(CommandLineInterfaceTest, OutputDirectoryNotFoundError) {
  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");

  Run("protocol_compiler --test_out=$tmpdir/nosuchdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectErrorSubstring("nosuchdir/: No such file or directory");
}

TEST_F(CommandLineInterfaceTest, PluginOutputDirectoryNotFoundError) {
  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");

  Run("protocol_compiler --plug_out=$tmpdir/nosuchdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectErrorSubstring("nosuchdir/: No such file or directory");
}

TEST_F(CommandLineInterfaceTest, OutputDirectoryIsFileError) {
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
  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message MockCodeGenerator_Error {}\n");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectErrorSubstring(
      "--test_out: foo.proto: Saw message type MockCodeGenerator_Error.");
}

TEST_F(CommandLineInterfaceTest, GeneratorPluginError) {
  // Test a generator plugin that returns an error.

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message MockCodeGenerator_Error {}\n");

  Run("protocol_compiler --plug_out=TestParameter:$tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectErrorSubstring(
      "--plug_out: foo.proto: Saw message type MockCodeGenerator_Error.");
}

TEST_F(CommandLineInterfaceTest, GeneratorPluginFail) {
  // Test a generator plugin that exits with an error code.

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message MockCodeGenerator_Exit {}\n");

  Run("protocol_compiler --plug_out=TestParameter:$tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectErrorSubstring("Saw message type MockCodeGenerator_Exit.");
  ExpectErrorSubstring(
      "--plug_out: prefix-gen-plug: Plugin failed with status code 123.");
}

TEST_F(CommandLineInterfaceTest, GeneratorPluginCrash) {
  // Test a generator plugin that crashes.

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message MockCodeGenerator_Abort {}\n");

  Run("protocol_compiler --plug_out=TestParameter:$tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectErrorSubstring("Saw message type MockCodeGenerator_Abort.");

#ifdef _WIN32
  // Windows doesn't have signals.  It looks like abort()ing causes the process
  // to exit with status code 3, but let's not depend on the exact number here.
  ExpectErrorSubstring(
      "--plug_out: prefix-gen-plug: Plugin failed with status code");
#else
  // Don't depend on the exact signal number.
  ExpectErrorSubstring("--plug_out: prefix-gen-plug: Plugin killed by signal");
#endif
}

TEST_F(CommandLineInterfaceTest, PluginReceivesSourceCodeInfo) {
  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message MockCodeGenerator_HasSourceCodeInfo {}\n");

  Run("protocol_compiler --plug_out=$tmpdir --proto_path=$tmpdir foo.proto");

  ExpectErrorSubstring(
      "Saw message type MockCodeGenerator_HasSourceCodeInfo: true.");
}

TEST_F(CommandLineInterfaceTest, PluginReceivesJsonName) {
  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message MockCodeGenerator_HasJsonName {\n"
                 "  optional int32 value = 1;\n"
                 "}\n");

  Run("protocol_compiler --plug_out=$tmpdir --proto_path=$tmpdir foo.proto");

  ExpectErrorSubstring("Saw json_name: true");
}

TEST_F(CommandLineInterfaceTest, PluginReceivesCompilerVersion) {
  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message MockCodeGenerator_ShowVersionNumber {\n"
                 "  optional int32 value = 1;\n"
                 "}\n");

  Run("protocol_compiler --plug_out=$tmpdir --proto_path=$tmpdir foo.proto");

  ExpectErrorSubstring(absl::StrFormat("Saw compiler_version: %d %s",
                                       GOOGLE_PROTOBUF_VERSION,
                                       GOOGLE_PROTOBUF_VERSION_SUFFIX));
}

TEST_F(CommandLineInterfaceTest, GeneratorPluginNotFound) {
  // Test what happens if the plugin isn't found.

  CreateTempFile("error.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");

  Run("protocol_compiler --badplug_out=TestParameter:$tmpdir "
      "--plugin=prefix-gen-badplug=no_such_file "
      "--proto_path=$tmpdir error.proto");

#ifdef _WIN32
  ExpectErrorSubstring(
      absl::StrCat("--badplug_out: prefix-gen-badplug: ",
                   Subprocess::Win32ErrorMessage(ERROR_FILE_NOT_FOUND)));
#else
  // Error written to stdout by child process after exec() fails.
  ExpectErrorSubstring("no_such_file: program not found or is not executable");

  ExpectErrorSubstring(
      "Please specify a program using absolute path or make sure "
      "the program is available in your PATH system variable");

  // Error written by parent process when child fails.
  ExpectErrorSubstring(
      "--badplug_out: prefix-gen-badplug: Plugin failed with status code 1.");
#endif
}

TEST_F(CommandLineInterfaceTest, GeneratorPluginNotAllowed) {
  // Test what happens if plugins aren't allowed.

  CreateTempFile("error.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");

  DisallowPlugins();
  Run("protocol_compiler --plug_out=TestParameter:$tmpdir "
      "--proto_path=$tmpdir error.proto");

  ExpectErrorSubstring("Unknown flag: --plug_out");
}

TEST_F(CommandLineInterfaceTest, HelpText) {
  Run("test_exec_name --help");

  ExpectCapturedStdoutSubstringWithZeroReturnCode("Usage: test_exec_name ");
  ExpectCapturedStdoutSubstringWithZeroReturnCode("--test_out=OUT_DIR");
  ExpectCapturedStdoutSubstringWithZeroReturnCode("Test output.");
  ExpectCapturedStdoutSubstringWithZeroReturnCode("--alt_out=OUT_DIR");
  ExpectCapturedStdoutSubstringWithZeroReturnCode("Alt output.");
}

TEST_F(CommandLineInterfaceTest, GccFormatErrors) {
  // Test --error_format=gcc (which is the default, but we want to verify
  // that it can be set explicitly).

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

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "badsyntax\n");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir --error_format=msvs foo.proto");

  ExpectErrorText(
      "$tmpdir/foo.proto(2) : error in column=1: Expected top-level statement "
      "(e.g. \"message\").\n");
}

TEST_F(CommandLineInterfaceTest, InvalidErrorFormat) {
  // Test invalid --error_format

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "badsyntax\n");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir --error_format=invalid foo.proto");

  ExpectErrorText("Unknown error format: invalid\n");
}

TEST_F(CommandLineInterfaceTest, Warnings) {
  // Test --fatal_warnings.

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "import \"bar.proto\";\n");
  CreateTempFile("bar.proto", "syntax = \"proto2\";\n");

  Run("protocol_compiler --test_out=$tmpdir "
      "--proto_path=$tmpdir foo.proto");
  ExpectCapturedStderrSubstringWithZeroReturnCode(
      "foo.proto:2:1: warning: Import bar.proto is unused.");

  Run("protocol_compiler --test_out=$tmpdir --fatal_warnings "
      "--proto_path=$tmpdir foo.proto");
  ExpectErrorSubstring("foo.proto:2:1: warning: Import bar.proto is unused.");
}

// -------------------------------------------------------------------
// Flag parsing tests

TEST_F(CommandLineInterfaceTest, ParseSingleCharacterFlag) {
  // Test that a single-character flag works.

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");

  Run("protocol_compiler -t$tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator", "", "foo.proto", "Foo");
}

TEST_F(CommandLineInterfaceTest, ParseSpaceDelimitedValue) {
  // Test that separating the flag value with a space works.

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");

  Run("protocol_compiler --test_out $tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator", "", "foo.proto", "Foo");
}

TEST_F(CommandLineInterfaceTest, ParseSingleCharacterSpaceDelimitedValue) {
  // Test that separating the flag value with a space works for
  // single-character flags.

  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {}\n");

  Run("protocol_compiler -t $tmpdir "
      "--proto_path=$tmpdir foo.proto");

  ExpectNoErrors();
  ExpectGenerated("test_generator", "", "foo.proto", "Foo");
}

TEST_F(CommandLineInterfaceTest, MissingValueError) {
  // Test that we get an error if a flag is missing its value.

  Run("protocol_compiler --test_out --proto_path=$tmpdir foo.proto");

  ExpectErrorText("Missing value for flag: --test_out\n");
}

TEST_F(CommandLineInterfaceTest, MissingValueAtEndError) {
  // Test that we get an error if the last argument is a flag requiring a
  // value.

  Run("protocol_compiler --test_out");

  ExpectErrorText("Missing value for flag: --test_out\n");
}

TEST_F(CommandLineInterfaceTest, Proto3OptionalDisallowedNoCodegenSupport) {
  CreateTempFile("google/foo.proto",
                 "syntax = \"proto3\";\n"
                 "message Foo {\n"
                 "  optional int32 i = 1;\n"
                 "}\n");

  CreateGeneratorWithMissingFeatures("--no_proto3_optional_out",
                                     "Doesn't support proto3 optional",
                                     CodeGenerator::FEATURE_PROTO3_OPTIONAL);

  Run("protocol_compiler --experimental_allow_proto3_optional "
      "--proto_path=$tmpdir google/foo.proto --no_proto3_optional_out=$tmpdir");

  ExpectErrorSubstring(
      "code generator --no_proto3_optional_out hasn't been updated to support "
      "optional fields in proto3");
}

TEST_F(CommandLineInterfaceTest, ReservedFieldNumbersFail) {
  CreateTempFile("foo.proto",
                 R"(
syntax = "proto2";
message Foo {
  optional int32 i = 19123;
}
)");

  Run("protocol_compiler --test_out=$tmpdir --proto_path=$tmpdir foo.proto");

  ExpectErrorSubstring(
      "foo.proto: Field numbers 19000 through 19999 are reserved for the "
      "protocol buffer library implementation.");
}

TEST_F(CommandLineInterfaceTest, ReservedFieldNumbersFailAsOneof) {
  CreateTempFile("foo.proto",
                 R"(
syntax = "proto2";
message Foo {
  oneof one {
    int32 i = 19123;
  }
}
)");

  Run("protocol_compiler --test_out=$tmpdir --proto_path=$tmpdir foo.proto");

  ExpectErrorSubstring(
      "foo.proto: Field numbers 19000 through 19999 are reserved for the "
      "protocol buffer library implementation.");
}

TEST_F(CommandLineInterfaceTest, ReservedFieldNumbersFailAsExtension) {
  CreateTempFile("foo.proto",
                 R"(
syntax = "proto2";
message Foo {
  extensions 4 to max;
}
extend Foo {
  optional int32 i = 19123;
}
)");

  Run("protocol_compiler --test_out=$tmpdir --proto_path=$tmpdir foo.proto");

  ExpectErrorSubstring(
      "foo.proto: Field numbers 19000 through 19999 are reserved for the "
      "protocol buffer library implementation.");

  CreateTempFile("foo.proto",
                 R"(
syntax = "proto2";
message Foo {
  extensions 4 to max;
}
message Bar {
  extend Foo {
    optional int32 i = 19123;
  }
}
)");

  Run("protocol_compiler --test_out=$tmpdir --proto_path=$tmpdir foo.proto");

  ExpectErrorSubstring(
      "foo.proto: Field numbers 19000 through 19999 are reserved for the "
      "protocol buffer library implementation.");
}


TEST_F(CommandLineInterfaceTest, Proto3OptionalAllowWithFlag) {
  CreateTempFile("google/foo.proto",
                 "syntax = \"proto3\";\n"
                 "message Foo {\n"
                 "  optional int32 i = 1;\n"
                 "}\n");

  Run("protocol_compiler --experimental_allow_proto3_optional "
      "--proto_path=$tmpdir google/foo.proto --test_out=$tmpdir");
  ExpectNoErrors();
}

TEST_F(CommandLineInterfaceTest, PrintFreeFieldNumbers) {
  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "package foo;\n"
                 "message Foo {\n"
                 "  optional int32 a = 2;\n"
                 "  optional string b = 4;\n"
                 "  optional string c = 5;\n"
                 "  optional int64 d = 8;\n"
                 "  optional double e = 10;\n"
                 "}\n");
  CreateTempFile("bar.proto",
                 "syntax = \"proto2\";\n"
                 "message Bar {\n"
                 "  optional int32 a = 2;\n"
                 "  extensions 4 to 5;\n"
                 "  optional int64 d = 8;\n"
                 "  extensions 10;\n"
                 "}\n");
  CreateTempFile("baz.proto",
                 "syntax = \"proto2\";\n"
                 "message Baz {\n"
                 "  optional int32 a = 2;\n"
                 "  optional int64 d = 8;\n"
                 "  extensions 15 to max;\n"  // unordered.
                 "  extensions 13;\n"
                 "  extensions 10 to 12;\n"
                 "  extensions 5;\n"
                 "  extensions 4;\n"
                 "}\n");
  CreateTempFile(
      "quz.proto",
      "syntax = \"proto2\";\n"
      "message Quz {\n"
      "  message Foo {}\n"  // nested message
      "  optional int32 a = 2;\n"
      "  optional group C = 4 {\n"
      "    optional int32 d = 5;\n"
      "  }\n"
      "  extensions 8 to 10;\n"
      "  optional group E = 11 {\n"
      "    optional int32 f = 9;\n"    // explicitly reuse extension range 8-10
      "    optional group G = 15 {\n"  // nested group
      "      message Foo {}\n"         // nested message inside nested group
      "    }\n"
      "  }\n"
      "}\n");

  Run("protocol_compiler --print_free_field_numbers --proto_path=$tmpdir "
      "foo.proto bar.proto baz.proto quz.proto");

  ExpectNoErrors();

  // TODO: Cygwin doesn't work well if we try to capture stderr and
  // stdout at the same time. Need to figure out why and add this test back
  // for Cygwin.
#if !defined(__CYGWIN__)
  ExpectCapturedStdout(
      "foo.Foo                             free: 1 3 6-7 9 11-INF\n"
      "Bar                                 free: 1 3 6-7 9 11-INF\n"
      "Baz                                 free: 1 3 6-7 9 14\n"
      "Quz.Foo                             free: 1-INF\n"
      "Quz.C                               free: 1-4 6-INF\n"
      "Quz.E.G.Foo                         free: 1-INF\n"
      "Quz.E.G                             free: 1-INF\n"
      "Quz.E                               free: 1-8 10-14 16-INF\n"
      "Quz                                 free: 1 3 5-7 12-INF\n");
#endif
}

TEST_F(CommandLineInterfaceTest, TargetTypeEnforcement) {
  // The target option on a field indicates what kind of entity it may apply to
  // when it is used as an option. This test verifies that the enforcement
  // works correctly on all entity types.
  CreateTempFile("google/protobuf/descriptor.proto",
                 google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  CreateTempFile("foo.proto",
                 R"schema(
      syntax = "proto2";
      package protobuf_unittest;
      import "google/protobuf/descriptor.proto";
      message MyOptions {
        optional string file_option = 1 [targets = TARGET_TYPE_FILE];
        optional string extension_range_option = 2 [targets =
      TARGET_TYPE_EXTENSION_RANGE];
        optional string message_option = 3 [targets = TARGET_TYPE_MESSAGE];
        optional string field_option = 4 [targets = TARGET_TYPE_FIELD];
        optional string oneof_option = 5 [targets = TARGET_TYPE_ONEOF];
        optional string enum_option = 6 [targets = TARGET_TYPE_ENUM];
        optional string enum_value_option = 7 [targets =
      TARGET_TYPE_ENUM_ENTRY];
        optional string service_option = 8 [targets = TARGET_TYPE_SERVICE];
        optional string method_option = 9 [targets = TARGET_TYPE_METHOD];
      }
      extend google.protobuf.FileOptions {
        optional MyOptions file_options = 5000;
      }
      extend google.protobuf.ExtensionRangeOptions {
        optional MyOptions extension_range_options = 5000;
      }
      extend google.protobuf.MessageOptions {
        optional MyOptions message_options = 5000;
      }
      extend google.protobuf.FieldOptions {
        optional MyOptions field_options = 5000;
      }
      extend google.protobuf.OneofOptions {
        optional MyOptions oneof_options = 5000;
      }
      extend google.protobuf.EnumOptions {
        optional MyOptions enum_options = 5000;
      }
      extend google.protobuf.EnumValueOptions {
        optional MyOptions enum_value_options = 5000;
      }
      extend google.protobuf.ServiceOptions {
        optional MyOptions service_options = 5000;
      }
      extend google.protobuf.MethodOptions {
        optional MyOptions method_options = 5000;
      }
      option (file_options).enum_option = "x";
      message MyMessage {
        option (message_options).enum_option = "x";
        optional int32 i = 1 [(field_options).enum_option = "x"];
        extensions 2 [(extension_range_options).enum_option = "x"];
        oneof o {
          option (oneof_options).enum_option = "x";
          bool oneof_field = 3;
        }
      }
      enum MyEnum {
        option (enum_options).file_option = "x";
        UNKNOWN_MY_ENUM = 0 [(enum_value_options).enum_option = "x"];
      }
      service MyService {
        option (service_options).enum_option = "x";
        rpc MyMethod(MyMessage) returns (MyMessage) {
          option (method_options).enum_option = "x";
        }
      }
      )schema");

  Run("protocol_compiler --plug_out=$tmpdir --proto_path=$tmpdir foo.proto");
  ExpectErrorSubstring(
      "Option protobuf_unittest.MyOptions.enum_option "
      "cannot be set on an entity of type `file`.");
  ExpectErrorSubstring(
      "Option protobuf_unittest.MyOptions.enum_option "
      "cannot be set on an entity of type `extension range`.");
  ExpectErrorSubstring(
      "Option protobuf_unittest.MyOptions.enum_option "
      "cannot be set on an entity of type `message`.");
  ExpectErrorSubstring(
      "Option protobuf_unittest.MyOptions.enum_option "
      "cannot be set on an entity of type `field`.");
  ExpectErrorSubstring(
      "Option protobuf_unittest.MyOptions.enum_option "
      "cannot be set on an entity of type `oneof`.");
  ExpectErrorSubstring(
      "Option protobuf_unittest.MyOptions.file_option "
      "cannot be set on an entity of type `enum`.");
  ExpectErrorSubstring(
      "Option protobuf_unittest.MyOptions.enum_option "
      "cannot be set on an entity of type `enum entry`.");
  ExpectErrorSubstring(
      "Option protobuf_unittest.MyOptions.enum_option "
      "cannot be set on an entity of type `service`.");
  ExpectErrorSubstring(
      "Option protobuf_unittest.MyOptions.enum_option "
      "cannot be set on an entity of type `method`.");
}

TEST_F(CommandLineInterfaceTest, TargetTypeEnforcementMultipleTargetsValid) {
  CreateTempFile("google/protobuf/descriptor.proto",
                 google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  CreateTempFile("foo.proto",
                 R"schema(
      syntax = "proto2";
      package protobuf_unittest;
      import "google/protobuf/descriptor.proto";
      message MyOptions {
        optional string message_or_file_option = 1 [
            targets = TARGET_TYPE_MESSAGE, targets = TARGET_TYPE_FILE];
      }
      extend google.protobuf.FileOptions {
        optional MyOptions file_options = 5000;
      }
      extend google.protobuf.MessageOptions {
        optional MyOptions message_options = 5000;
      }
      option (file_options).message_or_file_option = "x";
      message MyMessage {
        option (message_options).message_or_file_option = "y";
      }
      )schema");

  Run("protocol_compiler --plug_out=$tmpdir --proto_path=$tmpdir foo.proto");
  ExpectNoErrors();
}

TEST_F(CommandLineInterfaceTest, TargetTypeEnforcementMultipleTargetsInvalid) {
  CreateTempFile("google/protobuf/descriptor.proto",
                 google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  CreateTempFile("foo.proto",
                 R"schema(
      syntax = "proto2";
      package protobuf_unittest;
      import "google/protobuf/descriptor.proto";
      message MyOptions {
        optional string message_or_file_option = 1 [
            targets = TARGET_TYPE_MESSAGE, targets = TARGET_TYPE_FILE];
      }
      extend google.protobuf.EnumOptions {
        optional MyOptions enum_options = 5000;
      }
      enum MyEnum {
        MY_ENUM_UNSPECIFIED = 0;
        option (enum_options).message_or_file_option = "y";
      }
      )schema");

  Run("protocol_compiler --plug_out=$tmpdir --proto_path=$tmpdir foo.proto");
  ExpectErrorSubstring(
      "Option protobuf_unittest.MyOptions.message_or_file_option cannot be set "
      "on an entity of type `enum`.");
}

TEST_F(CommandLineInterfaceTest,
       TargetTypeEnforcementMultipleEdgesWithConstraintsValid) {
  CreateTempFile("google/protobuf/descriptor.proto",
                 google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  CreateTempFile("foo.proto",
                 R"schema(
      syntax = "proto2";
      package protobuf_unittest;
      import "google/protobuf/descriptor.proto";
      message A {
        optional B b = 1 [targets = TARGET_TYPE_FILE,
                          targets = TARGET_TYPE_ENUM];
      }
      message B {
        optional int32 i = 1 [targets = TARGET_TYPE_ONEOF,
                              targets = TARGET_TYPE_FILE];
      }
      extend google.protobuf.FileOptions {
        optional A file_options = 5000;
      }
      option (file_options).b.i = 42;
      )schema");

  Run("protocol_compiler --plug_out=$tmpdir --proto_path=$tmpdir foo.proto");
  ExpectNoErrors();
}

TEST_F(CommandLineInterfaceTest,
       TargetTypeEnforcementMultipleEdgesWithConstraintsInvalid) {
  CreateTempFile("google/protobuf/descriptor.proto",
                 google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  CreateTempFile("foo.proto",
                 R"schema(
      syntax = "proto2";
      package protobuf_unittest;
      import "google/protobuf/descriptor.proto";
      message A {
        optional B b = 1 [targets = TARGET_TYPE_ENUM];
      }
      message B {
        optional int32 i = 1 [targets = TARGET_TYPE_ONEOF];
      }
      extend google.protobuf.FileOptions {
        optional A file_options = 5000;
      }
      option (file_options).b.i = 42;
      )schema");

  Run("protocol_compiler --plug_out=$tmpdir --proto_path=$tmpdir foo.proto");
  // We have target constraint violations at two different edges in the file
  // options, so let's make sure both are caught.
  ExpectErrorSubstring(
      "Option protobuf_unittest.A.b cannot be set on an entity of type `file`.");
  ExpectErrorSubstring(
      "Option protobuf_unittest.B.i cannot be set on an entity of type `file`.");
}


TEST_F(CommandLineInterfaceTest, ExtensionDeclarationEnforcement) {
  CreateTempFile("foo.proto", R"schema(
    syntax = "proto2";
    package foo;
    message Foo {
      extensions 4000 to max [
        declaration = {
          number: 5000,
          full_name: ".foo.o"
          type: "int32"
        },
        declaration = {
          number: 9000,
          full_name: ".baz.z"
          type: ".foo.Bar"
      }];
    }

    extend Foo {
      optional int32 o = 5000;
      repeated int32 i = 9000;
    })schema");

  Run("protocol_compiler --test_out=$tmpdir --proto_path=$tmpdir foo.proto");
  ExpectErrorSubstring(
      "extension field 9000 is expected to be type \".foo.Bar\", not "
      "\"int32\"");
  ExpectErrorSubstring(
      "extension field 9000 is expected to have field name \".baz.z\", not "
      "\".foo.i\"");
  ExpectErrorSubstring("extension field 9000 is expected to be optional");
}

TEST_F(CommandLineInterfaceTest, ExtensionDeclarationDuplicateNames) {
  CreateTempFile("foo.proto", R"schema(
    syntax = "proto2";
    package foo;
    message Foo {
      extensions 4000 to max [
        declaration = {
          number: 5000,
          full_name: ".foo.o"
          type: "int32"
        },
        declaration = {
          number: 9000,
          full_name: ".foo.o"
          type: ".foo.Bar"
      }];
    })schema");

  Run("protocol_compiler --test_out=$tmpdir --proto_path=$tmpdir foo.proto");
  ExpectErrorSubstring(
      "Extension field name \".foo.o\" is declared multiple times");
}

TEST_F(CommandLineInterfaceTest, ExtensionDeclarationDuplicateNumbers) {
  CreateTempFile("foo.proto", R"schema(
    syntax = "proto2";
    package foo;
    message Foo {
      extensions 4000 to max [
        declaration = {
          number: 5000,
          full_name: ".foo.o"
          type: "int32"
        },
        declaration = {
          number: 5000,
          full_name: ".foo.o"
          type: ".foo.Bar"
      }];
    })schema");

  Run("protocol_compiler --test_out=$tmpdir --proto_path=$tmpdir foo.proto");
  ExpectErrorSubstring(
      "Extension declaration number 5000 is declared multiple times");
}

TEST_F(CommandLineInterfaceTest, ExtensionDeclarationCannotUseReservedNumber) {
  CreateTempFile("foo.proto", R"schema(
    syntax = "proto2";
    package foo;
    message Foo {
      extensions 4000 to max [
        declaration = {
          number: 5000,
          reserved: true
          full_name: ".foo.o"
          type: "int32"
        }];
    }

    extend Foo {
      optional int32 o = 5000;
    })schema");

  Run("protocol_compiler --test_out=$tmpdir --proto_path=$tmpdir foo.proto");
  ExpectErrorSubstring(
      "Cannot use number 5000 for extension field foo.o, as it is reserved in "
      "the extension declarations for message foo.Foo.");
}

TEST_F(CommandLineInterfaceTest,
       ExtensionDeclarationReservedMissingOneOfNameAndType) {
  CreateTempFile("foo.proto", R"schema(
    syntax = "proto2";
    package foo;
    message Foo {
      extensions 4000 to max [
        declaration = {
          number: 5000,
          reserved: true
          type: "int32"
        }];
    })schema");

  Run("protocol_compiler --test_out=$tmpdir --proto_path=$tmpdir foo.proto");
  ExpectErrorSubstring(
      "Extension declaration #5000 should have both \"full_name\" and \"type\" "
      "set");
}

TEST_F(CommandLineInterfaceTest, ExtensionDeclarationMissingBothNameAndType) {
  CreateTempFile("foo.proto", R"schema(
    syntax = "proto2";
    package foo;
    message Foo {
      extensions 4000 to max [
        declaration = {
          number: 6000
        }];
    })schema");

  Run("protocol_compiler --test_out=$tmpdir --proto_path=$tmpdir foo.proto");
  ExpectErrorSubstring(
      "Extension declaration #6000 should have both \"full_name\" and \"type\" "
      "set");
}

TEST_F(CommandLineInterfaceTest,
       ExtensionDeclarationReservedOptionalNameAndType) {
  CreateTempFile("foo.proto", R"schema(
    syntax = "proto2";
    package foo;
    message Foo {
      extensions 4000 to max [
        declaration = {
          number: 5000,
          reserved: true
          full_name: ".foo.o"
          type: "int32"
        },
        declaration = {
          number: 9000,
          reserved: true
        }];
    })schema");

  Run("protocol_compiler --test_out=$tmpdir --proto_path=$tmpdir foo.proto");
  ExpectNoErrors();
}

TEST_F(CommandLineInterfaceTest,
       ExtensionDeclarationRequireDeclarationsForAll) {
  CreateTempFile("foo.proto", R"schema(
    syntax = "proto2";
    package foo;
    message Foo {
      extensions 4000 to max [ declaration = {
          number: 5000,
          full_name: ".foo.o"
          type: "int32"
        }];
    }

    extend Foo {
      optional int32 o = 5000;
      repeated int32 i = 9000;
    })schema");

  Run("protocol_compiler --test_out=$tmpdir --proto_path=$tmpdir foo.proto");
  ExpectErrorSubstring(
      "Missing extension declaration for field foo.i with number 9000 in "
      "extendee message foo.Foo");
}

TEST_F(CommandLineInterfaceTest,
       ExtensionDeclarationVerificationDeclarationUndeclaredError) {
  CreateTempFile("foo.proto", R"schema(
    syntax = "proto2";
    package foo;
    message Foo {
      extensions 4000 to max [verification = DECLARATION];
    }
    extend Foo {
      optional string my_field = 5000;
    })schema");

  Run("protocol_compiler --test_out=$tmpdir --proto_path=$tmpdir foo.proto");
  ExpectErrorSubstring(
      "Missing extension declaration for field foo.my_field with number 5000 "
      "in extendee message foo.Foo");
}

TEST_F(CommandLineInterfaceTest,
       ExtensionDeclarationVerificationDeclarationDeclaredCompile) {
  CreateTempFile("foo.proto", R"schema(
    syntax = "proto2";
    package foo;
    message Foo {
      extensions 4000 to max [
        verification = DECLARATION,
        declaration = {
          number: 5000,
          full_name: ".foo.my_field",
          type: "string"
      }];
    }
    extend Foo {
      optional string my_field = 5000;
    })schema");

  Run("protocol_compiler --test_out=$tmpdir --proto_path=$tmpdir foo.proto");
  ExpectNoErrors();
}

TEST_F(CommandLineInterfaceTest,
       ExtensionDeclarationUnverifiedWithDeclarationsError) {
  CreateTempFile("foo.proto", R"schema(
    syntax = "proto2";
    package foo;
    message Foo {
      extensions 4000 to max [
        verification = UNVERIFIED,
        declaration = {
          number: 5000,
          full_name: "foo.my_field",
          type: "string"
        }];
    }
    extend Foo {
      optional string my_field = 5000;
    })schema");

  Run("protocol_compiler --test_out=$tmpdir --proto_path=$tmpdir foo.proto");
  ExpectErrorSubstring(
      "Cannot mark the extension range as UNVERIFIED when it has extension(s) "
      "declared.");
}

TEST_F(CommandLineInterfaceTest,
       ExtensionDeclarationDefaultUnverifiedEmptyRange) {
  CreateTempFile("foo.proto", R"schema(
    syntax = "proto2";
    package foo;
    message Foo {
      extensions 4000 to max;
    }
    extend Foo {
      optional string my_field = 5000;
    })schema");

  Run("protocol_compiler --test_out=$tmpdir --proto_path=$tmpdir foo.proto");
  ExpectNoErrors();
}

// Returns true if x is a prefix of y.
bool IsPrefix(absl::Span<const int> x, absl::Span<const int> y) {
  return x.size() <= y.size() && x == y.subspan(0, x.size());
}

TEST_F(CommandLineInterfaceTest, SourceInfoOptionRetention) {
  CreateTempFile("foo.proto",
                 "syntax = \"proto2\";\n"
                 "message Foo {\n"
                 "  extensions 1000 to max [\n"
                 "    declaration = {\n"
                 "      number: 1000\n"
                 "      full_name: \".video.cat_video\"\n"
                 "      type: \".video.CatVideo\"\n"
                 "  }];\n"
                 "}\n");

  Run("protocol_compiler --descriptor_set_out=$tmpdir/descriptor_set "
      "--include_source_info --proto_path=$tmpdir foo.proto");

  ExpectNoErrors();

  FileDescriptorSet descriptor_set;
  ReadDescriptorSet("descriptor_set", &descriptor_set);
  if (HasFatalFailure()) return;
  ASSERT_EQ(descriptor_set.file_size(), 1);
  EXPECT_EQ(descriptor_set.file(0).name(), "foo.proto");

  // Everything starting with this path should be have been stripped from the
  // source code info.
  const int declaration_option_path[] = {
      FileDescriptorProto::kMessageTypeFieldNumber,
      0,
      DescriptorProto::kExtensionRangeFieldNumber,
      0,
      DescriptorProto::ExtensionRange::kOptionsFieldNumber,
      ExtensionRangeOptions::kDeclarationFieldNumber};

  const SourceCodeInfo& source_code_info =
      descriptor_set.file(0).source_code_info();
  EXPECT_GT(source_code_info.location_size(), 0);
  for (const SourceCodeInfo::Location& location : source_code_info.location()) {
    EXPECT_FALSE(IsPrefix(declaration_option_path, location.path()));
  }
}

// ===================================================================

// Test for --encode and --decode.  Note that it would be easier to do this
// test as a shell script, but we'd like to be able to run the test on
// platforms that don't have a Bourne-compatible shell available (especially
// Windows/MSVC).

enum EncodeDecodeTestMode { PROTO_PATH, DESCRIPTOR_SET_IN };

class EncodeDecodeTest : public testing::TestWithParam<EncodeDecodeTestMode> {
 protected:
  void SetUp() override {
    WriteUnittestProtoDescriptorSet();
    duped_stdin_ = dup(STDIN_FILENO);
  }

  void TearDown() override {
    dup2(duped_stdin_, STDIN_FILENO);
    close(duped_stdin_);
  }

  void RedirectStdinFromText(const std::string& input) {
    std::string filename = absl::StrCat(TestTempDir(), "/test_stdin");
    ABSL_CHECK_OK(File::SetContents(filename, input, true));
    ABSL_CHECK(RedirectStdinFromFile(filename));
  }

  bool RedirectStdinFromFile(const std::string& filename) {
    int fd = open(filename.c_str(), O_RDONLY);
    if (fd < 0) return false;
    dup2(fd, STDIN_FILENO);
    close(fd);
    return true;
  }

  // Remove '\r' characters from text.
  std::string StripCR(const std::string& text) {
    std::string result;

    for (size_t i = 0; i < text.size(); ++i) {
      if (text[i] != '\r') {
        result.push_back(text[i]);
      }
    }

    return result;
  }

  enum Type { TEXT, BINARY };
  enum ReturnCode { SUCCESS, ERROR };

  bool Run(const std::string& command, bool specify_proto_files = true) {
    std::vector<std::string> args;
    args.push_back("protoc");
    for (absl::string_view split_piece :
         absl::StrSplit(command, ' ', absl::SkipEmpty())) {
      args.push_back(std::string(split_piece));
    }
    if (specify_proto_files) {
      switch (GetParam()) {
        case PROTO_PATH:
          args.push_back(
              absl::StrCat("--proto_path=", TestUtil::TestSourceDir()));
          break;
        case DESCRIPTOR_SET_IN:
          args.push_back(absl::StrCat("--descriptor_set_in=",
                                      unittest_proto_descriptor_set_filename_));
          break;
        default:
          ADD_FAILURE() << "unexpected EncodeDecodeTestMode: " << GetParam();
      }
    }

    std::unique_ptr<const char*[]> argv(new const char*[args.size()]);
    for (size_t i = 0; i < args.size(); ++i) {
      argv[i] = args[i].c_str();
    }

    CommandLineInterface cli;

    CaptureTestStdout();
    CaptureTestStderr();

    int result = cli.Run(args.size(), argv.get());

    captured_stdout_ = GetCapturedTestStdout();
    captured_stderr_ = GetCapturedTestStderr();

    return result == 0;
  }

  void ExpectStdoutMatchesBinaryFile(const std::string& filename) {
    std::string expected_output;
    ABSL_CHECK_OK(
        File::GetContents(filename, &expected_output, true));

    // Don't use EXPECT_EQ because we don't want to print raw binary data to
    // stdout on failure.
    EXPECT_TRUE(captured_stdout_ == expected_output);
  }

  void ExpectStdoutMatchesTextFile(const std::string& filename) {
    std::string expected_output;
    ABSL_CHECK_OK(
        File::GetContents(filename, &expected_output, true));

    ExpectStdoutMatchesText(expected_output);
  }

  void ExpectStdoutMatchesText(const std::string& expected_text) {
    EXPECT_EQ(StripCR(expected_text), StripCR(captured_stdout_));
  }

  void ExpectStderrMatchesText(const std::string& expected_text) {
    EXPECT_EQ(StripCR(expected_text), StripCR(captured_stderr_));
  }

  void ExpectStderrContainsText(const std::string& expected_text) {
    EXPECT_NE(StripCR(captured_stderr_).find(StripCR(expected_text)),
              std::string::npos);
  }

 private:
  void WriteUnittestProtoDescriptorSet() {
    unittest_proto_descriptor_set_filename_ =
        absl::StrCat(TestTempDir(), "/unittest_proto_descriptor_set.bin");
    FileDescriptorSet file_descriptor_set;
    protobuf_unittest::TestAllTypes test_all_types;
    test_all_types.descriptor()->file()->CopyTo(file_descriptor_set.add_file());

    protobuf_unittest_import::ImportMessage import_message;
    import_message.descriptor()->file()->CopyTo(file_descriptor_set.add_file());

    protobuf_unittest_import::PublicImportMessage public_import_message;
    public_import_message.descriptor()->file()->CopyTo(
        file_descriptor_set.add_file());
    ABSL_DCHECK(file_descriptor_set.IsInitialized());

    std::string binary_proto;
    ABSL_CHECK(file_descriptor_set.SerializeToString(&binary_proto));
    ABSL_CHECK_OK(File::SetContents(unittest_proto_descriptor_set_filename_,
                                    binary_proto, true));
  }

  int duped_stdin_;
  std::string captured_stdout_;
  std::string captured_stderr_;
  std::string unittest_proto_descriptor_set_filename_;
};

TEST_P(EncodeDecodeTest, Encode) {
  RedirectStdinFromFile(TestUtil::GetTestDataPath(
      "google/protobuf/"
      "testdata/text_format_unittest_data_oneof_implemented.txt"));
  std::string args;
  if (GetParam() != DESCRIPTOR_SET_IN) {
    args.append("google/protobuf/unittest.proto");
  }
  EXPECT_TRUE(
      Run(absl::StrCat(args, " --encode=protobuf_unittest.TestAllTypes")));
  ExpectStdoutMatchesBinaryFile(TestUtil::GetTestDataPath(
      "google/protobuf/testdata/golden_message_oneof_implemented"));
  ExpectStderrMatchesText("");
}

TEST_P(EncodeDecodeTest, Decode) {
  RedirectStdinFromFile(TestUtil::GetTestDataPath(
      "google/protobuf/testdata/golden_message_oneof_implemented"));
  EXPECT_TRUE(
      Run("google/protobuf/unittest.proto"
          " --decode=protobuf_unittest.TestAllTypes"));
  ExpectStdoutMatchesTextFile(TestUtil::GetTestDataPath(
      "google/protobuf/"
      "testdata/text_format_unittest_data_oneof_implemented.txt"));
  ExpectStderrMatchesText("");
}

TEST_P(EncodeDecodeTest, Partial) {
  RedirectStdinFromText("");
  EXPECT_TRUE(
      Run("google/protobuf/unittest.proto"
          " --encode=protobuf_unittest.TestRequired"));
  ExpectStdoutMatchesText("");
  ExpectStderrMatchesText(
      "warning:  Input message is missing required fields:  a, b, c\n");
}

TEST_P(EncodeDecodeTest, DecodeRaw) {
  protobuf_unittest::TestAllTypes message;
  message.set_optional_int32(123);
  message.set_optional_string("foo");
  std::string data;
  message.SerializeToString(&data);

  RedirectStdinFromText(data);
  EXPECT_TRUE(Run("--decode_raw", /*specify_proto_files=*/false));
  ExpectStdoutMatchesText(
      "1: 123\n"
      "14: \"foo\"\n");
  ExpectStderrMatchesText("");
}

TEST_P(EncodeDecodeTest, UnknownType) {
  EXPECT_FALSE(
      Run("google/protobuf/unittest.proto"
          " --encode=NoSuchType"));
  ExpectStdoutMatchesText("");
  ExpectStderrMatchesText("Type not defined: NoSuchType\n");
}

TEST_P(EncodeDecodeTest, ProtoParseError) {
  EXPECT_FALSE(
      Run("net/proto2/internal/no_such_file.proto "
          "--encode=NoSuchType"));
  ExpectStdoutMatchesText("");
  ExpectStderrContainsText(
      "net/proto2/internal/no_such_file.proto: No such file or directory\n");
}

TEST_P(EncodeDecodeTest, EncodeDeterministicOutput) {
  RedirectStdinFromFile(TestUtil::GetTestDataPath(
      "google/protobuf/"
      "testdata/text_format_unittest_data_oneof_implemented.txt"));
  std::string args;
  if (GetParam() != DESCRIPTOR_SET_IN) {
    args.append("google/protobuf/unittest.proto");
  }
  EXPECT_TRUE(Run(absl::StrCat(
      args, " --encode=protobuf_unittest.TestAllTypes --deterministic_output")));
  ExpectStdoutMatchesBinaryFile(TestUtil::GetTestDataPath(
      "google/protobuf/testdata/golden_message_oneof_implemented"));
  ExpectStderrMatchesText("");
}

TEST_P(EncodeDecodeTest, DecodeDeterministicOutput) {
  RedirectStdinFromFile(TestUtil::GetTestDataPath(
      "google/protobuf/testdata/golden_message_oneof_implemented"));
  EXPECT_FALSE(
      Run("google/protobuf/unittest.proto"
          " --decode=protobuf_unittest.TestAllTypes --deterministic_output"));
  ExpectStderrMatchesText(
      "Can only use --deterministic_output with --encode.\n");
}

INSTANTIATE_TEST_SUITE_P(FileDescriptorSetSource, EncodeDecodeTest,
                         testing::Values(PROTO_PATH, DESCRIPTOR_SET_IN));
}  // anonymous namespace

#endif  // !GOOGLE_PROTOBUF_HEAP_CHECK_DRACONIAN

#include "google/protobuf/port_undef.inc"

}  // namespace compiler
}  // namespace protobuf
}  // namespace google
