// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/java/map_field_lite.h"

#include <cstdint>
#include <string>

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

  std::string pass_through_nullness =
      context->options().opensource_runtime
          ? "/* nullable */\n"
          : "@com.google.protobuf.Internal.ProtoPassThroughNullness ";

  (*variables)["key_type"] = TypeName(key, name_resolver, false);
  (*variables)["boxed_key_type"] = TypeName(key, name_resolver, true);
  (*variables)["kt_key_type"] = KotlinTypeName(key, name_resolver);
  (*variables)["kt_value_type"] = KotlinTypeName(value, name_resolver);
  (*variables)["key_wire_type"] = WireType(key);
  (*variables)["key_default_value"] =
      DefaultValue(key, true, name_resolver, context->options());
  // We use `x.getClass()` as a null check because it generates less bytecode
  // than an `if (x == null) { throw ... }` statement.
  (*variables)["key_null_check"] =
      IsReferenceType(keyJavaType)
          ? "java.lang.Class<?> keyClass = key.getClass();"
          : "";
  (*variables)["value_null_check"] =
      IsReferenceType(valueJavaType)
          ? "java.lang.Class<?> valueClass = value.getClass();"
          : "";

  if (GetJavaType(value) == JAVATYPE_ENUM) {
    // We store enums as Integers internally.
    (*variables)["value_type"] = "int";
    (*variables)["value_type_pass_through_nullness"] = "int";
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

  variables->insert(
      {"default_entry", absl::StrCat((*variables)["capitalized_name"],
                                     "DefaultEntryHolder.defaultEntry")});
  // { and } variables are used as delimiters when emitting annotations.
  (*variables)["{"] = "";
  (*variables)["}"] = "";
}

}  // namespace

ImmutableMapFieldLiteGenerator::ImmutableMapFieldLiteGenerator(
    const FieldDescriptor* descriptor, int messageBitIndex, Context* context)
    : descriptor_(descriptor),
      context_(context),
      name_resolver_(context->GetNameResolver()) {
  SetMessageVariables(descriptor, messageBitIndex, 0,
                      context->GetFieldGeneratorInfo(descriptor), context,
                      &variables_);
}

ImmutableMapFieldLiteGenerator::~ImmutableMapFieldLiteGenerator() {}

int ImmutableMapFieldLiteGenerator::GetNumBitsForMessage() const { return 0; }

void ImmutableMapFieldLiteGenerator::GenerateInterfaceMembers(
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
                     "$deprecation$\n"
                     "$value_type_pass_through_nullness$ "
                     "${$get$capitalized_name$ValueOrDefault$}$(\n"
                     "    $key_type$ key,\n"
                     "    $value_type_pass_through_nullness$ defaultValue);\n");
      printer->Annotate("{", "}", descriptor_);
      WriteFieldDocComment(printer, descriptor_, context_->options());
      printer->Print(variables_,
                     "$deprecation$\n"
                     "$value_type$ ${$get$capitalized_name$ValueOrThrow$}$(\n"
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
                   "$deprecation$\n"
                   "$value_type_pass_through_nullness$ "
                   "${$get$capitalized_name$OrDefault$}$(\n"
                   "    $key_type$ key,\n"
                   "    $value_type_pass_through_nullness$ defaultValue);\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(variables_,
                   "$deprecation$\n"
                   "$value_type$ ${$get$capitalized_name$OrThrow$}$(\n"
                   "    $key_type$ key);\n");
    printer->Annotate("{", "}", descriptor_);
  }
}

