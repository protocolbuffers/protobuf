// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: dweis@google.com (Daniel Weis)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#ifndef GOOGLE_PROTOBUF_COMPILER_JAVA_IMMUTABLE_ONEOF_GENERATOR_H__
#define GOOGLE_PROTOBUF_COMPILER_JAVA_IMMUTABLE_ONEOF_GENERATOR_H__

#include <map>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/java/full/field_generator.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {
class Context;
}  // namespace java
}  // namespace compiler
namespace io {
class Printer;
}
}  // namespace protobuf
}  // namespace google

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

class OneofGenerator {
 public:
  OneofGenerator(
      const OneofDescriptor* descriptor, Context* context,
      const FieldGeneratorMap<ImmutableFieldGenerator>& field_generators);
  OneofGenerator(const OneofGenerator&) = delete;
  OneofGenerator& operator=(const OneofGenerator&) = delete;
  ~OneofGenerator();

  void GenerateInterfaceMembers(io::Printer* printer) const;
  void GenerateMembers(io::Printer* printer) const;
  void GenerateEqualsCode(
      io::Printer* printer,
      const FieldGeneratorMap<ImmutableFieldGenerator>& field_generators) const;
  void GenerateHashCode(
      io::Printer* printer,
      const FieldGeneratorMap<ImmutableFieldGenerator>& field_generators) const;

  void GenerateCommonBuilderMethods(
      io::Printer* printer,
      const FieldGeneratorMap<ImmutableFieldGenerator>& field_generators) const;
  void GenerateBuilderClearMethod(io::Printer* printer) const;

  void GenerateMergingCode(
      io::Printer* printer,
      const FieldGeneratorMap<ImmutableFieldGenerator>& field_generators) const;
  bool HasScalarFields(int shard) const;
  void GenerateBuildingCode(io::Printer* printer, int shard) const;

 private:
  void GenerateBuilderGetOneofCase(io::Printer* printer) const;
  void GenerateBuilderClearOneof(io::Printer* printer) const;
  void GenerateBuilderClearOneofHasBits(io::Printer* printer) const;

  const OneofDescriptor* descriptor_;
  absl::flat_hash_map<absl::string_view, std::string> variables_;
  // A map from bit_field index (e.g. 0 for hasBits0) to the mask
  // representing the bits to clear for this oneof's fields.
  std::map<int, uint32_t> clear_masks_;
  // A map from bit_field index to the mask of scalar fields for this oneof.
  std::map<int, uint32_t> scalar_masks_;
};

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_JAVA_IMMUTABLE_ONEOF_GENERATOR_H__
