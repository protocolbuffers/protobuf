// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)

#include "google/protobuf/compiler/mock_code_generator.h"

#include <stdlib.h>

#include <cstdint>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/file.h"
#include "google/protobuf/descriptor.pb.h"
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/plugin.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor_legacy.h"
#include "google/protobuf/descriptor_visitor.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/unittest_features.pb.h"

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
  std::vector<absl::string_view> names;
  for (size_t i = 0; i < all_files.size(); i++) {
    names.push_back(all_files[i]->name());
  }
  return absl::StrJoin(names, ",");
}

static constexpr absl::string_view kFirstInsertionPointName =
    "first_mock_insertion_point";
static constexpr absl::string_view kSecondInsertionPointName =
    "second_mock_insertion_point";
static constexpr absl::string_view kFirstInsertionPoint =
    "# @@protoc_insertion_point(first_mock_insertion_point) is here\n";
static constexpr absl::string_view kSecondInsertionPoint =
    "  # @@protoc_insertion_point(second_mock_insertion_point) is here\n";

MockCodeGenerator::MockCodeGenerator(absl::string_view name) : name_(name) {
  absl::string_view key = getenv("TEST_CASE");
  if (key == "no_editions") {
    suppressed_features_ |= CodeGenerator::FEATURE_SUPPORTS_EDITIONS;
  } else if (key == "invalid_features") {
    feature_extensions_ = {nullptr};
  } else if (key == "no_feature_defaults") {
    feature_extensions_ = {};
  }
}

MockCodeGenerator::~MockCodeGenerator() = default;

uint64_t MockCodeGenerator::GetSupportedFeatures() const {
  uint64_t all_features = CodeGenerator::FEATURE_PROTO3_OPTIONAL |
                          CodeGenerator::FEATURE_SUPPORTS_EDITIONS;
  return all_features & ~suppressed_features_;
}

void MockCodeGenerator::SuppressFeatures(uint64_t features) {
  suppressed_features_ = features;
}

void MockCodeGenerator::ExpectGenerated(
    absl::string_view name, absl::string_view parameter,
    absl::string_view insertions, absl::string_view file,
    absl::string_view first_message_name,
    absl::string_view first_parsed_file_name,
    absl::string_view output_directory) {
  std::string content;
  ABSL_CHECK_OK(File::GetContents(
      absl::StrCat(output_directory, "/", GetOutputFileName(name, file)),
      &content, true));

  std::vector<std::string> lines =
      absl::StrSplit(content, '\n', absl::SkipEmpty());

  while (!lines.empty() && lines.back().empty()) {
    lines.pop_back();
  }
  for (size_t i = 0; i < lines.size(); i++) {
    absl::StrAppend(&lines[i], "\n");
  }

  std::vector<std::string> insertion_list;
  if (!insertions.empty()) {
    insertion_list = absl::StrSplit(insertions, ',', absl::SkipEmpty());
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
    EXPECT_EQ(absl::StrCat(
                  "  ", GetOutputFileContent(insertion_list[i], "second_insert",
                                             file, file, first_message_name)),
              lines[2 + insertion_list.size() + i]);
  }
}