void ImmutableMapFieldLiteGenerator::GenerateMembers(
    io::Printer* printer) const {
  printer->Print(
      variables_,
      "private static final class $capitalized_name$DefaultEntryHolder {\n"
      "  static final com.google.protobuf.MapEntryLite<\n"
      "      $type_parameters$> defaultEntry =\n"
      "          com.google.protobuf.MapEntryLite\n"
      "          .<$type_parameters$>newDefaultInstance(\n"
      "              $key_wire_type$,\n"
      "              $key_default_value$,\n"
      "              $value_wire_type$,\n"
      "              $value_default_value$);\n"
      "}\n");
  printer->Print(variables_,
                 "private com.google.protobuf.MapFieldLite<\n"
                 "    $type_parameters$> $name$_ =\n"
                 "        com.google.protobuf.MapFieldLite.emptyMapField();\n"
                 "private com.google.protobuf.MapFieldLite<$type_parameters$>\n"
                 "internalGet$capitalized_name$() {\n"
                 "  return $name$_;\n"
                 "}\n"
                 "private com.google.protobuf.MapFieldLite<$type_parameters$>\n"
                 "internalGetMutable$capitalized_name$() {\n"
                 "  if (!$name$_.isMutable()) {\n"
                 "    $name$_ = $name$_.mutableCopy();\n"
                 "  }\n"
                 "  return $name$_;\n"
                 "}\n");
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$\n"
                 "public int ${$get$capitalized_name$Count$}$() {\n"
                 "  return internalGet$capitalized_name$().size();\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$\n"
                 "public boolean ${$contains$capitalized_name$$}$(\n"
                 "    $key_type$ key) {\n"
                 "  $key_null_check$\n"
                 "  return internalGet$capitalized_name$().containsKey(key);\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);

  const FieldDescriptor* value = MapValueField(descriptor_);
  if (GetJavaType(value) == JAVATYPE_ENUM) {
    printer->Print(
        variables_,
        "private static final\n"
        "com.google.protobuf.Internal.MapAdapter.Converter<\n"
        "    java.lang.Integer, $value_enum_type$> $name$ValueConverter =\n"
        "        com.google.protobuf.Internal.MapAdapter.newEnumConverter(\n"
        "            $value_enum_type$.internalGetValueMap(),\n"
        "            $unrecognized_value$);\n");
    if (context_->options().opensource_runtime) {
      printer->Print(
          variables_,
          "/**\n"
          " * Use {@link #get$capitalized_name$Map()} instead.\n"
          " */\n"
          "@java.lang.Deprecated\n"
          "public java.util.Map<$boxed_key_type$, $value_enum_type$>\n"
          "${$get$capitalized_name$$}$() {\n"
          "  return get$capitalized_name$Map();\n"
          "}\n");
      printer->Annotate("{", "}", descriptor_);
    }
    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$\n"
        "public java.util.Map<$boxed_key_type$, $value_enum_type$>\n"
        "${$get$capitalized_name$Map$}$() {\n"
        "  return java.util.Collections.unmodifiableMap(\n"
        "      new com.google.protobuf.Internal.MapAdapter<\n"
        "        $boxed_key_type$, $value_enum_type$, java.lang.Integer>(\n"
        "            internalGet$capitalized_name$(),\n"
        "            $name$ValueConverter));\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$\n"
        "public $value_enum_type_pass_through_nullness$ "
        "${$get$capitalized_name$OrDefault$}$(\n"
        "    $key_type$ key,\n"
        "    $value_enum_type_pass_through_nullness$ defaultValue) {\n"
        "  $key_null_check$\n"
        "  java.util.Map<$boxed_key_type$, $boxed_value_type$> map =\n"
        "      internalGet$capitalized_name$();\n"
        "  return map.containsKey(key)\n"
        "         ? $name$ValueConverter.doForward(map.get(key))\n"
        "         : defaultValue;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$\n"
        "public $value_enum_type$ ${$get$capitalized_name$OrThrow$}$(\n"
        "    $key_type$ key) {\n"
        "  $key_null_check$\n"
        "  java.util.Map<$boxed_key_type$, $boxed_value_type$> map =\n"
        "      internalGet$capitalized_name$();\n"
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
      printer->Print(
          variables_,
          "@java.lang.Override\n"
          "$deprecation$\n"
          "public java.util.Map<$boxed_key_type$, $boxed_value_type$>\n"
          "${$get$capitalized_name$ValueMap$}$() {\n"
          "  return java.util.Collections.unmodifiableMap(\n"
          "      internalGet$capitalized_name$());\n"
          "}\n");
      printer->Annotate("{", "}", descriptor_);
      WriteFieldDocComment(printer, descriptor_, context_->options());
      printer->Print(
          variables_,
          "@java.lang.Override\n"
          "$deprecation$\n"
          "public $value_type_pass_through_nullness$ "
          "${$get$capitalized_name$ValueOrDefault$}$(\n"
          "    $key_type$ key,\n"
          "    $value_type_pass_through_nullness$ defaultValue) {\n"
          "  $key_null_check$\n"
          "  java.util.Map<$boxed_key_type$, $boxed_value_type$> map =\n"
          "      internalGet$capitalized_name$();\n"
          "  return map.containsKey(key) ? map.get(key) : defaultValue;\n"
          "}\n");
      printer->Annotate("{", "}", descriptor_);
      WriteFieldDocComment(printer, descriptor_, context_->options());
      printer->Print(
          variables_,
          "@java.lang.Override\n"
          "$deprecation$\n"
          "public $value_type$ ${$get$capitalized_name$ValueOrThrow$}$(\n"
          "    $key_type$ key) {\n"
          "  $key_null_check$\n"
          "  java.util.Map<$boxed_key_type$, $boxed_value_type$> map =\n"
          "      internalGet$capitalized_name$();\n"
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
                   "$deprecation$\n"
                   "public java.util.Map<$type_parameters$> "
                   "${$get$capitalized_name$Map$}$() {\n"
                   "  return java.util.Collections.unmodifiableMap(\n"
                   "      internalGet$capitalized_name$());\n"
                   "}\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$\n"
        "public $value_type_pass_through_nullness$ "
        "${$get$capitalized_name$OrDefault$}$(\n"
        "    $key_type$ key,\n"
        "    $value_type_pass_through_nullness$ defaultValue) {\n"
        "  $key_null_check$\n"
        "  java.util.Map<$type_parameters$> map =\n"
        "      internalGet$capitalized_name$();\n"
        "  return map.containsKey(key) ? map.get(key) : defaultValue;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(variables_,
                   "@java.lang.Override\n"
                   "$deprecation$\n"
                   "public $value_type$ ${$get$capitalized_name$OrThrow$}$(\n"
                   "    $key_type$ key) {\n"
                   "  $key_null_check$\n"
                   "  java.util.Map<$type_parameters$> map =\n"
                   "      internalGet$capitalized_name$();\n"
                   "  if (!map.containsKey(key)) {\n"
                   "    throw new java.lang.IllegalArgumentException();\n"
                   "  }\n"
                   "  return map.get(key);\n"
                   "}\n");
    printer->Annotate("{", "}", descriptor_);
  }

  // Generate private setters for the builder to proxy into.
  if (GetJavaType(value) == JAVATYPE_ENUM) {
    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(
        variables_,
        "private java.util.Map<$boxed_key_type$, $value_enum_type$>\n"
        "getMutable$capitalized_name$Map() {\n"
        "  return new com.google.protobuf.Internal.MapAdapter<\n"
        "      $boxed_key_type$, $value_enum_type$, java.lang.Integer>(\n"
        "          internalGetMutable$capitalized_name$(),\n"
        "          $name$ValueConverter);\n"
        "}\n");
    if (SupportUnknownEnumValue(value)) {
      WriteFieldDocComment(printer, descriptor_, context_->options());
      printer->Print(
          variables_,
          "private java.util.Map<$boxed_key_type$, $boxed_value_type$>\n"
          "getMutable$capitalized_name$ValueMap() {\n"
          "  return internalGetMutable$capitalized_name$();\n"
          "}\n");
    }
  } else {
    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(variables_,
                   "private java.util.Map<$type_parameters$>\n"
                   "getMutable$capitalized_name$Map() {\n"
                   "  return internalGetMutable$capitalized_name$();\n"
                   "}\n");
  }
}

