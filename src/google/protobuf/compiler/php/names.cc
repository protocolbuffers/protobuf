// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/php/names.h"

#include <algorithm>
#include <string>

#include "absl/strings/ascii.h"
#include "absl/strings/string_view.h"
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
  std::string lower = absl::AsciiStrToLower(name);
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
  while (containing != nullptr) {
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
