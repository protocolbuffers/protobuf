// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/java/message_field_lite.h"

#include <cstdint>
#include <string>

#include "absl/strings/str_cat.h"
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

void SetMessageVariables(
    const FieldDescriptor* descriptor, int messageBitIndex, int builderBitIndex,
    const FieldGeneratorInfo* info, ClassNameResolver* name_resolver,
    absl::flat_hash_map<absl::string_view, std::string>* variables,
    Context* context) {
  SetCommonFieldVariables(descriptor, info, variables);

  (*variables)["type"] =
      name_resolver->GetImmutableClassName(descriptor->message_type());
  variables->insert({"kt_type", EscapeKotlinKeywords((*variables)["type"])});
  (*variables)["mutable_type"] =
      name_resolver->GetMutableClassName(descriptor->message_type());
  (*variables)["group_or_message"] =
      (GetType(descriptor) == FieldDescriptor::TYPE_GROUP) ? "Group"
                                                           : "Message";
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
                       absl::StrCat((*variables)["name"], "_ != null")});
  }

  (*variables)["get_has_field_bit_from_local"] =
      GenerateGetBitFromLocal(builderBitIndex);
  (*variables)["set_has_field_bit_to_local"] =
      GenerateSetBitToLocal(messageBitIndex);

  // We use `x.getClass()` as a null check because it generates less bytecode
  // than an `if (x == null) { throw ... }` statement.
  (*variables)["null_check"] = "value.getClass();\n";
  // Annotations often use { and } to determine ranges.
  (*variables)["{"] = "";
  (*variables)["}"] = "";
}

}  // namespace

// ===================================================================

ImmutableMessageFieldLiteGenerator::ImmutableMessageFieldLiteGenerator(
    const FieldDescriptor* descriptor, int messageBitIndex, Context* context)
    : descriptor_(descriptor),
      messageBitIndex_(messageBitIndex),
      name_resolver_(context->GetNameResolver()),
      context_(context) {
  SetMessageVariables(descriptor, messageBitIndex, 0,
                      context->GetFieldGeneratorInfo(descriptor),
                      name_resolver_, &variables_, context);
}

ImmutableMessageFieldLiteGenerator::~ImmutableMessageFieldLiteGenerator() {}

int ImmutableMessageFieldLiteGenerator::GetNumBitsForMessage() const {
  // TODO: We don't need a has bit for messages as they have null
  // sentinels and no user should be reflecting on this. We could save some
  // bits by setting to 0 and updating the runtimes but this might come at a
  // runtime performance cost since we can't memoize has-bit reads.
  return HasHasbit(descriptor_) ? 1 : 0;
}

void ImmutableMessageFieldLiteGenerator::GenerateInterfaceMembers(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, HAZZER,
                               context_->options());
  printer->Print(variables_,
                 "$deprecation$boolean ${$has$capitalized_name$$}$();\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, GETTER,
                               context_->options());
  printer->Print(variables_,
                 "$deprecation$$type$ ${$get$capitalized_name$$}$();\n");
  printer->Annotate("{", "}", descriptor_);
}

void ImmutableMessageFieldLiteGenerator::GenerateMembers(
    io::Printer* printer) const {

  printer->Print(variables_, "private $type$ $name$_;\n");
  PrintExtraFieldInfo(variables_, printer);

  if (HasHasbit(descriptor_)) {
    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
        "  return $get_has_field_bit_message$;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$public $type$ ${$get$capitalized_name$$}$() {\n"
        "  return $name$_ == null ? $type$.getDefaultInstance() : $name$_;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
  } else {
    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
        "  return $name$_ != null;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$public $type$ ${$get$capitalized_name$$}$() {\n"
        "  return $name$_ == null ? $type$.getDefaultInstance() : $name$_;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
  }

  // Field.Builder setField(Field value)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "private void set$capitalized_name$($type$ value) {\n"
                 "  $null_check$"
                 "  $name$_ = value;\n"
                 "  $set_has_field_bit_message$\n"
                 "  }\n");

  // Field.Builder mergeField(Field value)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "@java.lang.SuppressWarnings({\"ReferenceEquality\"})\n"
      "private void merge$capitalized_name$($type$ value) {\n"
      "  $null_check$"
      "  if ($name$_ != null &&\n"
      "      $name$_ != $type$.getDefaultInstance()) {\n"
      "    $name$_ =\n"
      "      $type$.newBuilder($name$_).mergeFrom(value).buildPartial();\n"
      "  } else {\n"
      "    $name$_ = value;\n"
      "  }\n"
      "  $set_has_field_bit_message$\n"
      "}\n");

  // Field.Builder clearField()
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "private void clear$capitalized_name$() {"
                 "  $name$_ = null;\n"
                 "  $clear_has_field_bit_message$\n"
                 "}\n");
}

