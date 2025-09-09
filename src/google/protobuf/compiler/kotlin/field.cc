#include "google/protobuf/compiler/kotlin/field.h"

#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/java/context.h"
#include "google/protobuf/compiler/java/doc_comment.h"
#include "google/protobuf/compiler/java/field_common.h"
#include "google/protobuf/compiler/java/helpers.h"
#include "google/protobuf/compiler/java/internal_helpers.h"
#include "google/protobuf/compiler/java/name_resolver.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace kotlin {

FieldGenerator::FieldGenerator(const FieldDescriptor* descriptor,
                               java::Context* context, bool lite)
    : descriptor_(descriptor), context_(context), lite_(lite) {
  java::SetCommonFieldVariables(
      descriptor, context->GetFieldGeneratorInfo(descriptor), &variables_);
  variables_.insert(
      {"kt_deprecation",
       descriptor->options().deprecated()
           ? absl::StrCat("@kotlin.Deprecated(message = \"Field ",
                          variables_["name"], " is deprecated\") ")
           : ""});
}

void FieldGenerator::Generate(io::Printer* printer) const {
  auto cleanup = printer->WithVars(variables_);
  switch (java::GetJavaType(descriptor_)) {
    case java::JAVATYPE_MESSAGE:
      if (descriptor_->is_repeated() &&
          java::IsMapEntry(descriptor_->message_type())) {
        GenerateMapField(printer);
      } else {
        GenerateMessageField(printer);
      }
      break;
    case java::JAVATYPE_STRING:
      GenerateStringField(printer);
      break;
    case java::JAVATYPE_ENUM:
      GenerateEnumField(printer);
      break;
    default:
      GeneratePritimiveField(printer);
      break;
  }
}

void FieldGenerator::GeneratePritimiveField(io::Printer* printer) const {
  java::JavaType javaType = java::GetJavaType(descriptor_);
  auto cleanup = printer->WithVars(
      {{"kt_type", std::string(java::KotlinTypeName(javaType))}});

  if (descriptor_->is_repeated()) {
    GenerateRepeatedPritimiveField(printer);
    return;
  }

  java::JvmNameContext name_ctx = {context_->options(), printer, lite_};
  java::WriteFieldDocComment(printer, descriptor_, context_->options(),
                             /* kdoc */ true);
  if (descriptor_->name() == "is_initialized") {
    printer->Emit(
        {
            {"jvm_name_get",
             [&] { JvmName("${$get$kt_capitalized_name$$}$", name_ctx); }},
            {"jvm_name_set",
             [&] { JvmName("${$set$kt_capitalized_name$$}$", name_ctx); }},
        },
        "// TODO: b/336400327 - remove this hack; we should access properties\n"
        "$kt_deprecation$public var $kt_name$: $kt_type$\n"
        "  $jvm_name_get$"
        "  get() = $kt_dsl_builder$.get${$$kt_capitalized_name$$}$()\n"
        "  $jvm_name_set$"
        "  set(value) {\n"
        "    $kt_dsl_builder$.${$set$kt_capitalized_name$$}$(value)\n"
        "  }\n");
  } else {
    printer->Emit(
        {
            {"jvm_name_get",
             [&] { JvmName("${$get$kt_capitalized_name$$}$", name_ctx); }},
            {"jvm_name_set",
             [&] { JvmName("${$set$kt_capitalized_name$$}$", name_ctx); }},
        },
        "$kt_deprecation$public var $kt_name$: $kt_type$\n"
        "  $jvm_name_get$"
        "  get() = $kt_dsl_builder$.${$$kt_safe_name$$}$\n"
        "  $jvm_name_set$"
        "  set(value) {\n"
        "    $kt_dsl_builder$.${$$kt_safe_name$$}$ = value\n"
        "  }\n");
  }

  WriteFieldAccessorDocComment(printer, descriptor_, java::CLEARER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ true);
  printer->Print(
      "public fun ${$clear$kt_capitalized_name$$}$() {\n"
      "  $kt_dsl_builder$.${$clear$capitalized_name$$}$()\n"
      "}\n");

  if (descriptor_->has_presence()) {
    WriteFieldAccessorDocComment(printer, descriptor_, java::HAZZER,
                                 context_->options(), /* builder */ false,
                                 /* kdoc */ true);
    printer->Print(
        "public fun ${$has$kt_capitalized_name$$}$(): kotlin.Boolean {\n"
        "  return $kt_dsl_builder$.${$has$capitalized_name$$}$()\n"
        "}\n");
  }
}

