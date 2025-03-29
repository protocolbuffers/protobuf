// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/java/lite/primitive_field.h"

#include <cstdint>
#include <string>

#include "absl/log/absl_check.h"
#include "absl/strings/ascii.h"
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
using Semantic = ::google::protobuf::io::AnnotationCollector::Semantic;

namespace {
bool EnableExperimentalRuntimeForLite() {
#ifdef PROTOBUF_EXPERIMENT
  return PROTOBUF_EXPERIMENT;
#else   // PROTOBUF_EXPERIMENT
  return false;
#endif  // !PROTOBUF_EXPERIMENT
}

void SetPrimitiveVariables(
    const FieldDescriptor* descriptor, int messageBitIndex, int builderBitIndex,
    const FieldGeneratorInfo* info, ClassNameResolver* name_resolver,
    absl::flat_hash_map<absl::string_view, std::string>* variables,
    Context* context) {
  SetCommonFieldVariables(descriptor, info, variables);
  JavaType javaType = GetJavaType(descriptor);
  (*variables)["type"] = std::string(PrimitiveTypeName(javaType));
  (*variables)["boxed_type"] = std::string(BoxedPrimitiveTypeName(javaType));
  variables->insert({"field_type", (*variables)["type"]});
  (*variables)["default"] =
      ImmutableDefaultValue(descriptor, name_resolver, context->options());
  (*variables)["capitalized_type"] = std::string(GetCapitalizedType(
      descriptor, /* immutable = */ true, context->options()));
  (*variables)["tag"] =
      absl::StrCat(static_cast<int32_t>(WireFormat::MakeTag(descriptor)));
  (*variables)["tag_size"] = absl::StrCat(
      WireFormat::TagSize(descriptor->number(), GetType(descriptor)));
  (*variables)["required"] = descriptor->is_required() ? "true" : "false";

  std::string capitalized_type = UnderscoresToCamelCase(
      PrimitiveTypeName(javaType), true /* cap_next_letter */);
  std::string name = (*variables)["name"];
  switch (javaType) {
    case JAVATYPE_INT:
    case JAVATYPE_LONG:
    case JAVATYPE_FLOAT:
    case JAVATYPE_DOUBLE:
    case JAVATYPE_BOOLEAN:
      (*variables)["field_list_type"] = absl::StrCat(
          "com.google.protobuf.Internal.", capitalized_type, "List");
      (*variables)["empty_list"] =
          absl::StrCat("empty", capitalized_type, "List()");
      (*variables)["make_name_unmodifiable"] =
          absl::StrCat(name, "_.makeImmutable()");
      (*variables)["repeated_get"] =
          absl::StrCat(name, "_.get", capitalized_type);
      (*variables)["repeated_add"] =
          absl::StrCat(name, "_.add", capitalized_type);
      (*variables)["repeated_set"] =
          absl::StrCat(name, "_.set", capitalized_type);
      (*variables)["visit_type"] = capitalized_type;
      (*variables)["visit_type_list"] =
          absl::StrCat("visit", capitalized_type, "List");
      break;
    default:
      variables->insert(
          {"field_list_type",
           absl::StrCat("com.google.protobuf.Internal.ProtobufList<",
                        (*variables)["boxed_type"], ">")});
      (*variables)["empty_list"] = "emptyProtobufList()";
      (*variables)["make_name_unmodifiable"] =
          absl::StrCat(name, "_.makeImmutable()");
      (*variables)["repeated_get"] = absl::StrCat(name, "_.get");
      (*variables)["repeated_add"] = absl::StrCat(name, "_.add");
      (*variables)["repeated_set"] = absl::StrCat(name, "_.set");
      (*variables)["visit_type"] = "ByteString";
      (*variables)["visit_type_list"] = "visitList";
  }

  if (javaType == JAVATYPE_BYTES) {
    (*variables)["bytes_default"] =
        absl::StrCat(absl::AsciiStrToUpper(name), "_DEFAULT_VALUE");
  }

  if (IsReferenceType(javaType)) {
    // We use `x.getClass()` as a null check because it generates less bytecode
    // than an `if (x == null) { throw ... }` statement.
    (*variables)["null_check"] =
        "  java.lang.Class<?> valueClass = value.getClass();\n";
  } else {
    (*variables)["null_check"] = "";
  }
  // TODO: Add @deprecated javadoc when generating javadoc is supported
  // by the proto compiler
  (*variables)["deprecation"] =
      descriptor->options().deprecated() ? "@java.lang.Deprecated " : "";
  int fixed_size = FixedSize(GetType(descriptor));
  if (fixed_size != -1) {
    (*variables)["fixed_size"] = absl::StrCat(fixed_size);
  }

  if (HasHasbit(descriptor)) {
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

    switch (descriptor->type()) {
      case FieldDescriptor::TYPE_BYTES:
        (*variables)["is_field_present_message"] =
            absl::StrCat("!", name, "_.isEmpty()");
        break;
      case FieldDescriptor::TYPE_FLOAT:
        (*variables)["is_field_present_message"] =
            absl::StrCat("java.lang.Float.floatToRawIntBits(", name, "_) != 0");
        break;
      case FieldDescriptor::TYPE_DOUBLE:
        (*variables)["is_field_present_message"] = absl::StrCat(
            "java.lang.Double.doubleToRawLongBits(", name, "_) != 0");
        break;
      default:
        variables->insert(
            {"is_field_present_message",
             absl::StrCat(name, "_ != ", (*variables)["default"])});
        break;
    }
  }

  // Annotations often use { and } variables to denote ranges.
  (*variables)["{"] = "";
  (*variables)["}"] = "";
}

}  // namespace

