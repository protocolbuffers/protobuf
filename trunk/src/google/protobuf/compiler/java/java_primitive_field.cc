// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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
#include <string>

#include <google/protobuf/compiler/java/java_primitive_field.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/compiler/java/java_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/stubs/substitute.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

namespace {

const char* PrimitiveTypeName(JavaType type) {
  switch (type) {
    case JAVATYPE_INT    : return "int";
    case JAVATYPE_LONG   : return "long";
    case JAVATYPE_FLOAT  : return "float";
    case JAVATYPE_DOUBLE : return "double";
    case JAVATYPE_BOOLEAN: return "boolean";
    case JAVATYPE_STRING : return "java.lang.String";
    case JAVATYPE_BYTES  : return "com.google.protobuf.ByteString";
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
        return "com.google.protobuf.ByteString.EMPTY";
      }

      // Escaping strings correctly for Java and generating efficient
      // initializers for ByteStrings are both tricky.  We can sidestep the
      // whole problem by just grabbing the default value from the descriptor.
      return strings::Substitute(
        "(($0) $1.getDescriptor().getFields().get($2).getDefaultValue())",
        isBytes ? "com.google.protobuf.ByteString" : "java.lang.String",
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
    "private boolean has$capitalized_name$;\n"
    "private $type$ $name$_ = $default$;\n"
    "public boolean has$capitalized_name$() { return has$capitalized_name$; }\n"
    "public $type$ get$capitalized_name$() { return $name$_; }\n");
}

void PrimitiveFieldGenerator::
GenerateBuilderMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "public boolean has$capitalized_name$() {\n"
    "  return result.has$capitalized_name$();\n"
    "}\n"
    "public $type$ get$capitalized_name$() {\n"
    "  return result.get$capitalized_name$();\n"
    "}\n"
    "public Builder set$capitalized_name$($type$ value) {\n"
    "  result.has$capitalized_name$ = true;\n"
    "  result.$name$_ = value;\n"
    "  return this;\n"
    "}\n"
    "public Builder clear$capitalized_name$() {\n"
    "  result.has$capitalized_name$ = false;\n"
    "  result.$name$_ = $default$;\n"
    "  return this;\n"
    "}\n");
}

void PrimitiveFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (other.has$capitalized_name$()) {\n"
    "  set$capitalized_name$(other.get$capitalized_name$());\n"
    "}\n");
}

void PrimitiveFieldGenerator::
GenerateBuildingCode(io::Printer* printer) const {
  // Nothing to do here for primitive types.
}

void PrimitiveFieldGenerator::
GenerateParsingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "set$capitalized_name$(input.read$capitalized_type$());\n");
}

void PrimitiveFieldGenerator::
GenerateSerializationCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (has$capitalized_name$()) {\n"
    "  output.write$capitalized_type$($number$, get$capitalized_name$());\n"
    "}\n");
}

void PrimitiveFieldGenerator::
GenerateSerializedSizeCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (has$capitalized_name$()) {\n"
    "  size += com.google.protobuf.CodedOutputStream\n"
    "    .compute$capitalized_type$Size($number$, get$capitalized_name$());\n"
    "}\n");
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
    "private java.util.List<$boxed_type$> $name$_ =\n"
    "  java.util.Collections.emptyList();\n"
    "public java.util.List<$boxed_type$> get$capitalized_name$List() {\n"
    "  return $name$_;\n"   // note:  unmodifiable list
    "}\n"
    "public int get$capitalized_name$Count() { return $name$_.size(); }\n"
    "public $type$ get$capitalized_name$(int index) {\n"
    "  return $name$_.get(index);\n"
    "}\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateBuilderMembers(io::Printer* printer) const {
  printer->Print(variables_,
    // Note:  We return an unmodifiable list because otherwise the caller
    //   could hold on to the returned list and modify it after the message
    //   has been built, thus mutating the message which is supposed to be
    //   immutable.
    "public java.util.List<$boxed_type$> get$capitalized_name$List() {\n"
    "  return java.util.Collections.unmodifiableList(result.$name$_);\n"
    "}\n"
    "public int get$capitalized_name$Count() {\n"
    "  return result.get$capitalized_name$Count();\n"
    "}\n"
    "public $type$ get$capitalized_name$(int index) {\n"
    "  return result.get$capitalized_name$(index);\n"
    "}\n"
    "public Builder set$capitalized_name$(int index, $type$ value) {\n"
    "  result.$name$_.set(index, value);\n"
    "  return this;\n"
    "}\n"
    "public Builder add$capitalized_name$($type$ value) {\n"
    "  if (result.$name$_.isEmpty()) {\n"
    "    result.$name$_ = new java.util.ArrayList<$boxed_type$>();\n"
    "  }\n"
    "  result.$name$_.add(value);\n"
    "  return this;\n"
    "}\n"
    "public Builder addAll$capitalized_name$(\n"
    "    java.lang.Iterable<? extends $boxed_type$> values) {\n"
    "  if (result.$name$_.isEmpty()) {\n"
    "    result.$name$_ = new java.util.ArrayList<$boxed_type$>();\n"
    "  }\n"
    "  super.addAll(values, result.$name$_);\n"
    "  return this;\n"
    "}\n"
    "public Builder clear$capitalized_name$() {\n"
    "  result.$name$_ = java.util.Collections.emptyList();\n"
    "  return this;\n"
    "}\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (!other.$name$_.isEmpty()) {\n"
    "  if (result.$name$_.isEmpty()) {\n"
    "    result.$name$_ = new java.util.ArrayList<$boxed_type$>();\n"
    "  }\n"
    "  result.$name$_.addAll(other.$name$_);\n"
    "}\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateBuildingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (result.$name$_ != java.util.Collections.EMPTY_LIST) {\n"
    "  result.$name$_ =\n"
    "    java.util.Collections.unmodifiableList(result.$name$_);\n"
    "}\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateParsingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "add$capitalized_name$(input.read$capitalized_type$());\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateSerializationCode(io::Printer* printer) const {
  printer->Print(variables_,
    "for ($type$ element : get$capitalized_name$List()) {\n"
    "  output.write$capitalized_type$($number$, element);\n"
    "}\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateSerializedSizeCode(io::Printer* printer) const {
  printer->Print(variables_,
    "for ($type$ element : get$capitalized_name$List()) {\n"
    "  size += com.google.protobuf.CodedOutputStream\n"
    "    .compute$capitalized_type$Size($number$, element);\n"
    "}\n");
}

string RepeatedPrimitiveFieldGenerator::GetBoxedType() const {
  return BoxedPrimitiveTypeName(GetJavaType(descriptor_));
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
