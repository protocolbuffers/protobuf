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
#include "google/protobuf/compiler/code_generator_lite.h"
#include "google/protobuf/compiler/java/context.h"
#include "google/protobuf/compiler/java/generator_common.h"
#include "google/protobuf/compiler/java/helpers.h"
#include "google/protobuf/compiler/java/full/field_generator.h"
#include "google/protobuf/compiler/java/name_resolver.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

namespace {

void SetOneofVariables(
    const OneofDescriptor* descriptor, Context* context,
    absl::flat_hash_map<absl::string_view, std::string>* variables) {
  (*variables)["oneof_name"] = context->GetOneofGeneratorInfo(descriptor)->name;
  (*variables)["oneof_capitalized_name"] =
      context->GetOneofGeneratorInfo(descriptor)->capitalized_name;
  (*variables)["oneof_index"] = absl::StrCat(descriptor->index());
  (*variables)["cap_oneof_name"] =
      absl::AsciiStrToUpper((*variables)["oneof_name"]);
  (*variables)["classname"] = context->GetNameResolver()->GetImmutableClassName(
      descriptor->containing_type());
  // These variables are placeholders to pick out the beginning and ends of
  // identifiers for annotations (when doing so with existing variables
  // would be ambiguous or impossible). They should never be set to anything
  // but the empty string.
  (*variables)["{"] = "";
  (*variables)["}"] = "";
}

}  // namespace

OneofGenerator::OneofGenerator(const OneofDescriptor* descriptor,
                               Context* context)
    : descriptor_(descriptor) {
  SetOneofVariables(descriptor, context, &variables_);
}

OneofGenerator::~OneofGenerator() = default;

void OneofGenerator::GenerateCommonBuilderMethods(io::Printer* printer) const {
  // oneofCase_ and oneof_
  printer->Print(variables_,
                 "private int $oneof_name$Case_ = 0;\n"
                 "private java.lang.Object $oneof_name$_;\n");
  GenerateBuilderGetOneofCase(printer);
  GenerateBuilderClearOneof(printer);
}

