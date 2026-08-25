// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/java/full/message_field.h"

#include <cstdint>
#include <string>

#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "absl/types/optional.h"
#include "google/protobuf/compiler/java/context.h"
#include "google/protobuf/compiler/java/doc_comment.h"
#include "google/protobuf/compiler/java/field_common.h"
#include "google/protobuf/compiler/java/helpers.h"
#include "google/protobuf/compiler/java/full/field_generator.h"
#include "google/protobuf/compiler/java/name_resolver.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/wire_format.h"

// Must be last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {


namespace {
using Semantic = ::google::protobuf::io::AnnotationCollector::Semantic;

void SetMessageVariables(
    const FieldDescriptor* descriptor, int bit_index,
    const FieldGeneratorInfo* info, ClassNameResolver* name_resolver,
    absl::flat_hash_map<absl::string_view, std::string>* variables,
    Context* context) {
  SetCommonFieldVariables(descriptor, info, variables);

  (*variables)["type"] =
      name_resolver->GetImmutableClassName(descriptor->message_type());
  (*variables)["group_or_message"] =
      (GetType(descriptor) == FieldDescriptor::TYPE_GROUP) ? "Group"
                                                           : "Message";
  (*variables)["empty_list"] =
      absl::StrCat("emptyList(", (*variables)["type"], ".class)");

  // TODO: Add @deprecated javadoc when generating javadoc is supported
  // by the proto compiler
  (*variables)["deprecation"] =
      descriptor->options().deprecated() ? "@java.lang.Deprecated " : "";
  (*variables)["on_changed"] = "onChanged();";
  (*variables)["get_parser"] = "parser()";

  if (HasHasbit(descriptor)) {
    // For singular messages and builders, one bit is used for the hasField bit.
    // Note that these have a trailing ";".
    (*variables)["set_has_field_bit_to_local"] =
        GenerateSetBitToLocal(bit_index);

    (*variables)["is_field_present"] = GenerateGetBit(bit_index);
  } else {
    (*variables)["set_has_field_bit_to_local"] = "";
    variables->insert(
        {"is_field_present", absl::StrCat((*variables)["name"], "_ != null")});
  }

  // For repeated builders, one bit is used for whether the array is immutable.
  (*variables)["get_mutable_bit_builder"] = GenerateGetBit(bit_index);
  (*variables)["set_mutable_bit_builder"] = GenerateSetBit(bit_index);
  (*variables)["clear_mutable_bit_builder"] = GenerateClearBit(bit_index);

  (*variables)["get_has_field_bit"] = GenerateGetBit(bit_index);
  (*variables)["set_has_field_bit"] =
      absl::StrCat(GenerateSetBit(bit_index), ";");
  (*variables)["clear_has_field_bit"] =
      absl::StrCat(GenerateClearBit(bit_index), ";");
  (*variables)["get_has_field_bit_from_local"] =
      GenerateGetBitFromLocal(bit_index);

  (*variables)["tag"] = absl::StrCat(
      static_cast<int32_t>(internal::WireFormat::MakeTag(descriptor)));
  (*variables)["tag_size"] = absl::StrCat(
      internal::WireFormat::TagSize(descriptor->number(), GetType(descriptor)));
}

}  // namespace

// ===================================================================

ImmutableMessageFieldGenerator::ImmutableMessageFieldGenerator(
    const FieldDescriptor* descriptor, int bit_index, Context* context)
    : ImmutableFieldGenerator(descriptor, bit_index, context) {
  SetMessageVariables(descriptor, bit_index,
                      context->GetFieldGeneratorInfo(descriptor),
                      name_resolver_, &variables_, context);
}

ImmutableMessageFieldGenerator::~ImmutableMessageFieldGenerator() = default;

void ImmutableMessageFieldGenerator::GenerateInterfaceHasMethod(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, HAZZER,
                               context_->options());
  printer->Print(variables_, "$deprecation$boolean has$capitalized_name$();\n");
}

void ImmutableMessageFieldGenerator::GenerateInterfaceGetMethod(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, GETTER,
                               context_->options());
  printer->Print(variables_, "$deprecation$$type$ get$capitalized_name$();\n");
}

void ImmutableMessageFieldGenerator::GenerateInterfaceGetOrBuilderMethod(
    io::Printer* printer) const {
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "$deprecation$$type$OrBuilder get$capitalized_name$OrBuilder();\n");
}
void ImmutableMessageFieldGenerator::GenerateInterfaceMembers(
    io::Printer* printer) const {
  // TODO: In the future, consider having a method specific to the
  // interface so that builders can choose dynamically to either return a
  // message or a nested builder, so that asking for the interface doesn't
  // cause a message to ever be built.
  GenerateInterfaceHasMethod(printer);
  GenerateInterfaceGetMethod(printer);
  GenerateInterfaceGetOrBuilderMethod(printer);
}

void ImmutableMessageFieldGenerator::GenerateHasMethod(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, HAZZER,
                               context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
                 "  return $is_field_present$;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void ImmutableMessageFieldGenerator::GenerateGetMethod(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, GETTER,
                               context_->options());
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public $type$ ${$get$capitalized_name$$}$() {\n"
      "  return $name$_ == null ? $type$.getDefaultInstance() : $name$_;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void ImmutableMessageFieldGenerator::GenerateGetOrBuilderMethod(
    io::Printer* printer) const {
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public $type$OrBuilder "
      "${$get$capitalized_name$OrBuilder$}$() {\n"
      "  return $name$_ == null ? $type$.getDefaultInstance() : $name$_;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void ImmutableMessageFieldGenerator::GenerateMembers(
    io::Printer* printer) const {
  printer->Print(variables_, "private $type$ $name$_;\n");
  PrintExtraFieldInfo(variables_, printer);

  GenerateHasMethod(printer);
  GenerateGetMethod(printer);
  GenerateGetOrBuilderMethod(printer);
  GenerateWriteFieldMethod(printer);
}

void ImmutableMessageFieldGenerator::GenerateWriteFieldMethod(
    io::Printer* printer) const {
  if (GetType(descriptor_) == FieldDescriptor::TYPE_GROUP) {
    printer->Print(variables_,
                   "private void write$capitalized_name$Field(\n"
                   "    com.google.protobuf.CodedOutputStream output)\n"
                   "    throws java.io.IOException {\n"
                   "  output.writeGroup($number$, get$capitalized_name$());\n"
                   "}\n");
  } else {
    printer->Print(variables_,
                   "private void write$capitalized_name$Field(\n"
                   "    com.google.protobuf.CodedOutputStream output)\n"
                   "    throws java.io.IOException {\n"
                   "  $type$ tmp = get$capitalized_name$();\n"
                   "  output.writeUInt32NoTag($tag$);\n"
                   "  output.writeUInt32NoTag(tmp.getSerializedSize());\n"
                   "  tmp.writeTo(output);\n"
                   "}\n");
  }
}

