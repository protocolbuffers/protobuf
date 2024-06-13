// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/annotation_test_util.h"

#include <cstdint>
#include <memory>

#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/file.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/command_line_interface.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/testing/googletest.h"
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace annotation_test_util {
namespace {

// A CodeGenerator that captures the FileDescriptor it's passed as a
// FileDescriptorProto.
class DescriptorCapturingGenerator : public CodeGenerator {
 public:
  // Does not own file; file must outlive the Generator.
  explicit DescriptorCapturingGenerator(FileDescriptorProto* file)
      : file_(file) {}

  bool Generate(const FileDescriptor* file, const std::string& parameter,
                GeneratorContext* context, std::string* error) const override {
    file->CopyTo(file_);
    return true;
  }

 private:
  FileDescriptorProto* file_;
};
}  // namespace

void AddFile(absl::string_view filename, absl::string_view data) {
  ABSL_CHECK_OK(File::SetContents(
      absl::StrCat(TestTempDir(), "/", filename), data, true));
}

bool RunProtoCompiler(const std::string& filename,
                      const std::string& plugin_specific_args,
                      CommandLineInterface* cli, FileDescriptorProto* file) {
  cli->SetInputsAreProtoPathRelative(true);

  DescriptorCapturingGenerator capturing_generator(file);
  cli->RegisterGenerator("--capture_out", &capturing_generator, "");

  std::string proto_path = absl::StrCat("-I", TestTempDir());
  std::string capture_out = absl::StrCat("--capture_out=", TestTempDir());

  const char* argv[] = {"protoc", proto_path.c_str(),
                        plugin_specific_args.c_str(), capture_out.c_str(),
                        filename.c_str()};

  return cli->Run(5, argv) == 0;
}

bool DecodeMetadata(const std::string& path, GeneratedCodeInfo* info) {
  std::string data;
  ABSL_CHECK_OK(File::GetContents(path, &data, true));
  io::ArrayInputStream input(data.data(), data.size());
  return info->ParseFromZeroCopyStream(&input);
}

void FindAnnotationsOnPath(
    const GeneratedCodeInfo& info, const std::string& source_file,
    const std::vector<int>& path,
    std::vector<const GeneratedCodeInfo::Annotation*>* annotations) {
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
      annotations->push_back(annotation);
    }
  }
}

const GeneratedCodeInfo::Annotation* FindAnnotationOnPath(
    const GeneratedCodeInfo& info, const std::string& source_file,
    const std::vector<int>& path) {
  std::vector<const GeneratedCodeInfo::Annotation*> annotations;
  FindAnnotationsOnPath(info, source_file, path, &annotations);
  if (annotations.empty()) {
    return nullptr;
  }
  return annotations[0];
}

bool AtLeastOneAnnotationMatchesSubstring(
    const std::string& file_content,
    const std::vector<const GeneratedCodeInfo::Annotation*>& annotations,
    const std::string& expected_text,
    absl::optional<GeneratedCodeInfo::Annotation::Semantic> semantic) {
  for (std::vector<const GeneratedCodeInfo::Annotation*>::const_iterator
           i = annotations.begin(),
           e = annotations.end();
       i != e; ++i) {
    const GeneratedCodeInfo::Annotation* annotation = *i;
    uint32_t begin = annotation->begin();
    uint32_t end = annotation->end();
    if (end < begin || end > file_content.size()) {
      return false;
    }
    if (file_content.substr(begin, end - begin) == expected_text) {
      return !semantic || annotation->semantic() == *semantic;
    }
  }
  return false;
}

bool AnnotationMatchesSubstring(const std::string& file_content,
                                const GeneratedCodeInfo::Annotation* annotation,
                                const std::string& expected_text) {
  std::vector<const GeneratedCodeInfo::Annotation*> annotations;
  annotations.push_back(annotation);
  return AtLeastOneAnnotationMatchesSubstring(file_content, annotations,
                                              expected_text);
}

absl::optional<absl::string_view> GetAnnotationSubstring(
    absl::string_view file_content,
    const GeneratedCodeInfo::Annotation& annotation) {
  auto begin = annotation.begin();
  auto end = annotation.end();
  if (begin < 0 || end < begin || end > file_content.size()) {
    return absl::nullopt;
  }
  return file_content.substr(begin, end - begin);
}
}  // namespace annotation_test_util
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
