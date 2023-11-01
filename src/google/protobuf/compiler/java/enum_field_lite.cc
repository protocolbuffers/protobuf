// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/java/enum_field_lite.h"

#include <cstdint>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "google/protobuf/compiler/java/context.h"
#include "google/protobuf/compiler/java/doc_comment.h"
#include "google/protobuf/compiler/java/helpers.h"
#include "google/protobuf/compiler/java/name_resolver.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/wire_format.h"

// Must be last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {
using Semantic = ::google::protobuf::io::AnnotationCollector::Semantic;

namespace {
bool EnableExperimentalRuntimeForLite() {
#ifdef PROTOBUF_EXPERIMENT
  return PROTOBUF_EXPERIMENT;
#else   // PROTOBUF_EXPERIMENT
  return false;
#endif  // !PROTOBUF_EXPERIMENT
}

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
  (*variables)["required"] = descriptor->is_required() ? "true" : "false";

  if (HasHasbit(descriptor)) {
    if (!context->options().opensource_runtime) {
      (*variables)["bit_field_id"] = absl::StrCat(messageBitIndex / 32);
      (*variables)["bit_field_name"] = GetBitFieldNameForBit(messageBitIndex);
      (*variables)["bit_field_mask"] =
          absl::StrCat(1 << (messageBitIndex % 32));
    }
    // For singular messages and builders, one bit is used for the hasField bit.
    (*variables)["get_has_field_bit_message"] = GenerateGetBit(messageBitIndex);

    // Note that these have a trailing ";".
    (*variables)["set_has_field_bit_message"] =
        absl::StrCat(GenerateSetBit(messageBitIndex), ";");
    (*variables)["clear_has_field_bit_message"] =
        absl::StrCat(GenerateClearBit(messageBitIndex), ";");

    (*variables)["is_field_present_message"] = GenerateGetBit(messageBitIndex);
  } else {
    (*variables)["set_has_field_bit_message"] = "";
    (*variables)["clear_has_field_bit_message"] = "";

    variables->insert({"is_field_present_message",
                       absl::StrCat((*variables)["name"], "_ != ",
                                    (*variables)["default"], ".getNumber()")});
  }

  if (SupportUnknownEnumValue(descriptor)) {
    variables->insert(
        {"unknown", absl::StrCat((*variables)["type"], ".UNRECOGNIZED")});
  } else {
    variables->insert({"unknown", (*variables)["default"]});
  }

  // We use `x.getClass()` as a null check because it generates less bytecode
  // than an `if (x == null) { throw ... }` statement.
  (*variables)["null_check"] = "value.getClass();\n";
  // Calls to Annotate() use variable ranges to know which text to annotate.
  (*variables)["{"] = "";
  (*variables)["}"] = "";
}

}  // namespace

// ===================================================================

ImmutableEnumFieldLiteGenerator::ImmutableEnumFieldLiteGenerator(
    const FieldDescriptor* descriptor, int messageBitIndex, Context* context)
    : descriptor_(descriptor),
      messageBitIndex_(messageBitIndex),
      context_(context),
      name_resolver_(context->GetNameResolver()) {
  SetEnumVariables(descriptor, messageBitIndex, 0,
                   context->GetFieldGeneratorInfo(descriptor), name_resolver_,
                   &variables_, context);
}

ImmutableEnumFieldLiteGenerator::~ImmutableEnumFieldLiteGenerator() {}

int ImmutableEnumFieldLiteGenerator::GetNumBitsForMessage() const {
  return HasHasbit(descriptor_) ? 1 : 0;
}

void ImmutableEnumFieldLiteGenerator::GenerateInterfaceMembers(
    io::Printer* printer) const {
  if (descriptor_->has_presence()) {
    WriteFieldAccessorDocComment(printer, descriptor_, HAZZER,
                                 context_->options());
    printer->Print(variables_,
                   "$deprecation$boolean ${$has$capitalized_name$$}$();\n");
    printer->Annotate("{", "}", descriptor_);
  }
  if (SupportUnknownEnumValue(descriptor_)) {
    WriteFieldEnumValueAccessorDocComment(printer, descriptor_, GETTER,
                                          context_->options());
    printer->Print(variables_,
                   "$deprecation$int ${$get$capitalized_name$Value$}$();\n");
    printer->Annotate("{", "}", descriptor_);
  }
  WriteFieldAccessorDocComment(printer, descriptor_, GETTER,
                               context_->options());
  printer->Print(variables_,
                 "$deprecation$$type$ ${$get$capitalized_name$$}$();\n");
  printer->Annotate("{", "}", descriptor_);
}

