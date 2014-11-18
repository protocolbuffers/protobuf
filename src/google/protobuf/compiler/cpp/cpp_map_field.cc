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
                         map<string, string>* variables,
                         const Options& options) {
  SetCommonFieldVariables(descriptor, variables, options);
  (*variables)["type"] = FieldMessageTypeName(descriptor);
  (*variables)["stream_writer"] = (*variables)["declared_type"] +
      (HasFastArraySerialization(descriptor->message_type()->file()) ?
       "MaybeToArray" :
       "");
  (*variables)["full_name"] = descriptor->full_name();

  const FieldDescriptor* key =
      descriptor->message_type()->FindFieldByName("key");
  const FieldDescriptor* val =
      descriptor->message_type()->FindFieldByName("value");
  (*variables)["key_cpp"] = PrimitiveTypeName(key->cpp_type());
  switch (val->cpp_type()) {
    case FieldDescriptor::CPPTYPE_MESSAGE:
      (*variables)["val_cpp"] = FieldMessageTypeName(val);
      (*variables)["wrapper"] = "EntryWrapper";
      break;
    case FieldDescriptor::CPPTYPE_ENUM:
      (*variables)["val_cpp"] = ClassName(val->enum_type(), false);
      (*variables)["wrapper"] = "EnumEntryWrapper";
      break;
    default:
      (*variables)["val_cpp"] = PrimitiveTypeName(val->cpp_type());
      (*variables)["wrapper"] = "EntryWrapper";
  }
  (*variables)["key_type"] =
      "::google::protobuf::FieldDescriptor::TYPE_" +
      ToUpper(DeclaredTypeMethodName(key->type()));
  (*variables)["val_type"] =
      "::google::protobuf::FieldDescriptor::TYPE_" +
      ToUpper(DeclaredTypeMethodName(val->type()));
  (*variables)["map_classname"] = ClassName(descriptor->message_type(), false);
  (*variables)["number"] = Int32ToString(descriptor->number());

  if (!IsProto3Field(descriptor) &&
      val->type() == FieldDescriptor::TYPE_ENUM) {
    const EnumValueDescriptor* default_value = val->default_value_enum();
    (*variables)["default_enum_value"] = Int32ToString(default_value->number());
  } else {
    (*variables)["default_enum_value"] = "0";
  }
}

MapFieldGenerator::
MapFieldGenerator(const FieldDescriptor* descriptor,
                              const Options& options)
    : descriptor_(descriptor) {
  SetMessageVariables(descriptor, &variables_, options);
}

MapFieldGenerator::~MapFieldGenerator() {}

void MapFieldGenerator::
GeneratePrivateMembers(io::Printer* printer) const {
  printer->Print(variables_,
      "typedef ::google::protobuf::internal::MapEntry<\n"
      "    $key_cpp$, $val_cpp$,\n"
      "    $key_type$,\n"
      "    $val_type$, $default_enum_value$>\n"
      "    $map_classname$;\n"
      "::google::protobuf::internal::MapField< $key_cpp$, $val_cpp$,"
      "$key_type$, $val_type$, $default_enum_value$ > $name$_;\n");
}

void MapFieldGenerator::
GenerateAccessorDeclarations(io::Printer* printer) const {
  printer->Print(variables_,
      "inline const ::google::protobuf::Map< $key_cpp$, $val_cpp$ >&\n"
      "    $name$() const$deprecation$;\n"
      "inline ::google::protobuf::Map< $key_cpp$, $val_cpp$ >*\n"
      "    mutable_$name$()$deprecation$;\n");
}

void MapFieldGenerator::
GenerateInlineAccessorDefinitions(io::Printer* printer) const {
  printer->Print(variables_,
      "inline const ::google::protobuf::Map< $key_cpp$, $val_cpp$ >&\n"
      "$classname$::$name$() const {\n"
      "  // @@protoc_insertion_point(field_map:$full_name$)\n"
      "  return $name$_.GetMap();\n"
      "}\n"
      "inline ::google::protobuf::Map< $key_cpp$, $val_cpp$ >*\n"
      "$classname$::mutable_$name$() {\n"
      "  // @@protoc_insertion_point(field_mutable_map:$full_name$)\n"
      "  return $name$_.MutableMap();\n"
      "}\n");
}

void MapFieldGenerator::
GenerateClearingCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$_.Clear();\n");
}

void MapFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$_.MergeFrom(from.$name$_);\n");
}

