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

#include <google/protobuf/compiler/csharp/csharp_enum_field.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/compiler/csharp/csharp_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

namespace {

// TODO(kenton):  Factor out a "SetCommonFieldVariables()" to get rid of
//   repeat code between this and the other field types.
void SetEnumVariables(const FieldDescriptor* descriptor,
                      map<string, string>* variables) {
  const EnumValueDescriptor* default_value;
  default_value = descriptor->default_value_enum();

  string type = ClassName(descriptor->enum_type());

  (*variables)["name"] =
    UnderscoresToCamelCase(descriptor);
  (*variables)["capitalized_name"] =
    UnderscoresToCapitalizedCamelCase(descriptor);
  (*variables)["number"] = SimpleItoa(descriptor->number());
  (*variables)["type"] = type;
  (*variables)["default"] = type + "." + default_value->name();
}

}  // namespace

// ===================================================================

EnumFieldGenerator::
EnumFieldGenerator(const FieldDescriptor* descriptor)
  : descriptor_(descriptor) {
  SetEnumVariables(descriptor, &variables_);
}

EnumFieldGenerator::~EnumFieldGenerator() {}

void EnumFieldGenerator::
GenerateMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "private bool has$capitalized_name$;\r\n"
    "private $type$ $name$_ = $default$;\r\n"
    "public bool Has$capitalized_name$ {\r\n"
    "  get { return has$capitalized_name$; }\r\n"
    "}\r\n"
    "public $type$ $capitalized_name$ {"
    "  get { return $name$_; }"
    "}\r\n");
}

void EnumFieldGenerator::
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
    "public Builder Clear$capitalized_name$() {\r\n"
    "  result.has$capitalized_name$ = false;\r\n"
    "  result.$name$_ = $default$;\r\n"
    "  return this;\r\n"
    "}\r\n");
}

void EnumFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (other.Has$capitalized_name$) {\r\n"
    "  $capitalized_name$ = other.$capitalized_name$;\r\n"
    "}\r\n");
}

void EnumFieldGenerator::
GenerateBuildingCode(io::Printer* printer) const {
  // Nothing to do here for enum types.
}

void EnumFieldGenerator::
GenerateParsingCode(io::Printer* printer) const {
  printer->Print(variables_,
    // TODO(jonskeet): Make a more efficient way of doing this
    "int rawValue = input.ReadEnum();\r\n"
    "if (!global::System.Enum.IsDefined(typeof($type$), rawValue)) {\r\n"
    "  unknownFields.MergeVarintField($number$, (ulong) rawValue);\r\n"
    "} else {\r\n"
    "  $capitalized_name$ = ($type$) rawValue;\r\n"
    "}\r\n");
}

void EnumFieldGenerator::
GenerateSerializationCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (Has$capitalized_name$) {\r\n"
    "  output.WriteEnum($number$, (int) $capitalized_name$);\r\n"
    "}\r\n");
}

void EnumFieldGenerator::
GenerateSerializedSizeCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (Has$capitalized_name$) {\r\n"
    "  size += pb::CodedOutputStream\r\n"
    "    .ComputeEnumSize($number$, (int) $capitalized_name$);\r\n"
    "}\r\n");
}

// ===================================================================

RepeatedEnumFieldGenerator::
RepeatedEnumFieldGenerator(const FieldDescriptor* descriptor)
  : descriptor_(descriptor) {
  SetEnumVariables(descriptor, &variables_);
}

RepeatedEnumFieldGenerator::~RepeatedEnumFieldGenerator() {}

void RepeatedEnumFieldGenerator::
GenerateMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "private scg::IList<$type$> $name$_ = new scg::List<$type$> ();\r\n"
    "public scg.IList<$type$> $capitalized_name$List {\r\n"
    "  get { return pbc::Lists.AsReadOnly($name$_); }\r\n"   // note:  unmodifiable list
    "}\r\n"

    // Redundant API calls?
    // TODO(jonskeet): Possibly. Include for portability to start with though.
    "public int $capitalized_name$Count {\r\n"
    "  get { return $name$_.Count; }\r\n"
    "}\r\n"
    "public $type$ Get$capitalized_name$(int index) {\r\n"
    "  return $name$_[index];\r\n"
    "}\r\n"
    );
}

