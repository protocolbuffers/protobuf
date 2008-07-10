// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <set>
#include <map>

#include <google/protobuf/compiler/cpp/cpp_enum.h>
#include <google/protobuf/compiler/cpp/cpp_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

EnumGenerator::EnumGenerator(const EnumDescriptor* descriptor,
                             const string& dllexport_decl)
  : descriptor_(descriptor),
    classname_(ClassName(descriptor, false)),
    dllexport_decl_(dllexport_decl) {
}

EnumGenerator::~EnumGenerator() {}

void EnumGenerator::GenerateDefinition(io::Printer* printer) {
  map<string, string> vars;
  vars["classname"] = classname_;
  vars["short_name"] = descriptor_->name();

  printer->Print(vars, "enum $classname$ {\n");
  printer->Indent();

  const EnumValueDescriptor* min_value = descriptor_->value(0);
  const EnumValueDescriptor* max_value = descriptor_->value(0);

  for (int i = 0; i < descriptor_->value_count(); i++) {
    vars["name"] = descriptor_->value(i)->name();
    vars["number"] = SimpleItoa(descriptor_->value(i)->number());
    vars["prefix"] = (descriptor_->containing_type() == NULL) ?
      "" : classname_ + "_";

    printer->Print(vars, "$prefix$$name$ = $number$,\n");

    if (descriptor_->value(i)->number() < min_value->number()) {
      min_value = descriptor_->value(i);
    }
    if (descriptor_->value(i)->number() > max_value->number()) {
      max_value = descriptor_->value(i);
    }
  }

  printer->Outdent();
  printer->Print("};\n");

  vars["min_name"] = min_value->name();
  vars["max_name"] = max_value->name();

  if (dllexport_decl_.empty()) {
    vars["dllexport"] = "";
  } else {
    vars["dllexport"] = dllexport_decl_ + " ";
  }

  printer->Print(vars,
    "$dllexport$const ::google::protobuf::EnumDescriptor* $classname$_descriptor();\n"
    "$dllexport$bool $classname$_IsValid(int value);\n"
    "const $classname$ $prefix$$short_name$_MIN = $prefix$$min_name$;\n"
    "const $classname$ $prefix$$short_name$_MAX = $prefix$$max_name$;\n"
    "\n");
}

void EnumGenerator::GenerateSymbolImports(io::Printer* printer) {
  map<string, string> vars;
  vars["nested_name"] = descriptor_->name();
  vars["classname"] = classname_;
  printer->Print(vars, "typedef $classname$ $nested_name$;\n");

  for (int j = 0; j < descriptor_->value_count(); j++) {
    vars["tag"] = descriptor_->value(j)->name();
    printer->Print(vars,
      "static const $nested_name$ $tag$ = $classname$_$tag$;\n");
  }

  printer->Print(vars,
    "static inline const ::google::protobuf::EnumDescriptor*\n"
    "$nested_name$_descriptor() {\n"
    "  return $classname$_descriptor();\n"
    "}\n"
    "static inline bool $nested_name$_IsValid(int value) {\n"
    "  return $classname$_IsValid(value);\n"
    "}\n"
    "static const $nested_name$ $nested_name$_MIN =\n"
    "  $classname$_$nested_name$_MIN;\n"
    "static const $nested_name$ $nested_name$_MAX =\n"
    "  $classname$_$nested_name$_MAX;\n");
}

void EnumGenerator::GenerateDescriptorInitializer(
    io::Printer* printer, int index) {
  map<string, string> vars;
  vars["classname"] = classname_;
  vars["index"] = SimpleItoa(index);

  if (descriptor_->containing_type() == NULL) {
    printer->Print(vars,
      "$classname$_descriptor_ = file->enum_type($index$);\n");
  } else {
    vars["parent"] = ClassName(descriptor_->containing_type(), false);
    printer->Print(vars,
      "$classname$_descriptor_ = $parent$_descriptor_->enum_type($index$);\n");
  }
}

void EnumGenerator::GenerateMethods(io::Printer* printer) {
  map<string, string> vars;
  vars["classname"] = classname_;
  vars["builddescriptorsname"] =
      GlobalBuildDescriptorsName(descriptor_->file()->name());

  printer->Print(vars,
    "const ::google::protobuf::EnumDescriptor* $classname$_descriptor() {\n"
    "  if ($classname$_descriptor_ == NULL) $builddescriptorsname$();\n"
    "  return $classname$_descriptor_;\n"
    "}\n"
    "bool $classname$_IsValid(int value) {\n"
    "  switch(value) {\n");

  // Multiple values may have the same number.  Make sure we only cover
  // each number once by first constructing a set containing all valid
  // numbers, then printing a case statement for each element.

  set<int> numbers;
  for (int j = 0; j < descriptor_->value_count(); j++) {
    const EnumValueDescriptor* value = descriptor_->value(j);
    numbers.insert(value->number());
  }

  for (set<int>::iterator iter = numbers.begin();
       iter != numbers.end(); ++iter) {
    printer->Print(
      "    case $number$:\n",
      "number", SimpleItoa(*iter));
  }

  printer->Print(vars,
    "      return true;\n"
    "    default:\n"
    "      return false;\n"
    "  }\n"
    "}\n"
    "\n");

  if (descriptor_->containing_type() != NULL) {
    // We need to "define" the static constants which were declared in the
    // header, to give the linker a place to put them.  Or at least the C++
    // standard says we have to.  MSVC actually insists tha we do _not_ define
    // them again in the .cc file.
    printer->Print("#ifndef _MSC_VER\n");

    vars["parent"] = ClassName(descriptor_->containing_type(), false);
    vars["nested_name"] = descriptor_->name();
    for (int i = 0; i < descriptor_->value_count(); i++) {
      vars["value"] = descriptor_->value(i)->name();
      printer->Print(vars,
        "const $classname$ $parent$::$value$;\n");
    }
    printer->Print(vars,
      "const $classname$ $parent$::$nested_name$_MIN;\n"
      "const $classname$ $parent$::$nested_name$_MAX;\n");

    printer->Print("#endif  // _MSC_VER\n");
  }
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
