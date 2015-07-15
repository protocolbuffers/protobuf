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

#include <google/protobuf/compiler/csharp/csharp_helpers.h>
#include <google/protobuf/compiler/csharp/csharp_wrapper_field.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

WrapperFieldGenerator::WrapperFieldGenerator(const FieldDescriptor* descriptor,
                                       int fieldOrdinal)
    : FieldGeneratorBase(descriptor, fieldOrdinal) {
  variables_["has_property_check"] = name() + "_ != null";
  variables_["has_not_property_check"] = name() + "_ == null";
  variables_["message_type_name"] = GetClassName(descriptor->message_type());
  const FieldDescriptor* wrapped_field = descriptor->message_type()->field(0);
  is_value_type = wrapped_field->type() != FieldDescriptor::TYPE_STRING &&
      wrapped_field->type() != FieldDescriptor::TYPE_BYTES;
  variables_["deref"] = is_value_type ? ".Value" : "";
  // This will always be a single byte, because it's always field 1.
  variables_["message_tag_bytes"] = SimpleItoa(FixedMakeTag(wrapped_field));
}

WrapperFieldGenerator::~WrapperFieldGenerator() {
}

void WrapperFieldGenerator::GenerateMembers(io::Printer* printer) {
  // Back the underlying property with an underlying message. This isn't efficient,
  // but it makes it easier to be compliant with what platforms which don't support wrapper
  // types would do. Currently, each time the value is changed, we create a new instance.
  // With suitable care to avoid aliasing, we could probably check whether or not we've already
  // got an instance, and simply mutate the existing one.
  printer->Print(
    variables_,
    "private $message_type_name$ $name$_;\n");
  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "$access_level$ $type_name$ $property_name$ {\n"
    "  get { return $name$_ == null ? ($type_name$) null : $name$_.Value; }\n"
    "  set {\n"
    "    pb::Freezable.CheckMutable(this);\n"
    "    $name$_ = value == null ? null : new $message_type_name$ { Value = value$deref$ };\n"
    "  }\n"
    "}\n");
}

void WrapperFieldGenerator::GenerateMergingCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if (other.$has_property_check$) {\n"
    "  if ($has_not_property_check$) {\n"
    "    $name$_ = new $message_type_name$();\n"
    "  }\n"
    "  $name$_.MergeFrom(other.$name$_);\n"
    "}\n");
}

void WrapperFieldGenerator::GenerateParsingCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if ($has_not_property_check$) {\n"
    "  $name$_ = new $message_type_name$();\n"
    "}\n"
    "input.ReadMessage($name$_);\n"); // No need to support TYPE_GROUP...
}

void WrapperFieldGenerator::GenerateSerializationCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if ($has_property_check$) {\n"
    "  output.WriteRawTag($tag_bytes$);\n"
    "  output.WriteMessage($name$_);\n"
    "}\n");
}

void WrapperFieldGenerator::GenerateSerializedSizeCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if ($has_property_check$) {\n"
    "  size += $tag_size$ + pb::CodedOutputStream.ComputeMessageSize($name$_);\n"
    "}\n");
}

void WrapperFieldGenerator::WriteHash(io::Printer* printer) {
  printer->Print(
    variables_,
    "if ($has_property_check$) hash ^= $property_name$.GetHashCode();\n");
}

void WrapperFieldGenerator::WriteEquals(io::Printer* printer) {
  printer->Print(
    variables_,
    "if ($property_name$ != other.$property_name$) return false;\n");
}

void WrapperFieldGenerator::WriteToString(io::Printer* printer) {
  // TODO: Implement if we ever actually need it...
}

void WrapperFieldGenerator::GenerateCloningCode(io::Printer* printer) {
  // This will effectively perform a deep clone - it will create a new
  // underlying message if necessary
  printer->Print(variables_,
    "$property_name$ = other.$property_name$;\n");
}

void WrapperFieldGenerator::GenerateCodecCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "pb::FieldCodec.ForWrapperType<$type_name$, $message_type_name$>($tag$, $message_type_name$.Parser)");
}

WrapperOneofFieldGenerator::WrapperOneofFieldGenerator(const FieldDescriptor* descriptor,
    int fieldOrdinal)
    : WrapperFieldGenerator(descriptor, fieldOrdinal) {
    SetCommonOneofFieldVariables(&variables_);
}

WrapperOneofFieldGenerator::~WrapperOneofFieldGenerator() {
}

void WrapperOneofFieldGenerator::GenerateMembers(io::Printer* printer) {
  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "$access_level$ $type_name$ $property_name$ {\n"
    "  get { return $has_property_check$ ? (($message_type_name$) $oneof_name$_).Value : ($type_name$) null; }\n"
    "  set {\n"
    "    pb::Freezable.CheckMutable(this);\n"
    "    $oneof_name$_ = value == null ? null : new $message_type_name$ { Value = value$deref$ };\n"
    "    $oneof_name$Case_ = value == null ? $oneof_property_name$OneofCase.None : $oneof_property_name$OneofCase.$property_name$;\n"
    "  }\n"
    "}\n");
}



void WrapperOneofFieldGenerator::GenerateParsingCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "$message_type_name$ subBuilder = new $message_type_name$();\n"
    "if ($has_property_check$) {\n"
    "  subBuilder.MergeFrom(($message_type_name$) $oneof_name$_);\n"
    "}\n"
    "input.ReadMessage(subBuilder);\n"
    // Don't set the property, which would create a new and equivalent message; just set the two fields.
    "$oneof_name$Case_ = $oneof_property_name$OneofCase.$property_name$;\n"
    "$oneof_name$_ = subBuilder;\n");
}

void WrapperOneofFieldGenerator::GenerateSerializationCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if ($has_property_check$) {\n"
    "  output.WriteRawTag($tag_bytes$);\n"
    "  output.WriteMessage(($message_type_name$) $oneof_name$_);\n"
    "}\n");
}

void WrapperOneofFieldGenerator::GenerateSerializedSizeCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if ($has_property_check$) {\n"
    "  size += $tag_size$ + pb::CodedOutputStream.ComputeMessageSize((($message_type_name$) $oneof_name$_));\n"
    "}\n");
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
