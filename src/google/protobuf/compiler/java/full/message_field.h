// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#ifndef GOOGLE_PROTOBUF_COMPILER_JAVA_IMMUTABLE_MESSAGE_FIELD_H__
#define GOOGLE_PROTOBUF_COMPILER_JAVA_IMMUTABLE_MESSAGE_FIELD_H__

#include <string>

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

class ImmutableMessageFieldGenerator : public ImmutableFieldGenerator {
 public:
  explicit ImmutableMessageFieldGenerator(const FieldDescriptor* descriptor,
                                          int bit_index, Context* context);
  ImmutableMessageFieldGenerator(const ImmutableMessageFieldGenerator&) =
      delete;
  ImmutableMessageFieldGenerator& operator=(
      const ImmutableMessageFieldGenerator&) = delete;
  ~ImmutableMessageFieldGenerator() override;

  // implements ImmutableFieldGenerator
  // ---------------------------------------

  void GenerateInterfaceMembers(io::Printer* printer) const override;
  void GenerateMembers(io::Printer* printer) const override;
  void GenerateBuilderMembers(io::Printer* printer) const override;
  void GenerateInitializationCode(io::Printer* printer) const override;
  void GenerateBuilderClearCode(io::Printer* printer) const override;
  void GenerateMergingCode(io::Printer* printer) const override;
  void GenerateBuildingCode(io::Printer* printer) const override;
  void GenerateBuilderParsingCode(io::Printer* printer) const override;
  void GenerateSerializationCode(io::Printer* printer) const override;
  void GenerateSerializedSizeCode(io::Printer* printer) const override;
  void GenerateFieldBuilderInitializationCode(
      io::Printer* printer) const override;
  void GenerateEqualsCode(io::Printer* printer) const override;
  void GenerateHashCode(io::Printer* printer) const override;

  std::string GetBoxedType() const override;

 protected:
  virtual void PrintNestedBuilderCondition(
      io::Printer* printer, const char* regular_case,
      const char* nested_builder_case) const;
  virtual void PrintNestedBuilderFunction(
      io::Printer* printer, const char* method_prototype,
      const char* regular_case, const char* nested_builder_case,
      const char* trailing_code,
      absl::optional<io::AnnotationCollector::Semantic> semantic =
          absl::nullopt) const;

 private:
  void GenerateInterfaceHasMethod(io::Printer* printer) const;
  void GenerateInterfaceGetMethod(io::Printer* printer) const;
  void GenerateInterfaceGetOrBuilderMethod(io::Printer* printer) const;

  void GenerateHasMethod(io::Printer* printer) const;
  void GenerateGetMethod(io::Printer* printer) const;
  void GenerateGetOrBuilderMethod(io::Printer* printer) const;
  void GenerateWriteFieldMethod(io::Printer* printer) const;

  void GenerateBuilderHasMethod(io::Printer* printer) const;
  void GenerateBuilderGetMethod(io::Printer* printer) const;
  void GenerateBuilderSetMethod(io::Printer* printer) const;
  void GenerateBuilderSetBuilderMethod(io::Printer* printer) const;
  void GenerateBuilderMergeMethod(io::Printer* printer) const;
  void GenerateBuilderClearMethod(io::Printer* printer) const;
  void GenerateBuilderGetBuilderMethod(io::Printer* printer) const;
  void GenerateBuilderGetOrBuilderMethod(io::Printer* printer) const;
  void GenerateBuilderInternalGetFieldBuilderMethod(io::Printer* printer) const;
  void GenerateBuilderParseMethod(io::Printer* printer) const;
};

