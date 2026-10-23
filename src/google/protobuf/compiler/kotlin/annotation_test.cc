#include <string>
#include <vector>

#include "google/protobuf/testing/file.h"
#include "google/protobuf/descriptor.pb.h"
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/ascii.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/annotation_test_util.h"
#include "google/protobuf/compiler/command_line_interface.h"
#include "google/protobuf/compiler/kotlin/generator.h"

namespace atu = ::google::protobuf::compiler::annotation_test_util;

namespace google::protobuf::compiler::kotlin {
namespace {

using Annotation = GeneratedCodeInfo::Annotation;
using Semantic = Annotation::Semantic;

const int kFieldFieldNumber = DescriptorProto::kFieldFieldNumber;
const int kMessageTypeFieldNumber =
    FileDescriptorProto::kMessageTypeFieldNumber;

class KotlinMetadataTest : public ::testing::Test {
 public:
  bool CaptureMetadata(const std::string& filename, FileDescriptorProto* file,
                       const std::vector<atu::ExpectedOutput*>& outputs) {
    google::protobuf::compiler::CommandLineInterface cli;

    KotlinGenerator kotlin_generator;
    cli.RegisterGenerator("--kotlin_out", &kotlin_generator, "");
    std::string kotlin_out = "--kotlin_out=";
    kotlin_out += "annotate_code,";

    kotlin_out += ":" + ::testing::TempDir();
    const bool result = atu::RunProtoCompiler(filename, kotlin_out, &cli, file);

    for (auto output : outputs) {
      ABSL_CHECK_OK(
          File::GetContents(::testing::TempDir() + "/" + output->file_path,
                            &output->file_content, true));

      // Extra annotation metadata which is embedded as base64 proto
      // in a comment on of format `// google.protobuf.GeneratedCodeInfo: <data>`
      std::string comment = "// google.protobuf.GeneratedCodeInfo: ";
      auto comment_pos = output->file_content.find(comment);
      ABSL_CHECK_NE(comment_pos, std::string::npos);
      std::string encoded_annotations_line =
          output->file_content.substr(comment_pos + comment.size());
      absl::StripAsciiWhitespace(&encoded_annotations_line);
      std::string unencoded_annotation_string;
      absl::Base64Unescape(encoded_annotations_line,
                           &unencoded_annotation_string);
      ABSL_CHECK(
          output->file_info.ParseFromString(unencoded_annotation_string));
    }
    return result;
  }
};

void CheckAnnotation(const atu::ExpectedOutput& output,
                     const std::vector<int>& path,
                     const std::string& expected_text,
                     Semantic expected_semantic = Annotation::NONE) {
  std::vector<const GeneratedCodeInfo::Annotation*> annotations;
  atu::FindAnnotationsOnPath(output.file_info, "test.proto", path,
                             &annotations);
  EXPECT_GT(annotations.size(), 0);

  EXPECT_TRUE(atu::AtLeastOneAnnotationMatchesSubstring(
      output.file_content, annotations, expected_text, expected_semantic))
      << "Didn't find " << expected_text << " in annotations for"
      << output.file_path;
}

TEST_F(KotlinMetadataTest, CapturesFooOrNull) {
  atu::AddFile("test.proto", R"(
    syntax = "proto3";
    package bar;
    message Message {
      Message foo = 1;
    }
  )");
  FileDescriptorProto file;
  atu::ExpectedOutput output("com/google/protos/bar/MessageKt.kt");
  EXPECT_TRUE(CaptureMetadata("test.proto", &file, {&output}));
  std::vector<int> foo_path = {kMessageTypeFieldNumber, 0, kFieldFieldNumber,
                               0};
  CheckAnnotation(output, foo_path, "fooOrNull",
                  /* expected_semantic= */ Annotation::NONE);
  CheckAnnotation(output, foo_path, "foo",
                  /* expected_semantic= */ Annotation::NONE);
  CheckAnnotation(output, foo_path, "get",
                  /* expected_semantic= */ Annotation::NONE);
  CheckAnnotation(output, foo_path, "set",
                  /* expected_semantic= */ Annotation::SET);
  CheckAnnotation(output, foo_path, "hasFoo",
                  /* expected_semantic= */ Annotation::NONE);
  CheckAnnotation(output, foo_path, "clearFoo",
                  /* expected_semantic= */ Annotation::SET);
}

TEST_F(KotlinMetadataTest, StripDocComments) {
  atu::AddFile("strip_comments.proto", R"(
    syntax = "proto3";
    package bar;
    option java_package = "com.example.bar";

    // Leading comment on StripMessage.
    message StripMessage {
      // Leading comment on nested enum Mode.
      enum Mode {
        // Leading comment on enum value MODE_UNSPECIFIED.
        MODE_UNSPECIFIED = 0;
      }
      // Leading comment on field alpha.
      string alpha = 1;
      // Leading comment on deprecated field beta.
      int32 beta = 2 [deprecated = true];
      // Leading comment on oneof variant.
      oneof variant {
        // Leading comment on oneof field flag.
        bool flag = 3;
      }
      // Leading comment on map field tags.
      map<string, int32> tags = 4;
      Mode mode = 5;
    }
  )");

  FileDescriptorProto file;
  google::protobuf::compiler::CommandLineInterface cli;
  KotlinGenerator kotlin_generator;
  cli.RegisterGenerator("--kotlin_out", &kotlin_generator, "");

  std::string default_dir = ::testing::TempDir() + "/default_out";
  ABSL_CHECK_OK(
      File::RecursivelyCreateDir(default_dir, 0777));
  ASSERT_TRUE(atu::RunProtoCompiler(
      "strip_comments.proto", "--kotlin_out=:" + default_dir, &cli, &file));
  std::string default_kt;
  ABSL_CHECK_OK(
      File::GetContents(default_dir + "/com/example/bar/StripMessageKt.kt",
                        &default_kt, true));
  EXPECT_NE(default_kt.find("Leading comment on StripMessage."),
            std::string::npos);
  EXPECT_NE(default_kt.find("Leading comment on field alpha."),
            std::string::npos);
  EXPECT_NE(default_kt.find("Leading comment on map field tags."),
            std::string::npos);

  struct StripTestCase {
    absl::string_view opts;
    absl::string_view dir_suffix;
  };
  for (const StripTestCase test_case :
       {StripTestCase{"strip_doc_comments:", "stripped_full"},
        StripTestCase{"lite,strip_doc_comments:", "stripped_lite"}}) {
    std::string stripped_dir =
        absl::StrCat(::testing::TempDir(), "/", test_case.dir_suffix);
    ABSL_CHECK_OK(
        File::RecursivelyCreateDir(stripped_dir, 0777));
    ASSERT_TRUE(atu::RunProtoCompiler(
        "strip_comments.proto",
        absl::StrCat("--kotlin_out=", test_case.opts, stripped_dir), &cli,
        &file));
    std::string stripped_kt;
    ABSL_CHECK_OK(
        File::GetContents(stripped_dir + "/com/example/bar/StripMessageKt.kt",
                          &stripped_kt, true));
    EXPECT_EQ(stripped_kt.find("Leading comment on StripMessage."),
              std::string::npos);
    EXPECT_EQ(stripped_kt.find("Leading comment on field alpha."),
              std::string::npos);
    EXPECT_EQ(stripped_kt.find("Leading comment on deprecated field beta."),
              std::string::npos);
    EXPECT_EQ(stripped_kt.find("Leading comment on oneof field flag."),
              std::string::npos);
    EXPECT_EQ(stripped_kt.find("Leading comment on map field tags."),
              std::string::npos);
  }
}

TEST_F(KotlinMetadataTest, StripDocCommentsBitForBitIdentical) {
  atu::AddFile("identical.proto", R"(
    syntax = "proto3";
    package bar;
    option java_package = "com.example.bar";

    // Version 1 message comment.
    message IdenticalMessage {
      // Version 1 field comment.
      string name = 1;
      // Version 1 map comment.
      map<string, int32> counts = 2;
    }
  )");

  FileDescriptorProto file;
  google::protobuf::compiler::CommandLineInterface cli;
  KotlinGenerator kotlin_generator;
  cli.RegisterGenerator("--kotlin_out", &kotlin_generator, "");

  std::string out1 = ::testing::TempDir() + "/identical_out1";
  std::string lite1 = ::testing::TempDir() + "/identical_lite1";
  ABSL_CHECK_OK(File::RecursivelyCreateDir(out1, 0777));
  ABSL_CHECK_OK(File::RecursivelyCreateDir(lite1, 0777));
  ASSERT_TRUE(atu::RunProtoCompiler("identical.proto",
                                    "--kotlin_out=strip_doc_comments:" + out1,
                                    &cli, &file));
  ASSERT_TRUE(atu::RunProtoCompiler(
      "identical.proto", "--kotlin_out=lite,strip_doc_comments:" + lite1, &cli,
      &file));

  atu::AddFile("identical.proto", R"(
    syntax = "proto3";
    package bar;
    option java_package = "com.example.bar";

    // Completely rewritten multi-line Version 2 comment.
    // Extra line shifting source line numbers below.
    message IdenticalMessage {
      // Updated comment on field name.
      // Another line of explanation.
      string name = 1;
      // Updated map comment.
      // Extra line.
      map<string, int32> counts = 2;
    }
  )");

  std::string out2 = ::testing::TempDir() + "/identical_out2";
  std::string lite2 = ::testing::TempDir() + "/identical_lite2";
  ABSL_CHECK_OK(File::RecursivelyCreateDir(out2, 0777));
  ABSL_CHECK_OK(File::RecursivelyCreateDir(lite2, 0777));
  ASSERT_TRUE(atu::RunProtoCompiler("identical.proto",
                                    "--kotlin_out=strip_doc_comments:" + out2,
                                    &cli, &file));
  ASSERT_TRUE(atu::RunProtoCompiler(
      "identical.proto", "--kotlin_out=lite,strip_doc_comments:" + lite2, &cli,
      &file));

  std::string content1;
  std::string content2;
  ABSL_CHECK_OK(
      File::GetContents(out1 + "/com/example/bar/IdenticalMessageKt.kt",
                        &content1, true));
  ABSL_CHECK_OK(
      File::GetContents(out2 + "/com/example/bar/IdenticalMessageKt.kt",
                        &content2, true));
  EXPECT_EQ(content1, content2);

  std::string lite_content1;
  std::string lite_content2;
  ABSL_CHECK_OK(
      File::GetContents(lite1 + "/com/example/bar/IdenticalMessageKt.kt",
                        &lite_content1, true));
  ABSL_CHECK_OK(
      File::GetContents(lite2 + "/com/example/bar/IdenticalMessageKt.kt",
                        &lite_content2, true));
  EXPECT_EQ(lite_content1, lite_content2);
}

}  // namespace
}  // namespace protobuf
}  // namespace google::compiler::kotlin
