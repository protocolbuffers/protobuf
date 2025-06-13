// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstdint>
#include <string>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/plugin.h"
#include "google/protobuf/descriptor.h"
#include "upb/mem/arena.hpp"
#include "upb/reflection/def.hpp"
#include "upb_generator/file_layout.h"
#include "upb_generator/plugin.h"
#include "upb_generator/reflection/context.h"
#include "upb_generator/reflection/header.h"
#include "upb_generator/reflection/source.h"

namespace upb {
namespace generator {
namespace reflection {

void GenerateFile(upb::FileDefPtr file, const Options& options,
                  google::protobuf::compiler::GeneratorContext* context) {
  GenerateReflectionHeader(file, options, context);
  GenerateReflectionSource(file, options, context);
}

bool ParseOptions(absl::string_view parameter, Options* options,
                  std::string* error) {
  for (const auto& pair : ParseGeneratorParameter(parameter)) {
    if (pair.first == "dllexport_decl") {
      options->dllexport_decl = pair.second;
    } else {
      *error = absl::Substitute("Unknown parameter: $0", pair.first);
      return false;
    }
  }

  return true;
}

class ReflectionGenerator : public google::protobuf::compiler::CodeGenerator {
  bool Generate(const google::protobuf::FileDescriptor* file,
                const std::string& parameter,
                google::protobuf::compiler::GeneratorContext* generator_context,
                std::string* error) const override {
    std::vector<const google::protobuf::FileDescriptor*> files{file};
    return GenerateAll(files, parameter, generator_context, error);
  }

  bool GenerateAll(const std::vector<const google::protobuf::FileDescriptor*>& files,
                   const std::string& parameter,
                   google::protobuf::compiler::GeneratorContext* generator_context,
                   std::string* error) const override {
    Options options;
    if (!ParseOptions(parameter, &options, error)) {
      return false;
    }

    upb::Arena arena;
    DefPoolPair pools;
    absl::flat_hash_set<std::string> files_seen;
    for (const auto* file : files) {
      PopulateDefPool(file, &arena, &pools, &files_seen);
      upb::FileDefPtr upb_file = pools.GetFile(file->name());
      GenerateFile(upb_file, options, generator_context);
    }

    return true;
  }

  uint64_t GetSupportedFeatures() const override {
    return FEATURE_PROTO3_OPTIONAL | FEATURE_SUPPORTS_EDITIONS;
  }
  google::protobuf::Edition GetMinimumEdition() const override {
    return google::protobuf::Edition::EDITION_PROTO2;
  }
  google::protobuf::Edition GetMaximumEdition() const override {
    return google::protobuf::Edition::EDITION_2023;
  }
};

}  // namespace reflection
}  // namespace generator
}  // namespace upb

int main(int argc, char** argv) {
  upb::generator::reflection::ReflectionGenerator generator;
  return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
