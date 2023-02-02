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
#include "absl/strings/str_cat.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/cpp/field.h"
#include "google/protobuf/compiler/cpp/field_generators/generators.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
namespace {
using ::google::protobuf::internal::cpp::HasHasbit;
using Sub = google::protobuf::io::Printer::Sub;

std::string ReinterpretCast(absl::string_view type, absl::string_view expr,
                            bool weak) {
  if (!weak) {
    return std::string(expr);
  }

  return absl::Substitute("reinterpret_cast<$0>($1)", type, expr);
}

std::vector<Sub> Vars(const FieldDescriptor* field, const Options& opts,
                      bool weak) {
  bool split = ShouldSplit(field, opts);
  std::string field_name = FieldMemberName(field, split);
  std::string type = FieldMessageTypeName(field, opts);
  std::string default_ref =
      QualifiedDefaultInstanceName(field->message_type(), opts);

  auto make_cast = [weak](absl::string_view type, absl::string_view expr,
                          bool is_const) {
    if (!weak) {
      return std::string(expr);
    }

    return absl::Substitute("reinterpret_cast<$0$1>($2)",
                            is_const ? "const " : "", type, expr);
  };

  return {
      {"Submsg", type},
      {"cast_field_", make_cast(type, field_name, false)},
      {"const_field_", make_cast(type, field_name, true)},
      {"callable_field_",
       !IsCrossFileMessage(field)
           ? field_name
           : absl::Substitute("reinterpret_cast<::PROTOBUF_NAMESPACE_ID::"
                              "MessageLite*>($0)",
                              field_name)},
      {"kDefault", default_ref},
      {"kDefaultPtr",
       make_cast("::PROTOBUF_NAMESPACE_ID::MessageLite",
                 QualifiedDefaultInstancePtr(field->message_type(), opts),
                 true)},
      {"Weak", weak ? "Weak" : ""},
      {".weak", weak ? ".weak" : ""},

      {"cast_value", !weak ? "value"
                           : "reinterpret_cast<::PROTOBUF_NAMESPACE_ID::"
                             "MessageLite*>(value)"},
      {"callable_value", !IsCrossFileMessage(field)
                             ? "value"
                             : "reinterpret_cast<::PROTOBUF_NAMESPACE_ID::"
                               "MessageLite*>(value)"},
      Sub("StrongRef",
          !weak ? ""
                : absl::Substitute(
                      "::PROTOBUF_NAMESPACE_ID::internal::StronReference("
                      "reinterpret_cast<const $0&>($1)",
                      type, default_ref))
          .WithSuffix(";"),
  };
}

class SingularMessage : public FieldGeneratorBase {
 public:
  SingularMessage(const FieldDescriptor* field, const Options& opts,
                  MessageSCCAnalyzer* scc)
      : FieldGeneratorBase(field, opts),
        field_(field),
        opts_(&opts),
        weak_(IsImplicitWeakField(field_, *opts_, scc)),
        has_required_(scc->HasRequiredFields(field_->message_type())),
        is_oneof_(field->real_containing_oneof() != nullptr) {}

  ~SingularMessage() override = default;

  std::vector<Sub> MakeVars() const override {
    return Vars(field_, *opts_, weak_);
  }

  void GeneratePrivateMembers(io::Printer* p) const override {
    if (weak_) {
      p->Emit(R"cc(
        $pb$::MessageLite* $name$_;
      )cc");
      return;
    }

    p->Emit(R"cc(
      $Submsg$* $name$_;
    )cc");
  }

  void GenerateSwappingCode(io::Printer* p) const override {
    if (is_oneof_) {
      // Don't print any swapping code. Swapping the union will swap this field.
      return;
    }

    p->Emit(R"cc(
      swap($field_$, other->$field_$);
    )cc");
  }

  void GenerateByteSize(io::Printer* p) const override {
    p->Emit(R"cc(
      total_size +=
          $kTagBytes$ + $pbi$::WireFormatLite::$DeclaredType$Size(*$field_$);
    )cc");
  }

