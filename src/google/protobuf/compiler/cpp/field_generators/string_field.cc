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
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/cpp/field.h"
#include "google/protobuf/compiler/cpp/field_generators/generators.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
namespace {
using ::google::protobuf::internal::cpp::HasHasbit;
using ::google::protobuf::io::AnnotationCollector;
using Sub = ::google::protobuf::io::Printer::Sub;

std::vector<Sub> Vars(const FieldDescriptor* field, const Options& opts) {
  auto trivial_default =
      absl::StrCat("::", ProtobufNamespace(opts),
                   "::internal::GetEmptyStringAlreadyInited()");
  auto lazy_var =
      absl::StrCat(QualifiedClassName(field->containing_type(), opts),
                   "::", MakeDefaultFieldName(field));

  bool empty_default = field->default_value_string().empty();
  bool bytes = field->type() == FieldDescriptor::TYPE_BYTES;

  return {
      {"kDefault", DefaultValue(opts, field)},
      {"kDefaultLen", field->default_value_string().size()},
      {"default_variable_name", MakeDefaultName(field)},
      {"default_variable_field", MakeDefaultFieldName(field)},

      {"kDefaultStr",
       !empty_default ? absl::StrCat(lazy_var, ".get()") : trivial_default},
      {"kDefaultValue",
       !empty_default ? "nullptr" : absl::StrCat("&", trivial_default)},

      {"lazy_var", lazy_var},
      Sub{"lazy_args", !empty_default ? absl::StrCat(lazy_var, ",") : ""}
          .WithSuffix(","),

      {"Set", bytes ? "SetBytes" : "Set"},
  };
}

class SingularString : public FieldGeneratorBase {
 public:
  SingularString(const FieldDescriptor* field, const Options& opts,
                 MessageSCCAnalyzer* scc)
      : FieldGeneratorBase(field, opts, scc), opts_(&opts) {}
  ~SingularString() override = default;

  std::vector<Sub> MakeVars() const override { return Vars(field_, *opts_); }

  bool IsInlined() const override { return is_inlined(); }

  ArenaDtorNeeds NeedsArenaDestructor() const override {
    return is_inlined() ? ArenaDtorNeeds::kOnDemand : ArenaDtorNeeds::kNone;
  }

  void GeneratePrivateMembers(io::Printer* p) const override {
    // Skips the automatic destruction if inlined; rather calls it explicitly if
    // allocating arena is null.
    p->Emit({{"Str", is_inlined() ? "InlinedStringField" : "ArenaStringPtr"}},
            R"cc(
              $pbi$::$Str$ $name$_;
            )cc");
  }

  bool RequiresArena(GeneratorFunction function) const override {
    switch (function) {
      case GeneratorFunction::kMergeFrom:
        return is_oneof();
    }
    return false;
  }

  void GenerateMergingCode(io::Printer* p) const override {
    if (is_oneof()) {
      p->Emit(R"cc(
        if (oneof_needs_init) {
          _this->$field_$.InitDefault();
        }
        _this->$field_$.Set(from._internal_$name$(), arena);
      )cc");
    } else {
      p->Emit(R"cc(
        _this->_internal_set_$name$(from._internal_$name$());
      )cc");
    }
  }

  void GenerateArenaDestructorCode(io::Printer* p) const override {
    if (!is_inlined()) return;

    p->Emit(R"cc(
      if (!_this->_internal_$name$_donated()) {
        _this->$field_$.~InlinedStringField();
      }
    )cc");
  }

  void GenerateNonInlineAccessorDefinitions(io::Printer* p) const override {
    if (EmptyDefault()) return;
    p->Emit(R"cc(
      /*static*/ const ::_pbi::LazyString $Msg$::$default_variable_field${
          {{$kDefault$, $kDefaultLen$}},
          {nullptr},
      };
    )cc");
  }

  void GenerateByteSize(io::Printer* p) const override {
    p->Emit(R"cc(
      total_size += $kTagBytes$ + $pbi$::WireFormatLite::$DeclaredType$Size(
                                      this_._internal_$name$());
    )cc");
  }


