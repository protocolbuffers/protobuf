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
// Author: jonp@google.com (Jon Perlow)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <google/protobuf/compiler/java/java_string_field_lite.h>

#include <cstdint>
#include <map>
#include <string>

#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/compiler/java/java_context.h>
#include <google/protobuf/compiler/java/java_doc_comment.h>
#include <google/protobuf/compiler/java/java_helpers.h>
#include <google/protobuf/compiler/java/java_name_resolver.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

using internal::WireFormat;
using internal::WireFormatLite;

namespace {

void SetPrimitiveVariables(const FieldDescriptor* descriptor,
                           int messageBitIndex, int builderBitIndex,
                           const FieldGeneratorInfo* info,
                           ClassNameResolver* name_resolver,
                           std::map<std::string, std::string>* variables) {
  SetCommonFieldVariables(descriptor, info, variables);

  (*variables)["empty_list"] =
      "com.google.protobuf.GeneratedMessageLite.emptyProtobufList()";

  (*variables)["default"] = ImmutableDefaultValue(descriptor, name_resolver);
  (*variables)["default_init"] =
      "= " + ImmutableDefaultValue(descriptor, name_resolver);
  (*variables)["capitalized_type"] = "java.lang.String";
  (*variables)["tag"] =
      StrCat(static_cast<int32_t>(WireFormat::MakeTag(descriptor)));
  (*variables)["tag_size"] = StrCat(
      WireFormat::TagSize(descriptor->number(), GetType(descriptor)));
  // We use `x.getClass()` as a null check because it generates less bytecode
  // than an `if (x == null) { throw ... }` statement.
  (*variables)["null_check"] =
      "  java.lang.Class<?> valueClass = value.getClass();\n";

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
        "!" + (*variables)["name"] + "_.isEmpty()";
  }

  (*variables)["get_has_field_bit_from_local"] =
      GenerateGetBitFromLocal(builderBitIndex);
  (*variables)["set_has_field_bit_to_local"] =
      GenerateSetBitToLocal(messageBitIndex);
}

}  // namespace

// ===================================================================

ImmutableStringFieldLiteGenerator::ImmutableStringFieldLiteGenerator(
    const FieldDescriptor* descriptor, int messageBitIndex, Context* context)
    : descriptor_(descriptor),
      messageBitIndex_(messageBitIndex),
      name_resolver_(context->GetNameResolver()) {
  SetPrimitiveVariables(descriptor, messageBitIndex, 0,
                        context->GetFieldGeneratorInfo(descriptor),
                        name_resolver_, &variables_);
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

// TODO(dweis): Consider dropping all of the *Bytes() methods. They really
//     shouldn't be necessary or used on devices.
void ImmutableStringFieldLiteGenerator::GenerateInterfaceMembers(
    io::Printer* printer) const {
  if (HasHazzer(descriptor_)) {
    WriteFieldAccessorDocComment(printer, descriptor_, HAZZER);
    printer->Print(variables_,
                   "$deprecation$boolean has$capitalized_name$();\n");
  }
  WriteFieldAccessorDocComment(printer, descriptor_, GETTER);
  printer->Print(variables_,
                 "$deprecation$java.lang.String get$capitalized_name$();\n");
  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, GETTER);
  printer->Print(variables_,
                 "$deprecation$com.google.protobuf.ByteString\n"
                 "    get$capitalized_name$Bytes();\n");
}

