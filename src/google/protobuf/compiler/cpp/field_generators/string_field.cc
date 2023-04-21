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
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/memory/memory.h"
#include "google/protobuf/compiler/cpp/field.h"
#include "google/protobuf/compiler/cpp/field_generators/generators.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"

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
  bool is_bytes = field->type() == FieldDescriptor::TYPE_BYTES;

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

      {"byte", is_bytes ? "void" : "char"},
      {"Set", is_bytes ? "SetBytes" : "Set"},
  };
}

class SingularString : public FieldGeneratorBase {
 public:
  SingularString(const FieldDescriptor* field, const Options& opts)
      : FieldGeneratorBase(field, opts),
        field_(field),
        opts_(&opts),
        is_oneof_(field->real_containing_oneof() != nullptr),
        inlined_(IsStringInlined(field, opts)) {}
  ~SingularString() override = default;

  std::vector<Sub> MakeVars() const override { return Vars(field_, *opts_); }

  bool IsInlined() const override { return inlined_; }

  ArenaDtorNeeds NeedsArenaDestructor() const override {
    return inlined_ ? ArenaDtorNeeds::kOnDemand : ArenaDtorNeeds::kNone;
  }

  void GeneratePrivateMembers(io::Printer* p) const override {
    // Skips the automatic destruction if inlined; rather calls it explicitly if
    // allocating arena is null.
    p->Emit({{"Str", inlined_ ? "InlinedStringField" : "ArenaStringPtr"}}, R"cc(
      $pbi$::$Str$ $name$_;
    )cc");
  }

  void GenerateMergingCode(io::Printer* p) const override {
    p->Emit(R"cc(
      _this->_internal_set_$name$(from._internal_$name$());
    )cc");
  }

  void GenerateArenaDestructorCode(io::Printer* p) const override {
    if (!inlined_) return;

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
                                      this->_internal_$name$());
    )cc");
  }

  void GenerateCopyAggregateInitializer(io::Printer* p) const override {
    p->Emit(R"cc(
      decltype($field_$) {}
    )cc");
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

  const FieldDescriptor* field_;
  const Options* opts_;
  bool is_oneof_;
  bool inlined_;
};