  void GenerateCopyAggregateInitializer(io::Printer* p) const override {
    p->Emit(R"cc(
      decltype($field_$){},
    )cc");
  }

  void GenerateMemberConstexprConstructor(io::Printer* p) const override {
    if (is_inlined()) {
      p->Emit("$name$_(nullptr, false)");
    } else {
      p->Emit(
          "$name$_(\n"
          "    &$pbi$::fixed_address_empty_string,\n"
          "    ::_pbi::ConstantInitialized())");
    }
  }

  void GenerateMemberConstructor(io::Printer* p) const override {
    if (is_inlined()) {
      p->Emit("$name$_{}");
    } else if (EmptyDefault()) {
      p->Emit("$name$_(arena)");
    } else {
      p->Emit("$name$_(arena, $default_variable_field$)");
    }
  }

  void GenerateMemberCopyConstructor(io::Printer* p) const override {
    if (is_inlined() || EmptyDefault()) {
      p->Emit("$name$_(arena, from.$name$_)");
    } else {
      p->Emit("$name$_(arena, from.$name$_, $default_variable_name$)");
    }
  }

  void GenerateOneofCopyConstruct(io::Printer* p) const override {
    if (is_inlined() || EmptyDefault()) {
      p->Emit("new (&$field$) decltype($field$){arena, from.$field$};\n");
    } else {
      p->Emit(
          "new (&$field$) decltype($field$){arena, from.$field$,"
          " $default_variable_field$};\n");
    }
  }

  void GenerateStaticMembers(io::Printer* p) const override;
  void GenerateAccessorDeclarations(io::Printer* p) const override;
  void GenerateInlineAccessorDefinitions(io::Printer* p) const override;
  void GenerateClearingCode(io::Printer* p) const override;
  void GenerateMessageClearingCode(io::Printer* p) const override;
  void GenerateSwappingCode(io::Printer* p) const override;
  void GenerateConstructorCode(io::Printer* p) const override;
  void GenerateCopyConstructorCode(io::Printer* p) const override;
  void GenerateDestructorCode(io::Printer* p) const override;
  void GenerateSerializeWithCachedSizesToArray(io::Printer* p) const override;
  void GenerateConstexprAggregateInitializer(io::Printer* p) const override;
  void GenerateAggregateInitializer(io::Printer* p) const override;

 private:
  bool EmptyDefault() const { return field_->default_value_string().empty(); }
  void ReleaseImpl(io::Printer* p) const;
  void SetAllocatedImpl(io::Printer* p) const;

