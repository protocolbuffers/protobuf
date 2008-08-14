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

#include <google/protobuf/compiler/csharp/csharp_primitive_field.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/compiler/csharp/csharp_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/stubs/substitute.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

namespace {

const char* GetCapitalizedType(const FieldDescriptor* field) {
  switch (field->type()) {
    case FieldDescriptor::TYPE_INT32   : return "Int32"   ;
    case FieldDescriptor::TYPE_UINT32  : return "UInt32"  ;
    case FieldDescriptor::TYPE_SINT32  : return "SInt32"  ;
    case FieldDescriptor::TYPE_FIXED32 : return "Fixed32" ;
    case FieldDescriptor::TYPE_SFIXED32: return "SFixed32";
    case FieldDescriptor::TYPE_INT64   : return "Int64"   ;
    case FieldDescriptor::TYPE_UINT64  : return "UInt64"  ;
    case FieldDescriptor::TYPE_SINT64  : return "SInt64"  ;
    case FieldDescriptor::TYPE_FIXED64 : return "Fixed64" ;
    case FieldDescriptor::TYPE_SFIXED64: return "SFixed64";
    case FieldDescriptor::TYPE_FLOAT   : return "Float"   ;
    case FieldDescriptor::TYPE_DOUBLE  : return "Double"  ;
    case FieldDescriptor::TYPE_BOOL    : return "Bool"    ;
    case FieldDescriptor::TYPE_STRING  : return "String"  ;
    case FieldDescriptor::TYPE_BYTES   : return "Bytes"   ;
    case FieldDescriptor::TYPE_ENUM    : return "Enum"    ;
    case FieldDescriptor::TYPE_GROUP   : return "Group"   ;
    case FieldDescriptor::TYPE_MESSAGE : return "Message" ;

    // No default because we want the compiler to complain if any new
    // types are added.
  }

  GOOGLE_LOG(FATAL) << "Can't get here.";
  return NULL;
}

bool AllPrintableAscii(const string& text) {
  // Cannot use isprint() because it's locale-specific.  :(
  for (int i = 0; i < text.size(); i++) {
    if ((text[i] < 0x20) || text[i] >= 0x7F) {
      return false;
    }
  }
  return true;
}

string DefaultValue(const FieldDescriptor* field) {
  // Switch on cpp_type since we need to know which default_value_* method
  // of FieldDescriptor to call.
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32:
      return SimpleItoa(field->default_value_int32());
    case FieldDescriptor::CPPTYPE_UINT32:
      return SimpleItoa(field->default_value_uint32());
    case FieldDescriptor::CPPTYPE_INT64:
      return SimpleItoa(field->default_value_int64()) + "L";
    case FieldDescriptor::CPPTYPE_UINT64:
      return SimpleItoa(field->default_value_uint64()) +
             "UL";
    case FieldDescriptor::CPPTYPE_DOUBLE:
      return SimpleDtoa(field->default_value_double()) + "D";
    case FieldDescriptor::CPPTYPE_FLOAT:
      return SimpleFtoa(field->default_value_float()) + "F";
    case FieldDescriptor::CPPTYPE_BOOL:
      return field->default_value_bool() ? "true" : "false";
    case FieldDescriptor::CPPTYPE_STRING: {
      bool isBytes = field->type() == FieldDescriptor::TYPE_BYTES;

      if (!isBytes && AllPrintableAscii(field->default_value_string())) {
        // All chars are ASCII and printable.  In this case CEscape() works
        // fine (it will only escape quotes and backslashes).
        // Note:  If this "optimization" is removed, DescriptorProtos will
        //   no longer be able to initialize itself due to bootstrapping
        //   problems.
        return "\"" + CEscape(field->default_value_string()) + "\"";
      }

      if (isBytes && !field->has_default_value()) {
        return "pb::ByteString.Empty";
      }

      // Escaping strings correctly for Java and generating efficient
      // initializers for ByteStrings are both tricky.  We can sidestep the
      // whole problem by just grabbing the default value from the descriptor.
      // TODO(jonskeet): FIXME!
      return strings::Substitute(
        "(($0) $1.Descriptor.Fields[$2].DefaultValue)",
        isBytes ? "pb::ByteString" : "string",
        ClassName(field->containing_type()), field->index());
    }

    case FieldDescriptor::CPPTYPE_ENUM:
    case FieldDescriptor::CPPTYPE_MESSAGE:
      GOOGLE_LOG(FATAL) << "Can't get here.";
      return "";

    // No default because we want the compiler to complain if any new
    // types are added.
  }

  GOOGLE_LOG(FATAL) << "Can't get here.";
  return "";
}

