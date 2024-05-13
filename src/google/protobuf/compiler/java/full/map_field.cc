// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/java/full/map_field.h"

#include <string>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "google/protobuf/compiler/java/context.h"
#include "google/protobuf/compiler/java/doc_comment.h"
#include "google/protobuf/compiler/java/field_common.h"
#include "google/protobuf/compiler/java/helpers.h"
#include "google/protobuf/compiler/java/internal_helpers.h"
#include "google/protobuf/compiler/java/name_resolver.h"
#include "google/protobuf/io/printer.h"

// Must be last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

namespace {
using Semantic = ::google::protobuf::io::AnnotationCollector::Semantic;

std::string TypeName(const FieldDescriptor* field,
                     ClassNameResolver* name_resolver, bool boxed) {
  if (GetJavaType(field) == JAVATYPE_MESSAGE) {
    return name_resolver->GetImmutableClassName(field->message_type());
  } else if (GetJavaType(field) == JAVATYPE_ENUM) {
    return name_resolver->GetImmutableClassName(field->enum_type());
  } else {
    return std::string(boxed ? BoxedPrimitiveTypeName(GetJavaType(field))
                             : PrimitiveTypeName(GetJavaType(field)));
  }
}

std::string KotlinTypeName(const FieldDescriptor* field,
                           ClassNameResolver* name_resolver) {
  if (GetJavaType(field) == JAVATYPE_MESSAGE) {
    return name_resolver->GetImmutableClassName(field->message_type());
  } else if (GetJavaType(field) == JAVATYPE_ENUM) {
    return name_resolver->GetImmutableClassName(field->enum_type());
  } else {
    return std::string(KotlinTypeName(GetJavaType(field)));
  }
}

std::string WireType(const FieldDescriptor* field) {
  return absl::StrCat("com.google.protobuf.WireFormat.FieldType.",
                      FieldTypeName(field->type()));
}

}  // namespace

ImmutableMapFieldGenerator::ImmutableMapFieldGenerator(
    const FieldDescriptor* descriptor, int messageBitIndex, int builderBitIndex,
    Context* context)
    : descriptor_(descriptor),
      message_bit_index_(messageBitIndex),
      builder_bit_index_(builderBitIndex),
      name_resolver_(context->GetNameResolver()),
      context_(context) {
  SetMessageVariables(context->GetFieldGeneratorInfo(descriptor));
}

void ImmutableMapFieldGenerator::SetMessageVariables(
    const FieldGeneratorInfo* info) {
  SetCommonFieldVariables(descriptor_, info, &variables_);
  ClassNameResolver* name_resolver = context_->GetNameResolver();

  variables_["type"] =
      name_resolver->GetImmutableClassName(descriptor_->message_type());
  const FieldDescriptor* key = MapKeyField(descriptor_);
  const FieldDescriptor* value = MapValueField(descriptor_);
  const JavaType keyJavaType = GetJavaType(key);
  const JavaType valueJavaType = GetJavaType(value);

  // The code that generates the open-source version appears not to understand
  // #else, so we have an #ifndef instead.
  std::string pass_through_nullness =
      context_->options().opensource_runtime
          ? "/* nullable */\n"
          : "@com.google.protobuf.Internal.ProtoPassThroughNullness ";

  variables_["key_type"] = TypeName(key, name_resolver, false);
  std::string boxed_key_type = TypeName(key, name_resolver, true);
  variables_["boxed_key_type"] = boxed_key_type;
  variables_["kt_key_type"] = KotlinTypeName(key, name_resolver);
  variables_["kt_value_type"] = KotlinTypeName(value, name_resolver);
  // Used for calling the serialization function.
  variables_["short_key_type"] =
      boxed_key_type.substr(boxed_key_type.rfind('.') + 1);
  variables_["key_wire_type"] = WireType(key);
  variables_["key_default_value"] =
      DefaultValue(key, true, name_resolver, context_->options());
  variables_["key_null_check"] =
      IsReferenceType(keyJavaType)
          ? "if (key == null) { throw new NullPointerException(\"map key\"); }"
          : "";
  variables_["value_null_check"] =
      valueJavaType != JAVATYPE_ENUM && IsReferenceType(valueJavaType)
          ? "if (value == null) { "
            "throw new NullPointerException(\"map value\"); }"
          : "";
  if (valueJavaType == JAVATYPE_ENUM) {
    // We store enums as Integers internally.
    variables_["value_type"] = "int";
    variables_.insert(
        {"value_type_pass_through_nullness", variables_["value_type"]});
    variables_["boxed_value_type"] = "java.lang.Integer";
    variables_["value_wire_type"] = WireType(value);
    variables_["value_default_value"] =
        DefaultValue(value, true, name_resolver, context_->options()) +
        ".getNumber()";

    variables_["value_enum_type"] = TypeName(value, name_resolver, false);

    variables_.insert(
        {"value_enum_type_pass_through_nullness",
         absl::StrCat(pass_through_nullness, variables_["value_enum_type"])});

    if (SupportUnknownEnumValue(value)) {
      // Map unknown values to a special UNRECOGNIZED value if supported.
      variables_.insert(
          {"unrecognized_value",
           absl::StrCat(variables_["value_enum_type"], ".UNRECOGNIZED")});
    } else {
      // Map unknown values to the default value if we don't have UNRECOGNIZED.
      variables_["unrecognized_value"] =
          DefaultValue(value, true, name_resolver, context_->options());
    }
  } else {
    variables_["value_type"] = TypeName(value, name_resolver, false);

    variables_.insert(
        {"value_type_pass_through_nullness",
         absl::StrCat(
             (IsReferenceType(valueJavaType) ? pass_through_nullness : ""),
             variables_["value_type"])});

    variables_["boxed_value_type"] = TypeName(value, name_resolver, true);
    variables_["value_wire_type"] = WireType(value);
    variables_["value_default_value"] =
        DefaultValue(value, true, name_resolver, context_->options());
  }

  variables_.insert(
      {"type_parameters", absl::StrCat(variables_["boxed_key_type"], ", ",
                                       variables_["boxed_value_type"])});

  if (valueJavaType == JAVATYPE_MESSAGE) {
    variables_["value_interface_type"] =
        absl::StrCat(variables_["boxed_value_type"], "OrBuilder");
    variables_["value_builder_type"] =
        absl::StrCat(variables_["boxed_value_type"], ".Builder");
    variables_["builder_type_parameters"] = absl::StrJoin(
        {variables_["boxed_key_type"], variables_["value_interface_type"],
         variables_["boxed_value_type"], variables_["value_builder_type"]},
        ", ");
  }
  // TODO: Add @deprecated javadoc when generating javadoc is supported
  // by the proto compiler
  variables_["deprecation"] =
      descriptor_->options().deprecated() ? "@java.lang.Deprecated " : "";
  variables_.insert(
      {"kt_deprecation",
       descriptor_->options().deprecated()
           ? absl::StrCat("@kotlin.Deprecated(message = \"Field ",
                          variables_["name"], " is deprecated\") ")
           : ""});
  variables_["on_changed"] = "onChanged();";

  variables_.insert(
      {"default_entry", absl::StrCat(variables_["capitalized_name"],
                                     "DefaultEntryHolder.defaultEntry")});
  variables_.insert({"map_field_parameter", variables_["default_entry"]});
  variables_["descriptor"] = absl::StrCat(
      name_resolver->GetImmutableClassName(descriptor_->file()), ".internal_",
      UniqueFileScopeIdentifier(descriptor_->message_type()), "_descriptor, ");
  variables_["get_has_field_bit_builder"] = GenerateGetBit(builder_bit_index_);
  variables_["get_has_field_bit_from_local"] =
      GenerateGetBitFromLocal(builder_bit_index_);
  variables_["set_has_field_bit_builder"] =
      absl::StrCat(GenerateSetBit(builder_bit_index_), ";");
  variables_["clear_has_field_bit_builder"] =
      absl::StrCat(GenerateClearBit(builder_bit_index_), ";");
}