void ImmutableMessageFieldGenerator::PrintNestedBuilderCondition(
    io::Printer* printer, const char* regular_case,
    const char* nested_builder_case) const {
  printer->Print(variables_, "if ($name$Builder_ == null) {\n");
  printer->Indent();
  printer->Print(variables_, regular_case);
  printer->Outdent();
  printer->Print("} else {\n");
  printer->Indent();
  printer->Print(variables_, nested_builder_case);
  printer->Outdent();
  printer->Print("}\n");
}

void ImmutableMessageFieldGenerator::PrintNestedBuilderFunction(
    io::Printer* printer, const char* method_prototype,
    const char* regular_case, const char* nested_builder_case,
    const char* trailing_code,
    absl::optional<io::AnnotationCollector::Semantic> semantic) const {
  printer->Print(variables_, method_prototype);
  printer->Annotate("{", "}", descriptor_, semantic);
  printer->Print(" {\n");
  printer->Indent();
  PrintNestedBuilderCondition(printer, regular_case, nested_builder_case);
  if (trailing_code != nullptr) {
    printer->Print(variables_, trailing_code);
  }
  printer->Outdent();
  printer->Print("}\n");
}

void ImmutableMessageFieldGenerator::GenerateBuilderHasMethod(
    io::Printer* printer) const {
  // boolean hasField()
  WriteFieldAccessorDocComment(printer, descriptor_, HAZZER,
                               context_->options());
  printer->Print(variables_,
                 "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
                 "  return $get_has_field_bit$;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void ImmutableMessageFieldGenerator::GenerateBuilderGetMethod(
    io::Printer* printer) const {
  // Field getField()
  WriteFieldAccessorDocComment(printer, descriptor_, GETTER,
                               context_->options());
  PrintNestedBuilderFunction(
      printer, "$deprecation$public $type$ ${$get$capitalized_name$$}$()",
      "return $name$_ == null ? $type$.getDefaultInstance() : $name$_;\n",
      "return $name$Builder_.getMessage();\n", nullptr);
}

void ImmutableMessageFieldGenerator::GenerateBuilderSetMethod(
    io::Printer* printer) const {
  // Field.Builder setField(Field value)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  PrintNestedBuilderFunction(
      printer,
      "$deprecation$public Builder ${$set$capitalized_name$$}$($type$ value)",

      "java.util.Objects.requireNonNull(value);\n"
      "$name$_ = value;\n",

      "$name$Builder_.setMessage(value);\n",

      "$set_has_field_bit$\n"
      "$on_changed$\n"
      "return this;\n",
      Semantic::kSet);
}

void ImmutableMessageFieldGenerator::GenerateBuilderSetBuilderMethod(
    io::Printer* printer) const {
  // Field.Builder setField(Field.Builder builderForValue)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$public Builder ${$set$capitalized_name$$}$(\n"
                 "    $type$.Builder builderForValue) {\n"
                 "  java.util.Objects.requireNonNull(builderForValue);\n"
                 "  return set$capitalized_name$(builderForValue.build());\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void ImmutableMessageFieldGenerator::GenerateBuilderMergeMethod(
    io::Printer* printer) const {
  // Message.Builder mergeField(Field value)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  PrintNestedBuilderFunction(
      printer,
      "$deprecation$public Builder ${$merge$capitalized_name$$}$($type$ value)",
      "if ($get_has_field_bit$ &&\n"
      "  $name$_ != null &&\n"
      "  $name$_ != $type$.getDefaultInstance()) {\n"
      "  get$capitalized_name$Builder().mergeFrom(value);\n"
      "} else {\n"
      "  $name$_ = value;\n"
      "}\n",

      "$name$Builder_.mergeFrom(value);\n",

      "if ($name$_ != null) {\n"
      "  $set_has_field_bit$\n"
      "  $on_changed$\n"
      "}\n"
      "return this;\n",
      Semantic::kSet);
}

void ImmutableMessageFieldGenerator::GenerateBuilderClearMethod(
    io::Printer* printer) const {
  // Message.Builder clearField()
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$clear$capitalized_name$$}$() {\n"
      "  $clear_has_field_bit$\n"
      "  $name$_ = null;\n"
      "  if ($name$Builder_ != null) {\n"
      "    $name$Builder_.dispose();\n"
      "    $name$Builder_ = null;\n"
      "  }\n"
      "  $on_changed$\n"
      "  return this;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void ImmutableMessageFieldGenerator::GenerateBuilderGetBuilderMethod(
    io::Printer* printer) const {
  // Field.Builder getFieldBuilder()
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$public $type$.Builder "
                 "${$get$capitalized_name$Builder$}$() {\n"
                 "  $set_has_field_bit$\n"
                 "  $on_changed$\n"
                 "  return "
                 "internalGet$capitalized_name$FieldBuilder().getBuilder();\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void ImmutableMessageFieldGenerator::GenerateBuilderGetOrBuilderMethod(
    io::Printer* printer) const {
  // FieldOrBuilder getFieldOrBuilder()
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$public $type$OrBuilder "
                 "${$get$capitalized_name$OrBuilder$}$() {\n"
                 "  if ($name$Builder_ != null) {\n"
                 "    return $name$Builder_.getMessageOrBuilder();\n"
                 "  } else {\n"
                 "    return $name$_ == null ?\n"
                 "        $type$.getDefaultInstance() : $name$_;\n"
                 "  }\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void ImmutableMessageFieldGenerator::
    GenerateBuilderInternalGetFieldBuilderMethod(io::Printer* printer) const {
  // SingleFieldBuilder getFieldFieldBuilder
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "private com.google.protobuf.SingleFieldBuilder<\n"
      "    $type$, $type$.Builder, $type$OrBuilder> \n"
      "    internalGet$capitalized_name$FieldBuilder() {\n"
      "  if ($name$Builder_ == null) {\n"
      "    $name$Builder_ = new com.google.protobuf.SingleFieldBuilder<\n"
      "        $type$, $type$.Builder, $type$OrBuilder>(\n"
      "            get$capitalized_name$(),\n"
      "            getParentForChildren(),\n"
      "            isClean());\n"
      "    $name$_ = null;\n"
      "  }\n"
      "  $set_has_field_bit$\n"
      "  return $name$Builder_;\n"
      "}\n");
}