void FieldGenerator::GenerateRepeatedPritimiveField(
    io::Printer* printer) const {
  java::JvmNameContext name_ctx = {context_->options(), printer, lite_};
  printer->Print(
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
      "$kt_deprecation$ public val $kt_name$: "
      "com.google.protobuf.kotlin.DslList"
      "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>\n"
      "$  jvm_synthetic$"
      "  get() = com.google.protobuf.kotlin.DslList(\n"
      "    $kt_dsl_builder$.${$$kt_property_name$List$}$\n"
      "  )\n");

  WriteFieldAccessorDocComment(printer, descriptor_, java::LIST_ADDER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name", [&] { JvmName("add$kt_capitalized_name$", name_ctx); }},
      },
      "$jvm_synthetic$"
      "$jvm_name$"
      "public fun com.google.protobuf.kotlin.DslList"
      "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
      "add(value: $kt_type$) {\n"
      "  $kt_dsl_builder$.${$add$capitalized_name$$}$(value)\n"
      "}");

  WriteFieldAccessorDocComment(printer, descriptor_, java::LIST_ADDER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name",
           [&] { JvmName("plusAssign$kt_capitalized_name$", name_ctx); }},
      },
      "$jvm_synthetic$"
      "$jvm_name$"
      "@Suppress(\"NOTHING_TO_INLINE\")\n"
      "public inline operator fun com.google.protobuf.kotlin.DslList"
      "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
      "plusAssign(value: $kt_type$) {\n"
      "  add(value)\n"
      "}");

  WriteFieldAccessorDocComment(printer, descriptor_, java::LIST_MULTI_ADDER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name",
           [&] { JvmName("addAll$kt_capitalized_name$", name_ctx); }},
      },
      "$jvm_synthetic$"
      "$jvm_name$"
      "public fun com.google.protobuf.kotlin.DslList"
      "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
      "addAll(values: kotlin.collections.Iterable<$kt_type$>) {\n"
      "  $kt_dsl_builder$.${$addAll$capitalized_name$$}$(values)\n"
      "}");

  WriteFieldAccessorDocComment(printer, descriptor_, java::LIST_MULTI_ADDER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name",
           [&] { JvmName("plusAssignAll$kt_capitalized_name$", name_ctx); }},
      },
      "$jvm_synthetic$"
      "$jvm_name$"
      "@Suppress(\"NOTHING_TO_INLINE\")\n"
      "public inline operator fun com.google.protobuf.kotlin.DslList"
      "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
      "plusAssign(values: kotlin.collections.Iterable<$kt_type$>) {\n"
      "  addAll(values)\n"
      "}");

  WriteFieldAccessorDocComment(printer, descriptor_, java::LIST_INDEXED_SETTER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name", [&] { JvmName("set$kt_capitalized_name$", name_ctx); }},
      },
      "$jvm_synthetic$"
      "$jvm_name$"
      "public operator fun com.google.protobuf.kotlin.DslList"
      "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
      "set(index: kotlin.Int, value: $kt_type$) {\n"
      "  $kt_dsl_builder$.${$set$capitalized_name$$}$(index, value)\n"
      "}");

  WriteFieldAccessorDocComment(printer, descriptor_, java::CLEARER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name",
           [&] { JvmName("clear$kt_capitalized_name$", name_ctx); }},
      },
      "$jvm_synthetic$"
      "$jvm_name$"
      "public fun com.google.protobuf.kotlin.DslList"
      "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
      "clear() {\n"
      "  $kt_dsl_builder$.${$clear$capitalized_name$$}$()\n"
      "}");
}

