// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/csharp/csharp_enum_field.h"

#include <sstream>

#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/compiler/csharp/csharp_doc_comment.h"
#include "google/protobuf/compiler/csharp/csharp_helpers.h"
#include "google/protobuf/compiler/csharp/csharp_options.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/zero_copy_stream.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

EnumFieldGenerator::EnumFieldGenerator(const FieldDescriptor* descriptor,
                                       int presenceIndex, const Options *options)
    : PrimitiveFieldGenerator(descriptor, presenceIndex, options) {
}

EnumFieldGenerator::~EnumFieldGenerator() {
}

void EnumFieldGenerator::GenerateParsingCode(io::Printer* printer) {
  printer->Print(variables_,
    "$property_name$ = ($type_name$) input.ReadEnum();\n");
}

void EnumFieldGenerator::GenerateSerializationCode(io::Printer* printer) {
  printer->Print(variables_,
    "if ($has_property_check$) {\n"
    "  output.WriteRawTag($tag_bytes$);\n"
    "  output.WriteEnum((int) $property_name$);\n"
    "}\n");
}

void EnumFieldGenerator::GenerateSerializedSizeCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if ($has_property_check$) {\n"
      "  size += $tag_size$ + pb::CodedOutputStream.ComputeEnumSize((int) $property_name$);\n"
    "}\n");
}

void EnumFieldGenerator::GenerateCodecCode(io::Printer* printer) {
  printer->Print(
      variables_,
      "pb::FieldCodec.ForEnum($tag$, x => (int) x, x => ($type_name$) x, $default_value$)");
}

void EnumFieldGenerator::GenerateExtensionCode(io::Printer* printer) {
  WritePropertyDocComment(printer, descriptor_);
  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "$access_level$ static readonly pb::Extension<$extended_type$, $type_name$> $property_name$ =\n"
    "  new pb::Extension<$extended_type$, $type_name$>($number$, ");
  GenerateCodecCode(printer);
  printer->Print(");\n");
}

EnumOneofFieldGenerator::EnumOneofFieldGenerator(
    const FieldDescriptor* descriptor, int presenceIndex, const Options *options)
  : PrimitiveOneofFieldGenerator(descriptor, presenceIndex, options) {
}

EnumOneofFieldGenerator::~EnumOneofFieldGenerator() {
}

void EnumOneofFieldGenerator::GenerateMergingCode(io::Printer* printer) {
  printer->Print(variables_, "$property_name$ = other.$property_name$;\n");
}

void EnumOneofFieldGenerator::GenerateParsingCode(io::Printer* printer) {
  // TODO: What about if we read the default value?
  printer->Print(
    variables_,
    "$oneof_name$_ = input.ReadEnum();\n"
    "$oneof_name$Case_ = $oneof_property_name$OneofCase.$oneof_case_name$;\n");
}

void EnumOneofFieldGenerator::GenerateSerializationCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if ($has_property_check$) {\n"
    "  output.WriteRawTag($tag_bytes$);\n"
    "  output.WriteEnum((int) $property_name$);\n"
    "}\n");
}

void EnumOneofFieldGenerator::GenerateSerializedSizeCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if ($has_property_check$) {\n"
    "  size += $tag_size$ + pb::CodedOutputStream.ComputeEnumSize((int) $property_name$);\n"
    "}\n");
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
