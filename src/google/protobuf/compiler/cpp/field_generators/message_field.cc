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
#include <vector>

#include "absl/memory/memory.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/cpp/field.h"
#include "google/protobuf/compiler/cpp/field_generators/generators.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
namespace {
using ::google::protobuf::internal::cpp::HasHasbit;
using Sub = ::google::protobuf::io::Printer::Sub;
using ::google::protobuf::io::AnnotationCollector;

std::vector<Sub> Vars(const FieldDescriptor* field, const Options& opts,
                      bool weak) {
  bool split = ShouldSplit(field, opts);
  bool is_foreign = IsCrossFileMessage(field);
  std::string field_name = FieldMemberName(field, split);
  std::string qualified_type = FieldMessageTypeName(field, opts);
  std::string default_ref =
      QualifiedDefaultInstanceName(field->message_type(), opts);
  std::string default_ptr =
      QualifiedDefaultInstancePtr(field->message_type(), opts);
  absl::string_view base = "::google::protobuf::MessageLite";

  return {
      {"Submsg", qualified_type},
      {"MemberType", !weak ? qualified_type : base},
      {"CompleteType", !is_foreign ? qualified_type : base},
      {"kDefault", default_ref},
      {"kDefaultPtr", !weak
                          ? default_ptr
                          : absl::Substitute("reinterpret_cast<const $0*>($1)",
                                             base, default_ptr)},
      {"base_cast",
       absl::Substitute("reinterpret_cast<$0*>",
                        !is_foreign && !weak ? qualified_type : base)},
      Sub{"weak_cast",
          !weak ? "" : absl::Substitute("reinterpret_cast<$0*>", base)}
          .ConditionalFunctionCall(),
      Sub{"foreign_cast",
          !is_foreign ? "" : absl::Substitute("reinterpret_cast<$0*>", base)}
          .ConditionalFunctionCall(),
      {"cast_field_", !weak ? field_name
                            : absl::Substitute("reinterpret_cast<$0*>($1)",
                                               qualified_type, field_name)},
      {"Weak", weak ? "Weak" : ""},
      {".weak", weak ? ".weak" : ""},
      {"_weak", weak ? "_weak" : ""},
      Sub("StrongRef",
          !weak ? ""
                : absl::Substitute("::google::protobuf::internal::StrongReference("
                                   "reinterpret_cast<const $0&>($1));\n",
                                   qualified_type, default_ref))
          .WithSuffix(";"),
  };
}

class SingularMessage : public FieldGeneratorBase {
 public:
  SingularMessage(const FieldDescriptor* field, const Options& opts,
                  MessageSCCAnalyzer* scc)
      : FieldGeneratorBase(field, opts, scc),
        field_(field),
        opts_(&opts),
        has_required_(scc->HasRequiredFields(field->message_type())),
        has_hasbit_(HasHasbit(field)) {}

  ~SingularMessage() override = default;

  std::vector<Sub> MakeVars() const override {
    return Vars(field_, *opts_, is_weak());
  }

  void GeneratePrivateMembers(io::Printer* p) const override {
    p->Emit(R"cc(
      $MemberType$* $name$_;
    )cc");
  }

  void GenerateNonInlineAccessorDefinitions(io::Printer* p) const override {}

  void GenerateAccessorDeclarations(io::Printer* p) const override;
  void GenerateInlineAccessorDefinitions(io::Printer* p) const override;
  void GenerateInternalAccessorDeclarations(io::Printer* p) const override;
  void GenerateInternalAccessorDefinitions(io::Printer* p) const override;
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
    p->Emit("$name$_{CreateMaybeMessage<$Submsg$>(arena, *from.$name$_)}");
  }

  void GenerateOneofCopyConstruct(io::Printer* p) const override {
    p->Emit(R"cc(
      $field$ = CreateMaybeMessage<$Submsg$>(arena, *from.$field$);
    )cc");
  }

 private:
  friend class OneofMessage;

  const FieldDescriptor* field_;
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
    $DEPRECATED$ PROTOBUF_NODISCARD $Submsg$* $release_name$();
    $DEPRECATED$ $Submsg$* $mutable_name$();
    $DEPRECATED$ void $set_allocated_name$($Submsg$* value);
    $DEPRECATED$ void $unsafe_arena_set_allocated_name$($Submsg$* value);
    $DEPRECATED$ $Submsg$* $unsafe_arena_release_name$();