void FieldGenerator::GenerateMessageField(io::Printer* printer) const {
  auto name_resolver = context_->GetNameResolver();
  auto cleanup = printer->WithVars(
      {{"kt_type",
        java::EscapeKotlinKeywords(name_resolver->GetImmutableClassName(
            descriptor_->message_type()))}});

  if (descriptor_->is_repeated()) {
    GenerateRepeatedMessageField(printer);
    return;
  }

  java::JvmNameContext name_ctx = {context_->options(), printer, lite_};
  WriteFieldDocComment(printer, descriptor_, context_->options(),
                       /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name_get",
           [&] { JvmName("${$get$kt_capitalized_name$$}$", name_ctx); }},
          {"jvm_name_set",
           [&] { JvmName("${$set$kt_capitalized_name$$}$", name_ctx); }},
      },
      "$kt_deprecation$public var $kt_name$: $kt_type$\n"
      "  $jvm_name_get$"
      "  get() = $kt_dsl_builder$.${$$kt_safe_name$$}$\n"
      "  $jvm_name_set$"
      "  set(value) {\n"
      "    $kt_dsl_builder$.${$$kt_safe_name$$}$ = value\n"
      "  }\n");

  WriteFieldAccessorDocComment(printer, descriptor_, java::CLEARER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ true);
  printer->Print(
      "public fun ${$clear$kt_capitalized_name$$}$() {\n"
      "  $kt_dsl_builder$.${$clear$capitalized_name$$}$()\n"
      "}\n");

  WriteFieldAccessorDocComment(printer, descriptor_, java::HAZZER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ true);
  printer->Print(
      "public fun ${$has$kt_capitalized_name$$}$(): kotlin.Boolean {\n"
      "  return $kt_dsl_builder$.${$has$capitalized_name$$}$()\n"
      "}\n");
  if (descriptor_->has_presence() &&
      descriptor_->real_containing_oneof() == nullptr) {
    printer->Print(
        "$kt_deprecation$\n"
        "public val $classname$Kt.Dsl.$name$OrNull: $kt_type$?\n"
        "  get() = $kt_dsl_builder$.$name$OrNull\n");
  }
}

