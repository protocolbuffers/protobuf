// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <string>
#include <vector>

#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/file.h"
#include "google/protobuf/compiler/command_line_interface.h"
#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/annotation_test_util.h"
#include "google/protobuf/compiler/cpp/generator.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

namespace atu = annotation_test_util;

namespace {

using ::testing::IsEmpty;
using Annotation = GeneratedCodeInfo::Annotation;

class CppMetadataTest : public ::testing::Test {
 public:
  // Tries to capture a FileDescriptorProto, GeneratedCodeInfo, and output
  // code from the previously added file with name `filename`. Returns true on
  // success. If pb_h is non-null, expects a .pb.h and a .pb.h.meta (copied to
  // pb_h and pb_h_info respecfively); similarly for proto_h and proto_h_info.
  bool CaptureMetadata(const std::string& filename, FileDescriptorProto* file,
                       std::string* pb_h, GeneratedCodeInfo* pb_h_info,
                       std::string* proto_h, GeneratedCodeInfo* proto_h_info,
                       std::string* pb_cc) {
    CommandLineInterface cli;
    CppGenerator cpp_generator;
    cli.RegisterGenerator("--cpp_out", &cpp_generator, "");
    std::string cpp_out = absl::StrCat(
        "--cpp_out=annotate_headers=true,"
        "annotation_pragma_name=pragma_name,"
        "annotation_guard_name=guard_name:",
        ::testing::TempDir());

    const bool result = atu::RunProtoCompiler(filename, cpp_out, &cli, file);

    if (!result) {
      return result;
    }

    std::string output_base =
        absl::StrCat(::testing::TempDir(), "/", StripProto(filename));

    if (pb_cc != nullptr) {
      ABSL_CHECK_OK(File::GetContents(absl::StrCat(output_base, ".pb.cc"),
                                      pb_cc, true));
    }

    if (pb_h != nullptr && pb_h_info != nullptr) {
      ABSL_CHECK_OK(File::GetContents(absl::StrCat(output_base, ".pb.h"), pb_h,
                                      true));
      if (!atu::DecodeMetadata(absl::StrCat(output_base, ".pb.h.meta"),
                               pb_h_info)) {
        return false;
      }
    }

    if (proto_h != nullptr && proto_h_info != nullptr) {
      ABSL_CHECK_OK(File::GetContents(absl::StrCat(output_base, ".proto.h"),
                                      proto_h, true));
      if (!atu::DecodeMetadata(absl::StrCat(output_base, ".proto.h.meta"),
                               proto_h_info)) {
        return false;
      }
    }

    return true;
  }