  const Options* opts_;
};

void SingularString::GenerateStaticMembers(io::Printer* p) const {
  if (!EmptyDefault()) {
    p->Emit(R"cc(
      static const $pbi$::LazyString $default_variable_name$;
    )cc");
  }
  if (is_inlined()) {
    // `_init_inline_xxx` is used for initializing default instances.
    p->Emit(R"cc(
      static ::std::true_type _init_inline_$name$_;
    )cc");
  }
}

void SingularString::GenerateAccessorDeclarations(io::Printer* p) const {
  // If we're using SingularString for a field with a ctype, it's
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
  if (internal::cpp::IsStringFieldWithPrivatizedAccessors(*field_)) {
    p->Emit(R"cc(
      private:  // Hidden due to unknown ctype option.
    )cc");
  }

  auto vars = AnnotatedAccessors(field_, {"", "set_allocated_"});
  vars.push_back(Sub{
      "release_name",
      SafeFunctionName(field_->containing_type(), field_, "release_"),
  }
                     .AnnotatedAs(field_));
  auto v1 = p->WithVars(vars);
  auto v2 = p->WithVars(
      AnnotatedAccessors(field_, {"set_"}, AnnotationCollector::kSet));
  auto v3 = p->WithVars(
      AnnotatedAccessors(field_, {"mutable_"}, AnnotationCollector::kAlias));

  p->Emit({{"donated",
            [&] {
              if (!is_inlined()) return;
              p->Emit(R"cc(
                PROTOBUF_ALWAYS_INLINE bool _internal_$name$_donated() const;
              )cc");
            }}},
          R"cc(
            $DEPRECATED$ const ::std::string& $name$() const;
            //~ Using `Arg_ = const std::string&` will make the type of `arg`
            //~ default to `const std::string&`, due to reference collapse. This
            //~ is necessary because there are a handful of users that rely on
            //~ this default.
            template <typename Arg_ = const ::std::string&, typename... Args_>
            $DEPRECATED$ void $set_name$(Arg_&& arg, Args_... args);
            $DEPRECATED$ ::std::string* $nonnull$ $mutable_name$();
            $DEPRECATED$ [[nodiscard]] ::std::string* $nullable$ $release_name$();
            $DEPRECATED$ void $set_allocated_name$(::std::string* $nullable$ value);

            private:
            const ::std::string& _internal_$name$() const;
            PROTOBUF_ALWAYS_INLINE void _internal_set_$name$(const ::std::string& value);
            ::std::string* $nonnull$ _internal_mutable_$name$();
            $donated$;

            public:
          )cc");
}

void UpdateHasbitSet(io::Printer* p, bool is_oneof) {
  if (!is_oneof) {
    p->Emit(R"cc(
      $set_hasbit$;
    )cc");
    return;
  }

  p->Emit(R"cc(
    if ($not_has_field$) {
      clear_$oneof_name$();

      set_has_$name_internal$();
      $field_$.InitDefault();
    }
  )cc");
}

void ArgsForSetter(io::Printer* p, bool inlined) {
  if (!inlined) {
    p->Emit("GetArena()");
    return;
  }
  p->Emit(
      "GetArena(), _internal_$name_internal$_donated(), "
      "&$donating_states_word$, $mask_for_undonate$, this");
}

void SingularString::ReleaseImpl(io::Printer* p) const {
  if (is_oneof()) {
    p->Emit(R"cc(
      if ($not_has_field$) {
        return nullptr;
      }
      clear_has_$oneof_name$();
      return $field_$.Release();
    )cc");
    return;
  }

  if (!HasHasbit(field_)) {
    p->Emit(R"cc(
      return $field_$.Release();
    )cc");
    return;
  }

  if (is_inlined()) {
    p->Emit(R"cc(
      if (($has_hasbit$) == 0) {
        return nullptr;
      }
      $clear_hasbit$;

      return $field_$.Release(GetArena(), _internal_$name_internal$_donated());
    )cc");
    return;
  }

  p->Emit(R"cc(
    if (($has_hasbit$) == 0) {
      return nullptr;
    }
    $clear_hasbit$;
  )cc");

  if (!EmptyDefault()) {
    p->Emit(R"cc(
      return $field_$.Release();
    )cc");
    return;
  }

  p->Emit(R"cc(
    auto* released = $field_$.Release();
    if ($pbi$::DebugHardenForceCopyDefaultString()) {
      $field_$.Set("", $set_args$);
    }
    return released;
  )cc");
}

void SingularString::SetAllocatedImpl(io::Printer* p) const {
  if (is_oneof()) {
    p->Emit(R"cc(
      if (has_$oneof_name$()) {
        clear_$oneof_name$();
      }
      if (value != nullptr) {
        set_has_$name_internal$();
        $field_$.InitAllocated(value, GetArena());
      }
    )cc");
    return;
  }

  if (HasHasbit(field_)) {
    p->Emit(R"cc(
      if (value != nullptr) {
        $set_hasbit$
      } else {
        $clear_hasbit$
      }
    )cc");
  }

  if (is_inlined()) {
    // Currently, string fields with default value can't be inlined.
    p->Emit(R"cc(
      $field_$.SetAllocated(nullptr, value, $set_args$);
    )cc");
    return;
  }

  p->Emit(R"cc(
    $field_$.SetAllocated(value, $set_args$);
  )cc");

  if (EmptyDefault()) {
    p->Emit(R"cc(
      if ($pbi$::DebugHardenForceCopyDefaultString() && $field_$.IsDefault()) {
        $field_$.Set("", $set_args$);
      }
    )cc");
  }
}

void SingularString::GenerateInlineAccessorDefinitions(io::Printer* p) const {
  std::vector<Sub> vars = {
      {"if_IsDefault",
       [&] {
         if (EmptyDefault() || is_oneof()) return;
         p->Emit(R"cc(
           if ($field_$.IsDefault()) {
             return $default_variable_field$.get();
           }
         )cc");
       }},
      {"update_hasbit", [&] { UpdateHasbitSet(p, is_oneof()); }},
      {"set_args", [&] { ArgsForSetter(p, is_inlined()); }},
      {"check_hasbit",
       [&] {
         if (!is_oneof()) return;
         p->Emit(R"cc(
           if ($not_has_field$) {
             return $kDefaultStr$;
           }
         )cc");
       }},
      {"release_name",
       SafeFunctionName(field_->containing_type(), field_, "release_")},
      {"release_impl", [&] { ReleaseImpl(p); }},
      {"set_allocated_impl", [&] { SetAllocatedImpl(p); }},
  };
  absl::string_view code =
      R"cc(
    inline const ::std::string& $Msg$::$name$() const
        ABSL_ATTRIBUTE_LIFETIME_BOUND {
      $WeakDescriptorSelfPin$;
      $annotate_get$;
      // @@protoc_insertion_point(field_get:$pkg.Msg.field$)
      $if_IsDefault$;
      return _internal_$name_internal$();
    }
    template <typename Arg_, typename... Args_>
    PROTOBUF_ALWAYS_INLINE void $Msg$::set_$name$(Arg_&& arg, Args_... args) {
      $WeakDescriptorSelfPin$;
      $TsanDetectConcurrentMutation$;
      $PrepareSplitMessageForWrite$;
      $update_hasbit$;
      $field_$.$Set$(static_cast<Arg_&&>(arg), args..., $set_args$);
      $annotate_set$;
      // @@protoc_insertion_point(field_set:$pkg.Msg.field$)
    }
    inline ::std::string* $nonnull$ $Msg$::mutable_$name$()
        ABSL_ATTRIBUTE_LIFETIME_BOUND {
      $WeakDescriptorSelfPin$;
      $PrepareSplitMessageForWrite$;
      ::std::string* _s = _internal_mutable_$name_internal$();
      $annotate_mutable$;
      // @@protoc_insertion_point(field_mutable:$pkg.Msg.field$)
      return _s;
    }
    inline const ::std::string& $Msg$::_internal_$name_internal$() const {
      $TsanDetectConcurrentRead$;
      $check_hasbit$;
      return $field_$.Get();
    }
    inline void $Msg$::_internal_set_$name_internal$(const ::std::string& value) {
      $TsanDetectConcurrentMutation$;
      $update_hasbit$;
      //~ Don't use $Set$ here; we always want the std::string variant
      //~ regardless of whether this is a `bytes` field.
      $field_$.Set(value, $set_args$);
    }
    inline ::std::string* $nonnull$ $Msg$::_internal_mutable_$name_internal$() {
      $TsanDetectConcurrentMutation$;
      $update_hasbit$;
      return $field_$.Mutable($lazy_args$, $set_args$);
    }
    inline ::std::string* $nullable$ $Msg$::$release_name$() {
      $WeakDescriptorSelfPin$;
      $TsanDetectConcurrentMutation$;
      $annotate_release$;
      $PrepareSplitMessageForWrite$;
      // @@protoc_insertion_point(field_release:$pkg.Msg.field$)
      $release_impl$;
    }
    inline void $Msg$::set_allocated_$name$(::std::string* $nullable$ value) {
      $WeakDescriptorSelfPin$;
      $TsanDetectConcurrentMutation$;
      $PrepareSplitMessageForWrite$;
      $set_allocated_impl$;
      $annotate_set$;
      // @@protoc_insertion_point(field_set_allocated:$pkg.Msg.field$)
    })cc";
  p->Emit(vars, code);

