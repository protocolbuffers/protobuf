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
#include <google/protobuf/compiler/csharp/csharp_message_field.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

MessageFieldGenerator::MessageFieldGenerator(const FieldDescriptor* descriptor,
                                             int fieldOrdinal)
    : FieldGeneratorBase(descriptor, fieldOrdinal) {
  variables_["has_property_check"] = "has" + property_name();
  variables_["message_or_group"] = message_or_group();
}

MessageFieldGenerator::~MessageFieldGenerator() {

}

void MessageFieldGenerator::GenerateMembers(io::Printer* printer) {
  printer->Print(
    variables_,
    "private bool has$property_name$;\n"
    "private $type_name$ $name$_;\n");
  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "public bool Has$property_name$ {\n"
    "  get { return has$property_name$; }\n"
    "}\n");
  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "public $type_name$ $property_name$ {\n"
    "  get { return $name$_ ?? $default_value$; }\n"
    "}\n");
}

void MessageFieldGenerator::GenerateBuilderMembers(io::Printer* printer) {
  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "public bool Has$property_name$ {\n"
    " get { return result.has$property_name$; }\n"
    "}\n");
  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "public $type_name$ $property_name$ {\n"
    "  get { return result.$property_name$; }\n"
    "  set { Set$property_name$(value); }\n"
    "}\n");
  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "public Builder Set$property_name$($type_name$ value) {\n");
  AddNullCheck(printer);
  printer->Print(
    variables_,
    "  PrepareBuilder();\n"
    "  result.has$property_name$ = true;\n"
    "  result.$name$_ = value;\n"
    "  return this;\n"
    "}\n");
  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "public Builder Set$property_name$($type_name$.Builder builderForValue) {\n");
  AddNullCheck(printer, "builderForValue");
  printer->Print(
    variables_,
    "  PrepareBuilder();\n"
    "  result.has$property_name$ = true;\n"
    "  result.$name$_ = builderForValue.Build();\n"
    "  return this;\n"
    "}\n");
  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "public Builder Merge$property_name$($type_name$ value) {\n");
  AddNullCheck(printer);
  printer->Print(
    variables_,
    "  PrepareBuilder();\n"
    "  if (result.has$property_name$ &&\n"
    "      result.$name$_ != $default_value$) {\n"
    "      result.$name$_ = $type_name$.CreateBuilder(result.$name$_).MergeFrom(value).BuildPartial();\n"
    "  } else {\n"
    "    result.$name$_ = value;\n"
    "  }\n"
    "  result.has$property_name$ = true;\n"
    "  return this;\n"
    "}\n");
  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "public Builder Clear$property_name$() {\n"
    "  PrepareBuilder();\n"
    "  result.has$property_name$ = false;\n"
    "  result.$name$_ = null;\n"
    "  return this;\n"
    "}\n");
}

void MessageFieldGenerator::GenerateMergingCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if (other.Has$property_name$) {\n"
    "  Merge$property_name$(other.$property_name$);\n"
    "}\n");
}

void MessageFieldGenerator::GenerateBuildingCode(io::Printer* printer) {
  // Nothing to do for singular fields
}

void MessageFieldGenerator::GenerateParsingCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "$type_name$.Builder subBuilder = $type_name$.CreateBuilder();\n"
    "if (result.has$property_name$) {\n"
    "  subBuilder.MergeFrom($property_name$);\n"
    "}\n");

  if (descriptor_->type() == FieldDescriptor::TYPE_GROUP) {
    printer->Print(
      variables_,
      "input.ReadGroup($number$, subBuilder, extensionRegistry);\n");
  } else {
    printer->Print("input.ReadMessage(subBuilder, extensionRegistry);\n");
  }
  printer->Print(
    variables_,
    "$property_name$ = subBuilder.BuildPartial();\n");
}

void MessageFieldGenerator::GenerateSerializationCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if ($has_property_check$) {\n"
    "  output.Write$message_or_group$($number$, field_names[$field_ordinal$], $property_name$);\n"
    "}\n");
}

void MessageFieldGenerator::GenerateSerializedSizeCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if ($has_property_check$) {\n"
    "  size += pb::CodedOutputStream.Compute$message_or_group$Size($number$, $property_name$);\n"
    "}\n");
}

void MessageFieldGenerator::WriteHash(io::Printer* printer) {
  printer->Print(
    variables_,
    "if (has$property_name$) hash ^= $name$_.GetHashCode();\n");
}
void MessageFieldGenerator::WriteEquals(io::Printer* printer) {
  printer->Print(
    variables_,
    "if (has$property_name$ != other.has$property_name$ || (has$property_name$ && !$name$_.Equals(other.$name$_))) return false;\n");
}
void MessageFieldGenerator::WriteToString(io::Printer* printer) {
  variables_["field_name"] = GetFieldName(descriptor_);
  printer->Print(
    variables_,
    "PrintField(\"$field_name$\", has$property_name$, $name$_, writer);\n");
}