  // Helper function to get annotations by the provided path and vefify that
  // these annotations contain a subset of the expected annotations (by
  // substring) + semantics.
  void ExpectAnnotationsForPathContain(
      const GeneratedCodeInfo& info, const std::string& filename,
      const std::string& pb_h, const std::vector<int>& path,
      const absl::flat_hash_map<std::string, Annotation::Semantic>&
          expected_annotations) {
    // Create a copy of the expected annotations so that we can erase the
    // annotations that we found.
    absl::flat_hash_map<std::string, Annotation::Semantic>
        expected_annotations_copy = expected_annotations;
    std::vector<const GeneratedCodeInfo::Annotation*> annotations;
    atu::FindAnnotationsOnPath(info, filename, path, &annotations);
    EXPECT_TRUE(!annotations.empty());
    for (const auto* annotation : annotations) {
      auto substring = atu::GetAnnotationSubstring(pb_h, *annotation);
      ASSERT_TRUE(substring.has_value());
      if (expected_annotations_copy.contains(*substring)) {
        EXPECT_EQ(expected_annotations_copy.at(*substring),
                  annotation->semantic())
            << "for substring " << *substring;
        expected_annotations_copy.erase(*substring);
      }
    }
    EXPECT_THAT(expected_annotations_copy, IsEmpty())
        << "substrings above were not found in the annotations.";
  }
};

constexpr absl::string_view kSmallTestFile =
    "syntax = \"proto2\";\n"
    "package foo;\n"
    "enum Enum { VALUE = 0; }\n"
    "message Message { }\n";

TEST_F(CppMetadataTest, CapturesEnumNames) {
  FileDescriptorProto file;
  GeneratedCodeInfo info;
  std::string pb_h;
  atu::AddFile("test.proto", kSmallTestFile);
  EXPECT_TRUE(CaptureMetadata("test.proto", &file, &pb_h, &info, nullptr,
                              nullptr, nullptr));
  EXPECT_EQ("Enum", file.enum_type(0).name());
  std::vector<int> enum_path;
  enum_path.push_back(FileDescriptorProto::kEnumTypeFieldNumber);
  enum_path.push_back(0);
  const GeneratedCodeInfo::Annotation* enum_annotation =
      atu::FindAnnotationOnPath(info, "test.proto", enum_path);
  EXPECT_TRUE(nullptr != enum_annotation);
  EXPECT_TRUE(atu::AnnotationMatchesSubstring(pb_h, enum_annotation, "Enum"));
}

TEST_F(CppMetadataTest, AddsPragma) {
  FileDescriptorProto file;
  GeneratedCodeInfo info;
  std::string pb_h;
  atu::AddFile("test.proto", kSmallTestFile);
  EXPECT_TRUE(CaptureMetadata("test.proto", &file, &pb_h, &info, nullptr,
                              nullptr, nullptr));
  EXPECT_TRUE(pb_h.find("#ifdef guard_name") != std::string::npos);
  EXPECT_TRUE(pb_h.find("#pragma pragma_name \"test.pb.h.meta\"") !=
              std::string::npos);
}

TEST_F(CppMetadataTest, CapturesMessageNames) {
  FileDescriptorProto file;
  GeneratedCodeInfo info;
  std::string pb_h;
  atu::AddFile("test.proto", kSmallTestFile);
  EXPECT_TRUE(CaptureMetadata("test.proto", &file, &pb_h, &info, nullptr,
                              nullptr, nullptr));
  EXPECT_EQ("Message", file.message_type(0).name());
  std::vector<int> message_path;
  message_path.push_back(FileDescriptorProto::kMessageTypeFieldNumber);
  message_path.push_back(0);
  const GeneratedCodeInfo::Annotation* message_annotation =
      atu::FindAnnotationOnPath(info, "test.proto", message_path);
  EXPECT_TRUE(nullptr != message_annotation);
  EXPECT_TRUE(
      atu::AnnotationMatchesSubstring(pb_h, message_annotation, "Message"));
}

TEST_F(CppMetadataTest, RangeChecksWork) {
  absl::string_view test = "test";
  GeneratedCodeInfo::Annotation annotation;
  annotation.set_begin(-1);
  annotation.set_end(0);
  EXPECT_FALSE(atu::GetAnnotationSubstring(test, annotation).has_value());
  annotation.set_begin(1);
  EXPECT_FALSE(atu::GetAnnotationSubstring(test, annotation).has_value());
  annotation.set_begin(0);
  annotation.set_end(1);
  EXPECT_TRUE(atu::GetAnnotationSubstring(test, annotation).has_value());
  annotation.set_begin(4);
  annotation.set_end(4);
  ASSERT_TRUE(atu::GetAnnotationSubstring(test, annotation).has_value());
  EXPECT_EQ("", *atu::GetAnnotationSubstring(test, annotation));
  annotation.set_end(5);
  EXPECT_FALSE(atu::GetAnnotationSubstring(test, annotation).has_value());
}

constexpr absl::string_view kEnumFieldTestFile = R"(
  syntax = "proto2";
  package foo;
  enum Enum { VALUE = 0; }
  message Message {
    optional Enum efield = 1;
    repeated Enum refield = 2;
  }
)";

TEST_F(CppMetadataTest, AnnotatesEnumSemantics) {
  FileDescriptorProto file;
  GeneratedCodeInfo info;
  std::string pb_h;
  atu::AddFile("test.proto", kEnumFieldTestFile);
  EXPECT_TRUE(CaptureMetadata("test.proto", &file, &pb_h, &info, nullptr,
                              nullptr, nullptr));
  EXPECT_EQ("Message", file.message_type(0).name());

  // Check annotations for `efield`.
  std::vector<int> field_path{
      FileDescriptorProto::kMessageTypeFieldNumber,
      0,
      DescriptorProto::kFieldFieldNumber,
      0,
  };
  ExpectAnnotationsForPathContain(info, "test.proto", pb_h, field_path,
                                  {{"efield", Annotation::NONE},
                                   {"set_efield", Annotation::SET},
                                   {"clear_efield", Annotation::SET}});

  // Check annotations for `refield`.
  field_path = {
      FileDescriptorProto::kMessageTypeFieldNumber,
      0,
      DescriptorProto::kFieldFieldNumber,
      1,
  };
  ExpectAnnotationsForPathContain(info, "test.proto", pb_h, field_path,
                                  {{"refield", Annotation::NONE},
                                   {"set_refield", Annotation::SET},
                                   {"clear_refield", Annotation::SET},
                                   {"add_refield", Annotation::SET},
                                   {"mutable_refield", Annotation::ALIAS}});
}

