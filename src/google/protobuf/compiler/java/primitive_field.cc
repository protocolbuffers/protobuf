// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/java/primitive_field.h"

#include <cstdint>
#include <string>

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/str_cat.h"
#include "google/protobuf/compiler/java/context.h"
#include "google/protobuf/compiler/java/doc_comment.h"
#include "google/protobuf/compiler/java/helpers.h"
#include "google/protobuf/compiler/java/name_resolver.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/wire_format.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

using internal::WireFormat;
using Semantic = ::google::protobuf::io::AnnotationCollector::Semantic;

namespace {

void SetPrimitiveVariables(
    const FieldDescriptor* descriptor, int messageBitIndex, int builderBitIndex,
    const FieldGeneratorInfo* info, ClassNameResolver* name_resolver,
    absl::flat_hash_map<absl::string_view, std::string>* variables,
    Context* context) {
  SetCommonFieldVariables(descriptor, info, variables);
  JavaType javaType = GetJavaType(descriptor);

  (*variables)["type"] = std::string(PrimitiveTypeName(javaType));
  (*variables)["boxed_type"] = std::string(BoxedPrimitiveTypeName(javaType));
  (*variables)["kt_type"] = std::string(KotlinTypeName(javaType));
  variables->insert({"field_type", (*variables)["type"]});

  std::string name = (*variables)["name"];
  (*variables)["name_make_immutable"] = absl::StrCat(name, "_.makeImmutable()");
  if (javaType == JAVATYPE_BOOLEAN || javaType == JAVATYPE_DOUBLE ||
      javaType == JAVATYPE_FLOAT || javaType == JAVATYPE_INT ||
      javaType == JAVATYPE_LONG) {
    std::string capitalized_type = UnderscoresToCamelCase(
        PrimitiveTypeName(javaType), /*cap_first_letter=*/true);
    (*variables)["field_list_type"] =
        absl::StrCat("com.google.protobuf.Internal.", capitalized_type, "List");
    (*variables)["empty_list"] =
        absl::StrCat("empty", capitalized_type, "List()");
    (*variables)["repeated_get"] =
        absl::StrCat(name, "_.get", capitalized_type);
    (*variables)["repeated_add"] =
        absl::StrCat(name, "_.add", capitalized_type);
    (*variables)["repeated_set"] =
        absl::StrCat(name, "_.set", capitalized_type);
  } else {
    (*variables)["field_list_type"] =
        "com.google.protobuf.Internal.ProtobufList<com.google.protobuf."
        "ByteString>";
    (*variables)["empty_list"] =
        "emptyList(com.google.protobuf.ByteString.class)";
    (*variables)["repeated_get"] = absl::StrCat(name, "_.get");
    (*variables)["repeated_add"] = absl::StrCat(name, "_.add");
    (*variables)["repeated_set"] = absl::StrCat(name, "_.set");
  }

  (*variables)["default"] =
      ImmutableDefaultValue(descriptor, name_resolver, context->options());
  (*variables)["default_init"] =
      IsDefaultValueJavaDefault(descriptor)
          ? ""
          : absl::StrCat("= ", ImmutableDefaultValue(descriptor, name_resolver,
                                                     context->options()));
  (*variables)["capitalized_type"] = std::string(GetCapitalizedType(
      descriptor, /* immutable = */ true, context->options()));
  (*variables)["tag"] =
      absl::StrCat(static_cast<int32_t>(WireFormat::MakeTag(descriptor)));
  (*variables)["tag_size"] = absl::StrCat(
      WireFormat::TagSize(descriptor->number(), GetType(descriptor)));
  if (IsReferenceType(GetJavaType(descriptor))) {
    (*variables)["null_check"] =
        "if (value == null) { throw new NullPointerException(); }";
  } else {
    (*variables)["null_check"] = "";
  }
  // TODO: Add @deprecated javadoc when generating javadoc is supported
  // by the proto compiler
  (*variables)["deprecation"] =
      descriptor->options().deprecated() ? "@java.lang.Deprecated " : "";
  (*variables)["kt_deprecation"] =
      descriptor->options().deprecated()
          ? absl::StrCat("@kotlin.Deprecated(message = \"Field ", name,
                         " is deprecated\") ")
          : "";
  int fixed_size = FixedSize(GetType(descriptor));
  if (fixed_size != -1) {
    (*variables)["fixed_size"] = absl::StrCat(fixed_size);
  }
  (*variables)["on_changed"] = "onChanged();";

  if (HasHasbit(descriptor)) {
    // For singular messages and builders, one bit is used for the hasField bit.
    (*variables)["get_has_field_bit_message"] = GenerateGetBit(messageBitIndex);
    // Note that these have a trailing ";".
    (*variables)["set_has_field_bit_to_local"] =
        absl::StrCat(GenerateSetBitToLocal(messageBitIndex), ";");
    (*variables)["is_field_present_message"] = GenerateGetBit(messageBitIndex);
  } else {
    (*variables)["set_has_field_bit_to_local"] = "";
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

  // Always track the presence of a field explicitly in the builder, regardless
  // of syntax.
  (*variables)["get_has_field_bit_builder"] = GenerateGetBit(builderBitIndex);
  (*variables)["get_has_field_bit_from_local"] =
      GenerateGetBitFromLocal(builderBitIndex);
  (*variables)["set_has_field_bit_builder"] =
      absl::StrCat(GenerateSetBit(builderBitIndex), ";");
  (*variables)["clear_has_field_bit_builder"] =
      absl::StrCat(GenerateClearBit(builderBitIndex), ";");
}

}  // namespace

// ===================================================================

ImmutablePrimitiveFieldGenerator::ImmutablePrimitiveFieldGenerator(
    const FieldDescriptor* descriptor, int messageBitIndex, int builderBitIndex,
    Context* context)
    : descriptor_(descriptor),
      message_bit_index_(messageBitIndex),
      builder_bit_index_(builderBitIndex),
      context_(context),
      name_resolver_(context->GetNameResolver()) {
  SetPrimitiveVariables(descriptor, messageBitIndex, builderBitIndex,
                        context->GetFieldGeneratorInfo(descriptor),
                        name_resolver_, &variables_, context);
}

ImmutablePrimitiveFieldGenerator::~ImmutablePrimitiveFieldGenerator() {}

int ImmutablePrimitiveFieldGenerator::GetMessageBitIndex() const {
  return message_bit_index_;
}

int ImmutablePrimitiveFieldGenerator::GetBuilderBitIndex() const {
  return builder_bit_index_;
}

int ImmutablePrimitiveFieldGenerator::GetNumBitsForMessage() const {
  return HasHasbit(descriptor_) ? 1 : 0;
}

int ImmutablePrimitiveFieldGenerator::GetNumBitsForBuilder() const { return 1; }

void ImmutablePrimitiveFieldGenerator::GenerateInterfaceMembers(
    io::Printer* printer) const {
  if (descriptor_->has_presence()) {
    WriteFieldAccessorDocComment(printer, descriptor_, HAZZER,
                                 context_->options());
    printer->Print(variables_,
                   "$deprecation$boolean has$capitalized_name$();\n");
  }
  WriteFieldAccessorDocComment(printer, descriptor_, GETTER,
                               context_->options());
  printer->Print(variables_, "$deprecation$$type$ get$capitalized_name$();\n");
}

void ImmutablePrimitiveFieldGenerator::GenerateMembers(
    io::Printer* printer) const {
  printer->Print(variables_, "private $field_type$ $name$_ = $default$;\n");
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

  WriteFieldAccessorDocComment(printer, descriptor_, GETTER,
                               context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public $type$ ${$get$capitalized_name$$}$() {\n"
                 "  return $name$_;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void ImmutablePrimitiveFieldGenerator::GenerateBuilderMembers(
    io::Printer* printer) const {
  printer->Print(variables_, "private $field_type$ $name$_ $default_init$;\n");

  if (descriptor_->has_presence()) {
    WriteFieldAccessorDocComment(printer, descriptor_, HAZZER,
                                 context_->options());
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
        "  return $get_has_field_bit_builder$;\n"
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
                               context_->options(),
                               /* builder */ true);
  printer->Print(variables_,
                 "$deprecation$public Builder "
                 "${$set$capitalized_name$$}$($type$ value) {\n"
                 "  $null_check$\n"
                 "  $name$_ = value;\n"
                 "  $set_has_field_bit_builder$\n"
                 "  $on_changed$\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);

  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$clear$capitalized_name$$}$() {\n"
      "  $clear_has_field_bit_builder$\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
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
  printer->Print(variables_,
                 "  $on_changed$\n"
                 "  return this;\n"
                 "}\n");
}

void ImmutablePrimitiveFieldGenerator::GenerateKotlinDslMembers(
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

void ImmutablePrimitiveFieldGenerator::GenerateFieldBuilderInitializationCode(
    io::Printer* printer) const {
  // noop for primitives
}

void ImmutablePrimitiveFieldGenerator::GenerateInitializationCode(
    io::Printer* printer) const {
  if (!IsDefaultValueJavaDefault(descriptor_)) {
    printer->Print(variables_, "$name$_ = $default$;\n");
  }
}

void ImmutablePrimitiveFieldGenerator::GenerateBuilderClearCode(
    io::Printer* printer) const {
  // No need to clear the has-bit since we clear the bitField ints all at once.
  printer->Print(variables_, "$name$_ = $default$;\n");
}

void ImmutablePrimitiveFieldGenerator::GenerateMergingCode(
    io::Printer* printer) const {
  if (descriptor_->has_presence()) {
    printer->Print(variables_,
                   "if (other.has$capitalized_name$()) {\n"
                   "  set$capitalized_name$(other.get$capitalized_name$());\n"
                   "}\n");
  } else {
    printer->Print(variables_,
                   "if (other.get$capitalized_name$() != $default$) {\n"
                   "  set$capitalized_name$(other.get$capitalized_name$());\n"
                   "}\n");
  }
}

void ImmutablePrimitiveFieldGenerator::GenerateBuildingCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "if ($get_has_field_bit_from_local$) {\n"
                 "  result.$name$_ = $name$_;\n");
  if (GetNumBitsForMessage() > 0) {
    printer->Print(variables_, "  $set_has_field_bit_to_local$\n");
  }
  printer->Print("}\n");
}

void ImmutablePrimitiveFieldGenerator::GenerateBuilderParsingCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "$name$_ = input.read$capitalized_type$();\n"
                 "$set_has_field_bit_builder$\n");
}

void ImmutablePrimitiveFieldGenerator::GenerateSerializationCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "if ($is_field_present_message$) {\n"
                 "  output.write$capitalized_type$($number$, $name$_);\n"
                 "}\n");
}