  if (is_inlined()) {
    p->Emit(R"cc(
      inline bool $Msg$::_internal_$name_internal$_donated() const {
        return $inlined_string_donated$;
      }
    )cc");
  }
}

void SingularString::GenerateClearingCode(io::Printer* p) const {
  if (is_oneof()) {
    p->Emit(R"cc(
      $field_$.Destroy();
    )cc");
    return;
  }

  if (EmptyDefault()) {
    p->Emit(R"cc(
      $field_$.ClearToEmpty();
    )cc");
    return;
  }

  ABSL_DCHECK(!is_inlined());
  p->Emit(R"cc(
    $field_$.ClearToDefault($lazy_var$, GetArena());
  )cc");
}

void SingularString::GenerateMessageClearingCode(io::Printer* p) const {
  if (is_oneof()) {
    p->Emit(R"cc(
      $field_$.Destroy();
    )cc");
    return;
  }

  // Two-dimension specialization here: supporting arenas, field presence, or
  // not, and default value is the empty string or not. Complexity here ensures
  // the minimal number of branches / amount of extraneous code at runtime
  // (given that the below methods are inlined one-liners)!

  // If we have a hasbit, then the Clear() method of the protocol buffer
  // will have checked that this field is set.  If so, we can avoid redundant
  // checks against the default variable.

  if (is_inlined() && HasHasbit(field_)) {
    // Calling mutable_$name$() gives us a string reference and sets the has bit
    // for $name$ (in proto2).  We may get here when the string field is inlined
    // but the string's contents have not been changed by the user, so we cannot
    // make an assertion about the contents of the string and could never make
    // an assertion about the string instance.
    //
    // For non-inlined strings, we distinguish from non-default by comparing
    // instances, rather than contents.
    p->Emit(R"cc(
      $DCHK$(!$field_$.IsDefault());
    )cc");
  }

  if (!EmptyDefault()) {
    // Clear to a non-empty default is more involved, as we try to use the
    // Arena if one is present and may need to reallocate the string.
    p->Emit(R"cc(
      $field_$.ClearToDefault($lazy_var$, GetArena());
    )cc");
    return;
  }

  p->Emit({{"Clear",
            HasHasbit(field_) ? "ClearNonDefaultToEmpty" : "ClearToEmpty"}},
          R"cc(
            $field_$.$Clear$();
          )cc");
}

