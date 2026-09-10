// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
// Author: jonp@google.com (Jon Perlow)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#ifndef GOOGLE_PROTOBUF_COMPILER_JAVA_IMMUTABLE_STRING_FIELD_H__
#define GOOGLE_PROTOBUF_COMPILER_JAVA_IMMUTABLE_STRING_FIELD_H__

#include <string>

#include "google/protobuf/compiler/java/full/field_generator.h"
#include "google/protobuf/descriptor.h"

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

class ImmutableStringFieldGenerator : public ImmutableFieldGenerator {
 public:
  explicit ImmutableStringFieldGenerator(const FieldDescriptor* descriptor,
                                         int bit_index, Context* context);
  ImmutableStringFieldGenerator(const ImmutableStringFieldGenerator&) = delete;
  ImmutableStringFieldGenerator& operator=(
      const ImmutableStringFieldGenerator&) = delete;
  ~ImmutableStringFieldGenerator() override;

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

 private:
  void GenerateInterfaceHasMethod(io::Printer* printer) const;
  void GenerateInterfaceGetMethod(io::Printer* printer) const;
  void GenerateInterfaceGetBytesMethod(io::Printer* printer) const;

  void GenerateHasMethod(io::Printer* printer) const;
  void GenerateGetMethod(io::Printer* printer) const;
  void GenerateGetBytesMethod(io::Printer* printer) const;

  void GenerateBuilderHasMethod(io::Printer* printer) const;
  void GenerateBuilderGetMethod(io::Printer* printer) const;
  void GenerateBuilderGetBytesMethod(io::Printer* printer) const;
  void GenerateBuilderSetMethod(io::Printer* printer) const;
  void GenerateBuilderClearMethod(io::Printer* printer) const;
  void GenerateBuilderSetBytesMethod(io::Printer* printer) const;
};

class ImmutableStringOneofFieldGenerator
    : public ImmutableStringFieldGenerator {
 public:
  ImmutableStringOneofFieldGenerator(const FieldDescriptor* descriptor,
                                     int bit_index, Context* context);
  ImmutableStringOneofFieldGenerator(
      const ImmutableStringOneofFieldGenerator&) = delete;
  ImmutableStringOneofFieldGenerator& operator=(
      const ImmutableStringOneofFieldGenerator&) = delete;
  ~ImmutableStringOneofFieldGenerator() override;

  void GenerateMembers(io::Printer* printer) const override;
  void GenerateBuilderMembers(io::Printer* printer) const override;
  void GenerateBuilderClearCode(io::Printer* printer) const override;
  void GenerateMergingCode(io::Printer* printer) const override;
  void GenerateBuildingCode(io::Printer* printer) const override;
  void GenerateBuilderParsingCode(io::Printer* printer) const override;
  void GenerateSerializationCode(io::Printer* printer) const override;
  void GenerateSerializedSizeCode(io::Printer* printer) const override;

 private:
  void GenerateHasMethod(io::Printer* printer) const;
  void GenerateGetMethod(io::Printer* printer) const;
  void GenerateGetBytesMethod(io::Printer* printer) const;

  void GenerateBuilderHasMethod(io::Printer* printer) const;
  void GenerateBuilderGetMethod(io::Printer* printer) const;
  void GenerateBuilderGetBytesMethod(io::Printer* printer) const;
  void GenerateBuilderSetMethod(io::Printer* printer) const;
  void GenerateBuilderClearMethod(io::Printer* printer) const;
  void GenerateBuilderSetBytesMethod(io::Printer* printer) const;
};

class RepeatedImmutableStringFieldGenerator
    : public ImmutableStringFieldGenerator {
 public:
  explicit RepeatedImmutableStringFieldGenerator(
      const FieldDescriptor* descriptor, int bit_index, Context* context);
  RepeatedImmutableStringFieldGenerator(
      const RepeatedImmutableStringFieldGenerator&) = delete;
  RepeatedImmutableStringFieldGenerator& operator=(
      const RepeatedImmutableStringFieldGenerator&) = delete;
  ~RepeatedImmutableStringFieldGenerator() override;

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
  void GenerateInterfaceGetBytesMethod(io::Printer* printer) const;

  void GenerateGetListMethod(io::Printer* printer) const;
  void GenerateGetCountMethod(io::Printer* printer) const;
  void GenerateGetMethod(io::Printer* printer) const;
  void GenerateGetBytesMethod(io::Printer* printer) const;

  void GenerateEnsureIsMutableMethod(io::Printer* printer) const;
  void GenerateBuilderGetListMethod(io::Printer* printer) const;
  void GenerateBuilderGetCountMethod(io::Printer* printer) const;
  void GenerateBuilderGetMethod(io::Printer* printer) const;
  void GenerateBuilderGetBytesMethod(io::Printer* printer) const;
  void GenerateBuilderSetMethod(io::Printer* printer) const;
  void GenerateBuilderAddMethod(io::Printer* printer) const;
  void GenerateBuilderAddAllMethod(io::Printer* printer) const;
  void GenerateBuilderClearMethod(io::Printer* printer) const;
  void GenerateBuilderAddBytesMethod(io::Printer* printer) const;
};

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_JAVA_IMMUTABLE_STRING_FIELD_H__
