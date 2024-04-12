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

#include "google/protobuf/compiler/php/names.h"

#include <string>

#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/plugin.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"

const char* const kReservedNames[] = {
    "abstract",     "and",        "array",        "as",         "break",
    "callable",     "case",       "catch",        "class",      "clone",
    "const",        "continue",   "declare",      "default",    "die",
    "do",           "echo",       "else",         "elseif",     "empty",
    "enddeclare",   "endfor",     "endforeach",   "endif",      "endswitch",
    "endwhile",     "eval",       "exit",         "extends",    "final",
    "finally",      "fn",         "for",          "foreach",    "function",
    "global",       "goto",       "if",           "implements", "include",
    "include_once", "instanceof", "insteadof",    "interface",  "isset",
    "list",         "match",      "namespace",    "new",        "or",
    "parent",       "print",      "private",      "protected",  "public",
    "readonly",     "require",    "require_once", "return",     "self",
    "static",       "switch",     "throw",        "trait",      "try",
    "unset",        "use",        "var",          "while",      "xor",
    "yield",        "int",        "float",        "bool",       "string",
    "true",         "false",      "null",         "void",       "iterable"};
const int kReservedNamesSize = 80;

namespace google {
namespace protobuf {
namespace compiler {
namespace php {

bool IsReservedName(absl::string_view name) {
  std::string lower(name);
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
  for (int i = 0; i < kReservedNamesSize; i++) {
    if (lower == kReservedNames[i]) {
      return true;
    }
  }
  return false;
}

std::string ReservedNamePrefix(const std::string& classname,
                               const FileDescriptor* file) {
  if (IsReservedName(classname)) {
    if (file->package() == "google.protobuf") {
      return "GPB";
    } else {
      return "PB";
    }
  }

  return "";
}

namespace {

template <typename DescriptorType>
std::string ClassNamePrefixImpl(const std::string& classname,
                                const DescriptorType* desc) {
  const std::string& prefix = (desc->file()->options()).php_class_prefix();
  if (!prefix.empty()) {
    return prefix;
  }

  return ReservedNamePrefix(classname, desc->file());
}

template <typename DescriptorType>
std::string GeneratedClassNameImpl(const DescriptorType* desc) {
  std::string classname = ClassNamePrefixImpl(desc->name(), desc) + desc->name();
  const Descriptor* containing = desc->containing_type();
  while (containing != NULL) {
    classname = ClassNamePrefixImpl(containing->name(), desc) + containing->name()
       + '\\' + classname;
    containing = containing->containing_type();
  }
  return classname;
}

std::string GeneratedClassNameImpl(const ServiceDescriptor* desc) {
  std::string classname = desc->name();
  return ClassNamePrefixImpl(classname, desc) + classname;
}

}  // namespace

std::string ClassNamePrefix(const std::string& classname,
                            const Descriptor* desc) {
  return ClassNamePrefixImpl(classname, desc);
}
std::string ClassNamePrefix(const std::string& classname,
                            const EnumDescriptor* desc) {
  return ClassNamePrefixImpl(classname, desc);
}

std::string GeneratedClassName(const Descriptor* desc) {
  return GeneratedClassNameImpl(desc);
}

std::string GeneratedClassName(const EnumDescriptor* desc) {
  return GeneratedClassNameImpl(desc);
}

std::string GeneratedClassName(const ServiceDescriptor* desc) {
  return GeneratedClassNameImpl(desc);
}

}  // namespace php
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
