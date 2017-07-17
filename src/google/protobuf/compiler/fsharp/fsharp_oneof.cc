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

#include <map>

#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/printer.h>

#include <google/protobuf/compiler/fsharp/fsharp_doc_comment.h>
#include <google/protobuf/compiler/fsharp/fsharp_helpers.h>
#include <google/protobuf/compiler/fsharp/fsharp_oneof.h>
#include <google/protobuf/compiler/fsharp/fsharp_options.h>
#include <google/protobuf/compiler/fsharp/fsharp_primitive_field.h>

using google::protobuf::internal::scoped_ptr;

namespace google {
namespace protobuf {
namespace compiler {
namespace fsharp {

OneofGenerator::OneofGenerator(const OneofDescriptor* descriptor, const FieldGeneratorMap* generators) {
  descriptor_ = descriptor;
  generators_ = generators;
  variables_["access_level"] = "public";
  variables_["oneof_type_name"] = GetTypeName(descriptor_);
  variables_["oneof_field_name"] = UnderscoresToCamelCase(descriptor_->name(), false) + "_";
  variables_["oneof_property_name"] = UnderscoresToCamelCase(descriptor_->name(), true);
}

OneofGenerator::~OneofGenerator() {}

void OneofGenerator::GenerateTypeDefinition(io::Printer* printer) {
  printer->Print(variables_, "and $oneof_type_name$ =\n");
  printer->Indent();
  printer->Print("| OneofNone\n");
  for (int i = 0; i < descriptor_->field_count(); i++) {
    variables_["field_name"] = UnderscoresToCamelCase(descriptor_->field(i)->name(), true);
    variables_["field_type"] = FieldGeneratorBase::TypeName(descriptor_->field(i));
    printer->Print(variables_, "| $field_name$ of $field_type$\n");
  }
  printer->Outdent();
  printer->Print("\n");
}

void OneofGenerator::GenerateValDeclaration(io::Printer* printer) {
  printer->Print(variables_, "val mutable private $oneof_field_name$ : $oneof_type_name$\n");
}

void OneofGenerator::GenerateConstructorValue(io::Printer* printer) {
  printer->Print(variables_,
    "$oneof_field_name$ = $oneof_type_name$.OneofNone\n"
  );
}

void OneofGenerator::GenerateMembers(io::Printer* printer) {
  // TODO(bbus) add comments and attributes
  printer->Print(
    variables_,
    "member $access_level$ this.$oneof_property_name$\n"
    "  with get () = this.$oneof_field_name$\n"
    "  and set(value: $oneof_type_name$) =\n"
    "    this.$oneof_field_name$ <- value\n\n");
}

void OneofGenerator::WriteHash(io::Printer* printer) {
  printer->Print(variables_,
    "match this.$oneof_property_name$ with\n"
    "| $oneof_type_name$.OneofNone -> ()\n"
  );
  for (int i = 0; i < descriptor_->field_count(); i++) {
    generators_->Get(descriptor_->field(i)->index())->WriteHash(printer);
  }
}

void OneofGenerator::GenerateSerializedSizeCode(io::Printer* printer) {
  printer->Print(variables_,
    "match this.$oneof_property_name$ with\n"
  );
  for (int i = 0; i < descriptor_->field_count(); i++) {
    generators_->Get(descriptor_->field(i)->index())->GenerateSerializedSizeCode(printer);
  }
  printer->Print("| _ -> ()\n");
}

void OneofGenerator::GenerateMergingCode(io::Printer* printer) {
  printer->Print(variables_, "this.$oneof_property_name$ <-\n");
  printer->Indent();
  printer->Print(variables_,
    "match other.$oneof_property_name$ with\n"
  );
  for (int i = 0; i < descriptor_->field_count(); i++) {
    generators_->Get(descriptor_->field(i)->index())->GenerateMergingCode(printer);
  }
  printer->Print(variables_, "| _ -> $oneof_type_name$.OneofNone\n");
  printer->Outdent();
}

void OneofGenerator::GenerateCloningCode(io::Printer* printer) {
  printer->Print(variables_,
    "$oneof_field_name$ =\n"
    "  match other.$oneof_field_name$ with\n"
  );
  printer->Indent();
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* fd = descriptor_->field(i);
    generators_->Get(descriptor_->field(i)->index())->GenerateCloningCode(printer);
  }
  printer->Print(variables_, "| $oneof_type_name$.OneofNone -> $oneof_type_name$.OneofNone\n");
  printer->Outdent();

}

}  // namespace fsharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
