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
#include "absl/strings/substitute.h"
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

std::vector<Sub> Vars(const FieldDescriptor* field, const Options& opts,
                      bool is_weak, bool use_base_class) {
  bool split = ShouldSplit(field, opts);
  bool is_foreign = IsCrossFileMessage(field);
  std::string field_name = FieldMemberName(field, split);
  std::string qualified_type = FieldMessageTypeName(field, opts);
  std::string default_ref =
      QualifiedDefaultInstanceName(field->message_type(), opts);
  std::string base = absl::StrCat(
      "::", ProtobufNamespace(opts), "::",
      HasDescriptorMethods(field->file(), opts) ? "Message" : "MessageLite");

  return {
      {"Submsg", qualified_type},
      {"MemberType", use_base_class ? base : qualified_type},
      {"kDefault", default_ref},
      Sub{"cast_to_field",
          use_base_class ? absl::Substitute("reinterpret_cast<$0*>", base) : ""}
          .ConditionalFunctionCall(),
      Sub{"arena_cast",
          !is_foreign ? "" : absl::Substitute("reinterpret_cast<$0*>", base)}
          .ConditionalFunctionCall(),
      {"cast_field_", use_base_class
                          ? absl::Substitute("reinterpret_cast<$0*>($1)",
                                             qualified_type, field_name)
                          : field_name},
      {"Weak", is_weak ? "Weak" : ""},
      {".weak", is_weak ? ".weak" : ""},
      {"_weak", is_weak ? "_weak" : ""},
      Sub("StrongRef", !is_weak ? ""
                                : absl::StrCat(StrongReferenceToType(
                                                   field->message_type(), opts),
                                               ";"))
          .WithSuffix(";"),
  };
}

class SingularMessage : public FieldGeneratorBase {
 public:
  SingularMessage(const FieldDescriptor* field, const Options& opts,
                  MessageSCCAnalyzer* scc)
      : FieldGeneratorBase(field, opts, scc),
        opts_(&opts),
        has_required_(scc->HasRequiredFields(field->message_type())),
        has_hasbit_(HasHasbit(field)) {}

  ~SingularMessage() override = default;

  std::vector<Sub> MakeVars() const override {
    return Vars(field_, *opts_, is_weak(), is_weak());
  }

  void GeneratePrivateMembers(io::Printer* p) const override {
    p->Emit(R"cc(
      $MemberType$* $nullable$ $name$_;
    )cc");
  }

  bool RequiresArena(GeneratorFunction function) const override;

  void GenerateNonInlineAccessorDefinitions(io::Printer* p) const override {}

  void GenerateAccessorDeclarations(io::Printer* p) const override;
  void GenerateInlineAccessorDefinitions(io::Printer* p) const override;
  void GenerateClearingCode(io::Printer* p) const override;
  void GenerateMessageClearingCode(io::Printer* p) const override;
  void GenerateMergingCode(io::Printer* p) const override;
  void GenerateSwappingCode(io::Printer* p) const override;
  void GenerateDestructorCode(io::Printer* p) const override;
  void GenerateConstructorCode(io::Printer* p) const override {}
  void GenerateCopyConstructorCode(io::Printer* p) const override;
  void GenerateSerializeWithCachedSizesToArray(io::Printer* p) const override;
  void GenerateByteSize(io::Printer* p) const override;
  void GenerateIsInitialized(io::Printer* p) const override;
  bool NeedsIsInitialized() const override;
  void GenerateConstexprAggregateInitializer(io::Printer* p) const override;
  void GenerateAggregateInitializer(io::Printer* p) const override;
  void GenerateCopyAggregateInitializer(io::Printer* p) const override;

  void GenerateMemberConstexprConstructor(io::Printer* p) const override {
    p->Emit("$name$_{nullptr}");
  }

  void GenerateMemberConstructor(io::Printer* p) const override {
    p->Emit("$name$_{nullptr}");
  }

  void GenerateMemberCopyConstructor(io::Printer* p) const override {
    p->Emit("$name$_{$superclass$::CopyConstruct(arena, *from.$name$_)}");
  }

  void GenerateOneofCopyConstruct(io::Printer* p) const override {
    p->Emit(R"cc(
      $field$ = $superclass$::CopyConstruct(arena, *from.$field$);
    )cc");
  }

 private:
  friend class OneofMessage;

  const Options* opts_;
  bool has_required_;
  bool has_hasbit_;
};

