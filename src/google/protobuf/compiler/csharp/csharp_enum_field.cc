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

#include <sstream>

#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>

#include <google/protobuf/compiler/csharp/csharp_helpers.h>
#include <google/protobuf/compiler/csharp/csharp_enum_field.h>
#include <google/protobuf/compiler/csharp/csharp_writer.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

EnumFieldGenerator::EnumFieldGenerator(const FieldDescriptor* descriptor,
                                       int fieldOrdinal)
    : PrimitiveFieldGenerator(descriptor, fieldOrdinal) {
}

EnumFieldGenerator::~EnumFieldGenerator() {
}

void EnumFieldGenerator::GenerateParsingCode(Writer* writer) {
  writer->WriteLine("object unknown;");
  writer->WriteLine("if(input.ReadEnum(ref result.$0$_, out unknown)) {", name());
  if (SupportFieldPresence(descriptor_->file())) {
    writer->WriteLine("  result.has$0$ = true;", property_name());
  }
  writer->WriteLine("} else if(unknown is int) {");
  if (!use_lite_runtime()) {
    writer->WriteLine("  if (unknownFields == null) {");  // First unknown field - create builder now
    writer->WriteLine(
        "    unknownFields = pb::UnknownFieldSet.CreateBuilder(this.UnknownFields);");
    writer->WriteLine("  }");
    writer->WriteLine(
        "  unknownFields.MergeVarintField($0$, (ulong)(int)unknown);",
        number());
  }
  writer->WriteLine("}");
}

void EnumFieldGenerator::GenerateSerializationCode(Writer* writer) {
  writer->WriteLine("if ($0$) {", has_property_check);
  writer->WriteLine(
      "  output.WriteEnum($0$, field_names[$2$], (int) $1$, $1$);", number(),
      property_name(), field_ordinal());
  writer->WriteLine("}");
}

void EnumFieldGenerator::GenerateSerializedSizeCode(Writer* writer) {
  writer->WriteLine("if ($0$) {", has_property_check);
  writer->WriteLine(
      "  size += pb::CodedOutputStream.ComputeEnumSize($0$, (int) $1$);",
      number(), property_name());
  writer->WriteLine("}");
}

EnumOneofFieldGenerator::EnumOneofFieldGenerator(const FieldDescriptor* descriptor,
						 int fieldOrdinal)
  : PrimitiveOneofFieldGenerator(descriptor, fieldOrdinal) {
}

EnumOneofFieldGenerator::~EnumOneofFieldGenerator() {
}

void EnumOneofFieldGenerator::GenerateParsingCode(Writer* writer) {
  writer->WriteLine("object unknown;");
  writer->WriteLine("$0$ enumValue = $1$;", type_name(), default_value());
  writer->WriteLine("if(input.ReadEnum(ref enumValue, out unknown)) {",
                    name());
  writer->WriteLine("  result.$0$_ = enumValue;", oneof_name());
  writer->WriteLine("  result.$0$Case_ = $1$OneofCase.$2$;",
		    oneof_name(), oneof_property_name(), property_name());
  writer->WriteLine("} else if(unknown is int) {");
  if (!use_lite_runtime()) {
    writer->WriteLine("  if (unknownFields == null) {");  // First unknown field - create builder now
    writer->WriteLine(
        "    unknownFields = pb::UnknownFieldSet.CreateBuilder(this.UnknownFields);");
    writer->WriteLine("  }");
    writer->WriteLine(
        "  unknownFields.MergeVarintField($0$, (ulong)(int)unknown);",
        number());
  }
  writer->WriteLine("}");
}

void EnumOneofFieldGenerator::GenerateSerializationCode(Writer* writer) {
  writer->WriteLine("if ($0$) {", has_property_check);
  writer->WriteLine(
      "  output.WriteEnum($0$, field_names[$2$], (int) $1$, $1$);", number(),
      property_name(), field_ordinal());
  writer->WriteLine("}");
}

void EnumOneofFieldGenerator::GenerateSerializedSizeCode(Writer* writer) {
  writer->WriteLine("if ($0$) {", has_property_check);
  writer->WriteLine(
      "  size += pb::CodedOutputStream.ComputeEnumSize($0$, (int) $1$);",
      number(), property_name());
  writer->WriteLine("}");
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