int ImmutableMapFieldGenerator::GetMessageBitIndex() const {
  return message_bit_index_;
}

int ImmutableMapFieldGenerator::GetBuilderBitIndex() const {
  return builder_bit_index_;
}

int ImmutableMapFieldGenerator::GetNumBitsForMessage() const { return 0; }

int ImmutableMapFieldGenerator::GetNumBitsForBuilder() const { return 1; }

void ImmutableMapFieldGenerator::GenerateInterfaceMembers(
    io::Printer* printer) const {
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$int ${$get$capitalized_name$Count$}$();\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$boolean ${$contains$capitalized_name$$}$(\n"
                 "    $key_type$ key);\n");
  printer->Annotate("{", "}", descriptor_);

  const FieldDescriptor* value = MapValueField(descriptor_);
  if (GetJavaType(value) == JAVATYPE_ENUM) {
    if (context_->options().opensource_runtime) {
      printer->Print(variables_,
                     "/**\n"
                     " * Use {@link #get$capitalized_name$Map()} instead.\n"
                     " */\n"
                     "@java.lang.Deprecated\n"
                     "java.util.Map<$boxed_key_type$, $value_enum_type$>\n"
                     "${$get$capitalized_name$$}$();\n");
      printer->Annotate("{", "}", descriptor_);
    }
    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(
        variables_,
        "$deprecation$java.util.Map<$boxed_key_type$, $value_enum_type$>\n"
        "${$get$capitalized_name$Map$}$();\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(variables_,
                   "$deprecation$$value_enum_type_pass_through_nullness$ "
                   "${$get$capitalized_name$OrDefault$}$(\n"
                   "    $key_type$ key,\n"
                   "    $value_enum_type_pass_through_nullness$ "
                   "        defaultValue);\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(
        variables_,
        "$deprecation$$value_enum_type$ ${$get$capitalized_name$OrThrow$}$(\n"
        "    $key_type$ key);\n");
    printer->Annotate("{", "}", descriptor_);
    if (SupportUnknownEnumValue(value)) {
      printer->Print(
          variables_,
          "/**\n"
          " * Use {@link #get$capitalized_name$ValueMap()} instead.\n"
          " */\n"
          "@java.lang.Deprecated\n"
          "java.util.Map<$type_parameters$>\n"
          "${$get$capitalized_name$Value$}$();\n");
      printer->Annotate("{", "}", descriptor_);
      WriteFieldDocComment(printer, descriptor_, context_->options());
      printer->Print(variables_,
                     "$deprecation$java.util.Map<$type_parameters$>\n"
                     "${$get$capitalized_name$ValueMap$}$();\n");
      printer->Annotate("{", "}", descriptor_);
      WriteFieldDocComment(printer, descriptor_, context_->options());
      printer->Print(variables_,
                     "$deprecation$$value_type_pass_through_nullness$ "
                     "${$get$capitalized_name$ValueOrDefault$}$(\n"
                     "    $key_type$ key,\n"
                     "    $value_type_pass_through_nullness$ defaultValue);\n");
      printer->Annotate("{", "}", descriptor_);
      WriteFieldDocComment(printer, descriptor_, context_->options());
      printer->Print(
          variables_,
          "$deprecation$$value_type$ ${$get$capitalized_name$ValueOrThrow$}$(\n"
          "    $key_type$ key);\n");
      printer->Annotate("{", "}", descriptor_);
    }
  } else {
    if (context_->options().opensource_runtime) {
      printer->Print(variables_,
                     "/**\n"
                     " * Use {@link #get$capitalized_name$Map()} instead.\n"
                     " */\n"
                     "@java.lang.Deprecated\n"
                     "java.util.Map<$type_parameters$>\n"
                     "${$get$capitalized_name$$}$();\n");
      printer->Annotate("{", "}", descriptor_);
    }
    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(variables_,
                   "$deprecation$java.util.Map<$type_parameters$>\n"
                   "${$get$capitalized_name$Map$}$();\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(variables_,
                   "$deprecation$$value_type_pass_through_nullness$ "
                   "${$get$capitalized_name$OrDefault$}$(\n"
                   "    $key_type$ key,\n"
                   "    $value_type_pass_through_nullness$ defaultValue);\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(
        variables_,
        "$deprecation$$value_type$ ${$get$capitalized_name$OrThrow$}$(\n"
        "    $key_type$ key);\n");
    printer->Annotate("{", "}", descriptor_);
  }
}

