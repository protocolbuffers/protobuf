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
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/cpp/field.h"
#include "google/protobuf/compiler/cpp/field_generators/generators.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
namespace {
void SetCordVariables(
    const FieldDescriptor* descriptor,
    absl::flat_hash_map<absl::string_view, std::string>* variables,
    const Options& options) {
  (*variables)["default"] = absl::StrCat(
      "\"", absl::CEscape(descriptor->default_value_string()), "\"");
  (*variables)["default_length"] =
      absl::StrCat(descriptor->default_value_string().length());
  (*variables)["full_name"] = descriptor->full_name();
  // For one of Cords
  (*variables)["default_variable_name"] = MakeDefaultName(descriptor);
  (*variables)["default_variable_field"] = MakeDefaultFieldName(descriptor);
  (*variables)["default_variable"] =
      descriptor->default_value_string().empty()
          ? absl::StrCat(ProtobufNamespace(options),
                         "::internal::GetEmptyCordAlreadyInited()")
          : absl::StrCat(
                QualifiedClassName(descriptor->containing_type(), options),
                "::", MakeDefaultFieldName(descriptor));
}

class CordFieldGenerator : public FieldGeneratorBase {
 public:
  CordFieldGenerator(const FieldDescriptor* descriptor, const Options& options);
  ~CordFieldGenerator() override = default;

  void GeneratePrivateMembers(io::Printer* printer) const override;
  void GenerateAccessorDeclarations(io::Printer* printer) const override;
  void GenerateInlineAccessorDefinitions(io::Printer* printer) const override;
  void GenerateClearingCode(io::Printer* printer) const override;
  void GenerateMergingCode(io::Printer* printer) const override;
  void GenerateSwappingCode(io::Printer* printer) const override;
  void GenerateConstructorCode(io::Printer* printer) const override;
  void GenerateDestructorCode(io::Printer* printer) const override;
  void GenerateArenaDestructorCode(io::Printer* printer) const override;
  void GenerateSerializeWithCachedSizesToArray(
      io::Printer* printer) const override;
  void GenerateByteSize(io::Printer* printer) const override;
  void GenerateAggregateInitializer(io::Printer* printer) const override;
  void GenerateConstexprAggregateInitializer(
      io::Printer* printer) const override;
  ArenaDtorNeeds NeedsArenaDestructor() const override {
    return ArenaDtorNeeds::kRequired;
  }
};

class CordOneofFieldGenerator : public CordFieldGenerator {
 public:
  CordOneofFieldGenerator(const FieldDescriptor* descriptor,
                          const Options& options);
  ~CordOneofFieldGenerator() override = default;

