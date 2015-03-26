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
    : FieldGeneratorBase(descriptor, fieldOrdinal) {
}

EnumFieldGenerator::~EnumFieldGenerator() {

}

void EnumFieldGenerator::GenerateMembers(Writer* writer) {
  writer->WriteLine("private bool has$0$;", property_name());
  writer->WriteLine("private $0$ $1$_ = $2$;", type_name(), name(),
                    default_value());
  AddDeprecatedFlag(writer);
  writer->WriteLine("public bool Has$0$ {", property_name());
  writer->WriteLine("  get { return has$0$; }", property_name());
  writer->WriteLine("}");
  AddPublicMemberAttributes(writer);
  writer->WriteLine("public $0$ $1$ {", type_name(), property_name());
  writer->WriteLine("  get { return $0$_; }", name());
  writer->WriteLine("}");
}

void EnumFieldGenerator::GenerateBuilderMembers(Writer* writer) {
  AddDeprecatedFlag(writer);
  writer->WriteLine("public bool Has$0$ {", property_name());
  writer->WriteLine(" get { return result.has$0$; }", property_name());
  writer->WriteLine("}");
  AddPublicMemberAttributes(writer);
  writer->WriteLine("public $0$ $1$ {", type_name(), property_name());
  writer->WriteLine("  get { return result.$0$; }", property_name());
  writer->WriteLine("  set { Set$0$(value); }", property_name());
  writer->WriteLine("}");
  AddPublicMemberAttributes(writer);
  writer->WriteLine("public Builder Set$0$($1$ value) {", property_name(),
                    type_name());
  writer->WriteLine("  PrepareBuilder();");
  writer->WriteLine("  result.has$0$ = true;", property_name());
  writer->WriteLine("  result.$0$_ = value;", name());
  writer->WriteLine("  return this;");
  writer->WriteLine("}");
  AddDeprecatedFlag(writer);
  writer->WriteLine("public Builder Clear$0$() {", property_name());
  writer->WriteLine("  PrepareBuilder();");
  writer->WriteLine("  result.has$0$ = false;", property_name());
  writer->WriteLine("  result.$0$_ = $1$;", name(), default_value());
  writer->WriteLine("  return this;");
  writer->WriteLine("}");
}

void EnumFieldGenerator::GenerateMergingCode(Writer* writer) {
  writer->WriteLine("if (other.Has$0$) {", property_name());
  writer->WriteLine("  $0$ = other.$0$;", property_name());
  writer->WriteLine("}");
}

void EnumFieldGenerator::GenerateBuildingCode(Writer* writer) {
  // Nothing to do here for enum types
}

void EnumFieldGenerator::GenerateParsingCode(Writer* writer) {
  writer->WriteLine("object unknown;");
  writer->WriteLine("if(input.ReadEnum(ref result.$0$_, out unknown)) {",
                    name());
  writer->WriteLine("  result.has$0$ = true;", property_name());
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
  writer->WriteLine("if (has$0$) {", property_name());
  writer->WriteLine(
      "  output.WriteEnum($0$, field_names[$2$], (int) $1$, $1$);", number(),
      property_name(), field_ordinal());
  writer->WriteLine("}");
}

void EnumFieldGenerator::GenerateSerializedSizeCode(Writer* writer) {
  writer->WriteLine("if (has$0$) {", property_name());
  writer->WriteLine(
      "  size += pb::CodedOutputStream.ComputeEnumSize($0$, (int) $1$);",
      number(), property_name());
  writer->WriteLine("}");
}

void EnumFieldGenerator::WriteHash(Writer* writer) {
  writer->WriteLine("if (has$0$) hash ^= $1$_.GetHashCode();", property_name(),
                    name());
}
void EnumFieldGenerator::WriteEquals(Writer* writer) {
  writer->WriteLine(
      "if (has$0$ != other.has$0$ || (has$0$ && !$1$_.Equals(other.$1$_))) return false;",
      property_name(), name());
}
void EnumFieldGenerator::WriteToString(Writer* writer) {
  writer->WriteLine("PrintField(\"$0$\", has$1$, $2$_, writer);",
                    descriptor_->name(), property_name(), name());
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