void ImmutableStringFieldLiteGenerator::GenerateMembers(
    io::Printer* printer) const {
  printer->Print(variables_, "private java.lang.String $name$_;\n");
  PrintExtraFieldInfo(variables_, printer);

  if (HasHazzer(descriptor_)) {
    WriteFieldAccessorDocComment(printer, descriptor_, HAZZER);
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
        "  return $get_has_field_bit_message$;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
  }

  WriteFieldAccessorDocComment(printer, descriptor_, GETTER);
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public java.lang.String ${$get$capitalized_name$$}$() {\n"
      "  return $name$_;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, GETTER);
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public com.google.protobuf.ByteString\n"
      "    ${$get$capitalized_name$Bytes$}$() {\n"
      "  return com.google.protobuf.ByteString.copyFromUtf8($name$_);\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);

  WriteFieldAccessorDocComment(printer, descriptor_, SETTER);
  printer->Print(variables_,
                 "private void set$capitalized_name$(\n"
                 "    java.lang.String value) {\n"
                 "$null_check$"
                 "  $set_has_field_bit_message$\n"
                 "  $name$_ = value;\n"
                 "}\n");
  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER);
  printer->Print(variables_,
                 "private void clear$capitalized_name$() {\n"
                 "  $clear_has_field_bit_message$\n"
                 // The default value is not a simple literal so we want to
                 // avoid executing it multiple times.  Instead, get the default
                 // out of the default instance.
                 "  $name$_ = getDefaultInstance().get$capitalized_name$();\n"
                 "}\n");

  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, SETTER);
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
  if (HasHazzer(descriptor_)) {
    WriteFieldAccessorDocComment(printer, descriptor_, HAZZER);
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
        "  return instance.has$capitalized_name$();\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
  }

  WriteFieldAccessorDocComment(printer, descriptor_, GETTER);
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public java.lang.String ${$get$capitalized_name$$}$() {\n"
      "  return instance.get$capitalized_name$();\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);

  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, GETTER);
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public com.google.protobuf.ByteString\n"
                 "    ${$get$capitalized_name$Bytes$}$() {\n"
                 "  return instance.get$capitalized_name$Bytes();\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  WriteFieldAccessorDocComment(printer, descriptor_, SETTER,
                               /* builder */ true);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$set$capitalized_name$$}$(\n"
                 "    java.lang.String value) {\n"
                 "  copyOnWrite();\n"
                 "  instance.set$capitalized_name$(value);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               /* builder */ true);
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$clear$capitalized_name$$}$() {\n"
      "  copyOnWrite();\n"
      "  instance.clear$capitalized_name$();\n"
      "  return this;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);

  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, SETTER,
                                          /* builder */ true);
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$set$capitalized_name$Bytes$}$(\n"
      "    com.google.protobuf.ByteString value) {\n"
      "  copyOnWrite();\n"
      "  instance.set$capitalized_name$Bytes(value);\n"
      "  return this;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void ImmutableStringFieldLiteGenerator::GenerateKotlinDslMembers(
    io::Printer* printer) const {
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "$kt_deprecation$public var $kt_name$: kotlin.String\n"
                 "  @JvmName(\"${$get$kt_capitalized_name$$}$\")\n"
                 "  get() = $kt_dsl_builder$.${$get$capitalized_name$$}$()\n"
                 "  @JvmName(\"${$set$kt_capitalized_name$$}$\")\n"
                 "  set(value) {\n"
                 "    $kt_dsl_builder$.${$set$capitalized_name$$}$(value)\n"
                 "  }\n");

  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               /* builder */ false);
  printer->Print(variables_,
                 "public fun ${$clear$kt_capitalized_name$$}$() {\n"
                 "  $kt_dsl_builder$.${$clear$capitalized_name$$}$()\n"
                 "}\n");

  if (HasHazzer(descriptor_)) {
    WriteFieldAccessorDocComment(printer, descriptor_, HAZZER);
    printer->Print(
        variables_,
        "public fun ${$has$kt_capitalized_name$$}$(): kotlin.Boolean {\n"
        "  return $kt_dsl_builder$.${$has$capitalized_name$$}$()\n"
        "}\n");
  }
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
  GOOGLE_DCHECK(HasHazzer(descriptor_));
  WriteFieldAccessorDocComment(printer, descriptor_, HAZZER);
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
                 "  return $has_oneof_case_message$;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  WriteFieldAccessorDocComment(printer, descriptor_, GETTER);
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

  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, GETTER);
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

  WriteFieldAccessorDocComment(printer, descriptor_, SETTER);
  printer->Print(variables_,
                 "private void ${$set$capitalized_name$$}$(\n"
                 "    java.lang.String value) {\n"
                 "$null_check$"
                 "  $set_oneof_case_message$;\n"
                 "  $oneof_name$_ = value;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER);
  printer->Print(variables_,
                 "private void ${$clear$capitalized_name$$}$() {\n"
                 "  if ($has_oneof_case_message$) {\n"
                 "    $clear_oneof_case_message$;\n"
                 "    $oneof_name$_ = null;\n"
                 "  }\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, SETTER);
  printer->Print(variables_,
                 "private void ${$set$capitalized_name$Bytes$}$(\n"
                 "    com.google.protobuf.ByteString value) {\n");
  printer->Annotate("{", "}", descriptor_);
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
  GOOGLE_DCHECK(HasHazzer(descriptor_));
  WriteFieldAccessorDocComment(printer, descriptor_, HAZZER);
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
                 "  return instance.has$capitalized_name$();\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  WriteFieldAccessorDocComment(printer, descriptor_, GETTER);
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public java.lang.String ${$get$capitalized_name$$}$() {\n"
      "  return instance.get$capitalized_name$();\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);

  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, GETTER);
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public com.google.protobuf.ByteString\n"
                 "    ${$get$capitalized_name$Bytes$}$() {\n"
                 "  return instance.get$capitalized_name$Bytes();\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  WriteFieldAccessorDocComment(printer, descriptor_, SETTER,
                               /* builder */ true);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$set$capitalized_name$$}$(\n"
                 "    java.lang.String value) {\n"
                 "  copyOnWrite();\n"
                 "  instance.set$capitalized_name$(value);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               /* builder */ true);
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$clear$capitalized_name$$}$() {\n"
      "  copyOnWrite();\n"
      "  instance.clear$capitalized_name$();\n"
      "  return this;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);

  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, SETTER,
                                          /* builder */ true);
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$set$capitalized_name$Bytes$}$(\n"
      "    com.google.protobuf.ByteString value) {\n"
      "  copyOnWrite();\n"
      "  instance.set$capitalized_name$Bytes(value);\n"
      "  return this;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
}