void SingularMessage::GenerateAccessorDeclarations(io::Printer* p) const {
  auto vars = AnnotatedAccessors(
      field_, {"", "set_allocated_", "unsafe_arena_set_allocated_",
               "unsafe_arena_release_"});
  vars.push_back(Sub{
      "release_name",
      SafeFunctionName(field_->containing_type(), field_, "release_"),
  }
                     .AnnotatedAs(field_));
  auto v1 = p->WithVars(vars);
  auto v2 = p->WithVars(
      AnnotatedAccessors(field_, {"mutable_"}, AnnotationCollector::kAlias));

  p->Emit(R"cc(
    $DEPRECATED$ const $Submsg$& $name$() const;
    $DEPRECATED$ [[nodiscard]] $Submsg$* $nullable$ $release_name$();
    $DEPRECATED$ $Submsg$* $nonnull$ $mutable_name$();
    $DEPRECATED$ void $set_allocated_name$($Submsg$* $nullable$ value);
    $DEPRECATED$ void $unsafe_arena_set_allocated_name$($Submsg$* $nullable$ value);
    $DEPRECATED$ $Submsg$* $nullable$ $unsafe_arena_release_name$();

    private:
    const $Submsg$& _internal_$name$() const;
    $Submsg$* $nonnull$ _internal_mutable_$name$();

    public:
  )cc");
}

void SingularMessage::GenerateInlineAccessorDefinitions(io::Printer* p) const {
  auto v =
      p->WithVars({{"release_name", SafeFunctionName(field_->containing_type(),
                                                     field_, "release_")}});
  std::vector<Sub> subs = {
      {"update_hasbit",
       [&] {
         if (!has_hasbit_) return;
         p->Emit(R"cc(
           if (value != nullptr) {
             $set_hasbit$
           } else {
             $clear_hasbit$
           }
         )cc");
       }},
  };
  absl::string_view code = R"cc(
    inline const $Submsg$& $Msg$::_internal_$name_internal$() const {
      $TsanDetectConcurrentRead$;
      $StrongRef$;
      const $Submsg$* p = $cast_field_$;
      return p != nullptr ? *p : reinterpret_cast<const $Submsg$&>($kDefault$);
    }
    inline const $Submsg$& $Msg$::$name$() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
      $WeakDescriptorSelfPin$;
      $annotate_get$;
      // @@protoc_insertion_point(field_get:$pkg.Msg.field$)
      return _internal_$name_internal$();
    }
    inline void $Msg$::unsafe_arena_set_allocated_$name$(
        $Submsg$* $nullable$ value) {
      $WeakDescriptorSelfPin$;
      $TsanDetectConcurrentMutation$;
      $PrepareSplitMessageForWrite$;
      //~ If we're not on an arena, free whatever we were holding before.
      //~ (If we are on arena, we can just forget the earlier pointer.)
      if (GetArena() == nullptr) {
        delete reinterpret_cast<$pb$::MessageLite*>($field_$);
      }
      $field_$ = reinterpret_cast<$MemberType$*>(value);
      $update_hasbit$;
      $annotate_set$;
      // @@protoc_insertion_point(field_unsafe_arena_set_allocated:$pkg.Msg.field$)
    }
    inline $Submsg$* $nullable$ $Msg$::$release_name$() {
      $WeakDescriptorSelfPin$;
      $TsanDetectConcurrentMutation$;
      $StrongRef$;
      $annotate_release$;
      $PrepareSplitMessageForWrite$;

      $clear_hasbit$;
      $Submsg$* released = $cast_field_$;
      $field_$ = nullptr;
      if ($pbi$::DebugHardenForceCopyInRelease()) {
        auto* old = reinterpret_cast<$pb$::MessageLite*>(released);
        released = $pbi$::DuplicateIfNonNull(released);
        if (GetArena() == nullptr) {
          delete old;
        }
      } else {
        if (GetArena() != nullptr) {
          released = $pbi$::DuplicateIfNonNull(released);
        }
      }
      return released;
    }
    inline $Submsg$* $nullable$ $Msg$::unsafe_arena_release_$name$() {
      $WeakDescriptorSelfPin$;
      $TsanDetectConcurrentMutation$;
      $annotate_release$;
      // @@protoc_insertion_point(field_release:$pkg.Msg.field$)
      $StrongRef$;
      $PrepareSplitMessageForWrite$;

      $clear_hasbit$;
      $Submsg$* temp = $cast_field_$;
      $field_$ = nullptr;
      return temp;
    }
    inline $Submsg$* $nonnull$ $Msg$::_internal_mutable_$name_internal$() {
      $TsanDetectConcurrentMutation$;
      $StrongRef$;
      if ($field_$ == nullptr) {
        auto* p = $superclass$::DefaultConstruct<$Submsg$>(GetArena());
        $field_$ = reinterpret_cast<$MemberType$*>(p);
      }
      return $cast_field_$;
    }
    inline $Submsg$* $nonnull$ $Msg$::mutable_$name$()
        ABSL_ATTRIBUTE_LIFETIME_BOUND {
      //~ TODO: add tests to make sure all write accessors are
      //~ able to prepare split message allocation.
      $WeakDescriptorSelfPin$;
      $PrepareSplitMessageForWrite$;
      $set_hasbit$;
      $Submsg$* _msg = _internal_mutable_$name_internal$();
      $annotate_mutable$;
      // @@protoc_insertion_point(field_mutable:$pkg.Msg.field$)
      return _msg;
    }
    //~ We handle the most common case inline, and delegate less common
    //~ cases to the slow fallback function.
    inline void $Msg$::set_allocated_$name$($Submsg$* $nullable$ value) {
      $WeakDescriptorSelfPin$;
      $pb$::Arena* message_arena = GetArena();
      $TsanDetectConcurrentMutation$;
      $PrepareSplitMessageForWrite$;
      if (message_arena == nullptr) {
        delete reinterpret_cast<$pb$::MessageLite*>($field_$);
      }

      if (value != nullptr) {
        $pb$::Arena* submessage_arena = $arena_cast$(value)->GetArena();
        if (message_arena != submessage_arena) {
          value = $pbi$::GetOwnedMessage(message_arena, value, submessage_arena);
        }
        $set_hasbit$;
      } else {
        $clear_hasbit$;
      }

      $field_$ = reinterpret_cast<$MemberType$*>(value);
      $annotate_set$;
      // @@protoc_insertion_point(field_set_allocated:$pkg.Msg.field$)
    }
  )cc";
  p->Emit(subs, code);
}