void SingularString::GenerateSwappingCode(io::Printer* p) const {
  if (is_oneof()) {
    // Don't print any swapping code. Swapping the union will swap this field.
    return;
  }

  if (!is_inlined()) {
    p->Emit(R"cc(
      ::_pbi::ArenaStringPtr::InternalSwap(&$field_$, &other->$field_$, arena);
    )cc");
    return;
  }

  p->Emit(R"cc(
    {
      bool lhs_dtor_registered = ($inlined_string_donated_array$[0] & 1) == 0;
      bool rhs_dtor_registered =
          (other->$inlined_string_donated_array$[0] & 1) == 0;
      ::_pbi::InlinedStringField::InternalSwap(
          &$field_$, lhs_dtor_registered, this, &other->$field_$,
          rhs_dtor_registered, other, arena);
    }
  )cc");
}

void SingularString::GenerateConstructorCode(io::Printer* p) const {
  if ((is_inlined() && EmptyDefault()) || is_oneof()) return;
  ABSL_DCHECK(!is_inlined());

  p->Emit(R"cc(
    $field_$.InitDefault();
  )cc");

  if (IsString(field_) && EmptyDefault()) {
    p->Emit(R"cc(
      if ($pbi$::DebugHardenForceCopyDefaultString()) {
        $field_$.Set("", GetArena());
      }
    )cc");
  }
}

void SingularString::GenerateCopyConstructorCode(io::Printer* p) const {
  GenerateConstructorCode(p);

  if (is_inlined()) {
    p->Emit(R"cc(
      new (&_this->$field_$)::_pbi::InlinedStringField;
    )cc");
  }

  p->Emit(
      {{"hazzer",
        [&] {
          if (HasHasbit(field_)) {
            p->Emit(R"cc((from.$has_hasbit$) != 0)cc");
          } else {
            p->Emit(R"cc(!from._internal_$name$().empty())cc");
          }
        }},
       {"set_args",
        [&] {
          if (!is_inlined()) {
            p->Emit("_this->GetArena()");
          } else {
            p->Emit(
                "_this->GetArena(), "
                "_this->_internal_$name$_donated(), "
                "&_this->$donating_states_word$, $mask_for_undonate$, _this");
          }
        }}},
      R"cc(
        if ($hazzer$) {
          _this->$field_$.Set(from._internal_$name$(), $set_args$);
        }
      )cc");
}