void OneofGenerator::GenerateBuilderGetOneofCase(io::Printer* printer) const {
  printer->Print(variables_,
                 "public $oneof_capitalized_name$Case\n"
                 "    ${$get$oneof_capitalized_name$Case$}$() {\n"
                 "  return $oneof_capitalized_name$Case.forNumber(\n"
                 "      $oneof_name$Case_);\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void OneofGenerator::GenerateBuilderClearOneof(io::Printer* printer) const {
  printer->Print(variables_,
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
  printer->Print(variables_,
                 "$oneof_name$Case_ = 0;\n"
                 "$oneof_name$_ = null;\n");
}

void OneofGenerator::GenerateMergingCode(
    io::Printer* printer,
    const FieldGeneratorMap<ImmutableFieldGenerator>& field_generators) const {
  printer->Print(variables_,
                 "switch (other.get$oneof_capitalized_name$Case()) {\n");
  printer->Indent();
  for (int j = 0; j < descriptor_->field_count(); j++) {
    const FieldDescriptor* field = descriptor_->field(j);
    printer->Print("case $field_name$: {\n", "field_name",
                   absl::AsciiStrToUpper(field->name()));
    printer->Indent();
    field_generators.get(field).GenerateMergingCode(printer);
    printer->Outdent();
    printer->Print(
        "  break;\n"
        "}\n");
  }
  printer->Outdent();
  printer->Print(variables_,
                 "  case $cap_oneof_name$_NOT_SET: {\n"
                 "    break;\n"
                 "  }\n"
                 "}\n");
}

void OneofGenerator::GenerateBuildingCode(
    io::Printer* printer,
    const FieldGeneratorMap<ImmutableFieldGenerator>& field_generators) const {
  printer->Print(variables_,
                 "result.$oneof_name$Case_ = $oneof_name$Case_;\n"
                 "result.$oneof_name$_ = this.$oneof_name$_;\n");
  for (int i = 0; i < descriptor_->field_count(); ++i) {
    if (descriptor_->field(i)->message_type() != nullptr) {
      const ImmutableFieldGenerator& field =
          field_generators.get(descriptor_->field(i));
      field.GenerateBuildingCode(printer);
    }
  }
}

void OneofGenerator::GenerateInterfaceMembers(io::Printer* printer) const {
  printer->Print(variables_,
                 "\n"
                 "$classname$.$oneof_capitalized_name$Case "
                 "get$oneof_capitalized_name$Case();\n");
}

void OneofGenerator::GenerateMembers(io::Printer* printer) const {
  const OneofDescriptor* oneof = descriptor_;
  // oneofCase_ and oneof_
  printer->Print(variables_,
                 "private int $oneof_name$Case_ = 0;\n"
                 "@SuppressWarnings(\"serial\")\n"
                 "private java.lang.Object $oneof_name$_;\n");
  // OneofCase enum
  printer->Print(
      variables_,
      "public enum ${$$oneof_capitalized_name$Case$}$\n"
      // TODO: Remove EnumLite when we want to break compatibility with
      // 3.x users
      "    implements com.google.protobuf.Internal.EnumLite,\n"
      "        com.google.protobuf.AbstractMessage.InternalOneOfEnum {\n");
  printer->Annotate("{", "}", oneof);
  printer->Indent();
  for (int j = 0; j < (oneof)->field_count(); j++) {
    const FieldDescriptor* field = (oneof)->field(j);
    printer->Print(
        "$deprecation$$field_name$($field_number$),\n", "deprecation",
        field->options().deprecated() ? "@java.lang.Deprecated " : "",
        "field_name", absl::AsciiStrToUpper(field->name()), "field_number",
        absl::StrCat(field->number()));
    printer->Annotate("field_name", field);
  }
  printer->Print(variables_, "$cap_oneof_name$_NOT_SET(0);\n");
  printer->Print(variables_,
                 "private final int value;\n"
                 "private $oneof_capitalized_name$Case(int value) {\n"
                 "  this.value = value;\n"
                 "}\n");
  if (google::protobuf::internal::IsOss()) {
    printer->Print(
        variables_,
        "/**\n"
        " * @param value The number of the enum to look for.\n"
        " * @return The enum associated with the given number.\n"
        " * @deprecated Use {@link #forNumber(int)} instead.\n"
        " */\n"
        "@java.lang.Deprecated\n"
        "public static $oneof_capitalized_name$Case valueOf(int value) {\n"
        "  return forNumber(value);\n"
        "}\n"
        "\n");
  }
  if (!google::protobuf::internal::IsOss()) {
    printer->Print("@com.google.protobuf.Internal.ProtoMethodMayReturnNull\n");
  }
  printer->Print(
      variables_,
      "public static $oneof_capitalized_name$Case forNumber(int value) {\n"
      "  switch (value) {\n");
  for (int j = 0; j < (oneof)->field_count(); j++) {
    const FieldDescriptor* field = (oneof)->field(j);
    printer->Print("    case $field_number$: return $field_name$;\n",
                   "field_number", absl::StrCat(field->number()), "field_name",
                   absl::AsciiStrToUpper(field->name()));
  }
  printer->Print(variables_,
                 "    case 0: return $cap_oneof_name$_NOT_SET;\n"
                 "    default: return null;\n"
                 "  }\n"
                 "}\n"
                 "public int getNumber() {\n"
                 "  return this.value;\n"
                 "}\n");
  printer->Outdent();
  printer->Print("};\n\n");
  // oneofCase()
  printer->Print(variables_,
                 "public $oneof_capitalized_name$Case\n"
                 "${$get$oneof_capitalized_name$Case$}$() {\n"
                 "  return $oneof_capitalized_name$Case.forNumber(\n"
                 "      $oneof_name$Case_);\n"
                 "}\n"
                 "\n");
  printer->Annotate("{", "}", oneof);
}

void OneofGenerator::GenerateEqualsCode(
    io::Printer* printer,
    const FieldGeneratorMap<ImmutableFieldGenerator>& field_generators) const {
  const OneofDescriptor* oneof = descriptor_;
  printer->Print(variables_,
                 "if (!get$oneof_capitalized_name$Case().equals("
                 "other.get$oneof_capitalized_name$Case())) return false;\n");
  printer->Print(variables_, "switch ($oneof_name$Case_) {\n");
  printer->Indent();
  for (int j = 0; j < (oneof)->field_count(); j++) {
    const FieldDescriptor* field = (oneof)->field(j);
    printer->Print("case $field_number$:\n", "field_number",
                   absl::StrCat(field->number()));
    printer->Indent();
    field_generators.get(field).GenerateEqualsCode(printer);
    printer->Print("break;\n");
    printer->Outdent();
  }
  printer->Outdent();
  printer->Print(
      "  case 0:\n"
      "  default:\n"
      "}\n");
}

void OneofGenerator::GenerateHashCode(
    io::Printer* printer,
    const FieldGeneratorMap<ImmutableFieldGenerator>& field_generators) const {
  const OneofDescriptor* oneof = descriptor_;
  printer->Print(variables_, "switch ($oneof_name$Case_) {\n");
  printer->Indent();
  for (int j = 0; j < (oneof)->field_count(); j++) {
    const FieldDescriptor* field = (oneof)->field(j);
    printer->Print("case $field_number$:\n", "field_number",
                   absl::StrCat(field->number()));
    printer->Indent();
    field_generators.get(field).GenerateHashCode(printer);
    printer->Print("break;\n");
    printer->Outdent();
  }
  printer->Outdent();
  printer->Print(
      "  case 0:\n"
      "  default:\n"
      "}\n");
}
}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