void ImmutableMessageFieldGenerator::GenerateBuilderParseMethod(
    io::Printer* printer) const {
  // Private parse method for this field, created to avoid code size too large
  // issues in the try-catch block.
  printer->Print(
      variables_,
      "private void parse$capitalized_name$Field(\n"
      "    com.google.protobuf.CodedInputStream input,\n"
      "    com.google.protobuf.ExtensionRegistryLite extensionRegistry)\n"
      "    throws java.io.IOException {\n");
  printer->Indent();

  if (GetType(descriptor_) == FieldDescriptor::TYPE_GROUP) {
    printer->Print(variables_,
                   "if ($name$_ != null || $name$Builder_ != null) {\n"
                   "  input.readGroup($number$,\n"
                   "      "
                   "internalGet$capitalized_name$FieldBuilder().getBuilder(),\n"
                   "      extensionRegistry);\n"
                   "} else {\n"
                   "  $name$_ = input.readGroup($number$, $type$.parser(),\n"
                   "      extensionRegistry);\n"
                   "}\n"
                   "$set_has_field_bit$\n");
  } else {
    printer->Print(
        variables_,
        "final int oldLimit = input.pushLimitBeforeMessage();\n"
        "if ($name$_ != null || $name$Builder_ != null) {\n"
        "  internalGet$capitalized_name$FieldBuilder().getBuilder()\n"
        "      .mergeFrom(input, extensionRegistry);\n"
        "} else {\n"
        "  $name$_ = $type$.parser().parsePartialFrom(input, "
        "extensionRegistry);\n"
        "}\n"
        "input.popLimitAfterMessage(oldLimit);\n"
        "$set_has_field_bit$\n");
  }
  printer->Outdent();
  printer->Print("}\n");
}

void ImmutableMessageFieldGenerator::GenerateBuilderMembers(
    io::Printer* printer) const {
  // When using nested-builders, the code initially works just like the
  // non-nested builder case. It only creates a nested builder lazily on
  // demand and then forever delegates to it after creation.
  printer->Print(variables_, "private $type$ $name$_;\n");

  printer->Print(variables_,
                 // If this builder is non-null, it is used and the other fields
                 // are ignored.
                 "private com.google.protobuf.SingleFieldBuilder<\n"
                 "    $type$, $type$.Builder, $type$OrBuilder> $name$Builder_;"
                 "\n");

  GenerateBuilderHasMethod(printer);
  GenerateBuilderGetMethod(printer);
  GenerateBuilderSetMethod(printer);
  GenerateBuilderSetBuilderMethod(printer);
  GenerateBuilderMergeMethod(printer);
  GenerateBuilderClearMethod(printer);
  GenerateBuilderGetBuilderMethod(printer);
  GenerateBuilderGetOrBuilderMethod(printer);
  GenerateBuilderInternalGetFieldBuilderMethod(printer);
  GenerateBuilderParseMethod(printer);
}

void ImmutableMessageFieldGenerator::GenerateFieldBuilderInitializationCode(
    io::Printer* printer) const {
  printer->Print(variables_, "internalGet$capitalized_name$FieldBuilder();\n");
}

void ImmutableMessageFieldGenerator::GenerateInitializationCode(
    io::Printer* printer) const {}

void ImmutableMessageFieldGenerator::GenerateBuilderClearCode(
    io::Printer* printer) const {
  // No need to clear the has-bit since we clear the bitField ints all at once.
  printer->Print(variables_,
                 "$name$_ = null;\n"
                 "if ($name$Builder_ != null) {\n"
                 "  $name$Builder_.dispose();\n"
                 "  $name$Builder_ = null;\n"
                 "}\n");
}

void ImmutableMessageFieldGenerator::GenerateMergingCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "if (other.has$capitalized_name$()) {\n"
                 "  merge$capitalized_name$(other.get$capitalized_name$());\n"
                 "}\n");
}

void ImmutableMessageFieldGenerator::GenerateBuildingCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "if ($get_has_field_bit_from_local$) {\n"
                 "  result.$name$_ = $name$Builder_ == null\n"
                 "      ? $name$_\n"
                 "      : $name$Builder_.build();\n");
  if (GetNumBits() > 0) {
    printer->Print(variables_, "  $set_has_field_bit_to_local$;\n");
  }
  printer->Print("}\n");
}

void ImmutableMessageFieldGenerator::GenerateBuilderParsingCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "parse$capitalized_name$Field(input, extensionRegistry);\n");
}

void ImmutableMessageFieldGenerator::GenerateSerializationCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "if ($is_field_present$) {\n"
                 "  write$capitalized_name$Field(output);\n"
                 "}\n");
}

void ImmutableMessageFieldGenerator::GenerateSerializedSizeCode(
    io::Printer* printer) const {
  printer->Print(
      variables_,
      "if ($is_field_present$) {\n"
      "  size += com.google.protobuf.CodedOutputStream\n"
      "    .compute$group_or_message$Size($number$, get$capitalized_name$());\n"
      "}\n");
}

void ImmutableMessageFieldGenerator::GenerateEqualsCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "if (!get$capitalized_name$()\n"
                 "    .equals(other.get$capitalized_name$())) return false;\n");
}

void ImmutableMessageFieldGenerator::GenerateHashCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "hash = (37 * hash) + $constant_name$;\n"
                 "hash = (53 * hash) + get$capitalized_name$().hashCode();\n");
}

std::string ImmutableMessageFieldGenerator::GetBoxedType() const {
  return name_resolver_->GetImmutableClassName(descriptor_->message_type());
}

// ===================================================================

ImmutableMessageOneofFieldGenerator::ImmutableMessageOneofFieldGenerator(
    const FieldDescriptor* descriptor, int bit_index, Context* context)
    : ImmutableMessageFieldGenerator(descriptor, bit_index, context) {
  const OneofGeneratorInfo* info =
      context->GetOneofGeneratorInfo(descriptor->containing_oneof());
  SetCommonOneofVariables(descriptor, info, &variables_);
}

ImmutableMessageOneofFieldGenerator::~ImmutableMessageOneofFieldGenerator() =
    default;

