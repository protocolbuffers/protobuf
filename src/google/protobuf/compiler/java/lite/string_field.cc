// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
// Author: jonp@google.com (Jon Perlow)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/java/lite/string_field.h"

#include <cstdint>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "google/protobuf/compiler/java/context.h"
#include "google/protobuf/compiler/java/doc_comment.h"
#include "google/protobuf/compiler/java/field_common.h"
#include "google/protobuf/compiler/java/helpers.h"
#include "google/protobuf/compiler/java/internal_helpers.h"
#include "google/protobuf/compiler/java/name_resolver.h"
#include "google/protobuf/wire_format.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

using internal::WireFormat;
using internal::WireFormatLite;
using Semantic = ::google::protobuf::io::AnnotationCollector::Semantic;

namespace {

void SetPrimitiveVariables(
    const FieldDescriptor* descriptor, int messageBitIndex, int builderBitIndex,
    const FieldGeneratorInfo* info, ClassNameResolver* name_resolver,
    absl::flat_hash_map<absl::string_view, std::string>* variables,
    Context* context) {
  SetCommonFieldVariables(descriptor, info, variables);

  (*variables)["empty_list"] =
      "com.google.protobuf.GeneratedMessageLite.emptyProtobufList()";

  (*variables)["default"] =
      ImmutableDefaultValue(descriptor, name_resolver, context->options());
  (*variables)["default_init"] = absl::StrCat(
      "= ",
      ImmutableDefaultValue(descriptor, name_resolver, context->options()));
  (*variables)["capitalized_type"] = "java.lang.String";
  (*variables)["tag"] =
      absl::StrCat(static_cast<int32_t>(WireFormat::MakeTag(descriptor)));
  (*variables)["tag_size"] = absl::StrCat(
      WireFormat::TagSize(descriptor->number(), GetType(descriptor)));

  // TODO: Add @deprecated javadoc when generating javadoc is supported
  // by the proto compiler
  (*variables)["deprecation"] =
      descriptor->options().deprecated() ? "@java.lang.Deprecated " : "";
  (*variables)["required"] = descriptor->is_required() ? "true" : "false";
  if (!context->options().opensource_runtime) {
    (*variables)["enforce_utf8"] = CheckUtf8(descriptor) ? "true" : "false";
  }

  if (HasHasbit(descriptor)) {
    if (!context->options().opensource_runtime) {
      (*variables)["bit_field_id"] = absl::StrCat(messageBitIndex / 32);
      (*variables)["bit_field_name"] = GetBitFieldNameForBit(messageBitIndex);
      (*variables)["bit_field_mask"] =
          absl::StrCat(1 << (messageBitIndex % 32));
    }
    // For singular messages and builders, one bit is used for the hasField bit.
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
                       absl::StrCat("!", (*variables)["name"], "_.isEmpty()")});
  }

  // Annotations often use { and } variables to denote text ranges.
  (*variables)["{"] = "";
  (*variables)["}"] = "";
}

}  // namespace

// ===================================================================

ImmutableStringFieldLiteGenerator::ImmutableStringFieldLiteGenerator(
    const FieldDescriptor* descriptor, int messageBitIndex, Context* context)
    : descriptor_(descriptor),
      messageBitIndex_(messageBitIndex),
      name_resolver_(context->GetNameResolver()),
      context_(context) {
  SetPrimitiveVariables(descriptor, messageBitIndex, 0,
                        context->GetFieldGeneratorInfo(descriptor),
                        name_resolver_, &variables_, context);
}

ImmutableStringFieldLiteGenerator::~ImmutableStringFieldLiteGenerator() {}

int ImmutableStringFieldLiteGenerator::GetNumBitsForMessage() const {
  return HasHasbit(descriptor_) ? 1 : 0;
}

// A note about how strings are handled. In the SPEED and CODE_SIZE runtimes,
// strings are not stored as java.lang.String in the Message because of two
// issues:
//
//  1. It wouldn't roundtrip byte arrays that were not valid UTF-8 encoded
//     strings, but rather fields that were raw bytes incorrectly marked
//     as strings in the proto file. This is common because in the proto1
//     syntax, string was the way to indicate bytes and C++ engineers can
//     easily make this mistake without affecting the C++ API. By converting to
//     strings immediately, some java code might corrupt these byte arrays as
//     it passes through a java server even if the field was never accessed by
//     application code.
//
//  2. There's a performance hit to converting between bytes and strings and
//     it many cases, the field is never even read by the application code. This
//     avoids unnecessary conversions in the common use cases.
//
// In the LITE_RUNTIME, we store strings as java.lang.String because we assume
// that the users of this runtime are not subject to proto1 constraints and are
// running code on devices that are user facing. That is, the developers are
// properly incentivized to only fetch the data they need to read and wish to
// reduce the number of allocations incurred when running on a user's device.

