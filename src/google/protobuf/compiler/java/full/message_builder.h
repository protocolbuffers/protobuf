// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: dweis@google.com (Daniel Weis)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#ifndef GOOGLE_PROTOBUF_COMPILER_JAVA_IMMUTABLE_MESSAGE_BUILDER_H__
#define GOOGLE_PROTOBUF_COMPILER_JAVA_IMMUTABLE_MESSAGE_BUILDER_H__

#include <memory>
#include <string>

#include "absl/container/btree_map.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "google/protobuf/compiler/java/generator_common.h"
#include "google/protobuf/compiler/java/full/field_generator.h"
#include "google/protobuf/compiler/java/full/oneof_generator.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {
class Context;            // context.h
class ClassNameResolver;  // name_resolver.h
}  // namespace java
}  // namespace compiler
namespace io {
class Printer;  // printer.h
}
}  // namespace protobuf
}  // namespace google

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

class MessageBuilderGenerator {
 public:
  MessageBuilderGenerator(
      const Descriptor* descriptor, Context* context,
      const absl::btree_map<int, std::unique_ptr<OneofGenerator>>&
          oneof_generators);
  MessageBuilderGenerator(const MessageBuilderGenerator&) = delete;
  MessageBuilderGenerator& operator=(const MessageBuilderGenerator&) = delete;
  virtual ~MessageBuilderGenerator();

  virtual void Generate(io::Printer* printer);

 private:
  void GenerateCommonBuilderMethods(io::Printer* printer);
  void GenerateBuilderConstructors(io::Printer* printer);
  void GenerateBuilderClearMethod(io::Printer* printer);
  void GenerateBuilderGetDescriptorForTypeMethod(io::Printer* printer);
  void GenerateBuilderGetDefaultInstanceForTypeMethod(io::Printer* printer);
  void GenerateBuilderBuildMethod(io::Printer* printer);
  void GenerateBuilderExtensionMethods(io::Printer* printer);
  void GenerateBuilderMergeFromMethods(io::Printer* printer);
  void GenerateBuilderMergeFromSubfunction(
      io::Printer* printer, absl::Span<const std::string> merging_code_blocks,
      absl::string_view method_suffix);
  void GenerateBuildPartial(io::Printer* printer);
  void GenerateBuildPartialShard(io::Printer* printer, int shard);
  void GenerateDescriptorMethods(io::Printer* printer);
  void GenerateBuilderParsingMethods(io::Printer* printer);
  void GenerateBuilderFieldParsingCases(io::Printer* printer);
  void GenerateBuilderFieldParsingCase(io::Printer* printer,
                                       const FieldDescriptor* field);
  void GenerateBuilderPackedFieldParsingCase(io::Printer* printer,
                                             const FieldDescriptor* field);
  void GenerateIsInitialized(io::Printer* printer);

  const Descriptor* descriptor_;
  Context* context_;
  ClassNameResolver* name_resolver_;
  FieldGeneratorMap<ImmutableFieldGenerator> field_generators_;
  const absl::btree_map<int, std::unique_ptr<OneofGenerator>>&
      oneof_generators_;
};

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_JAVA_IMMUTABLE_MESSAGE_BUILDER_H__