void ImmutableMessageOneofFieldGenerator::GenerateHasMethod(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, HAZZER,
                               context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
                 "  return $has_oneof_case_message$;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void ImmutableMessageOneofFieldGenerator::GenerateGetMethod(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, GETTER,
                               context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public $type$ ${$get$capitalized_name$$}$() {\n"
                 "  if ($has_oneof_case_message$) {\n"
                 "     return ($type$) $oneof_name$_;\n"
                 "  }\n"
                 "  return $type$.getDefaultInstance();\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void ImmutableMessageOneofFieldGenerator::GenerateGetOrBuilderMethod(
    io::Printer* printer) const {
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public $type$OrBuilder "
                 "${$get$capitalized_name$OrBuilder$}$() {\n"
                 "  if ($has_oneof_case_message$) {\n"
                 "     return ($type$) $oneof_name$_;\n"
                 "  }\n"
                 "  return $type$.getDefaultInstance();\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void ImmutableMessageOneofFieldGenerator::GenerateMembers(
    io::Printer* printer) const {
  PrintExtraFieldInfo(variables_, printer);
  GenerateHasMethod(printer);
  GenerateGetMethod(printer);
  GenerateGetOrBuilderMethod(printer);
  GenerateWriteFieldMethod(printer);
}

void ImmutableMessageOneofFieldGenerator::GenerateWriteFieldMethod(
    io::Printer* printer) const {
  if (GetType(descriptor_) == FieldDescriptor::TYPE_GROUP) {
    printer->Print(variables_,
                   "private void write$capitalized_name$Field(\n"
                   "    com.google.protobuf.CodedOutputStream output)\n"
                   "    throws java.io.IOException {\n"
                   "  output.writeGroup($number$, ($type$) $oneof_name$_);\n"
                   "}\n");
  } else {
    printer->Print(variables_,
                   "private void write$capitalized_name$Field(\n"
                   "    com.google.protobuf.CodedOutputStream output)\n"
                   "    throws java.io.IOException {\n"
                   "  $type$ tmp = ($type$) $oneof_name$_;\n"
                   "  output.writeUInt32NoTag($tag$);\n"
                   "  output.writeUInt32NoTag(tmp.getSerializedSize());\n"
                   "  tmp.writeTo(output);\n"
                   "}\n");
  }
}

void ImmutableMessageOneofFieldGenerator::GenerateBuilderHasMethod(
    io::Printer* printer) const {
  // boolean hasField()
  WriteFieldAccessorDocComment(printer, descriptor_, HAZZER,
                               context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
                 "  return $has_oneof_case_message$;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void ImmutableMessageOneofFieldGenerator::GenerateBuilderGetMethod(
    io::Printer* printer) const {
  // Field getField()
  WriteFieldAccessorDocComment(printer, descriptor_, GETTER,
                               context_->options());
  PrintNestedBuilderFunction(
      printer,
      "@java.lang.Override\n"
      "$deprecation$public $type$ ${$get$capitalized_name$$}$()",

      "if ($has_oneof_case_message$) {\n"
      "  return ($type$) $oneof_name$_;\n"
      "}\n"
      "return $type$.getDefaultInstance();\n",

      "if ($has_oneof_case_message$) {\n"
      "  return $name$Builder_.getMessage();\n"
      "}\n"
      "return $type$.getDefaultInstance();\n",

      nullptr);
}

void ImmutableMessageOneofFieldGenerator::GenerateBuilderSetMethod(
    io::Printer* printer) const {
  // Field.Builder setField(Field value)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  PrintNestedBuilderFunction(
      printer,
      "$deprecation$public Builder ${$set$capitalized_name$$}$($type$ value)",

      "java.util.Objects.requireNonNull(value);\n"
      "$oneof_name$_ = value;\n"
      "$on_changed$\n",

      "$name$Builder_.setMessage(value);\n",

      "$set_oneof_case_message$;\n"
      "return this;\n",
      Semantic::kSet);
}

void ImmutableMessageOneofFieldGenerator::GenerateBuilderSetBuilderMethod(
    io::Printer* printer) const {
  // Field.Builder setField(Field.Builder builderForValue)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$public Builder ${$set$capitalized_name$$}$(\n"
                 "    $type$.Builder builderForValue) {\n"
                 "  java.util.Objects.requireNonNull(builderForValue);\n"
                 "  return set$capitalized_name$(builderForValue.build());\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void ImmutableMessageOneofFieldGenerator::GenerateBuilderMergeMethod(
    io::Printer* printer) const {
  // Field.Builder mergeField(Field value)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  PrintNestedBuilderFunction(
      printer,
      "$deprecation$public Builder ${$merge$capitalized_name$$}$($type$ value)",

      "if ($has_oneof_case_message$ &&\n"
      "    $oneof_name$_ != $type$.getDefaultInstance()) {\n"
      "  $oneof_name$_ = $type$.newBuilder(($type$) $oneof_name$_)\n"
      "      .mergeFrom(value).buildPartial();\n"
      "} else {\n"
      "  $oneof_name$_ = value;\n"
      "}\n"
      "$on_changed$\n",

      "if ($has_oneof_case_message$) {\n"
      "  $name$Builder_.mergeFrom(value);\n"
      "} else {\n"
      "  $name$Builder_.setMessage(value);\n"
      "}\n",

      "$set_oneof_case_message$;\n"
      "return this;\n",
      Semantic::kSet);
}

void ImmutableMessageOneofFieldGenerator::GenerateBuilderClearMethod(
    io::Printer* printer) const {
  // Field.Builder clearField()
  WriteFieldDocComment(printer, descriptor_, context_->options());
  PrintNestedBuilderFunction(
      printer, "$deprecation$public Builder ${$clear$capitalized_name$$}$()",

      "if ($has_oneof_case_message$) {\n"
      "  $clear_oneof_case_message$;\n"
      "  $oneof_name$_ = null;\n"
      "  $on_changed$\n"
      "}\n",

      "if ($has_oneof_case_message$) {\n"
      "  $clear_oneof_case_message$;\n"
      "  $oneof_name$_ = null;\n"
      "}\n"
      "$name$Builder_.clear();\n",

      "return this;\n", Semantic::kSet);
}

