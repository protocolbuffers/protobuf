// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <string>
#include <vector>

#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/file.h"
#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/annotation_test_util.h"
#include "google/protobuf/compiler/code_generator_lite.h"
#include "google/protobuf/compiler/command_line_interface.h"
#include "hpb_generator/generator.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace hpb {

namespace atu = annotation_test_util;

namespace {

using ::testing::IsEmpty;
using Annotation = GeneratedCodeInfo::Annotation;

class HpbMetadataTest : public ::testing::Test {
 public:
  // Tries to capture a FileDescriptorProto, GeneratedCodeInfo, and output
  // code from the previously added file with name `filename`. Returns true on
  // success.
  bool CaptureMetadata(const std::string& filename, FileDescriptorProto& file,
                       std::string& hpb_h, GeneratedCodeInfo& hpb_h_info,
                       absl::string_view extra_params = "") {
    CommandLineInterface cli;
    hpb_generator::Generator hpb_generator;
    cli.RegisterGenerator("--hpb_out", &hpb_generator, "");
    std::string hpb_out = absl::StrCat(
        "--hpb_out=annotate_headers=true",
        extra_params.empty() ? "" : absl::StrCat(",", extra_params), ":",
        ::testing::TempDir());

    const bool result = atu::RunProtoCompiler(filename, hpb_out, &cli, &file);

    if (!result) {
      return result;
    }

    std::string output_base =
        absl::StrCat(::testing::TempDir(), "/", StripProto(filename));

    ABSL_CHECK_OK(File::GetContents(absl::StrCat(output_base, ".hpb.h"), &hpb_h,
                                    true));
    if (!atu::DecodeMetadata(absl::StrCat(output_base, ".hpb.h.meta"),
                             &hpb_h_info)) {
      return false;
    }

    return true;
  }

  // Helper function to get annotations by the provided path and verify that
  // these annotations contain a subset of the expected annotations (by
  // substring) + semantics.
  void ExpectAnnotationsForPathContain(
      const GeneratedCodeInfo& info, const std::string& filename,
      const std::string& hpb_h, const std::vector<int>& path,
      const absl::flat_hash_map<std::string, Annotation::Semantic>&
          expected_annotations) {
    absl::flat_hash_map<std::string, Annotation::Semantic>
        expected_annotations_copy = expected_annotations;
    std::vector<const GeneratedCodeInfo::Annotation*> annotations;
    atu::FindAnnotationsOnPath(info, filename, path, &annotations);
    EXPECT_TRUE(!annotations.empty());
    for (const auto* annotation : annotations) {
      auto substring = atu::GetAnnotationSubstring(hpb_h, *annotation);
      ASSERT_TRUE(substring.has_value());
      if (auto node = expected_annotations_copy.extract(*substring);
          !node.empty()) {
        EXPECT_EQ(node.mapped(), annotation->semantic())
            << "for substring " << *substring;
      }
    }
    EXPECT_THAT(expected_annotations_copy, IsEmpty())
        << "substrings above were not found in the annotations.";
  }
};

constexpr absl::string_view kSmallTestFile = R"schema(syntax = "proto2";
package foo;
message Message { }
)schema";

TEST_F(HpbMetadataTest, CapturesMessageNames) {
  FileDescriptorProto file;
  GeneratedCodeInfo info;
  std::string hpb_h;
  atu::AddFile("test.proto", kSmallTestFile);
  EXPECT_TRUE(CaptureMetadata("test.proto", file, hpb_h, info));
  EXPECT_EQ("Message", file.message_type(0).name());
  std::vector<int> message_path;
  message_path.push_back(FileDescriptorProto::kMessageTypeFieldNumber);
  message_path.push_back(0);
  const GeneratedCodeInfo::Annotation* message_annotation =
      atu::FindAnnotationOnPath(info, "test.proto", message_path);
  EXPECT_TRUE(nullptr != message_annotation);
  EXPECT_TRUE(
      atu::AnnotationMatchesSubstring(hpb_h, message_annotation, "Message"));
}

constexpr absl::string_view kStringFieldTestFile = R"schema(
    syntax = "proto2";
    package foo;
    message Message {
      optional string sfield = 1;
      repeated string rsfield = 2;
    }
)schema";

TEST_F(HpbMetadataTest, AnnotatesStringSemantics) {
  FileDescriptorProto file;
  GeneratedCodeInfo info;
  std::string hpb_h;
  atu::AddFile("test.proto", kStringFieldTestFile);
  EXPECT_TRUE(CaptureMetadata("test.proto", file, hpb_h, info));
  EXPECT_EQ("Message", file.message_type(0).name());

  // Check annotations for `sfield`.
  std::vector<int> field_path{
      FileDescriptorProto::kMessageTypeFieldNumber,
      0,
      DescriptorProto::kFieldFieldNumber,
      0,
  };
  ExpectAnnotationsForPathContain(info, "test.proto", hpb_h, field_path,
                                  {
                                      {"sfield", Annotation::NONE},
                                      {"set_sfield", Annotation::SET},
                                      {"has_sfield", Annotation::NONE},
                                      {"clear_sfield", Annotation::NONE},
                                  });

  // Check annotations for `rsfield`.
  field_path = {
      FileDescriptorProto::kMessageTypeFieldNumber,
      0,
      DescriptorProto::kFieldFieldNumber,
      1,
  };
  ExpectAnnotationsForPathContain(info, "test.proto", hpb_h, field_path,
                                  {
                                      {"rsfield", Annotation::NONE},
                                      {"add_rsfield", Annotation::SET},
                                      {"mutable_rsfield", Annotation::ALIAS},
                                      {"rsfield_size", Annotation::NONE},
                                  });
}

TEST_F(HpbMetadataTest, GeneratesMetadataPragma) {
  FileDescriptorProto file;
  GeneratedCodeInfo info;
  std::string hpb_h;
  atu::AddFile("test.proto", kSmallTestFile);
  EXPECT_TRUE(CaptureMetadata("test.proto", file, hpb_h, info));

  EXPECT_THAT(hpb_h, ::testing::HasSubstr("#ifdef KYTHE_IS_RUNNING"));
  EXPECT_THAT(hpb_h, ::testing::HasSubstr(
                         "#pragma kythe_metadata \"test.hpb.h.meta\""));
}

TEST_F(HpbMetadataTest, GeneratesMetadataPragmaWithCppBackend) {
  FileDescriptorProto file;
  GeneratedCodeInfo info;
  std::string hpb_h;
  atu::AddFile("test.proto", kSmallTestFile);
  EXPECT_TRUE(CaptureMetadata("test.proto", file, hpb_h, info, "backend=cpp"));

  EXPECT_THAT(hpb_h, ::testing::HasSubstr("#ifdef KYTHE_IS_RUNNING"));
  EXPECT_THAT(hpb_h, ::testing::HasSubstr(
                         "#pragma kythe_metadata \"test.hpb.h.meta\""));
}

}  // namespace
}  // namespace hpb
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
