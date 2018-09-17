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

#include <map>

#include <google/protobuf/compiler/cpp/cpp_enum.h>
#include <google/protobuf/compiler/cpp/cpp_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

namespace {
// The GOOGLE_ARRAYSIZE constant is the max enum value plus 1. If the max enum value
// is kint32max, GOOGLE_ARRAYSIZE will overflow. In such cases we should omit the
// generation of the GOOGLE_ARRAYSIZE constant.
bool ShouldGenerateArraySize(const EnumDescriptor* descriptor) {
  int32 max_value = descriptor->value(0)->number();
  for (int i = 0; i < descriptor->value_count(); i++) {
    if (descriptor->value(i)->number() > max_value) {
      max_value = descriptor->value(i)->number();
    }
  }
  return max_value != kint32max;
}
}  // namespace

EnumGenerator::EnumGenerator(const EnumDescriptor* descriptor,
                             const std::map<string, string>& vars,
                             const Options& options)
    : descriptor_(descriptor),
      classname_(ClassName(descriptor, false)),
      options_(options),
      generate_array_size_(ShouldGenerateArraySize(descriptor)),
      variables_(vars) {
  variables_["classname"] = classname_;
  variables_["classtype"] = QualifiedClassName(descriptor_);
  variables_["short_name"] = descriptor_->name();
  variables_["enumbase"] = options_.proto_h ? " : int" : "";
  variables_["nested_name"] = descriptor_->name();
  variables_["constexpr"] = options_.proto_h ? "constexpr" : "";
  variables_["prefix"] =
      (descriptor_->containing_type() == NULL) ? "" : classname_ + "_";
}

EnumGenerator::~EnumGenerator() {}

void EnumGenerator::GenerateDefinition(io::Printer* printer) {
  Formatter format(printer, variables_);
  format("enum ${1$$classname$$}$$enumbase$ {\n", descriptor_);
  format.Indent();

  const EnumValueDescriptor* min_value = descriptor_->value(0);
  const EnumValueDescriptor* max_value = descriptor_->value(0);

  for (int i = 0; i < descriptor_->value_count(); i++) {
    auto format_value = format;
    format_value.Set("name", EnumValueName(descriptor_->value(i)));
    // In C++, an value of -2147483648 gets interpreted as the negative of
    // 2147483648, and since 2147483648 can't fit in an integer, this produces a
    // compiler warning.  This works around that issue.
    format_value.Set("number", Int32ToString(descriptor_->value(i)->number()));
    format_value.Set(
        "deprecation",
        DeprecatedAttribute(options_,
                            descriptor_->value(i)->options().deprecated()));

    if (i > 0) format_value(",\n");
    format_value("${1$$prefix$$name$$}$ $deprecation$= $number$",
                 descriptor_->value(i));

    if (descriptor_->value(i)->number() < min_value->number()) {
      min_value = descriptor_->value(i);
    }
    if (descriptor_->value(i)->number() > max_value->number()) {
      max_value = descriptor_->value(i);
    }
  }

  if (HasPreservingUnknownEnumSemantics(descriptor_->file())) {
    // For new enum semantics: generate min and max sentinel values equal to
    // INT32_MIN and INT32_MAX
    if (descriptor_->value_count() > 0) format(",\n");
    format(
        "$classname$_$prefix$INT_MIN_SENTINEL_DO_NOT_USE_ = "
        "std::numeric_limits<$int32$>::min(),\n"
        "$classname$_$prefix$INT_MAX_SENTINEL_DO_NOT_USE_ = "
        "std::numeric_limits<$int32$>::max()");
  }

  format.Outdent();
  format("\n};\n");

  format(
      "$dllexport_decl $bool $classname$_IsValid(int value);\n"
      "const $classname$ ${1$$prefix$$short_name$_MIN$}$ = "
      "$prefix$$2$;\n"
      "const $classname$ ${1$$prefix$$short_name$_MAX$}$ = "
      "$prefix$$3$;\n",
      descriptor_, EnumValueName(min_value), EnumValueName(max_value));

  if (generate_array_size_) {
    format(
        "const int ${1$$prefix$$short_name$_ARRAYSIZE$}$ = "
        "$prefix$$short_name$_MAX + 1;\n\n",
        descriptor_);
  }

  if (HasDescriptorMethods(descriptor_->file(), options_)) {
    format(
        "$dllexport_decl $const ::$proto_ns$::EnumDescriptor* "
        "$classname$_descriptor();\n");
    // The _Name and _Parse methods
    if (options_.opensource_runtime) {
      // TODO(haberman): consider removing this in favor of the stricter
      // version below.  Would this break our compatibility guarantees?
      format(
          "inline const $string$& $classname$_Name($classname$ value) {\n"
          "  return ::$proto_ns$::internal::NameOfEnum(\n"
          "    $classname$_descriptor(), value);\n"
          "}\n");
    } else {
      // Support a stricter, type-checked enum-to-string method that
      // statically checks whether the parameter is the exact enum type or is
      // an integral type.
      format(
          "template<typename T>\n"
          "inline const $string$& $classname$_Name(T enum_t_value) {\n"
          "  static_assert(::std::is_same<T, $classname$>::value ||\n"
          "    ::std::is_integral<T>::value,\n"
          "    \"Incorrect type passed to function $classname$_Name.\");\n"
          "  return ::$proto_ns$::internal::NameOfEnum(\n"
          "    $classname$_descriptor(), enum_t_value);\n"
          "}\n");
    }
    format(
        "inline bool $classname$_Parse(\n"
        "    const $string$& name, $classname$* value) {\n"
        "  return ::$proto_ns$::internal::ParseNamedEnum<$classname$>(\n"
        "    $classname$_descriptor(), name, value);\n"
        "}\n");
  }
}