void ImmutableMessageFieldLiteGenerator::GenerateBuilderMembers(
    io::Printer* printer) const {
  // The comments above the methods below are based on a hypothetical
  // field of type "Field" called "Field".

  // boolean hasField()
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
                 "  return instance.has$capitalized_name$();\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  // Field getField()
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public $type$ ${$get$capitalized_name$$}$() {\n"
                 "  return instance.get$capitalized_name$();\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  // Field.Builder setField(Field value)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$public Builder "
                 "${$set$capitalized_name$$}$($type$ value) {\n"
                 "  copyOnWrite();\n"
                 "  instance.set$capitalized_name$(value);\n"
                 "  return this;\n"
                 "  }\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);

  // Field.Builder setField(Field.Builder builderForValue)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$public Builder ${$set$capitalized_name$$}$(\n"
                 "    $type$.Builder builderForValue) {\n"
                 "  copyOnWrite();\n"
                 "  instance.set$capitalized_name$(builderForValue.build());\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);

  // Field.Builder mergeField(Field value)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$public Builder "
                 "${$merge$capitalized_name$$}$($type$ value) {\n"
                 "  copyOnWrite();\n"
                 "  instance.merge$capitalized_name$(value);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);

  // Field.Builder clearField()
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$public Builder ${$clear$capitalized_name$$}$() {"
                 "  copyOnWrite();\n"
                 "  instance.clear$capitalized_name$();\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void ImmutableMessageFieldLiteGenerator::GenerateKotlinDslMembers(
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

  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               context_->options(),
                               /* builder */ false, /* kdoc */ true);
  printer->Print(variables_,
                 "public fun ${$clear$kt_capitalized_name$$}$() {\n"
                 "  $kt_dsl_builder$.${$clear$capitalized_name$$}$()\n"
                 "}\n");

  WriteFieldAccessorDocComment(printer, descriptor_, HAZZER,
                               context_->options(),
                               /* builder */ false, /* kdoc */ true);
  printer->Print(
      variables_,
      "public fun ${$has$kt_capitalized_name$$}$(): kotlin.Boolean {\n"
      "  return $kt_dsl_builder$.${$has$capitalized_name$$}$()\n"
      "}\n");
  GenerateKotlinOrNull(printer);
}

void ImmutableMessageFieldLiteGenerator::GenerateKotlinOrNull(io::Printer* printer) const {
  if (descriptor_->has_presence() &&
      descriptor_->real_containing_oneof() == nullptr) {
    printer->Print(variables_,
                   "public val $classname$Kt.Dsl.$name$OrNull: $kt_type$?\n"
                   "  get() = $kt_dsl_builder$.$name$OrNull\n");
  }
}

void ImmutableMessageFieldLiteGenerator::GenerateFieldInfo(
    io::Printer* printer, std::vector<uint16_t>* output) const {
  WriteIntToUtf16CharSequence(descriptor_->number(), output);
  WriteIntToUtf16CharSequence(GetExperimentalJavaFieldType(descriptor_),
                              output);
  if (HasHasbit(descriptor_)) {
    WriteIntToUtf16CharSequence(messageBitIndex_, output);
  }
  printer->Print(variables_, "\"$name$_\",\n");
}

void ImmutableMessageFieldLiteGenerator::GenerateInitializationCode(
    io::Printer* printer) const {}

std::string ImmutableMessageFieldLiteGenerator::GetBoxedType() const {
  return name_resolver_->GetImmutableClassName(descriptor_->message_type());
}

// ===================================================================

ImmutableMessageOneofFieldLiteGenerator::
    ImmutableMessageOneofFieldLiteGenerator(const FieldDescriptor* descriptor,
                                            int messageBitIndex,
                                            Context* context)
    : ImmutableMessageFieldLiteGenerator(descriptor, messageBitIndex, context) {
  const OneofGeneratorInfo* info =
      context->GetOneofGeneratorInfo(descriptor->containing_oneof());
  SetCommonOneofVariables(descriptor, info, &variables_);
}

ImmutableMessageOneofFieldLiteGenerator::
    ~ImmutableMessageOneofFieldLiteGenerator() {}

void ImmutableMessageOneofFieldLiteGenerator::GenerateMembers(
    io::Printer* printer) const {
  PrintExtraFieldInfo(variables_, printer);
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
                 "  return $has_oneof_case_message$;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public $type$ ${$get$capitalized_name$$}$() {\n"
                 "  if ($has_oneof_case_message$) {\n"
                 "     return ($type$) $oneof_name$_;\n"
                 "  }\n"
                 "  return $type$.getDefaultInstance();\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  // Field.Builder setField(Field value)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "private void set$capitalized_name$($type$ value) {\n"
                 "  $null_check$"
                 "  $oneof_name$_ = value;\n"
                 "  $set_oneof_case_message$;\n"
                 "}\n");

  // Field.Builder mergeField(Field value)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "private void merge$capitalized_name$($type$ value) {\n"
      "  $null_check$"
      "  if ($has_oneof_case_message$ &&\n"
      "      $oneof_name$_ != $type$.getDefaultInstance()) {\n"
      "    $oneof_name$_ = $type$.newBuilder(($type$) $oneof_name$_)\n"
      "        .mergeFrom(value).buildPartial();\n"
      "  } else {\n"
      "    $oneof_name$_ = value;\n"
      "  }\n"
      "  $set_oneof_case_message$;\n"
      "}\n");

  // Field.Builder clearField()
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "private void clear$capitalized_name$() {\n"
                 "  if ($has_oneof_case_message$) {\n"
                 "    $clear_oneof_case_message$;\n"
                 "    $oneof_name$_ = null;\n"
                 "  }\n"
                 "}\n");
}