// ===================================================================

RepeatedImmutableStringFieldLiteGenerator::
    RepeatedImmutableStringFieldLiteGenerator(const FieldDescriptor* descriptor,
                                              int messageBitIndex,
                                              Context* context)
    : descriptor_(descriptor), name_resolver_(context->GetNameResolver()) {
  SetPrimitiveVariables(descriptor, messageBitIndex, 0,
                        context->GetFieldGeneratorInfo(descriptor),
                        name_resolver_, &variables_);
}

RepeatedImmutableStringFieldLiteGenerator::
    ~RepeatedImmutableStringFieldLiteGenerator() {}

int RepeatedImmutableStringFieldLiteGenerator::GetNumBitsForMessage() const {
  return 0;
}

void RepeatedImmutableStringFieldLiteGenerator::GenerateInterfaceMembers(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_GETTER);
  printer->Print(variables_,
                 "$deprecation$java.util.List<java.lang.String>\n"
                 "    get$capitalized_name$List();\n");
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_COUNT);
  printer->Print(variables_,
                 "$deprecation$int get$capitalized_name$Count();\n");
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_INDEXED_GETTER);
  printer->Print(
      variables_,
      "$deprecation$java.lang.String get$capitalized_name$(int index);\n");
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_INDEXED_GETTER);
  printer->Print(variables_,
                 "$deprecation$com.google.protobuf.ByteString\n"
                 "    get$capitalized_name$Bytes(int index);\n");
}

