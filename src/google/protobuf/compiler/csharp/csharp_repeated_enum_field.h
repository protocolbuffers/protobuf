// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_CSHARP_REPEATED_ENUM_FIELD_H__
#define GOOGLE_PROTOBUF_COMPILER_CSHARP_REPEATED_ENUM_FIELD_H__

#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/csharp/csharp_field_base.h"
#include "google/protobuf/compiler/csharp/csharp_source_generator_base.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

// TODO: Refactor repeated field support; all the implementations are
// *really* similar. We should probably have a RepeatedFieldGeneratorBase.
class RepeatedEnumFieldGenerator : public FieldGeneratorBase {
 public:
  RepeatedEnumFieldGenerator(const FieldDescriptor* descriptor,
                             int presenceIndex,
                             const Options *options);
  ~RepeatedEnumFieldGenerator();

  RepeatedEnumFieldGenerator(const RepeatedEnumFieldGenerator&) = delete;
  RepeatedEnumFieldGenerator& operator=(const RepeatedEnumFieldGenerator&) =
      delete;

  virtual void GenerateCloningCode(io::Printer* printer) override;
  virtual void GenerateFreezingCode(io::Printer* printer) override;
  virtual void GenerateMembers(io::Printer* printer) override;
  virtual void GenerateMergingCode(io::Printer* printer) override;
  virtual void GenerateParsingCode(io::Printer* printer) override;
  virtual void GenerateParsingCode(io::Printer* printer, bool use_parse_context) override;
  virtual void GenerateSerializationCode(io::Printer* printer) override;
  virtual void GenerateSerializationCode(io::Printer* printer, bool use_write_context) override;
  virtual void GenerateSerializedSizeCode(io::Printer* printer) override;
  virtual void GenerateExtensionCode(io::Printer* printer) override;

  virtual void WriteHash(io::Printer* printer) override;
  virtual void WriteEquals(io::Printer* printer) override;
  virtual void WriteToString(io::Printer* printer) override;
};

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_CSHARP_REPEATED_ENUM_FIELD_H__