void FieldGenerator::GenerateRepeatedMessageField(io::Printer* printer) const {
  java::JvmNameContext name_ctx = {context_->options(), printer, lite_};
  printer->Print(
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
      "$kt_deprecation$ public val $kt_name$: "
      "com.google.protobuf.kotlin.DslList"
      "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>\n"
      "$  jvm_synthetic$"
      "  get() = com.google.protobuf.kotlin.DslList(\n"
      "    $kt_dsl_builder$.${$$kt_property_name$List$}$\n"
      "  )\n");

  WriteFieldAccessorDocComment(printer, descriptor_, java::LIST_ADDER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name", [&] { JvmName("add$kt_capitalized_name$", name_ctx); }},
      },
      "$jvm_synthetic$"
      "$jvm_name$"
      "public fun com.google.protobuf.kotlin.DslList"
      "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
      "add(value: $kt_type$) {\n"
      "  $kt_dsl_builder$.${$add$capitalized_name$$}$(value)\n"
      "}\n");

  WriteFieldAccessorDocComment(printer, descriptor_, java::LIST_ADDER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name",
           [&] { JvmName("plusAssign$kt_capitalized_name$", name_ctx); }},
      },
      "$jvm_synthetic$"
      "$jvm_name$"
      "@Suppress(\"NOTHING_TO_INLINE\")\n"
      "public inline operator fun com.google.protobuf.kotlin.DslList"
      "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
      "plusAssign(value: $kt_type$) {\n"
      "  add(value)\n"
      "}\n");

  WriteFieldAccessorDocComment(printer, descriptor_, java::LIST_MULTI_ADDER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name",
           [&] { JvmName("addAll$kt_capitalized_name$", name_ctx); }},
      },
      "$jvm_synthetic$"
      "$jvm_name$"
      "public fun com.google.protobuf.kotlin.DslList"
      "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
      "addAll(values: kotlin.collections.Iterable<$kt_type$>) {\n"
      "  $kt_dsl_builder$.${$addAll$capitalized_name$$}$(values)\n"
      "}\n");

  WriteFieldAccessorDocComment(printer, descriptor_, java::LIST_MULTI_ADDER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name",
           [&] { JvmName("plusAssignAll$kt_capitalized_name$", name_ctx); }},
      },
      "$jvm_synthetic$"
      "$jvm_name$"
      "@Suppress(\"NOTHING_TO_INLINE\")\n"
      "public inline operator fun com.google.protobuf.kotlin.DslList"
      "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
      "plusAssign(values: kotlin.collections.Iterable<$kt_type$>) {\n"
      "  addAll(values)\n"
      "}\n");

  WriteFieldAccessorDocComment(printer, descriptor_, java::LIST_INDEXED_SETTER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name", [&] { JvmName("set$kt_capitalized_name$", name_ctx); }},
      },
      "$jvm_synthetic$"
      "$jvm_name$"
      "public operator fun com.google.protobuf.kotlin.DslList"
      "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
      "set(index: kotlin.Int, value: $kt_type$) {\n"
      "  $kt_dsl_builder$.${$set$capitalized_name$$}$(index, value)\n"
      "}\n");

  WriteFieldAccessorDocComment(printer, descriptor_, java::CLEARER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name",
           [&] { JvmName("clear$kt_capitalized_name$", name_ctx); }},
      },

      "$jvm_synthetic$"
      "$jvm_name$"
      "public fun com.google.protobuf.kotlin.DslList"
      "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
      "clear() {\n"
      "  $kt_dsl_builder$.${$clear$capitalized_name$$}$()\n"
      "}\n");
}

void FieldGenerator::GenerateStringField(io::Printer* printer) const {
  if (descriptor_->is_repeated()) {
    GenerateRepeatedStringField(printer);
    return;
  }
  java::JvmNameContext name_ctx = {context_->options(), printer, lite_};
  WriteFieldDocComment(printer, descriptor_, context_->options(),
                       /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name_get",
           [&] { JvmName("${$get$kt_capitalized_name$$}$", name_ctx); }},
          {"jvm_name_set",
           [&] { JvmName("${$set$kt_capitalized_name$$}$", name_ctx); }},
      },
      "$kt_deprecation$public var $kt_name$: kotlin.String\n"
      "  $jvm_name_get$"
      "  get() = $kt_dsl_builder$.${$$kt_safe_name$$}$\n"
      "  $jvm_name_set$"
      "  set(value) {\n"
      "    $kt_dsl_builder$.${$$kt_safe_name$$}$ = value\n"
      "  }\n");

  WriteFieldAccessorDocComment(printer, descriptor_, java::CLEARER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ true);
  printer->Print(
      "public fun ${$clear$kt_capitalized_name$$}$() {\n"
      "  $kt_dsl_builder$.${$clear$capitalized_name$$}$()\n"
      "}\n");

  if (descriptor_->has_presence()) {
    WriteFieldAccessorDocComment(printer, descriptor_, java::HAZZER,
                                 context_->options(), /* builder */ false,
                                 /* kdoc */ true);
    printer->Print(
        "public fun ${$has$kt_capitalized_name$$}$(): kotlin.Boolean {\n"
        "  return $kt_dsl_builder$.${$has$capitalized_name$$}$()\n"
        "}\n");
  }
}

