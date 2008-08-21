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

#include <google/protobuf/compiler/csharp/csharp_message_field.h>
#include <google/protobuf/compiler/csharp/csharp_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

namespace {

// TODO(kenton):  Factor out a "SetCommonFieldVariables()" to get rid of
//   repeat code between this and the other field types.
void SetMessageVariables(const FieldDescriptor* descriptor,
                         map<string, string>* variables) {
  (*variables)["name"] =
    UnderscoresToCamelCase(descriptor);
  (*variables)["capitalized_name"] =
    UnderscoresToCapitalizedCamelCase(descriptor);
  (*variables)["number"] = SimpleItoa(descriptor->number());
  (*variables)["type"] = ClassName(descriptor->message_type());
  (*variables)["group_or_message"] =
    (descriptor->type() == FieldDescriptor::TYPE_GROUP) ?
    "Group" : "Message";
}

}  // namespace

// ===================================================================

MessageFieldGenerator::
MessageFieldGenerator(const FieldDescriptor* descriptor)
  : descriptor_(descriptor) {
  SetMessageVariables(descriptor, &variables_);
}

MessageFieldGenerator::~MessageFieldGenerator() {}

void MessageFieldGenerator::
GenerateMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "private bool has$capitalized_name$;\r\n"
    "private $type$ $name$_ = $type$.DefaultInstance;\r\n"
    "public bool Has$capitalized_name$ {\r\n"
    "  get { return has$capitalized_name$; }\r\n"
    "}\r\n"
    "public $type$ $capitalized_name$ {\r\n"
    "  get { return $name$_; }\r\n"
    "}\r\n");
}

void MessageFieldGenerator::
GenerateBuilderMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "public bool Has$capitalized_name$ {\r\n"
    "  get { return result.Has$capitalized_name$; }\r\n"
    "}\r\n"
    "public $type$ $capitalized_name$ {\r\n"
    "  get { return result.$capitalized_name$; }\r\n"
    "  set { Set$capitalized_name$(value); }\r\n"
    "}\r\n"
    "public Builder Set$capitalized_name$($type$ value) {\r\n"
    "  result.has$capitalized_name$ = true;\r\n"
    "  result.$name$_ = value;\r\n"
    "  return this;\r\n"
    "}\r\n"
    "public Builder Set$capitalized_name$($type$.Builder builderForValue) {\r\n"
    "  result.has$capitalized_name$ = true;\r\n"
    "  result.$name$_ = builderForValue.Build();\r\n"
    "  return this;\r\n"
    "}\r\n"
    "public Builder Merge$capitalized_name$($type$ value) {\r\n"
    "  if (result.Has$capitalized_name$ &&\r\n"
    "      result.$name$_ != $type$.DefaultInstance) {\r\n"
    "    result.$name$_ =\r\n"
    "      $type$.CreateBuilder(result.$name$_).MergeFrom(value).BuildPartial();\r\n"
    "  } else {\r\n"
    "    result.$name$_ = value;\r\n"
    "  }\r\n"
    "  result.has$capitalized_name$ = true;\r\n"
    "  return this;\r\n"
    "}\r\n"
    "public Builder Clear$capitalized_name$() {\r\n"
    "  result.has$capitalized_name$ = false;\r\n"
    "  result.$name$_ = $type$.DefaultInstance;\r\n"
    "  return this;\r\n"
    "}\r\n");
}

void MessageFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (other.Has$capitalized_name$) {\r\n"
    "  Merge$capitalized_name$(other.$capitalized_name$);\r\n"
    "}\r\n");
}

void MessageFieldGenerator::
GenerateBuildingCode(io::Printer* printer) const {
  // Nothing to do for singular fields.
}

void MessageFieldGenerator::
GenerateParsingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "$type$.Builder subBuilder = $type$.CreateBuilder();\r\n"
    "if (Has$capitalized_name$) {\r\n"
    "  subBuilder.MergeFrom($capitalized_name$);\r\n"
    "}\r\n");

  if (descriptor_->type() == FieldDescriptor::TYPE_GROUP) {
    printer->Print(variables_,
      "input.ReadGroup($number$, subBuilder, extensionRegistry);\r\n");
  } else {
    printer->Print(variables_,
      "input.ReadMessage(subBuilder, extensionRegistry);\r\n");
  }

  printer->Print(variables_,
    "$capitalized_name$ = subBuilder.BuildPartial();\r\n");
}

void MessageFieldGenerator::
GenerateSerializationCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (Has$capitalized_name$) {\r\n"
    "  output.Write$group_or_message$($number$, $capitalized_name$);\r\n"
    "}\r\n");
}