void ImmutablePrimitiveFieldGenerator::GenerateSerializedSizeCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "if ($is_field_present_message$) {\n"
                 "  size += com.google.protobuf.CodedOutputStream\n"
                 "    .compute$capitalized_type$Size($number$, $name$_);\n"
                 "}\n");
}

void ImmutablePrimitiveFieldGenerator::GenerateEqualsCode(
    io::Printer* printer) const {
  switch (GetJavaType(descriptor_)) {
    case JAVATYPE_INT:
    case JAVATYPE_LONG:
    case JAVATYPE_BOOLEAN:
      printer->Print(variables_,
                     "if (get$capitalized_name$()\n"
                     "    != other.get$capitalized_name$()) return false;\n");
      break;

    case JAVATYPE_FLOAT:
      printer->Print(
          variables_,
          "if (java.lang.Float.floatToIntBits(get$capitalized_name$())\n"
          "    != java.lang.Float.floatToIntBits(\n"
          "        other.get$capitalized_name$())) return false;\n");
      break;

    case JAVATYPE_DOUBLE:
      printer->Print(
          variables_,
          "if (java.lang.Double.doubleToLongBits(get$capitalized_name$())\n"
          "    != java.lang.Double.doubleToLongBits(\n"
          "        other.get$capitalized_name$())) return false;\n");
      break;

    case JAVATYPE_STRING:
    case JAVATYPE_BYTES:
      printer->Print(
          variables_,
          "if (!get$capitalized_name$()\n"
          "    .equals(other.get$capitalized_name$())) return false;\n");
      break;

    case JAVATYPE_ENUM:
    case JAVATYPE_MESSAGE:
    default:
      ABSL_LOG(FATAL) << "Can't get here.";
      break;
  }
}

