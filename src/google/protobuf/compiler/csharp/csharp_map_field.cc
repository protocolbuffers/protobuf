// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/csharp/csharp_map_field.h"

#include <sstream>

#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/compiler/csharp/csharp_doc_comment.h"
#include "google/protobuf/compiler/csharp/csharp_helpers.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

MapFieldGenerator::MapFieldGenerator(const FieldDescriptor* descriptor,
                                     int presenceIndex,
                                     const Options* options)
    : FieldGeneratorBase(descriptor, presenceIndex, options) {
}

MapFieldGenerator::~MapFieldGenerator() {
}

void MapFieldGenerator::GenerateMembers(io::Printer* printer) {
  const FieldDescriptor* key_descriptor =
      descriptor_->message_type()->map_key();
  const FieldDescriptor* value_descriptor =
      descriptor_->message_type()->map_value();
  variables_["key_type_name"] = type_name(key_descriptor);
  if (options()->enable_nullable) {
    variables_["value_type_name"] = value_type_name(value_descriptor);
  } else {
    variables_["value_type_name"] = type_name(value_descriptor);
  }
  std::unique_ptr<FieldGeneratorBase> key_generator(
      CreateFieldGenerator(key_descriptor, 1, this->options()));
  std::unique_ptr<FieldGeneratorBase> value_generator(
      CreateFieldGenerator(value_descriptor, 2, this->options()));

  printer->Print(
    variables_,
    "private static readonly pbc::MapField<$key_type_name$, $value_type_name$>.Codec _map_$name$_codec\n"
    "    = new pbc::MapField<$key_type_name$, $value_type_name$>.Codec(");
  key_generator->GenerateCodecCode(printer);
  printer->Print(", ");
  value_generator->GenerateCodecCode(printer);
  printer->Print(
    variables_,
    ", $tag$);\n");

  if (!options()->use_properties) {
    printer->Print(variables_, 
      "$access_level$ pbc::MapField<$key_type_name$, $value_type_name$> $cs_field_name$ = new pbc::MapField<$key_type_name$, $value_type_name$>();\n");
    return;
  }

  printer->Print(variables_, 
    "private readonly pbc::MapField<$key_type_name$, $value_type_name$> $cs_field_name$ = new pbc::MapField<$key_type_name$, $value_type_name$>();\n");
  WritePropertyDocComment(printer, options(), descriptor_);
  AddPublicMemberAttributes(printer);
  printer->Print(
    variables_,
    "$access_level$ pbc::MapField<$key_type_name$, $value_type_name$> $property_name$ {\n"
    "  get { return $cs_field_name$; }\n"
    "}\n");
}

void MapFieldGenerator::GenerateMergingCode(io::Printer* printer) {
  printer->Print(variables_,
                 "$cs_field_name$.MergeFrom(other.$cs_field_name$);\n");
}

void MapFieldGenerator::GenerateParsingCode(io::Printer* printer) {
  GenerateParsingCode(printer, true);
}

void MapFieldGenerator::GenerateParsingCode(io::Printer* printer, bool use_parse_context) {
  printer->Print(
    variables_,
    use_parse_context
    ? "$cs_field_name$.AddEntriesFrom(ref input, _map_$name$_codec);\n"
    : "$cs_field_name$.AddEntriesFrom(input, _map_$name$_codec);\n");
}

void MapFieldGenerator::GenerateSerializationCode(io::Printer* printer) {
  GenerateSerializationCode(printer, true);
}

void MapFieldGenerator::GenerateSerializationCode(io::Printer* printer, bool use_write_context) {
  printer->Print(
    variables_,
    use_write_context
    ? "$cs_field_name$.WriteTo(ref output, _map_$name$_codec);\n"
    : "$cs_field_name$.WriteTo(output, _map_$name$_codec);\n");
}

void MapFieldGenerator::GenerateSerializedSizeCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "size += $cs_field_name$.CalculateSize(_map_$name$_codec);\n");
}

void MapFieldGenerator::WriteHash(io::Printer* printer) {
  printer->Print(
    variables_,
    "hash = 17 * hash + $property_name$.GetHashCode();\n");
}
void MapFieldGenerator::WriteEquals(io::Printer* printer) {
  printer->Print(
    variables_,
    "if (!$property_name$.Equals(other.$property_name$)) return false;\n");
}

void MapFieldGenerator::WriteToString(io::Printer* printer) {
    // TODO: If we ever actually use ToString, we'll need to impleme this...
}

void MapFieldGenerator::GenerateCloningCode(io::Printer* printer) {
  printer->Print(variables_,
    "$cs_field_name$ = other.$cs_field_name$.Clone();\n");
}

void MapFieldGenerator::GenerateFreezingCode(io::Printer* printer) {
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