void ImmutableMessageOneofFieldGenerator::GenerateBuilderGetBuilderMethod(
    io::Printer* printer) const {
  // $type$.Builder getFieldBuilder
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$public $type$.Builder "
                 "${$get$capitalized_name$Builder$}$() {\n"
                 "  return "
                 "internalGet$capitalized_name$FieldBuilder().getBuilder();\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void ImmutableMessageOneofFieldGenerator::GenerateBuilderGetOrBuilderMethod(
    io::Printer* printer) const {
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public $type$OrBuilder "
      "${$get$capitalized_name$OrBuilder$}$() {\n"
      "  if (($has_oneof_case_message$) && ($name$Builder_ != null)) {\n"
      "    return $name$Builder_.getMessageOrBuilder();\n"
      "  } else {\n"
      "    if ($has_oneof_case_message$) {\n"
      "      return ($type$) $oneof_name$_;\n"
      "    }\n"
      "    return $type$.getDefaultInstance();\n"
      "  }\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void ImmutableMessageOneofFieldGenerator::
    GenerateBuilderInternalGetFieldBuilderMethod(io::Printer* printer) const {
  // SingleFieldBuilder internalGetFieldFieldBuilder
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "private com.google.protobuf.SingleFieldBuilder<\n"
      "    $type$, $type$.Builder, $type$OrBuilder> \n"
      "    ${$internalGet$capitalized_name$FieldBuilder$}$() {\n"
      "  if ($name$Builder_ == null) {\n"
      "    if (!($has_oneof_case_message$)) {\n"
      "      $oneof_name$_ = $type$.getDefaultInstance();\n"
      "    }\n"
      "    $name$Builder_ = new com.google.protobuf.SingleFieldBuilder<\n"
      "        $type$, $type$.Builder, $type$OrBuilder>(\n"
      "            ($type$) $oneof_name$_,\n"
      "            getParentForChildren(),\n"
      "            isClean());\n"
      "    $oneof_name$_ = null;\n"
      "  }\n"
      "  $set_oneof_case_message$;\n"
      "  $on_changed$\n"
      "  return $name$Builder_;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void ImmutableMessageOneofFieldGenerator::GenerateBuilderMembers(
    io::Printer* printer) const {
  // When using nested-builders, the code initially works just like the
  // non-nested builder case. It only creates a nested builder lazily on
  // demand and then forever delegates to it after creation.
  printer->Print(variables_,
                 // If this builder is non-null, it is used and the other fields
                 // are ignored.
                 "private com.google.protobuf.SingleFieldBuilder<\n"
                 "    $type$, $type$.Builder, $type$OrBuilder> $name$Builder_;"
                 "\n");

  GenerateBuilderHasMethod(printer);
  GenerateBuilderGetMethod(printer);
  GenerateBuilderSetMethod(printer);
  GenerateBuilderSetBuilderMethod(printer);
  GenerateBuilderMergeMethod(printer);
  GenerateBuilderClearMethod(printer);
  GenerateBuilderGetBuilderMethod(printer);
  GenerateBuilderGetOrBuilderMethod(printer);
  GenerateBuilderInternalGetFieldBuilderMethod(printer);
}

void ImmutableMessageOneofFieldGenerator::GenerateBuilderClearCode(
    io::Printer* printer) const {
  // Make sure the builder gets cleared.
  printer->Print(variables_,
                 "if ($name$Builder_ != null) {\n"
                 "  $name$Builder_.clear();\n"
                 "}\n");
}

void ImmutableMessageOneofFieldGenerator::GenerateBuildingCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "if ($has_oneof_case_message$ &&\n"
                 "    $name$Builder_ != null) {\n"
                 "  result.$oneof_name$_ = $name$Builder_.build();\n"
                 "}\n");
}

void ImmutableMessageOneofFieldGenerator::GenerateMergingCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "merge$capitalized_name$(other.get$capitalized_name$());\n");
}

void ImmutableMessageOneofFieldGenerator::GenerateBuilderParsingCode(
    io::Printer* printer) const {
  if (GetType(descriptor_) == FieldDescriptor::TYPE_GROUP) {
    printer->Print(variables_,
                   "input.readGroup($number$,\n"
                   "    "
                   "internalGet$capitalized_name$FieldBuilder().getBuilder(),\n"
                   "    extensionRegistry);\n"
                   "$set_oneof_case_message$;\n");
  } else {
    printer->Print(variables_,
                   "input.readMessage(\n"
                   "    "
                   "internalGet$capitalized_name$FieldBuilder().getBuilder(),\n"
                   "    extensionRegistry);\n"
                   "$set_oneof_case_message$;\n");
  }
}

void ImmutableMessageOneofFieldGenerator::GenerateSerializationCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "if ($has_oneof_case_message$) {\n"
                 "  write$capitalized_name$Field(output);\n"
                 "}\n");
}

void ImmutableMessageOneofFieldGenerator::GenerateSerializedSizeCode(
    io::Printer* printer) const {
  printer->Print(
      variables_,
      "if ($has_oneof_case_message$) {\n"
      "  size += com.google.protobuf.CodedOutputStream\n"
      "    .compute$group_or_message$Size($number$, ($type$) $oneof_name$_);\n"
      "}\n");
}

// ===================================================================

RepeatedImmutableMessageFieldGenerator::RepeatedImmutableMessageFieldGenerator(
    const FieldDescriptor* descriptor, int bit_index, Context* context)
    : ImmutableMessageFieldGenerator(descriptor, bit_index, context) {}

RepeatedImmutableMessageFieldGenerator::
    ~RepeatedImmutableMessageFieldGenerator() = default;

void RepeatedImmutableMessageFieldGenerator::GenerateInterfaceGetListMethod(
    io::Printer* printer) const {
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$java.util.List<$type$> \n"
                 "    get$capitalized_name$List();\n");
}

void RepeatedImmutableMessageFieldGenerator::GenerateInterfaceGetCountMethod(
    io::Printer* printer) const {
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$int get$capitalized_name$Count();\n");
}

void RepeatedImmutableMessageFieldGenerator::GenerateInterfaceGetMethod(
    io::Printer* printer) const {
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$$type$ get$capitalized_name$(int index);\n");
}

void RepeatedImmutableMessageFieldGenerator::
    GenerateInterfaceGetOrBuilderListMethod(io::Printer* printer) const {
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$java.util.List<? extends $type$OrBuilder> \n"
                 "    get$capitalized_name$OrBuilderList();\n");
}

void RepeatedImmutableMessageFieldGenerator::
    GenerateInterfaceGetOrBuilderMethod(io::Printer* printer) const {
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "$deprecation$$type$OrBuilder get$capitalized_name$OrBuilder(\n"
      "    int index);\n");
}

void RepeatedImmutableMessageFieldGenerator::GenerateInterfaceMembers(
    io::Printer* printer) const {
  // TODO: In the future, consider having methods specific to the
  // interface so that builders can choose dynamically to either return a
  // message or a nested builder, so that asking for the interface doesn't
  // cause a message to ever be built.
  GenerateInterfaceGetListMethod(printer);
  GenerateInterfaceGetMethod(printer);
  GenerateInterfaceGetCountMethod(printer);
  GenerateInterfaceGetOrBuilderListMethod(printer);
  GenerateInterfaceGetOrBuilderMethod(printer);
}