constexpr absl::string_view kMapFieldTestFile = R"(
  syntax = "proto2";
  package foo;
  message Message {
    map<string, int32> mfield = 1;
  }
)";

TEST_F(CppMetadataTest, AnnotatesMapSemantics) {
  FileDescriptorProto file;
  GeneratedCodeInfo info;
  std::string pb_h;
  atu::AddFile("test.proto", kMapFieldTestFile);
  EXPECT_TRUE(CaptureMetadata("test.proto", &file, &pb_h, &info, nullptr,
                              nullptr, nullptr));
  EXPECT_EQ("Message", file.message_type(0).name());

  // Check annotations for `mfield`.
  std::vector<int> field_path{
      FileDescriptorProto::kMessageTypeFieldNumber,
      0,
      DescriptorProto::kFieldFieldNumber,
      0,
  };
  ExpectAnnotationsForPathContain(info, "test.proto", pb_h, field_path,
                                  {{"mfield", Annotation::NONE},
                                   {"clear_mfield", Annotation::SET},
                                   {"mutable_mfield", Annotation::ALIAS}});
}

constexpr absl::string_view kPrimFieldTestFile = R"(
  syntax = "proto2";
  package foo;
  message Message {
    optional int32 ifield = 1;
    repeated int32 rifield = 2;
  }
)";

TEST_F(CppMetadataTest, AnnotatesPrimSemantics) {
  FileDescriptorProto file;
  GeneratedCodeInfo info;
  std::string pb_h;
  atu::AddFile("test.proto", kPrimFieldTestFile);
  EXPECT_TRUE(CaptureMetadata("test.proto", &file, &pb_h, &info, nullptr,
                              nullptr, nullptr));
  EXPECT_EQ("Message", file.message_type(0).name());

  // Check annotations for `ifield`.
  std::vector<int> field_path{
      FileDescriptorProto::kMessageTypeFieldNumber,
      0,
      DescriptorProto::kFieldFieldNumber,
      0,
  };
  ExpectAnnotationsForPathContain(info, "test.proto", pb_h, field_path,
                                  {{"ifield", Annotation::NONE},
                                   {"set_ifield", Annotation::SET},
                                   {"clear_ifield", Annotation::SET}});

  // Check annotations for `rifield`.
  field_path = {
      FileDescriptorProto::kMessageTypeFieldNumber,
      0,
      DescriptorProto::kFieldFieldNumber,
      1,
  };
  ExpectAnnotationsForPathContain(info, "test.proto", pb_h, field_path,
                                  {{"rifield", Annotation::NONE},
                                   {"set_rifield", Annotation::SET},
                                   {"clear_rifield", Annotation::SET},
                                   {"add_rifield", Annotation::SET},
                                   {"mutable_rifield", Annotation::ALIAS}});
}

constexpr absl::string_view kCordFieldTestFile = R"(
    syntax = "proto2";
    package foo;
    message Message {
      optional string sfield = 1 [ctype = CORD];
      repeated string rsfield = 2 [ctype = CORD];
    }
)";

TEST_F(CppMetadataTest, AnnotatesCordSemantics) {
  FileDescriptorProto file;
  GeneratedCodeInfo info;
  std::string pb_h;
  atu::AddFile("test.proto", kCordFieldTestFile);
  EXPECT_TRUE(CaptureMetadata("test.proto", &file, &pb_h, &info, nullptr,
                              nullptr, nullptr));
  EXPECT_EQ("Message", file.message_type(0).name());

  // Check annotations for `sfield`.
  std::vector<int> field_path{
      FileDescriptorProto::kMessageTypeFieldNumber,
      0,
      DescriptorProto::kFieldFieldNumber,
      0,
  };
  ExpectAnnotationsForPathContain(info, "test.proto", pb_h, field_path,
                                  {{"sfield", Annotation::NONE},
                                   {"set_sfield", Annotation::SET},
                                   {"clear_sfield", Annotation::SET},
                                   {"mutable_sfield", Annotation::ALIAS}});

  // Check annotations for `rsfield`.
  field_path = {
      FileDescriptorProto::kMessageTypeFieldNumber,
      0,
      DescriptorProto::kFieldFieldNumber,
      1,
  };
  ExpectAnnotationsForPathContain(info, "test.proto", pb_h, field_path,
                                  {{"rsfield", Annotation::NONE},
                                   {"clear_rsfield", Annotation::SET},
                                   {"add_rsfield", Annotation::SET},
                                   {"mutable_rsfield", Annotation::ALIAS}});
}

