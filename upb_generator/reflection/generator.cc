// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/match.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/plugin.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
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
    return google::protobuf::Edition::EDITION_2024;
  }
};

class StandaloneGeneratorContext : public google::protobuf::compiler::GeneratorContext {
 public:
  explicit StandaloneGeneratorContext(const std::string& out_dir)
      : out_dir_(out_dir) {}
  google::protobuf::io::ZeroCopyOutputStream* Open(const std::string& filename) override {
    std::string full_path = out_dir_ + "/" + filename;
    file_streams_.emplace_back(
        std::make_unique<std::ofstream>(full_path, std::ios::binary));
    out_streams_.emplace_back(std::make_unique<google::protobuf::io::OstreamOutputStream>(
        file_streams_.back().get()));
    return out_streams_.back().get();
  }

 private:
  std::string out_dir_;
  std::vector<std::unique_ptr<std::ofstream>> file_streams_;
  std::vector<std::unique_ptr<google::protobuf::io::ZeroCopyOutputStream>> out_streams_;
};

int StandaloneMain(int argc, char** argv) {
  std::vector<std::string> descriptor_sets;
  std::string direct_descriptor_set;
  std::string upbdefs_out;
  std::string parameter;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (absl::StartsWith(arg, "--descriptor_set=")) {
      descriptor_sets.push_back(arg.substr(17));
    } else if (absl::StartsWith(arg, "--direct_descriptor_set=")) {
      direct_descriptor_set = arg.substr(24);
    } else if (absl::StartsWith(arg, "--upbdefs_out=")) {
      upbdefs_out = arg.substr(14);
    }
  }

  if (descriptor_sets.empty() || direct_descriptor_set.empty() ||
      upbdefs_out.empty()) {
    std::cerr << "Usage: protoc-gen-upbdefs --direct_descriptor_set=<path> "
                 "[--descriptor_set=<path> ...] --upbdefs_out=<dir>\n";
    return 1;
  }

  std::string out_dir = upbdefs_out;
  size_t colon_pos = upbdefs_out.find_last_of(':');
  if (colon_pos != std::string::npos) {
    parameter = upbdefs_out.substr(0, colon_pos);
    out_dir = upbdefs_out.substr(colon_pos + 1);
  }

  Options options;
  std::string error;
  if (!ParseOptions(parameter, &options, &error)) {
    std::cerr << "Error parsing options: " << error << "\n";
    return 1;
  }

  google::protobuf::DescriptorPool proto_pool;
  for (const std::string& descriptor_set : descriptor_sets) {
    google::protobuf::FileDescriptorSet file_set;
    std::ifstream ifs(descriptor_set, std::ios::binary);
    if (!file_set.ParseFromIstream(&ifs)) {
      std::cerr << "Failed to parse descriptor set: " << descriptor_set << "\n";
      return 1;
    }
    for (int i = 0; i < file_set.file_size(); ++i) {
      if (!proto_pool.FindFileByName(file_set.file(i).name())) {
        proto_pool.BuildFile(file_set.file(i));
      }
    }
  }

  google::protobuf::FileDescriptorSet direct_file_set;
  {
    std::ifstream ifs(direct_descriptor_set, std::ios::binary);
    if (!direct_file_set.ParseFromIstream(&ifs)) {
      std::cerr << "Failed to parse direct descriptor set: "
                << direct_descriptor_set << "\n";
      return 1;
    }
  }

  upb::Arena arena;
  DefPoolPair pools;
  absl::flat_hash_set<std::string> files_seen;

  for (int i = 0; i < direct_file_set.file_size(); ++i) {
    std::string primary_file = direct_file_set.file(i).name();
    const google::protobuf::FileDescriptor* file_desc =
        proto_pool.FindFileByName(primary_file);
    if (!file_desc) {
      std::cerr << "File not found in descriptor pool: " << primary_file
                << "\n";
      return 1;
    }
    PopulateDefPool(file_desc, &arena, &pools, &files_seen);
  }

  StandaloneGeneratorContext context(out_dir);
  for (int i = 0; i < direct_file_set.file_size(); ++i) {
    std::string primary_file = direct_file_set.file(i).name();
    upb::FileDefPtr upb_file = pools.GetFile(primary_file);
    GenerateFile(upb_file, options, &context);
  }

  return 0;
}

}  // namespace reflection
}  // namespace generator
}  // namespace upb

int main(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (absl::StartsWith(arg, "--descriptor_set=")) {
      return upb::generator::reflection::StandaloneMain(argc, argv);
    }
  }
  upb::generator::reflection::ReflectionGenerator generator;
  return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