class ImmutableMessageOneofFieldGenerator
    : public ImmutableMessageFieldGenerator {
 public:
  ImmutableMessageOneofFieldGenerator(const FieldDescriptor* descriptor,
                                      int bit_index, Context* context);
  ImmutableMessageOneofFieldGenerator(
      const ImmutableMessageOneofFieldGenerator&) = delete;
  ImmutableMessageOneofFieldGenerator& operator=(
      const ImmutableMessageOneofFieldGenerator&) = delete;
  ~ImmutableMessageOneofFieldGenerator() override;

  void GenerateMembers(io::Printer* printer) const override;
  void GenerateBuilderMembers(io::Printer* printer) const override;
  void GenerateBuilderClearCode(io::Printer* printer) const override;
  void GenerateBuildingCode(io::Printer* printer) const override;
  void GenerateMergingCode(io::Printer* printer) const override;
  void GenerateBuilderParsingCode(io::Printer* printer) const override;
  void GenerateSerializationCode(io::Printer* printer) const override;
  void GenerateSerializedSizeCode(io::Printer* printer) const override;

 private:
  void GenerateHasMethod(io::Printer* printer) const;
  void GenerateGetMethod(io::Printer* printer) const;
  void GenerateGetOrBuilderMethod(io::Printer* printer) const;
  void GenerateWriteFieldMethod(io::Printer* printer) const;

  void GenerateBuilderHasMethod(io::Printer* printer) const;
  void GenerateBuilderGetMethod(io::Printer* printer) const;
  void GenerateBuilderSetMethod(io::Printer* printer) const;
  void GenerateBuilderSetBuilderMethod(io::Printer* printer) const;
  void GenerateBuilderMergeMethod(io::Printer* printer) const;
  void GenerateBuilderClearMethod(io::Printer* printer) const;
  void GenerateBuilderGetBuilderMethod(io::Printer* printer) const;
  void GenerateBuilderGetOrBuilderMethod(io::Printer* printer) const;
  void GenerateBuilderInternalGetFieldBuilderMethod(io::Printer* printer) const;
};

class RepeatedImmutableMessageFieldGenerator
    : public ImmutableMessageFieldGenerator {
 public:
  explicit RepeatedImmutableMessageFieldGenerator(
      const FieldDescriptor* descriptor, int bit_index, Context* context);
  RepeatedImmutableMessageFieldGenerator(
      const RepeatedImmutableMessageFieldGenerator&) = delete;
  RepeatedImmutableMessageFieldGenerator& operator=(
      const RepeatedImmutableMessageFieldGenerator&) = delete;
  ~RepeatedImmutableMessageFieldGenerator() override;

  // implements ImmutableFieldGenerator ---------------------------------------
  void GenerateInterfaceMembers(io::Printer* printer) const override;
  void GenerateMembers(io::Printer* printer) const override;
  void GenerateBuilderMembers(io::Printer* printer) const override;
  void GenerateInitializationCode(io::Printer* printer) const override;
  void GenerateBuilderClearCode(io::Printer* printer) const override;
  void GenerateMergingCode(io::Printer* printer) const override;
  void GenerateBuildingCode(io::Printer* printer) const override;
  void GenerateBuilderParsingCode(io::Printer* printer) const override;
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
  void GenerateInterfaceGetOrBuilderListMethod(io::Printer* printer) const;
  void GenerateInterfaceGetOrBuilderMethod(io::Printer* printer) const;

  void GenerateGetListMethod(io::Printer* printer) const;
  void GenerateGetCountMethod(io::Printer* printer) const;
  void GenerateGetMethod(io::Printer* printer) const;
  void GenerateGetOrBuilderListMethod(io::Printer* printer) const;
  void GenerateGetOrBuilderMethod(io::Printer* printer) const;
  void GenerateWriteFieldMethod(io::Printer* printer) const;

  void GenerateEnsureIsMutableMethod(io::Printer* printer) const;
  void GenerateBuilderGetListMethod(io::Printer* printer) const;
  void GenerateBuilderGetCountMethod(io::Printer* printer) const;
  void GenerateBuilderGetMethod(io::Printer* printer) const;
  void GenerateBuilderSetMethod(io::Printer* printer) const;
  void GenerateBuilderSetBuilderMethod(io::Printer* printer) const;
  void GenerateBuilderAddMethod(io::Printer* printer) const;
  void GenerateBuilderAddAtIndexMethod(io::Printer* printer) const;
  void GenerateBuilderAddBuilderMethod(io::Printer* printer) const;
  void GenerateBuilderAddBuilderAtIndexMethod(io::Printer* printer) const;
  void GenerateBuilderAddAllMethod(io::Printer* printer) const;
  void GenerateBuilderClearMethod(io::Printer* printer) const;
  void GenerateBuilderRemoveMethod(io::Printer* printer) const;
  void GenerateBuilderGetBuilderMethod(io::Printer* printer) const;
  void GenerateBuilderGetOrBuilderMethod(io::Printer* printer) const;
  void GenerateBuilderGetOrBuilderListMethod(io::Printer* printer) const;
  void GenerateBuilderAddBuilderNoArgsMethod(io::Printer* printer) const;
  void GenerateBuilderAddBuilderAtIndexNoArgsMethod(io::Printer* printer) const;
  void GenerateBuilderGetBuilderListMethod(io::Printer* printer) const;
  void GenerateBuilderInternalGetFieldBuilderMethod(io::Printer* printer) const;
};

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_JAVA_IMMUTABLE_MESSAGE_FIELD_H__
