// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
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
#include <google/protobuf/compiler/fsharp/fsharp_map_field.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace fsharp {

MapFieldGenerator::MapFieldGenerator(const FieldDescriptor* descriptor,
  int fieldOrdinal,
  const Options* options)
  : FieldGeneratorBase(descriptor, fieldOrdinal, options) {
  const FieldDescriptor* key_descriptor =
    descriptor_->message_type()->FindFieldByName("key");
  const FieldDescriptor* value_descriptor =
    descriptor_->message_type()->FindFieldByName("value");
  variables_["key_type_name"] = TypeName(key_descriptor);
  variables_["value_type_name"] = TypeName(value_descriptor);
  variables_["full_codec"] = descriptor->containing_type()->name() + "._map_" + variables_["name"] + "_codec";
}

MapFieldGenerator::~MapFieldGenerator() {
}

void MapFieldGenerator::GenerateValDeclaration(io::Printer* printer) {
  printer->Print(
    variables_,
    "val mutable private $name$_ : MapField<$key_type_name$, $value_type_name$>\n"
  );
}

void MapFieldGenerator::GenerateConstructorValue(io::Printer* printer) {
  printer->Print(
    variables_,
    "$name$_ = new MapField<$key_type_name$, $value_type_name$>()\n"
  );
}

void MapFieldGenerator::GenerateMembers(io::Printer* printer) {
  const FieldDescriptor* key_descriptor =
    descriptor_->message_type()->FindFieldByName("key");
  const FieldDescriptor* value_descriptor =
    descriptor_->message_type()->FindFieldByName("value");
  scoped_ptr<FieldGeneratorBase> key_generator(
    CreateFieldGenerator(key_descriptor, 1, this->options()));
  scoped_ptr<FieldGeneratorBase> value_generator(
    CreateFieldGenerator(value_descriptor, 2, this->options()));

  printer->Print(
    variables_,
    "static member _map_$name$_codec =\n"
    "  new MapField<$key_type_name$, $value_type_name$>.Codec(");
  key_generator->GenerateCodecCode(printer);
  printer->Print(", ");
  value_generator->GenerateCodecCode(printer);
  printer->Print(
    variables_,
    ", $tag$u)\n");
  WritePropertyDocComment(printer, descriptor_);
  AddPublicMemberAttributes(printer);
  printer->Print(
    variables_,
    "member $access_level$ this.$property_name$\n"
    "  with get() = this.$name$_\n");
}

void MapFieldGenerator::GenerateMergingCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "this.$name$_.Add(other.$name$_)\n");
}

void MapFieldGenerator::GenerateParsingCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "this.$name$_.AddEntriesFrom(input, $full_codec$)\n");
}

void MapFieldGenerator::GenerateSerializationCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "this.$name$_.WriteTo(output, $full_codec$)\n");
}

void MapFieldGenerator::GenerateSerializedSizeCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "size <- size + this.$name$_.CalculateSize($full_codec$)\n");
}

void MapFieldGenerator::WriteHash(io::Printer* printer) {
  printer->Print(
    variables_,
    "hash <- hash ^^^ this.$property_name$.GetHashCode()\n");
}
void MapFieldGenerator::WriteEquals(io::Printer* printer) {
  printer->Print(
    variables_,
    "else if not (this.$property_name$.Equals(other.$property_name$)) then false\n");
}

void MapFieldGenerator::WriteToString(io::Printer* printer) {
  // TODO: If we ever actually use ToString, we'll need to impleme this...
}

void MapFieldGenerator::GenerateCloningCode(io::Printer* printer) {
  printer->Print(variables_,
    "$name$_ = other.$name$_.Clone()\n");
}

void MapFieldGenerator::GenerateFreezingCode(io::Printer* printer) {
}

}  // namespace fsharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