void ImmutableMapFieldGenerator::GenerateMembers(io::Printer* printer) const {
  printer->Print(
      variables_,
      "private static final class $capitalized_name$DefaultEntryHolder {\n"
      "  static final com.google.protobuf.MapEntry<\n"
      "      $type_parameters$> defaultEntry =\n"
      "          com.google.protobuf.MapEntry\n"
      "          .<$type_parameters$>newDefaultInstance(\n"
      "              $descriptor$\n"
      "              $key_wire_type$,\n"
      "              $key_default_value$,\n"
      "              $value_wire_type$,\n"
      "              $value_default_value$);\n"
      "}\n");
  printer->Print(variables_,
                 "@SuppressWarnings(\"serial\")\n"
                 "private com.google.protobuf.MapField<\n"
                 "    $type_parameters$> $name$_;\n"
                 "private com.google.protobuf.MapField<$type_parameters$>\n"
                 "internalGet$capitalized_name$() {\n"
                 "  if ($name$_ == null) {\n"
                 "    return com.google.protobuf.MapField.emptyMapField(\n"
                 "        $map_field_parameter$);\n"
                 "  }\n"
                 "  return $name$_;\n"
                 "}\n");
  if (GetJavaType(MapValueField(descriptor_)) == JAVATYPE_ENUM) {
    printer->Print(
        variables_,
        "private static final\n"
        "com.google.protobuf.Internal.MapAdapter.Converter<\n"
        "    java.lang.Integer, $value_enum_type$> $name$ValueConverter =\n"
        "        com.google.protobuf.Internal.MapAdapter.newEnumConverter(\n"
        "            $value_enum_type$.internalGetValueMap(),\n"
        "            $unrecognized_value$);\n");
    printer->Print(
        variables_,
        "private static final java.util.Map<$boxed_key_type$, "
        "$value_enum_type$>\n"
        "internalGetAdapted$capitalized_name$Map(\n"
        "    java.util.Map<$boxed_key_type$, $boxed_value_type$> map) {\n"
        "  return new com.google.protobuf.Internal.MapAdapter<\n"
        "      $boxed_key_type$, $value_enum_type$, java.lang.Integer>(\n"
        "          map, $name$ValueConverter);\n"
        "}\n");
  }
  GenerateMapGetters(printer);
}

