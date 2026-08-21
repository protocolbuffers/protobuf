#include <string>
#include <utility>
#include <vector>

#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/file.h"
#include "google/protobuf/descriptor.pb.h"
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/ascii.h"
#include "absl/strings/escaping.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
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

}  // namespace
}  // namespace protobuf
}  // namespace google::compiler::kotlin
