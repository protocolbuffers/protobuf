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

// Author: kenton@google.com (Kenton Varda)

#include <google/protobuf/compiler/mock_code_generator.h>

#include <stdlib.h>

#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

#include <google/protobuf/stubs/strutil.h>

#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/testing/file.h>
#include <google/protobuf/testing/file.h>
#include <google/protobuf/testing/file.h>
#include <google/protobuf/compiler/plugin.pb.h>
#include <google/protobuf/descriptor.pb.h>
#include <gtest/gtest.h>
#include <google/protobuf/stubs/substitute.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/text_format.h>

#ifdef major
#undef major
#endif
#ifdef minor
#undef minor
#endif

namespace google {
namespace protobuf {
namespace compiler {

// Returns the list of the names of files in all_files in the form of a
// comma-separated string.
std::string CommaSeparatedList(
    const std::vector<const FileDescriptor*>& all_files) {
  std::vector<std::string> names;
  for (size_t i = 0; i < all_files.size(); i++) {
    names.push_back(all_files[i]->name());
  }
  return Join(names, ",");
}

static const char* kFirstInsertionPointName = "first_mock_insertion_point";
static const char* kSecondInsertionPointName = "second_mock_insertion_point";
static const char* kFirstInsertionPoint =
    "# @@protoc_insertion_point(first_mock_insertion_point) is here\n";
static const char* kSecondInsertionPoint =
    "  # @@protoc_insertion_point(second_mock_insertion_point) is here\n";

MockCodeGenerator::MockCodeGenerator(const std::string& name) : name_(name) {}

MockCodeGenerator::~MockCodeGenerator() {}

uint64_t MockCodeGenerator::GetSupportedFeatures() const {
  uint64_t all_features = CodeGenerator::FEATURE_PROTO3_OPTIONAL;
  return all_features & ~suppressed_features_;
}

void MockCodeGenerator::SuppressFeatures(uint64_t features) {
  suppressed_features_ = features;
}

void MockCodeGenerator::ExpectGenerated(
    const std::string& name, const std::string& parameter,
    const std::string& insertions, const std::string& file,
    const std::string& first_message_name,
    const std::string& first_parsed_file_name,
    const std::string& output_directory) {
  std::string content;
  GOOGLE_CHECK_OK(
      File::GetContents(output_directory + "/" + GetOutputFileName(name, file),
                        &content, true));

  std::vector<std::string> lines =
      Split(content, "\n", true);

  while (!lines.empty() && lines.back().empty()) {
    lines.pop_back();
  }
  for (size_t i = 0; i < lines.size(); i++) {
    lines[i] += "\n";
  }

  std::vector<std::string> insertion_list;
  if (!insertions.empty()) {
    insertion_list = Split(insertions, ",", true);
  }

  EXPECT_EQ(lines.size(), 3 + insertion_list.size() * 2);
  EXPECT_EQ(GetOutputFileContent(name, parameter, file, first_parsed_file_name,
                                 first_message_name),
            lines[0]);

  EXPECT_EQ(kFirstInsertionPoint, lines[1 + insertion_list.size()]);
  EXPECT_EQ(kSecondInsertionPoint, lines[2 + insertion_list.size() * 2]);

  for (size_t i = 0; i < insertion_list.size(); i++) {
    EXPECT_EQ(GetOutputFileContent(insertion_list[i], "first_insert", file,
                                   file, first_message_name),
              lines[1 + i]);
    // Second insertion point is indented, so the inserted text should
    // automatically be indented too.
    EXPECT_EQ("  " + GetOutputFileContent(insertion_list[i], "second_insert",
                                          file, file, first_message_name),
              lines[2 + insertion_list.size() + i]);
  }
}

namespace {
void CheckSingleAnnotation(const std::string& expected_file,
                           const std::string& expected_text,
                           const std::string& file_content,
                           const GeneratedCodeInfo::Annotation& annotation) {
  EXPECT_EQ(expected_file, annotation.source_file());
  ASSERT_GE(file_content.size(), annotation.begin());
  ASSERT_GE(file_content.size(), annotation.end());
  ASSERT_LE(annotation.begin(), annotation.end());
  EXPECT_EQ(expected_text.size(), annotation.end() - annotation.begin());
  EXPECT_EQ(expected_text,
            file_content.substr(annotation.begin(), expected_text.size()));
}
}  // anonymous namespace

void MockCodeGenerator::CheckGeneratedAnnotations(
    const std::string& name, const std::string& file,
    const std::string& output_directory) {
  std::string file_content;
  GOOGLE_CHECK_OK(
      File::GetContents(output_directory + "/" + GetOutputFileName(name, file),
                        &file_content, true));
  std::string meta_content;
  GOOGLE_CHECK_OK(File::GetContents(
      output_directory + "/" + GetOutputFileName(name, file) + ".pb.meta",
      &meta_content, true));
  GeneratedCodeInfo annotations;
  GOOGLE_CHECK(TextFormat::ParseFromString(meta_content, &annotations));
  ASSERT_EQ(7, annotations.annotation_size());

  CheckSingleAnnotation("first_annotation", "first", file_content,
                        annotations.annotation(0));
  CheckSingleAnnotation("first_path",
                        "test_generator: first_insert,\n foo.proto,\n "
                        "MockCodeGenerator_Annotate,\n foo.proto\n",
                        file_content, annotations.annotation(1));
  CheckSingleAnnotation("first_path",
                        "test_plugin: first_insert,\n foo.proto,\n "
                        "MockCodeGenerator_Annotate,\n foo.proto\n",
                        file_content, annotations.annotation(2));
  CheckSingleAnnotation("second_annotation", "second", file_content,
                        annotations.annotation(3));
  // This annotated text has changed because it was inserted at an indented
  // insertion point.
  CheckSingleAnnotation("second_path",
                        "test_generator: second_insert,\n   foo.proto,\n   "
                        "MockCodeGenerator_Annotate,\n   foo.proto\n",
                        file_content, annotations.annotation(4));
  CheckSingleAnnotation("second_path",
                        "test_plugin: second_insert,\n   foo.proto,\n   "
                        "MockCodeGenerator_Annotate,\n   foo.proto\n",
                        file_content, annotations.annotation(5));
  CheckSingleAnnotation("third_annotation", "third", file_content,
                        annotations.annotation(6));
}

bool MockCodeGenerator::Generate(const FileDescriptor* file,
                                 const std::string& parameter,
                                 GeneratorContext* context,
                                 std::string* error) const {
  bool annotate = false;
  for (int i = 0; i < file->message_type_count(); i++) {
    if (HasPrefixString(file->message_type(i)->name(), "MockCodeGenerator_")) {
      std::string command = StripPrefixString(
          file->message_type(i)->name(), "MockCodeGenerator_");
      if (command == "Error") {
        *error = "Saw message type MockCodeGenerator_Error.";
        return false;
      } else if (command == "Exit") {
        std::cerr << "Saw message type MockCodeGenerator_Exit." << std::endl;
        exit(123);
      } else if (command == "Abort") {
        std::cerr << "Saw message type MockCodeGenerator_Abort." << std::endl;
        abort();
      } else if (command == "HasSourceCodeInfo") {
        FileDescriptorProto file_descriptor_proto;
        file->CopySourceCodeInfoTo(&file_descriptor_proto);
        bool has_source_code_info =
            file_descriptor_proto.has_source_code_info() &&
            file_descriptor_proto.source_code_info().location_size() > 0;
        std::cerr << "Saw message type MockCodeGenerator_HasSourceCodeInfo: "
                  << has_source_code_info << "." << std::endl;
        abort();
      } else if (command == "HasJsonName") {
        FieldDescriptorProto field_descriptor_proto;
        file->message_type(i)->field(0)->CopyTo(&field_descriptor_proto);
        std::cerr << "Saw json_name: " << field_descriptor_proto.has_json_name()
                  << std::endl;
        abort();
      } else if (command == "Annotate") {
        annotate = true;
      } else if (command == "ShowVersionNumber") {
        Version compiler_version;
        context->GetCompilerVersion(&compiler_version);
        std::cerr << "Saw compiler_version: "
                  << compiler_version.major() * 1000000 +
                         compiler_version.minor() * 1000 +
                         compiler_version.patch()
                  << " " << compiler_version.suffix() << std::endl;
        abort();
      } else {
        GOOGLE_LOG(FATAL) << "Unknown MockCodeGenerator command: " << command;
      }
    }
  }

  bool insert_endlines = HasPrefixString(parameter, "insert_endlines=");
  if (insert_endlines || HasPrefixString(parameter, "insert=")) {
    std::vector<std::string> insert_into = Split(
        StripPrefixString(
            parameter, insert_endlines ? "insert_endlines=" : "insert="),
        ",", true);

    for (size_t i = 0; i < insert_into.size(); i++) {
      {
        google::protobuf::GeneratedCodeInfo info;
        std::string content =
            GetOutputFileContent(name_, "first_insert", file, context);
        if (insert_endlines) {
          GlobalReplaceSubstring(",", ",\n", &content);
        }
        if (annotate) {
          auto* annotation = info.add_annotation();
          annotation->set_begin(0);
          annotation->set_end(content.size());
          annotation->set_source_file("first_path");
        }
        std::unique_ptr<io::ZeroCopyOutputStream> output(
            context->OpenForInsertWithGeneratedCodeInfo(
                GetOutputFileName(insert_into[i], file),
                kFirstInsertionPointName, info));
        io::Printer printer(output.get(), '$');
        printer.PrintRaw(content);
        if (printer.failed()) {
          *error = "MockCodeGenerator detected write error.";
          return false;
        }
      }

      {
        google::protobuf::GeneratedCodeInfo info;
        std::string content =
            GetOutputFileContent(name_, "second_insert", file, context);
        if (insert_endlines) {
          GlobalReplaceSubstring(",", ",\n", &content);
        }
        if (annotate) {
          auto* annotation = info.add_annotation();
          annotation->set_begin(0);
          annotation->set_end(content.size());
          annotation->set_source_file("second_path");
        }
        std::unique_ptr<io::ZeroCopyOutputStream> output(
            context->OpenForInsertWithGeneratedCodeInfo(
                GetOutputFileName(insert_into[i], file),
                kSecondInsertionPointName, info));
        io::Printer printer(output.get(), '$');
        printer.PrintRaw(content);
        if (printer.failed()) {
          *error = "MockCodeGenerator detected write error.";
          return false;
        }
      }
    }
  } else {
    std::unique_ptr<io::ZeroCopyOutputStream> output(
        context->Open(GetOutputFileName(name_, file)));

    GeneratedCodeInfo annotations;
    io::AnnotationProtoCollector<GeneratedCodeInfo> annotation_collector(
        &annotations);
    io::Printer printer(output.get(), '$',
                        annotate ? &annotation_collector : nullptr);
    printer.PrintRaw(GetOutputFileContent(name_, parameter, file, context));
    std::string annotate_suffix = "_annotation";
    if (annotate) {
      printer.Print("$p$\n", "p", "first");
      printer.Annotate("p", "first" + annotate_suffix);
    }
    printer.PrintRaw(kFirstInsertionPoint);
    if (annotate) {
      printer.Print("$p$\n", "p", "second");
      printer.Annotate("p", "second" + annotate_suffix);
    }
    printer.PrintRaw(kSecondInsertionPoint);
    if (annotate) {
      printer.Print("$p$\n", "p", "third");
      printer.Annotate("p", "third" + annotate_suffix);
    }

    if (printer.failed()) {
      *error = "MockCodeGenerator detected write error.";
      return false;
    }
    if (annotate) {
      std::unique_ptr<io::ZeroCopyOutputStream> meta_output(
          context->Open(GetOutputFileName(name_, file) + ".pb.meta"));
      if (!TextFormat::Print(annotations, meta_output.get())) {
        *error = "MockCodeGenerator couldn't write .pb.meta";
        return false;
      }
    }
  }

  return true;
}

std::string MockCodeGenerator::GetOutputFileName(
    const std::string& generator_name, const FileDescriptor* file) {
  return GetOutputFileName(generator_name, file->name());
}

std::string MockCodeGenerator::GetOutputFileName(
    const std::string& generator_name, const std::string& file) {
  return file + ".MockCodeGenerator." + generator_name;
}

std::string MockCodeGenerator::GetOutputFileContent(
    const std::string& generator_name, const std::string& parameter,
    const FileDescriptor* file, GeneratorContext* context) {
  std::vector<const FileDescriptor*> all_files;
  context->ListParsedFiles(&all_files);
  return GetOutputFileContent(
      generator_name, parameter, file->name(), CommaSeparatedList(all_files),
      file->message_type_count() > 0 ? file->message_type(0)->name()
                                     : "(none)");
}

std::string MockCodeGenerator::GetOutputFileContent(
    const std::string& generator_name, const std::string& parameter,
    const std::string& file, const std::string& parsed_file_list,
    const std::string& first_message_name) {
  return strings::Substitute("$0: $1, $2, $3, $4\n", generator_name, parameter,
                          file, first_message_name, parsed_file_list);
}

}  // namespace compiler
}  // namespace protobuf
}  // namespace google
