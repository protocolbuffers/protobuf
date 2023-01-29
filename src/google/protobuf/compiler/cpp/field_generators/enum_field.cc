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

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <memory>
#include <string>
#include <tuple>

#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/memory/memory.h"
#include "google/protobuf/compiler/cpp/field.h"
#include "google/protobuf/compiler/cpp/field_generators/generators.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/descriptor.pb.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
namespace {
void SetEnumVariables(
    const FieldDescriptor* descriptor,
    absl::flat_hash_map<absl::string_view, std::string>* variables,
    const Options& options) {
  SetCommonFieldVariables(descriptor, variables, options);
  const EnumValueDescriptor* default_value = descriptor->default_value_enum();
  (*variables)["type"] = QualifiedClassName(descriptor->enum_type(), options);
  (*variables)["default"] = Int32ToString(default_value->number());
  (*variables)["full_name"] = descriptor->full_name();
  (*variables)["cached_byte_size_name"] = MakeVarintCachedSizeName(descriptor);
  bool cold = ShouldSplit(descriptor, options);
  (*variables)["cached_byte_size_field"] =
      MakeVarintCachedSizeFieldName(descriptor, cold);
}

class EnumFieldGenerator : public FieldGeneratorBase {
 public:
  EnumFieldGenerator(const FieldDescriptor* descriptor, const Options& options);
  ~EnumFieldGenerator() override = default;

  void GeneratePrivateMembers(io::Printer* printer) const override;
  void GenerateAccessorDeclarations(io::Printer* printer) const override;
  void GenerateInlineAccessorDefinitions(io::Printer* printer) const override;
  void GenerateClearingCode(io::Printer* printer) const override;
  void GenerateMergingCode(io::Printer* printer) const override;
  void GenerateSwappingCode(io::Printer* printer) const override;
  void GenerateConstructorCode(io::Printer* printer) const override {}
  void GenerateCopyConstructorCode(io::Printer* printer) const override;
  void GenerateSerializeWithCachedSizesToArray(
      io::Printer* printer) const override;
  void GenerateByteSize(io::Printer* printer) const override;
  void GenerateConstexprAggregateInitializer(
      io::Printer* printer) const override;
  void GenerateAggregateInitializer(io::Printer* printer) const override;
  void GenerateCopyAggregateInitializer(io::Printer* printer) const override;
};

class EnumOneofFieldGenerator : public EnumFieldGenerator {
 public:
  EnumOneofFieldGenerator(const FieldDescriptor* descriptor,
                          const Options& options);
  ~EnumOneofFieldGenerator() override = default;

  void GenerateInlineAccessorDefinitions(io::Printer* printer) const override;
  void GenerateClearingCode(io::Printer* printer) const override;
  void GenerateSwappingCode(io::Printer* printer) const override;
  void GenerateConstructorCode(io::Printer* printer) const override;
};

class RepeatedEnumFieldGenerator : public FieldGeneratorBase {
 public:
  RepeatedEnumFieldGenerator(const FieldDescriptor* descriptor,
                             const Options& options);
  ~RepeatedEnumFieldGenerator() override = default;

