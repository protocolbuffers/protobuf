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

#include <google/protobuf/compiler/cpp/cpp_string_field.h>
#include <google/protobuf/compiler/cpp/cpp_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/wire_format_inl.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

using internal::WireFormat;

namespace {

// TODO(kenton):  Factor out a "SetCommonFieldVariables()" to get rid of
//   repeat code between this and the other field types.
void SetStringVariables(const FieldDescriptor* descriptor,
                        map<string, string>* variables) {
  (*variables)["name"] = FieldName(descriptor);
  (*variables)["default"] =
    "\"" + CEscape(descriptor->default_value_string()) + "\"";
  (*variables)["index"] = SimpleItoa(descriptor->index());
  (*variables)["number"] = SimpleItoa(descriptor->number());
  (*variables)["classname"] = ClassName(FieldScope(descriptor), false);
  (*variables)["declared_type"] = DeclaredTypeMethodName(descriptor->type());
  (*variables)["tag_size"] = SimpleItoa(
    WireFormat::TagSize(descriptor->number(), descriptor->type()));
}

}  // namespace

// ===================================================================

StringFieldGenerator::
StringFieldGenerator(const FieldDescriptor* descriptor)
  : descriptor_(descriptor) {
  SetStringVariables(descriptor, &variables_);
}

StringFieldGenerator::~StringFieldGenerator() {}

void StringFieldGenerator::
GeneratePrivateMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "::std::string* $name$_;\n"
    "static const ::std::string _default_$name$_;\n");
}

void StringFieldGenerator::
GenerateAccessorDeclarations(io::Printer* printer) const {
  // If we're using StringFieldGenerator for a field with a ctype, it's
  // because that ctype isn't actually implemented.  In particular, this is
  // true of ctype=CORD and ctype=STRING_PIECE in the open source release.
  // We aren't releasing Cord because it has too many Google-specific
  // dependencies and we aren't releasing StringPiece because it's hardly
  // useful outside of Google and because it would get confusing to have
  // multiple instances of the StringPiece class in different libraries (PCRE
  // already includes it for their C++ bindings, which came from Google).
  //
  // In any case, we make all the accessors private while still actually
  // using a string to represent the field internally.  This way, we can
  // guarantee that if we do ever implement the ctype, it won't break any
  // existing users who might be -- for whatever reason -- already using .proto
  // files that applied the ctype.  The field can still be accessed via the
  // reflection interface since the reflection interface is independent of
  // the string's underlying representation.
  if (descriptor_->options().has_ctype()) {
    printer->Outdent();
    printer->Print(
      " private:\n"
      "  // Hidden due to unknown ctype option.\n");
    printer->Indent();
  }

  printer->Print(variables_,
    "inline const ::std::string& $name$() const;\n"
    "inline void set_$name$(const ::std::string& value);\n"
    "inline void set_$name$(const char* value);\n");
  if (descriptor_->type() == FieldDescriptor::TYPE_BYTES) {
    printer->Print(variables_,
      "inline void set_$name$(const void* value, size_t size);\n");
  }

  printer->Print(variables_,
    "inline ::std::string* mutable_$name$();\n");

  if (descriptor_->options().has_ctype()) {
    printer->Outdent();
    printer->Print(" public:\n");
    printer->Indent();
  }
}

void StringFieldGenerator::
GenerateInlineAccessorDefinitions(io::Printer* printer) const {
  printer->Print(variables_,
    "inline const ::std::string& $classname$::$name$() const {\n"
    "  return *$name$_;\n"
    "}\n"
    "inline void $classname$::set_$name$(const ::std::string& value) {\n"
    "  _set_bit($index$);\n"
    "  if ($name$_ == &_default_$name$_) {\n"
    "    $name$_ = new ::std::string;\n"
    "  }\n"
    "  $name$_->assign(value);\n"
    "}\n"
    "inline void $classname$::set_$name$(const char* value) {\n"
    "  _set_bit($index$);\n"
    "  if ($name$_ == &_default_$name$_) {\n"
    "    $name$_ = new ::std::string;\n"
    "  }\n"
    "  $name$_->assign(value);\n"
    "}\n");

  if (descriptor_->type() == FieldDescriptor::TYPE_BYTES) {
    printer->Print(variables_,
      "inline void $classname$::set_$name$(const void* value, size_t size) {\n"
      "  _set_bit($index$);\n"
      "  if ($name$_ == &_default_$name$_) {\n"
      "    $name$_ = new ::std::string;\n"
      "  }\n"
      "  $name$_->assign(reinterpret_cast<const char*>(value), size);\n"
      "}\n");
  }

  printer->Print(variables_,
    "inline ::std::string* $classname$::mutable_$name$() {\n"
    "  _set_bit($index$);\n"
    "  if ($name$_ == &_default_$name$_) {\n");
  if (descriptor_->default_value_string().empty()) {
    printer->Print(variables_,
      "    $name$_ = new ::std::string;\n");
  } else {
    printer->Print(variables_,
      "    $name$_ = new ::std::string(_default_$name$_);\n");
  }
  printer->Print(variables_,
    "  }\n"
    "  return $name$_;\n"
    "}\n");
}

