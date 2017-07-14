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

#include <google/protobuf/compiler/fsharp/fsharp_doc_comment.h>
#include <google/protobuf/compiler/fsharp/fsharp_helpers.h>
#include <google/protobuf/compiler/fsharp/fsharp_repeated_enum_field.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace fsharp {

RepeatedEnumFieldGenerator::RepeatedEnumFieldGenerator(
  const FieldDescriptor* descriptor, int fieldOrdinal, const Options *options)
  : FieldGeneratorBase(descriptor, fieldOrdinal, options) {
  variables_["full_codec"] = descriptor->containing_type()->name() + "._repeated_" + variables_["name"] + "_codec";
}

RepeatedEnumFieldGenerator::~RepeatedEnumFieldGenerator() {

}

void RepeatedEnumFieldGenerator::GenerateValDeclaration(io::Printer* printer) {
  printer->Print(
    variables_,
    "val mutable private $name$_ : RepeatedField<$type_name$>\n"
  );
}

void RepeatedEnumFieldGenerator::GenerateConstructorValue(io::Printer* printer) {
  printer->Print(
    variables_,
    "$name$_ = new RepeatedField<$type_name$>()\n"
  );
}

void RepeatedEnumFieldGenerator::GenerateMembers(io::Printer* printer) {
  printer->Print(
    variables_,
    "static member private _repeated_$name$_codec =\n"
    "  FieldCodec.ForEnum($tag$u, (fun x -> int x), (fun x -> enum x))\n");

  WritePropertyDocComment(printer, descriptor_);
  AddPublicMemberAttributes(printer);
  printer->Print(
    variables_,
    "member $access_level$ this.$property_name$\n"
    "  with get() = this.$name$_\n");
}

void RepeatedEnumFieldGenerator::GenerateMergingCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "this.$name$_.Add(other.$name$_)\n");
}

void RepeatedEnumFieldGenerator::GenerateParsingCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "this.$name$_.AddEntriesFrom(input, $full_codec$)\n");
}

void RepeatedEnumFieldGenerator::GenerateSerializationCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "this.$name$_.WriteTo(output, $full_codec$)\n");
}

void RepeatedEnumFieldGenerator::GenerateSerializedSizeCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "size <- size + this.$name$_.CalculateSize($full_codec$)\n");
}

void RepeatedEnumFieldGenerator::WriteHash(io::Printer* printer) {
  printer->Print(
    variables_,
    "hash <- hash ^^^ this.$name$_.GetHashCode()\n");
}

void RepeatedEnumFieldGenerator::WriteEquals(io::Printer* printer) {
  printer->Print(
    variables_,
    "else if not (this.$name$_.Equals(other.$name$_)) then false\n");
}

void RepeatedEnumFieldGenerator::WriteToString(io::Printer* printer) {
  printer->Print(variables_,
    "PrintField(\"$descriptor_name$\", this.$name$_, writer)\n");
}

void RepeatedEnumFieldGenerator::GenerateCloningCode(io::Printer* printer) {
  printer->Print(variables_,
    "$name$_ = other.$name$_.Clone()\n");
}

void RepeatedEnumFieldGenerator::GenerateFreezingCode(io::Printer* printer) {
}

}  // namespace fsharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
