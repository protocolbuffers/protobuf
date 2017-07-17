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
#include <google/protobuf/compiler/fsharp/fsharp_message_field.h>
#include <google/protobuf/compiler/fsharp/fsharp_options.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace fsharp {

MessageFieldGenerator::MessageFieldGenerator(const FieldDescriptor* descriptor,
  int fieldOrdinal,
  const Options *options)
  : FieldGeneratorBase(descriptor, fieldOrdinal, options) {
  variables_["has_property_check"] = name() + "_ <> null";
  variables_["has_not_property_check"] = name() + "_ = null";
}

MessageFieldGenerator::~MessageFieldGenerator() {

}

void MessageFieldGenerator::GenerateValDeclaration(io::Printer* printer) {
  printer->Print(
    variables_,
    "val mutable private $name$_ : $type_name$\n"
  );
}

void MessageFieldGenerator::GenerateConstructorValue(io::Printer* printer) {
  printer->Print(
    variables_,
    "$name_def_message$\n"
  );
}

void MessageFieldGenerator::GenerateMembers(io::Printer* printer) {
  WritePropertyDocComment(printer, descriptor_);
  AddPublicMemberAttributes(printer);
  printer->Print(
    variables_,
    "member $access_level$ this.$property_name$\n"
    "  with get () = this.$name$_\n"
    "  and set(value: $type_name$) =\n"
    "    this.$name$_ <- value\n");
}

void MessageFieldGenerator::GenerateMergingCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if other.$has_property_check$ then\n"
    "  if this.$has_not_property_check$ then\n"
    "    this.$name$_ <- new $type_name$()\n"
    "  (this.$property_name$ :> IMessage<_>).MergeFrom(other.$property_name$)\n");
}

void MessageFieldGenerator::GenerateParsingCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if this.$has_not_property_check$ then\n"
    "  this.$name$_ <- new $type_name$()\n"
    // TODO(jonskeet): Do we really need merging behaviour like this?
    "input.ReadMessage(this.$name$_)\n"); // No need to support TYPE_GROUP...
}

void MessageFieldGenerator::GenerateSerializationCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if this.$has_property_check$ then\n"
    "  output.WriteRawTag($tag_bytes$)\n"
    "  output.WriteMessage(this.$property_name$)\n");
}

void MessageFieldGenerator::GenerateSerializedSizeCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if this.$has_property_check$ then\n"
    "  size <- size + $tag_size$ + CodedOutputStream.ComputeMessageSize(this.$property_name$)\n");
}

void MessageFieldGenerator::WriteHash(io::Printer* printer) {
  printer->Print(
    variables_,
    "if this.$has_property_check$ then hash <- hash ^^^ this.$property_name$.GetHashCode()\n");
}
void MessageFieldGenerator::WriteEquals(io::Printer* printer) {
  printer->Print(
    variables_,
    "else if not (System.Object.Equals(this.$property_name$, other.$property_name$)) then false\n");
}
void MessageFieldGenerator::WriteToString(io::Printer* printer) {
  variables_["field_name"] = GetFieldName(descriptor_);
  printer->Print(
    variables_,
    "PrintField(\"$field_name$\", has$property_name$, $name$_, writer);\n");
}

void MessageFieldGenerator::GenerateCloningCode(io::Printer* printer) {
  printer->Print(variables_,
    "$name$_ = if other.$has_property_check$ then (other.$name$_ :> IDeepCloneable<_>).Clone() else null\n");
}

void MessageFieldGenerator::GenerateFreezingCode(io::Printer* printer) {
}

void MessageFieldGenerator::GenerateCodecCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "FieldCodec.ForMessage($tag$u, $type_name$.Parser)");
}

MessageOneofFieldGenerator::MessageOneofFieldGenerator(
  const FieldDescriptor* descriptor,
  int fieldOrdinal,
  const Options *options)
  : MessageFieldGenerator(descriptor, fieldOrdinal, options) {
  SetCommonOneofFieldVariables(&variables_);
  const OneofDescriptor* containing_oneof = descriptor->containing_oneof();
}

MessageOneofFieldGenerator::~MessageOneofFieldGenerator() {

}
void MessageOneofFieldGenerator::GenerateValDeclaration(io::Printer* printer) {
  // NOOP
}

void MessageOneofFieldGenerator::GenerateConstructorValue(io::Printer* printer) {
  // NOOP
}

void MessageOneofFieldGenerator::GenerateMembers(io::Printer* printer) {
  // NOOP
}

void MessageOneofFieldGenerator::GenerateParsingCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "let subBuilder = new $type_name$()\n"
    "match this.$oneof_property_name$ with\n"
    "| $qualified_type$ x ->\n"
    "  (subBuilder :> IMessage<_>).MergeFrom(x)\n"
    "| _ -> ()\n"
    "input.ReadMessage(subBuilder)\n"
    "this.$oneof_property_name$ <- $qualified_type$ subBuilder\n");
}

void MessageOneofFieldGenerator::GenerateSerializationCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "match this.$oneof_field_name$ with\n"
    "  | $qualified_type$ x ->\n"
    "    output.WriteRawTag($tag_bytes$)\n"
    "    output.WriteMessage(x)\n"
    "  | _ -> ()\n\n");
}

void MessageOneofFieldGenerator::GenerateSerializedSizeCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "| $qualified_type$ x -> size <-size + $tag_size$ + CodedOutputStream.ComputeMessageSize(x)\n");
}

void MessageOneofFieldGenerator::WriteToString(io::Printer* printer) {
  printer->Print(
    variables_,
    "PrintField(\"$descriptor_name$\", $has_property_check$, $oneof_name$_, writer);\n");
}

void MessageOneofFieldGenerator::WriteHash(io::Printer* printer) {
  printer->Print(
    variables_,
    "| $qualified_type$ x -> hash <- hash ^^^ x.GetHashCode()");
  printer->Print(" ^^^ $ordinal$\n", "ordinal", number());
}

void MessageOneofFieldGenerator::GenerateMergingCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "| $qualified_type$ x ->\n"
  );
  printer->Indent();
  printer->Print(variables_,
    "match this.$oneof_property_name$ with\n"
    "| $qualified_type$ y ->\n"
    "  (y :> IMessage<_>).MergeFrom(x)\n"
    "  $qualified_type$ y\n"
    "| _ ->\n"
    "  let y = new $type_name$()\n"
    "  (y :> IMessage<_>).MergeFrom(x)\n"
    "  $qualified_type$ y\n"
  );

  printer->Outdent();
}

void MessageOneofFieldGenerator::GenerateCloningCode(io::Printer* printer) {
  printer->Print(variables_,
    "| $qualified_type$ x -> $qualified_type$ ((x :> IDeepCloneable<_>).Clone())\n");
}

}  // namespace fsharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
