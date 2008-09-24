// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

#include <google/protobuf/compiler/java/java_extension.h>
#include <google/protobuf/compiler/java/java_helpers.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/io/printer.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

ExtensionGenerator::ExtensionGenerator(const FieldDescriptor* descriptor)
  : descriptor_(descriptor) {
  if (descriptor_->extension_scope() != NULL) {
    scope_ = ClassName(descriptor_->extension_scope());
  } else {
    scope_ = ClassName(descriptor_->file());
  }
}

ExtensionGenerator::~ExtensionGenerator() {}

void ExtensionGenerator::Generate(io::Printer* printer) {
  map<string, string> vars;
  vars["name"] = UnderscoresToCamelCase(descriptor_);
  vars["containing_type"] = ClassName(descriptor_->containing_type());

  JavaType java_type = GetJavaType(descriptor_);
  string singular_type;
  switch (java_type) {
    case JAVATYPE_MESSAGE:
      vars["type"] = ClassName(descriptor_->message_type());
      break;
    case JAVATYPE_ENUM:
      vars["type"] = ClassName(descriptor_->enum_type());
      break;
    default:
      vars["type"] = BoxedPrimitiveTypeName(java_type);
      break;
  }

  if (descriptor_->is_repeated()) {
    printer->Print(vars,
      "public static\n"
      "  com.google.protobuf.GeneratedMessage.GeneratedExtension<\n"
      "    $containing_type$,\n"
      "    java.util.List<$type$>> $name$;\n");
  } else {
    printer->Print(vars,
      "public static\n"
      "  com.google.protobuf.GeneratedMessage.GeneratedExtension<\n"
      "    $containing_type$,\n"
      "    $type$> $name$;\n");
  }
}

void ExtensionGenerator::GenerateInitializationCode(io::Printer* printer) {
  map<string, string> vars;
  vars["name"] = UnderscoresToCamelCase(descriptor_);
  vars["scope"] = scope_;
  vars["index"] = SimpleItoa(descriptor_->index());

  JavaType java_type = GetJavaType(descriptor_);
  string singular_type;
  switch (java_type) {
    case JAVATYPE_MESSAGE:
      vars["type"] = ClassName(descriptor_->message_type());
      break;
    case JAVATYPE_ENUM:
      vars["type"] = ClassName(descriptor_->enum_type());
      break;
    default:
      vars["type"] = BoxedPrimitiveTypeName(java_type);
      break;
  }

  if (descriptor_->is_repeated()) {
    printer->Print(vars,
      "$scope$.$name$ =\n"
      "  com.google.protobuf.GeneratedMessage\n"
      "    .newRepeatedGeneratedExtension(\n"
      "      $scope$.getDescriptor().getExtensions().get($index$),\n"
      "      $type$.class);\n");
  } else {
    printer->Print(vars,
      "$scope$.$name$ =\n"
      "  com.google.protobuf.GeneratedMessage.newGeneratedExtension(\n"
      "    $scope$.getDescriptor().getExtensions().get($index$),\n"
      "    $type$.class);\n");
  }
}

void ExtensionGenerator::GenerateRegistrationCode(io::Printer* printer) {
  printer->Print(
    "registry.add($scope$.$name$);\n",
    "scope", scope_,
    "name", UnderscoresToCamelCase(descriptor_));
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf

}  // namespace google
