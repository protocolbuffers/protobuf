// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: jonskeet@google.com (Jon Skeet)
//  Following the Java generator by kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <map>
#include <string>

#include <google/protobuf/compiler/csharp/csharp_enum_field.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/compiler/csharp/csharp_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

namespace {

// TODO(kenton):  Factor out a "SetCommonFieldVariables()" to get rid of
//   repeat code between this and the other field types.
void SetEnumVariables(const FieldDescriptor* descriptor,
                      map<string, string>* variables) {
  const EnumValueDescriptor* default_value;
  default_value = descriptor->default_value_enum();

  string type = ClassName(descriptor->enum_type());

  (*variables)["name"] =
    UnderscoresToCamelCase(descriptor);
  (*variables)["capitalized_name"] =
    UnderscoresToCapitalizedCamelCase(descriptor);
  (*variables)["number"] = SimpleItoa(descriptor->number());
  (*variables)["type"] = type;
  (*variables)["default"] = type + "." + default_value->name();
}

}  // namespace

// ===================================================================

EnumFieldGenerator::
EnumFieldGenerator(const FieldDescriptor* descriptor)
  : descriptor_(descriptor) {
  SetEnumVariables(descriptor, &variables_);
}

EnumFieldGenerator::~EnumFieldGenerator() {}

void EnumFieldGenerator::
GenerateMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "private boolean has$capitalized_name$;\r\n"
    "private $type$ $name$_ = $default$;\r\n"
    "public boolean has$capitalized_name$() { return has$capitalized_name$; }\r\n"
    "public $type$ get$capitalized_name$() { return $name$_; }\r\n");
}

void EnumFieldGenerator::
GenerateBuilderMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "public boolean has$capitalized_name$() {\r\n"
    "  return result.has$capitalized_name$();\r\n"
    "}\r\n"
    "public $type$ get$capitalized_name$() {\r\n"
    "  return result.get$capitalized_name$();\r\n"
    "}\r\n"
    "public Builder set$capitalized_name$($type$ value) {\r\n"
    "  result.has$capitalized_name$ = true;\r\n"
    "  result.$name$_ = value;\r\n"
    "  return this;\r\n"
    "}\r\n"
    "public Builder clear$capitalized_name$() {\r\n"
    "  result.has$capitalized_name$ = false;\r\n"
    "  result.$name$_ = $default$;\r\n"
    "  return this;\r\n"
    "}\r\n");
}

void EnumFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (other.has$capitalized_name$()) {\r\n"
    "  set$capitalized_name$(other.get$capitalized_name$());\r\n"
    "}\r\n");
}

void EnumFieldGenerator::
GenerateBuildingCode(io::Printer* printer) const {
  // Nothing to do here for enum types.
}

void EnumFieldGenerator::
GenerateParsingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "int rawValue = input.readEnum();\r\n"
    "$type$ value = $type$.valueOf(rawValue);\r\n"
    "if (value == null) {\r\n"
    "  unknownFields.mergeVarintField($number$, rawValue);\r\n"
    "} else {\r\n"
    "  set$capitalized_name$(value);\r\n"
    "}\r\n");
}

void EnumFieldGenerator::
GenerateSerializationCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (has$capitalized_name$()) {\r\n"
    "  output.writeEnum($number$, get$capitalized_name$().getNumber());\r\n"
    "}\r\n");
}

void EnumFieldGenerator::
GenerateSerializedSizeCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (has$capitalized_name$()) {\r\n"
    "  size += pb::CodedOutputStream\r\n"
    "    .computeEnumSize($number$, get$capitalized_name$().getNumber());\r\n"
    "}\r\n");
}

string EnumFieldGenerator::GetBoxedType() const {
  return ClassName(descriptor_->enum_type());
}

// ===================================================================

RepeatedEnumFieldGenerator::
RepeatedEnumFieldGenerator(const FieldDescriptor* descriptor)
  : descriptor_(descriptor) {
  SetEnumVariables(descriptor, &variables_);
}

RepeatedEnumFieldGenerator::~RepeatedEnumFieldGenerator() {}

