#include "google/protobuf/compiler/java/name_resolver.h"

#include <string>

#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/file.h"
#include <gmock/gmock.h>
#include "google/protobuf/testing/googletest.h"
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "google/protobuf/compiler/command_line_interface.h"
#include "google/protobuf/compiler/java/generator.h"
#include "google/protobuf/compiler/java/test_file_name.pb.h"
#include "google/protobuf/compiler/java/test_file_name_2024.pb.h"
#include "google/protobuf/compiler/java/test_multiple_file_no.pb.h"
#include "google/protobuf/compiler/java/test_multiple_file_yes.pb.h"

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

}  // namespace
}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
