// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/java/extension_lite.h"

#include <string>

#include "absl/container/flat_hash_map.h"
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

ImmutableExtensionLiteGenerator::ImmutableExtensionLiteGenerator(
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

ImmutableExtensionLiteGenerator::~ImmutableExtensionLiteGenerator() {}

void ImmutableExtensionLiteGenerator::Generate(io::Printer* printer) {
  absl::flat_hash_map<absl::string_view, std::string> vars;
  const bool kUseImmutableNames = true;
  InitTemplateVars(descriptor_, scope_, kUseImmutableNames, name_resolver_,
                   &vars, context_);
  printer->Print(vars, "public static final int $constant_name$ = $number$;\n");

  WriteFieldDocComment(printer, descriptor_, context_->options());
  if (descriptor_->is_repeated()) {
    printer->Print(
        vars,
        "public static final\n"
        "  com.google.protobuf.GeneratedMessageLite.GeneratedExtension<\n"
        "    $containing_type$,\n"
        "    $type$> $name$ = com.google.protobuf.GeneratedMessageLite\n"
        "        .newRepeatedGeneratedExtension(\n"
        "      $containing_type$.getDefaultInstance(),\n"
        "      $prototype$,\n"
        "      $enum_map$,\n"
        "      $number$,\n"
        "      com.google.protobuf.WireFormat.FieldType.$type_constant$,\n"
        "      $packed$,\n"
        "      $singular_type$.class);\n");
  } else {
    printer->Print(
        vars,
        "public static final\n"
        "  com.google.protobuf.GeneratedMessageLite.GeneratedExtension<\n"
        "    $containing_type$,\n"
        "    $type$> $name$ = com.google.protobuf.GeneratedMessageLite\n"
        "        .newSingularGeneratedExtension(\n"
        "      $containing_type$.getDefaultInstance(),\n"
        "      $default$,\n"
        "      $prototype$,\n"
        "      $enum_map$,\n"
        "      $number$,\n"
        "      com.google.protobuf.WireFormat.FieldType.$type_constant$,\n"
        "      $singular_type$.class);\n");
  }
  printer->Annotate("name", descriptor_);
}

int ImmutableExtensionLiteGenerator::GenerateNonNestedInitializationCode(
    io::Printer* printer) {
  return 0;
}

int ImmutableExtensionLiteGenerator::GenerateRegistrationCode(
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
