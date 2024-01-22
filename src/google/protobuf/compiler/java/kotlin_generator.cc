// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/java/kotlin_generator.h"

#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/java/file.h"
#include "google/protobuf/compiler/java/generator.h"
#include "google/protobuf/compiler/java/helpers.h"
#include "google/protobuf/compiler/java/options.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

KotlinGenerator::KotlinGenerator() {}
KotlinGenerator::~KotlinGenerator() {}

uint64_t KotlinGenerator::GetSupportedFeatures() const {
  return CodeGenerator::Feature::FEATURE_PROTO3_OPTIONAL |
         CodeGenerator::Feature::FEATURE_SUPPORTS_EDITIONS;
}

bool KotlinGenerator::Generate(const FileDescriptor* file,
                               const std::string& parameter,
                               GeneratorContext* context,
                               std::string* error) const {
  // -----------------------------------------------------------------
  // parse generator options

  std::vector<std::pair<std::string, std::string> > options;
  ParseGeneratorParameter(parameter, &options);
  Options file_options;

  for (auto& option : options) {
    if (option.first == "output_list_file") {
      file_options.output_list_file = option.second;
    } else if (option.first == "immutable") {
      // Note: the option is considered always set regardless of the input.
      file_options.generate_immutable_code = true;
    } else if (option.first == "mutable") {
      *error = "Mutable not supported by Kotlin generator";
      return false;
    } else if (option.first == "shared") {
      // Note: the option is considered always set regardless of the input.
      file_options.generate_shared_code = true;
    } else if (option.first == "lite") {
      file_options.enforce_lite = true;
    } else if (option.first == "annotate_code") {
      file_options.annotate_code = true;
    } else if (option.first == "annotation_list_file") {
      file_options.annotation_list_file = option.second;
    } else if (option.first == "experimental_strip_nonfunctional_codegen") {
      file_options.strip_nonfunctional_codegen = true;
    } else {
      *error = absl::StrCat("Unknown generator option: ", option.first);
      return false;
    }
  }

  // We only support generation of immutable code so we do it.
  file_options.generate_immutable_code = true;
  file_options.generate_shared_code = true;

  std::vector<std::string> all_files;
  std::vector<std::string> all_annotations;

  std::unique_ptr<FileGenerator> file_generator(
        new FileGenerator(file, file_options, /* immutable_api = */ true));

  if (!file_generator || !file_generator->Validate(error)) {
    return false;
  }

  auto open_file = [context](const std::string& filename) {
    return std::unique_ptr<io::ZeroCopyOutputStream>(context->Open(filename));
  };
  std::string package_dir = JavaPackageToDir(file_generator->java_package());
  std::string kotlin_filename =
      absl::StrCat(package_dir, file_generator->GetKotlinClassname(), ".kt");
  all_files.push_back(kotlin_filename);
  std::string info_full_path = absl::StrCat(kotlin_filename, ".pb.meta");
  if (file_options.annotate_code) {
    all_annotations.push_back(info_full_path);
  }

  // Generate main kotlin file.
  auto output = open_file(kotlin_filename);
  GeneratedCodeInfo annotations;
  io::AnnotationProtoCollector<GeneratedCodeInfo> annotation_collector(
      &annotations);
  io::Printer printer(
      output.get(), '$',
      file_options.annotate_code ? &annotation_collector : nullptr);

  file_generator->GenerateKotlin(&printer);

  file_generator->GenerateKotlinSiblings(package_dir, context, &all_files,
                                         &all_annotations);

  if (file_options.annotate_code) {
    auto info_output = open_file(info_full_path);
    annotations.SerializeToZeroCopyStream(info_output.get());
  }

  // Generate output list if requested.
  if (!file_options.output_list_file.empty()) {
    // Generate output list.  This is just a simple text file placed in a
    // deterministic location which lists the .kt files being generated.
    auto srclist_raw_output = open_file(file_options.output_list_file);
    io::Printer srclist_printer(srclist_raw_output.get(), '$');
    for (auto& all_file : all_files) {
      srclist_printer.Print("$filename$\n", "filename", all_file);
    }
  }

  if (!file_options.annotation_list_file.empty()) {
    // Generate output list.  This is just a simple text file placed in a
    // deterministic location which lists the .kt files being generated.
    auto annotation_list_raw_output =
        open_file(file_options.annotation_list_file);
    io::Printer annotation_list_printer(annotation_list_raw_output.get(), '$');
    for (auto& all_annotation : all_annotations) {
      annotation_list_printer.Print("$filename$\n", "filename", all_annotation);
    }
  }

  return true;
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