void EnumGenerator::GenerateGetEnumDescriptorSpecializations(
    io::Printer* printer) {
  Formatter format(printer, variables_);
  format(
      "template <> struct is_proto_enum< $classtype$> : ::std::true_type "
      "{};\n");
  if (HasDescriptorMethods(descriptor_->file(), options_)) {
    format(
        "template <>\n"
        "inline const EnumDescriptor* GetEnumDescriptor< $classtype$>() {\n"
        "  return $classtype$_descriptor();\n"
        "}\n");
  }
}

void EnumGenerator::GenerateSymbolImports(io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("typedef $classname$ $nested_name$;\n");

  for (int j = 0; j < descriptor_->value_count(); j++) {
    string deprecated_attr = DeprecatedAttribute(
        options_, descriptor_->value(j)->options().deprecated());
    format(
        "$1$static $constexpr $const $nested_name$ ${2$$3$$}$ =\n"
        "  $classname$_$3$;\n",
        deprecated_attr, descriptor_->value(j),
        EnumValueName(descriptor_->value(j)));
  }

  format(
      "static inline bool $nested_name$_IsValid(int value) {\n"
      "  return $classname$_IsValid(value);\n"
      "}\n"
      "static const $nested_name$ ${1$$nested_name$_MIN$}$ =\n"
      "  $classname$_$nested_name$_MIN;\n"
      "static const $nested_name$ ${1$$nested_name$_MAX$}$ =\n"
      "  $classname$_$nested_name$_MAX;\n",
      descriptor_);
  if (generate_array_size_) {
    format(
        "static const int ${1$$nested_name$_ARRAYSIZE$}$ =\n"
        "  $classname$_$nested_name$_ARRAYSIZE;\n",
        descriptor_);
  }

  if (HasDescriptorMethods(descriptor_->file(), options_)) {
    format(
        "static inline const ::$proto_ns$::EnumDescriptor*\n"
        "$nested_name$_descriptor() {\n"
        "  return $classname$_descriptor();\n"
        "}\n");
    if (options_.opensource_runtime) {
      // TODO(haberman): consider removing this in favor of the stricter
      // version below.  Would this break our compatibility guarantees?
      format(
          "static inline const $string$& "
          "$nested_name$_Name($nested_name$ value) {"
          "\n"
          "  return $classname$_Name(value);\n"
          "}\n");
    } else {
      // Support a stricter, type-checked enum-to-string method that
      // statically checks whether the parameter is the exact enum type or is
      // an integral type.
      format(
          "template<typename T>\n"
          "static inline const $string$& $nested_name$_Name(T enum_t_value) {\n"
          "  static_assert(::std::is_same<T, $nested_name$>::value ||\n"
          "    ::std::is_integral<T>::value,\n"
          "    \"Incorrect type passed to function $nested_name$_Name.\");\n"
          "  return $classname$_Name(enum_t_value);\n"
          "}\n");
    }
    format(
        "static inline bool $nested_name$_Parse(const $string$& name,\n"
        "    $nested_name$* value) {\n"
        "  return $classname$_Parse(name, value);\n"
        "}\n");
  }
}

void EnumGenerator::GenerateMethods(int idx, io::Printer* printer) {
  Formatter format(printer, variables_);
  if (HasDescriptorMethods(descriptor_->file(), options_)) {
    format(
        "const ::$proto_ns$::EnumDescriptor* $classname$_descriptor() {\n"
        "  ::$proto_ns$::internal::AssignDescriptors(&$assign_desc_table$);\n"
        "  return $file_level_enum_descriptors$[$1$];\n"
        "}\n",
        idx);
  }

  format(
      "bool $classname$_IsValid(int value) {\n"
      "  switch (value) {\n");

  // Multiple values may have the same number.  Make sure we only cover
  // each number once by first constructing a set containing all valid
  // numbers, then printing a case statement for each element.

  std::set<int> numbers;
  for (int j = 0; j < descriptor_->value_count(); j++) {
    const EnumValueDescriptor* value = descriptor_->value(j);
    numbers.insert(value->number());
  }

  for (std::set<int>::iterator iter = numbers.begin(); iter != numbers.end();
       ++iter) {
    format("    case $1$:\n", Int32ToString(*iter));
  }

  format(
      "      return true;\n"
      "    default:\n"
      "      return false;\n"
      "  }\n"
      "}\n"
      "\n");

  if (descriptor_->containing_type() != NULL) {
    string parent = ClassName(descriptor_->containing_type(), false);
    // We need to "define" the static constants which were declared in the
    // header, to give the linker a place to put them.  Or at least the C++
    // standard says we have to.  MSVC actually insists that we do _not_ define
    // them again in the .cc file, prior to VC++ 2015.
    format("#if !defined(_MSC_VER) || _MSC_VER >= 1900\n");

    for (int i = 0; i < descriptor_->value_count(); i++) {
      format("$constexpr $const $classname$ $1$::$2$;\n", parent,
             EnumValueName(descriptor_->value(i)));
    }
    format(
        "const $classname$ $1$::$nested_name$_MIN;\n"
        "const $classname$ $1$::$nested_name$_MAX;\n",
        parent);
    if (generate_array_size_) {
      format("const int $1$::$nested_name$_ARRAYSIZE;\n", parent);
    }

    format("#endif  // !defined(_MSC_VER) || _MSC_VER >= 1900\n");
  }
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
