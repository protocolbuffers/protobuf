// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/code_generator.h"
#include "upb/reflection/def.hpp"
#include "upb_generator/common.h"
#include "upb_generator/file_layout.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace upb {
namespace generator {

struct MiniTableOptions {
  bool bootstrap = false;
  bool one_output_per_message = false;
  bool strip_nonfunctional_codegen = false;
};

void WriteMiniTableSource(const DefPoolPair& pools, upb::FileDefPtr file,
                          const MiniTableOptions& options, Output& output);
void WriteMiniTableMultipleSources(const DefPoolPair& pools,
                                   upb::FileDefPtr file,
                                   const MiniTableOptions& options,
                                   google::protobuf::compiler::GeneratorContext* context);
void WriteMiniTableHeader(const DefPoolPair& pools, upb::FileDefPtr file,
                          const MiniTableOptions& options, Output& output);

class PROTOC_EXPORT MiniTableGenerator
    : public google::protobuf::compiler::CodeGenerator {
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
                   std::string* error) const override;

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

#include "google/protobuf/port_undef.inc"