// ===================================================================

ImmutablePrimitiveFieldLiteGenerator::ImmutablePrimitiveFieldLiteGenerator(
    const FieldDescriptor* descriptor, int messageBitIndex, Context* context)
    : descriptor_(descriptor),
      messageBitIndex_(messageBitIndex),
      context_(context),
      name_resolver_(context->GetNameResolver()) {
  SetPrimitiveVariables(descriptor, messageBitIndex, 0,
                        context->GetFieldGeneratorInfo(descriptor),
                        name_resolver_, &variables_, context);
}

ImmutablePrimitiveFieldLiteGenerator::~ImmutablePrimitiveFieldLiteGenerator() {}

int ImmutablePrimitiveFieldLiteGenerator::GetNumBitsForMessage() const {
  return HasHasbit(descriptor_) ? 1 : 0;
}

void ImmutablePrimitiveFieldLiteGenerator::GenerateInterfaceMembers(
    io::Printer* printer) const {
  if (descriptor_->has_presence()) {
    WriteFieldAccessorDocComment(printer, descriptor_, HAZZER,
                                 context_->options());
    printer->Print(variables_,
                   "$deprecation$boolean has$capitalized_name$();\n");
  }
  WriteFieldAccessorDocComment(printer, descriptor_, GETTER,
                               context_->options());
  printer->Print(variables_,
                 "$deprecation$$type$ ${$get$capitalized_name$$}$();\n");
  printer->Annotate("{", "}", descriptor_);
}

void ImmutablePrimitiveFieldLiteGenerator::GenerateMembers(
    io::Printer* printer) const {
  if (IsByteStringWithCustomDefaultValue(descriptor_)) {
    // allocate this once statically since we know ByteStrings are immutable
    // values that can be reused.
    printer->Print(
        variables_,
        "private static final $field_type$ $bytes_default$ = $default$;\n");
  }
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
  printer->Print(variables_, "private $field_type$ $name$_;\n");
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
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public $type$ ${$get$capitalized_name$$}$() {\n"
                 "  return $name$_;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  WriteFieldAccessorDocComment(printer, descriptor_, SETTER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ false, /* is_private */ true);
  printer->Print(variables_,
                 "private void set$capitalized_name$($type$ value) {\n"
                 "$null_check$"
                 "  $set_has_field_bit_message$\n"
                 "  $name$_ = value;\n"
                 "}\n");

  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ false, /* is_private */ true);
  printer->Print(variables_,
                 "private void clear$capitalized_name$() {\n"
                 "  $clear_has_field_bit_message$\n");
  JavaType type = GetJavaType(descriptor_);
  if (type == JAVATYPE_STRING || type == JAVATYPE_BYTES) {
    // The default value is not a simple literal so we want to avoid executing
    // it multiple times.  Instead, get the default out of the default instance.
    printer->Print(
        variables_,
        "  $name$_ = getDefaultInstance().get$capitalized_name$();\n");
  } else {
    printer->Print(variables_, "  $name$_ = $default$;\n");
  }
  printer->Print(variables_, "}\n");
}