void RepeatedEnumFieldGenerator::
GenerateMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "private java.util.List<$type$> $name$_ =\r\n"
    "  java.util.Collections.emptyList();\r\n"
    "public java.util.List<$type$> get$capitalized_name$List() {\r\n"
    "  return $name$_;\r\n"   // note:  unmodifiable list
    "}\r\n"
    "public int get$capitalized_name$Count() { return $name$_.size(); }\r\n"
    "public $type$ get$capitalized_name$(int index) {\r\n"
    "  return $name$_.get(index);\r\n"
    "}\r\n");
}

void RepeatedEnumFieldGenerator::
GenerateBuilderMembers(io::Printer* printer) const {
  printer->Print(variables_,
    // Note:  We return an unmodifiable list because otherwise the caller
    //   could hold on to the returned list and modify it after the message
    //   has been built, thus mutating the message which is supposed to be
    //   immutable.
    "public global::System.Collections.Generic::IList<$type$> get$capitalized_name$List() {\r\n"
    "  return java.util.Collections.unmodifiableList(result.$name$_);\r\n"
    "}\r\n"
    "public int get$capitalized_name$Count() {\r\n"
    "  return result.get$capitalized_name$Count();\r\n"
    "}\r\n"
    "public $type$ get$capitalized_name$(int index) {\r\n"
    "  return result.get$capitalized_name$(index);\r\n"
    "}\r\n"
    "public Builder set$capitalized_name$(int index, $type$ value) {\r\n"
    "  result.$name$_.set(index, value);\r\n"
    "  return this;\r\n"
    "}\r\n"
    "public Builder add$capitalized_name$($type$ value) {\r\n"
    "  if (result.$name$_.isEmpty()) {\r\n"
    "    result.$name$_ = new java.util.ArrayList<$type$>();\r\n"
    "  }\r\n"
    "  result.$name$_.add(value);\r\n"
    "  return this;\r\n"
    "}\r\n"
    "public Builder addAll$capitalized_name$<T>(\r\n"
    "    global::System.Collections.Generic.IEnumerable<T> values) where T : $type$ {\r\n"
    "  if (result.$name$_.isEmpty()) {\r\n"
    "    result.$name$_ = new java.util.ArrayList<$type$>();\r\n"
    "  }\r\n"
    "  super.addAll(values, result.$name$_);\r\n"
    "  return this;\r\n"
    "}\r\n"
    "public Builder clear$capitalized_name$() {\r\n"
    "  result.$name$_ = java.util.Collections.emptyList();\r\n"
    "  return this;\r\n"
    "}\r\n");
}

void RepeatedEnumFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (!other.$name$_.isEmpty()) {\r\n"
    "  if (result.$name$_.isEmpty()) {\r\n"
    "    result.$name$_ = new java.util.ArrayList<$type$>();\r\n"
    "  }\r\n"
    "  result.$name$_.addAll(other.$name$_);\r\n"
    "}\r\n");
}

void RepeatedEnumFieldGenerator::
GenerateBuildingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (result.$name$_ != java.util.Collections.EMPTY_LIST) {\r\n"
    "  result.$name$_ =\r\n"
    "    java.util.Collections.unmodifiableList(result.$name$_);\r\n"
    "}\r\n");
}

void RepeatedEnumFieldGenerator::
GenerateParsingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "int rawValue = input.readEnum();\r\n"
    "$type$ value = $type$.valueOf(rawValue);\r\n"
    "if (value == null) {\r\n"
    "  unknownFields.mergeVarintField($number$, rawValue);\r\n"
    "} else {\r\n"
    "  add$capitalized_name$(value);\r\n"
    "}\r\n");
}

void RepeatedEnumFieldGenerator::
GenerateSerializationCode(io::Printer* printer) const {
  printer->Print(variables_,
    "for ($type$ element : get$capitalized_name$List()) {\r\n"
    "  output.writeEnum($number$, element.getNumber());\r\n"
    "}\r\n");
}

void RepeatedEnumFieldGenerator::
GenerateSerializedSizeCode(io::Printer* printer) const {
  printer->Print(variables_,
    "for ($type$ element : get$capitalized_name$List()) {\r\n"
    "  size += pb::CodedOutputStream\r\n"
    "    .computeEnumSize($number$, element.getNumber());\r\n"
    "}\r\n");
}

string RepeatedEnumFieldGenerator::GetBoxedType() const {
  return ClassName(descriptor_->enum_type());
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