// TODO: Consider dropping all of the *Bytes() methods. They really
//     shouldn't be necessary or used on devices.
void ImmutableStringFieldLiteGenerator::GenerateInterfaceMembers(
    io::Printer* printer) const {
  if (descriptor_->has_presence()) {
    WriteFieldAccessorDocComment(printer, descriptor_, HAZZER,
                                 context_->options());
    printer->Print(variables_,
                   "$deprecation$boolean ${$has$capitalized_name$$}$();\n");
    printer->Annotate("{", "}", descriptor_);
  }
  WriteFieldAccessorDocComment(printer, descriptor_, GETTER,
                               context_->options());
  printer->Print(
      variables_,
      "$deprecation$java.lang.String ${$get$capitalized_name$$}$();\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, GETTER,
                                          context_->options());
  printer->Print(variables_,
                 "$deprecation$com.google.protobuf.ByteString\n"
                 "    ${$get$capitalized_name$Bytes$}$();\n");
  printer->Annotate("{", "}", descriptor_);
}

void ImmutableStringFieldLiteGenerator::GenerateMembers(
    io::Printer* printer) const {
  if (!context_->options().opensource_runtime) {
    printer->Print(
        variables_,
        "@com.google.protobuf.ProtoField(\n"
        "  isRequired=$required$)\n");
    if (HasHasbit(descriptor_)) {
      printer->Print(variables_,
                     "@com.google.protobuf.ProtoPresenceCheckedField(\n"
                     "  presenceBitsId=$bit_field_id$,\n"
                     "  mask=$bit_field_mask$)\n");
    }
  }
  printer->Print(variables_, "private java.lang.String $name$_;\n");
  PrintExtraFieldInfo(variables_, printer);

  if (descriptor_->has_presence()) {
    WriteFieldAccessorDocComment(printer, descriptor_, HAZZER,
                                 context_->options());
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
        "  return $is_field_present_message$;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
  }

  WriteFieldAccessorDocComment(printer, descriptor_, GETTER,
                               context_->options());
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public java.lang.String ${$get$capitalized_name$$}$() {\n"
      "  return $name$_;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, GETTER,
                                          context_->options());
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public com.google.protobuf.ByteString\n"
      "    ${$get$capitalized_name$Bytes$}$() {\n"
      "  return com.google.protobuf.ByteString.copyFromUtf8($name$_);\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);

  WriteFieldAccessorDocComment(printer, descriptor_, SETTER,
                               context_->options());
  printer->Print(variables_,
                 "@java.lang.SuppressWarnings(\"ReturnValueIgnored\")\n"
                 "private void set$capitalized_name$(\n"
                 "    java.lang.String value) {\n"
                 "  value.getClass();  // minimal bytecode null check\n"
                 "  $set_has_field_bit_message$\n"
                 "  $name$_ = value;\n"
                 "}\n");
  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               context_->options());
  printer->Print(variables_,
                 "private void clear$capitalized_name$() {\n"
                 "  $clear_has_field_bit_message$\n"
                 // The default value is not a simple literal so we want to
                 // avoid executing it multiple times.  Instead, get the default
                 // out of the default instance.
                 "  $name$_ = getDefaultInstance().get$capitalized_name$();\n"
                 "}\n");

  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, SETTER,
                                          context_->options());
  printer->Print(variables_,
                 "private void set$capitalized_name$Bytes(\n"
                 "    com.google.protobuf.ByteString value) {\n");
  if (CheckUtf8(descriptor_)) {
    printer->Print(variables_, "  checkByteStringIsUtf8(value);\n");
  }
  printer->Print(variables_,
                 "  $name$_ = value.toStringUtf8();\n"
                 "  $set_has_field_bit_message$\n"
                 "}\n");
}