  void GenerateConstexprAggregateInitializer(io::Printer* p) const override {
    p->Emit(R"cc(
      /*decltype($field_$)*/ nullptr
    )cc");
  }

  void GenerateCopyAggregateInitializer(io::Printer* p) const override {
    p->Emit(R"cc(
      decltype($field_$) { nullptr }
    )cc");
  }

  void GenerateAggregateInitializer(io::Printer* p) const override {
    if (ShouldSplit(field_, *opts_)) {
      p->Emit(R"cc(
        decltype(Impl_::Split::$name$_) { nullptr }
      )cc");
      return;
    }

    p->Emit(R"cc(
      decltype($field_$) { nullptr }
    )cc");
  }

  void GenerateConstructorCode(io::Printer* p) const override {}

  void GenerateAccessorDeclarations(io::Printer* p) const override;
  void GenerateInlineAccessorDefinitions(io::Printer* p) const override;
  void GenerateInternalAccessorDeclarations(io::Printer* p) const override;
  void GenerateInternalAccessorDefinitions(io::Printer* p) const override;
  void GenerateClearingCode(io::Printer* p) const override;
  void GenerateMessageClearingCode(io::Printer* p) const override;
  void GenerateMergingCode(io::Printer* p) const override;
  void GenerateDestructorCode(io::Printer* p) const override;
  void GenerateCopyConstructorCode(io::Printer* p) const override;
  void GenerateSerializeWithCachedSizesToArray(io::Printer* p) const override;
  void GenerateIsInitialized(io::Printer* p) const override;
  void GenerateNonInlineAccessorDefinitions(io::Printer* p) const override;

 private:
  void ReleaseImpl(io::Printer* p) const;
  void ArenaReleaseImpl(io::Printer* p) const;
  void ArenaSetAllocImpl(io::Printer* p) const;
  void GetMutImpl(io::Printer* p) const;