void MessageFieldGenerator::
GenerateSerializedSizeCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (Has$capitalized_name$) {\r\n"
    "  size += pb::CodedOutputStream.Compute$group_or_message$Size($number$, $capitalized_name$);\r\n"
    "}\r\n");
}

// ===================================================================

RepeatedMessageFieldGenerator::
RepeatedMessageFieldGenerator(const FieldDescriptor* descriptor)
  : descriptor_(descriptor) {
  SetMessageVariables(descriptor, &variables_);
}

RepeatedMessageFieldGenerator::~RepeatedMessageFieldGenerator() {}

void RepeatedMessageFieldGenerator::
GenerateMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "private pbc::PopsicleList<$type$> $name$_ = new pbc::PopsicleList<$type$>();\r\n"
    "public scg::IList<$type$> $capitalized_name$List {\r\n"
    "  get { return $name$_; } \r\n"   // Will be unmodifiable by the time it's exposed
    "}\r\n"
    "public int $capitalized_name$Count\r\n"
    "  { get { return $name$_.Count; }\r\n"
    "}\r\n"
    "public $type$ Get$capitalized_name$(int index) {\r\n"
    "  return $name$_ [index];\r\n"
    "}\r\n");
}

void RepeatedMessageFieldGenerator::
GenerateBuilderMembers(io::Printer* printer) const {
  printer->Print(variables_,
    // Note:  We can return the original list here, because we
    // make it read-only when we build.
    "public scg::IList<$type$> $capitalized_name$List {\r\n"
    "  get { return result.$name$_; }\r\n"
    "}\r\n"
    "public int $capitalized_name$Count {\r\n"
    "  get { return result.$capitalized_name$Count; }\r\n"
    "}\r\n"
    "public $type$ Get$capitalized_name$(int index) {\r\n"
    "  return result.Get$capitalized_name$(index);\r\n"
    "}\r\n"
    "public Builder Set$capitalized_name$(int index, $type$ value) {\r\n"
    "  result.$name$_[index] = value;\r\n"
    "  return this;\r\n"
    "}\r\n"
    "public Builder Set$capitalized_name$(int index, $type$.Builder builderForValue) {\r\n"
    "  result.$name$_[index] = builderForValue.Build();\r\n"
    "  return this;\r\n"
    "}\r\n"
    "public Builder Add$capitalized_name$($type$ value) {\r\n"
    "  result.$name$_.Add(value);\r\n"
    "  return this;\r\n"
    "}\r\n"
    "public Builder Add$capitalized_name$($type$.Builder builderForValue) {\r\n"
    "  result.$name$_.Add(builderForValue.Build());\r\n"
    "  return this;\r\n"
    "}\r\n"
    "public Builder AddRange$capitalized_name$(scg::IEnumerable<$type$> values) {\r\n"
    "  base.AddRange(values, result.$name$_);\r\n"
    "  return this;\r\n"
    "}\r\n"
    "public Builder Clear$capitalized_name$() {\r\n"
    "  result.$name$_.Clear();\r\n"
    "  return this;\r\n"
    "}\r\n");
}

void RepeatedMessageFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (other.$name$_.Count != 0) {\r\n"
    "  base.AddRange(other.$name$_, result.$name$_);\r\n"
    "}\r\n");
}

void RepeatedMessageFieldGenerator::
GenerateBuildingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "result.$name$_.MakeReadOnly();\r\n");
}

void RepeatedMessageFieldGenerator::
GenerateParsingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "$type$.Builder subBuilder = $type$.CreateBuilder();\r\n");

  if (descriptor_->type() == FieldDescriptor::TYPE_GROUP) {
    printer->Print(variables_,
      "input.ReadGroup($number$, subBuilder, extensionRegistry);\r\n");
  } else {
    printer->Print(variables_,
      "input.ReadMessage(subBuilder, extensionRegistry);\r\n");
  }

  printer->Print(variables_,
    "Add$capitalized_name$(subBuilder.BuildPartial());\r\n");
}

void RepeatedMessageFieldGenerator::
GenerateSerializationCode(io::Printer* printer) const {
  printer->Print(variables_,
    "foreach ($type$ element in $capitalized_name$List) {\r\n"
    "  output.Write$group_or_message$($number$, element);\r\n"
    "}\r\n");
}

void RepeatedMessageFieldGenerator::
GenerateSerializedSizeCode(io::Printer* printer) const {
  printer->Print(variables_,
    "foreach ($type$ element in $capitalized_name$List) {\r\n"
    "  size += pb::CodedOutputStream.Compute$group_or_message$Size($number$, element);\r\n"
    "}\r\n");
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