void FieldGenerator::GenerateRepeatedStringField(io::Printer* printer) const {
  java::JvmNameContext name_ctx = {context_->options(), printer, lite_};
  printer->Print(
      "/**\n"
      " * An uninstantiable, behaviorless type to represent the field in\n"
      " * generics.\n"
      " */\n"
      "@kotlin.OptIn"
      "(com.google.protobuf.kotlin.OnlyForUseByGeneratedProtoCode::class)\n"
      "public class ${$$kt_capitalized_name$Proxy$}$ private constructor()"
      " : com.google.protobuf.kotlin.DslProxy()\n");

  // property for List<String>
  WriteFieldAccessorDocComment(printer, descriptor_, java::LIST_GETTER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ true);
  printer->Print(
      "$kt_deprecation$public val $kt_name$: "
      "com.google.protobuf.kotlin.DslList"
      "<kotlin.String, ${$$kt_capitalized_name$Proxy$}$>\n"
      "@kotlin.OptIn"
      "(com.google.protobuf.kotlin.OnlyForUseByGeneratedProtoCode::class)\n"
      "  get() = com.google.protobuf.kotlin.DslList(\n"
      "    $kt_dsl_builder$.${$$kt_property_name$List$}$\n"
      "  )\n");

  // List<String>.add(String)
  WriteFieldAccessorDocComment(printer, descriptor_, java::LIST_ADDER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name", [&] { JvmName("add$kt_capitalized_name$", name_ctx); }},
      },
      "$jvm_synthetic$"
      "$jvm_name$"
      "public fun com.google.protobuf.kotlin.DslList"
      "<kotlin.String, ${$$kt_capitalized_name$Proxy$}$>."
      "add(value: kotlin.String) {\n"
      "  $kt_dsl_builder$.${$add$capitalized_name$$}$(value)\n"
      "}\n");

  // List<String> += String
  WriteFieldAccessorDocComment(printer, descriptor_, java::LIST_ADDER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name",
           [&] { JvmName("plusAssign$kt_capitalized_name$", name_ctx); }},
      },
      "$jvm_synthetic$"
      "$jvm_name$"
      "@Suppress(\"NOTHING_TO_INLINE\")\n"
      "public inline operator fun com.google.protobuf.kotlin.DslList"
      "<kotlin.String, ${$$kt_capitalized_name$Proxy$}$>."
      "plusAssign(value: kotlin.String) {\n"
      "  add(value)\n"
      "}\n");

  // List<String>.addAll(Iterable<String>)
  WriteFieldAccessorDocComment(printer, descriptor_, java::LIST_MULTI_ADDER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name",
           [&] { JvmName("addAll$kt_capitalized_name$", name_ctx); }},
      },
      "$jvm_synthetic$"
      "$jvm_name$"
      "public fun com.google.protobuf.kotlin.DslList"
      "<kotlin.String, ${$$kt_capitalized_name$Proxy$}$>."
      "addAll(values: kotlin.collections.Iterable<kotlin.String>) {\n"
      "  $kt_dsl_builder$.${$addAll$capitalized_name$$}$(values)\n"
      "}\n");

  // List<String> += Iterable<String>
  WriteFieldAccessorDocComment(printer, descriptor_, java::LIST_MULTI_ADDER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name",
           [&] { JvmName("plusAssignAll$kt_capitalized_name$", name_ctx); }},
      },
      "$jvm_synthetic$"
      "$jvm_name$"
      "@Suppress(\"NOTHING_TO_INLINE\")\n"
      "public inline operator fun com.google.protobuf.kotlin.DslList"
      "<kotlin.String, ${$$kt_capitalized_name$Proxy$}$>."
      "plusAssign(values: kotlin.collections.Iterable<kotlin.String>) {\n"
      "  addAll(values)\n"
      "}\n");

  // List<String>[Int] = String
  WriteFieldAccessorDocComment(printer, descriptor_, java::LIST_INDEXED_SETTER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name", [&] { JvmName("set$kt_capitalized_name$", name_ctx); }},
      },
      "$jvm_synthetic$"
      "$jvm_name$"
      "public operator fun com.google.protobuf.kotlin.DslList"
      "<kotlin.String, ${$$kt_capitalized_name$Proxy$}$>."
      "set(index: kotlin.Int, value: kotlin.String) {\n"
      "  $kt_dsl_builder$.${$set$capitalized_name$$}$(index, value)\n"
      "}");

  WriteFieldAccessorDocComment(printer, descriptor_, java::CLEARER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name", [&] { JvmName("set$kt_capitalized_name$", name_ctx); }},
      },
      "$jvm_synthetic$"
      "$jvm_name$"
      "public fun com.google.protobuf.kotlin.DslList"
      "<kotlin.String, ${$$kt_capitalized_name$Proxy$}$>."
      "clear() {\n"
      "  $kt_dsl_builder$.${$clear$capitalized_name$$}$()\n"
      "}");
}