void SingularMessage::GenerateClearingCode(io::Printer* p) const {
  ABSL_CHECK(has_hasbit_);
  p->Emit(
      R"cc(
        if ($field_$ != nullptr) $field_$->Clear();
      )cc");
}

void SingularMessage::GenerateMessageClearingCode(io::Printer* p) const {
  ABSL_CHECK(has_hasbit_);
  p->Emit(
      R"cc(
        $DCHK$($field_$ != nullptr);
        $field_$->Clear();
      )cc");
}

bool SingularMessage::RequiresArena(GeneratorFunction function) const {
  switch (function) {
    case GeneratorFunction::kMergeFrom:
      return !should_split();
  }
  return false;
}

void SingularMessage::GenerateMergingCode(io::Printer* p) const {
  if (is_weak()) {
    p->Emit(
        R"cc(
          if (_this->$field_$ == nullptr) {
            _this->$field_$ = from.$field_$->New(arena);
          }
          _this->$field_$->CheckTypeAndMergeFrom(*from.$field_$);
        )cc");
  } else if (should_split()) {
    p->Emit(
        R"cc(
          _this->_internal_mutable_$name$()->$Submsg$::MergeFrom(
              from._internal_$name$());
        )cc");
  } else {
    // Important: we set `hasbits` after we copied the field. There are cases
    // where people assign root values to child values or vice versa which
    // are not always checked, so we delay this change becoming 'visible'
    // until after we copied the message.
    // TODO enforces this as undefined behavior in debug builds.
    p->Emit(R"cc(
      $DCHK$(from.$field_$ != nullptr);
      if (_this->$field_$ == nullptr) {
        _this->$field_$ = $superclass$::CopyConstruct(arena, *from.$field_$);
      } else {
        _this->$field_$->MergeFrom(*from.$field_$);
      }
    )cc");
  }
}

void SingularMessage::GenerateSwappingCode(io::Printer* p) const {
  p->Emit("swap($field_$, other->$field_$);\n");
}

void SingularMessage::GenerateDestructorCode(io::Printer* p) const {
  if (should_split()) {
    p->Emit(R"cc(
      delete $cached_split_ptr$->$name$_;
    )cc");
  } else {
    p->Emit(R"cc(
      delete this_.$field_$;
    )cc");
  }
}

void SingularMessage::GenerateCopyConstructorCode(io::Printer* p) const {
  ABSL_CHECK(has_hasbit_);
  p->Emit(R"cc(
    if ((from.$has_hasbit$) != 0) {
      _this->$field_$ = $superclass$::CopyConstruct(arena, *from.$field_$);
    }
  )cc");
}

void SingularMessage::GenerateSerializeWithCachedSizesToArray(
    io::Printer* p) const {
  if (!is_group()) {
    p->Emit(R"cc(
      target = $pbi$::WireFormatLite::InternalWrite$declared_type$(
          $number$, *this_.$field_$, this_.$field_$->GetCachedSize(), target,
          stream);
    )cc");
  } else {
    p->Emit(R"cc(
      target = stream->EnsureSpace(target);
      target = $pbi$::WireFormatLite::InternalWrite$declared_type$(
          $number$, *this_.$field_$, target, stream);
    )cc");
  }
}

