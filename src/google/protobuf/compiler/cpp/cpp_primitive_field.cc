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

#include <google/protobuf/compiler/cpp/cpp_primitive_field.h>
#include <google/protobuf/compiler/cpp/cpp_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/wire_format_inl.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

using internal::WireFormat;

namespace {

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
    case FieldDescriptor::TYPE_FIXED32 : return WireFormat::kFixed32Size;
    case FieldDescriptor::TYPE_FIXED64 : return WireFormat::kFixed64Size;
    case FieldDescriptor::TYPE_SFIXED32: return WireFormat::kSFixed32Size;
    case FieldDescriptor::TYPE_SFIXED64: return WireFormat::kSFixed64Size;
    case FieldDescriptor::TYPE_FLOAT   : return WireFormat::kFloatSize;
    case FieldDescriptor::TYPE_DOUBLE  : return WireFormat::kDoubleSize;

    case FieldDescriptor::TYPE_BOOL    : return WireFormat::kBoolSize;
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

string DefaultValue(const FieldDescriptor* field) {
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32:
      return SimpleItoa(field->default_value_int32());
    case FieldDescriptor::CPPTYPE_UINT32:
      return SimpleItoa(field->default_value_uint32()) + "u";
    case FieldDescriptor::CPPTYPE_INT64:
      return "GOOGLE_LONGLONG(" + SimpleItoa(field->default_value_int64()) + ")";
    case FieldDescriptor::CPPTYPE_UINT64:
      return "GOOGLE_ULONGLONG(" + SimpleItoa(field->default_value_uint64())+ ")";
    case FieldDescriptor::CPPTYPE_DOUBLE:
      return SimpleDtoa(field->default_value_double());
    case FieldDescriptor::CPPTYPE_FLOAT:
      return SimpleFtoa(field->default_value_float());
    case FieldDescriptor::CPPTYPE_BOOL:
      return field->default_value_bool() ? "true" : "false";

    case FieldDescriptor::CPPTYPE_ENUM:
    case FieldDescriptor::CPPTYPE_STRING:
    case FieldDescriptor::CPPTYPE_MESSAGE:
      GOOGLE_LOG(FATAL) << "Shouldn't get here.";
      return "";
  }
  // Can't actually get here; make compiler happy.  (We could add a default
  // case above but then we wouldn't get the nice compiler warning when a
  // new type is added.)
  return "";
}

// TODO(kenton):  Factor out a "SetCommonFieldVariables()" to get rid of
//   repeat code between this and the other field types.
void SetPrimitiveVariables(const FieldDescriptor* descriptor,
                           map<string, string>* variables) {
  (*variables)["name"] = FieldName(descriptor);
  (*variables)["type"] = PrimitiveTypeName(descriptor->cpp_type());
  (*variables)["default"] = DefaultValue(descriptor);
  (*variables)["index"] = SimpleItoa(descriptor->index());
  (*variables)["number"] = SimpleItoa(descriptor->number());
  (*variables)["classname"] = ClassName(FieldScope(descriptor), false);
  (*variables)["declared_type"] = DeclaredTypeMethodName(descriptor->type());
  (*variables)["tag_size"] = SimpleItoa(
    WireFormat::TagSize(descriptor->number(), descriptor->type()));

  int fixed_size = FixedSize(descriptor->type());
  if (fixed_size != -1) {
    (*variables)["fixed_size"] = SimpleItoa(fixed_size);
  }
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
GeneratePrivateMembers(io::Printer* printer) const {
  printer->Print(variables_, "$type$ $name$_;\n");
}

void PrimitiveFieldGenerator::
GenerateAccessorDeclarations(io::Printer* printer) const {
  printer->Print(variables_,
    "inline $type$ $name$() const;\n"
    "inline void set_$name$($type$ value);\n");
}

void PrimitiveFieldGenerator::
GenerateInlineAccessorDefinitions(io::Printer* printer) const {
  printer->Print(variables_,
    "inline $type$ $classname$::$name$() const {\n"
    "  return $name$_;\n"
    "}\n"
    "inline void $classname$::set_$name$($type$ value) {\n"
    "  _set_bit($index$);\n"
    "  $name$_ = value;\n"
    "}\n");
}

void PrimitiveFieldGenerator::
GenerateClearingCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$_ = $default$;\n");
}

void PrimitiveFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_, "set_$name$(from.$name$());\n");
}

