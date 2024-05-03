// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/java/full/enum_field.h"

#include <cstdint>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/str_cat.h"
#include "google/protobuf/compiler/java/context.h"
#include "google/protobuf/compiler/java/doc_comment.h"
#include "google/protobuf/compiler/java/field_common.h"
#include "google/protobuf/compiler/java/helpers.h"
#include "google/protobuf/compiler/java/internal_helpers.h"
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

void SetEnumVariables(
    const FieldDescriptor* descriptor, int messageBitIndex, int builderBitIndex,
    const FieldGeneratorInfo* info, ClassNameResolver* name_resolver,
    absl::flat_hash_map<absl::string_view, std::string>* variables,
    Context* context) {
  SetCommonFieldVariables(descriptor, info, variables);

  (*variables)["type"] =
      name_resolver->GetImmutableClassName(descriptor->enum_type());
  variables->insert({"kt_type", EscapeKotlinKeywords((*variables)["type"])});
  (*variables)["mutable_type"] =
      name_resolver->GetMutableClassName(descriptor->enum_type());
  (*variables)["default"] =
      ImmutableDefaultValue(descriptor, name_resolver, context->options());
  (*variables)["default_number"] =
      absl::StrCat(descriptor->default_value_enum()->number());
  (*variables)["tag"] = absl::StrCat(
      static_cast<int32_t>(internal::WireFormat::MakeTag(descriptor)));
  (*variables)["tag_size"] = absl::StrCat(
      internal::WireFormat::TagSize(descriptor->number(), GetType(descriptor)));
  // TODO: Add @deprecated javadoc when generating javadoc is supported
  // by the proto compiler
  (*variables)["deprecation"] =
      descriptor->options().deprecated() ? "@java.lang.Deprecated " : "";
  variables->insert(
      {"kt_deprecation",
       descriptor->options().deprecated()
           ? absl::StrCat("@kotlin.Deprecated(message = \"Field ",
                          (*variables)["name"], " is deprecated\") ")
           : ""});
  if (HasHasbit(descriptor)) {
    // For singular messages and builders, one bit is used for the hasField bit.
    (*variables)["get_has_field_bit_message"] = GenerateGetBit(messageBitIndex);
    // Note that these have a trailing ";".
    (*variables)["set_has_field_bit_message"] =
        absl::StrCat(GenerateSetBit(messageBitIndex), ";");
    (*variables)["set_has_field_bit_to_local"] =
        GenerateSetBitToLocal(messageBitIndex);
    (*variables)["is_field_present_message"] = GenerateGetBit(messageBitIndex);
  } else {
    (*variables)["set_has_field_bit_message"] = "";
    (*variables)["set_has_field_bit_to_local"] = "";
    variables->insert({"is_field_present_message",
                       absl::StrCat((*variables)["name"], "_ != ",
                                    (*variables)["default"], ".getNumber()")});
  }

  // For repeated builders, one bit is used for whether the array is immutable.
  (*variables)["get_mutable_bit_builder"] = GenerateGetBit(builderBitIndex);
  (*variables)["set_mutable_bit_builder"] = GenerateSetBit(builderBitIndex);
  (*variables)["clear_mutable_bit_builder"] = GenerateClearBit(builderBitIndex);

  (*variables)["get_has_field_bit_builder"] = GenerateGetBit(builderBitIndex);

  // Note that these have a trailing ";".
  (*variables)["set_has_field_bit_builder"] =
      absl::StrCat(GenerateSetBit(builderBitIndex), ";");
  (*variables)["clear_has_field_bit_builder"] =
      absl::StrCat(GenerateClearBit(builderBitIndex), ";");
  (*variables)["get_has_field_bit_from_local"] =
      GenerateGetBitFromLocal(builderBitIndex);

  if (SupportUnknownEnumValue(descriptor)) {
    variables->insert(
        {"unknown", absl::StrCat((*variables)["type"], ".UNRECOGNIZED")});
  } else {
    variables->insert({"unknown", (*variables)["default"]});
  }
}

}  // namespace

// ===================================================================

ImmutableEnumFieldGenerator::ImmutableEnumFieldGenerator(
    const FieldDescriptor* descriptor, int messageBitIndex, int builderBitIndex,
    Context* context)
    : descriptor_(descriptor),
      message_bit_index_(messageBitIndex),
      builder_bit_index_(builderBitIndex),
      context_(context),
      name_resolver_(context->GetNameResolver()) {
  SetEnumVariables(descriptor, messageBitIndex, builderBitIndex,
                   context->GetFieldGeneratorInfo(descriptor), name_resolver_,
                   &variables_, context);
}

ImmutableEnumFieldGenerator::~ImmutableEnumFieldGenerator() {}

int ImmutableEnumFieldGenerator::GetMessageBitIndex() const {
  return message_bit_index_;
}

int ImmutableEnumFieldGenerator::GetBuilderBitIndex() const {
  return builder_bit_index_;
}

int ImmutableEnumFieldGenerator::GetNumBitsForMessage() const {
  return HasHasbit(descriptor_) ? 1 : 0;
}

int ImmutableEnumFieldGenerator::GetNumBitsForBuilder() const { return 1; }