void ImmutablePrimitiveFieldGenerator::GenerateHashCode(
    io::Printer* printer) const {
  printer->Print(variables_, "hash = (37 * hash) + $constant_name$;\n");
  switch (GetJavaType(descriptor_)) {
    case JAVATYPE_INT:
      printer->Print(variables_,
                     "hash = (53 * hash) + get$capitalized_name$();\n");
      break;

    case JAVATYPE_LONG:
      printer->Print(
          variables_,
          "hash = (53 * hash) + com.google.protobuf.Internal.hashLong(\n"
          "    get$capitalized_name$());\n");
      break;

    case JAVATYPE_BOOLEAN:
      printer->Print(
          variables_,
          "hash = (53 * hash) + com.google.protobuf.Internal.hashBoolean(\n"
          "    get$capitalized_name$());\n");
      break;

    case JAVATYPE_FLOAT:
      printer->Print(variables_,
                     "hash = (53 * hash) + java.lang.Float.floatToIntBits(\n"
                     "    get$capitalized_name$());\n");
      break;

    case JAVATYPE_DOUBLE:
      printer->Print(
          variables_,
          "hash = (53 * hash) + com.google.protobuf.Internal.hashLong(\n"
          "    java.lang.Double.doubleToLongBits(get$capitalized_name$()));\n");
      break;

    case JAVATYPE_STRING:
    case JAVATYPE_BYTES:
      printer->Print(
          variables_,
          "hash = (53 * hash) + get$capitalized_name$().hashCode();\n");
      break;

    case JAVATYPE_ENUM:
    case JAVATYPE_MESSAGE:
    default:
      ABSL_LOG(FATAL) << "Can't get here.";
      break;
  }
}

