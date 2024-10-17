// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/csharp/csharp_generator.h"

#include <sstream>

#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/compiler/csharp/csharp_helpers.h"
#include "google/protobuf/compiler/csharp/csharp_options.h"
#include "google/protobuf/compiler/csharp/csharp_reflection_class.h"
#include "google/protobuf/compiler/csharp/names.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/zero_copy_stream.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

Generator::Generator() {}
Generator::~Generator() {}

uint64_t Generator::GetSupportedFeatures() const {
  return CodeGenerator::Feature::FEATURE_PROTO3_OPTIONAL |
         CodeGenerator::Feature::FEATURE_SUPPORTS_EDITIONS;
}

void GenerateFile(const FileDescriptor* file, io::Printer* printer,
                  const Options* options) {
  ReflectionClassGenerator reflectionClassGenerator(file, options);
  reflectionClassGenerator.Generate(printer);
}

bool Generator::Generate(const FileDescriptor* file,
                         const std::string& parameter,
                         GeneratorContext* generator_context,
                         std::string* error) const {
  std::vector<std::pair<std::string, std::string> > options;
  ParseGeneratorParameter(parameter, &options);

  struct Options cli_options;

  for (size_t i = 0; i < options.size(); i++) {
    if (options[i].first == "file_extension") {
      cli_options.file_extension = options[i].second;
    } else if (options[i].first == "base_namespace") {
      cli_options.base_namespace = options[i].second;
      cli_options.base_namespace_specified = true;
    } else if (options[i].first == "internal_access") {
      cli_options.internal_access = true;
    } else if (options[i].first == "serializable") {
      cli_options.serializable = true;
    } else if (options[i].first == "experimental_strip_nonfunctional_codegen") {
      cli_options.strip_nonfunctional_codegen = true;
    } else {
      *error = absl::StrCat("Unknown generator option: ", options[i].first);
      return false;
    }
  }

  std::string filename_error = "";
  std::string filename = GetOutputFile(file,
      cli_options.file_extension,
      cli_options.base_namespace_specified,
      cli_options.base_namespace,
      &filename_error);

  if (filename.empty()) {
    *error = filename_error;
    return false;
  }
  std::unique_ptr<io::ZeroCopyOutputStream> output(
      generator_context->Open(filename));
  io::Printer printer(output.get(), '$');

  GenerateFile(file, &printer, &cli_options);

  return true;
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
