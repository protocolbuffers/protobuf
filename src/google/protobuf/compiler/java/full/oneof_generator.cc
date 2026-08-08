// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: dweis@google.com (Daniel Weis)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/java/full/oneof_generator.h"

#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/java/context.h"
#include "google/protobuf/compiler/java/helpers.h"
#include "google/protobuf/compiler/java/name_resolver.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

OneofGenerator::OneofGenerator(const OneofDescriptor* descriptor,
                               Context* context)
    : descriptor_(descriptor), context_(context) {}

OneofGenerator::~OneofGenerator() = default;

void OneofGenerator::GenerateCommonBuilderMethods(io::Printer* printer) const {
  absl::flat_hash_map<absl::string_view, std::string> vars = {
      // These variables are placeholders to pick out the beginning and ends of
      // identifiers for annotations (when doing so with existing variables
      // would be ambiguous or impossible). They should never be set to anything
      // but the empty string.
      {"{", ""},
      {"}", ""},
  };
  vars["oneof_name"] = context_->GetOneofGeneratorInfo(descriptor_)->name;
  vars["oneof_capitalized_name"] =
      context_->GetOneofGeneratorInfo(descriptor_)->capitalized_name;
  vars["oneof_index"] = absl::StrCat(descriptor_->index());

  // oneofCase_ and oneof_
  printer->Print(vars,
                 "private int $oneof_name$Case_ = 0;\n"
                 "private java.lang.Object $oneof_name$_;\n");
  GenerateBuilderGetOneofCase(printer);
  GenerateBuilderClearOneof(printer);
}

void OneofGenerator::GenerateBuilderGetOneofCase(io::Printer* printer) const {
  absl::flat_hash_map<absl::string_view, std::string> vars = {
      {"{", ""},
      {"}", ""},
  };
  vars["oneof_name"] = context_->GetOneofGeneratorInfo(descriptor_)->name;
  vars["oneof_capitalized_name"] =
      context_->GetOneofGeneratorInfo(descriptor_)->capitalized_name;

  printer->Print(vars,
                 "public $oneof_capitalized_name$Case\n"
                 "    ${$get$oneof_capitalized_name$Case$}$() {\n"
                 "  return $oneof_capitalized_name$Case.forNumber(\n"
                 "      $oneof_name$Case_);\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void OneofGenerator::GenerateBuilderClearOneof(io::Printer* printer) const {
  absl::flat_hash_map<absl::string_view, std::string> vars = {
      {"{", ""},
      {"}", ""},
  };
  vars["oneof_name"] = context_->GetOneofGeneratorInfo(descriptor_)->name;
  vars["oneof_capitalized_name"] =
      context_->GetOneofGeneratorInfo(descriptor_)->capitalized_name;

  printer->Print(vars,
                 "\n"
                 "public Builder ${$clear$oneof_capitalized_name$$}$() {\n"
                 "  $oneof_name$Case_ = 0;\n"
                 "  $oneof_name$_ = null;\n"
                 "  onChanged();\n"
                 "  return this;\n"
                 "}\n"
                 "\n");
  printer->Annotate("{", "}", descriptor_,
                    io::AnnotationCollector::Semantic::kSet);
}

void OneofGenerator::GenerateBuilderClearMethod(io::Printer* printer) const {
  printer->Print(
      "$oneof_name$Case_ = 0;\n"
      "$oneof_name$_ = null;\n",
      "oneof_name", context_->GetOneofGeneratorInfo(descriptor_)->name);
}

void OneofGenerator::GenerateMergingCode(
    io::Printer* printer,
    const FieldGeneratorMap<ImmutableFieldGenerator>& field_generators) const {
  printer->Print(
      "switch (other.get$oneof_capitalized_name$Case()) {\n",
      "oneof_capitalized_name",
      context_->GetOneofGeneratorInfo(descriptor_)->capitalized_name);
  printer->Indent();
  for (int j = 0; j < descriptor_->field_count(); j++) {
    const FieldDescriptor* field = descriptor_->field(j);
    printer->Print("case $field_name$: {\n", "field_name",
                   absl::AsciiStrToUpper(field->name()));
    printer->Indent();
    field_generators.get(field).GenerateMergingCode(printer);
    printer->Print("break;\n");
    printer->Outdent();
    printer->Print("}\n");
  }
  printer->Print(
      "case $cap_oneof_name$_NOT_SET: {\n"
      "  break;\n"
      "}\n",
      "cap_oneof_name",
      absl::AsciiStrToUpper(
          context_->GetOneofGeneratorInfo(descriptor_)->name));
  printer->Outdent();
  printer->Print("}\n");
}

void OneofGenerator::GenerateBuildingCode(
    io::Printer* printer,
    const FieldGeneratorMap<ImmutableFieldGenerator>& field_generators) const {
  printer->Print(
      "result.$oneof_name$Case_ = $oneof_name$Case_;\n"
      "result.$oneof_name$_ = this.$oneof_name$_;\n",
      "oneof_name", context_->GetOneofGeneratorInfo(descriptor_)->name);
  for (int i = 0; i < descriptor_->field_count(); ++i) {
    if (descriptor_->field(i)->message_type() != nullptr) {
      const ImmutableFieldGenerator& field =
          field_generators.get(descriptor_->field(i));
      field.GenerateBuildingCode(printer);
    }
  }
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