namespace {
void CheckSingleAnnotation(absl::string_view expected_file,
                           absl::string_view expected_text,
                           absl::string_view file_content,
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
    absl::string_view name, absl::string_view file,
    absl::string_view output_directory) {
  std::string file_content;
  ABSL_CHECK_OK(File::GetContents(
      absl::StrCat(output_directory, "/", GetOutputFileName(name, file)),
      &file_content, true));
  std::string meta_content;
  ABSL_CHECK_OK(
      File::GetContents(absl::StrCat(output_directory, "/",
                                     GetOutputFileName(name, file), ".pb.meta"),
                        &meta_content, true));
  GeneratedCodeInfo annotations;
  ABSL_CHECK(TextFormat::ParseFromString(meta_content, &annotations));
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
  if (FileDescriptorLegacy(file).syntax() ==
          FileDescriptorLegacy::SYNTAX_EDITIONS &&
      (suppressed_features_ & CodeGenerator::FEATURE_SUPPORTS_EDITIONS) == 0) {
    internal::VisitDescriptors(*file, [&](const auto& descriptor) {
      const FeatureSet& features = GetResolvedSourceFeatures(descriptor);
      ABSL_CHECK(features.HasExtension(pb::test))
          << "Test features were not resolved properly";
      ABSL_CHECK(features.GetExtension(pb::test).has_int_file_feature())
          << "Test features were not resolved properly";
      ABSL_CHECK(features.GetExtension(pb::test).has_int_source_feature())
          << "Test features were not resolved properly";
    });
  }

  bool annotate = false;
  for (int i = 0; i < file->message_type_count(); i++) {
    if (absl::StartsWith(file->message_type(i)->name(), "MockCodeGenerator_")) {
      absl::string_view command = absl::StripPrefix(
          file->message_type(i)->name(), "MockCodeGenerator_");
      if (command == "Error") {
        *error = "Saw message type MockCodeGenerator_Error.";
        return false;
      }
      if (command == "Exit") {
        std::cerr << "Saw message type MockCodeGenerator_Exit." << std::endl;
        exit(123);
      }
      ABSL_CHECK(command != "Abort")
          << "Saw message type MockCodeGenerator_Abort.";
      if (command == "HasSourceCodeInfo") {
        FileDescriptorProto file_descriptor_proto;
        file->CopySourceCodeInfoTo(&file_descriptor_proto);
        bool has_source_code_info =
            file_descriptor_proto.has_source_code_info() &&
            file_descriptor_proto.source_code_info().location_size() > 0;
        ABSL_LOG(FATAL)
            << "Saw message type MockCodeGenerator_HasSourceCodeInfo: "
            << has_source_code_info << ".";
      } else if (command == "HasJsonName") {
        FieldDescriptorProto field_descriptor_proto;
        file->message_type(i)->field(0)->CopyTo(&field_descriptor_proto);
        ABSL_LOG(FATAL) << "Saw json_name: "
                        << field_descriptor_proto.has_json_name();
      } else if (command == "Annotate") {
        annotate = true;
      } else if (command == "ShowVersionNumber") {
        Version compiler_version;
        context->GetCompilerVersion(&compiler_version);
        ABSL_LOG(FATAL) << "Saw compiler_version: "
                        << compiler_version.major() * 1000000 +
                               compiler_version.minor() * 1000 +
                               compiler_version.patch()
                        << " " << compiler_version.suffix();
      } else {
        ABSL_LOG(FATAL) << "Unknown MockCodeGenerator command: " << command;
      }
    }
  }

  bool insert_endlines = absl::StartsWith(parameter, "insert_endlines=");
  if (insert_endlines || absl::StartsWith(parameter, "insert=")) {
    std::vector<std::string> insert_into = absl::StrSplit(
        absl::StripPrefix(parameter,
                          insert_endlines ? "insert_endlines=" : "insert="),
        ',', absl::SkipEmpty());

    for (size_t i = 0; i < insert_into.size(); i++) {
      {
        google::protobuf::GeneratedCodeInfo info;
        std::string content =
            GetOutputFileContent(name_, "first_insert", file, context);
        if (insert_endlines) {
          absl::StrReplaceAll({{",", ",\n"}}, &content);
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
                std::string(kFirstInsertionPointName), info));
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
          absl::StrReplaceAll({{",", ",\n"}}, &content);
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
                std::string(kSecondInsertionPointName), info));
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
      printer.Annotate("p", absl::StrCat("first", annotate_suffix));
    }
    printer.PrintRaw(kFirstInsertionPoint);
    if (annotate) {
      printer.Print("$p$\n", "p", "second");
      printer.Annotate("p", absl::StrCat("second", annotate_suffix));
    }
    printer.PrintRaw(kSecondInsertionPoint);
    if (annotate) {
      printer.Print("$p$\n", "p", "third");
      printer.Annotate("p", absl::StrCat("third", annotate_suffix));
    }

    if (printer.failed()) {
      *error = "MockCodeGenerator detected write error.";
      return false;
    }
    if (annotate) {
      std::unique_ptr<io::ZeroCopyOutputStream> meta_output(context->Open(
          absl::StrCat(GetOutputFileName(name_, file), ".pb.meta")));
      if (!TextFormat::Print(annotations, meta_output.get())) {
        *error = "MockCodeGenerator couldn't write .pb.meta";
        return false;
      }
    }
  }

  return true;
}

std::string MockCodeGenerator::GetOutputFileName(
    absl::string_view generator_name, const FileDescriptor* file) {
  return GetOutputFileName(generator_name, file->name());
}

std::string MockCodeGenerator::GetOutputFileName(
    absl::string_view generator_name, absl::string_view file) {
  return absl::StrCat(file, ".MockCodeGenerator.", generator_name);
}

std::string MockCodeGenerator::GetOutputFileContent(
    absl::string_view generator_name, absl::string_view parameter,
    const FileDescriptor* file, GeneratorContext* context) {
  std::vector<const FileDescriptor*> all_files;
  context->ListParsedFiles(&all_files);
  return GetOutputFileContent(
      generator_name, parameter, file->name(), CommaSeparatedList(all_files),
      file->message_type_count() > 0 ? file->message_type(0)->name()
                                     : "(none)");
}

std::string MockCodeGenerator::GetOutputFileContent(
    absl::string_view generator_name, absl::string_view parameter,
    absl::string_view file, absl::string_view parsed_file_list,
    absl::string_view first_message_name) {
  return absl::Substitute("$0: $1, $2, $3, $4\n", generator_name, parameter,
                          file, first_message_name, parsed_file_list);
}

}  // namespace compiler
}  // namespace protobuf
}  // namespace google
