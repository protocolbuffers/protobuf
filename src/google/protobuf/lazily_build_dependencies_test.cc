// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <limits.h>

#include <memory>

#include "google/protobuf/any.pb.h"
#include "google/protobuf/descriptor.pb.h"
#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/cpp_features.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor_database.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/io/coded_stream.h"
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

namespace google {
namespace protobuf {

class LazilyBuildDependenciesTest : public testing::Test {
 public:
  LazilyBuildDependenciesTest() : pool_(&db_, nullptr) {
    pool_.InternalSetLazilyBuildDependencies();
  }

  void ParseProtoAndAddToDb(absl::string_view proto) {
    FileDescriptorProto tmp;
    ASSERT_TRUE(TextFormat::ParseFromString(proto, &tmp));
    db_.Add(tmp);
  }

  void AddSimpleMessageProtoFileToDb(absl::string_view file_name,
                                     absl::string_view message_name) {
    ParseProtoAndAddToDb(absl::StrFormat(
        R"pb(
          name: '%s.proto'
          package: "proto2_unittest"
          message_type {
            name: '%s'
            field { name: 'a' number: 1 label: LABEL_OPTIONAL type: TYPE_INT32 }
          })pb",
        file_name, message_name));
  }

  void AddSimpleEnumProtoFileToDb(absl::string_view file_name,
                                  absl::string_view enum_name,
                                  absl::string_view enum_value_name) {
    ParseProtoAndAddToDb(absl::StrCat("name: '", file_name,
                                      ".proto' "
                                      "package: 'proto2_unittest' "
                                      "enum_type { "
                                      "  name:'",
                                      enum_name,
                                      "' "
                                      "  value { name:'",
                                      enum_value_name,
                                      "' number:1 } "
                                      "}"));
  }