void ImmutableMessageOneofFieldLiteGenerator::GenerateFieldInfo(
    io::Printer* printer, std::vector<uint16_t>* output) const {
  WriteIntToUtf16CharSequence(descriptor_->number(), output);
  WriteIntToUtf16CharSequence(GetExperimentalJavaFieldType(descriptor_),
                              output);
  WriteIntToUtf16CharSequence(descriptor_->containing_oneof()->index(), output);
  printer->Print(variables_, "$oneof_stored_type$.class,\n");
}

void ImmutableMessageOneofFieldLiteGenerator::GenerateBuilderMembers(
    io::Printer* printer) const {
  // The comments above the methods below are based on a hypothetical
  // field of type "Field" called "Field".

  // boolean hasField()
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
                 "  return instance.has$capitalized_name$();\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  // Field getField()
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public $type$ ${$get$capitalized_name$$}$() {\n"
                 "  return instance.get$capitalized_name$();\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  // Field.Builder setField(Field value)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$public Builder "
                 "${$set$capitalized_name$$}$($type$ value) {\n"
                 "  copyOnWrite();\n"
                 "  instance.set$capitalized_name$(value);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);

  // Field.Builder setField(Field.Builder builderForValue)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$public Builder ${$set$capitalized_name$$}$(\n"
                 "    $type$.Builder builderForValue) {\n"
                 "  copyOnWrite();\n"
                 "  instance.set$capitalized_name$(builderForValue.build());\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);

  // Field.Builder mergeField(Field value)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$public Builder "
                 "${$merge$capitalized_name$$}$($type$ value) {\n"
                 "  copyOnWrite();\n"
                 "  instance.merge$capitalized_name$(value);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);

  // Field.Builder clearField()
  WriteFieldDocComment(printer, descriptor_, context_->options());
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

RepeatedImmutableMessageFieldLiteGenerator::
    RepeatedImmutableMessageFieldLiteGenerator(
        const FieldDescriptor* descriptor, int messageBitIndex,
        Context* context)
    : descriptor_(descriptor),
      name_resolver_(context->GetNameResolver()),
      context_(context) {
  SetMessageVariables(descriptor, messageBitIndex, 0,
                      context->GetFieldGeneratorInfo(descriptor),
                      name_resolver_, &variables_, context);
}

RepeatedImmutableMessageFieldLiteGenerator::
    ~RepeatedImmutableMessageFieldLiteGenerator() {}

int RepeatedImmutableMessageFieldLiteGenerator::GetNumBitsForMessage() const {
  return 0;
}

void RepeatedImmutableMessageFieldLiteGenerator::GenerateInterfaceMembers(
    io::Printer* printer) const {
  // TODO: In the future, consider having methods specific to the
  // interface so that builders can choose dynamically to either return a
  // message or a nested builder, so that asking for the interface doesn't
  // cause a message to ever be built.
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$java.util.List<$type$> \n"
                 "    ${$get$capitalized_name$List$}$();\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "$deprecation$$type$ ${$get$capitalized_name$$}$(int index);\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$int ${$get$capitalized_name$Count$}$();\n");
  printer->Annotate("{", "}", descriptor_);
}

void RepeatedImmutableMessageFieldLiteGenerator::GenerateMembers(
    io::Printer* printer) const {
  printer->Print(
      variables_,
      "private com.google.protobuf.Internal.ProtobufList<$type$> $name$_;\n");
  PrintExtraFieldInfo(variables_, printer);
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public java.util.List<$type$> "
                 "${$get$capitalized_name$List$}$() {\n"
                 "  return $name$_;\n"  // note:  unmodifiable list
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "$deprecation$public java.util.List<? extends $type$OrBuilder> \n"
      "    ${$get$capitalized_name$OrBuilderList$}$() {\n"
      "  return $name$_;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public int ${$get$capitalized_name$Count$}$() {\n"
      "  return $name$_.size();\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public $type$ ${$get$capitalized_name$$}$(int index) {\n"
      "  return $name$_.get(index);\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$public $type$OrBuilder "
                 "${$get$capitalized_name$OrBuilder$}$(\n"
                 "    int index) {\n"
                 "  return $name$_.get(index);\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  printer->Print(
      variables_,
      "private void ensure$capitalized_name$IsMutable() {\n"
      // Use a temporary to avoid a redundant iget-object.
      "  com.google.protobuf.Internal.ProtobufList<$type$> tmp = $name$_;\n"
      "  if (!tmp.isModifiable()) {\n"
      "    $name$_ =\n"
      "        com.google.protobuf.GeneratedMessageLite.mutableCopy(tmp);\n"
      "   }\n"
      "}\n"
      "\n");

  // Builder setRepeatedField(int index, Field value)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "private void set$capitalized_name$(\n"
                 "    int index, $type$ value) {\n"
                 "  $null_check$"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  $name$_.set(index, value);\n"
                 "}\n");

  // Builder addRepeatedField(Field value)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "private void add$capitalized_name$($type$ value) {\n"
                 "  $null_check$"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  $name$_.add(value);\n"
                 "}\n");

  // Builder addRepeatedField(int index, Field value)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "private void add$capitalized_name$(\n"
                 "    int index, $type$ value) {\n"
                 "  $null_check$"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  $name$_.add(index, value);\n"
                 "}\n");

  // Builder addAllRepeatedField(Iterable<Field> values)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "private void addAll$capitalized_name$(\n"
                 "    java.lang.Iterable<? extends $type$> values) {\n"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  com.google.protobuf.AbstractMessageLite.addAll(\n"
                 "      values, $name$_);\n"
                 "}\n");

  // Builder clearAllRepeatedField()
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "private void clear$capitalized_name$() {\n"
                 "  $name$_ = emptyProtobufList();\n"
                 "}\n");

  // Builder removeRepeatedField(int index)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "private void remove$capitalized_name$(int index) {\n"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  $name$_.remove(index);\n"
                 "}\n");
}