void SingularString::GenerateStaticMembers(io::Printer* p) const {
  if (!EmptyDefault()) {
    p->Emit(R"cc(
      static const $pbi$::LazyString $default_variable_name$;
    )cc");
  }
  if (inlined_) {
    // `_init_inline_xxx` is used for initializing default instances.
    p->Emit(R"cc(
      static std::true_type _init_inline_$name$_;
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
  bool unknown_ctype =
      field_->options().ctype() != internal::cpp::EffectiveStringCType(field_);

  if (unknown_ctype) {
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

  p->Emit(
      {{"donated",
        [&] {
          if (!inlined_) return;
          p->Emit(R"cc(
            inline PROTOBUF_ALWAYS_INLINE bool _internal_$name$_donated() const;
          )cc");
        }}},
      R"cc(
        $DEPRECATED$ const std::string& $name$() const;
        //~ Using `Arg_ = const std::string&` will make the type of `arg`
        //~ default to `const std::string&`, due to reference collapse. This is
        //~ necessary because there are a handful of users that rely on this
        //~ default.
        template <typename Arg_ = const std::string&, typename... Args_>
        $DEPRECATED$ void $set_name$(Arg_&& arg, Args_... args);
        $DEPRECATED$ std::string* $mutable_name$();
        $DEPRECATED$ PROTOBUF_NODISCARD std::string* $release_name$();
        $DEPRECATED$ void $set_allocated_name$(std::string* ptr);

        private:
        const std::string& _internal_$name$() const;
        inline PROTOBUF_ALWAYS_INLINE void _internal_set_$name$(
            const std::string& value);
        std::string* _internal_mutable_$name$();
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

      set_has_$name$();
      $field_$.InitDefault();
    }
  )cc");
}

void ArgsForSetter(io::Printer* p, bool inlined) {
  if (!inlined) {
    p->Emit("GetArenaForAllocation()");
    return;
  }
  p->Emit(
      "GetArenaForAllocation(), _internal_$name$_donated(), "
      "&$donating_states_word$, $mask_for_undonate$, this");
}

void SingularString::ReleaseImpl(io::Printer* p) const {
  if (is_oneof_) {
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

  if (inlined_) {
    p->Emit(R"cc(
      if (($has_hasbit$) == 0) {
        return nullptr;
      }
      $clear_hasbit$;

      return $field_$.Release(GetArenaForAllocation(), _internal_$name$_donated());
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
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    $field_$.Set("", $set_args$);
#endif  // PROTOBUF_FORCE_COPY_DEFAULT_STRING
    return released;
  )cc");
}

void SingularString::SetAllocatedImpl(io::Printer* p) const {
  if (is_oneof_) {
    p->Emit(R"cc(
      if (has_$oneof_name$()) {
        clear_$oneof_name$();
      }
      if (value != nullptr) {
        set_has_$name$();
        $field_$.InitAllocated(value, GetArenaForAllocation());
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

  if (inlined_) {
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
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
      if ($field_$.IsDefault()) {
        $field_$.Set("", $set_args$);
      }
#endif  // PROTOBUF_FORCE_COPY_DEFAULT_STRING
    )cc");
  }
}

void SingularString::GenerateInlineAccessorDefinitions(io::Printer* p) const {
  p->Emit(
      {
          {"if_IsDefault",
           [&] {
             if (EmptyDefault() || is_oneof_) return;
             p->Emit(R"cc(
               if ($field_$.IsDefault()) {
                 return $default_variable_field$.get();
               }
             )cc");
           }},
          {"update_hasbit", [&] { UpdateHasbitSet(p, is_oneof_); }},
          {"set_args", [&] { ArgsForSetter(p, inlined_); }},
          {"check_hasbit",
           [&] {
             if (!is_oneof_) return;
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
      },
      R"cc(
        inline const std::string& $Msg$::$name$() const {
          $annotate_get$;
          // @@protoc_insertion_point(field_get:$pkg.Msg.field$)
          $if_IsDefault$;
          return _internal_$name$();
        }
        template <typename Arg_, typename... Args_>
        inline PROTOBUF_ALWAYS_INLINE void $Msg$::set_$name$(Arg_&& arg,
                                                             Args_... args) {
          $PrepareSplitMessageForWrite$;
          $update_hasbit$;
          $field_$.$Set$(static_cast<Arg_&&>(arg), args..., $set_args$);
          $annotate_set$;
          // @@protoc_insertion_point(field_set:$pkg.Msg.field$)
        }
        inline std::string* $Msg$::mutable_$name$() {
          $PrepareSplitMessageForWrite$;
          std::string* _s = _internal_mutable_$name$();
          $annotate_mutable$;
          // @@protoc_insertion_point(field_mutable:$pkg.Msg.field$)
          return _s;
        }
        inline const std::string& $Msg$::_internal_$name$() const {
          $check_hasbit$;
          return $field_$.Get();
        }
        inline void $Msg$::_internal_set_$name$(const std::string& value) {
          $update_hasbit$;
          //~ Don't use $Set$ here; we always want the std::string variant
          //~ regardless of whether this is a `bytes` field.
          $field_$.Set(value, $set_args$);
        }
        inline std::string* $Msg$::_internal_mutable_$name$() {
          $update_hasbit$;
          return $field_$.Mutable($lazy_args$, $set_args$);
        }
        inline std::string* $Msg$::$release_name$() {
          $annotate_release$;
          $PrepareSplitMessageForWrite$;
          // @@protoc_insertion_point(field_release:$pkg.Msg.field$)
          $release_impl$;
        }
        inline void $Msg$::set_allocated_$name$(std::string* value) {
          $PrepareSplitMessageForWrite$;
          $set_allocated_impl$;
          $annotate_set$;
          // @@protoc_insertion_point(field_set_allocated:$pkg.Msg.field$)
        }
      )cc");

  if (inlined_) {
    p->Emit(R"cc(
      inline bool $Msg$::_internal_$name$_donated() const {
        return $inlined_string_donated$;
      }
    )cc");
  }
}

void SingularString::GenerateClearingCode(io::Printer* p) const {
  if (is_oneof_) {
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

  ABSL_DCHECK(!inlined_);
  p->Emit(R"cc(
    $field_$.ClearToDefault($lazy_var$, GetArenaForAllocation());
  )cc");
}

void SingularString::GenerateMessageClearingCode(io::Printer* p) const {
  if (is_oneof_) {
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

  if (inlined_ && HasHasbit(field_)) {
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
      $field_$.ClearToDefault($lazy_var$, GetArenaForAllocation());
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
  if (is_oneof_) {
    // Don't print any swapping code. Swapping the union will swap this field.
    return;
  }

  if (!inlined_) {
    p->Emit(R"cc(
      ::_pbi::ArenaStringPtr::InternalSwap(&$field_$, lhs_arena,
                                           &other->$field_$, rhs_arena);
    )cc");
    return;
  }

  p->Emit(R"cc(
    {
      bool lhs_dtor_registered = ($inlined_string_donated_array$[0] & 1) == 0;
      bool rhs_dtor_registered =
          (other->$inlined_string_donated_array$[0] & 1) == 0;
      ::_pbi::InlinedStringField::InternalSwap(
          &$field_$, lhs_arena, lhs_dtor_registered, this, &other->$field_$,
          rhs_arena, rhs_dtor_registered, other);
    }
  )cc");
}

void SingularString::GenerateConstructorCode(io::Printer* p) const {
  if ((inlined_ && EmptyDefault()) || is_oneof_) return;
  ABSL_DCHECK(!inlined_);

  p->Emit(R"cc(
    $field_$.InitDefault();
  )cc");

  if (IsString(field_, *opts_) && EmptyDefault()) {
    p->Emit(R"cc(
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
      $field_$.Set("", GetArenaForAllocation());
#endif  // PROTOBUF_FORCE_COPY_DEFAULT_STRING
    )cc");
  }
}

void SingularString::GenerateCopyConstructorCode(io::Printer* p) const {
  GenerateConstructorCode(p);

  if (inlined_) {
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
          if (!inlined_) {
            p->Emit("_this->GetArenaForAllocation()");
          } else {
            p->Emit(
                "_this->GetArenaForAllocation(), "
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
  if (inlined_) {
    // Explicitly calls ~InlinedStringField as its automatic call is disabled.
    // Destructor has been implicitly skipped as a union.
    ABSL_DCHECK(!ShouldSplit(field_, *opts_));
    p->Emit(R"cc(
      $field_$.~InlinedStringField();
    )cc");
    return;
  }

  if (ShouldSplit(field_, *opts_)) {
    p->Emit(R"cc(
      $cached_split_ptr$->$name$_.Destroy();
    )cc");
    return;
  }

  p->Emit(R"cc(
    $field_$.Destroy();
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
            const std::string& _s = this->_internal_$name$();
            $utf8_check$;
            target = stream->Write$DeclaredType$MaybeAliased($number$, _s, target);
          )cc");
}

void SingularString::GenerateConstexprAggregateInitializer(
    io::Printer* p) const {
  if (inlined_) {
    p->Emit(R"cc(
      /*decltype($field_$)*/ { nullptr, false }
    )cc");
    return;
  }

  p->Emit(R"cc(
    /*decltype($field_$)*/ {
      &::_pbi::fixed_address_empty_string, ::_pbi::ConstantInitialized {}
    }
  )cc");
}

void SingularString::GenerateAggregateInitializer(io::Printer* p) const {
  if (ShouldSplit(field_, options_)) {
    ABSL_CHECK(!inlined_);
    p->Emit(R"cc(
      decltype(Impl_::Split::$name$_) {}
    )cc");
    return;
  }

  if (!inlined_) {
    p->Emit(R"cc(
      decltype($field_$) {}
    )cc");
  } else {
    p->Emit(R"cc(
      decltype($field_$) { arena }
    )cc");
  }
}

class RepeatedString : public FieldGeneratorBase {
 public:
  RepeatedString(const FieldDescriptor* field, const Options& opts)
      : FieldGeneratorBase(field, opts), field_(field), opts_(&opts) {}
  ~RepeatedString() override = default;

  std::vector<Sub> MakeVars() const override { return Vars(field_, *opts_); }

  void GeneratePrivateMembers(io::Printer* p) const override {
    p->Emit(R"cc(
      $pb$::RepeatedPtrField<std::string> $name$_;
    )cc");
  }

  void GenerateClearingCode(io::Printer* p) const override {
    p->Emit(R"cc(
      _internal_mutable_$name$()->Clear();
    )cc");
  }

  void GenerateMergingCode(io::Printer* p) const override {
    p->Emit(R"cc(
      _this->_internal_mutable_$name$()->MergeFrom(from._internal_$name$());
    )cc");
  }

  void GenerateSwappingCode(io::Printer* p) const override {
    p->Emit(R"cc(
      _internal_mutable_$name$()->InternalSwap(
          other->_internal_mutable_$name$());
    )cc");
  }

  void GenerateDestructorCode(io::Printer* p) const override {
    p->Emit(R"cc(
      _internal_mutable_$name$()->~RepeatedPtrField();
    )cc");
  }

  void GenerateConstructorCode(io::Printer* p) const override {}

  void GenerateCopyConstructorCode(io::Printer* p) const override {
    ABSL_CHECK(!ShouldSplit(field_, options_));
  }

  void GenerateByteSize(io::Printer* p) const override {
    p->Emit(R"cc(
      total_size += $kTagBytes$ * $pbi$::FromIntSize(_internal_$name$().size());
      for (int i = 0, n = _internal_$name$().size(); i < n; ++i) {
        total_size += $pbi$::WireFormatLite::$DeclaredType$Size(
            _internal_$name$().Get(i));
      }
    )cc");
  }

  void GenerateAccessorDeclarations(io::Printer* p) const override;
  void GenerateInlineAccessorDefinitions(io::Printer* p) const override;
  void GenerateSerializeWithCachedSizesToArray(io::Printer* p) const override;

 private:
  const FieldDescriptor* field_;
  const Options* opts_;
};

void RepeatedString::GenerateAccessorDeclarations(io::Printer* p) const {
  bool unknown_ctype =
      field_->options().ctype() != internal::cpp::EffectiveStringCType(field_);

  if (unknown_ctype) {
    p->Emit(R"cc(
      private:  // Hidden due to unknown ctype option.
    )cc");
  }

  auto v1 = p->WithVars(
      AnnotatedAccessors(field_, {"", "_internal_", "_internal_add_"}));
  auto v2 = p->WithVars(
      AnnotatedAccessors(field_, {"set_", "add_"}, AnnotationCollector::kSet));
  auto v3 = p->WithVars(
      AnnotatedAccessors(field_, {"mutable_"}, AnnotationCollector::kAlias));

  p->Emit(R"cc(
    $DEPRECATED$ const std::string& $name$(int index) const;
    $DEPRECATED$ std::string* $mutable_name$(int index);
    $DEPRECATED$ void $set_name$(int index, const std::string& value);
    $DEPRECATED$ void $set_name$(int index, std::string&& value);
    $DEPRECATED$ void $set_name$(int index, const char* value);
    $DEPRECATED$ void $set_name$(int index, const $byte$* value, std::size_t size);
    $DEPRECATED$ void $set_name$(int index, absl::string_view value);
    $DEPRECATED$ std::string* $add_name$();
    $DEPRECATED$ void $add_name$(const std::string& value);
    $DEPRECATED$ void $add_name$(std::string&& value);
    $DEPRECATED$ void $add_name$(const char* value);
    $DEPRECATED$ void $add_name$(const $byte$* value, std::size_t size);
    $DEPRECATED$ void $add_name$(absl::string_view value);
    $DEPRECATED$ const $pb$::RepeatedPtrField<std::string>& $name$() const;
    $DEPRECATED$ $pb$::RepeatedPtrField<std::string>* $mutable_name$();

    private:
    const std::string& $_internal_name$(int index) const;
    std::string* $_internal_add_name$();
    const $pb$::RepeatedPtrField<std::string>& _internal_$name$() const;
    $pb$::RepeatedPtrField<std::string>* _internal_mutable_$name$();

    public:
  )cc");
}

void RepeatedString::GenerateInlineAccessorDefinitions(io::Printer* p) const {
  p->Emit({{"Get", opts_->safe_boundary_check ? "InternalCheckedGet" : "Get"},
           {"get_args",
            [&] {
              if (!opts_->safe_boundary_check) {
                p->Emit("index");
                return;
              }

              p->Emit(R"cc(index, $pbi$::GetEmptyStringAlreadyInited())cc");
            }}},
          R"cc(
            inline std::string* $Msg$::add_$name$() {
              std::string* _s = _internal_add_$name$();
              $annotate_add_mutable$;
              // @@protoc_insertion_point(field_add_mutable:$pkg.Msg.field$)
              return _s;
            }
            inline const std::string& $Msg$::$name$(int index) const {
              $annotate_get$;
              // @@protoc_insertion_point(field_get:$pkg.Msg.field$)
              return _internal_$name$(index);
            }
            inline std::string* $Msg$::mutable_$name$(int index) {
              $annotate_mutable$;
              // @@protoc_insertion_point(field_mutable:$pkg.Msg.field$)
              return _internal_mutable_$name$()->Mutable(index);
            }
            inline void $Msg$::set_$name$(int index, const std::string& value) {
              _internal_mutable_$name$()->Mutable(index)->assign(value);
              $annotate_set$;
              // @@protoc_insertion_point(field_set:$pkg.Msg.field$)
            }
            inline void $Msg$::set_$name$(int index, std::string&& value) {
              _internal_mutable_$name$()->Mutable(index)->assign(std::move(value));
              $annotate_set$;
              // @@protoc_insertion_point(field_set:$pkg.Msg.field$)
            }
            inline void $Msg$::set_$name$(int index, const char* value) {
              $DCHK$(value != nullptr);
              _internal_mutable_$name$()->Mutable(index)->assign(value);
              $annotate_set$;
              // @@protoc_insertion_point(field_set_char:$pkg.Msg.field$)
            }
            inline void $Msg$::set_$name$(int index, const $byte$* value,
                                          std::size_t size) {
              _internal_mutable_$name$()->Mutable(index)->assign(
                  reinterpret_cast<const char*>(value), size);
              $annotate_set$;
              // @@protoc_insertion_point(field_set_pointer:$pkg.Msg.field$)
            }
            inline void $Msg$::set_$name$(int index, absl::string_view value) {
              _internal_mutable_$name$()->Mutable(index)->assign(value.data(),
                                                                 value.size());
              $annotate_set$;
              // @@protoc_insertion_point(field_set_string_piece:$pkg.Msg.field$)
            }
            inline void $Msg$::add_$name$(const std::string& value) {
              _internal_mutable_$name$()->Add()->assign(value);
              $annotate_add$;
              // @@protoc_insertion_point(field_add:$pkg.Msg.field$)
            }
            inline void $Msg$::add_$name$(std::string&& value) {
              _internal_mutable_$name$()->Add(std::move(value));
              $annotate_add$;
              // @@protoc_insertion_point(field_add:$pkg.Msg.field$)
            }
            inline void $Msg$::add_$name$(const char* value) {
              $DCHK$(value != nullptr);
              _internal_mutable_$name$()->Add()->assign(value);
              $annotate_add$;
              // @@protoc_insertion_point(field_add_char:$pkg.Msg.field$)
            }
            inline void $Msg$::add_$name$(const $byte$* value, std::size_t size) {
              _internal_mutable_$name$()->Add()->assign(
                  reinterpret_cast<const char*>(value), size);
              $annotate_add$;
              // @@protoc_insertion_point(field_add_pointer:$pkg.Msg.field$)
            }
            inline void $Msg$::add_$name$(absl::string_view value) {
              _internal_mutable_$name$()->Add()->assign(value.data(), value.size());
              $annotate_add$;
              // @@protoc_insertion_point(field_add_string_piece:$pkg.Msg.field$)
            }
            inline const ::$proto_ns$::RepeatedPtrField<std::string>&
            $Msg$::$name$() const {
              $annotate_list$;
              // @@protoc_insertion_point(field_list:$pkg.Msg.field$)
              return _internal_$name$();
            }
            inline ::$proto_ns$::RepeatedPtrField<std::string>* $Msg$::mutable_$name$() {
              $annotate_mutable_list$;
              // @@protoc_insertion_point(field_mutable_list:$pkg.Msg.field$)
              return _internal_mutable_$name$();
            }
            inline const std::string& $Msg$::_internal_$name$(int index) const {
              return _internal_$name$().$Get$($get_args$);
            }
            inline std::string* $Msg$::_internal_add_$name$() {
              return _internal_mutable_$name$()->Add();
            }
            inline const ::$proto_ns$::RepeatedPtrField<std::string>&
            $Msg$::_internal_$name$() const {
              return $field_$;
            }
            inline ::$proto_ns$::RepeatedPtrField<std::string>*
            $Msg$::_internal_mutable_$name$() {
              return &$field_$;
            }
          )cc");
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
            for (int i = 0, n = this->_internal_$name$_size(); i < n; ++i) {
              const auto& s = this->_internal_$name$(i);
              $utf8_check$;
              target = stream->Write$DeclaredType$($number$, s, target);
            }
          )cc");
}
}  // namespace

std::unique_ptr<FieldGeneratorBase> MakeSinguarStringGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc) {
  return absl::make_unique<SingularString>(desc, options);
}

std::unique_ptr<FieldGeneratorBase> MakeRepeatedStringGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc) {
  return absl::make_unique<RepeatedString>(desc, options);
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