void FieldGenerator::GenerateEnumField(io::Printer* printer) const {
  auto name_resolver = context_->GetNameResolver();
  auto cleanup = printer->WithVars(
      {{"kt_type",
        java::EscapeKotlinKeywords(
            name_resolver->GetImmutableClassName(descriptor_->enum_type()))}});

  if (descriptor_->is_repeated()) {
    GenerateRepeatedEnumField(printer);
    return;
  }

  java::JvmNameContext name_ctx = {context_->options(), printer, lite_};
  WriteFieldDocComment(printer, descriptor_, context_->options(),
                       /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name_get",
           [&] { JvmName("${$get$kt_capitalized_name$$}$", name_ctx); }},
          {"jvm_name_set",
           [&] { JvmName("${$set$kt_capitalized_name$$}$", name_ctx); }},
      },
      "$kt_deprecation$public var $kt_name$: $kt_type$\n"
      "  $jvm_name_get$"
      "  get() = $kt_dsl_builder$.${$$kt_safe_name$$}$\n"
      "  $jvm_name_set$"
      "  set(value) {\n"
      "    $kt_dsl_builder$.${$$kt_safe_name$$}$ = value\n"
      "  }\n");

  if (java::SupportUnknownEnumValue(descriptor_)) {
    printer->Emit(
        {
            {"jvm_name_get",
             [&] { JvmName("${$get$kt_capitalized_name$Value$}$", name_ctx); }},
            {"jvm_name_set",
             [&] { JvmName("${$set$kt_capitalized_name$Value$}$", name_ctx); }},
        },
        "$kt_deprecation$public var $kt_name$Value: kotlin.Int\n"
        "  $jvm_name_get$"
        "  get() = $kt_dsl_builder$.${$$kt_property_name$Value$}$\n"
        "  $jvm_name_set$"
        "  set(value) {\n"
        "    $kt_dsl_builder$.${$$kt_property_name$Value$}$ = value\n"
        "  }\n");
  }

  WriteFieldAccessorDocComment(printer, descriptor_, java::CLEARER,
                               context_->options(),
                               /* builder */ false, /* kdoc */ true);
  printer->Print(
      "public fun ${$clear$kt_capitalized_name$$}$() {\n"
      "  $kt_dsl_builder$.${$clear$capitalized_name$$}$()\n"
      "}\n");

  if (descriptor_->has_presence()) {
    WriteFieldAccessorDocComment(printer, descriptor_, java::HAZZER,
                                 context_->options(),
                                 /* builder */ false, /* kdoc */ true);
    printer->Print(
        "public fun ${$has$kt_capitalized_name$$}$(): kotlin.Boolean {\n"
        "  return $kt_dsl_builder$.${$has$capitalized_name$$}$()\n"
        "}\n");
  }
}