void RepeatedImmutableMessageFieldLiteGenerator::GenerateBuilderMembers(
    io::Printer* printer) const {
  // The comments above the methods below are based on a hypothetical
  // repeated field of type "Field" called "RepeatedField".

  // List<Field> getRepeatedFieldList()
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public java.util.List<$type$> "
                 "${$get$capitalized_name$List$}$() {\n"
                 "  return java.util.Collections.unmodifiableList(\n"
                 "      instance.get$capitalized_name$List());\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  // int getRepeatedFieldCount()
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public int ${$get$capitalized_name$Count$}$() {\n"
      "  return instance.get$capitalized_name$Count();\n"
      "}");
  printer->Annotate("{", "}", descriptor_);

  // Field getRepeatedField(int index)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public $type$ ${$get$capitalized_name$$}$(int index) {\n"
      "  return instance.get$capitalized_name$(index);\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);

  // Builder setRepeatedField(int index, Field value)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$public Builder ${$set$capitalized_name$$}$(\n"
                 "    int index, $type$ value) {\n"
                 "  copyOnWrite();\n"
                 "  instance.set$capitalized_name$(index, value);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);

  // Builder setRepeatedField(int index, Field.Builder builderForValue)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$public Builder ${$set$capitalized_name$$}$(\n"
                 "    int index, $type$.Builder builderForValue) {\n"
                 "  copyOnWrite();\n"
                 "  instance.set$capitalized_name$(index,\n"
                 "      builderForValue.build());\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);

  // Builder addRepeatedField(Field value)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$public Builder "
                 "${$add$capitalized_name$$}$($type$ value) {\n"
                 "  copyOnWrite();\n"
                 "  instance.add$capitalized_name$(value);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);

  // Builder addRepeatedField(int index, Field value)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$public Builder ${$add$capitalized_name$$}$(\n"
                 "    int index, $type$ value) {\n"
                 "  copyOnWrite();\n"
                 "  instance.add$capitalized_name$(index, value);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
  // Builder addRepeatedField(Field.Builder builderForValue)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$public Builder ${$add$capitalized_name$$}$(\n"
                 "    $type$.Builder builderForValue) {\n"
                 "  copyOnWrite();\n"
                 "  instance.add$capitalized_name$(builderForValue.build());\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);

  // Builder addRepeatedField(int index, Field.Builder builderForValue)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$public Builder ${$add$capitalized_name$$}$(\n"
                 "    int index, $type$.Builder builderForValue) {\n"
                 "  copyOnWrite();\n"
                 "  instance.add$capitalized_name$(index,\n"
                 "      builderForValue.build());\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);

  // Builder addAllRepeatedField(Iterable<Field> values)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$public Builder ${$addAll$capitalized_name$$}$(\n"
                 "    java.lang.Iterable<? extends $type$> values) {\n"
                 "  copyOnWrite();\n"
                 "  instance.addAll$capitalized_name$(values);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);

  // Builder clearAllRepeatedField()
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$clear$capitalized_name$$}$() {\n"
      "  copyOnWrite();\n"
      "  instance.clear$capitalized_name$();\n"
      "  return this;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);

  // Builder removeRepeatedField(int index)
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$public Builder "
                 "${$remove$capitalized_name$$}$(int index) {\n"
                 "  copyOnWrite();\n"
                 "  instance.remove$capitalized_name$(index);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void RepeatedImmutableMessageFieldLiteGenerator::GenerateFieldInfo(
    io::Printer* printer, std::vector<uint16_t>* output) const {
  WriteIntToUtf16CharSequence(descriptor_->number(), output);
  WriteIntToUtf16CharSequence(GetExperimentalJavaFieldType(descriptor_),
                              output);
  printer->Print(variables_,
                 "\"$name$_\",\n"
                 "$type$.class,\n");
}

void RepeatedImmutableMessageFieldLiteGenerator::GenerateInitializationCode(
    io::Printer* printer) const {
  printer->Print(variables_, "$name$_ = emptyProtobufList();\n");
}

std::string RepeatedImmutableMessageFieldLiteGenerator::GetBoxedType() const {
  return name_resolver_->GetImmutableClassName(descriptor_->message_type());
}

void RepeatedImmutableMessageFieldLiteGenerator::GenerateKotlinDslMembers(
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
                 "}\n");

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
                 "}\n");

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
                 "}\n");

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
      "}\n");

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
      "}\n");

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
                 "}\n");
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
