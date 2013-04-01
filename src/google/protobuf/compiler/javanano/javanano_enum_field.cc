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

#include <google/protobuf/compiler/javanano/javanano_enum_field.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/compiler/javanano/javanano_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace javanano {

namespace {

// TODO(kenton):  Factor out a "SetCommonFieldVariables()" to get rid of
//   repeat code between this and the other field types.
void SetEnumVariables(const Params& params,
    const FieldDescriptor* descriptor, map<string, string>* variables) {
  (*variables)["name"] =
    RenameJavaKeywords(UnderscoresToCamelCase(descriptor));
  (*variables)["capitalized_name"] =
    RenameJavaKeywords(UnderscoresToCapitalizedCamelCase(descriptor));
  (*variables)["number"] = SimpleItoa(descriptor->number());
  (*variables)["type"] = "int";
  (*variables)["default"] = DefaultValue(params, descriptor);
  (*variables)["tag"] = SimpleItoa(internal::WireFormat::MakeTag(descriptor));
  (*variables)["tag_size"] = SimpleItoa(
      internal::WireFormat::TagSize(descriptor->number(), descriptor->type()));
  (*variables)["message_name"] = descriptor->containing_type()->name();
}

}  // namespace

// ===================================================================

EnumFieldGenerator::
EnumFieldGenerator(const FieldDescriptor* descriptor, const Params& params)
  : FieldGenerator(params), descriptor_(descriptor) {
  SetEnumVariables(params, descriptor, &variables_);
}

EnumFieldGenerator::~EnumFieldGenerator() {}

void EnumFieldGenerator::
GenerateMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "public int $name$ = $default$;\n");

  if (params_.generate_has()) {
    printer->Print(variables_,
      "public boolean has$capitalized_name$ = false;\n");
  }
}

void EnumFieldGenerator::
GenerateParsingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "  this.$name$ = input.readInt32();\n");

  if (params_.generate_has()) {
    printer->Print(variables_,
      "  has$capitalized_name$ = true;\n");
  }
}

void EnumFieldGenerator::
GenerateSerializationCode(io::Printer* printer) const {
  if (descriptor_->is_required()) {
    printer->Print(variables_,
      "output.writeInt32($number$, this.$name$);\n");
  } else {
    if (params_.generate_has()) {
      printer->Print(variables_,
        "if (this.$name$ != $default$ || has$capitalized_name$) {\n");
    } else {
      printer->Print(variables_,
        "if (this.$name$ != $default$) {\n");
    }
    printer->Print(variables_,
      "  output.writeInt32($number$, this.$name$);\n"
      "}\n");
  }
}

void EnumFieldGenerator::
GenerateSerializedSizeCode(io::Printer* printer) const {
  if (descriptor_->is_required()) {
    printer->Print(variables_,
      "size += com.google.protobuf.nano.CodedOutputByteBufferNano\n"
      "  .computeInt32Size($number$, this.$name$);\n");
  } else {
    if (params_.generate_has()) {
      printer->Print(variables_,
        "if (this.$name$ != $default$ || has$capitalized_name$) {\n");
    } else {
      printer->Print(variables_,
        "if (this.$name$ != $default$) {\n");
    }
    printer->Print(variables_,
      "  size += com.google.protobuf.nano.CodedOutputByteBufferNano\n"
      "    .computeInt32Size($number$, this.$name$);\n"
      "}\n");
  }
}

string EnumFieldGenerator::GetBoxedType() const {
  return ClassName(params_, descriptor_->enum_type());
}

// ===================================================================

RepeatedEnumFieldGenerator::
RepeatedEnumFieldGenerator(const FieldDescriptor* descriptor, const Params& params)
  : FieldGenerator(params), descriptor_(descriptor) {
  SetEnumVariables(params, descriptor, &variables_);
}

RepeatedEnumFieldGenerator::~RepeatedEnumFieldGenerator() {}