void ImmutablePrimitiveFieldLiteGenerator::GenerateBuilderMembers(
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

void ImmutablePrimitiveFieldLiteGenerator::GenerateFieldInfo(
    io::Printer* printer, std::vector<uint16_t>* output) const {
  WriteIntToUtf16CharSequence(descriptor_->number(), output);
  WriteIntToUtf16CharSequence(GetExperimentalJavaFieldType(descriptor_),
                              output);
  if (HasHasbit(descriptor_)) {
    WriteIntToUtf16CharSequence(messageBitIndex_, output);
  }
  printer->Print(variables_, "\"$name$_\",\n");
}

void ImmutablePrimitiveFieldLiteGenerator::GenerateInitializationCode(
    io::Printer* printer) const {
  if (IsByteStringWithCustomDefaultValue(descriptor_)) {
    printer->Print(variables_, "$name$_ = $bytes_default$;\n");
  } else if (!IsDefaultValueJavaDefault(descriptor_)) {
    printer->Print(variables_, "$name$_ = $default$;\n");
  }
}

std::string ImmutablePrimitiveFieldLiteGenerator::GetBoxedType() const {
  return std::string(BoxedPrimitiveTypeName(GetJavaType(descriptor_)));
}

// ===================================================================

ImmutablePrimitiveOneofFieldLiteGenerator::
    ImmutablePrimitiveOneofFieldLiteGenerator(const FieldDescriptor* descriptor,
                                              int messageBitIndex,
                                              Context* context)
    : ImmutablePrimitiveFieldLiteGenerator(descriptor, messageBitIndex,
                                           context) {
  const OneofGeneratorInfo* info =
      context->GetOneofGeneratorInfo(descriptor->containing_oneof());
  SetCommonOneofVariables(descriptor, info, &variables_);
}

ImmutablePrimitiveOneofFieldLiteGenerator::
    ~ImmutablePrimitiveOneofFieldLiteGenerator() {}

void ImmutablePrimitiveOneofFieldLiteGenerator::GenerateMembers(
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
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public $type$ ${$get$capitalized_name$$}$() {\n"
                 "  if ($has_oneof_case_message$) {\n"
                 "    return ($boxed_type$) $oneof_name$_;\n"
                 "  }\n"
                 "  return $default$;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  WriteFieldAccessorDocComment(printer, descriptor_, SETTER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ false, /* is_private */ true);
  printer->Print(variables_,
                 "private void set$capitalized_name$($type$ value) {\n"
                 "$null_check$"
                 "  $set_oneof_case_message$;\n"
                 "  $oneof_name$_ = value;\n"
                 "}\n");

  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ false, /* is_private */ true);
  printer->Print(variables_,
                 "private void clear$capitalized_name$() {\n"
                 "  if ($has_oneof_case_message$) {\n"
                 "    $clear_oneof_case_message$;\n"
                 "    $oneof_name$_ = null;\n"
                 "  }\n"
                 "}\n");
}

void ImmutablePrimitiveOneofFieldLiteGenerator::GenerateFieldInfo(
    io::Printer* printer, std::vector<uint16_t>* output) const {
  WriteIntToUtf16CharSequence(descriptor_->number(), output);
  WriteIntToUtf16CharSequence(GetExperimentalJavaFieldType(descriptor_),
                              output);
  WriteIntToUtf16CharSequence(descriptor_->containing_oneof()->index(), output);
}

void ImmutablePrimitiveOneofFieldLiteGenerator::GenerateBuilderMembers(
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

RepeatedImmutablePrimitiveFieldLiteGenerator::
    RepeatedImmutablePrimitiveFieldLiteGenerator(
        const FieldDescriptor* descriptor, int messageBitIndex,
        Context* context)
    : descriptor_(descriptor),
      context_(context),
      name_resolver_(context->GetNameResolver()) {
  SetPrimitiveVariables(descriptor, messageBitIndex, 0,
                        context->GetFieldGeneratorInfo(descriptor),
                        name_resolver_, &variables_, context);
}

RepeatedImmutablePrimitiveFieldLiteGenerator::
    ~RepeatedImmutablePrimitiveFieldLiteGenerator() {}

int RepeatedImmutablePrimitiveFieldLiteGenerator::GetNumBitsForMessage() const {
  return 0;
}

void RepeatedImmutablePrimitiveFieldLiteGenerator::GenerateInterfaceMembers(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_GETTER,
                               context_->options());
  printer->Print(variables_,
                 "$deprecation$java.util.List<$boxed_type$> "
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
}

void RepeatedImmutablePrimitiveFieldLiteGenerator::GenerateMembers(
    io::Printer* printer) const {
  printer->Print(variables_, "private $field_list_type$ $name$_;\n");
  PrintExtraFieldInfo(variables_, printer);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_GETTER,
                               context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public java.util.List<$boxed_type$>\n"
                 "    ${$get$capitalized_name$List$}$() {\n"
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
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public $type$ ${$get$capitalized_name$$}$(int index) {\n"
      "  return $repeated_get$(index);\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);

  if (!EnableExperimentalRuntimeForLite() && descriptor_->is_packed() &&
      context_->HasGeneratedMethods(descriptor_->containing_type())) {
    printer->Print(variables_,
                   "private int $name$MemoizedSerializedSize = -1;\n");
  }

  printer->Print(
      variables_,
      "private void ensure$capitalized_name$IsMutable() {\n"
      // Use a temporary to avoid a redundant iget-object.
      "  $field_list_type$ tmp = $name$_;\n"
      "  if (!tmp.isModifiable()) {\n"
      "    $name$_ =\n"
      "        com.google.protobuf.GeneratedMessageLite.mutableCopy(tmp);\n"
      "   }\n"
      "}\n");

  WriteFieldAccessorDocComment(printer, descriptor_, LIST_INDEXED_SETTER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ false, /* is_private */ true);
  printer->Print(variables_,
                 "private void set$capitalized_name$(\n"
                 "    int index, $type$ value) {\n"
                 "$null_check$"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  $repeated_set$(index, value);\n"
                 "}\n");
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_ADDER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ false, /* is_private */ true);
  printer->Print(variables_,
                 "private void add$capitalized_name$($type$ value) {\n"
                 "$null_check$"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  $repeated_add$(value);\n"
                 "}\n");
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_MULTI_ADDER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ false, /* is_private */ true);
  printer->Print(variables_,
                 "private void addAll$capitalized_name$(\n"
                 "    java.lang.Iterable<? extends $boxed_type$> values) {\n"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  com.google.protobuf.AbstractMessageLite.addAll(\n"
                 "      values, $name$_);\n"
                 "}\n");
  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ false, /* is_private */ true);
  printer->Print(variables_,
                 "private void clear$capitalized_name$() {\n"
                 "  $name$_ = $empty_list$;\n"
                 "}\n");
}

void RepeatedImmutablePrimitiveFieldLiteGenerator::GenerateBuilderMembers(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_GETTER,
                               context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public java.util.List<$boxed_type$>\n"
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
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public $type$ ${$get$capitalized_name$$}$(int index) {\n"
      "  return instance.get$capitalized_name$(index);\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, SETTER,
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
                 "    java.lang.Iterable<? extends $boxed_type$> values) {\n"
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
}

void RepeatedImmutablePrimitiveFieldLiteGenerator::GenerateFieldInfo(
    io::Printer* printer, std::vector<uint16_t>* output) const {
  WriteIntToUtf16CharSequence(descriptor_->number(), output);
  WriteIntToUtf16CharSequence(GetExperimentalJavaFieldType(descriptor_),
                              output);
  printer->Print(variables_, "\"$name$_\",\n");
}

void RepeatedImmutablePrimitiveFieldLiteGenerator::GenerateInitializationCode(
    io::Printer* printer) const {
  printer->Print(variables_, "$name$_ = $empty_list$;\n");
}

std::string RepeatedImmutablePrimitiveFieldLiteGenerator::GetBoxedType() const {
  return std::string(BoxedPrimitiveTypeName(GetJavaType(descriptor_)));
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