void RepeatedEnumFieldGenerator::
GenerateBuilderMembers(io::Printer* printer) const {
  printer->Print(variables_,
    // Note:  We return an unmodifiable list because otherwise the caller
    //   could hold on to the returned list and modify it after the message
    //   has been built, thus mutating the message which is supposed to be
    //   immutable.
    "public scg::IList<$type$> $capitalized_name$List {\r\n"
    "  get { return pbc::Lists.AsReadOnly(result.$name$_); }\r\n"
    "}\r\n"
    "public int $capitalized_name$Count {\r\n"
    "  get { return result.$capitalized_name$Count; } \r\n"
    "}\r\n"
    "public $type$ Get$capitalized_name$(int index) {\r\n"
    "  return result.Get$capitalized_name$(index);\r\n"
    "}\r\n"
    "public Builder Set$capitalized_name$(int index, $type$ value) {\r\n"
    "  result.$name$_[index] = value;\r\n"
    "  return this;\r\n"
    "}\r\n"
    "public Builder Add$capitalized_name$($type$ value) {\r\n"
    "  if (result.$name$_.Count == 0) {\r\n"
    "    result.$name$_ = new scg::List<$type$>();\r\n"
    "  }\r\n"
    "  result.$name$_.Add(value);\r\n"
    "  return this;\r\n"
    "}\r\n"
    "public Builder AddRange$capitalized_name$(scg::IEnumerable<$type$> values) {\r\n"
    "  if (result.$name$_.Count == 0) {\r\n"
    "    result.$name$_ = new scg::List<$type$>();\r\n"
    "  }\r\n"
    "  base.AddRange(values, result.$name$_);\r\n"
    "  return this;\r\n"
    "}\r\n"
    "public Builder Clear$capitalized_name$() {\r\n"
    "  result.$name$_ = pbc::Lists<$type$>.Empty;\r\n"
    "  return this;\r\n"
    "}\r\n");
}

void RepeatedEnumFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (other.$name$_.Count != 0) {\r\n"
    "  if (result.$name$_.Count == 0) {\r\n"
    "    result.$name$_ = new scg::List<$type$>();\r\n"
    "  }\r\n"
    "  base.AddRange(other.$name$_, result.$name$_);\r\n"
    "}\r\n");
}

void RepeatedEnumFieldGenerator::
GenerateBuildingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "result.$name$_ = pbc::Lists.AsReadOnly(result.$name$_);\r\n");
}

void RepeatedEnumFieldGenerator::
GenerateParsingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "int rawValue = input.ReadEnum();\r\n"
    "$type$ value = ($type$) rawValue;\r\n"
    "if (!global::System.Enum.IsDefined(typeof($type$), value)) {\r\n"
    "  unknownFields.MergeVarintField($number$, (ulong) rawValue);\r\n"
    "} else {\r\n"
    "  Add$capitalized_name$(value);\r\n"
    "}\r\n");
}

void RepeatedEnumFieldGenerator::
GenerateSerializationCode(io::Printer* printer) const {
  printer->Print(variables_,
    "foreach ($type$ element in $capitalized_name$List) {\r\n"
    "  output.WriteEnum($number$, (int) element);\r\n"
    "}\r\n");
}

void RepeatedEnumFieldGenerator::
GenerateSerializedSizeCode(io::Printer* printer) const {
  printer->Print(variables_,
    "foreach ($type$ element in $capitalized_name$List) {\r\n"
    "  size += pb::CodedOutputStream\r\n"
    "    .ComputeEnumSize($number$, (int) element);\r\n"
    "}\r\n");
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