  const FieldDescriptor* field_;
  const Options* opts_;
  bool weak_;
  bool has_required_;
  bool is_oneof_;
};

void SingularMessage::GenerateAccessorDeclarations(io::Printer* p) const {
  auto vars = AnnotatedAccessors(
      field_, {"", "set_", "mutable_", "set_allocated_",
               "unsafe_arena_set_allocated_", "unsafe_arena_release_"});
  vars.push_back(Sub{
      "release_name",
      SafeFunctionName(field_->containing_type(), field_, "release_"),
  }
                     .AnnotatedAs(field_));
  auto v = p->WithVars(vars);

  p->Emit(R"cc(
    $DEPRECATED$ const $Submsg$& $name$() const;
    $DEPRECATED$ $Submsg$* $mutable_name$();
    $DEPRECATED$ void $set_allocated_name$($Submsg$* value);
    $DEPRECATED$ void $unsafe_arena_set_allocated_name$($Submsg$* value);
    PROTOBUF_NODISCARD $DEPRECATED$ $Submsg$* $release_name$();
    $DEPRECATED$ $Submsg$* $unsafe_arena_release_name$();

    private:
    const $Submsg$& _internal_$name$() const;
    $Submsg$* _internal_mutable_$name$();

    public:
  )cc");
}

void SingularMessage::ReleaseImpl(io::Printer* p) const {
  if (is_oneof_) {
    p->Emit(R"cc(
      if ($not_has_field$) return nullptr;

      clear_has_$oneof_name$();
      $Submsg$* value = $cast_field_$;
      if (GetArenaForAllocation() != nullptr) {
        value = $pbi$::DuplicateIfNonNull(value);
      }
      $field_$ = nullptr;
      return value;
    )cc");
    return;
  }

  p->Emit(R"cc(
    $clear_hasbit$;
    $Submsg$* value = $cast_field_$;
#ifdef PROTOBUF_FORCE_COPY_IN_RELEASE
    value = $pbi$::DuplicateIfNonNull(value);
    if (GetArenaForAllocation() == nullptr) {
      delete $callable_field_$;
    }
#else   // PROTOBUF_FORCE_COPY_IN_RELEASE
    if (GetArenaForAllocation() != nullptr) {
      value = $pbi$::DuplicateIfNonNull(value);
    }
#endif  // !PROTOBUF_FORCE_COPY_IN_RELEASE
    $field_$ = nullptr;
    return value;
  )cc");
}

void SingularMessage::ArenaReleaseImpl(io::Printer* p) const {
  if (is_oneof_) {
    p->Emit(R"cc(
      if ($not_has_field$) return nullptr;
      clear_has_$oneof_name$();
      $Submsg$* temp = $cast_field_$;
      $field_$ = nullptr;
      return temp;
    )cc");
    return;
  }

  p->Emit(R"cc(
    $clear_hasbit$;
    $Submsg$* temp = $cast_field_$;
    $field_$ = nullptr;
    return temp;
  )cc");
}

void SingularMessage::ArenaSetAllocImpl(io::Printer* p) const {
  if (is_oneof_) {
    // We rely on the oneof clear method to free the earlier contents of
    // this oneof. We can directly use the pointer we're given to set the
    // new value.
    p->Emit(R"cc(
      clear_$oneof_name$();
      if (value != nullptr) {
        set_has_$name$();
      }
      $field_$ = $cast_value$;
    )cc");
    return;
  }

  p->Emit(R"cc(
    //~ If we're not on an arena, free whatever we were holding
    //~ before; if we are on arena, we can just forget the old
    //~ pointer.
    if (GetArenaForAllocation() == nullptr) {
      delete $callable_field_$;
    }
    $field_$ = $cast_value$;
    $update_hasbit$;
  )cc");
}

void SingularMessage::GetMutImpl(io::Printer* p) const {
  if (is_oneof_) {
    p->Emit(R"cc(
      if ($not_has_field$) {
        clear_$oneof_name$();
        set_has_$name$();
        auto* value = CreateMaybeMessage<$Submsg$>(GetArenaForAllocation());
        $field_$ = $cast_value$;
      }
      return $cast_field_$;
    )cc");
    return;
  }

  p->Emit(R"cc(
    $set_hasbit$;
    if ($field_$ == nullptr) {
      auto* value = CreateMaybeMessage<$Submsg$>(GetArenaForAllocation());
      $field_$ = $cast_value$;
    }
    return $cast_field_$;
  )cc");
}

void SingularMessage::GenerateInlineAccessorDefinitions(io::Printer* p) const {
  auto v =
      p->WithVars({{"release_name", SafeFunctionName(field_->containing_type(),
                                                     field_, "release_")}});

  p->Emit(
      {
          {"is_set",
           [&] {
             if (is_oneof_) {
               p->Emit("$has_field$");
             } else {
               p->Emit("msg != nullptr");
             }
           }},
          {"update_hasbit",
           [&] {
             if (!HasHasbit(field_)) return;
             p->Emit(R"cc(
               if (value != nullptr) {
                 $set_hasbit$;
               } else {
                 $clear_hasbit$;
               }
             )cc");
           }},
          {"release", [&] { ReleaseImpl(p); }},
          {"arena_release", [&] { ArenaReleaseImpl(p); }},
          {"arena_set_alloc", [&] { ArenaSetAllocImpl(p); }},
          {"mutable", [&] { GetMutImpl(p); }},
      },
      R"cc(
        inline const $Submsg$& $Msg$::_internal_$name$() const {
          $StrongRef$;
          const auto* msg = $const_field_$;
          return $is_set$ ? *msg : reinterpret_cast<const $Submsg$&>($kDefault$);
        }
        inline const $Submsg$& $Msg$::$name$() const {
          $annotate_get$;
          // @@protoc_insertion_point(field_get:$pkg.Msg.field$)
          return _internal_$name$();
        }
        inline void $Msg$::unsafe_arena_set_allocated_$name$($Submsg$* value) {
          $PrepareSplitMessageForWrite$;
          $arena_set_alloc$;
          $annotate_set$;
          // @@protoc_insertion_point(field_unsafe_arena_set_allocated:$pkg.Msg.field$)
        }
        inline $Submsg$* $Msg$::$release_name$() {
          $StrongRef$;
          $annotate_release$;
          // @@protoc_insertion_point(field_release:$pkg.Msg.field$)
          $PrepareSplitMessageForWrite$;
          $release$;
        }
        inline $Submsg$* $Msg$::unsafe_arena_release_$name$() {
          $annotate_release$;
          // @@protoc_insertion_point(field_release:$pkg.Msg.field$)
          $StrongRef$;
          $PrepareSplitMessageForWrite$;
          $arena_release$;
        }
        inline $Submsg$* $Msg$::_internal_mutable_$name$() {
          $StrongRef$;
          $mutable$;
        }
        inline $Submsg$* $Msg$::mutable_$name$() {
          //~ TODO(b/122856539): add tests to make sure all write accessors
          //~ are able to prepare split message allocation.
          $PrepareSplitMessageForWrite$;
          $Submsg$* _msg = _internal_mutable_$name$();
          $annotate_mutable$;
          // @@protoc_insertion_point(field_mutable:$pkg.Msg.field$)
          return _msg;
        }
      )cc");

  if (is_oneof_) {
    // For oneofs, set_allocates_* is not inline.
    return;
  }

  p->Emit(R"cc(
    inline void $Msg$::set_allocated_$name$($Submsg$* value) {
      $PrepareSplitMessageForWrite$;
      auto* arena = GetArenaForAllocation();
      if (arena == nullptr) {
        //~ We handle the most common case inline, and delegate less
        //~ common cases to the slow fallback function.
        delete $callable_field_$;
      }

      if (value != nullptr) {
        //~ We have to read the arena through the virtual method,
        //~ because the type isn't defined in this file.
        auto* sub_arena = $pb$::Arena::InternalGetOwningArena($callable_value$);
        if (arena != sub_arena) {
          value = $pbi$::GetOwnedMessage(arena, value, sub_arena);
        }
        $set_hasbit$;
      } else {
        $clear_hasbit$;
      }
      $field_$ = $cast_value$;
      $annotate_set$;
      // @@protoc_insertion_point(field_set_allocated:$pkg.Msg.field$)
    }
  )cc");
}

void SingularMessage::GenerateNonInlineAccessorDefinitions(
    io::Printer* p) const {
  if (!is_oneof_) {
    return;
  }

  p->Emit(R"cc(
    void $Msg$::set_allocated_$name$($Submsg$* value) {
      auto* arena = GetArenaForAllocation();
      clear_$oneof_name$();

      if (value != nullptr) {
        //~ We have to read the arena through the virtual method,
        //~ because the type isn't defined in this file.
        auto* sub_arena = $pb$::Arena::InternalGetOwningArena($callable_value$);
        if (arena != sub_arena) {
          value = $pbi$::GetOwnedMessage(arena, value, sub_arena);
        }
        set_has_$name$();
        $field_$ = value;
      }
      $annotate_set$;
      // @@protoc_insertion_point(field_set_allocated:$pkg.Msg.field$)
    }
  )cc");
}

void SingularMessage::GenerateInternalAccessorDeclarations(
    io::Printer* p) const {
  if (!weak_) {
    p->Emit(R"cc(
      static const $Submsg$& $name$(const $Msg$* msg);
    )cc");
    return;
  }
  p->Emit(R"cc(
    static const $pb$::MessageLite& $name$(const $Msg$* msg);
    static $pb$::MessageLite* mutable_$name$($Msg$* msg);
  )cc");
}

void SingularMessage::GenerateInternalAccessorDefinitions(
    io::Printer* p) const {
  if (!weak_) {
    // This inline accessor directly returns member field and is used in
    // Serialize such that AFDO profile correctly captures access information to
    // message fields under serialize.
    p->Emit(R"cc(
      const $Submsg$& $Msg$::_Internal::$name$(const $Msg$* msg) {
        return *msg->$field_$;
      }
    )cc");
    return;
  }

  // In theory, these accessors could be inline in _Internal. However, in
  // practice, the linker is then not able to throw them out making implicit
  // weak dependencies not work at all.
  //
  // These private accessors are used by MergeFrom and
  // MergePartialFromCodedStream, and their purpose is to provide access to
  // the field without creating a strong dependency on the message type.
  p->Emit(
      {
          {"hasbits",
           [&] {
             if (!HasHasbit(field_)) return;
             p->Emit(R"cc(
               msg->$set_hasbit$;
             )cc");
           }},
          {"present",
           [&] {
             if (field_->real_containing_oneof() == nullptr) {
               p->Emit("$field_$ == nullptr");
             } else {
               p->Emit("$not_has_field$");
             }
           }},
          {"clear_oneof",
           [&] {
             if (field_->real_containing_oneof() == nullptr) return;
             p->Emit(R"cc(
               msg->clear_$oneof_name$();
               msg->set_has_$name$();
             )cc");
           }},
      },
      R"cc(
        const $pb$::MessageLite& $Msg$::_Internal::$name$(const $Msg$* msg) {
          if (msg->$field_$ == nullptr) {
            return $kDefaultPtr$;
          }
          return *msg->$field_$;
        }

        $pb$::MessageLite* $Msg$::_Internal::mutable_$name$($Msg$* msg) {
          $hasbits$;
          if (msg_->$present$) {
            $clear_oneof$;
            msg->$field_$ = $kDefaultPtr$->New(msg->GetArenaForAllocation());
          }
          return msg->$field_$;
        }
      )cc");
}
void SingularMessage::GenerateClearingCode(io::Printer* p) const {
  if (HasHasbit(field_)) {
    p->Emit(R"cc(
      if ($field_$ != nullptr) {
        $field_$->Clear();
      }
    )cc");
    return;
  }

  // If we don't have has-bits, message presence is indicated only by ptr !=
  // nullptr. Thus on clear, we need to delete the object.
  p->Emit(R"cc(
    if (GetArenaForAllocation() == nullptr && $field_$ != nullptr) {
      delete $callable_field_$;
    }
  )cc");
  if (!is_oneof_) {
    p->Emit(R"cc(
      $field_$ = nullptr;
    )cc");
  }
}

void SingularMessage::GenerateMessageClearingCode(io::Printer* p) const {
  if (!is_oneof_ && HasHasbit(field_)) {
    p->Emit(R"cc(
      $DCHK$($field_$ != nullptr);
      $field_$->Clear();
    )cc");
    return;
  }

  GenerateClearingCode(p);
}

void SingularMessage::GenerateMergingCode(io::Printer* p) const {
  if (weak_) {
    p->Emit(R"cc(
      _Internal::mutable_$name$(_this)->CheckTypeAndMergeFrom(
          _Internal::$name$(&from));
    )cc");
  } else {
    p->Emit(R"cc(
      _this->_internal_mutable_$name$()->$Submsg$::MergeFrom(
          from._internal_$name$());
    )cc");
  }
}

void SingularMessage::GenerateDestructorCode(io::Printer* p) const {
  if (is_oneof_) {
    // We inherit from SingularMessage, so we need to override the default
    // behavior.
    return;
  }

  if (options_.opensource_runtime) {
    // TODO(gerbens) Remove this when we don't need to destruct default
    // instances.  In google3 a default instance will never get deleted so we
    // don't need to worry about that but in opensource protobuf default
    // instances are deleted in shutdown process and we need to take special
    // care when handling them.
    p->Emit(R"cc(
      if (this != internal_default_instance()) )cc");
  }

  if (ShouldSplit(field_, *opts_)) {
    p->Emit(R"cc(
      delete $cached_split_ptr$->$name$_;
    )cc");
    return;
  }

  p->Emit(R"cc(
    delete $callable_field_$;
  )cc");
}

void SingularMessage::GenerateCopyConstructorCode(io::Printer* p) const {
  p->Emit({{"has",
            [&] {
              if (HasHasbit(field_)) {
                p->Emit("$has_hasbit$");
              } else {
                p->Emit("_internal_has_$name$()");
              }
            }}},
          R"cc(
            if (from.$has$) {
              _this->$field_$ = new $Submsg$(*from.$field_$);
            }
          )cc");
}

void SingularMessage::GenerateSerializeWithCachedSizesToArray(
    io::Printer* p) const {
  Formatter format(p, variables_);
  if (field_->type() == FieldDescriptor::TYPE_MESSAGE) {
    p->Emit(R"cc(
      target = $pbi$::WireFormatLite::InternalWrite$DeclaredType$(
          $number$, _Internal::$name$(this),
          _Internal::$name$(this).GetCachedSize(), target, stream);
    )cc");
    return;
  }
  p->Emit(R"cc(
    target = stream->EnsureSpace(target);
    target = $pbi$::WireFormatLite::InternalWrite$DeclaredType$(
        $number$, _Internal::$name$(this), target, stream);
  )cc");
}

void SingularMessage::GenerateIsInitialized(io::Printer* p) const {
  if (!has_required_) return;

  p->Emit({{"has",
            [&] {
              if (is_oneof_) {
                p->Emit("$has_field$");
              } else if (HasHasbit(field_)) {
                p->Emit("$has_hasbit$");
              } else {
                p->Emit("_internal_has_$name$()");
              }
            }}},
          R"cc(
            if ($has$ && !$field_$->IsInitialized()) {
              return false;
            }
          )cc");
}

class RepeatedMessage : public FieldGeneratorBase {
 public:
  RepeatedMessage(const FieldDescriptor* field, const Options& opts,
                  MessageSCCAnalyzer* scc)
      : FieldGeneratorBase(field, opts),
        field_(field),
        opts_(&opts),
        weak_(IsImplicitWeakField(field_, *opts_, scc)),
        has_required_(scc->HasRequiredFields(field_->message_type())) {}
  ~RepeatedMessage() override = default;

