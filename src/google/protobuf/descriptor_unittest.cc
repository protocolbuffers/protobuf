// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// This file makes extensive use of RFC 3092.  :)

#include "google/protobuf/descriptor.h"

#include <limits.h>

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <ostream>
#include <string>
#include <thread>  // NOLINT
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "google/protobuf/any.pb.h"
#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/base/log_severity.h"
#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_set.h"
#include "absl/flags/flag.h"
#include "absl/functional/any_invocable.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/log/die_if_null.h"
#include "absl/log/scoped_mock_log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/match.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "absl/strings/substitute.h"
#include "absl/synchronization/notification.h"
#include "google/protobuf/compiler/importer.h"
#include "google/protobuf/compiler/parser.h"
#include "google/protobuf/cpp_features.pb.h"
#include "google/protobuf/descriptor_database.h"
#include "google/protobuf/descriptor_legacy.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/feature_resolver.h"
#include "google/protobuf/internal_feature_helper.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/tokenizer.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/test_textproto.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest_custom_options.pb.h"
#include "google/protobuf/unittest_delimited.pb.h"
#include "google/protobuf/unittest_delimited_import.pb.h"
#include "google/protobuf/unittest_features.pb.h"
#include "google/protobuf/unittest_lazy_dependencies.pb.h"
#include "google/protobuf/unittest_lazy_dependencies_custom_option.pb.h"
#include "google/protobuf/unittest_lazy_dependencies_enum.pb.h"
#include "google/protobuf/unittest_proto3_arena.pb.h"
#include "google/protobuf/unittest_string_type.pb.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

using ::google::protobuf::internal::cpp::GetFieldHasbitMode;
using ::google::protobuf::internal::cpp::GetUtf8CheckMode;
using ::google::protobuf::internal::cpp::HasbitMode;
using ::google::protobuf::internal::cpp::HasHasbit;
using ::google::protobuf::internal::cpp::HasPreservingUnknownEnumSemantics;
using ::google::protobuf::internal::cpp::Utf8CheckMode;
using ::testing::AnyOf;
using ::testing::AtLeast;
using ::testing::ElementsAre;
using ::testing::HasSubstr;
using ::testing::NotNull;
using ::testing::Return;

absl::Status GetStatus(const absl::Status& s) { return s; }
template <typename T>
absl::Status GetStatus(const absl::StatusOr<T>& s) {
  return s.status();
}
MATCHER_P2(StatusIs, status, message,
           absl::StrCat(".status() is ", testing::PrintToString(status))) {
  return GetStatus(arg).code() == status &&
         testing::ExplainMatchResult(message, GetStatus(arg).message(),
                                     result_listener);
}
#define EXPECT_OK(x) EXPECT_THAT(x, StatusIs(absl::StatusCode::kOk, testing::_))
#define ASSERT_OK(x) ASSERT_THAT(x, StatusIs(absl::StatusCode::kOk, testing::_))

namespace google {
namespace protobuf {

// Can't use an anonymous namespace here due to brokenness of Tru64 compiler.
namespace descriptor_unittest {

// Some helpers to make assembling descriptors faster.
DescriptorProto* AddMessage(FileDescriptorProto* file,
                            const std::string& name) {
  DescriptorProto* result = file->add_message_type();
  result->set_name(name);
  return result;
}

DescriptorProto* AddNestedMessage(DescriptorProto* parent,
                                  const std::string& name) {
  DescriptorProto* result = parent->add_nested_type();
  result->set_name(name);
  return result;
}

EnumDescriptorProto* AddEnum(FileDescriptorProto* file,
                             absl::string_view name) {
  EnumDescriptorProto* result = file->add_enum_type();
  result->set_name(name);
  return result;
}

EnumDescriptorProto* AddNestedEnum(DescriptorProto* parent,
                                   const std::string& name) {
  EnumDescriptorProto* result = parent->add_enum_type();
  result->set_name(name);
  return result;
}

ServiceDescriptorProto* AddService(FileDescriptorProto* file,
                                   const std::string& name) {
  ServiceDescriptorProto* result = file->add_service();
  result->set_name(name);
  return result;
}

FieldDescriptorProto* AddField(DescriptorProto* parent, const std::string& name,
                               int number, FieldDescriptorProto::Label label,
                               FieldDescriptorProto::Type type) {
  FieldDescriptorProto* result = parent->add_field();
  result->set_name(name);
  result->set_number(number);
  result->set_label(label);
  result->set_type(type);
  return result;
}

FieldDescriptorProto* AddExtension(FileDescriptorProto* file,
                                   const std::string& extendee,
                                   const std::string& name, int number,
                                   FieldDescriptorProto::Label label,
                                   FieldDescriptorProto::Type type) {
  FieldDescriptorProto* result = file->add_extension();
  result->set_name(name);
  result->set_number(number);
  result->set_label(label);
  result->set_type(type);
  result->set_extendee(extendee);
  return result;
}

FieldDescriptorProto* AddNestedExtension(DescriptorProto* parent,
                                         const std::string& extendee,
                                         const std::string& name, int number,
                                         FieldDescriptorProto::Label label,
                                         FieldDescriptorProto::Type type) {
  FieldDescriptorProto* result = parent->add_extension();
  result->set_name(name);
  result->set_number(number);
  result->set_label(label);
  result->set_type(type);
  result->set_extendee(extendee);
  return result;
}

DescriptorProto::ExtensionRange* AddExtensionRange(DescriptorProto* parent,
                                                   int start, int end) {
  DescriptorProto::ExtensionRange* result = parent->add_extension_range();
  result->set_start(start);
  result->set_end(end);
  return result;
}

DescriptorProto::ReservedRange* AddReservedRange(DescriptorProto* parent,
                                                 int start, int end) {
  DescriptorProto::ReservedRange* result = parent->add_reserved_range();
  result->set_start(start);
  result->set_end(end);
  return result;
}

EnumDescriptorProto::EnumReservedRange* AddReservedRange(
    EnumDescriptorProto* parent, int start, int end) {
  EnumDescriptorProto::EnumReservedRange* result = parent->add_reserved_range();
  result->set_start(start);
  result->set_end(end);
  return result;
}

EnumValueDescriptorProto* AddEnumValue(EnumDescriptorProto* enum_proto,
                                       const std::string& name, int number) {
  EnumValueDescriptorProto* result = enum_proto->add_value();
  result->set_name(name);
  result->set_number(number);
  return result;
}

MethodDescriptorProto* AddMethod(ServiceDescriptorProto* service,
                                 const std::string& name,
                                 const std::string& input_type,
                                 const std::string& output_type) {
  MethodDescriptorProto* result = service->add_method();
  result->set_name(name);
  result->set_input_type(input_type);
  result->set_output_type(output_type);
  return result;
}

// Empty enums technically aren't allowed.  We need to insert a dummy value
// into them.
void AddEmptyEnum(FileDescriptorProto* file, absl::string_view name) {
  AddEnumValue(AddEnum(file, name), absl::StrCat(name, "_DUMMY"), 1);
}

class MockErrorCollector : public DescriptorPool::ErrorCollector {
 public:
  MockErrorCollector() = default;
  ~MockErrorCollector() override = default;

  std::string text_;
  std::string warning_text_;

  // implements ErrorCollector ---------------------------------------
  void RecordError(absl::string_view filename, absl::string_view element_name,
                   const Message* descriptor, ErrorLocation location,
                   absl::string_view message) override {
    absl::SubstituteAndAppend(&text_, "$0: $1: $2: $3\n", filename,
                              element_name, ErrorLocationName(location),
                              message);
  }

  // implements ErrorCollector ---------------------------------------
  void RecordWarning(absl::string_view filename, absl::string_view element_name,
                     const Message* descriptor, ErrorLocation location,
                     absl::string_view message) override {
    absl::SubstituteAndAppend(&warning_text_, "$0: $1: $2: $3\n", filename,
                              element_name, ErrorLocationName(location),
                              message);
  }
};

// ===================================================================

// Test simple files.
class FileDescriptorTest : public testing::Test {
 protected:
  void SetUp() override {
    // Build descriptors for the following definitions:
    //   // in "custom_option.proto"
    //   import "google/protobuf/descriptor.proto";
    //   extend google.protobuf.FileOptions { optional int32 file_opt = 5000; }
    //
    //   // in "foo.proto"
    //   message FooMessage { extensions 1; }
    //   enum FooEnum {FOO_ENUM_VALUE = 1;}
    //   service FooService {}
    //   extend FooMessage { optional int32 foo_extension = 1; }
    //
    //   // in "bar.proto"
    //   import "foo.proto";
    //   import option "custom_option.proto";
    //   edition = "2024";
    //   package bar_package;
    //   message BarMessage { extensions 1; }
    //   enum BarEnum {BAR_ENUM_VALUE = 1;}
    //   service BarService {}
    //   extend BarMessage { optional int32 bar_extension = 1; }
    //
    // Also, we have an empty file "baz.proto".  This file's purpose is to
    // make sure that even though it has the same package as foo.proto,
    // searching it for members of foo.proto won't work.

    FileDescriptorProto custom_option_file;
    custom_option_file.set_name("custom_option.proto");
    custom_option_file.add_dependency("google/protobuf/descriptor.proto");
    AddExtension(&custom_option_file, "google.protobuf.FileOptions", "file_opt", 5000,
                 FieldDescriptorProto::LABEL_OPTIONAL,
                 FieldDescriptorProto::TYPE_INT32);

    FileDescriptorProto foo_file;
    foo_file.set_name("foo.proto");
    AddExtensionRange(AddMessage(&foo_file, "FooMessage"), 1, 2);
    AddEnumValue(AddEnum(&foo_file, "FooEnum"), "FOO_ENUM_VALUE", 1);
    AddService(&foo_file, "FooService");
    AddExtension(&foo_file, "FooMessage", "foo_extension", 1,
                 FieldDescriptorProto::LABEL_OPTIONAL,
                 FieldDescriptorProto::TYPE_INT32);

    FileDescriptorProto bar_file;
    bar_file.set_name("bar.proto");
    bar_file.set_package("bar_package");
    bar_file.set_edition(google::protobuf::Edition::EDITION_2024);
    bar_file.add_dependency("foo.proto");
    bar_file.add_option_dependency("custom_option.proto");
    AddExtensionRange(AddMessage(&bar_file, "BarMessage"), 1, 2);
    EnumDescriptorProto* bar_enum = AddEnum(&bar_file, "BarEnum");
    AddEnumValue(bar_enum, "BAR_ENUM_UNKNOWN", 0);
    AddEnumValue(bar_enum, "BAR_ENUM_VALUE", 1);
    AddService(&bar_file, "BarService");
    AddExtension(&bar_file, "bar_package.BarMessage", "bar_extension", 1,
                 FieldDescriptorProto::LABEL_OPTIONAL,
                 FieldDescriptorProto::TYPE_INT32);

    FileDescriptorProto baz_file;
    baz_file.set_name("baz.proto");

    // Build the descriptors and get the pointers.
    FileDescriptorProto descriptor_proto;
    google::protobuf::DescriptorProto::descriptor()->file()->CopyTo(&descriptor_proto);
    pool_.BuildFile(descriptor_proto);

    custom_option_file_ = pool_.BuildFile(custom_option_file);
    ASSERT_TRUE(custom_option_file_ != nullptr);

    foo_file_ = pool_.BuildFile(foo_file);
    ASSERT_TRUE(foo_file_ != nullptr);

    bar_file_ = pool_.BuildFile(bar_file);
    ASSERT_TRUE(bar_file_ != nullptr);

    baz_file_ = pool_.BuildFile(baz_file);
    ASSERT_TRUE(baz_file_ != nullptr);

    ASSERT_EQ(1, foo_file_->message_type_count());
    foo_message_ = foo_file_->message_type(0);
    ASSERT_EQ(1, foo_file_->enum_type_count());
    foo_enum_ = foo_file_->enum_type(0);
    ASSERT_EQ(1, foo_enum_->value_count());
    foo_enum_value_ = foo_enum_->value(0);
    ASSERT_EQ(1, foo_file_->service_count());
    foo_service_ = foo_file_->service(0);
    ASSERT_EQ(1, foo_file_->extension_count());
    foo_extension_ = foo_file_->extension(0);

    ASSERT_EQ(1, bar_file_->message_type_count());
    bar_message_ = bar_file_->message_type(0);
    ASSERT_EQ(1, bar_file_->enum_type_count());
    bar_enum_ = bar_file_->enum_type(0);
    ASSERT_EQ(2, bar_enum_->value_count());
    bar_enum_value_ = bar_enum_->value(1);
    ASSERT_EQ(1, bar_file_->service_count());
    bar_service_ = bar_file_->service(0);
    ASSERT_EQ(1, bar_file_->extension_count());
    bar_extension_ = bar_file_->extension(0);
  }

  DescriptorPool pool_;

  const FileDescriptor* custom_option_file_;
  const FileDescriptor* foo_file_;
  const FileDescriptor* bar_file_;
  const FileDescriptor* baz_file_;

  const Descriptor* foo_message_;
  const EnumDescriptor* foo_enum_;
  const EnumValueDescriptor* foo_enum_value_;
  const ServiceDescriptor* foo_service_;
  const FieldDescriptor* foo_extension_;

  const Descriptor* bar_message_;
  const EnumDescriptor* bar_enum_;
  const EnumValueDescriptor* bar_enum_value_;
  const ServiceDescriptor* bar_service_;
  const FieldDescriptor* bar_extension_;
};

TEST_F(FileDescriptorTest, Name) {
  EXPECT_EQ("foo.proto", foo_file_->name());
  EXPECT_EQ("bar.proto", bar_file_->name());
  EXPECT_EQ("baz.proto", baz_file_->name());
}

TEST_F(FileDescriptorTest, Package) {
  EXPECT_EQ("", foo_file_->package());
  EXPECT_EQ("bar_package", bar_file_->package());
}

TEST_F(FileDescriptorTest, Dependencies) {
  EXPECT_EQ(0, foo_file_->dependency_count());
  EXPECT_EQ(1, bar_file_->dependency_count());
  EXPECT_EQ(foo_file_, bar_file_->dependency(0));
}

TEST_F(FileDescriptorTest, OptionDependencies) {
  EXPECT_EQ(0, foo_file_->option_dependency_count());
  EXPECT_EQ(1, bar_file_->option_dependency_count());
  EXPECT_EQ(custom_option_file_->name(), bar_file_->option_dependency_name(0));
}

TEST_F(FileDescriptorTest, FindMessageTypeByName) {
  EXPECT_EQ(foo_message_, foo_file_->FindMessageTypeByName("FooMessage"));
  EXPECT_EQ(bar_message_, bar_file_->FindMessageTypeByName("BarMessage"));

  EXPECT_TRUE(foo_file_->FindMessageTypeByName("BarMessage") == nullptr);
  EXPECT_TRUE(bar_file_->FindMessageTypeByName("FooMessage") == nullptr);
  EXPECT_TRUE(baz_file_->FindMessageTypeByName("FooMessage") == nullptr);

  EXPECT_TRUE(foo_file_->FindMessageTypeByName("NoSuchMessage") == nullptr);
  EXPECT_TRUE(foo_file_->FindMessageTypeByName("FooEnum") == nullptr);
}

TEST_F(FileDescriptorTest, FindEnumTypeByName) {
  EXPECT_EQ(foo_enum_, foo_file_->FindEnumTypeByName("FooEnum"));
  EXPECT_EQ(bar_enum_, bar_file_->FindEnumTypeByName("BarEnum"));

  EXPECT_TRUE(foo_file_->FindEnumTypeByName("BarEnum") == nullptr);
  EXPECT_TRUE(bar_file_->FindEnumTypeByName("FooEnum") == nullptr);
  EXPECT_TRUE(baz_file_->FindEnumTypeByName("FooEnum") == nullptr);

  EXPECT_TRUE(foo_file_->FindEnumTypeByName("NoSuchEnum") == nullptr);
  EXPECT_TRUE(foo_file_->FindEnumTypeByName("FooMessage") == nullptr);
}

TEST_F(FileDescriptorTest, FindEnumValueByName) {
  EXPECT_EQ(foo_enum_value_, foo_file_->FindEnumValueByName("FOO_ENUM_VALUE"));
  EXPECT_EQ(bar_enum_value_, bar_file_->FindEnumValueByName("BAR_ENUM_VALUE"));

  EXPECT_TRUE(foo_file_->FindEnumValueByName("BAR_ENUM_VALUE") == nullptr);
  EXPECT_TRUE(bar_file_->FindEnumValueByName("FOO_ENUM_VALUE") == nullptr);
  EXPECT_TRUE(baz_file_->FindEnumValueByName("FOO_ENUM_VALUE") == nullptr);

  EXPECT_TRUE(foo_file_->FindEnumValueByName("NO_SUCH_VALUE") == nullptr);
  EXPECT_TRUE(foo_file_->FindEnumValueByName("FooMessage") == nullptr);
}

TEST_F(FileDescriptorTest, FindServiceByName) {
  EXPECT_EQ(foo_service_, foo_file_->FindServiceByName("FooService"));
  EXPECT_EQ(bar_service_, bar_file_->FindServiceByName("BarService"));

  EXPECT_TRUE(foo_file_->FindServiceByName("BarService") == nullptr);
  EXPECT_TRUE(bar_file_->FindServiceByName("FooService") == nullptr);
  EXPECT_TRUE(baz_file_->FindServiceByName("FooService") == nullptr);

  EXPECT_TRUE(foo_file_->FindServiceByName("NoSuchService") == nullptr);
  EXPECT_TRUE(foo_file_->FindServiceByName("FooMessage") == nullptr);
}

TEST_F(FileDescriptorTest, FindExtensionByName) {
  EXPECT_EQ(foo_extension_, foo_file_->FindExtensionByName("foo_extension"));
  EXPECT_EQ(bar_extension_, bar_file_->FindExtensionByName("bar_extension"));

  EXPECT_TRUE(foo_file_->FindExtensionByName("bar_extension") == nullptr);
  EXPECT_TRUE(bar_file_->FindExtensionByName("foo_extension") == nullptr);
  EXPECT_TRUE(baz_file_->FindExtensionByName("foo_extension") == nullptr);

  EXPECT_TRUE(foo_file_->FindExtensionByName("no_such_extension") == nullptr);
  EXPECT_TRUE(foo_file_->FindExtensionByName("FooMessage") == nullptr);
}

TEST_F(FileDescriptorTest, FindExtensionByNumber) {
  EXPECT_EQ(foo_extension_, pool_.FindExtensionByNumber(foo_message_, 1));
  EXPECT_EQ(bar_extension_, pool_.FindExtensionByNumber(bar_message_, 1));

  EXPECT_TRUE(pool_.FindExtensionByNumber(foo_message_, 2) == nullptr);
}


TEST_F(FileDescriptorTest, BuildAgain) {
  // Test that if we call BuildFile again on the same input we get the same
  // FileDescriptor back.
  FileDescriptorProto file;
  foo_file_->CopyTo(&file);
  EXPECT_EQ(foo_file_, pool_.BuildFile(file));

  // But if we change the file then it won't work.
  file.set_package("some.other.package");
  EXPECT_TRUE(pool_.BuildFile(file) == nullptr);
}

TEST_F(FileDescriptorTest, BuildAgainWithSyntax) {
  // Test that if we call BuildFile again on the same input we get the same
  // FileDescriptor back even if syntax param is specified.
  FileDescriptorProto proto_syntax2;
  proto_syntax2.set_name("foo_syntax2");
  proto_syntax2.set_syntax("proto2");

  const FileDescriptor* proto2_descriptor = pool_.BuildFile(proto_syntax2);
  EXPECT_TRUE(proto2_descriptor != nullptr);
  EXPECT_EQ(proto2_descriptor, pool_.BuildFile(proto_syntax2));

  FileDescriptorProto implicit_proto2;
  implicit_proto2.set_name("foo_implicit_syntax2");

  const FileDescriptor* implicit_proto2_descriptor =
      pool_.BuildFile(implicit_proto2);
  EXPECT_TRUE(implicit_proto2_descriptor != nullptr);
  // We get the same FileDescriptor back if syntax param is explicitly
  // specified.
  implicit_proto2.set_syntax("proto2");
  EXPECT_EQ(implicit_proto2_descriptor, pool_.BuildFile(implicit_proto2));

  FileDescriptorProto proto_syntax3;
  proto_syntax3.set_name("foo_syntax3");
  proto_syntax3.set_syntax("proto3");

  const FileDescriptor* proto3_descriptor = pool_.BuildFile(proto_syntax3);
  EXPECT_TRUE(proto3_descriptor != nullptr);
  EXPECT_EQ(proto3_descriptor, pool_.BuildFile(proto_syntax3));
}

TEST_F(FileDescriptorTest, Edition) {
  FileDescriptorProto proto;
  proto.set_name("foo");
  {
    proto.set_syntax("proto2");
    DescriptorPool pool;
    const FileDescriptor* file = pool.BuildFile(proto);
    ASSERT_TRUE(file != nullptr);
    EXPECT_EQ(FileDescriptorLegacy(file).edition(), Edition::EDITION_PROTO2);
    FileDescriptorProto other;
    file->CopyTo(&other);
    EXPECT_EQ("", other.syntax());
    EXPECT_FALSE(other.has_edition());
  }
  {
    proto.set_syntax("proto3");
    DescriptorPool pool;
    const FileDescriptor* file = pool.BuildFile(proto);
    ASSERT_TRUE(file != nullptr);
    EXPECT_EQ(FileDescriptorLegacy(file).edition(), Edition::EDITION_PROTO3);
    FileDescriptorProto other;
    file->CopyTo(&other);
    EXPECT_EQ("proto3", other.syntax());
    EXPECT_FALSE(other.has_edition());
  }
  {
    proto.set_syntax("editions");
    proto.set_edition(EDITION_2023);
    DescriptorPool pool;
    const FileDescriptor* file = pool.BuildFile(proto);
    ASSERT_TRUE(file != nullptr);
    EXPECT_EQ(FileDescriptorLegacy(file).edition(), Edition::EDITION_2023);
    FileDescriptorProto other;
    file->CopyTo(&other);
    EXPECT_EQ("editions", other.syntax());
    EXPECT_EQ(other.edition(), EDITION_2023);
  }
}

TEST_F(FileDescriptorTest, CopyHeadingTo) {
  FileDescriptorProto proto;
  proto.set_name("foo.proto");
  proto.set_package("foo.bar.baz");
  proto.set_syntax("proto3");
  proto.mutable_options()->set_java_package("foo.bar.baz");

  // Won't be copied.
  proto.add_message_type()->set_name("Foo");

  DescriptorPool pool;
  const FileDescriptor* file = pool.BuildFile(proto);
  ASSERT_NE(file, nullptr);

  FileDescriptorProto other;
  file->CopyHeadingTo(&other);
  EXPECT_EQ(other.name(), "foo.proto");
  EXPECT_EQ(other.package(), "foo.bar.baz");
  EXPECT_EQ(other.syntax(), "proto3");
  EXPECT_EQ(other.options().java_package(), "foo.bar.baz");
  EXPECT_TRUE(other.message_type().empty());
  EXPECT_EQ(&other.options().features(), &FeatureSet::default_instance());
  {
    proto.set_syntax("editions");
    proto.set_edition(EDITION_2023);

    DescriptorPool pool;
    const FileDescriptor* file = pool.BuildFile(proto);
    ASSERT_NE(file, nullptr);

    FileDescriptorProto other;
    file->CopyHeadingTo(&other);
    EXPECT_EQ(other.name(), "foo.proto");
    EXPECT_EQ(other.package(), "foo.bar.baz");
    EXPECT_EQ(other.syntax(), "editions");
    EXPECT_EQ(other.edition(), EDITION_2023);
    EXPECT_EQ(other.options().java_package(), "foo.bar.baz");
    EXPECT_TRUE(other.message_type().empty());
    EXPECT_EQ(&other.options().features(), &FeatureSet::default_instance());
  }
}

void ExtractDebugString(
    const FileDescriptor* file, absl::flat_hash_set<absl::string_view>* visited,
    std::vector<std::pair<absl::string_view, std::string>>* debug_strings) {
  if (!visited->insert(file->name()).second) {
    return;
  }
  for (int i = 0; i < file->dependency_count(); ++i) {
    ExtractDebugString(file->dependency(i), visited, debug_strings);
  }
  debug_strings->push_back({file->name(), file->DebugString()});
}

class SimpleErrorCollector : public io::ErrorCollector {
 public:
  // implements ErrorCollector ---------------------------------------
  void RecordError(int line, int column, absl::string_view message) override {
    last_error_ = absl::StrFormat("%d:%d:%s", line, column, message);
  }

  const std::string& last_error() { return last_error_; }

 private:
  std::string last_error_;
};
// Test that the result of FileDescriptor::DebugString() can be used to create
// the original descriptors.
TEST_F(FileDescriptorTest, DebugStringRoundTrip) {
  absl::flat_hash_set<absl::string_view> visited;
  std::vector<std::pair<absl::string_view, std::string>> debug_strings;
  ExtractDebugString(proto2_unittest::TestAllTypes::descriptor()->file(),
                     &visited, &debug_strings);
  ExtractDebugString(
      proto2_unittest::TestMessageWithCustomOptions::descriptor()->file(),
      &visited, &debug_strings);
  ExtractDebugString(proto3_arena_unittest::TestAllTypes::descriptor()->file(),
                     &visited, &debug_strings);
  ASSERT_GE(debug_strings.size(), 3);

  DescriptorPool pool;
  for (size_t i = 0; i < debug_strings.size(); ++i) {
    const absl::string_view name = debug_strings[i].first;
    const std::string& content = debug_strings[i].second;
    io::ArrayInputStream input_stream(content.data(), content.size());
    SimpleErrorCollector error_collector;
    io::Tokenizer tokenizer(&input_stream, &error_collector);
    compiler::Parser parser;
    parser.RecordErrorsTo(&error_collector);
    FileDescriptorProto proto;
    ASSERT_TRUE(parser.Parse(&tokenizer, &proto))
        << error_collector.last_error() << "\n"
        << content;
    ASSERT_EQ("", error_collector.last_error());
    proto.set_name(name);
    const FileDescriptor* descriptor = pool.BuildFile(proto);
    ASSERT_TRUE(descriptor != nullptr) << error_collector.last_error();
    EXPECT_EQ(content, descriptor->DebugString());
  }
}

TEST_F(FileDescriptorTest, DebugStringRoundTripVisibility) {
  DescriptorPool pool;

  // warning load-bearing whitespace below.  This tests the round-trip of
  // content string -> Parse -> Descriptor -> DebugString and asserts the input
  // content is identical to the output debugstring (white-space included).
  absl::string_view content = R"schema(edition = "2024";

package ed2024.visibility.unittest;

local message VisibilityLocalMessage {
  local message NestedLocalMessage {
    local message InnerNestedLocalMessage {
    }
  }
  export message NestedExportMessage {
    export message InnerNestedExportMessage {
    }
  }
  local enum NestedLocalEnum {
    YES = 0;
  }
  export enum NestedExportEnum {
    NO = 0;
  }
}

export message VisibilityExportMessage {
  local message NestedLocalMessage {
    local message InnerNestedLocalMessage {
    }
  }
  export message NestedExportMessage {
    export message InnerNestedExportMessage {
    }
  }
  local enum NestedLocalEnum {
    UP = 0;
  }
  export enum NestedExportEnum {
    DOWN = 0;
  }
}

)schema";

  io::ArrayInputStream input_stream(content.data(), content.size());
  SimpleErrorCollector error_collector;
  io::Tokenizer tokenizer(&input_stream, &error_collector);
  compiler::Parser parser;
  parser.RecordErrorsTo(&error_collector);
  FileDescriptorProto proto;
  ASSERT_TRUE(parser.Parse(&tokenizer, &proto))
      << error_collector.last_error() << "\n"
      << content;
  ASSERT_EQ("", error_collector.last_error());
  proto.set_name("google/protobuf/unittest_visibility_edition_2024.proto");
  const FileDescriptor* descriptor = pool.BuildFile(proto);
  ASSERT_TRUE(descriptor != nullptr) << error_collector.last_error();
  EXPECT_EQ(content, descriptor->DebugString());
}

TEST_F(FileDescriptorTest, CopyToRoundTripVisibility) {
  DescriptorPool pool;

  std::string content = R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2024
    message_type {
      name: "ExportMessage"
      visibility: VISIBILITY_EXPORT

      nested_type { name: "LocalMessage" visibility: VISIBILITY_EXPORT }
      enum_type {
        name: "LocalEnum"
        value { name: "DEFAULT" number: 0 }
        visibility: VISIBILITY_LOCAL
      }
    }
    enum_type {
      name: "ExportEnum"
      value { name: "DEFAULT" number: 0 }
      visibility: VISIBILITY_EXPORT
    }
  )pb";

  FileDescriptorProto proto;
  ASSERT_TRUE(TextFormat::ParseFromString(content, &proto));
  const FileDescriptor* descriptor = pool.BuildFile(proto);
  ASSERT_NE(descriptor, nullptr);

  FileDescriptorProto other;
  descriptor->CopyTo(&other);

  EXPECT_THAT(other, EqualsProto(content));
}

TEST_F(FileDescriptorTest, AbslStringifyWorks) {
  std::string s = absl::StrFormat(
      "%v",
      *proto2_unittest::TestMessageWithCustomOptions::descriptor()->file());
  EXPECT_THAT(s, HasSubstr("TestMessageWithCustomOptions"));
}

// ===================================================================

// Test simple flat messages and fields.
class DescriptorTest : public testing::Test {
 protected:
  void SetUp() override {
    // Build descriptors for the following definitions:
    //
    //   // in "foo.proto"
    //   message TestForeign {}
    //   enum TestEnum {}
    //
    //   message TestMessage {
    //     required string      foo = 1;
    //     optional TestEnum    bar = 6;
    //     repeated TestForeign baz = 500000000;
    //     optional group       moo = 15 {}
    //   }
    //
    //   // in "bar.proto"
    //   package corge.grault;
    //   message TestMessage2 {
    //     required string foo = 1;
    //     required string bar = 2;
    //     required string mooo = 6;
    //   }
    //
    //   // in "map.proto"
    //   message TestMessage3 {
    //     map<int32, int32> map_int32_int32 = 1;
    //   }
    //
    //   // in "json.proto"
    //   message TestMessage4 {
    //     optional int32 field_name1 = 1;
    //     optional int32 fieldName2 = 2;
    //     optional int32 FieldName3 = 3;
    //     optional int32 _field_name4 = 4;
    //     optional int32 FIELD_NAME5 = 5;
    //     optional int32 field_name6 = 6 [json_name = "@type"];
    //   }
    //
    // We cheat and use TestForeign as the type for moo rather than create
    // an actual nested type.
    //
    // Since all primitive types (including string) use the same building
    // code, there's no need to test each one individually.
    //
    // TestMessage2 is primarily here to test FindFieldByName and friends.
    // All messages created from the same DescriptorPool share the same lookup
    // table, so we need to insure that they don't interfere.

    FileDescriptorProto foo_file;
    foo_file.set_name("foo.proto");
    AddMessage(&foo_file, "TestForeign");
    AddEmptyEnum(&foo_file, "TestEnum");

    DescriptorProto* message = AddMessage(&foo_file, "TestMessage");
    AddField(message, "foo", 1, FieldDescriptorProto::LABEL_REQUIRED,
             FieldDescriptorProto::TYPE_STRING);
    AddField(message, "bar", 6, FieldDescriptorProto::LABEL_OPTIONAL,
             FieldDescriptorProto::TYPE_ENUM)
        ->set_type_name("TestEnum");
    AddField(message, "baz", 500000000, FieldDescriptorProto::LABEL_REPEATED,
             FieldDescriptorProto::TYPE_MESSAGE)
        ->set_type_name("TestForeign");
    AddField(message, "moo", 15, FieldDescriptorProto::LABEL_OPTIONAL,
             FieldDescriptorProto::TYPE_GROUP)
        ->set_type_name("TestForeign");

    FileDescriptorProto bar_file;
    bar_file.set_name("bar.proto");
    bar_file.set_package("corge.grault");

    DescriptorProto* message2 = AddMessage(&bar_file, "TestMessage2");
    AddField(message2, "foo", 1, FieldDescriptorProto::LABEL_REQUIRED,
             FieldDescriptorProto::TYPE_STRING);
    AddField(message2, "bar", 2, FieldDescriptorProto::LABEL_REQUIRED,
             FieldDescriptorProto::TYPE_STRING);
    AddField(message2, "mooo", 6, FieldDescriptorProto::LABEL_REQUIRED,
             FieldDescriptorProto::TYPE_STRING);

    FileDescriptorProto map_file;
    map_file.set_name("map.proto");
    DescriptorProto* message3 = AddMessage(&map_file, "TestMessage3");

    DescriptorProto* entry = AddNestedMessage(message3, "MapInt32Int32Entry");
    AddField(entry, "key", 1, FieldDescriptorProto::LABEL_OPTIONAL,
             FieldDescriptorProto::TYPE_INT32);
    AddField(entry, "value", 2, FieldDescriptorProto::LABEL_OPTIONAL,
             FieldDescriptorProto::TYPE_INT32);
    entry->mutable_options()->set_map_entry(true);

    AddField(message3, "map_int32_int32", 1,
             FieldDescriptorProto::LABEL_REPEATED,
             FieldDescriptorProto::TYPE_MESSAGE)
        ->set_type_name("MapInt32Int32Entry");

    FileDescriptorProto json_file;
    json_file.set_name("json.proto");
    json_file.set_syntax("proto3");
    DescriptorProto* message4 = AddMessage(&json_file, "TestMessage4");
    AddField(message4, "field_name1", 1, FieldDescriptorProto::LABEL_OPTIONAL,
             FieldDescriptorProto::TYPE_INT32);
    AddField(message4, "fieldName2", 2, FieldDescriptorProto::LABEL_OPTIONAL,
             FieldDescriptorProto::TYPE_INT32);
    AddField(message4, "FieldName3", 3, FieldDescriptorProto::LABEL_OPTIONAL,
             FieldDescriptorProto::TYPE_INT32);
    AddField(message4, "_field_name4", 4, FieldDescriptorProto::LABEL_OPTIONAL,
             FieldDescriptorProto::TYPE_INT32);
    AddField(message4, "FIELD_NAME5", 5, FieldDescriptorProto::LABEL_OPTIONAL,
             FieldDescriptorProto::TYPE_INT32);
    AddField(message4, "field_name6", 6, FieldDescriptorProto::LABEL_OPTIONAL,
             FieldDescriptorProto::TYPE_INT32)
        ->set_json_name("@type");
    AddField(message4, "fieldname7", 7, FieldDescriptorProto::LABEL_OPTIONAL,
             FieldDescriptorProto::TYPE_INT32);

    // Build the descriptors and get the pointers.
    foo_file_ = pool_.BuildFile(foo_file);
    ASSERT_TRUE(foo_file_ != nullptr);

    bar_file_ = pool_.BuildFile(bar_file);
    ASSERT_TRUE(bar_file_ != nullptr);

    map_file_ = pool_.BuildFile(map_file);
    ASSERT_TRUE(map_file_ != nullptr);

    json_file_ = pool_.BuildFile(json_file);
    ASSERT_TRUE(json_file_ != nullptr);

    ASSERT_EQ(1, foo_file_->enum_type_count());
    enum_ = foo_file_->enum_type(0);

    ASSERT_EQ(2, foo_file_->message_type_count());
    foreign_ = foo_file_->message_type(0);
    message_ = foo_file_->message_type(1);

    ASSERT_EQ(4, message_->field_count());
    foo_ = message_->field(0);
    bar_ = message_->field(1);
    baz_ = message_->field(2);
    moo_ = message_->field(3);

    ASSERT_EQ(1, bar_file_->message_type_count());
    message2_ = bar_file_->message_type(0);

    ASSERT_EQ(3, message2_->field_count());
    foo2_ = message2_->field(0);
    bar2_ = message2_->field(1);
    mooo2_ = message2_->field(2);

    ASSERT_EQ(1, map_file_->message_type_count());
    message3_ = map_file_->message_type(0);

    ASSERT_EQ(1, message3_->field_count());
    map_ = message3_->field(0);

    ASSERT_EQ(1, json_file_->message_type_count());
    message4_ = json_file_->message_type(0);
  }

  void CopyWithJsonName(const Descriptor* message, DescriptorProto* proto) {
    message->CopyTo(proto);
    message->CopyJsonNameTo(proto);
  }

  const EnumValueDescriptor* FindValueByNumberCreatingIfUnknown(
      const EnumDescriptor* desc, int number) {
    return desc->FindValueByNumberCreatingIfUnknown(number);
  }

  DescriptorPool pool_;

  const FileDescriptor* foo_file_;
  const FileDescriptor* bar_file_;
  const FileDescriptor* map_file_;
  const FileDescriptor* json_file_;

  const Descriptor* message_;
  const Descriptor* message2_;
  const Descriptor* message3_;
  const Descriptor* message4_;
  const Descriptor* foreign_;
  const EnumDescriptor* enum_;

  const FieldDescriptor* foo_;
  const FieldDescriptor* bar_;
  const FieldDescriptor* baz_;
  const FieldDescriptor* moo_;

  const FieldDescriptor* foo2_;
  const FieldDescriptor* bar2_;
  const FieldDescriptor* mooo2_;

  const FieldDescriptor* map_;
};

TEST_F(DescriptorTest, Name) {
  EXPECT_EQ("TestMessage", message_->name());
  EXPECT_EQ("TestMessage", message_->full_name());
  EXPECT_EQ(foo_file_, message_->file());

  EXPECT_EQ("TestMessage2", message2_->name());
  EXPECT_EQ("corge.grault.TestMessage2", message2_->full_name());
  EXPECT_EQ(bar_file_, message2_->file());
}

TEST_F(DescriptorTest, ContainingType) {
  EXPECT_TRUE(message_->containing_type() == nullptr);
  EXPECT_TRUE(message2_->containing_type() == nullptr);
}

TEST_F(DescriptorTest, FieldNamesDedupOnOptimizedCases) {
  const auto collect_unique_names = [](const FieldDescriptor* field) {
    absl::btree_set<absl::string_view> names{
        field->name(), field->lowercase_name(), field->camelcase_name(),
        field->json_name()};
    // For names following the style guide, verify that we have the same number
    // of string objects as we have string values. That is, duplicate names use
    // the same std::string object. This is for memory efficiency.
    EXPECT_EQ(names.size(),
              (absl::flat_hash_set<const void*>{
                  field->name().data(), field->lowercase_name().data(),
                  field->camelcase_name().data(), field->json_name().data()}
                   .size()))
        << testing::PrintToString(names);
    return names;
  };

  // field_name1
  EXPECT_THAT(collect_unique_names(message4_->field(0)),
              ElementsAre("fieldName1", "field_name1"));
  // fieldname7
  EXPECT_THAT(collect_unique_names(message4_->field(6)),
              ElementsAre("fieldname7"));
}

TEST_F(DescriptorTest, RegressionNamesAreNullTerminated) {
  // Name accessors where migrated from std::string to absl::string_view.
  // Some callers were taking the C-String out of the std::string via `.data()`
  // and that code kept working when the type was changed.
  // We want to keep that working for now to prevent breaking these users
  // dynamically.
  const auto check_nul_terminated = [](absl::string_view view) {
    EXPECT_EQ(view.data()[view.size()], '\0');
  };
  const auto check_nul_names = [&](auto* entity) {
    check_nul_terminated(entity->name());
    check_nul_terminated(entity->full_name());
  };

  const auto check_nul_field_names = [&](auto* field) {
    check_nul_terminated(field->name());
    check_nul_terminated(field->full_name());
    check_nul_terminated(field->lowercase_name());
    check_nul_terminated(field->camelcase_name());
    check_nul_terminated(field->json_name());
  };

  check_nul_names(message4_);
  check_nul_names(enum_);
  for (int i = 0; i < message4_->field_count(); ++i) {
    check_nul_field_names(message4_->field(i));
  }
}

TEST_F(DescriptorTest, FieldNamesMatchOnCornerCases) {
  const auto names = [&](auto* field) {
    return std::vector<absl::string_view>{
        field->name(), field->lowercase_name(), field->camelcase_name(),
        field->json_name()};
  };

  // field_name1
  EXPECT_THAT(
      names(message4_->field(0)),
      ElementsAre("field_name1", "field_name1", "fieldName1", "fieldName1"));
  // fieldName2
  EXPECT_THAT(
      names(message4_->field(1)),
      ElementsAre("fieldName2", "fieldname2", "fieldName2", "fieldName2"));
  // FieldName3
  EXPECT_THAT(
      names(message4_->field(2)),
      ElementsAre("FieldName3", "fieldname3", "fieldName3", "FieldName3"));
  // _field_name4
  EXPECT_THAT(
      names(message4_->field(3)),
      ElementsAre("_field_name4", "_field_name4", "fieldName4", "FieldName4"));
  // FIELD_NAME5
  EXPECT_THAT(
      names(message4_->field(4)),
      ElementsAre("FIELD_NAME5", "field_name5", "fIELDNAME5", "FIELDNAME5"));
  // field_name6, with json name @type
  EXPECT_THAT(names(message4_->field(5)),
              ElementsAre("field_name6", "field_name6", "fieldName6", "@type"));
  // fieldname7
  EXPECT_THAT(
      names(message4_->field(6)),
      ElementsAre("fieldname7", "fieldname7", "fieldname7", "fieldname7"));
}

TEST_F(DescriptorTest, FieldNameDedupJsonEqFull) {
  // Test a regression where json_name == full_name
  FileDescriptorProto proto;
  proto.set_name("file");
  auto* message = AddMessage(&proto, "Name1");
  auto* field =
      AddField(message, "Name2", 1, FieldDescriptorProto::LABEL_OPTIONAL,
               FieldDescriptorProto::TYPE_INT32);
  field->set_json_name("Name1.Name2");
  auto* file = pool_.BuildFile(proto);
  EXPECT_EQ(file->message_type(0)->name(), "Name1");
  EXPECT_EQ(file->message_type(0)->field(0)->name(), "Name2");
  EXPECT_EQ(file->message_type(0)->field(0)->full_name(), "Name1.Name2");
  EXPECT_EQ(file->message_type(0)->field(0)->json_name(), "Name1.Name2");
}

TEST_F(DescriptorTest, FieldsByIndex) {
  ASSERT_EQ(4, message_->field_count());
  EXPECT_EQ(foo_, message_->field(0));
  EXPECT_EQ(bar_, message_->field(1));
  EXPECT_EQ(baz_, message_->field(2));
  EXPECT_EQ(moo_, message_->field(3));
}

TEST_F(DescriptorTest, FindFieldByName) {
  // All messages in the same DescriptorPool share a single lookup table for
  // fields.  So, in addition to testing that FindFieldByName finds the fields
  // of the message, we need to test that it does *not* find the fields of
  // *other* messages.

  EXPECT_EQ(foo_, message_->FindFieldByName("foo"));
  EXPECT_EQ(bar_, message_->FindFieldByName("bar"));
  EXPECT_EQ(baz_, message_->FindFieldByName("baz"));
  EXPECT_EQ(moo_, message_->FindFieldByName("moo"));
  EXPECT_TRUE(message_->FindFieldByName("no_such_field") == nullptr);
  EXPECT_TRUE(message_->FindFieldByName("mooo") == nullptr);

  EXPECT_EQ(foo2_, message2_->FindFieldByName("foo"));
  EXPECT_EQ(bar2_, message2_->FindFieldByName("bar"));
  EXPECT_EQ(mooo2_, message2_->FindFieldByName("mooo"));
  EXPECT_TRUE(message2_->FindFieldByName("baz") == nullptr);
  EXPECT_TRUE(message2_->FindFieldByName("moo") == nullptr);
}

TEST_F(DescriptorTest, FindFieldByNumber) {
  EXPECT_EQ(foo_, message_->FindFieldByNumber(1));
  EXPECT_EQ(bar_, message_->FindFieldByNumber(6));
  EXPECT_EQ(baz_, message_->FindFieldByNumber(500000000));
  EXPECT_EQ(moo_, message_->FindFieldByNumber(15));
  EXPECT_TRUE(message_->FindFieldByNumber(837592) == nullptr);
  EXPECT_TRUE(message_->FindFieldByNumber(2) == nullptr);

  EXPECT_EQ(foo2_, message2_->FindFieldByNumber(1));
  EXPECT_EQ(bar2_, message2_->FindFieldByNumber(2));
  EXPECT_EQ(mooo2_, message2_->FindFieldByNumber(6));
  EXPECT_TRUE(message2_->FindFieldByNumber(15) == nullptr);
  EXPECT_TRUE(message2_->FindFieldByNumber(500000000) == nullptr);
}

TEST_F(DescriptorTest, FieldName) {
  EXPECT_EQ("foo", foo_->name());
  EXPECT_EQ("bar", bar_->name());
  EXPECT_EQ("baz", baz_->name());
  EXPECT_EQ("moo", moo_->name());
}

TEST_F(DescriptorTest, FieldFullName) {
  EXPECT_EQ("TestMessage.foo", foo_->full_name());
  EXPECT_EQ("TestMessage.bar", bar_->full_name());
  EXPECT_EQ("TestMessage.baz", baz_->full_name());
  EXPECT_EQ("TestMessage.moo", moo_->full_name());

  EXPECT_EQ("corge.grault.TestMessage2.foo", foo2_->full_name());
  EXPECT_EQ("corge.grault.TestMessage2.bar", bar2_->full_name());
  EXPECT_EQ("corge.grault.TestMessage2.mooo", mooo2_->full_name());
}

TEST_F(DescriptorTest, PrintableNameIsFullNameForNonExtensionFields) {
  EXPECT_EQ("TestMessage.foo", foo_->PrintableNameForExtension());
  EXPECT_EQ("TestMessage.bar", bar_->PrintableNameForExtension());
  EXPECT_EQ("TestMessage.baz", baz_->PrintableNameForExtension());
  EXPECT_EQ("TestMessage.moo", moo_->PrintableNameForExtension());

  EXPECT_EQ("corge.grault.TestMessage2.foo",
            foo2_->PrintableNameForExtension());
  EXPECT_EQ("corge.grault.TestMessage2.bar",
            bar2_->PrintableNameForExtension());
  EXPECT_EQ("corge.grault.TestMessage2.mooo",
            mooo2_->PrintableNameForExtension());
}

TEST_F(DescriptorTest, PrintableNameIsFullNameForNonMessageSetExtension) {
  EXPECT_EQ("proto2_unittest.Aggregate.nested",
            proto2_unittest::Aggregate::descriptor()
                ->FindExtensionByName("nested")
                ->PrintableNameForExtension());
}

TEST_F(DescriptorTest, PrintableNameIsExtendingTypeForMessageSetExtension) {
  EXPECT_EQ("proto2_unittest.AggregateMessageSetElement",
            proto2_unittest::AggregateMessageSetElement::descriptor()
                ->FindExtensionByName("message_set_extension")
                ->PrintableNameForExtension());
}

TEST_F(DescriptorTest, FieldJsonName) {
  EXPECT_EQ("fieldName1", message4_->field(0)->json_name());
  EXPECT_EQ("fieldName2", message4_->field(1)->json_name());
  EXPECT_EQ("FieldName3", message4_->field(2)->json_name());
  EXPECT_EQ("FieldName4", message4_->field(3)->json_name());
  EXPECT_EQ("FIELDNAME5", message4_->field(4)->json_name());
  EXPECT_EQ("@type", message4_->field(5)->json_name());

  DescriptorProto proto;
  message4_->CopyTo(&proto);
  ASSERT_EQ(7, proto.field_size());
  EXPECT_FALSE(proto.field(0).has_json_name());
  EXPECT_FALSE(proto.field(1).has_json_name());
  EXPECT_FALSE(proto.field(2).has_json_name());
  EXPECT_FALSE(proto.field(3).has_json_name());
  EXPECT_FALSE(proto.field(4).has_json_name());
  EXPECT_EQ("@type", proto.field(5).json_name());
  EXPECT_FALSE(proto.field(6).has_json_name());

  proto.Clear();
  CopyWithJsonName(message4_, &proto);
  ASSERT_EQ(7, proto.field_size());
  EXPECT_EQ("fieldName1", proto.field(0).json_name());
  EXPECT_EQ("fieldName2", proto.field(1).json_name());
  EXPECT_EQ("FieldName3", proto.field(2).json_name());
  EXPECT_EQ("FieldName4", proto.field(3).json_name());
  EXPECT_EQ("FIELDNAME5", proto.field(4).json_name());
  EXPECT_EQ("@type", proto.field(5).json_name());
  EXPECT_EQ("fieldname7", proto.field(6).json_name());

  // Test generated descriptor.
  const Descriptor* generated = proto2_unittest::TestJsonName::descriptor();
  ASSERT_EQ(7, generated->field_count());
  EXPECT_EQ("fieldName1", generated->field(0)->json_name());
  EXPECT_EQ("fieldName2", generated->field(1)->json_name());
  EXPECT_EQ("FieldName3", generated->field(2)->json_name());
  EXPECT_EQ("FieldName4", generated->field(3)->json_name());
  EXPECT_EQ("FIELDNAME5", generated->field(4)->json_name());
  EXPECT_EQ("@type", generated->field(5)->json_name());
  EXPECT_EQ("fieldname7", generated->field(6)->json_name());
}

TEST_F(DescriptorTest, FieldFile) {
  EXPECT_EQ(foo_file_, foo_->file());
  EXPECT_EQ(foo_file_, bar_->file());
  EXPECT_EQ(foo_file_, baz_->file());
  EXPECT_EQ(foo_file_, moo_->file());

  EXPECT_EQ(bar_file_, foo2_->file());
  EXPECT_EQ(bar_file_, bar2_->file());
  EXPECT_EQ(bar_file_, mooo2_->file());
}

TEST_F(DescriptorTest, FieldIndex) {
  EXPECT_EQ(0, foo_->index());
  EXPECT_EQ(1, bar_->index());
  EXPECT_EQ(2, baz_->index());
  EXPECT_EQ(3, moo_->index());
}

TEST_F(DescriptorTest, FieldNumber) {
  EXPECT_EQ(1, foo_->number());
  EXPECT_EQ(6, bar_->number());
  EXPECT_EQ(500000000, baz_->number());
  EXPECT_EQ(15, moo_->number());
}

TEST_F(DescriptorTest, FieldType) {
  EXPECT_EQ(FieldDescriptor::TYPE_STRING, foo_->type());
  EXPECT_EQ(FieldDescriptor::TYPE_ENUM, bar_->type());
  EXPECT_EQ(FieldDescriptor::TYPE_MESSAGE, baz_->type());
  EXPECT_EQ(FieldDescriptor::TYPE_GROUP, moo_->type());
}

TEST_F(DescriptorTest, FieldLabel) {
  EXPECT_EQ(FieldDescriptor::LABEL_REQUIRED, foo_->label());
  EXPECT_EQ(FieldDescriptor::LABEL_OPTIONAL, bar_->label());
  EXPECT_EQ(FieldDescriptor::LABEL_REPEATED, baz_->label());
  EXPECT_EQ(FieldDescriptor::LABEL_OPTIONAL, moo_->label());

  EXPECT_TRUE(foo_->is_required());
  EXPECT_FALSE(foo_->is_optional());
  EXPECT_FALSE(foo_->is_repeated());

  EXPECT_FALSE(bar_->is_required());
  EXPECT_TRUE(bar_->is_optional());
  EXPECT_FALSE(bar_->is_repeated());

  EXPECT_FALSE(baz_->is_required());
  EXPECT_FALSE(baz_->is_optional());
  EXPECT_TRUE(baz_->is_repeated());
}

TEST_F(DescriptorTest, NeedsUtf8Check) {
  EXPECT_FALSE(foo_->requires_utf8_validation());
  EXPECT_FALSE(bar_->requires_utf8_validation());

  // Build a copy of the file in proto3.
  FileDescriptorProto foo_file3;
  foo_file_->CopyTo(&foo_file3);
  foo_file3.set_syntax("proto3");

  // Make this valid proto3 by removing `required` and the one group field.
  for (auto& f : *foo_file3.mutable_message_type(1)->mutable_field()) {
    f.clear_label();
    if (f.type() == FieldDescriptorProto::TYPE_GROUP) {
      f.set_type(FieldDescriptorProto::TYPE_MESSAGE);
    }
  }
  // Make this valid proto3 by making the first enum value be zero.
  foo_file3.mutable_enum_type(0)->mutable_value(0)->set_number(0);

  DescriptorPool pool3;
  const Descriptor* message3 = pool3.BuildFile(foo_file3)->message_type(1);
  const FieldDescriptor* foo3 = message3->field(0);
  const FieldDescriptor* bar3 = message3->field(1);

  EXPECT_TRUE(foo3->requires_utf8_validation());
  EXPECT_FALSE(bar3->requires_utf8_validation());
}

TEST_F(DescriptorTest, EnumFieldTreatedAsClosed) {
  // Make an open enum definition.
  FileDescriptorProto open_enum_file;
  open_enum_file.set_name("open_enum.proto");
  open_enum_file.set_syntax("proto3");
  AddEnumValue(AddEnum(&open_enum_file, "TestEnumOpen"), "TestEnumOpen_VALUE0",
               0);

  const EnumDescriptor* open_enum =
      pool_.BuildFile(open_enum_file)->enum_type(0);
  EXPECT_FALSE(open_enum->is_closed());

  // Create a message that treats enum fields as closed.
  FileDescriptorProto closed_file;
  closed_file.set_name("closed_enum_field.proto");
  closed_file.add_dependency("open_enum.proto");
  closed_file.add_dependency("foo.proto");

  DescriptorProto* message = AddMessage(&closed_file, "TestClosedEnumField");
  AddField(message, "int_field", 1, FieldDescriptorProto::LABEL_OPTIONAL,
           FieldDescriptorProto::TYPE_INT32);
  AddField(message, "open_enum", 2, FieldDescriptorProto::LABEL_OPTIONAL,
           FieldDescriptorProto::TYPE_ENUM)
      ->set_type_name("TestEnumOpen");
  AddField(message, "closed_enum", 3, FieldDescriptorProto::LABEL_OPTIONAL,
           FieldDescriptorProto::TYPE_ENUM)
      ->set_type_name("TestEnum");
  const Descriptor* closed_message =
      pool_.BuildFile(closed_file)->message_type(0);

  EXPECT_FALSE(closed_message->FindFieldByName("int_field")
                   ->legacy_enum_field_treated_as_closed());
  EXPECT_TRUE(closed_message->FindFieldByName("closed_enum")
                  ->legacy_enum_field_treated_as_closed());
  EXPECT_TRUE(closed_message->FindFieldByName("open_enum")
                  ->legacy_enum_field_treated_as_closed());
}

TEST_F(DescriptorTest, EnumFieldTreatedAsOpen) {
  FileDescriptorProto open_enum_file;
  open_enum_file.set_name("open_enum.proto");
  open_enum_file.set_syntax("proto3");
  AddEnumValue(AddEnum(&open_enum_file, "TestEnumOpen"), "TestEnumOpen_VALUE0",
               0);
  DescriptorProto* message = AddMessage(&open_enum_file, "TestOpenEnumField");
  AddField(message, "int_field", 1, FieldDescriptorProto::LABEL_OPTIONAL,
           FieldDescriptorProto::TYPE_INT32);
  AddField(message, "open_enum", 2, FieldDescriptorProto::LABEL_OPTIONAL,
           FieldDescriptorProto::TYPE_ENUM)
      ->set_type_name("TestEnumOpen");
  const FileDescriptor* open_enum_file_desc = pool_.BuildFile(open_enum_file);
  const Descriptor* open_message = open_enum_file_desc->message_type(0);
  const EnumDescriptor* open_enum = open_enum_file_desc->enum_type(0);
  EXPECT_FALSE(open_enum->is_closed());
  EXPECT_FALSE(open_message->FindFieldByName("int_field")
                   ->legacy_enum_field_treated_as_closed());
  EXPECT_FALSE(open_message->FindFieldByName("open_enum")
                   ->legacy_enum_field_treated_as_closed());
}

TEST_F(DescriptorTest, IsMap) {
  EXPECT_TRUE(map_->is_map());
  EXPECT_FALSE(baz_->is_map());
  EXPECT_TRUE(map_->message_type()->options().map_entry());
}

TEST_F(DescriptorTest, GetMap) {
  const Descriptor* map_desc = map_->message_type();
  const FieldDescriptor* map_key = map_desc->map_key();
  ASSERT_TRUE(map_key != nullptr);
  EXPECT_EQ(map_key->name(), "key");
  EXPECT_EQ(map_key->number(), 1);

  const FieldDescriptor* map_value = map_desc->map_value();
  ASSERT_TRUE(map_value != nullptr);
  EXPECT_EQ(map_value->name(), "value");
  EXPECT_EQ(map_value->number(), 2);

  EXPECT_EQ(message_->map_key(), nullptr);
  EXPECT_EQ(message_->map_value(), nullptr);
}

TEST_F(DescriptorTest, FieldHasDefault) {
  EXPECT_FALSE(foo_->has_default_value());
  EXPECT_FALSE(bar_->has_default_value());
  EXPECT_FALSE(baz_->has_default_value());
  EXPECT_FALSE(moo_->has_default_value());
}

TEST_F(DescriptorTest, FieldContainingType) {
  EXPECT_EQ(message_, foo_->containing_type());
  EXPECT_EQ(message_, bar_->containing_type());
  EXPECT_EQ(message_, baz_->containing_type());
  EXPECT_EQ(message_, moo_->containing_type());

  EXPECT_EQ(message2_, foo2_->containing_type());
  EXPECT_EQ(message2_, bar2_->containing_type());
  EXPECT_EQ(message2_, mooo2_->containing_type());
}

TEST_F(DescriptorTest, FieldMessageType) {
  EXPECT_TRUE(foo_->message_type() == nullptr);
  EXPECT_TRUE(bar_->message_type() == nullptr);

  EXPECT_EQ(foreign_, baz_->message_type());
  EXPECT_EQ(foreign_, moo_->message_type());
}

TEST_F(DescriptorTest, FieldEnumType) {
  EXPECT_TRUE(foo_->enum_type() == nullptr);
  EXPECT_TRUE(baz_->enum_type() == nullptr);
  EXPECT_TRUE(moo_->enum_type() == nullptr);

  EXPECT_EQ(enum_, bar_->enum_type());
}

TEST_F(DescriptorTest, AbslStringifyWorks) {
  EXPECT_THAT(absl::StrFormat("%v", *message_),
              HasSubstr(message_->full_name()));
  EXPECT_THAT(absl::StrFormat("%v", *foo_), HasSubstr(foo_->name()));
}


// ===================================================================

// Test simple flat messages and fields.
class OneofDescriptorTest : public testing::Test {
 protected:
  void SetUp() override {
    // Build descriptors for the following definitions:
    //
    //   package garply;
    //   message TestOneof {
    //     optional int32 a = 1;
    //     oneof foo {
    //       string b = 2;
    //       TestOneof c = 3;
    //     }
    //     oneof bar {
    //       float d = 4;
    //     }
    //   }

    FileDescriptorProto baz_file;
    baz_file.set_name("baz.proto");
    baz_file.set_package("garply");

    DescriptorProto* oneof_message = AddMessage(&baz_file, "TestOneof");
    oneof_message->add_oneof_decl()->set_name("foo");
    oneof_message->add_oneof_decl()->set_name("bar");

    AddField(oneof_message, "a", 1, FieldDescriptorProto::LABEL_OPTIONAL,
             FieldDescriptorProto::TYPE_INT32);
    AddField(oneof_message, "b", 2, FieldDescriptorProto::LABEL_OPTIONAL,
             FieldDescriptorProto::TYPE_STRING);
    oneof_message->mutable_field(1)->set_oneof_index(0);
    AddField(oneof_message, "c", 3, FieldDescriptorProto::LABEL_OPTIONAL,
             FieldDescriptorProto::TYPE_MESSAGE);
    oneof_message->mutable_field(2)->set_oneof_index(0);
    oneof_message->mutable_field(2)->set_type_name("TestOneof");

    AddField(oneof_message, "d", 4, FieldDescriptorProto::LABEL_OPTIONAL,
             FieldDescriptorProto::TYPE_FLOAT);
    oneof_message->mutable_field(3)->set_oneof_index(1);

    // Build the descriptors and get the pointers.
    baz_file_ = pool_.BuildFile(baz_file);
    ASSERT_TRUE(baz_file_ != nullptr);

    ASSERT_EQ(1, baz_file_->message_type_count());
    oneof_message_ = baz_file_->message_type(0);

    ASSERT_EQ(2, oneof_message_->oneof_decl_count());
    oneof_ = oneof_message_->oneof_decl(0);
    oneof2_ = oneof_message_->oneof_decl(1);

    ASSERT_EQ(4, oneof_message_->field_count());
    a_ = oneof_message_->field(0);
    b_ = oneof_message_->field(1);
    c_ = oneof_message_->field(2);
    d_ = oneof_message_->field(3);
  }

  DescriptorPool pool_;

  const FileDescriptor* baz_file_;

  const Descriptor* oneof_message_;

  const OneofDescriptor* oneof_;
  const OneofDescriptor* oneof2_;
  const FieldDescriptor* a_;
  const FieldDescriptor* b_;
  const FieldDescriptor* c_;
  const FieldDescriptor* d_;
};

TEST_F(OneofDescriptorTest, Normal) {
  EXPECT_EQ("foo", oneof_->name());
  EXPECT_EQ("garply.TestOneof.foo", oneof_->full_name());
  EXPECT_EQ(0, oneof_->index());
  ASSERT_EQ(2, oneof_->field_count());
  EXPECT_EQ(b_, oneof_->field(0));
  EXPECT_EQ(c_, oneof_->field(1));
  EXPECT_TRUE(a_->containing_oneof() == nullptr);
  EXPECT_EQ(oneof_, b_->containing_oneof());
  EXPECT_EQ(oneof_, c_->containing_oneof());
}

TEST_F(OneofDescriptorTest, FindByName) {
  EXPECT_EQ(oneof_, oneof_message_->FindOneofByName("foo"));
  EXPECT_EQ(oneof2_, oneof_message_->FindOneofByName("bar"));
  EXPECT_TRUE(oneof_message_->FindOneofByName("no_such_oneof") == nullptr);
}

TEST_F(OneofDescriptorTest, AbslStringifyWorks) {
  EXPECT_THAT(absl::StrFormat("%v", *oneof_), HasSubstr(oneof_->name()));
}

// ===================================================================

class StylizedFieldNamesTest : public testing::Test {
 protected:
  void SetUp() override {
    FileDescriptorProto file;
    file.set_name("foo.proto");

    AddExtensionRange(AddMessage(&file, "ExtendableMessage"), 1, 1000);

    DescriptorProto* message = AddMessage(&file, "TestMessage");
    PROTOBUF_IGNORE_DEPRECATION_START
    message->mutable_options()->set_deprecated_legacy_json_field_conflicts(
        true);
    PROTOBUF_IGNORE_DEPRECATION_STOP
    AddField(message, "foo_foo", 1, FieldDescriptorProto::LABEL_OPTIONAL,
             FieldDescriptorProto::TYPE_INT32);
    AddField(message, "FooBar", 2, FieldDescriptorProto::LABEL_OPTIONAL,
             FieldDescriptorProto::TYPE_INT32);
    AddField(message, "fooBaz", 3, FieldDescriptorProto::LABEL_OPTIONAL,
             FieldDescriptorProto::TYPE_INT32);
    AddField(message, "fooFoo", 4,  // Camel-case conflict with foo_foo.
             FieldDescriptorProto::LABEL_OPTIONAL,
             FieldDescriptorProto::TYPE_INT32);
    AddField(message, "foobar", 5,  // Lower-case conflict with FooBar.
             FieldDescriptorProto::LABEL_OPTIONAL,
             FieldDescriptorProto::TYPE_INT32);

    AddNestedExtension(message, "ExtendableMessage", "bar_foo", 1,
                       FieldDescriptorProto::LABEL_OPTIONAL,
                       FieldDescriptorProto::TYPE_INT32);
    AddNestedExtension(message, "ExtendableMessage", "BarBar", 2,
                       FieldDescriptorProto::LABEL_OPTIONAL,
                       FieldDescriptorProto::TYPE_INT32);
    AddNestedExtension(message, "ExtendableMessage", "BarBaz", 3,
                       FieldDescriptorProto::LABEL_OPTIONAL,
                       FieldDescriptorProto::TYPE_INT32);
    AddNestedExtension(message, "ExtendableMessage", "barFoo", 4,  // Conflict
                       FieldDescriptorProto::LABEL_OPTIONAL,
                       FieldDescriptorProto::TYPE_INT32);
    AddNestedExtension(message, "ExtendableMessage", "barbar", 5,  // Conflict
                       FieldDescriptorProto::LABEL_OPTIONAL,
                       FieldDescriptorProto::TYPE_INT32);

    AddExtension(&file, "ExtendableMessage", "baz_foo", 11,
                 FieldDescriptorProto::LABEL_OPTIONAL,
                 FieldDescriptorProto::TYPE_INT32);
    AddExtension(&file, "ExtendableMessage", "BazBar", 12,
                 FieldDescriptorProto::LABEL_OPTIONAL,
                 FieldDescriptorProto::TYPE_INT32);
    AddExtension(&file, "ExtendableMessage", "BazBaz", 13,
                 FieldDescriptorProto::LABEL_OPTIONAL,
                 FieldDescriptorProto::TYPE_INT32);
    AddExtension(&file, "ExtendableMessage", "bazFoo", 14,  // Conflict
                 FieldDescriptorProto::LABEL_OPTIONAL,
                 FieldDescriptorProto::TYPE_INT32);
    AddExtension(&file, "ExtendableMessage", "bazbar", 15,  // Conflict
                 FieldDescriptorProto::LABEL_OPTIONAL,
                 FieldDescriptorProto::TYPE_INT32);

    file_ = pool_.BuildFile(file);
    ASSERT_TRUE(file_ != nullptr);
    ASSERT_EQ(2, file_->message_type_count());
    message_ = file_->message_type(1);
    ASSERT_EQ("TestMessage", message_->name());
    ASSERT_EQ(5, message_->field_count());
    ASSERT_EQ(5, message_->extension_count());
    ASSERT_EQ(5, file_->extension_count());
  }

  DescriptorPool pool_;
  const FileDescriptor* file_;
  const Descriptor* message_;
};

TEST_F(StylizedFieldNamesTest, LowercaseName) {
  EXPECT_EQ("foo_foo", message_->field(0)->lowercase_name());
  EXPECT_EQ("foobar", message_->field(1)->lowercase_name());
  EXPECT_EQ("foobaz", message_->field(2)->lowercase_name());
  EXPECT_EQ("foofoo", message_->field(3)->lowercase_name());
  EXPECT_EQ("foobar", message_->field(4)->lowercase_name());

  EXPECT_EQ("bar_foo", message_->extension(0)->lowercase_name());
  EXPECT_EQ("barbar", message_->extension(1)->lowercase_name());
  EXPECT_EQ("barbaz", message_->extension(2)->lowercase_name());
  EXPECT_EQ("barfoo", message_->extension(3)->lowercase_name());
  EXPECT_EQ("barbar", message_->extension(4)->lowercase_name());

  EXPECT_EQ("baz_foo", file_->extension(0)->lowercase_name());
  EXPECT_EQ("bazbar", file_->extension(1)->lowercase_name());
  EXPECT_EQ("bazbaz", file_->extension(2)->lowercase_name());
  EXPECT_EQ("bazfoo", file_->extension(3)->lowercase_name());
  EXPECT_EQ("bazbar", file_->extension(4)->lowercase_name());
}

TEST_F(StylizedFieldNamesTest, CamelcaseName) {
  EXPECT_EQ("fooFoo", message_->field(0)->camelcase_name());
  EXPECT_EQ("fooBar", message_->field(1)->camelcase_name());
  EXPECT_EQ("fooBaz", message_->field(2)->camelcase_name());
  EXPECT_EQ("fooFoo", message_->field(3)->camelcase_name());
  EXPECT_EQ("foobar", message_->field(4)->camelcase_name());

  EXPECT_EQ("barFoo", message_->extension(0)->camelcase_name());
  EXPECT_EQ("barBar", message_->extension(1)->camelcase_name());
  EXPECT_EQ("barBaz", message_->extension(2)->camelcase_name());
  EXPECT_EQ("barFoo", message_->extension(3)->camelcase_name());
  EXPECT_EQ("barbar", message_->extension(4)->camelcase_name());

  EXPECT_EQ("bazFoo", file_->extension(0)->camelcase_name());
  EXPECT_EQ("bazBar", file_->extension(1)->camelcase_name());
  EXPECT_EQ("bazBaz", file_->extension(2)->camelcase_name());
  EXPECT_EQ("bazFoo", file_->extension(3)->camelcase_name());
  EXPECT_EQ("bazbar", file_->extension(4)->camelcase_name());
}

TEST_F(StylizedFieldNamesTest, FindByLowercaseName) {
  EXPECT_EQ(message_->field(0), message_->FindFieldByLowercaseName("foo_foo"));
  EXPECT_THAT(message_->FindFieldByLowercaseName("foobar"),
              AnyOf(message_->field(1), message_->field(4)));
  EXPECT_EQ(message_->field(2), message_->FindFieldByLowercaseName("foobaz"));
  EXPECT_TRUE(message_->FindFieldByLowercaseName("FooBar") == nullptr);
  EXPECT_TRUE(message_->FindFieldByLowercaseName("fooBaz") == nullptr);
  EXPECT_TRUE(message_->FindFieldByLowercaseName("bar_foo") == nullptr);
  EXPECT_TRUE(message_->FindFieldByLowercaseName("nosuchfield") == nullptr);

  EXPECT_EQ(message_->extension(0),
            message_->FindExtensionByLowercaseName("bar_foo"));
  EXPECT_THAT(message_->FindExtensionByLowercaseName("barbar"),
              AnyOf(message_->extension(1), message_->extension(4)));
  EXPECT_EQ(message_->extension(2),
            message_->FindExtensionByLowercaseName("barbaz"));
  EXPECT_TRUE(message_->FindExtensionByLowercaseName("BarBar") == nullptr);
  EXPECT_TRUE(message_->FindExtensionByLowercaseName("barBaz") == nullptr);
  EXPECT_TRUE(message_->FindExtensionByLowercaseName("foo_foo") == nullptr);
  EXPECT_TRUE(message_->FindExtensionByLowercaseName("nosuchfield") == nullptr);

  EXPECT_EQ(file_->extension(0),
            file_->FindExtensionByLowercaseName("baz_foo"));
  EXPECT_THAT(file_->FindExtensionByLowercaseName("bazbar"),
              AnyOf(file_->extension(1), file_->extension(4)));
  EXPECT_EQ(file_->extension(2), file_->FindExtensionByLowercaseName("bazbaz"));
  EXPECT_TRUE(file_->FindExtensionByLowercaseName("BazBar") == nullptr);
  EXPECT_TRUE(file_->FindExtensionByLowercaseName("bazBaz") == nullptr);
  EXPECT_TRUE(file_->FindExtensionByLowercaseName("nosuchfield") == nullptr);
}

TEST_F(StylizedFieldNamesTest, FindByCamelcaseName) {
  // Conflict (here, foo_foo and fooFoo) always resolves to the field with
  // the lower field number.
  EXPECT_EQ(message_->field(0), message_->FindFieldByCamelcaseName("fooFoo"));
  EXPECT_EQ(message_->field(1), message_->FindFieldByCamelcaseName("fooBar"));
  EXPECT_EQ(message_->field(2), message_->FindFieldByCamelcaseName("fooBaz"));
  EXPECT_TRUE(message_->FindFieldByCamelcaseName("foo_foo") == nullptr);
  EXPECT_TRUE(message_->FindFieldByCamelcaseName("FooBar") == nullptr);
  EXPECT_TRUE(message_->FindFieldByCamelcaseName("barFoo") == nullptr);
  EXPECT_TRUE(message_->FindFieldByCamelcaseName("nosuchfield") == nullptr);

  // Conflict (here, bar_foo and barFoo) always resolves to the field with
  // the lower field number.
  EXPECT_EQ(message_->extension(0),
            message_->FindExtensionByCamelcaseName("barFoo"));
  EXPECT_EQ(message_->extension(1),
            message_->FindExtensionByCamelcaseName("barBar"));
  EXPECT_EQ(message_->extension(2),
            message_->FindExtensionByCamelcaseName("barBaz"));
  EXPECT_TRUE(message_->FindExtensionByCamelcaseName("bar_foo") == nullptr);
  EXPECT_TRUE(message_->FindExtensionByCamelcaseName("BarBar") == nullptr);
  EXPECT_TRUE(message_->FindExtensionByCamelcaseName("fooFoo") == nullptr);
  EXPECT_TRUE(message_->FindExtensionByCamelcaseName("nosuchfield") == nullptr);

  // Conflict (here, baz_foo and bazFoo) always resolves to the field with
  // the lower field number.
  EXPECT_EQ(file_->extension(0), file_->FindExtensionByCamelcaseName("bazFoo"));
  EXPECT_EQ(file_->extension(1), file_->FindExtensionByCamelcaseName("bazBar"));
  EXPECT_EQ(file_->extension(2), file_->FindExtensionByCamelcaseName("bazBaz"));
  EXPECT_TRUE(file_->FindExtensionByCamelcaseName("baz_foo") == nullptr);
  EXPECT_TRUE(file_->FindExtensionByCamelcaseName("BazBar") == nullptr);
  EXPECT_TRUE(file_->FindExtensionByCamelcaseName("nosuchfield") == nullptr);
}

// ===================================================================

// Test enum descriptors.
class EnumDescriptorTest : public testing::Test {
 protected:
  void SetUp() override {
    // Build descriptors for the following definitions:
    //
    //   // in "foo.proto"
    //   enum TestEnum {
    //     FOO = 1;
    //     BAR = 2;
    //   }
    //
    //   // in "bar.proto"
    //   package corge.grault;
    //   enum TestEnum2 {
    //     FOO = 1;
    //     BAZ = 3;
    //   }
    //
    // TestEnum2 is primarily here to test FindValueByName and friends.
    // All enums created from the same DescriptorPool share the same lookup
    // table, so we need to insure that they don't interfere.

    // TestEnum
    FileDescriptorProto foo_file;
    foo_file.set_name("foo.proto");

    EnumDescriptorProto* enum_proto = AddEnum(&foo_file, "TestEnum");
    AddEnumValue(enum_proto, "FOO", 1);
    AddEnumValue(enum_proto, "BAR", 2);

    // TestEnum2
    FileDescriptorProto bar_file;
    bar_file.set_name("bar.proto");
    bar_file.set_package("corge.grault");

    EnumDescriptorProto* enum2_proto = AddEnum(&bar_file, "TestEnum2");
    AddEnumValue(enum2_proto, "FOO", 1);
    AddEnumValue(enum2_proto, "BAZ", 3);

    // Build the descriptors and get the pointers.
    foo_file_ = pool_.BuildFile(foo_file);
    ASSERT_TRUE(foo_file_ != nullptr);

    bar_file_ = pool_.BuildFile(bar_file);
    ASSERT_TRUE(bar_file_ != nullptr);

    ASSERT_EQ(1, foo_file_->enum_type_count());
    enum_ = foo_file_->enum_type(0);

    ASSERT_EQ(2, enum_->value_count());
    foo_ = enum_->value(0);
    bar_ = enum_->value(1);

    ASSERT_EQ(1, bar_file_->enum_type_count());
    enum2_ = bar_file_->enum_type(0);

    ASSERT_EQ(2, enum2_->value_count());
    foo2_ = enum2_->value(0);
    baz2_ = enum2_->value(1);
  }

  DescriptorPool pool_;

  const FileDescriptor* foo_file_;
  const FileDescriptor* bar_file_;

  const EnumDescriptor* enum_;
  const EnumDescriptor* enum2_;

  const EnumValueDescriptor* foo_;
  const EnumValueDescriptor* bar_;

  const EnumValueDescriptor* foo2_;
  const EnumValueDescriptor* baz2_;
};

TEST_F(EnumDescriptorTest, Name) {
  EXPECT_EQ("TestEnum", enum_->name());
  EXPECT_EQ("TestEnum", enum_->full_name());
  EXPECT_EQ(foo_file_, enum_->file());

  EXPECT_EQ("TestEnum2", enum2_->name());
  EXPECT_EQ("corge.grault.TestEnum2", enum2_->full_name());
  EXPECT_EQ(bar_file_, enum2_->file());
}

TEST_F(EnumDescriptorTest, ContainingType) {
  EXPECT_TRUE(enum_->containing_type() == nullptr);
  EXPECT_TRUE(enum2_->containing_type() == nullptr);
}

TEST_F(EnumDescriptorTest, ValuesByIndex) {
  ASSERT_EQ(2, enum_->value_count());
  EXPECT_EQ(foo_, enum_->value(0));
  EXPECT_EQ(bar_, enum_->value(1));
}

TEST_F(EnumDescriptorTest, FindValueByName) {
  EXPECT_EQ(foo_, enum_->FindValueByName("FOO"));
  EXPECT_EQ(bar_, enum_->FindValueByName("BAR"));
  EXPECT_EQ(foo2_, enum2_->FindValueByName("FOO"));
  EXPECT_EQ(baz2_, enum2_->FindValueByName("BAZ"));

  EXPECT_TRUE(enum_->FindValueByName("NO_SUCH_VALUE") == nullptr);
  EXPECT_TRUE(enum_->FindValueByName("BAZ") == nullptr);
  EXPECT_TRUE(enum2_->FindValueByName("BAR") == nullptr);
}

TEST_F(EnumDescriptorTest, FindValueByNumber) {
  EXPECT_EQ(foo_, enum_->FindValueByNumber(1));
  EXPECT_EQ(bar_, enum_->FindValueByNumber(2));
  EXPECT_EQ(foo2_, enum2_->FindValueByNumber(1));
  EXPECT_EQ(baz2_, enum2_->FindValueByNumber(3));

  EXPECT_TRUE(enum_->FindValueByNumber(416) == nullptr);
  EXPECT_TRUE(enum_->FindValueByNumber(3) == nullptr);
  EXPECT_TRUE(enum2_->FindValueByNumber(2) == nullptr);
}

TEST_F(EnumDescriptorTest, ValueName) {
  EXPECT_EQ("FOO", foo_->name());
  EXPECT_EQ("BAR", bar_->name());
}

TEST_F(EnumDescriptorTest, ValueFullName) {
  EXPECT_EQ("FOO", foo_->full_name());
  EXPECT_EQ("BAR", bar_->full_name());
  EXPECT_EQ("corge.grault.FOO", foo2_->full_name());
  EXPECT_EQ("corge.grault.BAZ", baz2_->full_name());
}

TEST_F(EnumDescriptorTest, ValueIndex) {
  EXPECT_EQ(0, foo_->index());
  EXPECT_EQ(1, bar_->index());
}

TEST_F(EnumDescriptorTest, ValueNumber) {
  EXPECT_EQ(1, foo_->number());
  EXPECT_EQ(2, bar_->number());
}

TEST_F(EnumDescriptorTest, ValueType) {
  EXPECT_EQ(enum_, foo_->type());
  EXPECT_EQ(enum_, bar_->type());
  EXPECT_EQ(enum2_, foo2_->type());
  EXPECT_EQ(enum2_, baz2_->type());
}

TEST_F(EnumDescriptorTest, IsClosed) {
  // enum_ is proto2.
  EXPECT_TRUE(enum_->is_closed());

  // Make a proto3 version of enum_.
  FileDescriptorProto foo_file3;
  foo_file_->CopyTo(&foo_file3);
  foo_file3.set_syntax("proto3");

  // Make this valid proto3 by making the first enum value be zero.
  foo_file3.mutable_enum_type(0)->mutable_value(0)->set_number(0);

  DescriptorPool pool3;
  const EnumDescriptor* enum3 = pool3.BuildFile(foo_file3)->enum_type(0);
  EXPECT_FALSE(enum3->is_closed());
}

TEST_F(EnumDescriptorTest, AbslStringifyWorks) {
  EXPECT_THAT(absl::StrFormat("%v", *enum_), HasSubstr(enum_->full_name()));
  EXPECT_THAT(absl::StrFormat("%v", *foo_), HasSubstr(foo_->name()));
}

// ===================================================================

// Test service descriptors.
class ServiceDescriptorTest : public testing::Test {
 protected:
  void SetUp() override {
    // Build descriptors for the following messages and service:
    //    // in "foo.proto"
    //    message FooRequest  {}
    //    message FooResponse {}
    //    message BarRequest  {}
    //    message BarResponse {}
    //    message BazRequest  {}
    //    message BazResponse {}
    //
    //    service TestService {
    //      rpc Foo(FooRequest) returns (FooResponse);
    //      rpc Bar(BarRequest) returns (BarResponse);
    //    }
    //
    //    // in "bar.proto"
    //    package corge.grault
    //    service TestService2 {
    //      rpc Foo(FooRequest) returns (FooResponse);
    //      rpc Baz(BazRequest) returns (BazResponse);
    //    }

    FileDescriptorProto foo_file;
    foo_file.set_name("foo.proto");

    AddMessage(&foo_file, "FooRequest");
    AddMessage(&foo_file, "FooResponse");
    AddMessage(&foo_file, "BarRequest");
    AddMessage(&foo_file, "BarResponse");
    AddMessage(&foo_file, "BazRequest");
    AddMessage(&foo_file, "BazResponse");

    ServiceDescriptorProto* service = AddService(&foo_file, "TestService");
    AddMethod(service, "Foo", "FooRequest", "FooResponse");
    AddMethod(service, "Bar", "BarRequest", "BarResponse");

    FileDescriptorProto bar_file;
    bar_file.set_name("bar.proto");
    bar_file.set_package("corge.grault");
    bar_file.add_dependency("foo.proto");

    ServiceDescriptorProto* service2 = AddService(&bar_file, "TestService2");
    AddMethod(service2, "Foo", "FooRequest", "FooResponse");
    AddMethod(service2, "Baz", "BazRequest", "BazResponse");

    // Build the descriptors and get the pointers.
    foo_file_ = pool_.BuildFile(foo_file);
    ASSERT_TRUE(foo_file_ != nullptr);

    bar_file_ = pool_.BuildFile(bar_file);
    ASSERT_TRUE(bar_file_ != nullptr);

    ASSERT_EQ(6, foo_file_->message_type_count());
    foo_request_ = foo_file_->message_type(0);
    foo_response_ = foo_file_->message_type(1);
    bar_request_ = foo_file_->message_type(2);
    bar_response_ = foo_file_->message_type(3);
    baz_request_ = foo_file_->message_type(4);
    baz_response_ = foo_file_->message_type(5);

    ASSERT_EQ(1, foo_file_->service_count());
    service_ = foo_file_->service(0);

    ASSERT_EQ(2, service_->method_count());
    foo_ = service_->method(0);
    bar_ = service_->method(1);

    ASSERT_EQ(1, bar_file_->service_count());
    service2_ = bar_file_->service(0);

    ASSERT_EQ(2, service2_->method_count());
    foo2_ = service2_->method(0);
    baz2_ = service2_->method(1);
  }

  DescriptorPool pool_;

  const FileDescriptor* foo_file_;
  const FileDescriptor* bar_file_;

  const Descriptor* foo_request_;
  const Descriptor* foo_response_;
  const Descriptor* bar_request_;
  const Descriptor* bar_response_;
  const Descriptor* baz_request_;
  const Descriptor* baz_response_;

  const ServiceDescriptor* service_;
  const ServiceDescriptor* service2_;

  const MethodDescriptor* foo_;
  const MethodDescriptor* bar_;

  const MethodDescriptor* foo2_;
  const MethodDescriptor* baz2_;
};

TEST_F(ServiceDescriptorTest, Name) {
  EXPECT_EQ("TestService", service_->name());
  EXPECT_EQ("TestService", service_->full_name());
  EXPECT_EQ(foo_file_, service_->file());

  EXPECT_EQ("TestService2", service2_->name());
  EXPECT_EQ("corge.grault.TestService2", service2_->full_name());
  EXPECT_EQ(bar_file_, service2_->file());
}

TEST_F(ServiceDescriptorTest, MethodsByIndex) {
  ASSERT_EQ(2, service_->method_count());
  EXPECT_EQ(foo_, service_->method(0));
  EXPECT_EQ(bar_, service_->method(1));
}

TEST_F(ServiceDescriptorTest, FindMethodByName) {
  EXPECT_EQ(foo_, service_->FindMethodByName("Foo"));
  EXPECT_EQ(bar_, service_->FindMethodByName("Bar"));
  EXPECT_EQ(foo2_, service2_->FindMethodByName("Foo"));
  EXPECT_EQ(baz2_, service2_->FindMethodByName("Baz"));

  EXPECT_TRUE(service_->FindMethodByName("NoSuchMethod") == nullptr);
  EXPECT_TRUE(service_->FindMethodByName("Baz") == nullptr);
  EXPECT_TRUE(service2_->FindMethodByName("Bar") == nullptr);
}

TEST_F(ServiceDescriptorTest, MethodName) {
  EXPECT_EQ("Foo", foo_->name());
  EXPECT_EQ("Bar", bar_->name());
}
TEST_F(ServiceDescriptorTest, MethodFullName) {
  EXPECT_EQ("TestService.Foo", foo_->full_name());
  EXPECT_EQ("TestService.Bar", bar_->full_name());
  EXPECT_EQ("corge.grault.TestService2.Foo", foo2_->full_name());
  EXPECT_EQ("corge.grault.TestService2.Baz", baz2_->full_name());
}

TEST_F(ServiceDescriptorTest, MethodIndex) {
  EXPECT_EQ(0, foo_->index());
  EXPECT_EQ(1, bar_->index());
}

TEST_F(ServiceDescriptorTest, MethodParent) {
  EXPECT_EQ(service_, foo_->service());
  EXPECT_EQ(service_, bar_->service());
}

TEST_F(ServiceDescriptorTest, MethodInputType) {
  EXPECT_EQ(foo_request_, foo_->input_type());
  EXPECT_EQ(bar_request_, bar_->input_type());
}

TEST_F(ServiceDescriptorTest, MethodOutputType) {
  EXPECT_EQ(foo_response_, foo_->output_type());
  EXPECT_EQ(bar_response_, bar_->output_type());
}

TEST_F(ServiceDescriptorTest, AbslStringifyWorks) {
  EXPECT_THAT(absl::StrFormat("%v", *service_), HasSubstr(service_->name()));
  EXPECT_THAT(absl::StrFormat("%v", *foo_), HasSubstr(foo_->name()));
}

// ===================================================================

// Test nested types.
class NestedDescriptorTest : public testing::Test {
 protected:
  void SetUp() override {
    // Build descriptors for the following definitions:
    //
    //   // in "foo.proto"
    //   message TestMessage {
    //     message Foo {}
    //     message Bar {}
    //     enum Baz { A = 1; }
    //     enum Moo { B = 1; }
    //   }
    //
    //   // in "bar.proto"
    //   package corge.grault;
    //   message TestMessage2 {
    //     message Foo {}
    //     message Baz {}
    //     enum Moo  { A = 1; }
    //     enum Mooo { C = 1; }
    //   }
    //
    // TestMessage2 is primarily here to test FindNestedTypeByName and friends.
    // All messages created from the same DescriptorPool share the same lookup
    // table, so we need to insure that they don't interfere.
    //
    // We add enum values to the enums in order to test searching for enum
    // values across a message's scope.

    FileDescriptorProto foo_file;
    foo_file.set_name("foo.proto");

    DescriptorProto* message = AddMessage(&foo_file, "TestMessage");
    AddNestedMessage(message, "Foo");
    AddNestedMessage(message, "Bar");
    EnumDescriptorProto* baz = AddNestedEnum(message, "Baz");
    AddEnumValue(baz, "A", 1);
    EnumDescriptorProto* moo = AddNestedEnum(message, "Moo");
    AddEnumValue(moo, "B", 1);

    FileDescriptorProto bar_file;
    bar_file.set_name("bar.proto");
    bar_file.set_package("corge.grault");

    DescriptorProto* message2 = AddMessage(&bar_file, "TestMessage2");
    AddNestedMessage(message2, "Foo");
    AddNestedMessage(message2, "Baz");
    EnumDescriptorProto* moo2 = AddNestedEnum(message2, "Moo");
    AddEnumValue(moo2, "A", 1);
    EnumDescriptorProto* mooo2 = AddNestedEnum(message2, "Mooo");
    AddEnumValue(mooo2, "C", 1);

    // Build the descriptors and get the pointers.
    foo_file_ = pool_.BuildFile(foo_file);
    ASSERT_TRUE(foo_file_ != nullptr);

    bar_file_ = pool_.BuildFile(bar_file);
    ASSERT_TRUE(bar_file_ != nullptr);

    ASSERT_EQ(1, foo_file_->message_type_count());
    message_ = foo_file_->message_type(0);

    ASSERT_EQ(2, message_->nested_type_count());
    foo_ = message_->nested_type(0);
    bar_ = message_->nested_type(1);

    ASSERT_EQ(2, message_->enum_type_count());
    baz_ = message_->enum_type(0);
    moo_ = message_->enum_type(1);

    ASSERT_EQ(1, baz_->value_count());
    a_ = baz_->value(0);
    ASSERT_EQ(1, moo_->value_count());
    b_ = moo_->value(0);

    ASSERT_EQ(1, bar_file_->message_type_count());
    message2_ = bar_file_->message_type(0);

    ASSERT_EQ(2, message2_->nested_type_count());
    foo2_ = message2_->nested_type(0);
    baz2_ = message2_->nested_type(1);

    ASSERT_EQ(2, message2_->enum_type_count());
    moo2_ = message2_->enum_type(0);
    mooo2_ = message2_->enum_type(1);

    ASSERT_EQ(1, moo2_->value_count());
    a2_ = moo2_->value(0);
    ASSERT_EQ(1, mooo2_->value_count());
    c2_ = mooo2_->value(0);
  }

  DescriptorPool pool_;

  const FileDescriptor* foo_file_;
  const FileDescriptor* bar_file_;

  const Descriptor* message_;
  const Descriptor* message2_;

  const Descriptor* foo_;
  const Descriptor* bar_;
  const EnumDescriptor* baz_;
  const EnumDescriptor* moo_;
  const EnumValueDescriptor* a_;
  const EnumValueDescriptor* b_;

  const Descriptor* foo2_;
  const Descriptor* baz2_;
  const EnumDescriptor* moo2_;
  const EnumDescriptor* mooo2_;
  const EnumValueDescriptor* a2_;
  const EnumValueDescriptor* c2_;
};

TEST_F(NestedDescriptorTest, MessageName) {
  EXPECT_EQ("Foo", foo_->name());
  EXPECT_EQ("Bar", bar_->name());
  EXPECT_EQ("Foo", foo2_->name());
  EXPECT_EQ("Baz", baz2_->name());

  EXPECT_EQ("TestMessage.Foo", foo_->full_name());
  EXPECT_EQ("TestMessage.Bar", bar_->full_name());
  EXPECT_EQ("corge.grault.TestMessage2.Foo", foo2_->full_name());
  EXPECT_EQ("corge.grault.TestMessage2.Baz", baz2_->full_name());
}

TEST_F(NestedDescriptorTest, MessageContainingType) {
  EXPECT_EQ(message_, foo_->containing_type());
  EXPECT_EQ(message_, bar_->containing_type());
  EXPECT_EQ(message2_, foo2_->containing_type());
  EXPECT_EQ(message2_, baz2_->containing_type());
}

TEST_F(NestedDescriptorTest, NestedMessagesByIndex) {
  ASSERT_EQ(2, message_->nested_type_count());
  EXPECT_EQ(foo_, message_->nested_type(0));
  EXPECT_EQ(bar_, message_->nested_type(1));
}

TEST_F(NestedDescriptorTest, FindFieldByNameDoesntFindNestedTypes) {
  EXPECT_TRUE(message_->FindFieldByName("Foo") == nullptr);
  EXPECT_TRUE(message_->FindFieldByName("Moo") == nullptr);
  EXPECT_TRUE(message_->FindExtensionByName("Foo") == nullptr);
  EXPECT_TRUE(message_->FindExtensionByName("Moo") == nullptr);
}

TEST_F(NestedDescriptorTest, FindNestedTypeByName) {
  EXPECT_EQ(foo_, message_->FindNestedTypeByName("Foo"));
  EXPECT_EQ(bar_, message_->FindNestedTypeByName("Bar"));
  EXPECT_EQ(foo2_, message2_->FindNestedTypeByName("Foo"));
  EXPECT_EQ(baz2_, message2_->FindNestedTypeByName("Baz"));

  EXPECT_TRUE(message_->FindNestedTypeByName("NoSuchType") == nullptr);
  EXPECT_TRUE(message_->FindNestedTypeByName("Baz") == nullptr);
  EXPECT_TRUE(message2_->FindNestedTypeByName("Bar") == nullptr);

  EXPECT_TRUE(message_->FindNestedTypeByName("Moo") == nullptr);
}

TEST_F(NestedDescriptorTest, EnumName) {
  EXPECT_EQ("Baz", baz_->name());
  EXPECT_EQ("Moo", moo_->name());
  EXPECT_EQ("Moo", moo2_->name());
  EXPECT_EQ("Mooo", mooo2_->name());

  EXPECT_EQ("TestMessage.Baz", baz_->full_name());
  EXPECT_EQ("TestMessage.Moo", moo_->full_name());
  EXPECT_EQ("corge.grault.TestMessage2.Moo", moo2_->full_name());
  EXPECT_EQ("corge.grault.TestMessage2.Mooo", mooo2_->full_name());
}

TEST_F(NestedDescriptorTest, EnumContainingType) {
  EXPECT_EQ(message_, baz_->containing_type());
  EXPECT_EQ(message_, moo_->containing_type());
  EXPECT_EQ(message2_, moo2_->containing_type());
  EXPECT_EQ(message2_, mooo2_->containing_type());
}

TEST_F(NestedDescriptorTest, NestedEnumsByIndex) {
  ASSERT_EQ(2, message_->nested_type_count());
  EXPECT_EQ(foo_, message_->nested_type(0));
  EXPECT_EQ(bar_, message_->nested_type(1));
}

TEST_F(NestedDescriptorTest, FindEnumTypeByName) {
  EXPECT_EQ(baz_, message_->FindEnumTypeByName("Baz"));
  EXPECT_EQ(moo_, message_->FindEnumTypeByName("Moo"));
  EXPECT_EQ(moo2_, message2_->FindEnumTypeByName("Moo"));
  EXPECT_EQ(mooo2_, message2_->FindEnumTypeByName("Mooo"));

  EXPECT_TRUE(message_->FindEnumTypeByName("NoSuchType") == nullptr);
  EXPECT_TRUE(message_->FindEnumTypeByName("Mooo") == nullptr);
  EXPECT_TRUE(message2_->FindEnumTypeByName("Baz") == nullptr);

  EXPECT_TRUE(message_->FindEnumTypeByName("Foo") == nullptr);
}

TEST_F(NestedDescriptorTest, FindEnumValueByName) {
  EXPECT_EQ(a_, message_->FindEnumValueByName("A"));
  EXPECT_EQ(b_, message_->FindEnumValueByName("B"));
  EXPECT_EQ(a2_, message2_->FindEnumValueByName("A"));
  EXPECT_EQ(c2_, message2_->FindEnumValueByName("C"));

  EXPECT_TRUE(message_->FindEnumValueByName("NO_SUCH_VALUE") == nullptr);
  EXPECT_TRUE(message_->FindEnumValueByName("C") == nullptr);
  EXPECT_TRUE(message2_->FindEnumValueByName("B") == nullptr);

  EXPECT_TRUE(message_->FindEnumValueByName("Foo") == nullptr);
}

// ===================================================================

// Test extensions.
class ExtensionDescriptorTest : public testing::Test {
 protected:
  void SetUp() override {
    // Build descriptors for the following definitions:
    //
    //   enum Baz {}
    //   message Moo {}
    //
    //   message Foo {
    //     extensions 10 to 19;
    //     extensions 30 to 39;
    //   }
    //   extend Foo {
    //     optional int32 foo_int32 = 10;
    //   }
    //   extend Foo {
    //     repeated TestEnum foo_enum = 19;
    //   }
    //   message Bar {
    //     optional int32 non_ext_int32 = 1;
    //     extend Foo {
    //       optional Moo foo_message = 30;
    //       repeated Moo foo_group = 39;  // (but internally set to TYPE_GROUP)
    //     }
    //   }

    FileDescriptorProto foo_file;
    foo_file.set_name("foo.proto");

    AddEmptyEnum(&foo_file, "Baz");
    AddMessage(&foo_file, "Moo");

    DescriptorProto* foo = AddMessage(&foo_file, "Foo");
    AddExtensionRange(foo, 10, 20);
    AddExtensionRange(foo, 30, 40);

    AddExtension(&foo_file, "Foo", "foo_int32", 10,
                 FieldDescriptorProto::LABEL_OPTIONAL,
                 FieldDescriptorProto::TYPE_INT32);
    AddExtension(&foo_file, "Foo", "foo_enum", 19,
                 FieldDescriptorProto::LABEL_REPEATED,
                 FieldDescriptorProto::TYPE_ENUM)
        ->set_type_name("Baz");

    DescriptorProto* bar = AddMessage(&foo_file, "Bar");
    AddField(bar, "non_ext_int32", 1, FieldDescriptorProto::LABEL_OPTIONAL,
             FieldDescriptorProto::TYPE_INT32);
    AddNestedExtension(bar, "Foo", "foo_message", 30,
                       FieldDescriptorProto::LABEL_OPTIONAL,
                       FieldDescriptorProto::TYPE_MESSAGE)
        ->set_type_name("Moo");
    AddNestedExtension(bar, "Foo", "foo_group", 39,
                       FieldDescriptorProto::LABEL_REPEATED,
                       FieldDescriptorProto::TYPE_GROUP)
        ->set_type_name("Moo");

    // Build the descriptors and get the pointers.
    foo_file_ = pool_.BuildFile(foo_file);
    ASSERT_TRUE(foo_file_ != nullptr);

    ASSERT_EQ(1, foo_file_->enum_type_count());
    baz_ = foo_file_->enum_type(0);

    ASSERT_EQ(3, foo_file_->message_type_count());
    moo_ = foo_file_->message_type(0);
    foo_ = foo_file_->message_type(1);
    bar_ = foo_file_->message_type(2);
  }

  DescriptorPool pool_;

  const FileDescriptor* foo_file_;

  const Descriptor* foo_;
  const Descriptor* bar_;
  const EnumDescriptor* baz_;
  const Descriptor* moo_;
};

TEST_F(ExtensionDescriptorTest, ExtensionRanges) {
  EXPECT_EQ(0, bar_->extension_range_count());
  ASSERT_EQ(2, foo_->extension_range_count());

  EXPECT_EQ(10, foo_->extension_range(0)->start_number());
  EXPECT_EQ(30, foo_->extension_range(1)->start_number());

  EXPECT_EQ(20, foo_->extension_range(0)->end_number());
  EXPECT_EQ(40, foo_->extension_range(1)->end_number());
}

TEST_F(ExtensionDescriptorTest, Extensions) {
  EXPECT_EQ(0, foo_->extension_count());
  ASSERT_EQ(2, foo_file_->extension_count());
  ASSERT_EQ(2, bar_->extension_count());

  EXPECT_TRUE(foo_file_->extension(0)->is_extension());
  EXPECT_TRUE(foo_file_->extension(1)->is_extension());
  EXPECT_TRUE(bar_->extension(0)->is_extension());
  EXPECT_TRUE(bar_->extension(1)->is_extension());

  EXPECT_EQ("foo_int32", foo_file_->extension(0)->name());
  EXPECT_EQ("foo_enum", foo_file_->extension(1)->name());
  EXPECT_EQ("foo_message", bar_->extension(0)->name());
  EXPECT_EQ("foo_group", bar_->extension(1)->name());

  EXPECT_EQ(10, foo_file_->extension(0)->number());
  EXPECT_EQ(19, foo_file_->extension(1)->number());
  EXPECT_EQ(30, bar_->extension(0)->number());
  EXPECT_EQ(39, bar_->extension(1)->number());

  EXPECT_EQ(FieldDescriptor::TYPE_INT32, foo_file_->extension(0)->type());
  EXPECT_EQ(FieldDescriptor::TYPE_ENUM, foo_file_->extension(1)->type());
  EXPECT_EQ(FieldDescriptor::TYPE_MESSAGE, bar_->extension(0)->type());
  EXPECT_EQ(FieldDescriptor::TYPE_GROUP, bar_->extension(1)->type());

  EXPECT_EQ(baz_, foo_file_->extension(1)->enum_type());
  EXPECT_EQ(moo_, bar_->extension(0)->message_type());
  EXPECT_EQ(moo_, bar_->extension(1)->message_type());

  EXPECT_EQ(FieldDescriptor::LABEL_OPTIONAL, foo_file_->extension(0)->label());
  EXPECT_EQ(FieldDescriptor::LABEL_REPEATED, foo_file_->extension(1)->label());
  EXPECT_EQ(FieldDescriptor::LABEL_OPTIONAL, bar_->extension(0)->label());
  EXPECT_EQ(FieldDescriptor::LABEL_REPEATED, bar_->extension(1)->label());

  EXPECT_EQ(foo_, foo_file_->extension(0)->containing_type());
  EXPECT_EQ(foo_, foo_file_->extension(1)->containing_type());
  EXPECT_EQ(foo_, bar_->extension(0)->containing_type());
  EXPECT_EQ(foo_, bar_->extension(1)->containing_type());

  EXPECT_TRUE(foo_file_->extension(0)->extension_scope() == nullptr);
  EXPECT_TRUE(foo_file_->extension(1)->extension_scope() == nullptr);
  EXPECT_EQ(bar_, bar_->extension(0)->extension_scope());
  EXPECT_EQ(bar_, bar_->extension(1)->extension_scope());
}

TEST_F(ExtensionDescriptorTest, IsExtensionNumber) {
  EXPECT_FALSE(foo_->IsExtensionNumber(9));
  EXPECT_TRUE(foo_->IsExtensionNumber(10));
  EXPECT_TRUE(foo_->IsExtensionNumber(19));
  EXPECT_FALSE(foo_->IsExtensionNumber(20));
  EXPECT_FALSE(foo_->IsExtensionNumber(29));
  EXPECT_TRUE(foo_->IsExtensionNumber(30));
  EXPECT_TRUE(foo_->IsExtensionNumber(39));
  EXPECT_FALSE(foo_->IsExtensionNumber(40));
}

TEST_F(ExtensionDescriptorTest, FindExtensionByName) {
  // Note that FileDescriptor::FindExtensionByName() is tested by
  // FileDescriptorTest.
  ASSERT_EQ(2, bar_->extension_count());

  EXPECT_EQ(bar_->extension(0), bar_->FindExtensionByName("foo_message"));
  EXPECT_EQ(bar_->extension(1), bar_->FindExtensionByName("foo_group"));

  EXPECT_TRUE(bar_->FindExtensionByName("no_such_extension") == nullptr);
  EXPECT_TRUE(foo_->FindExtensionByName("foo_int32") == nullptr);
  EXPECT_TRUE(foo_->FindExtensionByName("foo_message") == nullptr);
}

TEST_F(ExtensionDescriptorTest, FieldVsExtension) {
  EXPECT_EQ(foo_->FindFieldByName("foo_message"), nullptr);
  EXPECT_EQ(bar_->FindFieldByName("foo_message"), nullptr);
  EXPECT_NE(bar_->FindFieldByName("non_ext_int32"), nullptr);
  EXPECT_EQ(foo_->FindExtensionByName("foo_message"), nullptr);
  EXPECT_NE(bar_->FindExtensionByName("foo_message"), nullptr);
  EXPECT_EQ(bar_->FindExtensionByName("non_ext_int32"), nullptr);
}

TEST_F(ExtensionDescriptorTest, FindExtensionByPrintableName) {
  EXPECT_TRUE(pool_.FindExtensionByPrintableName(foo_, "no_such_extension") ==
              nullptr);
  EXPECT_TRUE(pool_.FindExtensionByPrintableName(bar_, "no_such_extension") ==
              nullptr);

  ASSERT_FALSE(pool_.FindExtensionByPrintableName(foo_, "Bar.foo_message") ==
               nullptr);
  ASSERT_FALSE(pool_.FindExtensionByPrintableName(foo_, "Bar.foo_group") ==
               nullptr);
  EXPECT_TRUE(pool_.FindExtensionByPrintableName(bar_, "foo_message") ==
              nullptr);
  EXPECT_TRUE(pool_.FindExtensionByPrintableName(bar_, "foo_group") == nullptr);
  EXPECT_EQ(bar_->FindExtensionByName("foo_message"),
            pool_.FindExtensionByPrintableName(foo_, "Bar.foo_message"));
  EXPECT_EQ(bar_->FindExtensionByName("foo_group"),
            pool_.FindExtensionByPrintableName(foo_, "Bar.foo_group"));

  ASSERT_FALSE(pool_.FindExtensionByPrintableName(foo_, "foo_int32") ==
               nullptr);
  ASSERT_FALSE(pool_.FindExtensionByPrintableName(foo_, "foo_enum") == nullptr);
  EXPECT_TRUE(pool_.FindExtensionByPrintableName(bar_, "foo_int32") == nullptr);
  EXPECT_TRUE(pool_.FindExtensionByPrintableName(bar_, "foo_enum") == nullptr);
  EXPECT_EQ(foo_file_->FindExtensionByName("foo_int32"),
            pool_.FindExtensionByPrintableName(foo_, "foo_int32"));
  EXPECT_EQ(foo_file_->FindExtensionByName("foo_enum"),
            pool_.FindExtensionByPrintableName(foo_, "foo_enum"));
}

TEST_F(ExtensionDescriptorTest, FindAllExtensions) {
  std::vector<const FieldDescriptor*> extensions;
  pool_.FindAllExtensions(foo_, &extensions);
  ASSERT_EQ(4, extensions.size());
  EXPECT_EQ(10, extensions[0]->number());
  EXPECT_EQ(19, extensions[1]->number());
  EXPECT_EQ(30, extensions[2]->number());
  EXPECT_EQ(39, extensions[3]->number());
}


TEST_F(ExtensionDescriptorTest, DuplicateFieldNumber) {
  DescriptorPool pool;
  FileDescriptorProto file_proto;
  // Add "google/protobuf/descriptor.proto".
  FileDescriptorProto::descriptor()->file()->CopyTo(&file_proto);
  ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);
  // Add "foo.proto":
  //   import "google/protobuf/descriptor.proto";
  //   extend google.protobuf.FieldOptions {
  //     optional int32 option1 = 1000;
  //   }
  file_proto.Clear();
  file_proto.set_name("foo.proto");
  file_proto.add_dependency("google/protobuf/descriptor.proto");
  AddExtension(&file_proto, "google.protobuf.FieldOptions", "option1", 1000,
               FieldDescriptorProto::LABEL_OPTIONAL,
               FieldDescriptorProto::TYPE_INT32);
  ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);
  // Add "bar.proto":
  //   import "google/protobuf/descriptor.proto";
  //   extend google.protobuf.FieldOptions {
  //     optional int32 option2 = 1000;
  //   }
  file_proto.Clear();
  file_proto.set_name("bar.proto");
  file_proto.add_dependency("google/protobuf/descriptor.proto");
  AddExtension(&file_proto, "google.protobuf.FieldOptions", "option2", 1000,
               FieldDescriptorProto::LABEL_OPTIONAL,
               FieldDescriptorProto::TYPE_INT32);
  // Currently we only generate a warning for conflicting extension numbers.
  // TODO: Change it to an error.
  ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);
}

// ===================================================================

// Ensure that overlapping extension ranges are not allowed.
TEST(OverlappingExtensionRangeTest, ExtensionRangeInternal) {
  // Build descriptors for the following definitions:
  //
  //   message Foo {
  //     extensions 10 to 19;
  //     extensions 15;
  //   }
  FileDescriptorProto foo_file;
  foo_file.set_name("foo.proto");

  DescriptorProto* foo = AddMessage(&foo_file, "Foo");
  AddExtensionRange(foo, 10, 20);
  AddExtensionRange(foo, 15, 16);

  DescriptorPool pool;
  MockErrorCollector error_collector;
  // The extensions ranges are invalid, so the proto shouldn't build.
  ASSERT_TRUE(pool.BuildFileCollectingErrors(foo_file, &error_collector) ==
              nullptr);
  ASSERT_EQ(
      "foo.proto: Foo: NUMBER: Extension range 15 to 15 overlaps with "
      "already-defined range 10 to 19.\n",
      error_collector.text_);
}

TEST(OverlappingExtensionRangeTest, ExtensionRangeAfter) {
  // Build descriptors for the following definitions:
  //
  //   message Foo {
  //     extensions 10 to 19;
  //     extensions 15 to 24;
  //   }
  FileDescriptorProto foo_file;
  foo_file.set_name("foo.proto");

  DescriptorProto* foo = AddMessage(&foo_file, "Foo");
  AddExtensionRange(foo, 10, 20);
  AddExtensionRange(foo, 15, 25);

  DescriptorPool pool;
  MockErrorCollector error_collector;
  // The extensions ranges are invalid, so the proto shouldn't build.
  ASSERT_TRUE(pool.BuildFileCollectingErrors(foo_file, &error_collector) ==
              nullptr);
  ASSERT_EQ(
      "foo.proto: Foo: NUMBER: Extension range 15 to 24 overlaps with "
      "already-defined range 10 to 19.\n",
      error_collector.text_);
}

TEST(OverlappingExtensionRangeTest, ExtensionRangeBefore) {
  // Build descriptors for the following definitions:
  //
  //   message Foo {
  //     extensions 10 to 19;
  //     extensions 5 to 14;
  //   }
  FileDescriptorProto foo_file;
  foo_file.set_name("foo.proto");

  DescriptorProto* foo = AddMessage(&foo_file, "Foo");
  AddExtensionRange(foo, 10, 20);
  AddExtensionRange(foo, 5, 15);

  DescriptorPool pool;
  MockErrorCollector error_collector;
  // The extensions ranges are invalid, so the proto shouldn't build.
  ASSERT_TRUE(pool.BuildFileCollectingErrors(foo_file, &error_collector) ==
              nullptr);
  ASSERT_EQ(
      "foo.proto: Foo: NUMBER: Extension range 5 to 14 overlaps with "
      "already-defined range 10 to 19.\n",
      error_collector.text_);
}

// ===================================================================

// Test reserved fields.
class ReservedDescriptorTest : public testing::Test {
 protected:
  void SetUp() override {
    // Build descriptors for the following definitions:
    //
    //   message Foo {
    //     reserved 2, 9 to 11, 15;
    //     reserved "foo", "bar";
    //   }

    FileDescriptorProto foo_file;
    foo_file.set_name("foo.proto");

    DescriptorProto* foo = AddMessage(&foo_file, "Foo");
    AddReservedRange(foo, 2, 3);
    AddReservedRange(foo, 9, 12);
    AddReservedRange(foo, 15, 16);

    foo->add_reserved_name("foo");
    foo->add_reserved_name("bar");

    // Build the descriptors and get the pointers.
    foo_file_ = pool_.BuildFile(foo_file);
    ASSERT_TRUE(foo_file_ != nullptr);

    ASSERT_EQ(1, foo_file_->message_type_count());
    foo_ = foo_file_->message_type(0);
  }

  DescriptorPool pool_;
  const FileDescriptor* foo_file_;
  const Descriptor* foo_;
};

TEST_F(ReservedDescriptorTest, ReservedRanges) {
  ASSERT_EQ(3, foo_->reserved_range_count());

  EXPECT_EQ(2, foo_->reserved_range(0)->start);
  EXPECT_EQ(3, foo_->reserved_range(0)->end);

  EXPECT_EQ(9, foo_->reserved_range(1)->start);
  EXPECT_EQ(12, foo_->reserved_range(1)->end);

  EXPECT_EQ(15, foo_->reserved_range(2)->start);
  EXPECT_EQ(16, foo_->reserved_range(2)->end);
}

TEST_F(ReservedDescriptorTest, IsReservedNumber) {
  EXPECT_FALSE(foo_->IsReservedNumber(1));
  EXPECT_TRUE(foo_->IsReservedNumber(2));
  EXPECT_FALSE(foo_->IsReservedNumber(3));
  EXPECT_FALSE(foo_->IsReservedNumber(8));
  EXPECT_TRUE(foo_->IsReservedNumber(9));
  EXPECT_TRUE(foo_->IsReservedNumber(10));
  EXPECT_TRUE(foo_->IsReservedNumber(11));
  EXPECT_FALSE(foo_->IsReservedNumber(12));
  EXPECT_FALSE(foo_->IsReservedNumber(13));
  EXPECT_FALSE(foo_->IsReservedNumber(14));
  EXPECT_TRUE(foo_->IsReservedNumber(15));
  EXPECT_FALSE(foo_->IsReservedNumber(16));
}

TEST_F(ReservedDescriptorTest, ReservedNames) {
  ASSERT_EQ(2, foo_->reserved_name_count());

  EXPECT_EQ("foo", foo_->reserved_name(0));
  EXPECT_EQ("bar", foo_->reserved_name(1));
}

TEST_F(ReservedDescriptorTest, IsReservedName) {
  EXPECT_TRUE(foo_->IsReservedName("foo"));
  EXPECT_TRUE(foo_->IsReservedName("bar"));
  EXPECT_FALSE(foo_->IsReservedName("baz"));
}

// ===================================================================

// Test reserved enum fields.
class ReservedEnumDescriptorTest : public testing::Test {
 protected:
  void SetUp() override {
    // Build descriptors for the following definitions:
    //
    //   enum Foo {
    //     BAR = 1;
    //     reserved 2, 9 to 11, 15;
    //     reserved "foo", "bar";
    //   }

    FileDescriptorProto foo_file;
    foo_file.set_name("foo.proto");

    EnumDescriptorProto* foo = AddEnum(&foo_file, "Foo");
    EnumDescriptorProto* edge1 = AddEnum(&foo_file, "Edge1");
    EnumDescriptorProto* edge2 = AddEnum(&foo_file, "Edge2");

    AddEnumValue(foo, "BAR", 4);
    AddReservedRange(foo, -5, -3);
    AddReservedRange(foo, -2, 1);
    AddReservedRange(foo, 2, 3);
    AddReservedRange(foo, 9, 12);
    AddReservedRange(foo, 15, 16);

    foo->add_reserved_name("foo");
    foo->add_reserved_name("bar");

    // Some additional edge cases that cover most or all of the range of enum
    // values

    // Note: We use INT_MAX as the maximum reserved range upper bound,
    // inclusive.
    AddEnumValue(edge1, "EDGE1", 1);
    AddReservedRange(edge1, 10, INT_MAX);
    AddEnumValue(edge2, "EDGE2", 15);
    AddReservedRange(edge2, INT_MIN, 10);

    // Build the descriptors and get the pointers.
    foo_file_ = pool_.BuildFile(foo_file);
    ASSERT_TRUE(foo_file_ != nullptr);

    ASSERT_EQ(3, foo_file_->enum_type_count());
    foo_ = foo_file_->enum_type(0);
    edge1_ = foo_file_->enum_type(1);
    edge2_ = foo_file_->enum_type(2);
  }

  DescriptorPool pool_;
  const FileDescriptor* foo_file_;
  const EnumDescriptor* foo_;
  const EnumDescriptor* edge1_;
  const EnumDescriptor* edge2_;
};

TEST_F(ReservedEnumDescriptorTest, ReservedRanges) {
  ASSERT_EQ(5, foo_->reserved_range_count());

  EXPECT_EQ(-5, foo_->reserved_range(0)->start);
  EXPECT_EQ(-3, foo_->reserved_range(0)->end);

  EXPECT_EQ(-2, foo_->reserved_range(1)->start);
  EXPECT_EQ(1, foo_->reserved_range(1)->end);

  EXPECT_EQ(2, foo_->reserved_range(2)->start);
  EXPECT_EQ(3, foo_->reserved_range(2)->end);

  EXPECT_EQ(9, foo_->reserved_range(3)->start);
  EXPECT_EQ(12, foo_->reserved_range(3)->end);

  EXPECT_EQ(15, foo_->reserved_range(4)->start);
  EXPECT_EQ(16, foo_->reserved_range(4)->end);

  ASSERT_EQ(1, edge1_->reserved_range_count());
  EXPECT_EQ(10, edge1_->reserved_range(0)->start);
  EXPECT_EQ(INT_MAX, edge1_->reserved_range(0)->end);

  ASSERT_EQ(1, edge2_->reserved_range_count());
  EXPECT_EQ(INT_MIN, edge2_->reserved_range(0)->start);
  EXPECT_EQ(10, edge2_->reserved_range(0)->end);
}

TEST_F(ReservedEnumDescriptorTest, IsReservedNumber) {
  EXPECT_TRUE(foo_->IsReservedNumber(-5));
  EXPECT_TRUE(foo_->IsReservedNumber(-4));
  EXPECT_TRUE(foo_->IsReservedNumber(-3));
  EXPECT_TRUE(foo_->IsReservedNumber(-2));
  EXPECT_TRUE(foo_->IsReservedNumber(-1));
  EXPECT_TRUE(foo_->IsReservedNumber(0));
  EXPECT_TRUE(foo_->IsReservedNumber(1));
  EXPECT_TRUE(foo_->IsReservedNumber(2));
  EXPECT_TRUE(foo_->IsReservedNumber(3));
  EXPECT_FALSE(foo_->IsReservedNumber(8));
  EXPECT_TRUE(foo_->IsReservedNumber(9));
  EXPECT_TRUE(foo_->IsReservedNumber(10));
  EXPECT_TRUE(foo_->IsReservedNumber(11));
  EXPECT_TRUE(foo_->IsReservedNumber(12));
  EXPECT_FALSE(foo_->IsReservedNumber(13));
  EXPECT_FALSE(foo_->IsReservedNumber(13));
  EXPECT_FALSE(foo_->IsReservedNumber(14));
  EXPECT_TRUE(foo_->IsReservedNumber(15));
  EXPECT_TRUE(foo_->IsReservedNumber(16));
  EXPECT_FALSE(foo_->IsReservedNumber(17));

  EXPECT_FALSE(edge1_->IsReservedNumber(9));
  EXPECT_TRUE(edge1_->IsReservedNumber(10));
  EXPECT_TRUE(edge1_->IsReservedNumber(INT_MAX - 1));
  EXPECT_TRUE(edge1_->IsReservedNumber(INT_MAX));

  EXPECT_TRUE(edge2_->IsReservedNumber(INT_MIN));
  EXPECT_TRUE(edge2_->IsReservedNumber(9));
  EXPECT_TRUE(edge2_->IsReservedNumber(10));
  EXPECT_FALSE(edge2_->IsReservedNumber(11));
}

TEST_F(ReservedEnumDescriptorTest, ReservedNames) {
  ASSERT_EQ(2, foo_->reserved_name_count());

  EXPECT_EQ("foo", foo_->reserved_name(0));
  EXPECT_EQ("bar", foo_->reserved_name(1));
}

TEST_F(ReservedEnumDescriptorTest, IsReservedName) {
  EXPECT_TRUE(foo_->IsReservedName("foo"));
  EXPECT_TRUE(foo_->IsReservedName("bar"));
  EXPECT_FALSE(foo_->IsReservedName("baz"));
}

// ===================================================================

class MiscTest : public testing::Test {
 protected:
  // Function which makes a field descriptor of the given type.
  const FieldDescriptor* GetFieldDescriptorOfType(FieldDescriptor::Type type) {
    FileDescriptorProto file_proto;
    file_proto.set_name("foo.proto");
    AddEmptyEnum(&file_proto, "DummyEnum");

    DescriptorProto* message = AddMessage(&file_proto, "TestMessage");
    FieldDescriptorProto* field = AddField(
        message, "foo", 1, FieldDescriptorProto::LABEL_OPTIONAL,
        static_cast<FieldDescriptorProto::Type>(static_cast<int>(type)));

    if (type == FieldDescriptor::TYPE_MESSAGE ||
        type == FieldDescriptor::TYPE_GROUP) {
      field->set_type_name("TestMessage");
    } else if (type == FieldDescriptor::TYPE_ENUM) {
      field->set_type_name("DummyEnum");
    }

    // Build the descriptors and get the pointers.
    pool_ = std::make_unique<DescriptorPool>();
    const FileDescriptor* file = pool_->BuildFile(file_proto);

    if (file != nullptr && file->message_type_count() == 1 &&
        file->message_type(0)->field_count() == 1) {
      return file->message_type(0)->field(0);
    } else {
      return nullptr;
    }
  }

  absl::string_view GetTypeNameForFieldType(FieldDescriptor::Type type) {
    const FieldDescriptor* field = GetFieldDescriptorOfType(type);
    return field != nullptr ? field->type_name() : "";
  }

  FieldDescriptor::CppType GetCppTypeForFieldType(FieldDescriptor::Type type) {
    const FieldDescriptor* field = GetFieldDescriptorOfType(type);
    return field != nullptr ? field->cpp_type()
                            : static_cast<FieldDescriptor::CppType>(0);
  }

  absl::string_view GetCppTypeNameForFieldType(FieldDescriptor::Type type) {
    const FieldDescriptor* field = GetFieldDescriptorOfType(type);
    return field != nullptr ? field->cpp_type_name() : "";
  }

  const Descriptor* GetMessageDescriptorForFieldType(
      FieldDescriptor::Type type) {
    const FieldDescriptor* field = GetFieldDescriptorOfType(type);
    return field != nullptr ? field->message_type() : nullptr;
  }

  const EnumDescriptor* GetEnumDescriptorForFieldType(
      FieldDescriptor::Type type) {
    const FieldDescriptor* field = GetFieldDescriptorOfType(type);
    return field != nullptr ? field->enum_type() : nullptr;
  }

  std::unique_ptr<DescriptorPool> pool_;
};

TEST_F(MiscTest, TypeNames) {
  // Test that correct type names are returned.

  typedef FieldDescriptor FD;  // avoid ugly line wrapping

  EXPECT_EQ(absl::string_view("double"),
            GetTypeNameForFieldType(FD::TYPE_DOUBLE));
  EXPECT_EQ(absl::string_view("float"),
            GetTypeNameForFieldType(FD::TYPE_FLOAT));
  EXPECT_EQ(absl::string_view("int64"),
            GetTypeNameForFieldType(FD::TYPE_INT64));
  EXPECT_EQ(absl::string_view("uint64"),
            GetTypeNameForFieldType(FD::TYPE_UINT64));
  EXPECT_EQ(absl::string_view("int32"),
            GetTypeNameForFieldType(FD::TYPE_INT32));
  EXPECT_EQ(absl::string_view("fixed64"),
            GetTypeNameForFieldType(FD::TYPE_FIXED64));
  EXPECT_EQ(absl::string_view("fixed32"),
            GetTypeNameForFieldType(FD::TYPE_FIXED32));
  EXPECT_EQ(absl::string_view("bool"), GetTypeNameForFieldType(FD::TYPE_BOOL));
  EXPECT_EQ(absl::string_view("string"),
            GetTypeNameForFieldType(FD::TYPE_STRING));
  EXPECT_EQ(absl::string_view("group"),
            GetTypeNameForFieldType(FD::TYPE_GROUP));
  EXPECT_EQ(absl::string_view("message"),
            GetTypeNameForFieldType(FD::TYPE_MESSAGE));
  EXPECT_EQ(absl::string_view("bytes"),
            GetTypeNameForFieldType(FD::TYPE_BYTES));
  EXPECT_EQ(absl::string_view("uint32"),
            GetTypeNameForFieldType(FD::TYPE_UINT32));
  EXPECT_EQ(absl::string_view("enum"), GetTypeNameForFieldType(FD::TYPE_ENUM));
  EXPECT_EQ(absl::string_view("sfixed32"),
            GetTypeNameForFieldType(FD::TYPE_SFIXED32));
  EXPECT_EQ(absl::string_view("sfixed64"),
            GetTypeNameForFieldType(FD::TYPE_SFIXED64));
  EXPECT_EQ(absl::string_view("sint32"),
            GetTypeNameForFieldType(FD::TYPE_SINT32));
  EXPECT_EQ(absl::string_view("sint64"),
            GetTypeNameForFieldType(FD::TYPE_SINT64));
}

TEST_F(MiscTest, StaticTypeNames) {
  // Test that correct type names are returned.

  typedef FieldDescriptor FD;  // avoid ugly line wrapping

  EXPECT_EQ(absl::string_view("double"), FD::TypeName(FD::TYPE_DOUBLE));
  EXPECT_EQ(absl::string_view("float"), FD::TypeName(FD::TYPE_FLOAT));
  EXPECT_EQ(absl::string_view("int64"), FD::TypeName(FD::TYPE_INT64));
  EXPECT_EQ(absl::string_view("uint64"), FD::TypeName(FD::TYPE_UINT64));
  EXPECT_EQ(absl::string_view("int32"), FD::TypeName(FD::TYPE_INT32));
  EXPECT_EQ(absl::string_view("fixed64"), FD::TypeName(FD::TYPE_FIXED64));
  EXPECT_EQ(absl::string_view("fixed32"), FD::TypeName(FD::TYPE_FIXED32));
  EXPECT_EQ(absl::string_view("bool"), FD::TypeName(FD::TYPE_BOOL));
  EXPECT_EQ(absl::string_view("string"), FD::TypeName(FD::TYPE_STRING));
  EXPECT_EQ(absl::string_view("group"), FD::TypeName(FD::TYPE_GROUP));
  EXPECT_EQ(absl::string_view("message"), FD::TypeName(FD::TYPE_MESSAGE));
  EXPECT_EQ(absl::string_view("bytes"), FD::TypeName(FD::TYPE_BYTES));
  EXPECT_EQ(absl::string_view("uint32"), FD::TypeName(FD::TYPE_UINT32));
  EXPECT_EQ(absl::string_view("enum"), FD::TypeName(FD::TYPE_ENUM));
  EXPECT_EQ(absl::string_view("sfixed32"), FD::TypeName(FD::TYPE_SFIXED32));
  EXPECT_EQ(absl::string_view("sfixed64"), FD::TypeName(FD::TYPE_SFIXED64));
  EXPECT_EQ(absl::string_view("sint32"), FD::TypeName(FD::TYPE_SINT32));
  EXPECT_EQ(absl::string_view("sint64"), FD::TypeName(FD::TYPE_SINT64));
}

TEST_F(MiscTest, CppTypes) {
  // Test that CPP types are assigned correctly.

  typedef FieldDescriptor FD;  // avoid ugly line wrapping

  EXPECT_EQ(FD::CPPTYPE_DOUBLE, GetCppTypeForFieldType(FD::TYPE_DOUBLE));
  EXPECT_EQ(FD::CPPTYPE_FLOAT, GetCppTypeForFieldType(FD::TYPE_FLOAT));
  EXPECT_EQ(FD::CPPTYPE_INT64, GetCppTypeForFieldType(FD::TYPE_INT64));
  EXPECT_EQ(FD::CPPTYPE_UINT64, GetCppTypeForFieldType(FD::TYPE_UINT64));
  EXPECT_EQ(FD::CPPTYPE_INT32, GetCppTypeForFieldType(FD::TYPE_INT32));
  EXPECT_EQ(FD::CPPTYPE_UINT64, GetCppTypeForFieldType(FD::TYPE_FIXED64));
  EXPECT_EQ(FD::CPPTYPE_UINT32, GetCppTypeForFieldType(FD::TYPE_FIXED32));
  EXPECT_EQ(FD::CPPTYPE_BOOL, GetCppTypeForFieldType(FD::TYPE_BOOL));
  EXPECT_EQ(FD::CPPTYPE_STRING, GetCppTypeForFieldType(FD::TYPE_STRING));
  EXPECT_EQ(FD::CPPTYPE_MESSAGE, GetCppTypeForFieldType(FD::TYPE_GROUP));
  EXPECT_EQ(FD::CPPTYPE_MESSAGE, GetCppTypeForFieldType(FD::TYPE_MESSAGE));
  EXPECT_EQ(FD::CPPTYPE_STRING, GetCppTypeForFieldType(FD::TYPE_BYTES));
  EXPECT_EQ(FD::CPPTYPE_UINT32, GetCppTypeForFieldType(FD::TYPE_UINT32));
  EXPECT_EQ(FD::CPPTYPE_ENUM, GetCppTypeForFieldType(FD::TYPE_ENUM));
  EXPECT_EQ(FD::CPPTYPE_INT32, GetCppTypeForFieldType(FD::TYPE_SFIXED32));
  EXPECT_EQ(FD::CPPTYPE_INT64, GetCppTypeForFieldType(FD::TYPE_SFIXED64));
  EXPECT_EQ(FD::CPPTYPE_INT32, GetCppTypeForFieldType(FD::TYPE_SINT32));
  EXPECT_EQ(FD::CPPTYPE_INT64, GetCppTypeForFieldType(FD::TYPE_SINT64));
}

TEST_F(MiscTest, CppTypeNames) {
  // Test that correct CPP type names are returned.

  typedef FieldDescriptor FD;  // avoid ugly line wrapping

  EXPECT_EQ(absl::string_view("double"),
            GetCppTypeNameForFieldType(FD::TYPE_DOUBLE));
  EXPECT_EQ(absl::string_view("float"),
            GetCppTypeNameForFieldType(FD::TYPE_FLOAT));
  EXPECT_EQ(absl::string_view("int64"),
            GetCppTypeNameForFieldType(FD::TYPE_INT64));
  EXPECT_EQ(absl::string_view("uint64"),
            GetCppTypeNameForFieldType(FD::TYPE_UINT64));
  EXPECT_EQ(absl::string_view("int32"),
            GetCppTypeNameForFieldType(FD::TYPE_INT32));
  EXPECT_EQ(absl::string_view("uint64"),
            GetCppTypeNameForFieldType(FD::TYPE_FIXED64));
  EXPECT_EQ(absl::string_view("uint32"),
            GetCppTypeNameForFieldType(FD::TYPE_FIXED32));
  EXPECT_EQ(absl::string_view("bool"),
            GetCppTypeNameForFieldType(FD::TYPE_BOOL));
  EXPECT_EQ(absl::string_view("string"),
            GetCppTypeNameForFieldType(FD::TYPE_STRING));
  EXPECT_EQ(absl::string_view("message"),
            GetCppTypeNameForFieldType(FD::TYPE_GROUP));
  EXPECT_EQ(absl::string_view("message"),
            GetCppTypeNameForFieldType(FD::TYPE_MESSAGE));
  EXPECT_EQ(absl::string_view("string"),
            GetCppTypeNameForFieldType(FD::TYPE_BYTES));
  EXPECT_EQ(absl::string_view("uint32"),
            GetCppTypeNameForFieldType(FD::TYPE_UINT32));
  EXPECT_EQ(absl::string_view("enum"),
            GetCppTypeNameForFieldType(FD::TYPE_ENUM));
  EXPECT_EQ(absl::string_view("int32"),
            GetCppTypeNameForFieldType(FD::TYPE_SFIXED32));
  EXPECT_EQ(absl::string_view("int64"),
            GetCppTypeNameForFieldType(FD::TYPE_SFIXED64));
  EXPECT_EQ(absl::string_view("int32"),
            GetCppTypeNameForFieldType(FD::TYPE_SINT32));
  EXPECT_EQ(absl::string_view("int64"),
            GetCppTypeNameForFieldType(FD::TYPE_SINT64));
}

TEST_F(MiscTest, StaticCppTypeNames) {
  // Test that correct CPP type names are returned.

  typedef FieldDescriptor FD;  // avoid ugly line wrapping

  EXPECT_EQ(absl::string_view("int32"), FD::CppTypeName(FD::CPPTYPE_INT32));
  EXPECT_EQ(absl::string_view("int64"), FD::CppTypeName(FD::CPPTYPE_INT64));
  EXPECT_EQ(absl::string_view("uint32"), FD::CppTypeName(FD::CPPTYPE_UINT32));
  EXPECT_EQ(absl::string_view("uint64"), FD::CppTypeName(FD::CPPTYPE_UINT64));
  EXPECT_EQ(absl::string_view("double"), FD::CppTypeName(FD::CPPTYPE_DOUBLE));
  EXPECT_EQ(absl::string_view("float"), FD::CppTypeName(FD::CPPTYPE_FLOAT));
  EXPECT_EQ(absl::string_view("bool"), FD::CppTypeName(FD::CPPTYPE_BOOL));
  EXPECT_EQ(absl::string_view("enum"), FD::CppTypeName(FD::CPPTYPE_ENUM));
  EXPECT_EQ(absl::string_view("string"), FD::CppTypeName(FD::CPPTYPE_STRING));
  EXPECT_EQ(absl::string_view("message"), FD::CppTypeName(FD::CPPTYPE_MESSAGE));
}

TEST_F(MiscTest, MessageType) {
  // Test that message_type() is nullptr for non-aggregate fields

  typedef FieldDescriptor FD;  // avoid ugly line wrapping

  EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(FD::TYPE_DOUBLE));
  EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(FD::TYPE_FLOAT));
  EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(FD::TYPE_INT64));
  EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(FD::TYPE_UINT64));
  EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(FD::TYPE_INT32));
  EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(FD::TYPE_FIXED64));
  EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(FD::TYPE_FIXED32));
  EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(FD::TYPE_BOOL));
  EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(FD::TYPE_STRING));
  EXPECT_TRUE(nullptr != GetMessageDescriptorForFieldType(FD::TYPE_GROUP));
  EXPECT_TRUE(nullptr != GetMessageDescriptorForFieldType(FD::TYPE_MESSAGE));
  EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(FD::TYPE_BYTES));
  EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(FD::TYPE_UINT32));
  EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(FD::TYPE_ENUM));
  EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(FD::TYPE_SFIXED32));
  EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(FD::TYPE_SFIXED64));
  EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(FD::TYPE_SINT32));
  EXPECT_TRUE(nullptr == GetMessageDescriptorForFieldType(FD::TYPE_SINT64));
}

TEST_F(MiscTest, EnumType) {
  // Test that enum_type() is nullptr for non-enum fields

  typedef FieldDescriptor FD;  // avoid ugly line wrapping

  EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(FD::TYPE_DOUBLE));
  EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(FD::TYPE_FLOAT));
  EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(FD::TYPE_INT64));
  EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(FD::TYPE_UINT64));
  EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(FD::TYPE_INT32));
  EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(FD::TYPE_FIXED64));
  EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(FD::TYPE_FIXED32));
  EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(FD::TYPE_BOOL));
  EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(FD::TYPE_STRING));
  EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(FD::TYPE_GROUP));
  EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(FD::TYPE_MESSAGE));
  EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(FD::TYPE_BYTES));
  EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(FD::TYPE_UINT32));
  EXPECT_TRUE(nullptr != GetEnumDescriptorForFieldType(FD::TYPE_ENUM));
  EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(FD::TYPE_SFIXED32));
  EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(FD::TYPE_SFIXED64));
  EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(FD::TYPE_SINT32));
  EXPECT_TRUE(nullptr == GetEnumDescriptorForFieldType(FD::TYPE_SINT64));
}

TEST_F(MiscTest, DefaultValues) {
  // Test that setting default values works.
  FileDescriptorProto file_proto;
  file_proto.set_name("foo.proto");

  EnumDescriptorProto* enum_type_proto = AddEnum(&file_proto, "DummyEnum");
  AddEnumValue(enum_type_proto, "A", 1);
  AddEnumValue(enum_type_proto, "B", 2);

  DescriptorProto* message_proto = AddMessage(&file_proto, "TestMessage");

  typedef FieldDescriptorProto FD;  // avoid ugly line wrapping
  const FD::Label label = FD::LABEL_OPTIONAL;

  // Create fields of every CPP type with default values.
  AddField(message_proto, "int32", 1, label, FD::TYPE_INT32)
      ->set_default_value("-1");
  AddField(message_proto, "int64", 2, label, FD::TYPE_INT64)
      ->set_default_value("-1000000000000");
  AddField(message_proto, "uint32", 3, label, FD::TYPE_UINT32)
      ->set_default_value("42");
  AddField(message_proto, "uint64", 4, label, FD::TYPE_UINT64)
      ->set_default_value("2000000000000");
  AddField(message_proto, "float", 5, label, FD::TYPE_FLOAT)
      ->set_default_value("4.5");
  AddField(message_proto, "double", 6, label, FD::TYPE_DOUBLE)
      ->set_default_value("10e100");
  AddField(message_proto, "bool", 7, label, FD::TYPE_BOOL)
      ->set_default_value("true");
  AddField(message_proto, "string", 8, label, FD::TYPE_STRING)
      ->set_default_value("hello");
  AddField(message_proto, "data", 9, label, FD::TYPE_BYTES)
      ->set_default_value("\\001\\002\\003");
  AddField(message_proto, "data2", 10, label, FD::TYPE_BYTES)
      ->set_default_value("\\X01\\X2\\X3");
  AddField(message_proto, "data3", 11, label, FD::TYPE_BYTES)
      ->set_default_value("\\x01\\x2\\x3");

  FieldDescriptorProto* enum_field =
      AddField(message_proto, "enum", 12, label, FD::TYPE_ENUM);
  enum_field->set_type_name("DummyEnum");
  enum_field->set_default_value("B");

  // Strings are allowed to have empty defaults.  (At one point, due to
  // a bug, empty defaults for strings were rejected.  Oops.)
  AddField(message_proto, "empty_string", 13, label, FD::TYPE_STRING)
      ->set_default_value("");

  // Add a second set of fields with implicit default values.
  AddField(message_proto, "implicit_int32", 21, label, FD::TYPE_INT32);
  AddField(message_proto, "implicit_int64", 22, label, FD::TYPE_INT64);
  AddField(message_proto, "implicit_uint32", 23, label, FD::TYPE_UINT32);
  AddField(message_proto, "implicit_uint64", 24, label, FD::TYPE_UINT64);
  AddField(message_proto, "implicit_float", 25, label, FD::TYPE_FLOAT);
  AddField(message_proto, "implicit_double", 26, label, FD::TYPE_DOUBLE);
  AddField(message_proto, "implicit_bool", 27, label, FD::TYPE_BOOL);
  AddField(message_proto, "implicit_string", 28, label, FD::TYPE_STRING);
  AddField(message_proto, "implicit_data", 29, label, FD::TYPE_BYTES);
  AddField(message_proto, "implicit_enum", 30, label, FD::TYPE_ENUM)
      ->set_type_name("DummyEnum");

  // Build it.
  DescriptorPool pool;
  const FileDescriptor* file = pool.BuildFile(file_proto);
  ASSERT_TRUE(file != nullptr);

  ASSERT_EQ(1, file->enum_type_count());
  const EnumDescriptor* enum_type = file->enum_type(0);
  ASSERT_EQ(2, enum_type->value_count());
  const EnumValueDescriptor* enum_value_a = enum_type->value(0);
  const EnumValueDescriptor* enum_value_b = enum_type->value(1);

  ASSERT_EQ(1, file->message_type_count());
  const Descriptor* message = file->message_type(0);

  ASSERT_EQ(23, message->field_count());

  // Check the default values.
  ASSERT_TRUE(message->field(0)->has_default_value());
  ASSERT_TRUE(message->field(1)->has_default_value());
  ASSERT_TRUE(message->field(2)->has_default_value());
  ASSERT_TRUE(message->field(3)->has_default_value());
  ASSERT_TRUE(message->field(4)->has_default_value());
  ASSERT_TRUE(message->field(5)->has_default_value());
  ASSERT_TRUE(message->field(6)->has_default_value());
  ASSERT_TRUE(message->field(7)->has_default_value());
  ASSERT_TRUE(message->field(8)->has_default_value());
  ASSERT_TRUE(message->field(9)->has_default_value());
  ASSERT_TRUE(message->field(10)->has_default_value());
  ASSERT_TRUE(message->field(11)->has_default_value());
  ASSERT_TRUE(message->field(12)->has_default_value());

  EXPECT_EQ(-1, message->field(0)->default_value_int32());
  EXPECT_EQ(int64_t{-1000000000000}, message->field(1)->default_value_int64());
  EXPECT_EQ(42, message->field(2)->default_value_uint32());
  EXPECT_EQ(uint64_t{2000000000000}, message->field(3)->default_value_uint64());
  EXPECT_EQ(4.5, message->field(4)->default_value_float());
  EXPECT_EQ(10e100, message->field(5)->default_value_double());
  EXPECT_TRUE(message->field(6)->default_value_bool());
  EXPECT_EQ("hello", message->field(7)->default_value_string());
  EXPECT_EQ("\001\002\003", message->field(8)->default_value_string());
  EXPECT_EQ("\001\002\003", message->field(9)->default_value_string());
  EXPECT_EQ("\001\002\003", message->field(10)->default_value_string());
  EXPECT_EQ(enum_value_b, message->field(11)->default_value_enum());
  EXPECT_EQ("", message->field(12)->default_value_string());

  ASSERT_FALSE(message->field(13)->has_default_value());
  ASSERT_FALSE(message->field(14)->has_default_value());
  ASSERT_FALSE(message->field(15)->has_default_value());
  ASSERT_FALSE(message->field(16)->has_default_value());
  ASSERT_FALSE(message->field(17)->has_default_value());
  ASSERT_FALSE(message->field(18)->has_default_value());
  ASSERT_FALSE(message->field(19)->has_default_value());
  ASSERT_FALSE(message->field(20)->has_default_value());
  ASSERT_FALSE(message->field(21)->has_default_value());
  ASSERT_FALSE(message->field(22)->has_default_value());

  EXPECT_EQ(0, message->field(13)->default_value_int32());
  EXPECT_EQ(0, message->field(14)->default_value_int64());
  EXPECT_EQ(0, message->field(15)->default_value_uint32());
  EXPECT_EQ(0, message->field(16)->default_value_uint64());
  EXPECT_EQ(0.0f, message->field(17)->default_value_float());
  EXPECT_EQ(0.0, message->field(18)->default_value_double());
  EXPECT_FALSE(message->field(19)->default_value_bool());
  EXPECT_EQ("", message->field(20)->default_value_string());
  EXPECT_EQ("", message->field(21)->default_value_string());
  EXPECT_EQ(enum_value_a, message->field(22)->default_value_enum());
}

TEST_F(MiscTest, FieldOptions) {
  // Try setting field options.

  FileDescriptorProto file_proto;
  file_proto.set_name("foo.proto");

  DescriptorProto* message_proto = AddMessage(&file_proto, "TestMessage");
  AddField(message_proto, "foo", 1, FieldDescriptorProto::LABEL_OPTIONAL,
           FieldDescriptorProto::TYPE_INT32);
  FieldDescriptorProto* bar_proto =
      AddField(message_proto, "bar", 2, FieldDescriptorProto::LABEL_OPTIONAL,
               FieldDescriptorProto::TYPE_BYTES);

  FieldOptions* options = bar_proto->mutable_options();
  options->set_ctype(FieldOptions::CORD);

  // Build the descriptors and get the pointers.
  DescriptorPool pool;
  const FileDescriptor* file = pool.BuildFile(file_proto);
  ASSERT_TRUE(file != nullptr);

  ASSERT_EQ(1, file->message_type_count());
  const Descriptor* message = file->message_type(0);

  ASSERT_EQ(2, message->field_count());
  const FieldDescriptor* foo = message->field(0);
  const FieldDescriptor* bar = message->field(1);

  // "foo" had no options set, so it should return the default options.
  EXPECT_EQ(&FieldOptions::default_instance(), &foo->options());

  // "bar" had options set.
  EXPECT_NE(&FieldOptions::default_instance(), options);
  EXPECT_EQ(bar->cpp_string_type(), FieldDescriptor::CppStringType::kCord);
}

// ===================================================================

struct HasHasbitTestParam {
  struct ExpectedOutput {
    HasbitMode expected_hasbitmode;
    bool expected_has_presence;
    bool expected_has_hasbit;
  };

  std::string input_foo_proto;
  ExpectedOutput expected_output;
  bool is_extension = false;
};

class HasHasbitTest : public testing::TestWithParam<HasHasbitTestParam> {
 protected:
  void SetUp() override {
    ASSERT_TRUE(
        TextFormat::ParseFromString(GetParam().input_foo_proto, &foo_proto_));
    foo_ = pool_.BuildFile(foo_proto_);
  }

  const FieldDescriptor* GetField() {
    if (GetParam().is_extension) {
      return foo_->message_type(0)->extension(0);
    } else {
      return foo_->message_type(0)->field(0);
    }
  }

  DescriptorPool pool_;
  FileDescriptorProto foo_proto_;
  const FileDescriptor* foo_;
};

TEST_P(HasHasbitTest, TestHasHasbitExplicitPresence) {
  EXPECT_EQ(GetField()->has_presence(),
            GetParam().expected_output.expected_has_presence);
  EXPECT_EQ(GetFieldHasbitMode(GetField()),
            GetParam().expected_output.expected_hasbitmode);
  EXPECT_EQ(HasHasbit(GetField()),
            GetParam().expected_output.expected_has_hasbit);
}

// NOTE: with C++20 we can use designated initializers to ensure
// that struct members match commented names, but as we are still working with
// C++17 in the foreseeable future, we won't be able to refactor this for a
// while...
// https://github.com/google/oss-policies-info/blob/main/foundational-cxx-support-matrix.md
INSTANTIATE_TEST_SUITE_P(
    HasHasbitLegacySyntaxTests, HasHasbitTest,
    testing::Values(
        // Test case: proto2 singular fields
        HasHasbitTestParam{R"pb(name: 'foo.proto'
                                package: 'foo'
                                syntax: 'proto2'
                                message_type {
                                  name: 'FooMessage'
                                  field {
                                    name: 'f'
                                    number: 1
                                    type: TYPE_INT64
                                    label: LABEL_OPTIONAL
                                  }
                                }
                           )pb",
                           /*expected_output=*/{
                               /*expected_hasbitmode=*/HasbitMode::kTrueHasbit,
                               /*expected_has_presence=*/true,
                               /*expected_has_hasbit=*/true,
                           }},
        // Test case: proto2 repeated fields
        HasHasbitTestParam{R"pb(name: 'foo.proto'
                                package: 'foo'
                                syntax: 'proto2'
                                message_type {
                                  name: 'FooMessage'
                                  field {
                                    name: 'f'
                                    number: 1
                                    type: TYPE_STRING
                                    label: LABEL_REPEATED
                                  }
                                }
                           )pb",
                           /*expected_output=*/{
                               /*expected_hasbitmode=*/HasbitMode::kNoHasbit,
                               /*expected_has_presence=*/false,
                               /*expected_has_hasbit=*/false,
                           }},
        // Test case: proto3 singular fields
        HasHasbitTestParam{R"pb(name: 'foo.proto'
                                package: 'foo'
                                syntax: 'proto3'
                                message_type {
                                  name: 'FooMessage'
                                  field {
                                    name: 'f'
                                    number: 1
                                    type: TYPE_INT64
                                    label: LABEL_OPTIONAL
                                  }
                                }
                           )pb",
                           /*expected_output=*/{
                               /*expected_hasbitmode=*/HasbitMode::kHintHasbit,
                               /*expected_has_presence=*/false,
                               /*expected_has_hasbit=*/true,
                           }},
        // Test case: proto3 optional fields
        HasHasbitTestParam{
            R"pb(name: 'foo.proto'
                 package: 'foo'
                 syntax: 'proto3'
                 message_type {
                   name: 'Foo'
                   field {
                     name: 'int_field'
                     number: 1
                     type: TYPE_INT32
                     label: LABEL_OPTIONAL
                     oneof_index: 0
                     proto3_optional: true
                   }
                   oneof_decl { name: '_int_field' }
                 }
            )pb",
            /*expected_output=*/{
                /*expected_hasbitmode=*/HasbitMode::kTrueHasbit,
                /*expected_has_presence=*/true,
                /*expected_has_hasbit=*/true,
            }},
        // Test case: proto3 repeated fields
        HasHasbitTestParam{R"pb(name: 'foo.proto'
                                package: 'foo'
                                syntax: 'proto3'
                                message_type {
                                  name: 'FooMessage'
                                  field {
                                    name: 'f'
                                    number: 1
                                    type: TYPE_STRING
                                    label: LABEL_REPEATED
                                  }
                                }
                           )pb",
                           /*expected_output=*/{
                               /*expected_hasbitmode=*/HasbitMode::kNoHasbit,
                               /*expected_has_presence=*/false,
                               /*expected_has_hasbit=*/false,
                           }},
        // Test case: proto2 extension fields.
        // Note that extension fields don't have hasbits.
        HasHasbitTestParam{
            R"pb(name: 'foo.proto'
                 package: 'foo'
                 syntax: 'proto2'
                 message_type {
                   name: "FooMessage"
                   extension {
                     name: "foo"
                     number: 1
                     label: LABEL_OPTIONAL
                     type: TYPE_INT32
                     extendee: "FooMessage2"
                   }
                 }
                 message_type {
                   name: "FooMessage2"
                   extension_range { start: 1 end: 2 }
                 }
            )pb",
            /*expected_output=*/
            {
                /*expected_hasbitmode=*/HasbitMode::kNoHasbit,
                /*expected_has_presence=*/true,
                /*expected_has_hasbit=*/false,
            },
            /*is_extension=*/true}));

// NOTE: with C++20 we can use designated initializers to ensure
// that struct members match commented names, but as we are still working with
// C++17 in the foreseeable future, we won't be able to refactor this for a
// while...
// https://github.com/google/oss-policies-info/blob/main/foundational-cxx-support-matrix.md
INSTANTIATE_TEST_SUITE_P(
    HasHasbitEditionsTests, HasHasbitTest,
    testing::Values(
        // Test case: explicit-presence, singular fields
        HasHasbitTestParam{
            R"pb(name: 'foo.proto'
                 package: 'foo'
                 syntax: 'editions'
                 edition: EDITION_2023
                 message_type {
                   name: 'FooMessage'
                   field {
                     name: 'f'
                     number: 1
                     type: TYPE_INT64
                     options { features { field_presence: EXPLICIT } }
                   }
                 }
            )pb",
            /*expected_output=*/{
                /*expected_hasbitmode=*/HasbitMode::kTrueHasbit,
                /*expected_has_presence=*/true,
                /*expected_has_hasbit=*/true,
            }},
        // Test case: implicit-presence, singular fields
        HasHasbitTestParam{
            R"pb(name: 'foo.proto'
                 package: 'foo'
                 syntax: 'editions'
                 edition: EDITION_2023
                 message_type {
                   name: 'FooMessage'
                   field {
                     name: 'f'
                     number: 1
                     type: TYPE_INT64
                     options { features { field_presence: IMPLICIT } }
                   }
                 }
            )pb",
            /*expected_output=*/{
                /*expected_hasbitmode=*/HasbitMode::kHintHasbit,
                /*expected_has_presence=*/false,
                /*expected_has_hasbit=*/true,
            }},
        // Test case: oneof fields.
        // Note that oneof fields can't specify field presence.
        HasHasbitTestParam{
            R"pb(name: 'foo.proto'
                 package: 'foo'
                 syntax: 'editions'
                 edition: EDITION_2023
                 message_type {
                   name: 'FooMessage'
                   field {
                     name: 'f'
                     number: 1
                     type: TYPE_STRING
                     oneof_index: 0
                   }
                   oneof_decl { name: "onebar" }
                 }
            )pb",
            /*expected_output=*/{
                /*expected_hasbitmode=*/HasbitMode::kNoHasbit,
                /*expected_has_presence=*/true,
                /*expected_has_hasbit=*/false,
            }},
        // Test case: message fields.
        // Note that message fields cannot specify implicit presence.
        HasHasbitTestParam{
            R"pb(name: 'foo.proto'
                 package: 'foo'
                 syntax: 'editions'
                 edition: EDITION_2023
                 message_type {
                   name: 'FooMessage'
                   field {
                     name: 'f'
                     number: 1
                     type: TYPE_MESSAGE
                     type_name: "Bar"
                   }
                 }
                 message_type {
                   name: 'Bar'
                   field { name: 'int_field' number: 1 type: TYPE_INT32 }
                 }
            )pb",
            /*expected_output=*/{
                /*expected_hasbitmode=*/HasbitMode::kTrueHasbit,
                /*expected_has_presence=*/true,
                /*expected_has_hasbit=*/true,
            }},
        // Test case: repeated fields.
        // Note that repeated fields can't specify presence.
        HasHasbitTestParam{R"pb(name: 'foo.proto'
                                package: 'foo'
                                syntax: 'editions'
                                edition: EDITION_2023
                                message_type {
                                  name: 'FooMessage'
                                  field {
                                    name: 'f'
                                    number: 1
                                    type: TYPE_STRING
                                    label: LABEL_REPEATED
                                  }
                                }
                           )pb",
                           /*expected_output=*/{
                               /*expected_hasbitmode=*/HasbitMode::kNoHasbit,
                               /*expected_has_presence=*/false,
                               /*expected_has_hasbit=*/false,
                           }},
        // Test case: extension fields.
        // Note that extension fields don't have hasbits.
        HasHasbitTestParam{
            R"pb(name: 'foo.proto'
                 package: 'foo'
                 syntax: 'editions'
                 edition: EDITION_2023
                 message_type {
                   name: "FooMessage"
                   extension {
                     name: "foo"
                     number: 1
                     label: LABEL_OPTIONAL
                     type: TYPE_INT32
                     extendee: "FooMessage2"
                   }
                 }
                 message_type {
                   name: "FooMessage2"
                   extension_range { start: 1 end: 2 }
                 }
            )pb",
            /*expected_output=*/
            {
                /*expected_hasbitmode=*/HasbitMode::kNoHasbit,
                /*expected_has_presence=*/true,
                /*expected_has_hasbit=*/false,
            },
            /*is_extension=*/true}));


// ===================================================================
enum DescriptorPoolMode { NO_DATABASE, FALLBACK_DATABASE };

class AllowUnknownDependenciesTest
    : public testing::TestWithParam<
          std::tuple<DescriptorPoolMode, const char*>> {
 protected:
  DescriptorPoolMode mode() { return std::get<0>(GetParam()); }
  const char* syntax() { return std::get<1>(GetParam()); }

  void SetUp() override {
    FileDescriptorProto foo_proto, bar_proto;

    switch (mode()) {
      case NO_DATABASE:
        pool_ = std::make_unique<DescriptorPool>();
        break;
      case FALLBACK_DATABASE:
        pool_ = std::make_unique<DescriptorPool>(&db_);
        break;
    }

    pool_->AllowUnknownDependencies();

    ASSERT_TRUE(TextFormat::ParseFromString(
        "name: 'foo.proto'"
        "dependency: 'bar.proto'"
        "dependency: 'baz.proto'"
        "message_type {"
        "  name: 'Foo'"
        "  field { name:'bar' number:1 label:LABEL_OPTIONAL type_name:'Bar' }"
        "  field { name:'baz' number:2 label:LABEL_OPTIONAL type_name:'Baz' }"
        "  field { name:'moo' number:3 label:LABEL_OPTIONAL"
        "    type_name: '.corge.Moo'"
        "    type: TYPE_ENUM"
        "    options {"
        "      uninterpreted_option {"
        "        name {"
        "          name_part: 'grault'"
        "          is_extension: true"
        "        }"
        "        positive_int_value: 1234"
        "      }"
        "    }"
        "  }"
        "}",
        &foo_proto));
    foo_proto.set_syntax(syntax());

    ASSERT_TRUE(
        TextFormat::ParseFromString("name: 'bar.proto'"
                                    "message_type { name: 'Bar' }",
                                    &bar_proto));
    bar_proto.set_syntax(syntax());

    // Collect pointers to stuff.
    bar_file_ = BuildFile(bar_proto);
    ASSERT_TRUE(bar_file_ != nullptr);

    ASSERT_EQ(1, bar_file_->message_type_count());
    bar_type_ = bar_file_->message_type(0);

    foo_file_ = BuildFile(foo_proto);
    ASSERT_TRUE(foo_file_ != nullptr);

    ASSERT_EQ(1, foo_file_->message_type_count());
    foo_type_ = foo_file_->message_type(0);

    ASSERT_EQ(3, foo_type_->field_count());
    bar_field_ = foo_type_->field(0);
    baz_field_ = foo_type_->field(1);
    moo_field_ = foo_type_->field(2);
  }

  const FileDescriptor* BuildFile(const FileDescriptorProto& proto) {
    switch (mode()) {
      case NO_DATABASE:
        return pool_->BuildFile(proto);
        break;
      case FALLBACK_DATABASE: {
        EXPECT_TRUE(db_.Add(proto));
        return pool_->FindFileByName(proto.name());
      }
    }
    ABSL_LOG(FATAL) << "Can't get here.";
    return nullptr;
  }

  const FileDescriptor* bar_file_;
  const Descriptor* bar_type_;
  const FileDescriptor* foo_file_;
  const Descriptor* foo_type_;
  const FieldDescriptor* bar_field_;
  const FieldDescriptor* baz_field_;
  const FieldDescriptor* moo_field_;

  SimpleDescriptorDatabase db_;  // used if in FALLBACK_DATABASE mode.
  std::unique_ptr<DescriptorPool> pool_;
};

TEST_P(AllowUnknownDependenciesTest, PlaceholderFile) {
  ASSERT_EQ(2, foo_file_->dependency_count());
  EXPECT_EQ(bar_file_, foo_file_->dependency(0));
  EXPECT_FALSE(bar_file_->is_placeholder());

  const FileDescriptor* baz_file = foo_file_->dependency(1);
  EXPECT_EQ("baz.proto", baz_file->name());
  EXPECT_EQ(0, baz_file->message_type_count());
  EXPECT_TRUE(baz_file->is_placeholder());

  // Placeholder files should not be findable.
  EXPECT_EQ(bar_file_, pool_->FindFileByName(bar_file_->name()));
  EXPECT_TRUE(pool_->FindFileByName(baz_file->name()) == nullptr);

  // Copy*To should not crash for placeholder files.
  FileDescriptorProto baz_file_proto;
  baz_file->CopyTo(&baz_file_proto);
  baz_file->CopySourceCodeInfoTo(&baz_file_proto);
  EXPECT_FALSE(baz_file_proto.has_source_code_info());
}

TEST_P(AllowUnknownDependenciesTest, PlaceholderTypes) {
  ASSERT_EQ(FieldDescriptor::TYPE_MESSAGE, bar_field_->type());
  EXPECT_EQ(bar_type_, bar_field_->message_type());
  EXPECT_FALSE(bar_type_->is_placeholder());

  ASSERT_EQ(FieldDescriptor::TYPE_MESSAGE, baz_field_->type());
  const Descriptor* baz_type = baz_field_->message_type();
  EXPECT_EQ("Baz", baz_type->name());
  EXPECT_EQ("Baz", baz_type->full_name());
  EXPECT_EQ(0, baz_type->extension_range_count());
  EXPECT_TRUE(baz_type->is_placeholder());

  ASSERT_EQ(FieldDescriptor::TYPE_ENUM, moo_field_->type());
  const EnumDescriptor* moo_type = moo_field_->enum_type();
  EXPECT_EQ("Moo", moo_type->name());
  EXPECT_EQ("corge.Moo", moo_type->full_name());
  EXPECT_TRUE(moo_type->is_placeholder());
  // Placeholder enum values should not be findable.
  EXPECT_EQ(moo_type->FindValueByNumber(0), nullptr);

  // Placeholder types should not be findable.
  EXPECT_EQ(bar_type_, pool_->FindMessageTypeByName(bar_type_->full_name()));
  EXPECT_TRUE(pool_->FindMessageTypeByName(baz_type->full_name()) == nullptr);
  EXPECT_TRUE(pool_->FindEnumTypeByName(moo_type->full_name()) == nullptr);
}

TEST_P(AllowUnknownDependenciesTest, CopyTo) {
  // FieldDescriptor::CopyTo() should write non-fully-qualified type names
  // for placeholder types which were not originally fully-qualified.
  FieldDescriptorProto proto;

  // Bar is not a placeholder, so it is fully-qualified.
  bar_field_->CopyTo(&proto);
  EXPECT_EQ(".Bar", proto.type_name());
  EXPECT_EQ(FieldDescriptorProto::TYPE_MESSAGE, proto.type());

  // Baz is an unqualified placeholder.
  proto.Clear();
  baz_field_->CopyTo(&proto);
  EXPECT_EQ("Baz", proto.type_name());
  EXPECT_FALSE(proto.has_type());

  // Moo is a fully-qualified placeholder.
  proto.Clear();
  moo_field_->CopyTo(&proto);
  EXPECT_EQ(".corge.Moo", proto.type_name());
  EXPECT_EQ(FieldDescriptorProto::TYPE_ENUM, proto.type());
}

TEST_P(AllowUnknownDependenciesTest, CustomOptions) {
  // Moo should still have the uninterpreted option attached.
  ASSERT_EQ(1, moo_field_->options().uninterpreted_option_size());
  const UninterpretedOption& option =
      moo_field_->options().uninterpreted_option(0);
  ASSERT_EQ(1, option.name_size());
  EXPECT_EQ("grault", option.name(0).name_part());
}

TEST_P(AllowUnknownDependenciesTest, UnknownExtendee) {
  // Test that we can extend an unknown type.  This is slightly tricky because
  // it means that the placeholder type must have an extension range.

  FileDescriptorProto extension_proto;

  ASSERT_TRUE(TextFormat::ParseFromString(
      "name: 'extension.proto'"
      "extension { extendee: 'UnknownType' name:'some_extension' number:123"
      "            label:LABEL_OPTIONAL type:TYPE_INT32 }",
      &extension_proto));
  const FileDescriptor* file = BuildFile(extension_proto);

  ASSERT_TRUE(file != nullptr);

  ASSERT_EQ(1, file->extension_count());
  const Descriptor* extendee = file->extension(0)->containing_type();
  EXPECT_EQ("UnknownType", extendee->name());
  EXPECT_TRUE(extendee->is_placeholder());
  ASSERT_EQ(1, extendee->extension_range_count());
  EXPECT_EQ(1, extendee->extension_range(0)->start_number());
  EXPECT_EQ(FieldDescriptor::kMaxNumber + 1,
            extendee->extension_range(0)->end_number());
}


TEST_P(AllowUnknownDependenciesTest, CustomOption) {
  // Test that we can use a custom option without having parsed
  // descriptor.proto.

  FileDescriptorProto option_proto;

  ASSERT_TRUE(TextFormat::ParseFromString(
      "name: \"unknown_custom_options.proto\" "
      "dependency: \"google/protobuf/descriptor.proto\" "
      "extension { "
      "  extendee: \"google.protobuf.FileOptions\" "
      "  name: \"some_option\" "
      "  number: 123456 "
      "  label: LABEL_OPTIONAL "
      "  type: TYPE_INT32 "
      "} "
      "options { "
      "  uninterpreted_option { "
      "    name { "
      "      name_part: \"some_option\" "
      "      is_extension: true "
      "    } "
      "    positive_int_value: 1234 "
      "  } "
      "  uninterpreted_option { "
      "    name { "
      "      name_part: \"unknown_option\" "
      "      is_extension: true "
      "    } "
      "    positive_int_value: 1234 "
      "  } "
      "  uninterpreted_option { "
      "    name { "
      "      name_part: \"optimize_for\" "
      "      is_extension: false "
      "    } "
      "    identifier_value: \"SPEED\" "
      "  } "
      "}",
      &option_proto));

  const FileDescriptor* file = BuildFile(option_proto);
  ASSERT_TRUE(file != nullptr);

  // Verify that no extension options were set, but they were left as
  // uninterpreted_options.
  std::vector<const FieldDescriptor*> fields;
  file->options().GetReflection()->ListFields(file->options(), &fields);
  ASSERT_EQ(2, fields.size());
  EXPECT_TRUE(file->options().has_optimize_for());
  EXPECT_EQ(2, file->options().uninterpreted_option_size());
}

TEST_P(AllowUnknownDependenciesTest,
       UndeclaredDependencyTriggersBuildOfDependency) {
  // Crazy case: suppose foo.proto refers to a symbol without declaring the
  // dependency that finds it. In the event that the pool is backed by a
  // DescriptorDatabase, the pool will attempt to find the symbol in the
  // database. If successful, it will build the undeclared dependency to verify
  // that the file does indeed contain the symbol. If that file fails to build,
  // then its descriptors must be rolled back. However, we still want foo.proto
  // to build successfully, since we are allowing unknown dependencies.

  FileDescriptorProto undeclared_dep_proto;
  // We make this file fail to build by giving it two fields with tag 1.
  ASSERT_TRUE(TextFormat::ParseFromString(
      "name: \"invalid_file_as_undeclared_dep.proto\" "
      "package: \"undeclared\" "
      "message_type: {  "
      "  name: \"Mooo\"  "
      "  field { "
      "    name:'moo' number:1 label:LABEL_OPTIONAL type: TYPE_INT32 "
      "  }"
      "  field { "
      "    name:'mooo' number:1 label:LABEL_OPTIONAL type: TYPE_INT64 "
      "  }"
      "}",
      &undeclared_dep_proto));
  // We can't use the BuildFile() helper because we don't actually want to build
  // it into the descriptor pool in the fallback database case: it just needs to
  // be sitting in the database so that it gets built during the building of
  // test.proto below.
  switch (mode()) {
    case NO_DATABASE: {
      ASSERT_TRUE(pool_->BuildFile(undeclared_dep_proto) == nullptr);
      break;
    }
    case FALLBACK_DATABASE: {
      ASSERT_TRUE(db_.Add(undeclared_dep_proto));
    }
  }

  FileDescriptorProto test_proto;
  ASSERT_TRUE(TextFormat::ParseFromString(
      "name: \"test.proto\" "
      "message_type: { "
      "  name: \"Corge\" "
      "  field { "
      "    name:'mooo' number:1 label: LABEL_OPTIONAL "
      "    type_name:'undeclared.Mooo' type: TYPE_MESSAGE "
      "  }"
      "}",
      &test_proto));

  const FileDescriptor* file = BuildFile(test_proto);
  ASSERT_TRUE(file != nullptr);
  ABSL_LOG(INFO) << file->DebugString();

  EXPECT_EQ(0, file->dependency_count());
  ASSERT_EQ(1, file->message_type_count());
  const Descriptor* corge_desc = file->message_type(0);
  ASSERT_EQ("Corge", corge_desc->name());
  ASSERT_EQ(1, corge_desc->field_count());
  EXPECT_FALSE(corge_desc->is_placeholder());

  const FieldDescriptor* mooo_field = corge_desc->field(0);
  ASSERT_EQ(FieldDescriptor::TYPE_MESSAGE, mooo_field->type());
  ASSERT_EQ("Mooo", mooo_field->message_type()->name());
  ASSERT_EQ("undeclared.Mooo", mooo_field->message_type()->full_name());
  EXPECT_TRUE(mooo_field->message_type()->is_placeholder());
  // The place holder type should not be findable.
  ASSERT_TRUE(pool_->FindMessageTypeByName("undeclared.Mooo") == nullptr);
}

INSTANTIATE_TEST_SUITE_P(DatabaseSource, AllowUnknownDependenciesTest,
                         testing::Combine(testing::Values(NO_DATABASE,
                                                          FALLBACK_DATABASE),
                                          testing::Values("proto2", "proto3")));

// ===================================================================

TEST(CustomOptions, OptionLocations) {
  const Descriptor* message =
      proto2_unittest::TestMessageWithCustomOptions::descriptor();
  const FileDescriptor* file = message->file();
  const FieldDescriptor* field = message->FindFieldByName("field1");
  const OneofDescriptor* oneof = message->FindOneofByName("AnOneof");
  const FieldDescriptor* map_field = message->FindFieldByName("map_field");
  const EnumDescriptor* enm = message->FindEnumTypeByName("AnEnum");
  // TODO: Support EnumValue options, once the compiler does.
  const ServiceDescriptor* service =
      file->FindServiceByName("TestServiceWithCustomOptions");
  const MethodDescriptor* method = service->FindMethodByName("Foo");

  EXPECT_EQ(int64_t{9876543210},
            file->options().GetExtension(proto2_unittest::file_opt1));
  EXPECT_EQ(-56,
            message->options().GetExtension(proto2_unittest::message_opt1));
  EXPECT_EQ(int64_t{8765432109},
            field->options().GetExtension(proto2_unittest::field_opt1));
  EXPECT_EQ(42,  // Check that we get the default for an option we don't set.
            field->options().GetExtension(proto2_unittest::field_opt2));
  EXPECT_EQ(-99, oneof->options().GetExtension(proto2_unittest::oneof_opt1));
  EXPECT_EQ(int64_t{12345},
            map_field->options().GetExtension(proto2_unittest::field_opt1));
  EXPECT_EQ(-789, enm->options().GetExtension(proto2_unittest::enum_opt1));
  EXPECT_EQ(123, enm->value(1)->options().GetExtension(
                     proto2_unittest::enum_value_opt1));
  EXPECT_EQ(int64_t{-9876543210},
            service->options().GetExtension(proto2_unittest::service_opt1));
  EXPECT_EQ(proto2_unittest::METHODOPT1_VAL2,
            method->options().GetExtension(proto2_unittest::method_opt1));

  // See that the regular options went through unscathed.
  EXPECT_TRUE(message->options().has_message_set_wire_format());
  EXPECT_EQ(field->cpp_string_type(), FieldDescriptor::CppStringType::kString);
}

TEST(CustomOptions, OptionTypes) {
  const MessageOptions* options = nullptr;

  constexpr int32_t kint32min = std::numeric_limits<int32_t>::min();
  constexpr int32_t kint32max = std::numeric_limits<int32_t>::max();
  constexpr uint32_t kuint32max = std::numeric_limits<uint32_t>::max();
  constexpr int64_t kint64min = std::numeric_limits<int64_t>::min();
  constexpr int64_t kint64max = std::numeric_limits<int64_t>::max();
  constexpr uint64_t kuint64max = std::numeric_limits<uint64_t>::max();

  options =
      &proto2_unittest::CustomOptionMinIntegerValues::descriptor()->options();
  EXPECT_EQ(false, options->GetExtension(proto2_unittest::bool_opt));
  EXPECT_EQ(kint32min, options->GetExtension(proto2_unittest::int32_opt));
  EXPECT_EQ(kint64min, options->GetExtension(proto2_unittest::int64_opt));
  EXPECT_EQ(0, options->GetExtension(proto2_unittest::uint32_opt));
  EXPECT_EQ(0, options->GetExtension(proto2_unittest::uint64_opt));
  EXPECT_EQ(kint32min, options->GetExtension(proto2_unittest::sint32_opt));
  EXPECT_EQ(kint64min, options->GetExtension(proto2_unittest::sint64_opt));
  EXPECT_EQ(0, options->GetExtension(proto2_unittest::fixed32_opt));
  EXPECT_EQ(0, options->GetExtension(proto2_unittest::fixed64_opt));
  EXPECT_EQ(kint32min, options->GetExtension(proto2_unittest::sfixed32_opt));
  EXPECT_EQ(kint64min, options->GetExtension(proto2_unittest::sfixed64_opt));

  options =
      &proto2_unittest::CustomOptionMaxIntegerValues::descriptor()->options();
  EXPECT_EQ(true, options->GetExtension(proto2_unittest::bool_opt));
  EXPECT_EQ(kint32max, options->GetExtension(proto2_unittest::int32_opt));
  EXPECT_EQ(kint64max, options->GetExtension(proto2_unittest::int64_opt));
  EXPECT_EQ(kuint32max, options->GetExtension(proto2_unittest::uint32_opt));
  EXPECT_EQ(kuint64max, options->GetExtension(proto2_unittest::uint64_opt));
  EXPECT_EQ(kint32max, options->GetExtension(proto2_unittest::sint32_opt));
  EXPECT_EQ(kint64max, options->GetExtension(proto2_unittest::sint64_opt));
  EXPECT_EQ(kuint32max, options->GetExtension(proto2_unittest::fixed32_opt));
  EXPECT_EQ(kuint64max, options->GetExtension(proto2_unittest::fixed64_opt));
  EXPECT_EQ(kint32max, options->GetExtension(proto2_unittest::sfixed32_opt));
  EXPECT_EQ(kint64max, options->GetExtension(proto2_unittest::sfixed64_opt));

  options = &proto2_unittest::CustomOptionOtherValues::descriptor()->options();
  EXPECT_EQ(-100, options->GetExtension(proto2_unittest::int32_opt));
  EXPECT_FLOAT_EQ(12.3456789,
                  options->GetExtension(proto2_unittest::float_opt));
  EXPECT_DOUBLE_EQ(1.234567890123456789,
                   options->GetExtension(proto2_unittest::double_opt));
  EXPECT_EQ("Hello, \"World\"",
            options->GetExtension(proto2_unittest::string_opt));

  EXPECT_EQ(std::string("Hello\0World", 11),
            options->GetExtension(proto2_unittest::bytes_opt));

  EXPECT_EQ(proto2_unittest::DummyMessageContainingEnum::TEST_OPTION_ENUM_TYPE2,
            options->GetExtension(proto2_unittest::enum_opt));

  options =
      &proto2_unittest::SettingRealsFromPositiveInts::descriptor()->options();
  EXPECT_FLOAT_EQ(12, options->GetExtension(proto2_unittest::float_opt));
  EXPECT_DOUBLE_EQ(154, options->GetExtension(proto2_unittest::double_opt));

  options =
      &proto2_unittest::SettingRealsFromNegativeInts::descriptor()->options();
  EXPECT_FLOAT_EQ(-12, options->GetExtension(proto2_unittest::float_opt));
  EXPECT_DOUBLE_EQ(-154, options->GetExtension(proto2_unittest::double_opt));
}

TEST(CustomOptions, ComplexExtensionOptions) {
  const MessageOptions* options =
      &proto2_unittest::VariousComplexOptions::descriptor()->options();
  EXPECT_EQ(options->GetExtension(proto2_unittest::complex_opt1).foo(), 42);
  EXPECT_EQ(options->GetExtension(proto2_unittest::complex_opt1)
                .GetExtension(proto2_unittest::mooo),
            324);
  EXPECT_EQ(options->GetExtension(proto2_unittest::complex_opt1)
                .GetExtension(proto2_unittest::corge)
                .moo(),
            876);
  EXPECT_EQ(options->GetExtension(proto2_unittest::complex_opt2).baz(), 987);
  EXPECT_EQ(options->GetExtension(proto2_unittest::complex_opt2)
                .GetExtension(proto2_unittest::grault),
            654);
  EXPECT_EQ(options->GetExtension(proto2_unittest::complex_opt2).bar().foo(),
            743);
  EXPECT_EQ(options->GetExtension(proto2_unittest::complex_opt2)
                .bar()
                .GetExtension(proto2_unittest::mooo),
            1999);
  EXPECT_EQ(options->GetExtension(proto2_unittest::complex_opt2)
                .bar()
                .GetExtension(proto2_unittest::corge)
                .moo(),
            2008);
  EXPECT_EQ(options->GetExtension(proto2_unittest::complex_opt2)
                .GetExtension(proto2_unittest::garply)
                .foo(),
            741);
  EXPECT_EQ(options->GetExtension(proto2_unittest::complex_opt2)
                .GetExtension(proto2_unittest::garply)
                .GetExtension(proto2_unittest::mooo),
            1998);
  EXPECT_EQ(options->GetExtension(proto2_unittest::complex_opt2)
                .GetExtension(proto2_unittest::garply)
                .GetExtension(proto2_unittest::corge)
                .moo(),
            2121);
  EXPECT_EQ(options
                ->GetExtension(proto2_unittest::ComplexOptionType2::
                                   ComplexOptionType4::complex_opt4)
                .waldo(),
            1971);
  EXPECT_EQ(options->GetExtension(proto2_unittest::complex_opt2).fred().waldo(),
            321);
  EXPECT_EQ(9, options->GetExtension(proto2_unittest::complex_opt3).moo());
  EXPECT_EQ(22, options->GetExtension(proto2_unittest::complex_opt3)
                    .complexoptiontype5()
                    .plugh());
  EXPECT_EQ(24, options->GetExtension(proto2_unittest::complexopt6).xyzzy());
}

TEST(CustomOptions, OptionsFromDependency) {
  // Test that to use a custom option, we only need to import the file
  // defining the option; we do not also have to import descriptor.proto.
  DescriptorPool pool;
  {
    FileDescriptorProto file_proto;
    FileDescriptorProto::descriptor()->file()->CopyTo(&file_proto);
    ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);
  }
  {
    // We have to import the Any dependency.
    FileDescriptorProto any_proto;
    google::protobuf::Any::descriptor()->file()->CopyTo(&any_proto);
    ASSERT_TRUE(pool.BuildFile(any_proto) != nullptr);
  }
  FileDescriptorProto file_proto;
  proto2_unittest::TestMessageWithCustomOptions::descriptor()->file()->CopyTo(
      &file_proto);
  ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);

  ASSERT_TRUE(TextFormat::ParseFromString(
      "name: \"custom_options_import.proto\" "
      "package: \"proto2_unittest\" "
      "dependency: \"google/protobuf/unittest_custom_options.proto\" "
      "options { "
      "  uninterpreted_option { "
      "    name { "
      "      name_part: \"file_opt1\" "
      "      is_extension: true "
      "    } "
      "    positive_int_value: 1234 "
      "  } "
      // Test a non-extension option too.  (At one point this failed due to a
      // bug.)
      "  uninterpreted_option { "
      "    name { "
      "      name_part: \"java_package\" "
      "      is_extension: false "
      "    } "
      "    string_value: \"foo\" "
      "  } "
      // Test that enum-typed options still work too.  (At one point this also
      // failed due to a bug.)
      "  uninterpreted_option { "
      "    name { "
      "      name_part: \"optimize_for\" "
      "      is_extension: false "
      "    } "
      "    identifier_value: \"SPEED\" "
      "  } "
      "}",
      &file_proto));

  const FileDescriptor* file = pool.BuildFile(file_proto);
  ASSERT_TRUE(file != nullptr);
  EXPECT_EQ(1234, file->options().GetExtension(proto2_unittest::file_opt1));
  EXPECT_TRUE(file->options().has_java_package());
  EXPECT_EQ("foo", file->options().java_package());
  EXPECT_TRUE(file->options().has_optimize_for());
  EXPECT_EQ(FileOptions::SPEED, file->options().optimize_for());
}

TEST(CustomOptions, OptionsFromOptionDependency) {
  // Test that to use a custom option, we only need to import the file
  // defining the option; we do not also have to import descriptor.proto.
  DescriptorPool pool;
  {
    FileDescriptorProto file_proto;
    FileDescriptorProto::descriptor()->file()->CopyTo(&file_proto);
    ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);
  }
  {
    // We have to import the Any dependency.
    FileDescriptorProto any_proto;
    google::protobuf::Any::descriptor()->file()->CopyTo(&any_proto);
    ASSERT_TRUE(pool.BuildFile(any_proto) != nullptr);
  }
  FileDescriptorProto file_proto;
  proto2_unittest::TestMessageWithCustomOptions::descriptor()->file()->CopyTo(
      &file_proto);
  ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);

  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(name: "custom_options_import.proto"
           edition: EDITION_2024
           package: "proto2_unittest"
           option_dependency: "google/protobuf/unittest_custom_options.proto"
           options {
             uninterpreted_option {
               name { name_part: "file_opt1" is_extension: true }
               positive_int_value: 1234
             }
           })pb",
      &file_proto));

  const FileDescriptor* file = pool.BuildFile(file_proto);
  ASSERT_TRUE(file != nullptr);
  EXPECT_EQ(1234, file->options().GetExtension(proto2_unittest::file_opt1));
  EXPECT_EQ(FileOptions::SPEED, file->options().optimize_for());
}

TEST(CustomOptions, OptionExtensionFromOptionDependency) {
  DescriptorPool pool;
  {
    FileDescriptorProto file_proto;
    FileDescriptorProto::descriptor()->file()->CopyTo(&file_proto);
    ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);
  }
  {
    // We have to import the Any dependency.
    FileDescriptorProto any_proto;
    google::protobuf::Any::descriptor()->file()->CopyTo(&any_proto);
    ASSERT_TRUE(pool.BuildFile(any_proto) != nullptr);
  }
  FileDescriptorProto file_proto;
  proto2_unittest::TestMessageWithCustomOptions::descriptor()->file()->CopyTo(
      &file_proto);
  ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);

  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(name: "custom_options_import.proto"
           syntax: "editions"
           edition: EDITION_2024
           package: "proto2_unittest"
           option_dependency: "google/protobuf/unittest_custom_options.proto"
           message_type {
             name: "Foo"
             options {
               uninterpreted_option {
                 name { name_part: "complex_opt1" is_extension: true }
                 aggregate_value: "[proto2_unittest.mooo]: 1234"
               }
             }
           })pb",
      &file_proto));
  const FileDescriptor* file = pool.BuildFile(file_proto);
  ASSERT_TRUE(file != nullptr);
  EXPECT_EQ(1, file->message_type_count());
  EXPECT_EQ(1234, file->message_type(0)
                      ->options()
                      .GetExtension(proto2_unittest::complex_opt1)
                      .GetExtension(proto2_unittest::mooo));
}

TEST(CustomOptions, MessageOptionThreeFieldsSet) {
  // This tests a bug which previously existed in custom options parsing.  The
  // bug occurred when you defined a custom option with message type and then
  // set three fields of that option on a single definition (see the example
  // below).  The bug is a bit hard to explain, so check the change history if
  // you want to know more.
  DescriptorPool pool;

  {
    FileDescriptorProto file_proto;
    FileDescriptorProto::descriptor()->file()->CopyTo(&file_proto);
    ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);
  }
  {
    FileDescriptorProto any_proto;
    google::protobuf::Any::descriptor()->file()->CopyTo(&any_proto);
    ASSERT_TRUE(pool.BuildFile(any_proto) != nullptr);
  }
  FileDescriptorProto file_proto;
  proto2_unittest::TestMessageWithCustomOptions::descriptor()->file()->CopyTo(
      &file_proto);
  ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);

  // The following represents the definition:
  //
  //   import "google/protobuf/unittest_custom_options.proto"
  //   package proto2_unittest;
  //   message Foo {
  //     option (complex_opt1).foo  = 1234;
  //     option (complex_opt1).foo2 = 1234;
  //     option (complex_opt1).foo3 = 1234;
  //   }
  ASSERT_TRUE(TextFormat::ParseFromString(
      "name: \"custom_options_import.proto\" "
      "edition: EDITION_2024 "
      "package: \"proto2_unittest\" "
      "option_dependency: "
      "\"google/protobuf/unittest_custom_options.proto\" "
      "message_type { "
      "  name: \"Foo\" "
      "  options { "
      "    uninterpreted_option { "
      "      name { "
      "        name_part: \"complex_opt1\" "
      "        is_extension: true "
      "      } "
      "      name { "
      "        name_part: \"foo\" "
      "        is_extension: false "
      "      } "
      "      positive_int_value: 1234 "
      "    } "
      "    uninterpreted_option { "
      "      name { "
      "        name_part: \"complex_opt1\" "
      "        is_extension: true "
      "      } "
      "      name { "
      "        name_part: \"foo2\" "
      "        is_extension: false "
      "      } "
      "      positive_int_value: 1234 "
      "    } "
      "    uninterpreted_option { "
      "      name { "
      "        name_part: \"complex_opt1\" "
      "        is_extension: true "
      "      } "
      "      name { "
      "        name_part: \"foo3\" "
      "        is_extension: false "
      "      } "
      "      positive_int_value: 1234 "
      "    } "
      "  } "
      "}",
      &file_proto));

  const FileDescriptor* file = pool.BuildFile(file_proto);
  ASSERT_TRUE(file != nullptr);
  ASSERT_EQ(1, file->message_type_count());

  const MessageOptions& options = file->message_type(0)->options();
  EXPECT_EQ(1234, options.GetExtension(proto2_unittest::complex_opt1).foo());
}

TEST(CustomOptions, MessageOptionRepeatedLeafFieldSet) {
  // This test verifies that repeated fields in custom options can be
  // given multiple values by repeating the option with a different value.
  // This test checks repeated leaf values. Each repeated custom value
  // appears in a different uninterpreted_option, which will be concatenated
  // when they are merged into the final option value.
  DescriptorPool pool;

  {
    FileDescriptorProto file_proto;
    FileDescriptorProto::descriptor()->file()->CopyTo(&file_proto);
    ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);
  }
  {
    FileDescriptorProto any_proto;
    google::protobuf::Any::descriptor()->file()->CopyTo(&any_proto);
    ASSERT_TRUE(pool.BuildFile(any_proto) != nullptr);
  }
  FileDescriptorProto file_proto;
  proto2_unittest::TestMessageWithCustomOptions::descriptor()->file()->CopyTo(
      &file_proto);
  ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);

  // The following represents the definition:
  //
  //   import "google/protobuf/unittest_custom_options.proto"
  //   package proto2_unittest;
  //   message Foo {
  //     option (complex_opt1).foo4 = 12;
  //     option (complex_opt1).foo4 = 34;
  //     option (complex_opt1).foo4 = 56;
  //   }
  ASSERT_TRUE(TextFormat::ParseFromString(
      "name: \"custom_options_import.proto\" "
      "edition: EDITION_2024 "
      "package: \"proto2_unittest\" "
      "option_dependency: "
      "\"google/protobuf/unittest_custom_options.proto\" "
      "message_type { "
      "  name: \"Foo\" "
      "  options { "
      "    uninterpreted_option { "
      "      name { "
      "        name_part: \"complex_opt1\" "
      "        is_extension: true "
      "      } "
      "      name { "
      "        name_part: \"foo4\" "
      "        is_extension: false "
      "      } "
      "      positive_int_value: 12 "
      "    } "
      "    uninterpreted_option { "
      "      name { "
      "        name_part: \"complex_opt1\" "
      "        is_extension: true "
      "      } "
      "      name { "
      "        name_part: \"foo4\" "
      "        is_extension: false "
      "      } "
      "      positive_int_value: 34 "
      "    } "
      "    uninterpreted_option { "
      "      name { "
      "        name_part: \"complex_opt1\" "
      "        is_extension: true "
      "      } "
      "      name { "
      "        name_part: \"foo4\" "
      "        is_extension: false "
      "      } "
      "      positive_int_value: 56 "
      "    } "
      "  } "
      "}",
      &file_proto));

  const FileDescriptor* file = pool.BuildFile(file_proto);
  ASSERT_TRUE(file != nullptr);
  ASSERT_EQ(1, file->message_type_count());

  const MessageOptions& options = file->message_type(0)->options();
  EXPECT_EQ(3, options.GetExtension(proto2_unittest::complex_opt1).foo4_size());
  EXPECT_EQ(12, options.GetExtension(proto2_unittest::complex_opt1).foo4(0));
  EXPECT_EQ(34, options.GetExtension(proto2_unittest::complex_opt1).foo4(1));
  EXPECT_EQ(56, options.GetExtension(proto2_unittest::complex_opt1).foo4(2));
}

TEST(CustomOptions, MessageOptionRepeatedMsgFieldSet) {
  // This test verifies that repeated fields in custom options can be
  // given multiple values by repeating the option with a different value.
  // This test checks repeated message values. Each repeated custom value
  // appears in a different uninterpreted_option, which will be concatenated
  // when they are merged into the final option value.
  DescriptorPool pool;

  {
    FileDescriptorProto file_proto;
    FileDescriptorProto::descriptor()->file()->CopyTo(&file_proto);
    ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);
  }
  {
    FileDescriptorProto any_proto;
    google::protobuf::Any::descriptor()->file()->CopyTo(&any_proto);
    ASSERT_TRUE(pool.BuildFile(any_proto) != nullptr);
  }
  FileDescriptorProto file_proto;
  proto2_unittest::TestMessageWithCustomOptions::descriptor()->file()->CopyTo(
      &file_proto);
  ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);

  // The following represents the definition:
  //
  //   import "google/protobuf/unittest_custom_options.proto"
  //   package proto2_unittest;
  //   message Foo {
  //     option (complex_opt2).barney = {waldo: 1};
  //     option (complex_opt2).barney = {waldo: 10};
  //     option (complex_opt2).barney = {waldo: 100};
  //   }
  ASSERT_TRUE(TextFormat::ParseFromString(
      "name: \"custom_options_import.proto\" "
      "edition: EDITION_2024 "
      "package: \"proto2_unittest\" "
      "option_dependency: "
      "\"google/protobuf/unittest_custom_options.proto\" "
      "message_type { "
      "  name: \"Foo\" "
      "  options { "
      "    uninterpreted_option { "
      "      name { "
      "        name_part: \"complex_opt2\" "
      "        is_extension: true "
      "      } "
      "      name { "
      "        name_part: \"barney\" "
      "        is_extension: false "
      "      } "
      "      aggregate_value: \"waldo: 1\" "
      "    } "
      "    uninterpreted_option { "
      "      name { "
      "        name_part: \"complex_opt2\" "
      "        is_extension: true "
      "      } "
      "      name { "
      "        name_part: \"barney\" "
      "        is_extension: false "
      "      } "
      "      aggregate_value: \"waldo: 10\" "
      "    } "
      "    uninterpreted_option { "
      "      name { "
      "        name_part: \"complex_opt2\" "
      "        is_extension: true "
      "      } "
      "      name { "
      "        name_part: \"barney\" "
      "        is_extension: false "
      "      } "
      "      aggregate_value: \"waldo: 100\" "
      "    } "
      "  } "
      "}",
      &file_proto));

  const FileDescriptor* file = pool.BuildFile(file_proto);
  ASSERT_TRUE(file != nullptr);
  ASSERT_EQ(1, file->message_type_count());

  const MessageOptions& options = file->message_type(0)->options();
  EXPECT_EQ(3,
            options.GetExtension(proto2_unittest::complex_opt2).barney_size());
  EXPECT_EQ(
      1, options.GetExtension(proto2_unittest::complex_opt2).barney(0).waldo());
  EXPECT_EQ(
      10,
      options.GetExtension(proto2_unittest::complex_opt2).barney(1).waldo());
  EXPECT_EQ(
      100,
      options.GetExtension(proto2_unittest::complex_opt2).barney(2).waldo());
}

// Check that aggregate options were parsed and saved correctly in
// the appropriate descriptors.
TEST(CustomOptions, AggregateOptions) {
  const Descriptor* msg = proto2_unittest::AggregateMessage::descriptor();
  const FileDescriptor* file = msg->file();
  const FieldDescriptor* field = msg->FindFieldByName("fieldname");
  const EnumDescriptor* enumd = file->FindEnumTypeByName("AggregateEnum");
  const EnumValueDescriptor* enumv = enumd->FindValueByName("VALUE");
  const ServiceDescriptor* service =
      file->FindServiceByName("AggregateService");
  const MethodDescriptor* method = service->FindMethodByName("Method");

  // Tests for the different types of data embedded in fileopt
  const proto2_unittest::Aggregate& file_options =
      file->options().GetExtension(proto2_unittest::fileopt);
  EXPECT_EQ(100, file_options.i());
  EXPECT_EQ("FileAnnotation", file_options.s());
  EXPECT_EQ("NestedFileAnnotation", file_options.sub().s());
  EXPECT_EQ("FileExtensionAnnotation",
            file_options.file().GetExtension(proto2_unittest::fileopt).s());
  EXPECT_EQ("EmbeddedMessageSetElement",
            file_options.mset()
                .GetExtension(proto2_unittest::AggregateMessageSetElement ::
                                  message_set_extension)
                .s());

  proto2_unittest::AggregateMessageSetElement any_payload;
  ASSERT_TRUE(file_options.any().UnpackTo(&any_payload));
  EXPECT_EQ("EmbeddedMessageSetElement", any_payload.s());

  // Simple tests for all the other types of annotations
  EXPECT_EQ("MessageAnnotation",
            msg->options().GetExtension(proto2_unittest::msgopt).s());
  EXPECT_EQ("FieldAnnotation",
            field->options().GetExtension(proto2_unittest::fieldopt).s());
  EXPECT_EQ("EnumAnnotation",
            enumd->options().GetExtension(proto2_unittest::enumopt).s());
  EXPECT_EQ("EnumValueAnnotation",
            enumv->options().GetExtension(proto2_unittest::enumvalopt).s());
  EXPECT_EQ("ServiceAnnotation",
            service->options().GetExtension(proto2_unittest::serviceopt).s());
  EXPECT_EQ("MethodAnnotation",
            method->options().GetExtension(proto2_unittest::methodopt).s());
}

TEST(CustomOptions, UnusedImportError) {
  DescriptorPool pool;

  {
    FileDescriptorProto file_proto;
    FileDescriptorProto::descriptor()->file()->CopyTo(&file_proto);
    ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);
  }
  {
    FileDescriptorProto any_proto;
    google::protobuf::Any::descriptor()->file()->CopyTo(&any_proto);
    ASSERT_TRUE(pool.BuildFile(any_proto) != nullptr);
  }
  FileDescriptorProto file_proto;
  proto2_unittest::TestMessageWithCustomOptions::descriptor()->file()->CopyTo(
      &file_proto);
  ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);

  pool.AddDirectInputFile("custom_options_import.proto", true);
  ASSERT_TRUE(TextFormat::ParseFromString(
      "name: \"custom_options_import.proto\" "
      "package: \"proto2_unittest\" "
      "dependency: \"google/protobuf/unittest_custom_options.proto\" ",
      &file_proto));

  MockErrorCollector error_collector;
  EXPECT_FALSE(pool.BuildFileCollectingErrors(file_proto, &error_collector));
  EXPECT_EQ(
      "custom_options_import.proto: "
      "google/protobuf/unittest_custom_options.proto: IMPORT: Import "
      "google/protobuf/unittest_custom_options.proto is unused.\n",
      error_collector.text_);
}

TEST(CustomOptions, UnusedOptionImportError) {
  DescriptorPool pool;

  {
    FileDescriptorProto file_proto;
    FileDescriptorProto::descriptor()->file()->CopyTo(&file_proto);
    ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);
  }
  {
    FileDescriptorProto any_proto;
    google::protobuf::Any::descriptor()->file()->CopyTo(&any_proto);
    ASSERT_TRUE(pool.BuildFile(any_proto) != nullptr);
  }
  FileDescriptorProto file_proto;
  proto2_unittest::TestMessageWithCustomOptions::descriptor()->file()->CopyTo(
      &file_proto);
  ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);

  pool.AddDirectInputFile("custom_options_import.proto", true);
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        name: "custom_options_import.proto"
        edition: EDITION_2024
        package: "proto2_unittest"
        option_dependency: "google/protobuf/unittest_custom_options.proto"
      )pb",
      &file_proto));

  MockErrorCollector error_collector;
  EXPECT_FALSE(pool.BuildFileCollectingErrors(file_proto, &error_collector));
  EXPECT_EQ(
      "custom_options_import.proto: "
      "google/protobuf/unittest_custom_options.proto: IMPORT: Import "
      "google/protobuf/unittest_custom_options.proto is unused.\n",
      error_collector.text_);
}

// Verifies that proto files can correctly be parsed, even if the
// custom options defined in the file are incompatible with those
// compiled in the binary. See http://b/19276250.
TEST(CustomOptions, OptionsWithIncompatibleDescriptors) {
  DescriptorPool pool;

  FileDescriptorProto file_proto;
  MessageOptions::descriptor()->file()->CopyTo(&file_proto);
  ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);

  // Create a new file descriptor proto containing a subset of the
  // messages defined in google/protobuf/unittest_custom_options.proto.
  file_proto.Clear();
  file_proto.set_name("unittest_custom_options.proto");
  file_proto.set_package("proto2_unittest");
  file_proto.add_dependency("google/protobuf/descriptor.proto");

  // Add the "required_enum_opt" extension.
  FieldDescriptorProto* extension = file_proto.add_extension();
  proto2_unittest::OldOptionType::descriptor()
      ->file()
      ->FindExtensionByName("required_enum_opt")
      ->CopyTo(extension);

  // Add a test message that uses the "required_enum_opt" option.
  DescriptorProto* test_message_type = file_proto.add_message_type();
  proto2_unittest::TestMessageWithRequiredEnumOption::descriptor()->CopyTo(
      test_message_type);

  // Instruct the extension to use NewOptionType instead of
  // OldOptionType, and add the descriptor of NewOptionType.
  extension->set_type_name(".proto2_unittest.NewOptionType");
  DescriptorProto* new_option_type = file_proto.add_message_type();
  proto2_unittest::NewOptionType::descriptor()->CopyTo(new_option_type);

  // Replace the value of the "required_enum_opt" option used in the
  // test message with an enum value that only exists in NewOptionType.
  ASSERT_TRUE(
      TextFormat::ParseFromString("uninterpreted_option { "
                                  "  name { "
                                  "    name_part: 'required_enum_opt' "
                                  "    is_extension: true "
                                  "  } "
                                  "  aggregate_value: 'value: NEW_VALUE'"
                                  "}",
                                  test_message_type->mutable_options()));

  // Adding the file descriptor to the pool should fail.
  EXPECT_TRUE(pool.BuildFile(file_proto) == nullptr);
}

// Test that FileDescriptor::DebugString() formats custom options correctly.
TEST(CustomOptions, DebugString) {
  DescriptorPool pool;

  FileDescriptorProto file_proto;
  MessageOptions::descriptor()->file()->CopyTo(&file_proto);
  ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);

  // Add "foo.proto":
  //   import "google/protobuf/descriptor.proto";
  //   package "proto2_unittest";
  //   option (proto2_unittest.cc_option1) = 1;
  //   option (proto2_unittest.cc_option2) = 2;
  //   extend google.protobuf.FieldOptions {
  //     optional int32 cc_option1 = 7736974;
  //     optional int32 cc_option2 = 7736975;
  //   }
  ASSERT_TRUE(TextFormat::ParseFromString(
      "name: \"foo.proto\" "
      "package: \"proto2_unittest\" "
      "dependency: \"google/protobuf/descriptor.proto\" "
      "options { "
      "  uninterpreted_option { "
      "    name { "
      "      name_part: \"proto2_unittest.cc_option1\" "
      "      is_extension: true "
      "    } "
      "    positive_int_value: 1 "
      "  } "
      "  uninterpreted_option { "
      "    name { "
      "      name_part: \"proto2_unittest.cc_option2\" "
      "      is_extension: true "
      "    } "
      "    positive_int_value: 2 "
      "  } "
      "} "
      "extension { "
      "  name: \"cc_option1\" "
      "  extendee: \".google.protobuf.FileOptions\" "
      // This field number is intentionally chosen to be the same as
      // (.fileopt1) defined in unittest_custom_options.proto (linked
      // in this test binary). This is to test whether we are messing
      // generated pool with custom descriptor pools when dealing with
      // custom options.
      "  number: 7736974 "
      "  label: LABEL_OPTIONAL "
      "  type: TYPE_INT32 "
      "}"
      "extension { "
      "  name: \"cc_option2\" "
      "  extendee: \".google.protobuf.FileOptions\" "
      "  number: 7736975 "
      "  label: LABEL_OPTIONAL "
      "  type: TYPE_INT32 "
      "}",
      &file_proto));
  const FileDescriptor* descriptor = pool.BuildFile(file_proto);
  ASSERT_TRUE(descriptor != nullptr);

  EXPECT_EQ(2, descriptor->extension_count());

  ASSERT_EQ(
      "syntax = \"proto2\";\n"
      "\n"
      "import \"google/protobuf/descriptor.proto\";\n"
      "package proto2_unittest;\n"
      "\n"
      "option (.proto2_unittest.cc_option1) = 1;\n"
      "option (.proto2_unittest.cc_option2) = 2;\n"
      "\n"
      "extend .google.protobuf.FileOptions {\n"
      "  optional int32 cc_option1 = 7736974;\n"
      "  optional int32 cc_option2 = 7736975;\n"
      "}\n"
      "\n",
      descriptor->DebugString());
}

// ===================================================================


class ValidationErrorTest : public testing::Test {
 protected:
  void SetUp() override {
    // Enable extension declaration enforcement since most test cases want to
    // exercise the full validation.
    pool_.EnforceExtensionDeclarations(ExtDeclEnforcementLevel::kAllExtensions);
  }
  // Parse file_text as a FileDescriptorProto in text format and add it
  // to the DescriptorPool.  Expect no errors.
  const FileDescriptor* BuildFile(absl::string_view file_text) {
    FileDescriptorProto file_proto;
    EXPECT_TRUE(TextFormat::ParseFromString(file_text, &file_proto));
    return ABSL_DIE_IF_NULL(pool_.BuildFile(file_proto));
  }

  FileDescriptorProto ParseFile(absl::string_view file_name,
                                absl::string_view file_text) {
    io::ArrayInputStream input_stream(file_text.data(), file_text.size());
    SimpleErrorCollector error_collector;
    io::Tokenizer tokenizer(&input_stream, &error_collector);
    compiler::Parser parser;
    parser.RecordErrorsTo(&error_collector);
    FileDescriptorProto proto;
    ABSL_CHECK(parser.Parse(&tokenizer, &proto))
        << error_collector.last_error() << "\n"
        << file_text;
    ABSL_CHECK_EQ("", error_collector.last_error());
    proto.set_name(file_name);
    return proto;
  }

  const FileDescriptor* ParseAndBuildFile(absl::string_view file_name,
                                          absl::string_view file_text) {
    return pool_.BuildFile(ParseFile(file_name, file_text));
  }


  // Add file_proto to the DescriptorPool. Expect errors to be produced which
  // match the given error text.
  void BuildFileWithErrors(const FileDescriptorProto& file_proto,
                           testing::Matcher<std::string> expected_errors) {
    MockErrorCollector error_collector;
    EXPECT_TRUE(pool_.BuildFileCollectingErrors(file_proto, &error_collector) ==
                nullptr);
    EXPECT_THAT(error_collector.text_, expected_errors);
  }

  // Parse file_text as a FileDescriptorProto in text format and add it
  // to the DescriptorPool.  Expect errors to be produced which match the
  // given error text.
  void BuildFileWithErrors(const std::string& file_text,
                           testing::Matcher<std::string> expected_errors) {
    FileDescriptorProto file_proto;
    ASSERT_TRUE(TextFormat::ParseFromString(file_text, &file_proto));
    BuildFileWithErrors(file_proto, expected_errors);
  }

  // Parse a proto file and build it.  Expect errors to be produced which match
  // the given error text.
  void ParseAndBuildFileWithErrors(absl::string_view file_name,
                                   absl::string_view file_text,
                                   absl::string_view expected_errors) {
    MockErrorCollector error_collector;
    EXPECT_TRUE(pool_.BuildFileCollectingErrors(ParseFile(file_name, file_text),
                                                &error_collector) == nullptr);
    EXPECT_EQ(expected_errors, error_collector.text_);
  }

  void ParseAndBuildFileWithErrorSubstr(absl::string_view file_name,
                                        absl::string_view file_text,
                                        absl::string_view expected_errors) {
    MockErrorCollector error_collector;
    EXPECT_TRUE(pool_.BuildFileCollectingErrors(ParseFile(file_name, file_text),
                                                &error_collector) == nullptr);
    EXPECT_THAT(error_collector.text_, HasSubstr(expected_errors));
  }

  // Parse file_text as a FileDescriptorProto in text format and add it
  // to the DescriptorPool.  Expect errors to be produced which match the
  // given warning text.
  void BuildFileWithWarnings(const std::string& file_text,
                             const std::string& expected_warnings) {
    FileDescriptorProto file_proto;
    ASSERT_TRUE(TextFormat::ParseFromString(file_text, &file_proto));

    MockErrorCollector error_collector;
    EXPECT_TRUE(pool_.BuildFileCollectingErrors(file_proto, &error_collector));
    EXPECT_EQ(expected_warnings, error_collector.warning_text_);
  }

  // Builds some already-parsed file in our test pool.
  void BuildFileInTestPool(const FileDescriptor* file) {
    FileDescriptorProto file_proto;
    file->CopyTo(&file_proto);
    ASSERT_TRUE(pool_.BuildFile(file_proto) != nullptr);
  }

  // Build descriptor.proto in our test pool. This allows us to extend it in
  // the test pool, so we can test custom options.
  void BuildDescriptorMessagesInTestPool() {
    BuildFileInTestPool(DescriptorProto::descriptor()->file());
  }

  void BuildDescriptorMessagesInTestPoolWithErrors(
      absl::string_view expected_errors) {
    FileDescriptorProto file_proto;
    DescriptorProto::descriptor()->file()->CopyTo(&file_proto);
    MockErrorCollector error_collector;
    EXPECT_TRUE(pool_.BuildFileCollectingErrors(file_proto, &error_collector) ==
                nullptr);
    EXPECT_EQ(error_collector.text_, expected_errors);
  }

  DescriptorPool pool_;
};

TEST_F(ValidationErrorTest, AlreadyDefined) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type { name: \"Foo\" }"
      "message_type { name: \"Foo\" }",

      "foo.proto: Foo: NAME: \"Foo\" is already defined.\n");
}

TEST_F(ValidationErrorTest, AlreadyDefinedInPackage) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "package: \"foo.bar\" "
      "message_type { name: \"Foo\" }"
      "message_type { name: \"Foo\" }",

      "foo.proto: foo.bar.Foo: NAME: \"Foo\" is already defined in "
      "\"foo.bar\".\n");
}

TEST_F(ValidationErrorTest, AlreadyDefinedInOtherFile) {
  BuildFile(
      "name: \"foo.proto\" "
      "message_type { name: \"Foo\" }");

  BuildFileWithErrors(
      "name: \"bar.proto\" "
      "message_type { name: \"Foo\" }",

      "bar.proto: Foo: NAME: \"Foo\" is already defined in file "
      "\"foo.proto\".\n");
}

TEST_F(ValidationErrorTest, PackageAlreadyDefined) {
  BuildFile(
      "name: \"foo.proto\" "
      "message_type { name: \"foo\" }");
  BuildFileWithErrors(
      "name: \"bar.proto\" "
      "package: \"foo.bar\"",

      "bar.proto: foo: NAME: \"foo\" is already defined (as something other "
      "than a package) in file \"foo.proto\".\n");
}

TEST_F(ValidationErrorTest, EnumValueAlreadyDefinedInParent) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "enum_type { name: \"Foo\" value { name: \"FOO\" number: 1 } } "
      "enum_type { name: \"Bar\" value { name: \"FOO\" number: 1 } } ",

      "foo.proto: FOO: NAME: \"FOO\" is already defined.\n"
      "foo.proto: FOO: NAME: Note that enum values use C++ scoping rules, "
      "meaning that enum values are siblings of their type, not children of "
      "it.  Therefore, \"FOO\" must be unique within the global scope, not "
      "just within \"Bar\".\n");
}

TEST_F(ValidationErrorTest, EnumValueAlreadyDefinedInParentNonGlobal) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "package: \"pkg\" "
      "enum_type { name: \"Foo\" value { name: \"FOO\" number: 1 } } "
      "enum_type { name: \"Bar\" value { name: \"FOO\" number: 1 } } ",

      "foo.proto: pkg.FOO: NAME: \"FOO\" is already defined in \"pkg\".\n"
      "foo.proto: pkg.FOO: NAME: Note that enum values use C++ scoping rules, "
      "meaning that enum values are siblings of their type, not children of "
      "it.  Therefore, \"FOO\" must be unique within \"pkg\", not just within "
      "\"Bar\".\n");
}

TEST_F(ValidationErrorTest, MissingName) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type { }",

      "foo.proto: : NAME: Missing name.\n");
}

TEST_F(ValidationErrorTest, InvalidName) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type { name: \"$\" }",

      "foo.proto: $: NAME: \"$\" is not a valid identifier.\n");
}

TEST_F(ValidationErrorTest, InvalidPackageName) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "package: \"foo.$\"",

      "foo.proto: foo.$: NAME: \"$\" is not a valid identifier.\n");
}

// 'str' is a static C-style string that may contain '\0'
#define STATIC_STR(str) std::string((str), sizeof(str) - 1)

TEST_F(ValidationErrorTest, NullCharSymbolName) {
  BuildFileWithErrors(
      "name: \"bar.proto\" "
      "package: \"foo\""
      "message_type { "
      "  name: '\\000\\001\\013.Bar' "
      "  field { name: \"foo\" number:  9 label:LABEL_OPTIONAL type:TYPE_INT32 "
      "} "
      "}",
      STATIC_STR("bar.proto: foo.\0\x1\v.Bar: NAME: \"\0\x1\v.Bar\" is not a "
                 "valid identifier.\nbar.proto: foo.\0\x1\v.Bar.foo: NAME: "
                 "\"foo.\0\x1\v.Bar.foo\" contains null character.\nbar.proto: "
                 "foo.\0\x1\v.Bar: NAME: \"foo.\0\x1\v.Bar\" contains null "
                 "character.\n"));
}

TEST_F(ValidationErrorTest, NullCharFileName) {
  BuildFileWithErrors(
      "name: \"bar\\000\\001\\013.proto\" "
      "package: \"outer.foo\"",
      STATIC_STR("bar\0\x1\v.proto: bar\0\x1\v.proto: NAME: "
                 "\"bar\0\x1\v.proto\" contains null character.\n"));
}

TEST_F(ValidationErrorTest, NullCharPackageName) {
  BuildFileWithErrors(
      "name: \"bar.proto\" "
      "package: \"\\000\\001\\013.\"",
      STATIC_STR("bar.proto: \0\x1\v.: NAME: \"\0\x1\v.\" contains null "
                 "character.\n"));
}

TEST_F(ValidationErrorTest, MissingFileName) {
  BuildFileWithErrors("",

                      ": : OTHER: Missing field: FileDescriptorProto.name.\n");
}

TEST_F(ValidationErrorTest, DupeDependency) {
  BuildFile("name: \"foo.proto\"");
  BuildFileWithErrors(
      "name: \"bar.proto\" "
      "dependency: \"foo.proto\" "
      "dependency: \"foo.proto\" ",

      "bar.proto: foo.proto: IMPORT: Import \"foo.proto\" was listed twice.\n");
}

TEST_F(ValidationErrorTest, UnknownDependency) {
  BuildFileWithErrors(
      "name: \"bar.proto\" "
      "dependency: \"foo.proto\" ",

      "bar.proto: foo.proto: IMPORT: Import \"foo.proto\" has not been "
      "loaded.\n");
}

TEST_F(ValidationErrorTest, InvalidPublicDependencyIndex) {
  BuildFile("name: \"foo.proto\"");
  BuildFileWithErrors(
      "name: \"bar.proto\" "
      "dependency: \"foo.proto\" "
      "public_dependency: 1",
      "bar.proto: bar.proto: OTHER: Invalid public dependency index.\n");
}

TEST_F(ValidationErrorTest, ForeignUnimportedPackageNoCrash) {
  // Used to crash:  If we depend on a non-existent file and then refer to a
  // package defined in a file that we didn't import, and that package is
  // nested within a parent package which this file is also in, and we don't
  // include that parent package in the name (i.e. we do a relative lookup)...
  // Yes, really.
  BuildFile(
      "name: 'foo.proto' "
      "package: 'outer.foo' ");
  BuildFileWithErrors(
      "name: 'bar.proto' "
      "dependency: 'baz.proto' "
      "package: 'outer.bar' "
      "message_type { "
      "  name: 'Bar' "
      "  field { name:'bar' number:1 label:LABEL_OPTIONAL type_name:'foo.Foo' }"
      "}",

      "bar.proto: baz.proto: IMPORT: Import \"baz.proto\" has not been "
      "loaded.\n"
      "bar.proto: outer.bar.Bar.bar: TYPE: \"outer.foo\" seems to be defined "
      "in "
      "\"foo.proto\", which is not imported by \"bar.proto\".  To use it here, "
      "please add the necessary import.\n");
}

TEST_F(ValidationErrorTest, DupeFile) {
  BuildFile(
      "name: \"foo.proto\" "
      "message_type { name: \"Foo\" }");
  // Note:  We should *not* get redundant errors about "Foo" already being
  //   defined.
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type { name: \"Foo\" } "
      // Add another type so that the files aren't identical (in which case
      // there would be no error).
      "enum_type { name: \"Bar\" }",

      "foo.proto: foo.proto: OTHER: A file with this name is already in the "
      "pool.\n");
}

TEST_F(ValidationErrorTest, FieldInExtensionRange) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  field { name: \"foo\" number:  9 label:LABEL_OPTIONAL type:TYPE_INT32 "
      "}"
      "  field { name: \"bar\" number: 10 label:LABEL_OPTIONAL type:TYPE_INT32 "
      "}"
      "  field { name: \"baz\" number: 19 label:LABEL_OPTIONAL type:TYPE_INT32 "
      "}"
      "  field { name: \"moo\" number: 20 label:LABEL_OPTIONAL type:TYPE_INT32 "
      "}"
      "  extension_range { start: 10 end: 20 }"
      "}",

      "foo.proto: Foo.bar: NUMBER: Extension range 10 to 19 includes field "
      "\"bar\" (10).\n"
      "foo.proto: Foo.baz: NUMBER: Extension range 10 to 19 includes field "
      "\"baz\" (19).\n"
      "foo.proto: Foo: NUMBER: Suggested field numbers for Foo: 1, 2\n");
}

TEST_F(ValidationErrorTest, OverlappingExtensionRanges) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  extension_range { start: 10 end: 20 }"
      "  extension_range { start: 20 end: 30 }"
      "  extension_range { start: 19 end: 21 }"
      "}",

      "foo.proto: Foo: NUMBER: Extension range 19 to 20 overlaps with "
      "already-defined range 10 to 19.\n"
      "foo.proto: Foo: NUMBER: Extension range 19 to 20 overlaps with "
      "already-defined range 20 to 29.\n");
}

TEST_F(ValidationErrorTest, ReservedFieldError) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  field { name: \"foo\" number: 15 label:LABEL_OPTIONAL type:TYPE_INT32 "
      "}"
      "  reserved_range { start: 10 end: 20 }"
      "}",

      "foo.proto: Foo.foo: NUMBER: Field \"foo\" uses reserved number 15.\n"
      "foo.proto: Foo: NUMBER: Suggested field numbers for Foo: 1\n");
}

TEST_F(ValidationErrorTest, ReservedExtensionRangeError) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  extension_range { start: 10 end: 20 }"
      "  reserved_range { start: 5 end: 15 }"
      "}",

      "foo.proto: Foo: NUMBER: Extension range 10 to 19"
      " overlaps with reserved range 5 to 14.\n");
}

TEST_F(ValidationErrorTest, ReservedExtensionRangeAdjacent) {
  BuildFile(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  extension_range { start: 10 end: 20 }"
      "  reserved_range { start: 5 end: 10 }"
      "}");
}

TEST_F(ValidationErrorTest, ReservedRangeOverlap) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  reserved_range { start: 10 end: 20 }"
      "  reserved_range { start: 5 end: 15 }"
      "}",

      "foo.proto: Foo: NUMBER: Reserved range 5 to 14"
      " overlaps with already-defined range 10 to 19.\n");
}

TEST_F(ValidationErrorTest, ReservedNameError) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  field { name: \"foo\" number: 15 label:LABEL_OPTIONAL type:TYPE_INT32 "
      "}"
      "  field { name: \"bar\" number: 16 label:LABEL_OPTIONAL type:TYPE_INT32 "
      "}"
      "  field { name: \"baz\" number: 17 label:LABEL_OPTIONAL type:TYPE_INT32 "
      "}"
      "  reserved_name: \"foo\""
      "  reserved_name: \"bar\""
      "}",

      "foo.proto: Foo.foo: NAME: Field name \"foo\" is reserved.\n"
      "foo.proto: Foo.bar: NAME: Field name \"bar\" is reserved.\n");
}

TEST_F(ValidationErrorTest, ReservedNameRedundant) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  reserved_name: \"foo\""
      "  reserved_name: \"foo\""
      "}",

      "foo.proto: foo: NAME: Field name \"foo\" is reserved multiple times.\n");
}

TEST_F(ValidationErrorTest, ReservedFieldsDebugString) {
  const FileDescriptor* file = BuildFile(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  reserved_name: \"foo\""
      "  reserved_name: \"bar\""
      "  reserved_range { start: 5 end: 6 }"
      "  reserved_range { start: 10 end: 20 }"
      "}");

  ASSERT_EQ(
      "syntax = \"proto2\";\n\n"
      "message Foo {\n"
      "  reserved 5, 10 to 19;\n"
      "  reserved \"foo\", \"bar\";\n"
      "}\n\n",
      file->DebugString());
}

TEST_F(ValidationErrorTest, ReservedFieldsDebugString2023) {
  const FileDescriptor* file = BuildFile(R"pb(
    syntax: "editions"
    edition: EDITION_2023
    name: "foo.proto"
    message_type {
      name: "Foo"
      reserved_name: "foo"
      reserved_name: "bar"
      reserved_range { start: 5 end: 6 }
      reserved_range { start: 10 end: 20 }
    })pb");

  ASSERT_EQ(
      "edition = \"2023\";\n\n"
      "message Foo {\n"
      "  reserved 5, 10 to 19;\n"
      "  reserved foo, bar;\n"
      "}\n\n",
      file->DebugString());
}

TEST_F(ValidationErrorTest, DebugStringReservedRangeMax) {
  const FileDescriptor* file = BuildFile(absl::Substitute(
      "name: \"foo.proto\" "
      "enum_type { "
      "  name: \"Bar\""
      "  value { name:\"BAR\" number:1 }"
      "  reserved_range { start: 5 end: $0 }"
      "}"
      "message_type {"
      "  name: \"Foo\""
      "  reserved_range { start: 5 end: $1 }"
      "}",
      std::numeric_limits<int>::max(), FieldDescriptor::kMaxNumber + 1));

  ASSERT_EQ(
      "syntax = \"proto2\";\n\n"
      "enum Bar {\n"
      "  BAR = 1;\n"
      "  reserved 5 to max;\n"
      "}\n\n"
      "message Foo {\n"
      "  reserved 5 to max;\n"
      "}\n\n",
      file->DebugString());
}

TEST_F(ValidationErrorTest, EnumReservedFieldError) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "enum_type {"
      "  name: \"Foo\""
      "  value { name:\"BAR\" number:15 }"
      "  reserved_range { start: 10 end: 20 }"
      "}",

      "foo.proto: BAR: NUMBER: Enum value \"BAR\" uses reserved number 15.\n");
}

TEST_F(ValidationErrorTest, EnumNegativeReservedFieldError) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "enum_type {"
      "  name: \"Foo\""
      "  value { name:\"BAR\" number:-15 }"
      "  reserved_range { start: -20 end: -10 }"
      "}",

      "foo.proto: BAR: NUMBER: Enum value \"BAR\" uses reserved number -15.\n");
}

TEST_F(ValidationErrorTest, EnumReservedRangeOverlap) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "enum_type {"
      "  name: \"Foo\""
      "  value { name:\"BAR\" number:0 }"
      "  reserved_range { start: 10 end: 20 }"
      "  reserved_range { start: 5 end: 15 }"
      "}",

      "foo.proto: Foo: NUMBER: Reserved range 5 to 15"
      " overlaps with already-defined range 10 to 20.\n");
}

TEST_F(ValidationErrorTest, EnumReservedRangeOverlapByOne) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "enum_type {"
      "  name: \"Foo\""
      "  value { name:\"BAR\" number:0 }"
      "  reserved_range { start: 10 end: 20 }"
      "  reserved_range { start: 5 end: 10 }"
      "}",

      "foo.proto: Foo: NUMBER: Reserved range 5 to 10"
      " overlaps with already-defined range 10 to 20.\n");
}

TEST_F(ValidationErrorTest, EnumNegativeReservedRangeOverlap) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "enum_type {"
      "  name: \"Foo\""
      "  value { name:\"BAR\" number:0 }"
      "  reserved_range { start: -20 end: -10 }"
      "  reserved_range { start: -15 end: -5 }"
      "}",

      "foo.proto: Foo: NUMBER: Reserved range -15 to -5"
      " overlaps with already-defined range -20 to -10.\n");
}

TEST_F(ValidationErrorTest, EnumMixedReservedRangeOverlap) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "enum_type {"
      "  name: \"Foo\""
      "  value { name:\"BAR\" number:20 }"
      "  reserved_range { start: -20 end: 10 }"
      "  reserved_range { start: -15 end: 5 }"
      "}",

      "foo.proto: Foo: NUMBER: Reserved range -15 to 5"
      " overlaps with already-defined range -20 to 10.\n");
}

TEST_F(ValidationErrorTest, EnumMixedReservedRangeOverlap2) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "enum_type {"
      "  name: \"Foo\""
      "  value { name:\"BAR\" number:20 }"
      "  reserved_range { start: -20 end: 10 }"
      "  reserved_range { start: 10 end: 10 }"
      "}",

      "foo.proto: Foo: NUMBER: Reserved range 10 to 10"
      " overlaps with already-defined range -20 to 10.\n");
}

TEST_F(ValidationErrorTest, EnumReservedRangeStartGreaterThanEnd) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "enum_type {"
      "  name: \"Foo\""
      "  value { name:\"BAR\" number:20 }"
      "  reserved_range { start: 11 end: 10 }"
      "}",

      "foo.proto: Foo: NUMBER: Reserved range end number must be greater"
      " than start number.\n");
}

TEST_F(ValidationErrorTest, EnumReservedNameError) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "enum_type {"
      "  name: \"Foo\""
      "  value { name:\"FOO\" number:15 }"
      "  value { name:\"BAR\" number:15 }"
      "  reserved_name: \"FOO\""
      "  reserved_name: \"BAR\""
      "}",

      "foo.proto: FOO: NAME: Enum value \"FOO\" is reserved.\n"
      "foo.proto: BAR: NAME: Enum value \"BAR\" is reserved.\n");
}

TEST_F(ValidationErrorTest, EnumReservedNameRedundant) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "enum_type {"
      "  name: \"Foo\""
      "  value { name:\"FOO\" number:15 }"
      "  reserved_name: \"foo\""
      "  reserved_name: \"foo\""
      "}",

      "foo.proto: foo: NAME: Enum value \"foo\" is reserved multiple times.\n");
}

TEST_F(ValidationErrorTest, EnumReservedFieldsDebugString) {
  const FileDescriptor* file = BuildFile(
      "name: \"foo.proto\" "
      "enum_type {"
      "  name: \"Foo\""
      "  value { name:\"FOO\" number:3 }"
      "  reserved_name: \"foo\""
      "  reserved_name: \"bar\""
      "  reserved_range { start: -6 end: -6 }"
      "  reserved_range { start: -5 end: -4 }"
      "  reserved_range { start: -1 end: 1 }"
      "  reserved_range { start: 5 end: 5 }"
      "  reserved_range { start: 10 end: 19 }"
      "}");

  ASSERT_EQ(
      "syntax = \"proto2\";\n\n"
      "enum Foo {\n"
      "  FOO = 3;\n"
      "  reserved -6, -5 to -4, -1 to 1, 5, 10 to 19;\n"
      "  reserved \"foo\", \"bar\";\n"
      "}\n\n",
      file->DebugString());
}

TEST_F(ValidationErrorTest, EnumReservedFieldsDebugString2023) {
  const FileDescriptor* file = BuildFile(R"pb(
    syntax: "editions"
    edition: EDITION_2023
    name: "foo.proto"
    enum_type {
      name: "Foo"
      value { name: "FOO" number: 3 }
      options { features { enum_type: CLOSED } }
      reserved_name: "foo"
      reserved_name: "bar"
      reserved_range { start: -6 end: -6 }
      reserved_range { start: -5 end: -4 }
      reserved_range { start: -1 end: 1 }
      reserved_range { start: 5 end: 5 }
      reserved_range { start: 10 end: 19 }
    })pb");

  ASSERT_EQ(
      "edition = \"2023\";\n\n"
      "enum Foo {\n"
      "  option features = {\n"
      "    enum_type: CLOSED\n"
      "  };\n"
      "  FOO = 3;\n"
      "  reserved -6, -5 to -4, -1 to 1, 5, 10 to 19;\n"
      "  reserved foo, bar;\n"
      "}\n\n",
      file->DebugString());
}

TEST_F(ValidationErrorTest, InvalidDefaults) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""

      // Invalid number.
      "  field { name: \"foo\" number: 1 label: LABEL_OPTIONAL type: TYPE_INT32"
      "          default_value: \"abc\" }"

      // Empty default value.
      "  field { name: \"bar\" number: 2 label: LABEL_OPTIONAL type: TYPE_INT32"
      "          default_value: \"\" }"

      // Invalid boolean.
      "  field { name: \"baz\" number: 3 label: LABEL_OPTIONAL type: TYPE_BOOL"
      "          default_value: \"abc\" }"

      // Messages can't have defaults.
      "  field { name: \"moo\" number: 4 label: LABEL_OPTIONAL type: "
      "TYPE_MESSAGE"
      "          default_value: \"abc\" type_name: \"Foo\" }"

      // Same thing, but we don't know that this field has message type until
      // we look up the type name.
      "  field { name: \"mooo\" number: 5 label: LABEL_OPTIONAL"
      "          default_value: \"abc\" type_name: \"Foo\" }"

      // Repeateds can't have defaults.
      "  field { name: \"corge\" number: 6 label: LABEL_REPEATED type: "
      "TYPE_INT32"
      "          default_value: \"1\" }"

      // Invalid CEscaped bytes default.
      "  field { name: \"bytes_default\" number: 7 label: LABEL_OPTIONAL "
      "          type: TYPE_BYTES"
      "          default_value: \"\\\\\" }"

      "}",

      "foo.proto: Foo.foo: DEFAULT_VALUE: Couldn't parse default value "
      "\"abc\".\n"
      "foo.proto: Foo.bar: DEFAULT_VALUE: Couldn't parse default value \"\".\n"
      "foo.proto: Foo.baz: DEFAULT_VALUE: Boolean default must be true or "
      "false.\n"
      "foo.proto: Foo.moo: DEFAULT_VALUE: Messages can't have default values.\n"
      "foo.proto: Foo.corge: DEFAULT_VALUE: Repeated fields can't have default "
      "values.\n"
      "foo.proto: Foo.bytes_default: DEFAULT_VALUE: Invalid escaping in "
      "default value.\n"
      // This ends up being reported later because the error is detected at
      // cross-linking time.
      "foo.proto: Foo.mooo: DEFAULT_VALUE: Messages can't have default "
      "values.\n");
}

TEST_F(ValidationErrorTest, NegativeFieldNumber) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  field { name: \"foo\" number: -1 label:LABEL_OPTIONAL type:TYPE_INT32 "
      "}"
      "}",

      "foo.proto: Foo.foo: NUMBER: Field numbers must be positive integers.\n"
      "foo.proto: Foo: NUMBER: Suggested field numbers for Foo: 1\n");
}

TEST_F(ValidationErrorTest, HugeFieldNumber) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  field { name: \"foo\" number: 0x70000000 "
      "          label:LABEL_OPTIONAL type:TYPE_INT32 }"
      "}",

      "foo.proto: Foo.foo: NUMBER: Field numbers cannot be greater than "
      "536870911.\n"
      "foo.proto: Foo: NUMBER: Suggested field numbers for Foo: 1\n");
}

TEST_F(ValidationErrorTest, ExtensionMissingExtendee) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  extension { name: \"foo\" number: 1 label: LABEL_OPTIONAL"
      "              type_name: \"Foo\" }"
      "}",

      "foo.proto: Foo.foo: EXTENDEE: FieldDescriptorProto.extendee not set for "
      "extension field.\n");
}

TEST_F(ValidationErrorTest, NonExtensionWithExtendee) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Bar\""
      "  extension_range { start: 1 end: 2 }"
      "}"
      "message_type {"
      "  name: \"Foo\""
      "  field { name: \"foo\" number: 1 label: LABEL_OPTIONAL"
      "          type_name: \"Foo\" extendee: \"Bar\" }"
      "}",

      "foo.proto: Foo.foo: EXTENDEE: FieldDescriptorProto.extendee set for "
      "non-extension field.\n");
}

TEST_F(ValidationErrorTest, FieldOneofIndexTooLarge) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type:TYPE_INT32 "
      "          oneof_index: 1 }"
      "  field { name:\"dummy\" number:2 label:LABEL_OPTIONAL type:TYPE_INT32 "
      "          oneof_index: 0 }"
      "  oneof_decl { name:\"bar\" }"
      "}",

      "foo.proto: Foo.foo: TYPE: FieldDescriptorProto.oneof_index 1 is out of "
      "range for type \"Foo\".\n");
}

TEST_F(ValidationErrorTest, FieldOneofIndexNegative) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type:TYPE_INT32 "
      "          oneof_index: -1 }"
      "  field { name:\"dummy\" number:2 label:LABEL_OPTIONAL type:TYPE_INT32 "
      "          oneof_index: 0 }"
      "  oneof_decl { name:\"bar\" }"
      "}",

      "foo.proto: Foo.foo: TYPE: FieldDescriptorProto.oneof_index -1 is out "
      "of "
      "range for type \"Foo\".\n");
}

TEST_F(ValidationErrorTest, OneofFieldsConsecutiveDefinition) {
  // Fields belonging to the same oneof must be defined consecutively.
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  field { name:\"foo1\" number: 1 label:LABEL_OPTIONAL type:TYPE_INT32 "
      "          oneof_index: 0 }"
      "  field { name:\"bar\" number: 2 label:LABEL_OPTIONAL type:TYPE_INT32 }"
      "  field { name:\"foo2\" number: 3 label:LABEL_OPTIONAL type:TYPE_INT32 "
      "          oneof_index: 0 }"
      "  oneof_decl { name:\"foos\" }"
      "}",

      "foo.proto: Foo.bar: TYPE: Fields in the same oneof must be defined "
      "consecutively. \"bar\" cannot be defined before the completion of the "
      "\"foos\" oneof definition.\n");

  // Prevent interleaved fields, which belong to different oneofs.
  BuildFileWithErrors(
      "name: \"foo2.proto\" "
      "message_type {"
      "  name: \"Foo2\""
      "  field { name:\"foo1\" number: 1 label:LABEL_OPTIONAL type:TYPE_INT32 "
      "          oneof_index: 0 }"
      "  field { name:\"bar1\" number: 2 label:LABEL_OPTIONAL type:TYPE_INT32 "
      "          oneof_index: 1 }"
      "  field { name:\"foo2\" number: 3 label:LABEL_OPTIONAL type:TYPE_INT32 "
      "          oneof_index: 0 }"
      "  field { name:\"bar2\" number: 4 label:LABEL_OPTIONAL type:TYPE_INT32 "
      "          oneof_index: 1 }"
      "  oneof_decl { name:\"foos\" }"
      "  oneof_decl { name:\"bars\" }"
      "}",
      "foo2.proto: Foo2.bar1: TYPE: Fields in the same oneof must be defined "
      "consecutively. \"bar1\" cannot be defined before the completion of the "
      "\"foos\" oneof definition.\n"
      "foo2.proto: Foo2.foo2: TYPE: Fields in the same oneof must be defined "
      "consecutively. \"foo2\" cannot be defined before the completion of the "
      "\"bars\" oneof definition.\n");

  // Another case for normal fields and different oneof fields interleave.
  BuildFileWithErrors(
      "name: \"foo3.proto\" "
      "message_type {"
      "  name: \"Foo3\""
      "  field { name:\"foo1\" number: 1 label:LABEL_OPTIONAL type:TYPE_INT32 "
      "          oneof_index: 0 }"
      "  field { name:\"bar1\" number: 2 label:LABEL_OPTIONAL type:TYPE_INT32 "
      "          oneof_index: 1 }"
      "  field { name:\"baz\" number: 3 label:LABEL_OPTIONAL type:TYPE_INT32 }"
      "  field { name:\"foo2\" number: 4 label:LABEL_OPTIONAL type:TYPE_INT32 "
      "          oneof_index: 0 }"
      "  oneof_decl { name:\"foos\" }"
      "  oneof_decl { name:\"bars\" }"
      "}",
      "foo3.proto: Foo3.baz: TYPE: Fields in the same oneof must be defined "
      "consecutively. \"baz\" cannot be defined before the completion of the "
      "\"foos\" oneof definition.\n");
}

TEST_F(ValidationErrorTest, FieldNumberConflict) {
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        message_type {
          name: "Foo"
          field { name: "foo" number: 1 label: LABEL_OPTIONAL type: TYPE_INT32 }
          field { name: "bar" number: 1 label: LABEL_OPTIONAL type: TYPE_INT32 }
        }
      )pb",
      "foo.proto: Foo.bar: NUMBER: Field number 1 has already been used in "
      "\"Foo\" by field \"foo\". Next available field number is 2.\n");

  // Now we add other fields, extension ranges, reserved fields, etc.
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        message_type {
          name: "Foo"
          field { name: "foo" number: 1 label: LABEL_OPTIONAL type: TYPE_INT32 }
          field { name: "bar" number: 1 label: LABEL_OPTIONAL type: TYPE_INT32 }
          field { name: "baz" number: 2 label: LABEL_OPTIONAL type: TYPE_INT32 }
          extension_range { start: 3 end: 6 }
          field { name: "bak" number: 6 label: LABEL_OPTIONAL type: TYPE_INT32 }
          extension_range { start: 7 end: 10 }
          field { name: "bm" number: 10 label: LABEL_OPTIONAL type: TYPE_INT32 }
          reserved_range { start: 11 end: 20 }
          field { name: "bt" number: 20 label: LABEL_OPTIONAL type: TYPE_INT32 }
          field { name: "br" number: 22 label: LABEL_OPTIONAL type: TYPE_INT32 }
        }
      )pb",
      "foo.proto: Foo.bar: NUMBER: Field number 1 has already been used in "
      "\"Foo\" by field \"foo\". Next available field number is 21.\n");

  // Now there are no available numbers.
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        message_type {
          name: "Foo"
          field { name: "foo" number: 1 label: LABEL_OPTIONAL type: TYPE_INT32 }
          field { name: "bar" number: 1 label: LABEL_OPTIONAL type: TYPE_INT32 }
          reserved_range { start: 2 end: 536870913 }
        }
      )pb",
      "foo.proto: Foo.bar: NUMBER: Field number 1 has already been used in "
      "\"Foo\" by field \"foo\". There are no available field numbers.\n");

  // Overflow check. Exhaust the whole range, and make the field number INT_MAX.
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        message_type {
          name: "Foo"
          field { name: "foo" number: 2147483647 type: TYPE_INT32 }
          field { name: "bar" number: 2147483647 type: TYPE_INT32 }
          reserved_range { start: 1 end: 2147483647 }
        }
      )pb",
      HasSubstr("There are no available field numbers."));
  // Overflow check. Exhaust the whole range, and make ranges INT_MAX, INT_MIN.
  // The input is invalid, so we only care that it doesn't trigger a sanitizer
  // failure.
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        message_type {
          name: "Foo"
          field { name: "foo" number: 1 type: TYPE_INT32 }
          field { name: "bar" number: 1 type: TYPE_INT32 }
          extension_range { start: 2 end: 2147483647 }
          extension_range { start: 2 end: -2147483648 }
        }
      )pb",
      HasSubstr("field number"));
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        message_type {
          name: "Foo"
          field { name: "foo" number: 1 type: TYPE_INT32 }
          field { name: "bar" number: 1 type: TYPE_INT32 }
          reserved_range { start: 2 end: 2147483647 }
          reserved_range { start: 2 end: -2147483648 }
        }
      )pb",
      HasSubstr("field number"));
}

TEST_F(ValidationErrorTest, BadMessageSetExtensionType) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"MessageSet\""
      "  options { message_set_wire_format: true }"
      "  extension_range { start: 4 end: 5 }"
      "}"
      "message_type {"
      "  name: \"Foo\""
      "  extension { name:\"foo\" number:4 label:LABEL_OPTIONAL type:TYPE_INT32"
      "              extendee: \"MessageSet\" }"
      "}",

      "foo.proto: Foo.foo: TYPE: Extensions of MessageSets must be optional "
      "messages.\n");
}

TEST_F(ValidationErrorTest, BadMessageSetExtensionLabel) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"MessageSet\""
      "  options { message_set_wire_format: true }"
      "  extension_range { start: 4 end: 5 }"
      "}"
      "message_type {"
      "  name: \"Foo\""
      "  extension { name:\"foo\" number:4 label:LABEL_REPEATED "
      "type:TYPE_MESSAGE"
      "              type_name: \"Foo\" extendee: \"MessageSet\" }"
      "}",

      "foo.proto: Foo.foo: TYPE: Extensions of MessageSets must be optional "
      "messages.\n");
}

TEST_F(ValidationErrorTest, FieldInMessageSet) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  options { message_set_wire_format: true }"
      "  field { name: \"foo\" number: 1 label:LABEL_OPTIONAL type:TYPE_INT32 }"
      "}",

      "foo.proto: Foo.foo: NAME: MessageSets cannot have fields, only "
      "extensions.\n");
}

TEST_F(ValidationErrorTest, NegativeExtensionRangeNumber) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  extension_range { start: -10 end: -1 }"
      "}",

      "foo.proto: Foo: NUMBER: Extension numbers must be positive integers.\n");
}

TEST_F(ValidationErrorTest, HugeExtensionRangeNumber) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  extension_range { start: 1 end: 0x70000000 }"
      "}",

      "foo.proto: Foo: NUMBER: Extension numbers cannot be greater than "
      "536870911.\n");
}

TEST_F(ValidationErrorTest, ExtensionRangeEndBeforeStart) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  extension_range { start: 10 end: 10 }"
      "  extension_range { start: 10 end: 5 }"
      "}",

      "foo.proto: Foo: NUMBER: Extension range end number must be greater than "
      "start number.\n"
      "foo.proto: Foo: NUMBER: Extension range end number must be greater than "
      "start number.\n");
}

TEST_F(ValidationErrorTest, EmptyEnum) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "enum_type { name: \"Foo\" }"
      // Also use the empty enum in a message to make sure there are no crashes
      // during validation (possible if the code attempts to derive a default
      // value for the field).
      "message_type {"
      "  name: \"Bar\""
      "  field { name: \"foo\" number: 1 label:LABEL_OPTIONAL "
      "type_name:\"Foo\" }"
      "  field { name: \"bar\" number: 2 label:LABEL_OPTIONAL "
      "type_name:\"Foo\" "
      "          default_value: \"NO_SUCH_VALUE\" }"
      "}",

      "foo.proto: Foo: NAME: Enums must contain at least one value.\n"
      "foo.proto: Bar.bar: DEFAULT_VALUE: Enum type \"Foo\" has no value named "
      "\"NO_SUCH_VALUE\".\n");
}

TEST_F(ValidationErrorTest, UndefinedExtendee) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  extension { name:\"foo\" number:1 label:LABEL_OPTIONAL type:TYPE_INT32"
      "              extendee: \"Bar\" }"
      "}",

      "foo.proto: Foo.foo: EXTENDEE: \"Bar\" is not defined.\n");
}

TEST_F(ValidationErrorTest, NonMessageExtendee) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "enum_type { name: \"Bar\" value { name:\"DUMMY\" number:0 } }"
      "message_type {"
      "  name: \"Foo\""
      "  extension { name:\"foo\" number:1 label:LABEL_OPTIONAL type:TYPE_INT32"
      "              extendee: \"Bar\" }"
      "}",

      "foo.proto: Foo.foo: EXTENDEE: \"Bar\" is not a message type.\n");
}

TEST_F(ValidationErrorTest, NotAnExtensionNumber) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Bar\""
      "}"
      "message_type {"
      "  name: \"Foo\""
      "  extension { name:\"foo\" number:1 label:LABEL_OPTIONAL type:TYPE_INT32"
      "              extendee: \"Bar\" }"
      "}",

      "foo.proto: Foo.foo: NUMBER: \"Bar\" does not declare 1 as an extension "
      "number.\n");
}

TEST_F(ValidationErrorTest, RequiredExtension) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Bar\""
      "  extension_range { start: 1000 end: 10000 }"
      "}"
      "message_type {"
      "  name: \"Foo\""
      "  extension {"
      "    name:\"foo\""
      "    number:1000"
      "    label:LABEL_REQUIRED"
      "    type:TYPE_INT32"
      "    extendee: \"Bar\""
      "  }"
      "}",

      "foo.proto: Foo.foo: TYPE: The extension Foo.foo cannot be required.\n");
}

TEST_F(ValidationErrorTest, UndefinedFieldType) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type_name:\"Bar\" }"
      "}",

      "foo.proto: Foo.foo: TYPE: \"Bar\" is not defined.\n");
}

TEST_F(ValidationErrorTest, UndefinedFieldTypeWithDefault) {
  // See b/12533582. Previously this failed because the default value was not
  // accepted by the parser, which assumed an enum type, leading to an unclear
  // error message. We want this input to yield a validation error instead,
  // since the unknown type is the primary problem.
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type_name:\"int\" "
      "          default_value:\"1\" }"
      "}",

      "foo.proto: Foo.foo: TYPE: \"int\" is not defined.\n");
}

TEST_F(ValidationErrorTest, UndefinedNestedFieldType) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  nested_type { name:\"Baz\" }"
      "  field { name:\"foo\" number:1"
      "          label:LABEL_OPTIONAL"
      "          type_name:\"Foo.Baz.Bar\" }"
      "}",

      "foo.proto: Foo.foo: TYPE: \"Foo.Baz.Bar\" is not defined.\n");
}

TEST_F(ValidationErrorTest, FieldTypeDefinedInUndeclaredDependency) {
  BuildFile(
      "name: \"bar.proto\" "
      "message_type { name: \"Bar\" } ");

  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type_name:\"Bar\" }"
      "}",
      "foo.proto: Foo.foo: TYPE: \"Bar\" seems to be defined in \"bar.proto\", "
      "which is not imported by \"foo.proto\".  To use it here, please add the "
      "necessary import.\n");
}

TEST_F(ValidationErrorTest, FieldTypeDefinedInIndirectDependency) {
  // Test for hidden dependencies.
  //
  // // bar.proto
  // message Bar{}
  //
  // // forward.proto
  // import "bar.proto"
  //
  // // foo.proto
  // import "forward.proto"
  // message Foo {
  //   optional Bar foo = 1;  // Error, needs to import bar.proto explicitly.
  // }
  //
  BuildFile(
      "name: \"bar.proto\" "
      "message_type { name: \"Bar\" }");

  BuildFile(
      "name: \"forward.proto\""
      "dependency: \"bar.proto\"");

  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "dependency: \"forward.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type_name:\"Bar\" }"
      "}",
      "foo.proto: Foo.foo: TYPE: \"Bar\" seems to be defined in \"bar.proto\", "
      "which is not imported by \"foo.proto\".  To use it here, please add the "
      "necessary import.\n");
}

TEST_F(ValidationErrorTest, FieldTypeDefinedInPublicDependency) {
  // Test for public dependencies.
  //
  // // bar.proto
  // message Bar{}
  //
  // // forward.proto
  // import public "bar.proto"
  //
  // // foo.proto
  // import "forward.proto"
  // message Foo {
  //   optional Bar foo = 1;  // Correct. "bar.proto" is public imported into
  //                          // forward.proto, so when "foo.proto" imports
  //                          // "forward.proto", it imports "bar.proto" too.
  // }
  //
  BuildFile(
      "name: \"bar.proto\" "
      "message_type { name: \"Bar\" }");

  BuildFile(
      "name: \"forward.proto\""
      "dependency: \"bar.proto\" "
      "public_dependency: 0");

  BuildFile(
      "name: \"foo.proto\" "
      "dependency: \"forward.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type_name:\"Bar\" }"
      "}");
}

TEST_F(ValidationErrorTest, FieldTypeDefinedInTransitivePublicDependency) {
  // Test for public dependencies.
  //
  // // bar.proto
  // message Bar{}
  //
  // // forward.proto
  // import public "bar.proto"
  //
  // // forward2.proto
  // import public "forward.proto"
  //
  // // foo.proto
  // import "forward2.proto"
  // message Foo {
  //   optional Bar foo = 1;  // Correct, public imports are transitive.
  // }
  //
  BuildFile(
      "name: \"bar.proto\" "
      "message_type { name: \"Bar\" }");

  BuildFile(
      "name: \"forward.proto\""
      "dependency: \"bar.proto\" "
      "public_dependency: 0");

  BuildFile(
      "name: \"forward2.proto\""
      "dependency: \"forward.proto\" "
      "public_dependency: 0");

  BuildFile(
      "name: \"foo.proto\" "
      "dependency: \"forward2.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type_name:\"Bar\" }"
      "}");
}

TEST_F(ValidationErrorTest,
       FieldTypeDefinedInPrivateDependencyOfPublicDependency) {
  // Test for public dependencies.
  //
  // // bar.proto
  // message Bar{}
  //
  // // forward.proto
  // import "bar.proto"
  //
  // // forward2.proto
  // import public "forward.proto"
  //
  // // foo.proto
  // import "forward2.proto"
  // message Foo {
  //   optional Bar foo = 1;  // Error, the "bar.proto" is not public imported
  //                          // into "forward.proto", so will not be imported
  //                          // into either "forward2.proto" or "foo.proto".
  // }
  //
  BuildFile(
      "name: \"bar.proto\" "
      "message_type { name: \"Bar\" }");

  BuildFile(
      "name: \"forward.proto\""
      "dependency: \"bar.proto\"");

  BuildFile(
      "name: \"forward2.proto\""
      "dependency: \"forward.proto\" "
      "public_dependency: 0");

  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "dependency: \"forward2.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type_name:\"Bar\" }"
      "}",
      "foo.proto: Foo.foo: TYPE: \"Bar\" seems to be defined in \"bar.proto\", "
      "which is not imported by \"foo.proto\".  To use it here, please add the "
      "necessary import.\n");
}

class ImportOptionValidationErrorTest : public ValidationErrorTest {};

TEST_F(ImportOptionValidationErrorTest, OptionDefinedInOptionDependency) {
  BuildDescriptorMessagesInTestPool();
  ParseAndBuildFile("bar.proto",
                    R"schema(
    syntax = "proto2";
    import "google/protobuf/descriptor.proto";
    message Bar {
      optional int32 baz = 1;
    }
    extend google.protobuf.FieldOptions {
      optional Bar bar = 5000;
    })schema");
  // Correct. "bar.proto" is option imported so bar is defined.
  ParseAndBuildFile("foo.proto",
                    R"schema(
    edition = "2024";
    import option "bar.proto";
    message Foo {
      int32 foo = 1 [(bar) = {baz: 1}];
    })schema");
}

TEST_F(ImportOptionValidationErrorTest,
       OptionDefinedInTransitivePublicOptionDependency) {
  BuildDescriptorMessagesInTestPool();
  ParseAndBuildFile("bar.proto",
                    R"schema(
    syntax = "proto2";
    import "google/protobuf/descriptor.proto";
    message Bar {
      optional int32 baz = 1;
    }
    extend google.protobuf.FieldOptions {
      optional Bar bar = 5000;
    })schema");
  ParseAndBuildFile("forward.proto",
                    R"schema(
    edition = "2024";
    import option "bar.proto";
    )schema");
  ParseAndBuildFile("forward2.proto",
                    R"schema(
    syntax = "proto2";
    import public "forward.proto";
    )schema");
  // Incorrect. option imports of public imports are not transitive.
  ParseAndBuildFileWithErrors(
      "foo.proto",
      R"schema(
    syntax = "proto2";
    import public "forward2.proto";
    message Foo {
      optional int32 foo = 1 [(bar) = {baz: 1}];
    })schema",
      "foo.proto: Foo.foo: OPTION_NAME: Option \"(bar)\" unknown. Ensure that "
      "your proto "
      "definition file imports the proto which defines the option (i.e. via "
      "import option).\n");
}

TEST_F(ImportOptionValidationErrorTest,
       OptionDefinedInTransitiveOptionPublicDependency) {
  BuildDescriptorMessagesInTestPool();
  ParseAndBuildFile("bar.proto",
                    R"schema(
    syntax = "proto2";
    import "google/protobuf/descriptor.proto";
    message Bar {
      optional int32 baz = 1;
    }
    extend google.protobuf.FieldOptions {
      optional Bar bar = 5000;
    })schema");
  ParseAndBuildFile("forward.proto",
                    R"schema(
    syntax = "proto2";
    import public "bar.proto";
    )schema");
  ParseAndBuildFile("forward2.proto",
                    R"schema(
    syntax = "proto2";
    import public "forward.proto";
    )schema");
  // Correct. public imports of option imports are transitive.
  ParseAndBuildFile("foo.proto",
                    R"schema(
    edition = "2024";
    import option "forward2.proto";
    message Foo {
      int32 foo = 1 [(bar) = {baz: 1}];
    })schema");
}

TEST_F(ImportOptionValidationErrorTest,
       FieldMessageTypeDefinedInOptionDependencyErrors) {
  BuildDescriptorMessagesInTestPool();
  ParseAndBuildFile("bar.proto",
                    R"schema(
    syntax = "proto2";
    import "google/protobuf/descriptor.proto";
    message Bar {
      optional int32 baz = 1;
    }
    extend google.protobuf.FieldOptions {
      optional Bar bar = 5000;
    })schema");
  // Incorrect. "bar.proto" is option imported, so Bar is not defined.
  ParseAndBuildFileWithErrors(
      "foo.proto",
      R"schema(
      edition = "2024";
      import option "bar.proto";
      message Foo {
        Bar foo = 1;
      })schema",
      "foo.proto: Foo.foo: TYPE: \"Bar\" seems to be defined in \"bar.proto\", "
      "which is not imported by \"foo.proto\".  To use it here, please add the "
      "necessary import.\n");
}

TEST_F(ImportOptionValidationErrorTest,
       FieldEnumTypeDefinedInOptionDependencyErrors) {
  BuildDescriptorMessagesInTestPool();
  ParseAndBuildFile("bar.proto",
                    R"schema(
    syntax = "proto2";
    import "google/protobuf/descriptor.proto";
    enum Bar {
      BAR = 1;
    }
    extend google.protobuf.FieldOptions {
      optional Bar bar = 5000;
    })schema");
  // Incorrect. "bar.proto" is option imported, so Bar is not defined.
  ParseAndBuildFileWithErrors(
      "foo.proto",
      R"schema(
      edition = "2024";
      import option "bar.proto";
      message Foo {
        Bar foo = 1;
      })schema",
      "foo.proto: Foo.foo: TYPE: \"Bar\" seems to be defined in \"bar.proto\", "
      "which is not imported by \"foo.proto\".  To use it here, please add the "
      "necessary import.\n");
}

TEST_F(ImportOptionValidationErrorTest,
       InvalidOptionDependencyBeforeEdition2024) {
  BuildDescriptorMessagesInTestPool();
  ParseAndBuildFile("bar.proto",
                    R"schema(
      syntax = "proto2";
      import "google/protobuf/descriptor.proto";
      enum Bar {
        BAR = 1;
      }
      extend google.protobuf.FieldOptions {
        optional Bar bar = 5000;
      })schema");

  BuildFileWithErrors(
      R"pb(
        name: 'foo.proto'
        edition: EDITION_2023
        option_dependency: "bar.proto"
      )pb",
      "foo.proto: option: IMPORT: option imports are not supported before "
      "edition 2024.\n");
}


TEST_F(ValidationErrorTest, SearchMostLocalFirst) {
  // The following should produce an error that Bar.Baz is resolved but
  // not defined:
  //   message Bar { message Baz {} }
  //   message Foo {
  //     message Bar {
  //       // Placing "message Baz{}" here, or removing Foo.Bar altogether,
  //       // would fix the error.
  //     }
  //     optional Bar.Baz baz = 1;
  //   }
  // An one point the lookup code incorrectly did not produce an error in this
  // case, because when looking for Bar.Baz, it would try "Foo.Bar.Baz" first,
  // fail, and ten try "Bar.Baz" and succeed, even though "Bar" should actually
  // refer to the inner Bar, not the outer one.
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Bar\""
      "  nested_type { name: \"Baz\" }"
      "}"
      "message_type {"
      "  name: \"Foo\""
      "  nested_type { name: \"Bar\" }"
      "  field { name:\"baz\" number:1 label:LABEL_OPTIONAL"
      "          type_name:\"Bar.Baz\" }"
      "}",

      "foo.proto: Foo.baz: TYPE: \"Bar.Baz\" is resolved to \"Foo.Bar.Baz\","
      " which is not defined. The innermost scope is searched first in name "
      "resolution. Consider using a leading '.'(i.e., \".Bar.Baz\") to start "
      "from the outermost scope.\n");
}

TEST_F(ValidationErrorTest, SearchMostLocalFirst2) {
  // This test would find the most local "Bar" first, and does, but
  // proceeds to find the outer one because the inner one's not an
  // aggregate.
  BuildFile(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Bar\""
      "  nested_type { name: \"Baz\" }"
      "}"
      "message_type {"
      "  name: \"Foo\""
      "  field { name: \"Bar\" number:1 type:TYPE_BYTES } "
      "  field { name:\"baz\" number:2 label:LABEL_OPTIONAL"
      "          type_name:\"Bar.Baz\" }"
      "}");
}

TEST_F(ValidationErrorTest, PackageOriginallyDeclaredInTransitiveDependent) {
  // Imagine we have the following:
  //
  // foo.proto:
  //   package foo.bar;
  // bar.proto:
  //   package foo.bar;
  //   import "foo.proto";
  //   message Bar {}
  // baz.proto:
  //   package foo;
  //   import "bar.proto"
  //   message Baz { optional bar.Bar moo = 1; }
  //
  // When validating baz.proto, we will look up "bar.Bar".  As part of this
  // lookup, we first lookup "bar" then try to find "Bar" within it.  "bar"
  // should resolve to "foo.bar".  Note, though, that "foo.bar" was first
  // defined in foo.proto, which is not a direct dependency of baz.proto.  The
  // implementation of FindSymbol() normally only returns symbols in direct
  // dependencies, not indirect ones, for non-package symbols.  This test
  // insures that this does not prevent it from finding "foo.bar".

  BuildFile(
      "name: \"foo.proto\" "
      "package: \"foo.bar\" ");
  BuildFile(
      "name: \"bar.proto\" "
      "package: \"foo.bar\" "
      "dependency: \"foo.proto\" "
      "message_type { name: \"Bar\" }");
  BuildFile(
      "name: \"baz.proto\" "
      "package: \"foo\" "
      "dependency: \"bar.proto\" "
      "message_type { "
      "  name: \"Baz\" "
      "  field { name:\"moo\" number:1 label:LABEL_OPTIONAL "
      "          type_name:\"bar.Bar\" }"
      "}");
}

TEST_F(ValidationErrorTest,
       PackageOriginallyDeclaredInOptionTransitiveDependent) {
  // Imagine we have the following:
  //
  // foo.proto:
  //   package foo.bar;
  // bar.proto:
  //   package foo.bar;
  //   import "foo.proto";
  //   extend google.protobuf.FileOptions {
  //     optional uint64 file_opt1 = 7736974;
  //   }
  // baz.proto:
  //   package foo;
  //   import option "bar.proto"
  //   option (bar.file_opt1) = 1234;
  //
  //
  // When validating baz.proto, we will look up "bar.file_opt1".  As part of
  // this lookup, we first lookup "bar" then try to find "file_opt1" within it.
  // "bar" should resolve to "foo.bar".  Note, though, that "foo.bar" was first
  // defined in foo.proto, which is not a direct dependency of
  // baz.proto.  The implementation of FindSymbol() normally only returns
  // symbols in direct dependencies, not indirect ones, for non-package symbols.
  // This test insures that this does not prevent it from finding "foo.bar".
  BuildDescriptorMessagesInTestPool();
  BuildFile(
      R"pb(
        name: "foo.proto" package: "foo.bar"
      )pb");
  BuildFile(
      R"pb(
        name: "bar.proto"
        package: "foo.bar"
        dependency: "foo.proto"
        dependency: "google/protobuf/descriptor.proto"
        extension {
          name: "file_opt1"
          number: 7736974
          label: LABEL_OPTIONAL
          type: TYPE_UINT64
          extendee: ".google.protobuf.FileOptions"
        }
      )pb");
  BuildFile(
      R"pb(
        name: "baz.proto"
        edition: EDITION_2024
        package: "foo"
        option_dependency: "bar.proto"
        options {
          uninterpreted_option {
            name { name_part: "bar.file_opt1" is_extension: true }
            positive_int_value: 1234
          }
        }
      )pb");
}

TEST_F(ValidationErrorTest, FieldTypeNotAType) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL "
      "          type_name:\".Foo.bar\" }"
      "  field { name:\"bar\" number:2 label:LABEL_OPTIONAL type:TYPE_INT32 }"
      "}",

      "foo.proto: Foo.foo: TYPE: \".Foo.bar\" is not a type.\n");
}

TEST_F(ValidationErrorTest, RelativeFieldTypeNotAType) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  nested_type {"
      "    name: \"Bar\""
      "    field { name:\"Baz\" number:2 label:LABEL_OPTIONAL type:TYPE_INT32 }"
      "  }"
      "  name: \"Foo\""
      "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL "
      "          type_name:\"Bar.Baz\" }"
      "}",
      "foo.proto: Foo.foo: TYPE: \"Bar.Baz\" is not a type.\n");
}

TEST_F(ValidationErrorTest, FieldTypeMayBeItsName) {
  BuildFile(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Bar\""
      "}"
      "message_type {"
      "  name: \"Foo\""
      "  field { name:\"Bar\" number:1 label:LABEL_OPTIONAL type_name:\"Bar\" }"
      "}");
}

TEST_F(ValidationErrorTest, EnumFieldTypeIsMessage) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type { name: \"Bar\" } "
      "message_type {"
      "  name: \"Foo\""
      "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type:TYPE_ENUM"
      "          type_name:\"Bar\" }"
      "}",

      "foo.proto: Foo.foo: TYPE: \"Bar\" is not an enum type.\n");
}

TEST_F(ValidationErrorTest, MessageFieldTypeIsEnum) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "enum_type { name: \"Bar\" value { name:\"DUMMY\" number:0 } } "
      "message_type {"
      "  name: \"Foo\""
      "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type:TYPE_MESSAGE"
      "          type_name:\"Bar\" }"
      "}",

      "foo.proto: Foo.foo: TYPE: \"Bar\" is not a message type.\n");
}

TEST_F(ValidationErrorTest, BadEnumDefaultValue) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "enum_type { name: \"Bar\" value { name:\"DUMMY\" number:0 } } "
      "message_type {"
      "  name: \"Foo\""
      "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type_name:\"Bar\""
      "          default_value:\"NO_SUCH_VALUE\" }"
      "}",

      "foo.proto: Foo.foo: DEFAULT_VALUE: Enum type \"Bar\" has no value named "
      "\"NO_SUCH_VALUE\".\n");
}

TEST_F(ValidationErrorTest, EnumDefaultValueIsInteger) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "enum_type { name: \"Bar\" value { name:\"DUMMY\" number:0 } } "
      "message_type {"
      "  name: \"Foo\""
      "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type_name:\"Bar\""
      "          default_value:\"0\" }"
      "}",

      "foo.proto: Foo.foo: DEFAULT_VALUE: Default value for an enum field must "
      "be an identifier.\n");
}

TEST_F(ValidationErrorTest, PrimitiveWithTypeName) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type:TYPE_INT32"
      "          type_name:\"Foo\" }"
      "}",

      "foo.proto: Foo.foo: TYPE: Field with primitive type has type_name.\n");
}

TEST_F(ValidationErrorTest, NonPrimitiveWithoutTypeName) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type:TYPE_MESSAGE }"
      "}",

      "foo.proto: Foo.foo: TYPE: Field with message or enum type missing "
      "type_name.\n");
}

TEST_F(ValidationErrorTest, OneofWithNoFields) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  oneof_decl { name:\"bar\" }"
      "}",

      "foo.proto: Foo.bar: NAME: Oneof must have at least one field.\n");
}

TEST_F(ValidationErrorTest, OneofLabelMismatch) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"Foo\""
      "  field { name:\"foo\" number:1 label:LABEL_REPEATED type:TYPE_INT32 "
      "          oneof_index:0 }"
      "  oneof_decl { name:\"bar\" }"
      "}",

      "foo.proto: Foo.foo: NAME: Fields of oneofs must themselves have label "
      "LABEL_OPTIONAL.\n");
}

TEST_F(ValidationErrorTest, InputTypeNotDefined) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type { name: \"Foo\" } "
      "service {"
      "  name: \"TestService\""
      "  method { name: \"A\" input_type: \"Bar\" output_type: \"Foo\" }"
      "}",

      "foo.proto: TestService.A: INPUT_TYPE: \"Bar\" is not defined.\n");
}

TEST_F(ValidationErrorTest, ServiceWithEmptyName) {
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        message_type { name: "Foo" }
        service { name: "" }
      )pb",
      "foo.proto: : NAME: Missing name.\n");
}

TEST_F(ValidationErrorTest, InputTypeNotAMessage) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type { name: \"Foo\" } "
      "enum_type { name: \"Bar\" value { name:\"DUMMY\" number:0 } } "
      "service {"
      "  name: \"TestService\""
      "  method { name: \"A\" input_type: \"Bar\" output_type: \"Foo\" }"
      "}",

      "foo.proto: TestService.A: INPUT_TYPE: \"Bar\" is not a message type.\n");
}

TEST_F(ValidationErrorTest, OutputTypeNotDefined) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type { name: \"Foo\" } "
      "service {"
      "  name: \"TestService\""
      "  method { name: \"A\" input_type: \"Foo\" output_type: \"Bar\" }"
      "}",

      "foo.proto: TestService.A: OUTPUT_TYPE: \"Bar\" is not defined.\n");
}

TEST_F(ValidationErrorTest, OutputTypeNotAMessage) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type { name: \"Foo\" } "
      "enum_type { name: \"Bar\" value { name:\"DUMMY\" number:0 } } "
      "service {"
      "  name: \"TestService\""
      "  method { name: \"A\" input_type: \"Foo\" output_type: \"Bar\" }"
      "}",

      "foo.proto: TestService.A: OUTPUT_TYPE: \"Bar\" is not a message "
      "type.\n");
}


TEST_F(ValidationErrorTest, IllegalPackedField) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {\n"
      "  name: \"Foo\""
      "  field { name:\"packed_string\" number:1 label:LABEL_REPEATED "
      "          type:TYPE_STRING "
      "          options { uninterpreted_option {"
      "            name { name_part: \"packed\" is_extension: false }"
      "            identifier_value: \"true\" }}}\n"
      "  field { name:\"packed_message\" number:3 label:LABEL_REPEATED "
      "          type_name: \"Foo\""
      "          options { uninterpreted_option {"
      "            name { name_part: \"packed\" is_extension: false }"
      "            identifier_value: \"true\" }}}\n"
      "  field { name:\"optional_int32\" number: 4 label: LABEL_OPTIONAL "
      "          type:TYPE_INT32 "
      "          options { uninterpreted_option {"
      "            name { name_part: \"packed\" is_extension: false }"
      "            identifier_value: \"true\" }}}\n"
      "}",

      "foo.proto: Foo.packed_string: TYPE: [packed = true] can only be "
      "specified for repeated primitive fields.\n"
      "foo.proto: Foo.packed_message: TYPE: [packed = true] can only be "
      "specified for repeated primitive fields.\n"
      "foo.proto: Foo.optional_int32: TYPE: [packed = true] can only be "
      "specified for repeated primitive fields.\n");
}

TEST_F(ValidationErrorTest, OptionWrongType) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type { "
      "  name: \"TestMessage\" "
      "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type:TYPE_STRING "
      "          options { uninterpreted_option { name { name_part: \"ctype\" "
      "                                                  is_extension: false }"
      "                                           positive_int_value: 1 }"
      "          }"
      "  }"
      "}\n",

      "foo.proto: TestMessage.foo: OPTION_VALUE: Value must be identifier for "
      "enum-valued option \"google.protobuf.FieldOptions.ctype\".\n");
}

TEST_F(ValidationErrorTest, OptionExtendsAtomicType) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type { "
      "  name: \"TestMessage\" "
      "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type:TYPE_STRING "
      "          options { uninterpreted_option { name { name_part: \"ctype\" "
      "                                                  is_extension: false }"
      "                                           name { name_part: \"foo\" "
      "                                                  is_extension: true }"
      "                                           positive_int_value: 1 }"
      "          }"
      "  }"
      "}\n",

      "foo.proto: TestMessage.foo: OPTION_NAME: Option \"ctype\" is an "
      "atomic type, not a message.\n");
}

TEST_F(ValidationErrorTest, DupOption) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type { "
      "  name: \"TestMessage\" "
      "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type:TYPE_UINT32 "
      "          options { uninterpreted_option { name { name_part: \"ctype\" "
      "                                                  is_extension: false }"
      "                                           identifier_value: \"CORD\" }"
      "                    uninterpreted_option { name { name_part: \"ctype\" "
      "                                                  is_extension: false }"
      "                                           identifier_value: \"CORD\" }"
      "          }"
      "  }"
      "}\n",

      "foo.proto: TestMessage.foo: OPTION_NAME: Option \"ctype\" was "
      "already set.\n");
}

TEST_F(ValidationErrorTest, InvalidOptionName) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type { "
      "  name: \"TestMessage\" "
      "  field { name:\"foo\" number:1 label:LABEL_OPTIONAL type:TYPE_BOOL "
      "          options { uninterpreted_option { "
      "                      name { name_part: \"uninterpreted_option\" "
      "                             is_extension: false }"
      "                      positive_int_value: 1 "
      "                    }"
      "          }"
      "  }"
      "}\n",

      "foo.proto: TestMessage.foo: OPTION_NAME: Option must not use "
      "reserved name \"uninterpreted_option\".\n");
}

TEST_F(ValidationErrorTest, RepeatedMessageOption) {
  BuildDescriptorMessagesInTestPool();

  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "dependency: \"google/protobuf/descriptor.proto\" "
      "message_type: { name: \"Bar\" field: { "
      "  name: \"foo\" number: 1 label: LABEL_OPTIONAL type: TYPE_INT32 } "
      "} "
      "extension { name: \"bar\" number: 7672757 label: LABEL_REPEATED "
      "            type: TYPE_MESSAGE type_name: \"Bar\" "
      "            extendee: \"google.protobuf.FileOptions\" }"
      "options { uninterpreted_option { name { name_part: \"bar\" "
      "                                        is_extension: true } "
      "                                 name { name_part: \"foo\" "
      "                                        is_extension: false } "
      "                                 positive_int_value: 1 } }",

      "foo.proto: foo.proto: OPTION_NAME: Option field \"(bar)\" is a "
      "repeated message. Repeated message options must be initialized "
      "using an aggregate value.\n");
}

TEST_F(ValidationErrorTest, ResolveUndefinedOption) {
  // The following should produce an error that baz.bar is resolved but not
  // defined.
  // foo.proto:
  //   package baz
  //   import google/protobuf/descriptor.proto
  //   message Bar { optional int32 foo = 1; }
  //   extend FileOptions { optional Bar bar = 7672757; }
  //
  // moo.proto:
  //   package moo.baz
  //   option (baz.bar).foo = 1;
  //
  // Although "baz.bar" is already defined, the lookup code will try
  // "moo.baz.bar", since it's the match from the innermost scope, which will
  // cause a symbol not defined error.
  BuildDescriptorMessagesInTestPool();

  BuildFile(
      "name: \"foo.proto\" "
      "package: \"baz\" "
      "dependency: \"google/protobuf/descriptor.proto\" "
      "message_type: { name: \"Bar\" field: { "
      "  name: \"foo\" number: 1 label: LABEL_OPTIONAL type: TYPE_INT32 } "
      "} "
      "extension { name: \"bar\" number: 7672757 label: LABEL_OPTIONAL "
      "            type: TYPE_MESSAGE type_name: \"Bar\" "
      "            extendee: \"google.protobuf.FileOptions\" }");

  BuildFileWithErrors(
      "name: \"moo.proto\" "
      "package: \"moo.baz\" "
      "options { uninterpreted_option { name { name_part: \"baz.bar\" "
      "                                        is_extension: true } "
      "                                 name { name_part: \"foo\" "
      "                                        is_extension: false } "
      "                                 positive_int_value: 1 } }",

      "moo.proto: moo.proto: OPTION_NAME: Option \"(baz.bar)\" is resolved to "
      "\"(moo.baz.bar)\","
      " which is not defined. The innermost scope is searched first in name "
      "resolution. Consider using a leading '.'(i.e., \"(.baz.bar)\") to start "
      "from the outermost scope.\n");
}

TEST_F(ValidationErrorTest, UnknownOption) {
  BuildFileWithErrors(
      "name: \"moo.proto\" "
      "package: \"moo.baz\" "
      "options { uninterpreted_option { name { name_part: \"baaz.bar\" "
      "                                        is_extension: true } "
      "                                 name { name_part: \"foo\" "
      "                                        is_extension: false } "
      "                                 positive_int_value: 1 } }",

      "moo.proto: moo.proto: OPTION_NAME: Option \"(baaz.bar)\" unknown. "
      "Ensure "
      "that your proto definition file imports the proto which defines the "
      "option (i.e. via import option).\n");
}

TEST_F(ValidationErrorTest, CustomOptionConflictingFieldNumber) {
  BuildDescriptorMessagesInTestPool();

  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "dependency: \"google/protobuf/descriptor.proto\" "
      "extension { name: \"foo1\" number: 7672757 label: LABEL_OPTIONAL "
      "            type: TYPE_INT32 extendee: \"google.protobuf.FieldOptions\" }"
      "extension { name: \"foo2\" number: 7672757 label: LABEL_OPTIONAL "
      "            type: TYPE_INT32 extendee: \"google.protobuf.FieldOptions\" }",

      "foo.proto: foo2: NUMBER: Extension number 7672757 has already been used "
      "in \"google.protobuf.FieldOptions\" by extension \"foo1\".\n");
}

TEST_F(ValidationErrorTest, Int32OptionValueOutOfPositiveRange) {
  BuildDescriptorMessagesInTestPool();

  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "dependency: \"google/protobuf/descriptor.proto\" "
      "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
      "            type: TYPE_INT32 extendee: \"google.protobuf.FileOptions\" }"
      "options { uninterpreted_option { name { name_part: \"foo\" "
      "                                        is_extension: true } "
      "                                 positive_int_value: 0x80000000 } "
      "}",

      "foo.proto: foo.proto: OPTION_VALUE: Value out of range, -2147483648 to "
      "2147483647, for int32 option \"foo\".\n");
}

TEST_F(ValidationErrorTest, Int32OptionValueOutOfNegativeRange) {
  BuildDescriptorMessagesInTestPool();

  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "dependency: \"google/protobuf/descriptor.proto\" "
      "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
      "            type: TYPE_INT32 extendee: \"google.protobuf.FileOptions\" }"
      "options { uninterpreted_option { name { name_part: \"foo\" "
      "                                        is_extension: true } "
      "                                 negative_int_value: -0x80000001 } "
      "}",

      "foo.proto: foo.proto: OPTION_VALUE: Value out of range, -2147483648 to "
      "2147483647, for int32 option \"foo\".\n");
}

TEST_F(ValidationErrorTest, Int32OptionValueIsNotPositiveInt) {
  BuildDescriptorMessagesInTestPool();

  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "dependency: \"google/protobuf/descriptor.proto\" "
      "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
      "            type: TYPE_INT32 extendee: \"google.protobuf.FileOptions\" }"
      "options { uninterpreted_option { name { name_part: \"foo\" "
      "                                        is_extension: true } "
      "                                 string_value: \"5\" } }",

      "foo.proto: foo.proto: OPTION_VALUE: Value must be integer, from "
      "-2147483648 to 2147483647, for int32 option \"foo\".\n");
}

TEST_F(ValidationErrorTest, Int64OptionValueOutOfRange) {
  BuildDescriptorMessagesInTestPool();

  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "dependency: \"google/protobuf/descriptor.proto\" "
      "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
      "            type: TYPE_INT64 extendee: \"google.protobuf.FileOptions\" }"
      "options { uninterpreted_option { name { name_part: \"foo\" "
      "                                        is_extension: true } "
      "                                 positive_int_value: 0x8000000000000000 "
      "} "
      "}",

      "foo.proto: foo.proto: OPTION_VALUE: Value out of range, "
      "-9223372036854775808 to 9223372036854775807, for int64 option "
      "\"foo\".\n");
}

TEST_F(ValidationErrorTest, Int64OptionValueIsNotPositiveInt) {
  BuildDescriptorMessagesInTestPool();

  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "dependency: \"google/protobuf/descriptor.proto\" "
      "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
      "            type: TYPE_INT64 extendee: \"google.protobuf.FileOptions\" }"
      "options { uninterpreted_option { name { name_part: \"foo\" "
      "                                        is_extension: true } "
      "                                 identifier_value: \"5\" } }",

      "foo.proto: foo.proto: OPTION_VALUE: Value must be integer, from "
      "-9223372036854775808 to 9223372036854775807, for int64 option "
      "\"foo\".\n");
}

TEST_F(ValidationErrorTest, UInt32OptionValueOutOfRange) {
  BuildDescriptorMessagesInTestPool();

  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "dependency: \"google/protobuf/descriptor.proto\" "
      "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
      "            type: TYPE_UINT32 extendee: \"google.protobuf.FileOptions\" }"
      "options { uninterpreted_option { name { name_part: \"foo\" "
      "                                        is_extension: true } "
      "                                 positive_int_value: 0x100000000 } }",

      "foo.proto: foo.proto: OPTION_VALUE: Value out of range, 0 to "
      "4294967295, for uint32 option \"foo\".\n");
}

TEST_F(ValidationErrorTest, UInt32OptionValueIsNotPositiveInt) {
  BuildDescriptorMessagesInTestPool();

  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "dependency: \"google/protobuf/descriptor.proto\" "
      "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
      "            type: TYPE_UINT32 extendee: \"google.protobuf.FileOptions\" }"
      "options { uninterpreted_option { name { name_part: \"foo\" "
      "                                        is_extension: true } "
      "                                 double_value: -5.6 } }",

      "foo.proto: foo.proto: OPTION_VALUE: Value must be integer, from 0 to "
      "4294967295, for uint32 option \"foo\".\n");
}

TEST_F(ValidationErrorTest, UInt64OptionValueIsNotPositiveInt) {
  BuildDescriptorMessagesInTestPool();

  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "dependency: \"google/protobuf/descriptor.proto\" "
      "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
      "            type: TYPE_UINT64 extendee: \"google.protobuf.FileOptions\" }"
      "options { uninterpreted_option { name { name_part: \"foo\" "
      "                                        is_extension: true } "
      "                                 negative_int_value: -5 } }",

      "foo.proto: foo.proto: OPTION_VALUE: Value must be integer, from 0 to "
      "18446744073709551615, for uint64 option \"foo\".\n");
}

TEST_F(ValidationErrorTest, FloatOptionValueIsNotNumber) {
  BuildDescriptorMessagesInTestPool();

  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "dependency: \"google/protobuf/descriptor.proto\" "
      "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
      "            type: TYPE_FLOAT extendee: \"google.protobuf.FileOptions\" }"
      "options { uninterpreted_option { name { name_part: \"foo\" "
      "                                        is_extension: true } "
      "                                 string_value: \"bar\" } }",

      "foo.proto: foo.proto: OPTION_VALUE: Value must be number "
      "for float option \"foo\".\n");
}

TEST_F(ValidationErrorTest, DoubleOptionValueIsNotNumber) {
  BuildDescriptorMessagesInTestPool();

  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "dependency: \"google/protobuf/descriptor.proto\" "
      "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
      "            type: TYPE_DOUBLE extendee: \"google.protobuf.FileOptions\" }"
      "options { uninterpreted_option { name { name_part: \"foo\" "
      "                                        is_extension: true } "
      "                                 string_value: \"bar\" } }",

      "foo.proto: foo.proto: OPTION_VALUE: Value must be number "
      "for double option \"foo\".\n");
}

TEST_F(ValidationErrorTest, BoolOptionValueIsNotTrueOrFalse) {
  BuildDescriptorMessagesInTestPool();

  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "dependency: \"google/protobuf/descriptor.proto\" "
      "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
      "            type: TYPE_BOOL extendee: \"google.protobuf.FileOptions\" }"
      "options { uninterpreted_option { name { name_part: \"foo\" "
      "                                        is_extension: true } "
      "                                 identifier_value: \"bar\" } }",

      "foo.proto: foo.proto: OPTION_VALUE: Value must be \"true\" or \"false\" "
      "for boolean option \"foo\".\n");
}

TEST_F(ValidationErrorTest, EnumOptionValueIsNotIdentifier) {
  BuildDescriptorMessagesInTestPool();

  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "dependency: \"google/protobuf/descriptor.proto\" "
      "enum_type { name: \"FooEnum\" value { name: \"BAR\" number: 1 } "
      "                              value { name: \"BAZ\" number: 2 } }"
      "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
      "            type: TYPE_ENUM type_name: \"FooEnum\" "
      "            extendee: \"google.protobuf.FileOptions\" }"
      "options { uninterpreted_option { name { name_part: \"foo\" "
      "                                        is_extension: true } "
      "                                 string_value: \"MOOO\" } }",

      "foo.proto: foo.proto: OPTION_VALUE: Value must be identifier for "
      "enum-valued option \"foo\".\n");
}

TEST_F(ValidationErrorTest, EnumOptionValueIsNotEnumValueName) {
  BuildDescriptorMessagesInTestPool();

  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "dependency: \"google/protobuf/descriptor.proto\" "
      "enum_type { name: \"FooEnum\" value { name: \"BAR\" number: 1 } "
      "                              value { name: \"BAZ\" number: 2 } }"
      "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
      "            type: TYPE_ENUM type_name: \"FooEnum\" "
      "            extendee: \"google.protobuf.FileOptions\" }"
      "options { uninterpreted_option { name { name_part: \"foo\" "
      "                                        is_extension: true } "
      "                                 identifier_value: \"MOOO\" } }",

      "foo.proto: foo.proto: OPTION_VALUE: Enum type \"FooEnum\" has no value "
      "named \"MOOO\" for option \"foo\".\n");
}

TEST_F(ValidationErrorTest, EnumOptionValueIsSiblingEnumValueName) {
  BuildDescriptorMessagesInTestPool();

  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "dependency: \"google/protobuf/descriptor.proto\" "
      "enum_type { name: \"FooEnum1\" value { name: \"BAR\" number: 1 } "
      "                               value { name: \"BAZ\" number: 2 } }"
      "enum_type { name: \"FooEnum2\" value { name: \"MOO\" number: 1 } "
      "                               value { name: \"MOOO\" number: 2 } }"
      "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
      "            type: TYPE_ENUM type_name: \"FooEnum1\" "
      "            extendee: \"google.protobuf.FileOptions\" }"
      "options { uninterpreted_option { name { name_part: \"foo\" "
      "                                        is_extension: true } "
      "                                 identifier_value: \"MOOO\" } }",

      "foo.proto: foo.proto: OPTION_VALUE: Enum type \"FooEnum1\" has no value "
      "named \"MOOO\" for option \"foo\". This appears to be a value from a "
      "sibling type.\n");
}

TEST_F(ValidationErrorTest, StringOptionValueIsNotString) {
  BuildDescriptorMessagesInTestPool();

  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "dependency: \"google/protobuf/descriptor.proto\" "
      "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
      "            type: TYPE_STRING extendee: \"google.protobuf.FileOptions\" }"
      "options { uninterpreted_option { name { name_part: \"foo\" "
      "                                        is_extension: true } "
      "                                 identifier_value: \"MOOO\" } }",

      "foo.proto: foo.proto: OPTION_VALUE: Value must be quoted string "
      "for string option \"foo\".\n");
}

TEST_F(ValidationErrorTest, JsonNameOptionOnExtensions) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "package: \"foo\" "
      "message_type {"
      "  name: \"Foo\""
      "  extension_range { start: 10 end: 20 }"
      "}"
      "extension {"
      "  name: \"value\""
      "  number: 10"
      "  label: LABEL_OPTIONAL"
      "  type: TYPE_INT32"
      "  extendee: \"foo.Foo\""
      "  json_name: \"myName\""
      "}",
      "foo.proto: foo.value: OPTION_NAME: option json_name is not allowed on "
      "extension fields.\n");
}

TEST_F(ValidationErrorTest, JsonNameEmbeddedNull) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "package: \"foo\" "
      "message_type {"
      "  name: \"Foo\""
      "  field {"
      "    name: \"value\""
      "    number: 10"
      "    label: LABEL_OPTIONAL"
      "    type: TYPE_INT32"
      "    json_name: \"embedded\\000null\""
      "  }"
      "}",
      "foo.proto: foo.Foo.value: OPTION_NAME: json_name cannot have embedded "
      "null characters.\n");
}

void TestNameSizeLimit(const FileDescriptorProto& file, std::string& name,
                       absl::string_view element) {
  const std::string orig = name;

  // The exact threshold is not fixed, so find it.
  size_t success = 10;
  size_t fail = 70000;

  const auto test = [&](size_t size) {
    name.assign(size, 'x');
    MockErrorCollector error_collector;
    DescriptorPool pool;
    auto* out = pool.BuildFileCollectingErrors(file, &error_collector);
    if (out == nullptr) {
      EXPECT_THAT(error_collector.text_, HasSubstr("too long")) << element;
    }
    return out != nullptr;
  };

  while (fail - success > 1) {
    size_t mid = (fail + success) / 2;
    if (test(mid)) {
      success = mid;
    } else {
      fail = mid;
    }
  }

  ABSL_LOG(INFO) << "First failure on " << fail << " for " << element;
  for (size_t i = success - 5; i <= success; ++i) {
    EXPECT_TRUE(test(i)) << element;
  }
  for (size_t i = fail; i <= fail + 5; ++i) {
    EXPECT_FALSE(test(i)) << element;
  }

  // Reset the name for the next test.
  name = orig;
}

TEST_F(ValidationErrorTest, TooLongNamesCauseABuildError) {
  FileDescriptorProto file_proto;
  ASSERT_TRUE(TextFormat::ParseFromString(
      R"pb(
        name: "foo.proto"
        message_type {
          name: "Foo"
          field { name: "name" number: 1 type: TYPE_STRING }
        }
      )pb",
      &file_proto));

  // Grow package.
  TestNameSizeLimit(file_proto, *file_proto.mutable_package(), "package");

  // Grow message name.
  TestNameSizeLimit(file_proto,
                    *file_proto.mutable_message_type(0)->mutable_name(),
                    "message");

  // Grow field name.
  TestNameSizeLimit(
      file_proto,
      *file_proto.mutable_message_type(0)->mutable_field(0)->mutable_name(),
      "field");

  // Grow field json_name.
  TestNameSizeLimit(file_proto,
                    *file_proto.mutable_message_type(0)
                         ->mutable_field(0)
                         ->mutable_json_name(),
                    "json_name");
}

TEST_F(ValidationErrorTest, DuplicateExtensionFieldNumber) {
  BuildDescriptorMessagesInTestPool();

  BuildFile(
      "name: \"foo.proto\" "
      "dependency: \"google/protobuf/descriptor.proto\" "
      "extension { name: \"option1\" number: 1000 label: LABEL_OPTIONAL "
      "            type: TYPE_INT32 extendee: \"google.protobuf.FileOptions\" }");

  BuildFileWithWarnings(
      "name: \"bar.proto\" "
      "dependency: \"google/protobuf/descriptor.proto\" "
      "extension { name: \"option2\" number: 1000 label: LABEL_OPTIONAL "
      "            type: TYPE_INT32 extendee: \"google.protobuf.FileOptions\" }",
      "bar.proto: option2: NUMBER: Extension number 1000 has already been used "
      "in \"google.protobuf.FileOptions\" by extension \"option1\" defined in "
      "foo.proto.\n");
}

// Helper function for tests that check for aggregate value parsing
// errors.  The "value" argument is embedded inside the
// "uninterpreted_option" portion of the result.
static std::string EmbedAggregateValue(const char* value) {
  return absl::Substitute(
      "name: \"foo.proto\" "
      "dependency: \"google/protobuf/descriptor.proto\" "
      "message_type { name: \"Foo\" } "
      "extension { name: \"foo\" number: 7672757 label: LABEL_OPTIONAL "
      "            type: TYPE_MESSAGE type_name: \"Foo\" "
      "            extendee: \"google.protobuf.FileOptions\" }"
      "options { uninterpreted_option { name { name_part: \"foo\" "
      "                                        is_extension: true } "
      "                                 $0 } }",
      value);
}

TEST_F(ValidationErrorTest, AggregateValueNotFound) {
  BuildDescriptorMessagesInTestPool();

  BuildFileWithErrors(
      EmbedAggregateValue("string_value: \"\""),
      "foo.proto: foo.proto: OPTION_VALUE: Option \"foo\" is a message. "
      "To set the entire message, use syntax like "
      "\"foo = { <proto text format> }\". To set fields within it, use "
      "syntax like \"foo.foo = value\".\n");
}

TEST_F(ValidationErrorTest, AggregateValueParseError) {
  BuildDescriptorMessagesInTestPool();

  BuildFileWithErrors(
      EmbedAggregateValue("aggregate_value: \"1+2\""),
      "foo.proto: foo.proto: OPTION_VALUE: Error while parsing option "
      "value for \"foo\": Expected identifier, got: 1\n");
}

TEST_F(ValidationErrorTest, AggregateValueUnknownFields) {
  BuildDescriptorMessagesInTestPool();

  BuildFileWithErrors(
      EmbedAggregateValue("aggregate_value: \"x:100\""),
      "foo.proto: foo.proto: OPTION_VALUE: Error while parsing option "
      "value for \"foo\": Message type \"Foo\" has no field named \"x\".\n");
}

TEST_F(ValidationErrorTest, NotLiteImportsLite) {
  BuildFile(
      "name: \"bar.proto\" "
      "options { optimize_for: LITE_RUNTIME } ");

  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "dependency: \"bar.proto\" ",

      "foo.proto: bar.proto: IMPORT: Files that do not use optimize_for = "
      "LITE_RUNTIME cannot import files which do use this option.  This file "
      "is not lite, but it imports \"bar.proto\" which is.\n");
}

TEST_F(ValidationErrorTest, LiteExtendsNotLite) {
  BuildFile(
      "name: \"bar.proto\" "
      "message_type: {"
      "  name: \"Bar\""
      "  extension_range { start: 1 end: 1000 }"
      "}");

  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "dependency: \"bar.proto\" "
      "options { optimize_for: LITE_RUNTIME } "
      "extension { name: \"ext\" number: 123 label: LABEL_OPTIONAL "
      "            type: TYPE_INT32 extendee: \"Bar\" }",

      "foo.proto: ext: EXTENDEE: Extensions to non-lite types can only be "
      "declared in non-lite files.  Note that you cannot extend a non-lite "
      "type to contain a lite type, but the reverse is allowed.\n");
}

TEST_F(ValidationErrorTest, NoLiteServices) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "options {"
      "  optimize_for: LITE_RUNTIME"
      "  cc_generic_services: true"
      "  java_generic_services: true"
      "} "
      "service { name: \"Foo\" }",

      "foo.proto: Foo: NAME: Files with optimize_for = LITE_RUNTIME cannot "
      "define services unless you set both options cc_generic_services and "
      "java_generic_services to false.\n");

  BuildFile(
      "name: \"bar.proto\" "
      "options {"
      "  optimize_for: LITE_RUNTIME"
      "  cc_generic_services: false"
      "  java_generic_services: false"
      "} "
      "service { name: \"Bar\" }");
}

TEST_F(ValidationErrorTest, RollbackAfterError) {
  // Build a file which contains every kind of construct but references an
  // undefined type.  All these constructs will be added to the symbol table
  // before the undefined type error is noticed.  The DescriptorPool will then
  // have to roll everything back.
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"TestMessage\""
      "  field { name:\"foo\" label:LABEL_OPTIONAL type:TYPE_INT32 number:1 }"
      "} "
      "enum_type {"
      "  name: \"TestEnum\""
      "  value { name:\"BAR\" number:1 }"
      "} "
      "service {"
      "  name: \"TestService\""
      "  method {"
      "    name: \"Baz\""
      "    input_type: \"NoSuchType\""  // error
      "    output_type: \"TestMessage\""
      "  }"
      "}",

      "foo.proto: TestService.Baz: INPUT_TYPE: \"NoSuchType\" is not "
      "defined.\n");

  // Make sure that if we build the same file again with the error fixed,
  // it works.  If the above rollback was incomplete, then some symbols will
  // be left defined, and this second attempt will fail since it tries to
  // re-define the same symbols.
  BuildFile(
      "name: \"foo.proto\" "
      "message_type {"
      "  name: \"TestMessage\""
      "  field { name:\"foo\" label:LABEL_OPTIONAL type:TYPE_INT32 number:1 }"
      "} "
      "enum_type {"
      "  name: \"TestEnum\""
      "  value { name:\"BAR\" number:1 }"
      "} "
      "service {"
      "  name: \"TestService\""
      "  method { name:\"Baz\""
      "           input_type:\"TestMessage\""
      "           output_type:\"TestMessage\" }"
      "}");
}

TEST_F(ValidationErrorTest, ErrorsReportedToLogError) {
  // Test that errors are reported to ABSL_LOG(ERROR) if no error collector is
  // provided.

  FileDescriptorProto file_proto;
  ASSERT_TRUE(
      TextFormat::ParseFromString("name: \"foo.proto\" "
                                  "message_type { name: \"Foo\" } "
                                  "message_type { name: \"Foo\" } ",
                                  &file_proto));
  {
    absl::ScopedMockLog log(absl::MockLogDefault::kDisallowUnexpected);
    EXPECT_CALL(log, Log(absl::LogSeverity::kError, testing::_,
                         "Invalid proto descriptor for file \"foo.proto\":"));
    EXPECT_CALL(log, Log(absl::LogSeverity::kError, testing::_,
                         "  Foo: \"Foo\" is already defined."));
    log.StartCapturingLogs();
    EXPECT_TRUE(pool_.BuildFile(file_proto) == nullptr);
  }
}

TEST_F(ValidationErrorTest, DisallowEnumAlias) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "enum_type {"
      "  name: \"Bar\""
      "  value { name:\"ENUM_A\" number:0 }"
      "  value { name:\"ENUM_B\" number:0 }"
      "}",
      "foo.proto: Bar: NUMBER: "
      "\"ENUM_B\" uses the same enum value as \"ENUM_A\". "
      "If this is intended, set 'option allow_alias = true;' to the enum "
      "definition. The next available enum value is 1.\n");

  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        enum_type {
          name: "Bar"
          value { name: "ENUM_A" number: 10 }
          value { name: "ENUM_B" number: 10 }
          value { name: "ENUM_C" number: 11 }
          value { name: "ENUM_D" number: 20 }
        })pb",
      "foo.proto: Bar: NUMBER: "
      "\"ENUM_B\" uses the same enum value as \"ENUM_A\". "
      "If this is intended, set 'option allow_alias = true;' to the enum "
      "definition. The next available enum value is 12.\n");

  BuildFileWithErrors(
      absl::Substitute(R"pb(
                         name: "foo.proto"
                         enum_type {
                           name: "Bar"
                           value { name: "ENUM_A" number: $0 }
                           value { name: "ENUM_B" number: $0 }
                         })pb",
                       std::numeric_limits<int32_t>::max()),
      "foo.proto: Bar: NUMBER: "
      "\"ENUM_B\" uses the same enum value as \"ENUM_A\". "
      "If this is intended, set 'option allow_alias = true;' to the enum "
      "definition.\n");
}

TEST_F(ValidationErrorTest, AllowEnumAlias) {
  BuildFile(
      "name: \"foo.proto\" "
      "enum_type {"
      "  name: \"Bar\""
      "  value { name:\"ENUM_A\" number:0 }"
      "  value { name:\"ENUM_B\" number:0 }"
      "  options { allow_alias: true }"
      "}");
}

TEST_F(ValidationErrorTest, UnusedImportWarning) {
  pool_.AddDirectInputFile("bar.proto");
  BuildFile(
      "name: \"bar.proto\" "
      "message_type { name: \"Bar\" }");

  pool_.AddDirectInputFile("base.proto");
  BuildFile(
      "name: \"base.proto\" "
      "message_type { name: \"Base\" }");

  pool_.AddDirectInputFile("baz.proto");
  BuildFile(
      "name: \"baz.proto\" "
      "message_type { name: \"Baz\" }");

  pool_.AddDirectInputFile("public.proto");
  BuildFile(
      "name: \"public.proto\" "
      "dependency: \"bar.proto\""
      "public_dependency: 0");

  // // forward.proto
  // import "base.proto"       // No warning: Base message is used.
  // import "bar.proto"        // Will log a warning.
  // import public "baz.proto" // No warning: Do not track import public.
  // import "public.proto"     // No warning: public.proto has import public.
  // message Forward {
  //   optional Base base = 1;
  // }
  //
  pool_.AddDirectInputFile("forward.proto");
  BuildFileWithWarnings(
      "name: \"forward.proto\""
      "dependency: \"base.proto\""
      "dependency: \"bar.proto\""
      "dependency: \"baz.proto\""
      "dependency: \"public.proto\""
      "public_dependency: 2 "
      "message_type {"
      "  name: \"Forward\""
      "  field { name:\"base\" number:1 label:LABEL_OPTIONAL "
      "type_name:\"Base\" }"
      "}",
      "forward.proto: bar.proto: IMPORT: Import bar.proto is unused.\n");
}

// Verifies that the dependency checker isn't fooled by package symbols,
// which can be defined in multiple files.
TEST_F(ValidationErrorTest, SamePackageUnusedImportError) {
  BuildFile(R"pb(
    name: "unused_dependency.proto"
    package: "proto2_unittest.subpackage"
    message_type { name: "Foo" }
  )pb");

  BuildFile(R"pb(
    name: "used_dependency.proto"
    package: "proto2_unittest.subpackage"
    message_type { name: "Bar" }
  )pb");

  pool_.AddDirectInputFile("import.proto", true);
  BuildFileWithErrors(R"pb(
                        name: "import.proto"
                        package: "proto2_unittest"
                        dependency: "unused_dependency.proto"
                        dependency: "used_dependency.proto"
                        message_type {
                          name: "Baz"
                          field {
                            name: "bar"
                            number: 1
                            label: LABEL_OPTIONAL
                            type: TYPE_MESSAGE
                            type_name: "subpackage.Bar"
                          }
                        }
                      )pb",
                      "import.proto: unused_dependency.proto: "
                      "IMPORT: Import unused_dependency.proto is unused.\n");
}

namespace {
void FillValidMapEntry(FileDescriptorProto* file_proto) {
  ASSERT_TRUE(TextFormat::ParseFromString(
      "name: 'foo.proto' "
      "message_type { "
      "  name: 'Foo' "
      "  field { "
      "    name: 'foo_map' number: 1 label:LABEL_REPEATED "
      "    type_name: 'FooMapEntry' "
      "  } "
      "  nested_type { "
      "    name: 'FooMapEntry' "
      "    options {  map_entry: true } "
      "    field { "
      "      name: 'key' number: 1 type:TYPE_INT32 label:LABEL_OPTIONAL "
      "    } "
      "    field { "
      "      name: 'value' number: 2 type:TYPE_INT32 label:LABEL_OPTIONAL "
      "    } "
      "  } "
      "} "
      "message_type { "
      "  name: 'Bar' "
      "  extension_range { start: 1 end: 10 }"
      "} ",
      file_proto));
}
static const char* kMapEntryErrorMessage =
    "foo.proto: Foo.foo_map: TYPE: map_entry should not be set explicitly. "
    "Use map<KeyType, ValueType> instead.\n";
static const char* kMapEntryKeyTypeErrorMessage =
    "foo.proto: Foo.foo_map: TYPE: Key in map fields cannot be float/double, "
    "bytes or message types.\n";

}  // namespace

TEST_F(ValidationErrorTest, MapEntryBase) {
  FileDescriptorProto file_proto;
  FillValidMapEntry(&file_proto);
  std::string text_proto;
  TextFormat::PrintToString(file_proto, &text_proto);
  BuildFile(text_proto);
}

TEST_F(ValidationErrorTest, MapEntryExtensionRange) {
  FileDescriptorProto file_proto;
  FillValidMapEntry(&file_proto);
  TextFormat::MergeFromString(
      "extension_range { "
      "  start: 10 end: 20 "
      "} ",
      file_proto.mutable_message_type(0)->mutable_nested_type(0));
  BuildFileWithErrors(file_proto, kMapEntryErrorMessage);
}

TEST_F(ValidationErrorTest, MapEntryExtension) {
  FileDescriptorProto file_proto;
  FillValidMapEntry(&file_proto);
  TextFormat::MergeFromString(
      "extension { "
      "  name: 'foo_ext' extendee: '.Bar' number: 5"
      "} ",
      file_proto.mutable_message_type(0)->mutable_nested_type(0));
  BuildFileWithErrors(file_proto, kMapEntryErrorMessage);
}

TEST_F(ValidationErrorTest, MapEntryNestedType) {
  FileDescriptorProto file_proto;
  FillValidMapEntry(&file_proto);
  TextFormat::MergeFromString(
      "nested_type { "
      "  name: 'Bar' "
      "} ",
      file_proto.mutable_message_type(0)->mutable_nested_type(0));
  BuildFileWithErrors(file_proto, kMapEntryErrorMessage);
}

TEST_F(ValidationErrorTest, MapEntryEnumTypes) {
  FileDescriptorProto file_proto;
  FillValidMapEntry(&file_proto);
  TextFormat::MergeFromString(
      "enum_type { "
      "  name: 'BarEnum' "
      "  value { name: 'BAR_BAR' number:0 } "
      "} ",
      file_proto.mutable_message_type(0)->mutable_nested_type(0));
  BuildFileWithErrors(file_proto, kMapEntryErrorMessage);
}

TEST_F(ValidationErrorTest, MapEntryExtraField) {
  FileDescriptorProto file_proto;
  FillValidMapEntry(&file_proto);
  TextFormat::MergeFromString(
      "field { "
      "  name: 'other_field' "
      "  label: LABEL_OPTIONAL "
      "  type: TYPE_INT32 "
      "  number: 3 "
      "} ",
      file_proto.mutable_message_type(0)->mutable_nested_type(0));
  BuildFileWithErrors(file_proto, kMapEntryErrorMessage);
}

TEST_F(ValidationErrorTest, MapEntryMessageName) {
  FileDescriptorProto file_proto;
  FillValidMapEntry(&file_proto);
  file_proto.mutable_message_type(0)->mutable_nested_type(0)->set_name(
      "OtherMapEntry");
  file_proto.mutable_message_type(0)->mutable_field(0)->set_type_name(
      "OtherMapEntry");
  BuildFileWithErrors(file_proto, kMapEntryErrorMessage);
}

TEST_F(ValidationErrorTest, MapEntryNoneRepeatedMapEntry) {
  FileDescriptorProto file_proto;
  FillValidMapEntry(&file_proto);
  file_proto.mutable_message_type(0)->mutable_field(0)->set_label(
      FieldDescriptorProto::LABEL_OPTIONAL);
  BuildFileWithErrors(file_proto, kMapEntryErrorMessage);
}

TEST_F(ValidationErrorTest, MapEntryDifferentContainingType) {
  FileDescriptorProto file_proto;
  FillValidMapEntry(&file_proto);
  // Move the nested MapEntry message into the top level, which should not pass
  // the validation.
  file_proto.mutable_message_type()->AddAllocated(
      file_proto.mutable_message_type(0)->mutable_nested_type()->ReleaseLast());
  BuildFileWithErrors(file_proto, kMapEntryErrorMessage);
}

TEST_F(ValidationErrorTest, MapEntryKeyName) {
  FileDescriptorProto file_proto;
  FillValidMapEntry(&file_proto);
  FieldDescriptorProto* key =
      file_proto.mutable_message_type(0)->mutable_nested_type(0)->mutable_field(
          0);
  key->set_name("Key");
  BuildFileWithErrors(file_proto, kMapEntryErrorMessage);
}

TEST_F(ValidationErrorTest, MapEntryKeyLabel) {
  FileDescriptorProto file_proto;
  FillValidMapEntry(&file_proto);
  FieldDescriptorProto* key =
      file_proto.mutable_message_type(0)->mutable_nested_type(0)->mutable_field(
          0);
  key->set_label(FieldDescriptorProto::LABEL_REQUIRED);
  BuildFileWithErrors(file_proto, kMapEntryErrorMessage);
}

TEST_F(ValidationErrorTest, MapEntryKeyNumber) {
  FileDescriptorProto file_proto;
  FillValidMapEntry(&file_proto);
  FieldDescriptorProto* key =
      file_proto.mutable_message_type(0)->mutable_nested_type(0)->mutable_field(
          0);
  key->set_number(3);
  BuildFileWithErrors(file_proto, kMapEntryErrorMessage);
}

TEST_F(ValidationErrorTest, MapEntryValueName) {
  FileDescriptorProto file_proto;
  FillValidMapEntry(&file_proto);
  FieldDescriptorProto* value =
      file_proto.mutable_message_type(0)->mutable_nested_type(0)->mutable_field(
          1);
  value->set_name("Value");
  BuildFileWithErrors(file_proto, kMapEntryErrorMessage);
}

TEST_F(ValidationErrorTest, MapEntryValueLabel) {
  FileDescriptorProto file_proto;
  FillValidMapEntry(&file_proto);
  FieldDescriptorProto* value =
      file_proto.mutable_message_type(0)->mutable_nested_type(0)->mutable_field(
          1);
  value->set_label(FieldDescriptorProto::LABEL_REQUIRED);
  BuildFileWithErrors(file_proto, kMapEntryErrorMessage);
}

TEST_F(ValidationErrorTest, MapEntryValueNumber) {
  FileDescriptorProto file_proto;
  FillValidMapEntry(&file_proto);
  FieldDescriptorProto* value =
      file_proto.mutable_message_type(0)->mutable_nested_type(0)->mutable_field(
          1);
  value->set_number(3);
  BuildFileWithErrors(file_proto, kMapEntryErrorMessage);
}

TEST_F(ValidationErrorTest, MapEntryKeyTypeFloat) {
  FileDescriptorProto file_proto;
  FillValidMapEntry(&file_proto);
  FieldDescriptorProto* key =
      file_proto.mutable_message_type(0)->mutable_nested_type(0)->mutable_field(
          0);
  key->set_type(FieldDescriptorProto::TYPE_FLOAT);
  BuildFileWithErrors(file_proto, kMapEntryKeyTypeErrorMessage);
}

TEST_F(ValidationErrorTest, MapEntryKeyTypeDouble) {
  FileDescriptorProto file_proto;
  FillValidMapEntry(&file_proto);
  FieldDescriptorProto* key =
      file_proto.mutable_message_type(0)->mutable_nested_type(0)->mutable_field(
          0);
  key->set_type(FieldDescriptorProto::TYPE_DOUBLE);
  BuildFileWithErrors(file_proto, kMapEntryKeyTypeErrorMessage);
}

TEST_F(ValidationErrorTest, MapEntryKeyTypeBytes) {
  FileDescriptorProto file_proto;
  FillValidMapEntry(&file_proto);
  FieldDescriptorProto* key =
      file_proto.mutable_message_type(0)->mutable_nested_type(0)->mutable_field(
          0);
  key->set_type(FieldDescriptorProto::TYPE_BYTES);
  BuildFileWithErrors(file_proto, kMapEntryKeyTypeErrorMessage);
}

TEST_F(ValidationErrorTest, MapEntryKeyTypeEnum) {
  FileDescriptorProto file_proto;
  FillValidMapEntry(&file_proto);
  FieldDescriptorProto* key =
      file_proto.mutable_message_type(0)->mutable_nested_type(0)->mutable_field(
          0);
  key->clear_type();
  key->set_type_name("BarEnum");
  EnumDescriptorProto* enum_proto = file_proto.add_enum_type();
  enum_proto->set_name("BarEnum");
  EnumValueDescriptorProto* enum_value_proto = enum_proto->add_value();
  enum_value_proto->set_name("BAR_VALUE0");
  enum_value_proto->set_number(0);
  BuildFileWithErrors(file_proto,
                      "foo.proto: Foo.foo_map: TYPE: Key in map fields cannot "
                      "be enum types.\n");
  // Enum keys are not allowed in proto3 as well.
  // Get rid of extensions for proto3 to make it proto3 compatible.
  file_proto.mutable_message_type()->RemoveLast();
  file_proto.set_syntax("proto3");
  BuildFileWithErrors(file_proto,
                      "foo.proto: Foo.foo_map: TYPE: Key in map fields cannot "
                      "be enum types.\n");
}

TEST_F(ValidationErrorTest, MapEntryKeyTypeMessage) {
  FileDescriptorProto file_proto;
  FillValidMapEntry(&file_proto);
  FieldDescriptorProto* key =
      file_proto.mutable_message_type(0)->mutable_nested_type(0)->mutable_field(
          0);
  key->clear_type();
  key->set_type_name(".Bar");
  BuildFileWithErrors(file_proto, kMapEntryKeyTypeErrorMessage);
}

TEST_F(ValidationErrorTest, MapEntryConflictsWithField) {
  FileDescriptorProto file_proto;
  FillValidMapEntry(&file_proto);
  TextFormat::MergeFromString(
      "field { "
      "  name: 'FooMapEntry' "
      "  type: TYPE_INT32 "
      "  label: LABEL_OPTIONAL "
      "  number: 100 "
      "}",
      file_proto.mutable_message_type(0));
  BuildFileWithErrors(
      file_proto,
      "foo.proto: Foo.FooMapEntry: NAME: \"FooMapEntry\" is already defined in "
      "\"Foo\".\n"
      "foo.proto: Foo.foo_map: TYPE: \"FooMapEntry\" is not defined.\n"
      "foo.proto: Foo: NAME: Expanded map entry type FooMapEntry conflicts "
      "with an existing field.\n");
}

TEST_F(ValidationErrorTest, MapEntryConflictsWithMessage) {
  FileDescriptorProto file_proto;
  FillValidMapEntry(&file_proto);
  TextFormat::MergeFromString(
      "nested_type { "
      "  name: 'FooMapEntry' "
      "}",
      file_proto.mutable_message_type(0));
  BuildFileWithErrors(
      file_proto,
      "foo.proto: Foo.FooMapEntry: NAME: \"FooMapEntry\" is already defined in "
      "\"Foo\".\n"
      "foo.proto: Foo: NAME: Expanded map entry type FooMapEntry conflicts "
      "with an existing nested message type.\n");
}

TEST_F(ValidationErrorTest, MapEntryConflictsWithEnum) {
  FileDescriptorProto file_proto;
  FillValidMapEntry(&file_proto);
  TextFormat::MergeFromString(
      "enum_type { "
      "  name: 'FooMapEntry' "
      "  value { name: 'ENTRY_FOO' number: 0 }"
      "}",
      file_proto.mutable_message_type(0));
  BuildFileWithErrors(
      file_proto,
      "foo.proto: Foo.FooMapEntry: NAME: \"FooMapEntry\" is already defined in "
      "\"Foo\".\n"
      "foo.proto: Foo: NAME: Expanded map entry type FooMapEntry conflicts "
      "with an existing enum type.\n");
}

TEST_F(ValidationErrorTest, Proto3EnumValuesConflictWithDifferentCasing) {
  BuildFileWithErrors(
      "syntax: 'proto3'"
      "name: 'foo.proto' "
      "enum_type {"
      "  name: 'FooEnum' "
      "  value { name: 'BAR' number: 0 }"
      "  value { name: 'bar' number: 1 }"
      "}",
      "foo.proto: bar: NAME: Enum name bar has the same name as BAR "
      "if you ignore case and strip out the enum name prefix (if any). "
      "(If you are using allow_alias, please assign the same number "
      "to each enum value name.)\n");

  BuildFileWithErrors(
      "syntax: 'proto2'"
      "name: 'foo.proto' "
      "enum_type {"
      "  name: 'FooEnum' "
      "  value { name: 'BAR' number: 0 }"
      "  value { name: 'bar' number: 1 }"
      "}",
      "foo.proto: bar: NAME: Enum name bar has the same name as BAR "
      "if you ignore case and strip out the enum name prefix (if any). "
      "(If you are using allow_alias, please assign the same number "
      "to each enum value name.)\n");

  // Not an error because both enums are mapped to the same value.
  BuildFile(
      "syntax: 'proto3'"
      "name: 'foo.proto' "
      "enum_type {"
      "  name: 'FooEnum' "
      "  options { allow_alias: true }"
      "  value { name: 'UNKNOWN' number: 0 }"
      "  value { name: 'BAR' number: 1 }"
      "  value { name: 'bar' number: 1 }"
      "}");
}

TEST_F(ValidationErrorTest, EnumValuesConflictWhenPrefixesStripped) {
  BuildFileWithErrors(
      "syntax: 'proto3'"
      "name: 'foo.proto' "
      "enum_type {"
      "  name: 'FooEnum' "
      "  value { name: 'FOO_ENUM_BAZ' number: 0 }"
      "  value { name: 'BAZ' number: 1 }"
      "}",
      "foo.proto: BAZ: NAME: Enum name BAZ has the same name as FOO_ENUM_BAZ "
      "if you ignore case and strip out the enum name prefix (if any). "
      "(If you are using allow_alias, please assign the same number "
      "to each enum value name.)\n");

  BuildFileWithErrors(
      "syntax: 'proto3'"
      "name: 'foo.proto' "
      "enum_type {"
      "  name: 'FooEnum' "
      "  value { name: 'FOOENUM_BAZ' number: 0 }"
      "  value { name: 'BAZ' number: 1 }"
      "}",
      "foo.proto: BAZ: NAME: Enum name BAZ has the same name as FOOENUM_BAZ "
      "if you ignore case and strip out the enum name prefix (if any). "
      "(If you are using allow_alias, please assign the same number "
      "to each enum value name.)\n");

  BuildFileWithErrors(
      "syntax: 'proto3'"
      "name: 'foo.proto' "
      "enum_type {"
      "  name: 'FooEnum' "
      "  value { name: 'FOO_ENUM_BAR_BAZ' number: 0 }"
      "  value { name: 'BAR__BAZ' number: 1 }"
      "}",
      "foo.proto: BAR__BAZ: NAME: Enum name BAR__BAZ has the same name as "
      "FOO_ENUM_BAR_BAZ if you ignore case and strip out the enum name prefix "
      "(if any). (If you are using allow_alias, please assign the same number "
      "to each enum value name.)\n");

  BuildFileWithErrors(
      "syntax: 'proto3'"
      "name: 'foo.proto' "
      "enum_type {"
      "  name: 'FooEnum' "
      "  value { name: 'FOO_ENUM__BAR_BAZ' number: 0 }"
      "  value { name: 'BAR_BAZ' number: 1 }"
      "}",
      "foo.proto: BAR_BAZ: NAME: Enum name BAR_BAZ has the same name as "
      "FOO_ENUM__BAR_BAZ if you ignore case and strip out the enum name prefix "
      "(if any). (If you are using allow_alias, please assign the same number "
      "to each enum value name.)\n");

  BuildFileWithErrors(
      "syntax: 'proto2'"
      "name: 'foo.proto' "
      "enum_type {"
      "  name: 'FooEnum' "
      "  value { name: 'FOO_ENUM__BAR_BAZ' number: 0 }"
      "  value { name: 'BAR_BAZ' number: 1 }"
      "}",
      "foo.proto: BAR_BAZ: NAME: Enum name BAR_BAZ has the same name as "
      "FOO_ENUM__BAR_BAZ if you ignore case and strip out the enum name prefix "
      "(if any). (If you are using allow_alias, please assign the same number "
      "to each enum value name.)\n");

  // This isn't an error because the underscore will cause the PascalCase to
  // differ by case (BarBaz vs. Barbaz).
  BuildFile(
      "syntax: 'proto3'"
      "name: 'foo.proto' "
      "enum_type {"
      "  name: 'FooEnum' "
      "  value { name: 'BAR_BAZ' number: 0 }"
      "  value { name: 'BARBAZ' number: 1 }"
      "}");
}

TEST_F(ValidationErrorTest, EnumValuesConflictLegacyBehavior) {
  BuildFileWithErrors(
      "syntax: 'proto3'"
      "name: 'foo.proto' "
      "enum_type {"
      "  name: 'FooEnum' "
      "  options { deprecated_legacy_json_field_conflicts: true }"
      "  value { name: 'BAR' number: 0 }"
      "  value { name: 'bar' number: 1 }"
      "}",
      "foo.proto: bar: NAME: Enum name bar has the same name as BAR "
      "if you ignore case and strip out the enum name prefix (if any). "
      "(If you are using allow_alias, please assign the same number "
      "to each enum value name.)\n");

  BuildFileWithErrors(
      "syntax: 'proto3'"
      "name: 'foo.proto' "
      "enum_type {"
      "  name: 'FooEnum' "
      "  options { deprecated_legacy_json_field_conflicts: true }"
      "  value { name: 'FOO_ENUM__BAR_BAZ' number: 0 }"
      "  value { name: 'BAR_BAZ' number: 1 }"
      "}",
      "foo.proto: BAR_BAZ: NAME: Enum name BAR_BAZ has the same name as "
      "FOO_ENUM__BAR_BAZ if you ignore case and strip out the enum name "
      "prefix "
      "(if any). (If you are using allow_alias, please assign the same "
      "number to each enum value name.)\n");

  BuildFileWithWarnings(
      "syntax: 'proto2'"
      "name: 'foo.proto' "
      "enum_type {"
      "  name: 'FooEnum' "
      "  options { deprecated_legacy_json_field_conflicts: true }"
      "  value { name: 'BAR' number: 0 }"
      "  value { name: 'bar' number: 1 }"
      "}",
      "foo.proto: bar: NAME: Enum name bar has the same name as BAR "
      "if you ignore case and strip out the enum name prefix (if any). "
      "(If you are using allow_alias, please assign the same number "
      "to each enum value name.)\n");
}

TEST_F(ValidationErrorTest, MapEntryConflictsWithOneof) {
  FileDescriptorProto file_proto;
  FillValidMapEntry(&file_proto);
  TextFormat::MergeFromString(
      "oneof_decl { "
      "  name: 'FooMapEntry' "
      "}"
      "field { "
      "  name: 'int_field' "
      "  type: TYPE_INT32 "
      "  label: LABEL_OPTIONAL "
      "  oneof_index: 0 "
      "  number: 100 "
      "} ",
      file_proto.mutable_message_type(0));
  BuildFileWithErrors(
      file_proto,
      "foo.proto: Foo.FooMapEntry: NAME: \"FooMapEntry\" is already defined in "
      "\"Foo\".\n"
      "foo.proto: Foo.foo_map: TYPE: \"FooMapEntry\" is not defined.\n"
      "foo.proto: Foo: NAME: Expanded map entry type FooMapEntry conflicts "
      "with an existing oneof type.\n");
}

TEST_F(ValidationErrorTest, MapEntryUsesNoneZeroEnumDefaultValue) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "enum_type {"
      "  name: \"Bar\""
      "  value { name:\"ENUM_A\" number:1 }"
      "  value { name:\"ENUM_B\" number:2 }"
      "}"
      "message_type {"
      "  name: 'Foo' "
      "  field { "
      "    name: 'foo_map' number: 1 label:LABEL_REPEATED "
      "    type_name: 'FooMapEntry' "
      "  } "
      "  nested_type { "
      "    name: 'FooMapEntry' "
      "    options {  map_entry: true } "
      "    field { "
      "      name: 'key' number: 1 type:TYPE_INT32 label:LABEL_OPTIONAL "
      "    } "
      "    field { "
      "      name: 'value' number: 2 type_name:\"Bar\" label:LABEL_OPTIONAL "
      "    } "
      "  } "
      "}",
      "foo.proto: Foo.foo_map: "
      "TYPE: Enum value in map must define 0 as the first value.\n");
}

TEST_F(ValidationErrorTest, Proto3RequiredFields) {
  BuildFileWithErrors(
      "name: 'foo.proto' "
      "syntax: 'proto3' "
      "message_type { "
      "  name: 'Foo' "
      "  field { name:'foo' number:1 label:LABEL_REQUIRED type:TYPE_INT32 } "
      "}",
      "foo.proto: Foo.foo: TYPE: Required fields are not allowed in "
      "proto3.\n");

  // applied to nested types as well.
  BuildFileWithErrors(
      "name: 'foo.proto' "
      "syntax: 'proto3' "
      "message_type { "
      "  name: 'Foo' "
      "  nested_type { "
      "    name : 'Bar' "
      "    field { name:'bar' number:1 label:LABEL_REQUIRED type:TYPE_INT32 } "
      "  } "
      "}",
      "foo.proto: Foo.Bar.bar: TYPE: Required fields are not allowed in "
      "proto3.\n");

  // optional and repeated fields are OK.
  BuildFile(
      "name: 'foo.proto' "
      "syntax: 'proto3' "
      "message_type { "
      "  name: 'Foo' "
      "  field { name:'foo' number:1 label:LABEL_OPTIONAL type:TYPE_INT32 } "
      "  field { name:'bar' number:2 label:LABEL_REPEATED type:TYPE_INT32 } "
      "}");
}

TEST_F(ValidationErrorTest, ValidateProto3DefaultValue) {
  BuildFileWithErrors(
      "name: 'foo.proto' "
      "syntax: 'proto3' "
      "message_type { "
      "  name: 'Foo' "
      "  field { name:'foo' number:1 label:LABEL_OPTIONAL type:TYPE_INT32 "
      "          default_value: '1' }"
      "}",
      "foo.proto: Foo.foo: DEFAULT_VALUE: Explicit default values are not "
      "allowed in proto3.\n");

  BuildFileWithErrors(
      "name: 'foo.proto' "
      "syntax: 'proto3' "
      "message_type { "
      "  name: 'Foo' "
      "  nested_type { "
      "    name : 'Bar' "
      "    field { name:'bar' number:1 label:LABEL_OPTIONAL type:TYPE_INT32 "
      "            default_value: '1' }"
      "  } "
      "}",
      "foo.proto: Foo.Bar.bar: DEFAULT_VALUE: Explicit default values are not "
      "allowed in proto3.\n");
}

TEST_F(ValidationErrorTest, ValidateProto3ExtensionRange) {
  BuildFileWithErrors(
      "name: 'foo.proto' "
      "syntax: 'proto3' "
      "message_type { "
      "  name: 'Foo' "
      "  field { name:'foo' number:1 label:LABEL_OPTIONAL type:TYPE_INT32 } "
      "  extension_range { start:10 end:100 } "
      "}",
      "foo.proto: Foo: NUMBER: Extension ranges are not allowed in "
      "proto3.\n");

  BuildFileWithErrors(
      "name: 'foo.proto' "
      "syntax: 'proto3' "
      "message_type { "
      "  name: 'Foo' "
      "  nested_type { "
      "    name : 'Bar' "
      "    field { name:'bar' number:1 label:LABEL_OPTIONAL type:TYPE_INT32 } "
      "    extension_range { start:10 end:100 } "
      "  } "
      "}",
      "foo.proto: Foo.Bar: NUMBER: Extension ranges are not allowed in "
      "proto3.\n");
}

TEST_F(ValidationErrorTest, ValidateProto3MessageSetWireFormat) {
  BuildFileWithErrors(
      "name: 'foo.proto' "
      "syntax: 'proto3' "
      "message_type { "
      "  name: 'Foo' "
      "  options { message_set_wire_format: true } "
      "}",
      "foo.proto: Foo: NAME: MessageSet is not supported "
      "in proto3.\n");
}

TEST_F(ValidationErrorTest, ValidateProto3Enum) {
  BuildFileWithErrors(
      "name: 'foo.proto' "
      "syntax: 'proto3' "
      "enum_type { "
      "  name: 'FooEnum' "
      "  value { name: 'FOO_FOO' number:1 } "
      "}",
      "foo.proto: FooEnum: NUMBER: The first enum value must be "
      "zero for open enums.\n");

  BuildFileWithErrors(
      "name: 'foo.proto' "
      "syntax: 'proto3' "
      "message_type { "
      "  name: 'Foo' "
      "  enum_type { "
      "    name: 'FooEnum' "
      "    value { name: 'FOO_FOO' number:1 } "
      "  } "
      "}",
      "foo.proto: Foo.FooEnum: NUMBER: The first enum value must be "
      "zero for open enums.\n");

  // valid case.
  BuildFile(
      "name: 'foo.proto' "
      "syntax: 'proto3' "
      "enum_type { "
      "  name: 'FooEnum' "
      "  value { name: 'FOO_FOO' number:0 } "
      "}");
}

TEST_F(ValidationErrorTest, ValidateProto3Group) {
  BuildFileWithErrors(
      "name: 'foo.proto' "
      "syntax: 'proto3' "
      "message_type { "
      "  name: 'Foo' "
      "  nested_type { "
      "    name: 'FooGroup' "
      "  } "
      "  field { name:'foo_group' number: 1 label:LABEL_OPTIONAL "
      "          type: TYPE_GROUP type_name:'FooGroup' } "
      "}",
      "foo.proto: Foo.foo_group: TYPE: Groups are not supported in proto3 "
      "syntax.\n");
}


TEST_F(ValidationErrorTest, ValidateProto3EnumFromProto2) {
  // Define an enum in a proto2 file.
  BuildFile(
      "name: 'foo.proto' "
      "package: 'foo' "
      "syntax: 'proto2' "
      "enum_type { "
      "  name: 'FooEnum' "
      "  value { name: 'DEFAULT_OPTION' number:0 } "
      "}");

  // Now try to refer to it. (All tests in the fixture use the same pool, so we
  // can refer to the enum above in this definition.)
  BuildFileWithErrors(
      "name: 'bar.proto' "
      "dependency: 'foo.proto' "
      "syntax: 'proto3' "
      "message_type { "
      "  name: 'Foo' "
      "    field { name:'bar' number:1 label:LABEL_OPTIONAL type:TYPE_ENUM "
      "            type_name: 'foo.FooEnum' }"
      "}",
      "bar.proto: Foo.bar: TYPE: Enum type \"foo.FooEnum\" is not an open "
      "enum, but is used in \"Foo\" which is a proto3 message type.\n");
}

TEST_F(ValidationErrorTest, ValidateProto3ClosedEnum) {
  // Define a closed enum in an editions file.
  BuildFile(R"pb(name: 'foo.proto'
                 package: 'foo'
                 syntax: 'editions'
                 edition: EDITION_2023
                 enum_type {
                   name: 'FooEnum'
                   value { name: 'DEFAULT_OPTION' number: 0 }
                   options { features { enum_type: CLOSED } }
                 })pb");

  BuildFileWithErrors(
      R"pb(name: 'bar.proto'
           dependency: 'foo.proto'
           syntax: 'proto3'
           message_type {
             name: 'Foo'
             field {
               name: 'bar'
               number: 1
               label: LABEL_OPTIONAL
               type: TYPE_ENUM
               type_name: 'foo.FooEnum'
             }
           })pb",
      "bar.proto: Foo.bar: TYPE: Enum type \"foo.FooEnum\" is not an open "
      "enum, but is used in \"Foo\" which is a proto3 message type.\n");
}

TEST_F(ValidationErrorTest, ValidateProto3OpenEnum) {
  // Define an open enum in an editions file.
  const FileDescriptor* foo =
      BuildFile(R"pb(name: 'foo.proto'
                     package: 'foo'
                     syntax: 'editions'
                     edition: EDITION_2023
                     enum_type {
                       name: 'FooEnum'
                       value { name: 'DEFAULT_OPTION' number: 0 }
                     })pb");
  const EnumDescriptor* enm = foo->enum_type(0);
  ASSERT_NE(enm, nullptr);

  const FileDescriptor* bar = BuildFile(
      R"pb(name: 'bar.proto'
           dependency: 'foo.proto'
           syntax: 'proto3'
           message_type {
             name: 'Foo'
             field {
               name: 'bar'
               number: 1
               label: LABEL_OPTIONAL
               type: TYPE_ENUM
               type_name: 'foo.FooEnum'
             }
           })pb");
  ASSERT_NE(bar, nullptr);

  EXPECT_EQ(bar->message_type(0)->field(0)->enum_type(), enm);
}

TEST_F(ValidationErrorTest, ValidateProto3Extension) {
  // Valid for options.
  DescriptorPool pool;
  FileDescriptorProto file_proto;
  // Add "google/protobuf/descriptor.proto".
  FileDescriptorProto::descriptor()->file()->CopyTo(&file_proto);
  ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);
  // Add "foo.proto":
  //   import "google/protobuf/descriptor.proto";
  //   extend google.protobuf.FileOptions {
  //     optional string test_file_opt = 1001;
  //   }
  //   extend google.protobuf.MessageOptions {
  //     optional string test_msg_opt = 1002;
  //   }
  //   extend google.protobuf.FieldOptions {
  //     optional string test_field_opt = 1003;
  //   }
  //   extend google.protobuf.EnumOptions {
  //     repeated int32 test_enum_opt = 1004;
  //   }
  //   extend google.protobuf.EnumValueOptions {
  //     optional int32 test_enumval_opt = 1005;
  //   }
  //   extend google.protobuf.ServiceOptions {
  //     repeated int32 test_svc_opt = 1006;
  //   }
  //   extend google.protobuf.MethodOptions {
  //     optional string test_method_opt = 1007;
  //   }
  //   extend google.protobuf.OneofOptions {
  //     optional string test_oneof_opt = 1008;
  //   }
  //   extend google.protobuf.ExtensionRangeOptions {
  //     optional string test_ext_opt = 1009;
  //   }
  file_proto.Clear();
  file_proto.set_name("foo.proto");
  file_proto.set_syntax("proto3");
  file_proto.add_dependency("google/protobuf/descriptor.proto");
  AddExtension(&file_proto, "google.protobuf.FileOptions", "test_file_opt", 1001,
               FieldDescriptorProto::LABEL_OPTIONAL,
               FieldDescriptorProto::TYPE_STRING);
  AddExtension(&file_proto, "google.protobuf.MessageOptions", "test_msg_opt", 1001,
               FieldDescriptorProto::LABEL_OPTIONAL,
               FieldDescriptorProto::TYPE_STRING);
  AddExtension(&file_proto, "google.protobuf.FieldOptions", "test_field_opt", 1003,
               FieldDescriptorProto::LABEL_OPTIONAL,
               FieldDescriptorProto::TYPE_STRING);
  AddExtension(&file_proto, "google.protobuf.EnumOptions", "test_enum_opt", 1004,
               FieldDescriptorProto::LABEL_REPEATED,
               FieldDescriptorProto::TYPE_INT32);
  AddExtension(&file_proto, "google.protobuf.EnumValueOptions", "test_enumval_opt", 1005,
               FieldDescriptorProto::LABEL_OPTIONAL,
               FieldDescriptorProto::TYPE_INT32);
  AddExtension(&file_proto, "google.protobuf.ServiceOptions", "test_svc_opt", 1006,
               FieldDescriptorProto::LABEL_REPEATED,
               FieldDescriptorProto::TYPE_INT32);
  AddExtension(&file_proto, "google.protobuf.MethodOptions", "test_method_opt", 1007,
               FieldDescriptorProto::LABEL_OPTIONAL,
               FieldDescriptorProto::TYPE_STRING);
  AddExtension(&file_proto, "google.protobuf.OneofOptions", "test_oneof_opt", 1008,
               FieldDescriptorProto::LABEL_OPTIONAL,
               FieldDescriptorProto::TYPE_STRING);
  AddExtension(&file_proto, "google.protobuf.ExtensionRangeOptions", "test_ext_opt",
               1009, FieldDescriptorProto::LABEL_OPTIONAL,
               FieldDescriptorProto::TYPE_STRING);
  ASSERT_TRUE(pool.BuildFile(file_proto) != nullptr);

  // Copy and change the package of the descriptor.proto
  BuildFile(
      "name: 'google.protobuf.proto' "
      "syntax: 'proto2' "
      "message_type { "
      "  name: 'Container' extension_range { start: 1 end: 1000 } "
      "}");
  BuildFileWithErrors(
      "name: 'bar.proto' "
      "syntax: 'proto3' "
      "dependency: 'google.protobuf.proto' "
      "extension { "
      "  name: 'bar' number: 1 label: LABEL_OPTIONAL type: TYPE_INT32 "
      "  extendee: 'Container' "
      "}",
      "bar.proto: bar: EXTENDEE: Extensions in proto3 are only allowed for "
      "defining options.\n");
}

// Test that field names that may conflict in JSON is not allowed by protoc.
TEST_F(ValidationErrorTest, ValidateJsonNameConflictProto3) {
  // The comparison is case-insensitive.
  BuildFileWithErrors(
      "name: 'foo.proto' "
      "syntax: 'proto3' "
      "message_type {"
      "  name: 'Foo'"
      "  field { name:'_name' number:1 label:LABEL_OPTIONAL type:TYPE_INT32 }"
      "  field { name:'Name' number:2 label:LABEL_OPTIONAL type:TYPE_INT32 }"
      "}",
      "foo.proto: Foo: NAME: The default JSON name of field \"Name\" "
      "(\"Name\") "
      "conflicts with the default JSON name of field \"_name\".\n");

  // Underscores are ignored.
  BuildFileWithErrors(
      "name: 'foo.proto' "
      "syntax: 'proto3' "
      "message_type {"
      "  name: 'Foo'"
      "  field { name:'AB' number:1 label:LABEL_OPTIONAL type:TYPE_INT32 }"
      "  field { name:'_a__b_' number:2 label:LABEL_OPTIONAL type:TYPE_INT32 }"
      "}",
      "foo.proto: Foo: NAME: The default JSON name of field \"_a__b_\" "
      "(\"AB\") "
      "conflicts with the default JSON name of field \"AB\".\n");
}

TEST_F(ValidationErrorTest, ValidateJsonNameConflictProto2) {
  BuildFileWithWarnings(
      "name: 'foo.proto' "
      "syntax: 'proto2' "
      "message_type {"
      "  name: 'Foo'"
      "  field { name:'AB' number:1 label:LABEL_OPTIONAL type:TYPE_INT32 }"
      "  field { name:'_a__b_' number:2 label:LABEL_OPTIONAL type:TYPE_INT32 }"
      "}",
      "foo.proto: Foo: NAME: The default JSON name of field \"_a__b_\" "
      "(\"AB\") "
      "conflicts with the default JSON name of field \"AB\".\n");
}

// Test that field names that may conflict in JSON is not allowed by protoc.
TEST_F(ValidationErrorTest, ValidateJsonNameConflictProto3Legacy) {
  BuildFile(
      "name: 'foo.proto' "
      "syntax: 'proto3' "
      "message_type {"
      "  name: 'Foo'"
      "  options { deprecated_legacy_json_field_conflicts: true }"
      "  field { name:'AB' number:1 label:LABEL_OPTIONAL type:TYPE_INT32 }"
      "  field { name:'_a__b_' number:2 label:LABEL_OPTIONAL type:TYPE_INT32 }"
      "}");
}

TEST_F(ValidationErrorTest, ValidateJsonNameConflictProto2Legacy) {
  BuildFile(
      "name: 'foo.proto' "
      "syntax: 'proto2' "
      "message_type {"
      "  name: 'Foo'"
      "  options { deprecated_legacy_json_field_conflicts: true }"
      "  field { name:'AB' number:1 label:LABEL_OPTIONAL type:TYPE_INT32 }"
      "  field { name:'_a__b_' number:2 label:LABEL_OPTIONAL type:TYPE_INT32 }"
      "}");
}


TEST_F(ValidationErrorTest, UnusedImportWithOtherError) {
  BuildFile(
      "name: 'bar.proto' "
      "message_type {"
      "  name: 'Bar'"
      "}");

  pool_.AddDirectInputFile("foo.proto", true);
  BuildFileWithErrors(
      "name: 'foo.proto' "
      "dependency: 'bar.proto' "
      "message_type {"
      "  name: 'Foo'"
      "  extension { name:'foo' number:1 label:LABEL_OPTIONAL type:TYPE_INT32"
      "              extendee: 'Baz' }"
      "}",

      // Should not also contain unused import error.
      "foo.proto: Foo.foo: EXTENDEE: \"Baz\" is not defined.\n");
}

TEST(IsGroupLike, GroupLikeDelimited) {
  using internal::cpp::IsGroupLike;
  const Descriptor& msg = *editions_unittest::TestDelimited::descriptor();
  const FileDescriptor& file =
      *editions_unittest::TestDelimited::descriptor()->file();

  EXPECT_EQ(msg.FindFieldByName("grouplike")->type(),
            FieldDescriptor::TYPE_GROUP);
  EXPECT_TRUE(IsGroupLike(*msg.FindFieldByName("grouplike")));
  EXPECT_EQ(file.FindExtensionByName("grouplikefilescope")->type(),
            FieldDescriptor::TYPE_GROUP);
  EXPECT_TRUE(IsGroupLike(*file.FindExtensionByName("grouplikefilescope")));
}

TEST(IsGroupLike, GroupLikeNotDelimited) {
  using internal::cpp::IsGroupLike;
  const Descriptor& msg = *editions_unittest::TestDelimited::descriptor();
  const FileDescriptor& file =
      *editions_unittest::TestDelimited::descriptor()->file();

  EXPECT_EQ(msg.FindFieldByName("lengthprefixed")->type(),
            FieldDescriptor::TYPE_MESSAGE);
  EXPECT_FALSE(IsGroupLike(*msg.FindFieldByName("lengthprefixed")));
  EXPECT_EQ(file.FindExtensionByName("lengthprefixed")->type(),
            FieldDescriptor::TYPE_MESSAGE);
  EXPECT_FALSE(IsGroupLike(*file.FindExtensionByName("lengthprefixed")));
}

TEST(IsGroupLike, GroupLikeMismatchedName) {
  using internal::cpp::IsGroupLike;
  const Descriptor& msg = *editions_unittest::TestDelimited::descriptor();
  const FileDescriptor& file =
      *editions_unittest::TestDelimited::descriptor()->file();

  EXPECT_EQ(msg.FindFieldByName("notgrouplike")->type(),
            FieldDescriptor::TYPE_GROUP);
  EXPECT_FALSE(IsGroupLike(*msg.FindFieldByName("notgrouplike")));
  EXPECT_EQ(file.FindExtensionByName("not_group_like_scope")->type(),
            FieldDescriptor::TYPE_GROUP);
  EXPECT_FALSE(IsGroupLike(*file.FindExtensionByName("not_group_like_scope")));
}

TEST(IsGroupLike, GroupLikeMismatchedScope) {
  using internal::cpp::IsGroupLike;
  const Descriptor& msg = *editions_unittest::TestDelimited::descriptor();
  const FileDescriptor& file =
      *editions_unittest::TestDelimited::descriptor()->file();

  EXPECT_EQ(msg.FindFieldByName("notgrouplikescope")->type(),
            FieldDescriptor::TYPE_GROUP);
  EXPECT_FALSE(IsGroupLike(*msg.FindFieldByName("notgrouplikescope")));
  EXPECT_EQ(file.FindExtensionByName("grouplike")->type(),
            FieldDescriptor::TYPE_GROUP);
  EXPECT_FALSE(IsGroupLike(*file.FindExtensionByName("grouplike")));
}

TEST(IsGroupLike, GroupLikeMismatchedFile) {
  using internal::cpp::IsGroupLike;
  const Descriptor& msg = *editions_unittest::TestDelimited::descriptor();
  const FileDescriptor& file =
      *editions_unittest::TestDelimited::descriptor()->file();

  EXPECT_EQ(msg.FindFieldByName("messageimport")->type(),
            FieldDescriptor::TYPE_GROUP);
  EXPECT_FALSE(IsGroupLike(*msg.FindFieldByName("messageimport")));
  EXPECT_EQ(file.FindExtensionByName("messageimport")->type(),
            FieldDescriptor::TYPE_GROUP);
  EXPECT_FALSE(IsGroupLike(*file.FindExtensionByName("messageimport")));
}

using FeaturesBaseTest = ValidationErrorTest;

class FeaturesTest : public FeaturesBaseTest {
 protected:
  void SetUp() override {
    ValidationErrorTest::SetUp();

    auto default_spec = FeatureResolver::CompileDefaults(
        FeatureSet::descriptor(),
        {GetExtensionReflection(pb::cpp), GetExtensionReflection(pb::test),
         GetExtensionReflection(pb::TestMessage::test_message),
         GetExtensionReflection(pb::TestMessage::Nested::test_nested)},
        EDITION_PROTO2, EDITION_99999_TEST_ONLY);
    ASSERT_OK(default_spec);
    ASSERT_OK(pool_.SetFeatureSetDefaults(std::move(default_spec).value()));
  }
};

template <typename T>
const FeatureSet& GetFeatures(const T* descriptor) {
  return internal::InternalFeatureHelper::GetFeatures<T>(*descriptor);
}

template <typename T>
FeatureSet GetCoreFeatures(const T* descriptor) {
  FeatureSet features = GetFeatures(descriptor);
  // Strip test features to avoid excessive brittleness.
  features.ClearExtension(pb::test);
  features.ClearExtension(pb::TestMessage::test_message);
  features.ClearExtension(pb::TestMessage::Nested::test_nested);
  return features;
}

TEST_F(FeaturesTest, InvalidProto2Features) {
  BuildDescriptorMessagesInTestPool();
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        syntax: "proto2"
        options { features { field_presence: IMPLICIT } }
      )pb",
      "foo.proto: foo.proto: EDITIONS: Features are only valid under "
      "editions.\n");
}

TEST_F(FeaturesTest, InvalidProto3Features) {
  BuildDescriptorMessagesInTestPool();
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        syntax: "proto3"
        options { features { field_presence: IMPLICIT } }
      )pb",
      "foo.proto: foo.proto: EDITIONS: Features are only valid "
      "under editions.\n");
}

TEST_F(FeaturesTest, Proto2Features) {
  FileDescriptorProto file_proto = ParseTextOrDie(R"pb(
    name: "foo.proto"
    message_type {
      name: "Foo"
      field { name: "bar" number: 1 label: LABEL_OPTIONAL type: TYPE_INT64 }
      field {
        name: "group"
        number: 2
        label: LABEL_OPTIONAL
        type: TYPE_GROUP
        type_name: ".Foo"
      }
      field { name: "str" number: 3 label: LABEL_OPTIONAL type: TYPE_STRING }
      field { name: "rep" number: 4 label: LABEL_REPEATED type: TYPE_INT32 }
      field {
        name: "packed"
        number: 5
        label: LABEL_REPEATED
        type: TYPE_INT64
        options { packed: true }
      }
      field { name: "utf8" number: 6 label: LABEL_REPEATED type: TYPE_STRING }
      field { name: "req" number: 7 label: LABEL_REQUIRED type: TYPE_INT32 }
      field {
        name: "cord"
        number: 8
        label: LABEL_OPTIONAL
        type: TYPE_BYTES
        options { ctype: CORD }
      }
      field {
        name: "piece"
        number: 9
        label: LABEL_OPTIONAL
        type: TYPE_STRING
        options { ctype: STRING_PIECE }
      }
    }
    enum_type {
      name: "Foo2"
      value { name: "BAR" number: 1 }
    }
  )pb");

  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::CppFeatures::GetDescriptor()->file());
  const FileDescriptor* file = ABSL_DIE_IF_NULL(pool_.BuildFile(file_proto));
  const Descriptor* message = file->message_type(0);
  const FieldDescriptor* field = message->field(0);
  const FieldDescriptor* group = message->field(1);
  EXPECT_THAT(file->options(), EqualsProto(""));
  EXPECT_EQ(GetFeatures(file).GetExtension(pb::test).file_feature(),
            pb::VALUE1);
  EXPECT_THAT(GetCoreFeatures(file), EqualsProto(R"pb(
                field_presence: EXPLICIT
                enum_type: CLOSED
                repeated_field_encoding: EXPANDED
                utf8_validation: NONE
                message_encoding: LENGTH_PREFIXED
                json_format: LEGACY_BEST_EFFORT
                enforce_naming_style: STYLE_LEGACY
                default_symbol_visibility: EXPORT_ALL
                [pb.cpp] {
                  legacy_closed_enum: true
                  string_type: STRING
                  enum_name_uses_string_view: false
                })pb"));
  EXPECT_THAT(GetCoreFeatures(field), EqualsProto(R"pb(
                field_presence: EXPLICIT
                enum_type: CLOSED
                repeated_field_encoding: EXPANDED
                utf8_validation: NONE
                message_encoding: LENGTH_PREFIXED
                json_format: LEGACY_BEST_EFFORT
                enforce_naming_style: STYLE_LEGACY
                default_symbol_visibility: EXPORT_ALL
                [pb.cpp] {
                  legacy_closed_enum: true
                  string_type: STRING
                  enum_name_uses_string_view: false
                })pb"));
  EXPECT_THAT(GetCoreFeatures(group), EqualsProto(R"pb(
                field_presence: EXPLICIT
                enum_type: CLOSED
                repeated_field_encoding: EXPANDED
                utf8_validation: NONE
                message_encoding: DELIMITED
                json_format: LEGACY_BEST_EFFORT
                enforce_naming_style: STYLE_LEGACY
                default_symbol_visibility: EXPORT_ALL
                [pb.cpp] {
                  legacy_closed_enum: true
                  string_type: STRING
                  enum_name_uses_string_view: false
                })pb"));
  EXPECT_TRUE(field->has_presence());
  EXPECT_FALSE(field->requires_utf8_validation());
  EXPECT_EQ(
      GetUtf8CheckMode(message->FindFieldByName("str"), /*is_lite=*/false),
      Utf8CheckMode::kVerify);
  EXPECT_EQ(GetUtf8CheckMode(message->FindFieldByName("str"), /*is_lite=*/true),
            Utf8CheckMode::kNone);
  EXPECT_EQ(GetCoreFeatures(message->FindFieldByName("cord"))
                .GetExtension(pb::cpp)
                .string_type(),
            pb::CppFeatures::CORD);
  EXPECT_FALSE(field->is_packed());
  EXPECT_FALSE(field->legacy_enum_field_treated_as_closed());
  EXPECT_FALSE(HasPreservingUnknownEnumSemantics(field));
  EXPECT_FALSE(message->FindFieldByName("str")->requires_utf8_validation());
  EXPECT_FALSE(message->FindFieldByName("rep")->is_packed());
  EXPECT_FALSE(message->FindFieldByName("utf8")->requires_utf8_validation());
  EXPECT_TRUE(message->FindFieldByName("packed")->is_packed());
  EXPECT_TRUE(message->FindFieldByName("req")->is_required());
  EXPECT_TRUE(file->enum_type(0)->is_closed());

  EXPECT_EQ(message->FindFieldByName("str")->cpp_string_type(),
            FieldDescriptor::CppStringType::kString);
  EXPECT_EQ(message->FindFieldByName("cord")->cpp_string_type(),
            FieldDescriptor::CppStringType::kCord);

  // Check round-trip consistency.
  FileDescriptorProto proto;
  file->CopyTo(&proto);
  std::string file_textproto;
  google::protobuf::TextFormat::PrintToString(file_proto, &file_textproto);
  EXPECT_THAT(proto, EqualsProto(file_textproto));
}

TEST_F(FeaturesTest, Proto3Features) {
  FileDescriptorProto file_proto = ParseTextOrDie(R"pb(
    name: "foo.proto"
    syntax: "proto3"
    message_type {
      name: "Foo"
      field { name: "bar" number: 1 label: LABEL_OPTIONAL type: TYPE_INT64 }
      field { name: "rep" number: 2 label: LABEL_REPEATED type: TYPE_INT64 }
      field { name: "str" number: 3 label: LABEL_OPTIONAL type: TYPE_STRING }
      field {
        name: "expanded"
        number: 4
        label: LABEL_REPEATED
        type: TYPE_INT64
        options { packed: false }
      }
      field { name: "utf8" number: 5 label: LABEL_OPTIONAL type: TYPE_STRING }
    }
    enum_type {
      name: "Foo2"
      value { name: "DEFAULT" number: 0 }
      value { name: "BAR" number: 1 }
    })pb");

  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = ABSL_DIE_IF_NULL(pool_.BuildFile(file_proto));
  const Descriptor* message = file->message_type(0);
  const FieldDescriptor* field = message->field(0);
  EXPECT_THAT(file->options(), EqualsProto(""));
  EXPECT_EQ(GetFeatures(file).GetExtension(pb::test).file_feature(),
            pb::VALUE2);
  EXPECT_THAT(GetCoreFeatures(file), EqualsProto(R"pb(
                field_presence: IMPLICIT
                enum_type: OPEN
                repeated_field_encoding: PACKED
                utf8_validation: VERIFY
                message_encoding: LENGTH_PREFIXED
                json_format: ALLOW
                enforce_naming_style: STYLE_LEGACY
                default_symbol_visibility: EXPORT_ALL
                [pb.cpp] {
                  legacy_closed_enum: false
                  string_type: STRING
                  enum_name_uses_string_view: false
                })pb"));
  EXPECT_THAT(GetCoreFeatures(field), EqualsProto(R"pb(
                field_presence: IMPLICIT
                enum_type: OPEN
                repeated_field_encoding: PACKED
                utf8_validation: VERIFY
                message_encoding: LENGTH_PREFIXED
                json_format: ALLOW
                enforce_naming_style: STYLE_LEGACY
                default_symbol_visibility: EXPORT_ALL
                [pb.cpp] {
                  legacy_closed_enum: false
                  string_type: STRING
                  enum_name_uses_string_view: false
                })pb"));
  EXPECT_FALSE(field->has_presence());
  EXPECT_FALSE(field->requires_utf8_validation());
  EXPECT_EQ(
      GetUtf8CheckMode(message->FindFieldByName("str"), /*is_lite=*/false),
      Utf8CheckMode::kStrict);
  EXPECT_EQ(GetUtf8CheckMode(message->FindFieldByName("str"), /*is_lite=*/true),
            Utf8CheckMode::kStrict);
  EXPECT_FALSE(field->is_packed());
  EXPECT_FALSE(field->legacy_enum_field_treated_as_closed());
  EXPECT_FALSE(HasPreservingUnknownEnumSemantics(field));
  EXPECT_TRUE(message->FindFieldByName("rep")->is_packed());
  EXPECT_TRUE(message->FindFieldByName("str")->requires_utf8_validation());
  EXPECT_FALSE(message->FindFieldByName("expanded")->is_packed());
  EXPECT_FALSE(file->enum_type(0)->is_closed());

  // Check round-trip consistency.
  FileDescriptorProto proto;
  file->CopyTo(&proto);
  std::string file_textproto;
  google::protobuf::TextFormat::PrintToString(file_proto, &file_textproto);
  EXPECT_THAT(proto, EqualsProto(file_textproto));
}

TEST_F(FeaturesTest, Proto2Proto3EnumFeatures) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::CppFeatures::GetDescriptor()->file());
  const FileDescriptor* file_proto3 = BuildFile(R"pb(
    name: "foo3.proto"
    syntax: "proto3"
    enum_type {
      name: "Enum3"
      value { name: "DEFAULT_ENUM3" number: 0 }
      value { name: "BAR_ENUM3" number: 1 }
    }
    message_type {
      name: "Message3"
      field {
        name: "enum_field"
        number: 1
        label: LABEL_OPTIONAL
        type: TYPE_ENUM
        type_name: ".Enum3"
      }
    }
  )pb");
  const FileDescriptor* file_proto2 = BuildFile(R"pb(
    name: "foo2.proto"
    dependency: "foo3.proto"
    enum_type {
      name: "Enum2"
      value { name: "DEFAULT_ENUM2" number: 0 }
      value { name: "BAR_ENUM2" number: 1 }
    }
    message_type {
      name: "Message2"
      field {
        name: "enum_field2"
        number: 1
        label: LABEL_OPTIONAL
        type: TYPE_ENUM
        type_name: ".Enum2"
      }
      field {
        name: "enum_field3"
        number: 2
        label: LABEL_OPTIONAL
        type: TYPE_ENUM
        type_name: ".Enum3"
      }
    }
  )pb");
  const Descriptor* message_proto2 = file_proto2->message_type(0);
  const Descriptor* message_proto3 = file_proto3->message_type(0);
  const FieldDescriptor* field_proto3 = message_proto3->field(0);
  const FieldDescriptor* field_proto2_closed = message_proto2->field(0);
  const FieldDescriptor* field_proto2_open = message_proto2->field(1);

  EXPECT_FALSE(field_proto3->legacy_enum_field_treated_as_closed());
  EXPECT_TRUE(field_proto2_closed->legacy_enum_field_treated_as_closed());
  EXPECT_TRUE(field_proto2_open->legacy_enum_field_treated_as_closed());
}

// Reproduces the reported issue in b/286244726 where custom options in proto3
// ended up losing implicit presence.  This only occurs when options are defined
// and used in the same file.
TEST_F(FeaturesTest, Proto3Extensions) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "proto3"
    dependency: "google/protobuf/descriptor.proto"
    message_type {
      name: "Ext"
      field { name: "bar" number: 1 label: LABEL_OPTIONAL type: TYPE_STRING }
      field { name: "baz" number: 2 label: LABEL_OPTIONAL type: TYPE_INT64 }
    }
    extension {
      name: "bar_ext"
      number: 99999
      label: LABEL_OPTIONAL
      type: TYPE_MESSAGE
      type_name: ".Ext"
      extendee: ".google.protobuf.EnumValueOptions"
    }
    enum_type {
      name: "Foo"
      value {
        name: "BAR"
        number: 0
        options {
          uninterpreted_option {
            name { name_part: "bar_ext" is_extension: true }
            aggregate_value: "bar: \"\" baz: 1"
          }
        }
      }
    }
  )pb");
  EXPECT_THAT(file->enum_type(0)->value(0)->options(),
              EqualsProtoSerialized(&pool_, "google.protobuf.EnumValueOptions",
                                    R"pb([bar_ext] { baz: 1 })pb"));
}

TEST_F(FeaturesTest, Proto3ExtensionPresence) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "proto3"
    dependency: "google/protobuf/descriptor.proto"
    extension {
      name: "singular_ext"
      number: 1001
      label: LABEL_OPTIONAL
      type: TYPE_STRING
      extendee: ".google.protobuf.FileOptions"
    }
    extension {
      name: "singular_proto3_optional_ext"
      number: 1002
      label: LABEL_OPTIONAL
      type: TYPE_STRING
      extendee: ".google.protobuf.FileOptions"
      proto3_optional: true
    }
    extension {
      name: "repeated_ext"
      number: 1003
      label: LABEL_REPEATED
      type: TYPE_STRING
      extendee: ".google.protobuf.FileOptions"
    }
  )pb");

  EXPECT_TRUE(file->extension(0)->has_presence());
  EXPECT_TRUE(file->extension(1)->has_presence());
  EXPECT_FALSE(file->extension(2)->has_presence());
}

TEST_F(FeaturesTest, Edition2023Defaults) {
  FileDescriptorProto file_proto = ParseTextOrDie(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
  )pb");

  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = ABSL_DIE_IF_NULL(pool_.BuildFile(file_proto));
  EXPECT_THAT(file->options(), EqualsProto(""));
  EXPECT_THAT(GetCoreFeatures(file), EqualsProto(R"pb(
                field_presence: EXPLICIT
                enum_type: OPEN
                repeated_field_encoding: PACKED
                utf8_validation: VERIFY
                message_encoding: LENGTH_PREFIXED
                json_format: ALLOW
                enforce_naming_style: STYLE_LEGACY
                default_symbol_visibility: EXPORT_ALL
                [pb.cpp] {
                  legacy_closed_enum: false
                  string_type: STRING
                  enum_name_uses_string_view: false
                }
              )pb"));

  // Since pb::test is registered in the pool, it should end up with defaults in
  // our FeatureSet.
  EXPECT_TRUE(GetFeatures(file).HasExtension(pb::test));
  EXPECT_EQ(GetFeatures(file).GetExtension(pb::test).file_feature(),
            pb::VALUE3);
}

TEST_F(FeaturesTest, Edition2023InferredFeatures) {
  FileDescriptorProto file_proto = ParseTextOrDie(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    message_type {
      name: "Foo"
      field { name: "str" number: 1 label: LABEL_OPTIONAL type: TYPE_STRING }
      field {
        name: "cord"
        number: 2
        label: LABEL_OPTIONAL
        type: TYPE_STRING
        options { ctype: CORD }
      }
      field {
        name: "piece"
        number: 3
        label: LABEL_OPTIONAL
        type: TYPE_STRING
        options { ctype: STRING_PIECE }
      }
      field {
        name: "view"
        number: 4
        label: LABEL_OPTIONAL
        type: TYPE_STRING
        options {
          features {
            [pb.cpp] { string_type: VIEW }
          }
        }
      }
    }
  )pb");

  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::CppFeatures::GetDescriptor()->file());
  const FileDescriptor* file = ABSL_DIE_IF_NULL(pool_.BuildFile(file_proto));
  const Descriptor* message = file->message_type(0);

  EXPECT_EQ(
      GetCoreFeatures(message->field(0)).GetExtension(pb::cpp).string_type(),
      pb::CppFeatures::STRING);
  EXPECT_EQ(
      GetCoreFeatures(message->field(1)).GetExtension(pb::cpp).string_type(),
      pb::CppFeatures::CORD);
  EXPECT_EQ(
      GetCoreFeatures(message->field(3)).GetExtension(pb::cpp).string_type(),
      pb::CppFeatures::VIEW);
}

TEST_F(FeaturesTest, Edition2024Defaults) {
  FileDescriptorProto file_proto = ParseTextOrDie(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2024
  )pb");

  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = ABSL_DIE_IF_NULL(pool_.BuildFile(file_proto));
  EXPECT_THAT(file->options(), EqualsProto(""));
  EXPECT_THAT(GetCoreFeatures(file), EqualsProto(R"pb(
                field_presence: EXPLICIT
                enum_type: OPEN
                repeated_field_encoding: PACKED
                utf8_validation: VERIFY
                message_encoding: LENGTH_PREFIXED
                json_format: ALLOW
                enforce_naming_style: STYLE2024
                default_symbol_visibility: EXPORT_TOP_LEVEL
                [pb.cpp] {
                  legacy_closed_enum: false
                  string_type: VIEW
                  enum_name_uses_string_view: true
                }
              )pb"));

  // Since pb::test is registered in the pool, it should end up with defaults in
  // our FeatureSet.
  EXPECT_TRUE(GetFeatures(file).HasExtension(pb::test));
  EXPECT_EQ(GetFeatures(file).GetExtension(pb::test).file_feature(),
            pb::VALUE3);
}

TEST_F(FeaturesBaseTest, DefaultEdition2023Defaults) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    dependency: "google/protobuf/unittest_features.proto"
  )pb");
  ASSERT_NE(file, nullptr);

  EXPECT_THAT(file->options(), EqualsProto(""));
  EXPECT_THAT(GetFeatures(file), EqualsProto(R"pb(
                field_presence: EXPLICIT
                enum_type: OPEN
                repeated_field_encoding: PACKED
                utf8_validation: VERIFY
                message_encoding: LENGTH_PREFIXED
                json_format: ALLOW
                enforce_naming_style: STYLE_LEGACY
                default_symbol_visibility: EXPORT_ALL
                [pb.cpp] {
                  legacy_closed_enum: false
                  string_type: STRING
                  enum_name_uses_string_view: false
                }
              )pb"));
  EXPECT_FALSE(GetFeatures(file).HasExtension(pb::test));
}

TEST_F(FeaturesTest, ClearsOptions) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    options {
      java_package: "bar"
      features { field_presence: IMPLICIT }
    }
  )pb");
  EXPECT_THAT(file->options(), EqualsProto("java_package: 'bar'"));
  EXPECT_THAT(GetCoreFeatures(file), EqualsProto(R"pb(
                field_presence: IMPLICIT
                enum_type: OPEN
                repeated_field_encoding: PACKED
                utf8_validation: VERIFY
                message_encoding: LENGTH_PREFIXED
                json_format: ALLOW
                enforce_naming_style: STYLE_LEGACY
                default_symbol_visibility: EXPORT_ALL
                [pb.cpp] {
                  legacy_closed_enum: false
                  string_type: STRING
                  enum_name_uses_string_view: false
                })pb"));
}

TEST_F(FeaturesTest, RestoresOptionsRoundTrip) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    dependency: "google/protobuf/unittest_features.proto"
    options {
      java_package: "bar"
      features {
        [pb.test] { file_feature: VALUE3 }
      }
    }
    message_type {
      name: "Foo"
      options {
        deprecated: true
        features {
          [pb.test] { message_feature: VALUE3 }
        }
      }
      field {
        name: "bar"
        number: 1
        label: LABEL_REPEATED
        type: TYPE_INT64
        options {
          deprecated: true
          features {
            [pb.test] { field_feature: VALUE9 }
          }
        }
      }
      field {
        name: "oneof_field"
        number: 2
        label: LABEL_OPTIONAL
        type: TYPE_INT64
        oneof_index: 0
      }
      oneof_decl {
        name: "foo_oneof"
        options {
          features {
            [pb.test] { oneof_feature: VALUE7 }
          }
        }
      }
      extension_range {
        start: 10
        end: 100
        options {
          verification: UNVERIFIED
          features {
            [pb.test] { extension_range_feature: VALUE15 }
          }
        }
      }
    }
    enum_type {
      name: "FooEnum"
      options {
        deprecated: true
        features {
          [pb.test] { enum_feature: VALUE4 }
        }
      }
      value {
        name: "BAR"
        number: 0
        options {
          deprecated: true
          features {
            [pb.test] { enum_entry_feature: VALUE8 }
          }
        }
      }
    }
    service {
      name: "FooService"
      options {
        deprecated: true
        features {
          [pb.test] { service_feature: VALUE11 }
        }
      }
      method {
        name: "BarMethod"
        input_type: "Foo"
        output_type: "Foo"
        options {
          deprecated: true
          features {
            [pb.test] { method_feature: VALUE12 }
          }
        }
      }
    }
  )pb");
  FileDescriptorProto proto;
  file->CopyTo(&proto);
  EXPECT_THAT(proto.options(),
              EqualsProto(R"pb(java_package: 'bar'
                               features {
                                 [pb.test] { file_feature: VALUE3 }
                               })pb"));
  EXPECT_THAT(proto.message_type(0).options(),
              EqualsProto(R"pb(deprecated: true
                               features {
                                 [pb.test] { message_feature: VALUE3 }
                               })pb"));
  EXPECT_THAT(proto.message_type(0).field(0).options(),
              EqualsProto(R"pb(deprecated: true
                               features {
                                 [pb.test] { field_feature: VALUE9 }
                               })pb"));
  EXPECT_THAT(proto.message_type(0).oneof_decl(0).options(),
              EqualsProto(R"pb(features {
                                 [pb.test] { oneof_feature: VALUE7 }
                               })pb"));
  EXPECT_THAT(proto.message_type(0).extension_range(0).options(),
              EqualsProto(R"pb(verification: UNVERIFIED
                               features {
                                 [pb.test] { extension_range_feature: VALUE15 }
                               })pb"));
  EXPECT_THAT(proto.enum_type(0).options(),
              EqualsProto(R"pb(deprecated: true
                               features {
                                 [pb.test] { enum_feature: VALUE4 }
                               })pb"));
  EXPECT_THAT(proto.enum_type(0).value(0).options(),
              EqualsProto(R"pb(deprecated: true
                               features {
                                 [pb.test] { enum_entry_feature: VALUE8 }
                               })pb"));
  EXPECT_THAT(proto.service(0).options(),
              EqualsProto(R"pb(deprecated: true
                               features {
                                 [pb.test] { service_feature: VALUE11 }
                               })pb"));
  EXPECT_THAT(proto.service(0).method(0).options(),
              EqualsProto(R"pb(deprecated: true
                               features {
                                 [pb.test] { method_feature: VALUE12 }
                               })pb"));
}

TEST_F(FeaturesTest, ReusesFeaturesFromParent) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    options { features { field_presence: IMPLICIT } }
    message_type {
      name: "Foo"
      options { deprecated: true }
      field {
        name: "bar"
        number: 1
        label: LABEL_REPEATED
        type: TYPE_INT64
        options { deprecated: true }
      }
    }
  )pb");
  EXPECT_EQ(&GetFeatures(file), &GetFeatures(file->message_type(0)));
  EXPECT_EQ(&GetFeatures(file), &GetFeatures(file->message_type(0)->field(0)));
}

TEST_F(FeaturesTest, ReusesFeaturesFromSibling) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    options { features { field_presence: IMPLICIT } }
    message_type {
      name: "Foo"
      options { deprecated: true }
      field {
        name: "bar1"
        number: 1
        label: LABEL_OPTIONAL
        type: TYPE_INT64
        options {
          deprecated: true
          features { field_presence: EXPLICIT }
        }
      }
      field {
        name: "baz"
        number: 2
        label: LABEL_OPTIONAL
        type: TYPE_STRING
        options { features { field_presence: EXPLICIT } }
      }
    }
  )pb");
  const Descriptor* message = file->message_type(0);
  EXPECT_NE(&GetFeatures(file), &GetFeatures(message->field(0)));
  EXPECT_EQ(&GetFeatures(message->field(0)), &GetFeatures(message->field(1)));
}

TEST_F(FeaturesTest, ReusesFeaturesFromDifferentFile) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file1 = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    options { features { field_presence: IMPLICIT } }
  )pb");
  const FileDescriptor* file2 = BuildFile(R"pb(
    name: "bar.proto"
    syntax: "editions"
    edition: EDITION_2023
    options { features { field_presence: IMPLICIT } }
  )pb");
  EXPECT_EQ(&GetFeatures(file1), &GetFeatures(file2));
}

TEST_F(FeaturesTest, ReusesFeaturesExtension) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file1 = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    options {
      features {
        [pb.TestMessage.test_message] { file_feature: VALUE6 }
        [pb.TestMessage.Nested.test_nested] { file_feature: VALUE5 }
        [pb.test] { file_feature: VALUE7 }
      }
    }
  )pb");
  const FileDescriptor* file2 = BuildFile(R"pb(
    name: "bar.proto"
    syntax: "editions"
    edition: EDITION_2023
    options {
      features {
        [pb.test] { file_feature: VALUE7 }
        [pb.TestMessage.test_message] { file_feature: VALUE6 }
        [pb.TestMessage.Nested.test_nested] { file_feature: VALUE5 }
      }
    }
  )pb");
  EXPECT_EQ(&GetFeatures(file1), &GetFeatures(file2));
}

TEST_F(FeaturesTest, RestoresLabelRoundTrip) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    message_type {
      name: "Foo"
      field {
        name: "bar"
        number: 1
        label: LABEL_OPTIONAL
        type: TYPE_STRING
        options { features { field_presence: LEGACY_REQUIRED } }
      }
    }
  )pb");
  const FieldDescriptor* field = file->message_type(0)->field(0);
  ASSERT_EQ(field->label(), FieldDescriptor::LABEL_REQUIRED);
  ASSERT_TRUE(field->is_required());

  FileDescriptorProto proto;
  file->CopyTo(&proto);
  const FieldDescriptorProto& field_proto = proto.message_type(0).field(0);
  EXPECT_EQ(field_proto.label(), FieldDescriptorProto::LABEL_OPTIONAL);
  EXPECT_THAT(
      field_proto.options(),
      EqualsProto(R"pb(features { field_presence: LEGACY_REQUIRED })pb"));
}

TEST_F(FeaturesTest, RestoresGroupRoundTrip) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    message_type {
      name: "Foo"
      nested_type {
        name: "FooGroup"
        field { name: "bar" number: 1 label: LABEL_OPTIONAL type: TYPE_STRING }
      }
      field {
        name: "baz"
        number: 1
        label: LABEL_OPTIONAL
        type: TYPE_MESSAGE
        type_name: ".Foo.FooGroup"
        options { features { message_encoding: DELIMITED } }
      }
    }
  )pb");
  const FieldDescriptor* field = file->message_type(0)->field(0);
  ASSERT_EQ(field->type(), FieldDescriptor::TYPE_GROUP);
  ASSERT_NE(field->message_type(), nullptr);

  FileDescriptorProto proto;
  file->CopyTo(&proto);
  const FieldDescriptorProto& field_proto = proto.message_type(0).field(0);
  EXPECT_EQ(field_proto.type(), FieldDescriptorProto::TYPE_MESSAGE);
  EXPECT_THAT(field_proto.options(),
              EqualsProto(R"pb(features { message_encoding: DELIMITED })pb"));
}

TEST_F(FeaturesTest, OnlyMessagesInheritGroupEncoding) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    options { features { message_encoding: DELIMITED } }
    message_type {
      name: "Foo"
      nested_type {
        name: "FooGroup"
        field { name: "bar" number: 1 label: LABEL_OPTIONAL type: TYPE_STRING }
      }
      field {
        name: "baz"
        number: 1
        label: LABEL_OPTIONAL
        type: TYPE_MESSAGE
        type_name: ".Foo.FooGroup"
      }
      field { name: "str" number: 2 label: LABEL_OPTIONAL type: TYPE_STRING }
    }
  )pb");
  const FieldDescriptor* group_field = file->message_type(0)->field(0);
  const FieldDescriptor* string_field = file->message_type(0)->field(1);
  EXPECT_EQ(group_field->type(), FieldDescriptor::TYPE_GROUP);
  EXPECT_EQ(string_field->type(), FieldDescriptor::TYPE_STRING);
  EXPECT_NE(group_field->message_type(), nullptr);
  EXPECT_EQ(string_field->message_type(), nullptr);
}

TEST_F(FeaturesTest, NoOptions) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file =
      BuildFile(R"pb(
        name: "foo.proto" syntax: "editions" edition: EDITION_2023
      )pb");
  EXPECT_EQ(&file->options(), &FileOptions::default_instance());
  EXPECT_THAT(GetCoreFeatures(file), EqualsProto(R"pb(
                field_presence: EXPLICIT
                enum_type: OPEN
                repeated_field_encoding: PACKED
                utf8_validation: VERIFY
                message_encoding: LENGTH_PREFIXED
                json_format: ALLOW
                enforce_naming_style: STYLE_LEGACY
                default_symbol_visibility: EXPORT_ALL
                [pb.cpp] {
                  legacy_closed_enum: false
                  string_type: STRING
                  enum_name_uses_string_view: false
                })pb"));
}

TEST_F(FeaturesTest, InvalidEdition) {
  BuildDescriptorMessagesInTestPool();
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto" syntax: "editions" edition: EDITION_1_TEST_ONLY
      )pb",
      "foo.proto: foo.proto: EDITIONS: Edition 1_TEST_ONLY is earlier than the "
      "minimum supported edition PROTO2\n");
}

TEST_F(FeaturesTest, FileFeatures) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    options { features { field_presence: IMPLICIT } }
  )pb");
  EXPECT_THAT(file->options(), EqualsProto(""));
  EXPECT_THAT(GetCoreFeatures(file), EqualsProto(R"pb(
                field_presence: IMPLICIT
                enum_type: OPEN
                repeated_field_encoding: PACKED
                utf8_validation: VERIFY
                message_encoding: LENGTH_PREFIXED
                json_format: ALLOW
                enforce_naming_style: STYLE_LEGACY
                default_symbol_visibility: EXPORT_ALL
                [pb.cpp] {
                  legacy_closed_enum: false
                  string_type: STRING
                  enum_name_uses_string_view: false
                })pb"));
}

TEST_F(FeaturesTest, FileFeaturesExtension) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_99998_TEST_ONLY
    dependency: "google/protobuf/unittest_features.proto"
    options { features { field_presence: IMPLICIT } }
  )pb");
  EXPECT_THAT(file->options(), EqualsProto(""));
  EXPECT_EQ(GetFeatures(file).field_presence(), FeatureSet::IMPLICIT);
  EXPECT_EQ(GetFeatures(file).enum_type(), FeatureSet::OPEN);
  EXPECT_EQ(GetFeatures(file).GetExtension(pb::test).file_feature(),
            pb::VALUE5);
  EXPECT_EQ(GetFeatures(file)
                .GetExtension(pb::TestMessage::test_message)
                .file_feature(),
            pb::VALUE5);
  EXPECT_EQ(GetFeatures(file)
                .GetExtension(pb::TestMessage::Nested::test_nested)
                .file_feature(),
            pb::VALUE5);
}

TEST_F(FeaturesTest, FileFeaturesExtensionOverride) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_99998_TEST_ONLY
    dependency: "google/protobuf/unittest_features.proto"
    options {
      features {
        field_presence: IMPLICIT
        [pb.test] { file_feature: VALUE7 }
        [pb.TestMessage.test_message] { file_feature: VALUE6 }
        [pb.TestMessage.Nested.test_nested] { file_feature: VALUE5 }
      }
    }
  )pb");
  EXPECT_THAT(file->options(), EqualsProto(""));
  EXPECT_EQ(GetFeatures(file).field_presence(), FeatureSet::IMPLICIT);
  EXPECT_EQ(GetFeatures(file).enum_type(), FeatureSet::OPEN);
  EXPECT_EQ(GetFeatures(file).GetExtension(pb::test).file_feature(),
            pb::VALUE7);
  EXPECT_EQ(GetFeatures(file)
                .GetExtension(pb::TestMessage::test_message)
                .file_feature(),
            pb::VALUE6);
  EXPECT_EQ(GetFeatures(file)
                .GetExtension(pb::TestMessage::Nested::test_nested)
                .file_feature(),
            pb::VALUE5);
}

TEST_F(FeaturesTest, MessageFeaturesDefault) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    message_type { name: "Foo" }
  )pb");
  const Descriptor* message = file->message_type(0);
  EXPECT_THAT(message->options(), EqualsProto(""));
  EXPECT_THAT(GetCoreFeatures(message), EqualsProto(R"pb(
                field_presence: EXPLICIT
                enum_type: OPEN
                repeated_field_encoding: PACKED
                utf8_validation: VERIFY
                message_encoding: LENGTH_PREFIXED
                json_format: ALLOW
                enforce_naming_style: STYLE_LEGACY
                default_symbol_visibility: EXPORT_ALL
                [pb.cpp] {
                  legacy_closed_enum: false
                  string_type: STRING
                  enum_name_uses_string_view: false
                })pb"));
}

TEST_F(FeaturesTest, MessageFeaturesInherit) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    options { features { field_presence: IMPLICIT } }
    message_type { name: "Foo" }
  )pb");
  const Descriptor* message = file->message_type(0);
  EXPECT_THAT(message->options(), EqualsProto(""));
  EXPECT_EQ(GetFeatures(message).field_presence(), FeatureSet::IMPLICIT);
}

TEST_F(FeaturesTest, MessageFeaturesOverride) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    dependency: "google/protobuf/unittest_features.proto"
    options {
      features {
        [pb.test] { multiple_feature: VALUE2 }
      }
    }
    message_type {
      name: "Foo"
      options {
        features {
          [pb.test] { multiple_feature: VALUE9 }
        }
      }
    }
  )pb");
  const Descriptor* message = file->message_type(0);
  EXPECT_THAT(message->options(), EqualsProto(""));
  EXPECT_EQ(GetFeatures(message).GetExtension(pb::test).multiple_feature(),
            pb::VALUE9);
}

TEST_F(FeaturesTest, NestedMessageFeaturesOverride) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    dependency: "google/protobuf/unittest_features.proto"
    options {
      features {
        [pb.test] { multiple_feature: VALUE2 file_feature: VALUE7 }
      }
    }
    message_type {
      name: "Foo"
      options {
        features {
          [pb.test] { multiple_feature: VALUE10 message_feature: VALUE3 }
        }
      }
      nested_type {
        name: "Bar"
        options {
          features {
            [pb.test] { multiple_feature: VALUE5 }
          }
        }
      }
    }
  )pb");
  const Descriptor* message = file->message_type(0)->nested_type(0);
  EXPECT_THAT(message->options(), EqualsProto(""));
  EXPECT_EQ(GetFeatures(message).GetExtension(pb::test).field_feature(),
            pb::VALUE1);
  EXPECT_EQ(GetFeatures(message).GetExtension(pb::test).multiple_feature(),
            pb::VALUE5);
  EXPECT_EQ(GetFeatures(message).GetExtension(pb::test).file_feature(),
            pb::VALUE7);
  EXPECT_EQ(GetFeatures(message).GetExtension(pb::test).message_feature(),
            pb::VALUE3);
}

TEST_F(FeaturesTest, FieldFeaturesDefault) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    message_type {
      name: "Foo"
      field { name: "bar" number: 1 label: LABEL_REPEATED type: TYPE_INT64 }
    }
  )pb");
  const FieldDescriptor* field = file->message_type(0)->field(0);
  EXPECT_THAT(field->options(), EqualsProto(""));
  EXPECT_THAT(GetCoreFeatures(field), EqualsProto(R"pb(
                field_presence: EXPLICIT
                enum_type: OPEN
                repeated_field_encoding: PACKED
                utf8_validation: VERIFY
                message_encoding: LENGTH_PREFIXED
                json_format: ALLOW
                enforce_naming_style: STYLE_LEGACY
                default_symbol_visibility: EXPORT_ALL
                [pb.cpp] {
                  legacy_closed_enum: false
                  string_type: STRING
                  enum_name_uses_string_view: false
                })pb"));
}

TEST_F(FeaturesTest, FieldFeaturesInherit) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    dependency: "google/protobuf/unittest_features.proto"
    options {
      features {
        field_presence: IMPLICIT
        [pb.test] { multiple_feature: VALUE1 }
      }
    }
    message_type {
      name: "Foo"
      options {
        features {
          [pb.test] { multiple_feature: VALUE9 }
        }
      }
      field { name: "bar" number: 1 label: LABEL_REPEATED type: TYPE_INT64 }
    }
  )pb");
  const FieldDescriptor* field = file->message_type(0)->field(0);
  EXPECT_THAT(field->options(), EqualsProto(""));
  EXPECT_EQ(GetFeatures(field).field_presence(), FeatureSet::IMPLICIT);
  EXPECT_EQ(GetFeatures(field).GetExtension(pb::test).multiple_feature(),
            pb::VALUE9);
}

TEST_F(FeaturesTest, FieldFeaturesOverride) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    dependency: "google/protobuf/unittest_features.proto"
    options {
      features {
        enum_type: CLOSED
        field_presence: IMPLICIT
        [pb.test] { multiple_feature: VALUE2 }
      }
    }
    message_type {
      name: "Foo"
      options {
        features {
          [pb.test] { multiple_feature: VALUE3 }
        }
      }
      field {
        name: "bar"
        number: 1
        label: LABEL_OPTIONAL
        type: TYPE_STRING
        options {
          features {
            field_presence: EXPLICIT
            [pb.test] { multiple_feature: VALUE9 }
          }
        }
      }
    }
  )pb");
  const FieldDescriptor* field = file->message_type(0)->field(0);
  EXPECT_THAT(field->options(), EqualsProto(""));
  EXPECT_EQ(GetFeatures(field).field_presence(), FeatureSet::EXPLICIT);
  EXPECT_EQ(GetFeatures(field).enum_type(), FeatureSet::CLOSED);
  EXPECT_EQ(GetFeatures(field).GetExtension(pb::test).multiple_feature(),
            pb::VALUE9);
}

TEST_F(FeaturesTest, OneofFieldFeaturesInherit) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    dependency: "google/protobuf/unittest_features.proto"
    options {
      features {
        field_presence: IMPLICIT
        [pb.test] { multiple_feature: VALUE1 }
      }
    }
    message_type {
      name: "Foo"
      options {
        features {
          [pb.test] { multiple_feature: VALUE6 }
        }
      }
      field {
        name: "bar"
        number: 1
        label: LABEL_OPTIONAL
        type: TYPE_INT64
        oneof_index: 0
      }
      oneof_decl {
        name: "foo_oneof"
        options {
          features {
            [pb.test] { multiple_feature: VALUE9 }
          }
        }
      }
    }
  )pb");
  const FieldDescriptor* field = file->message_type(0)->field(0);
  EXPECT_THAT(field->options(), EqualsProto(""));
  EXPECT_EQ(GetFeatures(field).field_presence(), FeatureSet::IMPLICIT);
  EXPECT_EQ(GetFeatures(field).GetExtension(pb::test).multiple_feature(),
            pb::VALUE9);
}

TEST_F(FeaturesTest, OneofFieldFeaturesOverride) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    dependency: "google/protobuf/unittest_features.proto"
    options {
      features {
        [pb.test] { multiple_feature: VALUE2 file_feature: VALUE4 }
      }
    }
    message_type {
      name: "Foo"
      options {
        features {
          [pb.test] { multiple_feature: VALUE3 message_feature: VALUE3 }
        }
      }
      field {
        name: "bar"
        number: 1
        label: LABEL_OPTIONAL
        type: TYPE_STRING
        options {
          features {
            [pb.test] { multiple_feature: VALUE9 }
          }
        }
        oneof_index: 0
      }
      oneof_decl {
        name: "foo_oneof"
        options {
          features {
            [pb.test] { multiple_feature: VALUE6 oneof_feature: VALUE6 }
          }
        }
      }
    }
  )pb");
  const FieldDescriptor* field = file->message_type(0)->field(0);
  EXPECT_THAT(field->options(), EqualsProto(""));
  EXPECT_EQ(GetFeatures(field).GetExtension(pb::test).multiple_feature(),
            pb::VALUE9);
  EXPECT_EQ(GetFeatures(field).GetExtension(pb::test).oneof_feature(),
            pb::VALUE6);
  EXPECT_EQ(GetFeatures(field).GetExtension(pb::test).message_feature(),
            pb::VALUE3);
  EXPECT_EQ(GetFeatures(field).GetExtension(pb::test).file_feature(),
            pb::VALUE4);
}

TEST_F(FeaturesTest, MapFieldFeaturesOverride) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file = ParseAndBuildFile("foo.proto", R"schema(
    edition = "2023";

    import "google/protobuf/unittest_features.proto";

    option features.(pb.test).file_feature = VALUE7;
    option features.(pb.test).multiple_feature = VALUE1;

    message Foo {
      option features.(pb.test).message_feature = VALUE8;
      option features.(pb.test).multiple_feature = VALUE2;

      map<string, string> map_field = 1 [
        features.(pb.test).field_feature = VALUE10,
        features.(pb.test).multiple_feature = VALUE3
      ];
    }
  )schema");
  ASSERT_THAT(file, NotNull());

  const FieldDescriptor* map_field = file->message_type(0)->field(0);
  const FieldDescriptor* key = map_field->message_type()->field(0);
  const FieldDescriptor* value = map_field->message_type()->field(1);

  auto validate = [](const FieldDescriptor* desc) {
    EXPECT_EQ(GetFeatures(desc).GetExtension(pb::test).file_feature(),
              pb::VALUE7)
        << desc->DebugString();
    EXPECT_EQ(GetFeatures(desc).GetExtension(pb::test).message_feature(),
              pb::VALUE8)
        << desc->DebugString();
    EXPECT_EQ(GetFeatures(desc).GetExtension(pb::test).field_feature(),
              pb::VALUE10)
        << desc->DebugString();
    EXPECT_EQ(GetFeatures(desc).GetExtension(pb::test).multiple_feature(),
              pb::VALUE3)
        << desc->DebugString();
  };

  validate(map_field);
  validate(key);
  validate(value);
}

TEST_F(FeaturesTest, MapFieldFeaturesStringValidation) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = ParseAndBuildFile("foo.proto", R"schema(
    edition = "2023";

    message Foo {
      map<string, string> map_field = 1 [
        features.utf8_validation = NONE
      ];
      map<int32, string> map_field_value = 2 [
        features.utf8_validation = NONE
      ];
      map<string, int32> map_field_key = 3 [
        features.utf8_validation = NONE
      ];
    }
  )schema");
  ASSERT_THAT(file, NotNull());

  auto validate_map_field = [](const FieldDescriptor* field) {
    const FieldDescriptor* key = field->message_type()->field(0);
    const FieldDescriptor* value = field->message_type()->field(1);

    EXPECT_FALSE(field->requires_utf8_validation()) << field->DebugString();
    EXPECT_FALSE(key->requires_utf8_validation()) << field->DebugString();
    EXPECT_FALSE(value->requires_utf8_validation()) << field->DebugString();
  };

  validate_map_field(file->message_type(0)->field(0));
  validate_map_field(file->message_type(0)->field(1));
  validate_map_field(file->message_type(0)->field(2));
}

TEST_F(FeaturesTest, MapFieldFeaturesImplicitPresence) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* editions = ParseAndBuildFile("editions.proto", R"schema(
    edition = "2023";

    option features.field_presence = IMPLICIT;

    message Foo {
      map<string, Foo> message_map = 1;
      map<string, string> string_map = 2;
    }
  )schema");
  ASSERT_THAT(editions, NotNull());
  const FileDescriptor* proto3 = ParseAndBuildFile("proto3.proto", R"schema(
    syntax = "proto3";

    message Bar {
      map<string, Bar> message_map = 1;
      map<string, string> string_map = 2;
    }
  )schema");
  ASSERT_THAT(proto3, NotNull());

  auto validate_maps = [](const FileDescriptor* file) {
    const FieldDescriptor* message_map = file->message_type(0)->field(0);
    EXPECT_FALSE(message_map->has_presence());
    EXPECT_FALSE(message_map->message_type()->field(0)->has_presence());
    EXPECT_TRUE(message_map->message_type()->field(1)->has_presence());

    const FieldDescriptor* string_map = file->message_type(0)->field(1);
    EXPECT_FALSE(string_map->has_presence());
    EXPECT_FALSE(string_map->message_type()->field(0)->has_presence());
    EXPECT_FALSE(string_map->message_type()->field(1)->has_presence());
  };
  validate_maps(editions);
  validate_maps(proto3);
}

TEST_F(FeaturesTest, MapFieldFeaturesExplicitPresence) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* editions = ParseAndBuildFile("editions.proto", R"schema(
    edition = "2023";

    message Foo {
      map<string, Foo> message_map = 1;
      map<string, string> string_map = 2;
    }
  )schema");
  ASSERT_THAT(editions, NotNull());
  const FileDescriptor* proto2 = ParseAndBuildFile("google.protobuf.proto", R"schema(
    syntax = "proto2";

    message Bar {
      map<string, Bar> message_map = 1;
      map<string, string> string_map = 2;
    }
  )schema");
  ASSERT_THAT(proto2, NotNull());

  auto validate_maps = [](const FileDescriptor* file) {
    const FieldDescriptor* message_map = file->message_type(0)->field(0);
    EXPECT_FALSE(message_map->has_presence());
    EXPECT_TRUE(message_map->message_type()->field(0)->has_presence());
    EXPECT_TRUE(message_map->message_type()->field(1)->has_presence());

    const FieldDescriptor* string_map = file->message_type(0)->field(1);
    EXPECT_FALSE(string_map->has_presence());
    EXPECT_TRUE(string_map->message_type()->field(0)->has_presence());
    EXPECT_TRUE(string_map->message_type()->field(1)->has_presence());
  };
  validate_maps(editions);
  validate_maps(proto2);
}

TEST_F(FeaturesTest, NoNamingStyleViolationsUnlessPoolOptIn) {
  BuildDescriptorMessagesInTestPool();

  // By default, the pool does not enforce naming style violations.
  ASSERT_THAT(ParseAndBuildFile("naming.proto", R"schema(
    edition = "2024";
    package naming;
    message bad_message_name {}
  )schema"),
              NotNull());
}

TEST_F(FeaturesTest, NoNamingStyleViolationsWithPoolOptInIfMessagesAreGood) {
  BuildDescriptorMessagesInTestPool();

  pool_.EnforceNamingStyle(true);

  // Proto2 will have the name enforcement feature off.
  ASSERT_THAT(ParseAndBuildFile("naming1.proto", R"schema(
    syntax = "proto2";
    package naming1;
    message bad_message_name {}
  )schema"),
              NotNull());

  // Edition 2024 with good names.
  ASSERT_THAT(ParseAndBuildFile("naming2.proto", R"schema(
    edition = "2024";
    package naming2.good_package;
    message GoodMessageName { int32 good_field_name = 1; }
    enum GoodEnumName { GOOD_ENUM_VALUE = 0; }
    service GoodServiceName {
      rpc GoodMethodName(GoodMessageName) returns (GoodMessageName) {}
    }
  )schema"),
              NotNull());

  // Edition 2024 with bad names but out-out feature.
  ASSERT_THAT(ParseAndBuildFile("naming3.proto", R"schema(
    edition = "2024";
    package naming3;
    option features.enforce_naming_style = STYLE_LEGACY;
    message bad_message { oneof BadOneof { int32 BadFieldName = 1;  } }
    enum _bad_enum_ { bAd_eNuM_vAlUE = 0; }
    service BadServiceName__1 {
      rpc BadMethodName(bad_message) returns (bad_message) {}
    }
  )schema"),
              NotNull());
}

TEST_F(FeaturesTest, VisibilityFeatureSetStrict) {
  BuildDescriptorMessagesInTestPool();

  ASSERT_THAT(ParseAndBuildFile("vis.proto", R"schema(
    edition = "2024";
    package naming;

    option features.default_symbol_visibility = STRICT;

    local message LocalOuter {
      local enum Inner {
        VAL_1 = 0;
      }
    }

    export message ExportOuter {
      enum Inner {
        VAL_1 = 0;
      }
    }
  )schema"),
              NotNull());
}

TEST_F(FeaturesTest, VisibilityFeatureSetStrictBadNested) {
  BuildDescriptorMessagesInTestPool();

  ParseAndBuildFileWithErrorSubstr(
      "vis.proto", R"schema(
    edition = "2024";
    package naming;

    option features.default_symbol_visibility = STRICT;

    local message LocalOuter {
      export message Inner {
      }
    }
  )schema",
      "\"Inner\" is a nested message and cannot be `export` with STRICT "
      "default_symbol_visibility. It must be moved to top-level, ideally in "
      "its own file "
      "in order to be `export`.");
}

TEST_F(FeaturesTest, BadPackageName) {
  BuildDescriptorMessagesInTestPool();

  pool_.EnforceNamingStyle(true);

  ParseAndBuildFileWithErrorSubstr(
      "naming1.proto", R"schema(
      edition = "2024";
      package bad.Package.name;
      )schema",
      "Package name bad.Package.name should be lower_snake_case");

  ParseAndBuildFileWithErrorSubstr(
      "naming2.proto", R"schema(
      edition = "2024";
      package bad_____underscores;
      )schema",
      "Package name bad_____underscores contains style violating underscores");
}

TEST_F(FeaturesTest, BadMessageName) {
  BuildDescriptorMessagesInTestPool();

  pool_.EnforceNamingStyle(true);

  ParseAndBuildFileWithErrorSubstr(
      "naming.proto", R"schema(
    edition = "2024";
    package naming;
    message GoodMessageName { message badmessagename {} }
  )schema",
      "Message name badmessagename should begin with a capital letter");
}

TEST_F(FeaturesTest, BadOneofName) {
  BuildDescriptorMessagesInTestPool();

  pool_.EnforceNamingStyle(true);

  ParseAndBuildFileWithErrorSubstr(
      "naming1.proto", R"schema(
    edition = "2024";
    package naming1;
    message GoodMessageName { oneof BadOneofName { int32 x = 1; } }
  )schema",
      "Oneof name BadOneofName should be lower_snake_case");

  ParseAndBuildFileWithErrorSubstr(
      "naming2.proto", R"schema(
      edition = "2024";
      package naming2;
      message GoodMessageName { oneof o_ { int32 x = 1; } }
      )schema",
      "Oneof name o_ contains style violating underscores");
}

TEST_F(FeaturesTest, BadFieldName) {
  BuildDescriptorMessagesInTestPool();

  pool_.EnforceNamingStyle(true);

  ParseAndBuildFileWithErrorSubstr(
      "naming1.proto", R"schema(
    edition = "2024";
    package naming1;
    message GoodMessageName { int32 BadFieldName = 1; }
  )schema",
      "Field name BadFieldName should be lower_snake_case");

  ParseAndBuildFileWithErrorSubstr(
      "naming2.proto", R"schema(
      edition = "2024";
      package naming2;
      message GoodMessageName { int32 f_1 = 1; }
      )schema",
      "Field name f_1 contains style violating underscores");
}

TEST_F(FeaturesTest, BadEnumName) {
  BuildDescriptorMessagesInTestPool();

  pool_.EnforceNamingStyle(true);

  ParseAndBuildFileWithErrorSubstr("naming.proto", R"schema(
    edition = "2024";
    package naming;
    enum bad_enum { UNKNOWN = 0;}
  )schema",
                                   "Enum name bad_enum should be TitleCase");
}

TEST_F(FeaturesTest, BadEnumValueName) {
  BuildDescriptorMessagesInTestPool();

  pool_.EnforceNamingStyle(true);

  ParseAndBuildFileWithErrorSubstr(
      "naming.proto", R"schema(
    edition = "2024";
    package naming;
    enum GoodEnum { unknown = 0; }
  )schema",
      "Enum value name unknown should be UPPER_SNAKE_CASE");
}

TEST_F(FeaturesTest, BadServiceName) {
  BuildDescriptorMessagesInTestPool();

  pool_.EnforceNamingStyle(true);

  ParseAndBuildFileWithErrorSubstr(
      "naming1.proto", R"schema(
    edition = "2024";
    package naming1;
    message M {}
    service badService { rpc GoodMethodName(M) returns (M) {} }
  )schema",
      "Service name badService should begin with a capital letter");
}

TEST_F(FeaturesTest, BadMethodName) {
  BuildDescriptorMessagesInTestPool();

  pool_.EnforceNamingStyle(true);

  ParseAndBuildFileWithErrorSubstr(
      "naming1.proto", R"schema(
    edition = "2024";
    package naming1;
    message M {}
    service GoodService { rpc badMethodName(M) returns (M) {} }
  )schema",
      "Method name badMethodName should begin with a capital letter");
}

TEST_F(FeaturesTest, MapFieldFeaturesInheritedMessageEncoding) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = ParseAndBuildFile("foo.proto", R"schema(
    edition = "2023";

    option features.message_encoding = DELIMITED;

    message Foo {
      map<int32, Foo> message_map = 1;
      map<string, string> string_map = 2;
    }
  )schema");
  ASSERT_THAT(file, NotNull());

  const FieldDescriptor* message_map = file->message_type(0)->field(0);
  EXPECT_EQ(message_map->type(), FieldDescriptor::TYPE_MESSAGE);
  EXPECT_EQ(message_map->message_type()->field(0)->type(),
            FieldDescriptor::TYPE_INT32);
  EXPECT_EQ(message_map->message_type()->field(1)->type(),
            FieldDescriptor::TYPE_MESSAGE);

  const FieldDescriptor* string_map = file->message_type(0)->field(1);
  EXPECT_EQ(string_map->type(), FieldDescriptor::TYPE_MESSAGE);
  EXPECT_EQ(string_map->message_type()->field(0)->type(),
            FieldDescriptor::TYPE_STRING);
  EXPECT_EQ(string_map->message_type()->field(1)->type(),
            FieldDescriptor::TYPE_STRING);
}

TEST_F(FeaturesTest, RootExtensionFeaturesOverride) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    dependency: "google/protobuf/unittest_features.proto"
    options {
      features {
        enum_type: CLOSED
        field_presence: IMPLICIT
        [pb.test] { multiple_feature: VALUE2 }
      }
    }
    extension {
      name: "bar"
      number: 1
      label: LABEL_OPTIONAL
      type: TYPE_STRING
      options {
        features {
          enum_type: OPEN
          [pb.test] { multiple_feature: VALUE9 }
        }
      }
      extendee: "Foo"
    }
    message_type {
      name: "Foo"
      extension_range { start: 1 end: 2 }
    }
  )pb");
  const FieldDescriptor* field = file->extension(0);
  EXPECT_THAT(field->options(), EqualsProto(""));
  EXPECT_EQ(GetFeatures(field).field_presence(), FeatureSet::IMPLICIT);
  EXPECT_EQ(GetFeatures(field).enum_type(), FeatureSet::OPEN);
  EXPECT_EQ(GetFeatures(field).GetExtension(pb::test).multiple_feature(),
            pb::VALUE9);
}

TEST_F(FeaturesTest, MessageExtensionFeaturesOverride) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    dependency: "google/protobuf/unittest_features.proto"
    options {
      features {
        enum_type: CLOSED
        field_presence: IMPLICIT
        [pb.test] { multiple_feature: VALUE2 }
      }
    }
    message_type {
      name: "Foo"
      options {
        features {
          [pb.test] { multiple_feature: VALUE3 }
        }
      }
      extension {
        name: "bar"
        number: 1
        label: LABEL_OPTIONAL
        type: TYPE_STRING
        options { features { enum_type: OPEN } }
        extendee: "Foo2"
      }
    }
    message_type {
      name: "Foo2"
      extension_range { start: 1 end: 2 }
      options {
        features {
          [pb.test] { multiple_feature: VALUE7 }
        }
      }
    }
  )pb");
  const FieldDescriptor* field = file->message_type(0)->extension(0);
  EXPECT_THAT(field->options(), EqualsProto(""));
  EXPECT_EQ(GetFeatures(field).field_presence(), FeatureSet::IMPLICIT);
  EXPECT_EQ(GetFeatures(field).enum_type(), FeatureSet::OPEN);
  EXPECT_EQ(GetFeatures(field).GetExtension(pb::test).multiple_feature(),
            pb::VALUE3);
}

TEST_F(FeaturesTest, EnumFeaturesDefault) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    enum_type {
      name: "Foo"
      value { name: "BAR" number: 0 }
    }
  )pb");
  const EnumDescriptor* enm = file->enum_type(0);
  EXPECT_THAT(enm->options(), EqualsProto(""));
  EXPECT_THAT(GetCoreFeatures(enm), EqualsProto(R"pb(
                field_presence: EXPLICIT
                enum_type: OPEN
                repeated_field_encoding: PACKED
                utf8_validation: VERIFY
                message_encoding: LENGTH_PREFIXED
                json_format: ALLOW
                enforce_naming_style: STYLE_LEGACY
                default_symbol_visibility: EXPORT_ALL
                [pb.cpp] {
                  legacy_closed_enum: false
                  string_type: STRING
                  enum_name_uses_string_view: false
                })pb"));
}

TEST_F(FeaturesTest, EnumFeaturesInherit) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    options { features { enum_type: CLOSED } }
    enum_type {
      name: "Foo"
      value { name: "BAR" number: 0 }
    }
  )pb");
  const EnumDescriptor* enm = file->enum_type(0);
  EXPECT_THAT(enm->options(), EqualsProto(""));
  EXPECT_EQ(GetFeatures(enm).enum_type(), FeatureSet::CLOSED);
}

TEST_F(FeaturesTest, EnumFeaturesOverride) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    dependency: "google/protobuf/unittest_features.proto"
    options {
      features {
        [pb.test] { multiple_feature: VALUE2 }
      }
    }
    enum_type {
      name: "Foo"
      options {
        features {
          [pb.test] { multiple_feature: VALUE9 }
        }
      }
      value { name: "BAR" number: 0 }
    }
  )pb");
  const EnumDescriptor* enm = file->enum_type(0);
  EXPECT_THAT(enm->options(), EqualsProto(""));
  EXPECT_EQ(GetFeatures(enm).GetExtension(pb::test).multiple_feature(),
            pb::VALUE9);
}

TEST_F(FeaturesTest, NestedEnumFeatures) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    dependency: "google/protobuf/unittest_features.proto"
    options {
      features {
        [pb.test] { multiple_feature: VALUE2 file_feature: VALUE7 }
      }
    }
    message_type {
      name: "Foo"
      options {
        features {
          [pb.test] { multiple_feature: VALUE10 message_feature: VALUE3 }
        }
      }
      enum_type {
        name: "Bar"
        options {
          features {
            [pb.test] { multiple_feature: VALUE5 }
          }
        }
        value { name: "BAR" number: 0 }
      }
    }
  )pb");
  const EnumDescriptor* enm = file->message_type(0)->enum_type(0);
  EXPECT_THAT(enm->options(), EqualsProto(""));
  EXPECT_EQ(GetFeatures(enm).GetExtension(pb::test).field_feature(),
            pb::VALUE1);
  EXPECT_EQ(GetFeatures(enm).GetExtension(pb::test).multiple_feature(),
            pb::VALUE5);
  EXPECT_EQ(GetFeatures(enm).GetExtension(pb::test).file_feature(), pb::VALUE7);
  EXPECT_EQ(GetFeatures(enm).GetExtension(pb::test).message_feature(),
            pb::VALUE3);
}

TEST_F(FeaturesTest, EnumValueFeaturesDefault) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    enum_type {
      name: "Foo"
      value { name: "BAR" number: 0 }
    }
  )pb");
  const EnumValueDescriptor* value = file->enum_type(0)->value(0);
  EXPECT_THAT(value->options(), EqualsProto(""));
  EXPECT_THAT(GetCoreFeatures(value), EqualsProto(R"pb(
                field_presence: EXPLICIT
                enum_type: OPEN
                repeated_field_encoding: PACKED
                utf8_validation: VERIFY
                message_encoding: LENGTH_PREFIXED
                json_format: ALLOW
                enforce_naming_style: STYLE_LEGACY
                default_symbol_visibility: EXPORT_ALL
                [pb.cpp] {
                  legacy_closed_enum: false
                  string_type: STRING
                  enum_name_uses_string_view: false
                })pb"));
}

TEST_F(FeaturesTest, EnumValueFeaturesInherit) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    dependency: "google/protobuf/unittest_features.proto"
    options { features { enum_type: CLOSED } }
    enum_type {
      name: "Foo"
      options {
        features {
          [pb.test] { enum_feature: VALUE9 }
        }
      }
      value { name: "BAR" number: 0 }
    }
  )pb");
  const EnumValueDescriptor* value = file->enum_type(0)->value(0);
  EXPECT_THAT(value->options(), EqualsProto(""));
  EXPECT_EQ(GetFeatures(value).enum_type(), FeatureSet::CLOSED);
  EXPECT_EQ(GetFeatures(value).GetExtension(pb::test).enum_feature(),
            pb::VALUE9);
}

TEST_F(FeaturesTest, EnumValueFeaturesOverride) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    dependency: "google/protobuf/unittest_features.proto"
    options {
      features {
        [pb.test] { multiple_feature: VALUE7 }
      }
    }
    enum_type {
      name: "Foo"
      options {
        features {
          [pb.test] { multiple_feature: VALUE8 }
        }
      }
      value {
        name: "BAR"
        number: 0
        options {
          features {
            [pb.test] { multiple_feature: VALUE9 enum_entry_feature: VALUE8 }
          }
        }
      }
    }
  )pb");
  const EnumValueDescriptor* value = file->enum_type(0)->value(0);
  EXPECT_THAT(value->options(), EqualsProto(""));
  EXPECT_EQ(GetFeatures(value).GetExtension(pb::test).multiple_feature(),
            pb::VALUE9);
  EXPECT_EQ(GetFeatures(value).GetExtension(pb::test).enum_entry_feature(),
            pb::VALUE8);
}

TEST_F(FeaturesTest, OneofFeaturesDefault) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    message_type {
      name: "Foo"
      field {
        name: "oneof_field"
        number: 1
        label: LABEL_OPTIONAL
        type: TYPE_INT64
        oneof_index: 0
      }
      oneof_decl { name: "foo_oneof" }
    }
  )pb");
  const OneofDescriptor* oneof = file->message_type(0)->oneof_decl(0);
  EXPECT_THAT(oneof->options(), EqualsProto(""));
  EXPECT_THAT(GetCoreFeatures(oneof), EqualsProto(R"pb(
                field_presence: EXPLICIT
                enum_type: OPEN
                repeated_field_encoding: PACKED
                utf8_validation: VERIFY
                message_encoding: LENGTH_PREFIXED
                json_format: ALLOW
                enforce_naming_style: STYLE_LEGACY
                default_symbol_visibility: EXPORT_ALL
                [pb.cpp] {
                  legacy_closed_enum: false
                  string_type: STRING
                  enum_name_uses_string_view: false
                })pb"));
}

TEST_F(FeaturesTest, OneofFeaturesInherit) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    dependency: "google/protobuf/unittest_features.proto"
    options { features { enum_type: CLOSED } }
    message_type {
      name: "Foo"
      field {
        name: "oneof_field"
        number: 1
        label: LABEL_OPTIONAL
        type: TYPE_INT64
        oneof_index: 0
      }
      oneof_decl { name: "foo_oneof" }
      options {
        features {
          [pb.test] { message_feature: VALUE9 }
        }
      }
    }
  )pb");
  const OneofDescriptor* oneof = file->message_type(0)->oneof_decl(0);
  EXPECT_THAT(oneof->options(), EqualsProto(""));
  EXPECT_EQ(GetFeatures(oneof).enum_type(), FeatureSet::CLOSED);
  EXPECT_EQ(GetFeatures(oneof).GetExtension(pb::test).message_feature(),
            pb::VALUE9);
}

TEST_F(FeaturesTest, OneofFeaturesOverride) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    dependency: "google/protobuf/unittest_features.proto"
    options {
      features {
        [pb.test] { multiple_feature: VALUE2 file_feature: VALUE4 }
      }
    }
    message_type {
      name: "Foo"
      field {
        name: "oneof_field"
        number: 1
        label: LABEL_OPTIONAL
        type: TYPE_INT64
        oneof_index: 0
      }
      oneof_decl {
        name: "foo_oneof"
        options {
          features {
            [pb.test] { multiple_feature: VALUE9 }
          }
        }
      }
      options {
        features {
          [pb.test] { multiple_feature: VALUE5 message_feature: VALUE5 }
        }
      }
    }
  )pb");
  const OneofDescriptor* oneof = file->message_type(0)->oneof_decl(0);
  EXPECT_THAT(oneof->options(), EqualsProto(""));
  EXPECT_EQ(GetFeatures(oneof).GetExtension(pb::test).multiple_feature(),
            pb::VALUE9);
  EXPECT_EQ(GetFeatures(oneof).GetExtension(pb::test).message_feature(),
            pb::VALUE5);
  EXPECT_EQ(GetFeatures(oneof).GetExtension(pb::test).file_feature(),
            pb::VALUE4);
}

TEST_F(FeaturesTest, ExtensionRangeFeaturesDefault) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    message_type {
      name: "Foo"
      extension_range { start: 1 end: 100 }
    }
  )pb");
  const Descriptor::ExtensionRange* range =
      file->message_type(0)->extension_range(0);
  EXPECT_THAT(range->options(), EqualsProto(""));
  EXPECT_THAT(GetCoreFeatures(range), EqualsProto(R"pb(
                field_presence: EXPLICIT
                enum_type: OPEN
                repeated_field_encoding: PACKED
                utf8_validation: VERIFY
                message_encoding: LENGTH_PREFIXED
                json_format: ALLOW
                enforce_naming_style: STYLE_LEGACY
                default_symbol_visibility: EXPORT_ALL
                [pb.cpp] {
                  legacy_closed_enum: false
                  string_type: STRING
                  enum_name_uses_string_view: false
                })pb"));
}

TEST_F(FeaturesTest, ExtensionRangeFeaturesInherit) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    dependency: "google/protobuf/unittest_features.proto"
    options { features { enum_type: CLOSED } }
    message_type {
      name: "Foo"
      options {
        features {
          [pb.test] { message_feature: VALUE9 }
        }
      }
      extension_range { start: 1 end: 100 }
    }
  )pb");
  const Descriptor::ExtensionRange* range =
      file->message_type(0)->extension_range(0);
  EXPECT_THAT(range->options(), EqualsProto(""));
  EXPECT_EQ(GetFeatures(range).enum_type(), FeatureSet::CLOSED);
  EXPECT_EQ(GetFeatures(range).GetExtension(pb::test).message_feature(),
            pb::VALUE9);
}

TEST_F(FeaturesTest, ExtensionRangeFeaturesOverride) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    dependency: "google/protobuf/unittest_features.proto"
    options {
      features {
        [pb.test] { multiple_feature: VALUE2 file_feature: VALUE4 }
      }
    }
    message_type {
      name: "Foo"
      options {
        features {
          [pb.test] { multiple_feature: VALUE5 message_feature: VALUE5 }
        }
      }
      extension_range {
        start: 1
        end: 100
        options {
          features {
            [pb.test] { multiple_feature: VALUE9 }
          }
        }
      }
    }
  )pb");
  const Descriptor::ExtensionRange* range =
      file->message_type(0)->extension_range(0);
  EXPECT_THAT(range->options(), EqualsProto(""));
  EXPECT_EQ(GetFeatures(range).GetExtension(pb::test).multiple_feature(),
            pb::VALUE9);
  EXPECT_EQ(GetFeatures(range).GetExtension(pb::test).message_feature(),
            pb::VALUE5);
  EXPECT_EQ(GetFeatures(range).GetExtension(pb::test).file_feature(),
            pb::VALUE4);
}

TEST_F(FeaturesTest, ServiceFeaturesDefault) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    service { name: "Foo" }
  )pb");
  const ServiceDescriptor* service = file->service(0);
  EXPECT_THAT(service->options(), EqualsProto(""));
  EXPECT_THAT(GetCoreFeatures(service), EqualsProto(R"pb(
                field_presence: EXPLICIT
                enum_type: OPEN
                repeated_field_encoding: PACKED
                utf8_validation: VERIFY
                message_encoding: LENGTH_PREFIXED
                json_format: ALLOW
                enforce_naming_style: STYLE_LEGACY
                default_symbol_visibility: EXPORT_ALL
                [pb.cpp] {
                  legacy_closed_enum: false
                  string_type: STRING
                  enum_name_uses_string_view: false
                })pb"));
}

TEST_F(FeaturesTest, ServiceFeaturesInherit) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    options { features { enum_type: CLOSED } }
    service { name: "Foo" }
  )pb");
  const ServiceDescriptor* service = file->service(0);
  EXPECT_THAT(service->options(), EqualsProto(""));
  EXPECT_EQ(GetFeatures(service).enum_type(), FeatureSet::CLOSED);
}

TEST_F(FeaturesTest, ServiceFeaturesOverride) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    dependency: "google/protobuf/unittest_features.proto"
    options {
      features {
        [pb.test] { multiple_feature: VALUE2 }
      }
    }
    service {
      name: "Foo"
      options {
        features {
          [pb.test] { multiple_feature: VALUE9 }
        }
      }
    }
  )pb");
  const ServiceDescriptor* service = file->service(0);
  EXPECT_THAT(service->options(), EqualsProto(""));
  EXPECT_EQ(GetFeatures(service).GetExtension(pb::test).multiple_feature(),
            pb::VALUE9);
}

TEST_F(FeaturesTest, MethodFeaturesDefault) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    message_type { name: "EmptyMsg" }
    service {
      name: "Foo"
      method { name: "Bar" input_type: "EmptyMsg" output_type: "EmptyMsg" }
    }
  )pb");
  const MethodDescriptor* method = file->service(0)->method(0);
  EXPECT_THAT(method->options(), EqualsProto(""));
  EXPECT_THAT(GetCoreFeatures(method), EqualsProto(R"pb(
                field_presence: EXPLICIT
                enum_type: OPEN
                repeated_field_encoding: PACKED
                utf8_validation: VERIFY
                message_encoding: LENGTH_PREFIXED
                json_format: ALLOW
                enforce_naming_style: STYLE_LEGACY
                default_symbol_visibility: EXPORT_ALL
                [pb.cpp] {
                  legacy_closed_enum: false
                  string_type: STRING
                  enum_name_uses_string_view: false
                })pb"));
}

TEST_F(FeaturesTest, MethodFeaturesInherit) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    dependency: "google/protobuf/unittest_features.proto"
    message_type { name: "EmptyMsg" }
    options { features { enum_type: CLOSED } }
    service {
      name: "Foo"
      options {
        features {
          [pb.test] { service_feature: VALUE9 }
        }
      }
      method { name: "Bar" input_type: "EmptyMsg" output_type: "EmptyMsg" }
    }
  )pb");
  const MethodDescriptor* method = file->service(0)->method(0);
  EXPECT_THAT(method->options(), EqualsProto(""));
  EXPECT_EQ(GetFeatures(method).enum_type(), FeatureSet::CLOSED);
  EXPECT_EQ(GetFeatures(method).GetExtension(pb::test).service_feature(),
            pb::VALUE9);
}

TEST_F(FeaturesTest, MethodFeaturesOverride) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    dependency: "google/protobuf/unittest_features.proto"
    message_type { name: "EmptyMsg" }
    options {
      features {
        enum_type: CLOSED
        [pb.test] { multiple_feature: VALUE2 }
      }
    }
    service {
      name: "Foo"
      options {
        features {
          [pb.test] { service_feature: VALUE4 multiple_feature: VALUE4 }
        }
      }
      method {
        name: "Bar"
        input_type: "EmptyMsg"
        output_type: "EmptyMsg"
        options {
          features {
            [pb.test] { multiple_feature: VALUE9 }
          }
        }
      }
    }
  )pb");
  const MethodDescriptor* method = file->service(0)->method(0);
  EXPECT_THAT(method->options(), EqualsProto(""));
  EXPECT_EQ(GetFeatures(method).enum_type(), FeatureSet::CLOSED);
  EXPECT_EQ(GetFeatures(method).GetExtension(pb::test).service_feature(),
            pb::VALUE4);
  EXPECT_EQ(GetFeatures(method).GetExtension(pb::test).multiple_feature(),
            pb::VALUE9);
}

TEST_F(FeaturesTest, OptionDependencyFeaturesOverride) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_99998_TEST_ONLY
    option_dependency: "google/protobuf/unittest_features.proto"
    options {
      features {
        field_presence: IMPLICIT
        [pb.test] { file_feature: VALUE7 }
      }
    }
    message_type {
      name: "Foo"
      options {
        features {
          [pb.test] { message_feature: VALUE8 }
        }
      }
      field {
        name: "bar"
        number: 1
        type: TYPE_STRING
        options {
          features {
            [pb.test] { field_feature: VALUE9 }
          }
        }
      }
    }
  )pb");
  EXPECT_THAT(file->options(), EqualsProto(""));
  EXPECT_EQ(GetFeatures(file).GetExtension(pb::test).file_feature(),
            pb::VALUE7);
  EXPECT_EQ(GetFeatures(file->message_type(0))
                .GetExtension(pb::test)
                .message_feature(),
            pb::VALUE8);
  EXPECT_EQ(GetFeatures(file->message_type(0)->field(0))
                .GetExtension(pb::test)
                .field_feature(),
            pb::VALUE9);
}

TEST_F(FeaturesTest, FieldFeatureHelpers) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    message_type {
      name: "Foo"
      field { name: "def" number: 1 label: LABEL_OPTIONAL type: TYPE_STRING }
      field { name: "rep" number: 2 label: LABEL_REPEATED type: TYPE_INT32 }
      field {
        name: "implicit_field"
        number: 3
        label: LABEL_OPTIONAL
        type: TYPE_STRING
        options { features { field_presence: IMPLICIT } }
      }
      field {
        name: "required_field"
        number: 4
        label: LABEL_OPTIONAL
        type: TYPE_STRING
        options { features { field_presence: LEGACY_REQUIRED } }
      }
      field {
        name: "required_message_field"
        number: 5
        label: LABEL_OPTIONAL
        type: TYPE_MESSAGE
        type_name: "Foo"
        options { features { field_presence: LEGACY_REQUIRED } }
      }
      field {
        name: "expanded_field"
        number: 6
        label: LABEL_REPEATED
        type: TYPE_STRING
        options { features { repeated_field_encoding: EXPANDED } }
      }
      field {
        name: "utf8_verify_field"
        number: 7
        label: LABEL_REPEATED
        type: TYPE_STRING
        options { features { utf8_validation: NONE } }
      }
    }
  )pb");
  const Descriptor* message = file->message_type(0);
  const FieldDescriptor* default_field = message->field(0);
  const FieldDescriptor* default_repeated_field = message->field(1);
  const FieldDescriptor* implicit_field = message->field(2);
  const FieldDescriptor* required_field = message->field(3);
  const FieldDescriptor* required_message_field = message->field(4);
  const FieldDescriptor* expanded_field = message->field(5);
  const FieldDescriptor* utf8_verify_field = message->field(6);

  EXPECT_FALSE(default_field->is_packed());
  EXPECT_FALSE(default_field->is_required());
  EXPECT_TRUE(default_field->has_presence());
  EXPECT_TRUE(default_field->requires_utf8_validation());
  EXPECT_EQ(GetUtf8CheckMode(default_field, /*is_lite=*/false),
            Utf8CheckMode::kStrict);
  EXPECT_EQ(GetUtf8CheckMode(default_field, /*is_lite=*/true),
            Utf8CheckMode::kStrict);

  EXPECT_TRUE(default_repeated_field->is_packed());
  EXPECT_FALSE(default_repeated_field->has_presence());
  EXPECT_FALSE(default_repeated_field->requires_utf8_validation());
  EXPECT_EQ(GetUtf8CheckMode(default_repeated_field, /*is_lite=*/false),
            Utf8CheckMode::kNone);
  EXPECT_EQ(GetUtf8CheckMode(default_repeated_field, /*is_lite=*/true),
            Utf8CheckMode::kNone);

  EXPECT_TRUE(required_field->has_presence());
  EXPECT_TRUE(required_field->is_required());
  EXPECT_TRUE(required_message_field->has_presence());
  EXPECT_TRUE(required_message_field->is_required());

  EXPECT_FALSE(implicit_field->has_presence());
  EXPECT_FALSE(expanded_field->is_packed());
  EXPECT_FALSE(utf8_verify_field->requires_utf8_validation());
  EXPECT_EQ(GetUtf8CheckMode(utf8_verify_field, /*is_lite=*/false),
            Utf8CheckMode::kVerify);
  EXPECT_EQ(GetUtf8CheckMode(utf8_verify_field, /*is_lite=*/true),
            Utf8CheckMode::kNone);
  EXPECT_EQ(GetUtf8CheckMode(utf8_verify_field, /*is_lite=*/false),
            Utf8CheckMode::kVerify);
  EXPECT_EQ(GetUtf8CheckMode(utf8_verify_field, /*is_lite=*/true),
            Utf8CheckMode::kNone);
}

TEST_F(FeaturesTest, EnumFeatureHelpers) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::CppFeatures::GetDescriptor()->file());
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    dependency: "google/protobuf/cpp_features.proto"
    edition: EDITION_2023
    enum_type {
      name: "FooOpen"
      value { name: "BAR" number: 0 }
    }
    enum_type {
      name: "FooClosed"
      value { name: "BAZ" number: 0 }
      options { features { enum_type: CLOSED } }
    }
    message_type {
      name: "FooMessage"
      field {
        name: "open"
        number: 1
        label: LABEL_OPTIONAL
        type: TYPE_ENUM
        type_name: "FooOpen"
      }
      field {
        name: "closed"
        number: 2
        label: LABEL_OPTIONAL
        type: TYPE_ENUM
        type_name: "FooClosed"
      }
      field {
        name: "legacy_closed"
        number: 3
        label: LABEL_OPTIONAL
        type: TYPE_ENUM
        type_name: "FooOpen"
        options {
          features {
            [pb.cpp] { legacy_closed_enum: true }
          }
        }
      }
    }
  )pb");
  const EnumDescriptor* open = file->enum_type(0);
  const EnumDescriptor* closed = file->enum_type(1);
  const FieldDescriptor* field_open = file->message_type(0)->field(0);
  const FieldDescriptor* field_closed = file->message_type(0)->field(1);
  const FieldDescriptor* field_legacy_closed = file->message_type(0)->field(2);
  ASSERT_EQ(field_legacy_closed->enum_type(), field_open->enum_type());

  EXPECT_FALSE(open->is_closed());
  EXPECT_TRUE(closed->is_closed());
  EXPECT_FALSE(field_open->legacy_enum_field_treated_as_closed());
  EXPECT_TRUE(field_closed->legacy_enum_field_treated_as_closed());
  EXPECT_TRUE(field_legacy_closed->legacy_enum_field_treated_as_closed());
  EXPECT_TRUE(HasPreservingUnknownEnumSemantics(field_open));
  EXPECT_FALSE(HasPreservingUnknownEnumSemantics(field_closed));
  EXPECT_FALSE(HasPreservingUnknownEnumSemantics(field_legacy_closed));
}

TEST_F(FeaturesTest, FieldCppStringType) {
  BuildDescriptorMessagesInTestPool();
  const std::string file_contents = absl::Substitute(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2024
        message_type {
          name: "Foo"
          field {
            name: "view"
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_STRING
          }
          field {
            name: "str"
            number: 2
            label: LABEL_OPTIONAL
            type: TYPE_STRING
            options {
              features {
                [pb.cpp] { string_type: STRING }
              }
            }
          }
          field {
            name: "cord"
            number: 3
            label: LABEL_OPTIONAL
            type: TYPE_STRING
            options {
              features {
                [pb.cpp] { string_type: CORD }
              }
            }
          }
          field {
            name: "cord_bytes"
            number: 4
            label: LABEL_OPTIONAL
            type: TYPE_BYTES
            options {
              features {
                [pb.cpp] { string_type: CORD }
              }
            }
          } $0
          extension_range { start: 100 end: 200 }
        }
        extension {
          name: "cord_ext"
          number: 100
          label: LABEL_OPTIONAL
          type: TYPE_STRING
          options {
            features {
              [pb.cpp] { string_type: CORD }
            }
          }
          extendee: "Foo"
        }
      )pb",
      ""
  );
  const FileDescriptor* file = BuildFile(file_contents);
  const Descriptor* message = file->message_type(0);
  const FieldDescriptor* view = message->field(0);
  const FieldDescriptor* str = message->field(1);
  const FieldDescriptor* cord = message->field(2);
  const FieldDescriptor* cord_bytes = message->field(3);
  const FieldDescriptor* cord_ext = file->extension(0);

  EXPECT_EQ(view->cpp_string_type(), FieldDescriptor::CppStringType::kView);
  EXPECT_EQ(str->cpp_string_type(), FieldDescriptor::CppStringType::kString);
  EXPECT_EQ(cord_bytes->cpp_string_type(),
            FieldDescriptor::CppStringType::kCord);
  EXPECT_EQ(cord->cpp_string_type(), FieldDescriptor::CppStringType::kString);
  EXPECT_EQ(cord_ext->cpp_string_type(),
            FieldDescriptor::CppStringType::kString);

}

TEST_F(FeaturesTest, MergeFeatureValidationFailed) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        dependency: "google/protobuf/unittest_features.proto"
        options { features { field_presence: FIELD_PRESENCE_UNKNOWN } }
      )pb",
      "foo.proto: foo.proto: EDITIONS: Feature field "
      "`field_presence` must resolve to a known value, found "
      "FIELD_PRESENCE_UNKNOWN\n");
}

TEST_F(FeaturesTest, FeaturesOutsideEditions) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        syntax: "proto2"
        dependency: "google/protobuf/unittest_features.proto"
        options { features { field_presence: IMPLICIT } }
      )pb",
      "foo.proto: foo.proto: EDITIONS: Features are only valid under "
      "editions.\n");
}

TEST_F(FeaturesTest, InvalidFileRequiredPresence) {
  BuildDescriptorMessagesInTestPool();
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        options { features { field_presence: LEGACY_REQUIRED } }
      )pb",
      "foo.proto: foo.proto: EDITIONS: Required presence can't be specified "
      "by default.\n");
}

TEST_F(FeaturesTest, InvalidFileJavaStringCheckUtf8) {
  BuildDescriptorMessagesInTestPool();
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        options { java_string_check_utf8: true }
      )pb",
      "foo.proto: foo.proto: EDITIONS: File option java_string_check_utf8 is "
      "not allowed under editions. Use the (pb.java).utf8_validation feature "
      "to control this behavior.\n");
}

TEST_F(FeaturesTest, Proto2FileJavaStringCheckUtf8) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = BuildFile(
      R"pb(
        name: "foo.proto"
        syntax: "proto2"
        options { java_string_check_utf8: true }
      )pb");
  EXPECT_EQ(file->options().java_string_check_utf8(), true);
}
TEST_F(FeaturesTest, InvalidFieldPacked) {
  BuildDescriptorMessagesInTestPool();
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        message_type {
          name: "Foo"
          field {
            name: "bar"
            number: 1
            label: LABEL_REPEATED
            type: TYPE_INT64
            options { packed: true }
          }
        }
      )pb",
      "foo.proto: Foo.bar: NAME: Field option packed is not allowed under "
      "editions.  Use the repeated_field_encoding feature to control this "
      "behavior.\n");
}

TEST_F(FeaturesTest, NoCtypeFromEdition2024) {
  BuildDescriptorMessagesInTestPool();
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2024
        message_type {
          name: "Foo"
          field { name: "foo" number: 1 label: LABEL_OPTIONAL type: TYPE_INT32 }
          field {
            name: "bar"
            number: 2
            label: LABEL_OPTIONAL
            type: TYPE_STRING
            options { ctype: CORD }
          }
        }
      )pb",
      "foo.proto: Foo.bar: TYPE: ctype option is not allowed under edition "
      "2024 and beyond. Use the feature string_type = VIEW|CORD|STRING|... "
      "instead.\n");
}


TEST_F(FeaturesTest, InvalidFieldImplicitDefault) {
  BuildDescriptorMessagesInTestPool();
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        message_type {
          name: "Foo"
          field {
            name: "bar"
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_STRING
            default_value: "Hello world"
            options { features { field_presence: IMPLICIT } }
          }
        }
      )pb",
      "foo.proto: Foo.bar: NAME: Implicit presence fields can't specify "
      "defaults.\n");
}

TEST_F(FeaturesTest, ValidExtensionFieldImplicitDefault) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = BuildFile(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        options { features { field_presence: IMPLICIT } }
        message_type {
          name: "Foo"
          extension_range { start: 1 end: 100 }
        }
        extension {
          name: "bar"
          number: 1
          label: LABEL_OPTIONAL
          type: TYPE_STRING
          default_value: "Hello world"
          extendee: "Foo"
        }
      )pb");
  ASSERT_THAT(file, NotNull());

  EXPECT_TRUE(file->extension(0)->has_presence());
  EXPECT_EQ(file->extension(0)->default_value_string(), "Hello world");
}

TEST_F(FeaturesTest, ValidOneofFieldImplicitDefault) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = BuildFile(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        options { features { field_presence: IMPLICIT } }
        message_type {
          name: "Foo"
          field {
            name: "bar"
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_STRING
            default_value: "Hello world"
            oneof_index: 0
          }
          oneof_decl { name: "_foo" }
        }
      )pb");
  ASSERT_THAT(file, NotNull());

  EXPECT_TRUE(file->message_type(0)->field(0)->has_presence());
  EXPECT_EQ(file->message_type(0)->field(0)->default_value_string(),
            "Hello world");
}

TEST_F(FeaturesTest, InvalidFieldImplicitClosed) {
  BuildDescriptorMessagesInTestPool();
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        message_type {
          name: "Foo"
          field {
            name: "bar"
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_ENUM
            type_name: "Enum"
            options { features { field_presence: IMPLICIT } }
          }
        }
        enum_type {
          name: "Enum"
          value { name: "BAR" number: 0 }
          options { features { enum_type: CLOSED } }
        }
      )pb",
      "foo.proto: Foo.bar: NAME: Implicit presence enum fields must always "
      "be open.\n");
}

TEST_F(FeaturesTest, ValidRepeatedFieldImplicitClosed) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = BuildFile(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        options { features { field_presence: IMPLICIT } }
        message_type {
          name: "Foo"
          field {
            name: "bar"
            number: 1
            label: LABEL_REPEATED
            type: TYPE_ENUM
            type_name: "Enum"
          }
        }
        enum_type {
          name: "Enum"
          value { name: "BAR" number: 0 }
          options { features { enum_type: CLOSED } }
        }
      )pb");
  ASSERT_THAT(file, NotNull());

  EXPECT_FALSE(file->message_type(0)->field(0)->has_presence());
  EXPECT_TRUE(file->enum_type(0)->is_closed());
}

TEST_F(FeaturesTest, ValidOneofFieldImplicitClosed) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = BuildFile(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        options { features { field_presence: IMPLICIT } }
        message_type {
          name: "Foo"
          field {
            name: "bar"
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_ENUM
            type_name: "Enum"
            oneof_index: 0
          }
          oneof_decl { name: "_foo" }
        }
        enum_type {
          name: "Enum"
          value { name: "BAR" number: 0 }
          options { features { enum_type: CLOSED } }
        }
      )pb");
  ASSERT_THAT(file, NotNull());

  EXPECT_TRUE(file->message_type(0)->field(0)->has_presence());
  EXPECT_TRUE(file->enum_type(0)->is_closed());
}

TEST_F(FeaturesTest, InvalidFieldRequiredExtension) {
  BuildDescriptorMessagesInTestPool();
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        message_type {
          name: "Foo"
          extension_range { start: 1 end: 100 }
        }
        extension {
          name: "bar"
          number: 1
          label: LABEL_OPTIONAL
          type: TYPE_STRING
          options { features { field_presence: LEGACY_REQUIRED } }
          extendee: "Foo"
        }
      )pb",
      "foo.proto: bar: NAME: Extensions can't be required.\n");
}

TEST_F(FeaturesTest, InvalidFieldImplicitExtension) {
  BuildDescriptorMessagesInTestPool();
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        message_type {
          name: "Foo"
          extension_range { start: 1 end: 100 }
        }
        extension {
          name: "bar"
          number: 1
          label: LABEL_OPTIONAL
          type: TYPE_STRING
          options { features { field_presence: IMPLICIT } }
          extendee: "Foo"
        }
      )pb",
      "foo.proto: bar: NAME: Extensions can't specify field presence.\n");
}

TEST_F(FeaturesTest, InvalidFieldMessageImplicit) {
  BuildDescriptorMessagesInTestPool();
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        message_type {
          name: "Foo"
          field {
            name: "bar"
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_MESSAGE
            type_name: "Foo"
            options { features { field_presence: IMPLICIT } }
          }
        }
      )pb",
      "foo.proto: Foo.bar: NAME: Message fields can't specify implicit "
      "presence.\n");
}

TEST_F(FeaturesTest, InvalidFieldRepeatedImplicit) {
  BuildDescriptorMessagesInTestPool();
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        message_type {
          name: "Foo"
          field {
            name: "bar"
            number: 1
            label: LABEL_REPEATED
            type: TYPE_STRING
            options { features { field_presence: IMPLICIT } }
          }
        }
      )pb",
      "foo.proto: Foo.bar: NAME: Repeated fields can't specify field "
      "presence.\n");
}

TEST_F(FeaturesTest, InvalidFieldMapImplicit) {
  constexpr absl::string_view kProtoFile = R"schema(
    edition = "2023";

    message Foo {
      map<string, Foo> bar = 1 [
        features.field_presence = IMPLICIT
      ];
    }
  )schema";
  io::ArrayInputStream input_stream(kProtoFile.data(), kProtoFile.size());
  SimpleErrorCollector error_collector;
  io::Tokenizer tokenizer(&input_stream, &error_collector);
  compiler::Parser parser;
  parser.RecordErrorsTo(&error_collector);
  FileDescriptorProto proto;
  ASSERT_TRUE(parser.Parse(&tokenizer, &proto))
      << error_collector.last_error() << "\n"
      << kProtoFile;
  ASSERT_EQ("", error_collector.last_error());
  proto.set_name("foo.proto");

  BuildDescriptorMessagesInTestPool();
  BuildFileWithErrors(proto,
                      "foo.proto: Foo.bar: NAME: Repeated fields can't specify "
                      "field presence.\n");
}

TEST_F(FeaturesTest, InvalidFieldOneofImplicit) {
  BuildDescriptorMessagesInTestPool();
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        message_type {
          name: "Foo"
          field {
            name: "bar"
            number: 1
            oneof_index: 0
            label: LABEL_OPTIONAL
            type: TYPE_INT64
            options { features { field_presence: IMPLICIT } }
          }
          oneof_decl { name: "_foo" }
        }
      )pb",
      "foo.proto: Foo.bar: NAME: Oneof fields can't specify field presence.\n");
}

TEST_F(FeaturesTest, InvalidFieldRepeatedRequired) {
  BuildDescriptorMessagesInTestPool();
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        message_type {
          name: "Foo"
          field {
            name: "bar"
            number: 1
            label: LABEL_REPEATED
            type: TYPE_STRING
            options { features { field_presence: LEGACY_REQUIRED } }
          }
        }
      )pb",
      "foo.proto: Foo.bar: NAME: Repeated fields can't specify field "
      "presence.\n");
}

TEST_F(FeaturesTest, InvalidFieldOneofRequired) {
  BuildDescriptorMessagesInTestPool();
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        message_type {
          name: "Foo"
          field {
            name: "bar"
            number: 1
            oneof_index: 0
            label: LABEL_OPTIONAL
            type: TYPE_INT64
            options { features { field_presence: LEGACY_REQUIRED } }
          }
          oneof_decl { name: "_foo" }
        }
      )pb",
      "foo.proto: Foo.bar: NAME: Oneof fields can't specify field presence.\n");
}

TEST_F(FeaturesTest, InvalidFieldNonStringWithStringValidation) {
  BuildDescriptorMessagesInTestPool();
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        message_type {
          name: "Foo"
          field {
            name: "bar"
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_INT64
            options { features { utf8_validation: NONE } }
          }
        }
      )pb",
      "foo.proto: Foo.bar: NAME: Only string fields can specify "
      "utf8 validation.\n");
}

TEST_F(FeaturesTest, InvalidFieldNonStringMapWithStringValidation) {
  BuildDescriptorMessagesInTestPool();
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        message_type {
          name: "Foo"
          nested_type {
            name: "MapFieldEntry"
            field {
              name: "key"
              number: 1
              label: LABEL_OPTIONAL
              type: TYPE_INT32
              options {
                uninterpreted_option {
                  name { name_part: "features" is_extension: false }
                  name { name_part: "utf8_validation" is_extension: false }
                  identifier_value: "NONE"
                }
              }
            }
            field {
              name: "value"
              number: 2
              label: LABEL_OPTIONAL
              type: TYPE_INT32
              options {
                uninterpreted_option {
                  name { name_part: "features" is_extension: false }
                  name { name_part: "utf8_validation" is_extension: false }
                  identifier_value: "NONE"
                }
              }
            }
            options { map_entry: true }
          }
          field {
            name: "map_field"
            number: 1
            label: LABEL_REPEATED
            type_name: "MapFieldEntry"
            options {
              uninterpreted_option {
                name { name_part: "features" is_extension: false }
                name { name_part: "utf8_validation" is_extension: false }
                identifier_value: "NONE"
              }
            }
          }
        }
      )pb",
      "foo.proto: Foo.map_field: NAME: Only string fields can specify "
      "utf8 validation.\n");
}

TEST_F(FeaturesTest, InvalidFieldNonRepeatedWithRepeatedEncoding) {
  BuildDescriptorMessagesInTestPool();
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        message_type {
          name: "Foo"
          field {
            name: "bar"
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_INT64
            options { features { repeated_field_encoding: EXPANDED } }
          }
        }
      )pb",
      "foo.proto: Foo.bar: NAME: Only repeated fields can specify repeated "
      "field encoding.\n");
}

TEST_F(FeaturesTest, InvalidFieldNonPackableWithPackedRepeatedEncoding) {
  BuildDescriptorMessagesInTestPool();
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        message_type {
          name: "Foo"
          field {
            name: "bar"
            number: 1
            label: LABEL_REPEATED
            type: TYPE_STRING
            options { features { repeated_field_encoding: PACKED } }
          }
        }
      )pb",
      "foo.proto: Foo.bar: NAME: Only repeated primitive fields can specify "
      "PACKED repeated field encoding.\n");
}

TEST_F(FeaturesTest, InvalidFieldNonMessageWithMessageEncoding) {
  BuildDescriptorMessagesInTestPool();
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        message_type {
          name: "Foo"
          field {
            name: "bar"
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_INT64
            options { features { message_encoding: DELIMITED } }
          }
        }
      )pb",
      "foo.proto: Foo.bar: NAME: Only message fields can specify message "
      "encoding.\n");
}

TEST_F(FeaturesTest, InvalidFieldMapWithMessageEncoding) {
  constexpr absl::string_view kProtoFile = R"schema(
    edition = "2023";

    message Foo {
      map<string, Foo> bar = 1 [
        features.message_encoding = DELIMITED
      ];
    }
  )schema";
  io::ArrayInputStream input_stream(kProtoFile.data(), kProtoFile.size());
  SimpleErrorCollector error_collector;
  io::Tokenizer tokenizer(&input_stream, &error_collector);
  compiler::Parser parser;
  parser.RecordErrorsTo(&error_collector);
  FileDescriptorProto proto;
  ASSERT_TRUE(parser.Parse(&tokenizer, &proto))
      << error_collector.last_error() << "\n"
      << kProtoFile;
  ASSERT_EQ("", error_collector.last_error());
  proto.set_name("foo.proto");

  BuildDescriptorMessagesInTestPool();
  BuildFileWithErrors(
      proto,
      "foo.proto: Foo.bar: NAME: Only message fields can specify message "
      "encoding.\n");
}

TEST_F(FeaturesTest, InvalidOpenEnumNonZeroFirstValue) {
  BuildDescriptorMessagesInTestPool();
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        enum_type {
          name: "Enum"
          value { name: "BAR" number: 1 }
          options { features { enum_type: OPEN } }
        }
      )pb",
      "foo.proto: Enum: NUMBER: The first enum value must be zero for open "
      "enums.\n");
}

TEST_F(FeaturesTest, InvalidUseFeaturesInSameFile) {
  BuildDescriptorMessagesInTestPool();
  ParseAndBuildFileWithErrors("foo.proto", R"schema(
    edition = "2023";

    package test;
    import "google/protobuf/descriptor.proto";

    message Foo {
      string bar = 1 [
        features.(test.custom).foo = "xyz",
        features.(test.another) = {foo: -321}
      ];
    }

    message Custom {
      string foo = 1 [features = { [test.custom]: {foo: "abc"} }];
    }
    message Another {
      Enum foo = 1;
    }

    enum Enum {
      option features.enum_type = CLOSED;
      ZERO = 0;
      ONE = 1;
    }

    extend google.protobuf.FeatureSet {
      Custom custom = 1002 [features.message_encoding=DELIMITED];
      Another another = 1001;
    }
  )schema",
                              "foo.proto: test.Foo.bar: OPTION_NAME: Feature "
                              "\"features.(test.custom)\" can't be used in the "
                              "same file it's defined in.\n");
}

TEST_F(FeaturesTest, ClosedEnumNonZeroFirstValue) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = BuildFile(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        enum_type {
          name: "Enum"
          value { name: "BAR" number: 9 }
          options { features { enum_type: CLOSED } }
        }
      )pb");

  EXPECT_EQ(file->enum_type(0)->value(0)->number(), 9);
}

TEST_F(FeaturesTest, CopyToIncludesFeatures) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    dependency: "google/protobuf/unittest_features.proto"
    options {
      java_package: "pkg"
      features { field_presence: IMPLICIT }
    }
    message_type {
      name: "Foo"
      options {
        features {
          [pb.test] { multiple_feature: VALUE9 }
        }
      }
      field {
        name: "bar"
        number: 1
        label: LABEL_REPEATED
        type: TYPE_INT64
        options { features { repeated_field_encoding: EXPANDED } }
      }
    }
  )pb");
  FileDescriptorProto proto;
  file->CopyTo(&proto);
  EXPECT_THAT(proto.options(),
              EqualsProto(R"pb(java_package: "pkg"
                               features { field_presence: IMPLICIT })pb"));
  EXPECT_THAT(proto.message_type(0).options(),
              EqualsProto(R"pb(features {
                                 [pb.test] { multiple_feature: VALUE9 }
                               })pb"));
  EXPECT_THAT(
      proto.message_type(0).field(0).options(),
      EqualsProto(R"pb(features { repeated_field_encoding: EXPANDED })pb"));
}

TEST_F(FeaturesTest, UninterpretedOptions) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    options {
      uninterpreted_option {
        name { name_part: "features" is_extension: false }
        name { name_part: "field_presence" is_extension: false }
        identifier_value: "IMPLICIT"
      }
    }
  )pb");
  EXPECT_THAT(file->options(), EqualsProto(""));
  EXPECT_THAT(GetCoreFeatures(file), EqualsProto(R"pb(
                field_presence: IMPLICIT
                enum_type: OPEN
                repeated_field_encoding: PACKED
                utf8_validation: VERIFY
                message_encoding: LENGTH_PREFIXED
                json_format: ALLOW
                enforce_naming_style: STYLE_LEGACY
                default_symbol_visibility: EXPORT_ALL
                [pb.cpp] {
                  legacy_closed_enum: false
                  string_type: STRING
                  enum_name_uses_string_view: false
                })pb"));
}

TEST_F(FeaturesTest, UninterpretedOptionsMerge) {
  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    options {
      uninterpreted_option {
        name { name_part: "features" is_extension: false }
        name { name_part: "enum_type" is_extension: false }
        identifier_value: "CLOSED"
      }
    }
    message_type {
      name: "Foo"
      field {
        name: "bar"
        number: 1
        label: LABEL_OPTIONAL
        type: TYPE_STRING
        options {
          uninterpreted_option {
            name { name_part: "features" is_extension: false }
            name { name_part: "enum_type" is_extension: false }
            identifier_value: "OPEN"
          }
        }
      }
    }
  )pb");
  const FieldDescriptor* field = file->message_type(0)->field(0);
  EXPECT_THAT(file->options(), EqualsProto(""));
  EXPECT_THAT(field->options(), EqualsProto(""));
  EXPECT_EQ(GetFeatures(file).enum_type(), FeatureSet::CLOSED);
  EXPECT_EQ(GetFeatures(field).enum_type(), FeatureSet::OPEN);
}

TEST_F(FeaturesTest, UninterpretedOptionsMergeExtension) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file = BuildFile(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_2023
    dependency: "google/protobuf/unittest_features.proto"
    options {
      uninterpreted_option {
        name { name_part: "features" is_extension: false }
        name { name_part: "pb.test" is_extension: true }
        name { name_part: "multiple_feature" is_extension: false }
        identifier_value: "VALUE5"
      }
      uninterpreted_option {
        name { name_part: "features" is_extension: false }
        name { name_part: "pb.test" is_extension: true }
        name { name_part: "file_feature" is_extension: false }
        identifier_value: "VALUE5"
      }
    }
    message_type {
      name: "Foo"
      options {
        uninterpreted_option {
          name { name_part: "features" is_extension: false }
          name { name_part: "pb.test" is_extension: true }
          name { name_part: "multiple_feature" is_extension: false }
          identifier_value: "VALUE6"
        }
        uninterpreted_option {
          name { name_part: "features" is_extension: false }
          name { name_part: "pb.test" is_extension: true }
          name { name_part: "message_feature" is_extension: false }
          identifier_value: "VALUE6"
        }
      }
      field {
        name: "bar"
        number: 1
        label: LABEL_OPTIONAL
        type: TYPE_STRING
        options {
          uninterpreted_option {
            name { name_part: "features" is_extension: false }
            name { name_part: "pb.test" is_extension: true }
            name { name_part: "multiple_feature" is_extension: false }
            identifier_value: "VALUE7"
          }
          uninterpreted_option {
            name { name_part: "features" is_extension: false }
            name { name_part: "pb.test" is_extension: true }
            name { name_part: "field_feature" is_extension: false }
            identifier_value: "VALUE7"
          }
        }
      }
    }
  )pb");
  const FieldDescriptor* field = file->message_type(0)->field(0);
  EXPECT_THAT(field->options(), EqualsProto(""));
  EXPECT_EQ(GetFeatures(field).GetExtension(pb::test).file_feature(),
            pb::VALUE5);
  EXPECT_EQ(GetFeatures(field).GetExtension(pb::test).message_feature(),
            pb::VALUE6);
  EXPECT_EQ(GetFeatures(field).GetExtension(pb::test).field_feature(),
            pb::VALUE7);
  EXPECT_EQ(GetFeatures(field).GetExtension(pb::test).multiple_feature(),
            pb::VALUE7);
}

TEST_F(FeaturesTest, InvalidJsonUniquenessDefaultWarning) {
  BuildFileWithWarnings(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        message_type {
          name: "Foo"
          field {
            name: "bar"
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_STRING
          }
          field {
            name: "bar_"
            number: 2
            label: LABEL_OPTIONAL
            type: TYPE_STRING
          }
          options { features { json_format: LEGACY_BEST_EFFORT } }
        }
      )pb",
      "foo.proto: Foo: NAME: The default JSON name of field \"bar_\" (\"bar\") "
      "conflicts with the default JSON name of field \"bar\".\n");
}

TEST_F(FeaturesTest, InvalidJsonUniquenessDefaultError) {
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        message_type {
          name: "Foo"
          field {
            name: "bar"
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_STRING
          }
          field {
            name: "bar_"
            number: 2
            label: LABEL_OPTIONAL
            type: TYPE_STRING
          }
          options { features { json_format: ALLOW } }
        }
      )pb",
      "foo.proto: Foo: NAME: The default JSON name of field \"bar_\" (\"bar\") "
      "conflicts with the default JSON name of field \"bar\".\n");
}

TEST_F(FeaturesTest, InvalidJsonUniquenessCustomError) {
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        message_type {
          name: "Foo"
          field {
            name: "bar"
            json_name: "baz"
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_STRING
          }
          field {
            name: "bar2"
            json_name: "baz"
            number: 2
            label: LABEL_OPTIONAL
            type: TYPE_STRING
          }
          options { features { json_format: LEGACY_BEST_EFFORT } }
        }
      )pb",
      "foo.proto: Foo: NAME: The custom JSON name of field \"bar2\" (\"baz\") "
      "conflicts with the custom JSON name of field \"bar\".\n");
}

TEST_F(FeaturesTest, InvalidRequiredLabel) {
  BuildDescriptorMessagesInTestPool();
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        message_type {
          name: "Foo"
          field {
            name: "bar"
            number: 1
            label: LABEL_REQUIRED
            type: TYPE_STRING
          }
        }
      )pb",
      "foo.proto: Foo.bar: NAME: Required label is not allowed under editions. "
      " Use the feature field_presence = LEGACY_REQUIRED to control this "
      "behavior.\n");
}

TEST_F(FeaturesTest, InvalidGroupLabel) {
  BuildDescriptorMessagesInTestPool();
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        message_type {
          name: "Foo"
          field {
            name: "bar"
            number: 1
            type_name: ".Foo"
            label: LABEL_OPTIONAL
            type: TYPE_GROUP
          }
        }
      )pb",
      "foo.proto: Foo.bar: NAME: Group types are not allowed under editions.  "
      "Use the feature message_encoding = DELIMITED to control this "
      "behavior.\n");
}

TEST_F(FeaturesTest, DeprecatedFeature) {
  pool_.AddDirectInputFile("foo.proto");
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  BuildFileWithWarnings(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        dependency: "google/protobuf/unittest_features.proto"
        options {
          uninterpreted_option {
            name { name_part: "features" is_extension: false }
            name { name_part: "pb.test" is_extension: true }
            name { name_part: "removed_feature" is_extension: false }
            identifier_value: "VALUE9"
          }
        }
      )pb",
      "foo.proto: foo.proto: NAME: Feature "
      "pb.TestFeatures.removed_feature has been deprecated in edition 2023: "
      "Custom feature deprecation warning\n");
  const FileDescriptor* file = pool_.FindFileByName("foo.proto");
  ASSERT_THAT(file, NotNull());

  EXPECT_EQ(GetFeatures(file).GetExtension(pb::test).removed_feature(),
            pb::VALUE9);
}

TEST_F(FeaturesTest, IgnoreDeprecatedFeature) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  BuildFileWithWarnings(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        dependency: "google/protobuf/unittest_features.proto"
        options {
          uninterpreted_option {
            name { name_part: "features" is_extension: false }
            name { name_part: "pb.test" is_extension: true }
            name { name_part: "removed_feature" is_extension: false }
            identifier_value: "VALUE9"
          }
        }
      )pb",
      "");
}

TEST_F(FeaturesTest, IgnoreTransitiveFeature) {
  pool_.AddDirectInputFile("bar.proto");
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  BuildFileWithWarnings(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        dependency: "google/protobuf/unittest_features.proto"
        options {
          uninterpreted_option {
            name { name_part: "features" is_extension: false }
            name { name_part: "pb.test" is_extension: true }
            name { name_part: "removed_feature" is_extension: false }
            identifier_value: "VALUE9"
          }
        }
        message_type { name: "Foo" }
      )pb",
      "");
  BuildFileWithWarnings(
      R"pb(
        name: "bar.proto"
        syntax: "editions"
        edition: EDITION_2023
        dependency: "foo.proto"
        message_type {
          name: "Bar"
          field {
            name: "bar"
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_MESSAGE
            type_name: ".Foo"
          }
        }
      )pb",
      "");
}

TEST_F(FeaturesTest, RemovedFeature) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2024
        dependency: "google/protobuf/unittest_features.proto"
        options {
          features {
            [pb.test] { removed_feature: VALUE9 }
          }
        }
      )pb",
      "foo.proto: foo.proto: NAME: Feature "
      "pb.TestFeatures.removed_feature has been removed in edition 2024 and "
      "can't be used in edition 2024\n");
}

TEST_F(FeaturesTest, RemovedFeatureDefault) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file =
      BuildFile(R"pb(
        name: "foo.proto" syntax: "editions" edition: EDITION_2024
      )pb");
  ASSERT_THAT(file, NotNull());
  EXPECT_EQ(GetFeatures(file).GetExtension(pb::test).removed_feature(),
            pb::VALUE3);
}

TEST_F(FeaturesTest, FutureFeature) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        syntax: "editions"
        edition: EDITION_2023
        dependency: "google/protobuf/unittest_features.proto"
        options {
          features {
            [pb.test] { future_feature: VALUE9 }
          }
        }
      )pb",
      "foo.proto: foo.proto: NAME: Feature "
      "pb.TestFeatures.future_feature wasn't introduced until edition 2024 and "
      "can't be used in edition 2023\n");
}

TEST_F(FeaturesTest, FutureFeatureDefault) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file =
      BuildFile(R"pb(
        name: "foo.proto" syntax: "editions" edition: EDITION_2023
      )pb");
  ASSERT_THAT(file, NotNull());
  EXPECT_EQ(GetFeatures(file).GetExtension(pb::test).future_feature(),
            pb::VALUE1);
}

// Test that the result of FileDescriptor::DebugString() can be used to create
// the original descriptors.
class FeaturesDebugStringTest
    : public FeaturesTest,
      public testing::WithParamInterface<
          std::tuple<absl::string_view, absl::string_view>> {
 protected:
  const FileDescriptor* LoadFile(absl::string_view name,
                                 absl::string_view content) {
    io::ArrayInputStream input_stream(content.data(), content.size());
    SimpleErrorCollector error_collector;
    io::Tokenizer tokenizer(&input_stream, &error_collector);
    compiler::Parser parser;
    parser.RecordErrorsTo(&error_collector);
    FileDescriptorProto proto;
    ABSL_CHECK(parser.Parse(&tokenizer, &proto))
        << error_collector.last_error() << "\n"
        << content;
    ABSL_CHECK_EQ("", error_collector.last_error());
    proto.set_name(name);
    return ABSL_DIE_IF_NULL(roundtrip_pool_.BuildFile(proto));
  }

  std::string GetFileProto() { return std::string(std::get<1>(GetParam())); }

  auto EqualsRoundTrip() { return EqualsProto(std::get<1>(GetParam())); }

 private:
  DescriptorPool roundtrip_pool_;
};

TEST_P(FeaturesDebugStringTest, RoundTrip) {
  BuildDescriptorMessagesInTestPool();
  BuildFileInTestPool(pb::TestFeatures::descriptor()->file());
  const FileDescriptor* file = BuildFile(GetFileProto());
  ASSERT_THAT(file, NotNull());

  LoadFile("google/protobuf/descriptor.proto",
           DescriptorProto::GetDescriptor()->file()->DebugString());
  LoadFile("google/protobuf/unittest_features.proto",
           pb::TestFeatures::GetDescriptor()->file()->DebugString());
  const FileDescriptor* roundtripped =
      LoadFile(file->name(), file->DebugString());

  FileDescriptorProto roundtripped_proto;
  roundtripped->CopyTo(&roundtripped_proto);
  EXPECT_THAT(roundtripped_proto, EqualsRoundTrip())
      << "With generated proto file: \n"
      << file->DebugString();
}

INSTANTIATE_TEST_SUITE_P(
    FeaturesDebugStringTestInst, FeaturesDebugStringTest,
    testing::Values(
        std::make_tuple("Empty", R"pb(name: "foo.proto"
                                      syntax: "editions"
                                      edition: EDITION_2023
        )pb"),
        std::make_tuple(
            "FileFeature",
            R"pb(name: "foo.proto"
                 syntax: "editions"
                 edition: EDITION_2023
                 dependency: "google/protobuf/unittest_features.proto"
                 options {
                   features {
                     [pb.test] { file_feature: VALUE3 }
                   }
                 }
            )pb"),
        std::make_tuple("FieldFeature",
                        R"pb(name: "foo.proto"
                             syntax: "editions"
                             edition: EDITION_2023
                             message_type {
                               name: "Foo"
                               field {
                                 name: "bar"
                                 number: 1
                                 label: LABEL_OPTIONAL
                                 type: TYPE_INT64
                                 options {
                                   features { field_presence: IMPLICIT }
                                 }
                               }
                             }
                        )pb"),
        std::make_tuple("Required",
                        R"pb(name: "foo.proto"
                             syntax: "editions"
                             edition: EDITION_2023
                             message_type {
                               name: "Foo"
                               field {
                                 name: "bar"
                                 number: 1
                                 label: LABEL_OPTIONAL
                                 type: TYPE_INT64
                                 options {
                                   features { field_presence: LEGACY_REQUIRED }
                                 }
                               }
                             }
                        )pb"),
        std::make_tuple("Group",
                        R"pb(name: "foo.proto"
                             syntax: "editions"
                             edition: EDITION_2023
                             message_type {
                               name: "Foo"
                               nested_type {
                                 name: "Bar"
                                 field {
                                   name: "baz"
                                   number: 1
                                   label: LABEL_OPTIONAL
                                   type: TYPE_INT32
                                 }
                               }
                               field {
                                 name: "bar"
                                 number: 1
                                 label: LABEL_OPTIONAL
                                 type: TYPE_MESSAGE
                                 type_name: ".Foo.Bar"
                                 options {
                                   features { message_encoding: DELIMITED }
                                 }
                               }
                             }
                        )pb"),
        std::make_tuple("MessageFeature",
                        R"pb(name: "foo.proto"
                             syntax: "editions"
                             edition: EDITION_2023
                             message_type {
                               name: "Foo"
                               options {
                                 features { json_format: LEGACY_BEST_EFFORT }
                               }
                             }
                        )pb"),
        std::make_tuple(
            "OneofFeature",
            R"pb(name: "foo.proto"
                 syntax: "editions"
                 edition: EDITION_2023
                 dependency: "google/protobuf/unittest_features.proto"
                 message_type {
                   name: "Foo"
                   field {
                     name: "bar"
                     number: 2
                     label: LABEL_OPTIONAL
                     type: TYPE_INT64
                     oneof_index: 0
                   }
                   oneof_decl {
                     name: "foo_oneof"
                     options {
                       features {
                         [pb.test] { oneof_feature: VALUE7 }
                       }
                     }
                   }
                 })pb"),
        std::make_tuple(
            "ExtensionRangeFeature",
            R"pb(name: "foo.proto"
                 syntax: "editions"
                 edition: EDITION_2023
                 dependency: "google/protobuf/unittest_features.proto"
                 message_type {
                   name: "Foo"
                   extension_range {
                     start: 10
                     end: 100
                     options {
                       features {
                         [pb.test] { extension_range_feature: VALUE15 }
                       }
                     }
                   }
                 }
            )pb"),
        std::make_tuple("EnumFeature",
                        R"pb(name: "foo.proto"
                             syntax: "editions"
                             edition: EDITION_2023
                             enum_type {
                               name: "Foo"
                               value { name: "BAR" number: 1 }
                               options { features { enum_type: CLOSED } }
                             }
                        )pb"),
        std::make_tuple(
            "EnumValueFeature",
            R"pb(name: "foo.proto"
                 syntax: "editions"
                 edition: EDITION_2023
                 dependency: "google/protobuf/unittest_features.proto"
                 enum_type {
                   name: "Foo"
                   value {
                     name: "BAR"
                     number: 0
                     options {
                       features {
                         [pb.test] { enum_entry_feature: VALUE1 }
                       }
                     }
                   }

                 }
            )pb"),
        std::make_tuple(
            "ServiceFeature",
            R"pb(name: "foo.proto"
                 syntax: "editions"
                 edition: EDITION_2023
                 dependency: "google/protobuf/unittest_features.proto"
                 service {
                   name: "FooService"
                   options {
                     features {
                       [pb.test] { service_feature: VALUE11 }
                     }
                   }
                 }
            )pb"),
        std::make_tuple(
            "MethodFeature",
            R"pb(name: "foo.proto"
                 syntax: "editions"
                 edition: EDITION_2023
                 dependency: "google/protobuf/unittest_features.proto"
                 message_type { name: "EmptyMessage" }
                 service {
                   name: "FooService"
                   method {
                     name: "BarMethod"
                     input_type: ".EmptyMessage"
                     output_type: ".EmptyMessage"
                     options {
                       features {
                         [pb.test] { method_feature: VALUE12 }
                       }
                     }
                   }
                 })pb")),
    [](const ::testing::TestParamInfo<FeaturesDebugStringTest::ParamType>&
           info) { return std::string(std::get<0>(info.param)); });

using DescriptorPoolFeaturesTest = FeaturesBaseTest;

TEST_F(DescriptorPoolFeaturesTest, BuildStarted) {
  BuildDescriptorMessagesInTestPool();
  FeatureSetDefaults defaults = ParseTextOrDie(R"pb()pb");
  EXPECT_THAT(pool_.SetFeatureSetDefaults(std::move(defaults)),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("defaults can't be changed")));
}

TEST_F(DescriptorPoolFeaturesTest, InvalidRange) {
  FeatureSetDefaults defaults = ParseTextOrDie(R"pb(
    minimum_edition: EDITION_2023
    maximum_edition: EDITION_PROTO2
  )pb");
  EXPECT_THAT(pool_.SetFeatureSetDefaults(std::move(defaults)),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       AllOf(HasSubstr("Invalid edition range"),
                             HasSubstr("PROTO2"), HasSubstr("2023"))));
}

TEST_F(DescriptorPoolFeaturesTest, UnknownDefaults) {
  FeatureSetDefaults defaults = ParseTextOrDie(R"pb(
    defaults {
      edition: EDITION_UNKNOWN
      overridable_features {}
    }
    minimum_edition: EDITION_PROTO2
    maximum_edition: EDITION_2023
  )pb");
  EXPECT_THAT(pool_.SetFeatureSetDefaults(std::move(defaults)),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       AllOf(HasSubstr("Invalid edition UNKNOWN"))));
}

TEST_F(DescriptorPoolFeaturesTest, NotStrictlyIncreasing) {
  FeatureSetDefaults defaults = ParseTextOrDie(R"pb(
    defaults {
      edition: EDITION_PROTO3
      overridable_features {}
    }
    defaults {
      edition: EDITION_PROTO2
      overridable_features {}
    }
    minimum_edition: EDITION_PROTO2
    maximum_edition: EDITION_2023
  )pb");
  EXPECT_THAT(
      pool_.SetFeatureSetDefaults(std::move(defaults)),
      StatusIs(
          absl::StatusCode::kInvalidArgument,
          AllOf(
              HasSubstr("not strictly increasing"),
              HasSubstr("PROTO3 is greater than or equal to edition PROTO2"))));
}

TEST_F(DescriptorPoolFeaturesTest, OverrideDefaults) {
  FeatureSetDefaults defaults = ParseTextOrDie(R"pb(
    defaults {
      edition: EDITION_PROTO2
      overridable_features {
        field_presence: EXPLICIT
        enum_type: CLOSED
        repeated_field_encoding: EXPANDED
        utf8_validation: VERIFY
        message_encoding: LENGTH_PREFIXED
        json_format: ALLOW
        enforce_naming_style: STYLE_LEGACY
        default_symbol_visibility: EXPORT_ALL
      }
    }
    minimum_edition: EDITION_PROTO2
    maximum_edition: EDITION_2023
  )pb");
  EXPECT_OK(pool_.SetFeatureSetDefaults(std::move(defaults)));

  FileDescriptorProto file_proto = ParseTextOrDie(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_PROTO3
  )pb");

  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = ABSL_DIE_IF_NULL(pool_.BuildFile(file_proto));
  EXPECT_THAT(GetFeatures(file), EqualsProto(R"pb(
                field_presence: EXPLICIT
                enum_type: CLOSED
                repeated_field_encoding: EXPANDED
                utf8_validation: VERIFY
                message_encoding: LENGTH_PREFIXED
                json_format: ALLOW
                enforce_naming_style: STYLE_LEGACY
                default_symbol_visibility: EXPORT_ALL
              )pb"));
}

TEST_F(DescriptorPoolFeaturesTest, OverrideFieldDefaults) {
  FeatureSetDefaults defaults = ParseTextOrDie(R"pb(
    defaults {
      edition: EDITION_PROTO2
      overridable_features {
        field_presence: EXPLICIT
        enum_type: CLOSED
        repeated_field_encoding: EXPANDED
        utf8_validation: VERIFY
        message_encoding: LENGTH_PREFIXED
        json_format: ALLOW
        enforce_naming_style: STYLE_LEGACY
        default_symbol_visibility: EXPORT_ALL
      }
    }
    minimum_edition: EDITION_PROTO2
    maximum_edition: EDITION_2023
  )pb");
  EXPECT_OK(pool_.SetFeatureSetDefaults(std::move(defaults)));

  FileDescriptorProto file_proto = ParseTextOrDie(R"pb(
    name: "foo.proto"
    syntax: "editions"
    edition: EDITION_PROTO3
    message_type {
      name: "Foo"
      field { name: "bar" number: 1 label: LABEL_OPTIONAL type: TYPE_INT64 }
    }
  )pb");

  BuildDescriptorMessagesInTestPool();
  const FileDescriptor* file = ABSL_DIE_IF_NULL(pool_.BuildFile(file_proto));
  const FieldDescriptor* field = file->message_type(0)->field(0);
  EXPECT_THAT(GetFeatures(field), EqualsProto(R"pb(
                field_presence: EXPLICIT
                enum_type: CLOSED
                repeated_field_encoding: EXPANDED
                utf8_validation: VERIFY
                message_encoding: LENGTH_PREFIXED
                json_format: ALLOW
                enforce_naming_style: STYLE_LEGACY
                default_symbol_visibility: EXPORT_ALL
              )pb"));
}

TEST_F(DescriptorPoolFeaturesTest, ResolvesFeaturesForCppDefault) {
  EXPECT_FALSE(pool_.ResolvesFeaturesFor(pb::test));
  EXPECT_FALSE(pool_.ResolvesFeaturesFor(pb::TestMessage::test_message));
  EXPECT_TRUE(pool_.ResolvesFeaturesFor(pb::cpp));  // The default.
}

TEST_F(DescriptorPoolFeaturesTest, ResolvesFeaturesFor) {
  auto test_default_spec = FeatureResolver::CompileDefaults(
      FeatureSet::descriptor(), {GetExtensionReflection(pb::test)},
      EDITION_PROTO2, EDITION_99999_TEST_ONLY);
  ASSERT_OK(test_default_spec);
  ASSERT_OK(pool_.SetFeatureSetDefaults(std::move(test_default_spec).value()));

  EXPECT_TRUE(pool_.ResolvesFeaturesFor(pb::test));
  EXPECT_FALSE(pool_.ResolvesFeaturesFor(pb::TestMessage::test_message));
  EXPECT_FALSE(pool_.ResolvesFeaturesFor(pb::cpp));
}

class DescriptorPoolMemoizationTest : public ::testing::Test {
 protected:
  template <typename Desc, typename Func>
  const auto& MemoizeProjection(const Desc* descriptor, Func func) {
    return DescriptorPool::MemoizeProjection(descriptor, func);
  };
};

TEST_F(DescriptorPoolMemoizationTest, MemoizeProjectionBasic) {
  static int counter = 0;
  auto name_lambda = [](const FieldDescriptor* field) {
    counter++;
    return field->full_name();
  };
  const Descriptor* descriptor = proto2_unittest::TestAllTypes::descriptor();

  const auto& name = DescriptorPoolMemoizationTest::MemoizeProjection(
      descriptor->field(0), name_lambda);
  const auto& dupe_name = DescriptorPoolMemoizationTest::MemoizeProjection(
      descriptor->field(0), name_lambda);

  ASSERT_EQ(counter, 1);
  ASSERT_EQ(name, "proto2_unittest.TestAllTypes.optional_int32");
  ASSERT_EQ(dupe_name, "proto2_unittest.TestAllTypes.optional_int32");

  // Check that they are references aliasing the same object.
  EXPECT_TRUE(
      (std::is_same_v<decltype(name), const decltype(descriptor->name())&>));
  EXPECT_EQ(&name, &dupe_name);

  auto other_name = DescriptorPoolMemoizationTest::MemoizeProjection(
      descriptor->field(1), name_lambda);

  ASSERT_EQ(counter, 2);
  ASSERT_NE(other_name, "proto2_unittest.TestAllTypes.optional_int32");
}

TEST_F(DescriptorPoolMemoizationTest, SupportsDifferentDescriptorTypes) {
  static int counter;
  counter = 0;
  auto name_lambda = [](const auto* field) {
    counter++;
    return field->full_name();
  };

  const Descriptor* descriptor = proto2_unittest::TestAllTypes::descriptor();

  // Different descriptor types should be accepted and return the appropriate
  // result, even when reusing the same lambda type.
  EXPECT_EQ("proto2_unittest.TestAllTypes.optional_int32",
            DescriptorPoolMemoizationTest::MemoizeProjection(
                descriptor->field(0), name_lambda));
  EXPECT_EQ("proto2_unittest.TestAllTypes",
            DescriptorPoolMemoizationTest::MemoizeProjection(descriptor,
                                                             name_lambda));
  EXPECT_EQ("proto2_unittest.TestAllTypes.NestedMessage",
            DescriptorPoolMemoizationTest::MemoizeProjection(
                descriptor->nested_type(0), name_lambda));
  EXPECT_EQ(counter, 3);
}

TEST_F(DescriptorPoolMemoizationTest, MemoizeProjectionMultithreaded) {
  auto name_lambda = [](const FieldDescriptor* field) {
    return field->full_name();
  };
  proto2_unittest::TestAllTypes message;
  const Descriptor* descriptor = message.GetDescriptor();
  std::vector<std::thread> threads;
  for (int i = 0; i < descriptor->field_count(); ++i) {
    threads.emplace_back([this, name_lambda, descriptor, i]() {
      auto name = DescriptorPoolMemoizationTest::MemoizeProjection(
          descriptor->field(i), name_lambda);
      auto first_name = DescriptorPoolMemoizationTest::MemoizeProjection(
          descriptor->field(0), name_lambda);
      ASSERT_THAT(name, HasSubstr("proto2_unittest.TestAllTypes"));
      if (i != 0) {
        ASSERT_NE(name, "proto2_unittest.TestAllTypes.optional_int32");
      }
      ASSERT_EQ(first_name, "proto2_unittest.TestAllTypes.optional_int32");
    });
  }
  for (auto& thread : threads) {
    thread.join();
  }
}

TEST_F(DescriptorPoolMemoizationTest, MemoizeProjectionInsertionRace) {
  auto name_lambda = [](const FieldDescriptor* field) {
    return field->full_name();
  };
  proto2_unittest::TestAllTypes message;
  const Descriptor* descriptor = message.GetDescriptor();
  std::vector<std::thread> threads;
  for (int i = 0; i < descriptor->field_count(); ++i) {
    for (int j = 0; j < 3; ++j) {
      threads.emplace_back([this, name_lambda, descriptor, i]() {
        auto name = DescriptorPoolMemoizationTest::MemoizeProjection(
            descriptor->field(i), name_lambda);
        ASSERT_THAT(name, HasSubstr("proto2_unittest.TestAllTypes"));
      });
    }
  }
  for (auto& thread : threads) {
    thread.join();
  }
}




TEST_F(ValidationErrorTest, ExtensionDeclarationsMatchFullNameCompile) {
  BuildFile(R"pb(
    name: "foo.proto"
    package: "ext.test"
    message_type {
      name: "Foo"
      extension_range {
        start: 11
        end: 999
        options: {
          declaration: {
            number: 100
            full_name: ".ext.test.foo"
            type: ".ext.test.Bar"
          }
        }
      }
    }
    message_type { name: "Bar" }
    extension { extendee: "Foo" name: "foo" number: 100 type_name: "Bar" }
  )pb");
}

TEST_F(ValidationErrorTest, ExtensionDeclarationsMismatchFullName) {
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        package: "ext.test"
        message_type {
          name: "Foo"
          extension_range {
            start: 11
            end: 999
            options: {
              declaration: {
                number: 100
                full_name: ".ext.test.buz"
                type: ".ext.test.Bar"
              }
            }
          }
        }
        message_type { name: "Bar" }
        extension { extendee: "Foo" name: "foo" number: 100 type_name: "Bar" }
      )pb",
      "foo.proto: ext.test.foo: EXTENDEE: \"ext.test.Foo\" extension field 100"
      " is expected to have field name \".ext.test.buz\", not "
      "\".ext.test.foo\".\n");
}

TEST_F(ValidationErrorTest, ExtensionDeclarationsMismatchFullNameAllowed) {
  // Make sure that extension declaration names and types are not validated
  // outside of protoc. This is important for allowing extensions to be renamed
  // safely.
  pool_.EnforceExtensionDeclarations(ExtDeclEnforcementLevel::kNoEnforcement);
  BuildFile(
      R"pb(
        name: "foo.proto"
        package: "ext.test"
        message_type {
          name: "Foo"
          extension_range {
            start: 11
            end: 999
            options: {
              declaration: {
                number: 100
                full_name: ".ext.test.buz"
                type: ".ext.test.Bar"
              }
            }
          }
        }
        message_type { name: "Bar" }
        extension { extendee: "Foo" name: "foo" number: 100 type_name: "Bar" }
      )pb");
}

TEST_F(ValidationErrorTest,
       ExtensionDeclarationsFullNameDoesNotLookLikeIdentifier) {
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        message_type {
          name: "Foo"
          extension_range {
            start: 10
            end: 11
            options: {
              declaration: {
                number: 10
                full_name: ".ext..test.bar"
                type: ".baz"
              }
            }
          }
        }
      )pb",
      "foo.proto: Foo: NAME: \".ext..test.bar\" contains invalid "
      "identifiers.\n");
}

TEST_F(ValidationErrorTest, ExtensionDeclarationsDuplicateNames) {
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        message_type {
          name: "Foo"
          extension_range {
            start: 11
            end: 1000
            options: {
              declaration: {
                number: 123
                full_name: ".foo.Bar.baz",
                type: ".Bar"
              }
              declaration: {
                number: 999
                full_name: ".foo.Bar.baz",
                type: "int32"
              }
            }
          }
        }
      )pb",
      "foo.proto: .foo.Bar.baz: NAME: Extension field name \".foo.Bar.baz\" is "
      "declared multiple times.\n");
}

TEST_F(ValidationErrorTest, ExtensionDeclarationMissingFullNameOrType) {
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        message_type {
          name: "Foo"
          extension_range {
            start: 10
            end: 11
            options: { declaration: { number: 10 full_name: ".foo.Bar.foo" } }
          }
          extension_range {
            start: 11
            end: 12
            options: { declaration: { number: 11 type: ".Baz" } }
          }
        }
      )pb",
      "foo.proto: Foo: EXTENDEE: Extension declaration #10 should have both"
      " \"full_name\" and \"type\" set.\n"
      "foo.proto: Foo: EXTENDEE: Extension declaration #11 should have both"
      " \"full_name\" and \"type\" set.\n");
}

TEST_F(ValidationErrorTest, ExtensionDeclarationsNumberNotInRange) {
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        message_type {
          name: "Foo"
          extension_range {
            start: 4
            end: 9999
            options: {
              declaration: { number: 9999 full_name: ".abc" type: ".Bar" }
            }
          }
        }
      )pb",
      "foo.proto: Foo: NUMBER: Extension declaration number 9999 is not in the "
      "extension range.\n");
}

TEST_F(ValidationErrorTest, ExtensionDeclarationsFullNameMissingLeadingDot) {
  BuildFileWithErrors(
      R"pb(
        name: "foo.proto"
        message_type {
          name: "Foo"
          extension_range {
            start: 4
            end: 9999
            options: {
              declaration: { number: 10 full_name: "bar" type: "fixed64" }
            }
          }
        }
      )pb",
      "foo.proto: Foo: NAME: \"bar\" must have a leading dot to indicate the "
      "fully-qualified scope.\n");
}

TEST_F(ValidationErrorTest, VisibilityFromSame) {
  ParseAndBuildFile("vis.proto", R"schema(
        edition = "2024";
        package vis.test;

        local message LocalMessage {
        }
        export message ExportMessage {
          LocalMessage foo = 1;
        }
        )schema");
}

TEST_F(ValidationErrorTest, ExplicitVisibilityFromOther) {
  ParseAndBuildFile("vis.proto", R"schema(
        edition = "2024";
        package vis.test;

        local message LocalMessage {
        }
        export message ExportMessage {
        }
        )schema");

  ParseAndBuildFileWithErrorSubstr(
      "importer.proto",
      R"schema(
        edition = "2024";
        import "vis.proto";

        message BadImport {
          vis.test.LocalMessage foo = 1;
        }
      )schema",
      "importer.proto: BadImport.foo: TYPE: Symbol \"vis.test.LocalMessage\", "
      "defined in \"vis.proto\"  is not visible from \"importer.proto\". It is "
      "explicitly marked 'local' and cannot be accessed outside its own "
      "file\n");
}

TEST_F(ValidationErrorTest, Edition2024DefaultVisibilityFromOther) {
  ParseAndBuildFile("vis.proto", R"schema(
        edition = "2024";
        package vis.test;

        message TopLevelMessage {
          message NestedMessage {
          }
        }
        )schema");

  ParseAndBuildFile("good_importer.proto", R"schema(
        edition = "2024";
        import "vis.proto";

        message GoodImport {
          vis.test.TopLevelMessage foo = 1;
        }
        )schema");

  ParseAndBuildFileWithErrorSubstr(
      "bad_importer.proto", R"schema(
        edition = "2024";
        import "vis.proto";

        message BadImport {
          vis.test.TopLevelMessage.NestedMessage foo = 1;
        }
        )schema",

      "bad_importer.proto: BadImport.foo: TYPE: Symbol "
      "\"vis.test.TopLevelMessage.NestedMessage\", defined in \"vis.proto\"  "
      "is not visible from \"bad_importer.proto\". It defaulted to local from "
      "file-level 'option features.default_symbol_visibility = "
      "'EXPORT_TOP_LEVEL'; and cannot be accessed outside its own file\n");
}

TEST_F(ValidationErrorTest, VisibilityFromLocalExtender) {
  ParseAndBuildFile("vis.proto", R"schema(
        edition = "2024";
        package vis.test;

        local message LocalExtendee {
          extensions 1 to 100;
        }
        )schema");

  ParseAndBuildFileWithErrorSubstr(
      "bad_importer.proto", R"schema(
        edition = "2024";
        import "vis.proto";

        extend vis.test.LocalExtendee {
          string bar = 1;
        }
      )schema",
      "bad_importer.proto: bar: EXTENDEE: Symbol \"vis.test.LocalExtendee\", "
      "defined in \"vis.proto\" target of extend is not visible from "
      "\"bad_importer.proto\". It is explicitly marked 'local' and cannot be "
      "accessed outside its own file\n");
}

struct ExtensionDeclarationsTestParams {
  std::string test_name;
};

// For OSS, we only have declaration to test with.
using ExtensionDeclarationsTest =
    testing::TestWithParam<ExtensionDeclarationsTestParams>;

// For OSS, this is a function that directly returns the parsed
// FileDescriptorProto.
absl::StatusOr<FileDescriptorProto> ParameterizeFileProto(
    absl::string_view file_text, const ExtensionDeclarationsTestParams& param) {
  (void)file_text;  // Parameter is used by Google-internal code.
  (void)param;      // Parameter is used by Google-internal code.
  FileDescriptorProto file_proto;
  if (!TextFormat::ParseFromString(file_text, &file_proto)) {
    return absl::InvalidArgumentError("Failed to parse the input file text.");
  }

  return file_proto;
}

TEST_P(ExtensionDeclarationsTest, DotPrefixTypeCompile) {
  absl::StatusOr<FileDescriptorProto> file_proto = ParameterizeFileProto(
      R"pb(
        name: "foo.proto"
        package: "ext.test"
        message_type {
          name: "Foo"
          extension_range {
            start: 4
            end: 99999
            options: {
              declaration: {
                number: 10
                full_name: ".ext.test.bar"
                type: ".ext.test.Bar"
              }
            }
          }
        }
        message_type { name: "Bar" }
        extension { extendee: "Foo" name: "bar" number: 10 type_name: "Bar" }
      )pb",
      GetParam());
  ASSERT_OK(file_proto);

  DescriptorPool pool;
  pool.EnforceExtensionDeclarations(ExtDeclEnforcementLevel::kAllExtensions);
  EXPECT_NE(pool.BuildFile(*file_proto), nullptr);
}

TEST_P(ExtensionDeclarationsTest, EnumTypeCompile) {
  absl::StatusOr<FileDescriptorProto> file_proto = ParameterizeFileProto(
      R"pb(
        name: "foo.proto"
        package: "ext.test"
        message_type {
          name: "Foo"
          extension_range {
            start: 4
            end: 99999
            options: {
              declaration: {
                number: 10
                full_name: ".ext.test.bar"
                type: ".ext.test.Bar"
              }
            }
          }
        }
        enum_type {
          name: "Bar"
          value: { name: "BUZ" number: 123 }
        }
        extension { extendee: "Foo" name: "bar" number: 10 type_name: "Bar" }
      )pb",
      GetParam());
  ASSERT_OK(file_proto);

  DescriptorPool pool;
  pool.EnforceExtensionDeclarations(ExtDeclEnforcementLevel::kAllExtensions);
  EXPECT_NE(pool.BuildFile(*file_proto), nullptr);
}

TEST_P(ExtensionDeclarationsTest, MismatchEnumType) {
  absl::StatusOr<FileDescriptorProto> file_proto = ParameterizeFileProto(
      R"pb(
        name: "foo.proto"
        package: "ext.test"
        message_type {
          name: "Foo"
          extension_range {
            start: 4
            end: 99999
            options: {
              declaration: {
                number: 10
                full_name: ".ext.test.bar"
                type: ".ext.test.Bar"
              }
            }
          }
        }
        enum_type {
          name: "Bar"
          value: { name: "BUZ1" number: 123 }
        }
        enum_type {
          name: "Abc"
          value: { name: "BUZ2" number: 456 }
        }
        extension { extendee: "Foo" name: "bar" number: 10 type_name: "Abc" }
      )pb",
      GetParam());
  ASSERT_OK(file_proto);

  DescriptorPool pool;
  pool.EnforceExtensionDeclarations(ExtDeclEnforcementLevel::kAllExtensions);
  MockErrorCollector error_collector;
  EXPECT_EQ(pool.BuildFileCollectingErrors(*file_proto, &error_collector),
            nullptr);
  EXPECT_EQ(
      error_collector.text_,
      "foo.proto: ext.test.bar: EXTENDEE: \"ext.test.Foo\" extension field 10 "
      "is expected to be type \".ext.test.Bar\", not \".ext.test.Abc\".\n");
}

TEST_P(ExtensionDeclarationsTest, DotPrefixFullNameCompile) {
  absl::StatusOr<FileDescriptorProto> file_proto = ParameterizeFileProto(
      R"pb(
        name: "foo.proto"
        package: "ext.test"
        message_type {
          name: "Foo"
          extension_range {
            start: 4
            end: 99999
            options: {
              declaration: {
                number: 10
                full_name: ".ext.test.bar"
                type: ".ext.test.Bar"
              }
            }
          }
        }
        message_type { name: "Bar" }
        extension { extendee: "Foo" name: "bar" number: 10 type_name: "Bar" }
      )pb",
      GetParam());
  ASSERT_OK(file_proto);

  DescriptorPool pool;
  pool.EnforceExtensionDeclarations(ExtDeclEnforcementLevel::kAllExtensions);
  EXPECT_NE(pool.BuildFile(*file_proto), nullptr);
}

TEST_P(ExtensionDeclarationsTest, MismatchDotPrefixTypeExpectingMessage) {
  absl::StatusOr<FileDescriptorProto> file_proto = ParameterizeFileProto(
      R"pb(
        name: "foo.proto"
        package: "ext.test"
        message_type {
          name: "Foo"
          extension_range {
            start: 4
            end: 99999
            options: {
              declaration: {
                number: 10
                full_name: ".ext.test.bar"
                type: ".int32"
              }
            }
          }
        }
        extension { name: "bar" number: 10 type: TYPE_INT32 extendee: "Foo" }
      )pb",
      GetParam());
  ASSERT_OK(file_proto);

  DescriptorPool pool;
  pool.EnforceExtensionDeclarations(ExtDeclEnforcementLevel::kAllExtensions);
  MockErrorCollector error_collector;
  EXPECT_EQ(pool.BuildFileCollectingErrors(*file_proto, &error_collector),
            nullptr);
  EXPECT_EQ(error_collector.text_,
            "foo.proto: ext.test.bar: EXTENDEE: \"ext.test.Foo\" extension "
            "field 10 is expected to be type \".int32\", not \"int32\".\n");
}

TEST_P(ExtensionDeclarationsTest, MismatchDotPrefixTypeExpectingNonMessage) {
  absl::StatusOr<FileDescriptorProto> file_proto = ParameterizeFileProto(
      R"pb(
        name: "foo.proto"
        message_type {
          name: "Foo"
          extension_range {
            start: 4
            end: 99999
            options: {
              declaration: { number: 10 full_name: ".bar" type: "int32" }
            }
          }
        }
        message_type { name: "int32" }
        extension { name: "bar" number: 10 type_name: "int32" extendee: "Foo" }
      )pb",
      GetParam());
  ASSERT_OK(file_proto);

  DescriptorPool pool;
  pool.EnforceExtensionDeclarations(ExtDeclEnforcementLevel::kAllExtensions);
  MockErrorCollector error_collector;
  EXPECT_EQ(pool.BuildFileCollectingErrors(*file_proto, &error_collector),
            nullptr);
  EXPECT_EQ(error_collector.text_,
            "foo.proto: bar: EXTENDEE: \"Foo\" extension field 10 is expected "
            "to be type \"int32\", not \".int32\".\n");
}

TEST_P(ExtensionDeclarationsTest, MismatchMessageType) {
  absl::StatusOr<FileDescriptorProto> file_proto = ParameterizeFileProto(
      R"pb(
        name: "foo.proto"
        package: "ext.test"
        message_type {
          name: "Foo"
          extension_range {
            start: 4
            end: 99999
            options: {
              declaration: {
                number: 10
                full_name: ".ext.test.foo"
                type: ".ext.test.Foo"
              }
            }
          }
        }
        message_type { name: "Bar" }
        extension { extendee: "Foo" name: "foo" number: 10 type_name: "Bar" }
      )pb",
      GetParam());
  ASSERT_OK(file_proto);

  DescriptorPool pool;
  pool.EnforceExtensionDeclarations(ExtDeclEnforcementLevel::kAllExtensions);
  MockErrorCollector error_collector;
  EXPECT_EQ(pool.BuildFileCollectingErrors(*file_proto, &error_collector),
            nullptr);
  EXPECT_EQ(
      error_collector.text_,
      "foo.proto: ext.test.foo: EXTENDEE: \"ext.test.Foo\" extension field 10 "
      "is expected to be type \".ext.test.Foo\", not \".ext.test.Bar\".\n");
}

TEST_P(ExtensionDeclarationsTest, NonMessageTypeCompile) {
  absl::StatusOr<FileDescriptorProto> file_proto = ParameterizeFileProto(
      R"pb(
        name: "foo.proto"
        message_type {
          name: "Foo"
          extension_range {
            start: 10
            end: 11
            options: {
              declaration: { number: 10 full_name: ".bar" type: "fixed64" }
            }
          }
        }
        extension { name: "bar" number: 10 type: TYPE_FIXED64 extendee: "Foo" }
      )pb",
      GetParam());
  ASSERT_OK(file_proto);

  DescriptorPool pool;
  pool.EnforceExtensionDeclarations(ExtDeclEnforcementLevel::kAllExtensions);
  EXPECT_NE(pool.BuildFile(*file_proto), nullptr);
}

TEST_P(ExtensionDeclarationsTest, MismatchNonMessageType) {
  absl::StatusOr<FileDescriptorProto> file_proto = ParameterizeFileProto(
      R"pb(
        name: "foo.proto"
        package: "ext.test"
        message_type {
          name: "Foo"
          extension_range {
            start: 10
            end: 11
            options: {
              declaration: {
                number: 10
                full_name: ".ext.test.bar"
                type: "int32"
              }
            }
          }
        }
        extension { name: "bar" number: 10 type: TYPE_FIXED64 extendee: "Foo" }
      )pb",
      GetParam());
  ASSERT_OK(file_proto);

  DescriptorPool pool;
  pool.EnforceExtensionDeclarations(ExtDeclEnforcementLevel::kAllExtensions);
  MockErrorCollector error_collector;
  EXPECT_EQ(pool.BuildFileCollectingErrors(*file_proto, &error_collector),
            nullptr);
  EXPECT_EQ(error_collector.text_,
            "foo.proto: ext.test.bar: EXTENDEE: \"ext.test.Foo\" extension "
            "field 10 is expected to be type \"int32\", not \"fixed64\".\n");
}

TEST_P(ExtensionDeclarationsTest, MismatchCardinalityExpectingRepeated) {
  absl::StatusOr<FileDescriptorProto> file_proto = ParameterizeFileProto(
      R"pb(
        name: "foo.proto"
        package: "ext.test"
        message_type {
          name: "Foo"
          extension_range {
            start: 10
            end: 11
            options: {
              declaration: {
                number: 10
                full_name: ".ext.test.bar"
                type: "fixed64"
                repeated: true
              }
            }
          }
        }
        extension { name: "bar" number: 10 type: TYPE_FIXED64 extendee: "Foo" }
      )pb",
      GetParam());
  ASSERT_OK(file_proto);

  DescriptorPool pool;
  pool.EnforceExtensionDeclarations(ExtDeclEnforcementLevel::kAllExtensions);
  MockErrorCollector error_collector;
  EXPECT_EQ(pool.BuildFileCollectingErrors(*file_proto, &error_collector),
            nullptr);
  EXPECT_EQ(error_collector.text_,
            "foo.proto: ext.test.bar: EXTENDEE: \"ext.test.Foo\" extension "
            "field 10 is expected to be repeated.\n");
}

TEST_P(ExtensionDeclarationsTest, MismatchCardinalityExpectingOptional) {
  absl::StatusOr<FileDescriptorProto> file_proto = ParameterizeFileProto(
      R"pb(
        name: "foo.proto"
        package: "ext.test"
        message_type {
          name: "Foo"
          extension_range {
            start: 10
            end: 11
            options: {
              declaration: {
                number: 10
                full_name: ".ext.test.bar"
                type: "fixed64"
              }
            }
          }
        }
        extension {
          name: "bar"
          number: 10
          type: TYPE_FIXED64
          extendee: "Foo"
          label: LABEL_REPEATED
        }
      )pb",
      GetParam());
  ASSERT_OK(file_proto);

  DescriptorPool pool;
  pool.EnforceExtensionDeclarations(ExtDeclEnforcementLevel::kAllExtensions);
  MockErrorCollector error_collector;
  EXPECT_EQ(pool.BuildFileCollectingErrors(*file_proto, &error_collector),
            nullptr);
  EXPECT_EQ(error_collector.text_,
            "foo.proto: ext.test.bar: EXTENDEE: \"ext.test.Foo\" extension "
            "field 10 is expected to be optional.\n");
}

TEST_P(ExtensionDeclarationsTest, TypeDoesNotLookLikeIdentifier) {
  absl::StatusOr<FileDescriptorProto> file_proto = ParameterizeFileProto(
      R"pb(
        name: "foo.proto"
        message_type {
          name: "Foo"
          extension_range {
            start: 10
            end: 11
            options: {
              declaration: {
                number: 10
                full_name: ".ext.test.bar"
                type: ".b#az"
              }
            }
          }
        }
      )pb",
      GetParam());
  ASSERT_OK(file_proto);

  DescriptorPool pool;
  pool.EnforceExtensionDeclarations(ExtDeclEnforcementLevel::kAllExtensions);
  MockErrorCollector error_collector;
  EXPECT_EQ(pool.BuildFileCollectingErrors(*file_proto, &error_collector),
            nullptr);
  EXPECT_EQ(error_collector.text_,
            "foo.proto: Foo: NAME: \".b#az\" contains invalid identifiers.\n");
}

TEST_P(ExtensionDeclarationsTest, MultipleDeclarationsInARangeCompile) {
  absl::StatusOr<FileDescriptorProto> file_proto = ParameterizeFileProto(
      R"pb(
        name: "foo.proto"
        package: "ext.test"
        message_type {
          name: "Foo"
          extension_range {
            start: 4
            end: 99999
            options: {
              declaration: {
                number: 10
                full_name: ".ext.test.foo"
                type: ".ext.test.Bar"
              }
              declaration: {
                number: 99998
                full_name: ".ext.test.bar"
                type: ".ext.test.Bar"
              }
              declaration: {
                number: 12345
                full_name: ".ext.test.baz"
                type: ".ext.test.Bar"
              }
            }
          }
        }
        message_type { name: "Bar" }
        extension { extendee: "Foo" name: "foo" number: 10 type_name: "Bar" }
        extension { extendee: "Foo" name: "bar" number: 99998 type_name: "Bar" }
        extension { extendee: "Foo" name: "baz" number: 12345 type_name: "Bar" }
      )pb",
      GetParam());
  ASSERT_OK(file_proto);

  DescriptorPool pool;
  pool.EnforceExtensionDeclarations(ExtDeclEnforcementLevel::kAllExtensions);
  EXPECT_NE(pool.BuildFile(*file_proto), nullptr);
}

INSTANTIATE_TEST_SUITE_P(
    ExtensionDeclarationTests, ExtensionDeclarationsTest,
    testing::ValuesIn<ExtensionDeclarationsTestParams>({
        {"Declaration"},
    }),
    [](const testing::TestParamInfo<ExtensionDeclarationsTest::ParamType>&
           info) { return info.param.test_name; });


TEST_F(ValidationErrorTest, PackageTooLong) {
  BuildFileWithErrors(
      "name: \"foo.proto\" "
      "syntax: \"proto3\" "
      "package: "
      "\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaa\"",
      "foo.proto: "
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaa: NAME: Package name is too long\n");
}


// ===================================================================
// DescriptorDatabase

static void AddToDatabase(SimpleDescriptorDatabase* database,
                          absl::string_view file_text) {
  FileDescriptorProto file_proto;
  EXPECT_TRUE(TextFormat::ParseFromString(file_text, &file_proto));
  database->Add(file_proto);
}

class DatabaseBackedPoolTest : public testing::Test {
 protected:
  DatabaseBackedPoolTest() = default;

  SimpleDescriptorDatabase database_;

  void SetUp() override {
    AddToDatabase(
        &database_,
        "name: 'foo.proto' "
        "message_type { name:'Foo' extension_range { start: 1 end: 100 } } "
        "enum_type { name:'TestEnum' value { name:'DUMMY' number:0 } } "
        "service { name:'TestService' } ");
    AddToDatabase(&database_,
                  "name: 'bar.proto' "
                  "dependency: 'foo.proto' "
                  "message_type { name:'Bar' } "
                  "extension { name:'foo_ext' extendee: '.Foo' number:5 "
                  "            label:LABEL_OPTIONAL type:TYPE_INT32 } ");
    // Baz has an undeclared dependency on Foo.
    AddToDatabase(
        &database_,
        "name: 'baz.proto' "
        "message_type { "
        "  name:'Baz' "
        "  field { name:'foo' number:1 label:LABEL_OPTIONAL type_name:'Foo' } "
        "}");
  }

  // We can't inject a file containing errors into a DescriptorPool, so we
  // need an actual mock DescriptorDatabase to test errors.
  class ErrorDescriptorDatabase : public DescriptorDatabase {
   public:
    ErrorDescriptorDatabase() = default;
    ~ErrorDescriptorDatabase() override = default;

    // implements DescriptorDatabase ---------------------------------
    bool FindFileByName(StringViewArg filename,
                        FileDescriptorProto* output) override {
      // error.proto and error2.proto cyclically import each other.
      if (filename == "error.proto") {
        output->Clear();
        output->set_name("error.proto");
        output->add_dependency("error2.proto");
        return true;
      } else if (filename == "error2.proto") {
        output->Clear();
        output->set_name("error2.proto");
        output->add_dependency("error.proto");
        return true;
      } else {
        return false;
      }
    }
    bool FindFileContainingSymbol(StringViewArg symbol_name,
                                  FileDescriptorProto* output) override {
      return false;
    }
    bool FindFileContainingExtension(StringViewArg containing_type,
                                     int field_number,
                                     FileDescriptorProto* output) override {
      return false;
    }
  };

  // A DescriptorDatabase that counts how many times each method has been
  // called and forwards to some other DescriptorDatabase.
  class CallCountingDatabase : public DescriptorDatabase {
   public:
    explicit CallCountingDatabase(DescriptorDatabase* wrapped_db)
        : wrapped_db_(wrapped_db) {
      Clear();
    }
    ~CallCountingDatabase() override = default;

    DescriptorDatabase* wrapped_db_;

    int call_count_;

    void Clear() { call_count_ = 0; }

    // implements DescriptorDatabase ---------------------------------
    bool FindFileByName(StringViewArg filename,
                        FileDescriptorProto* output) override {
      ++call_count_;
      return wrapped_db_->FindFileByName(filename, output);
    }
    bool FindFileContainingSymbol(StringViewArg symbol_name,
                                  FileDescriptorProto* output) override {
      ++call_count_;
      return wrapped_db_->FindFileContainingSymbol(symbol_name, output);
    }
    bool FindFileContainingExtension(StringViewArg containing_type,
                                     int field_number,
                                     FileDescriptorProto* output) override {
      ++call_count_;
      return wrapped_db_->FindFileContainingExtension(containing_type,
                                                      field_number, output);
    }
  };

  // A DescriptorDatabase which falsely always returns foo.proto when searching
  // for any symbol or extension number.  This shouldn't cause the
  // DescriptorPool to reload foo.proto if it is already loaded.
  class FalsePositiveDatabase : public DescriptorDatabase {
   public:
    explicit FalsePositiveDatabase(DescriptorDatabase* wrapped_db)
        : wrapped_db_(wrapped_db) {}
    ~FalsePositiveDatabase() override = default;

    DescriptorDatabase* wrapped_db_;

    // implements DescriptorDatabase ---------------------------------
    bool FindFileByName(StringViewArg filename,
                        FileDescriptorProto* output) override {
      return wrapped_db_->FindFileByName(filename, output);
    }
    bool FindFileContainingSymbol(StringViewArg symbol_name,
                                  FileDescriptorProto* output) override {
      return FindFileByName("foo.proto", output);
    }
    bool FindFileContainingExtension(StringViewArg containing_type,
                                     int field_number,
                                     FileDescriptorProto* output) override {
      return FindFileByName("foo.proto", output);
    }
  };
};

TEST_F(DatabaseBackedPoolTest, FindFileByName) {
  DescriptorPool pool(&database_);

  const FileDescriptor* foo = pool.FindFileByName("foo.proto");
  ASSERT_TRUE(foo != nullptr);
  EXPECT_EQ("foo.proto", foo->name());
  ASSERT_EQ(1, foo->message_type_count());
  EXPECT_EQ("Foo", foo->message_type(0)->name());

  EXPECT_EQ(foo, pool.FindFileByName("foo.proto"));

  EXPECT_TRUE(pool.FindFileByName("no_such_file.proto") == nullptr);
}

TEST_F(DatabaseBackedPoolTest, FindDependencyBeforeDependent) {
  DescriptorPool pool(&database_);

  const FileDescriptor* foo = pool.FindFileByName("foo.proto");
  ASSERT_TRUE(foo != nullptr);
  EXPECT_EQ("foo.proto", foo->name());
  ASSERT_EQ(1, foo->message_type_count());
  EXPECT_EQ("Foo", foo->message_type(0)->name());

  const FileDescriptor* bar = pool.FindFileByName("bar.proto");
  ASSERT_TRUE(bar != nullptr);
  EXPECT_EQ("bar.proto", bar->name());
  ASSERT_EQ(1, bar->message_type_count());
  EXPECT_EQ("Bar", bar->message_type(0)->name());

  ASSERT_EQ(1, bar->dependency_count());
  EXPECT_EQ(foo, bar->dependency(0));
}

TEST_F(DatabaseBackedPoolTest, FindDependentBeforeDependency) {
  DescriptorPool pool(&database_);

  const FileDescriptor* bar = pool.FindFileByName("bar.proto");
  ASSERT_TRUE(bar != nullptr);
  EXPECT_EQ("bar.proto", bar->name());
  ASSERT_EQ(1, bar->message_type_count());
  ASSERT_EQ("Bar", bar->message_type(0)->name());

  const FileDescriptor* foo = pool.FindFileByName("foo.proto");
  ASSERT_TRUE(foo != nullptr);
  EXPECT_EQ("foo.proto", foo->name());
  ASSERT_EQ(1, foo->message_type_count());
  ASSERT_EQ("Foo", foo->message_type(0)->name());

  ASSERT_EQ(1, bar->dependency_count());
  EXPECT_EQ(foo, bar->dependency(0));
}

TEST_F(DatabaseBackedPoolTest, FindFileContainingSymbol) {
  DescriptorPool pool(&database_);

  const FileDescriptor* file = pool.FindFileContainingSymbol("Foo");
  ASSERT_TRUE(file != nullptr);
  EXPECT_EQ("foo.proto", file->name());
  EXPECT_EQ(file, pool.FindFileByName("foo.proto"));

  EXPECT_TRUE(pool.FindFileContainingSymbol("NoSuchSymbol") == nullptr);
}

TEST_F(DatabaseBackedPoolTest, FindMessageTypeByName) {
  DescriptorPool pool(&database_);

  const Descriptor* type = pool.FindMessageTypeByName("Foo");
  ASSERT_TRUE(type != nullptr);
  EXPECT_EQ("Foo", type->name());
  EXPECT_EQ(type->file(), pool.FindFileByName("foo.proto"));

  EXPECT_TRUE(pool.FindMessageTypeByName("NoSuchType") == nullptr);
}

TEST_F(DatabaseBackedPoolTest, FindExtensionByNumber) {
  DescriptorPool pool(&database_);

  const Descriptor* foo = pool.FindMessageTypeByName("Foo");
  ASSERT_TRUE(foo != nullptr);

  const FieldDescriptor* extension = pool.FindExtensionByNumber(foo, 5);
  ASSERT_TRUE(extension != nullptr);
  EXPECT_EQ("foo_ext", extension->name());
  EXPECT_EQ(extension->file(), pool.FindFileByName("bar.proto"));

  EXPECT_TRUE(pool.FindExtensionByNumber(foo, 12) == nullptr);
}

TEST_F(DatabaseBackedPoolTest, FindAllExtensions) {
  DescriptorPool pool(&database_);

  const Descriptor* foo = pool.FindMessageTypeByName("Foo");

  for (int i = 0; i < 2; ++i) {
    // Repeat the lookup twice, to check that we get consistent
    // results despite the fallback database lookup mutating the pool.
    std::vector<const FieldDescriptor*> extensions;
    pool.FindAllExtensions(foo, &extensions);
    ASSERT_EQ(1, extensions.size());
    EXPECT_EQ(5, extensions[0]->number());
  }
}

TEST_F(DatabaseBackedPoolTest, ErrorWithoutErrorCollector) {
  ErrorDescriptorDatabase error_database;
  DescriptorPool pool(&error_database);

  {
    absl::ScopedMockLog log;
    EXPECT_CALL(log, Log(absl::LogSeverity::kError, testing::_, testing::_))
        .Times(testing::AtLeast(1));
    log.StartCapturingLogs();
    EXPECT_TRUE(pool.FindFileByName("error.proto") == nullptr);
  }
}

TEST_F(DatabaseBackedPoolTest, ErrorWithErrorCollector) {
  ErrorDescriptorDatabase error_database;
  MockErrorCollector error_collector;
  DescriptorPool pool(&error_database, &error_collector);

  EXPECT_TRUE(pool.FindFileByName("error.proto") == nullptr);
  EXPECT_EQ(
      "error.proto: error2.proto: IMPORT: File recursively imports itself: "
      "error.proto -> error2.proto -> error.proto\n"
      "error2.proto: error.proto: IMPORT: Import \"error.proto\" was not "
      "found or had errors.\n"
      "error.proto: error2.proto: IMPORT: Import \"error2.proto\" was not "
      "found or had errors.\n",
      error_collector.text_);
}

TEST_F(DatabaseBackedPoolTest, UndeclaredDependencyOnUnbuiltType) {
  // Check that we find and report undeclared dependencies on types that exist
  // in the descriptor database but that have not been built yet.
  MockErrorCollector error_collector;
  DescriptorPool pool(&database_, &error_collector);
  EXPECT_TRUE(pool.FindMessageTypeByName("Baz") == nullptr);
  EXPECT_EQ(
      "baz.proto: Baz.foo: TYPE: \"Foo\" seems to be defined in \"foo.proto\", "
      "which is not imported by \"baz.proto\".  To use it here, please add "
      "the necessary import.\n",
      error_collector.text_);
}

TEST_F(DatabaseBackedPoolTest, RollbackAfterError) {
  // Make sure that all traces of bad types are removed from the pool. This used
  // to be b/4529436, due to the fact that a symbol resolution failure could
  // potentially cause another file to be recursively built, which would trigger
  // a checkpoint _past_ possibly invalid symbols.
  // Baz is defined in the database, but the file is invalid because it is
  // missing a necessary import.
  DescriptorPool pool(&database_);
  EXPECT_TRUE(pool.FindMessageTypeByName("Baz") == nullptr);
  // Make sure that searching again for the file or the type fails.
  EXPECT_TRUE(pool.FindFileByName("baz.proto") == nullptr);
  EXPECT_TRUE(pool.FindMessageTypeByName("Baz") == nullptr);
}

TEST_F(DatabaseBackedPoolTest, UnittestProto) {
  // Try to load all of unittest.proto from a DescriptorDatabase.  This should
  // thoroughly test all paths through DescriptorBuilder to insure that there
  // are no deadlocking problems when pool_->mutex_ is non-null.
  const FileDescriptor* original_file =
      proto2_unittest::TestAllTypes::descriptor()->file();

  DescriptorPoolDatabase database(*DescriptorPool::generated_pool());
  DescriptorPool pool(&database);
  const FileDescriptor* file_from_database =
      pool.FindFileByName(original_file->name());

  ASSERT_TRUE(file_from_database != nullptr);

  FileDescriptorProto original_file_proto;
  original_file->CopyTo(&original_file_proto);

  FileDescriptorProto file_from_database_proto;
  file_from_database->CopyTo(&file_from_database_proto);

  EXPECT_EQ(original_file_proto.DebugString(),
            file_from_database_proto.DebugString());

  // Also verify that CopyTo() did not omit any information.
  EXPECT_EQ(original_file->DebugString(), file_from_database->DebugString());
}

TEST_F(DatabaseBackedPoolTest, FeatureResolution) {
  {
    FileDescriptorProto proto;
    FileDescriptorProto::descriptor()->file()->CopyTo(&proto);
    std::string text_proto;
    google::protobuf::TextFormat::PrintToString(proto, &text_proto);
    AddToDatabase(&database_, text_proto);
  }
  {
    FileDescriptorProto proto;
    pb::TestFeatures::descriptor()->file()->CopyTo(&proto);
    std::string text_proto;
    google::protobuf::TextFormat::PrintToString(proto, &text_proto);
    AddToDatabase(&database_, text_proto);
  }
  AddToDatabase(&database_, R"pb(
    name: "features.proto"
    syntax: "editions"
    edition: EDITION_2023
    dependency: "google/protobuf/unittest_features.proto"
    options {
      features {
        enum_type: CLOSED
        [pb.test] { file_feature: VALUE9 multiple_feature: VALUE9 }
      }
    }
    message_type {
      name: "FooFeatures"
      options {
        features {
          [pb.test] { message_feature: VALUE8 multiple_feature: VALUE8 }
        }
      }
    }
  )pb");
  MockErrorCollector error_collector;
  DescriptorPool pool(&database_, &error_collector);

  auto default_spec = FeatureResolver::CompileDefaults(
      FeatureSet::descriptor(),
      {GetExtensionReflection(pb::cpp), GetExtensionReflection(pb::test)},
      EDITION_PROTO2, EDITION_99999_TEST_ONLY);
  ASSERT_OK(default_spec);
  ASSERT_OK(pool.SetFeatureSetDefaults(std::move(default_spec).value()));

  const Descriptor* foo = pool.FindMessageTypeByName("FooFeatures");
  ASSERT_TRUE(foo != nullptr);
  EXPECT_EQ(GetFeatures(foo).enum_type(), FeatureSet::CLOSED);
  EXPECT_EQ(GetFeatures(foo).repeated_field_encoding(), FeatureSet::PACKED);
  EXPECT_EQ(GetFeatures(foo).GetExtension(pb::test).enum_feature(), pb::VALUE1);
  EXPECT_EQ(GetFeatures(foo).GetExtension(pb::test).file_feature(), pb::VALUE9);
  EXPECT_EQ(GetFeatures(foo).GetExtension(pb::test).message_feature(),
            pb::VALUE8);
  EXPECT_EQ(GetFeatures(foo).GetExtension(pb::test).multiple_feature(),
            pb::VALUE8);
}

TEST_F(DatabaseBackedPoolTest, FeatureLifetimeError) {
  {
    FileDescriptorProto proto;
    FileDescriptorProto::descriptor()->file()->CopyTo(&proto);
    std::string text_proto;
    google::protobuf::TextFormat::PrintToString(proto, &text_proto);
    AddToDatabase(&database_, text_proto);
  }
  {
    FileDescriptorProto proto;
    pb::TestFeatures::descriptor()->file()->CopyTo(&proto);
    std::string text_proto;
    google::protobuf::TextFormat::PrintToString(proto, &text_proto);
    AddToDatabase(&database_, text_proto);
  }
  AddToDatabase(&database_, R"pb(
    name: "features.proto"
    syntax: "editions"
    edition: EDITION_2023
    dependency: "google/protobuf/unittest_features.proto"
    message_type {
      name: "FooFeatures"
      options {
        features {
          [pb.test] { future_feature: VALUE9 }
        }
      }
    }
  )pb");
  MockErrorCollector error_collector;
  DescriptorPool pool(&database_, &error_collector);

  EXPECT_TRUE(pool.FindMessageTypeByName("FooFeatures") == nullptr);
  EXPECT_EQ(error_collector.text_,
            "features.proto: FooFeatures: NAME: Feature "
            "pb.TestFeatures.future_feature wasn't introduced until edition "
            "2024 and can't be used in edition 2023\n");
}

TEST_F(DatabaseBackedPoolTest, FeatureLifetimeErrorUnknownDependencies) {
  {
    FileDescriptorProto proto;
    FileDescriptorProto::descriptor()->file()->CopyTo(&proto);
    std::string text_proto;
    google::protobuf::TextFormat::PrintToString(proto, &text_proto);
    AddToDatabase(&database_, text_proto);
  }
  {
    FileDescriptorProto proto;
    pb::TestFeatures::descriptor()->file()->CopyTo(&proto);
    std::string text_proto;
    google::protobuf::TextFormat::PrintToString(proto, &text_proto);
    AddToDatabase(&database_, text_proto);
  }
  AddToDatabase(&database_, R"pb(
    name: "option.proto"
    syntax: "editions"
    edition: EDITION_2023
    dependency: "google/protobuf/descriptor.proto"
    dependency: "google/protobuf/unittest_features.proto"
    extension {
      name: "foo_extension"
      number: 1000
      type: TYPE_STRING
      extendee: ".google.protobuf.MessageOptions"
      options {
        features {
          [pb.test] { legacy_feature: VALUE9 }
        }
      }
    }
  )pb");

  // Note, we very carefully don't put a dependency here, otherwise option.proto
  // will be built eagerly beforehand.  This triggers a rare condition where
  // DeferredValidation is filled with descriptors that are then rolled back.
  AddToDatabase(&database_, R"pb(
    name: "use_option.proto"
    syntax: "editions"
    edition: EDITION_2023
    message_type {
      name: "FooMessage"
      options {
        uninterpreted_option {
          name { name_part: "foo_extension" is_extension: true }
          string_value: "test"
        }
      }
      field { name: "bar" number: 1 type: TYPE_INT64 }
    }
  )pb");
  MockErrorCollector error_collector;
  DescriptorPool pool(&database_, &error_collector);

  ASSERT_EQ(pool.FindMessageTypeByName("FooMessage"), nullptr);
  EXPECT_EQ(error_collector.text_,
            "use_option.proto: FooMessage: OPTION_NAME: Option "
            "\"(foo_extension)\" unknown. Ensure that your proto definition "
            "file imports the proto which defines the option (i.e. via import "
            "option).\n");

  // Verify that the extension does trigger a lifetime error.
  error_collector.text_.clear();
  ASSERT_EQ(pool.FindExtensionByName("foo_extension"), nullptr);
  EXPECT_EQ(error_collector.text_,
            "option.proto: foo_extension: NAME: Feature "
            "pb.TestFeatures.legacy_feature has been removed in edition 2023 "
            "and can't be used in edition 2023\n");
}

TEST_F(DatabaseBackedPoolTest, DoesntRetryDbUnnecessarily) {
  // Searching for a child of an existing descriptor should never fall back
  // to the DescriptorDatabase even if it isn't found, because we know all
  // children are already loaded.
  CallCountingDatabase call_counter(&database_);
  DescriptorPool pool(&call_counter);

  const FileDescriptor* file = pool.FindFileByName("foo.proto");
  ASSERT_TRUE(file != nullptr);
  const Descriptor* foo = pool.FindMessageTypeByName("Foo");
  ASSERT_TRUE(foo != nullptr);
  const EnumDescriptor* test_enum = pool.FindEnumTypeByName("TestEnum");
  ASSERT_TRUE(test_enum != nullptr);
  const ServiceDescriptor* test_service = pool.FindServiceByName("TestService");
  ASSERT_TRUE(test_service != nullptr);

  EXPECT_NE(0, call_counter.call_count_);
  call_counter.Clear();

  EXPECT_TRUE(foo->FindFieldByName("no_such_field") == nullptr);
  EXPECT_TRUE(foo->FindExtensionByName("no_such_extension") == nullptr);
  EXPECT_TRUE(foo->FindNestedTypeByName("NoSuchMessageType") == nullptr);
  EXPECT_TRUE(foo->FindEnumTypeByName("NoSuchEnumType") == nullptr);
  EXPECT_TRUE(foo->FindEnumValueByName("NO_SUCH_VALUE") == nullptr);
  EXPECT_TRUE(test_enum->FindValueByName("NO_SUCH_VALUE") == nullptr);
  EXPECT_TRUE(test_service->FindMethodByName("NoSuchMethod") == nullptr);

  EXPECT_TRUE(file->FindMessageTypeByName("NoSuchMessageType") == nullptr);
  EXPECT_TRUE(file->FindEnumTypeByName("NoSuchEnumType") == nullptr);
  EXPECT_TRUE(file->FindEnumValueByName("NO_SUCH_VALUE") == nullptr);
  EXPECT_TRUE(file->FindServiceByName("NO_SUCH_VALUE") == nullptr);
  EXPECT_TRUE(file->FindExtensionByName("no_such_extension") == nullptr);

  EXPECT_TRUE(pool.FindFileContainingSymbol("Foo.no.such.field") == nullptr);
  EXPECT_TRUE(pool.FindFileContainingSymbol("Foo.no_such_field") == nullptr);
  EXPECT_TRUE(pool.FindMessageTypeByName("Foo.NoSuchMessageType") == nullptr);
  EXPECT_TRUE(pool.FindFieldByName("Foo.no_such_field") == nullptr);
  EXPECT_TRUE(pool.FindExtensionByName("Foo.no_such_extension") == nullptr);
  EXPECT_TRUE(pool.FindEnumTypeByName("Foo.NoSuchEnumType") == nullptr);
  EXPECT_TRUE(pool.FindEnumValueByName("Foo.NO_SUCH_VALUE") == nullptr);
  EXPECT_TRUE(pool.FindMethodByName("TestService.NoSuchMethod") == nullptr);

  EXPECT_EQ(0, call_counter.call_count_);
}

TEST_F(DatabaseBackedPoolTest, DoesntReloadFilesUncesessarily) {
  // If FindFileContainingSymbol() or FindFileContainingExtension() return a
  // file that is already in the DescriptorPool, it should not attempt to
  // reload the file.
  FalsePositiveDatabase false_positive_database(&database_);
  MockErrorCollector error_collector;
  DescriptorPool pool(&false_positive_database, &error_collector);

  // First make sure foo.proto is loaded.
  const Descriptor* foo = pool.FindMessageTypeByName("Foo");
  ASSERT_TRUE(foo != nullptr);

  // Try inducing false positives.
  EXPECT_TRUE(pool.FindMessageTypeByName("NoSuchSymbol") == nullptr);
  EXPECT_TRUE(pool.FindExtensionByNumber(foo, 22) == nullptr);

  // No errors should have been reported.  (If foo.proto was incorrectly
  // loaded multiple times, errors would have been reported.)
  EXPECT_EQ("", error_collector.text_);
}

// DescriptorDatabase that attempts to induce exponentially-bad performance
// in DescriptorPool. For every positive N, the database contains a file
// fileN.proto, which defines a message MessageN, which contains fields of
// type MessageK for all K in [0,N). Message0 is not defined anywhere
// (file0.proto exists, but is empty), so every other file and message type
// will fail to build.
//
// If the DescriptorPool is not careful to memoize errors, an attempt to
// build a descriptor for MessageN can require O(2^N) time.
class ExponentialErrorDatabase : public DescriptorDatabase {
 public:
  ExponentialErrorDatabase() = default;
  ~ExponentialErrorDatabase() override = default;

  // implements DescriptorDatabase ---------------------------------
  bool FindFileByName(StringViewArg filename,
                      FileDescriptorProto* output) override {
    int file_num = -1;
    FullMatch(filename, "file", ".proto", &file_num);
    if (file_num > -1) {
      return PopulateFile(file_num, output);
    } else {
      return false;
    }
  }
  bool FindFileContainingSymbol(StringViewArg symbol_name,
                                FileDescriptorProto* output) override {
    int file_num = -1;
    FullMatch(symbol_name, "Message", "", &file_num);
    if (file_num > 0) {
      return PopulateFile(file_num, output);
    } else {
      return false;
    }
  }
  bool FindFileContainingExtension(StringViewArg containing_type,
                                   int field_number,
                                   FileDescriptorProto* output) override {
    return false;
  }

 private:
  void FullMatch(absl::string_view name, absl::string_view begin_with,
                 absl::string_view end_with, int32_t* file_num) {
    if (!absl::ConsumePrefix(&name, begin_with)) return;
    if (!absl::ConsumeSuffix(&name, end_with)) return;
    ABSL_CHECK(absl::SimpleAtoi(name, file_num));
  }

  bool PopulateFile(int file_num, FileDescriptorProto* output) {
    ABSL_CHECK_GE(file_num, 0);
    output->Clear();
    output->set_name(absl::Substitute("file$0.proto", file_num));
    // file0.proto doesn't define Message0
    if (file_num > 0) {
      DescriptorProto* message = output->add_message_type();
      message->set_name(absl::Substitute("Message$0", file_num));
      for (int i = 0; i < file_num; ++i) {
        output->add_dependency(absl::Substitute("file$0.proto", i));
        FieldDescriptorProto* field = message->add_field();
        field->set_name(absl::Substitute("field$0", i));
        field->set_number(i);
        field->set_label(FieldDescriptorProto::LABEL_OPTIONAL);
        field->set_type(FieldDescriptorProto::TYPE_MESSAGE);
        field->set_type_name(absl::Substitute("Message$0", i));
      }
    }
    return true;
  }
};

TEST_F(DatabaseBackedPoolTest, DoesntReloadKnownBadFiles) {
  ExponentialErrorDatabase error_database;
  DescriptorPool pool(&error_database);

  ABSL_LOG(INFO) << "A timeout in this test probably indicates a real bug.";

  EXPECT_TRUE(pool.FindFileByName("file40.proto") == nullptr);
  EXPECT_TRUE(pool.FindMessageTypeByName("Message40") == nullptr);
}

TEST_F(DatabaseBackedPoolTest, DoesntFallbackOnWrongType) {
  // If a lookup finds a symbol of the wrong type (e.g. we pass a type name
  // to FindFieldByName()), we should fail fast, without checking the fallback
  // database.
  CallCountingDatabase call_counter(&database_);
  DescriptorPool pool(&call_counter);

  const FileDescriptor* file = pool.FindFileByName("foo.proto");
  ASSERT_TRUE(file != nullptr);
  const Descriptor* foo = pool.FindMessageTypeByName("Foo");
  ASSERT_TRUE(foo != nullptr);
  const EnumDescriptor* test_enum = pool.FindEnumTypeByName("TestEnum");
  ASSERT_TRUE(test_enum != nullptr);

  EXPECT_NE(0, call_counter.call_count_);
  call_counter.Clear();

  EXPECT_TRUE(pool.FindMessageTypeByName("TestEnum") == nullptr);
  EXPECT_TRUE(pool.FindFieldByName("Foo") == nullptr);
  EXPECT_TRUE(pool.FindExtensionByName("Foo") == nullptr);
  EXPECT_TRUE(pool.FindEnumTypeByName("Foo") == nullptr);
  EXPECT_TRUE(pool.FindEnumValueByName("Foo") == nullptr);
  EXPECT_TRUE(pool.FindServiceByName("Foo") == nullptr);
  EXPECT_TRUE(pool.FindMethodByName("Foo") == nullptr);

  EXPECT_EQ(0, call_counter.call_count_);
}

// ===================================================================

class AbortingErrorCollector : public DescriptorPool::ErrorCollector {
 public:
  AbortingErrorCollector() = default;
  AbortingErrorCollector(const AbortingErrorCollector&) = delete;
  AbortingErrorCollector& operator=(const AbortingErrorCollector&) = delete;

  void RecordError(absl::string_view filename, absl::string_view element_name,
                   const Message* message, ErrorLocation location,
                   absl::string_view error_message) override {
    ABSL_LOG(FATAL) << "AddError() called unexpectedly: " << filename << " ["
                    << element_name << "]: " << error_message;
  }
};

// A source tree containing only one file.
class SingletonSourceTree : public compiler::SourceTree {
 public:
  SingletonSourceTree(const std::string& filename, const std::string& contents)
      : filename_(filename), contents_(contents) {}
  SingletonSourceTree(const SingletonSourceTree&) = delete;
  SingletonSourceTree& operator=(const SingletonSourceTree&) = delete;

  io::ZeroCopyInputStream* Open(absl::string_view filename) override {
    return filename == filename_
               ? new io::ArrayInputStream(contents_.data(), contents_.size())
               : nullptr;
  }

 private:
  const std::string filename_;
  const std::string contents_;
};

const char* const kSourceLocationTestInput =
    "syntax = \"proto2\";\n"
    "option java_package = \"com.foo.bar\";\n"
    "option (test_file_opt) = \"foobar\";\n"
    "message A {\n"
    "  option (test_msg_opt) = \"foobar\";\n"
    "  optional int32 a = 1 [deprecated = true];\n"
    "  message B {\n"
    "    required double b = 1 [(test_field_opt) = \"foobar\"];\n"
    "  }\n"
    "  oneof c {\n"
    "    option (test_oneof_opt) = \"foobar\";\n"
    "    string d = 2;\n"
    "    string e = 3;\n"
    "    string f = 4;\n"
    "  }\n"
    "}\n"
    "enum Indecision {\n"
    "  option (test_enum_opt) = 21;\n"
    "  option (test_enum_opt) = 42;\n"
    "  option (test_enum_opt) = 63;\n"
    "  YES   = 1 [(test_enumval_opt).a = 100];\n"
    "  NO    = 2 [(test_enumval_opt) = {a:200}];\n"
    "  MAYBE = 3;\n"
    "}\n"
    "service S {\n"
    "  option (test_svc_opt) = {a:100};\n"
    "  option (test_svc_opt) = {a:200};\n"
    "  option (test_svc_opt) = {a:300};\n"
    "  rpc Method(A) returns (A.B);\n"
    // Put an empty line here to make the source location range match.
    "\n"
    "  rpc OtherMethod(A) returns (A) {\n"
    "    option deprecated = true;\n"
    "    option (test_method_opt) = \"foobar\";\n"
    "  }\n"
    "}\n"
    "message MessageWithExtensions {\n"
    "  extensions 1000 to 2000, 2001 to max [(test_ext_opt) = \"foobar\"];\n"
    "}\n"
    "extend MessageWithExtensions {\n"
    "  repeated int32 int32_extension = 1001 [packed=true];\n"
    "}\n"
    "message C {\n"
    "  extend MessageWithExtensions {\n"
    "    optional C message_extension = 1002;\n"
    "  }\n"
    "}\n"
    "import \"google/protobuf/descriptor.proto\";\n"
    "extend google.protobuf.FileOptions {\n"
    "  optional string test_file_opt = 10101;\n"
    "}\n"
    "extend google.protobuf.MessageOptions {\n"
    "  optional string test_msg_opt = 10101;\n"
    "}\n"
    "extend google.protobuf.FieldOptions {\n"
    "  optional string test_field_opt = 10101;\n"
    "}\n"
    "extend google.protobuf.EnumOptions {\n"
    "  repeated int32 test_enum_opt = 10101;\n"
    "}\n"
    "extend google.protobuf.EnumValueOptions {\n"
    "  optional A test_enumval_opt = 10101;\n"
    "}\n"
    "extend google.protobuf.ServiceOptions {\n"
    "  repeated A test_svc_opt = 10101;\n"
    "}\n"
    "extend google.protobuf.MethodOptions {\n"
    "  optional string test_method_opt = 10101;\n"
    "}\n"
    "extend google.protobuf.OneofOptions {\n"
    "  optional string test_oneof_opt = 10101;\n"
    "}\n"
    "extend google.protobuf.ExtensionRangeOptions {\n"
    "  optional string test_ext_opt = 10101;\n"
    "}\n";

class SourceLocationTest : public testing::Test {
 public:
  SourceLocationTest()
      : source_tree_("/test/test.proto", kSourceLocationTestInput),
        simple_db_(),
        source_tree_db_(&source_tree_),
        merged_db_(&simple_db_, &source_tree_db_),
        pool_(&merged_db_, &collector_) {
    // we need descriptor.proto to be accessible by the pool
    // since our test file imports it
    FileDescriptorProto::descriptor()->file()->CopyTo(&file_proto_);
    simple_db_.Add(file_proto_);
  }

  static std::string PrintSourceLocation(const SourceLocation& loc) {
    return absl::Substitute("$0:$1-$2:$3", 1 + loc.start_line,
                            1 + loc.start_column, 1 + loc.end_line,
                            1 + loc.end_column);
  }

 private:
  FileDescriptorProto file_proto_;
  AbortingErrorCollector collector_;
  SingletonSourceTree source_tree_;
  SimpleDescriptorDatabase simple_db_;  // contains descriptor.proto
  compiler::SourceTreeDescriptorDatabase source_tree_db_;  // loads test.proto
  MergedDescriptorDatabase merged_db_;  // combines above two dbs

 protected:
  DescriptorPool pool_;

  // tag number of all custom options in above test file
  static constexpr int kCustomOptionFieldNumber = 10101;
  // tag number of field "a" in message type "A" in above test file
  static constexpr int kAFieldNumber = 1;
};

// TODO: implement support for option fields and for
// subparts of declarations.

TEST_F(SourceLocationTest, GetSourceLocation) {
  SourceLocation loc;

  const FileDescriptor* file_desc =
      ABSL_DIE_IF_NULL(pool_.FindFileByName("/test/test.proto"));

  const Descriptor* a_desc = file_desc->FindMessageTypeByName("A");
  EXPECT_TRUE(a_desc->GetSourceLocation(&loc));
  EXPECT_EQ("4:1-16:2", PrintSourceLocation(loc));

  const Descriptor* a_b_desc = a_desc->FindNestedTypeByName("B");
  EXPECT_TRUE(a_b_desc->GetSourceLocation(&loc));
  EXPECT_EQ("7:3-9:4", PrintSourceLocation(loc));

  const EnumDescriptor* e_desc = file_desc->FindEnumTypeByName("Indecision");
  EXPECT_TRUE(e_desc->GetSourceLocation(&loc));
  EXPECT_EQ("17:1-24:2", PrintSourceLocation(loc));

  const EnumValueDescriptor* yes_desc = e_desc->FindValueByName("YES");
  EXPECT_TRUE(yes_desc->GetSourceLocation(&loc));
  EXPECT_EQ("21:3-21:42", PrintSourceLocation(loc));

  const ServiceDescriptor* s_desc = file_desc->FindServiceByName("S");
  EXPECT_TRUE(s_desc->GetSourceLocation(&loc));
  EXPECT_EQ("25:1-35:2", PrintSourceLocation(loc));

  const MethodDescriptor* m_desc = s_desc->FindMethodByName("Method");
  EXPECT_TRUE(m_desc->GetSourceLocation(&loc));
  EXPECT_EQ("29:3-29:31", PrintSourceLocation(loc));
}

TEST_F(SourceLocationTest, ExtensionSourceLocation) {
  SourceLocation loc;

  const FileDescriptor* file_desc =
      ABSL_DIE_IF_NULL(pool_.FindFileByName("/test/test.proto"));

  const FieldDescriptor* int32_extension_desc =
      file_desc->FindExtensionByName("int32_extension");
  EXPECT_TRUE(int32_extension_desc->GetSourceLocation(&loc));
  EXPECT_EQ("40:3-40:55", PrintSourceLocation(loc));

  const Descriptor* c_desc = file_desc->FindMessageTypeByName("C");
  EXPECT_TRUE(c_desc->GetSourceLocation(&loc));
  EXPECT_EQ("42:1-46:2", PrintSourceLocation(loc));

  const FieldDescriptor* message_extension_desc =
      c_desc->FindExtensionByName("message_extension");
  EXPECT_TRUE(message_extension_desc->GetSourceLocation(&loc));
  EXPECT_EQ("44:5-44:41", PrintSourceLocation(loc));
}

TEST_F(SourceLocationTest, InterpretedOptionSourceLocation) {
  // This one's a doozy. It checks every kind of option, including
  // extension range options.

  // We are verifying that the file's source info contains correct
  // info for interpreted options and that it does *not* contain
  // any info for corresponding uninterpreted option path.

  SourceLocation loc;

  const FileDescriptor* file_desc =
      ABSL_DIE_IF_NULL(pool_.FindFileByName("/test/test.proto"));

  // File options
  {
    int path[] = {FileDescriptorProto::kOptionsFieldNumber,
                  FileOptions::kJavaPackageFieldNumber};
    int unint[] = {FileDescriptorProto::kOptionsFieldNumber,
                   FileOptions::kUninterpretedOptionFieldNumber, 0};

    std::vector<int> vpath(path, path + 2);
    EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
    EXPECT_EQ("2:1-2:37", PrintSourceLocation(loc));

    std::vector<int> vunint(unint, unint + 3);
    EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
  }
  {
    int path[] = {FileDescriptorProto::kOptionsFieldNumber,
                  kCustomOptionFieldNumber};
    int unint[] = {FileDescriptorProto::kOptionsFieldNumber,
                   FileOptions::kUninterpretedOptionFieldNumber, 1};
    std::vector<int> vpath(path, path + 2);
    EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
    EXPECT_EQ("3:1-3:35", PrintSourceLocation(loc));

    std::vector<int> vunint(unint, unint + 3);
    EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
  }

  // Message option
  {
    int path[] = {FileDescriptorProto::kMessageTypeFieldNumber, 0,
                  DescriptorProto::kOptionsFieldNumber,
                  kCustomOptionFieldNumber};
    int unint[] = {FileDescriptorProto::kMessageTypeFieldNumber, 0,
                   DescriptorProto::kOptionsFieldNumber,
                   MessageOptions::kUninterpretedOptionFieldNumber, 0};
    std::vector<int> vpath(path, path + 4);
    EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
    EXPECT_EQ("5:3-5:36", PrintSourceLocation(loc));

    std::vector<int> vunint(unint, unint + 5);
    EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
  }

  // Field option
  {
    int path[] = {FileDescriptorProto::kMessageTypeFieldNumber,
                  0,
                  DescriptorProto::kFieldFieldNumber,
                  0,
                  FieldDescriptorProto::kOptionsFieldNumber,
                  FieldOptions::kDeprecatedFieldNumber};
    int unint[] = {FileDescriptorProto::kMessageTypeFieldNumber,
                   0,
                   DescriptorProto::kFieldFieldNumber,
                   0,
                   FieldDescriptorProto::kOptionsFieldNumber,
                   FieldOptions::kUninterpretedOptionFieldNumber,
                   0};
    std::vector<int> vpath(path, path + 6);
    EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
    EXPECT_EQ("6:25-6:42", PrintSourceLocation(loc));

    std::vector<int> vunint(unint, unint + 7);
    EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
  }

  // Nested message option
  {
    int path[] = {
        FileDescriptorProto::kMessageTypeFieldNumber, 0,
        DescriptorProto::kNestedTypeFieldNumber,      0,
        DescriptorProto::kFieldFieldNumber,           0,
        FieldDescriptorProto::kOptionsFieldNumber,    kCustomOptionFieldNumber};
    int unint[] = {FileDescriptorProto::kMessageTypeFieldNumber,
                   0,
                   DescriptorProto::kNestedTypeFieldNumber,
                   0,
                   DescriptorProto::kFieldFieldNumber,
                   0,
                   FieldDescriptorProto::kOptionsFieldNumber,
                   FieldOptions::kUninterpretedOptionFieldNumber,
                   0};
    std::vector<int> vpath(path, path + 8);
    EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
    EXPECT_EQ("8:28-8:55", PrintSourceLocation(loc));

    std::vector<int> vunint(unint, unint + 9);
    EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
  }

  // One-of option
  {
    int path[] = {
        FileDescriptorProto::kMessageTypeFieldNumber, 0,
        DescriptorProto::kOneofDeclFieldNumber,       0,
        OneofDescriptorProto::kOptionsFieldNumber,    kCustomOptionFieldNumber};
    int unint[] = {FileDescriptorProto::kMessageTypeFieldNumber,
                   0,
                   DescriptorProto::kOneofDeclFieldNumber,
                   0,
                   OneofDescriptorProto::kOptionsFieldNumber,
                   OneofOptions::kUninterpretedOptionFieldNumber,
                   0};
    std::vector<int> vpath(path, path + 6);
    EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
    EXPECT_EQ("11:5-11:40", PrintSourceLocation(loc));

    std::vector<int> vunint(unint, unint + 7);
    EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
  }

  // Enum option, repeated options
  {
    int path[] = {FileDescriptorProto::kEnumTypeFieldNumber, 0,
                  EnumDescriptorProto::kOptionsFieldNumber,
                  kCustomOptionFieldNumber, 0};
    int unint[] = {FileDescriptorProto::kEnumTypeFieldNumber, 0,
                   EnumDescriptorProto::kOptionsFieldNumber,
                   EnumOptions::kUninterpretedOptionFieldNumber, 0};
    std::vector<int> vpath(path, path + 5);
    EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
    EXPECT_EQ("18:3-18:31", PrintSourceLocation(loc));

    std::vector<int> vunint(unint, unint + 5);
    EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
  }
  {
    int path[] = {FileDescriptorProto::kEnumTypeFieldNumber, 0,
                  EnumDescriptorProto::kOptionsFieldNumber,
                  kCustomOptionFieldNumber, 1};
    int unint[] = {FileDescriptorProto::kEnumTypeFieldNumber, 0,
                   EnumDescriptorProto::kOptionsFieldNumber,
                   EnumOptions::kUninterpretedOptionFieldNumber, 1};
    std::vector<int> vpath(path, path + 5);
    EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
    EXPECT_EQ("19:3-19:31", PrintSourceLocation(loc));

    std::vector<int> vunint(unint, unint + 5);
    EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
  }
  {
    int path[] = {FileDescriptorProto::kEnumTypeFieldNumber, 0,
                  EnumDescriptorProto::kOptionsFieldNumber,
                  kCustomOptionFieldNumber, 2};
    int unint[] = {FileDescriptorProto::kEnumTypeFieldNumber, 0,
                   EnumDescriptorProto::kOptionsFieldNumber,
                   OneofOptions::kUninterpretedOptionFieldNumber, 2};
    std::vector<int> vpath(path, path + 5);
    EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
    EXPECT_EQ("20:3-20:31", PrintSourceLocation(loc));

    std::vector<int> vunint(unint, unint + 5);
    EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
  }

  // Enum value options
  {
    // option w/ message type that directly sets field
    int path[] = {FileDescriptorProto::kEnumTypeFieldNumber,
                  0,
                  EnumDescriptorProto::kValueFieldNumber,
                  0,
                  EnumValueDescriptorProto::kOptionsFieldNumber,
                  kCustomOptionFieldNumber,
                  kAFieldNumber};
    int unint[] = {FileDescriptorProto::kEnumTypeFieldNumber,
                   0,
                   EnumDescriptorProto::kValueFieldNumber,
                   0,
                   EnumValueDescriptorProto::kOptionsFieldNumber,
                   EnumValueOptions::kUninterpretedOptionFieldNumber,
                   0};
    std::vector<int> vpath(path, path + 7);
    EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
    EXPECT_EQ("21:14-21:40", PrintSourceLocation(loc));

    std::vector<int> vunint(unint, unint + 7);
    EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
  }
  {
    int path[] = {FileDescriptorProto::kEnumTypeFieldNumber,
                  0,
                  EnumDescriptorProto::kValueFieldNumber,
                  1,
                  EnumValueDescriptorProto::kOptionsFieldNumber,
                  kCustomOptionFieldNumber};
    int unint[] = {FileDescriptorProto::kEnumTypeFieldNumber,
                   0,
                   EnumDescriptorProto::kValueFieldNumber,
                   1,
                   EnumValueDescriptorProto::kOptionsFieldNumber,
                   EnumValueOptions::kUninterpretedOptionFieldNumber,
                   0};
    std::vector<int> vpath(path, path + 6);
    EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
    EXPECT_EQ("22:14-22:42", PrintSourceLocation(loc));

    std::vector<int> vunint(unint, unint + 7);
    EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
  }

  // Service option, repeated options
  {
    int path[] = {FileDescriptorProto::kServiceFieldNumber, 0,
                  ServiceDescriptorProto::kOptionsFieldNumber,
                  kCustomOptionFieldNumber, 0};
    int unint[] = {FileDescriptorProto::kServiceFieldNumber, 0,
                   ServiceDescriptorProto::kOptionsFieldNumber,
                   ServiceOptions::kUninterpretedOptionFieldNumber, 0};
    std::vector<int> vpath(path, path + 5);
    EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
    EXPECT_EQ("26:3-26:35", PrintSourceLocation(loc));

    std::vector<int> vunint(unint, unint + 5);
    EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
  }
  {
    int path[] = {FileDescriptorProto::kServiceFieldNumber, 0,
                  ServiceDescriptorProto::kOptionsFieldNumber,
                  kCustomOptionFieldNumber, 1};
    int unint[] = {FileDescriptorProto::kServiceFieldNumber, 0,
                   ServiceDescriptorProto::kOptionsFieldNumber,
                   ServiceOptions::kUninterpretedOptionFieldNumber, 1};
    std::vector<int> vpath(path, path + 5);
    EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
    EXPECT_EQ("27:3-27:35", PrintSourceLocation(loc));

    std::vector<int> vunint(unint, unint + 5);
    EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
  }
  {
    int path[] = {FileDescriptorProto::kServiceFieldNumber, 0,
                  ServiceDescriptorProto::kOptionsFieldNumber,
                  kCustomOptionFieldNumber, 2};
    int unint[] = {FileDescriptorProto::kServiceFieldNumber, 0,
                   ServiceDescriptorProto::kOptionsFieldNumber,
                   ServiceOptions::kUninterpretedOptionFieldNumber, 2};
    std::vector<int> vpath(path, path + 5);
    EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
    EXPECT_EQ("28:3-28:35", PrintSourceLocation(loc));

    std::vector<int> vunint(unint, unint + 5);
    EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
  }

  // Method options
  {
    int path[] = {FileDescriptorProto::kServiceFieldNumber,
                  0,
                  ServiceDescriptorProto::kMethodFieldNumber,
                  1,
                  MethodDescriptorProto::kOptionsFieldNumber,
                  MethodOptions::kDeprecatedFieldNumber};
    int unint[] = {FileDescriptorProto::kServiceFieldNumber,
                   0,
                   ServiceDescriptorProto::kMethodFieldNumber,
                   1,
                   MethodDescriptorProto::kOptionsFieldNumber,
                   MethodOptions::kUninterpretedOptionFieldNumber,
                   0};
    std::vector<int> vpath(path, path + 6);
    EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
    EXPECT_EQ("32:5-32:30", PrintSourceLocation(loc));

    std::vector<int> vunint(unint, unint + 7);
    EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
  }
  {
    int path[] = {
        FileDescriptorProto::kServiceFieldNumber,   0,
        ServiceDescriptorProto::kMethodFieldNumber, 1,
        MethodDescriptorProto::kOptionsFieldNumber, kCustomOptionFieldNumber};
    int unint[] = {FileDescriptorProto::kServiceFieldNumber,
                   0,
                   ServiceDescriptorProto::kMethodFieldNumber,
                   1,
                   MethodDescriptorProto::kOptionsFieldNumber,
                   MethodOptions::kUninterpretedOptionFieldNumber,
                   1};
    std::vector<int> vpath(path, path + 6);
    EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
    EXPECT_EQ("33:5-33:41", PrintSourceLocation(loc));

    std::vector<int> vunint(unint, unint + 7);
    EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
  }

  // Extension range options
  {
    int path[] = {FileDescriptorProto::kMessageTypeFieldNumber, 1,
                  DescriptorProto::kExtensionRangeFieldNumber, 0,
                  DescriptorProto_ExtensionRange::kOptionsFieldNumber};
    std::vector<int> vpath(path, path + 5);
    EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
    EXPECT_EQ("37:40-37:67", PrintSourceLocation(loc));
  }
  {
    int path[] = {FileDescriptorProto::kMessageTypeFieldNumber,
                  1,
                  DescriptorProto::kExtensionRangeFieldNumber,
                  0,
                  DescriptorProto_ExtensionRange::kOptionsFieldNumber,
                  kCustomOptionFieldNumber};
    int unint[] = {FileDescriptorProto::kMessageTypeFieldNumber,
                   1,
                   DescriptorProto::kExtensionRangeFieldNumber,
                   0,
                   DescriptorProto_ExtensionRange::kOptionsFieldNumber,
                   ExtensionRangeOptions::kUninterpretedOptionFieldNumber,
                   0};
    std::vector<int> vpath(path, path + 6);
    EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
    EXPECT_EQ("37:41-37:66", PrintSourceLocation(loc));

    std::vector<int> vunint(unint, unint + 7);
    EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
  }
  {
    int path[] = {FileDescriptorProto::kMessageTypeFieldNumber,
                  1,
                  DescriptorProto::kExtensionRangeFieldNumber,
                  1,
                  DescriptorProto_ExtensionRange::kOptionsFieldNumber,
                  kCustomOptionFieldNumber};
    int unint[] = {FileDescriptorProto::kMessageTypeFieldNumber,
                   1,
                   DescriptorProto::kExtensionRangeFieldNumber,
                   1,
                   DescriptorProto_ExtensionRange::kOptionsFieldNumber,
                   ExtensionRangeOptions::kUninterpretedOptionFieldNumber,
                   0};
    std::vector<int> vpath(path, path + 6);
    EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
    EXPECT_EQ("37:41-37:66", PrintSourceLocation(loc));

    std::vector<int> vunint(unint, unint + 7);
    EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
  }

  // Field option on extension
  {
    int path[] = {FileDescriptorProto::kExtensionFieldNumber, 0,
                  FieldDescriptorProto::kOptionsFieldNumber,
                  FieldOptions::kPackedFieldNumber};
    int unint[] = {FileDescriptorProto::kExtensionFieldNumber, 0,
                   FieldDescriptorProto::kOptionsFieldNumber,
                   FieldOptions::kUninterpretedOptionFieldNumber, 0};
    std::vector<int> vpath(path, path + 4);
    EXPECT_TRUE(file_desc->GetSourceLocation(vpath, &loc));
    EXPECT_EQ("40:42-40:53", PrintSourceLocation(loc));

    std::vector<int> vunint(unint, unint + 5);
    EXPECT_FALSE(file_desc->GetSourceLocation(vunint, &loc));
  }
}

// Missing SourceCodeInfo doesn't cause crash:
TEST_F(SourceLocationTest, GetSourceLocation_MissingSourceCodeInfo) {
  SourceLocation loc;

  const FileDescriptor* file_desc =
      ABSL_DIE_IF_NULL(pool_.FindFileByName("/test/test.proto"));

  FileDescriptorProto proto;
  file_desc->CopyTo(&proto);  // Note, this discards the SourceCodeInfo.
  EXPECT_FALSE(proto.has_source_code_info());

  DescriptorPool bad1_pool(&pool_);
  const FileDescriptor* bad1_file_desc =
      ABSL_DIE_IF_NULL(bad1_pool.BuildFile(proto));
  const Descriptor* bad1_a_desc = bad1_file_desc->FindMessageTypeByName("A");
  EXPECT_FALSE(bad1_a_desc->GetSourceLocation(&loc));
}

// Corrupt SourceCodeInfo doesn't cause crash:
TEST_F(SourceLocationTest, GetSourceLocation_BogusSourceCodeInfo) {
  SourceLocation loc;

  const FileDescriptor* file_desc =
      ABSL_DIE_IF_NULL(pool_.FindFileByName("/test/test.proto"));

  FileDescriptorProto proto;
  file_desc->CopyTo(&proto);  // Note, this discards the SourceCodeInfo.
  EXPECT_FALSE(proto.has_source_code_info());
  SourceCodeInfo_Location* loc_msg =
      proto.mutable_source_code_info()->add_location();
  loc_msg->add_path(1);
  loc_msg->add_path(2);
  loc_msg->add_path(3);
  loc_msg->add_span(4);
  loc_msg->add_span(5);
  loc_msg->add_span(6);

  DescriptorPool bad2_pool(&pool_);
  const FileDescriptor* bad2_file_desc =
      ABSL_DIE_IF_NULL(bad2_pool.BuildFile(proto));
  const Descriptor* bad2_a_desc = bad2_file_desc->FindMessageTypeByName("A");
  EXPECT_FALSE(bad2_a_desc->GetSourceLocation(&loc));
}

// ===================================================================

const char* const kCopySourceCodeInfoToTestInput =
    "syntax = \"proto2\";\n"
    "message Foo {}\n";

// Required since source code information is not preserved by
// FileDescriptorTest.
class CopySourceCodeInfoToTest : public testing::Test {
 public:
  CopySourceCodeInfoToTest()
      : source_tree_("/test/test.proto", kCopySourceCodeInfoToTestInput),
        db_(&source_tree_),
        pool_(&db_, &collector_) {}

 private:
  AbortingErrorCollector collector_;
  SingletonSourceTree source_tree_;
  compiler::SourceTreeDescriptorDatabase db_;

 protected:
  DescriptorPool pool_;
};

TEST_F(CopySourceCodeInfoToTest, CopyTo_DoesNotCopySourceCodeInfo) {
  const FileDescriptor* file_desc =
      ABSL_DIE_IF_NULL(pool_.FindFileByName("/test/test.proto"));
  FileDescriptorProto file_desc_proto;
  ASSERT_FALSE(file_desc_proto.has_source_code_info());

  file_desc->CopyTo(&file_desc_proto);
  EXPECT_FALSE(file_desc_proto.has_source_code_info());
}

TEST_F(CopySourceCodeInfoToTest, CopySourceCodeInfoTo) {
  const FileDescriptor* file_desc =
      ABSL_DIE_IF_NULL(pool_.FindFileByName("/test/test.proto"));
  FileDescriptorProto file_desc_proto;
  ASSERT_FALSE(file_desc_proto.has_source_code_info());

  file_desc->CopySourceCodeInfoTo(&file_desc_proto);
  const SourceCodeInfo& info = file_desc_proto.source_code_info();
  ASSERT_EQ(4, info.location_size());
  // Get the Foo message location
  const SourceCodeInfo_Location& foo_location = info.location(2);
  ASSERT_EQ(2, foo_location.path_size());
  EXPECT_EQ(FileDescriptorProto::kMessageTypeFieldNumber, foo_location.path(0));
  EXPECT_EQ(0, foo_location.path(1));      // Foo is the first message defined
  ASSERT_EQ(3, foo_location.span_size());  // Foo spans one line
  EXPECT_EQ(1, foo_location.span(0));      // Foo is declared on line 1
  EXPECT_EQ(0, foo_location.span(1));      // Foo starts at column 0
  EXPECT_EQ(14, foo_location.span(2));     // Foo ends on column 14
}

// ===================================================================

// This is effectively a static_assert ensuring that the generated
// descriptor_table variable is marked extern "C". The compiler will give us an
// error if the generated declaration does not match this one. We need this
// variable to be extern "C" so that we can refer to it from Rust.
//
// If this causes a linker error, it is likely because the name mangling
// changed. That can be fixed by updating to the new name from the generated
// code for unittest.proto.

#define DESCRIPTOR_TABLE_NAME \
  descriptor_table_google_2fprotobuf_2funittest_2eproto

extern "C" {
extern const ::google::protobuf::internal::DescriptorTable DESCRIPTOR_TABLE_NAME;
}

TEST(DescriptorTableExternLinkageTest, DescriptorTableExternLinkageTest) {
  // The goal of this assertion is just to verify that the descriptor_table
  // variable declaration above still refers to a real thing.
  EXPECT_TRUE(absl::EndsWith(DESCRIPTOR_TABLE_NAME.filename, "unittest.proto"));
}


}  // namespace descriptor_unittest
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
