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
GenerateSwappingCode(io::Printer* printer) const {
  printer->Print(variables_, "std::swap($name$_, other->$name$_);\n");
}

void EnumFieldGenerator::
GenerateConstructorCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$_ = $default$;\n");
}

void EnumFieldGenerator::
GenerateMergeFromCodedStream(io::Printer* printer) const {
  printer->Print(variables_,
    "int value;\n"
    "DO_(::google::protobuf::internal::WireFormat::ReadEnum(input, &value));\n"
    "if ($type$_IsValid(value)) {\n"
    "  set_$name$(static_cast< $type$ >(value));\n"
    "} else {\n"
    "  mutable_unknown_fields()->AddVarint($number$, value);\n"
    "}\n");
}

void EnumFieldGenerator::
GenerateSerializeWithCachedSizes(io::Printer* printer) const {
  printer->Print(variables_,
    "::google::protobuf::internal::WireFormat::WriteEnum("
      "$number$, this->$name$(), output);\n");
}

void EnumFieldGenerator::
GenerateSerializeWithCachedSizesToArray(io::Printer* printer) const {
  printer->Print(variables_,
    "target = ::google::protobuf::internal::WireFormat::WriteEnumToArray("
      "$number$, this->$name$(), target);\n");
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
  printer->Print(variables_,
    "::google::protobuf::RepeatedField<int> $name$_;\n");
  if (descriptor_->options().packed() &&
      descriptor_->file()->options().optimize_for() == FileOptions::SPEED) {
    printer->Print(variables_,
      "mutable int _$name$_cached_byte_size_;\n");
  }
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
GenerateSwappingCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$_.Swap(&other->$name$_);\n");
}

void RepeatedEnumFieldGenerator::
GenerateConstructorCode(io::Printer* printer) const {
  // Not needed for repeated fields.
}

void RepeatedEnumFieldGenerator::
GenerateMergeFromCodedStream(io::Printer* printer) const {
  if (descriptor_->options().packed()) {
    printer->Print(variables_,
      "::google::protobuf::uint32 length;\n"
      "DO_(input->ReadVarint32(&length));\n"
      "::google::protobuf::io::CodedInputStream::Limit limit = "
          "input->PushLimit(length);\n"
      "while (input->BytesUntilLimit() > 0) {\n"
      "  int value;\n"
      "  DO_(::google::protobuf::internal::WireFormat::ReadEnum(input, &value));\n"
      "  if ($type$_IsValid(value)) {\n"
      "    add_$name$(static_cast< $type$ >(value));\n"
      "  }\n"
      "}\n"
      "input->PopLimit(limit);\n");
  } else {
    printer->Print(variables_,
      "int value;\n"
      "DO_(::google::protobuf::internal::WireFormat::ReadEnum(input, &value));\n"
      "if ($type$_IsValid(value)) {\n"
      "  add_$name$(static_cast< $type$ >(value));\n"
      "} else {\n"
      "  mutable_unknown_fields()->AddVarint($number$, value);\n"
      "}\n");
  }
}

void RepeatedEnumFieldGenerator::
GenerateSerializeWithCachedSizes(io::Printer* printer) const {
  if (descriptor_->options().packed()) {
    // Write the tag and the size.
    printer->Print(variables_,
      "if (this->$name$_size() > 0) {\n"
      "  ::google::protobuf::internal::WireFormat::WriteTag("
          "$number$, "
          "::google::protobuf::internal::WireFormat::WIRETYPE_LENGTH_DELIMITED, "
          "output);\n"
      "  output->WriteVarint32(_$name$_cached_byte_size_);\n"
      "}\n");
  }
  printer->Print(variables_,
      "for (int i = 0; i < this->$name$_size(); i++) {\n");
  if (descriptor_->options().packed()) {
    printer->Print(variables_,
      "  ::google::protobuf::internal::WireFormat::WriteEnumNoTag("
          "this->$name$(i), output);\n");
  } else {
    printer->Print(variables_,
      "  ::google::protobuf::internal::WireFormat::WriteEnum("
          "$number$, this->$name$(i), output);\n");
  }
  printer->Print("}\n");
}

void RepeatedEnumFieldGenerator::
GenerateSerializeWithCachedSizesToArray(io::Printer* printer) const {
  if (descriptor_->options().packed()) {
    // Write the tag and the size.
    printer->Print(variables_,
      "if (this->$name$_size() > 0) {\n"
      "  target = ::google::protobuf::internal::WireFormat::WriteTagToArray("
          "$number$, "
          "::google::protobuf::internal::WireFormat::WIRETYPE_LENGTH_DELIMITED, "
          "target);\n"
      "  target = ::google::protobuf::io::CodedOutputStream::WriteVarint32ToArray("
          "_$name$_cached_byte_size_, target);\n"
      "}\n");
  }
  printer->Print(variables_,
      "for (int i = 0; i < this->$name$_size(); i++) {\n");
  if (descriptor_->options().packed()) {
    printer->Print(variables_,
      "  target = ::google::protobuf::internal::WireFormat::WriteEnumNoTagToArray("
          "this->$name$(i), target);\n");
  } else {
    printer->Print(variables_,
      "  target = ::google::protobuf::internal::WireFormat::WriteEnumToArray("
          "$number$, this->$name$(i), target);\n");
  }
  printer->Print("}\n");
}

void RepeatedEnumFieldGenerator::
GenerateByteSize(io::Printer* printer) const {
  printer->Print(variables_,
    "{\n"
    "  int data_size = 0;\n");
  printer->Indent();
  printer->Print(variables_,
      "for (int i = 0; i < this->$name$_size(); i++) {\n"
      "  data_size += ::google::protobuf::internal::WireFormat::EnumSize(\n"
      "    this->$name$(i));\n"
      "}\n");

  if (descriptor_->options().packed()) {
    printer->Print(variables_,
      "if (data_size > 0) {\n"
      "  total_size += $tag_size$ + "
        "::google::protobuf::internal::WireFormat::Int32Size(data_size);\n"
      "}\n"
      "_$name$_cached_byte_size_ = data_size;\n"
      "total_size += data_size;\n");
  } else {
    printer->Print(variables_,
      "total_size += $tag_size$ * this->$name$_size() + data_size;\n");
  }
  printer->Outdent();
  printer->Print("}\n");
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
