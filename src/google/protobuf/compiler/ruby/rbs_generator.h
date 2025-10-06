// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: jieluo@google.com (Jie Luo)
//
// Generates Ruby type definition (.rbs) for a given .proto file.

#ifndef GOOGLE_PROTOBUF_COMPILER_RUBY_RBS_GENERATOR_H__
#define GOOGLE_PROTOBUF_COMPILER_RUBY_RBS_GENERATOR_H__

#include <string>

#include "google/protobuf/compiler/code_generator.h"

#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace ruby {

class PROTOC_EXPORT RbsGenerator : public CodeGenerator {
 public:
  RbsGenerator();
  RbsGenerator(const RbsGenerator&) = delete;
  RbsGenerator& operator=(const RbsGenerator&) = delete;
  ~RbsGenerator() override;

  // CodeGenerator methods.
  uint64_t GetSupportedFeatures() const override {
    return FEATURE_PROTO3_OPTIONAL;
  }
  bool Generate(const FileDescriptor* file, const std::string& parameter,
                GeneratorContext* generator_context,
                std::string* error) const override;

};

}  // namespace ruby
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_COMPILER_RUBY_RBS_GENERATOR_H__
