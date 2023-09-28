// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
          ? absl::StrCat("::", ProtobufNamespace(options),
                         "::internal::GetEmptyCordAlreadyInited()")
          : absl::StrCat(
                QualifiedClassName(descriptor->containing_type(), options),
                "::", MakeDefaultFieldName(descriptor));
}

class CordFieldGenerator : public FieldGeneratorBase {
 public:
  CordFieldGenerator(const FieldDescriptor* descriptor, const Options& options,
                     MessageSCCAnalyzer* scc);
  ~CordFieldGenerator() override = default;

  void GeneratePrivateMembers(io::Printer* printer) const override;
  void GenerateAccessorDeclarations(io::Printer* printer) const override;
  void GenerateInlineAccessorDefinitions(io::Printer* printer) const override;
  void GenerateClearingCode(io::Printer* printer) const override;
  void GenerateMergingCode(io::Printer* printer) const override;
  void GenerateSwappingCode(io::Printer* printer) const override;
  void GenerateConstructorCode(io::Printer* printer) const override;
#ifndef PROTOBUF_EXPLICIT_CONSTRUCTORS
  void GenerateDestructorCode(io::Printer* printer) const override;
#endif  // !PROTOBUF_EXPLICIT_CONSTRUCTORS
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

  void GenerateMemberConstexprConstructor(io::Printer* p) const override {
    if (descriptor_->default_value_string().empty()) {
      p->Emit("$name$_{}");
    } else {
      p->Emit({{"Split", ShouldSplit(descriptor_, options_) ? "Split::" : ""}},
              "$name$_{::absl::strings_internal::MakeStringConstant("
              "$classname$::Impl_::$Split$_default_$name$_func_{})}");
    }
  }

  void GenerateMemberConstructor(io::Printer* p) const override {
    auto vars = p->WithVars(variables_);
    if (descriptor_->default_value_string().empty()) {
      p->Emit("$name$_{}");
    } else {
      p->Emit("$name$_{::absl::string_view($default$, $default_length$)}");
    }
  }

  void GenerateMemberCopyConstructor(io::Printer* p) const override {
    auto vars = p->WithVars(variables_);
    p->Emit("$name$_{from.$name$_}");
  }

  void GenerateOneofCopyConstruct(io::Printer* p) const override {
    auto vars = p->WithVars(variables_);
    p->Emit(R"cc(
      $field$ = ::$proto_ns$::Arena::Create<absl::Cord>(arena, *from.$field$);
    )cc");
  }
};

class CordOneofFieldGenerator : public CordFieldGenerator {
 public:
  CordOneofFieldGenerator(const FieldDescriptor* descriptor,
                          const Options& options, MessageSCCAnalyzer* scc);
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
                                       const Options& options,
                                       MessageSCCAnalyzer* scc)
    : FieldGeneratorBase(descriptor, options, scc) {
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
  auto v = printer->WithVars(variables_);
  printer->Emit(R"cc(
    inline const ::absl::Cord& $classname$::_internal_$name$() const {
      return $field$;
    }
  )cc");
  printer->Emit(R"cc(
    inline const ::absl::Cord& $classname$::$name$() const
        ABSL_ATTRIBUTE_LIFETIME_BOUND {
      $annotate_get$;
      // @@protoc_insertion_point(field_get:$full_name$)
      return _internal_$name$();
    }
  )cc");
  printer->Emit(R"cc(
    inline void $classname$::_internal_set_$name$(const ::absl::Cord& value) {
      $set_hasbit$;
      $field$ = value;
    }
  )cc");
  printer->Emit(R"cc(
    inline void $classname$::set_$name$(const ::absl::Cord& value) {
      $PrepareSplitMessageForWrite$ _internal_set_$name$(value);
      $annotate_set$;
      // @@protoc_insertion_point(field_set:$full_name$)
    }
  )cc");
  printer->Emit(R"cc(
    inline void $classname$::set_$name$(::absl::string_view value) {
      $PrepareSplitMessageForWrite$;
      $set_hasbit$;
      $field$ = value;
      $annotate_set$;
      // @@protoc_insertion_point(field_set_string_piece:$full_name$)
    }
  )cc");
  printer->Emit(R"cc(
    inline ::absl::Cord* $classname$::_internal_mutable_$name$() {
      $set_hasbit$;
      return &$field$;
    }
  )cc");
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
  ABSL_CHECK(!should_split());
  Formatter format(printer, variables_);
  if (!descriptor_->default_value_string().empty()) {
    format("$field$ = ::absl::string_view($default$, $default_length$);\n");
  }
}