void SingularMessage::GenerateByteSize(io::Printer* p) const {
  p->Emit(R"cc(
    total_size += $tag_size$ +
                  $pbi$::WireFormatLite::$declared_type$Size(*this_.$field_$);
  )cc");
}


void SingularMessage::GenerateIsInitialized(io::Printer* p) const {
  if (!NeedsIsInitialized()) return;

  if (HasHasbit(field_)) {
    p->Emit(R"cc(
      if ((this_.$has_hasbit$) != 0) {
        if (!this_.$field_$->IsInitialized()) return false;
      }
    )cc");
  } else {
    p->Emit(R"cc(
      if (this_._internal_has_$name$()) {
        if (!this_.$field_$->IsInitialized()) return false;
      }
    )cc");
  }
}

bool SingularMessage::NeedsIsInitialized() const { return has_required_; }

void SingularMessage::GenerateConstexprAggregateInitializer(
    io::Printer* p) const {
  p->Emit(R"cc(
    /*decltype($field_$)*/ nullptr,
  )cc");
}

void SingularMessage::GenerateCopyAggregateInitializer(io::Printer* p) const {
  p->Emit(R"cc(
    decltype($field_$){nullptr},
  )cc");
}

void SingularMessage::GenerateAggregateInitializer(io::Printer* p) const {
  if (should_split()) {
    p->Emit(R"cc(
      decltype(Impl_::Split::$name$_){nullptr},
    )cc");
  } else {
    p->Emit(R"cc(
      decltype($field_$){nullptr},
    )cc");
  }
}

class OneofMessage : public SingularMessage {
 public:
  OneofMessage(const FieldDescriptor* descriptor, const Options& options,
               MessageSCCAnalyzer* scc_analyzer)
      : SingularMessage(descriptor, options, scc_analyzer) {
    auto* oneof = descriptor->containing_oneof();
    num_message_fields_in_oneof_ = 0;
    for (int i = 0; i < oneof->field_count(); ++i) {
      num_message_fields_in_oneof_ +=
          oneof->field(i)->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE;
    }
  }

  ~OneofMessage() override = default;

  bool use_base_class() const {
    if (is_weak()) return true;

    // For non-weak oneof fields, we choose to use a base class pointer when the
    // oneof has many message fields in it.  Using a base class here is not
    // about correctness, but about performance and binary size.
    //
    // This allows the compiler to merge all the different switch cases (since
    // the code is identical for all message alternatives) reducing binary size.
    // The runtime dispatch is effectively changed from a switch statement to a
    // virtual function call. For many oneofs, it completely elides the switch
    // dispatch.
    //
    // This constant is a tradeoff. We want to allow optimizations (like
    // inlining) on small oneofs. For small oneofs the compiler can use faster
    // alternatives to table-based jumps. Also, the technique used here has less
    // of a binary size win for small oneofs.
    static constexpr int kMaxStaticTypeCount = 3;
    return num_message_fields_in_oneof_ >= kMaxStaticTypeCount &&
           // Hot alternatives are kept as their static type for performance..
           !IsLikelyPresent(field_, *opts_);
  }

  std::vector<Sub> MakeVars() const override {
    return Vars(field_, *opts_, is_weak(), use_base_class());
  }

  void GenerateInlineAccessorDefinitions(io::Printer* p) const override;
  void GenerateNonInlineAccessorDefinitions(io::Printer* p) const override;
  void GenerateClearingCode(io::Printer* p) const override;
  void GenerateMessageClearingCode(io::Printer* p) const override;
  void GenerateSwappingCode(io::Printer* p) const override;
  void GenerateDestructorCode(io::Printer* p) const override;
  void GenerateConstructorCode(io::Printer* p) const override;
  void GenerateCopyConstructorCode(io::Printer* p) const override;
  void GenerateIsInitialized(io::Printer* p) const override;
  bool NeedsIsInitialized() const override;
  void GenerateMergingCode(io::Printer* p) const override;
  bool RequiresArena(GeneratorFunction func) const override;

 private:
  int num_message_fields_in_oneof_;
};

void OneofMessage::GenerateNonInlineAccessorDefinitions(io::Printer* p) const {
  p->Emit(R"cc(
    void $Msg$::set_allocated_$name$($Submsg$* $nullable$ $name$) {
      $pb$::Arena* message_arena = GetArena();
      clear_$oneof_name$();
      if ($name$) {
        $pb$::Arena* submessage_arena = $arena_cast$($name$)->GetArena();
        if (message_arena != submessage_arena) {
          $name$ = $pbi$::GetOwnedMessage(message_arena, $name$, submessage_arena);
        }
        set_has_$name$();
        $field_$ = $name$;
      }
      $annotate_set$;
      // @@protoc_insertion_point(field_set_allocated:$pkg.Msg.field$)
    }
  )cc");
}

