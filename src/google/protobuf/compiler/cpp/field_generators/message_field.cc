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
#include <vector>

#include "absl/memory/memory.h"
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

std::vector<Sub> Vars(const FieldDescriptor* field, const Options& opts,
                      bool weak) {
  bool split = ShouldSplit(field, opts);
  std::string field_name = FieldMemberName(field, split);
  std::string type = FieldMessageTypeName(field, opts);
  std::string default_ref =
      QualifiedDefaultInstanceName(field->message_type(), opts);
  std::string default_ptr =
      QualifiedDefaultInstancePtr(field->message_type(), opts);

  return {
      {"Submsg", type},
      {"cast_field_",
       !weak ? field_name
             : absl::Substitute("reinterpret_cast<$0*>($1)", type, field_name)},
      {"kDefault", default_ref},
      {"kDefaultPtr",
       !weak ? default_ptr
             : absl::Substitute("reinterpret_cast<const "
                                "::PROTOBUF_NAMESPACE_ID::MessageLite*>($0)",
                                default_ptr)},
      {"Weak", weak ? "Weak" : ""},
      {".weak", weak ? ".weak" : ""},

      Sub("StrongRef",
          !weak ? ""
                : absl::Substitute(
                      "  ::PROTOBUF_NAMESPACE_ID::internal::StrongReference("
                      "reinterpret_cast<const $0&>($1));\n",
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
        weak_(IsImplicitWeakField(field, opts, scc)),
        has_required_(scc->HasRequiredFields(field->message_type())),
        has_hasbit_(HasHasbit(field)),
        is_oneof_(field_->real_containing_oneof() != nullptr),
        is_foreign_(IsCrossFileMessage(field)) {}

  ~SingularMessage() override = default;

  std::vector<Sub> MakeVars() const override {
    return Vars(field_, *opts_, weak_);
  }

  void GeneratePrivateMembers(io::Printer* p) const override;
  void GenerateAccessorDeclarations(io::Printer* p) const override;
  void GenerateInlineAccessorDefinitions(io::Printer* p) const override;
  void GenerateNonInlineAccessorDefinitions(io::Printer* p) const override;
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

 private:
  friend class OneofMessage;

  const FieldDescriptor* field_;
  const Options* opts_;
  bool weak_;
  bool has_required_;
  bool has_hasbit_;
  bool is_oneof_;
  bool is_foreign_;
};

void SingularMessage::GeneratePrivateMembers(io::Printer* p) const {
  if (weak_) {
    p->Emit("$pb$::MessageLite* $name$_;\n");
  } else {
    p->Emit("$Submsg$* $name$_;\n");
  }
}

void SingularMessage::GenerateAccessorDeclarations(io::Printer* p) const {
  auto v =
      p->WithVars({{"release_name", SafeFunctionName(field_->containing_type(),
                                                     field_, "release_")}});
  Formatter format(p);
  format(
      "$DEPRECATED$ const $Submsg$& ${1$$name$$}$() const;\n"
      "$DEPRECATED$ PROTOBUF_NODISCARD $Submsg$* "
      "${1$$release_name$$}$();\n",
      field_);
  format("$DEPRECATED$ $Submsg$* ${1$mutable_$name$$}$();\n",
         std::make_tuple(field_, GeneratedCodeInfo::Annotation::ALIAS));
  format(
      "$DEPRECATED$ void ${1$set_allocated_$name$$}$"
      "($Submsg$* $name$);\n"
      "private:\n"
      "const $Submsg$& ${1$_internal_$name$$}$() const;\n"
      "$Submsg$* ${1$_internal_mutable_$name$$}$();\n"
      "public:\n",
      field_);
  format(
      "$DEPRECATED$ void "
      "${1$unsafe_arena_set_allocated_$name$$}$(\n"
      "    $Submsg$* $name$);\n"
      "$DEPRECATED$ $Submsg$* ${1$unsafe_arena_release_$name$$}$();\n",
      field_);
}

void SingularMessage::GenerateNonInlineAccessorDefinitions(
    io::Printer* p) const {}

void SingularMessage::GenerateInlineAccessorDefinitions(io::Printer* p) const {
  auto v =
      p->WithVars({{"release_name", SafeFunctionName(field_->containing_type(),
                                                     field_, "release_")}});

  p->Emit(
      "inline const $Submsg$& $Msg$::_internal_$name$() const {\n"
      "$StrongRef$;"
      "  const $Submsg$* p = $cast_field_$;\n"
      "  return p != nullptr ? *p : reinterpret_cast<const $Submsg$&>(\n"
      "      $kDefault$);\n"
      "}\n"
      "inline const $Submsg$& $Msg$::$name$() const {\n"
      "$annotate_get$"
      "  // @@protoc_insertion_point(field_get:$pkg.Msg.field$)\n"
      "  return _internal_$name$();\n"
      "}\n");

  p->Emit(
      "inline void $Msg$::unsafe_arena_set_allocated_$name$(\n"
      "    $Submsg$* $name$) {\n"
      "$PrepareSplitMessageForWrite$"
      // If we're not on an arena, free whatever we were holding before.
      // (If we are on arena, we can just forget the earlier pointer.)
      "  if (GetArenaForAllocation() == nullptr) {\n"
      "    delete reinterpret_cast<$pb$::MessageLite*>($field_$);\n"
      "  }\n");
  if (weak_) {
    p->Emit("  $field_$ = reinterpret_cast<$pb$::MessageLite*>($name$);\n");
  } else {
    p->Emit("  $field_$ = $name$;\n");
  }
  if (has_hasbit_) {
    p->Emit(
        "  if ($name$) {\n"
        "    $set_hasbit$\n"
        "  } else {\n"
        "    $clear_hasbit$\n"
        "  }\n");
  }
  p->Emit(
      "$annotate_set$"
      "  // @@protoc_insertion_point(field_unsafe_arena_set_allocated"
      ":$pkg.Msg.field$)\n"
      "}\n");
  p->Emit(
      "inline $Submsg$* $Msg$::$release_name$() {\n"
      "$StrongRef$;"
      "$annotate_release$"
      "$PrepareSplitMessageForWrite$"
      "  $clear_hasbit$\n"
      "  $Submsg$* temp = $cast_field_$;\n"
      "  $field_$ = nullptr;\n"
      "#ifdef PROTOBUF_FORCE_COPY_IN_RELEASE\n"
      "  auto* old =  reinterpret_cast<$pb$::MessageLite*>(temp);\n"
      "  temp = $pbi$::DuplicateIfNonNull(temp);\n"
      "  if (GetArenaForAllocation() == nullptr) { delete old; }\n"
      "#else  // PROTOBUF_FORCE_COPY_IN_RELEASE\n"
      "  if (GetArenaForAllocation() != nullptr) {\n"
      "    temp = $pbi$::DuplicateIfNonNull(temp);\n"
      "  }\n"
      "#endif  // !PROTOBUF_FORCE_COPY_IN_RELEASE\n"
      "  return temp;\n"
      "}\n"
      "inline $Submsg$* $Msg$::unsafe_arena_release_$name$() {\n"
      "$annotate_release$"
      "  // @@protoc_insertion_point(field_release:$pkg.Msg.field$)\n"
      "$StrongRef$;"
      "$PrepareSplitMessageForWrite$"
      "  $clear_hasbit$\n"
      "  $Submsg$* temp = $cast_field_$;\n"
      "  $field_$ = nullptr;\n"
      "  return temp;\n"
      "}\n");

  p->Emit(
      "inline $Submsg$* $Msg$::_internal_mutable_$name$() {\n"
      "$StrongRef$;"
      "  $set_hasbit$\n"
      "  if ($field_$ == nullptr) {\n"
      "    auto* p = CreateMaybeMessage<$Submsg$>(GetArenaForAllocation());\n");
  if (weak_) {
    p->Emit("    $field_$ = reinterpret_cast<$pb$::MessageLite*>(p);\n");
  } else {
    p->Emit("    $field_$ = p;\n");
  }
  p->Emit(
      "  }\n"
      "  return $cast_field_$;\n"
      "}\n"
      "inline $Submsg$* $Msg$::mutable_$name$() {\n"
      // TODO(b/122856539): add tests to make sure all write accessors are able
      // to prepare split message allocation.
      "$PrepareSplitMessageForWrite$"
      "  $Submsg$* _msg = _internal_mutable_$name$();\n"
      "$annotate_mutable$"
      "  // @@protoc_insertion_point(field_mutable:$pkg.Msg.field$)\n"
      "  return _msg;\n"
      "}\n");

  // We handle the most common case inline, and delegate less common cases to
  // the slow fallback function.
  p->Emit(
      "inline void $Msg$::set_allocated_$name$($Submsg$* $name$) {\n"
      "  $pb$::Arena* message_arena = GetArenaForAllocation();\n");
  p->Emit(
      "$PrepareSplitMessageForWrite$"
      "  if (message_arena == nullptr) {\n");
  if (is_foreign_) {
    p->Emit(
        "    delete reinterpret_cast< "
        "$pb$::MessageLite*>($field_$);\n");
  } else {
    p->Emit("    delete $field_$;\n");
  }
  p->Emit(
      "  }\n"
      "  if ($name$) {\n");
  if (is_foreign_) {
    // We have to read the arena through the virtual method, because the type
    // isn't defined in this file.
    p->Emit(
        "    $pb$::Arena* submessage_arena =\n"
        "        $pb$::Arena::InternalGetOwningArena(\n"
        "                reinterpret_cast<$pb$::MessageLite*>("
        "$name$));\n");
  } else {
    p->Emit(
        "    $pb$::Arena* submessage_arena =\n"
        "        $pb$::Arena::InternalGetOwningArena("
        "$name$);\n");
  }
  p->Emit(
      "    if (message_arena != submessage_arena) {\n"
      "      $name$ = $pbi$::GetOwnedMessage(\n"
      "          message_arena, $name$, submessage_arena);\n"
      "    }\n"
      "    $set_hasbit$\n"
      "  } else {\n"
      "    $clear_hasbit$\n"
      "  }\n");
  if (weak_) {
    p->Emit("  $field_$ = reinterpret_cast<MessageLite*>($name$);\n");
  } else {
    p->Emit("  $field_$ = $name$;\n");
  }
  p->Emit(
      "$annotate_set$"
      "  // @@protoc_insertion_point(field_set_allocated:$pkg.Msg.field$)\n"
      "}\n");
}

void SingularMessage::GenerateInternalAccessorDeclarations(
    io::Printer* p) const {
  if (weak_) {
    p->Emit(
        "static const $pb$::MessageLite& $name$("
        "const $Msg$* msg);\n"
        "static $pb$::MessageLite* mutable_$name$("
        "$Msg$* msg);\n");
  } else {
    p->Emit("static const $Submsg$& $name$(const $Msg$* msg);\n");
  }
}

void SingularMessage::GenerateInternalAccessorDefinitions(
    io::Printer* p) const {
  // In theory, these accessors could be inline in _Internal. However, in
  // practice, the linker is then not able to throw them out making implicit
  // weak dependencies not work at all.
  if (weak_) {
    // These private accessors are used by MergeFrom and
    // MergePartialFromCodedStream, and their purpose is to provide access to
    // the field without creating a strong dependency on the message type.
    p->Emit(
        "const $pb$::MessageLite& $Msg$::_Internal::$name$(\n"
        "    const $Msg$* msg) {\n"
        "  if (msg->$field_$ != nullptr) {\n"
        "    return *msg->$field_$;\n"
        "  } else {\n"
        "    return *$kDefaultPtr$;\n"
        "  }\n"
        "}\n");
    p->Emit(
        "$pb$::MessageLite*\n"
        "$Msg$::_Internal::mutable_$name$($Msg$* msg) {\n");
    if (has_hasbit_) {
      p->Emit("  msg->$set_hasbit$\n");
    }
    if (!is_oneof_) {
      p->Emit("  if (msg->$field_$ == nullptr) {\n");
    } else {
      p->Emit(
          "  if (msg->$not_has_field$) {\n"
          "    msg->clear_$oneof_name$();\n"
          "    msg->set_has_$name$();\n");
    }
    p->Emit(
        "    msg->$field_$ = $kDefaultPtr$->New(\n"
        "        msg->GetArenaForAllocation());\n"
        "  }\n"
        "  return msg->$field_$;\n"
        "}\n");
  } else {
    // This inline accessor directly returns member field and is used in
    // Serialize such that AFDO profile correctly captures access information to
    // message fields under serialize.
    p->Emit(
        "const $Submsg$&\n"
        "$Msg$::_Internal::$name$(const $Msg$* msg) {\n"
        "  return *msg->$field_$;\n"
        "}\n");
  }
}

void SingularMessage::GenerateClearingCode(io::Printer* p) const {
  if (!has_hasbit_) {
    // If we don't have has-bits, message presence is indicated only by ptr !=
    // nullptr. Thus on clear, we need to delete the object.
    p->Emit(
        "if (GetArenaForAllocation() == nullptr && $field_$ != nullptr) {\n"
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
        "if (GetArenaForAllocation() == nullptr && $field_$ != nullptr) {\n"
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
  if (weak_) {
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
  if (opts_->opensource_runtime) {
    // TODO(gerbens) Remove this when we don't need to destruct default
    // instances.  In google3 a default instance will never get deleted so we
    // don't need to worry about that but in opensource protobuf default
    // instances are deleted in shutdown process and we need to take special
    // care when handling them.
    p->Emit("if (this != internal_default_instance()) ");
  }
  if (ShouldSplit(field_, *opts_)) {
    p->Emit("delete $cached_split_ptr$->$name$_;\n");
    return;
  }
  p->Emit("delete $field_$;\n");
}

using internal::cpp::HasHasbit;

void SingularMessage::GenerateCopyConstructorCode(io::Printer* p) const {
  if (has_hasbit_) {
    p->Emit(
        "if ((from.$has_hasbit$) != 0) {\n"
        "  _this->$field_$ = new $Submsg$(*from.$field_$);\n"
        "}\n");
  } else {
    p->Emit(
        "if (from._internal_has_$name$()) {\n"
        "  _this->$field_$ = new $Submsg$(*from.$field_$);\n"
        "}\n");
  }
}

void SingularMessage::GenerateSerializeWithCachedSizesToArray(
    io::Printer* p) const {
  if (field_->type() == FieldDescriptor::TYPE_MESSAGE) {
    p->Emit(
        "target = $pbi$::WireFormatLite::\n"
        "  InternalWrite$declared_type$($number$, _Internal::$name$(this),\n"
        "    _Internal::$name$(this).GetCachedSize(), target, stream);\n");
  } else {
    p->Emit(
        "target = stream->EnsureSpace(target);\n"
        "target = $pbi$::WireFormatLite::\n"
        "  InternalWrite$declared_type$(\n"
        "    $number$, _Internal::$name$(this), target, stream);\n");
  }
}

void SingularMessage::GenerateByteSize(io::Printer* p) const {
  p->Emit(
      "total_size += $tag_size$ +\n"
      "  $pbi$::WireFormatLite::$declared_type$Size(\n"
      "    *$field_$);\n");
}

void SingularMessage::GenerateIsInitialized(io::Printer* p) const {
  if (!has_required_) return;

  if (HasHasbit(field_)) {
    p->Emit(
        "if (($has_hasbit$) != 0) {\n"
        "  if (!$field_$->IsInitialized()) return false;\n"
        "}\n");
  } else {
    p->Emit(
        "if (_internal_has_$name$()) {\n"
        "  if (!$field_$->IsInitialized()) return false;\n"
        "}\n");
  }
}

void SingularMessage::GenerateConstexprAggregateInitializer(
    io::Printer* p) const {
  p->Emit("/*decltype($field_$)*/nullptr");
}

void SingularMessage::GenerateCopyAggregateInitializer(io::Printer* p) const {
  p->Emit("decltype($field_$){nullptr}");
}

void SingularMessage::GenerateAggregateInitializer(io::Printer* p) const {
  if (ShouldSplit(field_, *opts_)) {
    p->Emit("decltype(Impl_::Split::$name$_){nullptr}");
    return;
  }
  p->Emit("decltype($field_$){nullptr}");
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
  p->Emit(
      "void $Msg$::set_allocated_$name$($Submsg$* $name$) {\n"
      "  $pb$::Arena* message_arena = GetArenaForAllocation();\n"
      "  clear_$oneof_name$();\n"
      "  if ($name$) {\n");
  if (field_->file() != field_->message_type()->file()) {
    // We have to read the arena through the virtual method, because the type
    // isn't defined in this file.
    p->Emit(
        "    $pb$::Arena* submessage_arena =\n"
        "        $pb$::Arena::InternalGetOwningArena(\n"
        "                reinterpret_cast<$pb$::MessageLite*>("
        "$name$));\n");
  } else {
    p->Emit(
        "    $pb$::Arena* submessage_arena =\n"
        "      $pb$::Arena::InternalGetOwningArena($name$);\n");
  }
  p->Emit(
      "    if (message_arena != submessage_arena) {\n"
      "      $name$ = $pbi$::GetOwnedMessage(\n"
      "          message_arena, $name$, submessage_arena);\n"
      "    }\n"
      "    set_has_$name$();\n"
      "    $field_$ = $name$;\n"
      "  }\n"
      "$annotate_set$"
      "  // @@protoc_insertion_point(field_set_allocated:$pkg.Msg.field$)\n"
      "}\n");
}

void OneofMessage::GenerateInlineAccessorDefinitions(io::Printer* p) const {
  auto v =
      p->WithVars({{"release_name", SafeFunctionName(field_->containing_type(),
                                                     field_, "release_")}});

  p->Emit(
      "inline $Submsg$* $Msg$::$release_name$() {\n"
      "$annotate_release$"
      "  // @@protoc_insertion_point(field_release:$pkg.Msg.field$)\n"
      "$StrongRef$;"
      "  if ($has_field$) {\n"
      "    clear_has_$oneof_name$();\n"
      "    $Submsg$* temp = $cast_field_$;\n"
      "    if (GetArenaForAllocation() != nullptr) {\n"
      "      temp = $pbi$::DuplicateIfNonNull(temp);\n"
      "    }\n"
      "    $field_$ = nullptr;\n"
      "    return temp;\n"
      "  } else {\n"
      "    return nullptr;\n"
      "  }\n"
      "}\n");

  p->Emit(
      "inline const $Submsg$& $Msg$::_internal_$name$() const {\n"
      "$StrongRef$;"
      "  return $has_field$\n"
      "      ? *$cast_field_$\n"
      "      : reinterpret_cast<$Submsg$&>($kDefault$);\n"
      "}\n"
      "inline const $Submsg$& $Msg$::$name$() const {\n"
      "$annotate_get$"
      "  // @@protoc_insertion_point(field_get:$pkg.Msg.field$)\n"
      "  return _internal_$name$();\n"
      "}\n"
      "inline $Submsg$* $Msg$::unsafe_arena_release_$name$() {\n"
      "$annotate_release$"
      "  // @@protoc_insertion_point(field_unsafe_arena_release"
      ":$pkg.Msg.field$)\n"
      "$StrongRef$;"
      "  if ($has_field$) {\n"
      "    clear_has_$oneof_name$();\n"
      "    $Submsg$* temp = $cast_field_$;\n"
      "    $field_$ = nullptr;\n"
      "    return temp;\n"
      "  } else {\n"
      "    return nullptr;\n"
      "  }\n"
      "}\n"
      "inline void $Msg$::unsafe_arena_set_allocated_$name$"
      "($Submsg$* $name$) {\n"
      // We rely on the oneof clear method to free the earlier contents of
      // this oneof. We can directly use the pointer we're given to set the
      // new value.
      "  clear_$oneof_name$();\n"
      "  if ($name$) {\n"
      "    set_has_$name$();\n");
  if (weak_) {
    p->Emit(
        "    $field_$ = "
        "reinterpret_cast<$pb$::MessageLite*>($name$);\n");
  } else {
    p->Emit("    $field_$ = $name$;\n");
  }
  p->Emit(
      "  }\n"
      "$annotate_set$"
      "  // @@protoc_insertion_point(field_unsafe_arena_set_allocated:"
      "$pkg.Msg.field$)\n"
      "}\n"
      "inline $Submsg$* $Msg$::_internal_mutable_$name$() {\n"
      "$StrongRef$;"
      "  if ($not_has_field$) {\n"
      "    clear_$oneof_name$();\n"
      "    set_has_$name$();\n");
  if (weak_) {
    p->Emit(
        "    $field_$ = "
        "reinterpret_cast<$pb$::MessageLite*>(CreateMaybeMessage< "
        "$Submsg$ >(GetArenaForAllocation()));\n");
  } else {
    p->Emit(
        "    $field_$ = CreateMaybeMessage< $Submsg$ "
        ">(GetArenaForAllocation());\n");
  }
  p->Emit(
      "  }\n"
      "  return $cast_field_$;\n"
      "}\n"
      "inline $Submsg$* $Msg$::mutable_$name$() {\n"
      "  $Submsg$* _msg = _internal_mutable_$name$();\n"
      "$annotate_mutable$"
      "  // @@protoc_insertion_point(field_mutable:$pkg.Msg.field$)\n"
      "  return _msg;\n"
      "}\n");
}

void OneofMessage::GenerateClearingCode(io::Printer* p) const {
  p->Emit(
      "if (GetArenaForAllocation() == nullptr) {\n"
      "  delete $field_$;\n"
      "}\n");
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

  p->Emit(
      "if ($has_field$) {\n"
      "  if (!$field_$->IsInitialized()) return false;\n"
      "}\n");
}

class RepeatedMessage : public FieldGeneratorBase {
 public:
  RepeatedMessage(const FieldDescriptor* field, const Options& opts,
                  MessageSCCAnalyzer* scc)
      : FieldGeneratorBase(field, opts),
        field_(field),
        opts_(&opts),
        weak_(IsImplicitWeakField(field, opts, scc)),
        has_required_(scc->HasRequiredFields(field->message_type())) {}

  ~RepeatedMessage() override = default;

  std::vector<Sub> MakeVars() const override {
    return Vars(field_, *opts_, weak_);
  }

  void GeneratePrivateMembers(io::Printer* p) const override;
  void GenerateAccessorDeclarations(io::Printer* p) const override;
  void GenerateInlineAccessorDefinitions(io::Printer* p) const override;
  void GenerateClearingCode(io::Printer* p) const override;
  void GenerateMergingCode(io::Printer* p) const override;
  void GenerateSwappingCode(io::Printer* p) const override;
  void GenerateConstructorCode(io::Printer* p) const override;
  void GenerateCopyConstructorCode(io::Printer* p) const override {}
  void GenerateDestructorCode(io::Printer* p) const override;
  void GenerateSerializeWithCachedSizesToArray(io::Printer* p) const override;
  void GenerateByteSize(io::Printer* p) const override;
  void GenerateIsInitialized(io::Printer* p) const override;

 private:
  const FieldDescriptor* field_;
  const Options* opts_;
  bool weak_;
  bool has_required_;
};

void RepeatedMessage::GeneratePrivateMembers(io::Printer* p) const {
  if (weak_) {
    p->Emit("$pb$::WeakRepeatedPtrField< $Submsg$ > $name$_;\n");
  } else {
    p->Emit("$pb$::RepeatedPtrField< $Submsg$ > $name$_;\n");
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
      "const $Submsg$& ${1$_internal_$name$$}$(int index) const;\n"
      "$Submsg$* ${1$_internal_add_$name$$}$();\n"
      "const $pb$::RepeatedPtrField<$Submsg$>& _internal_$name$() const;\n"
      "$pb$::RepeatedPtrField<$Submsg$>* _internal_mutable_$name$();\n"
      "public:\n",
      field_);
  format("$DEPRECATED$ const $Submsg$& ${1$$name$$}$(int index) const;\n",
         field_);
  format("$DEPRECATED$ $Submsg$* ${1$add_$name$$}$();\n",
         std::make_tuple(field_, GeneratedCodeInfo::Annotation::SET));
  format(
      "$DEPRECATED$ const $pb$::RepeatedPtrField< $Submsg$ >&\n"
      "    ${1$$name$$}$() const;\n",
      field_);
}

void RepeatedMessage::GenerateInlineAccessorDefinitions(io::Printer* p) const {
  p->Emit(
      "inline $Submsg$* $Msg$::mutable_$name$(int index) {\n"
      "$annotate_mutable$"
      // TODO(dlj): move insertion points
      "  // @@protoc_insertion_point(field_mutable:$pkg.Msg.field$)\n"
      "$StrongRef$;"
      "  return _internal_mutable_$name$()->Mutable(index);\n"
      "}\n"
      "inline $pb$::RepeatedPtrField< $Submsg$ >*\n"
      "$Msg$::mutable_$name$() {\n"
      "$annotate_mutable_list$"
      "  // @@protoc_insertion_point(field_mutable_list:$pkg.Msg.field$)\n"
      "$StrongRef$;"
      "  return _internal_mutable_$name$();\n"
      "}\n");

  if (opts_->safe_boundary_check) {
    p->Emit(
        "inline const $Submsg$& $Msg$::_internal_$name$(int index) const "
        "{\n"
        "  return _internal_$name$().InternalCheckedGet(index,\n"
        "      reinterpret_cast<const $Submsg$&>($kDefault$));\n"
        "}\n");
  } else {
    p->Emit(
        "inline const $Submsg$& $Msg$::_internal_$name$(int index) const "
        "{\n"
        "$StrongRef$;"
        "  return _internal_$name$().Get(index);\n"
        "}\n");
  }

  p->Emit(
      "inline const $Submsg$& $Msg$::$name$(int index) const {\n"
      "$annotate_get$"
      "  // @@protoc_insertion_point(field_get:$pkg.Msg.field$)\n"
      "  return _internal_$name$(index);\n"
      "}\n"
      "inline $Submsg$* $Msg$::_internal_add_$name$() {\n"
      "  return _internal_mutable_$name$()->Add();\n"
      "}\n"
      "inline $Submsg$* $Msg$::add_$name$() {\n"
      "  $Submsg$* _add = _internal_add_$name$();\n"
      "$annotate_add_mutable$"
      "  // @@protoc_insertion_point(field_add:$pkg.Msg.field$)\n"
      "  return _add;\n"
      "}\n");

  p->Emit(
      "inline const $pb$::RepeatedPtrField< $Submsg$ >&\n"
      "$Msg$::$name$() const {\n"
      "$annotate_list$"
      "  // @@protoc_insertion_point(field_list:$pkg.Msg.field$)\n"
      "$StrongRef$;"
      "  return _internal_$name$();\n"
      "}\n");

  p->Emit(
      "inline const $pb$::RepeatedPtrField<$Submsg$>&\n"
      "$classname$::_internal_$name$() const {\n"
      "  return $field$$.weak$;\n"
      "}\n"
      "inline $pb$::RepeatedPtrField<$Submsg$>*\n"
      "$classname$::_internal_mutable_$name$() {\n"
      "  return &$field$$.weak$;\n"
      "}\n");
}

void RepeatedMessage::GenerateClearingCode(io::Printer* p) const {
  if (weak_) {
    p->Emit("$field_$.Clear();\n");
  } else {
    p->Emit("_internal_mutable_$name$()->Clear();\n");
  }
}

void RepeatedMessage::GenerateMergingCode(io::Printer* p) const {
  if (weak_) {
    p->Emit("_this->$field_$.MergeFrom(from.$field_$);\n");
  } else {
    p->Emit(
        "_this->_internal_mutable_$name$()->MergeFrom("
        "from._internal_$name$());\n");
  }
}

void RepeatedMessage::GenerateSwappingCode(io::Printer* p) const {
  if (weak_) {
    p->Emit("$field_$.InternalSwap(&other->$field_$);\n");
  } else {
    p->Emit(
        "_internal_mutable_$name$()->InternalSwap(other->_internal_mutable_"
        "$name$());\n");
  }
}

void RepeatedMessage::GenerateConstructorCode(io::Printer* p) const {
  // Not needed for repeated fields.
}

void RepeatedMessage::GenerateDestructorCode(io::Printer* p) const {
  if (weak_) {
    p->Emit("$field_$.~WeakRepeatedPtrField();\n");
  } else {
    p->Emit("_internal_mutable_$name$()->~RepeatedPtrField();\n");
  }
}

void RepeatedMessage::GenerateSerializeWithCachedSizesToArray(
    io::Printer* p) const {
  if (weak_) {
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
          "  const auto& repfield = this->_internal_$name$(i);\n"
          "  target = $pbi$::WireFormatLite::\n"
          "      InternalWrite$declared_type$($number$, "
          "repfield, repfield.GetCachedSize(), target, stream);\n"
          "}\n");
    } else {
      p->Emit(
          "  target = stream->EnsureSpace(target);\n"
          "  target = $pbi$::WireFormatLite::\n"
          "    InternalWrite$declared_type$($number$, "
          "this->_internal_$name$(i), target, stream);\n"
          "}\n");
    }
  }
}

void RepeatedMessage::GenerateByteSize(io::Printer* p) const {
  p->Emit("total_size += $tag_size$UL * this->_internal_$name$_size();\n");
  if (weak_) {
    p->Emit("for (const auto& msg : this->$field_$) {\n");
  } else {
    p->Emit("for (const auto& msg : this->_internal_$name$()) {\n");
  }
  p->Emit(
      "  total_size +=\n"
      "    $pbi$::WireFormatLite::$declared_type$Size(msg);\n"
      "}\n");
}

void RepeatedMessage::GenerateIsInitialized(io::Printer* p) const {
  if (!has_required_) return;

  if (weak_) {
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