  std::vector<Sub> MakeVars() const override {
    return Vars(field_, *opts_, weak_);
  }

  void GeneratePrivateMembers(io::Printer* p) const override {
    p->Emit(R"cc(
      $pb$::$Weak$RepeatedPtrField<$Submsg$> $name$_;
    )cc");
  }

  void GenerateClearingCode(io::Printer* p) const override {
    p->Emit(R"cc(
      $field_$.Clear();
    )cc");
  }

  void GenerateMergingCode(io::Printer* p) const override {
    p->Emit(R"cc(
      _this->$field_$.MergeFrom(from.$field_$);
    )cc");
  }

  void GenerateSwappingCode(io::Printer* p) const override {
    p->Emit(R"cc(
      $field_$.InternalSwap(&other->$field_$);
    )cc");
  }

  void GenerateConstructorCode(io::Printer* p) const override {}

  void GenerateCopyConstructorCode(io::Printer* p) const override {}

  void GenerateDestructorCode(io::Printer* p) const override {
    p->Emit(
        R"cc(
          $field_$.~$Weak$RepeatedPtrField();
        )cc");
  }

  void GenerateSerializeWithCachedSizesToArray(io::Printer* p) const override {
    p->Emit({{"serialize_impl", [&] { SerializeImpl(p); }}}, R"cc(
      for (const auto& msg : $field_$) {
        $serialize_impl$;
      }
    )cc");
  }