void OneofMessage::GenerateInlineAccessorDefinitions(io::Printer* p) const {
  auto v =
      p->WithVars({{"release_name", SafeFunctionName(field_->containing_type(),
                                                     field_, "release_")}});

  p->Emit(R"cc(
    inline $Submsg$* $nullable$ $Msg$::$release_name$() {
      $WeakDescriptorSelfPin$;
      $annotate_release$;
      // @@protoc_insertion_point(field_release:$pkg.Msg.field$)
      $StrongRef$;
      if ($has_field$) {
        clear_has_$oneof_name$();
        auto* temp = $cast_field_$;
        if (GetArena() != nullptr) {
          temp = $pbi$::DuplicateIfNonNull(temp);
        }
        $field_$ = nullptr;
        return temp;
      } else {
        return nullptr;
      }
    }
  )cc");
  p->Emit(R"cc(
    inline const $Submsg$& $Msg$::_internal_$name_internal$() const {
      $StrongRef$;
      return $has_field$ ? *$cast_field_$ : reinterpret_cast<$Submsg$&>($kDefault$);
    }
  )cc");
  p->Emit(R"cc(
    inline const $Submsg$& $Msg$::$name$() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
      $WeakDescriptorSelfPin$;
      $annotate_get$;
      // @@protoc_insertion_point(field_get:$pkg.Msg.field$)
      return _internal_$name_internal$();
    }
  )cc");
  p->Emit(R"cc(
    inline $Submsg$* $nullable$ $Msg$::unsafe_arena_release_$name$() {
      $WeakDescriptorSelfPin$;
      $annotate_release$;
      // @@protoc_insertion_point(field_unsafe_arena_release:$pkg.Msg.field$)
      $StrongRef$;
      if ($has_field$) {
        clear_has_$oneof_name$();
        auto* temp = $cast_field_$;
        $field_$ = nullptr;
        return temp;
      } else {
        return nullptr;
      }
    }
  )cc");
  p->Emit(R"cc(
    inline void $Msg$::unsafe_arena_set_allocated_$name$(
        $Submsg$* $nullable$ value) {
      $WeakDescriptorSelfPin$;
      // We rely on the oneof clear method to free the earlier contents
      // of this oneof. We can directly use the pointer we're given to
      // set the new value.
      clear_$oneof_name$();
      if (value) {
        set_has_$name_internal$();
        $field_$ = $cast_to_field$(value);
      }
      $annotate_set$;
      // @@protoc_insertion_point(field_unsafe_arena_set_allocated:$pkg.Msg.field$)
    }
  )cc");
  p->Emit(R"cc(
    inline $Submsg$* $nonnull$ $Msg$::_internal_mutable_$name_internal$() {
      $StrongRef$;
      if ($not_has_field$) {
        clear_$oneof_name$();
        set_has_$name_internal$();
        $field_$ = $cast_to_field$(
            $superclass$::DefaultConstruct<$Submsg$>(GetArena()));
      }
      return $cast_field_$;
    }
  )cc");
  p->Emit(R"cc(
    inline $Submsg$* $nonnull$ $Msg$::mutable_$name$()
        ABSL_ATTRIBUTE_LIFETIME_BOUND {
      $WeakDescriptorSelfPin$;
      $Submsg$* _msg = _internal_mutable_$name_internal$();
      $annotate_mutable$;
      // @@protoc_insertion_point(field_mutable:$pkg.Msg.field$)
      return _msg;
    }
  )cc");
}

void OneofMessage::GenerateClearingCode(io::Printer* p) const {
  p->Emit({{"poison_or_clear",
            [&] {
              if (HasDescriptorMethods(field_->file(), options_)) {
                p->Emit(R"cc(
                  $pbi$::MaybePoisonAfterClear($field_$);
                )cc");
              } else {
                p->Emit(R"cc(
                  if ($field_$ != nullptr) {
                    $field_$->Clear();
                  }
                )cc");
              }
            }}},
          R"cc(
            if (GetArena() == nullptr) {
              delete $field_$;
            } else if ($pbi$::DebugHardenClearOneofMessageOnArena()) {
              $poison_or_clear$;
            }
          )cc");
}

void OneofMessage::GenerateMessageClearingCode(io::Printer* p) const {
  GenerateClearingCode(p);
}

void OneofMessage::GenerateSwappingCode(io::Printer* p) const {
  // Don't print any swapping code. Swapping the union will swap this field.
}

void OneofMessage::GenerateDestructorCode(io::Printer* p) const {
  // We inherit from SingularMessage, so we need to override the default
  // behavior.
}

void OneofMessage::GenerateConstructorCode(io::Printer* p) const {
  // Don't print any constructor code. The field is in a union. We allocate
  // space only when this field is used.
}

