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

#include <google/protobuf/compiler/javanano/javanano_message_field.h>
#include <google/protobuf/compiler/javanano/javanano_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace javanano {

using internal::WireFormat;
using internal::WireFormatLite;

namespace {

// TODO(kenton):  Factor out a "SetCommonFieldVariables()" to get rid of
//   repeat code between this and the other field types.
void SetMessageVariables(const Params& params,
    const FieldDescriptor* descriptor, map<string, string>* variables) {
  (*variables)["name"] =
    RenameJavaKeywords(UnderscoresToCamelCase(descriptor));
  (*variables)["capitalized_name"] =
    RenameJavaKeywords(UnderscoresToCapitalizedCamelCase(descriptor));
  (*variables)["number"] = SimpleItoa(descriptor->number());
  (*variables)["type"] = ClassName(params, descriptor->message_type());
  (*variables)["group_or_message"] =
    (descriptor->type() == FieldDescriptor::TYPE_GROUP) ?
    "Group" : "Message";
  (*variables)["message_name"] = descriptor->containing_type()->name();
  //(*variables)["message_type"] = descriptor->message_type()->name();
  (*variables)["tag"] = SimpleItoa(WireFormat::MakeTag(descriptor));
}

}  // namespace

// ===================================================================

MessageFieldGenerator::
MessageFieldGenerator(const FieldDescriptor* descriptor, const Params& params)
  : FieldGenerator(params), descriptor_(descriptor) {
  SetMessageVariables(params, descriptor, &variables_);
}

MessageFieldGenerator::~MessageFieldGenerator() {}

void MessageFieldGenerator::
GenerateMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "public $type$ $name$;\n");
}

void MessageFieldGenerator::
GenerateClearCode(io::Printer* printer) const {
  printer->Print(variables_,
    "$name$ = null;\n");
}

void MessageFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (this.$name$ == null) {\n"
    "  this.$name$ = new $type$();\n"
    "}");

  if (descriptor_->type() == FieldDescriptor::TYPE_GROUP) {
    printer->Print(variables_,
      "input.readGroup(this.$name$, $number$);\n");
  } else {
    printer->Print(variables_,
      "input.readMessage(this.$name$);\n");
  }
}

void MessageFieldGenerator::
GenerateSerializationCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (this.$name$ != null) {\n"
    "  output.write$group_or_message$($number$, this.$name$);\n"
    "}\n");
}

void MessageFieldGenerator::
GenerateSerializedSizeCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (this.$name$ != null) {\n"
    "  size += com.google.protobuf.nano.CodedOutputByteBufferNano\n"
    "    .compute$group_or_message$Size($number$, this.$name$);\n"
    "}\n");
}

string MessageFieldGenerator::GetBoxedType() const {
  return ClassName(params_, descriptor_->message_type());
}

// ===================================================================

AccessorMessageFieldGenerator::
AccessorMessageFieldGenerator(
    const FieldDescriptor* descriptor, const Params& params)
  : FieldGenerator(params), descriptor_(descriptor) {
  SetMessageVariables(params, descriptor, &variables_);
}

AccessorMessageFieldGenerator::~AccessorMessageFieldGenerator() {}

void AccessorMessageFieldGenerator::
GenerateMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "private $type$ $name$_;\n"
    "public $type$ get$capitalized_name$() {\n"
    "  return $name$_;\n"
    "}\n"
    "public void set$capitalized_name$($type$ value) {\n"
    "  if (value == null) {\n"
    "    throw new java.lang.NullPointerException();\n"
    "  }\n"
    "  $name$_ = value;\n"
    "}\n"
    "public boolean has$capitalized_name$() {\n"
    "  return $name$_ != null;\n"
    "}\n"
    "public void clear$capitalized_name$() {\n"
    "  $name$_ = null;\n"
    "}\n");
}

void AccessorMessageFieldGenerator::
GenerateClearCode(io::Printer* printer) const {
  printer->Print(variables_,
    "$name$_ = null;\n");
}

void AccessorMessageFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (!has$capitalized_name$()) {\n"
    "  set$capitalized_name$(new $type$());\n"
    "}");

  if (descriptor_->type() == FieldDescriptor::TYPE_GROUP) {
    printer->Print(variables_,
      "input.readGroup($name$_, $number$);\n");
  } else {
    printer->Print(variables_,
      "input.readMessage($name$_);\n");
  }
}

void AccessorMessageFieldGenerator::
GenerateSerializationCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (has$capitalized_name$()) {\n"
    "  output.write$group_or_message$($number$, $name$_);\n"
    "}\n");
}

void AccessorMessageFieldGenerator::
GenerateSerializedSizeCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (has$capitalized_name$()) {\n"
    "  size += com.google.protobuf.nano.CodedOutputByteBufferNano\n"
    "    .compute$group_or_message$Size($number$, $name$_);\n"
    "}\n");
}

string AccessorMessageFieldGenerator::GetBoxedType() const {
  return ClassName(params_, descriptor_->message_type());
}

// ===================================================================

RepeatedMessageFieldGenerator::
RepeatedMessageFieldGenerator(const FieldDescriptor* descriptor, const Params& params)
  : FieldGenerator(params), descriptor_(descriptor) {
  SetMessageVariables(params, descriptor, &variables_);
}

RepeatedMessageFieldGenerator::~RepeatedMessageFieldGenerator() {}

void RepeatedMessageFieldGenerator::
GenerateMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "public $type$[] $name$;\n");
}

void RepeatedMessageFieldGenerator::
GenerateClearCode(io::Printer* printer) const {
  printer->Print(variables_,
    "$name$ = $type$.EMPTY_ARRAY;\n");
}

void RepeatedMessageFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  // First, figure out the length of the array, then parse.
  printer->Print(variables_,
    "int arrayLength = com.google.protobuf.nano.WireFormatNano"
    "    .getRepeatedFieldArrayLength(input, $tag$);\n"
    "int i = this.$name$ == null ? 0 : this.$name$.length;\n"
    "$type$[] newArray = new $type$[i + arrayLength];\n"
    "if (this.$name$ != null) {\n"
    "  System.arraycopy(this.$name$, 0, newArray, 0, i);\n"
    "}\n"
    "this.$name$ = newArray;\n"
    "for (; i < this.$name$.length - 1; i++) {\n"
    "  this.$name$[i] = new $type$();\n");

  if (descriptor_->type() == FieldDescriptor::TYPE_GROUP) {
    printer->Print(variables_,
      "  input.readGroup(this.$name$[i], $number$);\n");
  } else {
    printer->Print(variables_,
      "  input.readMessage(this.$name$[i]);\n");
  }

  printer->Print(variables_,
    "  input.readTag();\n"
    "}\n"
    "// Last one without readTag.\n"
    "this.$name$[i] = new $type$();\n");

  if (descriptor_->type() == FieldDescriptor::TYPE_GROUP) {
    printer->Print(variables_,
      "input.readGroup(this.$name$[i], $number$);\n");
  } else {
    printer->Print(variables_,
      "input.readMessage(this.$name$[i]);\n");
  }
}

void RepeatedMessageFieldGenerator::
GenerateSerializationCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (this.$name$ != null) {\n"
    "  for ($type$ element : this.$name$) {\n"
    "    output.write$group_or_message$($number$, element);\n"
    "  }\n"
    "}\n");
}

void RepeatedMessageFieldGenerator::
GenerateSerializedSizeCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (this.$name$ != null) {\n"
    "  for ($type$ element : this.$name$) {\n"
    "    size += com.google.protobuf.nano.CodedOutputByteBufferNano\n"
    "      .compute$group_or_message$Size($number$, element);\n"
    "  }\n"
    "}\n");
}

string RepeatedMessageFieldGenerator::GetBoxedType() const {
  return ClassName(params_, descriptor_->message_type());
}

}  // namespace javanano
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