void SetPrimitiveVariables(const FieldDescriptor* descriptor,
                           map<string, string>* variables) {
  (*variables)["name"] =
    UnderscoresToCamelCase(descriptor);
  (*variables)["capitalized_name"] =
    UnderscoresToCapitalizedCamelCase(descriptor);
  (*variables)["number"] = SimpleItoa(descriptor->number());
  (*variables)["type"] = MappedTypeName(GetMappedType(descriptor));
  (*variables)["default"] = DefaultValue(descriptor);
  (*variables)["capitalized_type"] = GetCapitalizedType(descriptor);
}

}  // namespace

// ===================================================================

PrimitiveFieldGenerator::
PrimitiveFieldGenerator(const FieldDescriptor* descriptor)
  : descriptor_(descriptor) {
  SetPrimitiveVariables(descriptor, &variables_);
}

PrimitiveFieldGenerator::~PrimitiveFieldGenerator() {}

void PrimitiveFieldGenerator::
GenerateMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "private bool has$capitalized_name$;\r\n"
    "private $type$ $name$_ = $default$;\r\n"
    "public bool Has$capitalized_name$ {\r\n"
    "  get { return has$capitalized_name$; }\r\n"
    "}\r\n"
    "public $type$ $capitalized_name$ {\r\n"
    "  get { return $name$_; }\r\n"
    "}\r\n");
}

void PrimitiveFieldGenerator::
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

void PrimitiveFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (other.Has$capitalized_name$) {\r\n"
    "  $capitalized_name$ = other.$capitalized_name$;\r\n"
    "}\r\n");
}

void PrimitiveFieldGenerator::
GenerateBuildingCode(io::Printer* printer) const {
  // Nothing to do here for primitive types.
}

void PrimitiveFieldGenerator::
GenerateParsingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "$capitalized_name$ = input.Read$capitalized_type$();\r\n");
}

void PrimitiveFieldGenerator::
GenerateSerializationCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (Has$capitalized_name$) {\r\n"
    "  output.Write$capitalized_type$($number$, $capitalized_name$);\r\n"
    "}\r\n");
}

void PrimitiveFieldGenerator::
GenerateSerializedSizeCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (Has$capitalized_name$) {\r\n"
    "  size += pb::CodedOutputStream.Compute$capitalized_type$Size($number$, $capitalized_name$);\r\n"
    "}\r\n");
}

// ===================================================================

RepeatedPrimitiveFieldGenerator::
RepeatedPrimitiveFieldGenerator(const FieldDescriptor* descriptor)
  : descriptor_(descriptor) {
  SetPrimitiveVariables(descriptor, &variables_);
}

RepeatedPrimitiveFieldGenerator::~RepeatedPrimitiveFieldGenerator() {}

void RepeatedPrimitiveFieldGenerator::
GenerateMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "private scg::IList<$type$> $name$_ = pbc::Lists<$type$>.Empty;\r\n"
    "public scg::IList<$type$> $capitalized_name$List {\r\n"
    "  get { return $name$_; }\r\n"   // note:  unmodifiable list
    "}\r\n"
    "public int $capitalized_name$Count {\r\n" // TODO(jonskeet): Remove?
    "  get { return $name$_.Count; }\r\n"
    "}\r\n"
    "public $type$ Get$capitalized_name$(int index) {\r\n" // TODO(jonskeet): Remove?
    "  return $name$_[index];\r\n"
    "}\r\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateBuilderMembers(io::Printer* printer) const {
  printer->Print(variables_,
    // Note:  We return an unmodifiable list because otherwise the caller
    //   could hold on to the returned list and modify it after the message
    //   has been built, thus mutating the message which is supposed to be
    //   immutable.
    "public scg::IList<$type$> $capitalized_name$List {\r\n"
    "  get { return pbc::Lists<$type$>.AsReadOnly(result.$name$_); }\r\n"
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

void RepeatedPrimitiveFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (other.$name$_.Count != 0) {\r\n"
    "  if (result.$name$_.Count == 0) {\r\n"
    "    result.$name$_ = new scg::List<$type$>();\r\n"
    "  }\r\n"
    "  base.AddRange(other.$name$_, result.$name$_);\r\n"
    "}\r\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateBuildingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "result.$name$_ = pbc::Lists<$type$>.AsReadOnly(result.$name$_);\r\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateParsingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "Add$capitalized_name$(input.Read$capitalized_type$());\r\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateSerializationCode(io::Printer* printer) const {
  printer->Print(variables_,
    "foreach ($type$ element in $capitalized_name$List) {\r\n"
    "  output.Write$capitalized_type$($number$, element);\r\n"
    "}\r\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateSerializedSizeCode(io::Printer* printer) const {
  printer->Print(variables_,
    "foreach ($type$ element in $capitalized_name$List) {\r\n"
    "  size += pb::CodedOutputStream\r\n"
    "    .Compute$capitalized_type$Size($number$, element);\r\n"
    "}\r\n");
}


}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
