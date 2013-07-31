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
#include <math.h>
#include <string>

#include <google/protobuf/compiler/javanano/javanano_primitive_field.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/compiler/javanano/javanano_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/stubs/substitute.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace javanano {

using internal::WireFormat;
using internal::WireFormatLite;

namespace {

const char* PrimitiveTypeName(JavaType type) {
  switch (type) {
    case JAVATYPE_INT    : return "int";
    case JAVATYPE_LONG   : return "long";
    case JAVATYPE_FLOAT  : return "float";
    case JAVATYPE_DOUBLE : return "double";
    case JAVATYPE_BOOLEAN: return "boolean";
    case JAVATYPE_STRING : return "java.lang.String";
    case JAVATYPE_BYTES  : return "byte[]";
    case JAVATYPE_ENUM   : return NULL;
    case JAVATYPE_MESSAGE: return NULL;

    // No default because we want the compiler to complain if any new
    // JavaTypes are added.
  }

  GOOGLE_LOG(FATAL) << "Can't get here.";
  return NULL;
}

bool IsReferenceType(JavaType type) {
  switch (type) {
    case JAVATYPE_INT    : return false;
    case JAVATYPE_LONG   : return false;
    case JAVATYPE_FLOAT  : return false;
    case JAVATYPE_DOUBLE : return false;
    case JAVATYPE_BOOLEAN: return false;
    case JAVATYPE_STRING : return true;
    case JAVATYPE_BYTES  : return true;
    case JAVATYPE_ENUM   : return false;
    case JAVATYPE_MESSAGE: return true;

    // No default because we want the compiler to complain if any new
    // JavaTypes are added.
  }

  GOOGLE_LOG(FATAL) << "Can't get here.";
  return false;
}

bool IsArrayType(JavaType type) {
  switch (type) {
    case JAVATYPE_INT    : return false;
    case JAVATYPE_LONG   : return false;
    case JAVATYPE_FLOAT  : return false;
    case JAVATYPE_DOUBLE : return false;
    case JAVATYPE_BOOLEAN: return false;
    case JAVATYPE_STRING : return false;
    case JAVATYPE_BYTES  : return true;
    case JAVATYPE_ENUM   : return false;
    case JAVATYPE_MESSAGE: return false;

    // No default because we want the compiler to complain if any new
    // JavaTypes are added.
  }

  GOOGLE_LOG(FATAL) << "Can't get here.";
  return false;
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

// For encodings with fixed sizes, returns that size in bytes.  Otherwise
// returns -1.
int FixedSize(FieldDescriptor::Type type) {
  switch (type) {
    case FieldDescriptor::TYPE_INT32   : return -1;
    case FieldDescriptor::TYPE_INT64   : return -1;
    case FieldDescriptor::TYPE_UINT32  : return -1;
    case FieldDescriptor::TYPE_UINT64  : return -1;
    case FieldDescriptor::TYPE_SINT32  : return -1;
    case FieldDescriptor::TYPE_SINT64  : return -1;
    case FieldDescriptor::TYPE_FIXED32 : return WireFormatLite::kFixed32Size;
    case FieldDescriptor::TYPE_FIXED64 : return WireFormatLite::kFixed64Size;
    case FieldDescriptor::TYPE_SFIXED32: return WireFormatLite::kSFixed32Size;
    case FieldDescriptor::TYPE_SFIXED64: return WireFormatLite::kSFixed64Size;
    case FieldDescriptor::TYPE_FLOAT   : return WireFormatLite::kFloatSize;
    case FieldDescriptor::TYPE_DOUBLE  : return WireFormatLite::kDoubleSize;

    case FieldDescriptor::TYPE_BOOL    : return WireFormatLite::kBoolSize;
    case FieldDescriptor::TYPE_ENUM    : return -1;

    case FieldDescriptor::TYPE_STRING  : return -1;
    case FieldDescriptor::TYPE_BYTES   : return -1;
    case FieldDescriptor::TYPE_GROUP   : return -1;
    case FieldDescriptor::TYPE_MESSAGE : return -1;

    // No default because we want the compiler to complain if any new
    // types are added.
  }
  GOOGLE_LOG(FATAL) << "Can't get here.";
  return -1;
}

// Returns true if the field has a default value equal to NaN.
bool IsDefaultNaN(const FieldDescriptor* field) {
  switch (field->type()) {
    case FieldDescriptor::TYPE_INT32   : return false;
    case FieldDescriptor::TYPE_UINT32  : return false;
    case FieldDescriptor::TYPE_SINT32  : return false;
    case FieldDescriptor::TYPE_FIXED32 : return false;
    case FieldDescriptor::TYPE_SFIXED32: return false;
    case FieldDescriptor::TYPE_INT64   : return false;
    case FieldDescriptor::TYPE_UINT64  : return false;
    case FieldDescriptor::TYPE_SINT64  : return false;
    case FieldDescriptor::TYPE_FIXED64 : return false;
    case FieldDescriptor::TYPE_SFIXED64: return false;
    case FieldDescriptor::TYPE_FLOAT   :
      return isnan(field->default_value_float());
    case FieldDescriptor::TYPE_DOUBLE  :
      return isnan(field->default_value_double());
    case FieldDescriptor::TYPE_BOOL    : return false;
    case FieldDescriptor::TYPE_STRING  : return false;
    case FieldDescriptor::TYPE_BYTES   : return false;
    case FieldDescriptor::TYPE_ENUM    : return false;
    case FieldDescriptor::TYPE_GROUP   : return false;
    case FieldDescriptor::TYPE_MESSAGE : return false;

    // No default because we want the compiler to complain if any new
    // types are added.
  }

  GOOGLE_LOG(FATAL) << "Can't get here.";
  return false;
}

// Return true if the type is a that has variable length
// for instance String's.
bool IsVariableLenType(JavaType type) {
  switch (type) {
    case JAVATYPE_INT    : return false;
    case JAVATYPE_LONG   : return false;
    case JAVATYPE_FLOAT  : return false;
    case JAVATYPE_DOUBLE : return false;
    case JAVATYPE_BOOLEAN: return false;
    case JAVATYPE_STRING : return true;
    case JAVATYPE_BYTES  : return true;
    case JAVATYPE_ENUM   : return false;
    case JAVATYPE_MESSAGE: return true;

    // No default because we want the compiler to complain if any new
    // JavaTypes are added.
  }

  GOOGLE_LOG(FATAL) << "Can't get here.";
  return false;
}

bool AllAscii(const string& text) {
  for (int i = 0; i < text.size(); i++) {
    if ((text[i] & 0x80) != 0) {
      return false;
    }
  }
  return true;
}

void SetPrimitiveVariables(const FieldDescriptor* descriptor, const Params params,
                           map<string, string>* variables) {
  (*variables)["name"] =
    RenameJavaKeywords(UnderscoresToCamelCase(descriptor));
  (*variables)["capitalized_name"] =
    RenameJavaKeywords(UnderscoresToCapitalizedCamelCase(descriptor));
  (*variables)["number"] = SimpleItoa(descriptor->number());
  (*variables)["type"] = PrimitiveTypeName(GetJavaType(descriptor));
  (*variables)["default"] = DefaultValue(params, descriptor);
  (*variables)["default_constant"] = FieldDefaultConstantName(descriptor);
  // For C++-string types (string and bytes), we might need to have
  // the generated code do the unicode decoding (see comments in
  // InternalNano.java for gory details.). We would like to do this
  // once into a "private static final" field and re-use that from
  // then on.
  if (descriptor->cpp_type() == FieldDescriptor::CPPTYPE_STRING &&
      !descriptor->default_value_string().empty()) {
    string default_value;
    if (descriptor->type() == FieldDescriptor::TYPE_BYTES) {
      default_value = strings::Substitute(
          "com.google.protobuf.nano.InternalNano.bytesDefaultValue(\"$0\")",
          CEscape(descriptor->default_value_string()));
    } else {
      if (AllAscii(descriptor->default_value_string())) {
        // All chars are ASCII.  In this case CEscape() works fine.
        default_value = "\"" + CEscape(descriptor->default_value_string()) + "\"";
      } else {
        default_value = strings::Substitute(
            "com.google.protobuf.nano.InternalNano.stringDefaultValue(\"$0\")",
            CEscape(descriptor->default_value_string()));
      }
    }
    (*variables)["default_constant_value"] = default_value;
  }
  (*variables)["boxed_type"] = BoxedPrimitiveTypeName(GetJavaType(descriptor));
  (*variables)["capitalized_type"] = GetCapitalizedType(descriptor);
  (*variables)["tag"] = SimpleItoa(WireFormat::MakeTag(descriptor));
  (*variables)["tag_size"] = SimpleItoa(
      WireFormat::TagSize(descriptor->number(), descriptor->type()));
  if (IsReferenceType(GetJavaType(descriptor))) {
    (*variables)["null_check"] =
        "  if (value == null) {\n"
        "    throw new NullPointerException();\n"
        "  }\n";
  } else {
    (*variables)["null_check"] = "";
  }
  int fixed_size = FixedSize(descriptor->type());
  if (fixed_size != -1) {
    (*variables)["fixed_size"] = SimpleItoa(fixed_size);
  }
  (*variables)["message_name"] = descriptor->containing_type()->name();
  (*variables)["empty_array_name"] = EmptyArrayName(params, descriptor);
}
}  // namespace

// ===================================================================

PrimitiveFieldGenerator::
PrimitiveFieldGenerator(const FieldDescriptor* descriptor, const Params& params)
  : FieldGenerator(params), descriptor_(descriptor) {
  SetPrimitiveVariables(descriptor, params, &variables_);
}

PrimitiveFieldGenerator::~PrimitiveFieldGenerator() {}

void PrimitiveFieldGenerator::
GenerateMembers(io::Printer* printer) const {
  if (variables_.find("default_constant_value") != variables_.end()) {
    // Those primitive types that need a saved default.
    printer->Print(variables_,
      "private static final $type$ $default_constant$ = $default_constant_value$;\n");
    if (descriptor_->type() == FieldDescriptor::TYPE_BYTES) {
      printer->Print(variables_,
        "public $type$ $name$ = $default$.clone();\n");
    } else {
      printer->Print(variables_,
        "public $type$ $name$ = $default$;\n");
    }
  } else {
    printer->Print(variables_,
      "public $type$ $name$ = $default$;\n");
  }

  if (params_.generate_has()) {
    printer->Print(variables_,
      "public boolean has$capitalized_name$ = false;\n");
  }
}

void PrimitiveFieldGenerator::
GenerateParsingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "this.$name$ = input.read$capitalized_type$();\n");

  if (params_.generate_has()) {
    printer->Print(variables_,
      "has$capitalized_name$ = true;\n");
  }
}