  void GenerateByteSize(io::Printer* p) const override {
    p->Emit(R"cc(
      total_size += std::size_t{$kTagBytes$} * _internal_$name$_size();
      for (const auto& msg : $field_$) {
        total_size += $pbi$::WireFormatLite::$DeclaredType$Size(msg);
      }
    )cc");
  }

  void GenerateIsInitialized(io::Printer* p) const override {
    if (!has_required_) return;

    p->Emit(R"cc(
      if (!$pbi$::AllAreInitialized$Weak$($field_$$.weak$)) {
        return false;
      }
    )cc");
  }

  void GenerateAccessorDeclarations(io::Printer* p) const override;
  void GenerateInlineAccessorDefinitions(io::Printer* p) const override;

 private:
  void InternalGetImpl(io::Printer* p) const;
  void SerializeImpl(io::Printer* p) const;

  const FieldDescriptor* field_;
  const Options* opts_;
  bool weak_;
  bool has_required_;
};

void RepeatedMessage::GenerateAccessorDeclarations(io::Printer* p) const {
  auto v =
      p->WithVars(AnnotatedAccessors(field_, {"", "set_", "mutable_", "add_",
                                              "_internal_", "_internal_add_"}));
  p->Emit(R"cc(
    $DEPRECATED$ const $Submsg$& $name$(int index) const;
    $DEPRECATED$ $Submsg$* $mutable_name$(int index);
    $DEPRECATED$ $Submsg$* $add_name$();
    $DEPRECATED$ const $pb$::RepeatedPtrField<$Submsg$>& $name$() const;
    $DEPRECATED$ $pb$::RepeatedPtrField<$Submsg$>* $mutable_name$();

    private:
    const $Submsg$& $_internal_name$(int index) const;
    $Submsg$* $_internal_add_name$();

    public:
  )cc");
}

void RepeatedMessage::InternalGetImpl(io::Printer* p) const {
  if (opts_->safe_boundary_check) {
    p->Emit(R"cc(
      return $field_$$.weak$.InternalCheckedGet(
          index, reinterpret_cast<const $Submsg$&>($kDefault$));
    )cc");
    return;
  }
  p->Emit(R"cc(
    $StrongRef$;
    return $field_$$.weak$.Get(index);
  )cc");
}

void RepeatedMessage::GenerateInlineAccessorDefinitions(io::Printer* p) const {
  p->Emit({{"internal_get", [&] { InternalGetImpl(p); }}},
          R"cc(
            inline $Submsg$* $Msg$::mutable_$name$(int index) {
              $annotate_mutable$;
              // @@protoc_insertion_point(field_mutable:$pkg.Msg.field$)
              $StrongRef$;
              return $field_$$.weak$.Mutable(index);
            }
            inline $pb$::RepeatedPtrField<$Submsg$>* $Msg$::mutable_$name$() {
              $annotate_mutable_list$;
              // @@protoc_insertion_point(field_mutable_list:$pkg.Msg.field$)
              $StrongRef$;
              return &$field_$$.weak$;
            }
            inline const $Submsg$& $Msg$::_internal_$name$(int index) const {
              $internal_get$;
            }
            inline const $Submsg$& $Msg$::$name$(int index) const {
              $annotate_get$;
              // @@protoc_insertion_point(field_get:$pkg.Msg.field$)
              return _internal_$name$(index);
            }
            inline $Submsg$* $Msg$::_internal_add_$name$() { return $field_$$.weak$.Add(); }
            inline $Submsg$* $Msg$::add_$name$() {
              $Submsg$* _add = _internal_add_$name$();
              $annotate_add_mutable$;
              // @@protoc_insertion_point(field_add:$pkg.Msg.field$)
              return _add;
            }
            inline const $pb$::RepeatedPtrField<$Submsg$>& $Msg$::$name$() const {
              $annotate_list$;
              // @@protoc_insertion_point(field_list:$pkg.Msg.field$)
              $StrongRef$;
              return $field_$$.weak$;
            }
          )cc");
}

void RepeatedMessage::SerializeImpl(io::Printer* p) const {
  if (field_->type() != FieldDescriptor::TYPE_MESSAGE) {
    p->Emit(R"cc(
      target = stream->EnsureSpace(target);
      target = $pbi$::WireFormatLite::InternalWrite$DeclaredType$(
          $number$, msg, target, stream);
    )cc");
    return;
  }

  p->Emit(R"cc(
    target = $pbi$::WireFormatLite::InternalWrite$DeclaredType$(
        $number$, msg, msg.GetCachedSize(), target, stream);
  )cc");
}
}  // namespace

std::unique_ptr<FieldGeneratorBase> MakeSingularMessageGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc) {
  return absl::make_unique<SingularMessage>(desc, options, scc);
}

std::unique_ptr<FieldGeneratorBase> MakeRepeatedMessageGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc) {
  return absl::make_unique<RepeatedMessage>(desc, options, scc);
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
