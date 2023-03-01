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

#include "google/protobuf/compiler/java/map_field.h"

#include "absl/strings/str_cat.h"
#include "google/protobuf/compiler/java/context.h"
#include "google/protobuf/compiler/java/doc_comment.h"
#include "google/protobuf/compiler/java/helpers.h"
#include "google/protobuf/compiler/java/name_resolver.h"
#include "google/protobuf/io/printer.h"

// Must be last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

namespace {

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

void SetMessageVariables(
    const FieldDescriptor* descriptor, int messageBitIndex, int builderBitIndex,
    const FieldGeneratorInfo* info, Context* context,
    absl::flat_hash_map<absl::string_view, std::string>* variables) {
  SetCommonFieldVariables(descriptor, info, variables);
  ClassNameResolver* name_resolver = context->GetNameResolver();

  (*variables)["type"] =
      name_resolver->GetImmutableClassName(descriptor->message_type());
  const FieldDescriptor* key = MapKeyField(descriptor);
  const FieldDescriptor* value = MapValueField(descriptor);
  const JavaType keyJavaType = GetJavaType(key);
  const JavaType valueJavaType = GetJavaType(value);

  // The code that generates the open-source version appears not to understand
  // #else, so we have an #ifndef instead.
  std::string pass_through_nullness =
      context->options().opensource_runtime
          ? "/* nullable */\n"
          : "@com.google.protobuf.Internal.ProtoPassThroughNullness ";

  (*variables)["key_type"] = TypeName(key, name_resolver, false);
  std::string boxed_key_type = TypeName(key, name_resolver, true);
  (*variables)["boxed_key_type"] = boxed_key_type;
  (*variables)["kt_key_type"] = KotlinTypeName(key, name_resolver);
  (*variables)["kt_value_type"] = KotlinTypeName(value, name_resolver);
  // Used for calling the serialization function.
  (*variables)["short_key_type"] =
      boxed_key_type.substr(boxed_key_type.rfind('.') + 1);
  (*variables)["key_wire_type"] = WireType(key);
  (*variables)["key_default_value"] =
      DefaultValue(key, true, name_resolver, context->options());
  (*variables)["key_null_check"] =
      IsReferenceType(keyJavaType)
          ? "if (key == null) { throw new NullPointerException(\"map key\"); }"
          : "";
  (*variables)["value_null_check"] =
      valueJavaType != JAVATYPE_ENUM && IsReferenceType(valueJavaType)
          ? "if (value == null) { "
            "throw new NullPointerException(\"map value\"); }"
          : "";
  if (valueJavaType == JAVATYPE_ENUM) {
    // We store enums as Integers internally.
    (*variables)["value_type"] = "int";
    variables->insert(
        {"value_type_pass_through_nullness", (*variables)["value_type"]});
    (*variables)["boxed_value_type"] = "java.lang.Integer";
    (*variables)["value_wire_type"] = WireType(value);
    (*variables)["value_default_value"] =
        DefaultValue(value, true, name_resolver, context->options()) +
        ".getNumber()";

    (*variables)["value_enum_type"] = TypeName(value, name_resolver, false);

    variables->insert(
        {"value_enum_type_pass_through_nullness",
         absl::StrCat(pass_through_nullness, (*variables)["value_enum_type"])});

    if (SupportUnknownEnumValue(value)) {
      // Map unknown values to a special UNRECOGNIZED value if supported.
      variables->insert(
          {"unrecognized_value",
           absl::StrCat((*variables)["value_enum_type"], ".UNRECOGNIZED")});
    } else {
      // Map unknown values to the default value if we don't have UNRECOGNIZED.
      (*variables)["unrecognized_value"] =
          DefaultValue(value, true, name_resolver, context->options());
    }
  } else {
    (*variables)["value_type"] = TypeName(value, name_resolver, false);

    variables->insert(
        {"value_type_pass_through_nullness",
         absl::StrCat(
             (IsReferenceType(valueJavaType) ? pass_through_nullness : ""),
             (*variables)["value_type"])});

    (*variables)["boxed_value_type"] = TypeName(value, name_resolver, true);
    (*variables)["value_wire_type"] = WireType(value);
    (*variables)["value_default_value"] =
        DefaultValue(value, true, name_resolver, context->options());
  }
  variables->insert(
      {"type_parameters", absl::StrCat((*variables)["boxed_key_type"], ", ",
                                       (*variables)["boxed_value_type"])});
  // TODO(birdo): Add @deprecated javadoc when generating javadoc is supported
  // by the proto compiler
  (*variables)["deprecation"] =
      descriptor->options().deprecated() ? "@java.lang.Deprecated " : "";
  variables->insert(
      {"kt_deprecation",
       descriptor->options().deprecated()
           ? absl::StrCat("@kotlin.Deprecated(message = \"Field ",
                          (*variables)["name"], " is deprecated\") ")
           : ""});
  (*variables)["on_changed"] = "onChanged();";

  variables->insert(
      {"default_entry", absl::StrCat((*variables)["capitalized_name"],
                                     "DefaultEntryHolder.defaultEntry")});
  variables->insert({"map_field_parameter", (*variables)["default_entry"]});
  (*variables)["descriptor"] = absl::StrCat(
      name_resolver->GetImmutableClassName(descriptor->file()), ".internal_",
      UniqueFileScopeIdentifier(descriptor->message_type()), "_descriptor, ");
  (*variables)["ver"] = GeneratedCodeVersionSuffix();

  (*variables)["get_has_field_bit_builder"] = GenerateGetBit(builderBitIndex);
  (*variables)["get_has_field_bit_from_local"] =
      GenerateGetBitFromLocal(builderBitIndex);
  (*variables)["set_has_field_bit_builder"] =
      absl::StrCat(GenerateSetBit(builderBitIndex), ";");
  (*variables)["clear_has_field_bit_builder"] =
      absl::StrCat(GenerateClearBit(builderBitIndex), ";");
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
  SetMessageVariables(descriptor, messageBitIndex, builderBitIndex,
                      context->GetFieldGeneratorInfo(descriptor), context,
                      &variables_);
}

ImmutableMapFieldGenerator::~ImmutableMapFieldGenerator() {}

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
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "$deprecation$int ${$get$capitalized_name$Count$}$();\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldDocComment(printer, descriptor_);
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
    WriteFieldDocComment(printer, descriptor_);
    printer->Print(
        variables_,
        "$deprecation$java.util.Map<$boxed_key_type$, $value_enum_type$>\n"
        "${$get$capitalized_name$Map$}$();\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldDocComment(printer, descriptor_);
    printer->Print(variables_,
                   "$deprecation$$value_enum_type_pass_through_nullness$ "
                   "${$get$capitalized_name$OrDefault$}$(\n"
                   "    $key_type$ key,\n"
                   "    $value_enum_type_pass_through_nullness$ "
                   "        defaultValue);\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldDocComment(printer, descriptor_);
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
      WriteFieldDocComment(printer, descriptor_);
      printer->Print(variables_,
                     "$deprecation$java.util.Map<$type_parameters$>\n"
                     "${$get$capitalized_name$ValueMap$}$();\n");
      printer->Annotate("{", "}", descriptor_);
      WriteFieldDocComment(printer, descriptor_);
      printer->Print(variables_,
                     "$deprecation$$value_type_pass_through_nullness$ "
                     "${$get$capitalized_name$ValueOrDefault$}$(\n"
                     "    $key_type$ key,\n"
                     "    $value_type_pass_through_nullness$ defaultValue);\n");
      printer->Annotate("{", "}", descriptor_);
      WriteFieldDocComment(printer, descriptor_);
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
    WriteFieldDocComment(printer, descriptor_);
    printer->Print(variables_,
                   "$deprecation$java.util.Map<$type_parameters$>\n"
                   "${$get$capitalized_name$Map$}$();\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldDocComment(printer, descriptor_);
    printer->Print(variables_,
                   "$deprecation$$value_type_pass_through_nullness$ "
                   "${$get$capitalized_name$OrDefault$}$(\n"
                   "    $key_type$ key,\n"
                   "    $value_type_pass_through_nullness$ defaultValue);\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldDocComment(printer, descriptor_);
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
  printer->Annotate("{", "}", descriptor_);

  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$remove$capitalized_name$$}$(\n"
                 "    $key_type$ key) {\n"
                 "  $key_null_check$\n"
                 "  internalGetMutable$capitalized_name$().getMutableMap()\n"
                 "      .remove(key);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

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

    WriteFieldDocComment(printer, descriptor_);
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
    printer->Annotate("{", "}", descriptor_);

    WriteFieldDocComment(printer, descriptor_);
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
    printer->Annotate("{", "}", descriptor_);

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

      WriteFieldDocComment(printer, descriptor_);
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
      printer->Annotate("{", "}", descriptor_);

      WriteFieldDocComment(printer, descriptor_);
      printer->Print(
          variables_,
          "$deprecation$public Builder ${$putAll$capitalized_name$Value$}$(\n"
          "    java.util.Map<$boxed_key_type$, $boxed_value_type$> values) {\n"
          "  internalGetMutable$capitalized_name$().getMutableMap()\n"
          "      .putAll(values);\n"
          "  $set_has_field_bit_builder$\n"
          "  return this;\n"
          "}\n");
      printer->Annotate("{", "}", descriptor_);
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

    WriteFieldDocComment(printer, descriptor_);
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
    printer->Annotate("{", "}", descriptor_);

    WriteFieldDocComment(printer, descriptor_);
    printer->Print(
        variables_,
        "$deprecation$public Builder ${$putAll$capitalized_name$$}$(\n"
        "    java.util.Map<$type_parameters$> values) {\n"
        "  internalGetMutable$capitalized_name$().getMutableMap()\n"
        "      .putAll(values);\n"
        "  $set_has_field_bit_builder$\n"
        "  return this;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
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

  WriteFieldDocComment(printer, descriptor_);
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

    WriteFieldDocComment(printer, descriptor_);
    printer->Print(variables_,
                   "@java.lang.Override\n"
                   "$deprecation$public java.util.Map<$boxed_key_type$, "
                   "$value_enum_type$>\n"
                   "${$get$capitalized_name$Map$}$() {\n"
                   "  return internalGetAdapted$capitalized_name$Map(\n"
                   "      internalGet$capitalized_name$().getMap());"
                   "}\n");
    printer->Annotate("{", "}", descriptor_);

    WriteFieldDocComment(printer, descriptor_);
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

    WriteFieldDocComment(printer, descriptor_);
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$public $value_enum_type$ get$capitalized_name$OrThrow(\n"
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
      WriteFieldDocComment(printer, descriptor_);
      printer->Print(variables_,
                     "@java.lang.Override\n"
                     "$deprecation$public java.util.Map<$boxed_key_type$, "
                     "$boxed_value_type$>\n"
                     "${$get$capitalized_name$ValueMap$}$() {\n"
                     "  return internalGet$capitalized_name$().getMap();\n"
                     "}\n");
      printer->Annotate("{", "}", descriptor_);
      WriteFieldDocComment(printer, descriptor_);
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
      WriteFieldDocComment(printer, descriptor_);
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
    WriteFieldDocComment(printer, descriptor_);
    printer->Print(variables_,
                   "@java.lang.Override\n"
                   "$deprecation$public java.util.Map<$type_parameters$> "
                   "${$get$capitalized_name$Map$}$() {\n"
                   "  return internalGet$capitalized_name$().getMap();\n"
                   "}\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldDocComment(printer, descriptor_);
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
    WriteFieldDocComment(printer, descriptor_);
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

  WriteFieldDocComment(printer, descriptor_, /* kdoc */ true);
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

  WriteFieldDocComment(printer, descriptor_, /* kdoc */ true);
  printer->Print(
      variables_,
      "@JvmName(\"put$kt_capitalized_name$\")\n"
      "public fun com.google.protobuf.kotlin.DslMap"
      "<$kt_key_type$, $kt_value_type$, ${$$kt_capitalized_name$Proxy$}$>\n"
      "  .put(key: $kt_key_type$, value: $kt_value_type$) {\n"
      "     $kt_dsl_builder$.${$put$capitalized_name$$}$(key, value)\n"
      "   }\n");

  WriteFieldDocComment(printer, descriptor_, /* kdoc */ true);
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

  WriteFieldDocComment(printer, descriptor_, /* kdoc */ true);
  printer->Print(
      variables_,
      "@kotlin.jvm.JvmSynthetic\n"
      "@JvmName(\"remove$kt_capitalized_name$\")\n"
      "public fun com.google.protobuf.kotlin.DslMap"
      "<$kt_key_type$, $kt_value_type$, ${$$kt_capitalized_name$Proxy$}$>\n"
      "  .remove(key: $kt_key_type$) {\n"
      "     $kt_dsl_builder$.${$remove$capitalized_name$$}$(key)\n"
      "   }\n");

  WriteFieldDocComment(printer, descriptor_, /* kdoc */ true);
  printer->Print(
      variables_,
      "@kotlin.jvm.JvmSynthetic\n"
      "@JvmName(\"putAll$kt_capitalized_name$\")\n"
      "public fun com.google.protobuf.kotlin.DslMap"
      "<$kt_key_type$, $kt_value_type$, ${$$kt_capitalized_name$Proxy$}$>\n"
      "  .putAll(map: kotlin.collections.Map<$kt_key_type$, $kt_value_type$>) "
      "{\n"
      "     $kt_dsl_builder$.${$putAll$capitalized_name$$}$(map)\n"
      "   }\n");

  WriteFieldDocComment(printer, descriptor_, /* kdoc */ true);
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
  printer->Print(variables_,
                 "if ($get_has_field_bit_from_local$) {\n"
                 "  result.$name$_ = internalGet$capitalized_name$();\n"
                 "  result.$name$_.makeImmutable();\n"
                 "}\n");
}

void ImmutableMapFieldGenerator::GenerateBuilderParsingCode(
    io::Printer* printer) const {
  const FieldDescriptor* value = MapValueField(descriptor_);
  if (!SupportUnknownEnumValue(value) && GetJavaType(value) == JAVATYPE_ENUM) {
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
  } else {
    printer->Print(
        variables_,
        "com.google.protobuf.MapEntry<$type_parameters$>\n"
        "$name$__ = input.readMessage(\n"
        "    $default_entry$.getParserForType(), extensionRegistry);\n"
        "internalGetMutable$capitalized_name$().getMutableMap().put(\n"
        "    $name$__.getKey(), $name$__.getValue());\n"
        "$set_has_field_bit_builder$\n");
  }
}
void ImmutableMapFieldGenerator::GenerateSerializationCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "com.google.protobuf.GeneratedMessage$ver$\n"
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
