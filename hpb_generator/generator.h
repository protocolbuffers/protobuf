// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_HPB_GENERATOR_H__
#define GOOGLE_PROTOBUF_COMPILER_HPB_GENERATOR_H__

#include <cstdint>
#include <string>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/plugin.h"
#include "google/protobuf/descriptor.h"

namespace google::protobuf::hpb_generator {

namespace protoc = ::google::protobuf::compiler;
namespace protobuf = ::proto2;
using FileDescriptor = ::google::protobuf::FileDescriptor;
using google::protobuf::Edition;

class Generator : public protoc::CodeGenerator {
 public:
  ~Generator() override = default;
  bool Generate(const protobuf::FileDescriptor* file,
                const std::string& parameter, protoc::GeneratorContext* context,
                std::string* error) const override;
  uint64_t GetSupportedFeatures() const override {
    return Feature::FEATURE_PROTO3_OPTIONAL |
           Feature::FEATURE_SUPPORTS_EDITIONS;
  }
  Edition GetMinimumEdition() const override { return Edition::EDITION_PROTO2; }
  Edition GetMaximumEdition() const override { return Edition::EDITION_2023; }
};

}  // namespace protobuf
}  // namespace google::hpb_generator

#endif  // GOOGLE_PROTOBUF_COMPILER_HPB_GENERATOR_H__
