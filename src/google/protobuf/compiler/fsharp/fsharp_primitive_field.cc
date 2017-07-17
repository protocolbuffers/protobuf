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
#include <google/protobuf/stubs/strutil.h>

#include <google/protobuf/compiler/fsharp/fsharp_doc_comment.h>
#include <google/protobuf/compiler/fsharp/fsharp_helpers.h>
#include <google/protobuf/compiler/fsharp/fsharp_options.h>
#include <google/protobuf/compiler/fsharp/fsharp_primitive_field.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace fsharp {

PrimitiveFieldGenerator::PrimitiveFieldGenerator(
  const FieldDescriptor* descriptor, int fieldOrdinal, const Options *options)
  : FieldGeneratorBase(descriptor, fieldOrdinal, options) {
  is_value_type = descriptor->type() != FieldDescriptor::TYPE_STRING
    && descriptor->type() != FieldDescriptor::TYPE_BYTES;
  if (!is_value_type) {
    variables_["has_property_check"] = "this." + variables_["property_name"] + ".Length <> 0";
    variables_["other_has_property_check"] = "other." + variables_["property_name"] + ".Length <> 0";
  }
}

PrimitiveFieldGenerator::~PrimitiveFieldGenerator() {
}

void PrimitiveFieldGenerator::GenerateValDeclaration(io::Printer* printer) {
  printer->Print(
    variables_,
    "val mutable private $name$_ : $type_name$\n"
  );
}

void PrimitiveFieldGenerator::GenerateConstructorValue(io::Printer* printer) {
  printer->Print(
    variables_,
    "$name_def_message$\n"
  );
}

void PrimitiveFieldGenerator::GenerateMembers(io::Printer* printer) {
  // TODO(jonskeet): Work out whether we want to prevent the fields from ever being
  // null, or whether we just handle it, in the cases of bytes and string.
  // (Basically, should null-handling code be in the getter or the setter?)
  WritePropertyDocComment(printer, descriptor_);
  AddPublicMemberAttributes(printer);
  printer->Print(
    variables_,
    "member $access_level$ $self_indentifier$.$property_name$\n");

  printer->Indent();
  printer->Print(
    variables_,
    "with get() = $self_indentifier$.$name$_\n"
    "and set(value: $type_name$) =\n"
  );

  printer->Indent();
  if (is_value_type) {
    printer->Print(
      variables_,
      "$self_indentifier$.$name$_ <- value\n");
  } else {
    printer->Print(
      variables_,
      "if value <> null then\n"
      "  $self_indentifier$.$name$_ <- value\n");
  }
  printer->Outdent();
  printer->Outdent();
  printer->Print("\n");
}

void PrimitiveFieldGenerator::GenerateMergingCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if $other_has_property_check$ then\n"
    "  this.$property_name$ <- other.$property_name$\n");
}

void PrimitiveFieldGenerator::GenerateParsingCode(io::Printer* printer) {
  // Note: invoke the property setter rather than writing straight to the field,
  // so that we can normalize "null to empty" for strings and bytes.
  printer->Print(
    variables_,
    "this.$property_name$ <- input.Read$capitalized_type_name$()\n");
}

void PrimitiveFieldGenerator::GenerateSerializationCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if $has_property_check$ then\n"
    "  output.WriteRawTag($tag_bytes$)\n"
    "  output.Write$capitalized_type_name$(this.$property_name$)\n"
    "\n");
}

void PrimitiveFieldGenerator::GenerateSerializedSizeCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if $has_property_check$ then\n");
  printer->Indent();
  int fixedSize = GetFixedSize(descriptor_->type());
  if (fixedSize == -1) {
    printer->Print(
      variables_,
      "size <- size + $tag_size$ + CodedOutputStream.Compute$capitalized_type_name$Size(this.$property_name$)\n");
  } else {
    printer->Print(
      "size <- size + $tag_size$ + $fixed_size$\n",
      "fixed_size", SimpleItoa(fixedSize),
      "tag_size", variables_["tag_size"]);
  }
  printer->Outdent();
}