void FieldGenerator::GenerateRepeatedEnumField(io::Printer* printer) const {
  java::JvmNameContext name_ctx = {context_->options(), printer, lite_};
  printer->Print(
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
      "$kt_deprecation$ public val $kt_name$: "
      "com.google.protobuf.kotlin.DslList"
      "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>\n"
      "$  jvm_synthetic$"
      "  get() = com.google.protobuf.kotlin.DslList(\n"
      "    $kt_dsl_builder$.${$$kt_property_name$List$}$\n"
      "  )\n");

  WriteFieldAccessorDocComment(printer, descriptor_, java::LIST_ADDER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name", [&] { JvmName("add$kt_capitalized_name$", name_ctx); }},
      },
      "$jvm_synthetic$"
      "$jvm_name$"
      "public fun com.google.protobuf.kotlin.DslList"
      "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
      "add(value: $kt_type$) {\n"
      "  $kt_dsl_builder$.${$add$capitalized_name$$}$(value)\n"
      "}");

  WriteFieldAccessorDocComment(printer, descriptor_, java::LIST_ADDER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name",
           [&] { JvmName("plusAssign$kt_capitalized_name$", name_ctx); }},
      },
      "$jvm_synthetic$"
      "$jvm_name$"
      "@Suppress(\"NOTHING_TO_INLINE\")\n"
      "public inline operator fun com.google.protobuf.kotlin.DslList"
      "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
      "plusAssign(value: $kt_type$) {\n"
      "  add(value)\n"
      "}");

  WriteFieldAccessorDocComment(printer, descriptor_, java::LIST_MULTI_ADDER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name",
           [&] { JvmName("addAll$kt_capitalized_name$", name_ctx); }},
      },
      "$jvm_synthetic$"
      "$jvm_name$"
      "public fun com.google.protobuf.kotlin.DslList"
      "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
      "addAll(values: kotlin.collections.Iterable<$kt_type$>) {\n"
      "  $kt_dsl_builder$.${$addAll$capitalized_name$$}$(values)\n"
      "}");

  WriteFieldAccessorDocComment(printer, descriptor_, java::LIST_MULTI_ADDER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name",
           [&] { JvmName("plusAssignAll$kt_capitalized_name$", name_ctx); }},
      },
      "$jvm_synthetic$"
      "$jvm_name$"
      "@Suppress(\"NOTHING_TO_INLINE\")\n"
      "public inline operator fun com.google.protobuf.kotlin.DslList"
      "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
      "plusAssign(values: kotlin.collections.Iterable<$kt_type$>) {\n"
      "  addAll(values)\n"
      "}");

  WriteFieldAccessorDocComment(printer, descriptor_, java::LIST_INDEXED_SETTER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name", [&] { JvmName("set$kt_capitalized_name$", name_ctx); }},
      },
      "$jvm_synthetic$"
      "$jvm_name$"
      "public operator fun com.google.protobuf.kotlin.DslList"
      "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
      "set(index: kotlin.Int, value: $kt_type$) {\n"
      "  $kt_dsl_builder$.${$set$capitalized_name$$}$(index, value)\n"
      "}");

  WriteFieldAccessorDocComment(printer, descriptor_, java::CLEARER,
                               context_->options(), /* builder */ false,
                               /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name",
           [&] { JvmName("clear$kt_capitalized_name$", name_ctx); }},
      },
      "$jvm_synthetic$"
      "$jvm_name$"
      "public fun com.google.protobuf.kotlin.DslList"
      "<$kt_type$, ${$$kt_capitalized_name$Proxy$}$>."
      "clear() {\n"
      "  $kt_dsl_builder$.${$clear$capitalized_name$$}$()\n"
      "}");
}

namespace {
std::string KotlinTypeName(const FieldDescriptor* field,
                           java::ClassNameResolver* name_resolver) {
  if (java::GetJavaType(field) == java::JAVATYPE_MESSAGE) {
    return name_resolver->GetImmutableClassName(field->message_type());
  } else if (java::GetJavaType(field) == java::JAVATYPE_ENUM) {
    return name_resolver->GetImmutableClassName(field->enum_type());
  } else {
    return std::string(java::KotlinTypeName(java::GetJavaType(field)));
  }
}
}  // namespace

