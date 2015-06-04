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
#include <google/protobuf/stubs/strutil.h>

#include <google/protobuf/compiler/csharp/csharp_helpers.h>
#include <google/protobuf/compiler/csharp/csharp_primitive_field.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

PrimitiveFieldGenerator::PrimitiveFieldGenerator(
    const FieldDescriptor* descriptor, int fieldOrdinal)
    : FieldGeneratorBase(descriptor, fieldOrdinal) {
}

PrimitiveFieldGenerator::~PrimitiveFieldGenerator() {

}

void PrimitiveFieldGenerator::GenerateMembers(io::Printer* printer) {
  if (SupportFieldPresence(descriptor_->file())) {
    printer->Print(variables_, "private bool has$property_name$;\n");
  }
  printer->Print(
    variables_,
    "private $type_name$ $name_def_message$;\n");
  AddDeprecatedFlag(printer);
  if (SupportFieldPresence(descriptor_->file())) {
    printer->Print(
      variables_,
      "public bool Has$property_name$ {\n"
      "  get { return has$property_name$; }\n"
      "}\n");
  }
  AddPublicMemberAttributes(printer);
  printer->Print(
    variables_,
    "public $type_name$ $property_name$ {\n"
    "  get { return $name$_; }\n"
    "}\n");
}

void PrimitiveFieldGenerator::GenerateBuilderMembers(io::Printer* printer) {
  AddDeprecatedFlag(printer);
  if (SupportFieldPresence(descriptor_->file())) {
    printer->Print(
      variables_,
      "public bool Has$property_name$ {\n"
      "  get { return result.has$property_name$; }\n"
      "}\n");
  }
  AddPublicMemberAttributes(printer);
  printer->Print(
    variables_,
    "public $type_name$ $property_name$ {\n"
    "  get { return result.$property_name$; }\n"
    "  set { Set$property_name$(value); }\n"
    "}\n");
  AddPublicMemberAttributes(printer);
  printer->Print(
    variables_,
    "public Builder Set$property_name$($type_name$ value) {\n");
  AddNullCheck(printer);
  printer->Print("  PrepareBuilder();\n");
  if (SupportFieldPresence(descriptor_->file())) {
    printer->Print(
      variables_,
      "  result.has$property_name$ = true;\n");
  }
  printer->Print(
    variables_,
    "  result.$name$_ = value;\n"
    "  return this;\n"
    "}\n");
  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "public Builder Clear$property_name$() {\n"
    "  PrepareBuilder();\n");
  if (SupportFieldPresence(descriptor_->file())) {
    printer->Print(
      variables_,
      "  result.has$property_name$ = false;\n");
  }
  printer->Print(
    variables_,
    "  result.$name$_ = $default_value$;\n"
    "  return this;\n"
    "}\n");
}

void PrimitiveFieldGenerator::GenerateMergingCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if ($other_has_property_check$) {\n"
    "  $property_name$ = other.$property_name$;\n"
    "}\n");
}

void PrimitiveFieldGenerator::GenerateBuildingCode(io::Printer* printer) {
  // Nothing to do here for primitive types
}

void PrimitiveFieldGenerator::GenerateParsingCode(io::Printer* printer) {
  if (SupportFieldPresence(descriptor_->file())) {
    printer->Print(
      variables_,
      "result.has$property_name$ = input.Read$capitalized_type_name$(ref result.$name$_);\n");
  } else {
    printer->Print(
      variables_,
      "input.Read$capitalized_type_name$(ref result.$name$_);\n");
  }
}

void PrimitiveFieldGenerator::GenerateSerializationCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if ($has_property_check$) {\n"
    "  output.Write$capitalized_type_name$($number$, field_names[$field_ordinal$], $property_name$);\n"
    "}\n");
}

void PrimitiveFieldGenerator::GenerateSerializedSizeCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if ($has_property_check$) {\n"
    "  size += pb::CodedOutputStream.Compute$capitalized_type_name$Size($number$, $property_name$);\n"
    "}\n");
}