 protected:
  SimpleDescriptorDatabase db_;
  DescriptorPool pool_;
};

TEST_F(LazilyBuildDependenciesTest, Message) {
  ParseProtoAndAddToDb(R"pb(
    name: 'foo.proto'
    package: 'proto2_unittest'
    dependency: 'bar.proto'
    message_type {
      name: 'Foo'
      field {
        name: 'bar'
        number: 1
        label: LABEL_OPTIONAL
        type: TYPE_MESSAGE
        type_name: '.proto2_unittest.Bar'
      }
    })pb");
  AddSimpleMessageProtoFileToDb("bar", "Bar");

  // Verify neither has been built yet.
  EXPECT_FALSE(pool_.InternalIsFileLoaded("foo.proto"));
  EXPECT_FALSE(pool_.InternalIsFileLoaded("bar.proto"));

  const FileDescriptor* file = pool_.FindFileByName("foo.proto");

  // Verify only foo gets built when asking for foo.proto
  EXPECT_TRUE(file != nullptr);
  EXPECT_TRUE(pool_.InternalIsFileLoaded("foo.proto"));
  EXPECT_FALSE(pool_.InternalIsFileLoaded("bar.proto"));

  // Verify calling FindFieldBy* works when the type of the field was
  // not built at cross link time. Verify this doesn't build the file
  // the field's type is defined in, as well.
  const Descriptor* desc = file->FindMessageTypeByName("Foo");
  const FieldDescriptor* field = desc->FindFieldByName("bar");
  EXPECT_TRUE(field != nullptr);
  EXPECT_EQ(field, desc->FindFieldByNumber(1));
  EXPECT_EQ(field, desc->FindFieldByLowercaseName("bar"));
  EXPECT_EQ(field, desc->FindFieldByCamelcaseName("bar"));
  EXPECT_FALSE(pool_.InternalIsFileLoaded("bar.proto"));

  // Finally, verify that if we call message_type() on the field, we will
  // build the file where the message is defined, and get a valid descriptor
  EXPECT_TRUE(field->message_type() != nullptr);
  EXPECT_TRUE(pool_.InternalIsFileLoaded("bar.proto"));
}

TEST_F(LazilyBuildDependenciesTest, UninterpretedCustomOption) {
  ParseProtoAndAddToDb(R"pb(
    name: 'foo.proto'
    package: 'proto2_unittest'
    option_dependency: 'bar.proto'
    message_type {
      name: 'Foo'
      field { name: 'foo' number: 1 type: TYPE_INT32 }
    }
    options {
      uninterpreted_option {
        name { name_part: 'file_opt' is_extension: true }
        positive_int_value: 1234
      }
    }
  )pb");
  ParseProtoAndAddToDb(R"pb(
    name: 'bar.proto'
    package: 'proto2_unittest'
    dependency: 'google/protobuf/descriptor.proto'
    extension {
      extendee: "google.protobuf.FileOptions"
      name: "file_opt"
      number: 123456
      type: TYPE_INT32
    }
  )pb");
  FileDescriptorProto descriptor_proto;
  FileDescriptorProto::descriptor()->file()->CopyTo(&descriptor_proto);
  db_.Add(descriptor_proto);

  // Verify neither has been built yet.
  EXPECT_FALSE(pool_.InternalIsFileLoaded("foo.proto"));
  EXPECT_FALSE(pool_.InternalIsFileLoaded("bar.proto"));
  EXPECT_FALSE(pool_.InternalIsFileLoaded("google/protobuf/descriptor.proto"));

  const FileDescriptor* file = pool_.FindFileByName("foo.proto");

  // Verify foo, bar, and descriptor.proto all get built even when lazy when
  // asking for foo.proto due to options interpretation.
  EXPECT_TRUE(file != nullptr);
  EXPECT_TRUE(pool_.InternalIsFileLoaded("foo.proto"));
  EXPECT_TRUE(pool_.InternalIsFileLoaded("bar.proto"));
  EXPECT_TRUE(pool_.InternalIsFileLoaded("google/protobuf/descriptor.proto"));
}

TEST_F(LazilyBuildDependenciesTest, InterpretedCustomOption) {
  ParseProtoAndAddToDb(R"pb(
    name: 'foo.proto'
    package: 'proto2_unittest'
    edition: EDITION_2024
    option_dependency: 'bar.proto'
    message_type {
      name: 'Foo'
      field { name: 'foo' number: 1 type: TYPE_INT32 }
    }
    options {
      uninterpreted_option {
        name { name_part: 'file_opt' is_extension: true }
        positive_int_value: 1234
      }
    }
  )pb");
  ParseProtoAndAddToDb(R"pb(
    name: 'bar.proto'
    package: 'proto2_unittest'
    dependency: 'google/protobuf/descriptor.proto'
    extension {
      extendee: "google.protobuf.FileOptions"
      name: "file_opt"
      number: 123456
      type: TYPE_INT32
    }
  )pb");
  FileDescriptorProto descriptor_proto;
  FileDescriptorProto::descriptor()->file()->CopyTo(&descriptor_proto);
  db_.Add(descriptor_proto);

  const FileDescriptor* file = pool_.FindFileByName("foo.proto");
  FileDescriptorProto file_proto;
  file->CopyTo(&file_proto);

  auto new_pool = std::make_unique<google::protobuf::DescriptorPool>();
  new_pool->BuildFile(file_proto);

  // New pool with options resolved without transitive dependencies
  const FileDescriptor* file_from_new_pool =
      new_pool->FindFileByName("foo.proto");
  EXPECT_FALSE(new_pool->FindFileByName("bar.proto"));
  EXPECT_FALSE(new_pool->FindFileByName("google/protobuf/descriptor.proto"));
  EXPECT_FALSE(new_pool->FindExtensionByName(".proto2_unittest.file_opt"));
  EXPECT_EQ(0, file_from_new_pool->options().uninterpreted_option_size());
  FileDescriptorProto new_file_proto;
  file_from_new_pool->CopyTo(&new_file_proto);
  EXPECT_EQ(file_proto.DebugString(), new_file_proto.DebugString());
}

TEST_F(LazilyBuildDependenciesTest, Enum) {
  ParseProtoAndAddToDb(
      R"pb(
        name: 'foo.proto'
        package: 'proto2_unittest'
        dependency: 'enum1.proto'
        dependency: 'enum2.proto'
        message_type {
          name: 'Lazy'
          field {
            name: 'enum1'
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_ENUM
            type_name: '.proto2_unittest.Enum1'
          }
          field {
            name: 'enum2'
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_ENUM
            type_name: '.proto2_unittest.Enum2'
          }
        })pb");
  AddSimpleEnumProtoFileToDb("enum1", "Enum1", "ENUM1");
  AddSimpleEnumProtoFileToDb("enum2", "Enum2", "ENUM2");

  const FileDescriptor* file = pool_.FindFileByName("foo.proto");

  // Verify calling enum_type() on a field whose definition is not
  // yet built will build the file and return a descriptor.
  EXPECT_FALSE(pool_.InternalIsFileLoaded("enum1.proto"));
  const Descriptor* desc = file->FindMessageTypeByName("Lazy");
  EXPECT_TRUE(desc != nullptr);
  const FieldDescriptor* field = desc->FindFieldByName("enum1");
  EXPECT_TRUE(field != nullptr);
  EXPECT_TRUE(field->enum_type() != nullptr);
  EXPECT_TRUE(pool_.InternalIsFileLoaded("enum1.proto"));

  // Verify calling default_value_enum() on a field whose definition is not
  // yet built will build the file and return a descriptor to the value.
  EXPECT_FALSE(pool_.InternalIsFileLoaded("enum2.proto"));
  field = desc->FindFieldByName("enum2");
  EXPECT_TRUE(field != nullptr);
  EXPECT_TRUE(field->default_value_enum() != nullptr);
  EXPECT_TRUE(pool_.InternalIsFileLoaded("enum2.proto"));
}

TEST_F(LazilyBuildDependenciesTest, Type) {
  ParseProtoAndAddToDb(
      R"pb(
        name: 'foo.proto'
        package: 'proto2_unittest'
        dependency: 'message1.proto'
        dependency: 'message2.proto'
        dependency: 'enum1.proto'
        dependency: 'enum2.proto'
        message_type {
          name: 'Lazy'
          field {
            name: 'message1'
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_MESSAGE
            type_name: '.proto2_unittest.Message1'
          }
          field {
            name: 'message2'
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_MESSAGE
            type_name: '.proto2_unittest.Message2'
          }
          field {
            name: 'enum1'
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_ENUM
            type_name: '.proto2_unittest.Enum1'
          }
          field {
            name: 'enum2'
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_ENUM
            type_name: '.proto2_unittest.Enum2'
          }
        })pb");
  AddSimpleMessageProtoFileToDb("message1", "Message1");
  AddSimpleMessageProtoFileToDb("message2", "Message2");
  AddSimpleEnumProtoFileToDb("enum1", "Enum1", "ENUM1");
  AddSimpleEnumProtoFileToDb("enum2", "Enum2", "ENUM2");

  const FileDescriptor* file = pool_.FindFileByName("foo.proto");

  // Verify calling type() on a field that is a message type will _not_
  // build the type defined in another file.
  EXPECT_FALSE(pool_.InternalIsFileLoaded("message1.proto"));
  const Descriptor* desc = file->FindMessageTypeByName("Lazy");
  EXPECT_TRUE(desc != nullptr);
  const FieldDescriptor* field = desc->FindFieldByName("message1");
  EXPECT_TRUE(field != nullptr);
  EXPECT_EQ(field->type(), FieldDescriptor::TYPE_MESSAGE);
  EXPECT_FALSE(pool_.InternalIsFileLoaded("message1.proto"));

  // Verify calling cpp_type() on a field that is a message type will _not_
  // build the type defined in another file.
  EXPECT_FALSE(pool_.InternalIsFileLoaded("message2.proto"));
  field = desc->FindFieldByName("message2");
  EXPECT_TRUE(field != nullptr);
  EXPECT_EQ(field->cpp_type(), FieldDescriptor::CPPTYPE_MESSAGE);
  EXPECT_FALSE(pool_.InternalIsFileLoaded("message2.proto"));

  // Verify calling type() on a field that is an enum type will _not_
  // build the type defined in another file.
  EXPECT_FALSE(pool_.InternalIsFileLoaded("enum1.proto"));
  field = desc->FindFieldByName("enum1");
  EXPECT_TRUE(field != nullptr);
  EXPECT_EQ(field->type(), FieldDescriptor::TYPE_ENUM);
  EXPECT_FALSE(pool_.InternalIsFileLoaded("enum1.proto"));

  // Verify calling cpp_type() on a field that is an enum type will _not_
  // build the type defined in another file.
  EXPECT_FALSE(pool_.InternalIsFileLoaded("enum2.proto"));
  field = desc->FindFieldByName("enum2");
  EXPECT_TRUE(field != nullptr);
  EXPECT_EQ(field->cpp_type(), FieldDescriptor::CPPTYPE_ENUM);
  EXPECT_FALSE(pool_.InternalIsFileLoaded("enum2.proto"));
}

TEST_F(LazilyBuildDependenciesTest, Extension) {
  ParseProtoAndAddToDb(
      R"pb(
        name: 'foo.proto'
        package: 'proto2_unittest'
        dependency: 'bar.proto'
        dependency: 'baz.proto'
        extension {
          extendee: '.proto2_unittest.Bar'
          name: 'bar'
          number: 11
          label: LABEL_OPTIONAL
          type: TYPE_MESSAGE
          type_name: '.proto2_unittest.Baz'
        }
      )pb");
  ParseProtoAndAddToDb(
      R"pb(
        name: 'bar.proto'
        package: 'proto2_unittest'
        message_type {
          name: 'Bar'
          extension_range { start: 10 end: 20 }
        }
      )pb");
  AddSimpleMessageProtoFileToDb("baz", "Baz");

  // Verify none have been built yet.
  EXPECT_FALSE(pool_.InternalIsFileLoaded("foo.proto"));
  EXPECT_FALSE(pool_.InternalIsFileLoaded("bar.proto"));
  EXPECT_FALSE(pool_.InternalIsFileLoaded("baz.proto"));

  const FileDescriptor* file = pool_.FindFileByName("foo.proto");

  // Verify foo.bar gets loaded, and bar.proto gets loaded
  // to register the extension. baz.proto should not get loaded.
  EXPECT_TRUE(file != nullptr);
  EXPECT_TRUE(pool_.InternalIsFileLoaded("foo.proto"));
  EXPECT_TRUE(pool_.InternalIsFileLoaded("bar.proto"));
  EXPECT_FALSE(pool_.InternalIsFileLoaded("baz.proto"));
}

TEST_F(LazilyBuildDependenciesTest, Service) {
  ParseProtoAndAddToDb(R"pb(
    name: 'foo.proto'
    package: 'proto2_unittest'
    dependency: 'message1.proto'
    dependency: 'message2.proto'
    dependency: 'message3.proto'
    dependency: 'message4.proto'
    service {
      name: 'LazyService'
      method {
        name: 'A'
        input_type: '.proto2_unittest.Message1'
        output_type: '.proto2_unittest.Message2'
      }
    })pb");
  AddSimpleMessageProtoFileToDb("message1", "Message1");
  AddSimpleMessageProtoFileToDb("message2", "Message2");
  AddSimpleMessageProtoFileToDb("message3", "Message3");
  AddSimpleMessageProtoFileToDb("message4", "Message4");

  const FileDescriptor* file = pool_.FindFileByName("foo.proto");

  // Verify calling FindServiceByName or FindMethodByName doesn't build the
  // files defining the input and output type, and input_type() and
  // output_type() does indeed build the appropriate files.
  const ServiceDescriptor* service = file->FindServiceByName("LazyService");
  EXPECT_TRUE(service != nullptr);
  const MethodDescriptor* method = service->FindMethodByName("A");
  EXPECT_FALSE(pool_.InternalIsFileLoaded("message1.proto"));
  EXPECT_FALSE(pool_.InternalIsFileLoaded("message2.proto"));
  EXPECT_TRUE(method != nullptr);
  EXPECT_TRUE(method->input_type() != nullptr);
  EXPECT_TRUE(pool_.InternalIsFileLoaded("message1.proto"));
  EXPECT_FALSE(pool_.InternalIsFileLoaded("message2.proto"));
  EXPECT_TRUE(method->output_type() != nullptr);
  EXPECT_TRUE(pool_.InternalIsFileLoaded("message2.proto"));
}


TEST_F(LazilyBuildDependenciesTest, GeneratedFile) {
  // Most testing is done with custom pools with lazy dependencies forced on,
  // do some sanity checking that lazy imports is on by default for the
  // generated pool, and do custom options testing with generated to
  // be able to use the GetExtension ids for the custom options.

  // Verify none of the files are loaded yet.
  EXPECT_FALSE(DescriptorPool::generated_pool()->InternalIsFileLoaded(
      "google/protobuf/unittest_lazy_dependencies.proto"));
  EXPECT_FALSE(DescriptorPool::generated_pool()->InternalIsFileLoaded(
      "google/protobuf/unittest_lazy_dependencies_custom_option.proto"));
  EXPECT_FALSE(DescriptorPool::generated_pool()->InternalIsFileLoaded(
      "google/protobuf/unittest_lazy_dependencies_enum.proto"));

  // Verify calling autogenerated function to get a descriptor in the base
  // file will build that file but none of its imports. This verifies that
  // lazily_build_dependencies_ is set on the generated pool, and also that
  // the generated function "descriptor()" doesn't somehow subvert the laziness
  // by manually loading the dependencies or something.
  EXPECT_TRUE(proto2_unittest::lazy_imports::ImportedMessage::descriptor() !=
              nullptr);
  EXPECT_TRUE(DescriptorPool::generated_pool()->InternalIsFileLoaded(
      "google/protobuf/unittest_lazy_dependencies.proto"));
  EXPECT_FALSE(DescriptorPool::generated_pool()->InternalIsFileLoaded(
      "google/protobuf/unittest_lazy_dependencies_custom_option.proto"));
  EXPECT_FALSE(DescriptorPool::generated_pool()->InternalIsFileLoaded(
      "google/protobuf/unittest_lazy_dependencies_enum.proto"));

  // Verify custom options work when defined in an import that isn't loaded,
  // and that a non-default value of a custom option doesn't load the file
  // where that enum is defined.
  const MessageOptions& options =
      proto2_unittest::lazy_imports::MessageCustomOption::descriptor()
          ->options();
  proto2_unittest::lazy_imports::LazyEnum custom_option_value =
      options.GetExtension(proto2_unittest::lazy_imports::lazy_enum_option);

  EXPECT_FALSE(DescriptorPool::generated_pool()->InternalIsFileLoaded(
      "google/protobuf/unittest_lazy_dependencies_custom_option.proto"));
  EXPECT_FALSE(DescriptorPool::generated_pool()->InternalIsFileLoaded(
      "google/protobuf/unittest_lazy_dependencies_enum.proto"));
  EXPECT_EQ(custom_option_value, proto2_unittest::lazy_imports::LAZY_ENUM_1);

  const MessageOptions& options2 =
      proto2_unittest::lazy_imports::MessageCustomOption2::descriptor()
          ->options();
  custom_option_value =
      options2.GetExtension(proto2_unittest::lazy_imports::lazy_enum_option);

  EXPECT_FALSE(DescriptorPool::generated_pool()->InternalIsFileLoaded(
      "google/protobuf/unittest_lazy_dependencies_custom_option.proto"));
  EXPECT_FALSE(DescriptorPool::generated_pool()->InternalIsFileLoaded(
      "google/protobuf/unittest_lazy_dependencies_enum.proto"));
  EXPECT_EQ(custom_option_value, proto2_unittest::lazy_imports::LAZY_ENUM_0);
}

TEST_F(LazilyBuildDependenciesTest, Dependency) {
  ParseProtoAndAddToDb(
      R"pb(
        name: 'foo.proto'
        package: 'proto2_unittest'
        dependency: 'bar.proto'
        message_type {
          name: 'Foo'
          field {
            name: 'bar'
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_MESSAGE
            type_name: '.proto2_unittest.Bar'
          }
        }
      )pb");
  ParseProtoAndAddToDb(
      R"pb(
        name: 'bar.proto'
        package: 'proto2_unittest'
        dependency: 'baz.proto'
        message_type {
          name: 'Bar'
          field {
            name: 'baz'
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_MESSAGE
            type_name: '.proto2_unittest.Baz'
          }
        }
      )pb");
  AddSimpleMessageProtoFileToDb("baz", "Baz");

  const FileDescriptor* foo_file = pool_.FindFileByName("foo.proto");
  EXPECT_TRUE(foo_file != nullptr);
  // As expected, requesting foo.proto shouldn't build its dependencies
  EXPECT_TRUE(pool_.InternalIsFileLoaded("foo.proto"));
  EXPECT_FALSE(pool_.InternalIsFileLoaded("bar.proto"));
  EXPECT_FALSE(pool_.InternalIsFileLoaded("baz.proto"));

  // Verify calling dependency(N) will build the dependency, but
  // not that file's dependencies.
  const FileDescriptor* bar_file = foo_file->dependency(0);
  EXPECT_TRUE(bar_file != nullptr);
  EXPECT_TRUE(pool_.InternalIsFileLoaded("bar.proto"));
  EXPECT_FALSE(pool_.InternalIsFileLoaded("baz.proto"));
}

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
