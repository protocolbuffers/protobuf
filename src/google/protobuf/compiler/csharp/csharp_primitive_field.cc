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

const char* PrimitiveTypeName(JavaType type) {
  switch (type) {
    case JAVATYPE_INT    : return "int";
    case JAVATYPE_LONG   : return "long";
    case JAVATYPE_FLOAT  : return "float";
    case JAVATYPE_DOUBLE : return "double";
    case JAVATYPE_BOOLEAN: return "boolean";
    case JAVATYPE_STRING : return "string";
    case JAVATYPE_BYTES  : return "pb::ByteString";
    case JAVATYPE_ENUM   : return NULL;
    case JAVATYPE_MESSAGE: return NULL;

    // No default because we want the compiler to complain if any new
    // JavaTypes are added.
  }

  GOOGLE_LOG(FATAL) << "Can't get here.";
  return NULL;
}

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
      // Need to print as a signed int since Java has no unsigned.
      return SimpleItoa(static_cast<int32>(field->default_value_uint32()));
    case FieldDescriptor::CPPTYPE_INT64:
      return SimpleItoa(field->default_value_int64()) + "L";
    case FieldDescriptor::CPPTYPE_UINT64:
      return SimpleItoa(static_cast<int64>(field->default_value_uint64())) +
             "L";
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
      return strings::Substitute(
        "(($0) $1.getDescriptor().getFields().get($2).getDefaultValue())",
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
  (*variables)["type"] = PrimitiveTypeName(GetJavaType(descriptor));
  (*variables)["boxed_type"] = BoxedPrimitiveTypeName(GetJavaType(descriptor));
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
    "public boolean Has$capitalized_name$ {\r\n"
    "  get { return has$capitalized_name$; }\r\n"
    "}\r\n"
    "public $type$ $capitalized_name$ {\r\n"
    "  get { return $name$_; }\r\n"
    "}\r\n");
}

void PrimitiveFieldGenerator::
GenerateBuilderMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "public boolean Has$capitalized_name$ {\r\n"
    "  get { return result.Has$capitalized_name$; }\r\n"
    "}\r\n"
    // TODO(jonskeet): Consider whether this is really the right pattern,
    // or whether we want a method returning a Builder. This allows for
    // object initializers.
    "public $type$ $capitalized_name$ {\r\n"
    "  get { return result.$capitalized_name$; }\r\n"
    "  set {\r\n"
    "    result.has$capitalized_name$ = true;\r\n"
    "    result.$name$_ = value;\r\n"
    "  }\r\n"
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
    "if (other.has$capitalized_name$()) {\r\n"
    "  set$capitalized_name$(other.get$capitalized_name$());\r\n"
    "}\r\n");
}

void PrimitiveFieldGenerator::
GenerateBuildingCode(io::Printer* printer) const {
  // Nothing to do here for primitive types.
}

void PrimitiveFieldGenerator::
GenerateParsingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "set$capitalized_name$(input.read$capitalized_type$());\r\n");
}

void PrimitiveFieldGenerator::
GenerateSerializationCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (has$capitalized_name$()) {\r\n"
    "  output.write$capitalized_type$($number$, get$capitalized_name$());\r\n"
    "}\r\n");
}

void PrimitiveFieldGenerator::
GenerateSerializedSizeCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (has$capitalized_name$()) {\r\n"
    "  size += pb::CodedOutputStream\r\n"
    "    .compute$capitalized_type$Size($number$, get$capitalized_name$());\r\n"
    "}\r\n");
}

string PrimitiveFieldGenerator::GetBoxedType() const {
  return BoxedPrimitiveTypeName(GetJavaType(descriptor_));
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
    "private java.util.List<$boxed_type$> $name$_ =\r\n"
    "  java.util.Collections.emptyList();\r\n"
    "public java.util.List<$boxed_type$> get$capitalized_name$List() {\r\n"
    "  return $name$_;\r\n"   // note:  unmodifiable list
    "}\r\n"
    "public int get$capitalized_name$Count() { return $name$_.size(); }\r\n"
    "public $type$ get$capitalized_name$(int index) {\r\n"
    "  return $name$_.get(index);\r\n"
    "}\r\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateBuilderMembers(io::Printer* printer) const {
  printer->Print(variables_,
    // Note:  We return an unmodifiable list because otherwise the caller
    //   could hold on to the returned list and modify it after the message
    //   has been built, thus mutating the message which is supposed to be
    //   immutable.
    "public java.util.List<$boxed_type$> get$capitalized_name$List() {\r\n"
    "  return java.util.Collections.unmodifiableList(result.$name$_);\r\n"
    "}\r\n"
    "public int get$capitalized_name$Count() {\r\n"
    "  return result.get$capitalized_name$Count();\r\n"
    "}\r\n"
    "public $type$ get$capitalized_name$(int index) {\r\n"
    "  return result.get$capitalized_name$(index);\r\n"
    "}\r\n"
    "public Builder set$capitalized_name$(int index, $type$ value) {\r\n"
    "  result.$name$_.set(index, value);\r\n"
    "  return this;\r\n"
    "}\r\n"
    "public Builder add$capitalized_name$($type$ value) {\r\n"
    "  if (result.$name$_.isEmpty()) {\r\n"
    "    result.$name$_ = new java.util.ArrayList<$boxed_type$>();\r\n"
    "  }\r\n"
    "  result.$name$_.add(value);\r\n"
    "  return this;\r\n"
    "}\r\n"
    "public Builder addAll$capitalized_name$(\r\n"
    "    java.lang.Iterable<? extends $boxed_type$> values) {\r\n"
    "  if (result.$name$_.isEmpty()) {\r\n"
    "    result.$name$_ = new java.util.ArrayList<$boxed_type$>();\r\n"
    "  }\r\n"
    "  super.addAll(values, result.$name$_);\r\n"
    "  return this;\r\n"
    "}\r\n"
    "public Builder clear$capitalized_name$() {\r\n"
    "  result.$name$_ = java.util.Collections.emptyList();\r\n"
    "  return this;\r\n"
    "}\r\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (!other.$name$_.isEmpty()) {\r\n"
    "  if (result.$name$_.isEmpty()) {\r\n"
    "    result.$name$_ = new java.util.ArrayList<$boxed_type$>();\r\n"
    "  }\r\n"
    "  result.$name$_.addAll(other.$name$_);\r\n"
    "}\r\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateBuildingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (result.$name$_ != java.util.Collections.EMPTY_LIST) {\r\n"
    "  result.$name$_ =\r\n"
    "    java.util.Collections.unmodifiableList(result.$name$_);\r\n"
    "}\r\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateParsingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "add$capitalized_name$(input.read$capitalized_type$());\r\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateSerializationCode(io::Printer* printer) const {
  printer->Print(variables_,
    "for ($type$ element : get$capitalized_name$List()) {\r\n"
    "  output.write$capitalized_type$($number$, element);\r\n"
    "}\r\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateSerializedSizeCode(io::Printer* printer) const {
  printer->Print(variables_,
    "for ($type$ element : get$capitalized_name$List()) {\r\n"
    "  size += pb::CodedOutputStream\r\n"
    "    .compute$capitalized_type$Size($number$, element);\r\n"
    "}\r\n");
}

string RepeatedPrimitiveFieldGenerator::GetBoxedType() const {
  return BoxedPrimitiveTypeName(GetJavaType(descriptor_));
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