void PrimitiveFieldGenerator::
GenerateSerializationConditional(io::Printer* printer) const {
  if (params_.generate_has()) {
    printer->Print(variables_,
      "if (has$capitalized_name$ || ");
  } else {
    printer->Print(variables_,
      "if (");
  }
  if (IsArrayType(GetJavaType(descriptor_))) {
    printer->Print(variables_,
      "!java.util.Arrays.equals(this.$name$, $default$)) {\n");
  } else if (IsReferenceType(GetJavaType(descriptor_))) {
    printer->Print(variables_,
      "!this.$name$.equals($default$)) {\n");
  } else if (IsDefaultNaN(descriptor_)) {
    printer->Print(variables_,
      "!$capitalized_type$.isNaN(this.$name$)) {\n");
  } else {
    printer->Print(variables_,
      "this.$name$ != $default$) {\n");
  }
}

void PrimitiveFieldGenerator::
GenerateSerializationCode(io::Printer* printer) const {
  if (descriptor_->is_required()) {
    printer->Print(variables_,
      "output.write$capitalized_type$($number$, this.$name$);\n");
  } else {
    GenerateSerializationConditional(printer);
    printer->Print(variables_,
      "  output.write$capitalized_type$($number$, this.$name$);\n"
      "}\n");
  }
}

