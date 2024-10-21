#include "google/protobuf/compiler/java/name_resolver.h"

#include <string>

#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/file.h"
#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include "google/protobuf/testing/googletest.h"
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/java/java_features.pb.h"
#include "google/protobuf/compiler/command_line_interface.h"
#include "google/protobuf/compiler/command_line_interface_tester.h"
#include "google/protobuf/compiler/java/generator.h"
#include "google/protobuf/compiler/java/test_file_name.pb.h"
#include "google/protobuf/compiler/java/test_file_name_2024.pb.h"
#include "google/protobuf/compiler/java/test_multiple_file_no.pb.h"
#include "google/protobuf/compiler/java/test_multiple_file_yes.pb.h"
#include "google/protobuf/descriptor.h"

// Must be last.

#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {
namespace {

using ::testing::HasSubstr;

#define PACKAGE_PREFIX ""

// Tests for Edition 2024 feature `use_old_outer_classname_default`.
TEST(NameResolverTest, GetFileDefaultImmutableClassNameEdition2024) {
  ClassNameResolver resolver;

  EXPECT_EQ(resolver.GetFileDefaultImmutableClassName(
                protobuf_unittest::TestFileName2024::GetDescriptor()->file()),
            "TestFileName2024Proto");
}

TEST(NameResolverTest, GetFileDefaultImmutableClassNameEdition2023) {
  ClassNameResolver resolver;

  EXPECT_EQ(resolver.GetFileDefaultImmutableClassName(
                protobuf_unittest::TestFileName::GetDescriptor()->file()),
            "TestFileName");
}

TEST(NameResolverTest, GetFileImmutableClassNameEdition2024) {
  ClassNameResolver resolver;

  EXPECT_EQ(resolver.GetFileImmutableClassName(
                protobuf_unittest::TestFileName2024::GetDescriptor()->file()),
            "TestFileName2024Proto");
}

TEST(NameResolverTest, GetFileImmutableClassNameEdition2023) {
  ClassNameResolver resolver;

  EXPECT_EQ(resolver.GetFileImmutableClassName(
                protobuf_unittest::TestFileName::GetDescriptor()->file()),
            "TestFileNameOuterClass");
}

TEST(NameResolverTest, GetFileImmutableClassNameConflictingEdition2024) {
  ClassNameResolver resolver;

  // Conflicting names in Edition 2024 will not add the "OuterClass" suffix.
  EXPECT_EQ(
      resolver.GetFileImmutableClassName(
          protobuf_unittest::TestFileName2024Proto::GetDescriptor()->file()),
      "TestFileName2024Proto");
}

TEST(NameResolverTest, InvalidConflictingProtoSuffixedMessageNameEdition2024) {
  CommandLineInterface cli;
  ABSL_CHECK_OK(File::SetContents(
      absl::StrCat(::testing::TempDir(), "/test_file_name.proto"),
      R"schema(
      edition = "2024";
      package foo;
      message TestFileNameProto {
        int32 field = 1;
      }
      )schema",
      true));
  JavaGenerator java_generator;
  cli.RegisterGenerator("--java_out", &java_generator, "");
  std::string java_out = absl::StrCat("--java_out=", ::testing::TempDir());
  std::string proto_path = absl::StrCat("-I", ::testing::TempDir());
  const char* argv[] = {"protoc", java_out.c_str(), proto_path.c_str(),
                        "test_file_name.proto", "--experimental_editions"};
  CaptureTestStderr();

  EXPECT_EQ(1, cli.Run(5, argv));
  EXPECT_THAT(GetCapturedTestStderr(),
              HasSubstr("Cannot generate Java output because the file's outer "
                        "class name, \"TestFileNameProto\", matches the name "
                        "of one of the types declared inside it"));
}

TEST(NameResolverTest, GetClassNameMultipleFilesServiceEdition2023) {
  ClassNameResolver resolver;

  EXPECT_EQ(resolver.GetClassName(
                protobuf_unittest::MultipleFileYesService::descriptor(),
                /* immutable = */ true),
            PACKAGE_PREFIX "protobuf_unittest.MultipleFileYesService");
  EXPECT_EQ(resolver.GetClassName(
                protobuf_unittest::MultipleFileNoService::descriptor(),
                /* immutable = */ true),
            PACKAGE_PREFIX
            "protobuf_unittest.TestMultipleFileNo."
            "MultipleFileNoService");
}

TEST(NameResolverTest, GetJavaClassNameMultipleFilesServiceEdition2023) {
  ClassNameResolver resolver;

  EXPECT_EQ(resolver.GetJavaImmutableClassName(
                protobuf_unittest::MultipleFileYesService::descriptor()),
            PACKAGE_PREFIX "protobuf_unittest.MultipleFileYesService");
  EXPECT_EQ(resolver.GetJavaImmutableClassName(
                protobuf_unittest::MultipleFileNoService::descriptor()),
            PACKAGE_PREFIX
            "protobuf_unittest.TestMultipleFileNo$"
            "MultipleFileNoService");
}

TEST(NameResolverTest, GetClassNameMultipleFilesMessageEdition2023) {
  ClassNameResolver resolver;

  EXPECT_EQ(resolver.GetClassName(
                protobuf_unittest::MultipleFileYesMessage::descriptor(),
                /* immutable = */ true),
            PACKAGE_PREFIX "protobuf_unittest.MultipleFileYesMessage");
  EXPECT_EQ(resolver.GetClassName(
                protobuf_unittest::MultipleFileNoMessage::descriptor(),
                /* immutable = */ true),
            PACKAGE_PREFIX
            "protobuf_unittest.TestMultipleFileNo."
            "MultipleFileNoMessage");
}