void MapFieldGenerator::
GenerateSwappingCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$_.Swap(&other->$name$_);\n");
}

void MapFieldGenerator::
GenerateConstructorCode(io::Printer* printer) const {
  if (HasDescriptorMethods(descriptor_->file())) {
    printer->Print(variables_,
        "$name$_.SetAssignDescriptorCallback(\n"
        "    protobuf_AssignDescriptorsOnce);\n"
        "$name$_.SetEntryDescriptor(\n"
        "    &$type$_descriptor_);\n");
  }
}

void MapFieldGenerator::
GenerateMergeFromCodedStream(io::Printer* printer) const {
  const FieldDescriptor* value_field =
      descriptor_->message_type()->FindFieldByName("value");
  printer->Print(variables_,
      "::google::protobuf::scoped_ptr<$map_classname$> entry($name$_.NewEntry());\n");

  if (IsProto3Field(descriptor_) ||
      value_field->type() != FieldDescriptor::TYPE_ENUM) {
    printer->Print(variables_,
        "DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(\n"
        "    input, entry.get()));\n");
    switch (value_field->cpp_type()) {
      case FieldDescriptor::CPPTYPE_MESSAGE:
        printer->Print(variables_,
            "(*mutable_$name$())[entry->key()].Swap("
            "entry->mutable_value());\n");
        break;
      case FieldDescriptor::CPPTYPE_ENUM:
        printer->Print(variables_,
            "(*mutable_$name$())[entry->key()] =\n"
            "    static_cast<$val_cpp$>(*entry->mutable_value());\n");
        break;
      default:
        printer->Print(variables_,
            "(*mutable_$name$())[entry->key()] = *entry->mutable_value();\n");
        break;
    }
  } else {
    printer->Print(variables_,
        "{\n"
        "  ::std::string data;\n"
        "  DO_(::google::protobuf::internal::WireFormatLite::ReadString(input, &data));\n"
        "  DO_(entry->ParseFromString(data));\n"
        "  if ($val_cpp$_IsValid(*entry->mutable_value())) {\n"
        "    (*mutable_$name$())[entry->key()] =\n"
        "        static_cast<$val_cpp$>(*entry->mutable_value());\n"
        "  } else {\n"
        "    mutable_unknown_fields()->AddLengthDelimited($number$, data);\n"
        "  }\n"
        "}\n");
  }
}

void MapFieldGenerator::
GenerateSerializeWithCachedSizes(io::Printer* printer) const {
  printer->Print(variables_,
      "{\n"
      "  ::google::protobuf::scoped_ptr<$map_classname$> entry;\n"
      "  for (::google::protobuf::Map< $key_cpp$, $val_cpp$ >::const_iterator\n"
      "      it = $name$().begin(); it != $name$().end(); ++it) {\n"
      "    entry.reset($name$_.New$wrapper$(it->first, it->second));\n"
      "    ::google::protobuf::internal::WireFormatLite::Write$stream_writer$(\n"
      "        $number$, *entry, output);\n"
      "  }\n"
      "}\n");
}

void MapFieldGenerator::
GenerateSerializeWithCachedSizesToArray(io::Printer* printer) const {
  printer->Print(variables_,
      "{\n"
      "  ::google::protobuf::scoped_ptr<$map_classname$> entry;\n"
      "  for (::google::protobuf::Map< $key_cpp$, $val_cpp$ >::const_iterator\n"
      "      it = $name$().begin(); it != $name$().end(); ++it) {\n"
      "    entry.reset($name$_.New$wrapper$(it->first, it->second));\n"
      "    target = ::google::protobuf::internal::WireFormatLite::\n"
      "        Write$declared_type$NoVirtualToArray(\n"
      "            $number$, *entry, target);\n"
      "  }\n"
      "}\n");
}

void MapFieldGenerator::
GenerateByteSize(io::Printer* printer) const {
  printer->Print(variables_,
      "total_size += $tag_size$ * this->$name$_size();\n"
      "{\n"
      "  ::google::protobuf::scoped_ptr<$map_classname$> entry;\n"
      "  for (::google::protobuf::Map< $key_cpp$, $val_cpp$ >::const_iterator\n"
      "      it = $name$().begin(); it != $name$().end(); ++it) {\n"
      "    entry.reset($name$_.New$wrapper$(it->first, it->second));\n"
      "    total_size += ::google::protobuf::internal::WireFormatLite::\n"
      "        $declared_type$SizeNoVirtual(*entry);\n"
      "  }\n"
      "}\n");
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