void PrimitiveFieldGenerator::
GenerateSerializedSizeCode(io::Printer* printer) const {
  if (descriptor_->is_required()) {
    printer->Print(variables_,
      "size += com.google.protobuf.nano.CodedOutputByteBufferNano\n"
      "    .compute$capitalized_type$Size($number$, this.$name$);\n");
  } else {
    GenerateSerializationConditional(printer);
    printer->Print(variables_,
      "  size += com.google.protobuf.nano.CodedOutputByteBufferNano\n"
      "      .compute$capitalized_type$Size($number$, this.$name$);\n"
      "}\n");
  }
}

string PrimitiveFieldGenerator::GetBoxedType() const {
  return BoxedPrimitiveTypeName(GetJavaType(descriptor_));
}

// ===================================================================

RepeatedPrimitiveFieldGenerator::
RepeatedPrimitiveFieldGenerator(const FieldDescriptor* descriptor, const Params& params)
  : FieldGenerator(params), descriptor_(descriptor) {
  SetPrimitiveVariables(descriptor, params, &variables_);
}

RepeatedPrimitiveFieldGenerator::~RepeatedPrimitiveFieldGenerator() {}

void RepeatedPrimitiveFieldGenerator::
GenerateMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "public $type$[] $name$ = $default$;\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateParsingCode(io::Printer* printer) const {
  // First, figure out the length of the array, then parse.
  if (descriptor_->options().packed()) {
    printer->Print(variables_,
      "int length = input.readRawVarint32();\n"
      "int limit = input.pushLimit(length);\n"
      "// First pass to compute array length.\n"
      "int arrayLength = 0;\n"
      "int startPos = input.getPosition();\n"
      "while (input.getBytesUntilLimit() > 0) {\n"
      "  input.read$capitalized_type$();\n"
      "  arrayLength++;\n"
      "}\n"
      "input.rewindToPosition(startPos);\n"
      "this.$name$ = new $type$[arrayLength];\n"
      "for (int i = 0; i < arrayLength; i++) {\n"
      "  this.$name$[i] = input.read$capitalized_type$();\n"
      "}\n"
      "input.popLimit(limit);\n");
  } else {
    printer->Print(variables_,
      "int arrayLength = com.google.protobuf.nano.WireFormatNano.getRepeatedFieldArrayLength(input, $tag$);\n"
      "int i = this.$name$.length;\n");

    if (GetJavaType(descriptor_) == JAVATYPE_BYTES) {
      printer->Print(variables_,
        "byte[][] newArray = new byte[i + arrayLength][];\n"
        "System.arraycopy(this.$name$, 0, newArray, 0, i);\n"
        "this.$name$ = newArray;\n");
    } else {
      printer->Print(variables_,
        "$type$[] newArray = new $type$[i + arrayLength];\n"
        "System.arraycopy(this.$name$, 0, newArray, 0, i);\n"
        "this.$name$ = newArray;\n");
    }
    printer->Print(variables_,
      "for (; i < this.$name$.length - 1; i++) {\n"
      "  this.$name$[i] = input.read$capitalized_type$();\n"
      "  input.readTag();\n"
      "}\n"
      "// Last one without readTag.\n"
      "this.$name$[i] = input.read$capitalized_type$();\n");
  }
}