MessageOneofFieldGenerator::MessageOneofFieldGenerator(const FieldDescriptor* descriptor,
						       int fieldOrdinal)
    : MessageFieldGenerator(descriptor, fieldOrdinal) {
  SetCommonOneofFieldVariables(&variables_);
}

MessageOneofFieldGenerator::~MessageOneofFieldGenerator() {

}

void MessageOneofFieldGenerator::GenerateMembers(io::Printer* printer) {
  if (SupportFieldPresence(descriptor_->file())) {
    AddDeprecatedFlag(printer);
    printer->Print(
      variables_,
      "public bool Has$property_name$ {\n"
      "  get { return $has_property_check$; }\n"
      "}\n");
  }
  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "public $type_name$ $property_name$ {\n"
    "  get { return $has_property_check$ ? ($type_name$) $oneof_name$_ : $default_value$; }\n"
    "}\n");
}

void MessageOneofFieldGenerator::GenerateBuilderMembers(io::Printer* printer) {
  if (SupportFieldPresence(descriptor_->file())) {
    AddDeprecatedFlag(printer);
    printer->Print(
      variables_,
      "public bool Has$property_name$ {\n"
      "  get { return result.$has_property_check$; }\n"
      "}\n");
  }
  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "public $type_name$ $property_name$ {\n"
    "  get { return result.$has_property_check$ ? ($type_name$) result.$oneof_name$_ : $default_value$; }\n"
    "  set { Set$property_name$(value); }\n"
    "}\n");
  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "public Builder Set$property_name$($type_name$ value) {\n");
  AddNullCheck(printer);
  printer->Print(
    variables_,
    "  PrepareBuilder();\n"
    "  result.$oneof_name$Case_ = $oneof_property_name$OneofCase.$property_name$;\n"
    "  result.$oneof_name$_ = value;\n"
    "  return this;\n"
    "}\n");
  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "public Builder Set$property_name$($type_name$.Builder builderForValue) {\n");
  AddNullCheck(printer, "builderForValue");
  printer->Print(
    variables_,
    "  PrepareBuilder();\n"
    "  result.$oneof_name$Case_ = $oneof_property_name$OneofCase.$property_name$;\n"
    "  result.$oneof_name$_ = builderForValue.Build();\n"
    "  return this;\n"
    "}\n");
  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "public Builder Merge$property_name$($type_name$ value) {\n");
  AddNullCheck(printer);
  printer->Print(
    variables_,
    "  PrepareBuilder();\n"
    "  if (result.$has_property_check$ &&\n"
    "      result.$property_name$ != $default_value$) {\n"
    "    result.$oneof_name$_ = $type_name$.CreateBuilder(result.$property_name$).MergeFrom(value).BuildPartial();\n"
    "  } else {\n"
    "    result.$oneof_name$_ = value;\n"
    "  }\n"
    "  result.$oneof_name$Case_ = $oneof_property_name$OneofCase.$property_name$;\n"
    "  return this;\n"
    "}\n");
  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "public Builder Clear$property_name$() {\n"
    "  if (result.$has_property_check$) {\n"
    "    PrepareBuilder();\n"
    "    result.$oneof_name$Case_ = $oneof_property_name$OneofCase.None;\n"
    "    result.$oneof_name$_ = null;\n"
    "  }\n"
    "  return this;\n"
    "}\n");
}

void MessageOneofFieldGenerator::GenerateParsingCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "$type_name$.Builder subBuilder = $type_name$.CreateBuilder();\n"
    "if (result.$has_property_check$) {\n"
    "  subBuilder.MergeFrom($property_name$);\n"
    "}\n");

  if (descriptor_->type() == FieldDescriptor::TYPE_GROUP) {
    printer->Print(
      variables_,
      "input.ReadGroup($number$, subBuilder, extensionRegistry);\n");
  } else {
    printer->Print("input.ReadMessage(subBuilder, extensionRegistry);\n");
  }
  printer->Print(
    variables_,
    "result.$oneof_name$_ = subBuilder.BuildPartial();\n"
    "result.$oneof_name$Case_ = $oneof_property_name$OneofCase.$property_name$;\n");
}

void MessageOneofFieldGenerator::WriteEquals(io::Printer* printer) {
  printer->Print(
    variables_,
    "if (!$property_name$.Equals(other.$property_name$)) return false;\n");
}
void MessageOneofFieldGenerator::WriteToString(io::Printer* printer) {
  printer->Print(
    variables_,
    "PrintField(\"$descriptor_name$\", $has_property_check$, $oneof_name$_, writer);\n");
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