void ImmutableEnumFieldGenerator::GenerateInterfaceMembers(
    io::Printer* printer) const {
  if (descriptor_->has_presence()) {
    WriteFieldAccessorDocComment(printer, descriptor_, HAZZER,
                                 context_->options());
    printer->Print(variables_,
                   "$deprecation$boolean has$capitalized_name$();\n");
  }
  if (SupportUnknownEnumValue(descriptor_)) {
    WriteFieldEnumValueAccessorDocComment(printer, descriptor_, GETTER,
                                          context_->options());
    printer->Print(variables_,
                   "$deprecation$int get$capitalized_name$Value();\n");
  }
  WriteFieldAccessorDocComment(printer, descriptor_, GETTER,
                               context_->options());
  printer->Print(variables_, "$deprecation$$type$ get$capitalized_name$();\n");
}

void ImmutableEnumFieldGenerator::GenerateMembers(io::Printer* printer) const {
  printer->Print(variables_, "private int $name$_ = $default_number$;\n");
  PrintExtraFieldInfo(variables_, printer);
  if (descriptor_->has_presence()) {
    WriteFieldAccessorDocComment(printer, descriptor_, HAZZER,
                                 context_->options());
    printer->Print(variables_,
                   "@java.lang.Override $deprecation$public boolean "
                   "${$has$capitalized_name$$}$() {\n"
                   "  return $get_has_field_bit_message$;\n"
                   "}\n");
    printer->Annotate("{", "}", descriptor_);
  }
  if (SupportUnknownEnumValue(descriptor_)) {
    WriteFieldEnumValueAccessorDocComment(printer, descriptor_, GETTER,
                                          context_->options());
    printer->Print(variables_,
                   "@java.lang.Override $deprecation$public int "
                   "${$get$capitalized_name$Value$}$() {\n"
                   "  return $name$_;\n"
                   "}\n");
    printer->Annotate("{", "}", descriptor_);
  }
  WriteFieldAccessorDocComment(printer, descriptor_, GETTER,
                               context_->options());
  printer->Print(variables_,
                 "@java.lang.Override $deprecation$public $type$ "
                 "${$get$capitalized_name$$}$() {\n"
                 "  $type$ result = $type$.forNumber($name$_);\n"
                 "  return result == null ? $unknown$ : result;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void ImmutableEnumFieldGenerator::GenerateBuilderMembers(
    io::Printer* printer) const {
  printer->Print(variables_, "private int $name$_ = $default_number$;\n");
  if (descriptor_->has_presence()) {
    WriteFieldAccessorDocComment(printer, descriptor_, HAZZER,
                                 context_->options());
    printer->Print(variables_,
                   "@java.lang.Override $deprecation$public boolean "
                   "${$has$capitalized_name$$}$() {\n"
                   "  return $get_has_field_bit_builder$;\n"
                   "}\n");
    printer->Annotate("{", "}", descriptor_);
  }
  if (SupportUnknownEnumValue(descriptor_)) {
    WriteFieldEnumValueAccessorDocComment(printer, descriptor_, GETTER,
                                          context_->options());
    printer->Print(variables_,
                   "@java.lang.Override $deprecation$public int "
                   "${$get$capitalized_name$Value$}$() {\n"
                   "  return $name$_;\n"
                   "}\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldEnumValueAccessorDocComment(printer, descriptor_, SETTER,
                                          context_->options(),
                                          /* builder */ true);
    printer->Print(variables_,
                   "$deprecation$public Builder "
                   "${$set$capitalized_name$Value$}$(int value) {\n"
                   "  $name$_ = value;\n"
                   "  $set_has_field_bit_builder$\n"
                   "  onChanged();\n"
                   "  return this;\n"
                   "}\n");
    printer->Annotate("{", "}", descriptor_, Semantic::kSet);
  }
  WriteFieldAccessorDocComment(printer, descriptor_, GETTER,
                               context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public $type$ ${$get$capitalized_name$$}$() {\n"
                 "  $type$ result = $type$.forNumber($name$_);\n"
                 "  return result == null ? $unknown$ : result;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, SETTER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(variables_,
                 "$deprecation$public Builder "
                 "${$set$capitalized_name$$}$($type$ value) {\n"
                 "  if (value == null) {\n"
                 "    throw new NullPointerException();\n"
                 "  }\n"
                 "  $set_has_field_bit_builder$\n"
                 "  $name$_ = value.getNumber();\n"
                 "  onChanged();\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$clear$capitalized_name$$}$() {\n"
      "  $clear_has_field_bit_builder$\n"
      "  $name$_ = $default_number$;\n"
      "  onChanged();\n"
      "  return this;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void ImmutableEnumFieldGenerator::GenerateKotlinDslMembers(
    io::Printer* printer) const {
  WriteFieldDocComment(printer, descriptor_, context_->options(),
                       /* kdoc */ true);
  printer->Print(variables_,
                 "$kt_deprecation$public var $kt_name$: $kt_type$\n"
                 "  @JvmName(\"${$get$kt_capitalized_name$$}$\")\n"
                 "  get() = $kt_dsl_builder$.${$get$capitalized_name$$}$()\n"
                 "  @JvmName(\"${$set$kt_capitalized_name$$}$\")\n"
                 "  set(value) {\n"
                 "    $kt_dsl_builder$.${$set$capitalized_name$$}$(value)\n"
                 "  }\n");

  if (SupportUnknownEnumValue(descriptor_)) {
    printer->Print(
        variables_,
        "$kt_deprecation$public var $kt_name$Value: kotlin.Int\n"
        "  @JvmName(\"${$get$kt_capitalized_name$Value$}$\")\n"
        "  get() = $kt_dsl_builder$.${$get$capitalized_name$Value$}$()\n"
        "  @JvmName(\"${$set$kt_capitalized_name$Value$}$\")\n"
        "  set(value) {\n"
        "    $kt_dsl_builder$.${$set$capitalized_name$Value$}$(value)\n"
        "  }\n");
  }

  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               context_->options(),
                               /* builder */ false, /* kdoc */ true);
  printer->Print(variables_,
                 "public fun ${$clear$kt_capitalized_name$$}$() {\n"
                 "  $kt_dsl_builder$.${$clear$capitalized_name$$}$()\n"
                 "}\n");

  if (descriptor_->has_presence()) {
    WriteFieldAccessorDocComment(printer, descriptor_, HAZZER,
                                 context_->options(),
                                 /* builder */ false, /* kdoc */ true);
    printer->Print(
        variables_,
        "public fun ${$has$kt_capitalized_name$$}$(): kotlin.Boolean {\n"
        "  return $kt_dsl_builder$.${$has$capitalized_name$$}$()\n"
        "}\n");
  }
}

void ImmutableEnumFieldGenerator::GenerateFieldBuilderInitializationCode(
    io::Printer* printer) const {
  // noop for enums
}

void ImmutableEnumFieldGenerator::GenerateInitializationCode(
    io::Printer* printer) const {
  printer->Print(variables_, "$name$_ = $default_number$;\n");
}

void ImmutableEnumFieldGenerator::GenerateBuilderClearCode(
    io::Printer* printer) const {
  printer->Print(variables_, "$name$_ = $default_number$;\n");
}

void ImmutableEnumFieldGenerator::GenerateMergingCode(
    io::Printer* printer) const {
  if (descriptor_->has_presence()) {
    printer->Print(variables_,
                   "if (other.has$capitalized_name$()) {\n"
                   "  set$capitalized_name$(other.get$capitalized_name$());\n"
                   "}\n");
  } else if (SupportUnknownEnumValue(descriptor_)) {
    printer->Print(
        variables_,
        "if (other.$name$_ != $default_number$) {\n"
        "  set$capitalized_name$Value(other.get$capitalized_name$Value());\n"
        "}\n");
  } else {
    ABSL_LOG(FATAL) << "Can't reach here.";
  }
}

void ImmutableEnumFieldGenerator::GenerateBuildingCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "if ($get_has_field_bit_from_local$) {\n"
                 "  result.$name$_ = $name$_;\n");
  if (GetNumBitsForMessage() > 0) {
    printer->Print(variables_, "  $set_has_field_bit_to_local$;\n");
  }
  printer->Print("}\n");
}