void RepeatedEnumFieldGenerator::
GenerateMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "public int[] $name$ = com.google.protobuf.nano.WireFormatNano.EMPTY_INT_ARRAY;\n");
  if (descriptor_->options().packed()) {
    printer->Print(variables_,
      "private int $name$MemoizedSerializedSize;\n");
  }
}

void RepeatedEnumFieldGenerator::
GenerateParsingCode(io::Printer* printer) const {
  // First, figure out the length of the array, then parse.
  if (descriptor_->options().packed()) {
    printer->Print(variables_,
      "int length = input.readRawVarint32();\n"
      "int limit = input.pushLimit(length);\n"
      "// First pass to compute array length.\n"
      "int arrayLength = 0;\n"
      "int startPos = input.getPosition();\n"
      "while (input.getBytesUntilLimit() > 0) {\n"
      "  input.readInt32();\n"
      "  arrayLength++;\n"
      "}\n"
      "input.rewindToPosition(startPos);\n"
      "this.$name$ = new $type$[arrayLength];\n"
      "for (int i = 0; i < arrayLength; i++) {\n"
      "  this.$name$[i] = input.readInt32();\n"
      "}\n"
      "input.popLimit(limit);\n");
  } else {
    printer->Print(variables_,
      "int arrayLength = com.google.protobuf.nano.WireFormatNano.getRepeatedFieldArrayLength(input, $tag$);\n"
      "int i = this.$name$.length;\n"
      "int[] newArray = new int[i + arrayLength];\n"
      "System.arraycopy(this.$name$, 0, newArray, 0, i);\n"
      "this.$name$ = newArray;\n"
      "for (; i < this.$name$.length - 1; i++) {\n"
      "  this.$name$[i] = input.readInt32();\n"
      "  input.readTag();\n"
      "}\n"
      "// Last one without readTag.\n"
      "this.$name$[i] = input.readInt32();\n");
  }
}

void RepeatedEnumFieldGenerator::
GenerateSerializationCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (this.$name$.length > 0) {\n");
  printer->Indent();

  if (descriptor_->options().packed()) {
    printer->Print(variables_,
      "output.writeRawVarint32($tag$);\n"
      "output.writeRawVarint32($name$MemoizedSerializedSize);\n"
      "for (int element : this.$name$) {\n"
      "  output.writeRawVarint32(element);\n"
      "}\n");
  } else {
    printer->Print(variables_,
      "for (int element : this.$name$) {\n"
      "  output.writeInt32($number$, element);\n"
      "}\n");
  }
  printer->Outdent();
  printer->Print(variables_,
    "}\n");
}

void RepeatedEnumFieldGenerator::
GenerateSerializedSizeCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (this.$name$.length > 0) {\n");
  printer->Indent();

  printer->Print(variables_,
    "int dataSize = 0;\n"
    "for (int element : this.$name$) {\n"
    "  dataSize += com.google.protobuf.nano.CodedOutputByteBufferNano\n"
    "    .computeInt32SizeNoTag(element);\n"
    "}\n");

  printer->Print(
    "size += dataSize;\n");
  if (descriptor_->options().packed()) {
    // cache the data size for packed fields.
    printer->Print(variables_,
      "size += $tag_size$;\n"
      "size += com.google.protobuf.nano.CodedOutputByteBufferNano\n"
      "  .computeRawVarint32Size(dataSize);\n"
      "$name$MemoizedSerializedSize = dataSize;\n");
  } else {
    printer->Print(variables_,
        "size += $tag_size$ * this.$name$.length;\n");
  }

  printer->Outdent();

  // set cached size to 0 for empty packed fields.
  if (descriptor_->options().packed()) {
    printer->Print(variables_,
      "} else {\n"
      "  $name$MemoizedSerializedSize = 0;\n"
      "}\n");
  } else {
    printer->Print(
      "}\n");
  }
}

string RepeatedEnumFieldGenerator::GetBoxedType() const {
  return ClassName(params_, descriptor_->enum_type());
}

}  // namespace javanano
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