void RepeatedImmutableMessageFieldGenerator::GenerateGetListMethod(
    io::Printer* printer) const {
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public java.util.List<$type$> "
                 "${$get$capitalized_name$List$}$() {\n"
                 "  return $name$_;\n"  // note:  unmodifiable list
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void RepeatedImmutableMessageFieldGenerator::GenerateGetOrBuilderListMethod(
    io::Printer* printer) const {
  // List<FieldOrBuilder> getFieldOrBuilderList()
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public java.util.List<? extends $type$OrBuilder> \n"
      "    ${$get$capitalized_name$OrBuilderList$}$() {\n"
      "  return $name$_;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void RepeatedImmutableMessageFieldGenerator::GenerateGetCountMethod(
    io::Printer* printer) const {
  // int getFieldCount()
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public int ${$get$capitalized_name$Count$}$() {\n"
      "  return $name$_.size();\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void RepeatedImmutableMessageFieldGenerator::GenerateGetMethod(
    io::Printer* printer) const {
  // Field getField(int index)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public $type$ ${$get$capitalized_name$$}$(int index) {\n"
      "  return $name$_.get(index);\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void RepeatedImmutableMessageFieldGenerator::GenerateGetOrBuilderMethod(
    io::Printer* printer) const {
  // FieldOrBuilder getFieldOrBuilder(int index)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public $type$OrBuilder "
                 "${$get$capitalized_name$OrBuilder$}$(\n"
                 "    int index) {\n"
                 "  return $name$_.get(index);\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void RepeatedImmutableMessageFieldGenerator::GenerateMembers(
    io::Printer* printer) const {
  printer->Print(
      variables_,
      "private com.google.protobuf.Internal.ProtobufList<$type$> $name$_ =\n"
      "    $empty_list$;\n");
  PrintExtraFieldInfo(variables_, printer);
  GenerateGetListMethod(printer);
  GenerateGetOrBuilderListMethod(printer);
  GenerateGetCountMethod(printer);
  GenerateGetMethod(printer);
  GenerateGetOrBuilderMethod(printer);
  GenerateWriteFieldMethod(printer);
}

void RepeatedImmutableMessageFieldGenerator::GenerateWriteFieldMethod(
    io::Printer* printer) const {
  if (GetType(descriptor_) == FieldDescriptor::TYPE_GROUP) {
    printer->Print(variables_,
                   "private void write$capitalized_name$Field(\n"
                   "    com.google.protobuf.CodedOutputStream output)\n"
                   "    throws java.io.IOException {\n"
                   "  for (int i = 0; i < $name$_.size(); i++) {\n"
                   "    output.writeGroup($number$, $name$_.get(i));\n"
                   "  }\n"
                   "}\n");
  } else {
    printer->Print(variables_,
                   "private void write$capitalized_name$Field(\n"
                   "    com.google.protobuf.CodedOutputStream output)\n"
                   "    throws java.io.IOException {\n"
                   "  for (int i = 0; i < $name$_.size(); i++) {\n"
                   "    $type$ tmp = $name$_.get(i);\n"
                   "    output.writeUInt32NoTag($tag$);\n"
                   "    output.writeUInt32NoTag(tmp.getSerializedSize());\n"
                   "    tmp.writeTo(output);\n"
                   "  }\n"
                   "}\n");
  }
}

void RepeatedImmutableMessageFieldGenerator::GenerateEnsureIsMutableMethod(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "private void ensure$capitalized_name$IsMutable() {\n"
                 "  if (!$name$_.isModifiable()) {\n"
                 "    $name$_ = makeMutableCopy($name$_);\n"
                 "  }\n"
                 "  $set_has_field_bit$\n"
                 "}\n");
}

void RepeatedImmutableMessageFieldGenerator::GenerateBuilderGetListMethod(
    io::Printer* printer) const {
  // List<Field> getRepeatedFieldList()
  WriteFieldDocComment(printer, descriptor_, context_->options());
  PrintNestedBuilderFunction(printer,
                             "$deprecation$public java.util.List<$type$> "
                             "${$get$capitalized_name$List$}$()",

                             "$name$_.makeImmutable();\n"
                             "return $name$_;\n",

                             "return $name$Builder_.getMessageList();\n",

                             nullptr);
}

void RepeatedImmutableMessageFieldGenerator::GenerateBuilderGetCountMethod(
    io::Printer* printer) const {
  // int getRepeatedFieldCount()
  WriteFieldDocComment(printer, descriptor_, context_->options());
  PrintNestedBuilderFunction(
      printer, "$deprecation$public int ${$get$capitalized_name$Count$}$()",

      "return $name$_.size();\n", "return $name$Builder_.getCount();\n",

      nullptr);
}

void RepeatedImmutableMessageFieldGenerator::GenerateBuilderGetMethod(
    io::Printer* printer) const {
  // Field getRepeatedField(int index)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  PrintNestedBuilderFunction(
      printer,
      "$deprecation$public $type$ ${$get$capitalized_name$$}$(int index)",

      "return $name$_.get(index);\n",

      "return $name$Builder_.getMessage(index);\n",

      nullptr);
}

void RepeatedImmutableMessageFieldGenerator::GenerateBuilderSetMethod(
    io::Printer* printer) const {
  // Builder setRepeatedField(int index, Field value)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  PrintNestedBuilderFunction(
      printer,
      "$deprecation$public Builder ${$set$capitalized_name$$}$(\n"
      "    int index, $type$ value)",
      "java.util.Objects.requireNonNull(value);\n"
      "ensure$capitalized_name$IsMutable();\n"
      "$name$_.set(index, value);\n"
      "$on_changed$\n",
      "$name$Builder_.setMessage(index, "
      "value);\n$set_has_field_bit$\n$on_changed$\n",
      "return this;\n", Semantic::kSet);
}

void RepeatedImmutableMessageFieldGenerator::GenerateBuilderSetBuilderMethod(
    io::Printer* printer) const {
  // Builder setRepeatedField(int index, Field.Builder builderForValue)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$set$capitalized_name$$}$(\n"
      "    int index, $type$.Builder builderForValue) {\n"
      "  java.util.Objects.requireNonNull(builderForValue);\n"
      "  return set$capitalized_name$(index, builderForValue.build());\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void RepeatedImmutableMessageFieldGenerator::GenerateBuilderAddMethod(
    io::Printer* printer) const {
  // Builder addRepeatedField(Field value)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  PrintNestedBuilderFunction(
      printer,
      "$deprecation$public Builder ${$add$capitalized_name$$}$($type$ value)",

      "java.util.Objects.requireNonNull(value);\n"
      "ensure$capitalized_name$IsMutable();\n"
      "$name$_.add(value);\n"

      "$on_changed$\n",

      "$name$Builder_.addMessage(value);\n"
      "$set_has_field_bit$\n"
      "$on_changed$\n",

      "return this;\n", Semantic::kSet);
}

void RepeatedImmutableMessageFieldGenerator::GenerateBuilderAddAtIndexMethod(
    io::Printer* printer) const {
  // Builder addRepeatedField(int index, Field value)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  PrintNestedBuilderFunction(
      printer,
      "$deprecation$public Builder ${$add$capitalized_name$$}$(\n"
      "    int index, $type$ value)",

      "java.util.Objects.requireNonNull(value);\n"
      "ensure$capitalized_name$IsMutable();\n"
      "$name$_.add(index, value);\n"
      "$on_changed$\n",

      "$name$Builder_.addMessage(index, value);\n"
      "$set_has_field_bit$\n"
      "$on_changed$\n",

      "return this;\n", Semantic::kSet);
}

