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

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/java/names.h"

#include <string>

#include "absl/container/flat_hash_set.h"
#include "google/protobuf/compiler/java/helpers.h"
#include "google/protobuf/compiler/java/name_resolver.h"
#include "google/protobuf/compiler/java/names.h"
#include "google/protobuf/compiler/java/options.h"
#include "google/protobuf/descriptor.pb.h"

// Must be last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

namespace {

const char* DefaultPackage(Options options) {
  return options.opensource_runtime ? "" : "com.google.protos";
}

bool IsReservedName(absl::string_view name) {
  static const auto& kReservedNames =
      *new absl::flat_hash_set<absl::string_view>({
          "abstract",   "assert",       "boolean",   "break",      "byte",
          "case",       "catch",        "char",      "class",      "const",
          "continue",   "default",      "do",        "double",     "else",
          "enum",       "extends",      "final",     "finally",    "float",
          "for",        "goto",         "if",        "implements", "import",
          "instanceof", "int",          "interface", "long",       "native",
          "new",        "package",      "private",   "protected",  "public",
          "return",     "short",        "static",    "strictfp",   "super",
          "switch",     "synchronized", "this",      "throw",      "throws",
          "transient",  "try",          "void",      "volatile",   "while",
      });
  return kReservedNames.contains(name);
}

bool IsForbidden(const std::string& field_name) {
  // Names that should be avoided (in UpperCamelCase format).
  // Using them will cause the compiler to generate accessors whose names
  // collide with methods defined in base classes.
  // Keep this list in sync with specialFieldNames in
  // java/core/src/main/java/com/google/protobuf/DescriptorMessageInfoFactory.java
  static const auto& kForbiddenNames =
      *new absl::flat_hash_set<absl::string_view>({
        // java.lang.Object:
          "Class",
          // com.google.protobuf.MessageLiteOrBuilder:
          "DefaultInstanceForType",
          // com.google.protobuf.MessageLite:
          "ParserForType",
          "SerializedSize",
          // com.google.protobuf.MessageOrBuilder:
          "AllFields",
          "DescriptorForType",
          "InitializationErrorString",
          "UnknownFields",
          // obsolete. kept for backwards compatibility of generated code
          "CachedSize",
      });
  return kForbiddenNames.contains(UnderscoresToCamelCase(field_name, true));
}

std::string FieldName(const FieldDescriptor* field) {
  std::string field_name;
  // Groups are hacky:  The name of the field is just the lower-cased name
  // of the group type.  In Java, though, we would like to retain the original
  // capitalization of the type name.
  if (GetType(field) == FieldDescriptor::TYPE_GROUP) {
    field_name = field->message_type()->name();
  } else {
    field_name = field->name();
  }
  if (IsForbidden(field_name)) {
    // Append a trailing "#" to indicate that the name should be decorated to
    // avoid collision with other names.
    field_name += "#";
  }
  return field_name;
}

}  // namespace

std::string ClassName(const Descriptor* descriptor) {
  ClassNameResolver name_resolver;
  return name_resolver.GetClassName(descriptor, true);
}

std::string ClassName(const EnumDescriptor* descriptor) {
  ClassNameResolver name_resolver;
  return name_resolver.GetClassName(descriptor, true);
}

std::string ClassName(const ServiceDescriptor* descriptor) {
  ClassNameResolver name_resolver;
  return name_resolver.GetClassName(descriptor, true);
}

std::string ClassName(const FileDescriptor* descriptor) {
  ClassNameResolver name_resolver;
  return name_resolver.GetClassName(descriptor, true);
}

std::string FileJavaPackage(const FileDescriptor* file, bool immutable,
                            Options options) {
  std::string result;

  if (file->options().has_java_package()) {
    result = file->options().java_package();
  } else {
    result = DefaultPackage(options);
    if (!file->package().empty()) {
      if (!result.empty()) result += '.';
      result += file->package();
    }
  }

  return result;
}

std::string FileJavaPackage(const FileDescriptor* file, Options options) {
  return FileJavaPackage(file, true /* immutable */, options);
}

std::string CapitalizedFieldName(const FieldDescriptor* field) {
  return UnderscoresToCamelCase(FieldName(field), true);
}

std::string UnderscoresToCamelCase(const FieldDescriptor* field) {
  return UnderscoresToCamelCase(FieldName(field), false);
}

std::string UnderscoresToCapitalizedCamelCase(const FieldDescriptor* field) {
  return UnderscoresToCamelCase(FieldName(field), true);
}

std::string UnderscoresToCamelCase(const MethodDescriptor* method) {
  return UnderscoresToCamelCase(method->name(), false);
}

std::string UnderscoresToCamelCaseCheckReserved(const FieldDescriptor* field) {
  std::string name = UnderscoresToCamelCase(field);
  if (IsReservedName(name)) {
    return name + "_";
  }
  return name;
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
