// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <memory>

#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/file.h"
#include "google/protobuf/compiler/cpp/generator.h"
#include "google/protobuf/compiler/command_line_interface.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/testing/googletest.h"
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "google/protobuf/compiler/annotation_test_util.h"
#include "google/protobuf/compiler/cpp/helpers.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

namespace atu = annotation_test_util;

namespace {

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
        TestTempDir());

    const bool result = atu::RunProtoCompiler(filename, cpp_out, &cli, file);

    if (!result) {
      return result;
    }

    std::string output_base =
        absl::StrCat(TestTempDir(), "/", StripProto(filename));

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
  std::vector<int> field_path{FileDescriptorProto::kMessageTypeFieldNumber, 0,
                              DescriptorProto::kFieldFieldNumber, 0};
  std::vector<const GeneratedCodeInfo::Annotation*> annotations;
  atu::FindAnnotationsOnPath(info, "test.proto", field_path, &annotations);
  EXPECT_TRUE(!annotations.empty());
  for (const auto* annotation : annotations) {
    auto substring = atu::GetAnnotationSubstring(pb_h, *annotation);
    ASSERT_TRUE(substring.has_value());
    if (*substring == "efield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_NONE,
                annotation->semantic());
    } else if (*substring == "set_efield" || *substring == "clear_efield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_SET,
                annotation->semantic());
    }
  }
  field_path.back() = 1;
  annotations.clear();
  atu::FindAnnotationsOnPath(info, "test.proto", field_path, &annotations);
  EXPECT_TRUE(!annotations.empty());
  for (const auto* annotation : annotations) {
    auto substring = atu::GetAnnotationSubstring(pb_h, *annotation);
    ASSERT_TRUE(substring.has_value());
    if (*substring == "refield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_NONE,
                annotation->semantic());
    } else if (*substring == "set_refield" || *substring == "clear_refield" ||
               *substring == "add_refield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_SET,
                annotation->semantic());
    } else if (*substring == "mutable_refield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_ALIAS,
                annotation->semantic());
    }
  }
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
  std::vector<int> field_path{FileDescriptorProto::kMessageTypeFieldNumber, 0,
                              DescriptorProto::kFieldFieldNumber, 0};
  std::vector<const GeneratedCodeInfo::Annotation*> annotations;
  atu::FindAnnotationsOnPath(info, "test.proto", field_path, &annotations);
  EXPECT_TRUE(!annotations.empty());
  for (const auto* annotation : annotations) {
    auto substring = atu::GetAnnotationSubstring(pb_h, *annotation);
    ASSERT_TRUE(substring.has_value());
    if (*substring == "mfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_NONE,
                annotation->semantic());
    } else if (*substring == "clear_mfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_SET,
                annotation->semantic());
    } else if (*substring == "mutable_mfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_ALIAS,
                annotation->semantic());
    }
  }
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
  std::vector<int> field_path{FileDescriptorProto::kMessageTypeFieldNumber, 0,
                              DescriptorProto::kFieldFieldNumber, 0};
  std::vector<const GeneratedCodeInfo::Annotation*> annotations;
  atu::FindAnnotationsOnPath(info, "test.proto", field_path, &annotations);
  EXPECT_TRUE(!annotations.empty());
  for (const auto* annotation : annotations) {
    auto substring = atu::GetAnnotationSubstring(pb_h, *annotation);
    ASSERT_TRUE(substring.has_value());
    if (*substring == "ifield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_NONE,
                annotation->semantic());
    } else if (*substring == "set_ifield" || *substring == "clear_ifield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_SET,
                annotation->semantic());
    }
  }
  field_path.back() = 1;
  annotations.clear();
  atu::FindAnnotationsOnPath(info, "test.proto", field_path, &annotations);
  EXPECT_TRUE(!annotations.empty());
  for (const auto* annotation : annotations) {
    auto substring = atu::GetAnnotationSubstring(pb_h, *annotation);
    ASSERT_TRUE(substring.has_value());
    if (*substring == "rifield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_NONE,
                annotation->semantic());
    } else if (*substring == "set_rifield" || *substring == "clear_rifield" ||
               *substring == "add_rifield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_SET,
                annotation->semantic());
    } else if (*substring == "mutable_rifield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_ALIAS,
                annotation->semantic());
    }
  }
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
  std::vector<int> field_path{FileDescriptorProto::kMessageTypeFieldNumber, 0,
                              DescriptorProto::kFieldFieldNumber, 0};
  std::vector<const GeneratedCodeInfo::Annotation*> annotations;
  atu::FindAnnotationsOnPath(info, "test.proto", field_path, &annotations);
  EXPECT_TRUE(!annotations.empty());
  for (const auto* annotation : annotations) {
    auto substring = atu::GetAnnotationSubstring(pb_h, *annotation);
    ASSERT_TRUE(substring.has_value());
    if (*substring == "sfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_NONE,
                annotation->semantic());
    } else if (*substring == "set_sfield" || *substring == "clear_sfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_SET,
                annotation->semantic());
    } else if (*substring == "mutable_sfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_ALIAS,
                annotation->semantic());
    }
  }
  field_path.back() = 1;
  annotations.clear();
  atu::FindAnnotationsOnPath(info, "test.proto", field_path, &annotations);
  EXPECT_TRUE(!annotations.empty());
  for (const auto* annotation : annotations) {
    auto substring = atu::GetAnnotationSubstring(pb_h, *annotation);
    ASSERT_TRUE(substring.has_value());
    if (*substring == "rsfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_NONE,
                annotation->semantic());
    } else if (*substring == "set_rsfield" || *substring == "clear_rsfield" ||
               *substring == "add_rsfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_SET,
                annotation->semantic());
    } else if (*substring == "mutable_rsfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_ALIAS,
                annotation->semantic());
    }
  }
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
  std::vector<int> field_path{FileDescriptorProto::kMessageTypeFieldNumber, 0,
                              DescriptorProto::kFieldFieldNumber, 0};
  std::vector<const GeneratedCodeInfo::Annotation*> annotations;
  atu::FindAnnotationsOnPath(info, "test.proto", field_path, &annotations);
  EXPECT_TRUE(!annotations.empty());
  for (const auto* annotation : annotations) {
    auto substring = atu::GetAnnotationSubstring(pb_h, *annotation);
    ASSERT_TRUE(substring.has_value());
    if (*substring == "sfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_NONE,
                annotation->semantic());
    } else if (*substring == "set_sfield" || *substring == "set_alias_sfield" ||
               *substring == "clear_sfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_SET,
                annotation->semantic());
    } else if (*substring == "mutable_sfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_ALIAS,
                annotation->semantic());
    }
  }
  field_path.back() = 1;
  annotations.clear();
  atu::FindAnnotationsOnPath(info, "test.proto", field_path, &annotations);
  EXPECT_TRUE(!annotations.empty());
  for (const auto* annotation : annotations) {
    auto substring = atu::GetAnnotationSubstring(pb_h, *annotation);
    ASSERT_TRUE(substring.has_value());
    if (*substring == "rsfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_NONE,
                annotation->semantic());
    } else if (*substring == "set_rsfield" ||
               *substring == "set_alias_rsfield" ||
               *substring == "clear_rsfield" ||
               *substring == "add_alias_rsfield" ||
               *substring == "add_rsfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_SET,
                annotation->semantic());
    } else if (*substring == "mutable_rsfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_ALIAS,
                annotation->semantic());
    }
  }
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
  std::vector<int> field_path{FileDescriptorProto::kMessageTypeFieldNumber, 0,
                              DescriptorProto::kFieldFieldNumber, 0};
  std::vector<const GeneratedCodeInfo::Annotation*> annotations;
  atu::FindAnnotationsOnPath(info, "test.proto", field_path, &annotations);
  EXPECT_TRUE(!annotations.empty());
  for (const auto* annotation : annotations) {
    auto substring = atu::GetAnnotationSubstring(pb_h, *annotation);
    ASSERT_TRUE(substring.has_value());
    if (*substring == "sfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_NONE,
                annotation->semantic());
    } else if (*substring == "set_sfield" || *substring == "clear_sfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_SET,
                annotation->semantic());
    } else if (*substring == "mutable_sfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_ALIAS,
                annotation->semantic());
    }
  }
  field_path.back() = 1;
  annotations.clear();
  atu::FindAnnotationsOnPath(info, "test.proto", field_path, &annotations);
  EXPECT_TRUE(!annotations.empty());
  for (const auto* annotation : annotations) {
    auto substring = atu::GetAnnotationSubstring(pb_h, *annotation);
    ASSERT_TRUE(substring.has_value());
    if (*substring == "rsfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_NONE,
                annotation->semantic());
    } else if (*substring == "set_rsfield" || *substring == "clear_rsfield" ||
               *substring == "add_rsfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_SET,
                annotation->semantic());
    } else if (*substring == "mutable_rsfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_ALIAS,
                annotation->semantic());
    }
  }
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
  std::vector<int> field_path;
  field_path.push_back(FileDescriptorProto::kMessageTypeFieldNumber);
  field_path.push_back(1);
  field_path.push_back(DescriptorProto::kFieldFieldNumber);
  field_path.push_back(0);
  std::vector<const GeneratedCodeInfo::Annotation*> annotations;
  atu::FindAnnotationsOnPath(info, "test.proto", field_path, &annotations);
  EXPECT_TRUE(!annotations.empty());
  for (const auto* annotation : annotations) {
    auto substring = atu::GetAnnotationSubstring(pb_h, *annotation);
    ASSERT_TRUE(substring.has_value());
    if (*substring == "mfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_NONE,
                annotation->semantic());
    } else if (*substring == "clear_mfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_SET,
                annotation->semantic());
    } else if (*substring == "mutable_mfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_ALIAS,
                annotation->semantic());
    }
  }
  field_path.back() = 1;
  annotations.clear();
  atu::FindAnnotationsOnPath(info, "test.proto", field_path, &annotations);
  EXPECT_TRUE(!annotations.empty());
  for (const auto* annotation : annotations) {
    auto substring = atu::GetAnnotationSubstring(pb_h, *annotation);
    ASSERT_TRUE(substring.has_value());
    if (*substring == "rmfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_NONE,
                annotation->semantic());
    } else if (*substring == "add_rmfield" || *substring == "clear_rmfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_SET,
                annotation->semantic());
    } else if (*substring == "mutable_rmfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_ALIAS,
                annotation->semantic());
    }
  }
  field_path.clear();
  field_path.push_back(FileDescriptorProto::kMessageTypeFieldNumber);
  field_path.push_back(1);
  field_path.push_back(DescriptorProto::kOneofDeclFieldNumber);
  field_path.push_back(0);
  annotations.clear();
  atu::FindAnnotationsOnPath(info, "test.proto", field_path, &annotations);
  EXPECT_TRUE(!annotations.empty());
  for (const auto* annotation : annotations) {
    auto substring = atu::GetAnnotationSubstring(pb_h, *annotation);
    ASSERT_TRUE(substring.has_value());
    if (*substring == "ofield_case") {
      EXPECT_EQ(GeneratedCodeInfo::Annotation::NONE, annotation->semantic());
    } else if (*substring == "clear_ofield") {
      EXPECT_EQ(GeneratedCodeInfo::Annotation::SET, annotation->semantic());
    }
  }
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
  std::vector<int> field_path;
  field_path.push_back(FileDescriptorProto::kMessageTypeFieldNumber);
  field_path.push_back(1);
  field_path.push_back(DescriptorProto::kFieldFieldNumber);
  field_path.push_back(0);
  std::vector<const GeneratedCodeInfo::Annotation*> annotations;
  atu::FindAnnotationsOnPath(info, "test.proto", field_path, &annotations);
  EXPECT_TRUE(!annotations.empty());
  for (const auto* annotation : annotations) {
    auto substring = atu::GetAnnotationSubstring(pb_h, *annotation);
    ASSERT_TRUE(substring.has_value());
    if (*substring == "mfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_NONE,
                annotation->semantic());
    } else if (*substring == "mutable_mfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_ALIAS,
                annotation->semantic());
    } else if (*substring == "set_encoded_mfield" ||
               *substring == "clear_mfield") {
      EXPECT_EQ(GeneratedCodeInfo_Annotation_Semantic_SET,
                annotation->semantic());
    }
  }
}
}  // namespace
}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
