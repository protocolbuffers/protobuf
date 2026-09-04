// Protocol Buffers - Google's data interchange format
// Copyright 2014 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/csharp/csharp_generator.h"

#include <memory>

#include "google/protobuf/any.pb.h"
#include <gtest/gtest.h>
#include "google/protobuf/compiler/command_line_interface.h"
#include "google/protobuf/compiler/command_line_interface_tester.h"
#include "google/protobuf/compiler/csharp/csharp_helpers.h"
#include "google/protobuf/descriptor.h"
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

TEST(CSharpWrapperType, IsWrapperTypeIgnoresDefiningFileName) {
  struct WrapperType {
    const char* message_name;
    const char* full_name;
    const char* field_name;
    FieldDescriptorProto::Type value_type;
  };
  constexpr WrapperType kWrapperTypes[] = {
      {"DoubleValue", ".google.protobuf.DoubleValue", "double_value",
       FieldDescriptorProto::TYPE_DOUBLE},
      {"FloatValue", ".google.protobuf.FloatValue", "float_value",
       FieldDescriptorProto::TYPE_FLOAT},
      {"Int64Value", ".google.protobuf.Int64Value", "int64_value",
       FieldDescriptorProto::TYPE_INT64},
      {"UInt64Value", ".google.protobuf.UInt64Value", "uint64_value",
       FieldDescriptorProto::TYPE_UINT64},
      {"Int32Value", ".google.protobuf.Int32Value", "int32_value",
       FieldDescriptorProto::TYPE_INT32},
      {"UInt32Value", ".google.protobuf.UInt32Value", "uint32_value",
       FieldDescriptorProto::TYPE_UINT32},
      {"StringValue", ".google.protobuf.StringValue", "string_value",
       FieldDescriptorProto::TYPE_STRING},
      {"BytesValue", ".google.protobuf.BytesValue", "bytes_value",
       FieldDescriptorProto::TYPE_BYTES},
      {"BoolValue", ".google.protobuf.BoolValue", "bool_value",
       FieldDescriptorProto::TYPE_BOOL},
  };

  DescriptorPool pool;
  FileDescriptorProto wrappers_file;
  wrappers_file.set_name("Google/protobuf/wrappers.proto");
  wrappers_file.set_package("google.protobuf");
  wrappers_file.set_syntax("proto3");
  for (const auto& wrapper : kWrapperTypes) {
    DescriptorProto* wrapper_message = wrappers_file.add_message_type();
    wrapper_message->set_name(wrapper.message_name);
    FieldDescriptorProto* value = wrapper_message->add_field();
    value->set_name("value");
    value->set_number(1);
    value->set_type(wrapper.value_type);
  }
  wrappers_file.add_message_type()->set_name("Any");
  ASSERT_NE(pool.BuildFile(wrappers_file), nullptr);

  FileDescriptorProto user_file;
  user_file.set_name("wrapper_fields.proto");
  user_file.set_package("test");
  user_file.set_syntax("proto3");
  user_file.add_dependency(wrappers_file.name());
  user_file.add_message_type()->set_name("StringValue");

  DescriptorProto* message = user_file.add_message_type();
  message->set_name("WrapperFields");
  int field_number = 1;
  for (const auto& wrapper : kWrapperTypes) {
    FieldDescriptorProto* field = message->add_field();
    field->set_name(wrapper.field_name);
    field->set_number(field_number++);
    field->set_type(FieldDescriptorProto::TYPE_MESSAGE);
    field->set_type_name(wrapper.full_name);
  }

  FieldDescriptorProto* non_wrapper = message->add_field();
  non_wrapper->set_name("not_a_wrapper");
  non_wrapper->set_number(field_number++);
  non_wrapper->set_type(FieldDescriptorProto::TYPE_MESSAGE);
  non_wrapper->set_type_name(".test.StringValue");

  FieldDescriptorProto* non_wrapper_wkt = message->add_field();
  non_wrapper_wkt->set_name("any");
  non_wrapper_wkt->set_number(field_number++);
  non_wrapper_wkt->set_type(FieldDescriptorProto::TYPE_MESSAGE);
  non_wrapper_wkt->set_type_name(".google.protobuf.Any");

  FieldDescriptorProto* scalar = message->add_field();
  scalar->set_name("scalar");
  scalar->set_number(field_number);
  scalar->set_type(FieldDescriptorProto::TYPE_STRING);

  const FileDescriptor* file = pool.BuildFile(user_file);
  ASSERT_NE(file, nullptr);
  const Descriptor* wrapper_fields =
      file->FindMessageTypeByName("WrapperFields");
  ASSERT_NE(wrapper_fields, nullptr);

  int field_index = 0;
  for (const auto& wrapper : kWrapperTypes) {
    EXPECT_TRUE(IsWrapperType(wrapper_fields->field(field_index++)))
        << wrapper.full_name;
  }
  EXPECT_FALSE(IsWrapperType(wrapper_fields->field(field_index++)));
  EXPECT_FALSE(IsWrapperType(wrapper_fields->field(field_index++)));
  EXPECT_FALSE(IsWrapperType(wrapper_fields->field(field_index)));
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

}  // namespace
}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
