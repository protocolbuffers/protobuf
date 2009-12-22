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

namespace {

const char* TypeName(FieldDescriptor::Type field_type) {
  switch (field_type) {
    case FieldDescriptor::TYPE_INT32   : return "INT32";
    case FieldDescriptor::TYPE_UINT32  : return "UINT32";
    case FieldDescriptor::TYPE_SINT32  : return "SINT32";
    case FieldDescriptor::TYPE_FIXED32 : return "FIXED32";
    case FieldDescriptor::TYPE_SFIXED32: return "SFIXED32";
    case FieldDescriptor::TYPE_INT64   : return "INT64";
    case FieldDescriptor::TYPE_UINT64  : return "UINT64";
    case FieldDescriptor::TYPE_SINT64  : return "SINT64";
    case FieldDescriptor::TYPE_FIXED64 : return "FIXED64";
    case FieldDescriptor::TYPE_SFIXED64: return "SFIXED64";
    case FieldDescriptor::TYPE_FLOAT   : return "FLOAT";
    case FieldDescriptor::TYPE_DOUBLE  : return "DOUBLE";
    case FieldDescriptor::TYPE_BOOL    : return "BOOL";
    case FieldDescriptor::TYPE_STRING  : return "STRING";
    case FieldDescriptor::TYPE_BYTES   : return "BYTES";
    case FieldDescriptor::TYPE_ENUM    : return "ENUM";
    case FieldDescriptor::TYPE_GROUP   : return "GROUP";
    case FieldDescriptor::TYPE_MESSAGE : return "MESSAGE";

    // No default because we want the compiler to complain if any new
    // types are added.
  }

  GOOGLE_LOG(FATAL) << "Can't get here.";
  return NULL;
}

}

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
  vars["number"] = SimpleItoa(descriptor_->number());
  vars["constant_name"] = FieldConstantName(descriptor_);
  vars["lite"] = HasDescriptorMethods(descriptor_->file()) ? "" : "Lite";

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

  printer->Print(vars,
    "public static final int $constant_name$ = $number$;\n");
  if (descriptor_->is_repeated()) {
    printer->Print(vars,
      "public static final\n"
      "  com.google.protobuf.GeneratedMessage$lite$.GeneratedExtension<\n"
      "    $containing_type$,\n"
      "    java.util.List<$type$>> $name$ =\n"
      "      com.google.protobuf.GeneratedMessage$lite$\n"
      "        .newGeneratedExtension();\n");
  } else {
    printer->Print(vars,
      "public static final\n"
      "  com.google.protobuf.GeneratedMessage$lite$.GeneratedExtension<\n"
      "    $containing_type$,\n"
      "    $type$> $name$ =\n"
      "      com.google.protobuf.GeneratedMessage$lite$\n"
      "        .newGeneratedExtension();\n");
  }
}

void ExtensionGenerator::GenerateInitializationCode(io::Printer* printer) {
  map<string, string> vars;
  vars["name"] = UnderscoresToCamelCase(descriptor_);
  vars["scope"] = scope_;
  vars["index"] = SimpleItoa(descriptor_->index());
  vars["extendee"] = ClassName(descriptor_->containing_type());
  vars["default"] = descriptor_->is_repeated() ? "" : DefaultValue(descriptor_);
  vars["number"] = SimpleItoa(descriptor_->number());
  vars["type_constant"] = TypeName(GetType(descriptor_));
  vars["packed"] = descriptor_->options().packed() ? "true" : "false";
  vars["enum_map"] = "null";
  vars["prototype"] = "null";

  JavaType java_type = GetJavaType(descriptor_);
  string singular_type;
  switch (java_type) {
    case JAVATYPE_MESSAGE:
      vars["type"] = ClassName(descriptor_->message_type());
      vars["prototype"] = ClassName(descriptor_->message_type()) +
                          ".getDefaultInstance()";
      break;
    case JAVATYPE_ENUM:
      vars["type"] = ClassName(descriptor_->enum_type());
      vars["enum_map"] = ClassName(descriptor_->enum_type()) +
                         ".internalGetValueMap()";
      break;
    default:
      vars["type"] = BoxedPrimitiveTypeName(java_type);
      break;
  }

  if (HasDescriptorMethods(descriptor_->file())) {
    printer->Print(vars,
      "$scope$.$name$.internalInit(\n"
      "    $scope$.getDescriptor().getExtensions().get($index$),\n"
      "    $type$.class);\n");
  } else {
    if (descriptor_->is_repeated()) {
      printer->Print(vars,
        "$scope$.$name$.internalInitRepeated(\n"
        "    $extendee$.getDefaultInstance(),\n"
        "    $prototype$,\n"
        "    $enum_map$,\n"
        "    $number$,\n"
        "    com.google.protobuf.WireFormat.FieldType.$type_constant$,\n"
        "    $packed$);\n");
    } else {
      printer->Print(vars,
        "$scope$.$name$.internalInitSingular(\n"
        "    $extendee$.getDefaultInstance(),\n"
        "    $default$,\n"
        "    $prototype$,\n"
        "    $enum_map$,\n"
        "    $number$,\n"
        "    com.google.protobuf.WireFormat.FieldType.$type_constant$);\n");
    }
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
