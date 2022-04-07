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

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <google/protobuf/compiler/java/message_field_lite.h>

#include <cstdint>
#include <map>
#include <string>

#include <google/protobuf/io/printer.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/compiler/java/context.h>
#include <google/protobuf/compiler/java/doc_comment.h>
#include <google/protobuf/compiler/java/helpers.h>
#include <google/protobuf/compiler/java/name_resolver.h>

// Must be last.
#include <google/protobuf/port_def.inc>

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

namespace {

void SetMessageVariables(const FieldDescriptor* descriptor, int messageBitIndex,
                         int builderBitIndex, const FieldGeneratorInfo* info,
                         ClassNameResolver* name_resolver,
                         std::map<std::string, std::string>* variables) {
  SetCommonFieldVariables(descriptor, info, variables);

  (*variables)["type"] =
      name_resolver->GetImmutableClassName(descriptor->message_type());
  (*variables)["kt_type"] = (*variables)["type"];
  (*variables)["mutable_type"] =
      name_resolver->GetMutableClassName(descriptor->message_type());
  (*variables)["group_or_message"] =
      (GetType(descriptor) == FieldDescriptor::TYPE_GROUP) ? "Group"
                                                           : "Message";
  // TODO(birdo): Add @deprecated javadoc when generating javadoc is supported
  // by the proto compiler
  (*variables)["deprecation"] =
      descriptor->options().deprecated() ? "@java.lang.Deprecated " : "";
  (*variables)["kt_deprecation"] =
      descriptor->options().deprecated()
          ? "@kotlin.Deprecated(message = \"Field " + (*variables)["name"] +
                " is deprecated\") "
          : "";
  (*variables)["required"] = descriptor->is_required() ? "true" : "false";

  if (HasHasbit(descriptor)) {
    // For singular messages and builders, one bit is used for the hasField bit.
    (*variables)["get_has_field_bit_message"] = GenerateGetBit(messageBitIndex);

    // Note that these have a trailing ";".
    (*variables)["set_has_field_bit_message"] =
        GenerateSetBit(messageBitIndex) + ";";
    (*variables)["clear_has_field_bit_message"] =
        GenerateClearBit(messageBitIndex) + ";";

    (*variables)["is_field_present_message"] = GenerateGetBit(messageBitIndex);
  } else {
    (*variables)["set_has_field_bit_message"] = "";
    (*variables)["clear_has_field_bit_message"] = "";

    (*variables)["is_field_present_message"] =
        (*variables)["name"] + "_ != null";
  }

  (*variables)["get_has_field_bit_from_local"] =
      GenerateGetBitFromLocal(builderBitIndex);
  (*variables)["set_has_field_bit_to_local"] =
      GenerateSetBitToLocal(messageBitIndex);

  // We use `x.getClass()` as a null check because it generates less bytecode
  // than an `if (x == null) { throw ... }` statement.
  (*variables)["null_check"] = "value.getClass();\n";
}

}  // namespace

// ===================================================================

ImmutableMessageFieldLiteGenerator::ImmutableMessageFieldLiteGenerator(
    const FieldDescriptor* descriptor, int messageBitIndex, Context* context)
    : descriptor_(descriptor),
      messageBitIndex_(messageBitIndex),
      name_resolver_(context->GetNameResolver()) {
  SetMessageVariables(descriptor, messageBitIndex, 0,
                      context->GetFieldGeneratorInfo(descriptor),
                      name_resolver_, &variables_);
}

ImmutableMessageFieldLiteGenerator::~ImmutableMessageFieldLiteGenerator() {}

int ImmutableMessageFieldLiteGenerator::GetNumBitsForMessage() const {
  // TODO(dweis): We don't need a has bit for messages as they have null
  // sentinels and no user should be reflecting on this. We could save some
  // bits by setting to 0 and updating the runtimes but this might come at a
  // runtime performance cost since we can't memoize has-bit reads.
  return HasHasbit(descriptor_) ? 1 : 0;
}

