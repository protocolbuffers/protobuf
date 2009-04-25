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
GenerateSwappingCode(io::Printer* printer) const {
  printer->Print(variables_, "std::swap($name$_, other->$name$_);\n");
}

void PrimitiveFieldGenerator::
GenerateConstructorCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$_ = $default$;\n");
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
    "::google::protobuf::internal::WireFormat::Write$declared_type$("
      "$number$, this->$name$(), output);\n");
}

void PrimitiveFieldGenerator::
GenerateSerializeWithCachedSizesToArray(io::Printer* printer) const {
  printer->Print(variables_,
    "target = ::google::protobuf::internal::WireFormat::Write$declared_type$ToArray("
      "$number$, this->$name$(), target);\n");
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
  if (descriptor_->options().packed() &&
      descriptor_->file()->options().optimize_for() == FileOptions::SPEED) {
    printer->Print(variables_,
      "mutable int _$name$_cached_byte_size_;\n");
  }
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
GenerateSwappingCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$_.Swap(&other->$name$_);\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateConstructorCode(io::Printer* printer) const {
  // Not needed for repeated fields.
}

void RepeatedPrimitiveFieldGenerator::
GenerateMergeFromCodedStream(io::Printer* printer) const {
  if (descriptor_->options().packed()) {
    printer->Print("{\n");
    printer->Indent();
    printer->Print(variables_,
      "::google::protobuf::uint32 length;\n"
      "DO_(input->ReadVarint32(&length));\n"
      "::google::protobuf::io::CodedInputStream::Limit limit = "
          "input->PushLimit(length);\n"
      "while (input->BytesUntilLimit() > 0) {\n"
      "  $type$ value;\n"
      "  DO_(::google::protobuf::internal::WireFormat::Read$declared_type$("
        "input, &value));\n"
      "  add_$name$(value);\n"
      "}\n"
      "input->PopLimit(limit);\n");
    printer->Outdent();
    printer->Print("}\n");
  } else {
    printer->Print(variables_,
      "$type$ value;\n"
      "DO_(::google::protobuf::internal::WireFormat::Read$declared_type$("
        "input, &value));\n"
      "add_$name$(value);\n");
  }
}

void RepeatedPrimitiveFieldGenerator::
GenerateSerializeWithCachedSizes(io::Printer* printer) const {
  if (descriptor_->options().packed()) {
    // Write the tag and the size.
    printer->Print(variables_,
      "if (this->$name$_size() > 0) {\n"
      "  ::google::protobuf::internal::WireFormat::WriteTag("
          "$number$, "
          "::google::protobuf::internal::WireFormat::WIRETYPE_LENGTH_DELIMITED, "
          "output);\n"
      "  output->WriteVarint32(_$name$_cached_byte_size_);\n"
      "}\n");
  }
  printer->Print(variables_,
      "for (int i = 0; i < this->$name$_size(); i++) {\n");
  if (descriptor_->options().packed()) {
    printer->Print(variables_,
      "  ::google::protobuf::internal::WireFormat::Write$declared_type$NoTag("
          "this->$name$(i), output);\n");
  } else {
    printer->Print(variables_,
      "  ::google::protobuf::internal::WireFormat::Write$declared_type$("
          "$number$, this->$name$(i), output);\n");
  }
  printer->Print("}\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateSerializeWithCachedSizesToArray(io::Printer* printer) const {
  if (descriptor_->options().packed()) {
    // Write the tag and the size.
    printer->Print(variables_,
      "if (this->$name$_size() > 0) {\n"
      "  target = ::google::protobuf::internal::WireFormat::WriteTagToArray("
          "$number$, "
          "::google::protobuf::internal::WireFormat::WIRETYPE_LENGTH_DELIMITED, "
          "target);\n"
      "  target = ::google::protobuf::io::CodedOutputStream::WriteVarint32ToArray("
          "_$name$_cached_byte_size_, target);\n"
      "}\n");
  }
  printer->Print(variables_,
      "for (int i = 0; i < this->$name$_size(); i++) {\n");
  if (descriptor_->options().packed()) {
    printer->Print(variables_,
      "  target = ::google::protobuf::internal::WireFormat::"
          "Write$declared_type$NoTagToArray("
          "this->$name$(i), target);\n");
  } else {
    printer->Print(variables_,
      "  target = ::google::protobuf::internal::WireFormat::"
          "Write$declared_type$ToArray("
          "$number$, this->$name$(i), target);\n");
  }
  printer->Print("}\n");
}

void RepeatedPrimitiveFieldGenerator::
GenerateByteSize(io::Printer* printer) const {
  printer->Print(variables_,
    "{\n"
    "  int data_size = 0;\n");
  printer->Indent();
  int fixed_size = FixedSize(descriptor_->type());
  if (fixed_size == -1) {
    printer->Print(variables_,
      "for (int i = 0; i < this->$name$_size(); i++) {\n"
      "  data_size += ::google::protobuf::internal::WireFormat::$declared_type$Size(\n"
      "    this->$name$(i));\n"
      "}\n");
  } else {
    printer->Print(variables_,
      "data_size = $fixed_size$ * this->$name$_size();\n");
  }

  if (descriptor_->options().packed()) {
    printer->Print(variables_,
      "if (data_size > 0) {\n"
      "  total_size += $tag_size$ + "
        "::google::protobuf::internal::WireFormat::Int32Size(data_size);\n"
      "}\n"
      "_$name$_cached_byte_size_ = data_size;\n"
      "total_size += data_size;\n");
  } else {
    printer->Print(variables_,
      "total_size += $tag_size$ * this->$name$_size() + data_size;\n");
  }
  printer->Outdent();
  printer->Print("}\n");
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