void OneofMessage::GenerateCopyConstructorCode(io::Printer* p) const {
  ABSL_CHECK(!has_hasbit_);
  p->Emit(R"cc(
    if (from._internal_has_$name$()) {
      _this->$field_$ = $superclass$::CopyConstruct(arena, *from.$field_$);
    }
  )cc");
}

void OneofMessage::GenerateIsInitialized(io::Printer* p) const {
  if (!NeedsIsInitialized()) return;

  p->Emit(R"cc(
    if (this_.$has_field$ && !this_.$field_$->IsInitialized())
      return false;
  )cc");
}

bool OneofMessage::NeedsIsInitialized() const { return has_required_; }

void OneofMessage::GenerateMergingCode(io::Printer* p) const {
  p->Emit({{"merge",
            use_base_class() && !HasDescriptorMethods(field_->file(), options_)
                ? "CheckTypeAndMergeFrom"
                : "MergeFrom"}},
          R"cc(
            if (oneof_needs_init) {
              _this->$field_$ = $superclass$::CopyConstruct(arena, *from.$field_$);
            } else {
              _this->$field_$->$merge$(*from.$field_$);
            }
          )cc");
}

bool OneofMessage::RequiresArena(GeneratorFunction func) const {
  switch (func) {
    case GeneratorFunction::kMergeFrom:
      return true;
  }
  return false;
}

class RepeatedMessage : public FieldGeneratorBase {
 public:
  RepeatedMessage(const FieldDescriptor* field, const Options& opts,
                  MessageSCCAnalyzer* scc)
      : FieldGeneratorBase(field, opts, scc),
        opts_(&opts),
        has_required_(scc->HasRequiredFields(field->message_type())) {}

  ~RepeatedMessage() override = default;

  std::vector<Sub> MakeVars() const override {
    return Vars(field_, *opts_, is_weak(), is_weak());
  }

  void GeneratePrivateMembers(io::Printer* p) const override;
  void GenerateAccessorDeclarations(io::Printer* p) const override;
  void GenerateInlineAccessorDefinitions(io::Printer* p) const override;
  void GenerateClearingCode(io::Printer* p) const override;
  void GenerateMergingCode(io::Printer* p) const override;
  void GenerateSwappingCode(io::Printer* p) const override;
  void GenerateConstructorCode(io::Printer* p) const override;
  void GenerateCopyConstructorCode(io::Printer* p) const override;
  void GenerateDestructorCode(io::Printer* p) const override;
  void GenerateSerializeWithCachedSizesToArray(io::Printer* p) const override;
  void GenerateByteSize(io::Printer* p) const override;
  void GenerateIsInitialized(io::Printer* p) const override;
  bool NeedsIsInitialized() const override;

 private:
  const Options* opts_;
  bool has_required_;
};

void RepeatedMessage::GeneratePrivateMembers(io::Printer* p) const {
  if (should_split()) {
    p->Emit(R"cc(
      $pbi$::RawPtr<$pb$::$Weak$RepeatedPtrField<$Submsg$>> $name$_;
    )cc");
  } else {
    p->Emit("$pb$::$Weak$RepeatedPtrField< $Submsg$ > $name$_;\n");
  }
}

void RepeatedMessage::GenerateAccessorDeclarations(io::Printer* p) const {
  auto v = p->WithVars(
      AnnotatedAccessors(field_, {"", "_internal_", "_internal_mutable_"}));
  auto vs = p->WithVars(
      AnnotatedAccessors(field_, {"add_"}, io::AnnotationCollector::kSet));
  auto vm = p->WithVars(AnnotatedAccessors(field_, {"mutable_"},
                                           io::AnnotationCollector::kAlias));

  p->Emit(R"cc(
    $DEPRECATED$ $Submsg$* $nonnull$ $mutable_name$(int index);
    $DEPRECATED$ $pb$::RepeatedPtrField<$Submsg$>* $nonnull$ $mutable_name$();

    private:
    const $pb$::RepeatedPtrField<$Submsg$>& $_internal_name$() const;
    $pb$::RepeatedPtrField<$Submsg$>* $nonnull$ $_internal_mutable_name$();
  )cc");
  if (is_weak()) {
    p->Emit(R"cc(
      const $pb$::WeakRepeatedPtrField<$Submsg$>& _internal_weak_$name$() const;
      $pb$::WeakRepeatedPtrField<$Submsg$>* $nonnull$ _internal_mutable_weak_$name$();
    )cc");
  }
  p->Emit(R"cc(
    public:
    $DEPRECATED$ const $Submsg$& $name$(int index) const;
    $DEPRECATED$ $Submsg$* $nonnull$ $add_name$();
    $DEPRECATED$ const $pb$::RepeatedPtrField<$Submsg$>& $name$() const;
  )cc");
}

