// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <memory>
#ifndef _SHARED_PTR_H
#include <google/protobuf/stubs/shared_ptr.h>
#endif

#include <google/protobuf/compiler/cpp/cpp_helpers.h>
#include <google/protobuf/compiler/cpp/cpp_generator.h>
#include <google/protobuf/compiler/command_line_interface.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/descriptor.pb.h>

#include <google/protobuf/testing/file.h>
#include <google/protobuf/testing/file.h>
#include <google/protobuf/testing/googletest.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
namespace {

// A CodeGenerator that captures the FileDescriptor it's passed as a
// FileDescriptorProto.
class DescriptorCapturingGenerator : public CodeGenerator {
 public:
  // Does not own file; file must outlive the Generator.
  explicit DescriptorCapturingGenerator(FileDescriptorProto* file)
      : file_(file) {}

  virtual bool Generate(const FileDescriptor* file, const string& parameter,
                        GeneratorContext* context, string* error) const {
    file->CopyTo(file_);
    return true;
  }

 private:
  FileDescriptorProto* file_;
};

class CppMetadataTest : public ::testing::Test {
 public:
  // Adds a file with name `filename` and content `data`.
  void AddFile(const string& filename, const string& data) {
    GOOGLE_CHECK_OK(File::SetContents(TestTempDir() + "/" + filename, data,
                               true));
  }

  // Tries to capture a FileDescriptorProto, GeneratedCodeInfo, and output
  // code from the previously added file with name `filename`. Returns true on
  // success. If pb_h is non-null, expects a .pb.h and a .pb.h.meta (copied to
  // pb_h and pb_h_info respecfively); similarly for proto_h and proto_h_info.
  bool CaptureMetadata(const string& filename, FileDescriptorProto* file,
                       string* pb_h, GeneratedCodeInfo* pb_h_info,
                       string* proto_h, GeneratedCodeInfo* proto_h_info,
                       string* pb_cc) {
    google::protobuf::compiler::CommandLineInterface cli;
    cli.SetInputsAreProtoPathRelative(true);

    CppGenerator cpp_generator;
    DescriptorCapturingGenerator capturing_generator(file);
    cli.RegisterGenerator("--cpp_out", &cpp_generator, "");
    cli.RegisterGenerator("--capture_out", &capturing_generator, "");

    string proto_path = "-I" + TestTempDir();
    string cpp_out =
        "--cpp_out=annotate_headers=true,"
        "annotation_pragma_name=pragma_name,"
        "annotation_guard_name=guard_name:" +
        TestTempDir();
    string capture_out = "--capture_out=" + TestTempDir();

    const char* argv[] = {"protoc", proto_path.c_str(), cpp_out.c_str(),
                          capture_out.c_str(), filename.c_str()};

    if (cli.Run(5, argv) != 0) {
      return false;
    }

    string output_base = TestTempDir() + "/" + StripProto(filename);

    if (pb_cc != NULL) {
      GOOGLE_CHECK_OK(
          File::GetContents(output_base + ".pb.cc", pb_cc, true));
    }

    if (pb_h != NULL && pb_h_info != NULL) {
      GOOGLE_CHECK_OK(
          File::GetContents(output_base + ".pb.h", pb_h, true));
      if (!DecodeMetadata(output_base + ".pb.h.meta", pb_h_info)) {
        return false;
      }
    }

    if (proto_h != NULL && proto_h_info != NULL) {
      GOOGLE_CHECK_OK(File::GetContents(output_base + ".proto.h", proto_h,
                                 true));
      if (!DecodeMetadata(output_base + ".proto.h.meta", proto_h_info)) {
        return false;
      }
    }

    return true;
  }

 private:
  // Decodes GeneratedCodeInfo stored in path and copies it to info.
  // Returns true on success.
  bool DecodeMetadata(const string& path, GeneratedCodeInfo* info) {
    string data;
    GOOGLE_CHECK_OK(File::GetContents(path, &data, true));
    io::ArrayInputStream input(data.data(), data.size());
    return info->ParseFromZeroCopyStream(&input);
  }
};

const char kSmallTestFile[] =
    "syntax = \"proto2\";\n"
    "package foo;\n"
    "enum Enum { VALUE = 0; }\n"
    "message Message { }\n";

// Finds the Annotation for a given source file and path (or returns null if it
// couldn't).
const GeneratedCodeInfo::Annotation* FindAnnotationOnPath(
    const GeneratedCodeInfo& info, const string& source_file,
    const vector<int>& path) {
  for (int i = 0; i < info.annotation_size(); ++i) {
    const GeneratedCodeInfo::Annotation* annotation = &info.annotation(i);
    if (annotation->source_file() != source_file ||
        annotation->path_size() != path.size()) {
      continue;
    }
    int node = 0;
    for (; node < path.size(); ++node) {
      if (annotation->path(node) != path[node]) {
        break;
      }
    }
    if (node == path.size()) {
      return annotation;
    }
  }
  return NULL;
}

// Returns true if the provided annotation covers a given substring in
// file_content.
bool AnnotationMatchesSubstring(const string& file_content,
                                const GeneratedCodeInfo::Annotation* annotation,
                                const string& expected_text) {
  uint32 begin = annotation->begin();
  uint32 end = annotation->end();
  if (end < begin || end > file_content.size()) {
    return false;
  }
  return file_content.substr(begin, end - begin) == expected_text;
}

TEST_F(CppMetadataTest, CapturesEnumNames) {
  FileDescriptorProto file;
  GeneratedCodeInfo info;
  string pb_h;
  AddFile("test.proto", kSmallTestFile);
  EXPECT_TRUE(
      CaptureMetadata("test.proto", &file, &pb_h, &info, NULL, NULL, NULL));
  EXPECT_EQ("Enum", file.enum_type(0).name());
  vector<int> enum_path;
  enum_path.push_back(FileDescriptorProto::kEnumTypeFieldNumber);
  enum_path.push_back(0);
  const GeneratedCodeInfo::Annotation* enum_annotation =
      FindAnnotationOnPath(info, "test.proto", enum_path);
  EXPECT_TRUE(NULL != enum_annotation);
  EXPECT_TRUE(AnnotationMatchesSubstring(pb_h, enum_annotation, "Enum"));
}

TEST_F(CppMetadataTest, AddsPragma) {
  FileDescriptorProto file;
  GeneratedCodeInfo info;
  string pb_h;
  AddFile("test.proto", kSmallTestFile);
  EXPECT_TRUE(
      CaptureMetadata("test.proto", &file, &pb_h, &info, NULL, NULL, NULL));
  EXPECT_TRUE(pb_h.find("#ifdef guard_name") != string::npos);
  EXPECT_TRUE(pb_h.find("#pragma pragma_name \"test.pb.h.meta\"") !=
              string::npos);
}

TEST_F(CppMetadataTest, CapturesMessageNames) {
  FileDescriptorProto file;
  GeneratedCodeInfo info;
  string pb_h;
  AddFile("test.proto", kSmallTestFile);
  EXPECT_TRUE(
      CaptureMetadata("test.proto", &file, &pb_h, &info, NULL, NULL, NULL));
  EXPECT_EQ("Message", file.message_type(0).name());
  vector<int> message_path;
  message_path.push_back(FileDescriptorProto::kMessageTypeFieldNumber);
  message_path.push_back(0);
  const GeneratedCodeInfo::Annotation* message_annotation =
      FindAnnotationOnPath(info, "test.proto", message_path);
  EXPECT_TRUE(NULL != message_annotation);
  EXPECT_TRUE(AnnotationMatchesSubstring(pb_h, message_annotation, "Message"));
}

}  // namespace
}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