void ImmutableStringFieldLiteGenerator::GenerateBuilderMembers(
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

  WriteFieldAccessorDocComment(printer, descriptor_, GETTER,
                               context_->options());
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public java.lang.String ${$get$capitalized_name$$}$() {\n"
      "  return instance.get$capitalized_name$();\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);

  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, GETTER,
                                          context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public com.google.protobuf.ByteString\n"
                 "    ${$get$capitalized_name$Bytes$}$() {\n"
                 "  return instance.get$capitalized_name$Bytes();\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  WriteFieldAccessorDocComment(printer, descriptor_, SETTER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$set$capitalized_name$$}$(\n"
                 "    java.lang.String value) {\n"
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

  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, SETTER,
                                          context_->options(),
                                          /* builder */ true);
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$set$capitalized_name$Bytes$}$(\n"
      "    com.google.protobuf.ByteString value) {\n"
      "  copyOnWrite();\n"
      "  instance.set$capitalized_name$Bytes(value);\n"
      "  return this;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void ImmutableStringFieldLiteGenerator::GenerateFieldInfo(
    io::Printer* printer, std::vector<uint16_t>* output) const {
  WriteIntToUtf16CharSequence(descriptor_->number(), output);
  WriteIntToUtf16CharSequence(GetExperimentalJavaFieldType(descriptor_),
                              output);
  if (HasHasbit(descriptor_)) {
    WriteIntToUtf16CharSequence(messageBitIndex_, output);
  }
  printer->Print(variables_, "\"$name$_\",\n");
}

void ImmutableStringFieldLiteGenerator::GenerateInitializationCode(
    io::Printer* printer) const {
  printer->Print(variables_, "$name$_ = $default$;\n");
}

std::string ImmutableStringFieldLiteGenerator::GetBoxedType() const {
  return "java.lang.String";
}

// ===================================================================

ImmutableStringOneofFieldLiteGenerator::ImmutableStringOneofFieldLiteGenerator(
    const FieldDescriptor* descriptor, int messageBitIndex, Context* context)
    : ImmutableStringFieldLiteGenerator(descriptor, messageBitIndex, context) {
  const OneofGeneratorInfo* info =
      context->GetOneofGeneratorInfo(descriptor->containing_oneof());
  SetCommonOneofVariables(descriptor, info, &variables_);
}

ImmutableStringOneofFieldLiteGenerator::
    ~ImmutableStringOneofFieldLiteGenerator() {}