void PrimitiveFieldGenerator::WriteHash(io::Printer* printer) {
  printer->Print(
    variables_,
    "if $has_property_check$ then hash <- hash ^^^ this.$property_name$.GetHashCode()\n");
}

void PrimitiveFieldGenerator::WriteEquals(io::Printer* printer) {
  printer->Print(
    variables_,
    "else if this.$property_name$ <> other.$property_name$ then false\n");
}
void PrimitiveFieldGenerator::WriteToString(io::Printer* printer) {
  printer->Print(
    variables_,
    "PrintField(\"$descriptor_name$\", $has_property_check$, $property_name$, writer);\n");
}

void PrimitiveFieldGenerator::GenerateCloningCode(io::Printer* printer) {
  printer->Print(variables_,
    "$name$_ = other.$name$_\n");
}

void PrimitiveFieldGenerator::GenerateCodecCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "FieldCodec.For$capitalized_type_name$($tag$u)");
}

PrimitiveOneofFieldGenerator::PrimitiveOneofFieldGenerator(
  const FieldDescriptor* descriptor, int fieldOrdinal, const Options *options)
  : PrimitiveFieldGenerator(descriptor, fieldOrdinal, options) {
  SetCommonOneofFieldVariables(&variables_);
  const OneofDescriptor* containing_oneof = descriptor->containing_oneof();
}

PrimitiveOneofFieldGenerator::~PrimitiveOneofFieldGenerator() {
}

void PrimitiveOneofFieldGenerator::GenerateValDeclaration(io::Printer* printer) {
  // NOOP
}

void PrimitiveOneofFieldGenerator::GenerateConstructorValue(io::Printer* printer) {
  // NOOP
}

void PrimitiveOneofFieldGenerator::GenerateMembers(io::Printer* printer) {
  // NOOP
}

void PrimitiveOneofFieldGenerator::GenerateSerializationCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "match this.$oneof_field_name$ with\n"
    "  | $qualified_type$ x ->\n"
    "    output.WriteRawTag($tag_bytes$)\n"
    "    output.Write$capitalized_type_name$(x)\n"
    "  | _ -> ()\n\n");
}

void PrimitiveOneofFieldGenerator::GenerateSerializedSizeCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "| $qualified_type$ x ->\n");
  printer->Indent();
  int fixedSize = GetFixedSize(descriptor_->type());
  if (fixedSize == -1) {
    printer->Print(
      variables_,
      "size <- size + $tag_size$ + CodedOutputStream.Compute$capitalized_type_name$Size(x)\n");
  } else {
    printer->Print(
      "size <- size + $tag_size$ + $fixed_size$\n",
      "fixed_size", SimpleItoa(fixedSize),
      "tag_size", variables_["tag_size"]);
  }
  printer->Outdent();
}

void PrimitiveOneofFieldGenerator::WriteToString(io::Printer* printer) {
  printer->Print(variables_,
    "PrintField(\"$descriptor_name$\", $has_property_check$, $oneof_name$_, writer);\n");
}

void PrimitiveOneofFieldGenerator::GenerateParsingCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "this.$oneof_property_name$ <- $qualified_type$ (input.Read$capitalized_type_name$())\n");
}

void PrimitiveOneofFieldGenerator::WriteHash(io::Printer* printer) {
  printer->Print(
    variables_,
    "| $qualified_type$ x -> hash <- hash ^^^ x.GetHashCode()");
  printer->Print(" ^^^ $ordinal$\n", "ordinal", number());
}


void PrimitiveOneofFieldGenerator::GenerateMergingCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "| $qualified_type$ x -> $qualified_type$ x\n");
}

void PrimitiveOneofFieldGenerator::GenerateCloningCode(io::Printer* printer) {
  printer->Print(variables_,
    "| $qualified_type$ x -> $qualified_type$ x\n");
}

}  // namespace fsharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