void PrimitiveFieldGenerator::
GenerateInitializer(io::Printer* printer) const {
  printer->Print(variables_, ",\n$name$_($default$)");
}

void PrimitiveFieldGenerator::
GenerateMergeFromCodedStream(io::Printer* printer) const {
  printer->Print(variables_,
    "DO_(::google::protobuf::internal::WireFormat::Read$declared_type$(\n"
    "      input, &$name$_));\n"
    "_set_bit($index$);\n");
}

void PrimitiveFieldGenerator::
GenerateSerializeWithCachedSizes(io::Printer* printer) const {
  printer->Print(variables_,
    "DO_(::google::protobuf::internal::WireFormat::Write$declared_type$("
      "$number$, this->$name$(), output));\n");
}

void PrimitiveFieldGenerator::
GenerateByteSize(io::Printer* printer) const {
  int fixed_size = FixedSize(descriptor_->type());
  if (fixed_size == -1) {
    printer->Print(variables_,
      "total_size += $tag_size$ +\n"
      "  ::google::protobuf::internal::WireFormat::$declared_type$Size(\n"
      "    this->$name$());\n");
  } else {
    printer->Print(variables_,
      "total_size += $tag_size$ + $fixed_size$;\n");
  }
}

// ===================================================================

RepeatedPrimitiveFieldGenerator::
RepeatedPrimitiveFieldGenerator(const FieldDescriptor* descriptor)
  : descriptor_(descriptor) {
  SetPrimitiveVariables(descriptor, &variables_);
}

RepeatedPrimitiveFieldGenerator::~RepeatedPrimitiveFieldGenerator() {}

void RepeatedPrimitiveFieldGenerator::
GeneratePrivateMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "::google::protobuf::RepeatedField< $type$ > $name$_;\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateAccessorDeclarations(io::Printer* printer) const {
  printer->Print(variables_,
    "inline const ::google::protobuf::RepeatedField< $type$ >& $name$() const;\n"
    "inline ::google::protobuf::RepeatedField< $type$ >* mutable_$name$();\n"
    "inline $type$ $name$(int index) const;\n"
    "inline void set_$name$(int index, $type$ value);\n"
    "inline void add_$name$($type$ value);\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateInlineAccessorDefinitions(io::Printer* printer) const {
  printer->Print(variables_,
    "inline const ::google::protobuf::RepeatedField< $type$ >&\n"
    "$classname$::$name$() const {\n"
    "  return $name$_;\n"
    "}\n"
    "inline ::google::protobuf::RepeatedField< $type$ >*\n"
    "$classname$::mutable_$name$() {\n"
    "  return &$name$_;\n"
    "}\n"
    "inline $type$ $classname$::$name$(int index) const {\n"
    "  return $name$_.Get(index);\n"
    "}\n"
    "inline void $classname$::set_$name$(int index, $type$ value) {\n"
    "  $name$_.Set(index, value);\n"
    "}\n"
    "inline void $classname$::add_$name$($type$ value) {\n"
    "  $name$_.Add(value);\n"
    "}\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateClearingCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$_.Clear();\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$_.MergeFrom(from.$name$_);\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateInitializer(io::Printer* printer) const {
  // Not needed for repeated fields.
}

void RepeatedPrimitiveFieldGenerator::
GenerateMergeFromCodedStream(io::Printer* printer) const {
  printer->Print(variables_,
    "$type$ value;\n"
    "DO_(::google::protobuf::internal::WireFormat::Read$declared_type$(input, &value));\n"
    "add_$name$(value);\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateSerializeWithCachedSizes(io::Printer* printer) const {
  printer->Print(variables_,
    "DO_(::google::protobuf::internal::WireFormat::Write$declared_type$("
      "$number$, this->$name$(i), output));\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateByteSize(io::Printer* printer) const {
  int fixed_size = FixedSize(descriptor_->type());
  if (fixed_size == -1) {
    printer->Print(variables_,
      "total_size += $tag_size$ * $name$_size();\n"
      "for (int i = 0; i < $name$_size(); i++) {\n"
      "  total_size += ::google::protobuf::internal::WireFormat::$declared_type$Size(\n"
      "    this->$name$(i));\n"
      "}\n");
  } else {
    printer->Print(variables_,
      "total_size += ($tag_size$ + $fixed_size$) * $name$_size();\n");
  }
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