void ImmutableEnumFieldGenerator::GenerateBuilderParsingCode(
    io::Printer* printer) const {
  if (SupportUnknownEnumValue(descriptor_)) {
    printer->Print(variables_,
                   "$name$_ = input.readEnum();\n"
                   "$set_has_field_bit_builder$\n");
  } else {
    printer->Print(variables_,
                   "int tmpRaw = input.readEnum();\n"
                   "$type$ tmpValue =\n"
                   "    $type$.forNumber(tmpRaw);\n"
                   "if (tmpValue == null) {\n"
                   "  mergeUnknownVarintField($number$, tmpRaw);\n"
                   "} else {\n"
                   "  $name$_ = tmpRaw;\n"
                   "  $set_has_field_bit_builder$\n"
                   "}\n");
  }
}

void ImmutableEnumFieldGenerator::GenerateSerializationCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "if ($is_field_present_message$) {\n"
                 "  output.writeEnum($number$, $name$_);\n"
                 "}\n");
}

void ImmutableEnumFieldGenerator::GenerateSerializedSizeCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "if ($is_field_present_message$) {\n"
                 "  size += com.google.protobuf.CodedOutputStream\n"
                 "    .computeEnumSize($number$, $name$_);\n"
                 "}\n");
}

void ImmutableEnumFieldGenerator::GenerateEqualsCode(
    io::Printer* printer) const {
  printer->Print(variables_, "if ($name$_ != other.$name$_) return false;\n");
}

void ImmutableEnumFieldGenerator::GenerateHashCode(io::Printer* printer) const {
  printer->Print(variables_,
                 "hash = (37 * hash) + $constant_name$;\n"
                 "hash = (53 * hash) + $name$_;\n");
}

std::string ImmutableEnumFieldGenerator::GetBoxedType() const {
  return name_resolver_->GetImmutableClassName(descriptor_->enum_type());
}

// ===================================================================

ImmutableEnumOneofFieldGenerator::ImmutableEnumOneofFieldGenerator(
    const FieldDescriptor* descriptor, int messageBitIndex, int builderBitIndex,
    Context* context)
    : ImmutableEnumFieldGenerator(descriptor, messageBitIndex, builderBitIndex,
                                  context) {
  const OneofGeneratorInfo* info =
      context->GetOneofGeneratorInfo(descriptor->containing_oneof());
  SetCommonOneofVariables(descriptor, info, &variables_);
}

ImmutableEnumOneofFieldGenerator::~ImmutableEnumOneofFieldGenerator() {}