    private:
    const $Submsg$& _internal_$name$() const;
    $Submsg$* _internal_mutable_$name$();

    public:
  )cc");
}

void SingularMessage::GenerateInlineAccessorDefinitions(io::Printer* p) const {
  auto v =
      p->WithVars({{"release_name", SafeFunctionName(field_->containing_type(),
                                                     field_, "release_")}});
  p->Emit(
      {
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
      },
      R"cc(
        inline const $Submsg$& $Msg$::_internal_$name$() const {
          $TsanDetectConcurrentRead$;
          $StrongRef$;
          const $Submsg$* p = $cast_field_$;
          return p != nullptr ? *p : reinterpret_cast<const $Submsg$&>($kDefault$);
        }
        inline const $Submsg$& $Msg$::$name$() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
          $annotate_get$;
          // @@protoc_insertion_point(field_get:$pkg.Msg.field$)
          return _internal_$name$();
        }
        inline void $Msg$::unsafe_arena_set_allocated_$name$($Submsg$* value) {
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
        inline $Submsg$* $Msg$::$release_name$() {
          $TsanDetectConcurrentMutation$;
          $StrongRef$;
          $annotate_release$;
          $PrepareSplitMessageForWrite$;

          $clear_hasbit$;
          $Submsg$* released = $cast_field_$;
          $field_$ = nullptr;
#ifdef PROTOBUF_FORCE_COPY_IN_RELEASE
          auto* old = reinterpret_cast<$pb$::MessageLite*>(released);
          released = $pbi$::DuplicateIfNonNull(released);
          if (GetArena() == nullptr) {
            delete old;
          }
#else   // PROTOBUF_FORCE_COPY_IN_RELEASE
          if (GetArena() != nullptr) {
            released = $pbi$::DuplicateIfNonNull(released);
          }
#endif  // !PROTOBUF_FORCE_COPY_IN_RELEASE
          return released;
        }
        inline $Submsg$* $Msg$::unsafe_arena_release_$name$() {
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
        inline $Submsg$* $Msg$::_internal_mutable_$name$() {
          $TsanDetectConcurrentMutation$;
          $StrongRef$;
          $set_hasbit$;
          if ($field_$ == nullptr) {
            auto* p = CreateMaybeMessage<$Submsg$>(GetArena());
            $field_$ = reinterpret_cast<$MemberType$*>(p);
          }
          return $cast_field_$;
        }
        inline $Submsg$* $Msg$::mutable_$name$() ABSL_ATTRIBUTE_LIFETIME_BOUND {
          //~ TODO: add tests to make sure all write accessors are
          //~ able to prepare split message allocation.
          $PrepareSplitMessageForWrite$;
          $Submsg$* _msg = _internal_mutable_$name$();
          $annotate_mutable$;
          // @@protoc_insertion_point(field_mutable:$pkg.Msg.field$)
          return _msg;
        }
        //~ We handle the most common case inline, and delegate less common
        //~ cases to the slow fallback function.
        inline void $Msg$::set_allocated_$name$($Submsg$* value) {
          $pb$::Arena* message_arena = GetArena();
          $TsanDetectConcurrentMutation$;
          $PrepareSplitMessageForWrite$;
          if (message_arena == nullptr) {
            delete $base_cast$($field_$);
          }

          if (value != nullptr) {
            //~ When $Submsg$ is a cross-file type, have to read the arena
            //~ through the virtual method, because the type isn't defined in
            //~ this file, only forward-declated.
            $pb$::Arena* submessage_arena = $base_cast$(value)->GetArena();
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
      )cc");
}

void SingularMessage::GenerateInternalAccessorDeclarations(
    io::Printer* p) const {
  if (!is_weak()) {
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
  // In theory, these accessors could be inline in _Internal. However, in
  // practice, the linker is then not able to throw them out making implicit
  // weak dependencies not work at all.

  if (!is_weak()) {
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

  // These private accessors are used by MergeFrom and
  // MergePartialFromCodedStream, and their purpose is to provide access to
  // the field without creating a strong dependency on the message type.
  p->Emit(
      {
          {"update_hasbit",
           [&] {
             if (!has_hasbit_) return;
             p->Emit(R"cc(
               msg->$set_hasbit$;
             )cc");
           }},
          {"is_already_set",
           [&] {
             if (!is_oneof()) {
               p->Emit("msg->$field_$ == nullptr");
             } else {
               p->Emit("msg->$not_has_field$");
             }
           }},
          {"clear_oneof",
           [&] {
             if (!is_oneof()) return;
             p->Emit(R"cc(
               msg->clear_$oneof_name$();
               msg->set_has_$name$();
             )cc");
           }},
      },
      R"cc(
        const $pb$::MessageLite& $Msg$::_Internal::$name$(const $Msg$* msg) {
          if (msg->$field_$ != nullptr) {
            return *msg->$field_$;
          } else {
            return *$kDefaultPtr$;
          }
        }
        $pb$::MessageLite* $Msg$::_Internal::mutable_$name$($Msg$* msg) {
          $update_hasbit$;
          if ($is_already_set$) {
            $clear_oneof$;
            msg->$field_$ = $kDefaultPtr$->New(msg->GetArena());
          }
          return msg->$field_$;
        }
      )cc");
}

void SingularMessage::GenerateClearingCode(io::Printer* p) const {
  if (!has_hasbit_) {
    // If we don't have has-bits, message presence is indicated only by ptr !=
    // nullptr. Thus on clear, we need to delete the object.
    p->Emit(
        "if (GetArena() == nullptr && $field_$ != nullptr) {\n"
        "  delete $field_$;\n"
        "}\n"
        "$field_$ = nullptr;\n");
  } else {
    p->Emit("if ($field_$ != nullptr) $field_$->Clear();\n");
  }
}

void SingularMessage::GenerateMessageClearingCode(io::Printer* p) const {
  if (!has_hasbit_) {
    // If we don't have has-bits, message presence is indicated only by ptr !=
    // nullptr. Thus on clear, we need to delete the object.
    p->Emit(
        "if (GetArena() == nullptr && $field_$ != nullptr) {\n"
        "  delete $field_$;\n"
        "}\n"
        "$field_$ = nullptr;\n");
  } else {
    p->Emit(
        "$DCHK$($field_$ != nullptr);\n"
        "$field_$->Clear();\n");
  }
}

void SingularMessage::GenerateMergingCode(io::Printer* p) const {
  if (is_weak()) {
    p->Emit(
        "_Internal::mutable_$name$(_this)->CheckTypeAndMergeFrom(\n"
        "    _Internal::$name$(&from));\n");
  } else {
    p->Emit(
        "_this->_internal_mutable_$name$()->$Submsg$::MergeFrom(\n"
        "    from._internal_$name$());\n");
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
      delete $field_$;
    )cc");
  }
}

using internal::cpp::HasHasbit;

#ifndef PROTOBUF_EXPLICIT_CONSTRUCTORS

void SingularMessage::GenerateCopyConstructorCode(io::Printer* p) const {
  if (has_hasbit_) {
    p->Emit(R"cc(
      if ((from.$has_hasbit$) != 0) {
        _this->$field_$ = new $Submsg$(*from.$field_$);
      }
    )cc");
  } else {
    p->Emit(R"cc(
      if (from._internal_has_$name$()) {
        _this->$field_$ = new $Submsg$(*from.$field_$);
      }
    )cc");
  }
}

#else  // !PROTOBUF_EXPLICIT_CONSTRUCTORS

void SingularMessage::GenerateCopyConstructorCode(io::Printer* p) const {
  if (has_hasbit_) {
    p->Emit(R"cc(
      if ((from.$has_hasbit$) != 0) {
        _this->$field_$ = CreateMaybeMessage<$Submsg$>(arena, *from.$field_$);
      }
    )cc");
  } else {
    p->Emit(R"cc(
      if (from._internal_has_$name$()) {
        _this->$field_$ = CreateMaybeMessage<$Submsg$>(arena, *from.$field_$);
      }
    )cc");
  }
}

#endif  // !PROTOBUF_EXPLICIT_CONSTRUCTORS

void SingularMessage::GenerateSerializeWithCachedSizesToArray(
    io::Printer* p) const {
  if (!is_group()) {
    p->Emit(R"cc(
      target = $pbi$::WireFormatLite::InternalWrite$declared_type$(
          $number$, _Internal::$name$(this),
          _Internal::$name$(this).GetCachedSize(), target, stream);
    )cc");
  } else {
    p->Emit(R"cc(
      target = stream->EnsureSpace(target);
      target = $pbi$::WireFormatLite::InternalWrite$declared_type$(
          $number$, _Internal::$name$(this), target, stream);
    )cc");
  }
}

void SingularMessage::GenerateByteSize(io::Printer* p) const {
  p->Emit(R"cc(
    total_size +=
        $tag_size$ + $pbi$::WireFormatLite::$declared_type$Size(*$field_$);
  )cc");
}

void SingularMessage::GenerateIsInitialized(io::Printer* p) const {
  if (!has_required_) return;

  if (HasHasbit(field_)) {
    p->Emit(R"cc(
      if (($has_hasbit$) != 0) {
        if (!$field_$->IsInitialized()) return false;
      }
    )cc");
  } else {
    p->Emit(R"cc(
      if (_internal_has_$name$()) {
        if (!$field_$->IsInitialized()) return false;
      }
    )cc");
  }
}

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
      : SingularMessage(descriptor, options, scc_analyzer) {}

  ~OneofMessage() override = default;

  void GenerateInlineAccessorDefinitions(io::Printer* p) const override;
  void GenerateNonInlineAccessorDefinitions(io::Printer* p) const override;
  void GenerateClearingCode(io::Printer* p) const override;
  void GenerateMessageClearingCode(io::Printer* p) const override;
  void GenerateSwappingCode(io::Printer* p) const override;
  void GenerateDestructorCode(io::Printer* p) const override;
  void GenerateConstructorCode(io::Printer* p) const override;
  void GenerateIsInitialized(io::Printer* p) const override;
};

void OneofMessage::GenerateNonInlineAccessorDefinitions(io::Printer* p) const {
  p->Emit(R"cc(
    void $Msg$::set_allocated_$name$($Submsg$* $name$) {
      $pb$::Arena* message_arena = GetArena();
      clear_$oneof_name$();
      if ($name$) {
        $pb$::Arena* submessage_arena = $foreign_cast$($name$)->GetArena();
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
    inline $Submsg$* $Msg$::$release_name$() {
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
    inline const $Submsg$& $Msg$::_internal_$name$() const {
      $StrongRef$;
      return $has_field$ ? *$cast_field_$ : reinterpret_cast<$Submsg$&>($kDefault$);
    }
  )cc");
  p->Emit(R"cc(
    inline const $Submsg$& $Msg$::$name$() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
      $annotate_get$;
      // @@protoc_insertion_point(field_get:$pkg.Msg.field$)
      return _internal_$name$();
    }
  )cc");
  p->Emit(R"cc(
    inline $Submsg$* $Msg$::unsafe_arena_release_$name$() {
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
    inline void $Msg$::unsafe_arena_set_allocated_$name$($Submsg$* value) {
      // We rely on the oneof clear method to free the earlier contents
      // of this oneof. We can directly use the pointer we're given to
      // set the new value.
      clear_$oneof_name$();
      if (value) {
        set_has_$name$();
        $field_$ = $weak_cast$(value);
      }
      $annotate_set$;
      // @@protoc_insertion_point(field_unsafe_arena_set_allocated:$pkg.Msg.field$)
    }
  )cc");
  p->Emit(R"cc(
    inline $Submsg$* $Msg$::_internal_mutable_$name$() {
      $StrongRef$;
      if ($not_has_field$) {
        clear_$oneof_name$();
        set_has_$name$();
        $field_$ = $weak_cast$(CreateMaybeMessage<$Submsg$>(GetArena()));
      }
      return $cast_field_$;
    }
  )cc");
  p->Emit(R"cc(
    inline $Submsg$* $Msg$::mutable_$name$() ABSL_ATTRIBUTE_LIFETIME_BOUND {
      $Submsg$* _msg = _internal_mutable_$name$();
      $annotate_mutable$;
      // @@protoc_insertion_point(field_mutable:$pkg.Msg.field$)
      return _msg;
    }
  )cc");
}

void OneofMessage::GenerateClearingCode(io::Printer* p) const {
  p->Emit(R"cc(
    if (GetArena() == nullptr) {
      delete $field_$;
    })cc");
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

void OneofMessage::GenerateIsInitialized(io::Printer* p) const {
  if (!has_required_) return;

  p->Emit(R"cc(
    if ($has_field$ && !$field_$->IsInitialized()) return false;
  )cc");
}

class RepeatedMessage : public FieldGeneratorBase {
 public:
  RepeatedMessage(const FieldDescriptor* field, const Options& opts,
                  MessageSCCAnalyzer* scc)
      : FieldGeneratorBase(field, opts, scc),
        field_(field),
        opts_(&opts),
        has_required_(scc->HasRequiredFields(field->message_type())) {}

  ~RepeatedMessage() override = default;

  std::vector<Sub> MakeVars() const override {
    return Vars(field_, *opts_, is_weak());
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

 private:
  const FieldDescriptor* field_;
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
  Formatter format(p);
  format("$DEPRECATED$ $Submsg$* ${1$mutable_$name$$}$(int index);\n",
         std::make_tuple(field_, GeneratedCodeInfo::Annotation::ALIAS));
  format(
      "$DEPRECATED$ $pb$::RepeatedPtrField< $Submsg$ >*\n"
      "    ${1$mutable_$name$$}$();\n",
      std::make_tuple(field_, GeneratedCodeInfo::Annotation::ALIAS));
  format(
      "private:\n"
      "const $pb$::RepeatedPtrField<$Submsg$>& _internal_$name$() const;\n"
      "$pb$::RepeatedPtrField<$Submsg$>* _internal_mutable_$name$();\n");
  if (is_weak()) {
    format(
        "const $pb$::WeakRepeatedPtrField<$Submsg$>& _internal_weak_$name$() "
        "const;\n"
        "$pb$::WeakRepeatedPtrField<$Submsg$>* "
        "_internal_mutable_weak_$name$();\n");
  }
  format(
      "public:\n"
      "$DEPRECATED$ const $Submsg$& ${1$$name$$}$(int index) const;\n",
      field_);
  format("$DEPRECATED$ $Submsg$* ${1$add_$name$$}$();\n",
         std::make_tuple(field_, GeneratedCodeInfo::Annotation::SET));
  format(
      "$DEPRECATED$ const $pb$::RepeatedPtrField< $Submsg$ >&\n"
      "    ${1$$name$$}$() const;\n",
      field_);
}

void RepeatedMessage::GenerateInlineAccessorDefinitions(io::Printer* p) const {
  // TODO: move insertion points
  p->Emit(R"cc(
    inline $Submsg$* $Msg$::mutable_$name$(int index)
        ABSL_ATTRIBUTE_LIFETIME_BOUND {
      $annotate_mutable$;
      // @@protoc_insertion_point(field_mutable:$pkg.Msg.field$)
      $StrongRef$;
      return _internal_mutable_$name$()->Mutable(index);
    }
  )cc");
  p->Emit(R"cc(
    inline $pb$::RepeatedPtrField<$Submsg$>* $Msg$::mutable_$name$()
        ABSL_ATTRIBUTE_LIFETIME_BOUND {
      $annotate_mutable_list$;
      // @@protoc_insertion_point(field_mutable_list:$pkg.Msg.field$)
      $StrongRef$;
      $TsanDetectConcurrentMutation$;
      return _internal_mutable_$name$();
    }
  )cc");
  p->Emit(
      {
          {"Get", opts_->safe_boundary_check ? "InternalCheckedGet" : "Get"},
          {"GetExtraArg",
           [&] {
             p->Emit(opts_->safe_boundary_check
                         ? ", reinterpret_cast<const $Submsg$&>($kDefault$)"
                         : "");
           }},
      },
      R"cc(
        inline const $Submsg$& $Msg$::$name$(int index) const
            ABSL_ATTRIBUTE_LIFETIME_BOUND {
          $annotate_get$;
          // @@protoc_insertion_point(field_get:$pkg.Msg.field$)
          $StrongRef$;
          return _internal_$name$().$Get$(index$GetExtraArg$);
        }
      )cc");
  p->Emit(R"cc(
    inline $Submsg$* $Msg$::add_$name$() ABSL_ATTRIBUTE_LIFETIME_BOUND {
      $TsanDetectConcurrentMutation$;
      $Submsg$* _add = _internal_mutable_$name$()->Add();
      $annotate_add_mutable$;
      // @@protoc_insertion_point(field_add:$pkg.Msg.field$)
      return _add;
    }
  )cc");
  p->Emit(R"cc(
    inline const $pb$::RepeatedPtrField<$Submsg$>& $Msg$::$name$() const
        ABSL_ATTRIBUTE_LIFETIME_BOUND {
      $annotate_list$;
      // @@protoc_insertion_point(field_list:$pkg.Msg.field$)
      $StrongRef$;
      return _internal_$name$();
    }
  )cc");

  if (should_split()) {
    p->Emit(R"cc(
      inline const $pb$::$Weak$RepeatedPtrField<$Submsg$>&
      $Msg$::_internal$_weak$_$name$() const {
        $TsanDetectConcurrentRead$;
        return *$field_$;
      }
      inline $pb$::$Weak$RepeatedPtrField<$Submsg$>*
      $Msg$::_internal_mutable$_weak$_$name$() {
        $TsanDetectConcurrentRead$;
        $PrepareSplitMessageForWrite$;
        if ($field_$.IsDefault()) {
          $field_$.Set(
              CreateMaybeMessage<$pb$::$Weak$RepeatedPtrField<$Submsg$>>(
                  GetArena()));
        }
        return $field_$.Get();
      }
    )cc");
  } else {
    p->Emit(R"cc(
      inline const $pb$::$Weak$RepeatedPtrField<$Submsg$>&
      $Msg$::_internal$_weak$_$name$() const {
        $TsanDetectConcurrentRead$;
        return $field_$;
      }
      inline $pb$::$Weak$RepeatedPtrField<$Submsg$>*
      $Msg$::_internal_mutable$_weak$_$name$() {
        $TsanDetectConcurrentRead$;
        return &$field_$;
      }
    )cc");
  }
  if (is_weak()) {
    p->Emit(R"cc(
      inline const $pb$::RepeatedPtrField<$Submsg$>& $Msg$::_internal_$name$()
          const {
        return _internal_weak_$name$().weak;
      }
      inline $pb$::RepeatedPtrField<$Submsg$>* $Msg$::_internal_mutable_$name$() {
        return &_internal_mutable_weak_$name$()->weak;
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
      $field_$.DeleteIfNotDefault();
    )cc");
  } else {
#ifndef PROTOBUF_EXPLICIT_CONSTRUCTORS
    p->Emit("$field_$.~$Weak$RepeatedPtrField();\n");
#endif  // !PROTOBUF_EXPLICIT_CONSTRUCTORS
  }
}

void RepeatedMessage::GenerateSerializeWithCachedSizesToArray(
    io::Printer* p) const {
  if (is_weak()) {
    p->Emit(
        "for (auto it = this->$field_$.pointer_begin(),\n"
        "          end = this->$field_$.pointer_end(); it < end; ++it) {\n");
    if (field_->type() == FieldDescriptor::TYPE_MESSAGE) {
      p->Emit(
          "  target = $pbi$::WireFormatLite::\n"
          "    InternalWrite$declared_type$($number$, "
          "**it, (**it).GetCachedSize(), target, stream);\n");
    } else {
      p->Emit(
          "  target = stream->EnsureSpace(target);\n"
          "  target = $pbi$::WireFormatLite::\n"
          "    InternalWrite$declared_type$($number$, **it, target, "
          "stream);\n");
    }
    p->Emit("}\n");
  } else {
    p->Emit(
        "for (unsigned i = 0,\n"
        "    n = static_cast<unsigned>(this->_internal_$name$_size());"
        " i < n; i++) {\n");
    if (field_->type() == FieldDescriptor::TYPE_MESSAGE) {
      p->Emit(
          "  const auto& repfield = this->_internal_$name$().Get(i);\n"
          "  target = $pbi$::WireFormatLite::\n"
          "      InternalWrite$declared_type$($number$, "
          "repfield, repfield.GetCachedSize(), target, stream);\n"
          "}\n");
    } else {
      p->Emit(
          "  target = stream->EnsureSpace(target);\n"
          "  target = $pbi$::WireFormatLite::\n"
          "    InternalWrite$declared_type$($number$, "
          "this->_internal_$name$().Get(i), target, stream);\n"
          "}\n");
    }
  }
}

void RepeatedMessage::GenerateByteSize(io::Printer* p) const {
  p->Emit(
      "total_size += $tag_size$UL * this->_internal_$name$_size();\n"
      "for (const auto& msg : this->_internal$_weak$_$name$()) {\n"
      "  total_size +=\n"
      "    $pbi$::WireFormatLite::$declared_type$Size(msg);\n"
      "}\n");
}

void RepeatedMessage::GenerateIsInitialized(io::Printer* p) const {
  if (!has_required_) return;

  if (is_weak()) {
    p->Emit(
        "if (!$pbi$::AllAreInitializedWeak($field_$.weak))\n"
        "  return false;\n");
  } else {
    p->Emit(
        "if (!$pbi$::AllAreInitialized(_internal_$name$()))\n"
        "  return false;\n");
  }
}
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
