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

#include "google/protobuf/compiler/cpp/map_field.h"

#include "google/protobuf/io/printer.h"
#include "google/protobuf/wire_format.h"
#include "absl/strings/ascii.h"
#include "google/protobuf/compiler/cpp/helpers.h"

#include "google/protobuf/stubs/strutil.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
void SetMessageVariables(const FieldDescriptor* descriptor,
                         std::map<std::string, std::string>* variables,
                         const Options& options) {
  SetCommonFieldVariables(descriptor, variables, options);
  (*variables)["type"] = ClassName(descriptor->message_type(), false);
  (*variables)["full_name"] = descriptor->full_name();

  const FieldDescriptor* key = descriptor->message_type()->map_key();
  const FieldDescriptor* val = descriptor->message_type()->map_value();
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
      "TYPE_" + absl::AsciiStrToUpper(DeclaredTypeMethodName(key->type()));
  (*variables)["val_wire_type"] =
      "TYPE_" + absl::AsciiStrToUpper(DeclaredTypeMethodName(val->type()));
  (*variables)["map_classname"] = ClassName(descriptor->message_type(), false);
  (*variables)["number"] = absl::StrCat(descriptor->number());
  (*variables)["tag"] = absl::StrCat(internal::WireFormat::MakeTag(descriptor));

  if (HasDescriptorMethods(descriptor->file(), options)) {
    (*variables)["lite"] = "";
  } else {
    (*variables)["lite"] = "Lite";
  }
}

MapFieldGenerator::MapFieldGenerator(const FieldDescriptor* descriptor,
                                     const Options& options,
                                     MessageSCCAnalyzer* scc_analyzer)
    : FieldGenerator(descriptor, options),
      has_required_fields_(
          scc_analyzer->HasRequiredFields(descriptor->message_type())) {
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
      "  return $field$.GetMap();\n"
      "}\n"
      "inline const ::$proto_ns$::Map< $key_cpp$, $val_cpp$ >&\n"
      "$classname$::$name$() const {\n"
      "$annotate_get$"
      "  // @@protoc_insertion_point(field_map:$full_name$)\n"
      "  return _internal_$name$();\n"
      "}\n"
      "inline ::$proto_ns$::Map< $key_cpp$, $val_cpp$ >*\n"
      "$classname$::_internal_mutable_$name$() {\n"
      "$maybe_prepare_split_message$"
      "  return $field$.MutableMap();\n"
      "}\n"
      "inline ::$proto_ns$::Map< $key_cpp$, $val_cpp$ >*\n"
      "$classname$::mutable_$name$() {\n"
      "$annotate_mutable$"
      "  // @@protoc_insertion_point(field_mutable_map:$full_name$)\n"
      "  return _internal_mutable_$name$();\n"
      "}\n");
}

void MapFieldGenerator::GenerateClearingCode(io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("$field$.Clear();\n");
}

void MapFieldGenerator::GenerateMergingCode(io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("_this->$field$.MergeFrom(from.$field$);\n");
}

void MapFieldGenerator::GenerateSwappingCode(io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("$field$.InternalSwap(&other->$field$);\n");
}

void MapFieldGenerator::GenerateCopyConstructorCode(
    io::Printer* printer) const {
  GenerateConstructorCode(printer);
  GenerateMergingCode(printer);
}

static void GenerateSerializationLoop(Formatter& format, bool string_key,
                                      bool string_value,
                                      bool is_deterministic) {
  if (is_deterministic) {
    format(
        "for (const auto& entry : "
        "::_pbi::MapSorter$1$<MapType>(map_field)) {\n",
        (string_key ? "Ptr" : "Flat"));
  } else {
    format("for (const auto& entry : map_field) {\n");
  }
  {
    auto loop_scope = format.ScopedIndent();
    format(
        "target = WireHelper::InternalSerialize($number$, "
        "entry.first, entry.second, target, stream);\n");

    if (string_key || string_value) {
      format("check_utf8(entry);\n");
    }
  }
  format("}\n");
}