void PrimitiveFieldGenerator::WriteHash(io::Printer* printer) {
  printer->Print(
    variables_,
    "if ($has_property_check$) {\n"
    "  hash ^= $name$_.GetHashCode();\n"
    "}\n");
}
void PrimitiveFieldGenerator::WriteEquals(io::Printer* printer) {
  if (SupportFieldPresence(descriptor_->file())) {
    printer->Print(
      variables_,
      "if (has$property_name$ != other.has$property_name$ || (has$property_name$ && !$name$_.Equals(other.$name$_))) return false;\n");
  } else {
    printer->Print(
      variables_,
      "if (!$name$_.Equals(other.$name$_)) return false;\n");
  }
}
void PrimitiveFieldGenerator::WriteToString(io::Printer* printer) {
  printer->Print(
    variables_,
    "PrintField(\"$descriptor_name$\", $has_property_check$, $name$_, writer);\n");
}

PrimitiveOneofFieldGenerator::PrimitiveOneofFieldGenerator(
    const FieldDescriptor* descriptor, int fieldOrdinal)
    : PrimitiveFieldGenerator(descriptor, fieldOrdinal) {
  SetCommonOneofFieldVariables(&variables_);
}

PrimitiveOneofFieldGenerator::~PrimitiveOneofFieldGenerator() {
}

void PrimitiveOneofFieldGenerator::GenerateMembers(io::Printer* printer) {
  AddDeprecatedFlag(printer);
  if (SupportFieldPresence(descriptor_->file())) {
    printer->Print(
      variables_,
      "public bool Has$property_name$ {\n"
      "  get { return $has_property_check$; }\n"
      "}\n");
  }
  AddPublicMemberAttributes(printer);
  printer->Print(
    variables_,
    "public $type_name$ $property_name$ {\n"
    "  get { return $has_property_check$ ? ($type_name$) $oneof_name$_ : $default_value$; }\n"
    "}\n");
}

void PrimitiveOneofFieldGenerator::GenerateBuilderMembers(io::Printer* printer) {
  AddDeprecatedFlag(printer);
  if (SupportFieldPresence(descriptor_->file())) {
    printer->Print(
      variables_,
      "public bool Has$property_name$ {\n"
      "  get { return result.$has_property_check$; }\n"
      "}\n");
  }
  AddPublicMemberAttributes(printer);
  printer->Print(
    variables_,
    "public $type_name$ $property_name$ {\n"
    "  get { return result.$has_property_check$ ? ($type_name$) result.$oneof_name$_ : $default_value$; }\n"
    "  set { Set$property_name$(value); }\n"
    "}\n");
  AddPublicMemberAttributes(printer);
  printer->Print(
    variables_,
    "public Builder Set$property_name$($type_name$ value) {\n");
  AddNullCheck(printer);
  printer->Print(
    variables_,
    "  PrepareBuilder();\n"
    "  result.$oneof_name$_ = value;\n"
    "  result.$oneof_name$Case_ = $oneof_property_name$OneofCase.$property_name$;\n"
    "  return this;\n"
    "}\n");
  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "public Builder Clear$property_name$() {\n"
    "  PrepareBuilder();\n"
    "  if (result.$has_property_check$) {\n"
    "    result.$oneof_name$Case_ = $oneof_property_name$OneofCase.None;\n"
    "  }\n"
    "  return this;\n"
    "}\n");
}

void PrimitiveOneofFieldGenerator::WriteEquals(io::Printer* printer) {
  printer->Print(
    variables_,
    "if (!$property_name$.Equals(other.$property_name$)) return false;\n");
}
void PrimitiveOneofFieldGenerator::WriteToString(io::Printer* printer) {
  printer->Print(variables_,
    "PrintField(\"$descriptor_name$\", $has_property_check$, $oneof_name$_, writer);\n");
}

void PrimitiveOneofFieldGenerator::GenerateParsingCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "$type_name$ value = $default_value$;\n"
    "if (input.Read$capitalized_type_name$(ref value)) {\n"
    "  result.$oneof_name$_ = value;\n"
    "  result.$oneof_name$Case_ = $oneof_property_name$OneofCase.$property_name$;\n"
    "}\n");
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