void RepeatedMessage::GenerateInlineAccessorDefinitions(io::Printer* p) const {
  // TODO: move insertion points

  // No need to set hasbit in mutable_$name$(int index). Hasbits only need to be
  // updated if a new element is (potentially) added, not if an existing element
  // is mutated.
  p->Emit({GetEmitRepeatedFieldMutableSub(*opts_, p)},
          R"cc(
            inline $Submsg$* $nonnull$ $Msg$::mutable_$name$(int index)
                ABSL_ATTRIBUTE_LIFETIME_BOUND {
              $WeakDescriptorSelfPin$;
              $annotate_mutable$;
              // @@protoc_insertion_point(field_mutable:$pkg.Msg.field$)
              $StrongRef$;
              return $mutable$;
            }
          )cc");

  p->Emit(R"cc(
    inline $pb$::RepeatedPtrField<$Submsg$>* $nonnull$ $Msg$::mutable_$name$()
        ABSL_ATTRIBUTE_LIFETIME_BOUND {
      $WeakDescriptorSelfPin$;
      $set_hasbit$;
      $annotate_mutable_list$;
      // @@protoc_insertion_point(field_mutable_list:$pkg.Msg.field$)
      $StrongRef$;
      $TsanDetectConcurrentMutation$;
      return _internal_mutable_$name_internal$();
    }
  )cc");
  p->Emit({GetEmitRepeatedFieldGetterSub(*opts_, p)},
          R"cc(
            inline const $Submsg$& $Msg$::$name$(int index) const
                ABSL_ATTRIBUTE_LIFETIME_BOUND {
              $WeakDescriptorSelfPin$;
              $annotate_get$;
              // @@protoc_insertion_point(field_get:$pkg.Msg.field$)
              $StrongRef$;
              return $getter$;
            }
          )cc");
  p->Emit(R"cc(
    inline $Submsg$* $nonnull$ $Msg$::add_$name$()
        ABSL_ATTRIBUTE_LIFETIME_BOUND {
      $WeakDescriptorSelfPin$;
      $TsanDetectConcurrentMutation$;
      $Submsg$* _add = _internal_mutable_$name_internal$()->Add();
      $set_hasbit$;
      $annotate_add_mutable$;
      // @@protoc_insertion_point(field_add:$pkg.Msg.field$)
      return _add;
    }
  )cc");
  p->Emit(R"cc(
    inline const $pb$::RepeatedPtrField<$Submsg$>& $Msg$::$name$() const
        ABSL_ATTRIBUTE_LIFETIME_BOUND {
      $WeakDescriptorSelfPin$;
      $annotate_list$;
      // @@protoc_insertion_point(field_list:$pkg.Msg.field$)
      $StrongRef$;
      return _internal_$name_internal$();
    }
  )cc");

  if (should_split()) {
    p->Emit(R"cc(
      inline const $pb$::$Weak$RepeatedPtrField<$Submsg$>&
      $Msg$::_internal$_weak$_$name_internal$() const {
        $TsanDetectConcurrentRead$;
        return *$field_$;
      }
      inline $pb$::$Weak$RepeatedPtrField<$Submsg$>* $nonnull$
      $Msg$::_internal_mutable$_weak$_$name_internal$() {
        $TsanDetectConcurrentRead$;
        $PrepareSplitMessageForWrite$;
        if ($field_$.IsDefault()) {
          $field_$.Set($superclass$::DefaultConstruct<
                       $pb$::$Weak$RepeatedPtrField<$Submsg$>>(GetArena()));
        }
        return $field_$.Get();
      }
    )cc");
  } else {
    p->Emit(R"cc(
      inline const $pb$::$Weak$RepeatedPtrField<$Submsg$>&
      $Msg$::_internal$_weak$_$name_internal$() const {
        $TsanDetectConcurrentRead$;
        return $field_$;
      }
      inline $pb$::$Weak$RepeatedPtrField<$Submsg$>* $nonnull$
      $Msg$::_internal_mutable$_weak$_$name_internal$() {
        $TsanDetectConcurrentRead$;
        return &$field_$;
      }
    )cc");
  }
  if (is_weak()) {
    p->Emit(R"cc(
      inline const $pb$::RepeatedPtrField<$Submsg$>&
      $Msg$::_internal_$name_internal$() const {
        return _internal_weak_$name_internal$().weak;
      }
      inline $pb$::RepeatedPtrField<$Submsg$>* $nonnull$
      $Msg$::_internal_mutable_$name_internal$() {
        return &_internal_mutable_weak_$name_internal$()->weak;
      }
    )cc");
  }
}

void RepeatedMessage::GenerateClearingCode(io::Printer* p) const {
  if (should_split()) {
    p->Emit("$field_$.ClearIfNotDefault();\n");
  } else {
    p->Emit("$field_$.Clear();\n");
  }
}

