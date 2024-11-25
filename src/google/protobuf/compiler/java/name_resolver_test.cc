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
    BuildFileAndPopulatePool(
        "third_party/java/protobuf/java_features.proto",
        pb::JavaFeatures::descriptor()->file()->DebugString());
  }

  const FileDescriptor* BuildFileAndPopulatePool(absl::string_view filename,
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
    return pool_.BuildFile(proto);
  }

  DescriptorPool pool_;
};

TEST_F(NameResolverTest, FileImmutableClassNameEdition2024) {
  auto file = BuildFileAndPopulatePool("foo.proto",
                                       R"schema(
      edition = "2024";

      package protobuf_unittest;

      message TestFileName2024 {
        int32 field = 1;
      }

      // Conflicting names in edition 2024.
      message FooProto {
        int32 field = 1;
      }
                )schema");

  ClassNameResolver resolver;
  EXPECT_EQ(resolver.GetFileDefaultImmutableClassName(file), "FooProto");
  EXPECT_EQ(resolver.GetFileImmutableClassName(file), "FooProto");
}

TEST_F(NameResolverTest, FileImmutableClassNameDefaultOverriddenEdition2024) {
  auto file = BuildFileAndPopulatePool("foo.proto",
                                       R"schema(
      edition = "2024";

      package protobuf_unittest;

      option java_outer_classname = "BarBuz";

      message FooProto {
        int32 field = 1;
      }
                )schema");

  ClassNameResolver resolver;
  EXPECT_EQ(resolver.GetFileDefaultImmutableClassName(file), "FooProto");
  EXPECT_EQ(resolver.GetFileImmutableClassName(file), "BarBuz");
}

TEST_F(NameResolverTest, FileImmutableClassNameEdition2023) {
  auto file = BuildFileAndPopulatePool("conflicting_file_class_name.proto",
                                       R"schema(
      edition = "2023";

      package protobuf_unittest;

      message ConflictingFileClassName {
        int32 field = 1;
      }
                )schema");

  ClassNameResolver resolver;
  EXPECT_EQ(resolver.GetFileDefaultImmutableClassName(file),
            "ConflictingFileClassName");
  EXPECT_EQ(resolver.GetFileImmutableClassName(file),
            "ConflictingFileClassNameOuterClass");
}

TEST_F(NameResolverTest, JavaMultipleFilesService) {
  const FileDescriptor* file = BuildFileAndPopulatePool("foo.proto",
                                                        R"schema(
      edition = "2023";

      option java_generic_services = true;
      option java_multiple_files = true;

      package protobuf_unittest;

      message Dummy {}
      service FooService {
        rpc FooMethod(Dummy) returns (Dummy) {}
      }
                )schema");

  auto service_descriptor = file->FindServiceByName("FooService");
  ClassNameResolver resolver;
  EXPECT_EQ(resolver.GetClassName(service_descriptor, /* immutable = */ true),
            PACKAGE_PREFIX "protobuf_unittest.FooService");
  EXPECT_EQ(resolver.GetJavaImmutableClassName(service_descriptor),
            PACKAGE_PREFIX "protobuf_unittest.FooService");
}

TEST_F(NameResolverTest, JavaMultipleFilesFalseService) {
  auto file = BuildFileAndPopulatePool("foo.proto",
                                       R"schema(
      edition = "2023";

      option java_generic_services = true;

      package protobuf_unittest;

      message Dummy {}
      service FooService {
        rpc FooMethod(Dummy) returns (Dummy) {}
      }
                )schema");

  auto service_descriptor = file->FindServiceByName("FooService");
  ClassNameResolver resolver;
  EXPECT_EQ(resolver.GetClassName(service_descriptor, /* immutable = */ true),
            PACKAGE_PREFIX "protobuf_unittest.Foo.FooService");
  EXPECT_EQ(resolver.GetJavaImmutableClassName(service_descriptor),
            PACKAGE_PREFIX "protobuf_unittest.Foo$FooService");
}
TEST_F(NameResolverTest, NestInFileClassService) {
  auto file = BuildFileAndPopulatePool("foo.proto",
                                       R"schema(
      edition = "2024";
      import "third_party/java/protobuf/java_features.proto";
      package protobuf_unittest;
      option java_generic_services = true;
      message Dummy {}
      service NestedInFileClassService {
        option features.(pb.java).nest_in_file_class = YES;
        rpc Method(Dummy) returns (Dummy) {}
      }
      service UnnestedService {
        rpc Method(Dummy) returns (Dummy) {}
      }
                )schema");
  ClassNameResolver resolver;
  auto nested_service = file->FindServiceByName("NestedInFileClassService");
  auto unnested_service = file->FindServiceByName("UnnestedService");

  EXPECT_EQ(resolver.GetClassName(unnested_service,
                                  /* immutable = */ true),
            PACKAGE_PREFIX "protobuf_unittest.UnnestedService");
  EXPECT_EQ(resolver.GetClassName(nested_service,
                                  /* immutable = */ true),
            PACKAGE_PREFIX
            "protobuf_unittest.FooProto.NestedInFileClassService");
  EXPECT_EQ(resolver.GetJavaImmutableClassName(unnested_service),
            PACKAGE_PREFIX "protobuf_unittest.UnnestedService");
  EXPECT_EQ(resolver.GetJavaImmutableClassName(nested_service), PACKAGE_PREFIX
            "protobuf_unittest.FooProto$NestedInFileClassService");
}