std::string ImmutablePrimitiveFieldGenerator::GetBoxedType() const {
  return std::string(BoxedPrimitiveTypeName(GetJavaType(descriptor_)));
}

// ===================================================================

ImmutablePrimitiveOneofFieldGenerator::ImmutablePrimitiveOneofFieldGenerator(
    const FieldDescriptor* descriptor, int messageBitIndex, int builderBitIndex,
    Context* context)
    : ImmutablePrimitiveFieldGenerator(descriptor, messageBitIndex,
                                       builderBitIndex, context) {
  const OneofGeneratorInfo* info =
      context->GetOneofGeneratorInfo(descriptor->containing_oneof());
  SetCommonOneofVariables(descriptor, info, &variables_);
}

ImmutablePrimitiveOneofFieldGenerator::
    ~ImmutablePrimitiveOneofFieldGenerator() {}

void ImmutablePrimitiveOneofFieldGenerator::GenerateMembers(
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
}

void ImmutablePrimitiveOneofFieldGenerator::GenerateBuilderMembers(
    io::Printer* printer) const {
  ABSL_DCHECK(descriptor_->has_presence());
  WriteFieldAccessorDocComment(printer, descriptor_, HAZZER,
                               context_->options());
  printer->Print(variables_,
                 "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
                 "  return $has_oneof_case_message$;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  WriteFieldAccessorDocComment(printer, descriptor_, GETTER,
                               context_->options());
  printer->Print(variables_,
                 "$deprecation$public $type$ ${$get$capitalized_name$$}$() {\n"
                 "  if ($has_oneof_case_message$) {\n"
                 "    return ($boxed_type$) $oneof_name$_;\n"
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
                 "  $null_check$\n"
                 "  $set_oneof_case_message$;\n"
                 "  $oneof_name$_ = value;\n"
                 "  $on_changed$\n"
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
      "    $on_changed$\n"
      "  }\n"
      "  return this;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void ImmutablePrimitiveOneofFieldGenerator::GenerateBuilderClearCode(
    io::Printer* printer) const {
  // No-Op: When a primitive field is in a oneof, clearing the oneof clears that
  // field.
}

void ImmutablePrimitiveOneofFieldGenerator::GenerateBuildingCode(
    io::Printer* printer) const {
  // no-op
}

void ImmutablePrimitiveOneofFieldGenerator::GenerateMergingCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "set$capitalized_name$(other.get$capitalized_name$());\n");
}

void ImmutablePrimitiveOneofFieldGenerator::GenerateBuilderParsingCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "$oneof_name$_ = input.read$capitalized_type$();\n"
                 "$set_oneof_case_message$;\n");
}