void ImmutableMapFieldGenerator::GenerateBuilderMembers(
    io::Printer* printer) const {
  if (GetJavaType(MapValueField(descriptor_)) == JAVATYPE_MESSAGE) {
    return GenerateMessageMapBuilderMembers(printer);
  }
  printer->Print(
      variables_,
      "private com.google.protobuf.MapField<\n"
      "    $type_parameters$> $name$_;\n"
      "$deprecation$private com.google.protobuf.MapField<$type_parameters$>\n"
      "    internalGet$capitalized_name$() {\n"
      "  if ($name$_ == null) {\n"
      "    return com.google.protobuf.MapField.emptyMapField(\n"
      "        $map_field_parameter$);\n"
      "  }\n"
      "  return $name$_;\n"
      "}\n"
      "$deprecation$private com.google.protobuf.MapField<$type_parameters$>\n"
      "    internalGetMutable$capitalized_name$() {\n"
      "  if ($name$_ == null) {\n"
      "    $name$_ = com.google.protobuf.MapField.newMapField(\n"
      "        $map_field_parameter$);\n"
      "  }\n"
      "  if (!$name$_.isMutable()) {\n"
      "    $name$_ = $name$_.copy();\n"
      "  }\n"
      "  $set_has_field_bit_builder$\n"
      "  $on_changed$\n"
      "  return $name$_;\n"
      "}\n");
  GenerateMapGetters(printer);
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$clear$capitalized_name$$}$() {\n"
      "  $clear_has_field_bit_builder$\n"
      "  internalGetMutable$capitalized_name$().getMutableMap()\n"
      "      .clear();\n"
      "  return this;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);

  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$public Builder ${$remove$capitalized_name$$}$(\n"
                 "    $key_type$ key) {\n"
                 "  $key_null_check$\n"
                 "  internalGetMutable$capitalized_name$().getMutableMap()\n"
                 "      .remove(key);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);

  const FieldDescriptor* value = MapValueField(descriptor_);
  if (GetJavaType(value) == JAVATYPE_ENUM) {
    if (context_->options().opensource_runtime) {
      printer->Print(
          variables_,
          "/**\n"
          " * Use alternate mutation accessors instead.\n"
          " */\n"
          "@java.lang.Deprecated\n"
          "public java.util.Map<$boxed_key_type$, $value_enum_type$>\n"
          "    ${$getMutable$capitalized_name$$}$() {\n"
          "  $set_has_field_bit_builder$\n"
          "  return internalGetAdapted$capitalized_name$Map(\n"
          "       internalGetMutable$capitalized_name$().getMutableMap());\n"
          "}\n");
      printer->Annotate("{", "}", descriptor_);
    }

    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(variables_,
                   "$deprecation$public Builder ${$put$capitalized_name$$}$(\n"
                   "    $key_type$ key,\n"
                   "    $value_enum_type$ value) {\n"
                   "  $key_null_check$\n"
                   "  $value_null_check$\n"
                   "  internalGetMutable$capitalized_name$().getMutableMap()\n"
                   "      .put(key, $name$ValueConverter.doBackward(value));\n"
                   "  $set_has_field_bit_builder$\n"
                   "  return this;\n"
                   "}\n");
    printer->Annotate("{", "}", descriptor_, Semantic::kSet);

    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(
        variables_,
        "$deprecation$public Builder ${$putAll$capitalized_name$$}$(\n"
        "    java.util.Map<$boxed_key_type$, $value_enum_type$> values) {\n"
        "  internalGetAdapted$capitalized_name$Map(\n"
        "      internalGetMutable$capitalized_name$().getMutableMap())\n"
        "          .putAll(values);\n"
        "  $set_has_field_bit_builder$\n"
        "  return this;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_, Semantic::kSet);

    if (SupportUnknownEnumValue(value)) {
      if (context_->options().opensource_runtime) {
        printer->Print(
            variables_,
            "/**\n"
            " * Use alternate mutation accessors instead.\n"
            " */\n"
            "@java.lang.Deprecated\n"
            "public java.util.Map<$boxed_key_type$, $boxed_value_type$>\n"
            "${$getMutable$capitalized_name$Value$}$() {\n"
            "  $set_has_field_bit_builder$\n"
            "  return internalGetMutable$capitalized_name$().getMutableMap();\n"
            "}\n");
        printer->Annotate("{", "}", descriptor_);
      }

      WriteFieldDocComment(printer, descriptor_, context_->options());
      printer->Print(
          variables_,
          "$deprecation$public Builder ${$put$capitalized_name$Value$}$(\n"
          "    $key_type$ key,\n"
          "    $value_type$ value) {\n"
          "  $key_null_check$\n"
          "  $value_null_check$\n"
          "  internalGetMutable$capitalized_name$().getMutableMap()\n"
          "      .put(key, value);\n"
          "  $set_has_field_bit_builder$\n"
          "  return this;\n"
          "}\n");
      printer->Annotate("{", "}", descriptor_, Semantic::kSet);

      WriteFieldDocComment(printer, descriptor_, context_->options());
      printer->Print(
          variables_,
          "$deprecation$public Builder ${$putAll$capitalized_name$Value$}$(\n"
          "    java.util.Map<$boxed_key_type$, $boxed_value_type$> values) {\n"
          "  internalGetMutable$capitalized_name$().getMutableMap()\n"
          "      .putAll(values);\n"
          "  $set_has_field_bit_builder$\n"
          "  return this;\n"
          "}\n");
      printer->Annotate("{", "}", descriptor_, Semantic::kSet);
    }
  } else {
    if (context_->options().opensource_runtime) {
      printer->Print(
          variables_,
          "/**\n"
          " * Use alternate mutation accessors instead.\n"
          " */\n"
          "@java.lang.Deprecated\n"
          "public java.util.Map<$type_parameters$>\n"
          "    ${$getMutable$capitalized_name$$}$() {\n"
          "  $set_has_field_bit_builder$\n"
          "  return internalGetMutable$capitalized_name$().getMutableMap();\n"
          "}\n");
      printer->Annotate("{", "}", descriptor_);
    }

    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(variables_,
                   "$deprecation$public Builder ${$put$capitalized_name$$}$(\n"
                   "    $key_type$ key,\n"
                   "    $value_type$ value) {\n"
                   "  $key_null_check$\n"
                   "  $value_null_check$\n"
                   "  internalGetMutable$capitalized_name$().getMutableMap()\n"
                   "      .put(key, value);\n"
                   "  $set_has_field_bit_builder$\n"
                   "  return this;\n"
                   "}\n");
    printer->Annotate("{", "}", descriptor_, Semantic::kSet);

    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(
        variables_,
        "$deprecation$public Builder ${$putAll$capitalized_name$$}$(\n"
        "    java.util.Map<$type_parameters$> values) {\n"
        "  internalGetMutable$capitalized_name$().getMutableMap()\n"
        "      .putAll(values);\n"
        "  $set_has_field_bit_builder$\n"
        "  return this;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_, Semantic::kSet);
  }
}

void ImmutableMapFieldGenerator::GenerateMapGetters(
    io::Printer* printer) const {
  printer->Print(
      variables_,
      "$deprecation$public int ${$get$capitalized_name$Count$}$() {\n"
      "  return internalGet$capitalized_name$().getMap().size();\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);

  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public boolean ${$contains$capitalized_name$$}$(\n"
      "    $key_type$ key) {\n"
      "  $key_null_check$\n"
      "  return internalGet$capitalized_name$().getMap().containsKey(key);\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);

  const FieldDescriptor* value = MapValueField(descriptor_);
  if (GetJavaType(value) == JAVATYPE_ENUM) {
    if (context_->options().opensource_runtime) {
      printer->Print(
          variables_,
          "/**\n"
          " * Use {@link #get$capitalized_name$Map()} instead.\n"
          " */\n"
          "@java.lang.Override\n"
          "@java.lang.Deprecated\n"
          "public java.util.Map<$boxed_key_type$, $value_enum_type$>\n"
          "${$get$capitalized_name$$}$() {\n"
          "  return get$capitalized_name$Map();\n"
          "}\n");
      printer->Annotate("{", "}", descriptor_);
    }

    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(variables_,
                   "@java.lang.Override\n"
                   "$deprecation$public java.util.Map<$boxed_key_type$, "
                   "$value_enum_type$>\n"
                   "${$get$capitalized_name$Map$}$() {\n"
                   "  return internalGetAdapted$capitalized_name$Map(\n"
                   "      internalGet$capitalized_name$().getMap());"
                   "}\n");
    printer->Annotate("{", "}", descriptor_);

    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$public $value_enum_type_pass_through_nullness$ "
        "${$get$capitalized_name$OrDefault$}$(\n"
        "    $key_type$ key,\n"
        "    $value_enum_type_pass_through_nullness$ defaultValue) {\n"
        "  $key_null_check$\n"
        "  java.util.Map<$boxed_key_type$, $boxed_value_type$> map =\n"
        "      internalGet$capitalized_name$().getMap();\n"
        "  return map.containsKey(key)\n"
        "         ? $name$ValueConverter.doForward(map.get(key))\n"
        "         : defaultValue;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);

    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$public $value_enum_type$ "
        "${$get$capitalized_name$OrThrow$}$(\n"
        "    $key_type$ key) {\n"
        "  $key_null_check$\n"
        "  java.util.Map<$boxed_key_type$, $boxed_value_type$> map =\n"
        "      internalGet$capitalized_name$().getMap();\n"
        "  if (!map.containsKey(key)) {\n"
        "    throw new java.lang.IllegalArgumentException();\n"
        "  }\n"
        "  return $name$ValueConverter.doForward(map.get(key));\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);

    if (SupportUnknownEnumValue(value)) {
      printer->Print(
          variables_,
          "/**\n"
          " * Use {@link #get$capitalized_name$ValueMap()} instead.\n"
          " */\n"
          "@java.lang.Override\n"
          "@java.lang.Deprecated\n"
          "public java.util.Map<$boxed_key_type$, $boxed_value_type$>\n"
          "${$get$capitalized_name$Value$}$() {\n"
          "  return get$capitalized_name$ValueMap();\n"
          "}\n");
      printer->Annotate("{", "}", descriptor_);
      WriteFieldDocComment(printer, descriptor_, context_->options());
      printer->Print(variables_,
                     "@java.lang.Override\n"
                     "$deprecation$public java.util.Map<$boxed_key_type$, "
                     "$boxed_value_type$>\n"
                     "${$get$capitalized_name$ValueMap$}$() {\n"
                     "  return internalGet$capitalized_name$().getMap();\n"
                     "}\n");
      printer->Annotate("{", "}", descriptor_);
      WriteFieldDocComment(printer, descriptor_, context_->options());
      printer->Print(
          variables_,
          "@java.lang.Override\n"
          "$deprecation$public $value_type_pass_through_nullness$ "
          "${$get$capitalized_name$ValueOrDefault$}$(\n"
          "    $key_type$ key,\n"
          "    $value_type_pass_through_nullness$ defaultValue) {\n"
          "  $key_null_check$\n"
          "  java.util.Map<$boxed_key_type$, $boxed_value_type$> map =\n"
          "      internalGet$capitalized_name$().getMap();\n"
          "  return map.containsKey(key) ? map.get(key) : defaultValue;\n"
          "}\n");
      printer->Annotate("{", "}", descriptor_);
      WriteFieldDocComment(printer, descriptor_, context_->options());
      printer->Print(
          variables_,
          "@java.lang.Override\n"
          "$deprecation$public $value_type$ "
          "${$get$capitalized_name$ValueOrThrow$}$(\n"
          "    $key_type$ key) {\n"
          "  $key_null_check$\n"
          "  java.util.Map<$boxed_key_type$, $boxed_value_type$> map =\n"
          "      internalGet$capitalized_name$().getMap();\n"
          "  if (!map.containsKey(key)) {\n"
          "    throw new java.lang.IllegalArgumentException();\n"
          "  }\n"
          "  return map.get(key);\n"
          "}\n");
      printer->Annotate("{", "}", descriptor_);
    }
  } else {
    if (context_->options().opensource_runtime) {
      printer->Print(variables_,
                     "/**\n"
                     " * Use {@link #get$capitalized_name$Map()} instead.\n"
                     " */\n"
                     "@java.lang.Override\n"
                     "@java.lang.Deprecated\n"
                     "public java.util.Map<$type_parameters$> "
                     "${$get$capitalized_name$$}$() {\n"
                     "  return get$capitalized_name$Map();\n"
                     "}\n");
      printer->Annotate("{", "}", descriptor_);
    }
    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(variables_,
                   "@java.lang.Override\n"
                   "$deprecation$public java.util.Map<$type_parameters$> "
                   "${$get$capitalized_name$Map$}$() {\n"
                   "  return internalGet$capitalized_name$().getMap();\n"
                   "}\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$public $value_type_pass_through_nullness$ "
        "${$get$capitalized_name$OrDefault$}$(\n"
        "    $key_type$ key,\n"
        "    $value_type_pass_through_nullness$ defaultValue) {\n"
        "  $key_null_check$\n"
        "  java.util.Map<$type_parameters$> map =\n"
        "      internalGet$capitalized_name$().getMap();\n"
        "  return map.containsKey(key) ? map.get(key) : defaultValue;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$public $value_type$ ${$get$capitalized_name$OrThrow$}$(\n"
        "    $key_type$ key) {\n"
        "  $key_null_check$\n"
        "  java.util.Map<$type_parameters$> map =\n"
        "      internalGet$capitalized_name$().getMap();\n"
        "  if (!map.containsKey(key)) {\n"
        "    throw new java.lang.IllegalArgumentException();\n"
        "  }\n"
        "  return map.get(key);\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
  }
}

void ImmutableMapFieldGenerator::GenerateMessageMapBuilderMembers(
    io::Printer* printer) const {
  printer->Print(
      variables_,
      "private static final class $capitalized_name$Converter implements "
      "com.google.protobuf.MapFieldBuilder.Converter<$boxed_key_type$, "
      "$value_interface_type$, $boxed_value_type$> "
      "{\n");
  {
    auto i1 = printer->WithIndent();
    printer->Print("@java.lang.Override\n");
    printer->Print(
        variables_,
        "public $boxed_value_type$ build($value_interface_type$ val) {\n");
    {
      auto i2 = printer->WithIndent();
      printer->Print(variables_,
                     "if (val instanceof $boxed_value_type$) {"
                     " return ($boxed_value_type$) val; }\n");
      printer->Print(variables_,
                     "return (($value_builder_type$) val).build();\n");
    }
    printer->Print("}\n\n");

    printer->Print("@java.lang.Override\n");
    printer->Print(variables_,
                   "public com.google.protobuf.MapEntry<$boxed_key_type$, "
                   "$boxed_value_type$> defaultEntry() {\n");
    {
      auto i2 = printer->WithIndent();
      printer->Print(
          variables_,
          "return $capitalized_name$DefaultEntryHolder.defaultEntry;\n");
    }
    printer->Print("}\n");
  }
  printer->Print("};\n");
  printer->Print(variables_,
                 "private static final $capitalized_name$Converter "
                 "$name$Converter = new $capitalized_name$Converter();\n\n");

  printer->Print(
      variables_,
      "private com.google.protobuf.MapFieldBuilder<\n"
      "    $builder_type_parameters$> $name$_;\n"
      "$deprecation$private "
      "com.google.protobuf.MapFieldBuilder<$builder_type_parameters$>\n"
      "    internalGet$capitalized_name$() {\n"
      "  if ($name$_ == null) {\n"
      "    return new com.google.protobuf.MapFieldBuilder<>($name$Converter);\n"
      "  }\n"
      "  return $name$_;\n"
      "}\n"
      "$deprecation$private "
      "com.google.protobuf.MapFieldBuilder<$builder_type_parameters$>\n"
      "    internalGetMutable$capitalized_name$() {\n"
      "  if ($name$_ == null) {\n"
      "    $name$_ = new "
      "com.google.protobuf.MapFieldBuilder<>($name$Converter);\n"
      "  }\n"
      "  $set_has_field_bit_builder$\n"
      "  $on_changed$\n"
      "  return $name$_;\n"
      "}\n");
  GenerateMessageMapGetters(printer);
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$clear$capitalized_name$$}$() {\n"
      "  $clear_has_field_bit_builder$\n"
      "  internalGetMutable$capitalized_name$().clear();\n"
      "  return this;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);

  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$public Builder ${$remove$capitalized_name$$}$(\n"
                 "    $key_type$ key) {\n"
                 "  $key_null_check$\n"
                 "  internalGetMutable$capitalized_name$().ensureBuilderMap()\n"
                 "      .remove(key);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);

  if (context_->options().opensource_runtime) {
    printer->Print(
        variables_,
        "/**\n"
        " * Use alternate mutation accessors instead.\n"
        " */\n"
        "@java.lang.Deprecated\n"
        "public java.util.Map<$type_parameters$>\n"
        "    ${$getMutable$capitalized_name$$}$() {\n"
        "  $set_has_field_bit_builder$\n"
        "  return internalGetMutable$capitalized_name$().ensureMessageMap();\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
  }

  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$public Builder ${$put$capitalized_name$$}$(\n"
                 "    $key_type$ key,\n"
                 "    $value_type$ value) {\n"
                 "  $key_null_check$\n"
                 "  $value_null_check$\n"
                 "  internalGetMutable$capitalized_name$().ensureBuilderMap()\n"
                 "      .put(key, value);\n"
                 "  $set_has_field_bit_builder$\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);

  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$putAll$capitalized_name$$}$(\n"
      "    java.util.Map<$type_parameters$> values) {\n"
      "  for (java.util.Map.Entry<$type_parameters$> e : values.entrySet()) {\n"
      "    if (e.getKey() == null || e.getValue() == null) {\n"
      "      throw new NullPointerException();\n"
      "    }\n"
      "  }\n"
      "  internalGetMutable$capitalized_name$().ensureBuilderMap()\n"
      "      .putAll(values);\n"
      "  $set_has_field_bit_builder$\n"
      "  return this;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);

  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "$deprecation$public $value_builder_type$ "
      "${$put$capitalized_name$BuilderIfAbsent$}$(\n"
      "    $key_type$ key) {\n"
      "  java.util.Map<$boxed_key_type$, $value_interface_type$> builderMap = "
      "internalGetMutable$capitalized_name$().ensureBuilderMap();\n"
      "  $value_interface_type$ entry = builderMap.get(key);\n"
      "  if (entry == null) {\n"
      "    entry = $value_type$.newBuilder();\n"
      "    builderMap.put(key, entry);\n"
      "  }\n"
      "  if (entry instanceof $value_type$) {\n"
      "    entry = (($value_type$) entry).toBuilder();\n"
      "    builderMap.put(key, entry);\n"
      "  }\n"
      "  return ($value_builder_type$) entry;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void ImmutableMapFieldGenerator::GenerateMessageMapGetters(
    io::Printer* printer) const {
  printer->Print(
      variables_,
      "$deprecation$public int ${$get$capitalized_name$Count$}$() {\n"
      "  return internalGet$capitalized_name$().ensureBuilderMap().size();\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);

  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public boolean ${$contains$capitalized_name$$}$(\n"
      "    $key_type$ key) {\n"
      "  $key_null_check$\n"
      "  return "
      "internalGet$capitalized_name$().ensureBuilderMap().containsKey(key);\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
  if (context_->options().opensource_runtime) {
    printer->Print(variables_,
                   "/**\n"
                   " * Use {@link #get$capitalized_name$Map()} instead.\n"
                   " */\n"
                   "@java.lang.Override\n"
                   "@java.lang.Deprecated\n"
                   "public java.util.Map<$type_parameters$> "
                   "${$get$capitalized_name$$}$() {\n"
                   "  return get$capitalized_name$Map();\n"
                   "}\n");
    printer->Annotate("{", "}", descriptor_);
  }
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public java.util.Map<$type_parameters$> "
                 "${$get$capitalized_name$Map$}$() {\n"
                 "  return internalGet$capitalized_name$().getImmutableMap();\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public $value_type_pass_through_nullness$ "
      "${$get$capitalized_name$OrDefault$}$(\n"
      "    $key_type$ key,\n"
      "    $value_type_pass_through_nullness$ defaultValue) {\n"
      "  $key_null_check$\n"
      "  java.util.Map<$boxed_key_type$, $value_interface_type$> map = "
      "internalGetMutable$capitalized_name$().ensureBuilderMap();\n"
      "  return map.containsKey(key) ? $name$Converter.build(map.get(key)) : "
      "defaultValue;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public $value_type$ ${$get$capitalized_name$OrThrow$}$(\n"
      "    $key_type$ key) {\n"
      "  $key_null_check$\n"
      "  java.util.Map<$boxed_key_type$, $value_interface_type$> map = "
      "internalGetMutable$capitalized_name$().ensureBuilderMap();\n"
      "  if (!map.containsKey(key)) {\n"
      "    throw new java.lang.IllegalArgumentException();\n"
      "  }\n"
      "  return $name$Converter.build(map.get(key));\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void ImmutableMapFieldGenerator::GenerateKotlinDslMembers(
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
  printer->Print(
      variables_,
      "$kt_deprecation$ public val $kt_name$: "
      "com.google.protobuf.kotlin.DslMap"
      "<$kt_key_type$, $kt_value_type$, ${$$kt_capitalized_name$Proxy$}$>\n"
      "  @kotlin.jvm.JvmSynthetic\n"
      "  @JvmName(\"get$kt_capitalized_name$Map\")\n"
      "  get() = com.google.protobuf.kotlin.DslMap(\n"
      "    $kt_dsl_builder$.${$get$capitalized_name$Map$}$()\n"
      "  )\n");

  WriteFieldDocComment(printer, descriptor_, context_->options(),
                       /* kdoc */ true);
  printer->Print(
      variables_,
      "@JvmName(\"put$kt_capitalized_name$\")\n"
      "public fun com.google.protobuf.kotlin.DslMap"
      "<$kt_key_type$, $kt_value_type$, ${$$kt_capitalized_name$Proxy$}$>\n"
      "  .put(key: $kt_key_type$, value: $kt_value_type$) {\n"
      "     $kt_dsl_builder$.${$put$capitalized_name$$}$(key, value)\n"
      "   }\n");

  WriteFieldDocComment(printer, descriptor_, context_->options(),
                       /* kdoc */ true);
  printer->Print(
      variables_,
      "@kotlin.jvm.JvmSynthetic\n"
      "@JvmName(\"set$kt_capitalized_name$\")\n"
      "@Suppress(\"NOTHING_TO_INLINE\")\n"
      "public inline operator fun com.google.protobuf.kotlin.DslMap"
      "<$kt_key_type$, $kt_value_type$, ${$$kt_capitalized_name$Proxy$}$>\n"
      "  .set(key: $kt_key_type$, value: $kt_value_type$) {\n"
      "     put(key, value)\n"
      "   }\n");

  WriteFieldDocComment(printer, descriptor_, context_->options(),
                       /* kdoc */ true);
  printer->Print(
      variables_,
      "@kotlin.jvm.JvmSynthetic\n"
      "@JvmName(\"remove$kt_capitalized_name$\")\n"
      "public fun com.google.protobuf.kotlin.DslMap"
      "<$kt_key_type$, $kt_value_type$, ${$$kt_capitalized_name$Proxy$}$>\n"
      "  .remove(key: $kt_key_type$) {\n"
      "     $kt_dsl_builder$.${$remove$capitalized_name$$}$(key)\n"
      "   }\n");

  WriteFieldDocComment(printer, descriptor_, context_->options(),
                       /* kdoc */ true);
  printer->Print(
      variables_,
      "@kotlin.jvm.JvmSynthetic\n"
      "@JvmName(\"putAll$kt_capitalized_name$\")\n"
      "public fun com.google.protobuf.kotlin.DslMap"
      "<$kt_key_type$, $kt_value_type$, ${$$kt_capitalized_name$Proxy$}$>\n"
      "  .putAll(map: kotlin.collections.Map<$kt_key_type$, "
      "$kt_value_type$>) "
      "{\n"
      "     $kt_dsl_builder$.${$putAll$capitalized_name$$}$(map)\n"
      "   }\n");

  WriteFieldDocComment(printer, descriptor_, context_->options(),
                       /* kdoc */ true);
  printer->Print(
      variables_,
      "@kotlin.jvm.JvmSynthetic\n"
      "@JvmName(\"clear$kt_capitalized_name$\")\n"
      "public fun com.google.protobuf.kotlin.DslMap"
      "<$kt_key_type$, $kt_value_type$, ${$$kt_capitalized_name$Proxy$}$>\n"
      "  .clear() {\n"
      "     $kt_dsl_builder$.${$clear$capitalized_name$$}$()\n"
      "   }\n");
}

void ImmutableMapFieldGenerator::GenerateFieldBuilderInitializationCode(
    io::Printer* printer) const {
  // Nothing to initialize.
}

void ImmutableMapFieldGenerator::GenerateInitializationCode(
    io::Printer* printer) const {
  // Nothing to initialize.
}

void ImmutableMapFieldGenerator::GenerateBuilderClearCode(
    io::Printer* printer) const {
  // No need to clear the has-bit since we clear the bitField ints all at once.
  printer->Print(variables_,
                 "internalGetMutable$capitalized_name$().clear();\n");
}

void ImmutableMapFieldGenerator::GenerateMergingCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "internalGetMutable$capitalized_name$().mergeFrom(\n"
                 "    other.internalGet$capitalized_name$());\n"
                 "$set_has_field_bit_builder$\n");
}

void ImmutableMapFieldGenerator::GenerateBuildingCode(
    io::Printer* printer) const {
  if (GetJavaType(MapValueField(descriptor_)) == JAVATYPE_MESSAGE) {
    printer->Print(
        variables_,
        "if ($get_has_field_bit_from_local$) {\n"
        "  result.$name$_ = "
        "internalGet$capitalized_name$().build($map_field_parameter$);\n"
        "}\n");
    return;
  }
  printer->Print(variables_,
                 "if ($get_has_field_bit_from_local$) {\n"
                 "  result.$name$_ = internalGet$capitalized_name$();\n"
                 "  result.$name$_.makeImmutable();\n"
                 "}\n");
}

void ImmutableMapFieldGenerator::GenerateBuilderParsingCode(
    io::Printer* printer) const {
  const FieldDescriptor* value = MapValueField(descriptor_);
  const JavaType type = GetJavaType(value);
  if (type == JAVATYPE_MESSAGE) {
    printer->Print(
        variables_,
        "com.google.protobuf.MapEntry<$type_parameters$>\n"
        "$name$__ = input.readMessage(\n"
        "    $default_entry$.getParserForType(), extensionRegistry);\n"
        "internalGetMutable$capitalized_name$().ensureBuilderMap().put(\n"
        "    $name$__.getKey(), $name$__.getValue());\n"
        "$set_has_field_bit_builder$\n");
    return;
  }
  if (!SupportUnknownEnumValue(value) && type == JAVATYPE_ENUM) {
    printer->Print(
        variables_,
        "com.google.protobuf.ByteString bytes = input.readBytes();\n"
        "com.google.protobuf.MapEntry<$type_parameters$>\n"
        "$name$__ = $default_entry$.getParserForType().parseFrom(bytes);\n"
        "if ($value_enum_type$.forNumber($name$__.getValue()) == null) {\n"
        "  mergeUnknownLengthDelimitedField($number$, bytes);\n"
        "} else {\n"
        "  internalGetMutable$capitalized_name$().getMutableMap().put(\n"
        "      $name$__.getKey(), $name$__.getValue());\n"
        "  $set_has_field_bit_builder$\n"
        "}\n");
    return;
  }
  printer->Print(variables_,
                 "com.google.protobuf.MapEntry<$type_parameters$>\n"
                 "$name$__ = input.readMessage(\n"
                 "    $default_entry$.getParserForType(), extensionRegistry);\n"
                 "internalGetMutable$capitalized_name$().getMutableMap().put(\n"
                 "    $name$__.getKey(), $name$__.getValue());\n"
                 "$set_has_field_bit_builder$\n");
}
void ImmutableMapFieldGenerator::GenerateSerializationCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "com.google.protobuf.GeneratedMessage\n"
                 "  .serialize$short_key_type$MapTo(\n"
                 "    output,\n"
                 "    internalGet$capitalized_name$(),\n"
                 "    $default_entry$,\n"
                 "    $number$);\n");
}

void ImmutableMapFieldGenerator::GenerateSerializedSizeCode(
    io::Printer* printer) const {
  printer->Print(
      variables_,
      "for (java.util.Map.Entry<$type_parameters$> entry\n"
      "     : internalGet$capitalized_name$().getMap().entrySet()) {\n"
      "  com.google.protobuf.MapEntry<$type_parameters$>\n"
      "  $name$__ = $default_entry$.newBuilderForType()\n"
      "      .setKey(entry.getKey())\n"
      "      .setValue(entry.getValue())\n"
      "      .build();\n"
      "  size += com.google.protobuf.CodedOutputStream\n"
      "      .computeMessageSize($number$, $name$__);\n"
      "}\n");
}

void ImmutableMapFieldGenerator::GenerateEqualsCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "if (!internalGet$capitalized_name$().equals(\n"
                 "    other.internalGet$capitalized_name$())) return false;\n");
}

void ImmutableMapFieldGenerator::GenerateHashCode(io::Printer* printer) const {
  printer->Print(
      variables_,
      "if (!internalGet$capitalized_name$().getMap().isEmpty()) {\n"
      "  hash = (37 * hash) + $constant_name$;\n"
      "  hash = (53 * hash) + internalGet$capitalized_name$().hashCode();\n"
      "}\n");
}

std::string ImmutableMapFieldGenerator::GetBoxedType() const {
  return name_resolver_->GetImmutableClassName(descriptor_->message_type());
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