void SingularString::GenerateDestructorCode(io::Printer* p) const {
  if (is_inlined()) {
    ABSL_DCHECK(!should_split());
    return;
  }

  if (should_split()) {
    p->Emit(R"cc(
      $cached_split_ptr$->$name$_.Destroy();
    )cc");
    return;
  }

  p->Emit(R"cc(
    this_.$field_$.Destroy();
  )cc");
}

void SingularString::GenerateSerializeWithCachedSizesToArray(
    io::Printer* p) const {
  p->Emit({{"utf8_check",
            [&] {
              GenerateUtf8CheckCodeForString(p, field_, options_, false,
                                             "_s.data(), "
                                             "static_cast<int>(_s.length()),");
            }}},
          R"cc(
            const ::std::string& _s = this_._internal_$name$();
            $utf8_check$;
            target = stream->Write$DeclaredType$MaybeAliased($number$, _s, target);
          )cc");
}


void SingularString::GenerateConstexprAggregateInitializer(
    io::Printer* p) const {
  if (is_inlined()) {
    p->Emit(R"cc(
      /*decltype($field_$)*/ {nullptr, false},
    )cc");
  } else {
    p->Emit(R"cc(
      /*decltype($field_$)*/ {
          &::_pbi::fixed_address_empty_string,
          ::_pbi::ConstantInitialized{},
      },
    )cc");
  }
}

void SingularString::GenerateAggregateInitializer(io::Printer* p) const {
  if (should_split()) {
    ABSL_CHECK(!is_inlined());
    p->Emit(R"cc(
      decltype(Impl_::Split::$name$_){},
    )cc");
  } else if (!is_inlined()) {
    p->Emit(R"cc(
      decltype($field_$){},
    )cc");
  } else {
    p->Emit(R"cc(
      decltype($field_$){arena},
    )cc");
  }
}

class RepeatedString : public FieldGeneratorBase {
 public:
  RepeatedString(const FieldDescriptor* field, const Options& opts,
                 MessageSCCAnalyzer* scc)
      : FieldGeneratorBase(field, opts, scc), opts_(&opts) {}
  ~RepeatedString() override = default;

  std::vector<Sub> MakeVars() const override { return Vars(field_, *opts_); }

  void GeneratePrivateMembers(io::Printer* p) const override {
    if (should_split()) {
      p->Emit(R"cc(
        $pbi$::RawPtr<$pb$::RepeatedPtrField<::std::string>> $name$_;
      )cc");
    } else {
      p->Emit(R"cc(
        $pb$::RepeatedPtrField<::std::string> $name$_;
      )cc");
    }
  }

  void GenerateClearingCode(io::Printer* p) const override {
    if (should_split()) {
      p->Emit("$field_$.ClearIfNotDefault();\n");
    } else {
      p->Emit("$field_$.Clear();\n");
    }
  }

  void GenerateMergingCode(io::Printer* p) const override {
    // TODO: experiment with simplifying this to be
    // `if (!from.empty()) { body(); }` for both split and non-split cases.
    auto body = [&] {
      p->Emit(R"cc(
        _this->_internal_mutable_$name$()->MergeFrom(from._internal_$name$());
      )cc");
    };
    if (!should_split()) {
      body();
    } else {
      p->Emit({{"body", body}}, R"cc(
        if (!from.$field_$.IsDefault()) {
          $body$;
        }
      )cc");
    }
  }

  void GenerateSwappingCode(io::Printer* p) const override {
    ABSL_CHECK(!should_split());
    p->Emit(R"cc(
      $field_$.InternalSwap(&other->$field_$);
    )cc");
  }

  void GenerateDestructorCode(io::Printer* p) const override {
    if (should_split()) {
      p->Emit(R"cc(
        this_.$field_$.DeleteIfNotDefault();
      )cc");
    }
  }

  void GenerateConstructorCode(io::Printer* p) const override {}