void ImmutableMessageFieldLiteGenerator::GenerateInterfaceMembers(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, HAZZER);
  printer->Print(variables_, "$deprecation$boolean has$capitalized_name$();\n");
  WriteFieldAccessorDocComment(printer, descriptor_, GETTER);
  printer->Print(variables_, "$deprecation$$type$ get$capitalized_name$();\n");
}

void ImmutableMessageFieldLiteGenerator::GenerateMembers(
    io::Printer* printer) const {

  printer->Print(variables_, "private $type$ $name$_;\n");
  PrintExtraFieldInfo(variables_, printer);

  if (HasHasbit(descriptor_)) {
    WriteFieldDocComment(printer, descriptor_);
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
        "  return $get_has_field_bit_message$;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldDocComment(printer, descriptor_);
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$public $type$ ${$get$capitalized_name$$}$() {\n"
        "  return $name$_ == null ? $type$.getDefaultInstance() : $name$_;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
  } else {
    WriteFieldDocComment(printer, descriptor_);
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
        "  return $name$_ != null;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldDocComment(printer, descriptor_);
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$public $type$ ${$get$capitalized_name$$}$() {\n"
        "  return $name$_ == null ? $type$.getDefaultInstance() : $name$_;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
  }

  // Field.Builder setField(Field value)
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "private void set$capitalized_name$($type$ value) {\n"
                 "  $null_check$"
                 "  $name$_ = value;\n"
                 "  $set_has_field_bit_message$\n"
                 "  }\n");

  // Field.Builder mergeField(Field value)
  WriteFieldDocComment(printer, descriptor_);
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
  WriteFieldDocComment(printer, descriptor_);
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
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
                 "  return instance.has$capitalized_name$();\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  // Field getField()
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public $type$ ${$get$capitalized_name$$}$() {\n"
                 "  return instance.get$capitalized_name$();\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  // Field.Builder setField(Field value)
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "$deprecation$public Builder "
                 "${$set$capitalized_name$$}$($type$ value) {\n"
                 "  copyOnWrite();\n"
                 "  instance.set$capitalized_name$(value);\n"
                 "  return this;\n"
                 "  }\n");
  printer->Annotate("{", "}", descriptor_);

  // Field.Builder setField(Field.Builder builderForValue)
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$set$capitalized_name$$}$(\n"
                 "    $type$.Builder builderForValue) {\n"
                 "  copyOnWrite();\n"
                 "  instance.set$capitalized_name$(builderForValue.build());\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  // Field.Builder mergeField(Field value)
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "$deprecation$public Builder "
                 "${$merge$capitalized_name$$}$($type$ value) {\n"
                 "  copyOnWrite();\n"
                 "  instance.merge$capitalized_name$(value);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  // Field.Builder clearField()
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$clear$capitalized_name$$}$() {"
                 "  copyOnWrite();\n"
                 "  instance.clear$capitalized_name$();\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void ImmutableMessageFieldLiteGenerator::GenerateKotlinDslMembers(
    io::Printer* printer) const {
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "$kt_deprecation$var $kt_name$: $kt_type$\n"
                 "  @JvmName(\"${$get$kt_capitalized_name$$}$\")\n"
                 "  get() = $kt_dsl_builder$.${$get$capitalized_name$$}$()\n"
                 "  @JvmName(\"${$set$kt_capitalized_name$$}$\")\n"
                 "  set(value) {\n"
                 "    $kt_dsl_builder$.${$set$capitalized_name$$}$(value)\n"
                 "  }\n");

  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               /* builder */ false);
  printer->Print(variables_,
                 "fun ${$clear$kt_capitalized_name$$}$() {\n"
                 "  $kt_dsl_builder$.${$clear$capitalized_name$$}$()\n"
                 "}\n");

  WriteFieldAccessorDocComment(printer, descriptor_, HAZZER);
  printer->Print(
      variables_,
      "fun ${$has$kt_capitalized_name$$}$(): kotlin.Boolean {\n"
      "  return $kt_dsl_builder$.${$has$capitalized_name$$}$()\n"
      "}\n");
  GenerateKotlinOrNull(printer);
}

