// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_JAVA_IMMUTABLE_MAP_FIELD_H__
#define GOOGLE_PROTOBUF_COMPILER_JAVA_IMMUTABLE_MAP_FIELD_H__

#include "google/protobuf/compiler/java/context.h"
#include "google/protobuf/compiler/java/full/field_generator.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

class ImmutableMapFieldGenerator : public ImmutableFieldGenerator {
 public:
  explicit ImmutableMapFieldGenerator(const FieldDescriptor* descriptor,
                                      int bit_index, Context* context);
  ~ImmutableMapFieldGenerator() override = default;

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
  void SetMessageVariables(const FieldGeneratorInfo* info);
  void GenerateMapGetters(io::Printer* printer) const;
  void GenerateMessageMapBuilderMembers(io::Printer* printer) const;
  void GenerateMessageMapGetters(io::Printer* printer) const;

  void GenerateInterfaceGetCountMethod(io::Printer* printer) const;
  void GenerateInterfaceContainsMethod(io::Printer* printer) const;
  void GenerateInterfaceGetMapMethod(io::Printer* printer) const;
  void GenerateInterfaceGetOrDefaultMethod(io::Printer* printer) const;
  void GenerateInterfaceGetOrThrowMethod(io::Printer* printer) const;
  void GenerateInterfaceGetValueMapMethod(io::Printer* printer) const;
  void GenerateInterfaceGetValueOrDefaultMethod(io::Printer* printer) const;
  void GenerateInterfaceGetValueOrThrowMethod(io::Printer* printer) const;

  void GenerateBuilderClearMethod(io::Printer* printer) const;
  void GenerateBuilderRemoveMethod(io::Printer* printer) const;
  void GenerateBuilderPutMethod(io::Printer* printer) const;
  void GenerateBuilderPutAllMethod(io::Printer* printer) const;
  void GenerateBuilderPutValueMethod(io::Printer* printer) const;
  void GenerateBuilderPutAllValueMethod(io::Printer* printer) const;

  void GenerateMessageMapBuilderClearMethod(io::Printer* printer) const;
  void GenerateMessageMapBuilderRemoveMethod(io::Printer* printer) const;
  void GenerateMessageMapBuilderPutMethod(io::Printer* printer) const;
  void GenerateMessageMapBuilderPutAllMethod(io::Printer* printer) const;
  void GenerateMessageMapBuilderPutBuilderIfAbsentMethod(
      io::Printer* printer) const;
};

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_JAVA_IMMUTABLE_MAP_FIELD_H__