  void GenerateCopyConstructorCode(io::Printer* p) const override {
    if (should_split()) {
      p->Emit(R"cc(
        if (!from._internal_$name$().empty()) {
          _internal_mutable_$name$()->MergeFrom(from._internal_$name$());
        }
      )cc");
    }
  }

  void GenerateByteSize(io::Printer* p) const override {
    p->Emit(R"cc(
      total_size +=
          $kTagBytes$ * $pbi$::FromIntSize(this_._internal_$name$().size());
      for (int i = 0, n = this_._internal_$name$().size(); i < n; ++i) {
        total_size += $pbi$::WireFormatLite::$DeclaredType$Size(
            this_._internal_$name$().Get(i));
      }
    )cc");
  }


  void GenerateAccessorDeclarations(io::Printer* p) const override;
  void GenerateInlineAccessorDefinitions(io::Printer* p) const override;
  void GenerateSerializeWithCachedSizesToArray(io::Printer* p) const override;

 private:
  const Options* opts_;
};

void RepeatedString::GenerateAccessorDeclarations(io::Printer* p) const {
  if (internal::cpp::IsStringFieldWithPrivatizedAccessors(*field_)) {
    p->Emit(R"cc(
      private:  // Hidden due to unknown ctype option.
    )cc");
  }

  auto v1 = p->WithVars(AnnotatedAccessors(field_, {"", "_internal_"}));
  auto v2 = p->WithVars(
      AnnotatedAccessors(field_, {"set_", "add_"}, AnnotationCollector::kSet));
  auto v3 = p->WithVars(
      AnnotatedAccessors(field_, {"mutable_"}, AnnotationCollector::kAlias));

  p->Emit(R"cc(
    $DEPRECATED$ const ::std::string& $name$(int index) const;
    $DEPRECATED$ ::std::string* $nonnull$ $mutable_name$(int index);
    template <typename Arg_ = const ::std::string&, typename... Args_>
    $DEPRECATED$ void set_$name$(int index, Arg_&& value, Args_... args);
    $DEPRECATED$ ::std::string* $nonnull$ $add_name$();
    template <typename Arg_ = const ::std::string&, typename... Args_>
    $DEPRECATED$ void $add_name$(Arg_&& value, Args_... args);
    $DEPRECATED$ const $pb$::RepeatedPtrField<::std::string>& $name$() const;
    $DEPRECATED$ $pb$::RepeatedPtrField<::std::string>* $nonnull$ $mutable_name$();

    private:
    const $pb$::RepeatedPtrField<::std::string>& _internal_$name$() const;
    $pb$::RepeatedPtrField<::std::string>* $nonnull$ _internal_mutable_$name$();

    public:
  )cc");
}