void ImmutableMessageFieldLiteGenerator::GenerateKotlinOrNull(io::Printer* printer) const {
  if (descriptor_->has_optional_keyword()) {
    printer->Print(variables_,
                   "val $classname$Kt.Dsl.$name$OrNull: $kt_type$?\n"
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
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
                 "  return $has_oneof_case_message$;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldDocComment(printer, descriptor_);
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
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "private void set$capitalized_name$($type$ value) {\n"
                 "  $null_check$"
                 "  $oneof_name$_ = value;\n"
                 "  $set_oneof_case_message$;\n"
                 "}\n");

  // Field.Builder mergeField(Field value)
  WriteFieldDocComment(printer, descriptor_);
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
  WriteFieldDocComment(printer, descriptor_);
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
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
                 "  return instance.has$capitalized_name$();\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  // Field getField()
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public $type$ ${$get$capitalized_name$$}$() {\n"
                 "  return instance.get$capitalized_name$();\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  // Field.Builder setField(Field value)
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "$deprecation$public Builder "
                 "${$set$capitalized_name$$}$($type$ value) {\n"
                 "  copyOnWrite();\n"
                 "  instance.set$capitalized_name$(value);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  // Field.Builder setField(Field.Builder builderForValue)
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$set$capitalized_name$$}$(\n"
                 "    $type$.Builder builderForValue) {\n"
                 "  copyOnWrite();\n"
                 "  instance.set$capitalized_name$(builderForValue.build());\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  // Field.Builder mergeField(Field value)
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "$deprecation$public Builder "
                 "${$merge$capitalized_name$$}$($type$ value) {\n"
                 "  copyOnWrite();\n"
                 "  instance.merge$capitalized_name$(value);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  // Field.Builder clearField()
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$clear$capitalized_name$$}$() {\n"
      "  copyOnWrite();\n"
      "  instance.clear$capitalized_name$();\n"
      "  return this;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
}

// ===================================================================

RepeatedImmutableMessageFieldLiteGenerator::
    RepeatedImmutableMessageFieldLiteGenerator(
        const FieldDescriptor* descriptor, int messageBitIndex,
        Context* context)
    : descriptor_(descriptor), name_resolver_(context->GetNameResolver()) {
  SetMessageVariables(descriptor, messageBitIndex, 0,
                      context->GetFieldGeneratorInfo(descriptor),
                      name_resolver_, &variables_);
}

RepeatedImmutableMessageFieldLiteGenerator::
    ~RepeatedImmutableMessageFieldLiteGenerator() {}

int RepeatedImmutableMessageFieldLiteGenerator::GetNumBitsForMessage() const {
  return 0;
}

void RepeatedImmutableMessageFieldLiteGenerator::GenerateInterfaceMembers(
    io::Printer* printer) const {
  // TODO(jonp): In the future, consider having methods specific to the
  // interface so that builders can choose dynamically to either return a
  // message or a nested builder, so that asking for the interface doesn't
  // cause a message to ever be built.
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "$deprecation$java.util.List<$type$> \n"
                 "    get$capitalized_name$List();\n");
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "$deprecation$$type$ get$capitalized_name$(int index);\n");
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "$deprecation$int get$capitalized_name$Count();\n");
}

