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

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <google/protobuf/compiler/cpp/cpp_enum_field.h>
#include <google/protobuf/compiler/cpp/cpp_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/wire_format_inl.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

using internal::WireFormat;

namespace {

// TODO(kenton):  Factor out a "SetCommonFieldVariables()" to get rid of
//   repeat code between this and the other field types.
void SetEnumVariables(const FieldDescriptor* descriptor,
                      map<string, string>* variables) {
  const EnumValueDescriptor* default_value = descriptor->default_value_enum();

  (*variables)["name"] = FieldName(descriptor);
  (*variables)["type"] = ClassName(descriptor->enum_type(), true);
  (*variables)["default"] = SimpleItoa(default_value->number());
  (*variables)["index"] = SimpleItoa(descriptor->index());
  (*variables)["number"] = SimpleItoa(descriptor->number());
  (*variables)["classname"] = ClassName(FieldScope(descriptor), false);
  (*variables)["tag_size"] = SimpleItoa(
    WireFormat::TagSize(descriptor->number(), descriptor->type()));
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
GeneratePrivateMembers(io::Printer* printer) const {
  printer->Print(variables_, "int $name$_;\n");
}

void EnumFieldGenerator::
GenerateAccessorDeclarations(io::Printer* printer) const {
  printer->Print(variables_,
    "inline $type$ $name$() const;\n"
    "inline void set_$name$($type$ value);\n");
}

void EnumFieldGenerator::
GenerateInlineAccessorDefinitions(io::Printer* printer) const {
  printer->Print(variables_,
    "inline $type$ $classname$::$name$() const {\n"
    "  return static_cast< $type$ >($name$_);\n"
    "}\n"
    "inline void $classname$::set_$name$($type$ value) {\n"
    "  GOOGLE_DCHECK($type$_IsValid(value));\n"
    "  _set_bit($index$);\n"
    "  $name$_ = value;\n"
    "}\n");
}

void EnumFieldGenerator::
GenerateClearingCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$_ = $default$;\n");
}

void EnumFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_, "set_$name$(from.$name$());\n");
}

void EnumFieldGenerator::
GenerateInitializer(io::Printer* printer) const {
  printer->Print(variables_, ",\n$name$_($default$)");
}

void EnumFieldGenerator::
GenerateMergeFromCodedStream(io::Printer* printer) const {
  printer->Print(variables_,
    "int value;\n"
    "DO_(::google::protobuf::internal::WireFormat::ReadEnum(input, &value));\n"
    "if ($type$_IsValid(value)) {\n"
    "  set_$name$(static_cast< $type$ >(value));\n"
    "} else {\n"
    "  mutable_unknown_fields()->AddField($number$)->add_varint(value);\n"
    "}\n");
}

void EnumFieldGenerator::
GenerateSerializeWithCachedSizes(io::Printer* printer) const {
  printer->Print(variables_,
    "DO_(::google::protobuf::internal::WireFormat::WriteEnum("
      "$number$, this->$name$(), output));\n");
}

void EnumFieldGenerator::
GenerateByteSize(io::Printer* printer) const {
  printer->Print(variables_,
    "total_size += $tag_size$ +\n"
    "  ::google::protobuf::internal::WireFormat::EnumSize(this->$name$());\n");
}

// ===================================================================

RepeatedEnumFieldGenerator::
RepeatedEnumFieldGenerator(const FieldDescriptor* descriptor)
  : descriptor_(descriptor) {
  SetEnumVariables(descriptor, &variables_);
}

RepeatedEnumFieldGenerator::~RepeatedEnumFieldGenerator() {}

void RepeatedEnumFieldGenerator::
GeneratePrivateMembers(io::Printer* printer) const {
  printer->Print(variables_, "::google::protobuf::RepeatedField<int> $name$_;\n");
}

void RepeatedEnumFieldGenerator::
GenerateAccessorDeclarations(io::Printer* printer) const {
  printer->Print(variables_,
    "inline const ::google::protobuf::RepeatedField<int>& $name$() const;\n"
    "inline ::google::protobuf::RepeatedField<int>* mutable_$name$();\n"
    "inline $type$ $name$(int index) const;\n"
    "inline void set_$name$(int index, $type$ value);\n"
    "inline void add_$name$($type$ value);\n");
}

void RepeatedEnumFieldGenerator::
GenerateInlineAccessorDefinitions(io::Printer* printer) const {
  printer->Print(variables_,
    "inline const ::google::protobuf::RepeatedField<int>&\n"
    "$classname$::$name$() const {\n"
    "  return $name$_;\n"
    "}\n"
    "inline ::google::protobuf::RepeatedField<int>*\n"
    "$classname$::mutable_$name$() {\n"
    "  return &$name$_;\n"
    "}\n"
    "inline $type$ $classname$::$name$(int index) const {\n"
    "  return static_cast< $type$ >($name$_.Get(index));\n"
    "}\n"
    "inline void $classname$::set_$name$(int index, $type$ value) {\n"
    "  GOOGLE_DCHECK($type$_IsValid(value));\n"
    "  $name$_.Set(index, value);\n"
    "}\n"
    "inline void $classname$::add_$name$($type$ value) {\n"
    "  GOOGLE_DCHECK($type$_IsValid(value));\n"
    "  $name$_.Add(value);\n"
    "}\n");
}

void RepeatedEnumFieldGenerator::
GenerateClearingCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$_.Clear();\n");
}

void RepeatedEnumFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$_.MergeFrom(from.$name$_);\n");
}

void RepeatedEnumFieldGenerator::
GenerateInitializer(io::Printer* printer) const {
  // Not needed for repeated fields.
}

void RepeatedEnumFieldGenerator::
GenerateMergeFromCodedStream(io::Printer* printer) const {
  printer->Print(variables_,
    "int value;\n"
    "DO_(::google::protobuf::internal::WireFormat::ReadEnum(input, &value));\n"
    "if ($type$_IsValid(value)) {\n"
    "  add_$name$(static_cast< $type$ >(value));\n"
    "} else {\n"
    "  mutable_unknown_fields()->AddField($number$)->add_varint(value);\n"
    "}\n");
}

void RepeatedEnumFieldGenerator::
GenerateSerializeWithCachedSizes(io::Printer* printer) const {
  printer->Print(variables_,
    "DO_(::google::protobuf::internal::WireFormat::WriteEnum("
      "$number$, this->$name$(i), output));\n");
}

void RepeatedEnumFieldGenerator::
GenerateByteSize(io::Printer* printer) const {
  printer->Print(variables_,
    "total_size += $tag_size$ * $name$_size();\n"
    "for (int i = 0; i < $name$_size(); i++) {\n"
    "  total_size += ::google::protobuf::internal::WireFormat::EnumSize(\n"
    "    this->$name$(i));\n"
    "}\n");
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