constexpr absl::string_view kStringPieceFieldTestFile = R"(
    syntax = "proto2";
    package foo;
    message Message {
      optional string sfield = 1 [ctype = STRING_PIECE];
      repeated string rsfield = 2 [ctype = STRING_PIECE];
    }
)";

TEST_F(CppMetadataTest, AnnotatesStringPieceSemantics) {
  FileDescriptorProto file;
  GeneratedCodeInfo info;
  std::string pb_h;
  atu::AddFile("test.proto", kStringPieceFieldTestFile);
  EXPECT_TRUE(CaptureMetadata("test.proto", &file, &pb_h, &info, nullptr,
                              nullptr, nullptr));
  EXPECT_EQ("Message", file.message_type(0).name());

  // Check annotations for `sfield`.
  std::vector<int> field_path{
      FileDescriptorProto::kMessageTypeFieldNumber,
      0,
      DescriptorProto::kFieldFieldNumber,
      0,
  };
  ExpectAnnotationsForPathContain(info, "test.proto", pb_h, field_path,
                                  {{"sfield", Annotation::NONE},
                                   {"set_sfield", Annotation::SET},
                                   {"clear_sfield", Annotation::SET},
                                   {"mutable_sfield", Annotation::ALIAS}});

  // Check annotations for `rsfield`.
  field_path = {
      FileDescriptorProto::kMessageTypeFieldNumber,
      0,
      DescriptorProto::kFieldFieldNumber,
      1,
  };
  ExpectAnnotationsForPathContain(info, "test.proto", pb_h, field_path,
                                  {{"rsfield", Annotation::NONE},
                                   {"clear_rsfield", Annotation::SET},
                                   {"add_rsfield", Annotation::SET},
                                   {"mutable_rsfield", Annotation::ALIAS}});
}

constexpr absl::string_view kStringFieldTestFile = R"(
    syntax = "proto2";
    package foo;
    message Message {
      optional string sfield = 1;
      repeated string rsfield = 2;
    }
)";

TEST_F(CppMetadataTest, AnnotatesStringSemantics) {
  FileDescriptorProto file;
  GeneratedCodeInfo info;
  std::string pb_h;
  atu::AddFile("test.proto", kStringFieldTestFile);
  EXPECT_TRUE(CaptureMetadata("test.proto", &file, &pb_h, &info, nullptr,
                              nullptr, nullptr));
  EXPECT_EQ("Message", file.message_type(0).name());

  // Check annotations for `sfield`.
  std::vector<int> field_path{
      FileDescriptorProto::kMessageTypeFieldNumber,
      0,
      DescriptorProto::kFieldFieldNumber,
      0,
  };
  ExpectAnnotationsForPathContain(
      info, "test.proto", pb_h, field_path,
      {
          {"sfield", Annotation::NONE},
          {"set_sfield", Annotation::SET},
          {"clear_sfield", Annotation::SET},
          {"mutable_sfield", Annotation::ALIAS},
          // NOTE: these annotations should have a semantic of SET.
          {"set_allocated_sfield", Annotation::NONE},
          {"release_sfield", Annotation::NONE},
      });

  // Check annotations for `rsfield`.
  field_path = {
      FileDescriptorProto::kMessageTypeFieldNumber,
      0,
      DescriptorProto::kFieldFieldNumber,
      1,
  };
  ExpectAnnotationsForPathContain(info, "test.proto", pb_h, field_path,
                                  {
                                      {"rsfield", Annotation::NONE},
                                      {"clear_rsfield", Annotation::SET},
                                      {"add_rsfield", Annotation::SET},
                                      {"mutable_rsfield", Annotation::ALIAS},
                                  });
}