void StringFieldGenerator::
GenerateNonInlineAccessorDefinitions(io::Printer* printer) const {
  if (descriptor_->default_value_string().empty()) {
    printer->Print(variables_,
      "const ::std::string $classname$::_default_$name$_;");
  } else {
    printer->Print(variables_,
      "const ::std::string $classname$::_default_$name$_($default$);");
  }
}

void StringFieldGenerator::
GenerateClearingCode(io::Printer* printer) const {
  if (descriptor_->default_value_string().empty()) {
    printer->Print(variables_,
      "if ($name$_ != &_default_$name$_) {\n"
      "  $name$_->clear();\n"
      "}\n");
  } else {
    printer->Print(variables_,
      "if ($name$_ != &_default_$name$_) {\n"
      "  $name$_->assign(_default_$name$_);\n"
      "}\n");
  }
}

void StringFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_, "set_$name$(from.$name$());\n");
}

void StringFieldGenerator::
GenerateSwappingCode(io::Printer* printer) const {
  printer->Print(variables_, "std::swap($name$_, other->$name$_);\n");
}

void StringFieldGenerator::
GenerateInitializer(io::Printer* printer) const {
  printer->Print(variables_,
    ",\n$name$_(const_cast< ::std::string*>(&_default_$name$_))");
}

void StringFieldGenerator::
GenerateDestructorCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if ($name$_ != &_default_$name$_) {\n"
    "  delete $name$_;\n"
    "}\n");
}

void StringFieldGenerator::
GenerateMergeFromCodedStream(io::Printer* printer) const {
  printer->Print(variables_,
    "DO_(::google::protobuf::internal::WireFormat::Read$declared_type$("
      "input, mutable_$name$()));\n");
}

void StringFieldGenerator::
GenerateSerializeWithCachedSizes(io::Printer* printer) const {
  printer->Print(variables_,
    "DO_(::google::protobuf::internal::WireFormat::Write$declared_type$("
      "$number$, this->$name$(), output));\n");
}

void StringFieldGenerator::
GenerateByteSize(io::Printer* printer) const {
  printer->Print(variables_,
    "total_size += $tag_size$ +\n"
    "  ::google::protobuf::internal::WireFormat::$declared_type$Size(this->$name$());\n");
}

// ===================================================================

RepeatedStringFieldGenerator::
RepeatedStringFieldGenerator(const FieldDescriptor* descriptor)
  : descriptor_(descriptor) {
  SetStringVariables(descriptor, &variables_);
}

RepeatedStringFieldGenerator::~RepeatedStringFieldGenerator() {}

void RepeatedStringFieldGenerator::
GeneratePrivateMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "::google::protobuf::RepeatedPtrField< ::std::string> $name$_;\n");
}

void RepeatedStringFieldGenerator::
GenerateAccessorDeclarations(io::Printer* printer) const {
  // See comment above about unknown ctypes.
  if (descriptor_->options().has_ctype()) {
    printer->Outdent();
    printer->Print(
      " private:\n"
      "  // Hidden due to unknown ctype option.\n");
    printer->Indent();
  }

  printer->Print(variables_,
    "inline const ::google::protobuf::RepeatedPtrField< ::std::string>& $name$() const;\n"
    "inline ::google::protobuf::RepeatedPtrField< ::std::string>* mutable_$name$();\n"
    "inline const ::std::string& $name$(int index) const;\n"
    "inline ::std::string* mutable_$name$(int index);\n"
    "inline void set_$name$(int index, const ::std::string& value);\n"
    "inline void set_$name$(int index, const char* value);\n"
    "inline ::std::string* add_$name$();\n"
    "inline void add_$name$(const ::std::string& value);\n"
    "inline void add_$name$(const char* value);\n");

  if (descriptor_->type() == FieldDescriptor::TYPE_BYTES) {
    printer->Print(variables_,
      "inline void set_$name$(int index, const void* value, size_t size);\n"
      "inline void add_$name$(const void* value, size_t size);\n");
  }

  if (descriptor_->options().has_ctype()) {
    printer->Outdent();
    printer->Print(" public:\n");
    printer->Indent();
  }
}

