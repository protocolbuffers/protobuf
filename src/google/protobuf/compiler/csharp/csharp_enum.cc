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

// Author: jonskeet@google.com (Jon Skeet)
//  Following the Java generator by kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <map>
#include <string>

#include <google/protobuf/compiler/csharp/csharp_enum.h>
#include <google/protobuf/compiler/csharp/csharp_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

EnumGenerator::EnumGenerator(const EnumDescriptor* descriptor)
  : descriptor_(descriptor) {
  for (int i = 0; i < descriptor_->value_count(); i++) {
    const EnumValueDescriptor* value = descriptor_->value(i);
    const EnumValueDescriptor* canonical_value =
      descriptor_->FindValueByNumber(value->number());

    if (value == canonical_value) {
      canonical_values_.push_back(value);
    } else {
      Alias alias;
      alias.value = value;
      alias.canonical_value = canonical_value;
      aliases_.push_back(alias);
    }
  }
}

EnumGenerator::~EnumGenerator() {}

void EnumGenerator::Generate(io::Printer* printer) {
  bool is_own_file =
    descriptor_->containing_type() == NULL &&
    descriptor_->file()->options().csharp_multiple_files();
  printer->Print(
    "public $static$ enum $classname$ {\r\n",
    "static", is_own_file ? "" : "static",
    "classname", descriptor_->name());
  printer->Indent();

  for (int i = 0; i < canonical_values_.size(); i++) {
    map<string, string> vars;
    vars["name"] = canonical_values_[i]->name();
    vars["index"] = SimpleItoa(canonical_values_[i]->index());
    vars["number"] = SimpleItoa(canonical_values_[i]->number());
    printer->Print(vars,
      "$name$($index$, $number$),\r\n");
  }

  printer->Print(
    ";\r\n"
    "\r\n");

  // -----------------------------------------------------------------

  for (int i = 0; i < aliases_.size(); i++) {
    map<string, string> vars;
    vars["classname"] = descriptor_->name();
    vars["name"] = aliases_[i].value->name();
    vars["canonical_name"] = aliases_[i].canonical_value->name();
    printer->Print(vars,
      "public static final $classname$ $name$ = $canonical_name$;\r\n");
  }

  // -----------------------------------------------------------------

  printer->Print(
    "\r\n"
    "public final int getNumber() { return value; }\r\n"
    "\r\n"
    "public static $classname$ valueOf(int value) {\r\n"
    "  switch (value) {\r\n",
    "classname", descriptor_->name());
  printer->Indent();
  printer->Indent();

  for (int i = 0; i < canonical_values_.size(); i++) {
    printer->Print(
      "case $number$: return $name$;\r\n",
      "name", canonical_values_[i]->name(),
      "number", SimpleItoa(canonical_values_[i]->number()));
  }

  printer->Outdent();
  printer->Outdent();
  printer->Print(
    "    default: return null;\r\n"
    "  }\r\n"
    "}\r\n"
    "\r\n");

  // -----------------------------------------------------------------
  // Reflection

  printer->Print(
    "public final com.google.protobuf.Descriptors.EnumValueDescriptor\r\n"
    "    getValueDescriptor() {\r\n"
    "  return getDescriptor().getValues().get(index);\r\n"
    "}\r\n"
    "public final com.google.protobuf.Descriptors.EnumDescriptor\r\n"
    "    getDescriptorForType() {\r\n"
    "  return getDescriptor();\r\n"
    "}\r\n"
    "public static final com.google.protobuf.Descriptors.EnumDescriptor\r\n"
    "    getDescriptor() {\r\n");

  // TODO(kenton):  Cache statically?  Note that we can't access descriptors
  //   at module init time because it wouldn't work with descriptor.proto, but
  //   we can cache the value the first time getDescriptor() is called.
  if (descriptor_->containing_type() == NULL) {
    printer->Print(
      "  return $file$.getDescriptor().getEnumTypes().get($index$);\r\n",
      "file", ClassName(descriptor_->file()),
      "index", SimpleItoa(descriptor_->index()));
  } else {
    printer->Print(
      "  return $parent$.getDescriptor().getEnumTypes().get($index$);\r\n",
      "parent", ClassName(descriptor_->containing_type()),
      "index", SimpleItoa(descriptor_->index()));
  }

  printer->Print(
    "}\r\n"
    "\r\n"
    "private static final $classname$[] VALUES = {\r\n"
    "  ",
    "classname", descriptor_->name());

  for (int i = 0; i < descriptor_->value_count(); i++) {
    printer->Print("$name$, ",
      "name", descriptor_->value(i)->name());
  }

  printer->Print(
    "\r\n"
    "};\r\n"
    "public static $classname$ valueOf(\r\n"
    "    com.google.protobuf.Descriptors.EnumValueDescriptor desc) {\r\n"
    "  if (desc.getType() != getDescriptor()) {\r\n"
    "    throw new java.lang.IllegalArgumentException(\r\n"
    "      \"EnumValueDescriptor is not for this type.\");\r\n"
    "  }\r\n"
    "  return VALUES[desc.getIndex()];\r\n"
    "}\r\n",
    "classname", descriptor_->name());

  // -----------------------------------------------------------------

  printer->Print(
    "private final int index;\r\n"
    "private final int value;\r\n"
    "private $classname$(int index, int value) {\r\n"
    "  this.index = index;\r\n"
    "  this.value = value;\r\n"
    "}\r\n",
    "classname", descriptor_->name());

  printer->Outdent();
  printer->Print("}\r\n\r\n");
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
