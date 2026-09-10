// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#ifndef GOOGLE_PROTOBUF_COMPILER_JAVA_IMMUTABLE_ENUM_FIELD_H__
#define GOOGLE_PROTOBUF_COMPILER_JAVA_IMMUTABLE_ENUM_FIELD_H__

#include <string>

#include "absl/container/flat_hash_map.h"
#include "google/protobuf/compiler/java/full/field_generator.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {
class Context;            // context.h
class ClassNameResolver;  // name_resolver.h
}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

class ImmutableEnumFieldGenerator : public ImmutableFieldGenerator {
 public:
  explicit ImmutableEnumFieldGenerator(const FieldDescriptor* descriptor,
                                       int bit_index, Context* context);
  ImmutableEnumFieldGenerator(const ImmutableEnumFieldGenerator&) = delete;
  ImmutableEnumFieldGenerator& operator=(const ImmutableEnumFieldGenerator&) =
      delete;
  ~ImmutableEnumFieldGenerator() override;

  // implements ImmutableFieldGenerator
  // ---------------------------------------

  void GenerateInterfaceMembers(io::Printer* printer) const override;
  void GenerateMembers(io::Printer* printer) const override;
  void GenerateBuilderMembers(io::Printer* printer) const override;
  void GenerateInitializationCode(io::Printer* printer) const override;
  void GenerateBuilderClearCode(io::Printer* printer) const override;
  void GenerateBuilderParsingCode(io::Printer* printer) const override;
  void GenerateMergingCode(io::Printer* printer) const override;
  void GenerateBuildingCode(io::Printer* printer) const override;
  void GenerateSerializationCode(io::Printer* printer) const override;
  void GenerateSerializedSizeCode(io::Printer* printer) const override;
  void GenerateFieldBuilderInitializationCode(
      io::Printer* printer) const override;
  void GenerateEqualsCode(io::Printer* printer) const override;
  void GenerateHashCode(io::Printer* printer) const override;

  std::string GetBoxedType() const override;

 private:
  void GenerateInterfaceHasMethod(io::Printer* printer) const;
  void GenerateInterfaceGetValueMethod(io::Printer* printer) const;
  void GenerateInterfaceGetMethod(io::Printer* printer) const;

  void GenerateHasMethod(io::Printer* printer) const;
  void GenerateGetValueMethod(io::Printer* printer) const;
  void GenerateGetMethod(io::Printer* printer) const;

  void GenerateBuilderHasMethod(io::Printer* printer) const;
  void GenerateBuilderGetValueMethod(io::Printer* printer) const;
  void GenerateBuilderSetValueMethod(io::Printer* printer) const;
  void GenerateBuilderGetMethod(io::Printer* printer) const;
  void GenerateBuilderSetMethod(io::Printer* printer) const;
  void GenerateBuilderClearMethod(io::Printer* printer) const;
};

class ImmutableEnumOneofFieldGenerator : public ImmutableEnumFieldGenerator {
 public:
  ImmutableEnumOneofFieldGenerator(const FieldDescriptor* descriptor,
                                   int bit_index, Context* context);
  ImmutableEnumOneofFieldGenerator(const ImmutableEnumOneofFieldGenerator&) =
      delete;
  ImmutableEnumOneofFieldGenerator& operator=(
      const ImmutableEnumOneofFieldGenerator&) = delete;
  ~ImmutableEnumOneofFieldGenerator() override;

  void GenerateMembers(io::Printer* printer) const override;
  void GenerateBuilderMembers(io::Printer* printer) const override;
  void GenerateBuilderClearCode(io::Printer* printer) const override;
  void GenerateMergingCode(io::Printer* printer) const override;
  void GenerateBuildingCode(io::Printer* printer) const override;
  void GenerateBuilderParsingCode(io::Printer* printer) const override;
  void GenerateSerializationCode(io::Printer* printer) const override;
  void GenerateSerializedSizeCode(io::Printer* printer) const override;
  void GenerateEqualsCode(io::Printer* printer) const override;
  void GenerateHashCode(io::Printer* printer) const override;