void RepeatedImmutableMessageFieldGenerator::GenerateBuilderAddBuilderMethod(
    io::Printer* printer) const {
  // Builder addRepeatedField(Field.Builder builderForValue)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$public Builder ${$add$capitalized_name$$}$(\n"
                 "    $type$.Builder builderForValue) {\n"
                 "  java.util.Objects.requireNonNull(builderForValue);\n"
                 "  return add$capitalized_name$(builderForValue.build());\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void RepeatedImmutableMessageFieldGenerator::
    GenerateBuilderAddBuilderAtIndexMethod(io::Printer* printer) const {
  // Builder addRepeatedField(int index, Field.Builder builderForValue)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$add$capitalized_name$$}$(\n"
      "    int index, $type$.Builder builderForValue) {\n"
      "  java.util.Objects.requireNonNull(builderForValue);\n"
      "  return add$capitalized_name$(index, builderForValue.build());\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void RepeatedImmutableMessageFieldGenerator::GenerateBuilderAddAllMethod(
    io::Printer* printer) const {
  // Builder addAllRepeatedField(Iterable<Field> values)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  PrintNestedBuilderFunction(
      printer,
      "$deprecation$public Builder ${$addAll$capitalized_name$$}$(\n"
      "    java.lang.Iterable<? extends $type$> values)",

      "$name$_ = com.google.protobuf.Internal.ProtobufList.concatenate(\n"
      "    $name$_, values);\n",

      "$name$Builder_.addAllMessages(values);\n",

      "if (get$capitalized_name$Count() > 0) {\n"
      "  $set_has_field_bit$;\n"
      "}\n"
      "$on_changed$\n"
      "return this;\n",
      Semantic::kSet);
}

void RepeatedImmutableMessageFieldGenerator::GenerateBuilderClearMethod(
    io::Printer* printer) const {
  // Builder clearRepeatedField()
  WriteFieldDocComment(printer, descriptor_, context_->options());
  PrintNestedBuilderFunction(
      printer, "$deprecation$public Builder ${$clear$capitalized_name$$}$()",

      "$name$_ = $empty_list$;\n",

      "$name$Builder_.clear();\n",

      "$clear_has_field_bit$\n"
      "$on_changed$\n"
      "return this;\n",
      Semantic::kSet);
}

void RepeatedImmutableMessageFieldGenerator::GenerateBuilderRemoveMethod(
    io::Printer* printer) const {
  // Builder removeRepeatedField(int index)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  PrintNestedBuilderFunction(
      printer,
      "$deprecation$public Builder ${$remove$capitalized_name$$}$(int index)",

      "ensure$capitalized_name$IsMutable();\n"
      "$name$_.remove(index);\n",

      "$name$Builder_.remove(index);\n",

      "if (get$capitalized_name$Count() == 0) {\n"
      "  $clear_has_field_bit$;\n"
      "}\n"
      "$on_changed$\n"
      "return this;\n",
      Semantic::kSet);
}

