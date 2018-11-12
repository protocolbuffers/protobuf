// Protocol Buffers - Google's data interchange format
// Copyright 2008 Oleksii Pylypenko.  All rights reserved.
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
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <google/protobuf/compiler/kotlin/kotlin_message.h>

#include <algorithm>
#include <map>
#include <memory>
#include <vector>

#include <google/protobuf/compiler/java/java_context.h>
#include <google/protobuf/compiler/java/java_name_resolver.h>
#include <google/protobuf/compiler/java/java_field.h>
#include <google/protobuf/io/printer.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace kotlin {

// ===================================================================

MessageGenerator::MessageGenerator(const Descriptor* descriptor,
                                   java::Context* context,
                                   bool immutable_api)
  : descriptor_(descriptor),
    context_(context),
    name_resolver_(context->GetNameResolver()),
    immutable_api_(immutable_api) {
}

MessageGenerator::~MessageGenerator() {}

void MessageGenerator::GenerateBuildFunction(io::Printer* printer) {
  printer->Print(
      "fun build$name$(\n"
      "  block: $full_name$.Builder.() -> Unit\n"
      ") = $full_name$.newBuilder()\n"
      "  .apply(block)\n"
      "  .build()\n\n",
      "name", descriptor_->name(),
      "full_name", name_resolver_->GetClassName(descriptor_, immutable_api_));

  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    // TODO(oleksiyp): don't know if it is needed
    // Don't generate Java classes for map entry messages.
    //if (IsMapEntry(descriptor_->nested_type(i))) continue;
    printer->Print(
        "object $classname$ {\n",
        "classname", descriptor_->nested_type(i)->name());
    printer->Indent();

    MessageGenerator messageGenerator(
        descriptor_->nested_type(i),
        context_,
        immutable_api_);
    messageGenerator.GenerateBuildFunction(printer);

    printer->Outdent();
    printer->Print("}\n\n");
  }

}

void MessageGenerator::GenerateAccessorBuilders(io::Printer* printer) {
  for (int j = 0; j < descriptor_->field_count(); ++j) {
    const FieldDescriptor *field = descriptor_->field(j);
    if (field->type() == FieldDescriptor::TYPE_MESSAGE) {
      const java::FieldGeneratorInfo* info = context_->GetFieldGeneratorInfo(field);

      if (field->is_repeated()) {
        printer->Print(
            "fun $full_name$.Builder.$field_name$(\n"
            "  block: MutableList<$field_type$>.() -> Unit\n"
            ") {\n"
            "  addAll$capitalized_field_name$(\n"
            "    mutableListOf<$field_type$>()\n"
            "      .apply(block)\n"
            "  )\n"
            "}\n\n",
            "field_name", field->name(),
            "capitalized_field_name", info->capitalized_name,
            "full_name", name_resolver_->GetClassName(descriptor_, immutable_api_),
            "field_type", name_resolver_->GetClassName(field->message_type(), immutable_api_));
      } else {
        printer->Print(
            "fun $full_name$.Builder.$field_name$(\n"
            "    block: $field_type$.Builder.() -> Unit\n"
            ") {\n"
            "  $field_name$Builder.apply(block)\n"
            "}\n\n",
            "field_name", field->name(),
            "full_name", name_resolver_->GetClassName(descriptor_, immutable_api_),
            "field_type", name_resolver_->GetClassName(field->message_type(), immutable_api_));
      }
    }
  }

  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    MessageGenerator messageGenerator(
        descriptor_->nested_type(i),
        context_,
        immutable_api_);
    messageGenerator.GenerateAccessorBuilders(printer);
  }
}

void MessageGenerator::GenerateMutableListAppender(io::Printer* printer) {
  printer->Print(
      "fun MutableList<$field_type$>.add$field_type_name$(\n"
      "    block: $field_type$.Builder.() -> Unit\n"
      ") {\n"
      "    add(\n"
      "      $field_type$.newBuilder()\n"
      "       .apply(block)\n"
      "       .build()\n"
      "    )\n"
      "}\n\n",
      "field_type_name", descriptor_->name(),
      "field_type", name_resolver_->GetClassName(descriptor_, immutable_api_));

  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    MessageGenerator messageGenerator(
        descriptor_->nested_type(i),
        context_,
        immutable_api_);
    messageGenerator.GenerateMutableListAppender(printer);
  }
}

}  // namespace kotlin
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