void FieldGenerator::GenerateMapField(io::Printer* printer) const {
  auto name_resolver = context_->GetNameResolver();
  const FieldDescriptor* key = java::MapKeyField(descriptor_);
  const FieldDescriptor* value = java::MapValueField(descriptor_);
  auto cleanup = printer->WithVars(
      {{"kt_key_type", KotlinTypeName(key, name_resolver)},
       {"kt_value_type", KotlinTypeName(value, name_resolver)}});

  java::JvmNameContext name_ctx = {context_->options(), printer, lite_};
  printer->Print(
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
  printer->Emit(
      {
          {"jvm_name",
           [&] { JvmName("get$kt_capitalized_name$Map", name_ctx); }},
      },
      "$kt_deprecation$ public val $kt_name$: "
      "com.google.protobuf.kotlin.DslMap"
      "<$kt_key_type$, $kt_value_type$, ${$$kt_capitalized_name$Proxy$}$>\n"
      "$  jvm_synthetic$"
      "$jvm_name$"
      "  get() = com.google.protobuf.kotlin.DslMap(\n"
      "    $kt_dsl_builder$.${$$kt_property_name$Map$}$\n"
      "  )\n");

  WriteFieldDocComment(printer, descriptor_, context_->options(),
                       /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name", [&] { JvmName("put$kt_capitalized_name$", name_ctx); }},
      },
      "$jvm_name$"
      "public fun com.google.protobuf.kotlin.DslMap"
      "<$kt_key_type$, $kt_value_type$, ${$$kt_capitalized_name$Proxy$}$>\n"
      "  .put(key: $kt_key_type$, value: $kt_value_type$) {\n"
      "     $kt_dsl_builder$.${$put$capitalized_name$$}$(key, value)\n"
      "   }\n");

  WriteFieldDocComment(printer, descriptor_, context_->options(),
                       /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name", [&] { JvmName("set$kt_capitalized_name$", name_ctx); }},
      },
      "$jvm_synthetic$"
      "$jvm_name$"
      "@Suppress(\"NOTHING_TO_INLINE\")\n"
      "public inline operator fun com.google.protobuf.kotlin.DslMap"
      "<$kt_key_type$, $kt_value_type$, ${$$kt_capitalized_name$Proxy$}$>\n"
      "  .set(key: $kt_key_type$, value: $kt_value_type$) {\n"
      "     put(key, value)\n"
      "   }\n");

  WriteFieldDocComment(printer, descriptor_, context_->options(),
                       /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name",
           [&] { JvmName("remove$kt_capitalized_name$", name_ctx); }},
      },
      "$jvm_synthetic$"
      "$jvm_name$"
      "public fun com.google.protobuf.kotlin.DslMap"
      "<$kt_key_type$, $kt_value_type$, ${$$kt_capitalized_name$Proxy$}$>\n"
      "  .remove(key: $kt_key_type$) {\n"
      "     $kt_dsl_builder$.${$remove$capitalized_name$$}$(key)\n"
      "   }\n");

  WriteFieldDocComment(printer, descriptor_, context_->options(),
                       /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name",
           [&] { JvmName("putAll$kt_capitalized_name$", name_ctx); }},
      },
      "$jvm_synthetic$"
      "$jvm_name$"
      "public fun com.google.protobuf.kotlin.DslMap"
      "<$kt_key_type$, $kt_value_type$, ${$$kt_capitalized_name$Proxy$}$>\n"
      "  .putAll(map: kotlin.collections.Map<$kt_key_type$, $kt_value_type$>) "
      "{\n"
      "     $kt_dsl_builder$.${$putAll$capitalized_name$$}$(map)\n"
      "   }\n");

  WriteFieldDocComment(printer, descriptor_, context_->options(),
                       /* kdoc */ true);
  printer->Emit(
      {
          {"jvm_name",
           [&] { JvmName("clear$kt_capitalized_name$", name_ctx); }},
      },
      "$jvm_synthetic$"
      "$jvm_name$"
      "public fun com.google.protobuf.kotlin.DslMap"
      "<$kt_key_type$, $kt_value_type$, ${$$kt_capitalized_name$Proxy$}$>\n"
      "  .clear() {\n"
      "     $kt_dsl_builder$.${$clear$capitalized_name$$}$()\n"
      "   }\n");
}

}  // namespace kotlin
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
