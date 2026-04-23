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
#include "google/protobuf/io/printer.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
namespace {

using Semantic = ::google::protobuf::io::AnnotationCollector::Semantic;

void SetCordVariables(
    const FieldDescriptor* descriptor,
    absl::flat_hash_map<absl::string_view, std::string>* variables,
    const Options& options) {
  (*variables)["default"] = absl::StrCat(
      "\"", absl::CEscape(descriptor->default_value_string()), "\"");
  (*variables)["default_length"] =
      absl::StrCat(descriptor->default_value_string().length());
  (*variables)["full_name"] = std::string(descriptor->full_name());
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
  CordFieldGenerator(const FieldDescriptor* descriptor, const Options& options);
  ~CordFieldGenerator() override = default;

  void GeneratePrivateMembers(io::Printer* p) const override;
  void GenerateAccessorDeclarations(io::Printer* p) const override;
  void GenerateInlineAccessorDefinitions(io::Printer* p) const override;
  void GenerateClearingCode(io::Printer* p) const override;
  void GenerateMergingCode(io::Printer* p) const override;
  void GenerateSwappingCode(io::Printer* p) const override;
  void GenerateArenaDestructorCode(io::Printer* p) const override;
  void GenerateSerializeWithCachedSizesToArray(io::Printer* p) const override;
  void GenerateByteSize(io::Printer* p) const override;
  void GenerateAggregateInitializer(io::Printer* p) const override;
  void GenerateConstexprAggregateInitializer(io::Printer* p) const override;
  ArenaDtorNeeds NeedsArenaDestructor() const override {
    return ArenaDtorNeeds::kRequired;
  }

  void GenerateMemberConstexprConstructor(io::Printer* p) const override {
    if (field_->default_value_string().empty()) {
      p->Emit(R"cc($name$_ {})cc");
    } else {
      p->Emit({{"Split", ShouldSplit(field_, options_) ? "Split::" : ""}},
              R"cc(
                $name$_ {
                  ::absl::strings_internal::MakeStringConstant(
                      $classname$::Impl_::$Split$_default_$name$_func_{})
                }
              )cc");
    }
  }

  void GenerateMemberConstructor(io::Printer* p) const override {
    auto vars = p->WithVars(variables_);
    if (field_->default_value_string().empty()) {
      p->Emit(R"cc($name$_ {})cc");
    } else {
      p->Emit(R"cc($name$_ { ::absl::string_view($default$, $default_length$) }
      )cc");
    }
  }

  void GenerateMemberCopyConstructor(io::Printer* p) const override {
    auto vars = p->WithVars(variables_);
    p->Emit(R"cc($name$_ { from.$name$_ })cc");
  }

  void GenerateOneofCopyConstruct(io::Printer* p) const override {
    auto vars = p->WithVars(variables_);
    p->Emit(R"cc(
      $field_$ = $pb$::Arena::Create<absl::Cord>(arena, *from.$field_$);
    )cc");
  }
};

class CordOneofFieldGenerator : public CordFieldGenerator {
 public:
  CordOneofFieldGenerator(const FieldDescriptor* descriptor,
                          const Options& options);
  ~CordOneofFieldGenerator() override = default;