void RepeatedString::GenerateInlineAccessorDefinitions(io::Printer* p) const {
  bool bytes = field_->type() == FieldDescriptor::TYPE_BYTES;
  p->Emit(
      {
          GetEmitRepeatedFieldGetterSub(*opts_, p),
          {"bytes_tag",
           [&] {
             if (bytes) {
               p->Emit(", $pbi$::BytesTag{}");
             }
           }},
          GetEmitRepeatedFieldMutableSub(*opts_, p),
      },
      R"cc(
        inline ::std::string* $nonnull$ $Msg$::add_$name$()
            ABSL_ATTRIBUTE_LIFETIME_BOUND {
          $WeakDescriptorSelfPin$;
          $TsanDetectConcurrentMutation$;
          ::std::string* _s = _internal_mutable_$name_internal$()->Add();
          $annotate_add_mutable$;
          // @@protoc_insertion_point(field_add_mutable:$pkg.Msg.field$)
          return _s;
        }
        inline const ::std::string& $Msg$::$name$(int index) const
            ABSL_ATTRIBUTE_LIFETIME_BOUND {
          $WeakDescriptorSelfPin$;
          $annotate_get$;
          // @@protoc_insertion_point(field_get:$pkg.Msg.field$)
          return $getter$;
        }
        inline ::std::string* $nonnull$ $Msg$::mutable_$name$(int index)
            ABSL_ATTRIBUTE_LIFETIME_BOUND {
          $WeakDescriptorSelfPin$;
          $annotate_mutable$;
          // @@protoc_insertion_point(field_mutable:$pkg.Msg.field$)
          return $mutable$;
        }
        template <typename Arg_, typename... Args_>
        inline void $Msg$::set_$name$(int index, Arg_&& value, Args_... args) {
          $WeakDescriptorSelfPin$;
          $pbi$::AssignToString(*$mutable$, ::std::forward<Arg_>(value),
                                args... $bytes_tag$);
          $annotate_set$;
          // @@protoc_insertion_point(field_set:$pkg.Msg.field$)
        }
        template <typename Arg_, typename... Args_>
        inline void $Msg$::add_$name$(Arg_&& value, Args_... args) {
          $WeakDescriptorSelfPin$;
          $TsanDetectConcurrentMutation$;
          $pbi$::AddToRepeatedPtrField(*_internal_mutable_$name_internal$(),
                                       ::std::forward<Arg_>(value),
                                       args... $bytes_tag$);
          $annotate_add$;
          // @@protoc_insertion_point(field_add:$pkg.Msg.field$)
        }
        inline const $pb$::RepeatedPtrField<::std::string>& $Msg$::$name$()
            const ABSL_ATTRIBUTE_LIFETIME_BOUND {
          $WeakDescriptorSelfPin$;
          $annotate_list$;
          // @@protoc_insertion_point(field_list:$pkg.Msg.field$)
          return _internal_$name_internal$();
        }
        inline $pb$::RepeatedPtrField<::std::string>* $nonnull$
        $Msg$::mutable_$name$() ABSL_ATTRIBUTE_LIFETIME_BOUND {
          $WeakDescriptorSelfPin$;
          $annotate_mutable_list$;
          // @@protoc_insertion_point(field_mutable_list:$pkg.Msg.field$)
          $TsanDetectConcurrentMutation$;
          return _internal_mutable_$name_internal$();
        }
      )cc");
  if (should_split()) {
    p->Emit(R"cc(
      inline const $pb$::RepeatedPtrField<::std::string>&
      $Msg$::_internal_$name_internal$() const {
        $TsanDetectConcurrentRead$;
        return *$field_$;
      }
      inline $pb$::RepeatedPtrField<::std::string>* $nonnull$
      $Msg$::_internal_mutable_$name_internal$() {
        $TsanDetectConcurrentRead$;
        $PrepareSplitMessageForWrite$;
        if ($field_$.IsDefault()) {
          $field_$.Set(
              $pb$::Arena::Create<$pb$::RepeatedPtrField<::std::string>>(
                  GetArena()));
        }
        return $field_$.Get();
      }
    )cc");
  } else {
    p->Emit(R"cc(
      inline const $pb$::RepeatedPtrField<::std::string>&
      $Msg$::_internal_$name_internal$() const {
        $TsanDetectConcurrentRead$;
        return $field_$;
      }
      inline $pb$::RepeatedPtrField<::std::string>* $nonnull$
      $Msg$::_internal_mutable_$name_internal$() {
        $TsanDetectConcurrentRead$;
        return &$field_$;
      }
    )cc");
  }
}

void RepeatedString::GenerateSerializeWithCachedSizesToArray(
    io::Printer* p) const {
  p->Emit({{"utf8_check",
            [&] {
              GenerateUtf8CheckCodeForString(
                  p, field_, options_, false,
                  "s.data(), static_cast<int>(s.length()),");
            }}},
          R"cc(
            for (int i = 0, n = this_._internal_$name$_size(); i < n; ++i) {
              const auto& s = this_._internal_$name$().Get(i);
              $utf8_check$;
              target = stream->Write$DeclaredType$($number$, s, target);
            }
          )cc");
}

}  // namespace

std::unique_ptr<FieldGeneratorBase> MakeSinguarStringGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc) {
  return absl::make_unique<SingularString>(desc, options, scc);
}

std::unique_ptr<FieldGeneratorBase> MakeRepeatedStringGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc) {
  return absl::make_unique<RepeatedString>(desc, options, scc);
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