void RepeatedImmutableMessageFieldLiteGenerator::GenerateMembers(
    io::Printer* printer) const {
  printer->Print(
      variables_,
      "private com.google.protobuf.Internal.ProtobufList<$type$> $name$_;\n");
  PrintExtraFieldInfo(variables_, printer);
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public java.util.List<$type$> "
                 "${$get$capitalized_name$List$}$() {\n"
                 "  return $name$_;\n"  // note:  unmodifiable list
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(
      variables_,
      "$deprecation$public java.util.List<? extends $type$OrBuilder> \n"
      "    ${$get$capitalized_name$OrBuilderList$}$() {\n"
      "  return $name$_;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public int ${$get$capitalized_name$Count$}$() {\n"
      "  return $name$_.size();\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public $type$ ${$get$capitalized_name$$}$(int index) {\n"
      "  return $name$_.get(index);\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldDocComment(printer, descriptor_);
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
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "private void set$capitalized_name$(\n"
                 "    int index, $type$ value) {\n"
                 "  $null_check$"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  $name$_.set(index, value);\n"
                 "}\n");

  // Builder addRepeatedField(Field value)
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "private void add$capitalized_name$($type$ value) {\n"
                 "  $null_check$"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  $name$_.add(value);\n"
                 "}\n");

  // Builder addRepeatedField(int index, Field value)
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "private void add$capitalized_name$(\n"
                 "    int index, $type$ value) {\n"
                 "  $null_check$"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  $name$_.add(index, value);\n"
                 "}\n");

  // Builder addAllRepeatedField(Iterable<Field> values)
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "private void addAll$capitalized_name$(\n"
                 "    java.lang.Iterable<? extends $type$> values) {\n"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  com.google.protobuf.AbstractMessageLite.addAll(\n"
                 "      values, $name$_);\n"
                 "}\n");

  // Builder clearAllRepeatedField()
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "private void clear$capitalized_name$() {\n"
                 "  $name$_ = emptyProtobufList();\n"
                 "}\n");

  // Builder removeRepeatedField(int index)
  WriteFieldDocComment(printer, descriptor_);
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
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public java.util.List<$type$> "
                 "${$get$capitalized_name$List$}$() {\n"
                 "  return java.util.Collections.unmodifiableList(\n"
                 "      instance.get$capitalized_name$List());\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  // int getRepeatedFieldCount()
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public int ${$get$capitalized_name$Count$}$() {\n"
      "  return instance.get$capitalized_name$Count();\n"
      "}");
  printer->Annotate("{", "}", descriptor_);

  // Field getRepeatedField(int index)
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public $type$ ${$get$capitalized_name$$}$(int index) {\n"
      "  return instance.get$capitalized_name$(index);\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);

  // Builder setRepeatedField(int index, Field value)
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$set$capitalized_name$$}$(\n"
                 "    int index, $type$ value) {\n"
                 "  copyOnWrite();\n"
                 "  instance.set$capitalized_name$(index, value);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  // Builder setRepeatedField(int index, Field.Builder builderForValue)
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$set$capitalized_name$$}$(\n"
                 "    int index, $type$.Builder builderForValue) {\n"
                 "  copyOnWrite();\n"
                 "  instance.set$capitalized_name$(index,\n"
                 "      builderForValue.build());\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  // Builder addRepeatedField(Field value)
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "$deprecation$public Builder "
                 "${$add$capitalized_name$$}$($type$ value) {\n"
                 "  copyOnWrite();\n"
                 "  instance.add$capitalized_name$(value);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  // Builder addRepeatedField(int index, Field value)
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$add$capitalized_name$$}$(\n"
                 "    int index, $type$ value) {\n"
                 "  copyOnWrite();\n"
                 "  instance.add$capitalized_name$(index, value);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
  // Builder addRepeatedField(Field.Builder builderForValue)
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$add$capitalized_name$$}$(\n"
                 "    $type$.Builder builderForValue) {\n"
                 "  copyOnWrite();\n"
                 "  instance.add$capitalized_name$(builderForValue.build());\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  // Builder addRepeatedField(int index, Field.Builder builderForValue)
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$add$capitalized_name$$}$(\n"
                 "    int index, $type$.Builder builderForValue) {\n"
                 "  copyOnWrite();\n"
                 "  instance.add$capitalized_name$(index,\n"
                 "      builderForValue.build());\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  // Builder addAllRepeatedField(Iterable<Field> values)
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$addAll$capitalized_name$$}$(\n"
                 "    java.lang.Iterable<? extends $type$> values) {\n"
                 "  copyOnWrite();\n"
                 "  instance.addAll$capitalized_name$(values);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  // Builder clearAllRepeatedField()
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$clear$capitalized_name$$}$() {\n"
      "  copyOnWrite();\n"
      "  instance.clear$capitalized_name$();\n"
      "  return this;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);

  // Builder removeRepeatedField(int index)
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "$deprecation$public Builder "
                 "${$remove$capitalized_name$$}$(int index) {\n"
                 "  copyOnWrite();\n"
                 "  instance.remove$capitalized_name$(index);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
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
      "class ${$$kt_capitalized_name$Proxy$}$ private constructor()"
      " : com.google.protobuf.kotlin.DslProxy()\n");

  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "$kt_deprecation$ val $kt_name$: "
                 "com.google.protobuf.kotlin.DslList"
                 "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>\n"
                 "  @kotlin.jvm.JvmSynthetic\n"
                 "  get() = com.google.protobuf.kotlin.DslList(\n"
                 "    $kt_dsl_builder$.${$get$capitalized_name$List$}$()\n"
                 "  )\n");

  WriteFieldAccessorDocComment(printer, descriptor_, LIST_ADDER,
                               /* builder */ false);
  printer->Print(variables_,
                 "@kotlin.jvm.JvmSynthetic\n"
                 "@kotlin.jvm.JvmName(\"add$kt_capitalized_name$\")\n"
                 "fun com.google.protobuf.kotlin.DslList"
                 "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
                 "add(value: $kt_type$) {\n"
                 "  $kt_dsl_builder$.${$add$capitalized_name$$}$(value)\n"
                 "}\n");

  WriteFieldAccessorDocComment(printer, descriptor_, LIST_ADDER,
                               /* builder */ false);
  printer->Print(variables_,
                 "@kotlin.jvm.JvmSynthetic\n"
                 "@kotlin.jvm.JvmName(\"plusAssign$kt_capitalized_name$\")\n"
                 "@Suppress(\"NOTHING_TO_INLINE\")\n"
                 "inline operator fun com.google.protobuf.kotlin.DslList"
                 "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
                 "plusAssign(value: $kt_type$) {\n"
                 "  add(value)\n"
                 "}\n");

  WriteFieldAccessorDocComment(printer, descriptor_, LIST_MULTI_ADDER,
                               /* builder */ false);
  printer->Print(variables_,
                 "@kotlin.jvm.JvmSynthetic\n"
                 "@kotlin.jvm.JvmName(\"addAll$kt_capitalized_name$\")\n"
                 "fun com.google.protobuf.kotlin.DslList"
                 "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
                 "addAll(values: kotlin.collections.Iterable<$kt_type$>) {\n"
                 "  $kt_dsl_builder$.${$addAll$capitalized_name$$}$(values)\n"
                 "}\n");

  WriteFieldAccessorDocComment(printer, descriptor_, LIST_MULTI_ADDER,
                               /* builder */ false);
  printer->Print(
      variables_,
      "@kotlin.jvm.JvmSynthetic\n"
      "@kotlin.jvm.JvmName(\"plusAssignAll$kt_capitalized_name$\")\n"
      "@Suppress(\"NOTHING_TO_INLINE\")\n"
      "inline operator fun com.google.protobuf.kotlin.DslList"
      "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
      "plusAssign(values: kotlin.collections.Iterable<$kt_type$>) {\n"
      "  addAll(values)\n"
      "}\n");

  WriteFieldAccessorDocComment(printer, descriptor_, LIST_INDEXED_SETTER,
                               /* builder */ false);
  printer->Print(
      variables_,
      "@kotlin.jvm.JvmSynthetic\n"
      "@kotlin.jvm.JvmName(\"set$kt_capitalized_name$\")\n"
      "operator fun com.google.protobuf.kotlin.DslList"
      "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
      "set(index: kotlin.Int, value: $kt_type$) {\n"
      "  $kt_dsl_builder$.${$set$capitalized_name$$}$(index, value)\n"
      "}\n");

  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               /* builder */ false);
  printer->Print(variables_,
                 "@kotlin.jvm.JvmSynthetic\n"
                 "@kotlin.jvm.JvmName(\"clear$kt_capitalized_name$\")\n"
                 "fun com.google.protobuf.kotlin.DslList"
                 "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
                 "clear() {\n"
                 "  $kt_dsl_builder$.${$clear$capitalized_name$$}$()\n"
                 "}\n");
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include <google/protobuf/port_undef.inc>