  void GeneratePrivateMembers(io::Printer* p) const override;
  void GenerateStaticMembers(io::Printer* p) const override;
  void GenerateInlineAccessorDefinitions(io::Printer* p) const override;
  void GenerateNonInlineAccessorDefinitions(io::Printer* p) const override;
  bool RequiresArena(GeneratorFunction func) const override;
  void GenerateClearingCode(io::Printer* p) const override;
  void GenerateSwappingCode(io::Printer* p) const override;
  void GenerateMergingCode(io::Printer* p) const override;
  void GenerateArenaDestructorCode(io::Printer* p) const override;
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

void CordFieldGenerator::GeneratePrivateMembers(io::Printer* p) const {
  auto vars = p->WithVars(variables_);
  p->Emit(R"cc(
    ::absl::Cord $name$_;
  )cc");
  if (!field_->default_value_string().empty()) {
    p->Emit(R"cc(
      struct _default_$name$_func_ {
        constexpr ::absl::string_view operator()() const {
          return ::absl::string_view($default$, $default_length$);
        }
      };
    )cc");
  }
}

void CordFieldGenerator::GenerateAccessorDeclarations(io::Printer* p) const {
  auto vars = p->WithVars(variables_);
  auto v = p->WithVars(AnnotatedAccessors(
      field_, {"", "_internal_", "_internal_mutable_", "_internal_set_"}));
  auto vs =
      p->WithVars(AnnotatedAccessors(field_, {"set_", "add_"}, Semantic::kSet));
  auto va =
      p->WithVars(AnnotatedAccessors(field_, {"mutable_"}, Semantic::kAlias));

  p->Emit(R"cc(
    [[nodiscard]] $deprecated_attr$const ::absl::Cord& $name$() const;
    $deprecated_attr$void $set_name$(const ::absl::Cord& value);
    $deprecated_attr$void $set_name$(::absl::string_view value);

    private:
    const ::absl::Cord& $_internal_name$() const;
    void $_internal_set_name$(const ::absl::Cord& value);
    ::absl::Cord* $nonnull$ $_internal_mutable_name$();

    public:
  )cc");
}

void CordFieldGenerator::GenerateInlineAccessorDefinitions(
    io::Printer* p) const {
  auto v = p->WithVars(variables_);
  p->Emit(R"cc(
    inline const ::absl::Cord& $classname$::_internal_$name_internal$() const {
      return $field_$;
    }
  )cc");
  p->Emit(R"cc(
    inline const ::absl::Cord& $classname$::$name$() const
        ABSL_ATTRIBUTE_LIFETIME_BOUND {
      $WeakDescriptorSelfPin$;
      $annotate_get$;
      // @@protoc_insertion_point(field_get:$full_name$)
      return _internal_$name_internal$();
    }
  )cc");
  p->Emit(R"cc(
    inline void $classname$::_internal_set_$name_internal$(
        const ::absl::Cord& value) {
      $set_hasbit$;
      $field_$ = value;
    }
  )cc");
  p->Emit(R"cc(
    inline void $classname$::set_$name$(const ::absl::Cord& value) {
      $WeakDescriptorSelfPin$;
      $PrepareSplitMessageForWrite$;
      _internal_set_$name_internal$(value);
      $annotate_set$;
      // @@protoc_insertion_point(field_set:$full_name$)
    }
  )cc");
  p->Emit(R"cc(
    inline void $classname$::set_$name$(::absl::string_view value) {
      $WeakDescriptorSelfPin$;
      $PrepareSplitMessageForWrite$;
      $set_hasbit$;
      $field_$ = value;
      $annotate_set$;
      // @@protoc_insertion_point(field_set_string_piece:$full_name$)
    }
  )cc");
  p->Emit(R"cc(
    inline ::absl::Cord* $nonnull$
    $classname$::_internal_mutable_$name_internal$() {
      $set_hasbit$;
      return &$field_$;
    }
  )cc");
}

void CordFieldGenerator::GenerateClearingCode(io::Printer* p) const {
  auto v = p->WithVars(variables_);
  if (field_->default_value_string().empty()) {
    p->Emit(R"cc(
      $field_$.Clear();
    )cc");
  } else {
    p->Emit(R"cc(
      $field_$ = ::absl::string_view($default$, $default_length$);
    )cc");
  }
}

void CordFieldGenerator::GenerateMergingCode(io::Printer* p) const {
  auto v = p->WithVars(variables_);
  p->Emit(R"cc(
    _this->_internal_set_$name$(from._internal_$name$());
  )cc");
}

void CordFieldGenerator::GenerateSwappingCode(io::Printer* p) const {
  auto v = p->WithVars(variables_);
  p->Emit(R"cc(
    $field_$.swap(other->$field_$);
  )cc");
}

void CordFieldGenerator::GenerateArenaDestructorCode(io::Printer* p) const {
  auto v = p->WithVars(variables_);
  // _this is the object being destructed (we are inside a static method here).
  p->Emit(R"cc(
    _this->$field_$.::absl::Cord::~Cord();
  )cc");
}

void CordFieldGenerator::GenerateSerializeWithCachedSizesToArray(
    io::Printer* p) const {
  auto v = p->WithVars(variables_);
  if (field_->type() == FieldDescriptor::TYPE_STRING) {
    GenerateUtf8CheckCodeForCord(
        p, field_, options_, false,
        absl::Substitute("this_._internal_$0(), ", p->LookupVar("name")));
  }
  p->Emit(R"cc(
    target =
        stream->Write$DeclaredType$($number$, this_._internal_$name$(), target);
  )cc");
}

void CordFieldGenerator::GenerateByteSize(io::Printer* p) const {
  auto v = p->WithVars(variables_);
  p->Emit(R"cc(
    total_size += $tag_size$ + $pbi$::WireFormatLite::$DeclaredType$Size(
                                   this_._internal_$name$());
  )cc");
}

void CordFieldGenerator::GenerateConstexprAggregateInitializer(
    io::Printer* p) const {
  if (field_->default_value_string().empty()) {
    p->Emit(R"cc(
      /*decltype($field_$)*/ {},
    )cc");
  } else {
    p->Emit(
        {{"Split", should_split() ? "Split::" : ""}},
        R"cc(
          /*decltype($field_$)*/ {::absl::strings_internal::MakeStringConstant(
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
      decltype($field_$){},
    )cc");
  }
}

// ===================================================================

CordOneofFieldGenerator::CordOneofFieldGenerator(
    const FieldDescriptor* descriptor, const Options& options)
    : CordFieldGenerator(descriptor, options) {}

void CordOneofFieldGenerator::GeneratePrivateMembers(io::Printer* p) const {
  auto v = p->WithVars(variables_);
  p->Emit(R"cc(
    ::absl::Cord* $nonnull$ $name$_;
  )cc");
}

void CordOneofFieldGenerator::GenerateStaticMembers(io::Printer* p) const {
  auto v = p->WithVars(variables_);
  if (!field_->default_value_string().empty()) {
    p->Emit(R"cc(
      struct _default_$name$_func_ {
        constexpr ::absl::string_view operator()() const {
          return ::absl::string_view($default$, $default_length$);
        }
      };
      static const ::absl::Cord $default_variable_name$;
    )cc");
  }
}

void CordOneofFieldGenerator::GenerateInlineAccessorDefinitions(
    io::Printer* p) const {
  auto v = p->WithVars(variables_);
  p->Emit(R"cc(
    inline const ::absl::Cord& $classname$::_internal_$name_internal$() const {
      if ($has_field$) {
        return *$field_$;
      }
      return $default_variable$;
    }
  )cc");
  p->Emit(R"cc(
    inline const ::absl::Cord& $classname$::$name$() const
        ABSL_ATTRIBUTE_LIFETIME_BOUND {
      $WeakDescriptorSelfPin$;
      $annotate_get$;
      // @@protoc_insertion_point(field_get:$full_name$)
      return _internal_$name_internal$();
    }
  )cc");
  p->Emit(R"cc(
    inline void $classname$::set_$name$(const ::absl::Cord& value) {
      $WeakDescriptorSelfPin$;
      if ($not_has_field$) {
        clear_$oneof_name$();
        set_has_$name_internal$();
        $field_$ = $pb$::Arena::Create<::absl::Cord>(GetArena());
      }
      *$field_$ = value;
      $annotate_set$;
      // @@protoc_insertion_point(field_set:$full_name$)
    }
  )cc");
  p->Emit(R"cc(
    inline void $classname$::set_$name$(::absl::string_view value) {
      $WeakDescriptorSelfPin$;
      if ($not_has_field$) {
        clear_$oneof_name$();
        set_has_$name_internal$();
        $field_$ = $pb$::Arena::Create<::absl::Cord>(GetArena());
      }
      *$field_$ = value;
      $annotate_set$;
      // @@protoc_insertion_point(field_set_string_piece:$full_name$)
    }
  )cc");
  p->Emit(R"cc(
    inline ::absl::Cord* $nonnull$
    $classname$::_internal_mutable_$name_internal$() {
      if ($not_has_field$) {
        clear_$oneof_name$();
        set_has_$name_internal$();
        $field_$ = $pb$::Arena::Create<::absl::Cord>(GetArena());
      }
      return $field_$;
    }
  )cc");
}

void CordOneofFieldGenerator::GenerateNonInlineAccessorDefinitions(
    io::Printer* p) const {
  auto v = p->WithVars(variables_);
  if (!field_->default_value_string().empty()) {
    p->Emit(R"cc(
      PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT const ::absl::Cord
          $classname$::$default_variable_field$(
              ::absl::strings_internal::MakeStringConstant(
                  _default_$name$_func_{}));
    )cc");
  }
}

bool CordOneofFieldGenerator::RequiresArena(GeneratorFunction func) const {
  switch (func) {
    case GeneratorFunction::kMergeFrom:
      return true;
  }
  return false;
}

void CordOneofFieldGenerator::GenerateClearingCode(io::Printer* p) const {
  auto v = p->WithVars(variables_);
  p->Emit(R"cc(
    if (GetArena() == nullptr) {
      delete $field_$;
    }
  )cc");
}

void CordOneofFieldGenerator::GenerateSwappingCode(io::Printer* p) const {
  // Don't print any swapping code. Swapping the union will swap this field.
}

void CordOneofFieldGenerator::GenerateArenaDestructorCode(
    io::Printer* p) const {
  // We inherit from CordFieldGenerator, so we need to re-override to the
  // default behavior here.
}

void CordOneofFieldGenerator::GenerateMergingCode(io::Printer* p) const {
  p->Emit(R"cc(
    if (oneof_needs_init) {
      _this->$field_$ = $pb$::Arena::Create<absl::Cord>(arena);
    }
    *_this->$field_$ = *from.$field_$;
  )cc");
}

// ===================================================================
}  // namespace

std::unique_ptr<FieldGeneratorBase> MakeSingularCordGenerator(
    const FieldDescriptor* desc, const Options& options) {
  return absl::make_unique<CordFieldGenerator>(desc, options);
}


std::unique_ptr<FieldGeneratorBase> MakeOneofCordGenerator(
    const FieldDescriptor* desc, const Options& options) {
  return absl::make_unique<CordOneofFieldGenerator>(desc, options);
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