TEST(NameResolverTest, GetJavaClassNameMultipleFilesMessageEdition2023) {
  ClassNameResolver resolver;

  EXPECT_EQ(resolver.GetJavaImmutableClassName(
                protobuf_unittest::MultipleFileYesMessage::descriptor()),
            PACKAGE_PREFIX "protobuf_unittest.MultipleFileYesMessage");
  EXPECT_EQ(resolver.GetJavaImmutableClassName(
                protobuf_unittest::MultipleFileNoMessage::descriptor()),
            PACKAGE_PREFIX
            "protobuf_unittest.TestMultipleFileNo$"
            "MultipleFileNoMessage");
}

TEST(NameResolverTest, GetClassNameMultipleFilesEnumEdition2023) {
  ClassNameResolver resolver;

  EXPECT_EQ(
      resolver.GetClassName(protobuf_unittest::MultipleFileYesEnum_descriptor(),
                            /* immutable = */ true),
      PACKAGE_PREFIX "protobuf_unittest.MultipleFileYesEnum");
  EXPECT_EQ(
      resolver.GetClassName(protobuf_unittest::MultipleFileNoEnum_descriptor(),
                            /* immutable = */ true),
      PACKAGE_PREFIX
      "protobuf_unittest.TestMultipleFileNo."
      "MultipleFileNoEnum");
}

TEST(NameResolverTest, GetJavaClassNameMultipleFilesEnumEdition2023) {
  ClassNameResolver resolver;

  EXPECT_EQ(resolver.GetJavaImmutableClassName(
                protobuf_unittest::MultipleFileYesEnum_descriptor()),
            PACKAGE_PREFIX "protobuf_unittest.MultipleFileYesEnum");
  EXPECT_EQ(resolver.GetJavaImmutableClassName(
                protobuf_unittest::MultipleFileNoEnum_descriptor()),
            PACKAGE_PREFIX
            "protobuf_unittest.TestMultipleFileNo$"
            "MultipleFileNoEnum");
}

// Build protos depending on "java_features.proto" on the fly to avoid the
// redefinition error coming from the bootstrap java_features C++ proto and the
// generated ones from {cc_}proto_library() in OSS.
class NameResolverTestEdition2024 : public CommandLineInterfaceTester {
 protected:
  NameResolverTestEdition2024() {
    // Generate built-in protos.
    CreateTempFile(
        "google/protobuf/descriptor.proto",
        google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
    CreateTempFile("third_party/java/protobuf/java_features.proto",
                   pb::JavaFeatures::descriptor()->file()->DebugString());

    FileDescriptorProto descriptor_file;
    google::protobuf::DescriptorProto::descriptor()->file()->CopyTo(&descriptor_file);
    FileDescriptorProto java_features_file;
    pb::JavaFeatures::descriptor()->file()->CopyTo(&java_features_file);
    java_features_file.set_name(
        "third_party/java/protobuf/java_features.proto");
    pool_.BuildFile(descriptor_file);
    pool_.BuildFile(java_features_file);
  }

  void RunProtocAndPopulatePool(absl::string_view command) {
    RunProtoc(command);
    ExpectNoErrors();
    FileDescriptorSet file_descriptor_set;
    ReadDescriptorSet("descriptor_set", &file_descriptor_set);
    for (const auto& file : file_descriptor_set.file()) {
      pool_.BuildFile(file);
    }
  }

  DescriptorPool pool_;
};

TEST_F(NameResolverTestEdition2024, MultipleFilesService) {
  CreateTempFile("foo.proto",
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

  RunProtocAndPopulatePool(
      "protocol_compiler --proto_path=$tmpdir "
      "--experimental_editions "
      "--descriptor_set_out=$tmpdir/descriptor_set foo.proto");
  ClassNameResolver resolver;
  auto nested_service =
      pool_.FindServiceByName("protobuf_unittest.NestedInFileClassService");
  auto unnested_service =
      pool_.FindServiceByName("protobuf_unittest.UnnestedService");

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

TEST_F(NameResolverTestEdition2024, MultipleFilesMessage) {
  CreateTempFile("foo.proto",
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

  RunProtocAndPopulatePool(
      "protocol_compiler --proto_path=$tmpdir "
      "--experimental_editions "
      "--descriptor_set_out=$tmpdir/descriptor_set foo.proto");
  ClassNameResolver resolver;
  auto nested_message =
      pool_.FindMessageTypeByName("protobuf_unittest.NestedInFileClassMessage");
  auto unnested_message =
      pool_.FindMessageTypeByName("protobuf_unittest.UnnestedMessage");

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

TEST_F(NameResolverTestEdition2024, MultipleFilesEnum) {
  CreateTempFile("foo.proto",
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

  RunProtocAndPopulatePool(
      "protocol_compiler --proto_path=$tmpdir "
      "--experimental_editions "
      "--descriptor_set_out=$tmpdir/descriptor_set foo.proto");
  ClassNameResolver resolver;
  auto nested_enum =
      pool_.FindEnumTypeByName("protobuf_unittest.NestedInFileClassEnum");
  auto unnested_enum =
      pool_.FindEnumTypeByName("protobuf_unittest.UnnestedEnum");

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