void ImmutableEnumOneofFieldGenerator::GenerateMembers(
    io::Printer* printer) const {
  PrintExtraFieldInfo(variables_, printer);
  ABSL_DCHECK(descriptor_->has_presence());
  WriteFieldAccessorDocComment(printer, descriptor_, HAZZER,
                               context_->options());
  printer->Print(variables_,
                 "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
                 "  return $has_oneof_case_message$;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  if (SupportUnknownEnumValue(descriptor_)) {
    WriteFieldEnumValueAccessorDocComment(printer, descriptor_, GETTER,
                                          context_->options());
    printer->Print(
        variables_,
        "$deprecation$public int ${$get$capitalized_name$Value$}$() {\n"
        "  if ($has_oneof_case_message$) {\n"
        "    return (java.lang.Integer) $oneof_name$_;\n"
        "  }\n"
        "  return $default_number$;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
  }
  WriteFieldAccessorDocComment(printer, descriptor_, GETTER,
                               context_->options());
  printer->Print(variables_,
                 "$deprecation$public $type$ ${$get$capitalized_name$$}$() {\n"
                 "  if ($has_oneof_case_message$) {\n"
                 "    $type$ result = $type$.forNumber(\n"
                 "        (java.lang.Integer) $oneof_name$_);\n"
                 "    return result == null ? $unknown$ : result;\n"
                 "  }\n"
                 "  return $default$;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void ImmutableEnumOneofFieldGenerator::GenerateBuilderMembers(
    io::Printer* printer) const {
  ABSL_DCHECK(descriptor_->has_presence());
  WriteFieldAccessorDocComment(printer, descriptor_, HAZZER,
                               context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
                 "  return $has_oneof_case_message$;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  if (SupportUnknownEnumValue(descriptor_)) {
    WriteFieldEnumValueAccessorDocComment(printer, descriptor_, GETTER,
                                          context_->options());
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$public int ${$get$capitalized_name$Value$}$() {\n"
        "  if ($has_oneof_case_message$) {\n"
        "    return ((java.lang.Integer) $oneof_name$_).intValue();\n"
        "  }\n"
        "  return $default_number$;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldEnumValueAccessorDocComment(printer, descriptor_, SETTER,
                                          context_->options(),
                                          /* builder */ true);
    printer->Print(variables_,
                   "$deprecation$public Builder "
                   "${$set$capitalized_name$Value$}$(int value) {\n"
                   "  $set_oneof_case_message$;\n"
                   "  $oneof_name$_ = value;\n"
                   "  onChanged();\n"
                   "  return this;\n"
                   "}\n");
    printer->Annotate("{", "}", descriptor_, Semantic::kSet);
  }
  WriteFieldAccessorDocComment(printer, descriptor_, GETTER,
                               context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public $type$ ${$get$capitalized_name$$}$() {\n"
                 "  if ($has_oneof_case_message$) {\n"
                 "    $type$ result = $type$.forNumber(\n"
                 "        (java.lang.Integer) $oneof_name$_);\n"
                 "    return result == null ? $unknown$ : result;\n"
                 "  }\n"
                 "  return $default$;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  WriteFieldAccessorDocComment(printer, descriptor_, SETTER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(variables_,
                 "$deprecation$public Builder "
                 "${$set$capitalized_name$$}$($type$ value) {\n"
                 "  if (value == null) {\n"
                 "    throw new NullPointerException();\n"
                 "  }\n"
                 "  $set_oneof_case_message$;\n"
                 "  $oneof_name$_ = value.getNumber();\n"
                 "  onChanged();\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);

  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$clear$capitalized_name$$}$() {\n"
      "  if ($has_oneof_case_message$) {\n"
      "    $clear_oneof_case_message$;\n"
      "    $oneof_name$_ = null;\n"
      "    onChanged();\n"
      "  }\n"
      "  return this;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void ImmutableEnumOneofFieldGenerator::GenerateBuilderClearCode(
    io::Printer* printer) const {
  // No-op: Enum fields in oneofs are correctly cleared by clearing the oneof
}

void ImmutableEnumOneofFieldGenerator::GenerateBuildingCode(
    io::Printer* printer) const {
  // No-Op: Handled by single statement for the oneof
}

void ImmutableEnumOneofFieldGenerator::GenerateMergingCode(
    io::Printer* printer) const {
  if (SupportUnknownEnumValue(descriptor_)) {
    printer->Print(
        variables_,
        "set$capitalized_name$Value(other.get$capitalized_name$Value());\n");
  } else {
    printer->Print(variables_,
                   "set$capitalized_name$(other.get$capitalized_name$());\n");
  }
}

void ImmutableEnumOneofFieldGenerator::GenerateBuilderParsingCode(
    io::Printer* printer) const {
  if (SupportUnknownEnumValue(descriptor_)) {
    printer->Print(variables_,
                   "int rawValue = input.readEnum();\n"
                   "$set_oneof_case_message$;\n"
                   "$oneof_name$_ = rawValue;\n");
  } else {
    printer->Print(variables_,
                   "int rawValue = input.readEnum();\n"
                   "$type$ value =\n"
                   "    $type$.forNumber(rawValue);\n"
                   "if (value == null) {\n"
                   "  mergeUnknownVarintField($number$, rawValue);\n"
                   "} else {\n"
                   "  $set_oneof_case_message$;\n"
                   "  $oneof_name$_ = rawValue;\n"
                   "}\n");
  }
}

void ImmutableEnumOneofFieldGenerator::GenerateSerializationCode(
    io::Printer* printer) const {
  printer->Print(
      variables_,
      "if ($has_oneof_case_message$) {\n"
      "  output.writeEnum($number$, ((java.lang.Integer) $oneof_name$_));\n"
      "}\n");
}

void ImmutableEnumOneofFieldGenerator::GenerateSerializedSizeCode(
    io::Printer* printer) const {
  printer->Print(
      variables_,
      "if ($has_oneof_case_message$) {\n"
      "  size += com.google.protobuf.CodedOutputStream\n"
      "    .computeEnumSize($number$, ((java.lang.Integer) $oneof_name$_));\n"
      "}\n");
}

void ImmutableEnumOneofFieldGenerator::GenerateEqualsCode(
    io::Printer* printer) const {
  if (SupportUnknownEnumValue(descriptor_)) {
    printer->Print(
        variables_,
        "if (get$capitalized_name$Value()\n"
        "    != other.get$capitalized_name$Value()) return false;\n");
  } else {
    printer->Print(
        variables_,
        "if (!get$capitalized_name$()\n"
        "    .equals(other.get$capitalized_name$())) return false;\n");
  }
}

void ImmutableEnumOneofFieldGenerator::GenerateHashCode(
    io::Printer* printer) const {
  if (SupportUnknownEnumValue(descriptor_)) {
    printer->Print(variables_,
                   "hash = (37 * hash) + $constant_name$;\n"
                   "hash = (53 * hash) + get$capitalized_name$Value();\n");
  } else {
    printer->Print(
        variables_,
        "hash = (37 * hash) + $constant_name$;\n"
        "hash = (53 * hash) + get$capitalized_name$().getNumber();\n");
  }
}

// ===================================================================

RepeatedImmutableEnumFieldGenerator::RepeatedImmutableEnumFieldGenerator(
    const FieldDescriptor* descriptor, int messageBitIndex, int builderBitIndex,
    Context* context)
    : ImmutableEnumFieldGenerator(descriptor, messageBitIndex, builderBitIndex,
                                  context) {}

RepeatedImmutableEnumFieldGenerator::~RepeatedImmutableEnumFieldGenerator() {}

int RepeatedImmutableEnumFieldGenerator::GetNumBitsForMessage() const {
  return 0;
}

int RepeatedImmutableEnumFieldGenerator::GetNumBitsForBuilder() const {
  return 1;
}

void RepeatedImmutableEnumFieldGenerator::GenerateInterfaceMembers(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_GETTER,
                               context_->options());
  printer->Print(
      variables_,
      "$deprecation$java.util.List<$type$> get$capitalized_name$List();\n");
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_COUNT,
                               context_->options());
  printer->Print(variables_,
                 "$deprecation$int get$capitalized_name$Count();\n");
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_INDEXED_GETTER,
                               context_->options());
  printer->Print(variables_,
                 "$deprecation$$type$ get$capitalized_name$(int index);\n");
  if (SupportUnknownEnumValue(descriptor_)) {
    WriteFieldEnumValueAccessorDocComment(printer, descriptor_, LIST_GETTER,
                                          context_->options());
    printer->Print(variables_,
                   "$deprecation$java.util.List<java.lang.Integer>\n"
                   "get$capitalized_name$ValueList();\n");
    WriteFieldEnumValueAccessorDocComment(
        printer, descriptor_, LIST_INDEXED_GETTER, context_->options());
    printer->Print(variables_,
                   "$deprecation$int get$capitalized_name$Value(int index);\n");
  }
}

void RepeatedImmutableEnumFieldGenerator::GenerateMembers(
    io::Printer* printer) const {
  printer->Print(
      variables_,
      "@SuppressWarnings(\"serial\")\n"
      "private java.util.List<java.lang.Integer> $name$_;\n"
      "private static final "
      "com.google.protobuf.Internal.ListAdapter.Converter<\n"
      "    java.lang.Integer, $type$> $name$_converter_ =\n"
      "        new com.google.protobuf.Internal.ListAdapter.Converter<\n"
      "            java.lang.Integer, $type$>() {\n"
      "          public $type$ convert(java.lang.Integer from) {\n"
      "            $type$ result = $type$.forNumber(from);\n"
      "            return result == null ? $unknown$ : result;\n"
      "          }\n"
      "        };\n");
  PrintExtraFieldInfo(variables_, printer);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_GETTER,
                               context_->options());
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public java.util.List<$type$> "
      "${$get$capitalized_name$List$}$() {\n"
      "  return new com.google.protobuf.Internal.ListAdapter<\n"
      "      java.lang.Integer, $type$>($name$_, $name$_converter_);\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_COUNT,
                               context_->options());
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public int ${$get$capitalized_name$Count$}$() {\n"
      "  return $name$_.size();\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_INDEXED_GETTER,
                               context_->options());
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public $type$ ${$get$capitalized_name$$}$(int index) {\n"
      "  return $name$_converter_.convert($name$_.get(index));\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
  if (SupportUnknownEnumValue(descriptor_)) {
    WriteFieldEnumValueAccessorDocComment(printer, descriptor_, LIST_GETTER,
                                          context_->options());
    printer->Print(variables_,
                   "@java.lang.Override\n"
                   "$deprecation$public java.util.List<java.lang.Integer>\n"
                   "${$get$capitalized_name$ValueList$}$() {\n"
                   "  return $name$_;\n"
                   "}\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldEnumValueAccessorDocComment(
        printer, descriptor_, LIST_INDEXED_GETTER, context_->options());
    printer->Print(variables_,
                   "@java.lang.Override\n"
                   "$deprecation$public int "
                   "${$get$capitalized_name$Value$}$(int index) {\n"
                   "  return $name$_.get(index);\n"
                   "}\n");
    printer->Annotate("{", "}", descriptor_);
  }

  if (descriptor_->is_packed()) {
    printer->Print(variables_, "private int $name$MemoizedSerializedSize;\n");
  }
}

