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

#include <google/protobuf/compiler/fsharp/fsharp_doc_comment.h>
#include <google/protobuf/compiler/fsharp/fsharp_helpers.h>
#include <google/protobuf/compiler/fsharp/fsharp_repeated_message_field.h>
#include <google/protobuf/compiler/fsharp/fsharp_message_field.h>
#include <google/protobuf/compiler/fsharp/fsharp_wrapper_field.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace fsharp {

RepeatedMessageFieldGenerator::RepeatedMessageFieldGenerator(
  const FieldDescriptor* descriptor, int fieldOrdinal, const Options *options)
  : FieldGeneratorBase(descriptor, fieldOrdinal, options) {
  variables_["full_codec"] = descriptor->containing_type()->name() + "._repeated_" + variables_["name"] + "_codec";
}

RepeatedMessageFieldGenerator::~RepeatedMessageFieldGenerator() {

}

void RepeatedMessageFieldGenerator::GenerateValDeclaration(io::Printer* printer) {
  printer->Print(
    variables_,
    "val mutable private $name$_ : RepeatedField<$type_name$>\n"
  );
}

void RepeatedMessageFieldGenerator::GenerateConstructorValue(io::Printer* printer) {
  printer->Print(
    variables_,
    "$name$_ = new RepeatedField<$type_name$>()\n"
  );
}

void RepeatedMessageFieldGenerator::GenerateMembers(io::Printer* printer) {
  printer->Print(
    variables_,
    "static member private _repeated_$name$_codec =\n"
    "  ");
  // Don't want to duplicate the codec code here... maybe we should have a
  // "create single field generator for this repeated field"
  // function, but it doesn't seem worth it for just this.
  if (IsWrapperType(descriptor_)) {
    scoped_ptr<FieldGeneratorBase> single_generator(
      new WrapperFieldGenerator(descriptor_, fieldOrdinal_, this->options()));
    single_generator->GenerateCodecCode(printer);
  } else {
    scoped_ptr<FieldGeneratorBase> single_generator(
      new MessageFieldGenerator(descriptor_, fieldOrdinal_, this->options()));
    single_generator->GenerateCodecCode(printer);
  }
  printer->Print("\n");
  WritePropertyDocComment(printer, descriptor_);
  AddPublicMemberAttributes(printer);
  printer->Print(
    variables_,
    "member $access_level$ this.$property_name$\n"
    "  with get() = this.$name$_\n"
  );
}

void RepeatedMessageFieldGenerator::GenerateMergingCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "this.$name$_.Add(other.$name$_)\n");
}

void RepeatedMessageFieldGenerator::GenerateParsingCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "this.$name$_.AddEntriesFrom(input, $full_codec$)\n");
}

void RepeatedMessageFieldGenerator::GenerateSerializationCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "this.$name$_.WriteTo(output, $full_codec$)\n");

}

void RepeatedMessageFieldGenerator::GenerateSerializedSizeCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "size <- size + this.$name$_.CalculateSize($full_codec$)\n");
}

void RepeatedMessageFieldGenerator::WriteHash(io::Printer* printer) {
  printer->Print(
    variables_,
    "hash <- hash ^^^ this.$name$_.GetHashCode()\n");
}

void RepeatedMessageFieldGenerator::WriteEquals(io::Printer* printer) {
  printer->Print(
    variables_,
    "else if not (this.$name$_.Equals(other.$name$_)) then false\n");
}

void RepeatedMessageFieldGenerator::WriteToString(io::Printer* printer) {
  variables_["field_name"] = GetFieldName(descriptor_);
  printer->Print(
    variables_,
    "PrintField(\"$field_name$\", $name$_, writer);\n");
}

void RepeatedMessageFieldGenerator::GenerateCloningCode(io::Printer* printer) {
  printer->Print(variables_,
    "$name$_ = other.$name$_.Clone()\n");
}

void RepeatedMessageFieldGenerator::GenerateFreezingCode(io::Printer* printer) {
}

}  // namespace fsharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
