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
#include <google/protobuf/wire_format.h>

#include <google/protobuf/compiler/csharp/csharp_helpers.h>
#include <google/protobuf/compiler/csharp/csharp_repeated_enum_field.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

RepeatedEnumFieldGenerator::RepeatedEnumFieldGenerator(
    const FieldDescriptor* descriptor, int fieldOrdinal)
    : FieldGeneratorBase(descriptor, fieldOrdinal) {
}

RepeatedEnumFieldGenerator::~RepeatedEnumFieldGenerator() {

}

void RepeatedEnumFieldGenerator::GenerateMembers(io::Printer* printer) {
  if (descriptor_->is_packed() && optimize_speed()) {
    printer->Print(variables_, "private int $name$MemoizedSerializedSize;\n");
  }
  printer->Print(variables_,
    "private pbc::PopsicleList<$type_name$> $name$_ = new pbc::PopsicleList<$type_name$>();\n");
  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "public scg::IList<$type_name$> $property_name$List {\n"
    "  get { return pbc::Lists.AsReadOnly($name$_); }\n"
    "}\n");

  // TODO(jonskeet): Redundant API calls? Possibly - include for portability though. Maybe create an option.
  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "public int $property_name$Count {\n"
    "  get { return $name$_.Count; }\n"
    "}\n");

  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "public $type_name$ Get$property_name$(int index) {\n"
    "  return $name$_[index];\n"
    "}\n");
}

void RepeatedEnumFieldGenerator::GenerateBuilderMembers(io::Printer* printer) {
  // Note:  We can return the original list here, because we make it unmodifiable when we build
  // We return it via IPopsicleList so that collection initializers work more pleasantly.
  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "public pbc::IPopsicleList<$type_name$> $property_name$List {\n"
    "  get { return PrepareBuilder().$name$_; }\n"
    "}\n");
  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "public int $property_name$Count {\n"
    "  get { return result.$property_name$Count; }\n"
    "}\n");
  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "public $type_name$ Get$property_name$(int index) {\n"
    "  return result.Get$property_name$(index);\n"
    "}\n");
  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "public Builder Set$property_name$(int index, $type_name$ value) {\n");
  printer->Print(
    variables_,
    "  PrepareBuilder();\n"
    "  result.$name$_[index] = value;\n"
    "  return this;\n"
    "}\n");
  AddDeprecatedFlag(printer);
  printer->Print(variables_,
    "public Builder Add$property_name$($type_name$ value) {\n");
  printer->Print(
    variables_,
    "  PrepareBuilder();\n"
    "  result.$name$_.Add(value);\n"
    "  return this;\n"
    "}\n");
  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "public Builder AddRange$property_name$(scg::IEnumerable<$type_name$> values) {\n"
    "  PrepareBuilder();\n"
    "  result.$name$_.Add(values);\n"
    "  return this;\n"
    "}\n");
  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "public Builder Clear$property_name$() {\n"
    "  PrepareBuilder();\n"
    "  result.$name$_.Clear();\n"
    "  return this;\n"
    "}\n");
}

void RepeatedEnumFieldGenerator::GenerateMergingCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if (other.$name$_.Count != 0) {\n"
    "  result.$name$_.Add(other.$name$_);\n"
    "}\n");
}

void RepeatedEnumFieldGenerator::GenerateBuildingCode(io::Printer* printer) {
  printer->Print(variables_, "$name$_.MakeReadOnly();\n");
}

void RepeatedEnumFieldGenerator::GenerateParsingCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "scg::ICollection<object> unknownItems;\n"
    "input.ReadEnumArray<$type_name$>(tag, field_name, result.$name$_, out unknownItems);\n");
  if (!use_lite_runtime()) {
    printer->Print(
      variables_,
      "if (unknownItems != null) {\n"
      "  if (unknownFields == null) {\n"
      "    unknownFields = pb::UnknownFieldSet.CreateBuilder(this.UnknownFields);\n"
      "  }\n"
      "  foreach (object rawValue in unknownItems)\n"
      "    if (rawValue is int)\n"
      "      unknownFields.MergeVarintField($number$, (ulong)(int)rawValue);\n"
      "}\n");
  }
}

void RepeatedEnumFieldGenerator::GenerateSerializationCode(io::Printer* printer) {
  printer->Print(variables_, "if ($name$_.Count > 0) {\n");
  printer->Indent();
  if (descriptor_->is_packed()) {
    printer->Print(
      variables_,
      "output.WritePackedEnumArray($number$, field_names[$field_ordinal$], $name$MemoizedSerializedSize, $name$_);\n");
  } else {
    printer->Print(variables_,
      "output.WriteEnumArray($number$, field_names[$field_ordinal$], $name$_);\n");
  }
  printer->Outdent();
  printer->Print("}\n");
}

void RepeatedEnumFieldGenerator::GenerateSerializedSizeCode(io::Printer* printer) {
  printer->Print("{\n");
  printer->Indent();
  printer->Print(
    variables_,
    "int dataSize = 0;\n"
    "if ($name$_.Count > 0) {\n");
  printer->Indent();
  printer->Print(
    variables_,
    "foreach ($type_name$ element in $name$_) {\n"
    "  dataSize += pb::CodedOutputStream.ComputeEnumSizeNoTag((int) element);\n"
    "}\n"
    "size += dataSize;\n");
  int tagSize = internal::WireFormat::TagSize(descriptor_->number(), descriptor_->type());
  if (descriptor_->is_packed()) {
    printer->Print(
      "size += $tag_size$;\n"
      "size += pb::CodedOutputStream.ComputeRawVarint32Size((uint) dataSize);\n",
      "tag_size", SimpleItoa(tagSize));
  } else {
    printer->Print(
      "size += $tag_size$ * $name$_.Count;\n",
      "tag_size", SimpleItoa(tagSize), "name", name());
  }
  printer->Outdent();
  printer->Print("}\n");
  // cache the data size for packed fields.
  if (descriptor_->is_packed()) {
    printer->Print(variables_,
      "$name$MemoizedSerializedSize = dataSize;\n");
  }
  printer->Outdent();
  printer->Print("}\n");
}

void RepeatedEnumFieldGenerator::WriteHash(io::Printer* printer) {
  printer->Print(
    variables_,
    "foreach($type_name$ i in $name$_)\n"
    "  hash ^= i.GetHashCode();\n");
}

void RepeatedEnumFieldGenerator::WriteEquals(io::Printer* printer) {
  printer->Print(
    variables_,
    "if($name$_.Count != other.$name$_.Count) return false;\n"
    "for(int ix=0; ix < $name$_.Count; ix++)\n"
    "  if(!$name$_[ix].Equals(other.$name$_[ix])) return false;\n");
}

void RepeatedEnumFieldGenerator::WriteToString(io::Printer* printer) {
  printer->Print(variables_,
    "PrintField(\"$descriptor_name$\", $name$_, writer);\n");
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