 private:
  void GenerateHasMethod(io::Printer* printer) const;
  void GenerateGetValueMethod(io::Printer* printer) const;
  void GenerateGetMethod(io::Printer* printer) const;

  void GenerateBuilderHasMethod(io::Printer* printer) const;
  void GenerateBuilderGetValueMethod(io::Printer* printer) const;
  void GenerateBuilderSetValueMethod(io::Printer* printer) const;
  void GenerateBuilderGetMethod(io::Printer* printer) const;
  void GenerateBuilderSetMethod(io::Printer* printer) const;
  void GenerateBuilderClearMethod(io::Printer* printer) const;
  void GenerateBuilderParserMethod(io::Printer* printer) const;
};

class RepeatedImmutableEnumFieldGenerator : public ImmutableEnumFieldGenerator {
 public:
  explicit RepeatedImmutableEnumFieldGenerator(
      const FieldDescriptor* descriptor, int bit_index, Context* context);
  RepeatedImmutableEnumFieldGenerator(
      const RepeatedImmutableEnumFieldGenerator&) = delete;
  RepeatedImmutableEnumFieldGenerator& operator=(
      const RepeatedImmutableEnumFieldGenerator&) = delete;
  ~RepeatedImmutableEnumFieldGenerator() override;

  // implements ImmutableFieldGenerator ---------------------------------------
  void GenerateInterfaceMembers(io::Printer* printer) const override;
  void GenerateMembers(io::Printer* printer) const override;
  void GenerateBuilderMembers(io::Printer* printer) const override;
  void GenerateInitializationCode(io::Printer* printer) const override;
  void GenerateBuilderClearCode(io::Printer* printer) const override;
  void GenerateMergingCode(io::Printer* printer) const override;
  void GenerateBuildingCode(io::Printer* printer) const override;
  void GenerateBuilderParsingCode(io::Printer* printer) const override;
  void GenerateBuilderParsingCodeFromPacked(
      io::Printer* printer) const override;
  void GenerateSerializationCode(io::Printer* printer) const override;
  void GenerateSerializedSizeCode(io::Printer* printer) const override;
  void GenerateFieldBuilderInitializationCode(
      io::Printer* printer) const override;
  void GenerateEqualsCode(io::Printer* printer) const override;
  void GenerateHashCode(io::Printer* printer) const override;

  std::string GetBoxedType() const override;

 private:
  void GenerateInterfaceGetListMethod(io::Printer* printer) const;
  void GenerateInterfaceGetCountMethod(io::Printer* printer) const;
  void GenerateInterfaceGetMethod(io::Printer* printer) const;
  void GenerateInterfaceGetValueListMethod(io::Printer* printer) const;
  void GenerateInterfaceGetValueMethod(io::Printer* printer) const;

  void GenerateGetListMethod(io::Printer* printer) const;
  void GenerateGetCountMethod(io::Printer* printer) const;
  void GenerateGetMethod(io::Printer* printer) const;
  void GenerateGetValueListMethod(io::Printer* printer) const;
  void GenerateGetValueMethod(io::Printer* printer) const;

  void GenerateEnsureIsMutableMethod(io::Printer* printer) const;
  void GenerateBuilderGetListMethod(io::Printer* printer) const;
  void GenerateBuilderGetCountMethod(io::Printer* printer) const;
  void GenerateBuilderGetMethod(io::Printer* printer) const;
  void GenerateBuilderSetMethod(io::Printer* printer) const;
  void GenerateBuilderAddMethod(io::Printer* printer) const;
  void GenerateBuilderAddAllMethod(io::Printer* printer) const;
  void GenerateBuilderClearMethod(io::Printer* printer) const;
  void GenerateBuilderGetValueListMethod(io::Printer* printer) const;
  void GenerateBuilderGetValueMethod(io::Printer* printer) const;
  void GenerateBuilderSetValueMethod(io::Printer* printer) const;
  void GenerateBuilderAddValueMethod(io::Printer* printer) const;
  void GenerateBuilderAddAllValueMethod(io::Printer* printer) const;
};

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_JAVA_IMMUTABLE_ENUM_FIELD_H__
