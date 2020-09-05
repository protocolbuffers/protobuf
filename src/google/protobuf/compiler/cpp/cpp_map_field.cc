// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
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

#include <google/protobuf/compiler/cpp/cpp_map_field.h>

#include <google/protobuf/compiler/cpp/cpp_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/stubs/strutil.h>


namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

bool IsProto3Field(const FieldDescriptor* field_descriptor) {
  const FileDescriptor* file_descriptor = field_descriptor->file();
  return file_descriptor->syntax() == FileDescriptor::SYNTAX_PROTO3;
}

void SetMessageVariables(const FieldDescriptor* descriptor,
                         std::map<std::string, std::string>* variables,
                         const Options& options) {
  SetCommonFieldVariables(descriptor, variables, options);
  (*variables)["type"] = ClassName(descriptor->message_type(), false);
  (*variables)["full_name"] = descriptor->full_name();

  const FieldDescriptor* key =
      descriptor->message_type()->FindFieldByName("key");
  const FieldDescriptor* val =
      descriptor->message_type()->FindFieldByName("value");
  (*variables)["key_cpp"] = PrimitiveTypeName(options, key->cpp_type());
  switch (val->cpp_type()) {
    case FieldDescriptor::CPPTYPE_MESSAGE:
      (*variables)["val_cpp"] = FieldMessageTypeName(val, options);
      break;
    case FieldDescriptor::CPPTYPE_ENUM:
      (*variables)["val_cpp"] = ClassName(val->enum_type(), true);
      break;
    default:
      (*variables)["val_cpp"] = PrimitiveTypeName(options, val->cpp_type());
  }
  (*variables)["key_wire_type"] =
      "TYPE_" + ToUpper(DeclaredTypeMethodName(key->type()));
  (*variables)["val_wire_type"] =
      "TYPE_" + ToUpper(DeclaredTypeMethodName(val->type()));
  (*variables)["map_classname"] = ClassName(descriptor->message_type(), false);
  (*variables)["number"] = StrCat(descriptor->number());
  (*variables)["tag"] = StrCat(internal::WireFormat::MakeTag(descriptor));

  if (HasDescriptorMethods(descriptor->file(), options)) {
    (*variables)["lite"] = "";
  } else {
    (*variables)["lite"] = "Lite";
  }
}

MapFieldGenerator::MapFieldGenerator(const FieldDescriptor* descriptor,
                                     const Options& options)
    : FieldGenerator(descriptor, options) {
  SetMessageVariables(descriptor, &variables_, options);
}

MapFieldGenerator::~MapFieldGenerator() {}

void MapFieldGenerator::GeneratePrivateMembers(io::Printer* printer) const {
  Formatter format(printer, variables_);
  format(
      "::$proto_ns$::internal::MapField$lite$<\n"
      "    $map_classname$,\n"
      "    $key_cpp$, $val_cpp$,\n"
      "    ::$proto_ns$::internal::WireFormatLite::$key_wire_type$,\n"
      "    ::$proto_ns$::internal::WireFormatLite::$val_wire_type$> "
      "$name$_;\n");
}

void MapFieldGenerator::GenerateAccessorDeclarations(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format(
      "private:\n"
      "const ::$proto_ns$::Map< $key_cpp$, $val_cpp$ >&\n"
      "    ${1$_internal_$name$$}$() const;\n"
      "::$proto_ns$::Map< $key_cpp$, $val_cpp$ >*\n"
      "    ${1$_internal_mutable_$name$$}$();\n"
      "public:\n"
      "$deprecated_attr$const ::$proto_ns$::Map< $key_cpp$, $val_cpp$ >&\n"
      "    ${1$$name$$}$() const;\n"
      "$deprecated_attr$::$proto_ns$::Map< $key_cpp$, $val_cpp$ >*\n"
      "    ${1$mutable_$name$$}$();\n",
      descriptor_);
}

void MapFieldGenerator::GenerateInlineAccessorDefinitions(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format(
      "inline const ::$proto_ns$::Map< $key_cpp$, $val_cpp$ >&\n"
      "$classname$::_internal_$name$() const {\n"
      "  return $name$_.GetMap();\n"
      "}\n"
      "inline const ::$proto_ns$::Map< $key_cpp$, $val_cpp$ >&\n"
      "$classname$::$name$() const {\n"
      "$annotate_accessor$"
      "  // @@protoc_insertion_point(field_map:$full_name$)\n"
      "  return _internal_$name$();\n"
      "}\n"
      "inline ::$proto_ns$::Map< $key_cpp$, $val_cpp$ >*\n"
      "$classname$::_internal_mutable_$name$() {\n"
      "  return $name$_.MutableMap();\n"
      "}\n"
      "inline ::$proto_ns$::Map< $key_cpp$, $val_cpp$ >*\n"
      "$classname$::mutable_$name$() {\n"
      "$annotate_accessor$"
      "  // @@protoc_insertion_point(field_mutable_map:$full_name$)\n"
      "  return _internal_mutable_$name$();\n"
      "}\n");
}

void MapFieldGenerator::GenerateClearingCode(io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("$name$_.Clear();\n");
}

