// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/java/extension.h"

#include "absl/container/flat_hash_map.h"
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

ImmutableExtensionGenerator::ImmutableExtensionGenerator(
    const FieldDescriptor* descriptor, Context* context)
    : descriptor_(descriptor),
      name_resolver_(context->GetNameResolver()),
      context_(context) {
  if (descriptor_->extension_scope() != NULL) {
    scope_ =
        name_resolver_->GetImmutableClassName(descriptor_->extension_scope());
  } else {
    scope_ = name_resolver_->GetImmutableClassName(descriptor_->file());
  }
}

ImmutableExtensionGenerator::~ImmutableExtensionGenerator() {}

// Initializes the vars referenced in the generated code templates.
void ExtensionGenerator::InitTemplateVars(
    const FieldDescriptor* descriptor, const std::string& scope, bool immutable,
    ClassNameResolver* name_resolver,
    absl::flat_hash_map<absl::string_view, std::string>* vars_pointer,
    Context* context) {
  absl::flat_hash_map<absl::string_view, std::string>& vars = *vars_pointer;
  vars["scope"] = scope;
  vars["name"] = UnderscoresToCamelCaseCheckReserved(descriptor);
  vars["containing_type"] =
      name_resolver->GetClassName(descriptor->containing_type(), immutable);
  vars["number"] = absl::StrCat(descriptor->number());
  vars["constant_name"] = FieldConstantName(descriptor);
  vars["index"] = absl::StrCat(descriptor->index());
  vars["default"] = descriptor->is_repeated()
                        ? ""
                        : DefaultValue(descriptor, immutable, name_resolver,
                                       context->options());
  vars["type_constant"] = std::string(FieldTypeName(GetType(descriptor)));
  vars["packed"] = descriptor->is_packed() ? "true" : "false";
  vars["enum_map"] = "null";
  vars["prototype"] = "null";

  JavaType java_type = GetJavaType(descriptor);
  std::string singular_type;
  switch (java_type) {
    case JAVATYPE_MESSAGE:
      singular_type =
          name_resolver->GetClassName(descriptor->message_type(), immutable);
      vars["prototype"] = absl::StrCat(singular_type, ".getDefaultInstance()");
      break;
    case JAVATYPE_ENUM:
      singular_type =
          name_resolver->GetClassName(descriptor->enum_type(), immutable);
      vars["enum_map"] = absl::StrCat(singular_type, ".internalGetValueMap()");
      break;
    case JAVATYPE_STRING:
      singular_type = "java.lang.String";
      break;
    case JAVATYPE_BYTES:
      singular_type = immutable ? "com.google.protobuf.ByteString" : "byte[]";
      break;
    default:
      singular_type = std::string(BoxedPrimitiveTypeName(java_type));
      break;
  }
  vars["type"] = descriptor->is_repeated()
                     ? absl::StrCat("java.util.List<", singular_type, ">")
                     : singular_type;
  vars["singular_type"] = singular_type;
}

void ImmutableExtensionGenerator::Generate(io::Printer* printer) {
  absl::flat_hash_map<absl::string_view, std::string> vars;
  const bool kUseImmutableNames = true;
  InitTemplateVars(descriptor_, scope_, kUseImmutableNames, name_resolver_,
                   &vars, context_);
  printer->Print(vars, "public static final int $constant_name$ = $number$;\n");

  WriteFieldDocComment(printer, descriptor_, context_->options());
  if (descriptor_->extension_scope() == NULL) {
    // Non-nested
    printer->Print(
        vars,
        "public static final\n"
        "  com.google.protobuf.GeneratedMessage.GeneratedExtension<\n"
        "    $containing_type$,\n"
        "    $type$> $name$ = com.google.protobuf.GeneratedMessage\n"
        "        .newFileScopedGeneratedExtension(\n"
        "      $singular_type$.class,\n"
        "      $prototype$);\n");
  } else {
    // Nested
    printer->Print(
        vars,
        "public static final\n"
        "  com.google.protobuf.GeneratedMessage.GeneratedExtension<\n"
        "    $containing_type$,\n"
        "    $type$> $name$ = com.google.protobuf.GeneratedMessage\n"
        "        .newMessageScopedGeneratedExtension(\n"
        "      $scope$.getDefaultInstance(),\n"
        "      $index$,\n"
        "      $singular_type$.class,\n"
        "      $prototype$);\n");
  }
  printer->Annotate("name", descriptor_);
}

int ImmutableExtensionGenerator::GenerateNonNestedInitializationCode(
    io::Printer* printer) {
  int bytecode_estimate = 0;
  if (descriptor_->extension_scope() == NULL) {
    // Only applies to non-nested extensions.
    printer->Print(
        "$name$.internalInit(descriptor.getExtensions().get($index$));\n",
        "name", UnderscoresToCamelCaseCheckReserved(descriptor_), "index",
        absl::StrCat(descriptor_->index()));
    bytecode_estimate += 21;
  }
  return bytecode_estimate;
}

int ImmutableExtensionGenerator::GenerateRegistrationCode(
    io::Printer* printer) {
  printer->Print("registry.add($scope$.$name$);\n", "scope", scope_, "name",
                 UnderscoresToCamelCaseCheckReserved(descriptor_));
  return 7;
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