void MapFieldGenerator::GenerateSerializeWithCachedSizesToArray(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("if (!this->_internal_$name$().empty()) {\n");
  format.Indent();
  const FieldDescriptor* key_field = descriptor_->message_type()->map_key();
  const FieldDescriptor* value_field = descriptor_->message_type()->map_value();
  const bool string_key = key_field->type() == FieldDescriptor::TYPE_STRING;
  const bool string_value = value_field->type() == FieldDescriptor::TYPE_STRING;

  format(
      "using MapType = ::_pb::Map<$key_cpp$, $val_cpp$>;\n"
      "using WireHelper = $map_classname$::Funcs;\n"
      "const auto& map_field = this->_internal_$name$();\n");
  bool utf8_check = string_key || string_value;
  if (utf8_check) {
    format("auto check_utf8 = [](const MapType::value_type& entry) {\n");
    {
      auto check_scope = format.ScopedIndent();
      // p may be unused when GetUtf8CheckMode evaluates to kNone,
      // thus disabling the validation.
      format("(void)entry;\n");
      if (string_key) {
        GenerateUtf8CheckCodeForString(
            key_field, options_, false,
            "entry.first.data(), static_cast<int>(entry.first.length()),\n",
            format);
      }
      if (string_value) {
        GenerateUtf8CheckCodeForString(
            value_field, options_, false,
            "entry.second.data(), static_cast<int>(entry.second.length()),\n",
            format);
      }
    }
    format("};\n");
  }

  format(
      "\n"
      "if (stream->IsSerializationDeterministic() && "
      "map_field.size() > 1) {\n");
  {
    auto deterministic_scope = format.ScopedIndent();
    GenerateSerializationLoop(format, string_key, string_value, true);
  }
  format("} else {\n");
  {
    auto map_order_scope = format.ScopedIndent();
    GenerateSerializationLoop(format, string_key, string_value, false);
  }
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

void MapFieldGenerator::GenerateIsInitialized(io::Printer* printer) const {
  if (!has_required_fields_) return;

  Formatter format(printer, variables_);
  format(
      "if (!::$proto_ns$::internal::AllAreInitialized($field$)) return "
      "false;\n");
}

void MapFieldGenerator::GenerateConstexprAggregateInitializer(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  if (HasDescriptorMethods(descriptor_->file(), options_)) {
    format("/*decltype($field$)*/{::_pbi::ConstantInitialized()}");
  } else {
    format("/*decltype($field$)*/{}");
  }
}

void MapFieldGenerator::GenerateCopyAggregateInitializer(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  // MapField has no move constructor, which prevents explicit aggregate
  // initialization pre-C++17.
  format("/*decltype($field$)*/{}");
}

void MapFieldGenerator::GenerateAggregateInitializer(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  if (ShouldSplit(descriptor_, options_)) {
    format(
        "/*decltype($classname$::Split::$name$_)*/"
        "{::_pbi::ArenaInitialized(), arena}");
    return;
  }
  // MapField has no move constructor.
  format("/*decltype($field$)*/{::_pbi::ArenaInitialized(), arena}");
}

void MapFieldGenerator::GenerateDestructorCode(io::Printer* printer) const {
  GOOGLE_CHECK(!IsFieldStripped(descriptor_, options_));

  Formatter format(printer, variables_);
  if (ShouldSplit(descriptor_, options_)) {
    format("$cached_split_ptr$->$name$_.Destruct();\n");
    format("$cached_split_ptr$->$name$_.~MapField$lite$();\n");
    return;
  }
  format("$field$.Destruct();\n");
  format("$field$.~MapField$lite$();\n");
}

void MapFieldGenerator::GenerateArenaDestructorCode(
    io::Printer* printer) const {
  if (NeedsArenaDestructor() == ArenaDtorNeeds::kNone) {
    return;
  }

  Formatter format(printer, variables_);
  // _this is the object being destructed (we are inside a static method here).
  format("_this->$field$.Destruct();\n");
}

ArenaDtorNeeds MapFieldGenerator::NeedsArenaDestructor() const {
  return HasDescriptorMethods(descriptor_->file(), options_)
             ? ArenaDtorNeeds::kRequired
             : ArenaDtorNeeds::kNone;
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
