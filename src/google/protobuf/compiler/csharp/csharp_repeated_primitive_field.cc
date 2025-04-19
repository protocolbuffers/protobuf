// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/csharp/csharp_repeated_primitive_field.h"

#include <sstream>

#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/wire_format.h"
#include "google/protobuf/compiler/csharp/csharp_doc_comment.h"
#include "google/protobuf/compiler/csharp/csharp_helpers.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/zero_copy_stream.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

RepeatedPrimitiveFieldGenerator::RepeatedPrimitiveFieldGenerator(
    const FieldDescriptor* descriptor, int presenceIndex, const Options *options)
    : FieldGeneratorBase(descriptor, presenceIndex, options) {}

RepeatedPrimitiveFieldGenerator::~RepeatedPrimitiveFieldGenerator() {

}

void RepeatedPrimitiveFieldGenerator::GenerateMembers(io::Printer* printer) {
  printer->Print(
    variables_,
    "private static readonly pb::FieldCodec<$value_type_name$> _repeated_$name$_codec\n"
    "    = pb::FieldCodec.For$capitalized_type_name$($tag$);\n");

  if (options()->emit_unity_attribs) {
    printer->Print("[UnityEngine.SerializeField]\n");
  }
  
  if (options()->use_properties) {
    printer->Print(variables_,
      "private readonly pbc::RepeatedField<$value_type_name$> $cs_field_name$ = new pbc::RepeatedField<$value_type_name$>();\n");
    WritePropertyDocComment(printer, options(), descriptor_);
    AddPublicMemberAttributes(printer);
    printer->Print(
      variables_,
      "$access_level$ pbc::RepeatedField<$value_type_name$> $property_name$ {\n"
      "  get { return $cs_field_name$; }\n"
      "}\n");
  } else {
    printer->Print(variables_,
      "public readonly pbc::RepeatedField<$value_type_name$> $cs_field_name$ = new pbc::RepeatedField<$value_type_name$>();\n");
  }
}

void RepeatedPrimitiveFieldGenerator::GenerateMergingCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "$cs_field_name$.Add(other.$cs_field_name$);\n");
}

void RepeatedPrimitiveFieldGenerator::GenerateParsingCode(io::Printer* printer) {
  GenerateParsingCode(printer, true);
}

void RepeatedPrimitiveFieldGenerator::GenerateParsingCode(io::Printer* printer, bool use_parse_context) {
  printer->Print(
    variables_,
    use_parse_context
    ? "$cs_field_name$.AddEntriesFrom(ref input, _repeated_$name$_codec);\n"
    : "$cs_field_name$.AddEntriesFrom(input, _repeated_$name$_codec);\n");
}

void RepeatedPrimitiveFieldGenerator::GenerateSerializationCode(io::Printer* printer) {
  GenerateSerializationCode(printer, true);
}

void RepeatedPrimitiveFieldGenerator::GenerateSerializationCode(io::Printer* printer, bool use_write_context) {
  printer->Print(
    variables_,
    use_write_context
    ? "$cs_field_name$.WriteTo(ref output, _repeated_$name$_codec);\n"
    : "$cs_field_name$.WriteTo(output, _repeated_$name$_codec);\n");
}

void RepeatedPrimitiveFieldGenerator::GenerateSerializedSizeCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "size += $cs_field_name$.CalculateSize(_repeated_$name$_codec);\n");
}

void RepeatedPrimitiveFieldGenerator::WriteHash(io::Printer* printer) {
  printer->Print(
    variables_,
    "hash = 17 * hash + $cs_field_name$.GetHashCode();\n");
}

void RepeatedPrimitiveFieldGenerator::WriteEquals(io::Printer* printer) {
  printer->Print(
    variables_,
    "if (!$cs_field_name$.Equals(other.$cs_field_name$)) return false;\n");
}
void RepeatedPrimitiveFieldGenerator::WriteToString(io::Printer* printer) {
  printer->Print(variables_,
    "PrintField(\"$descriptor_name$\", $cs_field_name$, writer);\n");
}

void RepeatedPrimitiveFieldGenerator::GenerateCloningCode(io::Printer* printer) {
  printer->Print(variables_,
    "$cs_field_name$ = other.$cs_field_name$.Clone();\n");
}

void RepeatedPrimitiveFieldGenerator::GenerateFreezingCode(io::Printer* printer) {
}

void RepeatedPrimitiveFieldGenerator::GenerateExtensionCode(io::Printer* printer) {
  WritePropertyDocComment(printer, options(), descriptor_);
  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "$access_level$ static readonly pb::RepeatedExtension<$extended_type$, $value_type_name$> $property_name$ =\n"
    "  new pb::RepeatedExtension<$extended_type$, $value_type_name$>($number$, pb::FieldCodec.For$capitalized_type_name$($tag$));\n");
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