void ImmutableStringOneofFieldLiteGenerator::GenerateMembers(
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

  WriteFieldAccessorDocComment(printer, descriptor_, GETTER,
                               context_->options());
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public java.lang.String ${$get$capitalized_name$$}$() {\n"
      "  java.lang.String ref $default_init$;\n"
      "  if ($has_oneof_case_message$) {\n"
      "    ref = (java.lang.String) $oneof_name$_;\n"
      "  }\n"
      "  return ref;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);

  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, GETTER,
                                          context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public com.google.protobuf.ByteString\n"
                 "    ${$get$capitalized_name$Bytes$}$() {\n"
                 "  java.lang.String ref $default_init$;\n"
                 "  if ($has_oneof_case_message$) {\n"
                 "    ref = (java.lang.String) $oneof_name$_;\n"
                 "  }\n"
                 "  return com.google.protobuf.ByteString.copyFromUtf8(ref);\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  WriteFieldAccessorDocComment(printer, descriptor_, SETTER,
                               context_->options());
  printer->Print(variables_,
                 "@java.lang.SuppressWarnings(\"ReturnValueIgnored\")\n"
                 "private void ${$set$capitalized_name$$}$(\n"
                 "    java.lang.String value) {\n"
                 "  value.getClass();  // minimal bytecode null check\n"
                 "  $set_oneof_case_message$;\n"
                 "  $oneof_name$_ = value;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               context_->options());
  printer->Print(variables_,
                 "private void ${$clear$capitalized_name$$}$() {\n"
                 "  if ($has_oneof_case_message$) {\n"
                 "    $clear_oneof_case_message$;\n"
                 "    $oneof_name$_ = null;\n"
                 "  }\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);

  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, SETTER,
                                          context_->options());
  printer->Print(variables_,
                 "private void ${$set$capitalized_name$Bytes$}$(\n"
                 "    com.google.protobuf.ByteString value) {\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
  if (CheckUtf8(descriptor_)) {
    printer->Print(variables_, "  checkByteStringIsUtf8(value);\n");
  }
  printer->Print(variables_,
                 "  $oneof_name$_ = value.toStringUtf8();\n"
                 "  $set_oneof_case_message$;\n"
                 "}\n");
}

void ImmutableStringOneofFieldLiteGenerator::GenerateFieldInfo(
    io::Printer* printer, std::vector<uint16_t>* output) const {
  WriteIntToUtf16CharSequence(descriptor_->number(), output);
  WriteIntToUtf16CharSequence(GetExperimentalJavaFieldType(descriptor_),
                              output);
  WriteIntToUtf16CharSequence(descriptor_->containing_oneof()->index(), output);
}

void ImmutableStringOneofFieldLiteGenerator::GenerateBuilderMembers(
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

  WriteFieldAccessorDocComment(printer, descriptor_, GETTER,
                               context_->options());
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public java.lang.String ${$get$capitalized_name$$}$() {\n"
      "  return instance.get$capitalized_name$();\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);

  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, GETTER,
                                          context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public com.google.protobuf.ByteString\n"
                 "    ${$get$capitalized_name$Bytes$}$() {\n"
                 "  return instance.get$capitalized_name$Bytes();\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  WriteFieldAccessorDocComment(printer, descriptor_, SETTER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$set$capitalized_name$$}$(\n"
                 "    java.lang.String value) {\n"
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

  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, SETTER,
                                          context_->options(),
                                          /* builder */ true);
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$set$capitalized_name$Bytes$}$(\n"
      "    com.google.protobuf.ByteString value) {\n"
      "  copyOnWrite();\n"
      "  instance.set$capitalized_name$Bytes(value);\n"
      "  return this;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

// ===================================================================

RepeatedImmutableStringFieldLiteGenerator::
    RepeatedImmutableStringFieldLiteGenerator(const FieldDescriptor* descriptor,
                                              int messageBitIndex,
                                              Context* context)
    : descriptor_(descriptor),
      context_(context),
      name_resolver_(context->GetNameResolver()) {
  SetPrimitiveVariables(descriptor, messageBitIndex, 0,
                        context->GetFieldGeneratorInfo(descriptor),
                        name_resolver_, &variables_, context);
}

RepeatedImmutableStringFieldLiteGenerator::
    ~RepeatedImmutableStringFieldLiteGenerator() {}

int RepeatedImmutableStringFieldLiteGenerator::GetNumBitsForMessage() const {
  return 0;
}

void RepeatedImmutableStringFieldLiteGenerator::GenerateInterfaceMembers(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_GETTER,
                               context_->options());
  printer->Print(variables_,
                 "$deprecation$java.util.List<java.lang.String>\n"
                 "    ${$get$capitalized_name$List$}$();\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_COUNT,
                               context_->options());
  printer->Print(variables_,
                 "$deprecation$int ${$get$capitalized_name$Count$}$();\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_INDEXED_GETTER,
                               context_->options());
  printer->Print(variables_,
                 "$deprecation$java.lang.String "
                 "${$get$capitalized_name$$}$(int index);\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_INDEXED_GETTER,
                               context_->options());
  printer->Print(variables_,
                 "$deprecation$com.google.protobuf.ByteString\n"
                 "    ${$get$capitalized_name$Bytes$}$(int index);\n");
  printer->Annotate("{", "}", descriptor_);
}

void RepeatedImmutableStringFieldLiteGenerator::GenerateMembers(
    io::Printer* printer) const {
  printer->Print(
      variables_,
      "private com.google.protobuf.Internal.ProtobufList<java.lang.String> "
      "$name$_;\n");
  PrintExtraFieldInfo(variables_, printer);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_GETTER,
                               context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public java.util.List<java.lang.String> "
                 "${$get$capitalized_name$List$}$() {\n"
                 "  return $name$_;\n"  // note:  unmodifiable list
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
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public java.lang.String "
                 "${$get$capitalized_name$$}$(int index) {\n"
                 "  return $name$_.get(index);\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldStringBytesAccessorDocComment(
      printer, descriptor_, LIST_INDEXED_GETTER, context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public com.google.protobuf.ByteString\n"
                 "    ${$get$capitalized_name$Bytes$}$(int index) {\n"
                 "  return com.google.protobuf.ByteString.copyFromUtf8(\n"
                 "      $name$_.get(index));\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  printer->Print(
      variables_,
      "private void ensure$capitalized_name$IsMutable() {\n"
      // Use a temporary to avoid a redundant iget-object.
      "  com.google.protobuf.Internal.ProtobufList<java.lang.String> tmp =\n"
      "      $name$_;"
      "  if (!tmp.isModifiable()) {\n"
      "    $name$_ =\n"
      "        com.google.protobuf.GeneratedMessageLite.mutableCopy(tmp);\n"
      "   }\n"
      "}\n");

  WriteFieldAccessorDocComment(printer, descriptor_, LIST_INDEXED_SETTER,
                               context_->options());
  printer->Print(variables_,
                 "@java.lang.SuppressWarnings(\"ReturnValueIgnored\")\n"
                 "private void set$capitalized_name$(\n"
                 "    int index, java.lang.String value) {\n"
                 "  value.getClass();  // minimal bytecode null check\n"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  $name$_.set(index, value);\n"
                 "}\n");
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_ADDER,
                               context_->options());
  printer->Print(variables_,
                 "@java.lang.SuppressWarnings(\"ReturnValueIgnored\")\n"
                 "private void add$capitalized_name$(\n"
                 "    java.lang.String value) {\n"
                 "  value.getClass();  // minimal bytecode null check\n"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  $name$_.add(value);\n"
                 "}\n");
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_MULTI_ADDER,
                               context_->options());
  printer->Print(variables_,
                 "private void addAll$capitalized_name$(\n"
                 "    java.lang.Iterable<java.lang.String> values) {\n"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  com.google.protobuf.AbstractMessageLite.addAll(\n"
                 "      values, $name$_);\n"
                 "}\n");
  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               context_->options());
  printer->Print(variables_,
                 "private void clear$capitalized_name$() {\n"
                 "  $name$_ = $empty_list$;\n"
                 "}\n");

  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, LIST_ADDER,
                                          context_->options());
  printer->Print(variables_,
                 "private void add$capitalized_name$Bytes(\n"
                 "    com.google.protobuf.ByteString value) {\n");
  if (CheckUtf8(descriptor_)) {
    printer->Print(variables_, "  checkByteStringIsUtf8(value);\n");
  }
  printer->Print(variables_,
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  $name$_.add(value.toStringUtf8());\n"
                 "}\n");
}