  void GeneratePrivateMembers(io::Printer* printer) const override;
  void GenerateStaticMembers(io::Printer* printer) const override;
  void GenerateInlineAccessorDefinitions(io::Printer* printer) const override;
  void GenerateNonInlineAccessorDefinitions(
      io::Printer* printer) const override;
  void GenerateClearingCode(io::Printer* printer) const override;
  void GenerateSwappingCode(io::Printer* printer) const override;
  void GenerateConstructorCode(io::Printer* printer) const override {}
  void GenerateArenaDestructorCode(io::Printer* printer) const override;
  // Overrides CordFieldGenerator behavior.
  ArenaDtorNeeds NeedsArenaDestructor() const override {
    return ArenaDtorNeeds::kNone;
  }
};


CordFieldGenerator::CordFieldGenerator(const FieldDescriptor* descriptor,
                                       const Options& options)
    : FieldGeneratorBase(descriptor, options) {
  SetCordVariables(descriptor, &variables_, options);
}

void CordFieldGenerator::GeneratePrivateMembers(io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("::absl::Cord $name$_;\n");
  if (!descriptor_->default_value_string().empty()) {
    format(
        "struct _default_$name$_func_ {\n"
        "  constexpr absl::string_view operator()() const {\n"
        "    return absl::string_view($default$, $default_length$);\n"
        "  }\n"
        "};\n");
  }
}

void CordFieldGenerator::GenerateAccessorDeclarations(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("$deprecated_attr$const ::absl::Cord& ${1$$name$$}$() const;\n",
         descriptor_);
  format(
      "$deprecated_attr$void ${1$set_$name$$}$(const ::absl::Cord& value);\n"
      "$deprecated_attr$void ${1$set_$name$$}$(::absl::string_view value);\n",
      std::make_tuple(descriptor_, GeneratedCodeInfo::Annotation::SET));
  format(
      "private:\n"
      "const ::absl::Cord& ${1$_internal_$name$$}$() const;\n"
      "void ${1$_internal_set_$name$$}$(const ::absl::Cord& value);\n"
      "::absl::Cord* ${1$_internal_mutable_$name$$}$();\n"
      "public:\n",
      descriptor_);
}

void CordFieldGenerator::GenerateInlineAccessorDefinitions(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format(
      "inline const ::absl::Cord& $classname$::_internal_$name$() const {\n"
      "  return $field$;\n"
      "}\n"
      "inline const ::absl::Cord& $classname$::$name$() const {\n"
      "$annotate_get$"
      "  // @@protoc_insertion_point(field_get:$full_name$)\n"
      "  return _internal_$name$();\n"
      "}\n"
      "inline void $classname$::_internal_set_$name$(const ::absl::Cord& "
      "value) {\n"
      "  $set_hasbit$\n"
      "  $field$ = value;\n"
      "}\n"
      "inline void $classname$::set_$name$(const ::absl::Cord& value) {\n"
      "$PrepareSplitMessageForWrite$"
      "  _internal_set_$name$(value);\n"
      "$annotate_set$"
      "  // @@protoc_insertion_point(field_set:$full_name$)\n"
      "}\n"
      "inline void $classname$::set_$name$(::absl::string_view value) {\n"
      "$PrepareSplitMessageForWrite$"
      "  $set_hasbit$\n"
      "  $field$ = value;\n"
      "$annotate_set$"
      "  // @@protoc_insertion_point(field_set_string_piece:$full_name$)\n"
      "}\n"
      "inline ::absl::Cord* $classname$::_internal_mutable_$name$() {\n"
      "  $set_hasbit$\n"
      "  return &$field$;\n"
      "}\n");
}

void CordFieldGenerator::GenerateClearingCode(io::Printer* printer) const {
  Formatter format(printer, variables_);
  if (descriptor_->default_value_string().empty()) {
    format("$field$.Clear();\n");
  } else {
    format("$field$ = ::absl::string_view($default$, $default_length$);\n");
  }
}

void CordFieldGenerator::GenerateMergingCode(io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("_this->_internal_set_$name$(from._internal_$name$());\n");
}

void CordFieldGenerator::GenerateSwappingCode(io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("$field$.swap(other->$field$);\n");
}

void CordFieldGenerator::GenerateConstructorCode(io::Printer* printer) const {
  ABSL_CHECK(!ShouldSplit(descriptor_, options_));
  Formatter format(printer, variables_);
  if (!descriptor_->default_value_string().empty()) {
    format("$field$ = ::absl::string_view($default$, $default_length$);\n");
  }
}

void CordFieldGenerator::GenerateDestructorCode(io::Printer* printer) const {
  Formatter format(printer, variables_);
  if (ShouldSplit(descriptor_, options_)) {
    // A cord field in the `Split` struct is automatically destroyed when the
    // split pointer is deleted and should not be explicitly destroyed here.
    return;
  }
  format("$field$.~Cord();\n");
}

void CordFieldGenerator::GenerateArenaDestructorCode(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  // _this is the object being destructed (we are inside a static method here).
  format("_this->$field$. ::absl::Cord::~Cord ();\n");
}

void CordFieldGenerator::GenerateSerializeWithCachedSizesToArray(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  if (descriptor_->type() == FieldDescriptor::TYPE_STRING) {
    GenerateUtf8CheckCodeForCord(
        descriptor_, options_, false,
        absl::Substitute("this->_internal_$0(), ", printer->LookupVar("name")),
        format);
  }
  format(
      "target = stream->Write$declared_type$($number$, "
      "this->_internal_$name$(), "
      "target);\n");
}

void CordFieldGenerator::GenerateByteSize(io::Printer* printer) const {
  Formatter format(printer, variables_);
  format(
      "total_size += $tag_size$ +\n"
      "  ::$proto_ns$::internal::WireFormatLite::$declared_type$Size(\n"
      "    this->_internal_$name$());\n");
}

void CordFieldGenerator::GenerateConstexprAggregateInitializer(
    io::Printer* p) const {
  if (descriptor_->default_value_string().empty()) {
    p->Emit(R"cc(
      /*decltype($field$)*/ {},
    )cc");
  } else {
    p->Emit(
        {{"Split", ShouldSplit(descriptor_, options_) ? "Split::" : ""}},
        R"cc(
          /*decltype($field$)*/ {::absl::strings_internal::MakeStringConstant(
              $classname$::Impl_::$Split$_default_$name$_func_{})},
        )cc");
  }
}

void CordFieldGenerator::GenerateAggregateInitializer(io::Printer* p) const {
  if (ShouldSplit(descriptor_, options_)) {
    p->Emit(R"cc(
      decltype(Impl_::Split::$name$_){},
    )cc");
  } else {
    p->Emit(R"cc(
      decltype($field$){},
    )cc");
  }
}

// ===================================================================

CordOneofFieldGenerator::CordOneofFieldGenerator(
    const FieldDescriptor* descriptor, const Options& options)
    : CordFieldGenerator(descriptor, options) {}

void CordOneofFieldGenerator::GeneratePrivateMembers(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("::absl::Cord *$name$_;\n");
}

void CordOneofFieldGenerator::GenerateStaticMembers(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  if (!descriptor_->default_value_string().empty()) {
    format(
        "struct _default_$name$_func_ {\n"
        "  constexpr absl::string_view operator()() const {\n"
        "    return absl::string_view($default$, $default_length$);\n"
        "  }\n"
        "};"
        "static const ::absl::Cord $default_variable_name$;\n");
  }
}

void CordOneofFieldGenerator::GenerateInlineAccessorDefinitions(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format(
      "inline const ::absl::Cord& $classname$::_internal_$name$() const {\n"
      "  if ($has_field$) {\n"
      "    return *$field$;\n"
      "  }\n"
      "  return $default_variable$;\n"
      "}\n"
      "inline const ::absl::Cord& $classname$::$name$() const {\n"
      "$annotate_get$"
      "  // @@protoc_insertion_point(field_get:$full_name$)\n"
      "  return _internal_$name$();\n"
      "}\n"
      "inline void $classname$::_internal_set_$name$(const ::absl::Cord& "
      "value) {\n"
      "  if ($not_has_field$) {\n"
      "    clear_$oneof_name$();\n"
      "    set_has_$name$();\n"
      "    $field$ = new ::absl::Cord;\n"
      "    if (GetArenaForAllocation() != nullptr) {\n"
      "      GetArenaForAllocation()->Own($field$);\n"
      "    }\n"
      "  }\n"
      "  *$field$ = value;\n"
      "}\n"
      "inline void $classname$::set_$name$(const ::absl::Cord& value) {\n"
      "  _internal_set_$name$(value);\n"
      "$annotate_set$"
      "  // @@protoc_insertion_point(field_set:$full_name$)\n"
      "}\n"
      "inline void $classname$::set_$name$(::absl::string_view value) {\n"
      "  if ($not_has_field$) {\n"
      "    clear_$oneof_name$();\n"
      "    set_has_$name$();\n"
      "    $field$ = new ::absl::Cord;\n"
      "    if (GetArenaForAllocation() != nullptr) {\n"
      "      GetArenaForAllocation()->Own($field$);\n"
      "    }\n"
      "  }\n"
      "  *$field$ = value;\n"
      "$annotate_set$"
      "  // @@protoc_insertion_point(field_set_string_piece:$full_name$)\n"
      "}\n"
      "inline ::absl::Cord* $classname$::_internal_mutable_$name$() {\n"
      "  if ($not_has_field$) {\n"
      "    clear_$oneof_name$();\n"
      "    set_has_$name$();\n"
      "    $field$ = new ::absl::Cord;\n"
      "    if (GetArenaForAllocation() != nullptr) {\n"
      "      GetArenaForAllocation()->Own($field$);\n"
      "    }\n"
      "  }\n"
      "  return $field$;\n"
      "}\n");
}

void CordOneofFieldGenerator::GenerateNonInlineAccessorDefinitions(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  if (!descriptor_->default_value_string().empty()) {
    format(
        "PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT "
        "const ::absl::Cord $classname$::$default_variable_field$(\n"
        "  ::absl::strings_internal::MakeStringConstant(\n"
        "    _default_$name$_func_{}));\n");
  }
}

void CordOneofFieldGenerator::GenerateClearingCode(io::Printer* printer) const {
  Formatter format(printer, variables_);
  format(
      "if (GetArenaForAllocation() == nullptr) {\n"
      "  delete $field$;\n"
      "}\n");
}

void CordOneofFieldGenerator::GenerateSwappingCode(io::Printer* printer) const {
  // Don't print any swapping code. Swapping the union will swap this field.
}

void CordOneofFieldGenerator::GenerateArenaDestructorCode(
    io::Printer* printer) const {
  // We inherit from CordFieldGenerator, so we need to re-override to the
  // default behavior here.
}

// ===================================================================
}  // namespace

std::unique_ptr<FieldGeneratorBase> MakeSingularCordGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc) {
  return absl::make_unique<CordFieldGenerator>(desc, options);
}


std::unique_ptr<FieldGeneratorBase> MakeOneofCordGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc) {
  return absl::make_unique<CordOneofFieldGenerator>(desc, options);
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
