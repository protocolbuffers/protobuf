// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_CSHARP_CSHARP_MAP_FIELD_H__
#define GOOGLE_PROTOBUF_COMPILER_CSHARP_CSHARP_MAP_FIELD_H__

#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/csharp/csharp_field_base.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

class MapFieldGenerator : public FieldGeneratorBase {
 public:
  MapFieldGenerator(const FieldDescriptor* descriptor,
                    int presenceIndex,
                    const Options* options);
  ~MapFieldGenerator() override;

  MapFieldGenerator(const MapFieldGenerator&) = delete;
  MapFieldGenerator& operator=(const MapFieldGenerator&) = delete;

  void GenerateCloningCode(io::Printer* printer) override;
  void GenerateFreezingCode(io::Printer* printer) override;
  void GenerateMembers(io::Printer* printer) override;
  void GenerateMergingCode(io::Printer* printer) override;
  void GenerateParsingCode(io::Printer* printer) override;
  void GenerateParsingCode(io::Printer* printer,
                           bool use_parse_context) override;
  void GenerateSerializationCode(io::Printer* printer) override;
  void GenerateSerializationCode(io::Printer* printer,
                                 bool use_write_context) override;
  void GenerateSerializedSizeCode(io::Printer* printer) override;

  void WriteHash(io::Printer* printer) override;
  void WriteEquals(io::Printer* printer) override;
  void WriteToString(io::Printer* printer) override;
};

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_CSHARP_CSHARP_MAP_FIELD_H__
