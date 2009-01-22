// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

#include <map>
#include <string>

#include <google/protobuf/compiler/java/java_enum_field.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/compiler/java/java_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/wire_format_inl.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

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
  (*variables)["tag"] = SimpleItoa(internal::WireFormat::MakeTag(descriptor));
  (*variables)["tag_size"] = SimpleItoa(
      internal::WireFormat::TagSize(descriptor->number(), descriptor->type()));
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
    "private boolean has$capitalized_name$;\n"
    "private $type$ $name$_ = $default$;\n"
    "public boolean has$capitalized_name$() { return has$capitalized_name$; }\n"
    "public $type$ get$capitalized_name$() { return $name$_; }\n");
}

void EnumFieldGenerator::
GenerateBuilderMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "public boolean has$capitalized_name$() {\n"
    "  return result.has$capitalized_name$();\n"
    "}\n"
    "public $type$ get$capitalized_name$() {\n"
    "  return result.get$capitalized_name$();\n"
    "}\n"
    "public Builder set$capitalized_name$($type$ value) {\n"
    "  if (value == null) {\n"
    "    throw new NullPointerException();\n"
    "  }\n"
    "  result.has$capitalized_name$ = true;\n"
    "  result.$name$_ = value;\n"
    "  return this;\n"
    "}\n"
    "public Builder clear$capitalized_name$() {\n"
    "  result.has$capitalized_name$ = false;\n"
    "  result.$name$_ = $default$;\n"
    "  return this;\n"
    "}\n");
}

void EnumFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (other.has$capitalized_name$()) {\n"
    "  set$capitalized_name$(other.get$capitalized_name$());\n"
    "}\n");
}

void EnumFieldGenerator::
GenerateBuildingCode(io::Printer* printer) const {
  // Nothing to do here for enum types.
}

void EnumFieldGenerator::
GenerateParsingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "int rawValue = input.readEnum();\n"
    "$type$ value = $type$.valueOf(rawValue);\n"
    "if (value == null) {\n"
    "  unknownFields.mergeVarintField($number$, rawValue);\n"
    "} else {\n"
    "  set$capitalized_name$(value);\n"
    "}\n");
}

void EnumFieldGenerator::
GenerateSerializationCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (has$capitalized_name$()) {\n"
    "  output.writeEnum($number$, get$capitalized_name$().getNumber());\n"
    "}\n");
}

void EnumFieldGenerator::
GenerateSerializedSizeCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (has$capitalized_name$()) {\n"
    "  size += com.google.protobuf.CodedOutputStream\n"
    "    .computeEnumSize($number$, get$capitalized_name$().getNumber());\n"
    "}\n");
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
    "private java.util.List<$type$> $name$_ =\n"
    "  java.util.Collections.emptyList();\n"
    "public java.util.List<$type$> get$capitalized_name$List() {\n"
    "  return $name$_;\n"   // note:  unmodifiable list
    "}\n"
    "public int get$capitalized_name$Count() { return $name$_.size(); }\n"
    "public $type$ get$capitalized_name$(int index) {\n"
    "  return $name$_.get(index);\n"
    "}\n");

  if (descriptor_->options().packed() &&
      descriptor_->file()->options().optimize_for() == FileOptions::SPEED) {
    printer->Print(variables_,
      "private int $name$MemoizedSerializedSize;\n");
  }
}

void RepeatedEnumFieldGenerator::
GenerateBuilderMembers(io::Printer* printer) const {
  printer->Print(variables_,
    // Note:  We return an unmodifiable list because otherwise the caller
    //   could hold on to the returned list and modify it after the message
    //   has been built, thus mutating the message which is supposed to be
    //   immutable.
    "public java.util.List<$type$> get$capitalized_name$List() {\n"
    "  return java.util.Collections.unmodifiableList(result.$name$_);\n"
    "}\n"
    "public int get$capitalized_name$Count() {\n"
    "  return result.get$capitalized_name$Count();\n"
    "}\n"
    "public $type$ get$capitalized_name$(int index) {\n"
    "  return result.get$capitalized_name$(index);\n"
    "}\n"
    "public Builder set$capitalized_name$(int index, $type$ value) {\n"
    "  if (value == null) {\n"
    "    throw new NullPointerException();\n"
    "  }\n"
    "  result.$name$_.set(index, value);\n"
    "  return this;\n"
    "}\n"
    "public Builder add$capitalized_name$($type$ value) {\n"
    "  if (value == null) {\n"
    "    throw new NullPointerException();\n"
    "  }\n"
    "  if (result.$name$_.isEmpty()) {\n"
    "    result.$name$_ = new java.util.ArrayList<$type$>();\n"
    "  }\n"
    "  result.$name$_.add(value);\n"
    "  return this;\n"
    "}\n"
    "public Builder addAll$capitalized_name$(\n"
    "    java.lang.Iterable<? extends $type$> values) {\n"
    "  if (result.$name$_.isEmpty()) {\n"
    "    result.$name$_ = new java.util.ArrayList<$type$>();\n"
    "  }\n"
    "  super.addAll(values, result.$name$_);\n"
    "  return this;\n"
    "}\n"
    "public Builder clear$capitalized_name$() {\n"
    "  result.$name$_ = java.util.Collections.emptyList();\n"
    "  return this;\n"
    "}\n");
}

void RepeatedEnumFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (!other.$name$_.isEmpty()) {\n"
    "  if (result.$name$_.isEmpty()) {\n"
    "    result.$name$_ = new java.util.ArrayList<$type$>();\n"
    "  }\n"
    "  result.$name$_.addAll(other.$name$_);\n"
    "}\n");
}

void RepeatedEnumFieldGenerator::
GenerateBuildingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (result.$name$_ != java.util.Collections.EMPTY_LIST) {\n"
    "  result.$name$_ =\n"
    "    java.util.Collections.unmodifiableList(result.$name$_);\n"
    "}\n");
}

void RepeatedEnumFieldGenerator::
GenerateParsingCode(io::Printer* printer) const {
  // If packed, set up the while loop
  if (descriptor_->options().packed()) {
    printer->Print(variables_,
      "int length = input.readRawVarint32();\n"
      "int oldLimit = input.pushLimit(length);\n"
      "while(input.getBytesUntilLimit() > 0) {\n");
    printer->Indent();
  }

  // Read and store the enum
  printer->Print(variables_,
    "int rawValue = input.readEnum();\n"
    "$type$ value = $type$.valueOf(rawValue);\n"
    "if (value == null) {\n"
    "  unknownFields.mergeVarintField($number$, rawValue);\n"
    "} else {\n"
    "  add$capitalized_name$(value);\n"
    "}\n");

  if (descriptor_->options().packed()) {
    printer->Outdent();
    printer->Print(variables_,
      "}\n"
      "input.popLimit(oldLimit);\n");
  }
}

void RepeatedEnumFieldGenerator::
GenerateSerializationCode(io::Printer* printer) const {
  if (descriptor_->options().packed()) {
    printer->Print(variables_,
      "if (get$capitalized_name$List().size() > 0) {\n"
      "  output.writeRawVarint32($tag$);\n"
      "  output.writeRawVarint32($name$MemoizedSerializedSize);\n"
      "}\n"
      "for ($type$ element : get$capitalized_name$List()) {\n"
      "  output.writeEnumNoTag(element.getNumber());\n"
      "}\n");
  } else {
    printer->Print(variables_,
      "for ($type$ element : get$capitalized_name$List()) {\n"
      "  output.writeEnum($number$, element.getNumber());\n"
      "}\n");
  }
}

void RepeatedEnumFieldGenerator::
GenerateSerializedSizeCode(io::Printer* printer) const {
  printer->Print(variables_,
    "{\n"
    "  int dataSize = 0;\n");
  printer->Indent();

  printer->Print(variables_,
    "for ($type$ element : get$capitalized_name$List()) {\n"
    "  dataSize += com.google.protobuf.CodedOutputStream\n"
    "    .computeEnumSizeNoTag(element.getNumber());\n"
    "}\n");
  printer->Print(
    "size += dataSize;\n");
  if (descriptor_->options().packed()) {
    printer->Print(variables_,
      "if (!get$capitalized_name$List().isEmpty()) {"
      "  size += $tag_size$;\n"
      "  size += com.google.protobuf.CodedOutputStream\n"
      "    .computeRawVarint32Size(dataSize);\n"
      "}");
  } else {
    printer->Print(variables_,
      "size += $tag_size$ * get$capitalized_name$List().size();\n");
  }

  // cache the data size for packed fields.
  if (descriptor_->options().packed()) {
    printer->Print(variables_,
      "$name$MemoizedSerializedSize = dataSize;\n");
  }

  printer->Outdent();
  printer->Print("}\n");
}

string RepeatedEnumFieldGenerator::GetBoxedType() const {
  return ClassName(descriptor_->enum_type());
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
