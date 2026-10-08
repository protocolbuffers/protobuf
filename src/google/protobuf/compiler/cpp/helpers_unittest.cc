// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/cpp/helpers.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/cpp_file_options.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/test_textproto.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest_proto3.pb.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
namespace {

using ::proto2_unittest::TestAllTypes;
using ::proto2_unittest::TestEmptyMessage;
using ::proto2_unittest::TestEmptyMessageWithExtensions;
using ::proto2_unittest::TestVerifyInt32;
using ::proto2_unittest::TestVerifyInt32BigFieldNumber;
using ::proto2_unittest::TestVerifyInt32Simple;
using ::proto2_unittest::TestVerifyMostlyInt32;
using ::proto2_unittest::TestVerifyMostlyInt32BigFieldNumber;
using ::proto2_unittest::TestVerifyOneInt32BigFieldNumber;
using ::proto2_unittest::TestVerifyOneUint32;
using ::proto2_unittest::TestVerifyUint32;
using ::proto2_unittest::TestVerifyUint32BigFieldNumber;
using ::proto2_unittest::TestVerifyUint32Simple;

TEST(EagerVerifyHelperTest, VerifyInt32Never) {
  EXPECT_EQ(ShouldVerifySimple(TestVerifyUint32::GetDescriptor()),
            VerifySimpleType::kCustomInt32Never);

  EXPECT_EQ(ShouldVerifySimple(TestVerifyUint32BigFieldNumber::GetDescriptor()),
            VerifySimpleType::kCustomInt32Never);

  EXPECT_EQ(ShouldVerifySimple(TestVerifyUint32Simple::GetDescriptor()),
            VerifySimpleType::kSimpleInt32Never);
}

TEST(EagerVerifyHelperTest, VerifyInt32Always) {
  EXPECT_EQ(
      ShouldVerifySimple(TestVerifyMostlyInt32BigFieldNumber::GetDescriptor()),
      VerifySimpleType::kCustomInt32Always);

  EXPECT_EQ(ShouldVerifySimple(TestVerifyInt32::GetDescriptor()),
            VerifySimpleType::kCustomInt32Always);

  EXPECT_EQ(ShouldVerifySimple(TestVerifyInt32BigFieldNumber::GetDescriptor()),
            VerifySimpleType::kCustomInt32Always);

  EXPECT_EQ(ShouldVerifySimple(TestVerifyInt32Simple::GetDescriptor()),
            VerifySimpleType::kSimpleInt32Always);
}

TEST(EagerVerifyHelperTest, VerifyInt32Custom) {
  EXPECT_EQ(ShouldVerifySimple(TestVerifyMostlyInt32::GetDescriptor()),
            VerifySimpleType::kCustom);

  EXPECT_EQ(ShouldVerifySimple(TestVerifyOneUint32::GetDescriptor()),
            VerifySimpleType::kCustom);

  EXPECT_EQ(
      ShouldVerifySimple(TestVerifyOneInt32BigFieldNumber::GetDescriptor()),
      VerifySimpleType::kCustom);
}


TEST(SimpleBaseClassTest, NotSimpleMessage) {
  EXPECT_EQ(SimpleBaseClass(TestAllTypes::GetDescriptor(), Options()), "");
  EXPECT_FALSE(HasSimpleBaseClass(TestAllTypes::GetDescriptor(), Options()));
}

TEST(SimpleBaseClassTest, EmptyMessage) {
  EXPECT_EQ(SimpleBaseClass(TestEmptyMessage::GetDescriptor(), Options()),
            "ZeroFieldsBase");
  EXPECT_TRUE(HasSimpleBaseClass(TestEmptyMessage::GetDescriptor(), Options()));
}

TEST(SimpleBaseClassTest, EmptyMessageLiteRuntime) {
  Options options;
  options.enforce_mode = EnforceOptimizeMode::kLiteRuntime;
  EXPECT_EQ(SimpleBaseClass(TestEmptyMessage::GetDescriptor(), options), "");
  EXPECT_FALSE(HasSimpleBaseClass(TestEmptyMessage::GetDescriptor(), options));
}

TEST(SimpleBaseClassTest, EmptyMessageWithFieldInjection) {
  Options options;
  options.field_listener_options.inject_field_listener_events = true;
  EXPECT_EQ(SimpleBaseClass(TestEmptyMessage::GetDescriptor(), options), "");
  EXPECT_FALSE(HasSimpleBaseClass(TestEmptyMessage::GetDescriptor(), options));
}

TEST(SimpleBaseClassTest, EmptyMessageWithExtension) {
  EXPECT_EQ(SimpleBaseClass(TestEmptyMessageWithExtensions::GetDescriptor(),
                            Options()),
            "");
  EXPECT_FALSE(HasSimpleBaseClass(
      TestEmptyMessageWithExtensions::GetDescriptor(), Options()));
}


TEST(Utf8Test, Proto2StringIsRaw) {
  const FieldDescriptor* field =
      ::proto2_unittest::TestAllTypes::GetDescriptor()->FindFieldByName(
          "optional_string");

  EXPECT_FALSE(IsStrictUtf8String(field, {}));
}

TEST(Utf8Test, Proto3StringIsStrictlyUtf8) {
  const FieldDescriptor* field =
      ::proto3_unittest::TestAllTypes::GetDescriptor()->FindFieldByName(
          "optional_string");

  EXPECT_TRUE(IsStrictUtf8String(field, {}));
}

template <typename MessageT>
bool IsFieldDescriptorRepeatedPtrField(absl::string_view name) {
  const auto* descriptor = MessageT::descriptor();
  const auto* field = descriptor->FindFieldByName(name);
  return compiler::cpp::IsRepeatedPtrField(field);
}

TEST(FieldDescriptorTest, IsRepeatedPtrField) {
  EXPECT_FALSE(IsFieldDescriptorRepeatedPtrField<proto2_unittest::TestAllTypes>(
      "repeated_int32"));
  EXPECT_FALSE(IsFieldDescriptorRepeatedPtrField<proto2_unittest::TestAllTypes>(
      "repeated_int32"));
  EXPECT_FALSE(IsFieldDescriptorRepeatedPtrField<proto2_unittest::TestAllTypes>(
      "repeated_int64"));
  EXPECT_FALSE(IsFieldDescriptorRepeatedPtrField<proto2_unittest::TestAllTypes>(
      "repeated_uint32"));
  EXPECT_FALSE(IsFieldDescriptorRepeatedPtrField<proto2_unittest::TestAllTypes>(
      "repeated_uint64"));
  EXPECT_FALSE(IsFieldDescriptorRepeatedPtrField<proto2_unittest::TestAllTypes>(
      "repeated_sint32"));
  EXPECT_FALSE(IsFieldDescriptorRepeatedPtrField<proto2_unittest::TestAllTypes>(
      "repeated_sint64"));
  EXPECT_FALSE(IsFieldDescriptorRepeatedPtrField<proto2_unittest::TestAllTypes>(
      "repeated_fixed32"));
  EXPECT_FALSE(IsFieldDescriptorRepeatedPtrField<proto2_unittest::TestAllTypes>(
      "repeated_fixed64"));
  EXPECT_FALSE(IsFieldDescriptorRepeatedPtrField<proto2_unittest::TestAllTypes>(
      "repeated_sfixed32"));
  EXPECT_FALSE(IsFieldDescriptorRepeatedPtrField<proto2_unittest::TestAllTypes>(
      "repeated_sfixed64"));
  EXPECT_FALSE(IsFieldDescriptorRepeatedPtrField<proto2_unittest::TestAllTypes>(
      "repeated_float"));
  EXPECT_FALSE(IsFieldDescriptorRepeatedPtrField<proto2_unittest::TestAllTypes>(
      "repeated_double"));
  EXPECT_FALSE(IsFieldDescriptorRepeatedPtrField<proto2_unittest::TestAllTypes>(
      "repeated_bool"));
  EXPECT_FALSE(IsFieldDescriptorRepeatedPtrField<proto2_unittest::TestAllTypes>(
      "repeated_nested_enum"));
  EXPECT_FALSE(IsFieldDescriptorRepeatedPtrField<proto2_unittest::TestAllTypes>(
      "repeated_foreign_enum"));
  EXPECT_FALSE(IsFieldDescriptorRepeatedPtrField<proto2_unittest::TestAllTypes>(
      "repeated_import_enum"));
  // Repeated Cord is only supported internally; in open-source it falls back
  // to RepeatedPtrField<std::string>.
  EXPECT_TRUE(IsFieldDescriptorRepeatedPtrField<proto2_unittest::TestAllTypes>(
      "repeated_cord"));

  EXPECT_TRUE(IsFieldDescriptorRepeatedPtrField<proto2_unittest::TestAllTypes>(
      "repeated_string"));
  EXPECT_TRUE(IsFieldDescriptorRepeatedPtrField<proto2_unittest::TestAllTypes>(
      "repeated_bytes"));
  EXPECT_TRUE(IsFieldDescriptorRepeatedPtrField<proto2_unittest::TestAllTypes>(
      "repeatedgroup"));
  EXPECT_TRUE(IsFieldDescriptorRepeatedPtrField<proto2_unittest::TestAllTypes>(
      "repeated_nested_message"));
  EXPECT_TRUE(IsFieldDescriptorRepeatedPtrField<proto2_unittest::TestAllTypes>(
      "repeated_foreign_message"));
  EXPECT_TRUE(IsFieldDescriptorRepeatedPtrField<proto2_unittest::TestAllTypes>(
      "repeated_import_message"));
  EXPECT_TRUE(IsFieldDescriptorRepeatedPtrField<proto2_unittest::TestAllTypes>(
      "repeated_string_piece"));
  EXPECT_TRUE(IsFieldDescriptorRepeatedPtrField<proto2_unittest::TestAllTypes>(
      "repeated_lazy_message"));
}

TEST(NamespaceTest, InvalidCharacterInNamespace) {
  ASSERT_NE(pb::file::CppFileOptions::descriptor(), nullptr);
  FileDescriptorProto file_proto = ParseTextOrDie(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_UNSTABLE
    option_dependency: "google/protobuf/cpp_file_options.proto"
    options {
      [pb.file.cpp] { namespace: "foo:test" }
    }
    message_type {
      name: "TestString"
      field { name: "string" number: 1 }
    }
  )pb");

  DescriptorPool pool;
  const FileDescriptor* file = pool.BuildFile(file_proto);
  ASSERT_NE(file, nullptr);
  std::string error;
  EXPECT_FALSE(ValidateCcNamespace(file, &error));
  EXPECT_EQ(error, "Namespace foo:test contains invalid characters.");
}


TEST(NamespaceTest, InvalidOnlyFullyQualifiedSymbolNamespace) {
  ASSERT_NE(pb::file::CppFileOptions::descriptor(), nullptr);
  FileDescriptorProto file_proto = ParseTextOrDie(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_UNSTABLE
    option_dependency: "google/protobuf/cpp_file_options.proto"
    options {
      [pb.file.cpp] { namespace: "::" }
    }
    message_type {
      name: "TestString"
      field { name: "string" number: 1 }
    }
  )pb");

  DescriptorPool pool;
  const FileDescriptor* file = pool.BuildFile(file_proto);
  ASSERT_NE(file, nullptr);
  std::string error;
  EXPECT_FALSE(ValidateCcNamespace(file, &error));
  EXPECT_EQ(error, "Namespace :: can not start with `::`.");
}

TEST(NamespaceTest, ValidNamespace) {
  ASSERT_NE(pb::file::CppFileOptions::descriptor(), nullptr);
  FileDescriptorProto file_proto = ParseTextOrDie(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_UNSTABLE
    option_dependency: "google/protobuf/cpp_file_options.proto"
    options {
      [pb.file.cpp] { namespace: "foo::test_1" }
    }
    message_type {
      name: "TestString"
      field { name: "string" number: 1 }
    }
  )pb");

  DescriptorPool pool;
  const FileDescriptor* file = pool.BuildFile(file_proto);
  ASSERT_NE(file, nullptr);
  std::string error;
  EXPECT_TRUE(ValidateCcNamespace(file, &error));
  EXPECT_EQ(error, "");
  EXPECT_EQ(Namespace(file), "::foo::test_1");
}

}  // namespace
}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