  // implements FieldGeneratorBase ---------------------------------------
  void GeneratePrivateMembers(io::Printer* printer) const override;
  void GenerateAccessorDeclarations(io::Printer* printer) const override;
  void GenerateInlineAccessorDefinitions(io::Printer* printer) const override;
  void GenerateClearingCode(io::Printer* printer) const override;
  void GenerateMergingCode(io::Printer* printer) const override;
  void GenerateSwappingCode(io::Printer* printer) const override;
  void GenerateConstructorCode(io::Printer* printer) const override;
  void GenerateCopyConstructorCode(io::Printer* /*printer*/) const override {
    ABSL_CHECK(!ShouldSplit(descriptor_, options_));
  }
  void GenerateDestructorCode(io::Printer* printer) const override;
  void GenerateSerializeWithCachedSizesToArray(
      io::Printer* printer) const override;
  void GenerateByteSize(io::Printer* printer) const override;
  void GenerateConstexprAggregateInitializer(
      io::Printer* printer) const override;
  void GenerateAggregateInitializer(io::Printer* printer) const override;
  void GenerateCopyAggregateInitializer(io::Printer* printer) const override;
};

// ===================================================================

EnumFieldGenerator::EnumFieldGenerator(const FieldDescriptor* descriptor,
                                       const Options& options)
    : FieldGeneratorBase(descriptor, options) {
  SetEnumVariables(descriptor, &variables_, options);
}

void EnumFieldGenerator::GeneratePrivateMembers(io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("int $name$_;\n");
}

void EnumFieldGenerator::GenerateAccessorDeclarations(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("$deprecated_attr$$type$ ${1$$name$$}$() const;\n", descriptor_);
  format("$deprecated_attr$void ${1$set_$name$$}$($type$ value);\n",
         std::make_tuple(descriptor_, GeneratedCodeInfo::Annotation::SET));
  format(
      "private:\n"
      "$type$ ${1$_internal_$name$$}$() const;\n"
      "void ${1$_internal_set_$name$$}$($type$ value);\n"
      "public:\n",
      descriptor_);
}

void EnumFieldGenerator::GenerateInlineAccessorDefinitions(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format(
      "inline $type$ $classname$::_internal_$name$() const {\n"
      "  return static_cast< $type$ >($field$);\n"
      "}\n"
      "inline $type$ $classname$::$name$() const {\n"
      "$annotate_get$"
      "  // @@protoc_insertion_point(field_get:$full_name$)\n"
      "  return _internal_$name$();\n"
      "}\n"
      "inline void $classname$::_internal_set_$name$($type$ value) {\n");
  if (!internal::cpp::HasPreservingUnknownEnumSemantics(descriptor_)) {
    format("  assert($type$_IsValid(value));\n");
  }
  format(
      "  $set_hasbit$\n"
      "  $field$ = value;\n"
      "}\n"
      "inline void $classname$::set_$name$($type$ value) {\n"
      "$maybe_prepare_split_message$"
      "  _internal_set_$name$(value);\n"
      "$annotate_set$"
      "  // @@protoc_insertion_point(field_set:$full_name$)\n"
      "}\n");
}

void EnumFieldGenerator::GenerateClearingCode(io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("$field$ = $default$;\n");
}

void EnumFieldGenerator::GenerateMergingCode(io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("_this->_internal_set_$name$(from._internal_$name$());\n");
}

void EnumFieldGenerator::GenerateSwappingCode(io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("swap($field$, other->$field$);\n");
}

void EnumFieldGenerator::GenerateCopyConstructorCode(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("_this->$field$ = from.$field$;\n");
}

void EnumFieldGenerator::GenerateSerializeWithCachedSizesToArray(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format(
      "target = stream->EnsureSpace(target);\n"
      "target = ::_pbi::WireFormatLite::WriteEnumToArray(\n"
      "  $number$, this->_internal_$name$(), target);\n");
}

void EnumFieldGenerator::GenerateByteSize(io::Printer* printer) const {
  Formatter format(printer, variables_);
  format(
      "total_size += $tag_size$ +\n"
      "  ::_pbi::WireFormatLite::EnumSize(this->_internal_$name$());\n");
}

void EnumFieldGenerator::GenerateConstexprAggregateInitializer(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("/*decltype($field$)*/$default$");
}

void EnumFieldGenerator::GenerateAggregateInitializer(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  if (ShouldSplit(descriptor_, options_)) {
    format("decltype(Impl_::Split::$name$_){$default$}");
    return;
  }
  format("decltype($field$){$default$}");
}

void EnumFieldGenerator::GenerateCopyAggregateInitializer(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("decltype($field$){}");
}

// ===================================================================

EnumOneofFieldGenerator::EnumOneofFieldGenerator(
    const FieldDescriptor* descriptor, const Options& options)
    : EnumFieldGenerator(descriptor, options) {
  SetCommonOneofFieldVariables(descriptor, &variables_);
}

void EnumOneofFieldGenerator::GenerateInlineAccessorDefinitions(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format(
      "inline $type$ $classname$::_internal_$name$() const {\n"
      "  if ($has_field$) {\n"
      "    return static_cast< $type$ >($field$);\n"
      "  }\n"
      "  return static_cast< $type$ >($default$);\n"
      "}\n"
      "inline $type$ $classname$::$name$() const {\n"
      "$annotate_get$"
      "  // @@protoc_insertion_point(field_get:$full_name$)\n"
      "  return _internal_$name$();\n"
      "}\n"
      "inline void $classname$::_internal_set_$name$($type$ value) {\n");
  if (!internal::cpp::HasPreservingUnknownEnumSemantics(descriptor_)) {
    format("  assert($type$_IsValid(value));\n");
  }
  format(
      "  if ($not_has_field$) {\n"
      "    clear_$oneof_name$();\n"
      "    set_has_$name$();\n"
      "  }\n"
      "  $field$ = value;\n"
      "}\n"
      "inline void $classname$::set_$name$($type$ value) {\n"
      "  _internal_set_$name$(value);\n"
      "$annotate_set$"
      "  // @@protoc_insertion_point(field_set:$full_name$)\n"
      "}\n");
}

void EnumOneofFieldGenerator::GenerateClearingCode(io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("$field$ = $default$;\n");
}

void EnumOneofFieldGenerator::GenerateSwappingCode(io::Printer* printer) const {
  // Don't print any swapping code. Swapping the union will swap this field.
}

void EnumOneofFieldGenerator::GenerateConstructorCode(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("$ns$::_$classname$_default_instance_.$field$ = $default$;\n");
}

// ===================================================================

RepeatedEnumFieldGenerator::RepeatedEnumFieldGenerator(
    const FieldDescriptor* descriptor, const Options& options)
    : FieldGeneratorBase(descriptor, options) {
  SetEnumVariables(descriptor, &variables_, options);
}

void RepeatedEnumFieldGenerator::GeneratePrivateMembers(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("::$proto_ns$::RepeatedField<int> $name$_;\n");
  if (descriptor_->is_packed() &&
      HasGeneratedMethods(descriptor_->file(), options_)) {
    format(
        "mutable ::$proto_ns$::internal::CachedSize "
        "$cached_byte_size_name$;\n");
  }
}

void RepeatedEnumFieldGenerator::GenerateAccessorDeclarations(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format(
      "private:\n"
      "$type$ ${1$_internal_$name$$}$(int index) const;\n"
      "void ${1$_internal_add_$name$$}$($type$ value);\n"
      "::$proto_ns$::RepeatedField<int>* "
      "${1$_internal_mutable_$name$$}$();\n"
      "public:\n"
      "$deprecated_attr$$type$ ${1$$name$$}$(int index) const;\n"
      "$deprecated_attr$void ${1$set_$name$$}$(int index, $type$ value);\n"
      "$deprecated_attr$void ${1$add_$name$$}$($type$ value);\n"
      "$deprecated_attr$const ::$proto_ns$::RepeatedField<int>& "
      "${1$$name$$}$() const;\n"
      "$deprecated_attr$::$proto_ns$::RepeatedField<int>* "
      "${1$mutable_$name$$}$();\n",
      descriptor_);
}

void RepeatedEnumFieldGenerator::GenerateInlineAccessorDefinitions(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format(
      "inline $type$ $classname$::_internal_$name$(int index) const {\n"
      "  return static_cast< $type$ >($field$.Get(index));\n"
      "}\n"
      "inline $type$ $classname$::$name$(int index) const {\n"
      "$annotate_get$"
      "  // @@protoc_insertion_point(field_get:$full_name$)\n"
      "  return _internal_$name$(index);\n"
      "}\n"
      "inline void $classname$::set_$name$(int index, $type$ value) {\n");
  if (!internal::cpp::HasPreservingUnknownEnumSemantics(descriptor_)) {
    format("  assert($type$_IsValid(value));\n");
  }
  format(
      "  $field$.Set(index, value);\n"
      "$annotate_set$"
      "  // @@protoc_insertion_point(field_set:$full_name$)\n"
      "}\n"
      "inline void $classname$::_internal_add_$name$($type$ value) {\n");
  if (!internal::cpp::HasPreservingUnknownEnumSemantics(descriptor_)) {
    format("  assert($type$_IsValid(value));\n");
  }
  format(
      "  $field$.Add(value);\n"
      "}\n"
      "inline void $classname$::add_$name$($type$ value) {\n"
      "  _internal_add_$name$(value);\n"
      "$annotate_add$"
      "  // @@protoc_insertion_point(field_add:$full_name$)\n"
      "}\n"
      "inline const ::$proto_ns$::RepeatedField<int>&\n"
      "$classname$::$name$() const {\n"
      "$annotate_list$"
      "  // @@protoc_insertion_point(field_list:$full_name$)\n"
      "  return $field$;\n"
      "}\n"
      "inline ::$proto_ns$::RepeatedField<int>*\n"
      "$classname$::_internal_mutable_$name$() {\n"
      "  return &$field$;\n"
      "}\n"
      "inline ::$proto_ns$::RepeatedField<int>*\n"
      "$classname$::mutable_$name$() {\n"
      "$annotate_mutable_list$"
      "  // @@protoc_insertion_point(field_mutable_list:$full_name$)\n"
      "  return _internal_mutable_$name$();\n"
      "}\n");
}

void RepeatedEnumFieldGenerator::GenerateClearingCode(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("$field$.Clear();\n");
}

void RepeatedEnumFieldGenerator::GenerateMergingCode(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("_this->$field$.MergeFrom(from.$field$);\n");
}

void RepeatedEnumFieldGenerator::GenerateSwappingCode(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("$field$.InternalSwap(&other->$field$);\n");
}

void RepeatedEnumFieldGenerator::GenerateConstructorCode(
    io::Printer* printer) const {
  // Not needed for repeated fields.
}

void RepeatedEnumFieldGenerator::GenerateDestructorCode(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("$field$.~RepeatedField();\n");
}

void RepeatedEnumFieldGenerator::GenerateSerializeWithCachedSizesToArray(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  if (descriptor_->is_packed()) {
    // Write the tag and the size.
    format(
        "{\n"
        "  int byte_size = "
        "$cached_byte_size_field$.Get();\n"
        "  if (byte_size > 0) {\n"
        "    target = stream->WriteEnumPacked(\n"
        "        $number$, $field$, byte_size, target);\n"
        "  }\n"
        "}\n");
  } else {
    format(
        "for (int i = 0, n = this->_internal_$name$_size(); i < n; i++) {\n"
        "  target = stream->EnsureSpace(target);\n"
        "  target = ::_pbi::WireFormatLite::WriteEnumToArray(\n"
        "      $number$, this->_internal_$name$(i), target);\n"
        "}\n");
  }
}

void RepeatedEnumFieldGenerator::GenerateByteSize(io::Printer* printer) const {
  Formatter format(printer, variables_);
  format(
      "{\n"
      "  ::size_t data_size = 0;\n"
      "  unsigned int count = static_cast<unsigned "
      "int>(this->_internal_$name$_size());");
  format.Indent();
  format(
      "for (unsigned int i = 0; i < count; i++) {\n"
      "  data_size += ::_pbi::WireFormatLite::EnumSize(\n"
      "    this->_internal_$name$(static_cast<int>(i)));\n"
      "}\n");

  if (descriptor_->is_packed()) {
    format(
        "if (data_size > 0) {\n"
        "  total_size += $tag_size$ +\n"
        "    "
        "::_pbi::WireFormatLite::Int32Size(static_cast<$int32$>(data_size));\n"
        "}\n"
        "int cached_size = ::_pbi::ToCachedSize(data_size);\n"
        "$cached_byte_size_field$.Set(cached_size);\n"
        "total_size += data_size;\n");
  } else {
    format("total_size += ($tag_size$UL * count) + data_size;\n");
  }
  format.Outdent();
  format("}\n");
}

void RepeatedEnumFieldGenerator::GenerateConstexprAggregateInitializer(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("/*decltype($field$)*/{}");
  if (descriptor_->is_packed() &&
      HasGeneratedMethods(descriptor_->file(), options_)) {
    format("\n, /*decltype($cached_byte_size_field$)*/{0}");
  }
}

void RepeatedEnumFieldGenerator::GenerateAggregateInitializer(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("decltype($field$){arena}");
  if (descriptor_->is_packed() &&
      HasGeneratedMethods(descriptor_->file(), options_)) {
    // std::atomic has no copy constructor, which prevents explicit aggregate
    // initialization pre-C++17.
    format("\n, /*decltype($cached_byte_size_field$)*/{0}");
  }
}

void RepeatedEnumFieldGenerator::GenerateCopyAggregateInitializer(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("decltype($field$){from.$field$}");
  if (descriptor_->is_packed() &&
      HasGeneratedMethods(descriptor_->file(), options_)) {
    // std::atomic has no copy constructor.
    format("\n, /*decltype($cached_byte_size_field$)*/{0}");
  }
}
}  // namespace

std::unique_ptr<FieldGeneratorBase> MakeSinguarEnumGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc) {
  return absl::make_unique<EnumFieldGenerator>(desc, options);
}

std::unique_ptr<FieldGeneratorBase> MakeRepeatedEnumGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc) {
  return absl::make_unique<RepeatedEnumFieldGenerator>(desc, options);
}

std::unique_ptr<FieldGeneratorBase> MakeOneofEnumGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc) {
  return absl::make_unique<EnumOneofFieldGenerator>(desc, options);
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