constexpr absl::string_view kMessageFieldTestFile = R"(
    syntax = "proto2";
    package foo;
    message SMessage { }
    message Message {
      optional SMessage mfield = 1;
      repeated SMessage rmfield = 2;
      oneof ofield {
        int32 oint = 3;
      }
    }
)";

TEST_F(CppMetadataTest, AnnotatesMessageSemantics) {
  FileDescriptorProto file;
  GeneratedCodeInfo info;
  std::string pb_h;
  atu::AddFile("test.proto", kMessageFieldTestFile);
  EXPECT_TRUE(CaptureMetadata("test.proto", &file, &pb_h, &info, nullptr,
                              nullptr, nullptr));
  EXPECT_EQ("Message", file.message_type(1).name());
  // Check annotations for `mfield`.
  std::vector<int> field_path = {
      FileDescriptorProto::kMessageTypeFieldNumber,
      1,
      DescriptorProto::kFieldFieldNumber,
      0,
  };
  ExpectAnnotationsForPathContain(
      info, "test.proto", pb_h, field_path,
      {
          {"mfield", Annotation::NONE},
          {"has_mfield", Annotation::NONE},
          {"clear_mfield", Annotation::SET},
          {"mutable_mfield", Annotation::ALIAS},
          // NOTE: these annotations should have a semantic of SET.
          {"release_mfield", Annotation::NONE},
          {"set_allocated_mfield", Annotation::NONE},
      });

  // Check annotations for `rmfield`.
  field_path = {
      FileDescriptorProto::kMessageTypeFieldNumber,
      1,
      DescriptorProto::kFieldFieldNumber,
      1,
  };
  ExpectAnnotationsForPathContain(info, "test.proto", pb_h, field_path,
                                  {
                                      {"rmfield", Annotation::NONE},
                                      {"add_rmfield", Annotation::SET},
                                      {"clear_rmfield", Annotation::SET},
                                      {"mutable_rmfield", Annotation::ALIAS},
                                      {"rmfield_size", Annotation::NONE},
                                      {"kRmfieldFieldNumber", Annotation::NONE},
                                  });

  // Check annotations for `ofield`.
  field_path = {
      FileDescriptorProto::kMessageTypeFieldNumber,
      1,
      DescriptorProto::kOneofDeclFieldNumber,
      0,
  };
  ExpectAnnotationsForPathContain(info, "test.proto", pb_h, field_path,
                                  {{"ofield_case", Annotation::NONE},
                                   {"clear_ofield", Annotation::SET},
                                   {"OfieldCase", Annotation::NONE}});

  // Check annotations for `oint`.
  field_path = {
      FileDescriptorProto::kMessageTypeFieldNumber,
      1,
      DescriptorProto::kFieldFieldNumber,
      2,
  };
  ExpectAnnotationsForPathContain(info, "test.proto", pb_h, field_path,
                                  {
                                      {"kOint", Annotation::NONE},
                                      {"kOintFieldNumber", Annotation::NONE},
                                      {"has_oint", Annotation::NONE},
                                      {"clear_oint", Annotation::SET},
                                      {"set_oint", Annotation::SET},
                                      {"oint", Annotation::NONE},
                                  });
}

constexpr absl::string_view kLazyMessageFieldTestFile = R"(
    syntax = "proto2";
    package foo;
    message SMessage { }
    message Message {
      optional SMessage mfield = 1 [lazy=true];
    }
)";

TEST_F(CppMetadataTest, AnnotatesLazyMessageSemantics) {
  FileDescriptorProto file;
  GeneratedCodeInfo info;
  std::string pb_h;
  atu::AddFile("test.proto", kLazyMessageFieldTestFile);
  EXPECT_TRUE(CaptureMetadata("test.proto", &file, &pb_h, &info, nullptr,
                              nullptr, nullptr));
  EXPECT_EQ("Message", file.message_type(1).name());

  // Check annotations for `mfield`.
  std::vector<int> field_path{
      FileDescriptorProto::kMessageTypeFieldNumber,
      1,
      DescriptorProto::kFieldFieldNumber,
      0,
  };
  ExpectAnnotationsForPathContain(info, "test.proto", pb_h, field_path,
                                  {{"mfield", Annotation::NONE},
                                   {"mutable_mfield", Annotation::ALIAS},
                                   {"clear_mfield", Annotation::SET}});
}
}  // namespace
}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