void ImmutablePrimitiveOneofFieldGenerator::GenerateSerializationCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "if ($has_oneof_case_message$) {\n"
                 "  output.write$capitalized_type$(\n");
  // $type$ and $boxed_type$ is the same for bytes fields so we don't need to
  // do redundant casts.
  if (GetJavaType(descriptor_) == JAVATYPE_BYTES) {
    printer->Print(variables_, "      $number$, ($type$) $oneof_name$_);\n");
  } else {
    printer->Print(
        variables_,
        "      $number$, ($type$)(($boxed_type$) $oneof_name$_));\n");
  }
  printer->Print("}\n");
}

void ImmutablePrimitiveOneofFieldGenerator::GenerateSerializedSizeCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "if ($has_oneof_case_message$) {\n"
                 "  size += com.google.protobuf.CodedOutputStream\n"
                 "    .compute$capitalized_type$Size(\n");
  // $type$ and $boxed_type$ is the same for bytes fields so we don't need to
  // do redundant casts.
  if (GetJavaType(descriptor_) == JAVATYPE_BYTES) {
    printer->Print(variables_, "        $number$, ($type$) $oneof_name$_);\n");
  } else {
    printer->Print(
        variables_,
        "        $number$, ($type$)(($boxed_type$) $oneof_name$_));\n");
  }
  printer->Print("}\n");
}

// ===================================================================

RepeatedImmutablePrimitiveFieldGenerator::
    RepeatedImmutablePrimitiveFieldGenerator(const FieldDescriptor* descriptor,
                                             int messageBitIndex,
                                             int builderBitIndex,
                                             Context* context)
    : ImmutablePrimitiveFieldGenerator(descriptor, messageBitIndex,
                                       builderBitIndex, context) {}

RepeatedImmutablePrimitiveFieldGenerator::
    ~RepeatedImmutablePrimitiveFieldGenerator() {}

int RepeatedImmutablePrimitiveFieldGenerator::GetNumBitsForMessage() const {
  return 0;
}

int RepeatedImmutablePrimitiveFieldGenerator::GetNumBitsForBuilder() const {
  return 1;
}

void RepeatedImmutablePrimitiveFieldGenerator::GenerateInterfaceMembers(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_GETTER,
                               context_->options());
  printer->Print(variables_,
                 "$deprecation$java.util.List<$boxed_type$> "
                 "get$capitalized_name$List();\n");
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_COUNT,
                               context_->options());
  printer->Print(variables_,
                 "$deprecation$int get$capitalized_name$Count();\n");
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_INDEXED_GETTER,
                               context_->options());
  printer->Print(variables_,
                 "$deprecation$$type$ get$capitalized_name$(int index);\n");
}

void RepeatedImmutablePrimitiveFieldGenerator::GenerateMembers(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "@SuppressWarnings(\"serial\")\n"
                 "private $field_list_type$ $name$_ =\n"
                 "    $empty_list$;\n");
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
      "$deprecation$public int ${$get$capitalized_name$Count$}$() {\n"
      "  return $name$_.size();\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_INDEXED_GETTER,
                               context_->options());
  printer->Print(
      variables_,
      "$deprecation$public $type$ ${$get$capitalized_name$$}$(int index) {\n"
      "  return $repeated_get$(index);\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);

  if (descriptor_->is_packed()) {
    printer->Print(variables_,
                   "private int $name$MemoizedSerializedSize = -1;\n");
  }
}