void RepeatedStringFieldGenerator::
GenerateInlineAccessorDefinitions(io::Printer* printer) const {
  printer->Print(variables_,
    "inline const ::google::protobuf::RepeatedPtrField< ::std::string>&\n"
    "$classname$::$name$() const {\n"
    "  return $name$_;\n"
    "}\n"
    "inline ::google::protobuf::RepeatedPtrField< ::std::string>*\n"
    "$classname$::mutable_$name$() {\n"
    "  return &$name$_;\n"
    "}\n"
    "inline const ::std::string& $classname$::$name$(int index) const {\n"
    "  return $name$_.Get(index);\n"
    "}\n"
    "inline ::std::string* $classname$::mutable_$name$(int index) {\n"
    "  return $name$_.Mutable(index);\n"
    "}\n"
    "inline void $classname$::set_$name$(int index, const ::std::string& value) {\n"
    "  $name$_.Mutable(index)->assign(value);\n"
    "}\n"
    "inline void $classname$::set_$name$(int index, const char* value) {\n"
    "  $name$_.Mutable(index)->assign(value);\n"
    "}\n"
    "inline ::std::string* $classname$::add_$name$() {\n"
    "  return $name$_.Add();\n"
    "}\n"
    "inline void $classname$::add_$name$(const ::std::string& value) {\n"
    "  $name$_.Add()->assign(value);\n"
    "}\n"
    "inline void $classname$::add_$name$(const char* value) {\n"
    "  $name$_.Add()->assign(value);\n"
    "}\n");

  if (descriptor_->type() == FieldDescriptor::TYPE_BYTES) {
    printer->Print(variables_,
      "inline void "
      "$classname$::set_$name$(int index, const void* value, size_t size) {\n"
      "  $name$_.Mutable(index)->assign(\n"
      "    reinterpret_cast<const char*>(value), size);\n"
      "}\n"
      "inline void $classname$::add_$name$(const void* value, size_t size) {\n"
      "  $name$_.Add()->assign(reinterpret_cast<const char*>(value), size);\n"
      "}\n");
  }
}

void RepeatedStringFieldGenerator::
GenerateClearingCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$_.Clear();\n");
}

void RepeatedStringFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$_.MergeFrom(from.$name$_);\n");
}

void RepeatedStringFieldGenerator::
GenerateSwappingCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$_.Swap(&other->$name$_);\n");
}

void RepeatedStringFieldGenerator::
GenerateInitializer(io::Printer* printer) const {
  printer->Print(variables_, ",\n$name$_()");
}

void RepeatedStringFieldGenerator::
GenerateMergeFromCodedStream(io::Printer* printer) const {
  printer->Print(variables_,
    "DO_(::google::protobuf::internal::WireFormat::Read$declared_type$(\n"
    "     input, add_$name$()));\n");
}

void RepeatedStringFieldGenerator::
GenerateSerializeWithCachedSizes(io::Printer* printer) const {
  printer->Print(variables_,
    "for (int i = 0; i < this->$name$_size(); i++) {\n"
    "  DO_(::google::protobuf::internal::WireFormat::Write$declared_type$("
        "$number$, this->$name$(i), output));\n"
    "}\n");
}

void RepeatedStringFieldGenerator::
GenerateByteSize(io::Printer* printer) const {
  printer->Print(variables_,
    "total_size += $tag_size$ * this->$name$_size();\n"
    "for (int i = 0; i < this->$name$_size(); i++) {\n"
    "  total_size += ::google::protobuf::internal::WireFormat::$declared_type$Size(\n"
    "    this->$name$(i));\n"
    "}\n");
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