void MapFieldGenerator::GenerateMergingCode(io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("$name$_.MergeFrom(from.$name$_);\n");
}

void MapFieldGenerator::GenerateSwappingCode(io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("$name$_.Swap(&other->$name$_);\n");
}

void MapFieldGenerator::GenerateCopyConstructorCode(
    io::Printer* printer) const {
  GenerateConstructorCode(printer);
  GenerateMergingCode(printer);
}

static void GenerateSerializationLoop(const Formatter& format, bool string_key,
                                      bool string_value,
                                      bool is_deterministic) {
  std::string ptr;
  if (is_deterministic) {
    format("for (size_type i = 0; i < n; i++) {\n");
    ptr = string_key ? "items[static_cast<ptrdiff_t>(i)]"
                     : "items[static_cast<ptrdiff_t>(i)].second";
  } else {
    format(
        "for (::$proto_ns$::Map< $key_cpp$, $val_cpp$ >::const_iterator\n"
        "    it = this->_internal_$name$().begin();\n"
        "    it != this->_internal_$name$().end(); ++it) {\n");
    ptr = "it";
  }
  format.Indent();

  format(
      "target = $map_classname$::Funcs::InternalSerialize($number$, "
      "$1$->first, $1$->second, target, stream);\n",
      ptr);

  if (string_key || string_value) {
    // ptr is either an actual pointer or an iterator, either way we can
    // create a pointer by taking the address after de-referencing it.
    format("Utf8Check::Check(&(*$1$));\n", ptr);
  }

  format.Outdent();
  format("}\n");
}

void MapFieldGenerator::GenerateSerializeWithCachedSizesToArray(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("if (!this->_internal_$name$().empty()) {\n");
  format.Indent();
  const FieldDescriptor* key_field =
      descriptor_->message_type()->FindFieldByName("key");
  const FieldDescriptor* value_field =
      descriptor_->message_type()->FindFieldByName("value");
  const bool string_key = key_field->type() == FieldDescriptor::TYPE_STRING;
  const bool string_value = value_field->type() == FieldDescriptor::TYPE_STRING;

  format(
      "typedef ::$proto_ns$::Map< $key_cpp$, $val_cpp$ >::const_pointer\n"
      "    ConstPtr;\n");
  if (string_key) {
    format(
        "typedef ConstPtr SortItem;\n"
        "typedef ::$proto_ns$::internal::"
        "CompareByDerefFirst<SortItem> Less;\n");
  } else {
    format(
        "typedef ::$proto_ns$::internal::SortItem< $key_cpp$, ConstPtr > "
        "SortItem;\n"
        "typedef ::$proto_ns$::internal::CompareByFirstField<SortItem> "
        "Less;\n");
  }
  bool utf8_check = string_key || string_value;
  if (utf8_check) {
    format(
        "struct Utf8Check {\n"
        "  static void Check(ConstPtr p) {\n");
    format.Indent();
    format.Indent();
    if (string_key) {
      GenerateUtf8CheckCodeForString(
          key_field, options_, false,
          "p->first.data(), static_cast<int>(p->first.length()),\n", format);
    }
    if (string_value) {
      GenerateUtf8CheckCodeForString(
          value_field, options_, false,
          "p->second.data(), static_cast<int>(p->second.length()),\n", format);
    }
    format.Outdent();
    format.Outdent();
    format(
        "  }\n"
        "};\n");
  }

  format(
      "\n"
      "if (stream->IsSerializationDeterministic() &&\n"
      "    this->_internal_$name$().size() > 1) {\n"
      "  ::std::unique_ptr<SortItem[]> items(\n"
      "      new SortItem[this->_internal_$name$().size()]);\n"
      "  typedef ::$proto_ns$::Map< $key_cpp$, $val_cpp$ >::size_type "
      "size_type;\n"
      "  size_type n = 0;\n"
      "  for (::$proto_ns$::Map< $key_cpp$, $val_cpp$ >::const_iterator\n"
      "      it = this->_internal_$name$().begin();\n"
      "      it != this->_internal_$name$().end(); ++it, ++n) {\n"
      "    items[static_cast<ptrdiff_t>(n)] = SortItem(&*it);\n"
      "  }\n"
      "  ::std::sort(&items[0], &items[static_cast<ptrdiff_t>(n)], Less());\n");
  format.Indent();
  GenerateSerializationLoop(format, string_key, string_value, true);
  format.Outdent();
  format("} else {\n");
  format.Indent();
  GenerateSerializationLoop(format, string_key, string_value, false);
  format.Outdent();
  format("}\n");
  format.Outdent();
  format("}\n");
}

void MapFieldGenerator::GenerateByteSize(io::Printer* printer) const {
  Formatter format(printer, variables_);
  format(
      "total_size += $tag_size$ *\n"
      "    "
      "::$proto_ns$::internal::FromIntSize(this->_internal_$name$_size());\n"
      "for (::$proto_ns$::Map< $key_cpp$, $val_cpp$ >::const_iterator\n"
      "    it = this->_internal_$name$().begin();\n"
      "    it != this->_internal_$name$().end(); ++it) {\n"
      "  total_size += $map_classname$::Funcs::ByteSizeLong(it->first, "
      "it->second);\n"
      "}\n");
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