void RepeatedImmutablePrimitiveFieldGenerator::GenerateBuilderMembers(
    io::Printer* printer) const {
  // We use a ProtobufArrayList because it starts as a mutable list that can be
  // switched to immutable when references are handed out. This allows copy-free
  // sharing. A bit in the bitfield tracks whether there are any items in the
  // list. The presence bit allows us to skip work on blocks of 32 fields by
  // by checking if the entire bit-field int == 0 (none of the fields are
  // present).
  printer->Print(variables_,
                 "private $field_list_type$ $name$_ = $empty_list$;\n"
                 "private void ensure$capitalized_name$IsMutable() {\n"
                 "  if (!$name$_.isModifiable()) {\n"
                 "    $name$_ = makeMutableCopy($name$_);\n"
                 "  }\n"
                 "  $set_has_field_bit_builder$\n"
                 "}\n");
  if (FixedSize(GetType(descriptor_)) != -1) {
    printer->Print(
        variables_,
        "private void ensure$capitalized_name$IsMutable(int capacity) {\n"
        "  if (!$name$_.isModifiable()) {\n"
        "    $name$_ = makeMutableCopy($name$_, capacity);\n"
        "  }\n"
        "  $set_has_field_bit_builder$\n"
        "}\n");
  }

  // Note:  We return an unmodifiable list because otherwise the caller
  //   could hold on to the returned list and modify it after the message
  //   has been built, thus mutating the message which is supposed to be
  //   immutable.
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_GETTER,
                               context_->options());
  printer->Print(variables_,
                 "$deprecation$public java.util.List<$boxed_type$>\n"
                 "    ${$get$capitalized_name$List$}$() {\n"
                 "  $name$_.makeImmutable();\n"
                 "  return $name$_;\n"
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
      "  return $repeated_get$(index);\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_INDEXED_SETTER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$set$capitalized_name$$}$(\n"
                 "    int index, $type$ value) {\n"
                 "  $null_check$\n"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  $repeated_set$(index, value);\n"
                 "  $set_has_field_bit_builder$\n"
                 "  $on_changed$\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_ADDER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(variables_,
                 "$deprecation$public Builder "
                 "${$add$capitalized_name$$}$($type$ value) {\n"
                 "  $null_check$\n"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  $repeated_add$(value);\n"
                 "  $set_has_field_bit_builder$\n"
                 "  $on_changed$\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_MULTI_ADDER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$addAll$capitalized_name$$}$(\n"
                 "    java.lang.Iterable<? extends $boxed_type$> values) {\n"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  com.google.protobuf.AbstractMessageLite.Builder.addAll(\n"
                 "      values, $name$_);\n"
                 "  $set_has_field_bit_builder$\n"
                 "  $on_changed$\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$clear$capitalized_name$$}$() {\n"
      "  $name$_ = $empty_list$;\n"
      "  $clear_has_field_bit_builder$\n"
      "  $on_changed$\n"
      "  return this;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void RepeatedImmutablePrimitiveFieldGenerator::GenerateKotlinDslMembers(
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

void RepeatedImmutablePrimitiveFieldGenerator::
    GenerateFieldBuilderInitializationCode(io::Printer* printer) const {
  // noop for primitives
}

void RepeatedImmutablePrimitiveFieldGenerator::GenerateInitializationCode(
    io::Printer* printer) const {
  printer->Print(variables_, "$name$_ = $empty_list$;\n");
}

void RepeatedImmutablePrimitiveFieldGenerator::GenerateBuilderClearCode(
    io::Printer* printer) const {
  printer->Print(variables_, "$name$_ = $empty_list$;\n");
}

void RepeatedImmutablePrimitiveFieldGenerator::GenerateMergingCode(
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
                 "    $name_make_immutable$;\n"
                 "    $set_has_field_bit_builder$\n");
  printer->Print(variables_,
                 "  } else {\n"
                 "    ensure$capitalized_name$IsMutable();\n"
                 "    $name$_.addAll(other.$name$_);\n"
                 "  }\n"
                 "  $on_changed$\n"
                 "}\n");
}

void RepeatedImmutablePrimitiveFieldGenerator::GenerateBuildingCode(
    io::Printer* printer) const {
  // The code below ensures that the result has an immutable list. If our
  // list is immutable, we can just reuse it. If not, we make it immutable.
  printer->Print(variables_,
                 "if ($get_has_field_bit_from_local$) {\n"
                 "  $name_make_immutable$;\n"
                 "  result.$name$_ = $name$_;\n"
                 "}\n");
}

void RepeatedImmutablePrimitiveFieldGenerator::GenerateBuilderParsingCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "$type$ v = input.read$capitalized_type$();\n"
                 "ensure$capitalized_name$IsMutable();\n"
                 "$repeated_add$(v);\n");
}