void ImmutableEnumFieldLiteGenerator::GenerateMembers(
    io::Printer* printer) const {
  if (!context_->options().opensource_runtime) {
    printer->Print(variables_,
                   "@com.google.protobuf.ProtoField(\n"
                   "  isRequired=$required$)\n");
    if (HasHasbit(descriptor_)) {
      printer->Print(variables_,
                     "@com.google.protobuf.ProtoPresenceCheckedField(\n"
                     "  presenceBitsId=$bit_field_id$,\n"
                     "  mask=$bit_field_mask$)\n");
    }
  }
  printer->Print(variables_, "private int $name$_;\n");
  PrintExtraFieldInfo(variables_, printer);
  if (descriptor_->has_presence()) {
    WriteFieldAccessorDocComment(printer, descriptor_, HAZZER,
                                 context_->options());
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
        "  return $get_has_field_bit_message$;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
  }
  if (SupportUnknownEnumValue(descriptor_)) {
    WriteFieldEnumValueAccessorDocComment(printer, descriptor_, GETTER,
                                          context_->options());
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$public int ${$get$capitalized_name$Value$}$() {\n"
        "  return $name$_;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
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

  // Generate private setters for the builder to proxy into.
  if (SupportUnknownEnumValue(descriptor_)) {
    WriteFieldEnumValueAccessorDocComment(printer, descriptor_, SETTER,
                                          context_->options());
    printer->Print(variables_,
                   "private void set$capitalized_name$Value(int value) {\n"
                   "  $set_has_field_bit_message$"
                   "  $name$_ = value;\n"
                   "}\n");
  }
  WriteFieldAccessorDocComment(printer, descriptor_, SETTER,
                               context_->options());
  printer->Print(variables_,
                 "private void set$capitalized_name$($type$ value) {\n"
                 "  $name$_ = value.getNumber();\n"
                 "  $set_has_field_bit_message$\n"
                 "}\n");
  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               context_->options());
  printer->Print(variables_,
                 "private void clear$capitalized_name$() {\n"
                 "  $clear_has_field_bit_message$\n"
                 "  $name$_ = $default_number$;\n"
                 "}\n");
}

void ImmutableEnumFieldLiteGenerator::GenerateBuilderMembers(
    io::Printer* printer) const {
  if (descriptor_->has_presence()) {
    WriteFieldAccessorDocComment(printer, descriptor_, HAZZER,
                                 context_->options());
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
        "  return instance.has$capitalized_name$();\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
  }
  if (SupportUnknownEnumValue(descriptor_)) {
    WriteFieldEnumValueAccessorDocComment(printer, descriptor_, GETTER,
                                          context_->options());
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$public int ${$get$capitalized_name$Value$}$() {\n"
        "  return instance.get$capitalized_name$Value();\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldAccessorDocComment(printer, descriptor_, SETTER,
                                 context_->options(),
                                 /* builder */ true);
    printer->Print(variables_,
                   "$deprecation$public Builder "
                   "${$set$capitalized_name$Value$}$(int value) {\n"
                   "  copyOnWrite();\n"
                   "  instance.set$capitalized_name$Value(value);\n"
                   "  return this;\n"
                   "}\n");
    printer->Annotate("{", "}", descriptor_, Semantic::kSet);
  }
  WriteFieldAccessorDocComment(printer, descriptor_, GETTER,
                               context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public $type$ ${$get$capitalized_name$$}$() {\n"
                 "  return instance.get$capitalized_name$();\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldEnumValueAccessorDocComment(printer, descriptor_, SETTER,
                                        context_->options(),
                                        /* builder */ true);
  printer->Print(variables_,
                 "$deprecation$public Builder "
                 "${$set$capitalized_name$$}$($type$ value) {\n"
                 "  copyOnWrite();\n"
                 "  instance.set$capitalized_name$(value);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$clear$capitalized_name$$}$() {\n"
      "  copyOnWrite();\n"
      "  instance.clear$capitalized_name$();\n"
      "  return this;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void ImmutableEnumFieldLiteGenerator::GenerateKotlinDslMembers(
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

void ImmutableEnumFieldLiteGenerator::GenerateInitializationCode(
    io::Printer* printer) const {
  if (!IsDefaultValueJavaDefault(descriptor_)) {
    printer->Print(variables_, "$name$_ = $default_number$;\n");
  }
}

void ImmutableEnumFieldLiteGenerator::GenerateFieldInfo(
    io::Printer* printer, std::vector<uint16_t>* output) const {
  WriteIntToUtf16CharSequence(descriptor_->number(), output);
  WriteIntToUtf16CharSequence(GetExperimentalJavaFieldType(descriptor_),
                              output);
  if (HasHasbit(descriptor_)) {
    WriteIntToUtf16CharSequence(messageBitIndex_, output);
  }
  printer->Print(variables_, "\"$name$_\",\n");
  if (!SupportUnknownEnumValue((descriptor_))) {
    PrintEnumVerifierLogic(printer, descriptor_, variables_,
                           /*var_name=*/"$type$",
                           /*terminating_string=*/",\n",
                           /*enforce_lite=*/context_->EnforceLite());
  }
}

std::string ImmutableEnumFieldLiteGenerator::GetBoxedType() const {
  return name_resolver_->GetImmutableClassName(descriptor_->enum_type());
}

// ===================================================================

ImmutableEnumOneofFieldLiteGenerator::ImmutableEnumOneofFieldLiteGenerator(
    const FieldDescriptor* descriptor, int messageBitIndex, Context* context)
    : ImmutableEnumFieldLiteGenerator(descriptor, messageBitIndex, context) {
  const OneofGeneratorInfo* info =
      context->GetOneofGeneratorInfo(descriptor->containing_oneof());
  SetCommonOneofVariables(descriptor, info, &variables_);
}

ImmutableEnumOneofFieldLiteGenerator::~ImmutableEnumOneofFieldLiteGenerator() {}

void ImmutableEnumOneofFieldLiteGenerator::GenerateMembers(
    io::Printer* printer) const {
  PrintExtraFieldInfo(variables_, printer);
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
        "    return (java.lang.Integer) $oneof_name$_;\n"
        "  }\n"
        "  return $default_number$;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
  }
  WriteFieldAccessorDocComment(printer, descriptor_, GETTER,
                               context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public $type$ ${$get$capitalized_name$$}$() {\n"
                 "  if ($has_oneof_case_message$) {\n"
                 "    $type$ result = $type$.forNumber((java.lang.Integer) "
                 "$oneof_name$_);\n"
                 "    return result == null ? $unknown$ : result;\n"
                 "  }\n"
                 "  return $default$;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  // Generate private setters for the builder to proxy into.
  if (SupportUnknownEnumValue(descriptor_)) {
    WriteFieldEnumValueAccessorDocComment(printer, descriptor_, SETTER,
                                          context_->options());
    printer->Print(variables_,
                   "private void set$capitalized_name$Value(int value) {\n"
                   "  $set_oneof_case_message$;\n"
                   "  $oneof_name$_ = value;\n"
                   "}\n");
  }
  WriteFieldAccessorDocComment(printer, descriptor_, SETTER,
                               context_->options());
  printer->Print(variables_,
                 "private void set$capitalized_name$($type$ value) {\n"
                 "  $oneof_name$_ = value.getNumber();\n"
                 "  $set_oneof_case_message$;\n"
                 "}\n");
  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               context_->options());
  printer->Print(variables_,
                 "private void clear$capitalized_name$() {\n"
                 "  if ($has_oneof_case_message$) {\n"
                 "    $clear_oneof_case_message$;\n"
                 "    $oneof_name$_ = null;\n"
                 "  }\n"
                 "}\n");
}

void ImmutableEnumOneofFieldLiteGenerator::GenerateFieldInfo(
    io::Printer* printer, std::vector<uint16_t>* output) const {
  WriteIntToUtf16CharSequence(descriptor_->number(), output);
  WriteIntToUtf16CharSequence(GetExperimentalJavaFieldType(descriptor_),
                              output);
  WriteIntToUtf16CharSequence(descriptor_->containing_oneof()->index(), output);
  if (!SupportUnknownEnumValue(descriptor_)) {
    PrintEnumVerifierLogic(printer, descriptor_, variables_,
                           /*var_name=*/"$type$",
                           /*terminating_string=*/",\n",
                           /*enforce_lite=*/context_->EnforceLite());
  }
}

void ImmutableEnumOneofFieldLiteGenerator::GenerateBuilderMembers(
    io::Printer* printer) const {
  ABSL_DCHECK(descriptor_->has_presence());
  WriteFieldAccessorDocComment(printer, descriptor_, HAZZER,
                               context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
                 "  return instance.has$capitalized_name$();\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  if (SupportUnknownEnumValue(descriptor_)) {
    WriteFieldEnumValueAccessorDocComment(printer, descriptor_, GETTER,
                                          context_->options());
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$public int ${$get$capitalized_name$Value$}$() {\n"
        "  return instance.get$capitalized_name$Value();\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldEnumValueAccessorDocComment(printer, descriptor_, SETTER,
                                          context_->options(),
                                          /* builder */ true);
    printer->Print(variables_,
                   "$deprecation$public Builder "
                   "${$set$capitalized_name$Value$}$(int value) {\n"
                   "  copyOnWrite();\n"
                   "  instance.set$capitalized_name$Value(value);\n"
                   "  return this;\n"
                   "}\n");
    printer->Annotate("{", "}", descriptor_, Semantic::kSet);
  }
  WriteFieldAccessorDocComment(printer, descriptor_, GETTER,
                               context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public $type$ ${$get$capitalized_name$$}$() {\n"
                 "  return instance.get$capitalized_name$();\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, SETTER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(variables_,
                 "$deprecation$public Builder "
                 "${$set$capitalized_name$$}$($type$ value) {\n"
                 "  copyOnWrite();\n"
                 "  instance.set$capitalized_name$(value);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$clear$capitalized_name$$}$() {\n"
      "  copyOnWrite();\n"
      "  instance.clear$capitalized_name$();\n"
      "  return this;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

// ===================================================================

RepeatedImmutableEnumFieldLiteGenerator::
    RepeatedImmutableEnumFieldLiteGenerator(const FieldDescriptor* descriptor,
                                            int messageBitIndex,
                                            Context* context)
    : descriptor_(descriptor),
      context_(context),
      name_resolver_(context->GetNameResolver()) {
  SetEnumVariables(descriptor, messageBitIndex, 0,
                   context->GetFieldGeneratorInfo(descriptor), name_resolver_,
                   &variables_, context);
}

RepeatedImmutableEnumFieldLiteGenerator::
    ~RepeatedImmutableEnumFieldLiteGenerator() {}

int RepeatedImmutableEnumFieldLiteGenerator::GetNumBitsForMessage() const {
  return 0;
}

void RepeatedImmutableEnumFieldLiteGenerator::GenerateInterfaceMembers(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_GETTER,
                               context_->options());
  printer->Print(variables_,
                 "$deprecation$java.util.List<$type$> "
                 "${$get$capitalized_name$List$}$();\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_COUNT,
                               context_->options());
  printer->Print(variables_,
                 "$deprecation$int ${$get$capitalized_name$Count$}$();\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_INDEXED_GETTER,
                               context_->options());
  printer->Print(
      variables_,
      "$deprecation$$type$ ${$get$capitalized_name$$}$(int index);\n");
  printer->Annotate("{", "}", descriptor_);
  if (SupportUnknownEnumValue(descriptor_)) {
    WriteFieldEnumValueAccessorDocComment(printer, descriptor_, LIST_GETTER,
                                          context_->options());
    printer->Print(variables_,
                   "$deprecation$java.util.List<java.lang.Integer>\n"
                   "${$get$capitalized_name$ValueList$}$();\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldEnumValueAccessorDocComment(
        printer, descriptor_, LIST_INDEXED_GETTER, context_->options());
    printer->Print(
        variables_,
        "$deprecation$int ${$get$capitalized_name$Value$}$(int index);\n");
    printer->Annotate("{", "}", descriptor_);
  }
}

void RepeatedImmutableEnumFieldLiteGenerator::GenerateMembers(
    io::Printer* printer) const {
  printer->Print(
      variables_,
      "private com.google.protobuf.Internal.IntList $name$_;\n"
      "private static final "
      "com.google.protobuf.Internal.ListAdapter.Converter<\n"
      "    java.lang.Integer, $type$> $name$_converter_ =\n"
      "        new com.google.protobuf.Internal.ListAdapter.Converter<\n"
      "            java.lang.Integer, $type$>() {\n"
      "          @java.lang.Override\n"
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
      // NB: Do not use the "$name$_converter_" field; the usage of generics
      // (and requisite upcasts to Object) prevent optimizations.  Even
      // without any optimizations, the below code is cheaper because it
      // avoids boxing an int and a checkcast from the generics.
      "@java.lang.Override\n"
      "$deprecation$public $type$ ${$get$capitalized_name$$}$(int index) {\n"
      "  $type$ result = $type$.forNumber($name$_.getInt(index));\n"
      "  return result == null ? $unknown$ : result;\n"
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
                   "  return $name$_.getInt(index);\n"
                   "}\n");
    printer->Annotate("{", "}", descriptor_);
  }

  if (!EnableExperimentalRuntimeForLite() && descriptor_->is_packed() &&
      context_->HasGeneratedMethods(descriptor_->containing_type())) {
    printer->Print(variables_, "private int $name$MemoizedSerializedSize;\n");
  }

  // Generate private setters for the builder to proxy into.
  printer->Print(
      variables_,
      "private void ensure$capitalized_name$IsMutable() {\n"
      // Use a temporary to avoid a redundant iget-object.
      "  com.google.protobuf.Internal.IntList tmp = $name$_;\n"
      "  if (!tmp.isModifiable()) {\n"
      "    $name$_ =\n"
      "        com.google.protobuf.GeneratedMessageLite.mutableCopy(tmp);\n"
      "  }\n"
      "}\n");
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_INDEXED_SETTER,
                               context_->options());
  printer->Print(variables_,
                 "private void set$capitalized_name$(\n"
                 "    int index, $type$ value) {\n"
                 "  $null_check$"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  $name$_.setInt(index, value.getNumber());\n"
                 "}\n");
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_ADDER,
                               context_->options());
  printer->Print(variables_,
                 "private void add$capitalized_name$($type$ value) {\n"
                 "  $null_check$"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  $name$_.addInt(value.getNumber());\n"
                 "}\n");
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_MULTI_ADDER,
                               context_->options());
  printer->Print(variables_,
                 "private void addAll$capitalized_name$(\n"
                 "    java.lang.Iterable<? extends $type$> values) {\n"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  for ($type$ value : values) {\n"
                 "    $name$_.addInt(value.getNumber());\n"
                 "  }\n"
                 "}\n");
  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               context_->options());
  printer->Print(variables_,
                 "private void clear$capitalized_name$() {\n"
                 "  $name$_ = emptyIntList();\n"
                 "}\n");

  if (SupportUnknownEnumValue(descriptor_)) {
    WriteFieldEnumValueAccessorDocComment(printer, descriptor_, SETTER,
                                          context_->options());
    printer->Print(variables_,
                   "private void set$capitalized_name$Value(\n"
                   "    int index, int value) {\n"
                   "  ensure$capitalized_name$IsMutable();\n"
                   "  $name$_.setInt(index, value);\n"
                   "}\n");
    WriteFieldEnumValueAccessorDocComment(printer, descriptor_, LIST_ADDER,
                                          context_->options());
    printer->Print(variables_,
                   "private void add$capitalized_name$Value(int value) {\n"
                   "  ensure$capitalized_name$IsMutable();\n"
                   "  $name$_.addInt(value);\n"
                   "}\n");
    WriteFieldEnumValueAccessorDocComment(
        printer, descriptor_, LIST_MULTI_ADDER, context_->options());
    printer->Print(variables_,
                   "private void addAll$capitalized_name$Value(\n"
                   "    java.lang.Iterable<java.lang.Integer> values) {\n"
                   "  ensure$capitalized_name$IsMutable();\n"
                   "  for (int value : values) {\n"
                   "    $name$_.addInt(value);\n"
                   "  }\n"
                   "}\n");
  }
}

void RepeatedImmutableEnumFieldLiteGenerator::GenerateFieldInfo(
    io::Printer* printer, std::vector<uint16_t>* output) const {
  WriteIntToUtf16CharSequence(descriptor_->number(), output);
  WriteIntToUtf16CharSequence(GetExperimentalJavaFieldType(descriptor_),
                              output);
  printer->Print(variables_, "\"$name$_\",\n");
  if (!SupportUnknownEnumValue(descriptor_)) {
    PrintEnumVerifierLogic(printer, descriptor_, variables_,
                           /*var_name=*/"$type$",
                           /*terminating_string=*/",\n",
                           /*enforce_lite=*/context_->EnforceLite());
  }
}

void RepeatedImmutableEnumFieldLiteGenerator::GenerateBuilderMembers(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_GETTER,
                               context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public java.util.List<$type$> "
                 "${$get$capitalized_name$List$}$() {\n"
                 "  return instance.get$capitalized_name$List();\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_COUNT,
                               context_->options());
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public int ${$get$capitalized_name$Count$}$() {\n"
      "  return instance.get$capitalized_name$Count();\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_INDEXED_GETTER,
                               context_->options());
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public $type$ ${$get$capitalized_name$$}$(int index) {\n"
      "  return instance.get$capitalized_name$(index);\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_INDEXED_SETTER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$set$capitalized_name$$}$(\n"
                 "    int index, $type$ value) {\n"
                 "  copyOnWrite();\n"
                 "  instance.set$capitalized_name$(index, value);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_ADDER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(variables_,
                 "$deprecation$public Builder "
                 "${$add$capitalized_name$$}$($type$ value) {\n"
                 "  copyOnWrite();\n"
                 "  instance.add$capitalized_name$(value);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_MULTI_ADDER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$addAll$capitalized_name$$}$(\n"
                 "    java.lang.Iterable<? extends $type$> values) {\n"
                 "  copyOnWrite();\n"
                 "  instance.addAll$capitalized_name$(values);"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$clear$capitalized_name$$}$() {\n"
      "  copyOnWrite();\n"
      "  instance.clear$capitalized_name$();\n"
      "  return this;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);

  if (SupportUnknownEnumValue(descriptor_)) {
    WriteFieldEnumValueAccessorDocComment(printer, descriptor_, LIST_GETTER,
                                          context_->options());
    printer->Print(variables_,
                   "@java.lang.Override\n"
                   "$deprecation$public java.util.List<java.lang.Integer>\n"
                   "${$get$capitalized_name$ValueList$}$() {\n"
                   "  return java.util.Collections.unmodifiableList(\n"
                   "      instance.get$capitalized_name$ValueList());\n"
                   "}\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldEnumValueAccessorDocComment(
        printer, descriptor_, LIST_INDEXED_GETTER, context_->options());
    printer->Print(variables_,
                   "@java.lang.Override\n"
                   "$deprecation$public int "
                   "${$get$capitalized_name$Value$}$(int index) {\n"
                   "  return instance.get$capitalized_name$Value(index);\n"
                   "}\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldEnumValueAccessorDocComment(
        printer, descriptor_, LIST_INDEXED_SETTER, context_->options(),
        /* builder */ true);
    printer->Print(
        variables_,
        "$deprecation$public Builder ${$set$capitalized_name$Value$}$(\n"
        "    int index, int value) {\n"
        "  copyOnWrite();\n"
        "  instance.set$capitalized_name$Value(index, value);\n"
        "  return this;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_, Semantic::kSet);
    WriteFieldEnumValueAccessorDocComment(printer, descriptor_, LIST_ADDER,
                                          context_->options(),
                                          /* builder */ true);
    printer->Print(variables_,
                   "$deprecation$public Builder "
                   "${$add$capitalized_name$Value$}$(int value) {\n"
                   "  copyOnWrite();\n"
                   "  instance.add$capitalized_name$Value(value);\n"
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
        "  copyOnWrite();\n"
        "  instance.addAll$capitalized_name$Value(values);\n"
        "  return this;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_, Semantic::kSet);
  }
}

void RepeatedImmutableEnumFieldLiteGenerator::GenerateInitializationCode(
    io::Printer* printer) const {
  printer->Print(variables_, "$name$_ = emptyIntList();\n");
}

void RepeatedImmutableEnumFieldLiteGenerator::GenerateKotlinDslMembers(
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
                 "$kt_deprecation$ public val $kt_name$: "
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

std::string RepeatedImmutableEnumFieldLiteGenerator::GetBoxedType() const {
  return name_resolver_->GetImmutableClassName(descriptor_->enum_type());
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
