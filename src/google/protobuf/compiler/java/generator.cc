// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/java/generator.h"

#include <utility>
#include <vector>


#include <memory>

#include "absl/strings/str_format.h"
#include "google/protobuf/compiler/java/file.h"
#include "google/protobuf/compiler/java/helpers.h"
#include "google/protobuf/compiler/java/name_resolver.h"
#include "google/protobuf/compiler/java/options.h"
#include "google/protobuf/compiler/java/shared_code_generator.h"
#include "google/protobuf/descriptor.pb.h"


namespace google {
namespace protobuf {
namespace compiler {
namespace java {


JavaGenerator::JavaGenerator() {}
JavaGenerator::~JavaGenerator() {}

uint64_t JavaGenerator::GetSupportedFeatures() const {
  return CodeGenerator::Feature::FEATURE_PROTO3_OPTIONAL;
}

bool JavaGenerator::Generate(const FileDescriptor* file,
                             const std::string& parameter,
                             GeneratorContext* context,
                             std::string* error) const {
  // -----------------------------------------------------------------
  // parse generator options

  std::vector<std::pair<std::string, std::string> > options;
  ParseGeneratorParameter(parameter, &options);
  Options file_options;

  file_options.opensource_runtime = opensource_runtime_;

  for (auto& option : options) {
    if (option.first == "output_list_file") {
      file_options.output_list_file = option.second;
    } else if (option.first == "immutable") {
      file_options.generate_immutable_code = true;
    } else if (option.first == "mutable") {
      file_options.generate_mutable_code = true;
    } else if (option.first == "shared") {
      file_options.generate_shared_code = true;
    } else if (option.first == "lite") {
      // Note: Java Lite does not guarantee API/ABI stability. We may choose to
      // break existing API in order to boost performance / reduce code size.
      file_options.enforce_lite = true;
    } else if (option.first == "annotate_code") {
      file_options.annotate_code = true;
    } else if (option.first == "annotation_list_file") {
      file_options.annotation_list_file = option.second;
    } else {
      *error = absl::StrCat("Unknown generator option: ", option.first);
      return false;
    }
  }

  if (file_options.enforce_lite && file_options.generate_mutable_code) {
    *error = "lite runtime generator option cannot be used with mutable API.";
    return false;
  }

  // By default we generate immutable code and shared code for immutable API.
  if (!file_options.generate_immutable_code &&
      !file_options.generate_mutable_code &&
      !file_options.generate_shared_code) {
    file_options.generate_immutable_code = true;
    file_options.generate_shared_code = true;
  }

  // -----------------------------------------------------------------


  std::vector<std::string> all_files;
  std::vector<std::string> all_annotations;


  std::vector<std::unique_ptr<FileGenerator>> file_generators;
  if (file_options.generate_immutable_code) {
    file_generators.emplace_back(
        std::make_unique<FileGenerator>(file, file_options,
                                        /* immutable = */ true));
  }
  if (file_options.generate_mutable_code) {
    file_generators.emplace_back(
        std::make_unique<FileGenerator>(file, file_options,
                                        /* mutable = */ false));
  }

  for (auto& file_generator : file_generators) {
    if (!file_generator->Validate(error)) {
      return false;
    }
  }

  for (auto& file_generator : file_generators) {
    std::string package_dir = JavaPackageToDir(file_generator->java_package());

    std::string java_filename =
        absl::StrCat(package_dir, file_generator->classname(), ".java");
    all_files.push_back(java_filename);
    std::string info_full_path = absl::StrCat(java_filename, ".pb.meta");
    if (file_options.annotate_code) {
      all_annotations.push_back(info_full_path);
    }

    // Generate main java file.
    std::unique_ptr<io::ZeroCopyOutputStream> output(
        context->Open(java_filename));
    GeneratedCodeInfo annotations;
    io::AnnotationProtoCollector<GeneratedCodeInfo> annotation_collector(
        &annotations);
    io::Printer printer(
        output.get(), '$',
        file_options.annotate_code ? &annotation_collector : NULL);

    file_generator->Generate(&printer);

    // Generate sibling files.
    file_generator->GenerateSiblings(package_dir, context, &all_files,
                                     &all_annotations);

    if (file_options.annotate_code) {
      std::unique_ptr<io::ZeroCopyOutputStream> info_output(
          context->Open(info_full_path));
      annotations.SerializeToZeroCopyStream(info_output.get());
    }
  }


  file_generators.clear();

  // Generate output list if requested.
  if (!file_options.output_list_file.empty()) {
    // Generate output list.  This is just a simple text file placed in a
    // deterministic location which lists the .java files being generated.
    std::unique_ptr<io::ZeroCopyOutputStream> srclist_raw_output(
        context->Open(file_options.output_list_file));
    io::Printer srclist_printer(srclist_raw_output.get(), '$');
    for (int i = 0; i < all_files.size(); i++) {
      srclist_printer.Print("$filename$\n", "filename", all_files[i]);
    }
  }

  if (!file_options.annotation_list_file.empty()) {
    // Generate output list.  This is just a simple text file placed in a
    // deterministic location which lists the .java files being generated.
    std::unique_ptr<io::ZeroCopyOutputStream> annotation_list_raw_output(
        context->Open(file_options.annotation_list_file));
    io::Printer annotation_list_printer(annotation_list_raw_output.get(), '$');
    for (int i = 0; i < all_annotations.size(); i++) {
      annotation_list_printer.Print("$filename$\n", "filename",
                                    all_annotations[i]);
    }
  }

  return true;
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