void RepeatedImmutableMessageFieldGenerator::GenerateBuilderGetBuilderMethod(
    io::Printer* printer) const {
  // Field.Builder getRepeatedFieldBuilder(int index)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "$deprecation$public $type$.Builder ${$get$capitalized_name$Builder$}$(\n"
      "    int index) {\n"
      "  return "
      "internalGet$capitalized_name$FieldBuilder().getBuilder(index);"
      "\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void RepeatedImmutableMessageFieldGenerator::GenerateBuilderGetOrBuilderMethod(
    io::Printer* printer) const {
  // FieldOrBuilder getRepeatedFieldOrBuilder(int index)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$public $type$OrBuilder "
                 "${$get$capitalized_name$OrBuilder$}$(\n"
                 "    int index) {\n"
                 "  if ($name$Builder_ == null) {\n"
                 "    return $name$_.get(index);"
                 "  } else {\n"
                 "    return $name$Builder_.getMessageOrBuilder(index);\n"
                 "  }\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void RepeatedImmutableMessageFieldGenerator::
    GenerateBuilderGetOrBuilderListMethod(io::Printer* printer) const {
  // List<FieldOrBuilder> getRepeatedFieldOrBuilderList()
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "$deprecation$public java.util.List<? extends $type$OrBuilder> \n"
      "     ${$get$capitalized_name$OrBuilderList$}$() {\n"
      "  if ($name$Builder_ != null) {\n"
      "    return $name$Builder_.getMessageOrBuilderList();\n"
      "  } else {\n"
      "    $name$_.makeImmutable();\n"
      "    return $name$_;\n"
      "  }\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void RepeatedImmutableMessageFieldGenerator::
    GenerateBuilderAddBuilderNoArgsMethod(io::Printer* printer) const {
  // Field.Builder addRepeatedFieldBuilder()
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$public $type$.Builder "
                 "${$add$capitalized_name$Builder$}$() {\n"
                 "  $set_has_field_bit$\n"
                 "  $on_changed$\n"
                 "  return "
                 "internalGet$capitalized_name$FieldBuilder().addBuilder(\n"
                 "      $type$.getDefaultInstance());\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void RepeatedImmutableMessageFieldGenerator::
    GenerateBuilderAddBuilderAtIndexNoArgsMethod(io::Printer* printer) const {
  // Field.Builder addRepeatedFieldBuilder(int index)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "$deprecation$public $type$.Builder ${$add$capitalized_name$Builder$}$(\n"
      "    int index) {\n"
      "  $set_has_field_bit$\n"
      "  $on_changed$\n"
      "  return "
      "internalGet$capitalized_name$FieldBuilder().addBuilder(\n"
      "      index, $type$.getDefaultInstance());\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void RepeatedImmutableMessageFieldGenerator::
    GenerateBuilderGetBuilderListMethod(io::Printer* printer) const {
  // List<Field.Builder> getRepeatedFieldBuilderList()
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$public java.util.List<$type$.Builder> \n"
                 "     ${$get$capitalized_name$BuilderList$}$() {\n"
                 "  return "
                 "internalGet$capitalized_name$FieldBuilder()."
                 "getBuilderList();\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void RepeatedImmutableMessageFieldGenerator::
    GenerateBuilderInternalGetFieldBuilderMethod(io::Printer* printer) const {
  printer->Print(variables_,
                 "private com.google.protobuf.RepeatedFieldBuilder<\n"
                 "    $type$, $type$.Builder, $type$OrBuilder> \n"
                 "    internalGet$capitalized_name$FieldBuilder() {\n"
                 "  if ($name$Builder_ == null) {\n"
                 "    $name$Builder_ = new "
                 "com.google.protobuf.RepeatedFieldBuilder<\n"
                 "        $type$, $type$.Builder, $type$OrBuilder>(\n"
                 "            $name$_,\n"
                 "            $name$_.isModifiable(),\n"
                 "            getParentForChildren(),\n"
                 "            isClean());\n"
                 "    $name$_ = null;\n"
                 "  }\n"
                 "  $set_has_field_bit$\n"
                 "  $on_changed$\n"
                 "  return $name$Builder_;\n"
                 "}\n");
}

void RepeatedImmutableMessageFieldGenerator::GenerateBuilderMembers(
    io::Printer* printer) const {
  printer->Print(
      variables_,
      "private com.google.protobuf.Internal.ProtobufList<$type$> $name$_ =\n"
      "    emptyList($type$.class);\n");

  GenerateEnsureIsMutableMethod(printer);

  printer->Print(
      variables_,
      // If this builder is non-null, it is used and the other fields are
      // ignored.
      "private com.google.protobuf.RepeatedFieldBuilder<\n"
      "    $type$, $type$.Builder, $type$OrBuilder> $name$Builder_;\n"
      "\n");

  GenerateBuilderGetListMethod(printer);
  GenerateBuilderGetCountMethod(printer);
  GenerateBuilderGetMethod(printer);
  GenerateBuilderSetMethod(printer);
  GenerateBuilderSetBuilderMethod(printer);
  GenerateBuilderAddMethod(printer);
  GenerateBuilderAddAtIndexMethod(printer);
  GenerateBuilderAddBuilderMethod(printer);
  GenerateBuilderAddBuilderAtIndexMethod(printer);
  GenerateBuilderAddAllMethod(printer);
  GenerateBuilderClearMethod(printer);
  GenerateBuilderRemoveMethod(printer);
  GenerateBuilderGetBuilderMethod(printer);
  GenerateBuilderGetOrBuilderMethod(printer);
  GenerateBuilderGetOrBuilderListMethod(printer);
  GenerateBuilderAddBuilderNoArgsMethod(printer);
  GenerateBuilderAddBuilderAtIndexNoArgsMethod(printer);
  GenerateBuilderGetBuilderListMethod(printer);
  GenerateBuilderInternalGetFieldBuilderMethod(printer);
}
void RepeatedImmutableMessageFieldGenerator::
    GenerateFieldBuilderInitializationCode(io::Printer* printer) const {
  printer->Print(variables_, "internalGet$capitalized_name$FieldBuilder();\n");
}

void RepeatedImmutableMessageFieldGenerator::GenerateInitializationCode(
    io::Printer* printer) const {
  printer->Print(variables_, "$name$_ = $empty_list$;\n");
}

void RepeatedImmutableMessageFieldGenerator::GenerateBuilderClearCode(
    io::Printer* printer) const {
  PrintNestedBuilderCondition(printer, "$name$_ = $empty_list$;\n",

                              "$name$_ = null;\n"
                              "$name$Builder_.clear();\n");
  printer->Print(variables_, "$clear_has_field_bit$;\n");
}

void RepeatedImmutableMessageFieldGenerator::GenerateMergingCode(
    io::Printer* printer) const {
  // The code below does two optimizations (non-nested builder case):
  //   1. If the other list is empty, there's nothing to do. This ensures we
  //      don't allocate a new array if we already have an immutable one.
  //   2. If the other list is non-empty and our current list is empty, we can
  //      reuse the other list which is guaranteed to be immutable.
  PrintNestedBuilderCondition(
      printer,
      "if (!other.$name$_.isEmpty()) {\n"
      "  if ($name$_.isEmpty()) {\n"
      "    $name$_ = other.$name$_;\n"
      "    $set_has_field_bit$\n"
      "  } else {\n"
      "    ensure$capitalized_name$IsMutable();\n"
      "    $name$_.addAll(other.$name$_);\n"
      "  }\n"
      "  $on_changed$\n"
      "}\n",

      "if (!other.$name$_.isEmpty()) {\n"
      "  if ($name$Builder_.isEmpty()) {\n"
      "    $name$Builder_.dispose();\n"
      "    $name$Builder_ = null;\n"
      "    $name$_ = other.$name$_;\n"
      "    $set_has_field_bit$\n"
      "    $name$Builder_ = \n"
      "      com.google.protobuf.GeneratedMessage.alwaysUseFieldBuilders "
      "?\n"
      "         internalGet$capitalized_name$FieldBuilder() : null;\n"
      "  } else {\n"
      "    $name$Builder_.addAllMessages(other.$name$_);\n"
      "    $set_has_field_bit$\n"
      "  }\n"
      "  $on_changed$\n"
      "}\n");
}

void RepeatedImmutableMessageFieldGenerator::GenerateBuildingCode(
    io::Printer* printer) const {
  // The code below (non-nested builder case) ensures that the result has an
  // immutable list. If our list is immutable, we can just reuse it. If not,
  // we make it immutable.
  printer->Print(variables_, "if ($get_has_field_bit_from_local$) {\n");
  PrintNestedBuilderCondition(
      printer,
      "$name$_.makeImmutable();\n"
      "result.$name$_ = $name$_;\n",

      "result.$name$_ = $name$Builder_.buildProtobufList();\n");
  printer->Print("}\n");
}

void RepeatedImmutableMessageFieldGenerator::GenerateBuilderParsingCode(
    io::Printer* printer) const {
  if (GetType(descriptor_) == FieldDescriptor::TYPE_GROUP) {
    printer->Print(variables_,
                   "$type$ m =\n"
                   "    input.readGroup($number$,\n"
                   "        $type$.$get_parser$,\n"
                   "        extensionRegistry);\n");
  } else {
    printer->Print(variables_,
                   "final int oldLimit = input.pushLimitBeforeMessage();\n"
                   "$type$ m = $type$.parser().parsePartialFrom(input, "
                   "extensionRegistry);\n"
                   "input.popLimitAfterMessage(oldLimit);\n");
  }
  PrintNestedBuilderCondition(printer,
                              "ensure$capitalized_name$IsMutable();\n"
                              "$name$_.add(m);\n",
                              "$name$Builder_.addMessage(m);\n"
                              "$set_has_field_bit$\n"
                              "$on_changed$\n");
}

void RepeatedImmutableMessageFieldGenerator::GenerateSerializationCode(
    io::Printer* printer) const {
  printer->Print(variables_, "write$capitalized_name$Field(output);\n");
}

void RepeatedImmutableMessageFieldGenerator::GenerateSerializedSizeCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 R"java(
    {
      final int count = $name$_.size();
      for (int i = 0; i < count; i++) {
        size += com.google.protobuf.CodedOutputStream
          .compute$group_or_message$SizeNoTag($name$_.get(i));
      }
      size += $tag_size$ * count;
    }
    )java");
}

void RepeatedImmutableMessageFieldGenerator::GenerateEqualsCode(
    io::Printer* printer) const {
  printer->Print(
      variables_,
      "if (!get$capitalized_name$List()\n"
      "    .equals(other.get$capitalized_name$List())) return false;\n");
}

void RepeatedImmutableMessageFieldGenerator::GenerateHashCode(
    io::Printer* printer) const {
  printer->Print(
      variables_,
      "if (get$capitalized_name$Count() > 0) {\n"
      "  hash = (37 * hash) + $constant_name$;\n"
      "  hash = (53 * hash) + get$capitalized_name$List().hashCode();\n"
      "}\n");
}

std::string RepeatedImmutableMessageFieldGenerator::GetBoxedType() const {
  return name_resolver_->GetImmutableClassName(descriptor_->message_type());
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