void RepeatedImmutableStringFieldLiteGenerator::GenerateBuilderMembers(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_GETTER,
                               context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public java.util.List<java.lang.String>\n"
                 "    ${$get$capitalized_name$List$}$() {\n"
                 "  return java.util.Collections.unmodifiableList(\n"
                 "      instance.get$capitalized_name$List());\n"
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
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public java.lang.String "
                 "${$get$capitalized_name$$}$(int index) {\n"
                 "  return instance.get$capitalized_name$(index);\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldStringBytesAccessorDocComment(
      printer, descriptor_, LIST_INDEXED_GETTER, context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public com.google.protobuf.ByteString\n"
                 "    ${$get$capitalized_name$Bytes$}$(int index) {\n"
                 "  return instance.get$capitalized_name$Bytes(index);\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_INDEXED_SETTER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$set$capitalized_name$$}$(\n"
                 "    int index, java.lang.String value) {\n"
                 "  copyOnWrite();\n"
                 "  instance.set$capitalized_name$(index, value);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_ADDER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$add$capitalized_name$$}$(\n"
                 "    java.lang.String value) {\n"
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
                 "    java.lang.Iterable<java.lang.String> values) {\n"
                 "  copyOnWrite();\n"
                 "  instance.addAll$capitalized_name$(values);\n"
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

  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, LIST_ADDER,
                                          context_->options(),
                                          /* builder */ true);
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$add$capitalized_name$Bytes$}$(\n"
      "    com.google.protobuf.ByteString value) {\n"
      "  copyOnWrite();\n"
      "  instance.add$capitalized_name$Bytes(value);\n"
      "  return this;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void RepeatedImmutableStringFieldLiteGenerator::GenerateFieldInfo(
    io::Printer* printer, std::vector<uint16_t>* output) const {
  WriteIntToUtf16CharSequence(descriptor_->number(), output);
  WriteIntToUtf16CharSequence(GetExperimentalJavaFieldType(descriptor_),
                              output);
  printer->Print(variables_, "\"$name$_\",\n");
}

void RepeatedImmutableStringFieldLiteGenerator::GenerateInitializationCode(
    io::Printer* printer) const {
  printer->Print(variables_, "$name$_ = $empty_list$;\n");
}

std::string RepeatedImmutableStringFieldLiteGenerator::GetBoxedType() const {
  return "java.lang.String";
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