void RepeatedPrimitiveFieldGenerator::
GenerateRepeatedDataSizeCode(io::Printer* printer) const {
  // Creates a variable dataSize and puts the serialized size in
  // there.
  if (FixedSize(descriptor_->type()) == -1) {
    printer->Print(variables_,
      "int dataSize = 0;\n"
      "for ($type$ element : this.$name$) {\n"
      "  dataSize += com.google.protobuf.nano.CodedOutputByteBufferNano\n"
      "    .compute$capitalized_type$SizeNoTag(element);\n"
      "}\n");
  } else {
    printer->Print(variables_,
      "int dataSize = $fixed_size$ * this.$name$.length;\n");
  }
}

void RepeatedPrimitiveFieldGenerator::
GenerateSerializationCode(io::Printer* printer) const {
  if (descriptor_->options().packed()) {
    printer->Print(variables_,
      "if (this.$name$.length > 0) {\n");
    printer->Indent();
    GenerateRepeatedDataSizeCode(printer);
    printer->Outdent();
    printer->Print(variables_,
      "  output.writeRawVarint32($tag$);\n"
      "  output.writeRawVarint32(dataSize);\n"
      "}\n");
    printer->Print(variables_,
      "for ($type$ element : this.$name$) {\n"
      "  output.write$capitalized_type$NoTag(element);\n"
      "}\n");
  } else {
    printer->Print(variables_,
      "for ($type$ element : this.$name$) {\n"
      "  output.write$capitalized_type$($number$, element);\n"
      "}\n");
  }
}

void RepeatedPrimitiveFieldGenerator::
GenerateSerializedSizeCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (this.$name$.length > 0) {\n");
  printer->Indent();

  GenerateRepeatedDataSizeCode(printer);

  printer->Print(
    "size += dataSize;\n");
  if (descriptor_->options().packed()) {
    printer->Print(variables_,
      "size += $tag_size$;\n"
      "size += com.google.protobuf.nano.CodedOutputByteBufferNano\n"
      "  .computeRawVarint32Size(dataSize);\n");
  } else {
    printer->Print(variables_,
        "size += $tag_size$ * this.$name$.length;\n");
  }

  printer->Outdent();

  printer->Print(
    "}\n");
}

string RepeatedPrimitiveFieldGenerator::GetBoxedType() const {
  return BoxedPrimitiveTypeName(GetJavaType(descriptor_));
}

}  // namespace javanano
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