void RepeatedMessage::GenerateMergingCode(io::Printer* p) const {
  // TODO: experiment with simplifying this to be
  // `if (!from.empty()) { body(); }` for both split and non-split cases.
  auto body = [&] {
    p->Emit(R"cc(
      _this->_internal_mutable$_weak$_$name$()->MergeFrom(
          from._internal$_weak$_$name$());
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

void RepeatedMessage::GenerateSwappingCode(io::Printer* p) const {
  ABSL_CHECK(!should_split());
  p->Emit(R"cc(
    $field_$.InternalSwap(&other->$field_$);
  )cc");
}

void RepeatedMessage::GenerateConstructorCode(io::Printer* p) const {
  // Not needed for repeated fields.
}

void RepeatedMessage::GenerateCopyConstructorCode(io::Printer* p) const {
  // TODO: For split repeated fields we might want to use type
  // erasure to reduce binary size costs.
  if (should_split()) {
    p->Emit(R"cc(
      if (!from._internal$_weak$_$name$().empty()) {
        _internal_mutable$_weak$_$name$()->MergeFrom(from._internal$_weak$_$name$());
      }
    )cc");
  }
}

void RepeatedMessage::GenerateDestructorCode(io::Printer* p) const {
  if (should_split()) {
    p->Emit(R"cc(
      this_.$field_$.DeleteIfNotDefault();
    )cc");
  }
}

void RepeatedMessage::GenerateSerializeWithCachedSizesToArray(
    io::Printer* p) const {
  if (is_weak()) {
    p->Emit({{"serialize_field",
              [&] {
                if (field_->type() == FieldDescriptor::TYPE_MESSAGE) {
                  p->Emit(
                      R"cc(
                        target =
                            $pbi$::WireFormatLite::InternalWrite$declared_type$(
                                $number$, **it, (**it).GetCachedSize(), target,
                                stream);
                      )cc");
                } else {
                  p->Emit(
                      R"cc(
                        target = stream->EnsureSpace(target);
                        target =
                            $pbi$::WireFormatLite::InternalWrite$declared_type$(
                                $number$, **it, target, stream);
                      )cc");
                }
              }}},
            R"cc(
              for (auto it = this_.$field_$.pointer_begin(),
                        end = this_.$field_$.pointer_end();
                   it < end; ++it) {
                $serialize_field$;
              }
            )cc");
  } else {
    p->Emit({{"serialize_field",
              [&] {
                if (field_->type() == FieldDescriptor::TYPE_MESSAGE) {
                  p->Emit(
                      R"cc(
                        const auto& repfield = this_._internal_$name$().Get(i);
                        target =
                            $pbi$::WireFormatLite::InternalWrite$declared_type$(
                                $number$, repfield, repfield.GetCachedSize(),
                                target, stream);
                      )cc");
                } else {
                  p->Emit(
                      R"cc(
                        target = stream->EnsureSpace(target);
                        target =
                            $pbi$::WireFormatLite::InternalWrite$declared_type$(
                                $number$, this_._internal_$name$().Get(i),
                                target, stream);
                      )cc");
                }
              }}},
            R"cc(
              for (unsigned i = 0, n = static_cast<unsigned>(
                                       this_._internal_$name$_size());
                   i < n; i++) {
                $serialize_field$;
              }
            )cc");
  }
}

void RepeatedMessage::GenerateByteSize(io::Printer* p) const {
  p->Emit(
      R"cc(
        total_size += $tag_size$UL * this_._internal_$name$_size();
        for (const auto& msg : this_._internal$_weak$_$name$()) {
          total_size += $pbi$::WireFormatLite::$declared_type$Size(msg);
        }
      )cc");
}


void RepeatedMessage::GenerateIsInitialized(io::Printer* p) const {
  if (!NeedsIsInitialized()) return;

  if (is_weak()) {
    p->Emit(
        R"cc(
          if (!$pbi$::AllAreInitializedWeak(this_.$field_$.weak))
            return false;
        )cc");
  } else {
    p->Emit(
        R"cc(
          if (!$pbi$::AllAreInitialized(this_._internal_$name$()))
            return false;
        )cc");
  }
}

bool RepeatedMessage::NeedsIsInitialized() const { return has_required_; }
}  // namespace

std::unique_ptr<FieldGeneratorBase> MakeSinguarMessageGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc) {
  return absl::make_unique<SingularMessage>(desc, options, scc);
}

std::unique_ptr<FieldGeneratorBase> MakeRepeatedMessageGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc) {
  return absl::make_unique<RepeatedMessage>(desc, options, scc);
}

std::unique_ptr<FieldGeneratorBase> MakeOneofMessageGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc) {
  return absl::make_unique<OneofMessage>(desc, options, scc);
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