void RepeatedImmutableEnumFieldGenerator::GenerateBuilderMembers(
    io::Printer* printer) const {
  printer->Print(
      variables_,
      // One field is the list and the other field keeps track of whether the
      // list is immutable. If it's immutable, the invariant is that it must
      // either an instance of Collections.emptyList() or it's an ArrayList
      // wrapped in a Collections.unmodifiableList() wrapper and nobody else has
      // a reference to the underlying ArrayList. This invariant allows us to
      // share instances of lists between protocol buffers avoiding expensive
      // memory allocations. Note, immutable is a strong guarantee here -- not
      // just that the list cannot be modified via the reference but that the
      // list can never be modified.
      "private java.util.List<java.lang.Integer> $name$_ =\n"
      "  java.util.Collections.emptyList();\n"

      "private void ensure$capitalized_name$IsMutable() {\n"
      "  if (!$get_mutable_bit_builder$) {\n"
      "    $name$_ = new java.util.ArrayList<java.lang.Integer>($name$_);\n"
      "    $set_mutable_bit_builder$;\n"
      "  }\n"
      "}\n");

  WriteFieldAccessorDocComment(printer, descriptor_, LIST_GETTER,
                               context_->options());
  printer->Print(
      variables_,
      // Note:  We return an unmodifiable list because otherwise the caller
      //   could hold on to the returned list and modify it after the message
      //   has been built, thus mutating the message which is supposed to be
      //   immutable.
      "$deprecation$public java.util.List<$type$> "
      "${$get$capitalized_name$List$}$() {\n"
      "  return new com.google.protobuf.Internal.ListAdapter<\n"
      "      java.lang.Integer, $type$>($name$_, $name$_converter_);\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_COUNT,
                               context_->options());
  printer->Print(
      variables_,
      "$deprecation$public int ${$get$capitalized_name$Count$}$() {\n"
      "  return $name$_.size();\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_INDEXED_GETTER,
                               context_->options());
  printer->Print(
      variables_,
      "$deprecation$public $type$ ${$get$capitalized_name$$}$(int index) {\n"
      "  return $name$_converter_.convert($name$_.get(index));\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_INDEXED_SETTER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$set$capitalized_name$$}$(\n"
                 "    int index, $type$ value) {\n"
                 "  if (value == null) {\n"
                 "    throw new NullPointerException();\n"
                 "  }\n"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  $name$_.set(index, value.getNumber());\n"
                 "  onChanged();\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_ADDER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(variables_,
                 "$deprecation$public Builder "
                 "${$add$capitalized_name$$}$($type$ value) {\n"
                 "  if (value == null) {\n"
                 "    throw new NullPointerException();\n"
                 "  }\n"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  $name$_.add(value.getNumber());\n"
                 "  onChanged();\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_MULTI_ADDER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$addAll$capitalized_name$$}$(\n"
                 "    java.lang.Iterable<? extends $type$> values) {\n"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  for ($type$ value : values) {\n"
                 "    $name$_.add(value.getNumber());\n"
                 "  }\n"
                 "  onChanged();\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$clear$capitalized_name$$}$() {\n"
      "  $name$_ = java.util.Collections.emptyList();\n"
      "  $clear_mutable_bit_builder$;\n"
      "  onChanged();\n"
      "  return this;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);

  if (SupportUnknownEnumValue(descriptor_)) {
    WriteFieldEnumValueAccessorDocComment(printer, descriptor_, LIST_GETTER,
                                          context_->options());
    printer->Print(variables_,
                   "$deprecation$public java.util.List<java.lang.Integer>\n"
                   "${$get$capitalized_name$ValueList$}$() {\n"
                   "  return java.util.Collections.unmodifiableList($name$_);\n"
                   "}\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldEnumValueAccessorDocComment(
        printer, descriptor_, LIST_INDEXED_GETTER, context_->options());
    printer->Print(variables_,
                   "$deprecation$public int "
                   "${$get$capitalized_name$Value$}$(int index) {\n"
                   "  return $name$_.get(index);\n"
                   "}\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldEnumValueAccessorDocComment(
        printer, descriptor_, LIST_INDEXED_SETTER, context_->options(),
        /* builder */ true);
    printer->Print(
        variables_,
        "$deprecation$public Builder ${$set$capitalized_name$Value$}$(\n"
        "    int index, int value) {\n"
        "  ensure$capitalized_name$IsMutable();\n"
        "  $name$_.set(index, value);\n"
        "  onChanged();\n"
        "  return this;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_, Semantic::kSet);
    WriteFieldEnumValueAccessorDocComment(printer, descriptor_, LIST_ADDER,
                                          context_->options(),
                                          /* builder */ true);
    printer->Print(variables_,
                   "$deprecation$public Builder "
                   "${$add$capitalized_name$Value$}$(int value) {\n"
                   "  ensure$capitalized_name$IsMutable();\n"
                   "  $name$_.add(value);\n"
                   "  onChanged();\n"
                   "  return this;\n"
                   "}\n");
    printer->Annotate("{", "}", descriptor_, Semantic::kSet);
    WriteFieldEnumValueAccessorDocComment(printer, descriptor_,
                                          LIST_MULTI_ADDER, context_->options(),
                                          /* builder */ true);
    printer->Print(
        variables_,
        "$deprecation$public Builder ${$addAll$capitalized_name$Value$}$(\n"
        "    java.lang.Iterable<java.lang.Integer> values) {\n"
        "  ensure$capitalized_name$IsMutable();\n"
        "  for (int value : values) {\n"
        "    $name$_.add(value);\n"
        "  }\n"
        "  onChanged();\n"
        "  return this;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
  }
}

void RepeatedImmutableEnumFieldGenerator::
    GenerateFieldBuilderInitializationCode(io::Printer* printer) const {
  // noop for enums
}

void RepeatedImmutableEnumFieldGenerator::GenerateInitializationCode(
    io::Printer* printer) const {
  printer->Print(variables_, "$name$_ = java.util.Collections.emptyList();\n");
}

void RepeatedImmutableEnumFieldGenerator::GenerateBuilderClearCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "$name$_ = java.util.Collections.emptyList();\n"
                 "$clear_mutable_bit_builder$;\n");
}

void RepeatedImmutableEnumFieldGenerator::GenerateMergingCode(
    io::Printer* printer) const {
  // The code below does two optimizations:
  //   1. If the other list is empty, there's nothing to do. This ensures we
  //      don't allocate a new array if we already have an immutable one.
  //   2. If the other list is non-empty and our current list is empty, we can
  //      reuse the other list which is guaranteed to be immutable.
  printer->Print(variables_,
                 "if (!other.$name$_.isEmpty()) {\n"
                 "  if ($name$_.isEmpty()) {\n"
                 "    $name$_ = other.$name$_;\n"
                 "    $clear_mutable_bit_builder$;\n"
                 "  } else {\n"
                 "    ensure$capitalized_name$IsMutable();\n"
                 "    $name$_.addAll(other.$name$_);\n"
                 "  }\n"
                 "  onChanged();\n"
                 "}\n");
}

void RepeatedImmutableEnumFieldGenerator::GenerateBuildingCode(
    io::Printer* printer) const {
  // The code below ensures that the result has an immutable list. If our
  // list is immutable, we can just reuse it. If not, we make it immutable.
  printer->Print(
      variables_,
      "if ($get_mutable_bit_builder$) {\n"
      "  $name$_ = java.util.Collections.unmodifiableList($name$_);\n"
      "  $clear_mutable_bit_builder$;\n"
      "}\n"
      "result.$name$_ = $name$_;\n");
}

void RepeatedImmutableEnumFieldGenerator::GenerateBuilderParsingCode(
    io::Printer* printer) const {
  // Read and store the enum
  if (SupportUnknownEnumValue(descriptor_)) {
    printer->Print(variables_,
                   "int tmpRaw = input.readEnum();\n"
                   "ensure$capitalized_name$IsMutable();\n"
                   "$name$_.add(tmpRaw);\n");
  } else {
    printer->Print(variables_,
                   "int tmpRaw = input.readEnum();\n"
                   "$type$ tmpValue =\n"
                   "    $type$.forNumber(tmpRaw);\n"
                   "if (tmpValue == null) {\n"
                   "  mergeUnknownVarintField($number$, tmpRaw);\n"
                   "} else {\n"
                   "  ensure$capitalized_name$IsMutable();\n"
                   "  $name$_.add(tmpRaw);\n"
                   "}\n");
  }
}

void RepeatedImmutableEnumFieldGenerator::GenerateBuilderParsingCodeFromPacked(
    io::Printer* printer) const {
  // Wrap GenerateParsingCode's contents with a while loop.

  printer->Print(variables_,
                 "int length = input.readRawVarint32();\n"
                 "int oldLimit = input.pushLimit(length);\n"
                 "while(input.getBytesUntilLimit() > 0) {\n");
  printer->Indent();

  GenerateBuilderParsingCode(printer);

  printer->Outdent();
  printer->Print(variables_,
                 "}\n"
                 "input.popLimit(oldLimit);\n");
}
void RepeatedImmutableEnumFieldGenerator::GenerateSerializationCode(
    io::Printer* printer) const {
  if (descriptor_->is_packed()) {
    printer->Print(variables_,
                   "if (get$capitalized_name$List().size() > 0) {\n"
                   "  output.writeUInt32NoTag($tag$);\n"
                   "  output.writeUInt32NoTag($name$MemoizedSerializedSize);\n"
                   "}\n"
                   "for (int i = 0; i < $name$_.size(); i++) {\n"
                   "  output.writeEnumNoTag($name$_.get(i));\n"
                   "}\n");
  } else {
    printer->Print(variables_,
                   "for (int i = 0; i < $name$_.size(); i++) {\n"
                   "  output.writeEnum($number$, $name$_.get(i));\n"
                   "}\n");
  }
}

void RepeatedImmutableEnumFieldGenerator::GenerateSerializedSizeCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "{\n"
                 "  int dataSize = 0;\n");
  printer->Indent();

  printer->Print(variables_,
                 "for (int i = 0; i < $name$_.size(); i++) {\n"
                 "  dataSize += com.google.protobuf.CodedOutputStream\n"
                 "    .computeEnumSizeNoTag($name$_.get(i));\n"
                 "}\n");
  printer->Print("size += dataSize;\n");
  if (descriptor_->is_packed()) {
    printer->Print(variables_,
                   "if (!get$capitalized_name$List().isEmpty()) {"
                   "  size += $tag_size$;\n"
                   "  size += com.google.protobuf.CodedOutputStream\n"
                   "    .computeUInt32SizeNoTag(dataSize);\n"
                   "}");
  } else {
    printer->Print(variables_, "size += $tag_size$ * $name$_.size();\n");
  }

  // cache the data size for packed fields.
  if (descriptor_->is_packed()) {
    printer->Print(variables_, "$name$MemoizedSerializedSize = dataSize;\n");
  }

  printer->Outdent();
  printer->Print("}\n");
}