void RepeatedImmutableStringFieldLiteGenerator::GenerateMembers(
    io::Printer* printer) const {
  printer->Print(
      variables_,
      "private com.google.protobuf.Internal.ProtobufList<java.lang.String> "
      "$name$_;\n");
  PrintExtraFieldInfo(variables_, printer);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_GETTER);
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public java.util.List<java.lang.String> "
                 "${$get$capitalized_name$List$}$() {\n"
                 "  return $name$_;\n"  // note:  unmodifiable list
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_COUNT);
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public int ${$get$capitalized_name$Count$}$() {\n"
      "  return $name$_.size();\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_INDEXED_GETTER);
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public java.lang.String "
                 "${$get$capitalized_name$$}$(int index) {\n"
                 "  return $name$_.get(index);\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldStringBytesAccessorDocComment(printer, descriptor_,
                                          LIST_INDEXED_GETTER);
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

  WriteFieldAccessorDocComment(printer, descriptor_, LIST_INDEXED_SETTER);
  printer->Print(variables_,
                 "private void set$capitalized_name$(\n"
                 "    int index, java.lang.String value) {\n"
                 "$null_check$"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  $name$_.set(index, value);\n"
                 "}\n");
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_ADDER);
  printer->Print(variables_,
                 "private void add$capitalized_name$(\n"
                 "    java.lang.String value) {\n"
                 "$null_check$"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  $name$_.add(value);\n"
                 "}\n");
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_MULTI_ADDER);
  printer->Print(variables_,
                 "private void addAll$capitalized_name$(\n"
                 "    java.lang.Iterable<java.lang.String> values) {\n"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  com.google.protobuf.AbstractMessageLite.addAll(\n"
                 "      values, $name$_);\n"
                 "}\n");
  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER);
  printer->Print(variables_,
                 "private void clear$capitalized_name$() {\n"
                 "  $name$_ = $empty_list$;\n"
                 "}\n");

  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, LIST_ADDER);
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
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_GETTER);
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public java.util.List<java.lang.String>\n"
                 "    ${$get$capitalized_name$List$}$() {\n"
                 "  return java.util.Collections.unmodifiableList(\n"
                 "      instance.get$capitalized_name$List());\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_COUNT);
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public int ${$get$capitalized_name$Count$}$() {\n"
      "  return instance.get$capitalized_name$Count();\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_INDEXED_GETTER);
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public java.lang.String "
                 "${$get$capitalized_name$$}$(int index) {\n"
                 "  return instance.get$capitalized_name$(index);\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldStringBytesAccessorDocComment(printer, descriptor_,
                                          LIST_INDEXED_GETTER);
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public com.google.protobuf.ByteString\n"
                 "    ${$get$capitalized_name$Bytes$}$(int index) {\n"
                 "  return instance.get$capitalized_name$Bytes(index);\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_INDEXED_SETTER,
                               /* builder */ true);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$set$capitalized_name$$}$(\n"
                 "    int index, java.lang.String value) {\n"
                 "  copyOnWrite();\n"
                 "  instance.set$capitalized_name$(index, value);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_ADDER,
                               /* builder */ true);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$add$capitalized_name$$}$(\n"
                 "    java.lang.String value) {\n"
                 "  copyOnWrite();\n"
                 "  instance.add$capitalized_name$(value);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_MULTI_ADDER,
                               /* builder */ true);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$addAll$capitalized_name$$}$(\n"
                 "    java.lang.Iterable<java.lang.String> values) {\n"
                 "  copyOnWrite();\n"
                 "  instance.addAll$capitalized_name$(values);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               /* builder */ true);
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$clear$capitalized_name$$}$() {\n"
      "  copyOnWrite();\n"
      "  instance.clear$capitalized_name$();\n"
      "  return this;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);

  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, LIST_ADDER,
                                          /* builder */ true);
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$add$capitalized_name$Bytes$}$(\n"
      "    com.google.protobuf.ByteString value) {\n"
      "  copyOnWrite();\n"
      "  instance.add$capitalized_name$Bytes(value);\n"
      "  return this;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void RepeatedImmutableStringFieldLiteGenerator::GenerateKotlinDslMembers(
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

  // property for List<String>
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_GETTER);
  printer->Print(
      variables_,
      "public val $kt_name$: "
      "com.google.protobuf.kotlin.DslList"
      "<kotlin.String, ${$$kt_capitalized_name$Proxy$}$>\n"
      "  @kotlin.OptIn"
      "(com.google.protobuf.kotlin.OnlyForUseByGeneratedProtoCode::class)\n"
      "  get() = com.google.protobuf.kotlin.DslList(\n"
      "    $kt_dsl_builder$.${$get$capitalized_name$List$}$()\n"
      "  )\n");

  // List<String>.add(String)
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_ADDER,
                               /* builder */ false);
  printer->Print(variables_,
                 "@kotlin.jvm.JvmSynthetic\n"
                 "@kotlin.jvm.JvmName(\"add$kt_capitalized_name$\")\n"
                 "public fun com.google.protobuf.kotlin.DslList"
                 "<kotlin.String, ${$$kt_capitalized_name$Proxy$}$>."
                 "add(value: kotlin.String) {\n"
                 "  $kt_dsl_builder$.${$add$capitalized_name$$}$(value)\n"
                 "}\n");

  // List<String> += String
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_ADDER,
                               /* builder */ false);
  printer->Print(variables_,
                 "@kotlin.jvm.JvmSynthetic\n"
                 "@kotlin.jvm.JvmName(\"plusAssign$kt_capitalized_name$\")\n"
                 "@Suppress(\"NOTHING_TO_INLINE\")\n"
                 "public inline operator fun com.google.protobuf.kotlin.DslList"
                 "<kotlin.String, ${$$kt_capitalized_name$Proxy$}$>."
                 "plusAssign(value: kotlin.String) {\n"
                 "  add(value)\n"
                 "}\n");

  // List<String>.addAll(Iterable<String>)
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_MULTI_ADDER,
                               /* builder */ false);
  printer->Print(
      variables_,
      "@kotlin.jvm.JvmSynthetic\n"
      "@kotlin.jvm.JvmName(\"addAll$kt_capitalized_name$\")\n"
      "public fun com.google.protobuf.kotlin.DslList"
      "<kotlin.String, ${$$kt_capitalized_name$Proxy$}$>."
      "addAll(values: kotlin.collections.Iterable<kotlin.String>) {\n"
      "  $kt_dsl_builder$.${$addAll$capitalized_name$$}$(values)\n"
      "}\n");

  // List<String> += Iterable<String>
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_MULTI_ADDER,
                               /* builder */ false);
  printer->Print(
      variables_,
      "@kotlin.jvm.JvmSynthetic\n"
      "@kotlin.jvm.JvmName(\"plusAssignAll$kt_capitalized_name$\")\n"
      "@Suppress(\"NOTHING_TO_INLINE\")\n"
      "public inline operator fun com.google.protobuf.kotlin.DslList"
      "<kotlin.String, ${$$kt_capitalized_name$Proxy$}$>."
      "plusAssign(values: kotlin.collections.Iterable<kotlin.String>) {\n"
      "  addAll(values)\n"
      "}\n");

  // List<String>[Int] = String
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_INDEXED_SETTER,
                               /* builder */ false);
  printer->Print(
      variables_,
      "@kotlin.jvm.JvmSynthetic\n"
      "@kotlin.jvm.JvmName(\"set$kt_capitalized_name$\")\n"
      "public operator fun com.google.protobuf.kotlin.DslList"
      "<kotlin.String, ${$$kt_capitalized_name$Proxy$}$>."
      "set(index: kotlin.Int, value: kotlin.String) {\n"
      "  $kt_dsl_builder$.${$set$capitalized_name$$}$(index, value)\n"
      "}");

  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               /* builder */ false);
  printer->Print(variables_,
                 "@kotlin.jvm.JvmSynthetic\n"
                 "@kotlin.jvm.JvmName(\"clear$kt_capitalized_name$\")\n"
                 "public fun com.google.protobuf.kotlin.DslList"
                 "<kotlin.String, ${$$kt_capitalized_name$Proxy$}$>."
                 "clear() {\n"
                 "  $kt_dsl_builder$.${$clear$capitalized_name$$}$()\n"
                 "}");
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