#ifndef PROTOBUF_EXPLICIT_CONSTRUCTORS
void CordFieldGenerator::GenerateDestructorCode(io::Printer* printer) const {
  Formatter format(printer, variables_);
  if (should_split()) {
    // A cord field in the `Split` struct is automatically destroyed when the
    // split pointer is deleted and should not be explicitly destroyed here.
    return;
  }
  format("$field$.~Cord();\n");
}
#endif  // !PROTOBUF_EXPLICIT_CONSTRUCTORS

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
        {{"Split", should_split() ? "Split::" : ""}},
        R"cc(
          /*decltype($field$)*/ {::absl::strings_internal::MakeStringConstant(
              $classname$::Impl_::$Split$_default_$name$_func_{})},
        )cc");
  }
}

void CordFieldGenerator::GenerateAggregateInitializer(io::Printer* p) const {
  if (should_split()) {
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
    const FieldDescriptor* descriptor, const Options& options,
    MessageSCCAnalyzer* scc)
    : CordFieldGenerator(descriptor, options, scc) {}

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
  auto v = printer->WithVars(variables_);
  printer->Emit(R"cc(
    inline const ::absl::Cord& $classname$::_internal_$name$() const {
      if ($has_field$) {
        return *$field$;
      }
      return $default_variable$;
    }
  )cc");
  printer->Emit(R"cc(
    inline const ::absl::Cord& $classname$::$name$() const
        ABSL_ATTRIBUTE_LIFETIME_BOUND {
      $annotate_get$;
      // @@protoc_insertion_point(field_get:$full_name$)
      return _internal_$name$();
    }
  )cc");
  printer->Emit(R"cc(
    inline void $classname$::_internal_set_$name$(const ::absl::Cord& value) {
      if ($not_has_field$) {
        clear_$oneof_name$();
        set_has_$name$();
        $field$ = new ::absl::Cord;
        if (GetArena() != nullptr) {
          GetArena()->Own($field$);
        }
      }
      *$field$ = value;
    }
  )cc");
  printer->Emit(R"cc(
    inline void $classname$::set_$name$(const ::absl::Cord& value) {
      _internal_set_$name$(value);
      $annotate_set$;
      // @@protoc_insertion_point(field_set:$full_name$)
    }
  )cc");
  printer->Emit(R"cc(
    inline void $classname$::set_$name$(::absl::string_view value) {
      if ($not_has_field$) {
        clear_$oneof_name$();
        set_has_$name$();
        $field$ = new ::absl::Cord;
        if (GetArena() != nullptr) {
          GetArena()->Own($field$);
        }
      }
      *$field$ = value;
      $annotate_set$;
      // @@protoc_insertion_point(field_set_string_piece:$full_name$)
    }
  )cc");
  printer->Emit(R"cc(
    inline ::absl::Cord* $classname$::_internal_mutable_$name$() {
      if ($not_has_field$) {
        clear_$oneof_name$();
        set_has_$name$();
        $field$ = new ::absl::Cord;
        if (GetArena() != nullptr) {
          GetArena()->Own($field$);
        }
      }
      return $field$;
    }
  )cc");
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
      "if (GetArena() == nullptr) {\n"
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
  return absl::make_unique<CordFieldGenerator>(desc, options, scc);
}


std::unique_ptr<FieldGeneratorBase> MakeOneofCordGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc) {
  return absl::make_unique<CordOneofFieldGenerator>(desc, options, scc);
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
