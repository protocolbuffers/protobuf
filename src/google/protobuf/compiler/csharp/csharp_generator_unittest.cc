// Protocol Buffers - Google's data interchange format
// Copyright 2014 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/csharp/csharp_generator.h"

#include <cstddef>
#include <memory>
#include <string>

#include "google/protobuf/any.pb.h"
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/command_line_interface.h"
#include "google/protobuf/compiler/command_line_interface_tester.h"
#include "google/protobuf/compiler/csharp/csharp_helpers.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/zero_copy_stream.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {
namespace {

TEST(CSharpEnumValue, PascalCasedPrefixStripping) {
  EXPECT_EQ("Bar", GetEnumValueName("Foo", "BAR"));
  EXPECT_EQ("BarBaz", GetEnumValueName("Foo", "BAR_BAZ"));
  EXPECT_EQ("Bar", GetEnumValueName("Foo", "FOO_BAR"));
  EXPECT_EQ("Bar", GetEnumValueName("Foo", "FOO__BAR"));
  EXPECT_EQ("BarBaz", GetEnumValueName("Foo", "FOO_BAR_BAZ"));
  EXPECT_EQ("BarBaz", GetEnumValueName("Foo", "Foo_BarBaz"));
  EXPECT_EQ("Bar", GetEnumValueName("FO_O", "FOO_BAR"));
  EXPECT_EQ("Bar", GetEnumValueName("FOO", "F_O_O_BAR"));
  EXPECT_EQ("Bar", GetEnumValueName("Foo", "BAR"));
  EXPECT_EQ("BarBaz", GetEnumValueName("Foo", "BAR_BAZ"));
  EXPECT_EQ("Foo", GetEnumValueName("Foo", "FOO"));
  EXPECT_EQ("Foo", GetEnumValueName("Foo", "FOO___"));
  // Identifiers can't start with digits
  EXPECT_EQ("_2Bar", GetEnumValueName("Foo", "FOO_2_BAR"));
  EXPECT_EQ("_2", GetEnumValueName("Foo", "FOO___2"));
}

TEST(DescriptorProtoHelpers, IsDescriptorProto) {
  EXPECT_TRUE(IsDescriptorProto(DescriptorProto::descriptor()->file()));
  EXPECT_FALSE(IsDescriptorProto(google::protobuf::Any::descriptor()->file()));
}

TEST(DescriptorProtoHelpers, IsDescriptorOptionMessage) {
  EXPECT_TRUE(IsDescriptorOptionMessage(FileOptions::descriptor()));
  EXPECT_FALSE(IsDescriptorOptionMessage(google::protobuf::Any::descriptor()));
  EXPECT_FALSE(IsDescriptorOptionMessage(DescriptorProto::descriptor()));
}

TEST(CSharpIdentifiers, UnderscoresToCamelCase) {
	EXPECT_EQ("FooBar", UnderscoresToCamelCase("Foo_Bar", true));
	EXPECT_EQ("fooBar", UnderscoresToCamelCase("FooBar", false));
	EXPECT_EQ("foo123", UnderscoresToCamelCase("foo_123", false));
	// remove leading underscores
	EXPECT_EQ("Foo123", UnderscoresToCamelCase("_Foo_123", true));
	// this one has slight unexpected output as it capitalises the first
	// letter after consuming the underscores, but this was the existing
	// behaviour so I have not changed it
	EXPECT_EQ("FooBar", UnderscoresToCamelCase("___fooBar", false));
	// leave a leading underscore for identifiers that would otherwise
	// be invalid because they would start with a digit
	EXPECT_EQ("_123Foo", UnderscoresToCamelCase("_123_foo", true));
	EXPECT_EQ("_123Foo", UnderscoresToCamelCase("___123_foo", true));
}

class CSharpGeneratorCliTest : public CommandLineInterfaceTester {
 protected:
  CSharpGeneratorCliTest() {
    RegisterGenerator("--csharp_out", "--csharp_opt",
                      std::make_unique<Generator>(), "C# test generator");

    CreateTempFile("google/protobuf/descriptor.proto",
                   DescriptorProto::descriptor()->file()->DebugString());
  }
};

TEST_F(CSharpGeneratorCliTest, InvalidCSharpNamespaceRejected) {
  CreateTempFile("foo.proto",
                 R"schema(
    syntax = "proto3";
    option csharp_namespace = "MyApp.Models;System.Diagnostics.Process.Start(\"calc\");//";
    message Foo {
      int32 bar = 1;
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --csharp_out=$tmpdir foo.proto");

  ExpectErrorSubstring("Invalid character");
}

TEST_F(CSharpGeneratorCliTest, ValidCSharpNamespaceAccepted) {
  CreateTempFile("foo.proto",
                 R"schema(
    syntax = "proto3";
    option csharp_namespace = "MyApp.Models.V2";
    message Foo {
      int32 bar = 1;
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --csharp_out=$tmpdir foo.proto");

  ExpectNoErrors();
}

TEST_F(CSharpGeneratorCliTest, CSharpNamespaceSemicolonRejected) {
  CreateTempFile("foo.proto",
                 R"schema(
    syntax = "proto3";
    option csharp_namespace = "MyApp;System.Diagnostics.Process.Start(\"calc\")";
    message Foo {
      int32 bar = 1;
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --csharp_out=$tmpdir foo.proto");

  ExpectErrorSubstring("Invalid character");
}

TEST_F(CSharpGeneratorCliTest, CSharpNamespaceBracesRejected) {
  CreateTempFile("foo.proto",
                 R"schema(
    syntax = "proto3";
    option csharp_namespace = "MyApp}class Evil{static void Main(){";
    message Foo {
      int32 bar = 1;
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --csharp_out=$tmpdir foo.proto");

  ExpectErrorSubstring("Invalid character");
}

// Returns true if `text` occurs within a delimited comment in `content`, i.e.
// if the closest comment start before it is not closed before it.
bool IsInsideComment(absl::string_view content, absl::string_view text) {
  size_t text_pos = content.find(text);
  if (text_pos == absl::string_view::npos) {
    return false;
  }
  size_t comment_start = content.rfind("/*", text_pos);
  if (comment_start == absl::string_view::npos) {
    return false;
  }
  size_t comment_end = content.find("*/", comment_start);
  return comment_end != absl::string_view::npos && comment_end > text_pos;
}

TEST_F(CSharpGeneratorCliTest, DocCommentsAreDelimitedComments) {
  CreateTempFile("foo.proto",
                 R"schema(
    syntax = "proto3";
    // This is a message comment.
    //
    //     Indented content.
    message Foo {
      int32 bar = 1;
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --csharp_out=$tmpdir foo.proto");

  ExpectNoErrors();
  // Every line has exactly the same " * " prefix, including the blank one, so
  // that the C# compiler strips it back off without changing the indentation
  // of the original comment.
  ExpectFileContentContainsSubstring("Foo.cs",
                                     "/**\n"
                                     " * <summary>\n"
                                     " *  This is a message comment.\n"
                                     " * \n"
                                     " *      Indented content.\n"
                                     " * </summary>\n"
                                     " */\n");
}

TEST_F(CSharpGeneratorCliTest, DocCommentsCannotEscapeViaLineTerminators) {
  // C# treats CR, NEL, LS and PS as line terminators, but the .proto parser
  // only ends a comment at '\n', so all of these end up in the comment text.
  CreateTempFile(
      "foo.proto",
      "syntax = \"proto3\";\n"
      "// Carriage return: \r public class EvilCr { }\n"
      "// Next line: \xc2\x85 public class EvilNel { }\n"
      "// Line separator: \xe2\x80\xa8 public class EvilLs { }\n"
      "// Paragraph separator: \xe2\x80\xa9 public class EvilPs { }\n"
      "message Foo {\n"
      "  int32 bar = 1;\n"
      "}\n");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --csharp_out=$tmpdir foo.proto");

  ExpectNoErrors();
  std::string content = FileContents("Foo.cs");
  EXPECT_TRUE(IsInsideComment(content, "class EvilCr"));
  EXPECT_TRUE(IsInsideComment(content, "class EvilNel"));
  EXPECT_TRUE(IsInsideComment(content, "class EvilLs"));
  EXPECT_TRUE(IsInsideComment(content, "class EvilPs"));
}

TEST_F(CSharpGeneratorCliTest, DocCommentsCannotEscapeViaCommentTerminator) {
  CreateTempFile("foo.proto",
                 R"schema(
    syntax = "proto3";
    // Not the end: */ public class Evil { }
    message Foo {
      int32 bar = 1;
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --csharp_out=$tmpdir foo.proto");

  ExpectNoErrors();
  ExpectFileContentContainsSubstring(
      "Foo.cs", "Not the end: *&#47; public class Evil { }");
  EXPECT_TRUE(IsInsideComment(FileContents("Foo.cs"), "class Evil"));
}

}  // namespace
}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