void RepeatedImmutableEnumFieldGenerator::GenerateEqualsCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "if (!$name$_.equals(other.$name$_)) return false;\n");
}

void RepeatedImmutableEnumFieldGenerator::GenerateHashCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "if (get$capitalized_name$Count() > 0) {\n"
                 "  hash = (37 * hash) + $constant_name$;\n"
                 "  hash = (53 * hash) + $name$_.hashCode();\n"
                 "}\n");
}

void RepeatedImmutableEnumFieldGenerator::GenerateKotlinDslMembers(
    io::Printer* printer) const {
  printer->Print(
      variables_,
      "/**\n"
      " * An uninstantiable, behaviorless type to represent the field in\n"
      " * generics.\n"
      " */\n"
      "@kotlin.OptIn"
      "(com.google.protobuf.kotlin.OnlyForUseByGeneratedProtoCode::class)\n"
      "public class ${$$kt_capitalized_name$Proxy$}$ private constructor()"
      " : com.google.protobuf.kotlin.DslProxy()\n");

  WriteFieldDocComment(printer, descriptor_, context_->options(),
                       /* kdoc */ true);
  printer->Print(variables_,
                 "$kt_deprecation$public val $kt_name$: "
                 "com.google.protobuf.kotlin.DslList"
                 "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>\n"
                 "  @kotlin.jvm.JvmSynthetic\n"
                 "  get() = com.google.protobuf.kotlin.DslList(\n"
                 "    $kt_dsl_builder$.${$get$capitalized_name$List$}$()\n"
                 "  )\n");

  WriteFieldAccessorDocComment(printer, descriptor_, LIST_ADDER,
                               context_->options(),
                               /* builder */ false, /* kdoc */ true);
  printer->Print(variables_,
                 "@kotlin.jvm.JvmSynthetic\n"
                 "@kotlin.jvm.JvmName(\"add$kt_capitalized_name$\")\n"
                 "public fun com.google.protobuf.kotlin.DslList"
                 "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
                 "add(value: $kt_type$) {\n"
                 "  $kt_dsl_builder$.${$add$capitalized_name$$}$(value)\n"
                 "}");

  WriteFieldAccessorDocComment(printer, descriptor_, LIST_ADDER,
                               context_->options(),
                               /* builder */ false, /* kdoc */ true);
  printer->Print(variables_,
                 "@kotlin.jvm.JvmSynthetic\n"
                 "@kotlin.jvm.JvmName(\"plusAssign$kt_capitalized_name$\")\n"
                 "@Suppress(\"NOTHING_TO_INLINE\")\n"
                 "public inline operator fun com.google.protobuf.kotlin.DslList"
                 "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
                 "plusAssign(value: $kt_type$) {\n"
                 "  add(value)\n"
                 "}");

  WriteFieldAccessorDocComment(printer, descriptor_, LIST_MULTI_ADDER,
                               context_->options(),
                               /* builder */ false, /* kdoc */ true);
  printer->Print(variables_,
                 "@kotlin.jvm.JvmSynthetic\n"
                 "@kotlin.jvm.JvmName(\"addAll$kt_capitalized_name$\")\n"
                 "public fun com.google.protobuf.kotlin.DslList"
                 "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
                 "addAll(values: kotlin.collections.Iterable<$kt_type$>) {\n"
                 "  $kt_dsl_builder$.${$addAll$capitalized_name$$}$(values)\n"
                 "}");

  WriteFieldAccessorDocComment(printer, descriptor_, LIST_MULTI_ADDER,
                               context_->options(),
                               /* builder */ false, /* kdoc */ true);
  printer->Print(
      variables_,
      "@kotlin.jvm.JvmSynthetic\n"
      "@kotlin.jvm.JvmName(\"plusAssignAll$kt_capitalized_name$\")\n"
      "@Suppress(\"NOTHING_TO_INLINE\")\n"
      "public inline operator fun com.google.protobuf.kotlin.DslList"
      "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
      "plusAssign(values: kotlin.collections.Iterable<$kt_type$>) {\n"
      "  addAll(values)\n"
      "}");

  WriteFieldAccessorDocComment(printer, descriptor_, LIST_INDEXED_SETTER,
                               context_->options(),
                               /* builder */ false, /* kdoc */ true);
  printer->Print(
      variables_,
      "@kotlin.jvm.JvmSynthetic\n"
      "@kotlin.jvm.JvmName(\"set$kt_capitalized_name$\")\n"
      "public operator fun com.google.protobuf.kotlin.DslList"
      "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
      "set(index: kotlin.Int, value: $kt_type$) {\n"
      "  $kt_dsl_builder$.${$set$capitalized_name$$}$(index, value)\n"
      "}");

  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               context_->options(),
                               /* builder */ false, /* kdoc */ true);
  printer->Print(variables_,
                 "@kotlin.jvm.JvmSynthetic\n"
                 "@kotlin.jvm.JvmName(\"clear$kt_capitalized_name$\")\n"
                 "public fun com.google.protobuf.kotlin.DslList"
                 "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
                 "clear() {\n"
                 "  $kt_dsl_builder$.${$clear$capitalized_name$$}$()\n"
                 "}");
}

std::string RepeatedImmutableEnumFieldGenerator::GetBoxedType() const {
  return name_resolver_->GetImmutableClassName(descriptor_->enum_type());
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
