#include "google/protobuf/compiler/java/name_resolver.h"

#include <string>

#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/java/generator.h"
#include "google/protobuf/compiler/parser.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/tokenizer.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"

// Must be last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {
namespace {

#define ASSERT_OK(x) ASSERT_TRUE(x.ok())
#define PACKAGE_PREFIX ""

class SimpleErrorCollector : public io::ErrorCollector {
 public:
  void RecordError(int line, int column, absl::string_view message) override {
    last_error_ = absl::StrFormat("%d:%d:%s", line, column, message);
  }

  const std::string& last_error() { return last_error_; }

 private:
  std::string last_error_;
};

// Gets descriptors with protos built on the fly to go around the "redefinition
// error" with bazel in OSS. This also avoids using the descriptors generated
// from the C++ code generator for Java features; instead, we use a custom
// descriptor pool with feature set defaults built from JavaGenerator.
class NameResolverTest : public testing::Test {
 protected:
  void SetUp() override {
    // Set the Java FeatureSet defaults from JavaGenerator.
    JavaGenerator generator;
    ASSERT_OK(
        pool_.SetFeatureSetDefaults(*generator.BuildFeatureSetDefaults()));

    // Parse and build built-in protos.
    BuildFileAndPopulatePool(
        "google/protobuf/descriptor.proto",
        google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
  }

  void BuildFileAndPopulatePool(absl::string_view filename,
                                absl::string_view contents) {
    io::ArrayInputStream input_stream(contents.data(), contents.size());
    SimpleErrorCollector error_collector;
    io::Tokenizer tokenizer(&input_stream, &error_collector);
    compiler::Parser parser;
    parser.RecordErrorsTo(&error_collector);
    FileDescriptorProto proto;
    ABSL_CHECK(parser.Parse(&tokenizer, &proto))
        << error_collector.last_error() << "\n"
        << contents;
    ABSL_CHECK_EQ("", error_collector.last_error());
    proto.set_name(filename);
    pool_.BuildFile(proto);
  }

  DescriptorPool pool_;
};

TEST_F(NameResolverTest, FileImmutableClassNameEdition2024) {
  BuildFileAndPopulatePool("foo.proto",
                           R"schema(
      edition = "2024";

      package proto2_unittest;

      message TestFileName2024 {
        int32 field = 1;
      }

      // Conflicting names in edition 2024.
      message FooProto {
        int32 field = 1;
      }
                )schema");

  ClassNameResolver resolver;
  auto file = pool_.FindFileByName("foo.proto");
  EXPECT_EQ(resolver.GetFileDefaultImmutableClassName(file), "FooProto");
  EXPECT_EQ(resolver.GetFileImmutableClassName(file), "FooProto");
}

TEST_F(NameResolverTest, FileImmutableClassNameDefaultOverriddenEdition2024) {
  BuildFileAndPopulatePool("foo.proto",
                           R"schema(
      edition = "2024";

      package proto2_unittest;

      option java_outer_classname = "BarBuz";

      message FooProto {
        int32 field = 1;
      }
                )schema");

  ClassNameResolver resolver;
  auto file = pool_.FindFileByName("foo.proto");
  EXPECT_EQ(resolver.GetFileDefaultImmutableClassName(file), "FooProto");
  EXPECT_EQ(resolver.GetFileImmutableClassName(file), "BarBuz");
}

TEST_F(NameResolverTest, FileImmutableClassNameEdition2023) {
  BuildFileAndPopulatePool("conflicting_file_class_name.proto",
                           R"schema(
      edition = "2023";

      package proto2_unittest;

      message ConflictingFileClassName {
        int32 field = 1;
      }
                )schema");

  ClassNameResolver resolver;
  auto file = pool_.FindFileByName("conflicting_file_class_name.proto");
  EXPECT_EQ(resolver.GetFileDefaultImmutableClassName(file),
            "ConflictingFileClassName");
  EXPECT_EQ(resolver.GetFileImmutableClassName(file),
            "ConflictingFileClassNameOuterClass");
}

TEST_F(NameResolverTest, MultipleFilesServiceEdition2023) {
  BuildFileAndPopulatePool("foo.proto",
                           R"schema(
      edition = "2023";

      option java_generic_services = true;
      option java_multiple_files = true;

      package proto2_unittest;

      message Dummy {}
      service FooService {
        rpc FooMethod(Dummy) returns (Dummy) {}
      }
                )schema");

  auto service_descriptor =
      pool_.FindServiceByName("proto2_unittest.FooService");
  ClassNameResolver resolver;
  EXPECT_EQ(resolver.GetClassName(service_descriptor, /* immutable = */ true),
            PACKAGE_PREFIX "proto2_unittest.FooService");
  EXPECT_EQ(resolver.GetJavaImmutableClassName(service_descriptor),
            PACKAGE_PREFIX "proto2_unittest.FooService");
}

TEST_F(NameResolverTest, SingleFileServiceEdition2023) {
  BuildFileAndPopulatePool("foo.proto",
                           R"schema(
      edition = "2023";

      option java_generic_services = true;

      package proto2_unittest;

      message Dummy {}
      service FooService {
        rpc FooMethod(Dummy) returns (Dummy) {}
      }
                )schema");

  auto service_descriptor =
      pool_.FindServiceByName("proto2_unittest.FooService");
  ClassNameResolver resolver;
  EXPECT_EQ(resolver.GetClassName(service_descriptor, /* immutable = */ true),
            PACKAGE_PREFIX "proto2_unittest.Foo.FooService");
  EXPECT_EQ(resolver.GetJavaImmutableClassName(service_descriptor),
            PACKAGE_PREFIX "proto2_unittest.Foo$FooService");
}

TEST_F(NameResolverTest, MultipleFilesMessageEdition2023) {
  BuildFileAndPopulatePool("foo.proto",
                           R"schema(
      edition = "2023";

      option java_multiple_files = true;

      package proto2_unittest;

      message FooMessage {}
                )schema");

  auto message_descriptor =
      pool_.FindMessageTypeByName("proto2_unittest.FooMessage");
  ClassNameResolver resolver;

  EXPECT_EQ(resolver.GetClassName(message_descriptor, /* immutable = */ true),
            PACKAGE_PREFIX "proto2_unittest.FooMessage");
  EXPECT_EQ(resolver.GetJavaImmutableClassName(message_descriptor),
            PACKAGE_PREFIX "proto2_unittest.FooMessage");
}

TEST_F(NameResolverTest, SingleFileMessageEdition2023) {
  BuildFileAndPopulatePool("foo.proto",
                           R"schema(
      edition = "2023";

      package proto2_unittest;

      message FooMessage {}
                )schema");

  auto message_descriptor =
      pool_.FindMessageTypeByName("proto2_unittest.FooMessage");
  ClassNameResolver resolver;

  EXPECT_EQ(resolver.GetClassName(message_descriptor, /* immutable = */ true),
            PACKAGE_PREFIX "proto2_unittest.Foo.FooMessage");
  EXPECT_EQ(resolver.GetJavaImmutableClassName(message_descriptor),
            PACKAGE_PREFIX "proto2_unittest.Foo$FooMessage");
}

TEST_F(NameResolverTest, MultipleFilesEnumEdition2023) {
  BuildFileAndPopulatePool("foo.proto",
                           R"schema(
      edition = "2023";

      package proto2_unittest;

      option java_multiple_files = true;

      enum FooEnum {
        FOO_ENUM_UNSPECIFIED = 0;
      }
                )schema");

  auto enum_descriptor = pool_.FindEnumTypeByName("proto2_unittest.FooEnum");
  ClassNameResolver resolver;

  EXPECT_EQ(resolver.GetClassName(enum_descriptor, /* immutable = */ true),
            PACKAGE_PREFIX "proto2_unittest.FooEnum");
  EXPECT_EQ(resolver.GetJavaImmutableClassName(enum_descriptor),
            PACKAGE_PREFIX "proto2_unittest.FooEnum");
}

TEST_F(NameResolverTest, SingleFileEnumEdition2023) {
  BuildFileAndPopulatePool("foo.proto",
                           R"schema(
      edition = "2023";

      package proto2_unittest;

      enum FooEnum {
        FOO_ENUM_UNSPECIFIED = 0;
      }
                )schema");

  auto enum_descriptor = pool_.FindEnumTypeByName("proto2_unittest.FooEnum");
  ClassNameResolver resolver;

  EXPECT_EQ(resolver.GetClassName(enum_descriptor, /* immutable = */ true),
            PACKAGE_PREFIX "proto2_unittest.Foo.FooEnum");
  EXPECT_EQ(resolver.GetJavaImmutableClassName(enum_descriptor),
            PACKAGE_PREFIX "proto2_unittest.Foo$FooEnum");
}

}  // namespace
}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