void ImmutableMapFieldLiteGenerator::GenerateFieldInfo(
    io::Printer* printer, std::vector<uint16_t>* output) const {
  WriteIntToUtf16CharSequence(descriptor_->number(), output);
  WriteIntToUtf16CharSequence(GetExperimentalJavaFieldType(descriptor_),
                              output);
  printer->Print(variables_,
                 "\"$name$_\",\n"
                 "$default_entry$,\n");
  const FieldDescriptor* value = MapValueField(descriptor_);
  if (!SupportUnknownEnumValue(value) && GetJavaType(value) == JAVATYPE_ENUM) {
    PrintEnumVerifierLogic(printer, MapValueField(descriptor_), variables_,
                           /*var_name=*/"$value_enum_type$",
                           /*terminating_string=*/",\n",
                           /*enforce_lite=*/context_->EnforceLite());
  }
}

void ImmutableMapFieldLiteGenerator::GenerateBuilderMembers(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$\n"
                 "public int ${$get$capitalized_name$Count$}$() {\n"
                 "  return instance.get$capitalized_name$Map().size();\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$\n"
      "public boolean ${$contains$capitalized_name$$}$(\n"
      "    $key_type$ key) {\n"
      "  $key_null_check$\n"
      "  return instance.get$capitalized_name$Map().containsKey(key);\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
  printer->Print(variables_,
                 "$deprecation$\n"
                 "public Builder ${$clear$capitalized_name$$}$() {\n"
                 "  copyOnWrite();\n"
                 "  instance.getMutable$capitalized_name$Map().clear();\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
  WriteFieldDocComment(printer, descriptor_, context_->options());
  printer->Print(variables_,
                 "$deprecation$\n"
                 "public Builder ${$remove$capitalized_name$$}$(\n"
                 "    $key_type$ key) {\n"
                 "  $key_null_check$\n"
                 "  copyOnWrite();\n"
                 "  instance.getMutable$capitalized_name$Map().remove(key);\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
  const FieldDescriptor* value = MapValueField(descriptor_);
  if (GetJavaType(value) == JAVATYPE_ENUM) {
    if (context_->options().opensource_runtime) {
      printer->Print(
          variables_,
          "/**\n"
          " * Use {@link #get$capitalized_name$Map()} instead.\n"
          " */\n"
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
                   "$deprecation$\n"
                   "public java.util.Map<$boxed_key_type$, $value_enum_type$>\n"
                   "${$get$capitalized_name$Map$}$() {\n"
                   "  return java.util.Collections.unmodifiableMap(\n"
                   "      instance.get$capitalized_name$Map());\n"
                   "}\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$\n"
        "public $value_enum_type_pass_through_nullness$ "
        "${$get$capitalized_name$OrDefault$}$(\n"
        "    $key_type$ key,\n"
        "    $value_enum_type_pass_through_nullness$ defaultValue) {\n"
        "  $key_null_check$\n"
        "  java.util.Map<$boxed_key_type$, $value_enum_type$> map =\n"
        "      instance.get$capitalized_name$Map();\n"
        "  return map.containsKey(key)\n"
        "         ? map.get(key)\n"
        "         : defaultValue;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$\n"
        "public $value_enum_type$ ${$get$capitalized_name$OrThrow$}$(\n"
        "    $key_type$ key) {\n"
        "  $key_null_check$\n"
        "  java.util.Map<$boxed_key_type$, $value_enum_type$> map =\n"
        "      instance.get$capitalized_name$Map();\n"
        "  if (!map.containsKey(key)) {\n"
        "    throw new java.lang.IllegalArgumentException();\n"
        "  }\n"
        "  return map.get(key);\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(
        variables_,
        "$deprecation$public Builder ${$put$capitalized_name$$}$(\n"
        "    $key_type$ key,\n"
        "    $value_enum_type$ value) {\n"
        "  $key_null_check$\n"
        "  $value_null_check$\n"
        "  copyOnWrite();\n"
        "  instance.getMutable$capitalized_name$Map().put(key, value);\n"
        "  return this;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(
        variables_,
        "$deprecation$public Builder ${$putAll$capitalized_name$$}$(\n"
        "    java.util.Map<$boxed_key_type$, $value_enum_type$> values) {\n"
        "  copyOnWrite();\n"
        "  instance.getMutable$capitalized_name$Map().putAll(values);\n"
        "  return this;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_, Semantic::kSet);
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
      printer->Print(
          variables_,
          "@java.lang.Override\n"
          "$deprecation$\n"
          "public java.util.Map<$boxed_key_type$, $boxed_value_type$>\n"
          "${$get$capitalized_name$ValueMap$}$() {\n"
          "  return java.util.Collections.unmodifiableMap(\n"
          "      instance.get$capitalized_name$ValueMap());\n"
          "}\n");
      printer->Annotate("{", "}", descriptor_);
      WriteFieldDocComment(printer, descriptor_, context_->options());
      printer->Print(
          variables_,
          "@java.lang.Override\n"
          "$deprecation$\n"
          "public $value_type_pass_through_nullness$ "
          "${$get$capitalized_name$ValueOrDefault$}$(\n"
          "    $key_type$ key,\n"
          "    $value_type_pass_through_nullness$ defaultValue) {\n"
          "  $key_null_check$\n"
          "  java.util.Map<$boxed_key_type$, $boxed_value_type$> map =\n"
          "      instance.get$capitalized_name$ValueMap();\n"
          "  return map.containsKey(key) ? map.get(key) : defaultValue;\n"
          "}\n");
      printer->Annotate("{", "}", descriptor_);
      WriteFieldDocComment(printer, descriptor_, context_->options());
      printer->Print(
          variables_,
          "@java.lang.Override\n"
          "$deprecation$\n"
          "public $value_type$ ${$get$capitalized_name$ValueOrThrow$}$(\n"
          "    $key_type$ key) {\n"
          "  $key_null_check$\n"
          "  java.util.Map<$boxed_key_type$, $boxed_value_type$> map =\n"
          "      instance.get$capitalized_name$ValueMap();\n"
          "  if (!map.containsKey(key)) {\n"
          "    throw new java.lang.IllegalArgumentException();\n"
          "  }\n"
          "  return map.get(key);\n"
          "}\n");
      printer->Annotate("{", "}", descriptor_);
      WriteFieldDocComment(printer, descriptor_, context_->options());
      printer->Print(
          variables_,
          "$deprecation$public Builder ${$put$capitalized_name$Value$}$(\n"
          "    $key_type$ key,\n"
          "    $value_type$ value) {\n"
          "  $key_null_check$\n"
          "  copyOnWrite();\n"
          "  instance.getMutable$capitalized_name$ValueMap().put(key, value);\n"
          "  return this;\n"
          "}\n");
      printer->Annotate("{", "}", descriptor_);
      WriteFieldDocComment(printer, descriptor_, context_->options());
      printer->Print(
          variables_,
          "$deprecation$public Builder ${$putAll$capitalized_name$Value$}$(\n"
          "    java.util.Map<$boxed_key_type$, $boxed_value_type$> values) {\n"
          "  copyOnWrite();\n"
          "  instance.getMutable$capitalized_name$ValueMap().putAll(values);\n"
          "  return this;\n"
          "}\n");
      printer->Annotate("{", "}", descriptor_, Semantic::kSet);
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
                   "$deprecation$"
                   "public java.util.Map<$type_parameters$> "
                   "${$get$capitalized_name$Map$}$() {\n"
                   "  return java.util.Collections.unmodifiableMap(\n"
                   "      instance.get$capitalized_name$Map());\n"
                   "}\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$\n"
        "public $value_type_pass_through_nullness$ "
        "${$get$capitalized_name$OrDefault$}$(\n"
        "    $key_type$ key,\n"
        "    $value_type_pass_through_nullness$ defaultValue) {\n"
        "  $key_null_check$\n"
        "  java.util.Map<$type_parameters$> map =\n"
        "      instance.get$capitalized_name$Map();\n"
        "  return map.containsKey(key) ? map.get(key) : defaultValue;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(variables_,
                   "@java.lang.Override\n"
                   "$deprecation$\n"
                   "public $value_type$ ${$get$capitalized_name$OrThrow$}$(\n"
                   "    $key_type$ key) {\n"
                   "  $key_null_check$\n"
                   "  java.util.Map<$type_parameters$> map =\n"
                   "      instance.get$capitalized_name$Map();\n"
                   "  if (!map.containsKey(key)) {\n"
                   "    throw new java.lang.IllegalArgumentException();\n"
                   "  }\n"
                   "  return map.get(key);\n"
                   "}\n");
    printer->Annotate("{", "}", descriptor_);
    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(
        variables_,
        "$deprecation$"
        "public Builder ${$put$capitalized_name$$}$(\n"
        "    $key_type$ key,\n"
        "    $value_type$ value) {\n"
        "  $key_null_check$\n"
        "  $value_null_check$\n"
        "  copyOnWrite();\n"
        "  instance.getMutable$capitalized_name$Map().put(key, value);\n"
        "  return this;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_, Semantic::kSet);
    WriteFieldDocComment(printer, descriptor_, context_->options());
    printer->Print(
        variables_,
        "$deprecation$"
        "public Builder ${$putAll$capitalized_name$$}$(\n"
        "    java.util.Map<$type_parameters$> values) {\n"
        "  copyOnWrite();\n"
        "  instance.getMutable$capitalized_name$Map().putAll(values);\n"
        "  return this;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_, Semantic::kSet);
  }
}

void ImmutableMapFieldLiteGenerator::GenerateKotlinDslMembers(
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
      "  .putAll(map: kotlin.collections.Map<$kt_key_type$, $kt_value_type$>) "
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

void ImmutableMapFieldLiteGenerator::GenerateInitializationCode(
    io::Printer* printer) const {
  // Nothing to initialize.
}

std::string ImmutableMapFieldLiteGenerator::GetBoxedType() const {
  return name_resolver_->GetImmutableClassName(descriptor_->message_type());
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
