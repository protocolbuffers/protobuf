// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/memory/memory.h"
#include "absl/strings/cord.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/code_generator_lite.h"
#include "google/protobuf/compiler/plugin.h"
#include "upb/base/status.hpp"
#include "upb/base/string_view.h"
#include "upb/mem/arena.hpp"
#include "upb/reflection/def.hpp"
#include "upb_generator/common.h"
#include "upb_generator/common/names.h"
#include "upb_generator/file_layout.h"
#include "upb_generator/minitable/generator.h"
#include "upb_generator/minitable/names_internal.h"
#include "upb_generator/plugin.h"

// Must be last.
#include "upb/port/def.inc"

namespace upb {
namespace generator {

std::string SourceFilename(upb::FileDefPtr file) {
  return StripExtension(file.name()) + ".upb_minitable.c";
}

void GenerateFile(const DefPoolPair& pools, upb::FileDefPtr file,
                  const MiniTableOptions& options,
                  google::protobuf::compiler::GeneratorContext* context) {
  Output h_output;
  WriteMiniTableHeader(pools, file, options, h_output);
  {
    auto stream = absl::WrapUnique(
        context->Open(MiniTableHeaderFilename(file.name(), false)));
    ABSL_CHECK(stream->WriteCord(absl::Cord(h_output.output())));
  }

  Output c_output;
  WriteMiniTableSource(pools, file, options, c_output);
  {
    auto stream = absl::WrapUnique(context->Open(SourceFilename(file)));
    ABSL_CHECK(stream->WriteCord(absl::Cord(c_output.output())));
  }

  if (options.one_output_per_message) {
    WriteMiniTableMultipleSources(pools, file, options, context);
  }
}

bool ParseOptions(MiniTableOptions* options, absl::string_view parameter,
                  std::string* error) {
  for (const auto& pair : ParseGeneratorParameter(parameter)) {
    if (pair.first == "bootstrap_stage") {
      options->bootstrap = true;
    } else if (pair.first == "experimental_strip_nonfunctional_codegen") {
      options->strip_nonfunctional_codegen = true;
    } else if (pair.first == "one_output_per_message") {
      options->one_output_per_message = true;
    } else {
      *error = absl::Substitute("Unknown parameter: $0", pair.first);
      return false;
    }
  }

  return true;
}

class MiniTableGenerator : public google::protobuf::compiler::CodeGenerator {
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
    MiniTableOptions options;
    if (!ParseOptions(&options, parameter, error)) {
      return false;
    }

    upb::Arena arena;
    DefPoolPair pools;
    absl::flat_hash_set<std::string> files_seen;
    for (const auto* file : files) {
      PopulateDefPool(file, &arena, &pools, &files_seen);
      upb::FileDefPtr upb_file = pools.GetFile(file->name());
      GenerateFile(pools, upb_file, options, generator_context);
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

}  // namespace generator
}  // namespace upb

int main(int argc, char** argv) {
  upb::generator::MiniTableGenerator generator;
  return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
