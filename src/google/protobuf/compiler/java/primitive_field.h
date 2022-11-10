// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#ifndef GOOGLE_PROTOBUF_COMPILER_JAVA_PRIMITIVE_FIELD_H__
#define GOOGLE_PROTOBUF_COMPILER_JAVA_PRIMITIVE_FIELD_H__

#include <string>

#include "google/protobuf/compiler/java/field.h"

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

class ImmutablePrimitiveFieldGenerator : public ImmutableFieldGenerator {
 public:
  explicit ImmutablePrimitiveFieldGenerator(const FieldDescriptor* descriptor,
                                            int messageBitIndex,
                                            int builderBitIndex,
                                            Context* context);
  ImmutablePrimitiveFieldGenerator(const ImmutablePrimitiveFieldGenerator&) =
      delete;
  ImmutablePrimitiveFieldGenerator& operator=(
      const ImmutablePrimitiveFieldGenerator&) = delete;
  ~ImmutablePrimitiveFieldGenerator() override;

  // implements ImmutableFieldGenerator
  // ---------------------------------------
  int GetMessageBitIndex() const override;
  int GetBuilderBitIndex() const override;
  int GetNumBitsForMessage() const override;
  int GetNumBitsForBuilder() const override;
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
  void GenerateKotlinDslMembers(io::Printer* printer) const override;

  std::string GetBoxedType() const override;

 protected:
  const FieldDescriptor* descriptor_;
  int message_bit_index_;
  int builder_bit_index_;
  absl::flat_hash_map<absl::string_view, std::string> variables_;
  ClassNameResolver* name_resolver_;
};

class ImmutablePrimitiveOneofFieldGenerator
    : public ImmutablePrimitiveFieldGenerator {
 public:
  ImmutablePrimitiveOneofFieldGenerator(const FieldDescriptor* descriptor,
                                        int messageBitIndex,
                                        int builderBitIndex, Context* context);
  ImmutablePrimitiveOneofFieldGenerator(
      const ImmutablePrimitiveOneofFieldGenerator&) = delete;
  ImmutablePrimitiveOneofFieldGenerator& operator=(
      const ImmutablePrimitiveOneofFieldGenerator&) = delete;
  ~ImmutablePrimitiveOneofFieldGenerator() override;

  void GenerateMembers(io::Printer* printer) const override;
  void GenerateBuilderMembers(io::Printer* printer) const override;
  void GenerateBuilderClearCode(io::Printer* printer) const override;
  void GenerateBuildingCode(io::Printer* printer) const override;
  void GenerateMergingCode(io::Printer* printer) const override;
  void GenerateBuilderParsingCode(io::Printer* printer) const override;
  void GenerateSerializationCode(io::Printer* printer) const override;
  void GenerateSerializedSizeCode(io::Printer* printer) const override;
};

class RepeatedImmutablePrimitiveFieldGenerator
    : public ImmutablePrimitiveFieldGenerator {
 public:
  explicit RepeatedImmutablePrimitiveFieldGenerator(
      const FieldDescriptor* descriptor, int messageBitIndex,
      int builderBitIndex, Context* context);
  RepeatedImmutablePrimitiveFieldGenerator(
      const RepeatedImmutablePrimitiveFieldGenerator&) = delete;
  RepeatedImmutablePrimitiveFieldGenerator& operator=(
      const RepeatedImmutablePrimitiveFieldGenerator&) = delete;
  ~RepeatedImmutablePrimitiveFieldGenerator() override;

  // implements ImmutableFieldGenerator ---------------------------------------
  int GetNumBitsForMessage() const override;
  int GetNumBitsForBuilder() const override;
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
  void GenerateKotlinDslMembers(io::Printer* printer) const override;

  std::string GetBoxedType() const override;
};

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_JAVA_PRIMITIVE_FIELD_H__
