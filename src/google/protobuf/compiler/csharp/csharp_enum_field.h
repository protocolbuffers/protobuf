// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_CSHARP_CSHARP_ENUM_FIELD_H__
#define GOOGLE_PROTOBUF_COMPILER_CSHARP_CSHARP_ENUM_FIELD_H__

#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/csharp/csharp_primitive_field.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

class EnumFieldGenerator : public PrimitiveFieldGenerator {
 public:
  EnumFieldGenerator(const FieldDescriptor* descriptor,
                     int presenceIndex,
                     const Options *options);
  ~EnumFieldGenerator() override;

  EnumFieldGenerator(const EnumFieldGenerator&) = delete;
  EnumFieldGenerator& operator=(const EnumFieldGenerator&) = delete;

  void GenerateCodecCode(io::Printer* printer) override;
  void GenerateParsingCode(io::Printer* printer) override;
  void GenerateSerializationCode(io::Printer* printer) override;
  void GenerateSerializedSizeCode(io::Printer* printer) override;
  void GenerateExtensionCode(io::Printer* printer) override;
};

class EnumOneofFieldGenerator : public PrimitiveOneofFieldGenerator {
 public:
  EnumOneofFieldGenerator(const FieldDescriptor* descriptor,
                          int presenceIndex,
                          const Options *options);
  ~EnumOneofFieldGenerator() override;

  EnumOneofFieldGenerator(const EnumOneofFieldGenerator&) = delete;
  EnumOneofFieldGenerator& operator=(const EnumOneofFieldGenerator&) = delete;

  void GenerateMergingCode(io::Printer* printer) override;
  void GenerateParsingCode(io::Printer* printer) override;
  void GenerateSerializationCode(io::Printer* printer) override;
  void GenerateSerializedSizeCode(io::Printer* printer) override;
};

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_CSHARP_CSHARP_ENUM_FIELD_H__