void RepeatedImmutablePrimitiveFieldGenerator::
    GenerateBuilderParsingCodeFromPacked(io::Printer* printer) const {
  if (FixedSize(GetType(descriptor_)) != -1) {
    // 4K limit on pre-allocations to prevent OOM from malformed input.
    printer->Print(variables_,
                   "int length = input.readRawVarint32();\n"
                   "int limit = input.pushLimit(length);\n"
                   "int alloc = length > 4096 ? 4096 : length;\n"
                   "ensure$capitalized_name$IsMutable(alloc / $fixed_size$);\n"
                   "while (input.getBytesUntilLimit() > 0) {\n"
                   "  $repeated_add$(input.read$capitalized_type$());\n"
                   "}\n"
                   "input.popLimit(limit);\n");
  } else {
    printer->Print(variables_,
                   "int length = input.readRawVarint32();\n"
                   "int limit = input.pushLimit(length);\n"
                   "ensure$capitalized_name$IsMutable();\n"
                   "while (input.getBytesUntilLimit() > 0) {\n"
                   "  $repeated_add$(input.read$capitalized_type$());\n"
                   "}\n"
                   "input.popLimit(limit);\n");
  }
}

void RepeatedImmutablePrimitiveFieldGenerator::GenerateSerializationCode(
    io::Printer* printer) const {
  if (descriptor_->is_packed()) {
    // We invoke getSerializedSize in writeTo for messages that have packed
    // fields in ImmutableMessageGenerator::GenerateMessageSerializationMethods.
    // That makes it safe to rely on the memoized size here.
    printer->Print(variables_,
                   "if (get$capitalized_name$List().size() > 0) {\n"
                   "  output.writeUInt32NoTag($tag$);\n"
                   "  output.writeUInt32NoTag($name$MemoizedSerializedSize);\n"
                   "}\n"
                   "for (int i = 0; i < $name$_.size(); i++) {\n"
                   "  output.write$capitalized_type$NoTag($repeated_get$(i));\n"
                   "}\n");
  } else {
    printer->Print(
        variables_,
        "for (int i = 0; i < $name$_.size(); i++) {\n"
        "  output.write$capitalized_type$($number$, $repeated_get$(i));\n"
        "}\n");
  }
}

void RepeatedImmutablePrimitiveFieldGenerator::GenerateSerializedSizeCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "{\n"
                 "  int dataSize = 0;\n");
  printer->Indent();

  if (FixedSize(GetType(descriptor_)) == -1) {
    printer->Print(
        variables_,
        "for (int i = 0; i < $name$_.size(); i++) {\n"
        "  dataSize += com.google.protobuf.CodedOutputStream\n"
        "    .compute$capitalized_type$SizeNoTag($repeated_get$(i));\n"
        "}\n");
  } else {
    printer->Print(
        variables_,
        "dataSize = $fixed_size$ * get$capitalized_name$List().size();\n");
  }

  printer->Print("size += dataSize;\n");

  if (descriptor_->is_packed()) {
    printer->Print(variables_,
                   "if (!get$capitalized_name$List().isEmpty()) {\n"
                   "  size += $tag_size$;\n"
                   "  size += com.google.protobuf.CodedOutputStream\n"
                   "      .computeInt32SizeNoTag(dataSize);\n"
                   "}\n");
  } else {
    printer->Print(
        variables_,
        "size += $tag_size$ * get$capitalized_name$List().size();\n");
  }

  // cache the data size for packed fields.
  if (descriptor_->is_packed()) {
    printer->Print(variables_, "$name$MemoizedSerializedSize = dataSize;\n");
  }

  printer->Outdent();
  printer->Print("}\n");
}

void RepeatedImmutablePrimitiveFieldGenerator::GenerateEqualsCode(
    io::Printer* printer) const {
  printer->Print(
      variables_,
      "if (!get$capitalized_name$List()\n"
      "    .equals(other.get$capitalized_name$List())) return false;\n");
}

void RepeatedImmutablePrimitiveFieldGenerator::GenerateHashCode(
    io::Printer* printer) const {
  printer->Print(
      variables_,
      "if (get$capitalized_name$Count() > 0) {\n"
      "  hash = (37 * hash) + $constant_name$;\n"
      "  hash = (53 * hash) + get$capitalized_name$List().hashCode();\n"
      "}\n");
}

std::string RepeatedImmutablePrimitiveFieldGenerator::GetBoxedType() const {
  return std::string(BoxedPrimitiveTypeName(GetJavaType(descriptor_)));
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