TEST_F(NameResolverTest, JavaMultipleFilesMessage) {
  auto file = BuildFileAndPopulatePool("foo.proto",
                                       R"schema(
      edition = "2023";

      option java_multiple_files = true;

      package protobuf_unittest;

      message FooMessage {}
                )schema");

  auto message_descriptor = file->FindMessageTypeByName("FooMessage");
  ClassNameResolver resolver;

  EXPECT_EQ(resolver.GetClassName(message_descriptor, /* immutable = */ true),
            PACKAGE_PREFIX "protobuf_unittest.FooMessage");
  EXPECT_EQ(resolver.GetJavaImmutableClassName(message_descriptor),
            PACKAGE_PREFIX "protobuf_unittest.FooMessage");
}

TEST_F(NameResolverTest, JavaMultipleFilesFalseMessage) {
  auto file = BuildFileAndPopulatePool("foo.proto",
                                       R"schema(
      edition = "2023";

      package protobuf_unittest;

      message FooMessage {}
                )schema");

  auto message_descriptor = file->FindMessageTypeByName("FooMessage");
  ClassNameResolver resolver;

  EXPECT_EQ(resolver.GetClassName(message_descriptor, /* immutable = */ true),
            PACKAGE_PREFIX "protobuf_unittest.Foo.FooMessage");
  EXPECT_EQ(resolver.GetJavaImmutableClassName(message_descriptor),
            PACKAGE_PREFIX "protobuf_unittest.Foo$FooMessage");
}

TEST_F(NameResolverTest, NestInFileClassMessage) {
  auto file = BuildFileAndPopulatePool("foo.proto",
                                       R"schema(
      edition = "2024";
      import "third_party/java/protobuf/java_features.proto";
      package protobuf_unittest;
      message NestedInFileClassMessage {
        option features.(pb.java).nest_in_file_class = YES;
        int32 unused = 1;
      }
      message UnnestedMessage {
        int32 unused = 1;
      }
                )schema");

  ClassNameResolver resolver;
  auto nested_message = file->FindMessageTypeByName("NestedInFileClassMessage");
  auto unnested_message = file->FindMessageTypeByName("UnnestedMessage");

  EXPECT_EQ(resolver.GetClassName(unnested_message,
                                  /* immutable = */ true),
            PACKAGE_PREFIX "protobuf_unittest.UnnestedMessage");
  EXPECT_EQ(resolver.GetClassName(nested_message,
                                  /* immutable = */ true),
            PACKAGE_PREFIX
            "protobuf_unittest.FooProto.NestedInFileClassMessage");
  EXPECT_EQ(resolver.GetJavaImmutableClassName(unnested_message),
            PACKAGE_PREFIX "protobuf_unittest.UnnestedMessage");
  EXPECT_EQ(resolver.GetJavaImmutableClassName(nested_message), PACKAGE_PREFIX
            "protobuf_unittest.FooProto$NestedInFileClassMessage");
}

TEST_F(NameResolverTest, JavaMultipleFilesEnum) {
  auto file = BuildFileAndPopulatePool("foo.proto",
                                       R"schema(
      edition = "2023";

      package protobuf_unittest;

      option java_multiple_files = true;

      enum FooEnum {
        FOO_ENUM_UNSPECIFIED = 0;
      }
                )schema");

  auto enum_descriptor = file->FindEnumTypeByName("FooEnum");
  ClassNameResolver resolver;

  EXPECT_EQ(resolver.GetClassName(enum_descriptor, /* immutable = */ true),
            PACKAGE_PREFIX "protobuf_unittest.FooEnum");
  EXPECT_EQ(resolver.GetJavaImmutableClassName(enum_descriptor),
            PACKAGE_PREFIX "protobuf_unittest.FooEnum");
}

TEST_F(NameResolverTest, JavaMultipleFilesFalseEnum) {
  auto file = BuildFileAndPopulatePool("foo.proto",
                                       R"schema(
      edition = "2023";

      package protobuf_unittest;

      enum FooEnum {
        FOO_ENUM_UNSPECIFIED = 0;
      }
                )schema");

  auto enum_descriptor = file->FindEnumTypeByName("FooEnum");
  ClassNameResolver resolver;

  EXPECT_EQ(resolver.GetClassName(enum_descriptor, /* immutable = */ true),
            PACKAGE_PREFIX "protobuf_unittest.Foo.FooEnum");
  EXPECT_EQ(resolver.GetJavaImmutableClassName(enum_descriptor),
            PACKAGE_PREFIX "protobuf_unittest.Foo$FooEnum");
}

TEST_F(NameResolverTest, NestInFileClassEnum) {
  auto file = BuildFileAndPopulatePool("foo.proto",
                                       R"schema(
      edition = "2024";
      import "third_party/java/protobuf/java_features.proto";
      package protobuf_unittest;
      enum NestedInFileClassEnum {
        option features.(pb.java).nest_in_file_class = YES;

        FOO_DEFAULT = 0;
        FOO_VALUE = 1;
      }

      enum UnnestedEnum {
        BAR_DEFAULT = 0;
        BAR_VALUE = 1;
      }
                )schema");

  ClassNameResolver resolver;
  auto nested_enum = file->FindEnumTypeByName("NestedInFileClassEnum");
  auto unnested_enum = file->FindEnumTypeByName("UnnestedEnum");

  EXPECT_EQ(resolver.GetClassName(unnested_enum,
                                  /* immutable = */ true),
            PACKAGE_PREFIX "protobuf_unittest.UnnestedEnum");
  EXPECT_EQ(resolver.GetClassName(nested_enum,
                                  /* immutable = */ true),
            PACKAGE_PREFIX "protobuf_unittest.FooProto.NestedInFileClassEnum");
  EXPECT_EQ(resolver.GetJavaImmutableClassName(unnested_enum),
            PACKAGE_PREFIX "protobuf_unittest.UnnestedEnum");
  EXPECT_EQ(resolver.GetJavaImmutableClassName(nested_enum),
            PACKAGE_PREFIX "protobuf_unittest.FooProto$NestedInFileClassEnum");
}

}  // namespace
}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
